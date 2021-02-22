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
#include "logging.h"
#include "maps.h"
#include "mp2.h"
#include "world.h"

namespace
{
    struct HeroToMove
    {
        Heroes * hero = nullptr;
        int patrolCenter = -1;
        uint32_t patrolDistance = 0;
    };

    // Used for caching object validations per hero.
    class ObjectValidator
    {
    public:
        ObjectValidator( const Heroes & hero )
            : _hero( hero )
        {}

        bool isValid( const int index )
        {
            auto iter = _validObjects.find( index );
            if ( iter != _validObjects.end() ) {
                return iter->second;
            }

            const bool valid = AI::HeroesValidObject( _hero, index );
            _validObjects[index] = valid;
            return valid;
        }

    private:
        const Heroes & _hero;
        std::map<int, bool> _validObjects;
    };

    // Used for caching of object value estimation per hero.
    class ObjectValueStorage
    {
    public:
        ObjectValueStorage( const Heroes & hero, const AI::Normal & ai, const double ignoreValue )
            : _hero( hero )
            , _ai( ai )
            , _ignoreValue( ignoreValue )
        {}

        double value( const std::pair<int, int> & objectInfo )
        {
            auto iter = _objectValue.find( objectInfo );
            if ( iter != _objectValue.end() ) {
                return iter->second;
            }

            const double value = _ai.getObjectValue( _hero, objectInfo.first, objectInfo.second, _ignoreValue );

            _objectValue[objectInfo] = value;
            return value;
        }

    private:
        const Heroes & _hero;
        const AI::Normal & _ai;
        const double _ignoreValue;
        std::map<std::pair<int, int>, double> _objectValue;
    };
}

namespace AI
{
    const double suboptimalTaskPenalty = 10000.0;
    const double dangerousTaskPenalty = 20000.0;

    double ScaleWithDistance( double value, uint32_t distance )
    {
        if ( distance == 0 )
            return value;
        // scale non-linearly (more value lost as distance increases)
        return value - ( distance * std::log10( distance ) );
    }

    double Normal::getObjectValue( const Heroes & hero, int index, int objectID, double valueToIgnore ) const
    {
        // In the future these hardcoded values could be configured by the mod
        // 1 tile distance is 100.0 value approximately
        const Maps::Tiles & tile = world.GetTiles( index );

        if ( objectID == MP2::OBJ_CASTLE ) {
            const Castle * castle = world.GetCastle( Maps::GetPoint( index ) );
            if ( !castle )
                return valueToIgnore;

            if ( hero.GetColor() == castle->GetColor() ) {
                double value = castle->getVisitValue( hero );
                if ( value < 1000 )
                    return valueToIgnore;

                if ( hero.isVisited( tile ) )
                    value -= suboptimalTaskPenalty;
                return value;
            }
            else {
                return castle->getBuildingValue() * 150.0 + 3000;
            }
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
                return ( value < 250 ) ? valueToIgnore : std::min( value, 10000.0 );
            }
            return 5000.0;
        }
        else if ( objectID == MP2::OBJ_MONSTER ) {
            return 1000.0;
        }
        else if ( objectID == MP2::OBJ_MINES || objectID == MP2::OBJ_SAWMILL || objectID == MP2::OBJ_ALCHEMYLAB ) {
            if ( tile.QuantityColor() == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return ( tile.QuantityResourceCount().first == Resource::GOLD ) ? 4000.0 : 2000.0;
        }
        else if ( MP2::isArtifactObject( objectID ) && tile.QuantityArtifact().isValid() ) {
            return 1000.0 * tile.QuantityArtifact().getArtifactValue();
        }
        else if ( MP2::isPickupObject( objectID ) ) {
            return 850.0;
        }
        else if ( MP2::isCaptureObject( objectID ) && MP2::isQuantityObject( objectID ) ) {
            // Objects like WATERWHEEL, WINDMILL and MAGICGARDEN if capture setting is enabled
            return 500.0;
        }
        else if ( objectID == MP2::OBJ_XANADU ) {
            return 3000.0;
        }
        else if ( objectID == MP2::OBJ_SHRINE1 ) {
            return 100;
        }
        else if ( objectID == MP2::OBJ_SHRINE2 ) {
            return 250;
        }
        else if ( objectID == MP2::OBJ_SHRINE3 ) {
            return 500;
        }
        else if ( MP2::isHeroUpgradeObject( objectID ) ) {
            return 500.0;
        }
        else if ( MP2::isMonsterDwelling( objectID ) ) {
            return tile.QuantityTroop().GetStrength();
        }
        else if ( objectID == MP2::OBJ_STONELITHS ) {
            const MapsIndexes & list = world.GetTeleportEndPoints( index );
            for ( const int teleportIndex : list ) {
                if ( world.GetTiles( teleportIndex ).isFog( hero.GetColor() ) )
                    return 0;
            }
            return valueToIgnore;
        }
        else if ( objectID == MP2::OBJ_OBSERVATIONTOWER ) {
            return _regions[tile.GetRegion()].fogCount * 100.0;
        }
        else if ( objectID == MP2::OBJ_COAST ) {
            const RegionStats & regionStats = _regions[tile.GetRegion()];
            const size_t objectCount = regionStats.validObjects.size();
            if ( objectCount < 1 )
                return valueToIgnore;

            double value = objectCount * 100.0 - 7500;
            if ( regionStats.friendlyHeroCount )
                value -= suboptimalTaskPenalty;
            return value;
        }
        else if ( objectID == MP2::OBJ_WHIRLPOOL ) {
            const MapsIndexes & list = world.GetWhirlpoolEndPoints( index );
            for ( const int whirlpoolIndex : list ) {
                if ( world.GetTiles( whirlpoolIndex ).isFog( hero.GetColor() ) )
                    return -3000.0;
            }
            return -dangerousTaskPenalty; // no point to even loose the army for this
        }
        else if ( objectID == MP2::OBJ_BOAT ) {
            // de-prioritize the water movement even harder
            return -5000.0;
        }
        else if ( objectID == MP2::OBJ_MAGICWELL ) {
            if ( !hero.HaveSpellBook() ) {
                return -dangerousTaskPenalty;
            }
            if ( hero.GetSpellPoints() * 2 >= hero.GetMaxSpellPoints() ) {
                return -2000; // no reason to visit the well with no magic book or with half of points
            }
            return 0;
        }
        else if ( objectID == MP2::OBJ_TEMPLE ) {
            const int moral = hero.GetMorale();
            if ( moral >= 3 ) {
                return -dangerousTaskPenalty; // no reason to visit with a maximum moral
            }
            else if ( moral == 2 ) {
                return -4000; // moral is good enough to avoid visting this object
            }
            else if ( moral == 1 ) {
                return -2000; // is it worth to visit this object with little better than neutral moral?
            }
            else if ( moral == 0 ) {
                return 0;
            }
            else {
                return 250;
            }
        }

        return 0;
    }

