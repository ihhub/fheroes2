/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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

#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "color.h"
#include "mp2.h"
#include "pairs.h"
#include "resource.h"
#include "world_pathfinding.h"

class Castle;
class HeroBase;
class Heroes;
class Kingdom;

struct VecCastles;
struct VecHeroes;

namespace Maps
{
    class Tiles;
}

namespace AI
{
    // Although AI heroes are capable to find their own tasks strategic AI should be able to focus them on most critical tasks
    enum class PriorityTaskType : int
    {
        // AI will focus on siegeing or chasing the selected enemy castle or hero.
        ATTACK,

        // Target will usually be a friendly castle. AI will move heroes to defend and garrison it.
        DEFEND,

        // Target must be a friendly castle or hero. AI with such priority set should focus on bringing more troops to the target.
        REINFORCE
    };

    // TODO: this structure is not being updated during AI heroes' actions.
    struct RegionStats
    {
        bool evaluated = false;
        double highestThreat = -1;
        int friendlyHeroes = 0;
        int friendlyCastles = 0;
        int enemyCastles = 0;
        int safetyFactor = 0;
        int spellLevel = 2;
    };

    struct AICastle
    {
        Castle * castle = nullptr;
        bool underThreat = false;
        int safetyFactor = 0;
        int buildingValue = 0;

        AICastle( Castle * inCastle, bool inThreat, int inSafety, int inValue )
            : castle( inCastle )
            , underThreat( inThreat )
            , safetyFactor( inSafety )
            , buildingValue( inValue )
        {
            assert( castle != nullptr );
        }
    };

    struct BudgetEntry
    {
        int resource = Resource::UNKNOWN;
        int missing = 0;
        bool priority = false;
        bool recurringCost = false;

        BudgetEntry( int type )
            : resource( type )
        {}

        void reset()
        {
            missing = 0;
            priority = false;
            recurringCost = false;
        }
    };

    struct HeroToMove
    {
        Heroes * hero = nullptr;
        int patrolCenter = -1;
        uint32_t patrolDistance = 0;
    };

    struct EnemyArmy
    {
        EnemyArmy() = default;

        EnemyArmy( const int32_t index_, const int color_, const Heroes * hero_, const double strength_, const uint32_t movePoints_ )
            : index( index_ )
            , color( color_ )
            , hero( hero_ )
            , strength( strength_ )
            , movePoints( movePoints_ )
        {}

        int32_t index{ -1 };
        int color{ Color::NONE };
        const Heroes * hero{ nullptr };
        double strength{ 0 };
        uint32_t movePoints{ 0 };
    };

    struct PriorityTask
    {
        PriorityTask() = default;

        explicit PriorityTask( PriorityTaskType taskType )
            : type( taskType )
        {}

        PriorityTask( PriorityTaskType taskType, int secondaryTask )
            : type( taskType )
        {
            secondaryTaskTileId.insert( secondaryTask );
        }

        PriorityTaskType type{ PriorityTaskType::ATTACK };
        std::set<int32_t> secondaryTaskTileId;
    };

    class Planner
    {
    public:
        static Planner & Get();

        void KingdomTurn( Kingdom & kingdom );

        // Implements the logic of transparent casting of the Summon Boat spell at the beginning of the hero's movement
        void HeroesBeginMovement( Heroes & hero );
        // Implements the logic of transparent casting of the Summon Boat spell during the hero's movement
        void HeroesActionNewPosition( Heroes & hero );

        void HeroesActionComplete( Heroes & hero, const int32_t tileIndex, const MP2::MapObjectType objectType );

        void resetPathfinder();

        void revealFog( const Maps::Tiles & tile, const Kingdom & kingdom );

        bool isValidHeroObject( const Heroes & hero, const int32_t index, const bool underHero );

        double getObjectValue( const Heroes & hero, const int index, const int objectType, const double valueToIgnore, const uint32_t distanceToObject ) const;
        double getTargetArmyStrength( const Maps::Tiles & tile, const MP2::MapObjectType objectType );

        static void HeroesPreBattle( HeroBase & hero, bool isAttacking );
        static void CastlePreBattle( Castle & castle );

    private:
        Planner() = default;

        void CastleTurn( Castle & castle, const bool defensiveStrategy );

        // Returns true if heroes can still do tasks but they have no move points.
        bool HeroesTurn( VecHeroes & heroes, const uint32_t startProgressValue, const uint32_t endProgressValue );

        bool recruitHero( Castle & castle, bool buyArmy );
        void reinforceHeroInCastle( Heroes & hero, Castle & castle, const Funds & budget );

        void evaluateRegionSafety();

        std::vector<AICastle> getSortedCastleList( const VecCastles & castles, const std::set<int> & castlesInDanger );

        int getPriorityTarget( const HeroToMove & heroInfo, double & maxPriority );

        double getGeneralObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        double getFighterObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        double getCourierObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        double getScoutObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        int getCourierMainTarget( const Heroes & hero, const AIWorldPathfinder & pathfinder, double lowestPossibleValue ) const;
        double getResourcePriorityModifier( const int resource, const bool isMine ) const;
        double getFundsValueBasedOnPriority( const Funds & funds ) const;

        void updatePriorityTargets( Heroes & hero, const int32_t tileIndex, const MP2::MapObjectType objectType );
        void updateKingdomBudget( const Kingdom & kingdom );

        bool purchaseNewHeroes( const std::vector<AICastle> & sortedCastleList, const std::set<int> & castlesInDanger, const int32_t availableHeroCount,
                                const bool moreTasksForHeroes );

        static bool isMonsterStrengthCacheable( const MP2::MapObjectType objectType )
        {
            return objectType == MP2::OBJ_MONSTER;
        }

        void updateMapActionObjectCache( const int mapIndex );

        std::set<int> findCastlesInDanger( const Kingdom & kingdom );

        void updatePriorityForEnemyArmy( const Kingdom & kingdom, const EnemyArmy & enemyArmy );
        void updatePriorityForCastle( const Castle & castle );

        // Return true if the castle is in danger.
        // IMPORTANT!!! Do not call this method directly. Use other methods which call it internally.
        bool updateIndividualPriorityForCastle( const Castle & castle, const EnemyArmy & enemyArmy );

        void removePriorityAttackTarget( const int32_t tileIndex );
        void updatePriorityAttackTarget( const Kingdom & kingdom, const Maps::Tiles & tile );

        bool isPriorityTask( const int32_t index ) const
        {
            return _priorityTargets.find( index ) != _priorityTargets.end();
        }

        bool isCriticalTask( const int32_t index ) const
        {
            const auto iter = _priorityTargets.find( index );
            if ( iter == _priorityTargets.end() ) {
                return false;
            }

            return iter->second.type == PriorityTaskType::ATTACK || iter->second.type == PriorityTaskType::DEFEND;
        }

        // The following member variables should not be saved or serialized
        std::vector<IndexObject> _mapActionObjects;
        std::map<int32_t, PriorityTask> _priorityTargets;
        std::map<int32_t, EnemyArmy> _enemyArmies;
        std::vector<RegionStats> _regions;
        std::array<BudgetEntry, 7> _budget = { Resource::WOOD, Resource::MERCURY, Resource::ORE, Resource::SULFUR, Resource::CRYSTAL, Resource::GEMS, Resource::GOLD };
        AIWorldPathfinder _pathfinder;

        // Monster strength is constant over the same turn for AI but its calculation is a heavy operation.
        // In order to avoid extra computations during AI turn it is important to keep cache of monster strength but update it when an action on a monster is taken.
        std::map<int32_t, double> _neutralMonsterStrengthCache;
    };
}
