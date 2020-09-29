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

#include <algorithm>

#include "ai_normal.h"
#include "heroes.h"
#include "maps.h"
#include "mp2.h"
#include "world.h"

namespace AI
{

    int GetPriorityTarget( const std::vector<MapObjectNode> & mapObjects, const Heroes & hero )
    {
        int priorityTarget = -1;

        const int heroIndex = hero.GetIndex();
        const uint32_t skill = hero.GetLevelSkill( Skill::Secondary::PATHFINDING );

        double maxPriority = 50000;
        const size_t listSize = mapObjects.size();
        
        for ( size_t it = 0; it < listSize; ++it ) {
            const MapObjectNode & node = mapObjects[it];
            if ( HeroesValidObject( hero, node.first ) ) {
                double value = world.getDistance( heroIndex, node.first, skill );
                if ( value && value < maxPriority ) {
                    maxPriority = value;
                    priorityTarget = node.first;
                }
            }
        }
        return priorityTarget;
    }

    bool MoveHero( Heroes & hero, int target )
    {
        if ( target != -1 && hero.GetPath().Calculate( target ) ) {
            HeroesMove( hero );
            return true;
        }

        hero.SetModes( AI::HERO_WAITING );
        return false;
    }

    void Normal::HeroesActionComplete( Heroes & hero, int index )
    {
        // std::remove( mapObjects.begin(), mapObjects.end(), 3 );
    }

    void Normal::HeroTurn( Heroes & hero )
    {
        hero.ResetModes( AI::HERO_WAITING | AI::HERO_MOVED | AI::HERO_SKIP_TURN );

        while ( hero.MayStillMove() && !hero.Modes( AI::HERO_WAITING | AI::HERO_MOVED ) ) {
            MoveHero( hero, GetPriorityTarget(mapObjects, hero ) );
        }

        if ( !hero.MayStillMove() ) {
            hero.SetModes( AI::HERO_MOVED );
        }
    }
}
