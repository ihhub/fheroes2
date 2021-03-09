/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include "ai_normal.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "battle_board.h"
#include "battle_command.h"
#include "battle_troop.h"
#include "heroes.h"
#include "logging.h"

using namespace Battle;

namespace AI
{
    SpellSeletion BattlePlanner::selectBestSpell( Arena & arena, bool retreating ) const
    {
        // Cast best spell with highest heuristic on target pointer saved
        SpellSeletion bestSpell;

        // Commander must be set before calling this function! Check both debug/release version
        assert( _commander != nullptr );
        if ( _commander == nullptr ) {
            return bestSpell;
        }

        const std::vector<Spell> allSpells = _commander->GetSpells();
        const Units friendly( arena.GetForce( _myColor ), true );
        const Units enemies( arena.GetForce( _myColor, true ), true );

        double bestHeuristic = 0;

        for ( const Spell & spell : allSpells ) {
            if ( !_commander->HaveSpellPoints( spell ) || !spell.isCombat() || ( !spell.isDamage() && retreating ) )
                continue;

            if ( spell.isDamage() ) {
                const SpellcastOutcome & outcome = spellDamageValue( spell, arena, friendly, enemies, retreating );
                if ( outcome.value > bestHeuristic ) {
                    bestHeuristic = outcome.value;
                    bestSpell.spellID = spell.GetID();
                    bestSpell.cell = outcome.cell;
                }
            }
        }
        return bestSpell;
    }

    SpellcastOutcome BattlePlanner::spellDamageValue( const Spell & spell, Arena & arena, const Units & friendly, const Units & enemies, bool retreating ) const
    {
        SpellcastOutcome bestOutcome;
        if ( !spell.isDamage() )
            return bestOutcome;

        const int spellPower = _commander->GetPower();
        const uint32_t totalDamage = spell.Damage() * spellPower;

        auto damageHeuristic = [&totalDamage, &spell, &spellPower, &retreating]( const Unit * unit ) {
            const uint32_t damage = totalDamage * ( 100 - unit->GetMagicResist( spell, spellPower ) ) / 100;
            // If we're retreating we don't care about partial damage, only actual units killed
            if ( retreating )
                return unit->GetMonsterStrength() * unit->HowManyWillKilled( damage );
            // Otherwise calculate amount of strength lost (% of unit times total strength)
            return std::min( static_cast<double>( damage ) / unit->GetHitPoints(), 1.0 ) * unit->GetStrength();
        };

        if ( spell.isSingleTarget() ) {
            for ( const Unit * enemy : enemies ) {
                const double spellHeuristic = damageHeuristic( enemy );

                if ( spellHeuristic > bestOutcome.value ) {
                    bestOutcome.value = spellHeuristic;
                    bestOutcome.cell = enemy->GetHeadIndex();
                }
            }
        }
        else if ( spell.isApplyWithoutFocusObject() ) {
            double spellHeuristic = 0;
            for ( const Unit * enemy : enemies ) {
                spellHeuristic += damageHeuristic( enemy );
            }
            for ( const Unit * unit : friendly ) {
                spellHeuristic -= damageHeuristic( unit );
            }

            if ( spellHeuristic > bestOutcome.value ) {
                bestOutcome.value = spellHeuristic;
            }
        }
        else {
            // Area of effect spells like Fireball
            auto areaOfEffectCheck = [&damageHeuristic, &bestOutcome]( const TargetsInfo & targets, const int32_t index, int myColor ) {
                double spellHeuristic = 0;
                for ( const TargetInfo & target : targets ) {
                    if ( target.defender->GetCurrentColor() == myColor ) {
                        spellHeuristic -= damageHeuristic( target.defender );
                    }
                    else {
                        spellHeuristic += damageHeuristic( target.defender );
                    }
                }

                if ( spellHeuristic > bestOutcome.value ) {
                    bestOutcome.value = spellHeuristic;
                    bestOutcome.cell = index;
                }
            };

            if ( spell.GetID() == Spell::CHAINLIGHTNING ) {
                for ( const Unit * enemy : enemies ) {
                    const int32_t index = enemy->GetHeadIndex();
                    areaOfEffectCheck( arena.GetTargetsForSpells( _commander, spell, index, false ), index, _myColor );
                }
            }
            else {
                const Board & board = *Arena::GetBoard();
                for ( const Cell & cell : board ) {
                    const int32_t index = cell.GetIndex();
                    areaOfEffectCheck( arena.GetTargetsForSpells( _commander, spell, index, false ), index, _myColor );
                }
            }
        }

        return bestOutcome;
    }
}
