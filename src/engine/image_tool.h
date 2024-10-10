/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "image.h"

namespace fheroes2
{
    struct ICNHeader;

    bool Save( const Image & image, const std::string & path );

    // Save an image into file. 'background' represents palette index from the original palette. Recommended value is 23.
    bool Save( const Image & image, const std::string & path, const uint8_t background );

    bool Load( const std::string & path, Image & image );

    Sprite decodeICNSprite( const uint8_t * data, const uint8_t * dataEnd, const ICNHeader & icnHeader );

    void decodeTILImages( const uint8_t * data, const size_t imageCount, const int32_t width, const int32_t height, std::vector<Image> & output );

    // By default only Bitmap (.bmp) format is supported.
    bool isPNGFormatSupported();
}
