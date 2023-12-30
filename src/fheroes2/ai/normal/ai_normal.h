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

#ifndef H2AI_NORMAL_H
#define H2AI_NORMAL_H

#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "ai.h"
#include "color.h"
#include "mp2.h"
#include "pairs.h"
#include "resource.h"
#include "world_pathfinding.h"

class Castle;
class HeroBase;
class Heroes;
class Kingdom;
class Spell;

namespace Battle
{
    class Actions;
    class Arena;
    class Position;
    class Unit;
    class Units;
}

namespace Maps
{
    class Tiles;
}

struct VecHeroes;
struct VecCastles;

namespace AI
{
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

        EnemyArmy( const int32_t index_, const Heroes * hero_, const double strength_, const uint32_t movePoints_ )
            : index( index_ )
            , hero( hero_ )
            , strength( strength_ )
            , movePoints( movePoints_ )
        {}

        int32_t index{ -1 };
        const Heroes * hero = nullptr;
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

    struct BattleTargetPair
    {
        int cell = -1;
        const Battle::Unit * unit = nullptr;
    };

    struct SpellSelection
    {
        int spellID = -1;
        int32_t cell = -1;
        double value = 0.0;
    };

    struct SpellcastOutcome
    {
        int32_t cell = -1;
        double value = 0.0;

        void updateOutcome( const double potentialValue, const int32_t targetCell, const bool isMassEffect = false )
        {
            if ( isMassEffect ) {
                value += potentialValue;
            }
            else if ( potentialValue > value ) {
                value = potentialValue;
                cell = targetCell;
            }
        }
    };

    class BattlePlanner
    {
    public:
        // Should be called at the beginning of the battle
        void battleBegins();

        // Checks whether the limit of turns is exceeded for the attacking AI-controlled
        // hero and inserts an appropriate action to the action list if necessary
        bool isLimitOfTurnsExceeded( const Battle::Arena & arena, Battle::Actions & actions );

        Battle::Actions planUnitTurn( Battle::Arena & arena, const Battle::Unit & currentUnit );

    private:
        void analyzeBattleState( const Battle::Arena & arena, const Battle::Unit & currentUnit );

        static Battle::Actions berserkTurn( Battle::Arena & arena, const Battle::Unit & currentUnit );
        Battle::Actions archerDecision( Battle::Arena & arena, const Battle::Unit & currentUnit ) const;

        BattleTargetPair meleeUnitOffense( Battle::Arena & arena, const Battle::Unit & currentUnit ) const;
        BattleTargetPair meleeUnitDefense( Battle::Arena & arena, const Battle::Unit & currentUnit ) const;

        SpellSelection selectBestSpell( Battle::Arena & arena, const Battle::Unit & currentUnit, bool retreating ) const;

        SpellcastOutcome spellDamageValue( const Spell & spell, Battle::Arena & arena, const Battle::Unit & currentUnit, const Battle::Units & friendly,
                                           const Battle::Units & enemies, bool retreating ) const;
        SpellcastOutcome spellDispelValue( const Spell & spell, const Battle::Units & friendly, const Battle::Units & enemies ) const;
        SpellcastOutcome spellResurrectValue( const Spell & spell, const Battle::Arena & arena ) const;
        SpellcastOutcome spellSummonValue( const Spell & spell, const Battle::Arena & arena, const int heroColor ) const;
        SpellcastOutcome spellEffectValue( const Spell & spell, const Battle::Units & targets ) const;

        double spellEffectValue( const Spell & spell, const Battle::Unit & target, bool targetIsLast, bool forDispel ) const;
        double getSpellDisruptingRayRatio( const Battle::Unit & target ) const;
        double getSpellSlowRatio( const Battle::Unit & target ) const;
        double getSpellHasteRatio( const Battle::Unit & target ) const;
        int32_t spellDurationMultiplier( const Battle::Unit & target ) const;

        bool isPositionLocatedInDefendedArea( const Battle::Unit & currentUnit, const Battle::Position & pos ) const;
        bool isUnitFaster( const Battle::Unit & currentUnit, const Battle::Unit & target ) const;
        bool isHeroWorthSaving( const Heroes & hero ) const;
        bool isCommanderCanSpellcast( const Battle::Arena & arena, const HeroBase * commander ) const;

        bool checkRetreatCondition( const Heroes & hero ) const;

        static double commanderMaximumSpellDamageValue( const HeroBase & commander );

