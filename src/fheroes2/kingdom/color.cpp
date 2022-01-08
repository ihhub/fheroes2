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

#include "color.h"
#include "players.h"
#include "serialize.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

std::string Color::String( int color )
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
    }

    return "None";
}

int Color::GetIndex( int color )
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

int Color::Count( int colors )
{
    return CountBits( colors & ALL );
}

int Color::FromInt( int col )
{
    switch ( col ) {
    case BLUE:
    case GREEN:
    case RED:
    case YELLOW:
    case ORANGE:
    case PURPLE:
        return col;
    default:
        break;
    }

    return NONE;
}

int Color::GetFirst( int colors )
{
    if ( colors & BLUE )
        return BLUE;
    else if ( colors & GREEN )
        return GREEN;
    else if ( colors & RED )
        return RED;
    else if ( colors & YELLOW )
        return YELLOW;
    else if ( colors & ORANGE )
        return ORANGE;
    else if ( colors & PURPLE )
        return PURPLE;

    return NONE;
}

const char * fheroes2::getBarrierColorName( const int color )
{
    switch ( color ) {
    case AQUA:
        return _( "barrier|Aqua" );
    case BLUE:
        return _( "barrier|Blue" );
    case BROWN:
        return _( "barrier|Brown" );
    case GOLD:
        return _( "barrier|Gold" );
    case GREEN:
        return _( "barrier|Green" );
    case ORANGE:
        return _( "barrier|Orange" );
    case PURPLE:
        return _( "barrier|Purple" );
    case RED:
        return _( "barrier|Red" );
    default:
        break;
    }

    return "None";
}

const char * fheroes2::getTentColorName( const int color )
{
    switch ( color ) {
    case AQUA:
        return _( "tent|Aqua" );
    case BLUE:
        return _( "tent|Blue" );
    case BROWN:
        return _( "tent|Brown" );
    case GOLD:
        return _( "tent|Gold" );
    case GREEN:
        return _( "tent|Green" );
    case ORANGE:
        return _( "tent|Orange" );
    case PURPLE:
        return _( "tent|Purple" );
    case RED:
        return _( "tent|Red" );
    default:
        break;
    }

    return "None";
}

Colors::Colors( int colors )
{
    reserve( 6 );

    if ( colors & Color::BLUE )
        push_back( Color::BLUE );
    if ( colors & Color::GREEN )
        push_back( Color::GREEN );
    if ( colors & Color::RED )
        push_back( Color::RED );
    if ( colors & Color::YELLOW )
        push_back( Color::YELLOW );
    if ( colors & Color::ORANGE )
        push_back( Color::ORANGE );
    if ( colors & Color::PURPLE )
        push_back( Color::PURPLE );
}

bool ColorBase::isFriends( int col ) const
{
    return ( col & Color::ALL ) && ( color == col || Players::isFriends( color, col ) );
}

void ColorBase::SetColor( int col )
{
    color = Color::FromInt( col );
}

Kingdom & ColorBase::GetKingdom( void ) const
{
    return world.GetKingdom( color );
}

StreamBase & operator<<( StreamBase & msg, const ColorBase & col )
{
    return msg << col.color;
}

StreamBase & operator>>( StreamBase & msg, ColorBase & col )
{
    return msg >> col.color;
}
