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

// The lobby on Cloudflare Workers, with the same rooms, list and relay as the server for Node.js (core.js, relay.js).
//
// One Durable Object holds the lobby and its relay, so that both share the rooms. It stays in memory while games are
// connected, and Cloudflare forgets it when nobody is there. Settings come from the variables of wrangler.toml.

import {connect} from 'cloudflare:sockets';

import {normalizeAddress} from './addresses.js';
import {Lobby} from './core.js';
import {CloseCode, decodeMessage, encodeMessage, HEADER_SIZE, KEY_SIZE, MessageType} from './protocol.js';
import {Relay} from './relay.js';

const RELAY_PATH = /^\/relay\/([0-9a-f]{16})$/;

const isOn = ( value, byDefault ) => ( value === undefined ? byDefault : value === 'on' );

// Connects to the game port of a host and checks that it answers the probe with the same value.
async function probeHost( { address, port, probeKey, value, timeoutMs } )
{
    let socket;
    let timer;
    try {
        socket = connect( { hostname : address, port } );
        const timeout = new Promise( ( resolve ) => { timer = setTimeout( () => resolve( false ), timeoutMs ); } );

        const check = ( async () => {
                          const writer = socket.writable.getWriter();
                          await writer.write( encodeMessage( MessageType.LOBBY_PROBE, { probeKey, value } ) );
                          writer.releaseLock();

                          const expectedSize = HEADER_SIZE + KEY_SIZE;
                          const reader = socket.readable.getReader();
                          let received = new Uint8Array( 0 );
                          while ( received.length < expectedSize ) {
                              const { value : chunk, done } = await reader.read();
                              if ( done ) {
                                  return false;
                              }
                              const joined = new Uint8Array( received.length + chunk.length );
                              joined.set( received );
                              joined.set( chunk, received.length );
                              received = joined;
                          }

                          const reply = decodeMessage( received.subarray( 0, expectedSize ) );
                          return reply.type === MessageType.LOBBY_PROBE_REPLY && reply.fields.key.every( ( byte, i ) => byte === value[i] );
                      } )()
                          .catch( () => false );

        return await Promise.race( [ check, timeout ] );
    }
    catch {
        return false;
    }
    finally {
        clearTimeout( timer );
        socket?.close().catch( () => {} );
    }
}

function closeSocket( webSocket, code )
{
    try {
        webSocket.close( code );
    }
    catch {
        // Some codes cannot be sent from here.
        try {
            webSocket.close( CloseCode.NORMAL );
        }
        catch {
            // Already closed.
        }
    }
}

export class LobbyObject
{
    #lobby;
    #relay;
    #ticker = null;
    #connections = 0;

    constructor( state, env )
    {
        const platform = {
            now : () => Date.now(),
            randomBytes : ( size ) => crypto.getRandomValues( new Uint8Array( size ) ),
            probe : probeHost,
            onRoomClosed : ( roomId ) => this.#relay?.closeRoom( roomId ),
        };

        this.#lobby = new Lobby(
            {
                name : env.LOBBY_NAME ?? 'fheroes2',
                messageOfTheDay : env.LOBBY_MESSAGE ?? '',
                isRelayEnabled : isOn( env.LOBBY_RELAY, true ),
                isProbeEnabled : isOn( env.LOBBY_PROBE, true ),
                allowPrivateAddresses : isOn( env.LOBBY_PRIVATE_ADDRESSES, false ),
                ...( env.LOBBY_ROOMS_PER_ADDRESS && { maxRoomsPerAddress : Number( env.LOBBY_ROOMS_PER_ADDRESS ) } ),
            },
            platform,
        );

        this.#relay = this.#lobby.settings.isRelayEnabled ? new Relay( {}, { now : platform.now, findRoom : ( roomId ) => this.#lobby.findRoom( roomId ) } ) : null;
    }

    // Ticks while someone is connected.
    #opened()
    {
        ++this.#connections;
        if ( this.#ticker === null ) {
            this.#ticker = setInterval( () => {
                this.#lobby.tick();
                this.#relay?.tick();
            }, 1000 );
        }
    }

    #closed()
    {
        --this.#connections;
        if ( this.#connections === 0 && this.#ticker !== null ) {
            clearInterval( this.#ticker );
            this.#ticker = null;
        }
    }

    async fetch( request )
    {
        const url = new URL( request.url );

        if ( request.headers.get( 'Upgrade' ) !== 'websocket' ) {
            if ( url.pathname === '/' ) {
                const { playersOnline, waitingRooms, roomsInGame } = this.#lobby.stats();
                return new Response(
                    `${this.#lobby.settings.name}: fheroes2 lobby\nplayers online: ${playersOnline}\nrooms waiting for players: ${waitingRooms}\nrooms in game: ${
                        roomsInGame}\n`,
                    { headers : { 'Content-Type' : 'text/plain; charset=utf-8' } },
                );
            }
            return new Response( 'not found\n', { status : 404 } );
        }

        const relayPath = RELAY_PATH.exec( url.pathname );
        if ( url.pathname !== '/lobby' && !( relayPath && this.#relay ) ) {
            return new Response( 'not found\n', { status : 404 } );
        }

        const [client, server] = Object.values( new WebSocketPair() );
        server.accept();
        // Binary messages come as Blob otherwise.
        server.binaryType = 'arraybuffer';
        this.#opened();

        const send = ( bytes ) => {
            try {
                server.send( bytes );
            }
            catch {
                // Closed meanwhile.
            }
            return true;
        };

        // An error does not always come with a close event.
        let isGone = false;
        const onGone = ( disconnect ) => () => {
            if ( !isGone ) {
                isGone = true;
                disconnect();
                this.#closed();
            }
        };

        if ( url.pathname === '/lobby' ) {
            const address = normalizeAddress( request.headers.get( 'CF-Connecting-IP' ) ?? '' ) ?? '';
            const lobbyClient = this.#lobby.connect( { address, send, close : ( code ) => closeSocket( server, code ) } );

            server.addEventListener( 'message', ( event ) => {
                if ( typeof event.data === 'string' ) {
                    closeSocket( server, CloseCode.UNSUPPORTED_DATA );
                    return;
                }
                // Any message shows the game is still there: no pong reaches this code.
                this.#lobby.touch( lobbyClient );
                this.#lobby.receive( lobbyClient, new Uint8Array( event.data ) );
            } );
            const gone = onGone( () => this.#lobby.disconnect( lobbyClient ) );
            server.addEventListener( 'close', gone );
            server.addEventListener( 'error', gone );
        }
        else {
            const relayClient = this.#relay.connect(
                { send, close : ( code ) => closeSocket( server, code ), pause : () => {}, resume : () => {} },
                BigInt( `0x${relayPath[1]}` ),
            );

            server.addEventListener( 'message', ( event ) => {
                if ( typeof event.data === 'string' ) {
                    closeSocket( server, CloseCode.UNSUPPORTED_DATA );
                    return;
                }
                this.#relay.receive( relayClient, new Uint8Array( event.data ) );
            } );
            const gone = onGone( () => this.#relay.disconnect( relayClient ) );
            server.addEventListener( 'close', gone );
            server.addEventListener( 'error', gone );
        }

        return new Response( null, { status : 101, webSocket : client } );
    }
}

export default {
    async fetch( request, env ) {
        // One lobby for every game.
        const lobby = env.LOBBY.get( env.LOBBY.idFromName( 'lobby' ) );
        return lobby.fetch( request );
    },
};
