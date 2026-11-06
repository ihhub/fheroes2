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
import {test} from 'node:test';

import {Lobby} from '../core.js';
import {
    CloseCode,
    decodeMessage,
    encodeMessage,
    isValidInviteCode,
    JoinCandidateKind,
    JoinResult,
    MessageType,
    RoomState,
    RoomStatusResult,
    RoomTurns,
    RoomVisibility,
    WelcomeResult,
} from '../protocol.js';

const GAME = {
    version : [ 1, 1, 17 ],
    protocolVersion : 18,
    buildId : 0x0123456789abcdefn,
    platform : 'Windows x64'
};

const HOST_TOKEN = new Uint8Array( 16 ).fill( 0xa0 );
const GUEST_TICKET = new Uint8Array( 16 ).fill( 0xb0 );

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

function makeLobby( settings = {} )
{
    let now = 1_000_000;
    const probes = [];
    const closedRooms = [];

    const platform = {
        now : () => now,
        randomBytes : ( size ) => new Uint8Array( randomBytes( size ) ),
        probe : ( probe ) => new Promise( ( resolve ) => probes.push( {...probe, resolve } ) ),
        onRoomClosed : ( roomId ) => closedRooms.push( roomId ),
    };

    const lobby = new Lobby( { allowPrivateAddresses : true, ...settings }, platform );

    return {
        lobby,
        probes,
        closedRooms,
        advance( ms ) {
            now += ms;
            lobby.tick();
        },
        // Lets the promises of the probes go on.
        settle : () => new Promise( ( resolve ) => setImmediate( resolve ) ),
    };
}

class Game
{
    constructor( lobby, address )
    {
        this.lobby = lobby;
        this.received = [];
        this.closeCode = null;
        this.client = lobby.connect( {
            address,
            send : ( bytes ) => this.received.push( decodeMessage( bytes ) ),
            close : ( code ) => { this.closeCode = code; },
        } );
    }

    send( type, fields = {} )
    {
        this.lobby.receive( this.client, encodeMessage( type, fields ) );
    }

    hello( nickname = 'Marc', game = GAME )
    {
        this.send( MessageType.LOBBY_HELLO, { lobbyProtocolVersion : 1, game, language : 'fr', nickname } );
        return this.take( MessageType.LOBBY_WELCOME ).at( -1 );
    }

    publish( fields = {} )
    {
        this.send( MessageType.ROOM_PUBLISH, {
            visibility : RoomVisibility.PUBLIC,
            isRelayOnly : false,
            gameName : 'Duel',
            game : ROOM_GAME,
            gamePort : 47219,
            probeKey : new Uint8Array( 16 ).fill( 9 ),
            publicAddresses : [],
            localAddresses : [ '192.168.1.30' ],
            roomId : 0n,
            inviteCode : '',
            state : RoomState.WAITING_FOR_PLAYERS,
            hostToken : HOST_TOKEN,
            guestTicket : GUEST_TICKET,
            ...fields,
        } );
        return this.take( MessageType.ROOM_STATUS ).at( -1 );
    }

    join( fields )
    {
        this.send( MessageType.ROOM_JOIN, { roomId : 0n, inviteCode : '', ...fields } );
        return this.take( MessageType.JOIN_PLAN ).at( -1 );
    }

    // Messages of this type received since the last call, removed from the received ones.
    take( type )
    {
        const taken = this.received.filter( ( message ) => message.type === type ).map( ( message ) => message.fields );
        this.received = this.received.filter( ( message ) => message.type !== type );
        return taken;
    }
}

