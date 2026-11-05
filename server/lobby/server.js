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

// The lobby server for Node.js 22 or later, without any package to install.
//
//   node server.js
//
// Settings come from the environment:
//   LOBBY_PORT                   port to listen on (47220)
//   LOBBY_HOST                   address to listen on (all addresses, IPv6 and IPv4)
//   LOBBY_NAME                   name shown in the game
//   LOBBY_MESSAGE                message of the day
//   LOBBY_RELAY                  "off" to have no relay
//   LOBBY_PROBE                  "off" to give the addresses of hosts without checking them
//   LOBBY_PRIVATE_ADDRESSES      "on" for a lobby on a local network: private addresses are probed and given too
//   LOBBY_TRUST_PROXY            "on" behind a reverse proxy: the address of a game is taken from X-Forwarded-For
//   LOBBY_ROOMS_PER_ADDRESS      games published from one address (3)
//   LOBBY_LOG                    "on" to log what the relay does (no address is ever logged)

import {randomBytes} from 'node:crypto';
import {createServer} from 'node:http';
import {connect as connectTcp} from 'node:net';
import {fileURLToPath} from 'node:url';

import {normalizeAddress} from './addresses.js';
import {Lobby} from './core.js';
import {decodeMessage, encodeMessage, HEADER_SIZE, KEY_SIZE, MAX_PAYLOAD_SIZE, MessageType} from './protocol.js';
import {defaultRelaySettings, Relay} from './relay.js';
import {acceptWebSocket, rejectUpgrade} from './websocket.js';

export const DEFAULT_PORT = 47220;

const RELAY_PATH = /^\/relay\/([0-9a-f]{16})$/;

// Connects to the game port of a host and checks that it answers the probe with the same value.
export function probeHost( {address, port, probeKey, value, timeoutMs} )
{
    return new Promise( ( resolve ) => {
        const expectedSize = HEADER_SIZE + KEY_SIZE;
        let received = Buffer.alloc( 0 );
        let isDone = false;

        const socket = connectTcp( { host : address, port } );
        const finish = ( isReachable ) => {
            if ( !isDone ) {
                isDone = true;
                clearTimeout( timer );
                socket.destroy();
                resolve( isReachable );
            }
        };
        const timer = setTimeout( () => finish( false ), timeoutMs );

        socket.on( 'connect', () => socket.write( encodeMessage( MessageType.LOBBY_PROBE, { probeKey, value } ) ) );
        socket.on( 'data', ( data ) => {
            received = Buffer.concat( [ received, data ] );
            if ( received.length < expectedSize ) {
                return;
            }
            try {
                const reply = decodeMessage( new Uint8Array( received.buffer, received.byteOffset, expectedSize ) );
                finish( reply.type === MessageType.LOBBY_PROBE_REPLY && Buffer.from( reply.fields.key ).equals( Buffer.from( value ) ) );
            }
            catch {
                finish( false );
            }
        } );
        socket.on( 'error', () => finish( false ) );
        socket.on( 'close', () => finish( false ) );
    } );
}

const toRoomId = ( hex ) => BigInt( `0x${hex}` );

