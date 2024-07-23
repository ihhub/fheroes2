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

#include <cstdint>

#include "color.h"

class HeroBase;
class Spell;

namespace Battle
{
    class Actions;
    class Arena;
    class Position;
    class Unit;
    class Units;
}

namespace AI
{
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
        static BattlePlanner & Get();

        // Should be called at the beginning of the battle
        void battleBegins();

        void BattleTurn( Battle::Arena & arena, const Battle::Unit & currentUnit, Battle::Actions & actions );

    private:
        BattlePlanner() = default;

        // Checks whether the limit of turns is exceeded for the attacking AI-controlled
        // hero and inserts an appropriate action to the action list if necessary
        bool isLimitOfTurnsExceeded( const Battle::Arena & arena, Battle::Actions & actions );

        Battle::Actions planUnitTurn( Battle::Arena & arena, const Battle::Unit & currentUnit );

        void analyzeBattleState( const Battle::Arena & arena, const Battle::Unit & currentUnit );

        Battle::Actions archerDecision( Battle::Arena & arena, const Battle::Unit & currentUnit ) const;

        BattleTargetPair meleeUnitOffense( Battle::Arena & arena, const Battle::Unit & currentUnit ) const;
        BattleTargetPair meleeUnitDefense( Battle::Arena & arena, const Battle::Unit & currentUnit ) const;

        bool isPositionLocatedInDefendedArea( const Battle::Unit & currentUnit, const Battle::Position & pos ) const;

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
}
