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
#include "world_pathfinding.h"

namespace Battle
{
    class Units;
}

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

    struct BattleTargetPair
    {
        int cell = -1;
        const Battle::Unit * unit = nullptr;
    };

    struct SpellSeletion
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
        Battle::Actions planUnitTurn( Battle::Arena & arena, const Battle::Unit & currentUnit );
        void analyzeBattleState( Battle::Arena & arena, const Battle::Unit & currentUnit );

        // decision-making helpers
        bool isUnitFaster( const Battle::Unit & currentUnit, const Battle::Unit & target ) const;
        bool isHeroWorthSaving( const Heroes & hero ) const;
        bool isCommanderCanSpellcast( const Battle::Arena & arena, const HeroBase * commander ) const;
        bool checkRetreatCondition( const Heroes & hero ) const;

    private:
        // to be exposed later once every BattlePlanner will be re-initialized at combat start
        Battle::Actions berserkTurn( Battle::Arena & arena, const Battle::Unit & currentUnit ) const;
        Battle::Actions archerDecision( Battle::Arena & arena, const Battle::Unit & currentUnit ) const;
        BattleTargetPair meleeUnitOffense( Battle::Arena & arena, const Battle::Unit & currentUnit ) const;
        BattleTargetPair meleeUnitDefense( Battle::Arena & arena, const Battle::Unit & currentUnit ) const;
        SpellSeletion selectBestSpell( Battle::Arena & arena, bool retreating ) const;
        SpellcastOutcome spellDamageValue( const Spell & spell, Battle::Arena & arena, const Battle::Units & friendly, const Battle::Units & enemies,
                                           bool retreating ) const;
        SpellcastOutcome spellDispellValue( const Spell & spell, const Battle::Units & friendly, const Battle::Units & enemies ) const;
        SpellcastOutcome spellResurrectValue( const Spell & spell, Battle::Arena & arena ) const;
        SpellcastOutcome spellSummonValue( const Spell & spell ) const;
        SpellcastOutcome spellEffectValue( const Spell & spell, const Battle::Units & targets ) const;
        double spellEffectValue( const Spell & spell, const Battle::Unit & target, bool targetIsLast, bool forDispell ) const;
        uint32_t spellDurationMultiplier( const Battle::Unit & target ) const;

        // turn variables that wouldn't persist
        const HeroBase * _commander = nullptr;
        int _myColor = Color::NONE;
        double _myArmyStrength = 0;
        double _enemyArmyStrength = 0;
        double _myShooterStr = 0;
        double _enemyShooterStr = 0;
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
        void CastleTurn( Castle & castle, bool defensive ) override;
        void BattleTurn( Battle::Arena & arena, const Battle::Unit & currentUnit, Battle::Actions & actions ) override;
        void HeroesTurn( VecHeroes & heroes ) override;

        void revealFog( const Maps::Tiles & tile ) override;

        void HeroesPreBattle( HeroBase & hero, bool isAttacking ) override;
        void HeroesActionComplete( Heroes & hero ) override;

        double getObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        int getPriorityTarget( const Heroes & hero, double & maxPriority, int patrolIndex = -1, uint32_t distanceLimit = 0 );
        void resetPathfinder() override;

    private:
        // following data won't be saved/serialized
        double _combinedHeroStrength = 0;
        std::vector<IndexObject> _mapObjects;
        std::vector<RegionStats> _regions;
        AIWorldPathfinder _pathfinder;
        BattlePlanner _battlePlanner;
    };
}

#endif
