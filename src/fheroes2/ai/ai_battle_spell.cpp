/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024 - 2025                                             *
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
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <vector>

#include "ai_battle.h" // IWYU pragma: associated
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
#include "monster_info.h"
#include "speed.h"
#include "spell.h"
#include "spell_info.h"
#include "spell_storage.h"

namespace
{
    const double antimagicLowLimit = 200.0;

    const double bloodLustRatio = 0.1;

    double ReduceEffectivenessByDistance( const Battle::Unit & unit )
    {
        // Reduce spell effectiveness if unit already crossed the battlefield
        const uint32_t result = Battle::Board::GetDistanceFromBoardEdgeAlongXAxis( unit.GetHeadIndex(), unit.isReflect() );
        assert( result > 0 );

        return result;
    }

    int32_t getSpellPower( const HeroBase * hero )
    {
        assert( hero != nullptr );

        return hero->GetPower() + hero->GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::EVERY_COMBAT_SPELL_DURATION );
    }
}

AI::SpellSelection AI::BattlePlanner::selectBestSpell( Battle::Arena & arena, const Battle::Unit & currentUnit, const bool retreating ) const
{
    SpellSelection bestSpell;

    if ( _commander == nullptr ) {
        assert( 0 );

        return bestSpell;
    }

    const SpellStorage allSpells = _commander->getAllSpells();

    const Battle::Units friendly( arena.getForce( _myColor ).getUnits(), Battle::Units::REMOVE_INVALID_UNITS );
    const Battle::Units enemies( arena.getEnemyForce( _myColor ).getUnits(), Battle::Units::REMOVE_INVALID_UNITS );

    const Battle::Units trueFriendly( arena.getForce( _myColor ).getUnits(), Battle::Units::REMOVE_INVALID_UNITS_AND_UNITS_THAT_CHANGED_SIDES );
    const Battle::Units trueEnemies( arena.getEnemyForce( _myColor ).getUnits(), Battle::Units::REMOVE_INVALID_UNITS_AND_UNITS_THAT_CHANGED_SIDES );

    // Hero should conserve spellpoints if already spent more than half or his army is stronger
    // Threshold is 0.04 when armies are equal (= 20% of single unit)
    double spellValueThreshold = _myArmyStrength * _myArmyStrength / _enemyArmyStrength * 0.04;
    if ( _enemyShootersStrength / _enemyArmyStrength > 0.5 ) {
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
            bestSpell.destinationCell = outcome.destinationCell;
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
        else if ( spell == Spell::DRAGONSLAYER ) {
            checkSelectBestSpell( spell, spellDragonSlayerValue( spell, friendly, enemies ) );
        }
        else if ( spell == Spell::TELEPORT ) {
            checkSelectBestSpell( spell, spellTeleportValue( arena, spell, currentUnit, enemies ) );
        }
        else if ( spell == Spell::EARTHQUAKE ) {
            checkSelectBestSpell( spell, spellEarthquakeValue( arena, spell, friendly ) );
        }
        else if ( spell.isApplyToFriends() ) {
            checkSelectBestSpell( spell, spellEffectValue( spell, trueFriendly, enemies ) );
        }
        else if ( spell.isApplyToEnemies() ) {
            checkSelectBestSpell( spell, spellEffectValue( spell, trueEnemies, enemies ) );
        }
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "Spell threshold is " << spellValueThreshold << ", unit ratio is " << ( spellValueThreshold * 5 / _myArmyStrength ) )
    if ( bestSpell.spellID != -1 ) {
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "Best spell is " << Spell( bestSpell.spellID ).GetName() << ", value is " << bestSpell.value )
    }

    return bestSpell;
}

