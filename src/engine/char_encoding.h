/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

namespace Encoding
{
    enum class CodePage : uint8_t
    {
        ASCII,
        CP1250,
        CP1251,
        CP1252,
        CP1254,
        CP1258,
        ISO8859_16
    };

    bool utf8ToCodePoint( const uint8_t * data, size_t length, uint32_t & codePoint );

    uint8_t getCodePageCharacter( const uint32_t value, const CodePage codePage );

    bool isASCIICharacter( const uint32_t value );
}
