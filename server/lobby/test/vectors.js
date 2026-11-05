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

// Writes the samples as test vectors for the game: node test/vectors.js --write
// The game reads them with --multi-selftest-lobby-protocol <vectors.txt>.

import {writeFileSync} from 'node:fs';
import {fileURLToPath} from 'node:url';

import {encodePayload} from '../protocol.js';

import {invalidSamples, validSamples} from './samples.js';

export const vectorsPath = fileURLToPath( new URL( 'vectors.txt', import.meta.url ) );

const toHex = ( bytes ) => Buffer.from( bytes ).toString( 'hex' );

export function makeVectors()
{
    const lines = [
        '# Lobby messages written by server/lobby/protocol.js, read by the game with --multi-selftest-lobby-protocol <this file>.',
        '# Made by: node test/vectors.js --write',
        '# <name> <message type> <payload in hex, - if empty>. Messages whose name starts with ! must be refused by both.',
    ];

    for ( const [name, [ type, fields ]] of Object.entries( validSamples ) ) {
        lines.push( `${name} ${type} ${toHex( encodePayload( type, fields ) ) || '-'}` );
    }
    for ( const [name, [ type, payload ]] of Object.entries( invalidSamples ) ) {
        lines.push( `!${name} ${type} ${toHex( payload ) || '-'}` );
    }

    return lines.join( '\n' ) + '\n';
}

if ( process.argv[1] === fileURLToPath( import.meta.url ) && process.argv.includes( '--write' ) ) {
    writeFileSync( vectorsPath, makeVectors() );
    console.log( `Written: ${vectorsPath}` );
}
