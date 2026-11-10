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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.              *
 ***************************************************************************/

#pragma once

#include <cstdint>
#include <string_view>

#include "image.h"

namespace fheroes2
{
    class FontType;

    namespace CjkFont
    {
        // Returns true when the current game language requires CJK glyph rendering (Simplified Chinese at the moment).
        bool isCjkTextRenderingEnabled();

        // Returns true when the CJK TrueType font has been successfully loaded.
        bool isLoaded();

        // Number of bytes of the UTF-8 character which starts with the given byte. ASCII characters occupy 1 byte.
        uint8_t getUtf8CharSize( const uint8_t firstByte );

        // Decodes a UTF-8 character of the given byte size (from 2 to 4, as returned by getUtf8CharSize) into a Unicode code point.
        // Invalid sequences are decoded as the Unicode replacement character.
        uint32_t decodeUtf8Char( const uint8_t * data, const uint8_t size );

        // Returns true if the given Unicode code point can be rendered by the loaded CJK font.
        bool hasGlyph( const uint32_t codePoint );

        // Renders (lazily, with caching) and returns a sprite for the given Unicode code point.
        // Returns an empty sprite if the CJK font is not loaded or does not contain this code point.
        const Sprite & getGlyph( const uint32_t codePoint, const FontType & fontType );
    }
}