test( 'a game says hello first', () => {
    const { lobby } = makeLobby( { name : 'Salon de test', messageOfTheDay : 'Bonjour' } );

    const game = new Game( lobby, '203.0.113.1' );
    const welcome = game.hello();
    assert.equal( welcome.result, WelcomeResult.ACCEPTED );
    assert.equal( welcome.lobbyName, 'Salon de test' );
    assert.equal( welcome.messageOfTheDay, 'Bonjour' );
    assert.equal( game.closeCode, null );

    const early = new Game( lobby, '203.0.113.2' );
    early.send( MessageType.ROOM_LIST_SUBSCRIBE, { isSubscribed : true } );
    assert.equal( early.closeCode, CloseCode.PROTOCOL_ERROR );

    const garbage = new Game( lobby, '203.0.113.3' );
    lobby.receive( garbage.client, new Uint8Array( [ 1, 2, 3 ] ) );
    assert.equal( garbage.closeCode, CloseCode.PROTOCOL_ERROR );

    const twice = new Game( lobby, '203.0.113.4' );
    twice.hello();
    twice.hello();
    assert.equal( twice.closeCode, CloseCode.PROTOCOL_ERROR );
} );

test( 'games which cannot come in are told why', () => {
    const { lobby } = makeLobby( { bannedWords : [ 'vilain' ], maxConnectionsPerAddress : 2, maxConnections : 4 } );

    const future = new Game( lobby, '203.0.113.1' );
    future.send( MessageType.LOBBY_HELLO, { lobbyProtocolVersion : 2, game : GAME, language : 'fr', nickname : 'Marc' } );
    assert.equal( future.take( MessageType.LOBBY_WELCOME )[0].result, WelcomeResult.INCOMPATIBLE_VERSIONS );
    assert.equal( future.closeCode, CloseCode.NORMAL );

    const rude = new Game( lobby, '203.0.113.1' );
    assert.equal( rude.hello( 'Le VILAIN' ).result, WelcomeResult.NICKNAME_REFUSED );

    const sameAddress = [ 1, 2, 3 ].map( () => new Game( lobby, '203.0.113.5' ) );
    assert.deepEqual(
        sameAddress.map( ( game ) => game.hello().result ),
        [ WelcomeResult.ACCEPTED, WelcomeResult.ACCEPTED, WelcomeResult.TOO_MANY_CONNECTIONS ],
    );

    const others = [ 6, 7, 8 ].map( ( i ) => new Game( lobby, `203.0.113.${i}` ) );
    assert.deepEqual(
        others.map( ( game ) => game.hello().result ),
        [ WelcomeResult.ACCEPTED, WelcomeResult.ACCEPTED, WelcomeResult.LOBBY_FULL ],
    );
} );

test( 'a published room is probed and listed', async () => {
    const { lobby, probes, settle } = makeLobby();

    const watcher = new Game( lobby, '203.0.113.9' );
    watcher.hello();
    watcher.send( MessageType.ROOM_LIST_SUBSCRIBE, { isSubscribed : true } );
    assert.deepEqual( watcher.take( MessageType.ROOM_LIST ), [ { isLastPart : true, rooms : [] } ] );
    assert.equal( watcher.take( MessageType.LOBBY_STATS ).length, 1 );

    const host = new Game( lobby, '203.0.113.1' );
    host.hello( 'Élodie' );
    const status = host.publish( { publicAddresses : [ '2001:db8::25', '10.0.0.1', '10.0.0.2' ] } );

    assert.equal( status.result, RoomStatusResult.PUBLISHED );
    assert.ok( status.roomId !== 0n );
    assert.ok( isValidInviteCode( status.inviteCode ) );
    assert.deepEqual( status.reach, { isIPv4 : false, isIPv6 : false, isProbeDone : false, isRelayAvailable : true } );

    const [first] = watcher.take( MessageType.ROOM_INFO );
    assert.equal( first.roomId, status.roomId );
    assert.equal( first.hostNickname, 'Élodie' );
    assert.equal( first.reach.isProbeDone, false );

    // One address after another: the game of the host takes one new connection at a time.
    assert.equal( probes.length, 1 );
    for ( const [i, isReachable] of [false, true, true].entries() ) {
        probes[i].resolve( isReachable );
        await settle();
    }

    // The address seen by the lobby, then the given ones, 3 at most.
    assert.deepEqual(
        probes.map( ( probe ) => probe.address ),
        [ '203.0.113.1', '2001:db8::25', '10.0.0.1' ],
    );
    assert.ok( probes.every( ( probe ) => probe.port === 47219 && probe.probeKey.every( ( byte ) => byte === 9 ) ) );

    assert.deepEqual( host.take( MessageType.ROOM_STATUS ).at( -1 ).reach, { isIPv4 : true, isIPv6 : true, isProbeDone : true, isRelayAvailable : true } );
    assert.deepEqual( watcher.take( MessageType.ROOM_INFO ).at( -1 ).reach, { isIPv4 : true, isIPv6 : true, isProbeDone : true, isRelayAvailable : true } );

    // Joined from another box: the addresses which answered, IPv6 first.
    const guest = new Game( lobby, '198.51.100.7' );
    guest.hello();
    const plan = guest.join( { roomId : status.roomId } );
    assert.equal( plan.result, JoinResult.READY );
    assert.deepEqual( plan.candidates, [
        { kind : JoinCandidateKind.IPV6, address : '2001:db8::25', port : 47219 },
        { kind : JoinCandidateKind.IPV4, address : '10.0.0.1', port : 47219 },
    ] );
    assert.equal( plan.room.gameName, 'Duel' );
    assert.equal( plan.relayTicket.length, 16 );
    assert.ok( plan.relayTicket.some( ( byte ) => byte !== 0 ) );

    // Joined from the same box: the local addresses first.
    const neighbour = new Game( lobby, '203.0.113.1' );
    neighbour.hello();
    assert.deepEqual( neighbour.join( { inviteCode : status.inviteCode } ).candidates[0], { kind : JoinCandidateKind.LOCAL, address : '192.168.1.30', port : 47219 } );
} );

