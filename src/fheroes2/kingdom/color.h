/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

class StreamBase;

namespace fheroes2
{
    enum ObjectColor
    {
        NONE = 0,
        AQUA = 1,
        BLUE = 2,
        BROWN = 3,
        GOLD = 4,
        GREEN = 5,
        ORANGE = 6,
        PURPLE = 7,
        RED = 8
    };
    const char * getBarrierColorName( const int color );
    const char * getTentColorName( const int color );
}

namespace Color
{
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

    std::string String( int );
    int Count( int );
    int GetIndex( int );
    int GetFirst( int );
    int FromInt( int );
}

class Colors : public std::vector<int>
{
public:
    explicit Colors( int = Color::ALL );
};

class Kingdom;

class ColorBase
{
    int color;

    friend StreamBase & operator<<( StreamBase &, const ColorBase & );
    friend StreamBase & operator>>( StreamBase &, ColorBase & );

public:
    explicit ColorBase( int col = Color::NONE )
        : color( col )
    {}

    bool isFriends( int ) const;
    void SetColor( int );

    Kingdom & GetKingdom() const;

    int GetColor() const
    {
        return color;
    }
};

StreamBase & operator<<( StreamBase &, const ColorBase & );
StreamBase & operator>>( StreamBase &, ColorBase & );

#endif
