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

#ifndef H2ZLIB_H
#define H2ZLIB_H

#include <cstddef>
#include <cstdint>
#include <string>

#include "image.h"
#include "serialize.h"

class ZStreamBuf : public StreamBuf
{
public:
    ZStreamBuf() = default;

    // Reads & unzips the zipped chunk from the specified file at the specified offset and appends
    // it to the end of the buffer. The current read position of the buffer does not change. Returns
    // true on success or false on error.
    bool read( const std::string & fn, const size_t offset = 0 );

    // Zips the contents of the buffer from the current read position to the end of the buffer and
    // writes (or appends) it to the specified file. The current read position of the buffer does
    // not change. Returns true on success and false on error.
    bool write( const std::string & fn, const bool append = false );
};

fheroes2::Image CreateImageFromZlib( int32_t width, int32_t height, const uint8_t * imageData, size_t imageSize, bool doubleLayer );

#endif
