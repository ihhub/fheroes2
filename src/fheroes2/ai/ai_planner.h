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
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "resource.h"
#include "world_pathfinding.h"

class Castle;
class HeroBase;
class Heroes;
class Kingdom;

struct VecCastles;
struct VecHeroes;

namespace MP2
{
    enum MapObjectType : uint16_t;
}

namespace Maps
{
    class Tile;
}

namespace Skill
{
    class Secondary;
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

    struct EnemyArmy
    {
        EnemyArmy() = default;

        EnemyArmy( const int32_t index_, const Heroes * hero_, const double strength_, const uint32_t movePoints_ )
            : index( index_ )
            , hero( hero_ )
            , strength( strength_ )
            , movePoints( movePoints_ )
        {}

        int32_t index{ -1 };
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

        void revealFog( const Maps::Tile & tile, const Kingdom & kingdom );

        bool isValidHeroObject( const Heroes & hero, const int32_t index, const bool underHero );

        double getObjectValue( const Heroes & hero, const int32_t index, const MP2::MapObjectType objectType, const double valueToIgnore,
                               const uint32_t distanceToObject ) const;

        // Returns the strength of the army guarding the given tile. Note that the army is obtained by calling
        // Army::setFromTile(), so this method is not suitable for hero armies or castle garrisons.
        double getTileArmyStrength( const Maps::Tile & tile );

        static void HeroesPreBattle( HeroBase & hero, bool isAttacking );
        static void CastlePreBattle( Castle & castle );

        static Skill::Secondary pickSecondarySkill( const Heroes & hero, const Skill::Secondary & left, const Skill::Secondary & right );

    private:
        Planner() = default;

        void CastleTurn( Castle & castle, const bool defensiveStrategy );

        // Returns true if heroes can still do tasks but they have no move points.
        bool HeroesTurn( VecHeroes & heroes, uint32_t & currentProgressValue, uint32_t endProgressValue );

        bool recruitHero( Castle & castle, bool buyArmy );
        void reinforceHeroInCastle( Heroes & hero, Castle & castle, const Funds & budget );

        void evaluateRegionSafety();

        std::vector<AICastle> getSortedCastleList( const VecCastles & castles, const std::set<int> & castlesInDanger );

        int getPriorityTarget( Heroes & hero, double & maxPriority );

        double getGeneralObjectValue( const Heroes & hero, const int32_t index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        double getFighterObjectValue( const Heroes & hero, const int32_t index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        double getCourierObjectValue( const Heroes & hero, const int32_t index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        double getScoutObjectValue( const Heroes & hero, const int32_t index, const double valueToIgnore, const uint32_t distanceToObject ) const;

        int getCourierMainTarget( const Heroes & hero, const double lowestPossibleValue );

        double getResourcePriorityModifier( const int resource, const bool isMine ) const;
        double getFundsValueBasedOnPriority( const Funds & funds ) const;

        void updatePriorityTargets( Heroes & hero, const int32_t tileIndex, const MP2::MapObjectType objectType );
        void updateKingdomBudget( const Kingdom & kingdom );

        bool purchaseNewHeroes( const std::vector<AICastle> & sortedCastleList, const std::set<int> & castlesInDanger, const int32_t availableHeroCount,
                                const bool moreTasksForHeroes );

        void updateMapActionObjectCache( const int mapIndex );

        std::set<int> findCastlesInDanger( const Kingdom & kingdom );

        void updatePriorityForEnemyArmy( const Kingdom & kingdom, const EnemyArmy & enemyArmy );
        void updatePriorityForCastle( const Castle & castle );

        // Return true if the castle is in danger.
        // IMPORTANT!!! Do not call this method directly. Use other methods which call it internally.
        bool updateIndividualPriorityForCastle( const Castle & castle, const EnemyArmy & enemyArmy );

        void removePriorityAttackTarget( const int32_t tileIndex );
        void updatePriorityAttackTarget( const Kingdom & kingdom, const Maps::Tile & tile );

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

        std::unordered_map<int32_t, MP2::MapObjectType> _mapActionObjects;
        std::unordered_map<int32_t, PriorityTask> _priorityTargets;
        std::unordered_map<int32_t, EnemyArmy> _enemyArmies;

        // Strength of the armies guarding the tiles (neutral monsters, guardians of dwellings, and so on) is constant for AI
        // during the same turn, but its calculation is a heavy operation, so it needs to be cached to speed up estimations.
        // It is important to update this cache after performing an action on the corresponding tile.
        std::unordered_map<int32_t, double> _tileArmyStrengthValues;

        std::vector<RegionStats> _regions;

        std::array<BudgetEntry, 7> _budget = { Resource::WOOD, Resource::MERCURY, Resource::ORE, Resource::SULFUR, Resource::CRYSTAL, Resource::GEMS, Resource::GOLD };

        AIWorldPathfinder _pathfinder;
    };
}
