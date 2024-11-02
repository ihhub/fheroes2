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
#ifndef H2COLOR_H
#define H2COLOR_H

#include <cstdint>
#include <string>
#include <vector>

class IStreamBase;
class OStreamBase;

class Kingdom;

namespace fheroes2
{
    const char * getBarrierColorName( const int color );
    const char * getTentColorName( const int color );
}

namespace Color
{
    // !!! IMPORTANT !!!
    // Do NOT change the order of the items as they are used for the map format.
    enum : uint8_t
    {
        NONE = 0x00,
        BLUE = 0x01,
        GREEN = 0x02,
        RED = 0x04,
        YELLOW = 0x08,
        ORANGE = 0x10,
        PURPLE = 0x20,
        UNUSED = 0x80,
        ALL = BLUE | GREEN | RED | YELLOW | ORANGE | PURPLE
    };

    std::string String( const int color );

    int GetIndex( const int color );

    int Count( const int colors );
    int GetFirst( const int colors );

    uint8_t IndexToColor( const int index );
}

class Colors : public std::vector<int>
{
public:
    explicit Colors( const int colors = Color::ALL );
};

class ColorBase
{
    int color;

    friend OStreamBase & operator<<( OStreamBase & stream, const ColorBase & col );
    friend IStreamBase & operator>>( IStreamBase & stream, ColorBase & col );

public:
    explicit ColorBase( const int col = Color::NONE )
        : color( col )
    {}

    bool isFriends( const int col ) const;

    Kingdom & GetKingdom() const;

    void SetColor( const int col );

    int GetColor() const
    {
        return color;
    }
};

#endif
