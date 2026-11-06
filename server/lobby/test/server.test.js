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

// The whole lobby server over real connections, with the WebSocket client of Node.js and a fake game host which
// answers probes.

import assert from 'node:assert/strict';
import {randomBytes} from 'node:crypto';
import {once} from 'node:events';
import {createServer} from 'node:net';
import {test} from 'node:test';

import {
    CloseCode,
    decodeMessage,
    encodeMessage,
    HEADER_SIZE,
    JoinCandidateKind,
    JoinResult,
    MessageType,
    RelayRefusal,
    RoomState,
    RoomStatusResult,
    RoomTurns,
    RoomVisibility,
    WelcomeResult,
} from '../protocol.js';
import {probeHost, startLobbyServer} from '../server.js';

const GAME = {
    version : [ 1, 1, 17 ],
    protocolVersion : 18,
    buildId : 0x0123456789abcdefn,
    platform : 'Windows x64'
};
const ROOM_GAME = {
    mapName : 'Brokenshores',
    mapWidth : 72,
    mapHeight : 72,
    players : 4,
    humanPlayers : 2,
    guestHumanPlayers : 1,
    turns : RoomTurns.SIMULTANEOUS,
    day : 1
};

class Client
{
    #queue = [];
    #waiters = [];

    static async open( port, path )
    {
        const client = new Client( new WebSocket( `ws://127.0.0.1:${port}${path}` ) );
        await once( client.webSocket, 'open' );
        return client;
    }

    constructor( webSocket )
    {
        this.webSocket = webSocket;
        webSocket.binaryType = 'arraybuffer';
        this.closed = new Promise( ( resolve ) => webSocket.addEventListener( 'close', ( event ) => resolve( event.code ) ) );
        webSocket.addEventListener( 'message', ( event ) => {
            this.#queue.push( new Uint8Array( event.data ) );
            this.#waiters.splice( 0 ).forEach( ( resolve ) => resolve() );
        } );
    }

    send( type, fields = {} )
    {
        this.webSocket.send( encodeMessage( type, fields ) );
    }

