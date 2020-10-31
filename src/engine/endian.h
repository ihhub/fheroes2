#ifndef ENDIAN_H
#define ENDIAN_H

#if defined( __linux__ ) || defined( __MINGW32__ )
#include <endian.h>

#elif defined( __FreeBSD__ )
#include <sys/endian.h>

#elif defined( _WIN32 ) || defined( _WIN64 )
#include <stdlib.h>

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
#include <libkern/OSByteOrder.h>
#define htobe16( x ) OSSwapHostToBigInt16( x )
#define htole16( x ) OSSwapHostToLittleInt16( x )
#define be16toh( x ) OSSwapBigToHostInt16( x )
#define le16toh( x ) OSSwapLittleToHostInt16( x )
#define htobe32( x ) OSSwapHostToBigInt32( x )
#define htole32( x ) OSSwapHostToLittleInt32( x )
#define be32toh( x ) OSSwapBigToHostInt32( x )
#define le32toh( x ) OSSwapLittleToHostInt32( x )

#else
#error "add your platform here"
#endif

#define IS_BIGENDIAN ( BYTE_ORDER == BIG_ENDIAN )
#endif