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

// The relay of the lobby, for games which cannot connect directly. It pairs a connection of a host waiting for a guest
// with the connection of a guest, then forwards what comes on one to the other without reading it. Nothing here depends
// on where the lobby runs.
//
// A connection is { send(bytes) giving false when the data could not leave at once, close(code), pause(), resume() }.
// The server calls connect() for each new connection to /relay/<room id>, receive() for each message, drained() when
// what was sent to a connection left, disconnect() when it is closed, and tick() every second.

import {CloseCode, decodeMessage, encodeMessage, MessageType, ProtocolError, RelayRefusal} from './protocol.js';

export const defaultRelaySettings = Object.freeze( {
    firstMessageTimeoutMs : 10_000,
    // A host keeps one connection waiting, and opens another one as soon as a guest takes it.
    maxWaitingHostsPerRoom : 2,
    // Five guests at most, each one may connect again while its old pair is not closed yet.
    maxPairsPerRoom : 12,
    // A game through the relay sends a few MB per hour.
    maxBytesPerRoomPerDay : 1_000_000_000,
    // The games send frames of 1 MB at most.
    maxFrameSize : 1024 * 1024,
    // A side which cannot send to the other one for this long is stuck: the pair is closed and the games connect again.
    maxPauseMs : 30_000,
} );

const RelayState = Object.freeze( { FIRST : 0, WAITING : 1, PAIRED : 2, CLOSED : 3 } );

const DAY_MS = 24 * 60 * 60 * 1000;

const keysEqual = ( first, second ) => first.length === second.length && first.reduce( ( difference, byte, i ) => difference | ( byte ^ second[i] ), 0 ) === 0;

export class Relay
{
    #settings;
    #platform;
    // Connections which did not say yet who they are.
    #newClients = new Set();
    // Only rooms of the lobby, so that connections to unknown rooms keep nothing.
    #rooms = new Map();

    // platform: { now(), findRoom(roomId) giving { hostToken, guestTicket }, { isClosed: true } or null, log(text) (optional) }.
    constructor( settings, platform )
    {
        this.#settings = {...defaultRelaySettings, ...settings };
        this.#platform = platform;
    }

    #log( text, room )
    {
        this.#platform.log?.( `relay: ${text}${room ? ` (${room.pairs} pair(s), ${room.waitingHosts.length} host(s) waiting)` : ''}` );
    }

