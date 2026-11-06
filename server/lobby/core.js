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

// The lobby: connections of games, rooms, the list of rooms and the plans to join them. Nothing here depends on where
// the lobby runs: the server (server.js for Node.js) gives the connections, the clock, random bytes and the probe.
//
// A connection is { address, send(bytes), close(code) }. The server calls connect() for each new connection, receive()
// for each message, touch() when a pong comes, disconnect() when it is closed, and tick() every second.

import {isIPv6, isPublicAddress, isSameBox, normalizeAddress} from './addresses.js';
import {
    CloseCode,
    decodeMessage,
    encodeMessage,
    INVITE_CODE_CHARACTERS,
    INVITE_CODE_SIZE,
    isValidText,
    JoinCandidateKind,
    JoinResult,
    KEY_SIZE,
    LOBBY_PROTOCOL_VERSION,
    MAX_ROOMS_PER_LIST,
    MessageType,
    ProtocolError,
    RoomState,
    RoomStatusResult,
    RoomVisibility,
    TextLimit,
    WelcomeResult,
} from './protocol.js';

export const defaultSettings = Object.freeze( {
    name : 'fheroes2',
    messageOfTheDay : '',
    // Empty when the relay is at the address of the lobby.
    relayAddress : '',
    isRelayEnabled : true,
    isProbeEnabled : true,
    // Addresses of local networks are probed and given to guests too: for a lobby on a local network, and for tests.
    allowPrivateAddresses : false,
    // Nicknames and game names containing one of these words (in lower case) are refused.
    bannedWords : [],
    maxConnections : 2000,
    maxConnectionsPerAddress : 10,
    maxRooms : 1000,
    maxRoomsPerAddress : 3,
    maxListedRooms : 500,
    helloTimeoutMs : 10_000,
    // Games send a ping every 30 seconds.
    idleTimeoutMs : 90_000,
    // A room stays this long after its host left, so that relayed games go on.
    roomGraceMs : 120_000,
    // A room closed by its host is remembered this long.
    closedRoomMemoryMs : 600_000,
    joinsPerMinute : 12,
    messagesPerSecond : 20,
    messageBurst : 40,
    statsIntervalMs : 2000,
    probeTimeoutMs : 3000,
} );

// At most this number of addresses of a host are probed.
const MAX_PROBED_ADDRESSES = 3;
const MAX_JOIN_CANDIDATES = 8;

const ClientState = Object.freeze( { HELLO : 0, READY : 1, CLOSED : 2 } );

const isSameGame = ( first, second ) => first.version.every( ( number, i ) => number === second.version[i] ) && first.protocolVersion === second.protocolVersion
                                        && first.buildId !== 0n && first.buildId === second.buildId;

const keysEqual = ( first, second ) => first.length === second.length && first.reduce( ( difference, byte, i ) => difference | ( byte ^ second[i] ), 0 ) === 0;

const statsEqual = ( first, second ) => first !== null && first.playersOnline === second.playersOnline && first.waitingRooms === second.waitingRooms
                                        && first.roomsInGame === second.roomsInGame;

export class Lobby
{
    #settings;
    #platform;
    #clients = new Set();
    #rooms = new Map();
    #roomsByCode = new Map();
    #sentStats = null;
    #statsSentAt = 0;
    // Room id and when its host closed it.
    #closedRooms = new Map();
    // Order of publishing, the newest rooms come first in the list.
    #publishCount = 0;

