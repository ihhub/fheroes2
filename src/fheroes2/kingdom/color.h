/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include "tools.h"

class IStreamBase;
class OStreamBase;

class Kingdom;

namespace fheroes2
{
    const char * getBarrierColorName( const int color );
    const char * getTentColorName( const int color );
}

// !!! IMPORTANT !!!
// Do NOT change the order of the items as they are used for the map format.
enum class PlayerColor : uint8_t
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

using PlayerColorUnderlyingType = std::underlying_type_t<PlayerColor>;

constexpr PlayerColor operator|( const PlayerColor lhs, const PlayerColor rhs )
{
    return static_cast<PlayerColor>( static_cast<PlayerColorUnderlyingType>( lhs ) | static_cast<PlayerColorUnderlyingType>( rhs ) );
}

constexpr PlayerColor & operator|=( PlayerColor & lhs, const PlayerColor rhs )
{
    return lhs = lhs | rhs;
}

constexpr PlayerColor operator&( const PlayerColor lhs, const PlayerColor rhs )
{
    return static_cast<PlayerColor>( static_cast<PlayerColorUnderlyingType>( lhs ) & static_cast<PlayerColorUnderlyingType>( rhs ) );
}

constexpr PlayerColor & operator&=( PlayerColor & lhs, const PlayerColor rhs )
{
    return lhs = lhs & rhs;
}

constexpr PlayerColor operator^( const PlayerColor lhs, const PlayerColor rhs )
{
    return static_cast<PlayerColor>( static_cast<PlayerColorUnderlyingType>( lhs ) ^ static_cast<PlayerColorUnderlyingType>( rhs ) );
}

constexpr PlayerColor & operator^=( PlayerColor & lhs, const PlayerColor rhs )
{
    return lhs = lhs ^ rhs;
}

constexpr PlayerColor operator~( const PlayerColor color )
{
    return static_cast<PlayerColor>( ~static_cast<PlayerColorUnderlyingType>( color ) );
}

class PlayerColors : public std::vector<PlayerColor>
{
public:
    explicit PlayerColors( const PlayerColor colors = PlayerColor::ALL );
};

namespace Color
{
    constexpr bool haveCommonColors( const PlayerColor firstColorSet, const PlayerColor secondColorSet )
    {
        return ( firstColorSet & secondColorSet ) != PlayerColor::NONE;
    }

    std::string String( const PlayerColor color );

    int GetIndex( const PlayerColor color );

    constexpr int Count( const PlayerColor colors )
    {
        return CountBits( static_cast<uint32_t>( colors & PlayerColor::ALL ) );
    }

    PlayerColor GetFirst( const PlayerColor colors );

    PlayerColor IndexToColor( const int index );
}

class ColorBase
{
public:
    ColorBase() = default;

    explicit ColorBase( const PlayerColor col )
        : _color( col )
    {
        // Do nothing.
    }

    bool isFriends( const PlayerColor col ) const;

    Kingdom & GetKingdom() const;

    void SetColor( const PlayerColor col );

    PlayerColor GetColor() const
    {
        return _color;
    }

private:
    PlayerColor _color{ PlayerColor::NONE };

    friend OStreamBase & operator<<( OStreamBase & stream, const ColorBase & col );
    friend IStreamBase & operator>>( IStreamBase & stream, ColorBase & col );
};