    int AI::Normal::getPriorityTarget( const Heroes & hero, double & maxPriority, int patrolIndex, uint32_t distanceLimit )
    {
        const double lowestPossibleValue = -1.0 * Maps::Ground::slowestMovePenalty * world.getSize();
        const bool heroInPatrolMode = patrolIndex != -1;
        const double heroStrength = hero.GetArmy().GetStrength();

        int priorityTarget = -1;
        maxPriority = lowestPossibleValue;
#ifdef WITH_DEBUG
        int objectID = MP2::OBJ_ZERO;
#endif

        // pre-cache the pathfinder
        _pathfinder.reEvaluateIfNeeded( hero );

        const uint32_t leftMovePoints = hero.GetMovePoints();

        ObjectValidator objectValidator( hero );
        ObjectValueStorage valueStorage( hero, *this, lowestPossibleValue );

        for ( size_t idx = 0; idx < _mapObjects.size(); ++idx ) {
            const IndexObject & node = _mapObjects[idx];

            // Skip if hero in patrol mode and object outside of reach
            if ( heroInPatrolMode && Maps::GetApproximateDistance( node.first, patrolIndex ) > distanceLimit )
                continue;

            if ( objectValidator.isValid( node.first ) ) {
                uint32_t dist = _pathfinder.getDistance( node.first );
                if ( dist == 0 )
                    continue;

                double value = valueStorage.value( node );

                const std::vector<IndexObject> & list = _pathfinder.getObjectsOnTheWay( node.first );
                for ( const IndexObject & pair : list ) {
                    if ( objectValidator.isValid( pair.first ) && std::binary_search( _mapObjects.begin(), _mapObjects.end(), pair ) )
                        value += valueStorage.value( pair );
                }
                const RegionStats & regionStats = _regions[world.GetTiles( node.first ).GetRegion()];
                if ( heroStrength < regionStats.highestThreat )
                    value -= dangerousTaskPenalty;

                if ( dist > leftMovePoints ) {
                    // Distant object which is out of reach for the current turn must have lower priority.
                    dist = leftMovePoints + ( dist - leftMovePoints ) * 2;
                }

                value = ScaleWithDistance( value, dist );

                if ( dist && value > maxPriority ) {
                    maxPriority = value;
                    priorityTarget = node.first;
#ifdef WITH_DEBUG
                    objectID = node.second;
#endif

                    DEBUG_LOG( DBG_AI, DBG_TRACE,
                               hero.GetName() << ": valid object at " << node.first << " value is " << value << " (" << MP2::StringObject( node.second ) << ")" );
                }
            }
        }

        if ( priorityTarget != -1 ) {
            DEBUG_LOG( DBG_AI, DBG_INFO,
                       hero.GetName() << ": priority selected: " << priorityTarget << " value is " << maxPriority << " (" << MP2::StringObject( objectID ) << ")" );
        }
        else if ( !heroInPatrolMode ) {
            priorityTarget = _pathfinder.getFogDiscoveryTile( hero );
            DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " can't find an object. Scouting the fog of war at " << priorityTarget );
        }

