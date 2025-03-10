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

#pragma once

#include <utility>

#include "color.h"
#include "game_io.h"
#include "mp2.h"
#include "save_format_version.h"
#include "serialize.h"

class IndexObject : public std::pair<int32_t, MP2::MapObjectType>
{
public:
    IndexObject()
        : std::pair<int32_t, MP2::MapObjectType>( -1, MP2::OBJ_NONE )
    {}

    IndexObject( int32_t index, MP2::MapObjectType object )
        : std::pair<int32_t, MP2::MapObjectType>( index, object )
    {}

    bool isIndex( int32_t index ) const
    {
        return index == first;
    }

    bool isObject( MP2::MapObjectType object ) const
    {
        return object == second;
    }
};

class ObjectColor : public std::pair<MP2::MapObjectType, int>
{
public:
    ObjectColor()
        : std::pair<MP2::MapObjectType, int>( MP2::OBJ_NONE, Color::NONE )
    {}

    ObjectColor( MP2::MapObjectType object, int color )
        : std::pair<MP2::MapObjectType, int>( object, color )
    {}

    bool isColor( int colors ) const
    {
        return ( colors & second ) != 0;
    }
};

inline IStreamBase & operator>>( IStreamBase & stream, IndexObject & indexObject )
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE1_1103_RELEASE,
                   "Remove this operator completely. It will be automatically replaced by IStreamBase & operator>>( std::pair<> & )" );

    stream >> indexObject.first;

    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE1_1103_RELEASE ) {
        int32_t temp = MP2::OBJ_NONE;
        stream >> temp;

        indexObject.second = static_cast<MP2::MapObjectType>( temp );
    }
    else {
        stream >> indexObject.second;
    }

    return stream;
}

inline IStreamBase & operator>>( IStreamBase & stream, ObjectColor & objectColor )
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE1_1103_RELEASE,
                   "Remove this operator completely. It will be automatically replaced by IStreamBase & operator>>( std::pair<> & )" );

    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE1_1103_RELEASE ) {
        int32_t temp = MP2::OBJ_NONE;
        stream >> temp;

        objectColor.first = static_cast<MP2::MapObjectType>( temp );
    }
    else {
        stream >> objectColor.first;
    }

    return stream >> objectColor.second;
}
