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

#ifndef H2AI_NORMAL_H
#define H2AI_NORMAL_H

#include "ai.h"
#include "pairs.h"
#include "world_pathfinding.h"

namespace AI
{
    struct RegionStats
    {
        double highestThreat = -1;
        double averageMonster = -1;
        int friendlyHeroCount = 0;
        int monsterCount = 0;
        int fogCount = 0;
        std::vector<IndexObject> validObjects;
    };

    class Normal : public Base
    {
    public:
        Normal();
        void KingdomTurn( Kingdom & kingdom );
        void CastleTurn( Castle & castle, bool defensive = false );
        void BattleTurn( Battle::Arena & arena, const Battle::Unit & currentUnit, Battle::Actions & actions );
        void HeroTurn( Heroes & hero );

        void revealFog( const Maps::Tiles & tile );

        void HeroesActionComplete( Heroes & hero );

        double getObjectValue( const Heroes & hero, int index, int objectID, double valueToIgnore ) const;
        int getPriorityTarget( const Heroes & hero, int patrolIndex = -1, uint32_t distanceLimit = 0 );
        void resetPathfinder();

    private:
        // following data won't be saved/serialized
        double _combinedHeroStrength = 0;
        std::vector<IndexObject> _mapObjects;
        std::vector<RegionStats> _regions;
        AIWorldPathfinder _pathfinder;
    };
}

#endif
