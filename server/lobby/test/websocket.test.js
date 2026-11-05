/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

import assert from 'node:assert/strict';
import {randomBytes} from 'node:crypto';
import {once} from 'node:events';
import {createServer} from 'node:http';
import {connect} from 'node:net';
import {test} from 'node:test';

import {CloseCode} from '../protocol.js';
import {acceptWebSocket, computeAcceptKey} from '../websocket.js';

async function startEchoServer( maxMessageSize )
{
    const sockets = new Set();
    const server = createServer();
    server.on( 'connection', ( socket ) => {
        sockets.add( socket );
        socket.on( 'close', () => sockets.delete( socket ) );
    } );
    server.on( 'upgrade', ( request, socket, head ) => {
        const webSocket = acceptWebSocket( request, socket, head, { maxMessageSize } );
        webSocket?.on( 'message', ( bytes ) => webSocket.send( bytes ) );
    } );
    server.listen( 0, '127.0.0.1' );
    await once( server, 'listening' );

    return {
        port : server.address().port,
        close : async () => {
            sockets.forEach( ( socket ) => socket.destroy() );
            server.close();
            await once( server, 'close' );
        },
    };
}

// A client which writes frames by hand, to send what a correct client never sends.
async function openRawClient( port, request )
{
    const socket = connect( port, '127.0.0.1' );
    await once( socket, 'connect' );

    const key = randomBytes( 16 ).toString( 'base64' );
    socket.write(
        request ?? `GET / HTTP/1.1\r\nHost: test\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: ${key}\r\nSec-WebSocket-Version: 13\r\n\r\n`,
    );

    let buffer = Buffer.alloc( 0 );
    let isEnded = false;
    const waiters = [];
    const wake = () => waiters.splice( 0 ).forEach( ( resolve ) => resolve() );
    socket.on( 'data', ( data ) => {
        buffer = Buffer.concat( [ buffer, data ] );
        wake();
    } );
    socket.on( 'close', () => {
        isEnded = true;
        wake();
    } );

    const waitFor = async ( isReady ) => {
        while ( !isReady() ) {
            if ( isEnded ) {
                return false;
            }
            await new Promise( ( resolve ) => waiters.push( resolve ) );
        }
        return true;
    };

    await waitFor( () => buffer.includes( '\r\n\r\n' ) );
    const end = buffer.indexOf( '\r\n\r\n' ) + 4;
    const response = buffer.subarray( 0, end ).toString();
    buffer = buffer.subarray( end );

    return {
        socket,
        response,
        key,
        send( opcode, payload, { isFinal = true, isMasked = true, length = payload.length } = {} ) {
            const header = [ ( isFinal ? 0x80 : 0 ) | opcode ];
            if ( length < 126 ) {
                header.push( ( isMasked ? 0x80 : 0 ) | length );
            }
            else {
                header.push( ( isMasked ? 0x80 : 0 ) | 126, length >> 8, length & 0xff );
            }
            const mask = isMasked ? [...randomBytes( 4 ) ] : [];
            const body = Buffer.from( payload ).map( ( byte, i ) => ( isMasked ? byte ^ mask[i & 3] : byte ) );
            socket.write( Buffer.from( [...header, ...mask, ...body ] ) );
        },
        // The next frame of the server, or null if the connection was closed.
        async frame() {
            const isReady = () => {
                if ( buffer.length < 2 ) {
                    return false;
                }
                const length = buffer[1] & 0x7f;
                const offset = length === 126 ? 4 : length === 127 ? 10 : 2;
                return buffer.length >= offset && buffer.length >= offset + ( length === 126 ? buffer.readUInt16BE( 2 ) : length );
            };
            if ( !( await waitFor( isReady ) ) ) {
                return null;
            }
            const opcode = buffer[0] & 0x0f;
            let length = buffer[1] & 0x7f;
            let offset = 2;
            if ( length === 126 ) {
                length = buffer.readUInt16BE( 2 );
                offset = 4;
            }
            const payload = buffer.subarray( offset, offset + length );
            buffer = buffer.subarray( offset + length );
            return { opcode, payload };
        },
        async closed() {
            await waitFor( () => false );
        },
    };
}

const closeCodeOf = ( frame ) => ( frame && frame.opcode === 8 ? frame.payload.readUInt16BE( 0 ) : null );

test( 'binary messages of every size go both ways with the WebSocket client of Node.js', async () => {
    const server = await startEchoServer( 2 * 1024 * 1024 );
    try {
        const webSocket = new WebSocket( `ws://127.0.0.1:${server.port}/` );
        webSocket.binaryType = 'arraybuffer';
        await once( webSocket, 'open' );

        // Sizes written on 7 bits, 16 bits and 64 bits.
        const messages =
            [ new Uint8Array( [ 1, 2, 3 ] ), new Uint8Array( 200 ).fill( 7 ), Uint8Array.from( { length : 70_000 }, ( _, i ) => i & 0xff ), randomBytes( 1024 * 1024 ) ];

        const received = [];
        const allReceived = new Promise( ( resolve ) => {
            webSocket.addEventListener( 'message', ( event ) => {
                received.push( new Uint8Array( event.data ) );
                if ( received.length === messages.length ) {
                    resolve();
                }
            } );
        } );
        messages.forEach( ( message ) => webSocket.send( message ) );
        await allReceived;

        messages.forEach( ( message, i ) => assert.deepEqual( received[i], new Uint8Array( message ), `message ${i}` ) );

        webSocket.close( 1000 );
        const closeEvent = await once( webSocket, 'close' );
        assert.equal( closeEvent[0].code, 1000 );
    }
    finally {
        await server.close();
    }
} );