test( 'private addresses are neither probed nor given, unless allowed', async () => {
    const { lobby, probes, settle } = makeLobby( { allowPrivateAddresses : false } );

    const host = new Game( lobby, '192.168.1.30' );
    host.hello();
    const status = host.publish( { publicAddresses : [ '10.0.0.1', '2001:db8::1', '2a01:cb00::1' ] } );

    assert.deepEqual(
        probes.map( ( probe ) => probe.address ),
        [ '2a01:cb00::1' ],
    );
    probes[0].resolve( true );
    await settle();

    const guest = new Game( lobby, '2a01:e0a::5' );
    guest.hello();
    assert.deepEqual(
        guest.join( { roomId : status.roomId } ).candidates.map( ( candidate ) => candidate.address ),
        [ '2a01:cb00::1' ],
    );
} );

test( 'a guest who joins during the probe gets the plan when it is over', async () => {
    const { lobby, probes, settle } = makeLobby();

    const host = new Game( lobby, '203.0.113.1' );
    host.hello();
    const status = host.publish();

    const guest = new Game( lobby, '198.51.100.7' );
    guest.hello();
    assert.equal( guest.join( { roomId : status.roomId } ), undefined );

    probes[0].resolve( true );
    await settle();
    const [plan] = guest.take( MessageType.JOIN_PLAN );
    assert.equal( plan.result, JoinResult.READY );
    assert.deepEqual(
        plan.candidates.map( ( candidate ) => candidate.address ),
        [ '203.0.113.1' ],
    );
} );

test( 'rooms shown only to invited players', async () => {
    const { lobby, probes, settle } = makeLobby();

    const watcher = new Game( lobby, '203.0.113.9' );
    watcher.hello();
    watcher.send( MessageType.ROOM_LIST_SUBSCRIBE, { isSubscribed : true } );

    const host = new Game( lobby, '203.0.113.1' );
    host.hello();
    const status = host.publish( { visibility : RoomVisibility.INVITE_ONLY } );
    probes[0].resolve( true );
    await settle();

    assert.equal( watcher.take( MessageType.ROOM_INFO ).length, 0 );

    const guest = new Game( lobby, '198.51.100.7' );
    guest.hello();
    assert.equal( guest.join( { roomId : status.roomId } ).result, JoinResult.NO_SUCH_ROOM );
    assert.equal( guest.join( { inviteCode : status.inviteCode } ).result, JoinResult.READY );
    assert.equal( guest.join( { inviteCode : 'ABCDEF' } ).result, JoinResult.NO_SUCH_ROOM );
} );

