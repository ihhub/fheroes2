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
import {readFileSync} from 'node:fs';
import {test} from 'node:test';

import {
    decodeMessage,
    decodePayload,
    encodeMessage,
    encodePayload,
    HEADER_SIZE,
    isValidInviteCode,
    isValidText,
    MAX_PAYLOAD_SIZE,
    MessageType,
    ProtocolError,
    TextLimit,
} from '../protocol.js';

import {invalidSamples, validSamples} from './samples.js';
import {makeVectors, vectorsPath} from './vectors.js';

test( 'every sample comes back the same', () => {
    for ( const [name, [ type, fields ]] of Object.entries( validSamples ) ) {
        assert.deepEqual( decodePayload( type, encodePayload( type, fields ) ), fields, name );
    }
} );

test( 'every message type has a sample', () => {
    const sampled = new Set( Object.values( validSamples ).map( ( [ type ] ) => type ) );
    for ( const [name, type] of Object.entries( MessageType ) ) {
        assert.ok( sampled.has( type ), name );
    }
} );

test( 'wrong messages are refused', () => {
    for ( const [name, [ type, payload ]] of Object.entries( invalidSamples ) ) {
        assert.throws( () => decodePayload( type, Uint8Array.from( payload ) ), ProtocolError, name );
    }
} );

test( 'the vectors read by the game are up to date', () => {
    const written = readFileSync( vectorsPath, 'utf8' ).replaceAll( '\r\n', '\n' );
    assert.equal( written, makeVectors(), 'run: node test/vectors.js --write' );
} );

test( 'header', () => {
    const message = encodeMessage( MessageType.LOBBY_STATS, { playersOnline : 1, waitingRooms : 2, roomsInGame : 3 } );
    assert.deepEqual( [...message.subarray( 0, HEADER_SIZE ) ], [ 0x66, 0x68, 0x32, 0, 0, 0x07, 0xd2, 0, 0, 0, 12 ] );
    assert.deepEqual( decodeMessage( message ), { type : MessageType.LOBBY_STATS, fields : { playersOnline : 1, waitingRooms : 2, roomsInGame : 3 } } );

    const wrongMarker = message.slice();
    wrongMarker[0] = 0x58;
    assert.throws( () => decodeMessage( wrongMarker ), ProtocolError );

    assert.throws( () => decodeMessage( message.subarray( 0, message.length - 1 ) ), ProtocolError, 'shorter than its header says' );
    assert.throws( () => decodeMessage( Uint8Array.from( [...message, 0 ] ) ), ProtocolError, 'longer than its header says' );
    assert.throws( () => decodeMessage( message.subarray( 0, 5 ) ), ProtocolError, 'no whole header' );

    const tooBig = message.slice();
    new DataView( tooBig.buffer ).setUint32( 7, MAX_PAYLOAD_SIZE + 1 );
    assert.throws( () => decodeMessage( tooBig ), ProtocolError, 'too big' );

    const unknownType = message.slice();
    new DataView( unknownType.buffer ).setUint32( 3, 1999 );
    assert.throws( () => decodeMessage( unknownType ), ProtocolError, 'unknown type' );
} );

test( 'the lobby never writes what the game would refuse', () => {
    const [helloType, hello] = validSamples.hello;
    assert.throws( () => encodePayload( helloType, {...hello, nickname : 'x'.repeat( 21 ) } ), ProtocolError );
    assert.throws( () => encodePayload( helloType, {...hello, nickname : '' } ), ProtocolError );
    assert.throws( () => encodePayload( helloType, {...hello, nickname : 'a\nb' } ), ProtocolError );
    assert.throws( () => encodePayload( helloType, {...hello, nickname : 'a\uD800b' } ), ProtocolError, 'lone surrogate' );
    assert.throws( () => encodePayload( helloType, {...hello, language : 'fé' } ), ProtocolError );

    const [statusType, status] = validSamples.status;
    assert.throws( () => encodePayload( statusType, {...status, inviteCode : 'K7Q2M0' } ), ProtocolError );
    assert.throws( () => encodePayload( statusType, {...status, roomId : 0n } ), ProtocolError );

    const [statsType] = validSamples.stats;
    assert.throws( () => encodePayload( statsType, { playersOnline : -1, waitingRooms : 0, roomsInGame : 0 } ), ProtocolError );
    assert.throws( () => encodePayload( statsType, { playersOnline : 2 ** 32, waitingRooms : 0, roomsInGame : 0 } ), ProtocolError );

    const [publishType, publish] = validSamples.publish;
    assert.throws( () => encodePayload( publishType, {...publish, hostToken : new Uint8Array( 15 ) } ), ProtocolError );
    assert.throws( () => encodePayload( publishType, {...publish, inviteCode : 'K7Q2MX' } ), ProtocolError );

    const [listType, list] = validSamples.list;
    assert.throws( () => encodePayload( listType, {...list, rooms : Array( 101 ).fill( list.rooms[0] ) } ), ProtocolError );

    assert.throws( () => encodePayload( 1999, {} ), ProtocolError, 'unknown type' );
} );

test( 'texts and invite codes', () => {
    assert.ok( isValidText( 'Élodie', TextLimit.NICKNAME ) );
    assert.ok( isValidText( '東京𝄞', TextLimit.NICKNAME ) );
    assert.ok( isValidText( 'x'.repeat( 20 ), TextLimit.NICKNAME ) );
    assert.ok( !isValidText( 'x'.repeat( 21 ), TextLimit.NICKNAME ) );
    assert.ok( !isValidText( '', TextLimit.NICKNAME ) );
    assert.ok( !isValidText( 'a\u0085', TextLimit.NICKNAME ) );
    // 20 characters but 80 bytes.
    assert.ok( !isValidText( '𝄞'.repeat( 20 ), TextLimit.NICKNAME ) );

    assert.ok( isValidInviteCode( 'K7Q2MX' ) );
    assert.ok( isValidInviteCode( 'ABCDEF' ) );
    for ( const code of ['K7Q2M', 'K7Q2MXX', 'K7Q2M0', 'K7Q2MO', 'K7Q2M1', 'K7Q2MI', 'k7q2mx', '', 'K7Q 2M'] ) {
        assert.ok( !isValidInviteCode( code ), code );
    }
} );
