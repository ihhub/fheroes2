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

// Messages written by the lobby and read back by the game, and wrong messages which both must refuse. The game has the
// same samples (runLobbyProtocolSelfTest in src/fheroes2/multi/multi_selftest.cpp).

import {
    encodePayload,
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

const key = ( first ) => Uint8Array.from( { length : 16 }, ( _, i ) => first + i );

const PROBE_KEY = key( 0x01 );
const HOST_TOKEN = key( 0xa0 );
const RELAY_TICKET = key( 0xb0 );
const PROBE_VALUE = key( 0xc0 );

const ROOM_ID = 0x1122334455667788n;

const PC_GAME = {
    version : [ 1, 1, 17 ],
    protocolVersion : 18,
    buildId : 0x0123456789abcdefn,
    platform : 'Windows x64'
};
const TABLET_GAME = {
    ...PC_GAME,
    platform : 'Android arm64'
};

const ROOM_GAME = {
    mapName : 'Brokenshores',
    mapWidth : 72,
    mapHeight : 72,
    players : 4,
    humanPlayers : 2,
    guestHumanPlayers : 1,
    turns : RoomTurns.SIMULTANEOUS,
    day : 1,
};

const ROOM_INFO = {
    roomId : ROOM_ID,
    state : RoomState.WAITING_FOR_PLAYERS,
    reach : { isIPv4 : false, isIPv6 : true, isProbeDone : true, isRelayAvailable : true },
    gameName : 'Duel 東京 𝄞',
    hostNickname : 'Élodie',
    hostLanguage : 'fr',
    hostGame : TABLET_GAME,
    game : ROOM_GAME,
};

const OTHER_ROOM_INFO = {
    roomId : 0x99n,
    state : RoomState.WAITING_FOR_PLAYERS,
    reach : { isIPv4 : false, isIPv6 : false, isProbeDone : true, isRelayAvailable : true },
    gameName : 'Entre amis',
    hostNickname : 'Marc',
    hostLanguage : 'en',
    hostGame : PC_GAME,
    game : { mapName : '', mapWidth : 144, mapHeight : 144, players : 6, humanPlayers : 6, guestHumanPlayers : 3, turns : RoomTurns.ONE_AFTER_ANOTHER, day : 112 },
};

const HELLO = {
    lobbyProtocolVersion : 1,
    game : PC_GAME,
    language : 'fr',
    nickname : 'Élodie'
};

const PUBLISH = {
    visibility : RoomVisibility.PUBLIC,
    isRelayOnly : false,
    gameName : 'La partie du samedi',
    game : ROOM_GAME,
    gamePort : 47219,
    probeKey : PROBE_KEY,
    publicAddresses : [ '2001:db8::25', '203.0.113.7' ],
    localAddresses : [ '192.168.1.30' ],
    roomId : 0n,
    inviteCode : '',
    state : RoomState.WAITING_FOR_PLAYERS,
    hostToken : HOST_TOKEN,
    guestTicket : RELAY_TICKET,
};

const JOIN_PLAN = {
    result : JoinResult.READY,
    room : ROOM_INFO,
    candidates : [
        { kind : JoinCandidateKind.LOCAL, address : '192.168.1.30', port : 47219 },
        { kind : JoinCandidateKind.IPV6, address : '2001:db8::25', port : 47219 },
        { kind : JoinCandidateKind.IPV4, address : '203.0.113.7', port : 47219 },
    ],
    relayTicket : RELAY_TICKET,
};

export const validSamples = {
    hello : [ MessageType.LOBBY_HELLO, HELLO ],
    // Every text as long as it can be.
    hello_longest : [ MessageType.LOBBY_HELLO, {...HELLO, language : 'pt_BR', nickname : 'É'.repeat( 20 )} ],
    welcome : [
        MessageType.LOBBY_WELCOME,
        {
            result : WelcomeResult.ACCEPTED,
            lobbyProtocolVersion : 1,
            lobbyName : 'Salon fheroes2',
            messageOfTheDay : 'Bienvenue ! Soyez courtois.',
            relayAddress : 'ws://relay.example.org:8080',
        },
    ],
    welcome_longest : [
        MessageType.LOBBY_WELCOME,
        {
            result : WelcomeResult.ACCEPTED,
            lobbyProtocolVersion : 1,
            lobbyName : 'L'.repeat( 40 ),
            messageOfTheDay : 'Bienvenue ! '.repeat( 25 ),
            relayAddress : `ws://${'r'.repeat( 115 )}`,
        },
    ],
    welcome_refused : [
        MessageType.LOBBY_WELCOME,
        {result : WelcomeResult.INCOMPATIBLE_VERSIONS, lobbyProtocolVersion : 3, lobbyName : 'Salon fheroes2', messageOfTheDay : '', relayAddress : ''},
    ],
    stats : [ MessageType.LOBBY_STATS, {playersOnline : 12, waitingRooms : 3, roomsInGame : 4} ],
    keepalive : [ MessageType.LOBBY_KEEPALIVE, {} ],
    publish : [ MessageType.ROOM_PUBLISH, PUBLISH ],
    publish_private : [
        MessageType.ROOM_PUBLISH,
        {
            ...PUBLISH,
            visibility : RoomVisibility.INVITE_ONLY,
            isRelayOnly : true,
            gameName : 'Entre amis',
            game : {...ROOM_GAME, turns : RoomTurns.ONE_AFTER_ANOTHER, day : 23},
            publicAddresses : [],
            localAddresses : [],
        },
    ],
    // Again after a cut, or after a restart of the lobby.
    publish_again : [ MessageType.ROOM_PUBLISH, {...PUBLISH, roomId : ROOM_ID, inviteCode : 'K7Q2MX', state : RoomState.IN_GAME} ],
    status : [
        MessageType.ROOM_STATUS,
        {
            result : RoomStatusResult.PUBLISHED,
            roomId : ROOM_ID,
            inviteCode : 'K7Q2MX',
            reach : {isIPv4 : true, isIPv6 : true, isProbeDone : true, isRelayAvailable : true},
        },
    ],
    status_refused : [ MessageType.ROOM_STATUS, {result : RoomStatusResult.TOO_MANY_ROOMS} ],
    update : [ MessageType.ROOM_UPDATE, {state : RoomState.IN_GAME, game : {...ROOM_GAME, day : 23}} ],
    close : [ MessageType.ROOM_CLOSE, {} ],
    subscribe : [ MessageType.ROOM_LIST_SUBSCRIBE, {isSubscribed : true} ],
    unsubscribe : [ MessageType.ROOM_LIST_SUBSCRIBE, {isSubscribed : false} ],
    list : [ MessageType.ROOM_LIST, {isLastPart : false, rooms : [ ROOM_INFO, OTHER_ROOM_INFO ]} ],
    list_empty : [ MessageType.ROOM_LIST, {isLastPart : true, rooms : []} ],
    info : [ MessageType.ROOM_INFO, ROOM_INFO ],
    removed : [ MessageType.ROOM_REMOVED, {roomId : ROOM_ID} ],
    join_id : [ MessageType.ROOM_JOIN, {roomId : ROOM_ID, inviteCode : ''} ],
    join_code : [ MessageType.ROOM_JOIN, {roomId : 0n, inviteCode : 'K7Q2MX'} ],
    plan : [ MessageType.JOIN_PLAN, JOIN_PLAN ],
    plan_relay_only : [ MessageType.JOIN_PLAN, {...JOIN_PLAN, candidates : []} ],
    plan_refused : [ MessageType.JOIN_PLAN, {result : JoinResult.NO_SUCH_ROOM} ],
    relay_host : [ MessageType.RELAY_HOST, {key : HOST_TOKEN} ],
    relay_guest : [ MessageType.RELAY_GUEST, {key : RELAY_TICKET} ],
    relay_ready : [ MessageType.RELAY_READY, {} ],
    relay_refused : [ MessageType.RELAY_REFUSED, {reason : RelayRefusal.NO_HOST_WAITING} ],
    relay_refused_closed : [ MessageType.RELAY_REFUSED, {reason : RelayRefusal.ROOM_CLOSED} ],
    probe : [ MessageType.LOBBY_PROBE, {probeKey : PROBE_KEY, value : PROBE_VALUE} ],
    probe_reply : [ MessageType.LOBBY_PROBE_REPLY, {key : PROBE_VALUE} ],
};

// Raw payloads, to write what the encoder would refuse.
const u8 = ( value ) => [value];
const u16 = ( value ) => [value >> 8, value & 0xff];
const u32 = ( value ) => [( value >>> 24 ) & 0xff, ( value >>> 16 ) & 0xff, ( value >>> 8 ) & 0xff, value & 0xff];
const u64 = ( value ) => [...u32( Number( value >> 32n ) ), ...u32( Number( value & 0xffffffffn ) )];
const bytes = ( list ) => [...u32( list.length ), ...list];
const text = ( value ) => bytes( [...new TextEncoder().encode( value ) ] );
const gameVersion = ( game = PC_GAME ) => [...game.version, ...u32( game.protocolVersion ), ...u64( game.buildId ), ...text( game.platform )];
const roomGame = ( fields = {} ) => {
    const game = {...ROOM_GAME, ...fields };
    return [
        ...text( game.mapName ),
        ...u16( game.mapWidth ),
        ...u16( game.mapHeight ),
        ...u8( game.players ),
        ...u8( game.humanPlayers ),
        ...u8( game.guestHumanPlayers ),
        ...u8( game.turns ),
        ...u32( game.day ),
    ];
};
const hello = ( nickname, language = text( 'fr' ) ) => [...u32( 1 ), ...gameVersion(), ...language, ...nickname];
const roomInfo = ( { roomId = ROOM_ID, state = 0, reach = 7 } = {} ) => [...u64( roomId ),
                                                                         ...u8( state ),
                                                                         ...u8( reach ),
                                                                         ...text( 'Duel' ),
                                                                         ...text( 'Marc' ),
                                                                         ...text( 'fr' ),
                                                                         ...gameVersion(),
                                                                         ...roomGame(),
];
const publish
    = ( { isRelayOnly = 0, game = roomGame(), port = 47219, publicAddresses = [], roomId = 0n, code = '', state = 0 } = {} ) => [...u8( 0 ),
                                                                                                                                 ...u8( isRelayOnly ),
                                                                                                                                 ...text( 'Duel' ),
                                                                                                                                 ...game,
                                                                                                                                 ...u16( port ),
                                                                                                                                 ...PROBE_KEY,
                                                                                                                                 ...u32( publicAddresses.length ),
                                                                                                                                 ...publicAddresses.flatMap( text ),
                                                                                                                                 ...u32( 0 ),
                                                                                                                                 ...u64( roomId ),
                                                                                                                                 ...text( code ),
                                                                                                                                 ...u8( state ),
                                                                                                                                 ...HOST_TOKEN,
                                                                                                                                 ...RELAY_TICKET,
];
const status = ( roomId, code ) => [...u8( 0 ), ...u64( roomId ), ...text( code ), ...u8( 15 )];
const plan = ( candidate ) => [...u8( 0 ), ...roomInfo(), ...u32( 1 ), ...candidate, ...RELAY_TICKET];

const validPayload = ( name ) => [...encodePayload( ...validSamples[name] )];

export const invalidSamples = {
    hello_truncated : [ MessageType.LOBBY_HELLO, validPayload( 'hello' ).slice( 0, -1 ) ],
    hello_extra_byte : [ MessageType.LOBBY_HELLO, [...validPayload( 'hello' ), 0 ] ],
    hello_overlong_utf8 : [ MessageType.LOBBY_HELLO, hello( bytes( [ 0xc1, 0x81 ] ) ) ],
    hello_surrogate : [ MessageType.LOBBY_HELLO, hello( bytes( [ 0xed, 0xa0, 0x80 ] ) ) ],
    hello_beyond_unicode : [ MessageType.LOBBY_HELLO, hello( bytes( [ 0xf4, 0x90, 0x80, 0x80 ] ) ) ],
    hello_cut_character : [ MessageType.LOBBY_HELLO, hello( bytes( [ 0x41, 0xc3 ] ) ) ],
    hello_tab : [ MessageType.LOBBY_HELLO, hello( text( 'a\tb' ) ) ],
    hello_c1_control : [ MessageType.LOBBY_HELLO, hello( bytes( [ 0x41, 0xc2, 0x85 ] ) ) ],
    hello_long_nickname : [ MessageType.LOBBY_HELLO, hello( text( 'ABCDEFGHIJKLMNOPQRSTU' ) ) ],
    hello_empty_nickname : [ MessageType.LOBBY_HELLO, hello( text( '' ) ) ],
    hello_language_not_ascii : [ MessageType.LOBBY_HELLO, hello( text( 'Marc' ), text( 'fé' ) ) ],
    hello_huge_string : [ MessageType.LOBBY_HELLO, [...u32( 1 ), 1, 1, 17, ...u32( 18 ), ...u64( 1n ), ...u32( 0xffffffff ) ] ],
    welcome_unknown_result : [ MessageType.LOBBY_WELCOME, [...u8( 6 ), ...u32( 1 ), ...text( '' ), ...text( '' ), ...text( '' ) ] ],
    welcome_long_message : [ MessageType.LOBBY_WELCOME, [...u8( 0 ), ...u32( 1 ), ...text( '' ), ...text( 'x'.repeat( 301 ) ), ...text( '' ) ] ],
    stats_short : [ MessageType.LOBBY_STATS, [...u32( 1 ), ...u32( 2 ) ] ],
    keepalive_not_empty : [ MessageType.LOBBY_KEEPALIVE, [ 0 ] ],
    publish_wrong_bool : [ MessageType.ROOM_PUBLISH, publish( {isRelayOnly : 2} ) ],
    publish_port_zero : [ MessageType.ROOM_PUBLISH, publish( {port : 0} ) ],
    publish_too_many_addresses : [ MessageType.ROOM_PUBLISH, publish( {publicAddresses : [ '1.1.1.1', '1.1.1.2', '1.1.1.3', '1.1.1.4' ]} ) ],
    publish_more_humans_than_players : [ MessageType.ROOM_PUBLISH, publish( {game : roomGame( {humanPlayers : 5} )} ) ],
    publish_more_guests_than_humans : [ MessageType.ROOM_PUBLISH, publish( {game : roomGame( {guestHumanPlayers : 3} )} ) ],
    publish_day_zero : [ MessageType.ROOM_PUBLISH, publish( {game : roomGame( {day : 0} )} ) ],
    publish_unknown_turns : [ MessageType.ROOM_PUBLISH, publish( {game : roomGame( {turns : 2} )} ) ],
    publish_empty_map : [ MessageType.ROOM_PUBLISH, publish( {game : roomGame( {mapWidth : 0} )} ) ],
    status_code_with_zero : [ MessageType.ROOM_STATUS, status( ROOM_ID, 'K7Q2M0' ) ],
    status_code_lowercase : [ MessageType.ROOM_STATUS, status( ROOM_ID, 'k7q2mx' ) ],
    status_code_short : [ MessageType.ROOM_STATUS, status( ROOM_ID, 'K7Q2M' ) ],
    status_room_zero : [ MessageType.ROOM_STATUS, status( 0n, 'K7Q2MX' ) ],
    status_refused_extra_byte : [ MessageType.ROOM_STATUS, [...u8( 3 ), 0 ] ],
    update_unknown_state : [ MessageType.ROOM_UPDATE, [...u8( 2 ), ...roomGame() ] ],
    close_not_empty : [ MessageType.ROOM_CLOSE, [ 0 ] ],
    publish_code_without_room : [ MessageType.ROOM_PUBLISH, publish( {code : 'K7Q2MX'} ) ],
    publish_room_without_code : [ MessageType.ROOM_PUBLISH, publish( {roomId : ROOM_ID} ) ],
    publish_wrong_code : [ MessageType.ROOM_PUBLISH, publish( {roomId : ROOM_ID, code : 'K7Q2M0'} ) ],
    publish_unknown_state : [ MessageType.ROOM_PUBLISH, publish( {state : 2} ) ],
    subscribe_wrong_bool : [ MessageType.ROOM_LIST_SUBSCRIBE, [ 2 ] ],
    list_too_many_rooms : [ MessageType.ROOM_LIST, [...u8( 1 ), ...u32( 101 ) ] ],
    list_missing_room : [ MessageType.ROOM_LIST, [...u8( 1 ), ...u32( 2 ), ...roomInfo() ] ],
    info_unknown_reach : [ MessageType.ROOM_INFO, roomInfo( {reach : 16} ) ],
    info_unknown_state : [ MessageType.ROOM_INFO, roomInfo( {state : 2} ) ],
    info_room_zero : [ MessageType.ROOM_INFO, roomInfo( {roomId : 0n} ) ],
    removed_room_zero : [ MessageType.ROOM_REMOVED, u64( 0n ) ],
    join_id_and_code : [ MessageType.ROOM_JOIN, [...u64( ROOM_ID ), ...text( 'K7Q2MX' ) ] ],
    join_nothing : [ MessageType.ROOM_JOIN, [...u64( 0n ), ...text( '' ) ] ],
    join_wrong_code : [ MessageType.ROOM_JOIN, [...u64( 0n ), ...text( 'K7Q2MI' ) ] ],
    plan_port_zero : [ MessageType.JOIN_PLAN, plan( [...u8( 0 ), ...text( '192.168.1.30' ), ...u16( 0 ) ] ) ],
    plan_unknown_kind : [ MessageType.JOIN_PLAN, plan( [...u8( 3 ), ...text( '192.168.1.30' ), ...u16( 47219 ) ] ) ],
    plan_address_not_ascii : [ MessageType.JOIN_PLAN, plan( [...u8( 0 ), ...text( 'héhé' ), ...u16( 47219 ) ] ) ],
    plan_empty_address : [ MessageType.JOIN_PLAN, plan( [...u8( 0 ), ...text( '' ), ...u16( 47219 ) ] ) ],
    plan_refused_extra_byte : [ MessageType.JOIN_PLAN, [...u8( 1 ), 0 ] ],
    relay_host_short_key : [ MessageType.RELAY_HOST, [...HOST_TOKEN.slice( 1 ) ] ],
    relay_ready_not_empty : [ MessageType.RELAY_READY, [ 0 ] ],
    relay_refused_unknown_reason : [ MessageType.RELAY_REFUSED, [ 5 ] ],
    probe_long : [ MessageType.LOBBY_PROBE, [...PROBE_KEY, ...PROBE_VALUE, 0 ] ],
};