test( 'games of another version cannot join', async () => {
    const { lobby, probes, settle } = makeLobby();

    const host = new Game( lobby, '203.0.113.1' );
    host.hello();
    const status = host.publish();
    probes[0].resolve( true );
    await settle();

    for ( const game of [{...GAME, buildId : 1n }, {...GAME, protocolVersion : 17 }, {...GAME, version : [ 1, 1, 18 ] }] ) {
        const guest = new Game( lobby, '198.51.100.7' );
        guest.hello( 'Marc', game );
        assert.equal( guest.join( { roomId : status.roomId } ).result, JoinResult.OTHER_VERSION );
    }

    // A build which could not be identified never plays with anyone.
    const unknownHost = new Game( lobby, '203.0.113.2' );
    unknownHost.hello( 'Marc', {...GAME, buildId : 0n } );
    const unknownStatus = unknownHost.publish();
    probes[1].resolve( true );
    await settle();

    const unknownGuest = new Game( lobby, '198.51.100.8' );
    unknownGuest.hello( 'Marc', {...GAME, buildId : 0n } );
    assert.equal( unknownGuest.join( { roomId : unknownStatus.roomId } ).result, JoinResult.OTHER_VERSION );

    const self = host.join( { roomId : status.roomId } );
    assert.equal( self.result, JoinResult.INVALID_MESSAGE );
} );

test( 'a room in game leaves the list and comes back', async () => {
    const { lobby, probes, settle } = makeLobby();

    const host = new Game( lobby, '203.0.113.1' );
    host.hello();
    const status = host.publish();
    probes[0].resolve( true );
    await settle();

    const watcher = new Game( lobby, '203.0.113.9' );
    watcher.hello();
    watcher.send( MessageType.ROOM_LIST_SUBSCRIBE, { isSubscribed : true } );
    assert.equal( watcher.take( MessageType.ROOM_LIST )[0].rooms.length, 1 );

    host.send( MessageType.ROOM_UPDATE, { state : RoomState.IN_GAME, game : ROOM_GAME } );
    assert.deepEqual( watcher.take( MessageType.ROOM_REMOVED ), [ { roomId : status.roomId } ] );

    const guest = new Game( lobby, '198.51.100.7' );
    guest.hello();
    assert.equal( guest.join( { roomId : status.roomId } ).result, JoinResult.ROOM_IN_GAME );
    assert.deepEqual( lobby.stats(), { playersOnline : 3, waitingRooms : 0, roomsInGame : 1 } );

    host.send( MessageType.ROOM_UPDATE, { state : RoomState.WAITING_FOR_PLAYERS, game : {...ROOM_GAME, day : 8 } } );
    assert.equal( watcher.take( MessageType.ROOM_INFO ).at( -1 ).game.day, 8 );

    watcher.send( MessageType.ROOM_LIST_SUBSCRIBE, { isSubscribed : false } );
    host.send( MessageType.ROOM_CLOSE );
    assert.equal( watcher.take( MessageType.ROOM_REMOVED ).length, 0 );
    assert.equal( guest.join( { roomId : status.roomId } ).result, JoinResult.NO_SUCH_ROOM );
} );

