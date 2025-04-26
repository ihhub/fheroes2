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

#include "color.h"

#include <cassert>

#include "game_io.h"
#include "players.h"
#include "save_format_version.h"
#include "serialize.h"
#include "translations.h"
#include "world.h"

namespace
{
    enum class BarrierColor : uint8_t
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
}

namespace Color
{
    PlayerColors::PlayerColors( const Color::PlayerColor colors /* = Color::ALL */ )
    {
        reserve( 6 );

        if ( haveCommonColors( colors, Color::PlayerColor::BLUE ) ) {
            push_back( Color::PlayerColor::BLUE );
        }
        if ( haveCommonColors( colors, Color::PlayerColor::GREEN ) ) {
            push_back( Color::PlayerColor::GREEN );
        }
        if ( haveCommonColors( colors, Color::PlayerColor::RED ) ) {
            push_back( Color::PlayerColor::RED );
        }
        if ( haveCommonColors( colors, Color::PlayerColor::YELLOW ) ) {
            push_back( Color::PlayerColor::YELLOW );
        }
        if ( haveCommonColors( colors, Color::PlayerColor::ORANGE ) ) {
            push_back( Color::PlayerColor::ORANGE );
        }
        if ( haveCommonColors( colors, Color::PlayerColor::PURPLE ) ) {
            push_back( Color::PlayerColor::PURPLE );
        }
    }

    std::string String( const PlayerColor color )
    {
        switch ( color ) {
        case PlayerColor::BLUE:
            return _( "Blue" );
        case PlayerColor::GREEN:
            return _( "Green" );
        case PlayerColor::RED:
            return _( "Red" );
        case PlayerColor::YELLOW:
            return _( "Yellow" );
        case PlayerColor::ORANGE:
            return _( "Orange" );
        case PlayerColor::PURPLE:
            return _( "Purple" );
        case PlayerColor::UNUSED:
            return "Unknown";
        default:
            break;
        }

        return "None";
    }

    int GetIndex( const PlayerColor color )
    {
        switch ( color ) {
        case PlayerColor::BLUE:
            return 0;
        case PlayerColor::GREEN:
            return 1;
        case PlayerColor::RED:
            return 2;
        case PlayerColor::YELLOW:
            return 3;
        case PlayerColor::ORANGE:
            return 4;
        case PlayerColor::PURPLE:
            return 5;
        default:
            break;
        }

        // NONE
        return 6;
    }

    PlayerColor GetFirst( const PlayerColor colors )
    {
        if ( haveCommonColors( colors, PlayerColor::BLUE ) ) {
            return PlayerColor::BLUE;
        }
        if ( haveCommonColors( colors, PlayerColor::GREEN ) ) {
            return PlayerColor::GREEN;
        }
        if ( haveCommonColors( colors, PlayerColor::RED ) ) {
            return PlayerColor::RED;
        }
        if ( haveCommonColors( colors, PlayerColor::YELLOW ) ) {
            return PlayerColor::YELLOW;
        }
        if ( haveCommonColors( colors, PlayerColor::ORANGE ) ) {
            return PlayerColor::ORANGE;
        }
        if ( haveCommonColors( colors, PlayerColor::PURPLE ) ) {
            return PlayerColor::PURPLE;
        }

        return PlayerColor::NONE;
    }

    PlayerColor IndexToColor( const int index )
    {
        switch ( index ) {
        case 0:
            return PlayerColor::BLUE;
        case 1:
            return PlayerColor::GREEN;
        case 2:
            return PlayerColor::RED;
        case 3:
            return PlayerColor::YELLOW;
        case 4:
            return PlayerColor::ORANGE;
        case 5:
            return PlayerColor::PURPLE;
        default:
            break;
        }

        return PlayerColor::NONE;
    }
}

const char * fheroes2::getBarrierColorName( const int color )
{
    switch ( static_cast<BarrierColor>( color ) ) {
    case BarrierColor::AQUA:
        return _( "barrier|Aqua" );
    case BarrierColor::BLUE:
        return _( "barrier|Blue" );
    case BarrierColor::BROWN:
        return _( "barrier|Brown" );
    case BarrierColor::GOLD:
        return _( "barrier|Gold" );
    case BarrierColor::GREEN:
        return _( "barrier|Green" );
    case BarrierColor::ORANGE:
        return _( "barrier|Orange" );
    case BarrierColor::PURPLE:
        return _( "barrier|Purple" );
    case BarrierColor::RED:
        return _( "barrier|Red" );
    default:
        break;
    }

    return "None";
}

const char * fheroes2::getTentColorName( const int color )
{
    switch ( static_cast<BarrierColor>( color ) ) {
    case BarrierColor::AQUA:
        return _( "tent|Aqua" );
    case BarrierColor::BLUE:
        return _( "tent|Blue" );
    case BarrierColor::BROWN:
        return _( "tent|Brown" );
    case BarrierColor::GOLD:
        return _( "tent|Gold" );
    case BarrierColor::GREEN:
        return _( "tent|Green" );
    case BarrierColor::ORANGE:
        return _( "tent|Orange" );
    case BarrierColor::PURPLE:
        return _( "tent|Purple" );
    case BarrierColor::RED:
        return _( "tent|Red" );
    default:
        break;
    }

    return "None";
}

bool ColorBase::isFriends( const Color::PlayerColor col ) const
{
    return Color::haveCommonColors( Color::PlayerColor::ALL, col ) && ( _color == col || Players::isFriends( _color, col ) );
}

void ColorBase::SetColor( const Color::PlayerColor col )
{
    switch ( col ) {
    case Color::PlayerColor::NONE:
    case Color::PlayerColor::BLUE:
    case Color::PlayerColor::GREEN:
    case Color::PlayerColor::RED:
    case Color::PlayerColor::YELLOW:
    case Color::PlayerColor::ORANGE:
    case Color::PlayerColor::PURPLE:
        _color = col;
        break;
    default:
#ifdef WITH_DEBUG
        assert( 0 );
#endif
        _color = Color::PlayerColor::NONE;
        break;
    }
}

Kingdom & ColorBase::GetKingdom() const
{
    return world.GetKingdom( _color );
}

OStreamBase & operator<<( OStreamBase & stream, const ColorBase & col )
{
    return stream << col._color;
}

IStreamBase & operator>>( IStreamBase & stream, ColorBase & col )
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1109_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1109_RELEASE ) {
        int temp;
        stream >> temp;
        col._color = static_cast<Color::PlayerColor>( temp );
    }
    else {
        stream >> col._color;
    }

    return stream;
}
