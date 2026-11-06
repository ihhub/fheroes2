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

// Messages between a game and a lobby server. The game reads and writes exactly the same bytes
// (src/fheroes2/multi/multi_lobby_protocol.cpp). Each message is described with its reader and its writer below.
//
// Texts are UTF-8. Every reader takes the whole payload and checks every field: anything wrong throws a ProtocolError.
// Writers check their fields too, the lobby must never send something the game would refuse.

export const LOBBY_PROTOCOL_VERSION = 1;

export const HEADER_SIZE = 11;
export const MAX_PAYLOAD_SIZE = 64 * 1024;
export const MAX_ROOMS_PER_LIST = 100;
export const KEY_SIZE = 16;

export const INVITE_CODE_CHARACTERS = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
export const INVITE_CODE_SIZE = 6;

export const MessageType = Object.freeze( {
    LOBBY_HELLO : 2000,
    LOBBY_WELCOME : 2001,
    LOBBY_STATS : 2002,
    LOBBY_KEEPALIVE : 2003,
    ROOM_PUBLISH : 2010,
    ROOM_STATUS : 2011,
    ROOM_UPDATE : 2012,
    ROOM_CLOSE : 2013,
    ROOM_LIST_SUBSCRIBE : 2020,
    ROOM_LIST : 2021,
    ROOM_INFO : 2022,
    ROOM_REMOVED : 2023,
    ROOM_JOIN : 2030,
    JOIN_PLAN : 2031,
    RELAY_HOST : 2040,
    RELAY_GUEST : 2041,
    RELAY_READY : 2042,
    RELAY_REFUSED : 2043,
    LOBBY_PROBE : 2050,
    LOBBY_PROBE_REPLY : 2051,
} );

export const WelcomeResult = Object.freeze( {
    ACCEPTED : 0,
    INVALID_MESSAGE : 1,
    INCOMPATIBLE_VERSIONS : 2,
    NICKNAME_REFUSED : 3,
    LOBBY_FULL : 4,
    TOO_MANY_CONNECTIONS : 5,
} );

export const RoomTurns = Object.freeze( {ONE_AFTER_ANOTHER : 0, SIMULTANEOUS : 1} );
export const RoomVisibility = Object.freeze( {PUBLIC : 0, INVITE_ONLY : 1} );
export const RoomStatusResult = Object.freeze( {PUBLISHED : 0, INVALID_MESSAGE : 1, GAME_NAME_REFUSED : 2, TOO_MANY_ROOMS : 3} );
export const RoomState = Object.freeze( {WAITING_FOR_PLAYERS : 0, IN_GAME : 1} );
export const JoinCandidateKind = Object.freeze( {LOCAL : 0, IPV6 : 1, IPV4 : 2} );
export const JoinResult = Object.freeze( {READY : 0, NO_SUCH_ROOM : 1, ROOM_IN_GAME : 2, OTHER_VERSION : 3, INVALID_MESSAGE : 4} );
export const RelayRefusal = Object.freeze( {INVALID_MESSAGE : 0, NO_SUCH_ROOM : 1, NO_HOST_WAITING : 2, LIMIT_REACHED : 3, ROOM_CLOSED : 4} );

// WebSocket close codes used by the lobby.
export const CloseCode = Object.freeze( {
    NORMAL : 1000,
    GOING_AWAY : 1001,
    PROTOCOL_ERROR : 1002,
    UNSUPPORTED_DATA : 1003,
    POLICY_VIOLATION : 1008,
    TOO_BIG : 1009,
    INTERNAL_ERROR : 1011,
} );

// Size in bytes and in characters.
export const TextLimit = Object.freeze( {
    NICKNAME : {maxBytes : 64, minCharacters : 1, maxCharacters : 20},
    GAME_NAME : {maxBytes : 120, minCharacters : 1, maxCharacters : 40},
    MAP_NAME : {maxBytes : 120, minCharacters : 0, maxCharacters : 40},
    LOBBY_NAME : {maxBytes : 120, minCharacters : 0, maxCharacters : 40},
    MESSAGE_OF_THE_DAY : {maxBytes : 1000, minCharacters : 0, maxCharacters : 300},
    PLATFORM : {maxBytes : 32, minCharacters : 0, maxCharacters : 32},
    LANGUAGE : {maxBytes : 8, minCharacters : 0, maxCharacters : 8, isAsciiOnly : true},
    ADDRESS : {maxBytes : 64, minCharacters : 1, maxCharacters : 64, isAsciiOnly : true},
    RELAY_ADDRESS : {maxBytes : 120, minCharacters : 0, maxCharacters : 120, isAsciiOnly : true},
} );