test( 'a room stays a while after its host left, for the relay', async () => {
    const { lobby, probes, settle, closedRooms, advance } = makeLobby( { roomGraceMs : 120_000 } );

    const host = new Game( lobby, '203.0.113.1' );
    host.hello();
    const status = host.publish();
    probes[0].resolve( true );
    await settle();

    const watcher = new Game( lobby, '203.0.113.9' );
    watcher.hello();
    watcher.send( MessageType.ROOM_LIST_SUBSCRIBE, { isSubscribed : true } );

    lobby.disconnect( host.client );
    assert.deepEqual( watcher.take( MessageType.ROOM_REMOVED ), [ { roomId : status.roomId } ] );
    assert.ok( lobby.findRoom( status.roomId ) );

    const guest = new Game( lobby, '198.51.100.7' );
    guest.hello();
    assert.equal( guest.join( { roomId : status.roomId } ).result, JoinResult.NO_SUCH_ROOM );

    // The games keep pinging.
    advance( 60_000 );
    lobby.touch( watcher.client );
    lobby.touch( guest.client );
    advance( 61_000 );
    assert.deepEqual( closedRooms, [ status.roomId ] );
    assert.equal( lobby.findRoom( status.roomId ), null );
} );

test( 'rooms through the relay only, and lobbies without probe', async () => {
    {
        const { lobby, probes } = makeLobby();
        const host = new Game( lobby, '203.0.113.1' );
        host.hello();
        // Nothing to probe: the status says so at once.
        const status = host.publish( { isRelayOnly : true, publicAddresses : [ '2a01:cb00::1' ] } );
        assert.equal( probes.length, 0 );
        assert.deepEqual( status.reach, { isIPv4 : false, isIPv6 : false, isProbeDone : true, isRelayAvailable : true } );

        // Not even the local addresses: the guest must not learn any address of the host.
        const neighbour = new Game( lobby, '203.0.113.1' );
        neighbour.hello();
        assert.deepEqual( neighbour.join( { roomId : status.roomId } ).candidates, [] );
    } {
        const { lobby, probes } = makeLobby( { isProbeEnabled : false } ); const host = new Game( lobby, '203.0.113.1' ); host.hello();
        const status = host.publish( { publicAddresses : [ '2a01:cb00::1' ] } );
        assert.equal( probes.length, 0 );
        assert.deepEqual( status.reach, { isIPv4 : false, isIPv6 : false, isProbeDone : true, isRelayAvailable : true } );

        const guest = new Game( lobby, '198.51.100.7' );
        guest.hello();
        assert.deepEqual(
            guest.join( { roomId : status.roomId } ).candidates.map( ( candidate ) => candidate.address ),
            [ '2a01:cb00::1', '203.0.113.1' ],
        );
    } {
        const { lobby } = makeLobby( { isRelayEnabled : false, isProbeEnabled : false } );
        const host = new Game( lobby, '203.0.113.1' );
        host.hello();
        const status = host.publish();
        assert.equal( status.reach.isRelayAvailable, false );

        const guest = new Game( lobby, '198.51.100.7' );
        guest.hello();
        assert.deepEqual( guest.join( { roomId : status.roomId } ).relayTicket, new Uint8Array( 16 ) );
    }
} );

test( 'limits', () => {
    const { lobby, advance } = makeLobby( { maxRoomsPerAddress : 2, joinsPerMinute : 3, isProbeEnabled : false, helloTimeoutMs : 10_000, idleTimeoutMs : 90_000 } );

    const host = new Game( lobby, '203.0.113.1' );
    host.hello();
    const status = host.publish();
    assert.equal( host.publish().result, RoomStatusResult.INVALID_MESSAGE, 'a second room on the same connection' );

    const second = new Game( lobby, '203.0.113.1' );
    second.hello();
    assert.equal( second.publish().result, RoomStatusResult.PUBLISHED );
    const third = new Game( lobby, '203.0.113.1' );
    third.hello();
    assert.equal( third.publish().result, RoomStatusResult.TOO_MANY_ROOMS );

    // Invite codes cannot be guessed: after a few tries, even the right code gives nothing.
    const guesser = new Game( lobby, '198.51.100.7' );
    guesser.hello();
    assert.equal( guesser.join( { inviteCode : 'AAAAAA' } ).result, JoinResult.NO_SUCH_ROOM );
    assert.equal( guesser.join( { inviteCode : 'BBBBBB' } ).result, JoinResult.NO_SUCH_ROOM );
    assert.equal( guesser.join( { inviteCode : status.inviteCode } ).result, JoinResult.READY );
    assert.equal( guesser.join( { inviteCode : status.inviteCode } ).result, JoinResult.NO_SUCH_ROOM );
    advance( 61_000 );
    lobby.touch( host.client );
    lobby.touch( second.client );
    assert.equal( guesser.join( { inviteCode : status.inviteCode } ).result, JoinResult.READY );

    // Too many messages at once.
    const flooder = new Game( lobby, '198.51.100.8' );
    flooder.hello();
    for ( let i = 0; i < 50 && flooder.closeCode === null; ++i ) {
        flooder.send( MessageType.ROOM_LIST_SUBSCRIBE, { isSubscribed : false } );
    }
    assert.equal( flooder.closeCode, CloseCode.POLICY_VIOLATION );

    // No hello, and no news at all.
    const silent = new Game( lobby, '198.51.100.9' );
    advance( 11_000 );
    assert.equal( silent.closeCode, CloseCode.POLICY_VIOLATION );
    assert.equal( host.closeCode, null );
    advance( 80_000 );
    assert.equal( host.closeCode, CloseCode.GOING_AWAY );
} );

