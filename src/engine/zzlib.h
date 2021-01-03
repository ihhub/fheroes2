/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#ifdef WITH_ZLIB

#include <iostream>
#include <vector>

#include "image.h"
#include "serialize.h"
#include "types.h"

std::vector<u8> zlibCompress( const u8 *, size_t srcsz );
std::vector<u8> zlibDecompress( const u8 *, size_t srcsz, size_t realsz = 0 );

#endif

class ZStreamFile : public StreamBuf
{
public:
    ZStreamFile() {}

    bool read( const std::string &, size_t offset = 0 );
    bool write( const std::string &, bool append = false ) const;
};

fheroes2::Image CreateImageFromZlib( int32_t width, int32_t height, const uint8_t * imageData, size_t imageSize );
fheroes2::Image CreateImageFromZlib( int32_t width, int32_t height, const uint8_t * imageData, size_t imageSize, const uint8_t * transformData, size_t transformSize );

class ImageZlib : public fheroes2::Image
{
    ImageZlib( int32_t width, int32_t height, const uint8_t * data, size_t size );
    virtual ~ImageZlib() {}
};

#endif
