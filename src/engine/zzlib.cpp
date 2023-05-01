/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "zzlib.h"

#include <algorithm>
#include <cstring>
#include <ostream>
#include <vector>

#include <zconf.h>
#include <zlib.h>

#include "logging.h"

namespace
{
    constexpr uint16_t FORMAT_VERSION_0 = 0;

    std::vector<uint8_t> zlibDecompress( const uint8_t * src, const size_t srcSize, size_t realSize = 0 )
    {
        if ( src == nullptr || srcSize == 0 ) {
            return {};
        }

        const uLong srcSizeULong = static_cast<uLong>( srcSize );
        if ( srcSizeULong != srcSize ) {
            ERROR_LOG( "The size of the compressed data is too large" )
            return {};
        }

        std::vector<uint8_t> res( realSize );

        if ( realSize == 0 ) {
            constexpr size_t sizeMultiplier = 7;

            if ( srcSize > res.max_size() / sizeMultiplier ) {
                // If the multiplicated size is too large, let's start with the original size and see how it goes
                realSize = srcSize;
            }
            else {
                realSize = srcSize * sizeMultiplier;
            }

            res.resize( realSize );
        }

        uLong dstSizeULong = static_cast<uLong>( res.size() );
        if ( dstSizeULong != res.size() ) {
            ERROR_LOG( "The size of the decompressed data is too large" )
            return {};
        }

        int ret = Z_BUF_ERROR;
        while ( Z_BUF_ERROR == ( ret = uncompress( res.data(), &dstSizeULong, src, srcSizeULong ) ) ) {
            constexpr size_t sizeMultiplier = 2;

            // Avoid infinite loop due to unsigned overflow on multiplication
            if ( res.size() > res.max_size() / sizeMultiplier ) {
                ERROR_LOG( "The size of the decompressed data is too large" )
                return {};
            }

            res.resize( res.size() * sizeMultiplier );

            dstSizeULong = static_cast<uLong>( res.size() );
            if ( dstSizeULong != res.size() ) {
                ERROR_LOG( "The size of the decompressed data is too large" )
                return {};
            }
        }

        if ( ret != Z_OK ) {
            ERROR_LOG( "zlib error: " << ret )
            return {};
        }

        res.resize( dstSizeULong );

        return res;
    }

    std::vector<uint8_t> zlibCompress( const uint8_t * src, const size_t srcSize )
    {
        if ( src == nullptr || srcSize == 0 ) {
            return {};
        }

        const uLong srcSizeULong = static_cast<uLong>( srcSize );
        if ( srcSizeULong != srcSize ) {
            ERROR_LOG( "The size of the source data is too large" )
            return {};
        }

        std::vector<uint8_t> res( compressBound( srcSizeULong ) );

        uLong dstSizeULong = static_cast<uLong>( res.size() );
        if ( dstSizeULong != res.size() ) {
            ERROR_LOG( "The size of the compressed data is too large" )
            return {};
        }

        const int ret = compress( res.data(), &dstSizeULong, src, srcSizeULong );

        if ( ret != Z_OK ) {
            ERROR_LOG( "zlib error: " << ret )
            return {};
        }

        res.resize( dstSizeULong );

        return res;
    }
}

bool ZStreamBuf::read( const std::string & fn, const size_t offset /* = 0 */ )
{
    StreamFile sf;
    sf.setbigendian( true );

    if ( !sf.open( fn, "rb" ) ) {
        return false;
    }

    if ( offset ) {
        sf.seek( offset );
    }

    const uint32_t rawSize = sf.get32();
    const uint32_t zipSize = sf.get32();
    if ( zipSize == 0 ) {
        return false;
    }

    const uint16_t version = sf.get16();
    if ( version != FORMAT_VERSION_0 ) {
        return false;
    }

    sf.skip( 2 ); // Unused bytes

    const std::vector<uint8_t> zip = sf.getRaw( zipSize );
    const std::vector<uint8_t> raw = zlibDecompress( zip.data(), zip.size(), rawSize );
    if ( raw.size() != rawSize ) {
        return false;
    }

    putRaw( reinterpret_cast<const char *>( raw.data() ), raw.size() );

    return !fail();
}

bool ZStreamBuf::write( const std::string & fn, const bool append /* = false */ ) const
{
    StreamFile sf;
    sf.setbigendian( true );

    if ( !sf.open( fn, append ? "ab" : "wb" ) ) {
        return false;
    }

    const std::vector<uint8_t> zip = zlibCompress( data(), size() );
    if ( zip.empty() ) {
        return false;
    }

    sf.put32( static_cast<uint32_t>( size() ) );
    sf.put32( static_cast<uint32_t>( zip.size() ) );
    sf.put16( FORMAT_VERSION_0 );
    sf.put16( 0 ); // Unused bytes
    sf.putRaw( reinterpret_cast<const char *>( zip.data() ), zip.size() );

    return !sf.fail();
}

fheroes2::Image CreateImageFromZlib( int32_t width, int32_t height, const uint8_t * imageData, size_t imageSize, bool doubleLayer )
{
    if ( imageData == nullptr || imageSize == 0 || width <= 0 || height <= 0 )
        return fheroes2::Image();

    const std::vector<uint8_t> & uncompressedData = zlibDecompress( imageData, imageSize );
    if ( doubleLayer && ( uncompressedData.size() & 1 ) == 1 ) {
        return fheroes2::Image();
    }

    const size_t uncompressedSize = doubleLayer ? uncompressedData.size() / 2 : uncompressedData.size();

    if ( static_cast<size_t>( width * height ) != uncompressedSize )
        return fheroes2::Image();

    fheroes2::Image out( width, height );

    uint32_t * outImage = out.image(); 
    uint8_t * outTransform = out.transform();
    for ( size_t i = 0; i < uncompressedSize; ++i, ++outImage, ++outTransform ) {
        *outImage = static_cast<uint32_t>( uncompressedData.data()[i] );
        *outTransform = static_cast<uint8_t>( doubleLayer ? uncompressedData.data()[i + uncompressedSize] : 0 );
    }
    return out;
}
