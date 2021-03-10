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
    double ReduceEffectivenessByDistance( const Unit * unit )
    {
        // Reduce spell effectiveness if unit already crossed the battlefield
        const int xPos = unit->GetHeadIndex() % ARENAW;
        return std::max( 1, unit->isReflect() ? ARENAW - xPos - 1 : xPos );
    }

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
        auto checkSelectBestSpell = [this, &bestHeuristic, &bestSpell]( const Spell & spell, const SpellcastOutcome & outcome ) {
            // Diminish spell effectiveness based on spell point cost
            // 1. Divide cost by 3 to make level 1 spells a baseline (1:1)
            // 2. Use square root to make sure relationship isn't linear for high-level spells
            const double spellPointValue = outcome.value / sqrt( spell.SpellPoint( _commander ) / 3.0 );
            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, spell.GetName() << " value is " << spellPointValue << ", best target is " << outcome.cell );

            if ( spellPointValue > bestHeuristic ) {
                bestHeuristic = spellPointValue;
                bestSpell.spellID = spell.GetID();
                bestSpell.cell = outcome.cell;
            }
        };

        for ( const Spell & spell : allSpells ) {
            if ( !_commander->HaveSpellPoints( spell ) || !spell.isCombat() || ( !spell.isDamage() && retreating ) )
                continue;

            if ( spell.isDamage() ) {
                checkSelectBestSpell( spell, spellDamageValue( spell, arena, friendly, enemies, retreating ) );
            }
            else if ( spell.isApplyToEnemies() ) {
                checkSelectBestSpell( spell, spellDebuffValue( spell, enemies ) );
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

    SpellcastOutcome BattlePlanner::spellDebuffValue( const Spell & spell, const Units & enemies ) const
    {
        SpellcastOutcome bestOutcome;
        const int spellID = spell.GetID();
        const bool lastEnemyLeft = enemies.size() == 1;

        double ratio = 0.0;
        switch ( spellID ) {
        case Spell::SLOW:
        case Spell::MASSSLOW:
            ratio = 0.3;
            break;
        case Spell::BLIND: {
            if ( lastEnemyLeft )
                return bestOutcome;
            ratio = 0.85;
            break;
        }
        case Spell::CURSE:
        case Spell::MASSCURSE:
            ratio = 0.15;
            break;
        case Spell::BERSERKER: {
            if ( lastEnemyLeft )
                return bestOutcome;
            ratio = 0.95;
            break;
        }
        case Spell::PARALYZE: {
            if ( lastEnemyLeft )
                return bestOutcome;
            ratio = 0.9;
            break;
        }
        case Spell::HYPNOTIZE: {
            if ( lastEnemyLeft )
                return bestOutcome;
            ratio = 1.5;
            break;
        }
        case Spell::DISRUPTINGRAY:
            ratio = 0.2;
            break;
        default:
            return bestOutcome;
        }

        const bool isMassSpell = spell.isMassActions();
        for ( const Unit * unit : enemies ) {
            // Make sure this spell can be applied to current unit
            if ( unit->isUnderSpellEffect( spell ) || !unit->AllowApplySpell( spell, _commander ) ) {
                continue;
            }

            if ( spellID == Spell::SLOW || spellID == Spell::MASSSLOW ) {
                if ( unit->Modes( SP_HASTE ) ) {
                    ratio *= 2;
                }
                else if ( unit->isArchers() || _attackingCastle ) {
                    // Slow is useless against archers or troops defending castle
                    ratio = 0.01;
                }
                else if ( !unit->isFlying() ) {
                    ratio /= ReduceEffectivenessByDistance( unit );
                }
            }
            else if ( spellID == Spell::BERSERKER && !unit->isArchers() ) {
                ratio /= ReduceEffectivenessByDistance( unit );
            }
            else if ( unit->Modes( SP_BLESS ) && ( spellID == Spell::CURSE || spellID == Spell::MASSCURSE ) ) {
                ratio *= 2;
            }

            const double spellValue = unit->GetStrength() * ratio;
            if ( isMassSpell ) {
                bestOutcome.value += spellValue;
            }
            else if ( spellValue > bestOutcome.value ) {
                bestOutcome.value = spellValue;
                bestOutcome.cell = unit->GetHeadIndex();
            }
        }
        return bestOutcome;
    }
}
