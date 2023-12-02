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

#include "char_encoding.h"

#include <array>
#include <cassert>

namespace
{
    // All code page related array indexes are equal to index + 128 value in their code pages.
    // For example, a first element in an array is actually value 128 in their code page.
    const std::array<uint32_t, 128> cp1251CodePoints{ 0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, 0x20AC, 0x2030, 0x0409, 0x2039, 0x040A,
                                                      0x040C, 0x040B, 0x040F, 0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x0000, 0x2122,
                                                      0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F, 0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6,
                                                      0x00A7, 0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407, 0x00B0, 0x00B1, 0x0406, 0x0456,
                                                      0x0491, 0x00B5, 0x00B6, 0x00B7, 0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457, 0x0410,
                                                      0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D,
                                                      0x041E, 0x041F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A,
                                                      0x042B, 0x042C, 0x042D, 0x042E, 0x042F, 0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
                                                      0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0444,
                                                      0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F };

    uint8_t codePointToCP1251( const uint32_t value )
    {
        if ( value < 0x80 ) {
            // This is an ASCII character that is a part of this code page.
            return static_cast<uint8_t>( value );
        }

        // TODO: optimize the code to avoid looping.
        for ( size_t i = 0; i < cp1251CodePoints.size(); ++i ) {
            if ( cp1251CodePoints[i] == value ) {
                return static_cast<uint8_t>( i + 128 );
            }
        }

        // This is an invalid character.
        return 0;
    }
}

namespace Encoding
{
    bool utf8ToCodePoint( const uint8_t * data, size_t length, uint32_t & codePoint )
    {
        if ( data == nullptr || length == 0 ) {
            // Why are you trying to decode empty data?
            assert( 0 );
            return false;
        }

        if ( length > 4 ) {
            // Length cannot be longer than 4.
            length = 4;
        }

        if ( data[0] < 0x80 ) {
            // This is an ASCII character. No need further processing.
            codePoint = data[0];
            return true;
        }

        if ( length < 2 || ( data[1] >> 6 ) != 2 ) {
            // This is an invalid character.
            return false;
        }

        if ( ( data[0] >> 5 ) == 6 ) {
            // This is a 2 byte character.
            codePoint = ( data[1] & 0x3F ) | ( data[0] & 0x1F ) << 6;
            return true;
        }

        if ( length < 3 || ( data[2] >> 6 ) != 2 ) {
            // This is an invalid character.
            return false;
        }

        if ( ( data[0] >> 4 ) == 14 ) {
            // This is a 3 byte character.
            codePoint = ( data[2] & 0x3F ) | ( ( data[1] & 0x3F ) << 6 ) | ( data[0] & 0x0F ) << 12;
            return true;
        }

        if ( length != 4 || ( data[3] >> 6 ) != 2 ) {
            // This is an invalid character.
            return false;
        }

        if ( ( data[0] >> 3 ) != 30 ) {
            // This is an invalid character.
            return false;
        }

        // This is a 4 byte character.
        codePoint = ( data[3] & 0x3F ) | ( ( data[2] & 0x3F ) << 6 ) | ( ( data[1] & 0x3F ) << 12 ) | ( data[0] & 0x07 ) << 18;
        return true;
    }

    uint8_t getCodePageCharacter( const uint32_t value, const CodePage codePage )
    {
        if ( codePage == CodePage::CP1251 ) {
            return codePointToCP1251( value );
        }

        if ( value < 0x80 ) {
            return static_cast<uint8_t>( value );
        }

        return 0;
    }

    bool isASCIICharacter( const uint32_t value )
    {
        return ( value < 0x80 );
    }
}
