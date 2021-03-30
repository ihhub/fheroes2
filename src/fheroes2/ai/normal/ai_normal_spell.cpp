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
    const double ANTIMAGIC_LOW_LIMIT = 200.0;

    double ReduceEffectivenessByDistance( const Unit & unit )
    {
        // Reduce spell effectiveness if unit already crossed the battlefield
        const int xPos = unit.GetHeadIndex() % ARENAW;
        return std::max( 1, unit.isReflect() ? ARENAW - xPos - 1 : xPos );
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
            else if ( spell.isEffectDispel() ) {
                checkSelectBestSpell( spell, spellDispellValue( spell, friendly, enemies ) );
            }
            else if ( spell.isSummon() ) {
                checkSelectBestSpell( spell, spellSummonValue( spell ) );
            }
            else if ( spell.isResurrect() ) {
                checkSelectBestSpell( spell, spellResurrectValue( spell, arena ) );
            }
            else if ( spell.isApplyToFriends() ) {
                checkSelectBestSpell( spell, spellEffectValue( spell, friendly ) );
            }
            else if ( spell.isApplyToEnemies() ) {
                checkSelectBestSpell( spell, spellEffectValue( spell, enemies ) );
            }
        }
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "Best spell " << Spell( bestSpell.spellID ).GetName() << ", value is " << bestHeuristic );

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
            double unitPercentageLost = std::min( static_cast<double>( damage ) / unit->GetHitPoints(), 1.0 );

            // Penalty for waking up disabled unit (if you kill only 30%, rest 70% is your penalty)
            if ( unit->Modes( SP_BLIND | SP_PARALYZE | SP_STONE ) ) {
                unitPercentageLost += unitPercentageLost - 1.0;
            }
            return unitPercentageLost * unit->GetStrength();
        };

        if ( spell.isSingleTarget() ) {
            for ( const Unit * enemy : enemies ) {
                bestOutcome.updateOutcome( damageHeuristic( enemy ), enemy->GetHeadIndex() );
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

            bestOutcome.updateOutcome( spellHeuristic, -1 );
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

                bestOutcome.updateOutcome( spellHeuristic, index );
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

    double BattlePlanner::spellEffectValue( const Spell & spell, const Battle::Unit & target, bool targetIsLast ) const
    {
        // Make sure this spell can be applied to current unit
        if ( target.isUnderSpellEffect( spell ) || !target.AllowApplySpell( spell, _commander ) ) {
            return 0.0;
        }

        const int spellID = spell.GetID();

        double ratio = 0.0;
        switch ( spellID ) {
        case Spell::SLOW:
        case Spell::MASSSLOW:
            ratio = 0.3;
            break;
        case Spell::BLIND: {
            if ( targetIsLast )
                return 0.0;
            ratio = 0.85;
            break;
        }
        case Spell::CURSE:
        case Spell::MASSCURSE:
            ratio = 0.15;
            break;
        case Spell::BERSERKER: {
            if ( targetIsLast || target.Modes( SP_BLIND | SP_PARALYZE | SP_STONE ) )
                return 0.0;
            ratio = 0.95;
            break;
        }
        case Spell::PARALYZE: {
            if ( targetIsLast )
                return 0.0;
            ratio = 0.9;
            break;
        }
        case Spell::HYPNOTIZE: {
            if ( targetIsLast || target.Modes( SP_BLIND | SP_PARALYZE | SP_STONE ) )
                return 0.0;
            ratio = 1.5;
            break;
        }
        case Spell::DISRUPTINGRAY:
            ratio = 0.2;
            break;
        case Spell::HASTE:
        case Spell::MASSHASTE:
            // Haste isn't effective if target is fast already
            ratio = target.GetSpeed() < _enemyAverageSpeed ? 0.3 : 0.1;
            break;
        case Spell::BLOODLUST:
            ratio = 0.1;
            break;
        case Spell::BLESS:
        case Spell::MASSBLESS: {
            if ( target.GetDamageMax() == target.GetDamageMin() )
                return 0.0;
            ratio = 0.15;
            break;
        }
        case Spell::STONESKIN:
            ratio = 0.1;
            break;
        case Spell::STEELSKIN:
            ratio = 0.2;
            break;
        // Following spell usefullness is conditional; ratio will be determined later
        case Spell::DRAGONSLAYER:
        case Spell::ANTIMAGIC:
        case Spell::MIRRORIMAGE:
            ratio = 0.0;
            break;
        default:
            return 0.0;
        }

        if ( spellID == Spell::SLOW || spellID == Spell::MASSSLOW ) {
            if ( target.Modes( SP_HASTE ) ) {
                ratio *= 2;
            }
            else if ( target.isArchers() || _attackingCastle ) {
                // Slow is useless against archers or troops defending castle
                ratio = 0.01;
            }
            else if ( !target.isFlying() ) {
                ratio /= ReduceEffectivenessByDistance( target );
            }
        }
        if ( spellID == Spell::HASTE || spellID == Spell::MASSHASTE ) {
            if ( target.Modes( SP_SLOW ) ) {
                ratio *= 2;
            }
            // Reduce effectiveness if we don't have to move
            else if ( target.isArchers() || _defensiveTactics ) {
                ratio /= 2;
            }
        }
        else if ( target.Modes( SP_BLESS ) && ( spellID == Spell::CURSE || spellID == Spell::MASSCURSE ) ) {
            ratio *= 2;
        }
        else if ( target.Modes( SP_CURSE ) && ( spellID == Spell::BLESS || spellID == Spell::MASSBLESS ) ) {
            ratio *= 2;
        }
        else if ( spellID == Spell::ANTIMAGIC && !target.Modes( IS_GOOD_MAGIC ) && _enemySpellStrength > ANTIMAGIC_LOW_LIMIT ) {
            double ratioLimit = 0.9;

            const std::vector<Spell> & spellList = _commander->GetSpells();
            for ( const Spell & otherSpell : spellList ) {
                if ( otherSpell.isResurrect() && _commander->HaveSpellPoints( otherSpell ) && target.AllowApplySpell( otherSpell, _commander ) ) {
                    // Can resurrect unit in the future, limit the ratio
                    ratioLimit = 0.15;
                    break;
                }
            }
            // 0...3000 spell strength (15 power * max knowledge) scales from 0.0 to 0.9 ratio
            ratio = std::min( _enemySpellStrength / ANTIMAGIC_LOW_LIMIT * 0.06, ratioLimit );

            if ( target.Modes( IS_BAD_MAGIC ) ) {
                ratio *= 2;
            }
        }
        else if ( spellID == Spell::MIRRORIMAGE ) {
            if ( target.isArchers() ) {
                ratio = 1.0;
            }
            else {
                ratio = target.isFlying() ? 0.55 : 0.33;
            }

            // Slow unit might be destroyed before taking its turn
            if ( target.GetSpeed() < _enemyAverageSpeed ) {
                ratio /= 5;
            }
        }
        else if ( spellID == Spell::BERSERKER && !target.isArchers() ) {
            ratio /= ReduceEffectivenessByDistance( target );
        }

        return target.GetStrength() * ratio;
    }

    SpellcastOutcome BattlePlanner::spellEffectValue( const Spell & spell, const Units & targets ) const
    {
        SpellcastOutcome bestOutcome;

        const bool isSingleTargetLeft = targets.size() == 1;
        const bool isMassSpell = spell.isMassActions();

        for ( const Unit * unit : targets ) {
            bestOutcome.updateOutcome( spellEffectValue( spell, *unit, isSingleTargetLeft ), unit->GetHeadIndex(), isMassSpell );
        }
        return bestOutcome;
    }

    SpellcastOutcome BattlePlanner::spellDispellValue( const Spell & spell, const Battle::Units & friendly, const Units & enemies ) const
    {
        SpellcastOutcome bestOutcome;

        const int spellID = spell.GetID();
        const bool isMassSpell = spell.isMassActions();
        const bool isDispel = spellID == Spell::DISPEL || spellID == Spell::MASSDISPEL;

        for ( const Unit * unit : friendly ) {
            if ( !unit->Modes( IS_MAGIC ) )
                continue;

            double unitValue = 0;
            const std::vector<Spell> & spellList = unit->getCurrentSpellEffects();
            for ( const Spell & spellOnFriend : spellList ) {
                const double effectValue = spellEffectValue( spellOnFriend, *unit, false );
                if ( spellOnFriend.isApplyToEnemies() ) {
                    unitValue += effectValue;
                }
                else if ( isDispel && spellOnFriend.isApplyToFriends() ) {
                    unitValue -= effectValue;
                }
            }

            bestOutcome.updateOutcome( unitValue, unit->GetHeadIndex(), isMassSpell );
        }

        if ( isDispel ) {
            const bool enemyLastUnit = enemies.size() == 1;

            for ( const Unit * unit : enemies ) {
                if ( !unit->Modes( IS_MAGIC ) )
                    continue;

                double unitValue = 0;
                const std::vector<Spell> & spellList = unit->getCurrentSpellEffects();
                for ( const Spell & spellOnEnemy : spellList ) {
                    const double effectValue = spellEffectValue( spellOnEnemy, *unit, enemyLastUnit );
                    unitValue += spellOnEnemy.isApplyToFriends() ? effectValue : -effectValue;
                }

                bestOutcome.updateOutcome( unitValue, unit->GetHeadIndex(), isMassSpell );
            }
        }

        return bestOutcome;
    }

    SpellcastOutcome BattlePlanner::spellResurrectValue( const Spell & spell, Battle::Arena & arena ) const
    {
        SpellcastOutcome bestOutcome;
        const uint32_t ankhModifier = _commander->HasArtifact( Artifact::ANKH ) ? 2 : 1;
        const uint32_t hpRestored = spell.Resurrect() * _commander->GetPower() * ankhModifier;

        // Get friendly units list including the invalid and dead ones
        const Force & friendlyForce = arena.GetForce( _myColor );

        for ( const Unit * unit : friendlyForce ) {
            if ( !unit || !unit->AllowApplySpell( spell, _commander ) )
                continue;

            uint32_t missingHP = unit->GetMissingHitPoints();
            missingHP = ( missingHP < hpRestored ) ? missingHP : hpRestored;

            double spellValue = missingHP * unit->GetMonsterStrength() / unit->Monster::GetHitPoints();

            // if we are winning battle; permanent resurrect bonus
            if ( _myArmyStrength > _enemyArmyStrength && spell.GetID() != Spell::RESURRECT ) {
                spellValue *= 1.5;
            }

            bestOutcome.updateOutcome( spellValue, unit->GetHeadIndex() );
        }

        return bestOutcome;
    }

    SpellcastOutcome BattlePlanner::spellSummonValue( const Spell & spell ) const
    {
        SpellcastOutcome bestOutcome;
        if ( spell.isSummon() ) {
            uint32_t count = spell.ExtraValue() * _commander->GetPower();
            if ( _commander->HasArtifact( Artifact::BOOK_ELEMENTS ) )
                count *= 2;

            const Troop summon( Monster( spell ), count );
            bestOutcome.value = summon.GetStrengthWithBonus( _commander->GetAttack(), _commander->GetDefense() );

            // Spell is less effective if we already winning this battle
            if ( _myArmyStrength > _enemyArmyStrength * 2 ) {
                bestOutcome.value /= 2;
            }
        }
        return bestOutcome;
    }
}
