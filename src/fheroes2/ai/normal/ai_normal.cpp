/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "ai_normal.h"
#include "maps_tiles.h"

namespace AI
{
    Normal::Normal()
        : _pathfinder( ARMY_STRENGTH_ADVANTAGE_LARGE )
    {
        _personality = Rand::Get( AI::WARRIOR, AI::EXPLORER );
    }

    void Normal::resetPathfinder()
    {
        _pathfinder.reset();
    }

    void Normal::revealFog( const Maps::Tiles & tile )
    {
        _mapObjects.emplace_back( tile.GetIndex(), tile.GetObject() );
    }
}