const MAX_PUBLIC_ADDRESSES = 3;
const MAX_LOCAL_ADDRESSES = 8;
const MAX_JOIN_CANDIDATES = 8;

const MARKER = [ 0x66, 0x68, 0x32 ]; // fh2

export class ProtocolError extends Error
{
    constructor( message )
    {
        super( message );
        this.name = 'ProtocolError';
    }
}

const fail = ( message ) => { throw new ProtocolError( message ); };

const utf8Decoder = new TextDecoder( 'utf-8', { fatal : true, ignoreBOM : true } );
const utf8Encoder = new TextEncoder();

// Same rules as the game: valid UTF-8 (the decoder refuses overlong forms and surrogates), no control character.
function checkText( text, bytes, limit, name )
{
    if ( bytes.length > limit.maxBytes ) {
        fail( `${name}: too long` );
    }
    if ( limit.isAsciiOnly && bytes.some( ( byte ) => byte >= 0x80 ) ) {
        fail( `${name}: not ASCII` );
    }
    if ( /[\u0000-\u001F\u007F-\u009F]/u.test( text ) ) {
        fail( `${name}: control character` );
    }
    const characters = [...text ].length;
    if ( characters < limit.minCharacters || characters > limit.maxCharacters ) {
        fail( `${name}: ${characters} characters` );
    }
}

export function isValidText( text, limit )
{
    try {
        if ( typeof text !== 'string' || text.isWellFormed?.() === false ) {
            return false;
        }
        checkText( text, utf8Encoder.encode( text ), limit, 'text' );
        return true;
    }
    catch {
        return false;
    }
}

export function isValidInviteCode( code )
{
    return typeof code === 'string' && code.length === INVITE_CODE_SIZE && [...code ].every( ( c ) => INVITE_CODE_CHARACTERS.includes( c ) );
}

class Writer
{
    #bytes = new Uint8Array( 256 );
    #size = 0;