    async nextBytes()
    {
        while ( this.#queue.length === 0 ) {
            await new Promise( ( resolve ) => this.#waiters.push( resolve ) );
        }
        return this.#queue.shift();
    }

    async next( type )
    {
        const message = decodeMessage( await this.nextBytes() );
        assert.equal( message.type, type );
        return message.fields;
    }

    close()
    {
        this.webSocket.close( 1000 );
    }
}

// Answers the probes with its key, on a port given to the lobby as its game port.
async function startFakeHost( probeKey )
{
    const server = createServer( ( socket ) => {
        let received = Buffer.alloc( 0 );
        socket.on( 'error', () => {} );
        socket.on( 'data', ( data ) => {
            received = Buffer.concat( [ received, data ] );
            if ( received.length < HEADER_SIZE + 32 ) {
                return;
            }
            const probe = decodeMessage( new Uint8Array( received.subarray( 0, HEADER_SIZE + 32 ) ) );
            if ( probe.type === MessageType.LOBBY_PROBE && Buffer.from( probe.fields.probeKey ).equals( Buffer.from( probeKey ) ) ) {
                socket.end( encodeMessage( MessageType.LOBBY_PROBE_REPLY, { key : probe.fields.value } ) );
            }
            else {
                socket.destroy();
            }
        } );
    } );
    server.listen( 0, '127.0.0.1' );
    await once( server, 'listening' );
    return { port : server.address().port, close : () => server.close() };
}

const hello = ( client, nickname ) => {
    client.send( MessageType.LOBBY_HELLO, { lobbyProtocolVersion : 1, game : GAME, language : 'fr', nickname } );
    return client.next( MessageType.LOBBY_WELCOME );
};

test( 'a host publishes, a guest finds the room, joins it and both go through the relay', async () => {
    const probeKey = new Uint8Array( randomBytes( 16 ) );
    const hostToken = new Uint8Array( randomBytes( 16 ) );
    const fakeHost = await startFakeHost( probeKey );
    const lobby = await startLobbyServer( { port : 0, host : '127.0.0.1', lobby : { name : 'Salon de test', allowPrivateAddresses : true, roomGraceMs : 0 } } );
    const clients = [];
    const open = async ( path ) => {
        const client = await Client.open( lobby.port, path );
        clients.push( client );
        return client;
    };

    try {
        const page = await ( await fetch( `http://127.0.0.1:${lobby.port}/` ) ).text();
        assert.match( page, /^Salon de test: fheroes2 lobby\nplayers online: 0\n/ );

        const host = await open( '/lobby' );
        assert.equal( ( await hello( host, 'Élodie' ) ).result, WelcomeResult.ACCEPTED );

        host.send( MessageType.ROOM_PUBLISH, {
            visibility : RoomVisibility.PUBLIC,
            isRelayOnly : false,
            gameName : 'Duel',
            game : ROOM_GAME,
            gamePort : fakeHost.port,
            probeKey,
            publicAddresses : [],
            localAddresses : [ '127.0.0.1' ],
            roomId : 0n,
            inviteCode : '',
            state : RoomState.WAITING_FOR_PLAYERS,
            hostToken,
            guestTicket : new Uint8Array( randomBytes( 16 ) ),
        } );
        const status = await host.next( MessageType.ROOM_STATUS );
        assert.equal( status.result, RoomStatusResult.PUBLISHED );
        assert.equal( status.reach.isProbeDone, false );
        assert.deepEqual( ( await host.next( MessageType.ROOM_STATUS ) ).reach, { isIPv4 : true, isIPv6 : false, isProbeDone : true, isRelayAvailable : true } );

        const guest = await open( '/lobby' );
        await hello( guest, 'Marc' );
        guest.send( MessageType.ROOM_LIST_SUBSCRIBE, { isSubscribed : true } );
        const list = await guest.next( MessageType.ROOM_LIST );
        assert.deepEqual(
            list.rooms.map( ( room ) => [room.gameName, room.hostNickname] ),
            [ [ 'Duel', 'Élodie' ] ],
        );
        assert.deepEqual( await guest.next( MessageType.LOBBY_STATS ), { playersOnline : 2, waitingRooms : 1, roomsInGame : 0 } );

        guest.send( MessageType.ROOM_JOIN, { roomId : status.roomId, inviteCode : '' } );
        const plan = await guest.next( MessageType.JOIN_PLAN );
        assert.equal( plan.result, JoinResult.READY );
        // Both on this computer: the local address first, then the address which answered the probe.
        assert.deepEqual( plan.candidates, [
            { kind : JoinCandidateKind.LOCAL, address : '127.0.0.1', port : fakeHost.port },
            { kind : JoinCandidateKind.IPV4, address : '127.0.0.1', port : fakeHost.port },
        ] );

        // The relay.
        const relayPath = `/relay/${status.roomId.toString( 16 ).padStart( 16, '0' )}`;
        const hostRelay = await open( relayPath );
        hostRelay.send( MessageType.RELAY_HOST, { key : hostToken } );
        const guestRelay = await open( relayPath );
        guestRelay.send( MessageType.RELAY_GUEST, { key : plan.relayTicket } );
        await hostRelay.next( MessageType.RELAY_READY );
        await guestRelay.next( MessageType.RELAY_READY );

        const fromGuest = [ randomBytes( 1024 * 1024 ), randomBytes( 1024 * 1024 ), randomBytes( 5 ) ];
        fromGuest.forEach( ( bytes ) => guestRelay.webSocket.send( bytes ) );
        for ( const bytes of fromGuest ) {
            assert.deepEqual( await hostRelay.nextBytes(), new Uint8Array( bytes ) );
        }
        const fromHost = randomBytes( 300 );
        hostRelay.webSocket.send( fromHost );
        assert.deepEqual( await guestRelay.nextBytes(), new Uint8Array( fromHost ) );

        guestRelay.close();
        assert.equal( await hostRelay.closed, CloseCode.NORMAL );

        const wrongTicket = await open( relayPath );
        wrongTicket.send( MessageType.RELAY_GUEST, { key : new Uint8Array( 16 ) } );
        assert.equal( ( await wrongTicket.next( MessageType.RELAY_REFUSED ) ).reason, RelayRefusal.INVALID_MESSAGE );
        assert.equal( await wrongTicket.closed, CloseCode.NORMAL );

        // When the host leaves, the room goes, and so does its connection waiting on the relay.
        const waitingRelay = await open( relayPath );
        waitingRelay.send( MessageType.RELAY_HOST, { key : hostToken } );
        host.close();
        assert.deepEqual( await guest.next( MessageType.ROOM_REMOVED ), { roomId : status.roomId } );
        assert.equal( await waitingRelay.closed, CloseCode.GOING_AWAY );
    }
    finally {
        clients.forEach( ( client ) => client.close() );
        await lobby.close();
        fakeHost.close();
    }
} );

test( 'unknown paths are refused', async () => {
    const lobby = await startLobbyServer( { port : 0, host : '127.0.0.1' } );
    try {
        const webSocket = new WebSocket( `ws://127.0.0.1:${lobby.port}/other` );
        const [event] = await once( webSocket, 'error' );
        assert.equal( event.type, 'error' );

        const response = await fetch( `http://127.0.0.1:${lobby.port}/other` );
        assert.equal( response.status, 404 );
    }
    finally {
        await lobby.close();
    }
} );

test( 'the probe checks the answer of the host', async () => {
    const probeKey = new Uint8Array( 16 ).fill( 1 );
    const fakeHost = await startFakeHost( probeKey );
    try {
        const value = new Uint8Array( 16 ).fill( 2 );
        assert.equal( await probeHost( { address : '127.0.0.1', port : fakeHost.port, probeKey, value, timeoutMs : 2000 } ), true );
        // Another key: the host does not answer.
        assert.equal( await probeHost( { address : '127.0.0.1', port : fakeHost.port, probeKey : new Uint8Array( 16 ), value, timeoutMs : 2000 } ), false );
    }
    finally {
        fakeHost.close();
    }

    // Nobody there.
    const start = Date.now();
    assert.equal( await probeHost( { address : '127.0.0.1', port : fakeHost.port, probeKey, value : probeKey, timeoutMs : 2000 } ), false );
    assert.ok( Date.now() - start < 1500 );
} );