export function startLobbyServer(
    {port = DEFAULT_PORT, host, trustProxy = false, lobby : lobbySettings = {}, relay : relaySettings = {}, probe = probeHost, log = () => {}} = {} )
{
    const platform = {
        now : () => Date.now(),
        randomBytes : ( size ) => new Uint8Array( randomBytes( size ) ),
        probe,
        onRoomClosed : ( roomId ) => relay?.closeRoom( roomId ),
        log,
    };

    const lobby = new Lobby( lobbySettings, platform );
    const relay = lobby.settings.isRelayEnabled ? new Relay( relaySettings, { now : platform.now, findRoom : ( roomId ) => lobby.findRoom( roomId ), log } ) : null;

    const sockets = new Set();

    const addressOf = ( request ) => {
        const forwarded = trustProxy ? request.headers['x-forwarded-for'] : undefined;
        const address = typeof forwarded === 'string' ? forwarded.split( ',' )[0].trim() : request.socket.remoteAddress;
        return normalizeAddress( address ) ?? '';
    };

    const server = createServer( ( request, response ) => {
        if ( request.method === 'GET' && request.url === '/' ) {
            const { playersOnline, waitingRooms, roomsInGame } = lobby.stats();
            response.writeHead( 200, { 'Content-Type' : 'text/plain; charset=utf-8' } );
            response.end(
                `${lobby.settings.name}: fheroes2 lobby\nplayers online: ${playersOnline}\nrooms waiting for players: ${waitingRooms}\nrooms in game: ${roomsInGame}\n` );
            return;
        }
        response.writeHead( 404, { 'Content-Type' : 'text/plain; charset=utf-8' } );
        response.end( 'not found\n' );
    } );

    server.on( 'connection', ( socket ) => {
        sockets.add( socket );
        socket.on( 'close', () => sockets.delete( socket ) );
    } );

    server.on( 'upgrade', ( request, socket, head ) => {
        socket.on( 'error', () => {} );

        if ( request.url === '/lobby' ) {
            const webSocket = acceptWebSocket( request, socket, head, { maxMessageSize : HEADER_SIZE + MAX_PAYLOAD_SIZE } );
            if ( !webSocket ) {
                return;
            }
            const client = lobby.connect( { address : addressOf( request ), send : ( bytes ) => webSocket.send( bytes ), close : ( code ) => webSocket.close( code ) } );
            webSocket.on( 'message', ( bytes ) => lobby.receive( client, bytes ) );
            webSocket.on( 'ping', () => lobby.touch( client ) );
            webSocket.on( 'close', () => lobby.disconnect( client ) );
            return;
        }

        const relayPath = RELAY_PATH.exec( request.url ?? '' );
        if ( relayPath && relay ) {
            const webSocket
                = acceptWebSocket( request, socket, head, { maxMessageSize : ( relaySettings.maxFrameSize ?? defaultRelaySettings.maxFrameSize ) + HEADER_SIZE } );
            if ( !webSocket ) {
                return;
            }
            const client = relay.connect(
                {
                    send : ( bytes ) => webSocket.send( bytes ),
                    close : ( code ) => webSocket.close( code ),
                    pause : () => webSocket.pause(),
                    resume : () => webSocket.resume(),
                },
                toRoomId( relayPath[1] ),
            );
            webSocket.on( 'message', ( bytes ) => relay.receive( client, bytes ) );
            webSocket.on( 'drain', () => relay.drained( client ) );
            webSocket.on( 'close', () => relay.disconnect( client ) );
            return;
        }

        rejectUpgrade( socket, 404, 'Not Found' );
    } );

    const ticker = setInterval( () => {
        lobby.tick();
        relay?.tick();
    }, 1000 );
    ticker.unref();

    return new Promise( ( resolve, reject ) => {
        server.once( 'error', reject );
        server.listen( { port, host, ipv6Only : false }, () => {
            server.off( 'error', reject );
            resolve( {
                port : server.address().port,
                lobby,
                relay,
                close : () => new Promise( ( done ) => {
                    clearInterval( ticker );
                    server.close( () => done() );
                    for ( const socket of sockets ) {
                        socket.destroy();
                    }
                } ),
            } );
        } );
    } );
}

if ( process.argv[1] === fileURLToPath( import.meta.url ) ) {
    const environment = process.env;
    const isOn = ( name, byDefault ) => ( environment[name] === undefined ? byDefault : environment[name] === 'on' );

    const { port } = await startLobbyServer( {
        port : Number( environment.LOBBY_PORT ?? DEFAULT_PORT ),
        host : environment.LOBBY_HOST,
        trustProxy : isOn( 'LOBBY_TRUST_PROXY', false ),
        lobby : {
            name : environment.LOBBY_NAME ?? 'fheroes2',
            messageOfTheDay : environment.LOBBY_MESSAGE ?? '',
            isRelayEnabled : environment.LOBBY_RELAY !== 'off',
            isProbeEnabled : environment.LOBBY_PROBE !== 'off',
            allowPrivateAddresses : isOn( 'LOBBY_PRIVATE_ADDRESSES', false ),
            ...( environment.LOBBY_ROOMS_PER_ADDRESS && { maxRoomsPerAddress : Number( environment.LOBBY_ROOMS_PER_ADDRESS ) } ),
        },
        log : isOn( 'LOBBY_LOG', false ) ? ( text ) => console.log( `${new Date().toISOString()} ${text}` ) : () => {},
    } );

    console.log( `fheroes2 lobby listening on port ${port}` );
}
