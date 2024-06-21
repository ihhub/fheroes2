/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#include <zconf.h>
#include <zlib.h>

#include "logging.h"
#include "serialize.h"

namespace
{
    constexpr uint16_t FORMAT_VERSION_0 = 0;
}

namespace Compression
{
    std::vector<uint8_t> decompressData( const uint8_t * src, const size_t srcSize, size_t realSize /* = 0 */ )
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

    std::vector<uint8_t> compressData( const uint8_t * src, const size_t srcSize )
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

    bool readFromFileStream( StreamFile & fileStream, StreamBuf & output )
    {
        const uint32_t rawSize = fileStream.get32();
        const uint32_t zipSize = fileStream.get32();
        if ( zipSize == 0 ) {
            return false;
        }

        const uint16_t version = fileStream.get16();
        if ( version != FORMAT_VERSION_0 ) {
            return false;
        }

        fileStream.skip( 2 ); // Unused bytes

        const std::vector<uint8_t> zip = fileStream.getRaw( zipSize );
        const std::vector<uint8_t> raw = decompressData( zip.data(), zip.size(), rawSize );
        if ( raw.size() != rawSize ) {
            return false;
        }

        output.putRaw( raw.data(), raw.size() );

        return !output.fail();
    }

    bool writeIntoFileStream( StreamFile & fileStream, StreamBuf & data )
    {
        const std::vector<uint8_t> zip = compressData( data.data(), data.size() );
        if ( zip.empty() ) {
            return false;
        }

        fileStream.put32( static_cast<uint32_t>( data.size() ) );
        fileStream.put32( static_cast<uint32_t>( zip.size() ) );
        fileStream.put16( FORMAT_VERSION_0 );
        fileStream.put16( 0 ); // Unused bytes
        fileStream.putRaw( zip.data(), zip.size() );

        return !fileStream.fail();
    }

    fheroes2::Image CreateImageFromZlib( int32_t width, int32_t height, const uint8_t * imageData, size_t imageSize, bool doubleLayer )
    {
        if ( imageData == nullptr || imageSize == 0 || width <= 0 || height <= 0 ) {
            return {};
        }

        const std::vector<uint8_t> & uncompressedData = decompressData( imageData, imageSize );
        if ( doubleLayer && ( uncompressedData.size() & 1 ) == 1 ) {
            return {};
        }

        const size_t uncompressedSize = doubleLayer ? uncompressedData.size() / 2 : uncompressedData.size();

        if ( static_cast<size_t>( width ) * height != uncompressedSize ) {
            return {};
        }

        fheroes2::Image out;
        if ( !doubleLayer ) {
            out._disableTransformLayer();
        }
        out.resize( width, height );

        std::memcpy( out.image(), uncompressedData.data(), uncompressedSize );
        if ( doubleLayer ) {
            std::memcpy( out.transform(), uncompressedData.data() + uncompressedSize, uncompressedSize );
        }
        return out;
    }
}