    connect( connection, roomId )
    {
        const client = { connection, roomId, state : RelayState.FIRST, connectedAt : this.#platform.now(), peer : null, isPaused : false, pausedAt : 0 };
        this.#newClients.add( client );
        return client;
    }

    receive( client, bytes )
    {
        switch ( client.state ) {
        case RelayState.PAIRED:
            this.#forward( client, bytes );
            return;
        case RelayState.FIRST:
            this.#first( client, bytes );
            return;
        case RelayState.WAITING:
            // Nothing is expected from a host before a guest comes.
            this.#drop( client, CloseCode.PROTOCOL_ERROR );
            return;
        default:
        }
    }

    // What was sent to this client left: its peer can send again.
    drained( client )
    {
        const peer = client.peer;
        if ( peer && peer.isPaused ) {
            peer.isPaused = false;
            peer.connection.resume();
        }
    }

    disconnect( client )
    {
        this.#forget( client );
    }

    closeRoom( roomId )
    {
        const room = this.#rooms.get( roomId );
        if ( room ) {
            this.#rooms.delete( roomId );
            for ( const client of [...room.clients] ) {
                this.#drop( client, CloseCode.GOING_AWAY );
            }
        }
    }

    tick()
    {
        const now = this.#platform.now();
        for ( const client of [...this.#newClients] ) {
            if ( now - client.connectedAt > this.#settings.firstMessageTimeoutMs ) {
                this.#drop( client, CloseCode.POLICY_VIOLATION );
            }
        }

        // A paused connection does not even see that it was closed.
        for ( const room of this.#rooms.values() ) {
            for ( const client of [...room.clients] ) {
                if ( client.isPaused && now - client.pausedAt > this.#settings.maxPauseMs ) {
                    this.#log( 'stuck pair closed', room );
                    this.#drop( client, CloseCode.GOING_AWAY );
                }
            }
        }
    }

    stats()
    {
        let waitingHosts = 0;
        let pairs = 0;
        for ( const room of this.#rooms.values() ) {
            waitingHosts += room.waitingHosts.length;
            pairs += room.pairs;
        }
        return { waitingHosts, pairs };
    }

    #refuse( client, reason )
    {
        this.#log( `refused, reason ${reason}`, this.#rooms.get( client.roomId ) );
        client.connection.send( encodeMessage( MessageType.RELAY_REFUSED, { reason } ) );
        this.#drop( client, CloseCode.NORMAL );
    }

    #first( client, bytes )
    {
        let message;
        try {
            message = decodeMessage( bytes );
        }
        catch ( error ) {
            if ( !( error instanceof ProtocolError ) ) {
                throw error;
            }
            this.#refuse( client, RelayRefusal.INVALID_MESSAGE );
            return;
        }

        if ( message.type !== MessageType.RELAY_HOST && message.type !== MessageType.RELAY_GUEST ) {
            this.#refuse( client, RelayRefusal.INVALID_MESSAGE );
            return;
        }

        const lobbyRoom = this.#platform.findRoom( client.roomId );
        if ( !lobbyRoom || lobbyRoom.isClosed ) {
            this.#refuse( client, lobbyRoom ? RelayRefusal.ROOM_CLOSED : RelayRefusal.NO_SUCH_ROOM );
            return;
        }

        let room = this.#rooms.get( client.roomId );
        if ( !room ) {
            room = { clients : new Set(), waitingHosts : [], pairs : 0, day : 0, bytes : 0 };
            this.#rooms.set( client.roomId, room );
        }

        if ( message.type === MessageType.RELAY_HOST ) {
            if ( !keysEqual( message.fields.key, lobbyRoom.hostToken ) ) {
                this.#refuse( client, RelayRefusal.INVALID_MESSAGE );
            }
            else if ( room.waitingHosts.length >= this.#settings.maxWaitingHostsPerRoom ) {
                this.#refuse( client, RelayRefusal.LIMIT_REACHED );
            }
            else {
                this.#newClients.delete( client );
                room.clients.add( client );
                client.state = RelayState.WAITING;
                room.waitingHosts.push( client );
                this.#log( 'host waiting', room );
            }
            return;
        }

        if ( !keysEqual( message.fields.key, lobbyRoom.guestTicket ) ) {
            this.#refuse( client, RelayRefusal.INVALID_MESSAGE );
            return;
        }
        if ( room.pairs >= this.#settings.maxPairsPerRoom || this.#isOverDailyLimit( room, 0 ) ) {
            this.#refuse( client, RelayRefusal.LIMIT_REACHED );
            return;
        }

        const host = room.waitingHosts.shift();
        if ( !host ) {
            this.#refuse( client, RelayRefusal.NO_HOST_WAITING );
            return;
        }

        this.#newClients.delete( client );
        room.clients.add( client );
        host.state = RelayState.PAIRED;
        client.state = RelayState.PAIRED;
        host.peer = client;
        client.peer = host;
        ++room.pairs;

        this.#log( 'paired', room );

        const ready = encodeMessage( MessageType.RELAY_READY );
        host.connection.send( ready );
        client.connection.send( ready );
    }

    #isOverDailyLimit( room, bytes )
    {
        const day = Math.floor( this.#platform.now() / DAY_MS );
        if ( room.day !== day ) {
            room.day = day;
            room.bytes = 0;
        }
        room.bytes += bytes;
        return room.bytes > this.#settings.maxBytesPerRoomPerDay;
    }

    #forward( client, bytes )
    {
        const room = this.#rooms.get( client.roomId );

        if ( bytes.length > this.#settings.maxFrameSize ) {
            this.#drop( client, CloseCode.TOO_BIG );
            return;
        }
        if ( this.#isOverDailyLimit( room, bytes.length ) ) {
            this.#drop( client, CloseCode.POLICY_VIOLATION );
            return;
        }

        // The peer does not read fast enough: stop reading here until what it was sent has left.
        if ( !client.peer.connection.send( bytes ) && !client.isPaused ) {
            this.#log( 'paused', room );
            client.isPaused = true;
            client.pausedAt = this.#platform.now();
            client.connection.pause();
        }
    }

    #drop( client, code )
    {
        if ( client.state !== RelayState.CLOSED ) {
            this.#forget( client );
            client.connection.close( code );
        }
    }

    // Both sides of a pair go together: the session of the games comes back as after a cut of a direct connection.
    #forget( client )
    {
        if ( client.state === RelayState.CLOSED ) {
            return;
        }

        client.state = RelayState.CLOSED;
        this.#newClients.delete( client );

        const room = this.#rooms.get( client.roomId );
        if ( room ) {
            room.clients.delete( client );
            room.waitingHosts = room.waitingHosts.filter( ( host ) => host !== client );
        }

        const peer = client.peer;
        client.peer = null;
        if ( peer ) {
            if ( room ) {
                --room.pairs;
            }
            this.#log( 'pair closed', room );
            peer.peer = null;
            this.#drop( peer, CloseCode.NORMAL );
        }
    }
}
