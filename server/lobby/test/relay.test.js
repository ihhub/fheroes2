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
import {test} from 'node:test';

import {CloseCode, decodeMessage, encodeMessage, MessageType, RelayRefusal} from '../protocol.js';
import {Relay} from '../relay.js';

const ROOM_ID = 0x1122334455667788n;
const HOST_TOKEN = new Uint8Array( 16 ).fill( 0xa0 );
const GUEST_TICKET = new Uint8Array( 16 ).fill( 0xb0 );

function makeRelay( settings = {} )
{
    let now = 0;
    const rooms = new Map( [ [ ROOM_ID, { hostToken : HOST_TOKEN, guestTicket : GUEST_TICKET } ] ] );
    const relay = new Relay( settings, { now : () => now, findRoom : ( roomId ) => rooms.get( roomId ) ?? null } );

    return {
        relay,
        rooms,
        advance( ms ) {
            now += ms;
            relay.tick();
        },
    };
}

class Side
{
    constructor( relay, roomId = ROOM_ID )
    {
        this.relay = relay;
        this.received = [];
        this.closeCode = null;
        this.isPaused = false;
        // When set, the connection says that what it was sent could not leave at once.
        this.isFull = false;
        this.client = relay.connect(
            {
                send : ( bytes ) => {
                    this.received.push( bytes );
                    return !this.isFull;
                },
                close : ( code ) => { this.closeCode = code; },
                pause : () => { this.isPaused = true; },
                resume : () => { this.isPaused = false; },
            },
            roomId,
        );
    }

    send( type, fields )
    {
        this.relay.receive( this.client, encodeMessage( type, fields ) );
    }

    sendBytes( bytes )
    {
        this.relay.receive( this.client, bytes );
    }

    // The lobby messages received.
    messages()
    {
        return this.received.map( ( bytes ) => decodeMessage( bytes ) );
    }
}

function pair( relay )
{
    const host = new Side( relay );
    host.send( MessageType.RELAY_HOST, { key : HOST_TOKEN } );
    const guest = new Side( relay );
    guest.send( MessageType.RELAY_GUEST, { key : GUEST_TICKET } );
    return { host, guest };
}

test( 'a host waiting and a guest are paired, then everything is forwarded as it is', () => {
    const { relay } = makeRelay();
    const { host, guest } = pair( relay );

    assert.deepEqual( host.messages(), [ { type : MessageType.RELAY_READY, fields : {} } ] );
    assert.deepEqual( guest.messages(), [ { type : MessageType.RELAY_READY, fields : {} } ] );
    assert.deepEqual( relay.stats(), { waitingHosts : 0, pairs : 1 } );

    // Not lobby messages: the relay never reads them.
    const fromGuest = new Uint8Array( [ 1, 2, 3 ] );
    const fromHost = new Uint8Array( 1024 * 1024 ).fill( 7 );
    guest.sendBytes( fromGuest );
    host.sendBytes( fromHost );
    assert.equal( host.received.at( -1 ), fromGuest );
    assert.equal( guest.received.at( -1 ), fromHost );

    // Both sides go together.
    relay.disconnect( guest.client );
    assert.equal( host.closeCode, CloseCode.NORMAL );
    assert.deepEqual( relay.stats(), { waitingHosts : 0, pairs : 0 } );
} );

test( 'connections which cannot be paired are told why', () => {
    const { relay, rooms } = makeRelay( { maxWaitingHostsPerRoom : 1 } );

    const refusalOf = ( side ) => {
        assert.equal( side.closeCode, CloseCode.NORMAL );
        const [message] = side.messages();
        assert.equal( message.type, MessageType.RELAY_REFUSED );
        return message.fields.reason;
    };

    const unknownRoom = new Side( relay, 5n );
    unknownRoom.send( MessageType.RELAY_GUEST, { key : GUEST_TICKET } );
    assert.equal( refusalOf( unknownRoom ), RelayRefusal.NO_SUCH_ROOM );

    // The host closed the game: the guest learns it at once.
    rooms.set( 6n, { isClosed : true } );
    const closedRoom = new Side( relay, 6n );
    closedRoom.send( MessageType.RELAY_GUEST, { key : GUEST_TICKET } );
    assert.equal( refusalOf( closedRoom ), RelayRefusal.ROOM_CLOSED );

    const wrongToken = new Side( relay );
    wrongToken.send( MessageType.RELAY_HOST, { key : GUEST_TICKET } );
    assert.equal( refusalOf( wrongToken ), RelayRefusal.INVALID_MESSAGE );

    const noHost = new Side( relay );
    noHost.send( MessageType.RELAY_GUEST, { key : GUEST_TICKET } );
    assert.equal( refusalOf( noHost ), RelayRefusal.NO_HOST_WAITING );

    const garbage = new Side( relay );
    garbage.sendBytes( new Uint8Array( [ 1, 2, 3 ] ) );
    assert.equal( refusalOf( garbage ), RelayRefusal.INVALID_MESSAGE );

    const otherMessage = new Side( relay );
    otherMessage.send( MessageType.RELAY_READY, {} );
    assert.equal( refusalOf( otherMessage ), RelayRefusal.INVALID_MESSAGE );

    const host = new Side( relay );
    host.send( MessageType.RELAY_HOST, { key : HOST_TOKEN } );
    const secondHost = new Side( relay );
    secondHost.send( MessageType.RELAY_HOST, { key : HOST_TOKEN } );
    assert.equal( refusalOf( secondHost ), RelayRefusal.LIMIT_REACHED );

    const wrongTicket = new Side( relay );
    wrongTicket.send( MessageType.RELAY_GUEST, { key : HOST_TOKEN } );
    assert.equal( refusalOf( wrongTicket ), RelayRefusal.INVALID_MESSAGE );

    // Nothing is expected from a host before a guest comes.
    host.sendBytes( new Uint8Array( [ 1 ] ) );
    assert.equal( host.closeCode, CloseCode.PROTOCOL_ERROR );
} );