    // platform: { now(), randomBytes(size), probe({ address, port, probeKey, value, timeoutMs }) giving a promise of a
    // boolean, onRoomClosed(roomId) (optional), log(text) (optional) }.
    constructor( settings, platform )
    {
        this.#settings = {...defaultSettings, ...settings };
        this.#platform = platform;

        const { name, messageOfTheDay, relayAddress } = this.#settings;
        if ( !isValidText( name, TextLimit.LOBBY_NAME ) || !isValidText( messageOfTheDay, TextLimit.MESSAGE_OF_THE_DAY )
             || !isValidText( relayAddress, TextLimit.RELAY_ADDRESS ) ) {
            throw new Error( 'the name, the message of the day or the relay address of the lobby is too long or has wrong characters' );
        }
        if ( !Number.isInteger( this.#settings.maxRoomsPerAddress ) || this.#settings.maxRoomsPerAddress < 1 ) {
            throw new Error( 'the number of games per address must be a whole number, 1 or more' );
        }
    }

    get settings()
    {
        return this.#settings;
    }

    stats()
    {
        let playersOnline = 0;
        for ( const client of this.#clients ) {
            if ( client.state === ClientState.READY ) {
                ++playersOnline;
            }
        }

        let waitingRooms = 0;
        let roomsInGame = 0;
        for ( const room of this.#rooms.values() ) {
            if ( room.state === RoomState.IN_GAME ) {
                ++roomsInGame;
            }
            else if ( room.host ) {
                ++waitingRooms;
            }
        }

        return { playersOnline, waitingRooms, roomsInGame };
    }

    // What the relay needs to know about a room: its keys, { isClosed: true } if its host closed it, or null if it does not
    // exist.
    findRoom( roomId )
    {
        const room = this.#rooms.get( roomId );
        if ( room ) {
            return { hostToken : room.hostToken, guestTicket : room.guestTicket };
        }
        return this.#closedRooms.has( roomId ) ? { isClosed : true } : null;
    }

    connect( connection )
    {
        const now = this.#platform.now();
        const client = {
            connection,
            address : normalizeAddress( connection.address ) ?? String( connection.address ),
            state : ClientState.HELLO,
            connectedAt : now,
            lastActivityAt : now,
            hello : null,
            room : null,
            isSubscribed : false,
            joinTimes : [],
            tokens : this.#settings.messageBurst,
            tokensAt : now,
        };
        this.#clients.add( client );
        return client;
    }

    receive( client, bytes )
    {
        if ( client.state === ClientState.CLOSED ) {
            return;
        }

        const now = this.#platform.now();
        client.lastActivityAt = now;

        if ( !this.#takeToken( client, now ) ) {
            this.#drop( client, CloseCode.POLICY_VIOLATION );
            return;
        }

        let message;
        try {
            message = decodeMessage( bytes );
        }
        catch ( error ) {
            if ( !( error instanceof ProtocolError ) ) {
                throw error;
            }
            this.#drop( client, CloseCode.PROTOCOL_ERROR );
            return;
        }

        if ( client.state === ClientState.HELLO ) {
            if ( message.type === MessageType.LOBBY_HELLO ) {
                this.#hello( client, message.fields );
            }
            else {
                this.#drop( client, CloseCode.PROTOCOL_ERROR );
            }
            return;
        }

        switch ( message.type ) {
        case MessageType.LOBBY_KEEPALIVE:
            // Nothing else to do: any message shows that the game is still there.
            return;
        case MessageType.ROOM_PUBLISH:
            this.#publish( client, message.fields );
            return;
        case MessageType.ROOM_UPDATE:
            this.#update( client, message.fields );
            return;
        case MessageType.ROOM_CLOSE:
            if ( client.room ) {
                this.#closedRooms.set( client.room.id, this.#platform.now() );
                this.#closeRoom( client.room );
            }
            return;
        case MessageType.ROOM_LIST_SUBSCRIBE:
            this.#subscribe( client, message.fields.isSubscribed );
            return;
        case MessageType.ROOM_JOIN:
            this.#join( client, message.fields );
            return;
        default:
            this.#drop( client, CloseCode.PROTOCOL_ERROR );
        }
    }

    touch( client )
    {
        client.lastActivityAt = this.#platform.now();
    }

    disconnect( client )
    {
        this.#forget( client );
    }

    tick()
    {
        const now = this.#platform.now();
        const settings = this.#settings;

        for ( const client of this.#clients ) {
            if ( client.state === ClientState.HELLO && now - client.connectedAt > settings.helloTimeoutMs ) {
                this.#drop( client, CloseCode.POLICY_VIOLATION );
            }
            else if ( now - client.lastActivityAt > settings.idleTimeoutMs ) {
                this.#drop( client, CloseCode.GOING_AWAY );
            }
        }

        for ( const room of [...this.#rooms.values()] ) {
            if ( !room.host && now - room.hostLeftAt > settings.roomGraceMs ) {
                this.#closeRoom( room );
            }
        }

        for ( const [roomId, closedAt] of [...this.#closedRooms] ) {
            if ( now - closedAt > settings.closedRoomMemoryMs ) {
                this.#closedRooms.delete( roomId );
            }
        }

        if ( now - this.#statsSentAt >= settings.statsIntervalMs ) {
            this.#statsSentAt = now;
            const stats = this.stats();
            if ( !statsEqual( this.#sentStats, stats ) ) {
                this.#sentStats = stats;
                this.#broadcast( MessageType.LOBBY_STATS, stats );
            }
        }
    }

    #send( client, type, fields )
    {
        if ( client.state !== ClientState.CLOSED ) {
            client.connection.send( encodeMessage( type, fields ) );
        }
    }

    #broadcast( type, fields )
    {
        const message = encodeMessage( type, fields );
        for ( const client of this.#clients ) {
            if ( client.isSubscribed && client.state === ClientState.READY ) {
                client.connection.send( message );
            }
        }
    }

    #takeToken( client, now )
    {
        const { messagesPerSecond, messageBurst } = this.#settings;
        client.tokens = Math.min( messageBurst, client.tokens + ( ( now - client.tokensAt ) * messagesPerSecond ) / 1000 );
        client.tokensAt = now;
        if ( client.tokens < 1 ) {
            return false;
        }
        client.tokens -= 1;
        return true;
    }

    #drop( client, code )
    {
        if ( client.state !== ClientState.CLOSED ) {
            this.#forget( client );
            client.connection.close( code );
        }
    }

    #forget( client )
    {
        if ( client.state === ClientState.CLOSED ) {
            return;
        }

        client.state = ClientState.CLOSED;
        client.isSubscribed = false;
        this.#clients.delete( client );

        for ( const room of this.#rooms.values() ) {
            room.pendingJoins = room.pendingJoins.filter( ( guest ) => guest !== client );
        }

        const room = client.room;
        if ( room ) {
            client.room = null;
            room.host = null;
            room.hostLeftAt = this.#platform.now();
            this.#refreshListing( room );
            this.#refusePendingJoins( room, JoinResult.NO_SUCH_ROOM );
        }
    }

    #hasBannedWord( text )
    {
        const lowerCase = text.toLowerCase();
        return this.#settings.bannedWords.some( ( word ) => lowerCase.includes( word ) );
    }

    #hello( client, hello )
    {
        const settings = this.#settings;
        let readyCount = 0;
        // This game and the ones already in.
        let sameAddressCount = 1;
        for ( const other of this.#clients ) {
            if ( other.state === ClientState.READY ) {
                ++readyCount;
                sameAddressCount += other.address === client.address ? 1 : 0;
            }
        }

        let result = WelcomeResult.ACCEPTED;
        if ( hello.lobbyProtocolVersion !== LOBBY_PROTOCOL_VERSION ) {
            result = WelcomeResult.INCOMPATIBLE_VERSIONS;
        }
        else if ( this.#hasBannedWord( hello.nickname ) ) {
            result = WelcomeResult.NICKNAME_REFUSED;
        }
        else if ( readyCount >= settings.maxConnections ) {
            result = WelcomeResult.LOBBY_FULL;
        }
        else if ( sameAddressCount > settings.maxConnectionsPerAddress ) {
            result = WelcomeResult.TOO_MANY_CONNECTIONS;
        }

        this.#send( client, MessageType.LOBBY_WELCOME, {
            result,
            lobbyProtocolVersion : LOBBY_PROTOCOL_VERSION,
            lobbyName : settings.name,
            messageOfTheDay : settings.messageOfTheDay,
            relayAddress : settings.relayAddress,
        } );

        if ( result !== WelcomeResult.ACCEPTED ) {
            this.#drop( client, CloseCode.NORMAL );
            return;
        }

        client.state = ClientState.READY;
        client.hello = hello;
    }

    #randomRoomId()
    {
        for ( ;; ) {
            const bytes = this.#platform.randomBytes( 8 );
            const roomId = new DataView( bytes.buffer, bytes.byteOffset, 8 ).getBigUint64( 0 );
            if ( roomId !== 0n && !this.#rooms.has( roomId ) ) {
                return roomId;
            }
        }
    }

    #randomInviteCode()
    {
        for ( ;; ) {
            // 32 characters: 5 bits of each byte, so every character is as likely.
            const code = [...this.#platform.randomBytes( INVITE_CODE_SIZE ) ].map( ( byte ) => INVITE_CODE_CHARACTERS[byte & 31] ).join( '' );
            if ( !this.#roomsByCode.has( code ) ) {
                return code;
            }
        }
    }

    #publish( client, publish )
    {
        const settings = this.#settings;

        const refuse = ( result ) => this.#send( client, MessageType.ROOM_STATUS, { result } );

        if ( client.room ) {
            refuse( RoomStatusResult.INVALID_MESSAGE );
            return;
        }
        if ( this.#hasBannedWord( publish.gameName ) ) {
            refuse( RoomStatusResult.GAME_NAME_REFUSED );
            return;
        }

        let room = publish.roomId !== 0n ? this.#rooms.get( publish.roomId ) : undefined;

        // Published again after a cut: only the host who has the token takes the room back. A connection of the same host
        // which is not known as lost yet gives it up.
        if ( room ) {
            if ( !keysEqual( publish.hostToken, room.hostToken ) ) {
                refuse( RoomStatusResult.INVALID_MESSAGE );
                return;
            }
            if ( room.host ) {
                room.host.room = null;
            }
        }
        else {
            let sameAddressCount = 0;
            for ( const other of this.#rooms.values() ) {
                sameAddressCount += other.hostAddress === client.address ? 1 : 0;
            }
            if ( sameAddressCount >= settings.maxRoomsPerAddress || this.#rooms.size >= settings.maxRooms ) {
                refuse( RoomStatusResult.TOO_MANY_ROOMS );
                return;
            }

            // After a restart of the lobby, the room comes back with the same id and code, so that the games going on find it.
            const isIdFree = publish.roomId !== 0n && !this.#closedRooms.has( publish.roomId );
            const isCodeFree = publish.inviteCode !== '' && !this.#roomsByCode.has( publish.inviteCode );

            room = {
                id : isIdFree ? publish.roomId : this.#randomRoomId(),
                inviteCode : isIdFree && isCodeFree ? publish.inviteCode : this.#randomInviteCode(),
                order : ++this.#publishCount,
                pendingJoins : [],
                isListed : false,
            };
            this.#rooms.set( room.id, room );
            this.#roomsByCode.set( room.inviteCode, room );
        }

        Object.assign( room, {
            hostToken : publish.hostToken,
            guestTicket : publish.guestTicket,
            host : client,
            hostAddress : client.address,
            hostGame : client.hello.game,
            hostNickname : client.hello.nickname,
            hostLanguage : client.hello.language,
            visibility : publish.visibility,
            isRelayOnly : publish.isRelayOnly,
            gameName : publish.gameName,
            game : publish.game,
            gamePort : publish.gamePort,
            probeKey : publish.probeKey,
            publicAddresses : publish.publicAddresses,
            localAddresses : publish.localAddresses,
            state : publish.state,
            reach : { isIPv4 : false, isIPv6 : false, isProbeDone : false, isRelayAvailable : settings.isRelayEnabled },
            directAddresses : [],
            hostLeftAt : 0,
        } );
        client.room = room;

        this.#sendStatus( room );
        this.#refreshListing( room );
        this.#probe( room );
    }

    #sendStatus( room )
    {
        if ( room.host ) {
            this.#send( room.host, MessageType.ROOM_STATUS, {
                result : RoomStatusResult.PUBLISHED,
                roomId : room.id,
                inviteCode : room.inviteCode,
                reach : room.reach,
            } );
        }
    }

    #probe( room )
    {
        const settings = this.#settings;

        const addresses = [];
        for ( const address of [room.hostAddress, ...room.publicAddresses.map( normalizeAddress )] ) {
            if ( address && !addresses.includes( address ) && ( settings.allowPrivateAddresses || isPublicAddress( address ) ) ) {
                addresses.push( address );
            }
        }
        addresses.splice( MAX_PROBED_ADDRESSES );

        if ( room.isRelayOnly ) {
            this.#endProbe( room, [], true );
            return;
        }

        if ( !settings.isProbeEnabled ) {
            // Given without check: guests try them before the relay.
            this.#endProbe( room, addresses, false );
            return;
        }

        // One address after another: the game of the host takes one new connection at a time.
        const value = this.#platform.randomBytes( KEY_SIZE );
        const reachable = [];
        const probeNext = ( index ) => {
            if ( this.#rooms.get( room.id ) !== room ) {
                return;
            }
            if ( index === addresses.length ) {
                this.#endProbe( room, reachable, true );
                return;
            }

            let probe;
            try {
                probe = Promise.resolve(
                    this.#platform.probe( { address : addresses[index], port : room.gamePort, probeKey : room.probeKey, value, timeoutMs : settings.probeTimeoutMs } ) );
            }
            catch {
                probe = Promise.resolve( false );
            }

            probe.catch( () => false ).then( ( isReachable ) => {
                if ( isReachable === true ) {
                    reachable.push( addresses[index] );
                }
                probeNext( index + 1 );
            } );
        };

        probeNext( 0 );
    }

    #endProbe( room, directAddresses, isChecked )
    {
        room.directAddresses = directAddresses;
        room.reach = {
            ...room.reach,
            isIPv4 : isChecked && directAddresses.some( ( address ) => !isIPv6( address ) ),
            isIPv6 : isChecked && directAddresses.some( ( address ) => isIPv6( address ) ),
            isProbeDone : true,
        };

        this.#sendStatus( room );
        this.#refreshListing( room );

        const guests = room.pendingJoins;
        room.pendingJoins = [];
        for ( const guest of guests ) {
            this.#sendPlan( guest, room );
        }
    }

    #isListed( room )
    {
        return room.host !== null && room.visibility === RoomVisibility.PUBLIC && room.state === RoomState.WAITING_FOR_PLAYERS;
    }

    #roomInfo( room )
    {
        return {
            roomId : room.id,
            state : room.state,
            reach : room.reach,
            gameName : room.gameName,
            hostNickname : room.hostNickname,
            hostLanguage : room.hostLanguage,
            hostGame : room.hostGame,
            game : room.game,
        };
    }

    // Tells the subscribers about a room which was added, changed or removed from the list.
    #refreshListing( room )
    {
        const isListed = this.#isListed( room );
        if ( isListed ) {
            this.#broadcast( MessageType.ROOM_INFO, this.#roomInfo( room ) );
        }
        else if ( room.isListed ) {
            this.#broadcast( MessageType.ROOM_REMOVED, { roomId : room.id } );
        }
        room.isListed = isListed;
    }

    #update( client, update )
    {
        const room = client.room;
        if ( !room ) {
            this.#drop( client, CloseCode.PROTOCOL_ERROR );
            return;
        }

        room.state = update.state;
        room.game = update.game;
        this.#refreshListing( room );

        if ( room.state === RoomState.IN_GAME ) {
            this.#refusePendingJoins( room, JoinResult.ROOM_IN_GAME );
        }
    }

    #closeRoom( room )
    {
        this.#rooms.delete( room.id );
        this.#roomsByCode.delete( room.inviteCode );

        if ( room.host ) {
            room.host.room = null;
            room.host = null;
        }
        this.#refreshListing( room );
        this.#refusePendingJoins( room, JoinResult.NO_SUCH_ROOM );

        this.#platform.onRoomClosed?.( room.id );
    }

    #refusePendingJoins( room, result )
    {
        const guests = room.pendingJoins;
        room.pendingJoins = [];
        for ( const guest of guests ) {
            this.#send( guest, MessageType.JOIN_PLAN, { result } );
        }
    }

    #subscribe( client, isSubscribed )
    {
        client.isSubscribed = isSubscribed;
        if ( !isSubscribed ) {
            return;
        }

        const rooms = [...this.#rooms.values() ]
                          .filter( ( room ) => this.#isListed( room ) )
                          .sort( ( first, second ) => second.order - first.order )
                          .slice( 0, this.#settings.maxListedRooms )
                          .map( ( room ) => this.#roomInfo( room ) );

        let start = 0;
        do {
            const part = rooms.slice( start, start + MAX_ROOMS_PER_LIST );
            start += MAX_ROOMS_PER_LIST;
            this.#send( client, MessageType.ROOM_LIST, { isLastPart : start >= rooms.length, rooms : part } );
        } while ( start < rooms.length );

        this.#send( client, MessageType.LOBBY_STATS, this.stats() );
    }

    #join( client, join )
    {
        const now = this.#platform.now();
        client.joinTimes = client.joinTimes.filter( ( time ) => now - time < 60_000 );

        // Invite codes cannot be guessed one after another. Nothing tells that the limit was reached.
        if ( client.joinTimes.length >= this.#settings.joinsPerMinute ) {
            this.#send( client, MessageType.JOIN_PLAN, { result : JoinResult.NO_SUCH_ROOM } );
            return;
        }
        client.joinTimes.push( now );

        const room = join.roomId !== 0n ? this.#rooms.get( join.roomId ) : this.#roomsByCode.get( join.inviteCode );

        let result = JoinResult.READY;
        // A room shown only to invited players is joined with its code.
        if ( !room || !room.host || ( join.roomId !== 0n && room.visibility !== RoomVisibility.PUBLIC ) ) {
            result = JoinResult.NO_SUCH_ROOM;
        }
        else if ( room.host === client ) {
            result = JoinResult.INVALID_MESSAGE;
        }
        else if ( room.state === RoomState.IN_GAME ) {
            result = JoinResult.ROOM_IN_GAME;
        }
        else if ( !isSameGame( client.hello.game, room.hostGame ) ) {
            result = JoinResult.OTHER_VERSION;
        }

        if ( result !== JoinResult.READY ) {
            this.#send( client, MessageType.JOIN_PLAN, { result } );
            return;
        }

        // The plan waits for the probe, which takes a few seconds at most.
        if ( !room.reach.isProbeDone ) {
            room.pendingJoins.push( client );
            return;
        }

        this.#sendPlan( client, room );
    }

    #sendPlan( client, room )
    {
        const candidates = [];

        if ( !room.isRelayOnly ) {
            if ( isSameBox( client.address, room.hostAddress ) ) {
                for ( const address of room.localAddresses ) {
                    candidates.push( { kind : JoinCandidateKind.LOCAL, address, port : room.gamePort } );
                }
            }

            const ipv6 = room.directAddresses.filter( ( address ) => isIPv6( address ) );
            const ipv4 = room.directAddresses.filter( ( address ) => !isIPv6( address ) );
            for ( const address of ipv6 ) {
                candidates.push( { kind : JoinCandidateKind.IPV6, address, port : room.gamePort } );
            }
            for ( const address of ipv4 ) {
                candidates.push( { kind : JoinCandidateKind.IPV4, address, port : room.gamePort } );
            }
        }

        this.#send( client, MessageType.JOIN_PLAN, {
            result : JoinResult.READY,
            room : this.#roomInfo( room ),
            candidates : candidates.slice( 0, MAX_JOIN_CANDIDATES ),
            relayTicket : this.#settings.isRelayEnabled ? room.guestTicket : new Uint8Array( KEY_SIZE ),
        } );
    }
}