AI::SpellcastOutcome AI::BattlePlanner::spellDamageValue( const Spell & spell, Battle::Arena & arena, const Battle::Unit & currentUnit, const Battle::Units & friendly,
                                                          const Battle::Units & enemies, bool retreating ) const
{
    if ( !spell.isDamage() ) {
        return {};
    }

    const uint32_t spellDamage = fheroes2::getSpellDamage( spell, _commander->GetPower(), _commander );

    const auto damageHeuristic = [this, spellDamage, &spell, retreating]( const Battle::Unit * unit, const double armyStrength, const double armySpeed ) {
        const uint32_t damage = spellDamage * ( 100 - unit->GetMagicResist( spell, _commander ) ) / 100;

        // If the unit is immune to this spell, then no one will be killed, no strength will be lost and the unit will not be woken up if it is disabled
        if ( damage == 0 ) {
            return 0.0;
        }

        // If we retreat, we are not interested in partial damage, but only in the number of units actually killed
        if ( retreating ) {
            return unit->GetMonsterStrength() * unit->HowManyWillBeKilled( damage );
        }

        // If the unit will be completely destroyed, then use its full strength plus a bonus for destroying the stack.
        // TODO: we also need to take into an account additional stack of Mirror Image if the creatures have one.
        //       Killing the original monsters will destroy the Mirror Image stack as well.
        const uint32_t hitpoints = unit->Modes( Battle::CAP_MIRRORIMAGE ) ? 1 : unit->GetHitPoints();
        if ( damage >= hitpoints ) {
            const double bonus = ( unit->GetSpeed() > armySpeed ) ? 0.07 : 0.035;
            return unit->GetStrength() + armyStrength * bonus;
        }

        // Otherwise use the amount of strength lost (% of the total unit's strength)
        double unitPercentageLost = std::min( static_cast<double>( damage ) / hitpoints, 1.0 );

        // Penalty for waking up disabled unit (if you kill only 30%, the remaining 70% is your penalty)
        if ( unit->isImmovable() ) {
            unitPercentageLost += unitPercentageLost - 1.0;
        }

        return unitPercentageLost * unit->GetStrength();
    };

    SpellcastOutcome bestOutcome;

    if ( spell.isSingleTarget() ) {
        for ( const Battle::Unit * enemy : enemies ) {
            bestOutcome.updateOutcome( damageHeuristic( enemy, _enemyArmyStrength, _enemyAverageSpeed ), enemy->GetHeadIndex() );
        }
    }
    else if ( spell.isApplyWithoutFocusObject() ) {
        double spellHeuristic = 0;
        for ( const Battle::Unit * enemy : enemies ) {
            spellHeuristic += damageHeuristic( enemy, _enemyArmyStrength, _enemyAverageSpeed );
        }
        for ( const Battle::Unit * unit : friendly ) {
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
            = [this, &damageHeuristic, &bestOutcome, &currentUnit, retreating]( const Battle::TargetsInfo & targets, const int32_t index, PlayerColor myColor ) {
                  double spellHeuristic = 0;

                  for ( const Battle::TargetInfo & target : targets ) {
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
            for ( const Battle::Unit * enemy : enemies ) {
                if ( !enemy->AllowApplySpell( spell, _commander ) ) {
                    continue;
                }

                const int32_t index = enemy->GetHeadIndex();
                areaOfEffectCheck( arena.GetTargetsForSpell( _commander, spell, index ), index, _myColor );
            }
        }
        else {
            const Battle::Board & board = *Battle::Arena::GetBoard();
            for ( const Battle::Cell & cell : board ) {
                const int32_t index = cell.GetIndex();
                areaOfEffectCheck( arena.GetTargetsForSpell( _commander, spell, index ), index, _myColor );
            }
        }
    }

    return bestOutcome;
}

int32_t AI::BattlePlanner::spellDurationMultiplier( const Battle::Unit & target ) const
{
    const int32_t duration = getSpellPower( _commander );

    // TODO: this logic might not be accurate for cases when certain spells are applied to reduce the retaliation damage, like Blind or Paralyze.
    if ( duration < 2 && target.Modes( Battle::TR_MOVED ) ) {
        return 0;
    }

    return 1;
}

double AI::BattlePlanner::getSpellDisruptingRayRatio( const Battle::Unit & target ) const
{
    const double targetDefense = target.GetDefense();

    if ( targetDefense <= 1 ) {
        // The target has minimum defense value. This spell is useless.
        return 0.0;
    }

    double ratio = 0.2;
    if ( targetDefense <= Spell( Spell::DISRUPTINGRAY ).ExtraValue() ) {
        // The spell is not applicable fully.
        const double actualDefenseChange = targetDefense - 1.0;
        const uint32_t spellValue = Spell( Spell::DISRUPTINGRAY ).ExtraValue();
        if ( spellValue == 0 ) {
            assert( spellValue > 0 );
            return 0;
        }

        ratio *= actualDefenseChange / spellValue;
    }

    const double targetStrength = target.GetStrength();

    // if current army has much lower strength than target unit, we reduce the ratio to prioritize direct damage spells
    if ( _myArmyStrength < targetStrength ) {
        ratio *= _myArmyStrength / targetStrength;
    }

    return ratio;
}

double AI::BattlePlanner::getSpellSlowRatio( const Battle::Unit & target ) const
{
    if ( target.isArchers() || _attackingCastle ) {
        // Slow is useless against archers or troops defending castle.
        //
        // The above is not always true. A stack of enemy archers can be killed by a slower flying creature if they are slowed down.
        // TODO: add a check for the last case.
        return 0.01;
    }
    const uint32_t currentSpeed = target.GetSpeed( false, true );
    const uint32_t newSpeed = Speed::getSlowSpeedFromSpell( currentSpeed );
    const uint32_t lostSpeed = currentSpeed - newSpeed; // usually 2
    double ratio = 0.1 * lostSpeed;

    if ( currentSpeed < _myArmyAverageSpeed ) {
        // Slow isn't useful if target is already slower than our army.
        ratio /= 2;
    }
    if ( target.Modes( Battle::SP_HASTE ) ) {
        ratio *= 2;
    }
    else if ( !target.isFlying() ) {
        ratio /= ReduceEffectivenessByDistance( target );
    }
    return ratio;
}

double AI::BattlePlanner::getSpellHasteRatio( const Battle::Unit & target ) const
{
    const uint32_t currentSpeed = target.GetSpeed( false, true );
    const uint32_t newSpeed = Speed::getHasteSpeedFromSpell( currentSpeed );
    const uint32_t gainedSpeed = newSpeed - currentSpeed; // usually 2
    double ratio = 0.05 * gainedSpeed;

    if ( currentSpeed < _enemyAverageSpeed ) {
        // Haste is very useful if target is slower than army.
        ratio *= 2;
    }
    if ( target.Modes( Battle::SP_SLOW ) ) {
        ratio *= 2;
    }
    // Reduce effectiveness if we don't have to move
    else if ( target.isArchers() || _defensiveTactics ) {
        ratio /= 2;
    }

    return ratio;
}

double AI::BattlePlanner::spellEffectValue( const Spell & spell, const Battle::Unit & target, const Battle::Units & enemies, const bool targetIsLast,
                                            const bool forDispel ) const
{
    // Make sure that this spell makes sense to apply (skip this check to evaluate the effect of dispelling)
    if ( !forDispel && ( isSpellcastUselessForUnit( target, enemies, spell ) || !target.AllowApplySpell( spell, _commander ) ) ) {
        return 0.0;
    }

    const int spellID = spell.GetID();

    double ratio = 0.0;
    switch ( spellID ) {
    case Spell::SLOW:
    case Spell::MASSSLOW:
        ratio = getSpellSlowRatio( target );
        break;
    case Spell::BLIND: {
        if ( targetIsLast ) {
            // TODO: add more complex logic to calculate the usefulness of this spell for a single target as blinded creature retaliates with 50% damage.
            //       It might be less useful against monsters with unlimited retaliation when you have more than one monster.
            //
            // As of now, we assume that the spell is going to be applicable for a monster that has no unlimited retaliation,
            // we are going to hit it immediately and the monster hasn't retaliated.
            if ( target.isAbilityPresent( fheroes2::MonsterAbilityType::UNLIMITED_RETALIATION ) ) {
                // Not so much useful.
                return 0;
            }

            if ( !target.isRetaliationAllowed() ) {
                // The monster has retaliated. Apply the spell might not be that good.
                // TODO: check whether we are going to attack the monster right now or the next turn.
                //       Plus Blind spell is useful if the monster is faster than other troops.
                return 0;
            }

            // The final ratio is smaller than the original one but not that much.
            ratio = 0.4;
        }
        else {
            ratio = 0.8;
        }
        break;
    }
    case Spell::CURSE:
    case Spell::MASSCURSE:
        if ( target.GetDamageMax() == target.GetDamageMin() ) {
            // It is useless to apply Curse spell as the monster already has minimal damage.
            return 0;
        }
        ratio = 0.15;
        break;
    case Spell::BERSERKER: {
        if ( targetIsLast ) {
            // No use of this spell if the last enemy creature is left for the battle.
            // However, there are some cases when it is going to be useful:
            // - to avoid the enemy hero cast spells as they won't have control of any of their monsters
            // - to lure the monster to our another monster, for example to attack Dragons rather than Mages.
            //
            // TODO: add the logic for the above cases.
            return 0;
        }
        ratio = 0.85;
        break;
    }
    case Spell::PARALYZE: {
        if ( targetIsLast ) {
            // TODO: add proper evaluation of the spell as a paralyzed creature has no retaliation damage.
            //       It might be less useful against monsters with unlimited retaliation when you have more than one monster.
            //
            // As of now, we assume that the spell is going to be applicable for a monster that has no unlimited retaliation,
            // we are going to hit it immediately and the monster hasn't retaliated.
            if ( target.isAbilityPresent( fheroes2::MonsterAbilityType::UNLIMITED_RETALIATION ) ) {
                // Not so much useful.
                return 0;
            }

            if ( !target.isRetaliationAllowed() ) {
                // The monster has retaliated. Apply the spell might not be that good.
                // TODO: check whether we are going to attack the monster right now or the next turn.
                //       Plus Paralyze spell is useful if the monster is faster than other troops.
                return 0;
            }

            const int32_t spellDuration = spellDurationMultiplier( target );
            if ( spellDuration < 1 ) {
                // This spell might not be useful at all.
                return 0;
            }

            // The final ratio is smaller than the original one but not that much.
            ratio = 0.5;
        }
        else {
            ratio = 0.85;
        }
        break;
    }
    case Spell::HYPNOTIZE: {
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
        ratio = bloodLustRatio;
        break;
    case Spell::BLESS:
    case Spell::MASSBLESS: {
        if ( target.GetDamageMax() == target.GetDamageMin() ) {
            // It is useless to apply Bless spell as the monster already has maximum damage.
            return 0;
        }
        ratio = 0.15;
        break;
    }
    case Spell::STONESKIN:
        ratio = 0.1;
        break;
    case Spell::STEELSKIN:
        ratio = 0.2;
        break;
    // Following spell usefulness is conditional. Ratio will be determined below.
    case Spell::ANTIMAGIC:
    case Spell::MIRRORIMAGE:
    case Spell::SHIELD:
    case Spell::MASSSHIELD:
        ratio = 0.0;
        break;
    default:
        return 0.0;
    }

    if ( target.Modes( Battle::SP_BLESS ) && ( spellID == Spell::CURSE || spellID == Spell::MASSCURSE ) ) {
        ratio *= 2;
    }
    else if ( target.Modes( Battle::SP_CURSE ) && ( spellID == Spell::BLESS || spellID == Spell::MASSBLESS ) ) {
        ratio *= 2;
    }
    else if ( spellID == Spell::ANTIMAGIC && !target.Modes( Battle::IS_GOOD_MAGIC ) && _enemySpellStrength > antimagicLowLimit ) {
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

        if ( target.Modes( Battle::IS_BAD_MAGIC ) ) {
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

    return target.GetStrength() * ratio * spellDurationMultiplier( target );
}

AI::SpellcastOutcome AI::BattlePlanner::spellEffectValue( const Spell & spell, const Battle::Units & targets, const Battle::Units & enemies ) const
{
    const bool isSingleTargetLeft = targets.size() == 1;
    const bool isMassSpell = spell.isMassActions();

    SpellcastOutcome bestOutcome;

    for ( const Battle::Unit * unit : targets ) {
        bestOutcome.updateOutcome( spellEffectValue( spell, *unit, enemies, isSingleTargetLeft, false ), unit->GetHeadIndex(), isMassSpell );
    }

    return bestOutcome;
}

AI::SpellcastOutcome AI::BattlePlanner::spellDispelValue( const Spell & spell, const Battle::Units & friendly, const Battle::Units & enemies ) const
{
    const int spellID = spell.GetID();
    const bool isMassSpell = spell.isMassActions();
    const bool isDispel = spellID == Spell::DISPEL || spellID == Spell::MASSDISPEL;

    SpellcastOutcome bestOutcome;

    for ( const Battle::Unit * unit : friendly ) {
        if ( !unit->Modes( Battle::IS_MAGIC ) ) {
            continue;
        }

        double unitValue = 0;
        const std::vector<Spell> & spellList = unit->getCurrentSpellEffects();
        for ( const Spell & spellOnFriend : spellList ) {
            const double effectValue = spellEffectValue( spellOnFriend, *unit, enemies, false, true );
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

        for ( const Battle::Unit * unit : enemies ) {
            if ( !unit->Modes( Battle::IS_MAGIC ) )
                continue;

            double unitValue = 0;
            const std::vector<Spell> & spellList = unit->getCurrentSpellEffects();
            for ( const Spell & spellOnEnemy : spellList ) {
                const double effectValue = spellEffectValue( spellOnEnemy, *unit, enemies, enemyLastUnit, true );
                unitValue += spellOnEnemy.isApplyToFriends() ? effectValue : -effectValue;
            }

            bestOutcome.updateOutcome( unitValue, unit->GetHeadIndex(), isMassSpell );
        }
    }

    return bestOutcome;
}

AI::SpellcastOutcome AI::BattlePlanner::spellResurrectValue( const Spell & spell, const Battle::Arena & arena ) const
{
    const uint32_t hpRestored = fheroes2::getResurrectPoints( spell, _commander->GetPower(), _commander );

    SpellcastOutcome bestOutcome;

    const auto updateBestOutcome = [this, &spell, &bestOutcome, hpRestored]( const Battle::Unit * unit ) {
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
    for ( const Battle::Unit * unit : arena.getForce( _myColor ) ) {
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
    for ( const int32_t idx : arena.getCellsOccupiedByGraveyard() ) {
        if ( !arena.isAbleToResurrectFromGraveyard( idx, spell ) ) {
            continue;
        }

        const Battle::Unit * unit = arena.getLastResurrectableUnitFromGraveyard( idx, spell );
        assert( unit != nullptr && !unit->isValid() );

        updateBestOutcome( unit );
    }

    return bestOutcome;
}

AI::SpellcastOutcome AI::BattlePlanner::spellSummonValue( const Spell & spell, const Battle::Arena & arena, const PlayerColor heroColor ) const
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

AI::SpellcastOutcome AI::BattlePlanner::spellDragonSlayerValue( const Spell & spell, const Battle::Units & friendly, const Battle::Units & enemies ) const
{
    assert( spell.GetID() == Spell::DRAGONSLAYER );

    double enemyArmyStrength = 0;
    double dragonsStrength = 0;

    int32_t numOfSlotsWithEnemyDragons = 0;

    for ( const Battle::Unit * unit : enemies ) {
        assert( unit != nullptr && unit->isValid() );

        const double strength = unit->GetStrength();

        if ( unit->isDragons() ) {
            ++numOfSlotsWithEnemyDragons;
            dragonsStrength += strength;
        }

        enemyArmyStrength += strength;
    }

    if ( numOfSlotsWithEnemyDragons == 0 ) {
        // This spell is useless as no Dragons exist in the enemy army.
        return {};
    }

    const auto getSpellAttackBonus = []( const Spell & sp ) {
        const uint32_t bonus = sp.ExtraValue();
        assert( bonus > 0 );

        return bonus;
    };

    // Make an estimation based on the value of Bloodlust since this spell also increases Attack but against every creature.
    // If the enemy army consists of other monsters that are not Dragons then Dragon Slayer spell isn't that valuable anymore.
    const double bloodlustAttackBonus = getSpellAttackBonus( Spell::BLOODLUST );
    const double dragonSlayerAttackBonus = getSpellAttackBonus( spell );

    const double dragonSlayerRatio = bloodLustRatio * dragonSlayerAttackBonus / bloodlustAttackBonus * dragonsStrength / enemyArmyStrength;

    SpellcastOutcome bestOutcome;

    for ( const Battle::Unit * unit : friendly ) {
        if ( isSpellcastUselessForUnit( *unit, enemies, spell ) ) {
            continue;
        }

        const double unitValue = unit->GetStrength() * dragonSlayerRatio * spellDurationMultiplier( *unit );
        bestOutcome.updateOutcome( unitValue, unit->GetHeadIndex(), false );
    }

    return bestOutcome;
}

bool AI::BattlePlanner::isSpellcastUselessForUnit( const Battle::Unit & unit, const Battle::Units & enemies, const Spell & spell ) const
{
    const int spellID = spell.GetID();

    if ( unit.isImmovable() && spellID != Spell::ANTIMAGIC ) {
        return true;
    }

    switch ( spellID ) {
    case Spell::BLESS:
    case Spell::MASSBLESS:
        return unit.Modes( Battle::SP_BLESS );

    case Spell::BLOODLUST:
        return unit.Modes( Battle::SP_BLOODLUST );

    case Spell::CURSE:
    case Spell::MASSCURSE:
        return unit.Modes( Battle::SP_CURSE );

    case Spell::HASTE:
    case Spell::MASSHASTE:
        return unit.Modes( Battle::SP_HASTE ) || ( unit.GetSpeed() == Speed::INSTANT );

    case Spell::SHIELD:
    case Spell::MASSSHIELD:
        // If a spell duration is just 1 round and all shooters already completed their turn
        // then this spell is useless.
        if ( getSpellPower( _commander ) == 1 ) {
            size_t activeShooters{ 0 };

            for ( const auto * enemy : enemies ) {
                if ( enemy->isArchers() && !enemy->isImmovable() && !enemy->Modes( Battle::TR_MOVED ) ) {
                    ++activeShooters;
                }
            }

            if ( activeShooters == 0 ) {
                // No shooters are going to make their move.
                return true;
            }
        }

        return unit.Modes( Battle::SP_SHIELD );

    case Spell::SLOW:
    case Spell::MASSSLOW:
        return unit.Modes( Battle::SP_SLOW ) || ( unit.GetSpeed() == Speed::CRAWLING );

    case Spell::STONESKIN:
    case Spell::STEELSKIN:
        // TODO: this is not always true. Steel Skin gives higher defense so applying it makes sense.
        return unit.Modes( Battle::SP_STONESKIN | Battle::SP_STEELSKIN );

    case Spell::BLIND:
    case Spell::PARALYZE:
    case Spell::PETRIFY:
        // TODO: these 3 modes serve different purposes and
        //       there are cases when applying Paralyze just at the end of Blind spell duration makes sense.
        return unit.Modes( Battle::SP_BLIND | Battle::SP_PARALYZE | Battle::SP_STONE );

    case Spell::DRAGONSLAYER:
        return unit.Modes( Battle::SP_DRAGONSLAYER );

    case Spell::ANTIMAGIC:
        return unit.Modes( Battle::SP_ANTIMAGIC );

    case Spell::BERSERKER:
        return unit.Modes( Battle::SP_BERSERKER );

    case Spell::HYPNOTIZE:
        return unit.Modes( Battle::SP_HYPNOTIZE );

    case Spell::MIRRORIMAGE:
        return unit.Modes( Battle::CAP_MIRROROWNER );

    case Spell::DISRUPTINGRAY:
        return unit.GetDefense() <= 1;

    default:
        break;
    }

    return false;
}

AI::SpellcastOutcome AI::BattlePlanner::spellTeleportValue( Battle::Arena & arena, const Spell & spell, const Battle::Unit & currentUnit,
                                                            const Battle::Units & enemies ) const
{
    assert( spell == Spell::TELEPORT );

    // Teleport spell is useful in many cases. The current implementation focuses only on offensive movements for melee units.

    if ( _defensiveTactics ) {
        // TODO: implement Teleport usage for defense.
        return {};
    }

    if ( isSpellcastUselessForUnit( currentUnit, enemies, spell ) ) {
        return {};
    }

    if ( currentUnit.isFlying() ) {
        // The unit can move to any cell. Teleport spell is useless here.
        return {};
    }

    if ( currentUnit.isArchers() ) {
        // TODO: implement Teleport usage for shooters.
        return {};
    }

    Battle::Position currentPos = currentUnit.GetPosition();

    BattleTargetPair currentBestTarget;
    const double currentDamage = getMeleeBestOutcome( arena, currentUnit, enemies, currentBestTarget );
    if ( currentDamage > 0.1 ) {
        // The current monster can reach some enemies.
        return {};
    }

    // The current unit cannot be modified. So, we need to get a non-const pointer to the same unit
    // to set temporary teleport ability.
    const Battle::Units friendly( arena.getForce( _myColor ).getUnits(), Battle::Units::REMOVE_INVALID_UNITS );
    Battle::Unit * tempUnit = nullptr;

    for ( Battle::Unit * unit : friendly ) {
        if ( unit == &currentUnit ) {
            tempUnit = unit;
            break;
        }
    }

    if ( tempUnit == nullptr ) {
        // This must not happen! The monster should belong to friendly units.
        assert( 0 );
        return {};
    }

    // Temporary grant teleport ability so the unit can reach any cell.
    tempUnit->SetModes( Battle::TELEPORT_ABILITY );

    BattleTargetPair bestTarget;
    const double bestDamage = getMeleeBestOutcome( arena, currentUnit, enemies, bestTarget );

    tempUnit->ResetModes( Battle::TELEPORT_ABILITY );

    if ( bestDamage < 0.1 ) {
        // None of enemies are reachable.
        return {};
    }

    return { currentPos.GetHead()->GetIndex(), currentUnit.GetStrength() * bloodLustRatio, bestTarget.cell };
}

AI::SpellcastOutcome AI::BattlePlanner::spellEarthquakeValue( const Battle::Arena & arena, const Spell & spell, const Battle::Units & friendly ) const
{
    assert( spell == Spell::EARTHQUAKE );

    // If we are not attacking a castle, then this spell is useless.
    if ( !_attackingCastle ) {
        return {};
    }

    // If everyone is flier or archer we also don't care about castle's walls as we don't need to rush to attack enemy units in melee.
    int32_t meleeUnits{ 0 };
    double meleeStrength = 0;
    for ( const Battle::Unit * unit : friendly ) {
        if ( !unit->isFlying() && !unit->isArchers() ) {
            ++meleeUnits;
            meleeStrength += unit->GetStrength();
        }
    }

    if ( meleeUnits == 0 ) {
        return {};
    }

    // Then we need to know the state of walls and towers.
    int32_t totalTargets{ 0 };
    int32_t targetsToDestroy{ 0 };
    for ( const Battle::CastleDefenseStructure target : Battle::Arena::getEarthQuakeSpellTargets() ) {
        if ( target == Battle::CastleDefenseStructure::TOP_BRIDGE_TOWER || target == Battle::CastleDefenseStructure::BOTTOM_BRIDGE_TOWER ) {
            // These are only cosmetic buildings. They have no value for us.
            continue;
        }

        const int targetValue = arena.getCastleDefenseStructureCondition( target, Battle::SiegeWeaponType::EarthquakeSpell );
        ++totalTargets;

        if ( targetValue > 0 ) {
            ++targetsToDestroy;
        }
    }

    const auto [minDamage, maxDamage] = Battle::Arena::getEarthquakeDamageRange( _commander );

    const double enemyShooterRatio = _enemyShootersStrength / _enemyArmyStrength;
    const double targetRatio = targetsToDestroy * 1.0 / totalTargets;
    const double averageDamage = ( maxDamage - minDamage ) / 2.0;
    const double meleeRatio = meleeStrength / _myArmyStrength;

    return { 0, meleeUnits * meleeStrength * meleeRatio * targetRatio * averageDamage * enemyShooterRatio * 0.2 };
}