        // When this limit of turns without deaths is exceeded for an attacking AI-controlled hero,
        // the auto battle should be interrupted (one way or another)
        static const uint32_t MAX_TURNS_WITHOUT_DEATHS = 50;

        // Member variables related to the logic of checking the limit of the number of turns
        uint32_t _currentTurnNumber = 0;
        uint32_t _numberOfRemainingTurnsWithoutDeaths = MAX_TURNS_WITHOUT_DEATHS;
        uint32_t _attackerForceNumberOfDead = 0;
        uint32_t _defenderForceNumberOfDead = 0;

        // Member variables with a lifetime in one turn
        const HeroBase * _commander = nullptr;
        int _myColor = Color::NONE;
        double _myArmyStrength = 0;
        double _enemyArmyStrength = 0;
        double _myShootersStrength = 0;
        double _enemyShootersStrength = 0;
        double _myRangedUnitsOnly = 0;
        double _enemyRangedUnitsOnly = 0;
        double _myArmyAverageSpeed = 0;
        double _enemyAverageSpeed = 0;
        double _enemySpellStrength = 0;
        bool _attackingCastle = false;
        bool _defendingCastle = false;
        bool _considerRetreat = false;
        bool _defensiveTactics = false;
        bool _cautiousOffensive = false;
    };

    class Normal : public Base
    {
    public:
        Normal();

        void KingdomTurn( Kingdom & kingdom ) override;
        void BattleTurn( Battle::Arena & arena, const Battle::Unit & currentUnit, Battle::Actions & actions ) override;

        void revealFog( const Maps::Tiles & tile, const Kingdom & kingdom ) override;

        // Implements the logic of transparent casting of the Summon Boat spell at the beginning of the hero's movement
        void HeroesBeginMovement( Heroes & hero ) override;
        void HeroesPreBattle( HeroBase & hero, bool isAttacking ) override;
        void HeroesActionComplete( Heroes & hero, const int32_t tileIndex, const MP2::MapObjectType objectType ) override;
        // Implements the logic of transparent casting of the Summon Boat spell during the hero's movement
        void HeroesActionNewPosition( Heroes & hero ) override;

        void CastlePreBattle( Castle & castle ) override;

        bool recruitHero( Castle & castle, bool buyArmy, bool underThreat );
        void reinforceHeroInCastle( Heroes & hero, Castle & castle, const Funds & budget );
        void evaluateRegionSafety();
        std::vector<AICastle> getSortedCastleList( const VecCastles & castles, const std::set<int> & castlesInDanger );

        void resetPathfinder() override;
        bool isValidHeroObject( const Heroes & hero, const int32_t index, const bool underHero ) override;

        void battleBegins() override;

        void tradingPostVisitEvent( Kingdom & kingdom ) override;

        double getObjectValue( const Heroes & hero, const int index, const int objectType, const double valueToIgnore, const uint32_t distanceToObject ) const;
        double getTargetArmyStrength( const Maps::Tiles & tile, const MP2::MapObjectType objectType );

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

    private:
        void CastleTurn( Castle & castle, const bool defensiveStrategy );

        // Returns true if heroes can still do tasks but they have no move points.
        bool HeroesTurn( VecHeroes & heroes, const uint32_t startProgressValue, const uint32_t endProgressValue );

        int getPriorityTarget( const HeroToMove & heroInfo, double & maxPriority );

        double getGeneralObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        double getFighterObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        double getCourierObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        int getCourierMainTarget( const Heroes & hero, const AIWorldPathfinder & pathfinder, double lowestPossibleValue ) const;
        double getResourcePriorityModifier( const int resource, const bool isMine ) const;

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

        // The following member variables should not be saved or serialized
        double _combinedHeroStrength = 0;
        std::vector<IndexObject> _mapActionObjects;
        std::map<int32_t, PriorityTask> _priorityTargets;
        std::map<int32_t, EnemyArmy> _enemyArmies;
        std::vector<RegionStats> _regions;
        std::array<BudgetEntry, 7> _budget = { Resource::WOOD, Resource::MERCURY, Resource::ORE, Resource::SULFUR, Resource::CRYSTAL, Resource::GEMS, Resource::GOLD };
        AIWorldPathfinder _pathfinder;
        BattlePlanner _battlePlanner;

        // Monster strength is constant over the same turn for AI but its calculation is a heavy operation.
        // In order to avoid extra computations during AI turn it is important to keep cache of monster strength but update it when an action on a monster is taken.
        std::map<int32_t, double> _neutralMonsterStrengthCache;
    };
}

#endif
