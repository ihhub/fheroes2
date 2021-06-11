/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "position.h"
#include "maps.h"

MapPosition::MapPosition( const fheroes2::Point & pt )
    : center( pt )
{}

s32 MapPosition::GetIndex( void ) const
{
    return Maps::GetIndexFromAbsPoint( center );
}

void MapPosition::SetCenter( const fheroes2::Point & pt )
{
    center = pt;
}

void MapPosition::SetIndex( const int32_t index )
{
    center = Maps::isValidAbsIndex( index ) ? Maps::GetPoint( index ) : fheroes2::Point( -1, -1 );
}

StreamBase & operator<<( StreamBase & sb, const MapPosition & st )
{
    // TODO: before 0.9.4 Point was int16_t type
    const int16_t x = static_cast<int16_t>( st.center.x );
    const int16_t y = static_cast<int16_t>( st.center.y );

    return sb << x << y;
}

StreamBase & operator>>( StreamBase & sb, MapPosition & st )
{
    // TODO: before 0.9.4 Point was int16_t type
    int16_t x = 0;
    int16_t y = 0;

    sb >> x >> y;

    st.center.x = x;
    st.center.y = y;

    return sb;
}