    #reserve( count )
    {
        if ( this.#size + count > this.#bytes.length ) {
            const bigger = new Uint8Array( Math.max( this.#bytes.length * 2, this.#size + count ) );
            bigger.set( this.#bytes.subarray( 0, this.#size ) );
            this.#bytes = bigger;
        }
        const offset = this.#size;
        this.#size += count;
        return offset;
    }

    #number( value, size, max, name )
    {
        if ( !Number.isInteger( value ) || value < 0 || value > max ) {
            fail( `${name}: ${value} is not a ${size * 8}-bit number` );
        }
        const offset = this.#reserve( size );
        for ( let i = size - 1; i >= 0; --i ) {
            this.#bytes[offset + i] = value & 0xff;
            value = Math.floor( value / 256 );
        }
    }

    u8( value, name = 'u8' )
    {
        this.#number( value, 1, 0xff, name );
    }

    u16( value, name = 'u16' )
    {
        this.#number( value, 2, 0xffff, name );
    }

    u32( value, name = 'u32' )
    {
        this.#number( value, 4, 0xffffffff, name );
    }

    u64( value, name = 'u64' )
    {
        if ( typeof value !== 'bigint' || value < 0n || value > 0xffffffffffffffffn ) {
            fail( `${name}: not a 64-bit number` );
        }
        const offset = this.#reserve( 8 );
        new DataView( this.#bytes.buffer ).setBigUint64( offset, value );
    }

    bool( value, name = 'bool' )
    {
        if ( typeof value !== 'boolean' ) {
            fail( `${name}: not a boolean` );
        }
        this.u8( value ? 1 : 0 );
    }

    enum
    ( value, values, name )
    {
        if ( !Object.values( values ).includes( value ) ) {
            fail( `${name}: unknown value ${value}` );
        }
        this.u8( value );
    }

    key( value, name = 'key' )
    {
        if ( !( value instanceof Uint8Array ) || value.length !== KEY_SIZE ) {
            fail( `${name}: not ${KEY_SIZE} bytes` );
        }
        const offset = this.#reserve( KEY_SIZE );
        this.#bytes.set( value, offset );
    }

    raw( bytes )
    {
        // Reserved first: the array may be replaced by a bigger one.
        const offset = this.#reserve( bytes.length );
        this.#bytes.set( bytes, offset );
    }

    text( value, limit, name )
    {
        if ( typeof value !== 'string' || value.isWellFormed?.() === false ) {
            fail( `${name}: not a string` );
        }
        const bytes = utf8Encoder.encode( value );
        checkText( value, bytes, limit, name );
        this.u32( bytes.length );
        this.raw( bytes );
    }

    list( items, maxCount, writeItem, name )
    {
        if ( !Array.isArray( items ) || items.length > maxCount ) {
            fail( `${name}: more than ${maxCount} items` );
        }
        this.u32( items.length );
        for ( const item of items ) {
            writeItem( item );
        }
    }

    finish()
    {
        return this.#bytes.slice( 0, this.#size );
    }
}

class Reader
{
    #bytes;
    #view;
    #offset = 0;

    constructor( bytes )
    {
        this.#bytes = bytes;
        this.#view = new DataView( bytes.buffer, bytes.byteOffset, bytes.byteLength );
    }

    #take( count, name )
    {
        if ( count > this.#bytes.length - this.#offset ) {
            fail( `${name}: missing data` );
        }
        const offset = this.#offset;
        this.#offset += count;
        return offset;
    }

    u8( name = 'u8' )
    {
        return this.#view.getUint8( this.#take( 1, name ) );
    }

    u16( name = 'u16' )
    {
        return this.#view.getUint16( this.#take( 2, name ) );
    }

    u32( name = 'u32' )
    {
        return this.#view.getUint32( this.#take( 4, name ) );
    }

    u64( name = 'u64' )
    {
        return this.#view.getBigUint64( this.#take( 8, name ) );
    }

    bool( name = 'bool' )
    {
        const value = this.u8( name );
        if ( value > 1 ) {
            fail( `${name}: ${value} is not a boolean` );
        }
        return value === 1;
    }

    enum
    ( values, name )
    {
        const value = this.u8( name );
        if ( !Object.values( values ).includes( value ) ) {
            fail( `${name}: unknown value ${value}` );
        }
        return value;
    }

    key( name = 'key' )
    {
        const offset = this.#take( KEY_SIZE, name );
        return this.#bytes.slice( offset, offset + KEY_SIZE );
    }

    string( maxBytes, name )
    {
        const size = this.u32( name );
        if ( size > maxBytes ) {
            fail( `${name}: too long` );
        }
        const offset = this.#take( size, name );
        const bytes = this.#bytes.subarray( offset, offset + size );
        try {
            return { text : utf8Decoder.decode( bytes ), bytes };
        }
        catch {
            return fail( `${name}: not UTF-8` );
        }
    }

    text( limit, name )
    {
        const { text, bytes } = this.string( limit.maxBytes, name );
        checkText( text, bytes, limit, name );
        return text;
    }

    count( maxCount, name )
    {
        const count = this.u32( name );
        if ( count > maxCount ) {
            fail( `${name}: ${count} items` );
        }
        return count;
    }

    list( maxCount, readItem, name )
    {
        const count = this.count( maxCount, name );
        const items = [];
        for ( let i = 0; i < count; ++i ) {
            items.push( readItem() );
        }
        return items;
    }

    end()
    {
        if ( this.#offset !== this.#bytes.length ) {
            fail( `${this.#bytes.length - this.#offset} bytes left` );
        }
    }
}

function writeGameVersion( w, game )
{
    if ( !Array.isArray( game.version ) || game.version.length !== 3 ) {
        fail( 'game version: not 3 numbers' );
    }
    game.version.forEach( ( number ) => w.u8( number, 'game version' ) );
    w.u32( game.protocolVersion, 'protocol version' );
    w.u64( game.buildId, 'build id' );
    w.text( game.platform, TextLimit.PLATFORM, 'platform' );
}

function readGameVersion( r )
{
    const version = [ r.u8(), r.u8(), r.u8() ];
    const protocolVersion = r.u32();
    const buildId = r.u64();
    const platform = r.text( TextLimit.PLATFORM, 'platform' );
    return { version, protocolVersion, buildId, platform };
}

function writeReach( w, reach )
{
    w.u8( ( reach.isIPv4 ? 1 : 0 ) | ( reach.isIPv6 ? 2 : 0 ) | ( reach.isProbeDone ? 4 : 0 ) | ( reach.isRelayAvailable ? 8 : 0 ) );
}

function readReach( r )
{
    const byte = r.u8( 'reach' );
    if ( byte >= 16 ) {
        fail( `reach: unknown bits ${byte}` );
    }
    return { isIPv4 : ( byte & 1 ) !== 0, isIPv6 : ( byte & 2 ) !== 0, isProbeDone : ( byte & 4 ) !== 0, isRelayAvailable : ( byte & 8 ) !== 0 };
}

function checkRoomGame( game )
{
    if ( game.mapWidth === 0 || game.mapHeight === 0 ) {
        fail( 'room game: empty map' );
    }
    if ( game.humanPlayers > game.players || game.guestHumanPlayers > game.humanPlayers ) {
        fail( 'room game: wrong number of players' );
    }
    if ( game.day === 0 ) {
        fail( 'room game: day 0' );
    }
}

function writeRoomGame( w, game )
{
    checkRoomGame( game );
    w.text( game.mapName, TextLimit.MAP_NAME, 'map name' );
    w.u16( game.mapWidth, 'map width' );
    w.u16( game.mapHeight, 'map height' );
    w.u8( game.players, 'players' );
    w.u8( game.humanPlayers, 'human players' );
    w.u8( game.guestHumanPlayers, 'guest human players' );
    w.enum( game.turns, RoomTurns, 'turns' );
    w.u32( game.day, 'day' );
}

function readRoomGame( r )
{
    const game = {
        mapName : r.text( TextLimit.MAP_NAME, 'map name' ),
        mapWidth : r.u16(),
        mapHeight : r.u16(),
        players : r.u8(),
        humanPlayers : r.u8(),
        guestHumanPlayers : r.u8(),
        turns : r.enum( RoomTurns, 'turns' ),
        day : r.u32(),
    };
    checkRoomGame( game );
    return game;
}

function writeRoomInfo( w, room )
{
    if ( room.roomId === 0n ) {
        fail( 'room id: 0' );
    }
    w.u64( room.roomId, 'room id' );
    w.enum( room.state, RoomState, 'state' );
    writeReach( w, room.reach );
    w.text( room.gameName, TextLimit.GAME_NAME, 'game name' );
    w.text( room.hostNickname, TextLimit.NICKNAME, 'host nickname' );
    w.text( room.hostLanguage, TextLimit.LANGUAGE, 'host language' );
    writeGameVersion( w, room.hostGame );
    writeRoomGame( w, room.game );
}

function readRoomInfo( r )
{
    const roomId = r.u64( 'room id' );
    if ( roomId === 0n ) {
        fail( 'room id: 0' );
    }
    return {
        roomId,
        state : r.enum( RoomState, 'state' ),
        reach : readReach( r ),
        gameName : r.text( TextLimit.GAME_NAME, 'game name' ),
        hostNickname : r.text( TextLimit.NICKNAME, 'host nickname' ),
        hostLanguage : r.text( TextLimit.LANGUAGE, 'host language' ),
        hostGame : readGameVersion( r ),
        game : readRoomGame( r ),
    };
}

function readInviteCode( r, allowEmpty )
{
    const { text } = r.string( INVITE_CODE_SIZE, 'invite code' );
    if ( !isValidInviteCode( text ) && !( allowEmpty && text === '' ) ) {
        fail( 'invite code: wrong characters' );
    }
    return text;
}

function writeKeyMessage( w, fields )
{
    w.key( fields.key );
}

function readKeyMessage( r )
{
    return { key : r.key() };
}

// For each message type: how to write its fields and how to read them back.
const codecs = {
    [MessageType.LOBBY_HELLO] : {
        write( w, hello ) {
            w.u32( hello.lobbyProtocolVersion, 'lobby protocol version' );
            writeGameVersion( w, hello.game );
            w.text( hello.language, TextLimit.LANGUAGE, 'language' );
            w.text( hello.nickname, TextLimit.NICKNAME, 'nickname' );
        },
        read : ( r ) => ( {
            lobbyProtocolVersion : r.u32(),
            game : readGameVersion( r ),
            language : r.text( TextLimit.LANGUAGE, 'language' ),
            nickname : r.text( TextLimit.NICKNAME, 'nickname' ),
        } ),
    },
    [MessageType.LOBBY_WELCOME] : {
        write( w, welcome ) {
            w.enum( welcome.result, WelcomeResult, 'result' );
            w.u32( welcome.lobbyProtocolVersion, 'lobby protocol version' );
            w.text( welcome.lobbyName, TextLimit.LOBBY_NAME, 'lobby name' );
            w.text( welcome.messageOfTheDay, TextLimit.MESSAGE_OF_THE_DAY, 'message of the day' );
            w.text( welcome.relayAddress, TextLimit.RELAY_ADDRESS, 'relay address' );
        },
        read : ( r ) => ( {
            result : r.enum( WelcomeResult, 'result' ),
            lobbyProtocolVersion : r.u32(),
            lobbyName : r.text( TextLimit.LOBBY_NAME, 'lobby name' ),
            messageOfTheDay : r.text( TextLimit.MESSAGE_OF_THE_DAY, 'message of the day' ),
            relayAddress : r.text( TextLimit.RELAY_ADDRESS, 'relay address' ),
        } ),
    },
    [MessageType.LOBBY_STATS] : {
        write( w, stats ) {
            w.u32( stats.playersOnline, 'players online' );
            w.u32( stats.waitingRooms, 'waiting rooms' );
            w.u32( stats.roomsInGame, 'rooms in game' );
        },
        read : ( r ) => ( { playersOnline : r.u32(), waitingRooms : r.u32(), roomsInGame : r.u32() } ),
    },
    [MessageType.LOBBY_KEEPALIVE] : { write() {}, read : () => ( {} ) },
    [MessageType.ROOM_PUBLISH] : {
        write( w, publish ) {
            w.enum( publish.visibility, RoomVisibility, 'visibility' );
            w.bool( publish.isRelayOnly, 'relay only' );
            w.text( publish.gameName, TextLimit.GAME_NAME, 'game name' );
            writeRoomGame( w, publish.game );
            if ( publish.gamePort === 0 ) {
                fail( 'game port: 0' );
            }
            w.u16( publish.gamePort, 'game port' );
            w.key( publish.probeKey, 'probe key' );
            w.list( publish.publicAddresses, MAX_PUBLIC_ADDRESSES, ( address ) => w.text( address, TextLimit.ADDRESS, 'address' ), 'public addresses' );
            w.list( publish.localAddresses, MAX_LOCAL_ADDRESSES, ( address ) => w.text( address, TextLimit.ADDRESS, 'address' ), 'local addresses' );
            // A room published again after a cut, or after a restart of the lobby, keeps its id and its code.
            if ( ( publish.roomId === 0n ) !== ( publish.inviteCode === '' ) || ( publish.inviteCode !== '' && !isValidInviteCode( publish.inviteCode ) ) ) {
                fail( 'room publish: give both a room id and its invite code, or none' );
            }
            w.u64( publish.roomId, 'room id' );
            w.u32( publish.inviteCode.length );
            w.raw( utf8Encoder.encode( publish.inviteCode ) );
            w.enum( publish.state, RoomState, 'state' );
            w.key( publish.hostToken, 'host token' );
            w.key( publish.guestTicket, 'guest ticket' );
        },
        read( r ) {
            const publish = {
                visibility : r.enum( RoomVisibility, 'visibility' ),
                isRelayOnly : r.bool( 'relay only' ),
                gameName : r.text( TextLimit.GAME_NAME, 'game name' ),
                game : readRoomGame( r ),
                gamePort : r.u16(),
                probeKey : r.key(),
                publicAddresses : r.list( MAX_PUBLIC_ADDRESSES, () => r.text( TextLimit.ADDRESS, 'address' ), 'public addresses' ),
                localAddresses : r.list( MAX_LOCAL_ADDRESSES, () => r.text( TextLimit.ADDRESS, 'address' ), 'local addresses' ),
                roomId : r.u64( 'room id' ),
                inviteCode : readInviteCode( r, true ),
                state : r.enum( RoomState, 'state' ),
                hostToken : r.key(),
                guestTicket : r.key(),
            };
            if ( publish.gamePort === 0 ) {
                fail( 'game port: 0' );
            }
            if ( ( publish.roomId === 0n ) !== ( publish.inviteCode === '' ) ) {
                fail( 'room publish: give both a room id and its invite code, or none' );
            }
            return publish;
        },
    },
    [MessageType.ROOM_STATUS] : {
        write( w, status ) {
            w.enum( status.result, RoomStatusResult, 'result' );
            if ( status.result !== RoomStatusResult.PUBLISHED ) {
                return;
            }
            if ( status.roomId === 0n || !isValidInviteCode( status.inviteCode ) ) {
                fail( 'room status: no room id or wrong invite code' );
            }
            w.u64( status.roomId, 'room id' );
            w.u32( status.inviteCode.length );
            w.raw( utf8Encoder.encode( status.inviteCode ) );
            writeReach( w, status.reach );
        },
        read( r ) {
            const result = r.enum( RoomStatusResult, 'result' );
            if ( result !== RoomStatusResult.PUBLISHED ) {
                return { result };
            }
            const roomId = r.u64( 'room id' );
            if ( roomId === 0n ) {
                fail( 'room id: 0' );
            }
            return { result, roomId, inviteCode : readInviteCode( r, false ), reach : readReach( r ) };
        },
    },
    [MessageType.ROOM_UPDATE] : {
        write( w, update ) {
            w.enum( update.state, RoomState, 'state' );
            writeRoomGame( w, update.game );
        },
        read : ( r ) => ( { state : r.enum( RoomState, 'state' ), game : readRoomGame( r ) } ),
    },
    [MessageType.ROOM_CLOSE] : { write() {}, read : () => ( {} ) },
    [MessageType.ROOM_LIST_SUBSCRIBE] : {
        write( w, subscribe ) {
            w.bool( subscribe.isSubscribed, 'subscribed' );
        },
        read : ( r ) => ( { isSubscribed : r.bool( 'subscribed' ) } ),
    },
    [MessageType.ROOM_LIST] : {
        write( w, list ) {
            w.bool( list.isLastPart, 'last part' );
            w.list( list.rooms, MAX_ROOMS_PER_LIST, ( room ) => writeRoomInfo( w, room ), 'rooms' );
        },
        read : ( r ) => ( { isLastPart : r.bool( 'last part' ), rooms : r.list( MAX_ROOMS_PER_LIST, () => readRoomInfo( r ), 'rooms' ) } ),
    },
    [MessageType.ROOM_INFO] : {
        write( w, room ) {
            writeRoomInfo( w, room );
        },
        read : ( r ) => readRoomInfo( r ),
    },
    [MessageType.ROOM_REMOVED] : {
        write( w, removed ) {
            if ( removed.roomId === 0n ) {
                fail( 'room id: 0' );
            }
            w.u64( removed.roomId, 'room id' );
        },
        read( r ) {
            const roomId = r.u64( 'room id' );
            if ( roomId === 0n ) {
                fail( 'room id: 0' );
            }
            return { roomId };
        },
    },
    [MessageType.ROOM_JOIN] : {
        write( w, join ) {
            w.u64( join.roomId, 'room id' );
            w.u32( join.inviteCode.length );
            w.raw( utf8Encoder.encode( join.inviteCode ) );
        },
        read( r ) {
            const roomId = r.u64( 'room id' );
            const inviteCode = readInviteCode( r, true );
            // Either a room id or an invite code.
            if ( ( roomId === 0n ) === ( inviteCode === '' ) ) {
                fail( 'room join: give a room id or an invite code' );
            }
            return { roomId, inviteCode };
        },
    },
    [MessageType.JOIN_PLAN] : {
        write( w, plan ) {
            w.enum( plan.result, JoinResult, 'result' );
            if ( plan.result !== JoinResult.READY ) {
                return;
            }
            writeRoomInfo( w, plan.room );
            w.list(
                plan.candidates,
                MAX_JOIN_CANDIDATES,
                ( candidate ) => {
                    w.enum( candidate.kind, JoinCandidateKind, 'candidate kind' );
                    w.text( candidate.address, TextLimit.ADDRESS, 'address' );
                    if ( candidate.port === 0 ) {
                        fail( 'candidate port: 0' );
                    }
                    w.u16( candidate.port, 'port' );
                },
                'candidates',
            );
            w.key( plan.relayTicket, 'relay ticket' );
        },
        read( r ) {
            const result = r.enum( JoinResult, 'result' );
            if ( result !== JoinResult.READY ) {
                return { result };
            }
            const room = readRoomInfo( r );
            const candidates = r.list(
                MAX_JOIN_CANDIDATES,
                () => {
                    const candidate = { kind : r.enum( JoinCandidateKind, 'candidate kind' ), address : r.text( TextLimit.ADDRESS, 'address' ), port : r.u16() };
                    if ( candidate.port === 0 ) {
                        fail( 'candidate port: 0' );
                    }
                    return candidate;
                },
                'candidates',
            );
            return { result, room, candidates, relayTicket : r.key() };
        },
    },
    [MessageType.RELAY_HOST] : { write : writeKeyMessage, read : readKeyMessage },
    [MessageType.RELAY_GUEST] : { write : writeKeyMessage, read : readKeyMessage },
    [MessageType.RELAY_READY] : { write() {}, read : () => ( {} ) },
    [MessageType.RELAY_REFUSED] : {
        write( w, refused ) {
            w.enum( refused.reason, RelayRefusal, 'reason' );
        },
        read : ( r ) => ( { reason : r.enum( RelayRefusal, 'reason' ) } ),
    },
    [MessageType.LOBBY_PROBE] : {
        write( w, probe ) {
            w.key( probe.probeKey, 'probe key' );
            w.key( probe.value, 'probe value' );
        },
        read : ( r ) => ( { probeKey : r.key(), value : r.key() } ),
    },
    [MessageType.LOBBY_PROBE_REPLY] : { write : writeKeyMessage, read : readKeyMessage },
};

export function encodePayload( type, fields = {} )
{
    const codec = codecs[type] ?? fail( `unknown message type ${type}` );
    const w = new Writer();
    codec.write( w, fields );
    const payload = w.finish();
    if ( payload.length > MAX_PAYLOAD_SIZE ) {
        fail( `message ${type}: ${payload.length} bytes` );
    }
    return payload;
}

export function decodePayload( type, payload )
{
    const codec = codecs[type] ?? fail( `unknown message type ${type}` );
    if ( payload.length > MAX_PAYLOAD_SIZE ) {
        fail( `message ${type}: ${payload.length} bytes` );
    }
    const r = new Reader( payload );
    const fields = codec.read( r );
    r.end();
    return fields;
}

// Header and payload, as one WebSocket frame.
export function encodeMessage( type, fields = {} )
{
    const payload = encodePayload( type, fields );
    const message = new Uint8Array( HEADER_SIZE + payload.length );
    const view = new DataView( message.buffer );
    message.set( MARKER, 0 );
    view.setUint32( 3, type );
    view.setUint32( 7, payload.length );
    message.set( payload, HEADER_SIZE );
    return message;
}

// Returns the type and the size of the payload, or throws for a wrong header.
export function decodeHeader( bytes )
{
    if ( bytes.length < HEADER_SIZE || MARKER.some( ( byte, i ) => bytes[i] !== byte ) ) {
        fail( 'not a fheroes2 message' );
    }
    const view = new DataView( bytes.buffer, bytes.byteOffset, HEADER_SIZE );
    const type = view.getUint32( 3 );
    const size = view.getUint32( 7 );
    if ( size > MAX_PAYLOAD_SIZE ) {
        fail( `message ${type}: ${size} bytes` );
    }
    return { type, size };
}

// One whole message, as received in one WebSocket frame.
export function decodeMessage( bytes )
{
    const { type, size } = decodeHeader( bytes );
    if ( bytes.length !== HEADER_SIZE + size ) {
        fail( `message ${type}: ${bytes.length - HEADER_SIZE} bytes instead of ${size}` );
    }
    return { type, fields : decodePayload( type, bytes.subarray( HEADER_SIZE ) ) };
}
