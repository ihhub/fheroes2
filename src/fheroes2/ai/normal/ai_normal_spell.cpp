/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2023                                             *
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
#include <cassert>
#include <cmath>
#include <cstdint>
#include <ostream>
#include <vector>

#include "ai.h"
#include "ai_normal.h"
#include "army_troop.h"
#include "artifact.h"
#include "artifact_info.h"
#include "battle.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "battle_board.h"
#include "battle_cell.h"
#include "battle_troop.h"
#include "heroes_base.h"
#include "logging.h"
#include "monster.h"
#include "speed.h"
#include "spell.h"
#include "spell_info.h"
#include "spell_storage.h"

using namespace Battle;

namespace
{
    const double antimagicLowLimit = 200.0;
}

namespace AI
{
    double ReduceEffectivenessByDistance( const Unit & unit )
    {
        // Reduce spell effectiveness if unit already crossed the battlefield
        return Board::DistanceFromOriginX( unit.GetHeadIndex(), unit.isReflect() );
    }

    SpellSelection BattlePlanner::selectBestSpell( Arena & arena, const Battle::Unit & currentUnit, bool retreating ) const
    {
        // Cast best spell with highest heuristic on target pointer saved
        SpellSelection bestSpell;

        // Commander must be set before calling this function! Check both debug/release version
        assert( _commander != nullptr );
        if ( _commander == nullptr ) {
            return bestSpell;
        }

        const SpellStorage allSpells = _commander->getAllSpells();
        const Units friendly( arena.getForce( _myColor ).getUnits(), true );
        const Units enemies( arena.getEnemyForce( _myColor ).getUnits(), true );

        // Hero should conserve spellpoints if already spent more than half or his army is stronger
        // Threshold is 0.04 when armies are equal (= 20% of single unit)
        double spellValueThreshold = _myArmyStrength * _myArmyStrength / _enemyArmyStrength * 0.04;
        if ( _enemyShooterStr / _enemyArmyStrength > 0.5 ) {
            spellValueThreshold *= 0.5;
        }
        if ( _commander->GetSpellPoints() * 2 < _commander->GetMaxSpellPoints() ) {
            spellValueThreshold *= 2;
        }

        const auto checkSelectBestSpell = [this, retreating, spellValueThreshold, &bestSpell]( const Spell & spell, const SpellcastOutcome & outcome ) {
            // Diminish spell effectiveness based on spell point cost
            // 1. Divide cost by 3 to make level 1 spells a baseline (1:1)
            // 2. Use square root to make sure relationship isn't linear for high-level spells
            const double spellPointValue = retreating ? outcome.value : outcome.value / sqrt( spell.spellPoints( _commander ) / 3.0 );
            const bool ignoreThreshold = retreating || spell.isResurrect();

            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, spell.GetName() << " value is " << spellPointValue << ", best target is " << outcome.cell )

            if ( spellPointValue > bestSpell.value && ( ignoreThreshold || spellPointValue > spellValueThreshold ) ) {
                bestSpell.spellID = spell.GetID();
                bestSpell.cell = outcome.cell;
                bestSpell.value = spellPointValue;
            }
        };

        for ( const Spell & spell : allSpells ) {
            if ( !spell.isCombat() || arena.isDisableCastSpell( spell ) || !_commander->CanCastSpell( spell ) || ( retreating && !spell.isDamage() ) ) {
                continue;
            }

            if ( spell.isDamage() ) {
                checkSelectBestSpell( spell, spellDamageValue( spell, arena, currentUnit, friendly, enemies, retreating ) );
            }
            else if ( spell.isEffectDispel() ) {
                checkSelectBestSpell( spell, spellDispelValue( spell, friendly, enemies ) );
            }
            else if ( spell.isSummon() ) {
                checkSelectBestSpell( spell, spellSummonValue( spell, arena, _commander->GetColor() ) );
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

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "Spell threshold is " << spellValueThreshold << ", unit ratio is " << ( spellValueThreshold * 5 / _myArmyStrength ) )
        if ( bestSpell.spellID != -1 ) {
            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "Best spell is " << Spell( bestSpell.spellID ).GetName() << ", value is " << bestSpell.value )
        }

