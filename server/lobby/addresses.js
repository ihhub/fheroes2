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

// IPv4 and IPv6 addresses as seen by the lobby, without any API of Node.js (the lobby also runs on Cloudflare).

// The 4 numbers of an IPv4 address, or null.
export function parseIPv4( text )
{
    const parts = text.split( '.' );
    if ( parts.length !== 4 || !parts.every( ( part ) => /^(0|[1-9][0-9]{0,2})$/.test( part ) && Number( part ) <= 255 ) ) {
        return null;
    }
    return parts.map( Number );
}

// The 8 numbers of an IPv6 address, or null. An IPv4 address at the end is accepted (::ffff:192.0.2.1). No zone.
export function parseIPv6( text )
{
    if ( !/^[0-9a-fA-F:.]+$/.test( text ) || text.length > 45 ) {
        return null;
    }

    const halves = text.split( '::' );
    if ( halves.length > 2 ) {
        return null;
    }

    // An IPv4 address can only end the address.
    const parseGroups = ( half, isLast ) => {
        if ( half === '' ) {
            return [];
        }
        const groups = [];
        const items = half.split( ':' );
        for ( let i = 0; i < items.length; ++i ) {
            const item = items[i];
            if ( isLast && i === items.length - 1 && item.includes( '.' ) ) {
                const ipv4 = parseIPv4( item );
                if ( !ipv4 ) {
                    return null;
                }
                groups.push( ( ipv4[0] << 8 ) | ipv4[1], ( ipv4[2] << 8 ) | ipv4[3] );
            }
            else if ( /^[0-9a-fA-F]{1,4}$/.test( item ) ) {
                groups.push( parseInt( item, 16 ) );
            }
            else {
                return null;
            }
        }
        return groups;
    };

    const head = parseGroups( halves[0], halves.length === 1 );
    const tail = halves.length === 2 ? parseGroups( halves[1], true ) : [];
    if ( !head || !tail ) {
        return null;
    }

    if ( halves.length === 1 ) {
        return head.length === 8 ? head : null;
    }

    const missing = 8 - head.length - tail.length;
    if ( missing < 1 ) {
        return null;
    }
    return [...head, ...Array( missing ).fill( 0 ), ...tail ];
}

const isMappedIPv4 = ( groups ) => groups.slice( 0, 5 ).every( ( group ) => group === 0 ) && groups[5] === 0xffff;

// The address as the lobby keeps it: an IPv4 address mapped in IPv6 (::ffff:192.0.2.1) becomes the IPv4 address, IPv6
// addresses are written in lower case. Returns null for anything else than an address.
export function normalizeAddress( text )
{
    if ( typeof text !== 'string' ) {
        return null;
    }
    if ( parseIPv4( text ) ) {
        return text;
    }
    const groups = parseIPv6( text );
    if ( !groups ) {
        return null;
    }
    if ( isMappedIPv4( groups ) ) {
        return [ groups[6] >> 8, groups[6] & 0xff, groups[7] >> 8, groups[7] & 0xff ].join( '.' );
    }
    return text.toLowerCase();
}

export function isIPv6( address )
{
    return parseIPv6( address ) !== null && parseIPv4( address ) === null;
}

// Addresses which can be reached from the internet: not private, local, shared by the provider, reserved for
// documentation, multicast...
export function isPublicAddress( address )
{
    const ipv4 = parseIPv4( address );
    if ( ipv4 ) {
        const [a, b] = ipv4;
        return !( a === 0 || a === 10 || a === 127 || ( a === 100 && b >= 64 && b <= 127 ) || ( a === 169 && b === 254 ) || ( a === 172 && b >= 16 && b <= 31 )
                  || ( a === 192 && b === 168 ) || ( a === 192 && b === 0 && ( ipv4[2] === 0 || ipv4[2] === 2 ) ) || ( a === 198 && ( b === 18 || b === 19 ) )
                  || ( a === 198 && b === 51 && ipv4[2] === 100 ) || ( a === 203 && b === 0 && ipv4[2] === 113 ) || a >= 224 );
    }

    const groups = parseIPv6( address );
    if ( !groups ) {
        return false;
    }
    if ( isMappedIPv4( groups ) ) {
        return isPublicAddress( normalizeAddress( address ) );
    }
    // Global unicast only (2000::/3), without documentation addresses (2001:db8::/32).
    return ( groups[0] & 0xe000 ) === 0x2000 && !( groups[0] === 0x2001 && groups[1] === 0x0db8 );
}

// Two games behind the same box: the same IPv4 address, or IPv6 addresses of the same local network (same /64 prefix).
export function isSameBox( first, second )
{
    if ( parseIPv4( first ) || parseIPv4( second ) ) {
        return first === second;
    }
    const a = parseIPv6( first );
    const b = parseIPv6( second );
    return a !== null && b !== null && a.slice( 0, 4 ).every( ( group, i ) => group === b[i] );
}