test( 'the handshake gives the accept key of the RFC', () => {
    // Example of RFC 6455, section 1.3.
    assert.equal( computeAcceptKey( 'dGhlIHNhbXBsZSBub25jZQ==' ), 's3pPLMBiTxaQ9kYGzzhZRbK+xOo=' );
} );

test( 'wrong upgrade requests are refused', async () => {
    const server = await startEchoServer( 1024 );
    try {
        for (
            const request of
                ['GET / HTTP/1.1\r\nHost: test\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\n\r\n',
                 'GET / HTTP/1.1\r\nHost: test\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: abc\r\nSec-WebSocket-Version: 13\r\n\r\n',
                 'GET / HTTP/1.1\r\nHost: test\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 8\r\n\r\n',
                 'POST / HTTP/1.1\r\nHost: test\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n',
        ] ) {
            const client = await openRawClient( server.port, request );
            assert.match( client.response, /^HTTP\/1.1 400 / );
            await client.closed();
        }
    }
    finally {
        await server.close();
    }
} );

test( 'frames from a client which breaks the rules close the connection', async () => {
    const server = await startEchoServer( 1024 );
    try {
        const cases = [
            [ 'unmasked frame', ( client ) => client.send( 2, [ 1, 2, 3 ], { isMasked : false } ), CloseCode.PROTOCOL_ERROR ],
            [ 'text frame', ( client ) => client.send( 1, Buffer.from( 'hello' ) ), CloseCode.UNSUPPORTED_DATA ],
            [ 'continuation without a start', ( client ) => client.send( 0, [ 1 ] ), CloseCode.PROTOCOL_ERROR ],
            [ 'fragmented ping', ( client ) => client.send( 9, [ 1 ], { isFinal : false } ), CloseCode.PROTOCOL_ERROR ],
            [ 'unknown opcode', ( client ) => client.send( 3, [ 1 ] ), CloseCode.PROTOCOL_ERROR ],
            // Refused from its header alone, before its data comes.
            [ 'frame bigger than the limit', ( client ) => client.send( 2, [], { length : 2000 } ), CloseCode.TOO_BIG ],
            [
                'fragments bigger than the limit',
                ( client ) => {
                    client.send( 2, Buffer.alloc( 600 ), { isFinal : false } );
                    client.send( 0, Buffer.alloc( 600 ) );
                },
                CloseCode.TOO_BIG,
            ],
        ];

        for ( const [name, sendWrongFrame, expectedCode] of cases ) {
            const client = await openRawClient( server.port );
            assert.match( client.response, /^HTTP\/1.1 101 /, name );
            assert.match( client.response, new RegExp( `Sec-WebSocket-Accept: ${computeAcceptKey( client.key ).replace( /[+/]/g, '\\$&' )}` ), name );
            sendWrongFrame( client );
            assert.equal( closeCodeOf( await client.frame() ), expectedCode, name );
            client.socket.end();
        }
    }
    finally {
        await server.close();
    }
} );

test( 'a client which closes its side without a close frame is gone', async () => {
    const closed = [];
    const server = createServer();
    server.on( 'upgrade', ( request, socket, head ) => {
        const webSocket = acceptWebSocket( request, socket, head, { maxMessageSize : 1024 } );
        webSocket?.on( 'close', () => closed.push( Date.now() ) );
    } );
    server.listen( 0, '127.0.0.1' );
    await once( server, 'listening' );

    try {
        const client = await openRawClient( server.address().port );
        assert.match( client.response, /^HTTP\/1.1 101 / );

        const start = Date.now();
        client.socket.end();
        await new Promise( ( resolve ) => setTimeout( resolve, 300 ) );
        assert.equal( closed.length, 1 );
        assert.ok( closed[0] - start < 300 );
    }
    finally {
        server.closeAllConnections?.();
        server.close();
    }
} );

test( 'fragments, ping and close', async () => {
    const server = await startEchoServer( 1024 );
    try {
        const client = await openRawClient( server.port );

        client.send( 2, [ 1, 2 ], { isFinal : false } );
        client.send( 9, [ 42 ] );
        client.send( 0, [ 3, 4 ] );

        // The ping between two fragments is answered first.
        assert.deepEqual( await client.frame(), { opcode : 10, payload : Buffer.from( [ 42 ] ) } );
        assert.deepEqual( await client.frame(), { opcode : 2, payload : Buffer.from( [ 1, 2, 3, 4 ] ) } );

        client.send( 8, [ 0x03, 0xe8 ] );
        assert.equal( closeCodeOf( await client.frame() ), 1000 );
        await client.closed();
    }
    finally {
        await server.close();
    }
} );
