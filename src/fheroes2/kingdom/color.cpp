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

#include "color.h"

#include <cassert>

#include "players.h"
#include "serialize.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

namespace
{
    enum class BarrierColor : int
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

std::string Color::String( const int color )
{
    switch ( color ) {
    case Color::BLUE:
        return _( "Blue" );
    case Color::GREEN:
        return _( "Green" );
    case Color::RED:
        return _( "Red" );
    case Color::YELLOW:
        return _( "Yellow" );
    case Color::ORANGE:
        return _( "Orange" );
    case Color::PURPLE:
        return _( "Purple" );
    case Color::UNUSED:
        return "Unknown";
    default:
        break;
    }

    return "None";
}

int Color::GetIndex( const int color )
{
    switch ( color ) {
    case BLUE:
        return 0;
    case GREEN:
        return 1;
    case RED:
        return 2;
    case YELLOW:
        return 3;
    case ORANGE:
        return 4;
    case PURPLE:
        return 5;
    default:
        break;
    }

    // NONE
    return 6;
}

int Color::Count( const int colors )
{
    return CountBits( colors & ALL );
}

int Color::GetFirst( const int colors )
{
    if ( colors & BLUE ) {
        return BLUE;
    }
    if ( colors & GREEN ) {
        return GREEN;
    }
    if ( colors & RED ) {
        return RED;
    }
    if ( colors & YELLOW ) {
        return YELLOW;
    }
    if ( colors & ORANGE ) {
        return ORANGE;
    }
    if ( colors & PURPLE ) {
        return PURPLE;
    }

    return NONE;
}

uint8_t Color::IndexToColor( const int index )
{
    switch ( index ) {
    case 0:
        return BLUE;
    case 1:
        return GREEN;
    case 2:
        return RED;
    case 3:
        return YELLOW;
    case 4:
        return ORANGE;
    case 5:
        return PURPLE;
    default:
        break;
    }

    return Color::NONE;
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

Colors::Colors( const int colors /* = Color::ALL */ )
{
    reserve( 6 );

    if ( colors & Color::BLUE ) {
        push_back( Color::BLUE );
    }
    if ( colors & Color::GREEN ) {
        push_back( Color::GREEN );
    }
    if ( colors & Color::RED ) {
        push_back( Color::RED );
    }
    if ( colors & Color::YELLOW ) {
        push_back( Color::YELLOW );
    }
    if ( colors & Color::ORANGE ) {
        push_back( Color::ORANGE );
    }
    if ( colors & Color::PURPLE ) {
        push_back( Color::PURPLE );
    }
}

bool ColorBase::isFriends( const int col ) const
{
    return ( col & Color::ALL ) && ( color == col || Players::isFriends( color, col ) );
}

void ColorBase::SetColor( const int col )
{
    switch ( col ) {
    case Color::NONE:
    case Color::BLUE:
    case Color::GREEN:
    case Color::RED:
    case Color::YELLOW:
    case Color::ORANGE:
    case Color::PURPLE:
        color = col;
        break;
    default:
#ifdef WITH_DEBUG
        assert( 0 );
#endif
        color = Color::NONE;
        break;
    }
}

Kingdom & ColorBase::GetKingdom() const
{
    return world.GetKingdom( color );
}

OStreamBase & operator<<( OStreamBase & stream, const ColorBase & col )
{
    return stream << col.color;
}

IStreamBase & operator>>( IStreamBase & stream, ColorBase & col )
{
    return stream >> col.color;
}
