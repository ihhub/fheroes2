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

#ifndef H2OBJXLOC_H
#define H2OBJXLOC_H

#include <cstdint>

namespace ObjXlc1
{
    int GetPassable( const uint8_t index );
    bool isAction( uint32_t index );
    bool isShadow( const uint8_t index );
    int GetActionObject( uint32_t index );
}

namespace ObjXlc2
{
    int GetPassable( const uint8_t index );
    bool isAction( uint32_t index );
    bool isShadow( const uint8_t index );
    int GetActionObject( uint32_t index );

    // Returns true if the index belongs to Reefs type of the object.
    bool isReefs( const uint8_t index );
}

namespace ObjXlc3
{
    int GetPassable( const uint8_t index );
    bool isAction( uint32_t index );
    bool isShadow( const uint8_t index );
    int GetActionObject( uint32_t index );
}

#endif