        return bestSpell;
    }

    SpellcastOutcome BattlePlanner::spellDamageValue( const Spell & spell, Arena & arena, const Battle::Unit & currentUnit, const Units & friendly, const Units & enemies,
                                                      bool retreating ) const
    {
        if ( !spell.isDamage() ) {
            return {};
        }

        const uint32_t spellDamage = fheroes2::getSpellDamage( spell, _commander->GetPower(), _commander );

        const auto damageHeuristic = [this, spellDamage, &spell, retreating]( const Unit * unit, const double armyStrength, const double armySpeed ) {
            const uint32_t damage = spellDamage * ( 100 - unit->GetMagicResist( spell, _commander ) ) / 100;

            // If we're retreating we don't care about partial damage, only actual units killed
            if ( retreating ) {
                return unit->GetMonsterStrength() * unit->HowManyWillBeKilled( damage );
            }

            // Otherwise calculate amount of strength lost (% of unit times total strength)
            const uint32_t hitpoints = unit->Modes( CAP_MIRRORIMAGE ) ? 1 : unit->GetHitPoints();
            if ( damage >= hitpoints ) {
                // bonus for finishing a stack
                const double bonus = ( unit->GetSpeed() > armySpeed ) ? 0.07 : 0.035;
                return unit->GetStrength() + armyStrength * bonus;
            }

            double unitPercentageLost = std::min( static_cast<double>( damage ) / hitpoints, 1.0 );

            // Penalty for waking up disabled unit (if you kill only 30%, rest 70% is your penalty)
            if ( unit->isImmovable() ) {
                unitPercentageLost += unitPercentageLost - 1.0;
            }

            return unitPercentageLost * unit->GetStrength();
        };

        SpellcastOutcome bestOutcome;

        if ( spell.isSingleTarget() ) {
            for ( const Unit * enemy : enemies ) {
                bestOutcome.updateOutcome( damageHeuristic( enemy, _enemyArmyStrength, _enemyAverageSpeed ), enemy->GetHeadIndex() );
            }
        }
        else if ( spell.isApplyWithoutFocusObject() ) {
            double spellHeuristic = 0;
            for ( const Unit * enemy : enemies ) {
                spellHeuristic += damageHeuristic( enemy, _enemyArmyStrength, _enemyAverageSpeed );
            }
            for ( const Unit * unit : friendly ) {
                const double valueLost = damageHeuristic( unit, _myArmyStrength, _myArmyAverageSpeed );
                // check if we're retreating and will lose current unit
                if ( retreating && unit->isUID( currentUnit.GetUID() ) && std::fabs( valueLost - unit->GetStrength() ) < 0.001 ) {
                    // avoid this spell and return early
                    return bestOutcome;
                }
                spellHeuristic -= valueLost;
            }

            bestOutcome.updateOutcome( spellHeuristic, -1 );
        }
        else {
            // Area of effect spells like Fireball
            const auto areaOfEffectCheck
                = [this, &damageHeuristic, &bestOutcome, &currentUnit, retreating]( const TargetsInfo & targets, const int32_t index, int myColor ) {
                      double spellHeuristic = 0;

                      for ( const TargetInfo & target : targets ) {
                          if ( target.defender->GetCurrentColor() == myColor ) {
                              const double valueLost = damageHeuristic( target.defender, _myArmyStrength, _myArmyAverageSpeed );
                              // check if we're retreating and will lose current unit
                              if ( retreating && target.defender->isUID( currentUnit.GetUID() ) && std::fabs( valueLost - target.defender->GetStrength() ) < 0.001 ) {
                                  // avoid this spell and return without updating the outcome
                                  return;
                              }
                              spellHeuristic -= valueLost;
                          }
                          else {
                              spellHeuristic += damageHeuristic( target.defender, _enemyArmyStrength, _enemyAverageSpeed );
                          }
                      }

                      bestOutcome.updateOutcome( spellHeuristic, index );
                  };

            if ( spell.GetID() == Spell::CHAINLIGHTNING ) {
                for ( const Unit * enemy : enemies ) {
                    if ( !enemy->AllowApplySpell( spell, _commander ) ) {
                        continue;
                    }

                    const int32_t index = enemy->GetHeadIndex();
                    areaOfEffectCheck( arena.GetTargetsForSpell( _commander, spell, index ), index, _myColor );
                }
            }
            else {
                const Board & board = *Arena::GetBoard();
                for ( const Cell & cell : board ) {
                    const int32_t index = cell.GetIndex();
                    areaOfEffectCheck( arena.GetTargetsForSpell( _commander, spell, index ), index, _myColor );
                }
            }
        }

        return bestOutcome;
    }

    int32_t BattlePlanner::spellDurationMultiplier( const Battle::Unit & target ) const
    {
        const int32_t duration
            = _commander->GetPower() + _commander->GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::EVERY_COMBAT_SPELL_DURATION );

        if ( duration < 2 && target.Modes( TR_MOVED ) ) {
            return 0;
        }

        return 1;
    }

    // Compute a heuristic for Disrupting Ray spell usage by AI
    double BattlePlanner::getSpellDisruptingRayRatio( const Battle::Unit & target ) const
    {
        const double targetDefense = target.GetDefense();

        if ( targetDefense <= 1 ) { // target is already at minimum defense: not useful to cast Disrupting Ray
            return 0.0;
        }

        double ratio = 0.2;
        if ( targetDefense <= Spell( Spell::DISRUPTINGRAY ).ExtraValue() ) { // disrupting ray can't have full effect
            const double actualDefenseChange = targetDefense - 1.0;
            ratio *= actualDefenseChange / Spell( Spell::DISRUPTINGRAY ).ExtraValue();
        }

        const double targetStrength = target.GetStrength();

        // if current army has much lower strength than target unit, we reduce the ratio to prioritize direct damage spells
        if ( _myArmyStrength < targetStrength ) {
            ratio *= _myArmyStrength / targetStrength;
        }

        return ratio;
    }

    // Compute a heuristic for Slow spell usage by AI
    double BattlePlanner::getSpellSlowRatio( const Battle::Unit & target ) const
    {
        if ( target.isArchers() || _attackingCastle ) {
            // Slow is useless against archers or troops defending castle
            return 0.01;
        }
        const int currentSpeed = target.GetSpeed( false, true );
        const int newSpeed = Speed::GetSlowSpeedFromSpell( currentSpeed );
        const int lostSpeed = currentSpeed - newSpeed; // usually 2
        double ratio = 0.1 * lostSpeed;

        if ( currentSpeed < _myArmyAverageSpeed ) { // Slow isn't useful if target is already slower than our army
            ratio /= 2;
        }
        if ( target.Modes( SP_HASTE ) ) {
            ratio *= 2;
        }
        else if ( !target.isFlying() ) {
            ratio /= ReduceEffectivenessByDistance( target );
        }
        return ratio;
    }

    // Compute a heuristic for Haste spell usage by AI
    double BattlePlanner::getSpellHasteRatio( const Battle::Unit & target ) const
    {
        const int currentSpeed = target.GetSpeed( false, true );
        const int newSpeed = Speed::GetHasteSpeedFromSpell( currentSpeed );
        const int gainedSpeed = newSpeed - currentSpeed; // usually 2
        double ratio = 0.05 * gainedSpeed;

        if ( currentSpeed < _enemyAverageSpeed ) { // Haste is very useful if target is slower than army
            ratio *= 2;
        }
        if ( target.Modes( SP_SLOW ) ) {
            ratio *= 2;
        }
        // Reduce effectiveness if we don't have to move
        else if ( target.isArchers() || _defensiveTactics ) {
            ratio /= 2;
        }

        return ratio;
    }

    double BattlePlanner::spellEffectValue( const Spell & spell, const Battle::Unit & target, bool targetIsLast, bool forDispel ) const
    {
        const int spellID = spell.GetID();

        // Make sure this spell can be applied to the current unit (skip check for dispel estimation)
        if ( !forDispel
             && ( ( target.isImmovable() && spellID != Spell::ANTIMAGIC ) || target.isUnderSpellEffect( spell ) || !target.AllowApplySpell( spell, _commander ) ) ) {
            return 0.0;
        }

        double ratio = 0.0;
        switch ( spellID ) {
        case Spell::SLOW:
        case Spell::MASSSLOW:
            ratio = getSpellSlowRatio( target );
            break;
        case Spell::BLIND: {
            if ( targetIsLast )
                return 0.0;
            ratio = 0.8;
            break;
        }
        case Spell::CURSE:
        case Spell::MASSCURSE:
            ratio = 0.15;
            break;
        case Spell::BERSERKER: {
            if ( targetIsLast )
                return 0.0;
            ratio = 0.85;
            break;
        }
        case Spell::PARALYZE: {
            if ( targetIsLast )
                return 0.0;
            ratio = 0.85;
            break;
        }
        case Spell::HYPNOTIZE: {
            if ( targetIsLast )
                return 0.0;
            ratio = 1.5;
            break;
        }
        case Spell::DISRUPTINGRAY:
            ratio = getSpellDisruptingRayRatio( target );
            break;
        case Spell::HASTE:
        case Spell::MASSHASTE:
            ratio = getSpellHasteRatio( target );
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
        // Following spell usefulness is conditional; ratio will be determined later
        case Spell::DRAGONSLAYER:
        case Spell::ANTIMAGIC:
        case Spell::MIRRORIMAGE:
        case Spell::SHIELD:
        case Spell::MASSSHIELD:
            ratio = 0.0;
            break;
        default:
            return 0.0;
        }

        if ( target.Modes( SP_BLESS ) && ( spellID == Spell::CURSE || spellID == Spell::MASSCURSE ) ) {
            ratio *= 2;
        }
        else if ( target.Modes( SP_CURSE ) && ( spellID == Spell::BLESS || spellID == Spell::MASSBLESS ) ) {
            ratio *= 2;
        }
        else if ( spellID == Spell::ANTIMAGIC && !target.Modes( IS_GOOD_MAGIC ) && _enemySpellStrength > antimagicLowLimit ) {
            double ratioLimit = 0.9;

            const SpellStorage spellList = _commander->getAllSpells();
            for ( const Spell & otherSpell : spellList ) {
                if ( otherSpell.isResurrect() && _commander->HaveSpellPoints( otherSpell ) && target.AllowApplySpell( otherSpell, _commander ) ) {
                    // Can resurrect unit in the future, limit the ratio
                    ratioLimit = 0.35;
                    break;
                }
            }

            // Convert 0...5000 range into 0.0 to 0.9 ratio and clamp it
            ratio = std::min( _enemySpellStrength / antimagicLowLimit * 0.036, ratioLimit );

            // Hero is stronger than its army, possible hit and run tactic
            if ( _enemySpellStrength > _enemyArmyStrength ) {
                ratio *= 1.5;
            }

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
        else if ( spellID == Spell::SHIELD || spellID == Spell::MASSSHIELD ) {
            ratio = _enemyRangedUnitsOnly / _enemyArmyStrength * 0.3;

            if ( target.isArchers() ) {
                ratio *= 1.25;
            }
        }
        else if ( spellID == Spell::DRAGONSLAYER ) {
            // TODO: add logic to check if the enemy army contains a dragon.
        }

        return target.GetStrength() * ratio * spellDurationMultiplier( target );
    }

    SpellcastOutcome BattlePlanner::spellEffectValue( const Spell & spell, const Units & targets ) const
    {
        const bool isSingleTargetLeft = targets.size() == 1;
        const bool isMassSpell = spell.isMassActions();

        SpellcastOutcome bestOutcome;

        for ( const Unit * unit : targets ) {
            bestOutcome.updateOutcome( spellEffectValue( spell, *unit, isSingleTargetLeft, false ), unit->GetHeadIndex(), isMassSpell );
        }

        return bestOutcome;
    }

    SpellcastOutcome BattlePlanner::spellDispelValue( const Spell & spell, const Battle::Units & friendly, const Units & enemies ) const
    {
        const int spellID = spell.GetID();
        const bool isMassSpell = spell.isMassActions();
        const bool isDispel = spellID == Spell::DISPEL || spellID == Spell::MASSDISPEL;

        SpellcastOutcome bestOutcome;

        for ( const Unit * unit : friendly ) {
            if ( !unit->Modes( IS_MAGIC ) )
                continue;

            double unitValue = 0;
            const std::vector<Spell> & spellList = unit->getCurrentSpellEffects();
            for ( const Spell & spellOnFriend : spellList ) {
                const double effectValue = spellEffectValue( spellOnFriend, *unit, false, true );
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
                    const double effectValue = spellEffectValue( spellOnEnemy, *unit, enemyLastUnit, true );
                    unitValue += spellOnEnemy.isApplyToFriends() ? effectValue : -effectValue;
                }

                bestOutcome.updateOutcome( unitValue, unit->GetHeadIndex(), isMassSpell );
            }
        }

        return bestOutcome;
    }

    SpellcastOutcome BattlePlanner::spellResurrectValue( const Spell & spell, const Battle::Arena & arena ) const
    {
        const uint32_t hpRestored = fheroes2::getResurrectPoints( spell, _commander->GetPower(), _commander );

        SpellcastOutcome bestOutcome;

        const auto updateBestOutcome = [this, &spell, &bestOutcome, hpRestored]( const Unit * unit ) {
            uint32_t missingHP = unit->GetMissingHitPoints();
            missingHP = ( missingHP < hpRestored ) ? missingHP : hpRestored;

            double spellValue = missingHP * unit->GetMonsterStrength() / unit->Monster::GetHitPoints();

            // if we are winning battle; permanent resurrect bonus
            if ( _myArmyStrength > _enemyArmyStrength && spell.GetID() != Spell::RESURRECT ) {
                spellValue *= 2;
            }

            bestOutcome.updateOutcome( spellValue, unit->GetHeadIndex() );
        };

        // First consider the still alive stacks
        for ( const Unit * unit : arena.getForce( _myColor ) ) {
            assert( unit != nullptr );

            if ( !unit->isValid() ) {
                continue;
            }

            if ( !unit->AllowApplySpell( spell, _commander ) ) {
                continue;
            }

            updateBestOutcome( unit );
        }

        // Then consider the stacks from the graveyard
        for ( const int32_t idx : arena.GraveyardOccupiedCells() ) {
            if ( !arena.GraveyardAllowResurrect( idx, spell ) ) {
                continue;
            }

            const Unit * unit = arena.GraveyardLastTroop( idx );
            assert( unit != nullptr && !unit->isValid() );

            updateBestOutcome( unit );
        }

        return bestOutcome;
    }

    SpellcastOutcome BattlePlanner::spellSummonValue( const Spell & spell, const Battle::Arena & arena, const int heroColor ) const
    {
        if ( !spell.isSummon() ) {
            return {};
        }

        if ( arena.GetFreePositionNearHero( heroColor ) < 0 ) {
            return {};
        }

        const Troop summon( Monster( spell ), fheroes2::getSummonMonsterCount( spell, _commander->GetPower(), _commander ) );

        SpellcastOutcome bestOutcome;

        bestOutcome.value = summon.GetStrengthWithBonus( _commander->GetAttack(), _commander->GetDefense() );

        // Spell is less effective if we already winning this battle
        if ( _myArmyStrength > _enemyArmyStrength * 2 ) {
            bestOutcome.value /= 2;
        }

        return bestOutcome;
    }

    double BattlePlanner::commanderMaximumSpellDamageValue( const HeroBase & commander )
    {
        double bestValue = 0;

        for ( const Spell & spell : commander.getAllSpells() ) {
            if ( !spell.isCombat() || !spell.isDamage() ) {
                continue;
            }

            if ( commander.GetSpellPoints() < spell.spellPoints() ) {
                continue;
            }

            bestValue = std::max( bestValue, static_cast<double>( fheroes2::getSpellDamage( spell, commander.GetPower(), &commander ) ) );
        }

        return bestValue;
    }
}
