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

// Server side of WebSocket connections (RFC 6455) on a TCP socket of Node.js, with only what the lobby needs: binary
// messages, ping, pong and close. Nothing coming from a client is trusted: every frame is checked and its size limited
// before it is kept in memory.

import {createHash} from 'node:crypto';
import {EventEmitter} from 'node:events';

import {CloseCode} from './protocol.js';

const ACCEPT_GUID = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11';

// A connection which does not answer a close frame is cut after this time.
const CLOSE_TIMEOUT_MS = 2000;

// Above this amount of data waiting to leave, send() asks the caller to wait for 'drain'.
const MAX_BUFFERED_BYTES = 4 * 1024 * 1024;

const Opcode = Object.freeze( { CONTINUATION : 0, TEXT : 1, BINARY : 2, CLOSE : 8, PING : 9, PONG : 10 } );

export function rejectUpgrade( socket, status, reason )
{
    socket.end( `HTTP/1.1 ${status} ${reason}\r\nConnection: close\r\nContent-Length: 0\r\n\r\n` );
}

export function computeAcceptKey( key )
{
    return createHash( 'sha1' ).update( key + ACCEPT_GUID ).digest( 'base64' );
}

const hasToken = ( header, token ) => typeof header === 'string' && header.toLowerCase().split( ',' ).some( ( item ) => item.trim() === token );

// Answers an upgrade request of the HTTP server. Returns the connection, or null after refusing a request which is not a
// valid WebSocket request. Extensions and subprotocols asked by the client are ignored.
export function acceptWebSocket( request, socket, head, {maxMessageSize} )
{
    const key = request.headers['sec-websocket-key'];

    const isValid = request.method === 'GET' && hasToken( request.headers.upgrade, 'websocket' ) && hasToken( request.headers.connection, 'upgrade' )
                    && request.headers['sec-websocket-version'] === '13' && typeof key === 'string' && /^[A-Za-z0-9+/]{22}==$/.test( key );

    if ( !isValid ) {
        rejectUpgrade( socket, 400, 'Bad Request' );
        return null;
    }

    socket.write(
        'HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n' +
            `Sec-WebSocket-Accept: ${computeAcceptKey( key )}\r\n\r\n`,
    );

    return new WebSocketConnection( socket, head, maxMessageSize );
}

// Events: 'message' (Uint8Array, one whole binary message), 'ping', 'pong', 'drain' (the data sent so far left), 'close'.
export class WebSocketConnection extends EventEmitter
{
    #socket;
    #maxMessageSize;
    #buffer = Buffer.alloc( 0 );
    #fragments = [];
    #fragmentsSize = 0;
    #isClosed = false;
    #isCloseSent = false;
    #closeTimer = null;

