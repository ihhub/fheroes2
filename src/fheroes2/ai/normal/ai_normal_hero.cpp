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
#include "ground.h"
#include "heroes.h"
#include "maps.h"
#include "mp2.h"
#include "world.h"

namespace AI
{
    double GetObjectValue( int index, int objectID )
    {
        // In the future these hardcoded values could be configured by the mod
        const Maps::Tiles & tile = world.GetTiles( index );

        if ( objectID == MP2::OBJ_CASTLE ) {
            return 2000.0;
        }
        else if ( objectID == MP2::OBJ_HEROES ) {
            return 1700.0;
        }
        else if ( objectID == MP2::OBJ_MONSTER ) {
            return 900.0;
        }
        else if ( objectID == MP2::OBJ_MINES || objectID == MP2::OBJ_SAWMILL || objectID == MP2::OBJN_ALCHEMYLAB ) {
            return 1000.0;
        }
        else if ( MP2::isArtifactObject( objectID ) && tile.QuantityArtifact().isValid() ) {
            return 500.0 * tile.QuantityArtifact().getArtifactValue();
        }
        else if ( MP2::isPickupObject( objectID ) ) {
            return 300.0;
        }
        else if ( MP2::isHeroUpgradeObject( objectID ) ) {
            return 400.0;
        }
        else if ( objectID == MP2::OBJ_OBSERVATIONTOWER ) {
            return 400.0;
        }

        return 0;
    }

    int GetPriorityTarget( const std::vector<MapObjectNode> & mapObjects, const Heroes & hero )
    {
        int priorityTarget = -1;

        const int heroIndex = hero.GetIndex();
        const uint32_t skill = hero.GetLevelSkill( Skill::Secondary::PATHFINDING );

        double maxPriority = -1.0 * Maps::Ground::slowestMovePenalty * world.w() * world.h();
        int objectID = 0;

        for ( size_t it = 0; it < mapObjects.size(); ++it ) {
            const MapObjectNode & node = mapObjects[it];
            if ( HeroesValidObject( hero, node.first ) ) {
                const uint32_t dist = world.getDistance( heroIndex, node.first, skill );
                if ( dist == 0 )
                    continue;

                const double value = GetObjectValue( node.first, node.second ) - static_cast<double>( dist );
                if ( dist && value > maxPriority ) {
                    maxPriority = value;
                    priorityTarget = node.first;
                    objectID = node.second;
                }
            }
        }
        DEBUG( DBG_AI, DBG_TRACE,
               hero.GetName() << ": priority selected: " << priorityTarget << " value is " << maxPriority << " (" << MP2::StringObject( objectID ) << ")" );

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
        Castle * castle = hero.inCastle();
        if ( castle ) {
            ReinforceHeroInCastle( hero, *castle, castle->GetKingdom().GetFunds() );
        }
    }

    void Normal::HeroTurn( Heroes & hero )
    {
        hero.ResetModes( AI::HERO_WAITING | AI::HERO_MOVED | AI::HERO_SKIP_TURN );

        while ( hero.MayStillMove() && !hero.Modes( AI::HERO_WAITING | AI::HERO_MOVED ) ) {
            MoveHero( hero, GetPriorityTarget( mapObjects, hero ) );
        }

        if ( !hero.MayStillMove() ) {
            hero.SetModes( AI::HERO_MOVED );
        }
    }
}