test( 'a slow side makes the other one wait', () => {
    const { relay, advance } = makeRelay( { maxPauseMs : 30_000 } );
    const { host, guest } = pair( relay );

    host.isFull = true;
    guest.sendBytes( new Uint8Array( [ 1 ] ) );
    assert.equal( guest.isPaused, true );

    relay.drained( host.client );
    assert.equal( guest.isPaused, false );

    // Both sides waiting for each other: a paused connection does not see that it was closed, the pair must not stay.
    guest.isFull = true;
    guest.sendBytes( new Uint8Array( [ 2 ] ) );
    host.sendBytes( new Uint8Array( [ 3 ] ) );
    assert.equal( guest.isPaused, true );
    assert.equal( host.isPaused, true );

    advance( 20_000 );
    assert.equal( host.closeCode, null );
    advance( 11_000 );
    assert.ok( host.closeCode !== null && guest.closeCode !== null );
    assert.deepEqual( relay.stats(), { waitingHosts : 0, pairs : 0 } );
} );

test( 'limits of the relay', () => {
    const { relay, advance } = makeRelay( { maxBytesPerRoomPerDay : 100, maxFrameSize : 64, firstMessageTimeoutMs : 10_000 } );

    {
        const { host, guest } = pair( relay );
        guest.sendBytes( new Uint8Array( 65 ) );
        assert.equal( guest.closeCode, CloseCode.TOO_BIG );
        assert.equal( host.closeCode, CloseCode.NORMAL );
    }
    {
        const { host, guest } = pair( relay );
        guest.sendBytes( new Uint8Array( 60 ) );
        assert.equal( guest.closeCode, null );
        guest.sendBytes( new Uint8Array( 60 ) );
        assert.equal( guest.closeCode, CloseCode.POLICY_VIOLATION );
        assert.equal( host.closeCode, CloseCode.NORMAL );
    }
    {
        // No more for this room today.
        const host = new Side( relay );
        host.send( MessageType.RELAY_HOST, { key : HOST_TOKEN } );
        const guest = new Side( relay );
        guest.send( MessageType.RELAY_GUEST, { key : GUEST_TICKET } );
        assert.equal( guest.messages()[0].fields.reason, RelayRefusal.LIMIT_REACHED );

        advance( 24 * 60 * 60 * 1000 );
        const nextDay = new Side( relay );
        nextDay.send( MessageType.RELAY_GUEST, { key : GUEST_TICKET } );
        assert.equal( nextDay.messages()[0].type, MessageType.RELAY_READY );
    }
    {
        const silent = new Side( relay );
        advance( 11_000 );
        assert.equal( silent.closeCode, CloseCode.POLICY_VIOLATION );
    }
} );

test( 'a room closed by the lobby closes its relay connections', () => {
    const { relay } = makeRelay();
    const { host, guest } = pair( relay );
    const waiting = new Side( relay );
    waiting.send( MessageType.RELAY_HOST, { key : HOST_TOKEN } );

    relay.closeRoom( ROOM_ID );
    assert.ok( [ host.closeCode, guest.closeCode ].includes( CloseCode.GOING_AWAY ) );
    assert.ok( host.closeCode !== null && guest.closeCode !== null );
    assert.equal( waiting.closeCode, CloseCode.GOING_AWAY );
    assert.deepEqual( relay.stats(), { waitingHosts : 0, pairs : 0 } );
} );