    constructor( socket, head, maxMessageSize )
    {
        super();
        this.#socket = socket;
        this.#maxMessageSize = maxMessageSize;
        this.lastActivity = Date.now();

        socket.setNoDelay( true );
        socket.on( 'data', ( data ) => this.#onData( data ) );
        // The HTTP server keeps sockets half open: a client which closes its side without a close frame would never be
        // seen as gone.
        socket.on( 'end', () => socket.end() );
        socket.on( 'drain', () => this.emit( 'drain' ) );
        socket.on( 'close', () => this.#onSocketClose() );
        // A 'close' event always follows.
        socket.on( 'error', () => {} );

        if ( head && head.length > 0 ) {
            this.#onData( head );
        }
    }

    get isOpen()
    {
        return !this.#isClosed && !this.#isCloseSent;
    }

    // Bytes given to send() but not sent yet.
    get bufferedAmount()
    {
        return this.#socket.writableLength;
    }

    // Returns false when a lot of data waits to leave: wait for 'drain' before sending more.
    send( bytes )
    {
        if ( !this.isOpen ) {
            return false;
        }
        this.#sendFrame( Opcode.BINARY, bytes );
        return this.#socket.writableLength <= MAX_BUFFERED_BYTES;
    }

    ping( bytes = new Uint8Array( 0 ) )
    {
        if ( this.isOpen ) {
            this.#sendFrame( Opcode.PING, bytes );
        }
    }

    close( code = CloseCode.NORMAL )
    {
        if ( this.#isClosed || this.#isCloseSent ) {
            return;
        }

        this.#isCloseSent = true;
        const payload = Buffer.alloc( 2 );
        payload.writeUInt16BE( code );
        this.#sendFrame( Opcode.CLOSE, payload );

        this.#closeTimer = setTimeout( () => this.#socket.destroy(), CLOSE_TIMEOUT_MS );
        this.#closeTimer.unref?.();
    }

    // Stops reading, for example while the other side of a relay is too slow.
    pause()
    {
        this.#socket.pause();
    }

    resume()
    {
        this.#socket.resume();
    }

    destroy()
    {
        this.#socket.destroy();
    }

    #sendFrame( opcode, payload )
    {
        const length = payload.length;
        let header;

        if ( length < 126 ) {
            header = Buffer.from( [ 0x80 | opcode, length ] );
        }
        else if ( length < 0x10000 ) {
            header = Buffer.alloc( 4 );
            header[0] = 0x80 | opcode;
            header[1] = 126;
            header.writeUInt16BE( length, 2 );
        }
        else {
            header = Buffer.alloc( 10 );
            header[0] = 0x80 | opcode;
            header[1] = 127;
            header.writeUInt32BE( Math.floor( length / 0x100000000 ), 2 );
            header.writeUInt32BE( length >>> 0, 6 );
        }

        this.#socket.write( header );
        return this.#socket.write( payload );
    }

    #fail( code )
    {
        this.close( code );
        // Nothing more is read from a client which broke the protocol.
        this.#buffer = Buffer.alloc( 0 );
        this.#socket.pause();
    }

    #onData( data )
    {
        if ( this.#isClosed ) {
            return;
        }

        this.#buffer = this.#buffer.length === 0 ? data : Buffer.concat( [ this.#buffer, data ] );
        this.lastActivity = Date.now();

        while ( !this.#isClosed && this.#buffer.length >= 2 ) {
            const first = this.#buffer[0];
            const second = this.#buffer[1];
            const isFinal = ( first & 0x80 ) !== 0;
            const opcode = first & 0x0f;
            let length = second & 0x7f;
            let offset = 2;

            // No extension was agreed, and clients must mask their frames.
            if ( ( first & 0x70 ) !== 0 || ( second & 0x80 ) === 0 ) {
                this.#fail( CloseCode.PROTOCOL_ERROR );
                return;
            }

            if ( length === 126 ) {
                if ( this.#buffer.length < 4 ) {
                    return;
                }
                length = this.#buffer.readUInt16BE( 2 );
                offset = 4;
            }
            else if ( length === 127 ) {
                if ( this.#buffer.length < 10 ) {
                    return;
                }
                if ( this.#buffer.readUInt32BE( 2 ) !== 0 ) {
                    this.#fail( CloseCode.TOO_BIG );
                    return;
                }
                length = this.#buffer.readUInt32BE( 6 );
                offset = 10;
            }

            const isControl = opcode >= 8;
            if ( isControl && ( !isFinal || length > 125 ) ) {
                this.#fail( CloseCode.PROTOCOL_ERROR );
                return;
            }

            // Checked before waiting for the payload, so that a client cannot make the lobby keep a huge frame.
            if ( !isControl && this.#fragmentsSize + length > this.#maxMessageSize ) {
                this.#fail( CloseCode.TOO_BIG );
                return;
            }

            if ( this.#buffer.length < offset + 4 + length ) {
                return;
            }

            const mask = this.#buffer.subarray( offset, offset + 4 );
            const payload = Buffer.allocUnsafe( length );
            for ( let i = 0; i < length; ++i ) {
                payload[i] = this.#buffer[offset + 4 + i] ^ mask[i & 3];
            }
            this.#buffer = this.#buffer.subarray( offset + 4 + length );

            this.#onFrame( opcode, isFinal, payload );
        }
    }

    #onFrame( opcode, isFinal, payload )
    {
        switch ( opcode ) {
        case Opcode.BINARY:
        case Opcode.CONTINUATION: {
            const isStart = opcode === Opcode.BINARY;
            if ( isStart === this.#fragments.length > 0 ) {
                this.#fail( CloseCode.PROTOCOL_ERROR );
                return;
            }

            this.#fragments.push( payload );
            this.#fragmentsSize += payload.length;

            if ( isFinal ) {
                const message = this.#fragments.length === 1 ? this.#fragments[0] : Buffer.concat( this.#fragments );
                this.#fragments = [];
                this.#fragmentsSize = 0;
                // A plain Uint8Array: Buffer.slice() would not copy.
                this.emit( 'message', new Uint8Array( message.buffer, message.byteOffset, message.length ) );
            }
            return;
        }
        case Opcode.TEXT:
            this.#fail( CloseCode.UNSUPPORTED_DATA );
            return;
        case Opcode.CLOSE:
            if ( !this.#isCloseSent ) {
                this.#isCloseSent = true;
                this.#sendFrame( Opcode.CLOSE, payload.length >= 2 ? payload.subarray( 0, 2 ) : Buffer.alloc( 0 ) );
            }
            this.#socket.end();
            return;
        case Opcode.PING:
            if ( this.isOpen ) {
                this.#sendFrame( Opcode.PONG, payload );
            }
            this.emit( 'ping' );
            return;
        case Opcode.PONG:
            this.emit( 'pong' );
            return;
        default:
            this.#fail( CloseCode.PROTOCOL_ERROR );
        }
    }

    #onSocketClose()
    {
        if ( this.#isClosed ) {
            return;
        }
        this.#isClosed = true;
        clearTimeout( this.#closeTimer );
        this.emit( 'close' );
    }
}
