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

#ifndef H2ZLIB_H
#define H2ZLIB_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "image.h"

class IStreamBase;
class OStreamBase;
class IStreamBuf;

namespace Compression
{
    // Unzips the input data and returns the uncompressed data or an empty vector in case of an error.
    // The 'realSize' parameter represents the planned size of the decompressed data and is optional
    // (it is only used to speed up the decompression process). If this parameter is omitted or set to
    // zero, the size of the decompressed data will be determined automatically.
    std::vector<uint8_t> unzipData( const uint8_t * src, const size_t srcSize, size_t realSize = 0 );

    // Zips the input data and returns the compressed data or an empty vector in case of an error.
    std::vector<uint8_t> zipData( const uint8_t * src, const size_t srcSize );

    // Reads & unzips the zipped chunk from the given input stream and writes it to the given output
    // stream. Returns true on success or false on error.
    bool unzipStream( IStreamBase & inputStream, OStreamBase & outputStream );

    // Zips the contents of the buffer from the current read position to the end of the buffer and writes
    // it to the given output stream. The current read position of the buffer does not change. Returns
    // true on success and false on error.
    bool zipStreamBuf( const IStreamBuf & inputStream, OStreamBase & outputStream );

    fheroes2::Image CreateImageFromZlib( int32_t width, int32_t height, const uint8_t * imageData, size_t imageSize, bool doubleLayer );
}

#endif