test( 'a long list comes in several parts', () => {
    const { lobby } = makeLobby( { isProbeEnabled : false, maxListedRooms : 230 } );

    for ( let i = 0; i < 250; ++i ) {
        const host = new Game( lobby, `10.0.${i >> 8}.${i & 0xff}` );
        host.hello();
        host.publish( { gameName : `Partie ${i}` } );
    }

    const watcher = new Game( lobby, '203.0.113.9' );
    watcher.hello();
    watcher.send( MessageType.ROOM_LIST_SUBSCRIBE, { isSubscribed : true } );
    const parts = watcher.take( MessageType.ROOM_LIST );
    assert.deepEqual(
        parts.map( ( part ) => [part.rooms.length, part.isLastPart] ),
        [
            [ 100, false ],
            [ 100, false ],
            [ 30, true ],
        ],
    );
    // The newest rooms first.
    assert.equal( parts[0].rooms[0].gameName, 'Partie 249' );
} );

test( 'the stats are sent when they change', () => {
    const { lobby, advance } = makeLobby( { isProbeEnabled : false, statsIntervalMs : 2000 } );

    const watcher = new Game( lobby, '203.0.113.9' );
    watcher.hello();
    watcher.send( MessageType.ROOM_LIST_SUBSCRIBE, { isSubscribed : true } );
    watcher.take( MessageType.LOBBY_STATS );

    advance( 2000 );
    const [first] = watcher.take( MessageType.LOBBY_STATS );
    assert.deepEqual( first, { playersOnline : 1, waitingRooms : 0, roomsInGame : 0 } );

    advance( 2000 );
    assert.equal( watcher.take( MessageType.LOBBY_STATS ).length, 0 );

    const host = new Game( lobby, '203.0.113.1' );
    host.hello();
    host.publish();
    advance( 2000 );
    assert.deepEqual( watcher.take( MessageType.LOBBY_STATS ), [ { playersOnline : 2, waitingRooms : 1, roomsInGame : 0 } ] );
} );

test( 'the settings of the lobby are checked', () => {
    assert.throws( () => makeLobby( { name : 'x'.repeat( 41 ) } ) );
    assert.throws( () => makeLobby( { messageOfTheDay : 'a\u0007b' } ) );
    assert.throws( () => makeLobby( { relayAddress : 'ws://élodie' } ) );
    assert.throws( () => makeLobby( { maxRoomsPerAddress : 0 } ) );
    assert.throws( () => makeLobby( { maxRoomsPerAddress : Number( 'trois' ) } ) );
} );

