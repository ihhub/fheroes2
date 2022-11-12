/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2022                                             *
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

#ifndef ENDIAN_H2_H
#define ENDIAN_H2_H

#if defined( __linux__ )
#include <endian.h> // IWYU pragma: export

#elif defined( __FreeBSD__ ) || defined( __OpenBSD__ )
#include <sys/endian.h> // IWYU pragma: export

#elif defined( _WIN32 )
#include <cstdlib>

#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#define BYTE_ORDER LITTLE_ENDIAN

#define htobe16( x ) _byteswap_ushort( x )
#define htole16( x ) ( x )
#define be16toh( x ) _byteswap_ushort( x )
#define le16toh( x ) ( x )
#define htobe32( x ) _byteswap_ulong( x )
#define htole32( x ) ( x )
#define be32toh( x ) _byteswap_ulong( x )
#define le32toh( x ) ( x )

#elif defined( __APPLE__ )
#include <libkern/OSByteOrder.h> // IWYU pragma: export
#define htobe16( x ) OSSwapHostToBigInt16( x )
#define htole16( x ) OSSwapHostToLittleInt16( x )
#define be16toh( x ) OSSwapBigToHostInt16( x )
#define le16toh( x ) OSSwapLittleToHostInt16( x )
#define htobe32( x ) OSSwapHostToBigInt32( x )
#define htole32( x ) OSSwapHostToLittleInt32( x )
#define be32toh( x ) OSSwapBigToHostInt32( x )
#define le32toh( x ) OSSwapLittleToHostInt32( x )

#elif defined( TARGET_PS_VITA )
#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#define BYTE_ORDER LITTLE_ENDIAN

#define htobe16( x ) __builtin_bswap16( x )
#define htole16( x ) ( x )
#define be16toh( x ) __builtin_bswap16( x )
#define le16toh( x ) ( x )
#define htobe32( x ) __builtin_bswap32( x )
#define htole32( x ) ( x )
#define be32toh( x ) __builtin_bswap32( x )
#define le32toh( x ) ( x )

#elif defined( TARGET_NINTENDO_SWITCH )
#include <machine/endian.h> // IWYU pragma: export
#define LITTLE_ENDIAN _LITTLE_ENDIAN
#define BIG_ENDIAN _BIG_ENDIAN
#define BYTE_ORDER _BYTE_ORDER
#define htobe16( x ) __bswap16( x )
#define htole16( x ) ( x )
#define be16toh( x ) __bswap16( x )
#define le16toh( x ) ( x )
#define htobe32( x ) __bswap32( x )
#define htole32( x ) ( x )
#define be32toh( x ) __bswap32( x )
#define le32toh( x ) ( x )

#else
#error "Unsupported platform"
#endif

#define IS_BIGENDIAN ( BYTE_ORDER == BIG_ENDIAN )
#endif