        return priorityTarget;
    }

    void Normal::HeroesActionComplete( Heroes & hero )
    {
        Castle * castle = hero.inCastle();
        if ( castle ) {
            ReinforceHeroInCastle( hero, *castle, castle->GetKingdom().GetFunds() );
        }
    }

    void Normal::HeroesTurn( VecHeroes & heroes )
    {
        std::vector<HeroToMove> availableHeroes;

        for ( Heroes * hero : heroes ) {
            if ( hero->Modes( Heroes::PATROL ) ) {
                if ( hero->GetSquarePatrol() == 0 ) {
                    DEBUG_LOG( DBG_AI, DBG_TRACE, hero->GetName() << " standing still. Skip turn." );
                    hero->SetModes( Heroes::MOVED );
                    continue;
                }
            }
            hero->ResetModes( Heroes::WAITING | Heroes::MOVED | Heroes::SKIPPED_TURN );
            if ( !hero->MayStillMove() ) {
                hero->SetModes( Heroes::MOVED );
            }
            else {
                availableHeroes.emplace_back();
                HeroToMove & heroInfo = availableHeroes.back();
                heroInfo.hero = hero;

                if ( hero->Modes( Heroes::PATROL ) ) {
                    heroInfo.patrolCenter = Maps::GetIndexFromAbsPoint( hero->GetCenterPatrol() );
                    heroInfo.patrolDistance = hero->GetSquarePatrol();
                }
            }
        }

        while ( !availableHeroes.empty() ) {
            Heroes * bestHero = availableHeroes.front().hero;
            double maxPriority = 0;
            int bestTargetIndex = -1;

            for ( HeroToMove & heroInfo : availableHeroes ) {
                double priority = -1;
                const int targetIndex = getPriorityTarget( *heroInfo.hero, priority, heroInfo.patrolCenter, heroInfo.patrolDistance );
                if ( targetIndex != -1 && ( priority > maxPriority || bestTargetIndex == -1 ) ) {
                    maxPriority = priority;
                    bestTargetIndex = targetIndex;
                    bestHero = heroInfo.hero;
                }
            }

            if ( bestTargetIndex == -1 ) {
                // Nothing to do. Stop eveything
                break;
            }

            _pathfinder.reEvaluateIfNeeded( *bestHero );
            bestHero->GetPath().setPath( _pathfinder.buildPath( bestTargetIndex ), bestTargetIndex );
            const int32_t idxToErase = bestHero->GetPath().GetDestinationIndex();

            HeroesMove( *bestHero );

            auto it = std::find_if( _mapObjects.begin(), _mapObjects.end(), [&idxToErase]( const IndexObject & o ) { return o.first == idxToErase; } );
            // Actually remove if this object single use only
            if ( it != _mapObjects.end() && !MP2::isCaptureObject( it->second ) && !MP2::isRemoveObject( it->second ) ) {
                // retains the vector order for binary search
                _mapObjects.erase( it );
            }

            for ( size_t i = 0; i < availableHeroes.size(); ) {
                if ( !availableHeroes[i].hero->MayStillMove() ) {
                    availableHeroes[i].hero->SetModes( Heroes::MOVED );
                    availableHeroes.erase( availableHeroes.begin() + i );
                    continue;
                }

                ++i;
            }
        }

        for ( HeroToMove & heroInfo : availableHeroes ) {
            if ( !heroInfo.hero->MayStillMove() ) {
                heroInfo.hero->SetModes( Heroes::MOVED );
            }
        }
    }
}
