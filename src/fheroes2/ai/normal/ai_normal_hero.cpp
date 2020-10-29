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
    double ScaleWithDistance( double value, uint32_t distance )
    {
        if ( distance == 0 )
            return value;
        // scale non-linearly (more value lost as distance increases)
        return value - ( distance * std::log10( distance ) );
    }

    double GetObjectValue( const Heroes & hero, int index, int objectID, double valueToIgnore )
    {
        // In the future these hardcoded values could be configured by the mod
        // 1 tile distance is 100.0 value approximately
        const Maps::Tiles & tile = world.GetTiles( index );

        if ( objectID == MP2::OBJ_CASTLE ) {
            Castle * castle = world.GetCastle( Maps::GetPoint( index ) );
            if ( !castle )
                return valueToIgnore;

            if ( hero.GetColor() == castle->GetColor() )
                return castle->getVisitValue( hero );
            else
                return castle->getBuildingValue() * 70.0 + 1250;
        }
        else if ( objectID == MP2::OBJ_HEROES ) {
            const Heroes * otherHero = tile.GetHeroes();
            if ( !otherHero )
                return valueToIgnore;

            if ( hero.GetColor() == otherHero->GetColor() ) {
                if ( hero.getStatsValue() + 2 > otherHero->getStatsValue() )
                    return valueToIgnore;

                const double value = hero.getMeetingValue( *otherHero );
                // limit the max value of friendly hero meeting to 30 tiles
                return ( value < 200 ) ? valueToIgnore : std::min( value, 3000.0 );
            }
            return 1700.0;
        }
        else if ( objectID == MP2::OBJ_MONSTER ) {
            return 400.0;
        }
        else if ( objectID == MP2::OBJ_MINES || objectID == MP2::OBJ_SAWMILL || objectID == MP2::OBJ_ALCHEMYLAB ) {
            return ( tile.QuantityResourceCount().first == Resource::GOLD ) ? 2000.0 : 1000.0;
        }
        else if ( MP2::isArtifactObject( objectID ) && tile.QuantityArtifact().isValid() ) {
            return 500.0 * tile.QuantityArtifact().getArtifactValue();
        }
        else if ( MP2::isPickupObject( objectID ) ) {
            return 500.0;
        }
        else if ( objectID == MP2::OBJ_XANADU ) {
            return 2000.0;
        }
        else if ( MP2::isHeroUpgradeObject( objectID ) ) {
            return 400.0;
        }
        else if ( objectID == MP2::OBJ_OBSERVATIONTOWER ) {
            return world.getRegion( tile.GetRegion() ).getFogRatio( hero.GetColor() ) * 1500;
        }
        else if ( objectID == MP2::OBJ_COAST ) {
            return world.getRegion( tile.GetRegion() ).getObjectCount() * 50.0 - 2000;
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
        const double lowestPossibleValue = -1.0 * Maps::Ground::slowestMovePenalty * world.getSize();
        int priorityTarget = -1;

        double maxPriority = lowestPossibleValue;
        int objectID = MP2::OBJ_ZERO;

        // pre-cache the pathfinder
        _pathfinder.reEvaluateIfNeeded( hero );

        for ( size_t idx = 0; idx < _mapObjects.size(); ++idx ) {
            const IndexObject & node = _mapObjects[idx];

            // Skip if hero in patrol mode and object outside of reach
            if ( heroInPatrolMode && _pathfinder.buildPath( node.first ).size() > distanceLimit )
                continue;

            if ( HeroesValidObject( hero, node.first ) ) {
                const uint32_t dist = _pathfinder.getDistance( node.first );
                if ( dist == 0 )
                    continue;

                double value = GetObjectValue( hero, node.first, node.second, lowestPossibleValue );

                const std::vector<IndexObject> & list = _pathfinder.getObjectsOnTheWay( node.first );
                for ( const IndexObject & pair : list ) {
                    if ( HeroesValidObject( hero, pair.first ) && std::binary_search( _mapObjects.begin(), _mapObjects.end(), pair ) )
                        value += GetObjectValue( hero, pair.first, pair.second, lowestPossibleValue );
                }
                value = ScaleWithDistance( value, dist );

                if ( dist && value > maxPriority ) {
                    maxPriority = value;
                    priorityTarget = node.first;
                    objectID = node.second;

                    DEBUG( DBG_AI, DBG_TRACE,
                           hero.GetName() << ": valid object at " << node.first << " value is " << value << " (" << MP2::StringObject( node.second ) << ")" );
                }
            }
        }

        if ( priorityTarget != -1 ) {
            DEBUG( DBG_AI, DBG_INFO,
                   hero.GetName() << ": priority selected: " << priorityTarget << " value is " << maxPriority << " (" << MP2::StringObject( objectID ) << ")" );
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
                _pathfinder.reEvaluateIfNeeded( hero );
                hero.GetPath().setPath( _pathfinder.buildPath( targetIndex ), targetIndex );
                objectsToErase.push_back( hero.GetPath().GetDestinationIndex() );

                HeroesMove( hero );
            }
            else {
                hero.SetModes( AI::HERO_WAITING );
                break;
            }
        }

        // Remove the object from the list so other heroes won't target it
        if ( objectsToErase.size() ) {
            for ( const int idxToErase : objectsToErase ) {
                auto it = std::find_if( _mapObjects.begin(), _mapObjects.end(), [&idxToErase]( const IndexObject & o ) { return o.first == idxToErase; } );
                // Actually remove if this object single use only
                if ( it != _mapObjects.end() && !MP2::isCaptureObject( it->second ) && !MP2::isRemoveObject( it->second ) ) {
                    // retains the vector order for binary search
                    _mapObjects.erase( it );
                }
            }
        }

        if ( !hero.MayStillMove() ) {
            hero.SetModes( AI::HERO_MOVED );
        }
    }
}
