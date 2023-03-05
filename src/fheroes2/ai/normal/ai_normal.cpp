/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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

#include <algorithm>
#include <cstdint>

#include "ai_normal.h"
#include "army.h"
#include "maps_tiles.h"
#include "pairs.h"
#include "rand.h"

namespace AI
{
    Normal::Normal()
        : _pathfinder( ARMY_ADVANTAGE_LARGE )
    {
        _personality = Rand::Get( AI::WARRIOR, AI::EXPLORER );
        _budget = { Resource::WOOD, Resource::MERCURY, Resource::ORE, Resource::SULFUR, Resource::CRYSTAL, Resource::GEMS, Resource::GOLD };
    }

    void Normal::resetPathfinder()
    {
        _pathfinder.reset();
    }

    void Normal::revealFog( const Maps::Tiles & tile )
    {
        const MP2::MapObjectType object = tile.GetObject();
        if ( object != MP2::OBJ_NONE )
            _mapObjects.emplace_back( tile.GetIndex(), object );
    }

    double Normal::getTargetArmyStrength( const Maps::Tiles & tile, const MP2::MapObjectType objectType )
    {
        if ( !isMonsterStrengthCacheable( objectType ) ) {
            return Army( tile ).GetStrength();
        }

        const int32_t tileId = tile.GetIndex();

        auto iter = _neutralMonsterStrengthCache.find( tileId );
        if ( iter != _neutralMonsterStrengthCache.end() ) {
            // Cache hit.
            return iter->second;
        }

        auto newEntry = _neutralMonsterStrengthCache.emplace( tileId, Army( tile ).GetStrength() );
        return newEntry.first->second;
    }
}