test( 'a host publishes its room again after a cut, or after a restart of the lobby', async () => {
    const { lobby, probes, settle, closedRooms, advance } = makeLobby( { roomGraceMs : 120_000 } );

    const host = new Game( lobby, '203.0.113.1' );
    host.hello();
    const status = host.publish();
    probes[0].resolve( true );
    await settle();

    const watcher = new Game( lobby, '203.0.113.9' );
    watcher.hello();
    watcher.send( MessageType.ROOM_LIST_SUBSCRIBE, { isSubscribed : true } );
    lobby.disconnect( host.client );
    assert.deepEqual( watcher.take( MessageType.ROOM_REMOVED ), [ { roomId : status.roomId } ] );

    const again = { roomId : status.roomId, inviteCode : status.inviteCode };

    // Only the host who has the token takes the room back.
    const stranger = new Game( lobby, '198.51.100.7' );
    stranger.hello();
    assert.equal( stranger.publish( {...again, hostToken : new Uint8Array( 16 ) } ).result, RoomStatusResult.INVALID_MESSAGE );

    // The host comes back from another address: the same room, probed again from there.
    const back = new Game( lobby, '203.0.113.2' );
    back.hello();
    const backStatus = back.publish( {...again, state : RoomState.IN_GAME } );
    assert.equal( backStatus.result, RoomStatusResult.PUBLISHED );
    assert.equal( backStatus.roomId, status.roomId );
    assert.equal( backStatus.inviteCode, status.inviteCode );
    assert.equal( backStatus.reach.isProbeDone, false );
    assert.equal( probes.at( -1 ).address, '203.0.113.2' );
    // In game: not in the list.
    assert.equal( watcher.take( MessageType.ROOM_INFO ).length, 0 );
    probes.at( -1 ).resolve( true );
    await settle();
    assert.equal( back.take( MessageType.ROOM_STATUS ).at( -1 ).reach.isIPv4, true );

    // A connection of the same host not known as lost yet gives the room up.
    const third = new Game( lobby, '203.0.113.3' );
    third.hello();
    assert.equal( third.publish( {...again, state : RoomState.IN_GAME } ).result, RoomStatusResult.PUBLISHED );
    lobby.disconnect( back.client );
    assert.ok( lobby.findRoom( status.roomId ).hostToken );
    advance( 130_000 );
    lobby.touch( third.client );
    lobby.touch( watcher.client );
    assert.ok( lobby.findRoom( status.roomId ).hostToken, 'the room has a host' );
    assert.deepEqual( closedRooms, [] );

    // After a restart, the lobby knows nothing: the room comes back with the same id and code.
    const restarted = makeLobby();
    const host2 = new Game( restarted.lobby, '203.0.113.1' );
    host2.hello();
    const recreated = host2.publish( {...again, state : RoomState.IN_GAME } );
    assert.equal( recreated.result, RoomStatusResult.PUBLISHED );
    assert.equal( recreated.roomId, status.roomId );
    assert.equal( recreated.inviteCode, status.inviteCode );
    assert.deepEqual( restarted.lobby.findRoom( status.roomId ), { hostToken : HOST_TOKEN, guestTicket : GUEST_TICKET } );
} );

test( 'a room closed by its host is remembered a while', () => {
    const { lobby, advance } = makeLobby( { isProbeEnabled : false, closedRoomMemoryMs : 600_000 } );

    const host = new Game( lobby, '203.0.113.1' );
    host.hello();
    const status = host.publish();
    host.send( MessageType.ROOM_CLOSE );
    assert.deepEqual( lobby.findRoom( status.roomId ), { isClosed : true } );

    // Published again on purpose: a new room.
    const again = host.publish( { roomId : status.roomId, inviteCode : status.inviteCode } );
    assert.notEqual( again.roomId, status.roomId );

    // A room whose host only left is not closed on purpose.
    const other = new Game( lobby, '203.0.113.2' );
    other.hello();
    const otherStatus = other.publish();
    lobby.disconnect( other.client );

    advance( 130_000 );
    lobby.touch( host.client );
    assert.equal( lobby.findRoom( otherStatus.roomId ), null );
    assert.deepEqual( lobby.findRoom( status.roomId ), { isClosed : true } );

    advance( 500_000 );
    assert.equal( lobby.findRoom( status.roomId ), null );
} );
