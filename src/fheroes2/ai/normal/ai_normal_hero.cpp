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
            return 400.0;
        }
        else if ( objectID == MP2::OBJ_MINES || objectID == MP2::OBJ_SAWMILL || objectID == MP2::OBJ_ALCHEMYLAB ) {
            return 1000.0;
        }
        else if ( MP2::isArtifactObject( objectID ) && tile.QuantityArtifact().isValid() ) {
            return 500.0 * tile.QuantityArtifact().getArtifactValue();
        }
        else if ( MP2::isPickupObject( objectID ) ) {
            return 500.0;
        }
        else if ( MP2::isHeroUpgradeObject( objectID ) ) {
            return 400.0;
        }
        else if ( objectID == MP2::OBJ_OBSERVATIONTOWER ) {
            return 500.0;
        }
        else if ( objectID == MP2::OBJ_COAST ) {
            // de-prioritize the landing
            return -1500.0;
        }
        else if ( objectID == MP2::OBJ_BOAT || objectID == MP2::OBJ_WHIRLPOOL ) {
            // de-prioritize the water movement even harder
            return -2500.0;
        }

        return 0;
    }

    int AI::Normal::GetPriorityTarget( const Heroes & hero, int patrolIndex, uint32_t distanceLimit )
    {
        const bool heroInPatrolMode = patrolIndex != -1;
        int priorityTarget = -1;

        double maxPriority = -1.0 * Maps::Ground::slowestMovePenalty * world.getSize();
        int objectID = MP2::OBJ_ZERO;

        std::vector<IndexObject> additionalObjectsList;
        for ( size_t idx = 0; idx < _mapObjects.size(); ++idx ) {
            const IndexObject & node = _mapObjects[idx];

            // Skip if hero in patrol mode and object outside of reach
            if ( heroInPatrolMode && Maps::GetApproximateDistance( patrolIndex, node.first ) > distanceLimit )
                continue;

            if ( HeroesValidObject( hero, node.first ) ) {
                const uint32_t dist = _pathfinder.getDistance( hero, node.first );
                if ( dist == 0 )
                    continue;

                double value = GetObjectValue( node.first, node.second );

                auto list = _pathfinder.getObjectsOnTheWay( node.first, false );
                for ( const IndexObject & pair : list ) {
                    if ( HeroesValidObject( hero, pair.first )
                         && std::binary_search( _mapObjects.begin(), _mapObjects.end(), pair ) )
                        value += ( GetObjectValue( pair.first, pair.second ) / 2 );
                }

                value -= static_cast<double>( dist );

                if ( dist && value > maxPriority ) {
                    maxPriority = value;
                    priorityTarget = node.first;
                    objectID = node.second;
                    additionalObjectsList = list;

                    DEBUG( DBG_AI, DBG_TRACE,
                           hero.GetName() << ": valid object at " << node.first << " value is " << value << " (" << MP2::StringObject( node.second ) << ")" );
                }
            }
        }

        if ( priorityTarget != -1 ) {
            DEBUG( DBG_AI, DBG_INFO,
                   hero.GetName() << ": priority selected: " << priorityTarget << " value is " << maxPriority << " (" << MP2::StringObject( objectID ) << ")" );

            for ( const IndexObject & pair : additionalObjectsList ) {
                DEBUG( DBG_AI, DBG_INFO, hero.GetName() << ": on the way: " << pair.first << " (" << MP2::StringObject( pair.second ) << ")" );
                //auto toErase = std::find( _mapObjects.begin(), _mapObjects.end(), pair );
                //if ( toErase != _mapObjects.end() )
                //    _mapObjects.erase( toErase );
            }
        }
        else if ( !heroInPatrolMode ) {
            priorityTarget = _pathfinder.getFogDiscoveryTile( hero );
            DEBUG( DBG_AI, DBG_INFO, hero.GetName() << " can't find an object. Scouting the fog of war at " << priorityTarget );
        }

        return priorityTarget;
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
        int patrolCenter = -1;
        uint32_t patrolDistance = 0;

        if ( hero.Modes( Heroes::PATROL ) ) {
            patrolDistance = hero.GetSquarePatrol();

            if ( patrolDistance == 0 ) {
                DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << " standing still. Skip turn." );
                hero.SetModes( AI::HERO_MOVED );
                return;
            }
            patrolCenter = Maps::GetIndexFromAbsPoint( hero.GetCenterPatrol() );
        }

        hero.ResetModes( AI::HERO_WAITING | AI::HERO_MOVED | AI::HERO_SKIP_TURN );

        std::vector<int> objectsToErase;
        while ( hero.MayStillMove() && !hero.Modes( AI::HERO_WAITING | AI::HERO_MOVED ) ) {
            const int startIndex = hero.GetIndex();
            const int targetIndex = GetPriorityTarget( hero, patrolCenter, patrolDistance );

            if ( targetIndex != -1 ) {
                objectsToErase.push_back( targetIndex );
                _pathfinder.reEvaluateIfNeeded( hero );
                hero.GetPath().setPath( _pathfinder.buildPath( targetIndex ), targetIndex );
                const int destIndex = hero.GetPath().GetDestinationIndex();
                HeroesMove( hero );

                // Check if hero is stuck
                if ( targetIndex != startIndex && hero.GetIndex() == startIndex ) {
                    hero.SetModes( AI::HERO_WAITING );
                    DEBUG( DBG_AI, DBG_WARN, hero.GetName() << " is stuck trying to reach " << targetIndex );
                }
                else {
                    auto toErase = std::find_if( _mapObjects.begin(), _mapObjects.end(), [&destIndex]( const IndexObject & obj ) { return destIndex == obj.first; } );
                    if ( toErase != _mapObjects.end() )
                        _mapObjects.erase( toErase );
                }
            }
            else {
                hero.SetModes( AI::HERO_WAITING );
                break;
            }
        }

        if ( !hero.MayStillMove() ) {
            hero.SetModes( AI::HERO_MOVED );
        }
    }
}
