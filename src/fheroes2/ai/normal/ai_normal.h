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
#include "world_pathfinding.h"

class Army;
class Castle;
class Funds;
class HeroBase;
class Heroes;
class Kingdom;
class Spell;

namespace Battle
{
    class Actions;
    class Arena;
    class Unit;
    class Units;
}

namespace Maps
{
    class Tiles;
}

namespace Rand
{
    class DeterministicRandomGenerator;
}

struct VecHeroes;
struct KingdomCastles;

namespace AI
{
    struct RegionStats
    {
        bool evaluated = false;
        double highestThreat = -1;
        double averageMonster = -1;
        int friendlyHeroes = 0;
        int friendlyCastles = 0;
        int enemyCastles = 0;
        int monsterCount = 0;
        int fogCount = 0;
        int safetyFactor = 0;
        int spellLevel = 2;
        std::vector<IndexObject> validObjects;
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

    struct HeroToMove
    {
        Heroes * hero = nullptr;
        int patrolCenter = -1;
        uint32_t patrolDistance = 0;
    };

    struct PriorityTask
    {
        PriorityTaskType type = PriorityTaskType::ATTACK;
        double threatLevel = 0.0;
        std::set<int> secondaryTaskTileId;

        PriorityTask() = default;
        PriorityTask( PriorityTaskType t, double threat )
            : type( t )
            , threatLevel( threat )
        {}

        PriorityTask( PriorityTaskType t, double threat, int secondaryTask )
            : type( t )
            , threatLevel( threat )
        {
            secondaryTaskTileId.insert( secondaryTask );
        }
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

        // decision-making helpers
        bool isUnitFaster( const Battle::Unit & currentUnit, const Battle::Unit & target ) const;
        bool isHeroWorthSaving( const Heroes & hero ) const;
        bool isCommanderCanSpellcast( const Battle::Arena & arena, const HeroBase * commander ) const;
        bool checkRetreatCondition( const Heroes & hero ) const;

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

        static double commanderMaximumSpellDamageValue( const HeroBase & commander );

        const Rand::DeterministicRandomGenerator * _randomGenerator = nullptr;

        // When this limit of turns without deaths is exceeded for an attacking AI-controlled hero,
        // the auto battle should be interrupted (one way or another)
        static const uint32_t MAX_TURNS_WITHOUT_DEATHS = 50;

        // Member variables related to the logic of checking the limit of the number of turns
        uint32_t _currentTurnNumber = 0;
        uint32_t _numberOfRemainingTurnsWithoutDeaths = MAX_TURNS_WITHOUT_DEATHS;
        uint32_t _attackerForceNumberOfDead = 0;
        uint32_t _defenderForceNumberOfDead = 0;

        // turn variables that wouldn't persist
        const HeroBase * _commander = nullptr;
        int _myColor = Color::NONE;
        double _myArmyStrength = 0;
        double _enemyArmyStrength = 0;
        double _myShooterStr = 0;
        double _enemyShooterStr = 0;
        double _myArmyAverageSpeed = 0;
        double _enemyAverageSpeed = 0;
        double _enemySpellStrength = 0;
        int _highestDamageExpected = 0;
        bool _attackingCastle = false;
        bool _defendingCastle = false;
        bool _considerRetreat = false;
        bool _defensiveTactics = false;
    };

    class Normal : public Base
    {
    public:
        Normal();

        void KingdomTurn( Kingdom & kingdom ) override;
        void BattleTurn( Battle::Arena & arena, const Battle::Unit & currentUnit, Battle::Actions & actions ) override;

        void revealFog( const Maps::Tiles & tile ) override;

        void HeroesPreBattle( HeroBase & hero, bool isAttacking ) override;
        void HeroesActionComplete( Heroes & hero, int32_t tileIndex, const MP2::MapObjectType objectType ) override;

        bool recruitHero( Castle & castle, bool buyArmy, bool underThreat );
        void reinforceHeroInCastle( Heroes & hero, Castle & castle, const Funds & budget );
        void evaluateRegionSafety();
        std::set<int> findCastlesInDanger( const KingdomCastles & castles, const std::vector<std::pair<int, const Army *>> & enemyArmies, int myColor );
        std::vector<AICastle> getSortedCastleList( const KingdomCastles & castles, const std::set<int> & castlesInDanger );

        double getObjectValue( const Heroes & hero, const int index, int & objectType, const double valueToIgnore, const uint32_t distanceToObject ) const;
        int getPriorityTarget( const HeroToMove & heroInfo, double & maxPriority );
        void resetPathfinder() override;

        void battleBegins() override;

        double getTargetArmyStrength( const Maps::Tiles & tile, const MP2::MapObjectType objectType );

        bool isCriticalTask( const int index ) const
        {
            return _priorityTargets.find( index ) != _priorityTargets.end();
        }

    private:
        // following data won't be saved/serialized
        double _combinedHeroStrength = 0;
        std::vector<IndexObject> _mapObjects;
        std::map<int, PriorityTask> _priorityTargets;
        std::vector<RegionStats> _regions;
        AIWorldPathfinder _pathfinder;
        BattlePlanner _battlePlanner;

        // Monster strength is constant over the same turn for AI but its calculation is a heavy operation.
        // In order to avoid extra computations during AI turn it is important to keep cache of monster strength but update it when an action on a monster is taken.
        std::map<int32_t, double> _neutralMonsterStrengthCache;

        void CastleTurn( Castle & castle, const bool defensiveStrategy );
        bool HeroesTurn( VecHeroes & heroes, const uint32_t startProgressValue, const uint32_t endProgressValue );

        double getGeneralObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        double getFighterObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        double getCourierObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        int getCourierMainTarget( const Heroes & hero, double lowestPossibleValue ) const;

        void updatePriorityTargets( Heroes & hero, const int32_t tileIndex, const MP2::MapObjectType objectType );

        bool purchaseNewHeroes( const std::vector<AICastle> & sortedCastleList, const std::set<int> & castlesInDanger, int32_t availableHeroCount,
                                bool moreTasksForHeroes );

        static bool isMonsterStrengthCacheable( const MP2::MapObjectType objectType )
        {
            return objectType == MP2::OBJ_MONSTER;
        }
    };
}

#endif
