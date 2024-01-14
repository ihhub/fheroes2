/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <ostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "ai.h"
#include "ai_normal.h"
#include "army.h"
#include "artifact.h"
#include "artifact_info.h"
#include "battle.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "battle_board.h"
#include "battle_cell.h"
#include "battle_command.h"
#include "battle_tower.h"
#include "battle_troop.h"
#include "castle.h"
#include "color.h"
#include "difficulty.h"
#include "game.h"
#include "heroes.h"
#include "heroes_base.h"
#include "kingdom.h"
#include "logging.h"
#include "monster_info.h"
#include "resource.h"
#include "settings.h"
#include "speed.h"
#include "spell.h"

using namespace Battle;

namespace AI
{
    const std::vector<int32_t> cellsUnderWallsIndexes = { 7, 28, 49, 72, 95 };

    struct MeleeAttackOutcome
    {
        int32_t fromIndex = -1;
        double attackValue = -INT32_MAX;
        double positionValue = -INT32_MAX;
        bool canAttackImmediately = false;
    };

    bool ValueHasImproved( double primary, double primaryMax, double secondary, double secondaryMax )
    {
        return primaryMax < primary || ( secondaryMax < secondary && std::fabs( primaryMax - primary ) < 0.001 );
    }

    bool IsOutcomeImproved( const MeleeAttackOutcome & newOutcome, const MeleeAttackOutcome & previous )
    {
        // Composite priority criteria:
        // Primary - whether the enemy unit can be attacked during the current turn
        // Secondary - position value
        // Tertiary - enemy unit's threat
        return ( newOutcome.canAttackImmediately && !previous.canAttackImmediately )
               || ( newOutcome.canAttackImmediately == previous.canAttackImmediately
                    && ValueHasImproved( newOutcome.positionValue, previous.positionValue, newOutcome.attackValue, previous.attackValue ) );
    }

    int32_t doubleCellAttackValue( const Unit & attacker, const Unit & target, const int32_t from, const int32_t targetCell )
    {
        const Cell * behind = Board::GetCell( targetCell, Board::GetDirection( from, targetCell ) );
        const Unit * secondaryTarget = ( behind != nullptr ) ? behind->GetUnit() : nullptr;

        if ( secondaryTarget && secondaryTarget->GetUID() != target.GetUID() && secondaryTarget->GetUID() != attacker.GetUID() ) {
            return secondaryTarget->evaluateThreatForUnit( attacker );
        }

        return 0;
    }

    std::pair<int32_t, int> optimalAttackVector( const Unit & attacker, const Unit & target, const Position & attackPos )
    {
        assert( attackPos.isValidForUnit( attacker ) );
        assert( Board::CanAttackTargetFromPosition( attacker, target, attackPos.GetHead()->GetIndex() ) );

        const Position & targetPos = target.GetPosition();

        const std::array<const Cell *, 2> attackCells = { attackPos.GetHead(), attackPos.GetTail() };
        const std::array<const Cell *, 2> targetCells = { targetPos.GetHead(), targetPos.GetTail() };

        std::pair<int32_t, int> bestAttackVector{ -1, UNKNOWN };
        double bestAttackValue = 0.0;

        for ( const Cell * attackCell : attackCells ) {
            if ( attackCell == nullptr ) {
                continue;
            }

            const int32_t attackCellIdx = attackCell->GetIndex();

            if ( !Board::CanAttackFromCell( attacker, attackCellIdx ) ) {
                continue;
            }

            for ( const Cell * targetCell : targetCells ) {
                if ( targetCell == nullptr ) {
                    continue;
                }

                const int32_t targetCellIdx = targetCell->GetIndex();

                if ( !Board::isNearIndexes( attackCellIdx, targetCellIdx ) ) {
                    continue;
                }

                if ( !attacker.isDoubleCellAttack() ) {
                    return { targetCellIdx, Board::GetDirection( attackCellIdx, targetCellIdx ) };
                }

                const double attackValue = doubleCellAttackValue( attacker, target, attackCellIdx, targetCellIdx );
                if ( bestAttackVector.first == -1 || bestAttackValue < attackValue ) {
                    bestAttackVector = { targetCellIdx, Board::GetDirection( attackCellIdx, targetCellIdx ) };
                    bestAttackValue = attackValue;
                }
            }
        }

        return bestAttackVector;
    }

    int32_t optimalAttackValue( const Unit & attacker, const Unit & target, const Position & attackPos )
    {
        assert( attackPos.isValidForUnit( attacker ) );

        if ( attacker.isAllAdjacentCellsAttack() ) {
            const Board * board = Arena::GetBoard();

            std::set<const Unit *> unitsUnderAttack;

            for ( const int32_t index : Board::GetAroundIndexes( attackPos ) ) {
                const Unit * unit = board->at( index ).GetUnit();

                if ( unit == nullptr || unit->GetColor() == attacker.GetCurrentColor() ) {
                    continue;
                }

                unitsUnderAttack.insert( unit );
            }

            return std::accumulate( unitsUnderAttack.begin(), unitsUnderAttack.end(), static_cast<int32_t>( 0 ),
                                    [&attacker]( const int32_t total, const Unit * unit ) { return total + unit->evaluateThreatForUnit( attacker ); } );
        }

        int32_t attackValue = target.evaluateThreatForUnit( attacker );

        // A double cell attack should only be considered if the attacker is actually able to attack the target from the given attack position. Otherwise, the attacker
        // can at least block the target if the target is a shooter, so this position can be valuable in any case.
        if ( attacker.isDoubleCellAttack() && Board::CanAttackTargetFromPosition( attacker, target, attackPos.GetHead()->GetIndex() ) ) {
            const auto [attackTargetIdx, attackDirection] = optimalAttackVector( attacker, target, attackPos );
            assert( Board::isValidDirection( attackTargetIdx, Board::GetReflectDirection( attackDirection ) ) );

            attackValue
                += doubleCellAttackValue( attacker, target, Board::GetIndexDirection( attackTargetIdx, Board::GetReflectDirection( attackDirection ) ), attackTargetIdx );
        }

        return attackValue;
    }

    using PositionValues = std::map<Position, int32_t>;

    PositionValues evaluatePotentialAttackPositions( Arena & arena, const Unit & attacker )
    {
        // Attacking unit can be under the influence of the Hypnotize spell
        Units enemies( arena.getEnemyForce( attacker.GetCurrentColor() ).getUnits(), &attacker );

        // For each position near enemy units, select the maximum attack value among neighboring enemy melee units, and then add the sum of the attack values of
        // neighboring enemy archers to encourage the use of attacking positions that block these archers
        std::sort( enemies.begin(), enemies.end(), []( const Unit * unit1, const Unit * unit2 ) { return !unit1->isArchers() && unit2->isArchers(); } );

        PositionValues result;

        for ( const Unit * enemyUnit : enemies ) {
            assert( enemyUnit != nullptr && enemyUnit->isValid() );

            std::set<Position> processedPositions;

            // Wide attacker can occupy positions from which it is able to block or attack several units at once, even if there are not one but two cells between
            // these units, e.g. like this:
            //
            // | | | |U|
            // | |A|A| |
            // |U| | | |
            //
            // It is necessary to correctly evaluate such a position as a position located "nearby" in relation to both units.
            for ( const int32_t idx : Board::GetDistanceIndexes( *enemyUnit, attacker.isWide() ? 2 : 1 ) ) {
                const Position pos = Position::GetPosition( attacker, idx );
                if ( pos.GetHead() == nullptr ) {
                    continue;
                }

                assert( pos.isValidForUnit( attacker ) );

                const uint32_t dist = Board::GetDistance( pos, enemyUnit->GetPosition() );
                assert( dist > 0 );

                if ( dist != 1 ) {
                    continue;
                }

                if ( !arena.isPositionReachable( attacker, pos, false ) ) {
                    continue;
                }

                const auto [dummy, inserted] = processedPositions.insert( pos );
                if ( !inserted ) {
                    continue;
                }

                const int32_t attackValue = optimalAttackValue( attacker, *enemyUnit, pos );
                const auto iter = result.find( pos );

                if ( iter == result.end() ) {
                    result.try_emplace( pos, attackValue );
                }
                // If attacker is able to attack all adjacent cells, then the values of all units in adjacent cells (including archers) have already been taken into
                // account
                else if ( attacker.isAllAdjacentCellsAttack() ) {
                    assert( iter->second == attackValue );
                }
                else if ( enemyUnit->isArchers() ) {
                    iter->second += attackValue;
                }
                else {
                    iter->second = std::max( iter->second, attackValue );
                }
            }
        }

        return result;
    }

    bool isUnitAbleToApproachPosition( const Unit * unit, const Position & pos )
    {
        assert( unit != nullptr );

        // Also consider the next turn, even if this unit has already acted during the current turn
        const uint32_t speed = unit->GetSpeed( false, true );

        // Immovable unit is not taken into account, even if it is already near the given position
        if ( speed == Speed::STANDING ) {
            return false;
        }

        for ( const int32_t nearbyIdx : Board::GetAroundIndexes( pos ) ) {
            const Position nearbyPos = Position::GetReachable( *unit, nearbyIdx, speed );
            if ( nearbyPos.GetHead() == nullptr ) {
                continue;
            }

            assert( nearbyPos.isValidForUnit( unit ) );

            return true;
        }

        return false;
    }

    MeleeAttackOutcome BestAttackOutcome( const Unit & attacker, const Unit & defender, const PositionValues & valuesOfAttackPositions,
                                          const std::function<bool( const Position & )> & posFilter = {} )
    {
        MeleeAttackOutcome bestOutcome;

        std::vector<Position> aroundDefender;
        aroundDefender.reserve( valuesOfAttackPositions.size() );

        for ( const auto & [pos, dummy] : valuesOfAttackPositions ) {
            const uint32_t dist = Board::GetDistance( pos, defender.GetPosition() );
            assert( dist > 0 );

            if ( dist != 1 ) {
                continue;
            }

            aroundDefender.push_back( pos );
        }

        // Prefer the positions closest to the current attacker's position
        std::sort( aroundDefender.begin(), aroundDefender.end(), [&attacker]( const Position & pos1, const Position & pos2 ) {
            return ( Board::GetDistance( attacker.GetPosition(), pos1 ) < Board::GetDistance( attacker.GetPosition(), pos2 ) );
        } );

        // Pick the best position to attack from
        for ( const Position & pos : aroundDefender ) {
            assert( pos.GetHead() != nullptr );

            if ( posFilter && !posFilter( pos ) ) {
                continue;
            }

            const int32_t posHeadIdx = pos.GetHead()->GetIndex();

            const auto posValueIter = valuesOfAttackPositions.find( pos );
            assert( posValueIter != valuesOfAttackPositions.end() );

            MeleeAttackOutcome current;
            current.attackValue = optimalAttackValue( attacker, defender, pos );
            current.positionValue = posValueIter->second;
            current.canAttackImmediately = Board::CanAttackTargetFromPosition( attacker, defender, posHeadIdx );

            // Pick target if either position has improved or unit is higher value at the same position value
            if ( IsOutcomeImproved( current, bestOutcome ) ) {
                bestOutcome.fromIndex = posHeadIdx;
                bestOutcome.attackValue = current.attackValue;
                bestOutcome.positionValue = current.positionValue;
                bestOutcome.canAttackImmediately = current.canAttackImmediately;
            }
        }

        return bestOutcome;
    }

    int32_t findOptimalPositionForSubsequentAttack( Arena & arena, const Indexes & path, const Unit & currentUnit, const Battle::Units & enemies )
    {
        const Position & currentUnitPos = currentUnit.GetPosition();

        std::vector<std::pair<Position, double>> pathStepsThreatLevels;
        pathStepsThreatLevels.reserve( path.size() );

        {
            Position stepPos = currentUnitPos;

            for ( const int32_t stepIdx : path ) {
                if ( currentUnit.isWide() ) {
                    const Cell * stepPosTailCell = stepPos.GetTail();
                    assert( stepPosTailCell != nullptr );

                    // Reversal is not a movement
                    if ( stepIdx == stepPosTailCell->GetIndex() ) {
                        stepPos.Set( stepIdx, currentUnit.isWide(), !stepPos.isReflect() );

                        continue;
                    }
                }

                stepPos.Set( stepIdx, currentUnit.isWide(), stepPos.isReflect() );

                assert( arena.isPositionReachable( currentUnit, stepPos, true ) );

#ifdef NDEBUG
                (void)arena;
#endif

                pathStepsThreatLevels.emplace_back( stepPos, 0.0 );
            }
        }

        for ( const Unit * enemy : enemies ) {
            assert( enemy != nullptr );

            // Archers and Flyers are always a threat
            if ( enemy->isFlying() || ( enemy->isArchers() && !enemy->isHandFighting() ) ) {
                continue;
            }

            for ( auto & [stepPos, stepThreatLevel] : pathStepsThreatLevels ) {
                if ( !isUnitAbleToApproachPosition( enemy, stepPos ) ) {
                    continue;
                }

                stepThreatLevel += enemy->evaluateThreatForUnit( currentUnit, stepPos );
            }
        }

        double lowestThreat = 0.0;
        int32_t targetIdx = -1;

        for ( const auto & [stepPos, stepThreatLevel] : pathStepsThreatLevels ) {
            // We need to get as close to the target as possible (taking into account the threat level)
            if ( targetIdx == -1 || stepThreatLevel < lowestThreat || std::fabs( stepThreatLevel - lowestThreat ) < 0.001 ) {
                assert( stepPos.isValidForUnit( currentUnit ) );

                lowestThreat = stepThreatLevel;
                // When moving along the path, the direction of a wide unit at some steps may be reversed in relation to the target one. Detect this and use the proper
                // index.
                targetIdx
                    = ( !currentUnit.isWide() || stepPos.isReflect() == currentUnitPos.isReflect() ) ? stepPos.GetHead()->GetIndex() : stepPos.GetTail()->GetIndex();
            }
        }

        return targetIdx;
    }

    struct CellDistanceInfo
    {
        int32_t idx = -1;
        uint32_t dist = UINT32_MAX;
    };

    CellDistanceInfo findNearestCellNextToUnit( Arena & arena, const Unit & currentUnit, const Unit & target )
    {
        CellDistanceInfo result;

        for ( const int32_t idx : Board::GetDistanceIndexes( target, currentUnit.isWide() ? 2 : 1 ) ) {
            const Position pos = Position::GetPosition( currentUnit, idx );
            if ( pos.GetHead() == nullptr ) {
                continue;
            }

            assert( pos.isValidForUnit( currentUnit ) );

            const uint32_t dist = Board::GetDistance( pos, target.GetPosition() );
            assert( dist > 0 );

            if ( dist != 1 ) {
                continue;
            }

            if ( !arena.isPositionReachable( currentUnit, pos, false ) ) {
                continue;
            }

            const uint32_t moveDist = arena.CalculateMoveDistance( currentUnit, pos );
            if ( result.idx == -1 || moveDist < result.dist ) {
                result = { idx, moveDist };
            }
        }

        return result;
    }

    int32_t getUnitMovementTarget( Arena & arena, const Unit & currentUnit, const int32_t idx )
    {
        // First try to find the position that is reachable on the current turn
        {
            const Position pos = Position::GetReachable( currentUnit, idx );
            if ( pos.GetHead() != nullptr ) {
                assert( pos.isValidForUnit( currentUnit ) );

                return pos.GetHead()->GetIndex();
            }
        }

        // If there is no such position, then use the last position on the path to the cell with the specified index
        const Position dstPos = Position::GetPosition( currentUnit, idx );
        assert( dstPos.isValidForUnit( currentUnit ) );

        const Position pos = arena.getClosestReachablePosition( currentUnit, dstPos );
        assert( pos.isValidForUnit( currentUnit ) );

        return pos.GetHead()->GetIndex();
    }

    void Normal::HeroesPreBattle( HeroBase & hero, bool isAttacking )
    {
        if ( isAttacking ) {
            OptimizeTroopsOrder( hero.GetArmy() );
        }
    }

    bool BattlePlanner::isHeroWorthSaving( const Heroes & hero ) const
    {
        return hero.GetLevel() > 2 || !hero.GetBagArtifacts().empty();
    }

    bool BattlePlanner::isCommanderCanSpellcast( const Arena & arena, const HeroBase * commander ) const
    {
        return commander && ( !commander->isControlHuman() || Settings::Get().BattleAutoSpellcast() ) && commander->HaveSpellBook()
               && !commander->Modes( Heroes::SPELLCASTED ) && !arena.isSpellcastDisabled();
    }

    bool BattlePlanner::checkRetreatCondition( const Heroes & hero ) const
    {
        if ( !_considerRetreat || hero.isControlHuman() || hero.isLosingGame() || !isHeroWorthSaving( hero ) || !CanPurchaseHero( hero.GetKingdom() ) ) {
            return false;
        }

        // Retreat if remaining army strength is a fraction of enemy's
        // Consider taking speed/turn order into account in the future
        return _myArmyStrength * Difficulty::getArmyStrengthRatioForAIRetreat( Game::getDifficulty() ) < _enemyArmyStrength;
    }

    bool BattlePlanner::isUnitFaster( const Unit & currentUnit, const Unit & target ) const
    {
        if ( currentUnit.isFlying() == target.isFlying() )
            return currentUnit.GetSpeed() > target.GetSpeed();
        return currentUnit.isFlying();
    }

    void BattlePlanner::battleBegins()
    {
        _currentTurnNumber = 0;
        _numberOfRemainingTurnsWithoutDeaths = MAX_TURNS_WITHOUT_DEATHS;
        _attackerForceNumberOfDead = 0;
        _defenderForceNumberOfDead = 0;
    }

    bool BattlePlanner::isLimitOfTurnsExceeded( const Arena & arena, Actions & actions )
    {
        const int currentColor = arena.GetCurrentColor();

        // Not the attacker's turn, no further checks
        if ( currentColor != arena.GetArmy1Color() ) {
            return false;
        }

        const uint32_t currentTurnNumber = arena.GetCurrentTurn();
        assert( currentTurnNumber > 0 );

        // This is the beginning of a new turn and we still haven't gone beyond the limit on the number of turns without deaths
        if ( currentTurnNumber > _currentTurnNumber && _numberOfRemainingTurnsWithoutDeaths > 0 ) {
            auto prevNumbersOfDead = std::tie( _attackerForceNumberOfDead, _defenderForceNumberOfDead );
            const auto currNumbersOfDead = std::make_tuple( arena.GetForce1().GetDeadCounts(), arena.GetForce2().GetDeadCounts() );

            // Either we don't have numbers of dead units from the previous turn, or there were changes in these numbers compared
            // to the previous turn, reset the counter
            if ( _currentTurnNumber == 0 || currentTurnNumber - _currentTurnNumber != 1 || prevNumbersOfDead != currNumbersOfDead ) {
                prevNumbersOfDead = currNumbersOfDead;

                _numberOfRemainingTurnsWithoutDeaths = MAX_TURNS_WITHOUT_DEATHS;
            }
            // No changes in numbers of dead units compared to the previous turn, decrease the counter of the remaining turns
            else {
                _numberOfRemainingTurnsWithoutDeaths -= 1;
            }

            _currentTurnNumber = currentTurnNumber;
        }

        // We have gone beyond the limit on the number of turns without deaths and have to stop
        if ( _numberOfRemainingTurnsWithoutDeaths == 0 ) {
            // If this is an auto battle (and not the instant battle, because the battle UI is present), then turn it off until the end of the battle
            if ( arena.AutoBattleInProgress() && Arena::GetInterface() != nullptr ) {
                assert( arena.CanToggleAutoBattle() );

                actions.emplace_back( Command::AUTO_SWITCH, currentColor );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO, Color::String( currentColor ) << " has used up the limit of turns without deaths, auto battle is turned off" )
            }
            // Otherwise the attacker's hero should retreat
            else {
                assert( arena.CanRetreatOpponent( currentColor ) && arena.GetCurrentCommander() != nullptr );

                actions.emplace_back( Command::RETREAT );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO,
                           Color::String( currentColor ) << " has used up the limit of turns without deaths, " << arena.GetCurrentCommander()->GetName() << " retreats" )
            }

            return true;
        }

        return false;
    }

    Actions BattlePlanner::planUnitTurn( Arena & arena, const Unit & currentUnit )
    {
        if ( currentUnit.Modes( SP_BERSERKER ) ) {
            return berserkTurn( arena, currentUnit );
        }

        Actions actions;

        // Step 1. Analyze current battle state and update variables
        analyzeBattleState( arena, currentUnit );

        DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " begin the turn, color: " << Color::String( _myColor ) )

        // Step 2. Check retreat/surrender condition
        const Heroes * actualHero = dynamic_cast<const Heroes *>( _commander );
        if ( actualHero && checkRetreatCondition( *actualHero ) ) {
            const auto farewellSpellcast = [this, &arena, &currentUnit, &actions]() {
                if ( !isCommanderCanSpellcast( arena, _commander ) ) {
                    return;
                }

                // Cast a spell with maximum damage
                const SpellSelection & bestSpell = selectBestSpell( arena, currentUnit, true );
                if ( bestSpell.spellID == -1 ) {
                    return;
                }

                actions.emplace_back( Command::SPELLCAST, bestSpell.spellID, bestSpell.cell );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO,
                           arena.GetCurrentCommander()->GetName() << " casts " << Spell( bestSpell.spellID ).GetName() << " on cell " << bestSpell.cell )
            };

            enum class Outcome
            {
                ContinueBattle,
                Retreat,
                Surrender
            };

            const Outcome outcome = [this, &arena, actualHero]() {
                const Force & force = arena.getForce( _myColor );
                const Kingdom & kingdom = actualHero->GetKingdom();

                const bool canRetreat = arena.CanRetreatOpponent( _myColor );
                const bool canSurrender = arena.CanSurrenderOpponent( _myColor );

                if ( !canRetreat ) {
                    if ( !canSurrender ) {
                        return Outcome::ContinueBattle;
                    }

                    if ( !kingdom.AllowPayment( { Resource::GOLD, force.GetSurrenderCost() } ) ) {
                        return Outcome::ContinueBattle;
                    }

                    return Outcome::Surrender;
                }

                if ( !canSurrender ) {
                    return Outcome::Retreat;
                }

                if ( force.getStrengthOfArmyRemainingInCaseOfSurrender() < Army::getStrengthOfAverageStartingArmy( actualHero ) ) {
                    return Outcome::Retreat;
                }

                if ( !kingdom.AllowPayment( Funds{ Resource::GOLD, force.GetSurrenderCost() }
                                            * Difficulty::getGoldReserveRatioForAISurrender( Game::getDifficulty() ) ) ) {
                    return Outcome::Retreat;
                }

                return Outcome::Surrender;
            }();

            switch ( outcome ) {
            case Outcome::ContinueBattle:
                break;
            case Outcome::Retreat:
                farewellSpellcast();

                actions.emplace_back( Command::RETREAT );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO, arena.GetCurrentCommander()->GetName() << " retreats" )

                return actions;
            case Outcome::Surrender:
                farewellSpellcast();

                actions.emplace_back( Command::SURRENDER );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO, arena.GetCurrentCommander()->GetName() << " surrenders" )

                return actions;
            default:
                assert( 0 );
                break;
            }
        }

        // Step 3. Calculate spell heuristics
        if ( isCommanderCanSpellcast( arena, _commander ) ) {
            const SpellSelection & bestSpell = selectBestSpell( arena, currentUnit, false );

            if ( bestSpell.spellID != -1 ) {
                actions.emplace_back( Command::SPELLCAST, bestSpell.spellID, bestSpell.cell );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO,
                           arena.GetCurrentCommander()->GetName() << " casts " << Spell( bestSpell.spellID ).GetName() << " on cell " << bestSpell.cell )

                return actions;
            }
        }

        // Step 4. Current unit decision tree
        const size_t actionsSize = actions.size();

        if ( currentUnit.isArchers() ) {
            const Actions archerActions = archerDecision( arena, currentUnit );
            actions.insert( actions.end(), archerActions.begin(), archerActions.end() );
        }
        else {
            // Melee unit decision tree (both flyers and walkers)
            BattleTargetPair target;

            // Determine unit target or cell to move to
            if ( _defensiveTactics ) {
                target = meleeUnitDefense( arena, currentUnit );
            }
            else {
                target = meleeUnitOffense( arena, currentUnit );
            }

            // Melee unit final stage - add actions to the queue
            if ( target.cell != -1 ) {
                // The target cell of the movement must be the cell that the unit's head will occupy
                const int32_t moveTargetIdx = getUnitMovementTarget( arena, currentUnit, target.cell );

                if ( target.unit ) {
                    const Position attackPos = Position::GetReachable( currentUnit, moveTargetIdx );
                    assert( attackPos.isValidForUnit( currentUnit ) );

                    const auto [attackTargetIdx, attackDirection] = optimalAttackVector( currentUnit, *target.unit, attackPos );

                    actions.emplace_back( Command::ATTACK, currentUnit.GetUID(), target.unit->GetUID(),
                                          ( currentUnit.GetHeadIndex() == moveTargetIdx ? -1 : moveTargetIdx ), attackTargetIdx, attackDirection );

                    DEBUG_LOG( DBG_BATTLE, DBG_INFO,
                               currentUnit.GetName() << " attacking enemy " << target.unit->GetName() << " from cell " << moveTargetIdx
                                                     << ", attack vector: " << Board::GetIndexDirection( attackTargetIdx, Board::GetReflectDirection( attackDirection ) )
                                                     << " -> " << attackTargetIdx << ", threat level: " << target.unit->evaluateThreatForUnit( currentUnit ) )
                }
                else if ( currentUnit.GetHeadIndex() != moveTargetIdx ) {
                    actions.emplace_back( Command::MOVE, currentUnit.GetUID(), moveTargetIdx );

                    DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " moving to cell " << moveTargetIdx )
                }
                // Else skip the turn
            }
            // Else skip the turn
        }

        // No action was taken, skip the turn
        if ( actions.size() == actionsSize ) {
            actions.emplace_back( Command::SKIP, currentUnit.GetUID() );

            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " skipping the turn" )
        }

        return actions;
    }

    void BattlePlanner::analyzeBattleState( const Arena & arena, const Unit & currentUnit )
    {
        _myColor = currentUnit.GetCurrentColor();
        _commander = arena.getCommander( _myColor );

        const Force & friendlyForce = arena.getForce( _myColor );
        const Force & enemyForce = arena.getEnemyForce( _myColor );

        // Friendly and enemy army analysis
        _myArmyStrength = 0;
        _enemyArmyStrength = 0;
        _myShootersStrength = 0;
        _enemyShootersStrength = 0;
        _myRangedUnitsOnly = 0;
        _enemyRangedUnitsOnly = 0;
        _myArmyAverageSpeed = 0;
        _enemyAverageSpeed = 0;
        _enemySpellStrength = 0;
        _attackingCastle = false;
        _defendingCastle = false;
        _considerRetreat = false;
        _defensiveTactics = false;
        _cautiousOffensive = false;

        if ( enemyForce.empty() ) {
            return;
        }

        double sumEnemyStr = 0.0;

        for ( const Unit * unit : enemyForce ) {
            assert( unit != nullptr );

            if ( !unit->isValid() ) {
                continue;
            }

            const double unitStr = unit->GetStrength();

            _enemyArmyStrength += unitStr;

            if ( unit->isArchers() && !unit->isImmovable() ) {
                _enemyRangedUnitsOnly += unitStr;
            }

            // The average speed is weighted by the troop strength
            _enemyAverageSpeed += unit->GetSpeed( false, true ) * unitStr;

            sumEnemyStr += unitStr;
        }

        _enemyShootersStrength = _enemyRangedUnitsOnly;

        if ( sumEnemyStr > 0.0 ) {
            _enemyAverageSpeed /= sumEnemyStr;
        }

        uint32_t initialUnitCount = 0;
        double sumArmyStr = 0.0;

        for ( const Unit * unit : friendlyForce ) {
            assert( unit != nullptr );

            // Do not check isValid() here to handle dead troops

            const uint32_t count = unit->GetCount();
            const uint32_t dead = unit->GetDead();

            // Count all valid troops in army (both alive and dead)
            if ( count > 0 || dead > 0 ) {
                ++initialUnitCount;
            }

            const double unitStr = unit->GetStrength();

            // The average speed is weighted by the troop strength
            _myArmyAverageSpeed += unit->GetSpeed( false, true ) * unitStr;

            sumArmyStr += unitStr;

            // Dead unit: trigger retreat condition and skip strength calculation
            if ( count == 0 && dead > 0 ) {
                _considerRetreat = true;
                continue;
            }

            _myArmyStrength += unitStr;

            if ( unit->isArchers() && !unit->isImmovable() ) {
                _myRangedUnitsOnly += unitStr;
            }
        }

        _myShootersStrength = _myRangedUnitsOnly;

        if ( sumArmyStr > 0.0 ) {
            _myArmyAverageSpeed /= sumArmyStr;
        }

        _considerRetreat = _considerRetreat || initialUnitCount < 4;

        // Add castle siege (and battle arena) modifiers
        const Castle * castle = Arena::GetCastle();

        // Mark as castle siege only if any tower is present. If no towers present then nothing to defend and most likely all walls are destroyed as well.
        if ( castle && Arena::isAnyTowerPresent() ) {
            const bool attackerIgnoresCover
                = arena.GetForce1().GetCommander()->GetBagArtifacts().isArtifactBonusPresent( fheroes2::ArtifactBonusType::NO_SHOOTING_PENALTY );

            const auto getTowerStrength = []( const Tower * tower ) { return ( tower && tower->isValid() ) ? tower->GetStrength() : 0; };

            double towerStr = getTowerStrength( Arena::GetTower( TowerType::TWR_CENTER ) );
            towerStr += getTowerStrength( Arena::GetTower( TowerType::TWR_LEFT ) );
            towerStr += getTowerStrength( Arena::GetTower( TowerType::TWR_RIGHT ) );

            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Castle strength: " << towerStr )

            // Tower strength can't be negative. If this assertion triggers something is wrong with the logic above.
            assert( towerStr >= 0 );

            if ( _myColor == castle->GetColor() ) {
                _defendingCastle = true;
                _myShootersStrength += towerStr;

                if ( !attackerIgnoresCover ) {
                    _enemyShootersStrength /= 1.5;
                }
            }
            else {
                _attackingCastle = true;
                _enemyShootersStrength += towerStr;

                if ( !attackerIgnoresCover ) {
                    _myShootersStrength /= 1.5;
                }
            }
        }

        // Calculate each hero spell strength and add it to shooter values after castle modifiers were applied
        if ( _commander && _myShootersStrength > 1 ) {
            _myShootersStrength += commanderMaximumSpellDamageValue( *_commander );
        }

        const HeroBase * enemyCommander = arena.getEnemyCommander( _myColor );
        if ( enemyCommander ) {
            _enemySpellStrength = enemyCommander->GetMagicStrategicValue( _myArmyStrength );
            _enemyShootersStrength += commanderMaximumSpellDamageValue( *enemyCommander );
        }

        assert( _myArmyStrength > 0.0 && _enemyArmyStrength > 0.0 );

        const double myArcherRatio = _myShootersStrength / _myArmyStrength;
        const double enemyArcherRatio = _enemyShootersStrength / _enemyArmyStrength;

        _defensiveTactics = [this, &currentUnit, myArcherRatio, enemyArcherRatio]() {
            // Unit is already in the enemy half of the battlefield, just let it keep attacking
            if ( !isPositionLocatedInDefendedArea( currentUnit, currentUnit.GetPosition() ) ) {
                return false;
            }

            const double overPowerRatio = ( currentUnit.isFlying() ? 6 : 10 );

            // When we have a X times stronger army than the enemy, then we are likely to win, there is no need to go on the defensive
            if ( _myArmyStrength > _enemyArmyStrength * overPowerRatio ) {
                return false;
            }

            // When we have fewer shooters than the enemy, it makes no sense to go on the defensive
            if ( _myShootersStrength < _enemyShootersStrength ) {
                return false;
            }

            // We have at least as many shooters as the enemy and we defend the castle under the protection of walls and towers, it makes sense to choose defensive
            // tactics
            if ( _defendingCastle ) {
                return true;
            }

            // If we have an unfavorable ratio of infantry and shooters for defense, then it is better to choose an offensive
            if ( myArcherRatio < 0.15 ) {
                return false;
            }

            // If the enemy has too many shooters, but not enough infantry to cover them, then it makes sense to choose an offensive
            if ( enemyArcherRatio > 0.66 ) {
                return false;
            }

            return true;
        }();

        // If an offensive tactic is chosen, then this means that, most likely, the enemy has more shooters, which means that we should try to attack the enemy and
        // neutralize his shooters as quickly as possible. A cautious offensive tactics can be chosen only if our army is fighting an enemy army that has limited
        // distance attack capabilities.
        _cautiousOffensive = ( enemyArcherRatio < 0.15 );

        DEBUG_LOG( DBG_BATTLE, DBG_INFO,
                   ( _defensiveTactics ? "Defensive" : ( _cautiousOffensive ? "Cautious offensive" : "Offensive" ) )
                       << " tactics have been chosen. Army strength: " << _myArmyStrength << ", shooters strength: " << _myShootersStrength
                       << ", enemy army strength: " << _enemyArmyStrength << ", enemy shooters strength: " << _enemyShootersStrength )
    }

    Actions BattlePlanner::archerDecision( Arena & arena, const Unit & currentUnit ) const
    {
        Actions actions;

        // Current unit can be under the influence of the Hypnotize spell
        const Units enemies( arena.getEnemyForce( _myColor ).getUnits(), &currentUnit );

        // Assess the current threat level and decide whether to retreat to another position
        const int32_t retreatPositionIndex = [&arena, &currentUnit, &enemies]() {
            // There is no point in trying to retreat from flying units regardless of their speed
            if ( std::any_of( enemies.begin(), enemies.end(), []( const Unit * enemy ) {
                     assert( enemy != nullptr );

                     return enemy->isFlying();
                 } ) ) {
                return -1;
            }

            struct PositionCharacteristics
            {
                // Indexes of the head cells of all enemy units that can potentially reach this position
                std::set<int32_t> threateningEnemiesIndexes;
                // Distance between this position and the nearest enemy unit
                uint32_t distanceToNearestEnemy{ UINT32_MAX };
            };

            const auto evaluatePotentialPositions = [&currentUnit, &enemies]( std::map<Position, PositionCharacteristics> & potentialPositions ) {
                class UnitRemover
                {
                public:
                    explicit UnitRemover( const Unit & unitToRemove )
                    {
                        const int32_t headIdx = unitToRemove.GetHeadIndex();
                        const int32_t tailIdx = unitToRemove.GetTailIndex();

                        assert( headIdx != -1 && ( unitToRemove.isWide() ? tailIdx != -1 : tailIdx == -1 ) );

                        unitToRestore = getUnitOnCell( headIdx );
                        assert( unitToRestore == &unitToRemove );

                        setUnitForCell( headIdx, nullptr );

                        if ( tailIdx != -1 ) {
                            assert( unitToRestore == getUnitOnCell( tailIdx ) );

                            setUnitForCell( tailIdx, nullptr );
                        }
                    }

                    UnitRemover( const UnitRemover & ) = delete;

                    ~UnitRemover()
                    {
                        assert( unitToRestore != nullptr );

                        const int32_t headIdx = unitToRestore->GetHeadIndex();
                        const int32_t tailIdx = unitToRestore->GetTailIndex();

                        assert( headIdx != -1 && ( unitToRestore->isWide() ? tailIdx != -1 : tailIdx == -1 ) );

                        setUnitForCell( headIdx, unitToRestore );

                        if ( tailIdx != -1 ) {
                            setUnitForCell( tailIdx, unitToRestore );
                        }
                    }

                    UnitRemover & operator=( const UnitRemover & ) = delete;

                private:
                    static Unit * getUnitOnCell( const int32_t idx )
                    {
                        Cell * cell = Board::GetCell( idx );
                        assert( cell != nullptr );

                        return cell->GetUnit();
                    }

                    static void setUnitForCell( const int32_t idx, Unit * unit )
                    {
                        Cell * cell = Board::GetCell( idx );
                        assert( cell != nullptr );

                        cell->SetUnit( unit );
                    }

                    Unit * unitToRestore = nullptr;
                };

                // In order to correctly assess the safety of potential positions for the unit to be moved, we need
                // to temporarily remove this unit from the battlefield, otherwise, it can block the movement of enemy
                // units towards these new potential positions.
                const UnitRemover unitRemover( currentUnit );

                for ( const Unit * enemy : enemies ) {
                    assert( enemy != nullptr );

                    for ( auto & [position, characteristics] : potentialPositions ) {
                        assert( position.GetHead() != nullptr );

                        const bool isPositionUnderEnemyThreat = [enemy]( const Position & pos ) {
                            // Archers who not blocked by enemy units generally threaten any position, but for the purpose
                            // of this assessment, it is assumed that they threaten only in melee, that is, in positions
                            // directly adjacent to them
                            if ( enemy->isArchers() && !enemy->isHandFighting() ) {
                                const uint32_t distanceToEnemy = Board::GetDistance( pos, enemy->GetPosition() );
                                assert( distanceToEnemy > 0 );

                                return ( distanceToEnemy == 1 );
                            }

                            // The potential event of enemy's good morale is not taken into account here
                            return isUnitAbleToApproachPosition( enemy, pos );
                        }( position );

                        if ( isPositionUnderEnemyThreat ) {
                            characteristics.threateningEnemiesIndexes.insert( enemy->GetHeadIndex() );
                        }

                        characteristics.distanceToNearestEnemy = std::min( characteristics.distanceToNearestEnemy, Board::GetDistance( position, enemy->GetPosition() ) );
                        assert( characteristics.distanceToNearestEnemy > 0 );
                    }
                }
            };

            std::map<Position, PositionCharacteristics> potentialPositions;

            // The current position is also considered as a potential one
            potentialPositions.try_emplace( currentUnit.GetPosition() );

            for ( const int32_t idx : arena.getAllAvailableMoves( currentUnit ) ) {
                potentialPositions.try_emplace( Position::GetReachable( currentUnit, idx ) );
            }

            evaluatePotentialPositions( potentialPositions );

            {
                const auto currentPositionIter = potentialPositions.find( currentUnit.GetPosition() );
                assert( currentPositionIter != potentialPositions.end() );

                const auto & [position, characteristics] = *currentPositionIter;

                // If the current position is not in danger, then nothing special should be done
                if ( characteristics.threateningEnemiesIndexes.empty() ) {
                    return -1;
                }

                const uint32_t currentUnitSpeed = currentUnit.GetSpeed();
                assert( currentUnitSpeed > Speed::STANDING );

                // The current position is in danger, let's evaluate the possibility of a retreat
                const bool isItWorthTryingToRetreat = std::all_of( characteristics.threateningEnemiesIndexes.begin(), characteristics.threateningEnemiesIndexes.end(),
                                                                   [&arena, currentUnitSpeed]( const int32_t enemyIdx ) {
                                                                       const Unit * enemy = arena.GetTroopBoard( enemyIdx );
                                                                       assert( enemy != nullptr && !enemy->isFlying() );

                                                                       // Also consider the next turn, even if this unit has already acted during the current turn
                                                                       const uint32_t enemySpeed = enemy->GetSpeed( false, true );
                                                                       assert( enemySpeed > Speed::STANDING );

                                                                       // In order for it to make sense to try to retreat from the enemy, the enemy should be somewhat
                                                                       // slower
                                                                       return ( enemySpeed + 2 < currentUnitSpeed );
                                                                   } );

                if ( !isItWorthTryingToRetreat ) {
                    return -1;
                }
            }

            // The current position is in danger, but there is an opportunity to retreat. Let's try to find a position to retreat.
            int32_t safestIdx = -1;
            // Distance to the nearest enemy unit (the more, the better) and inverse of the distance to the central cell of the battlefield (1/x,
            // i.e. the smaller the x the better). The idea is that corner cells should be avoided whenever possible when retreating because they
            // can easily be blocked by enemy units.
            std::pair<uint32_t, double> safestParams{ 0, 0.0 };

            for ( const auto & [position, characteristics] : potentialPositions ) {
                assert( position.GetHead() != nullptr && characteristics.distanceToNearestEnemy > 0 );

                // Only completely safe positions are considered as candidates
                if ( !characteristics.threateningEnemiesIndexes.empty() ) {
                    continue;
                }

                const int32_t idx = position.GetHead()->GetIndex();
                const uint32_t distanceToBattlefieldCenter = Board::GetDistance( idx, ARENASIZE / 2 );

                const auto idxParams
                    = std::make_pair( characteristics.distanceToNearestEnemy, distanceToBattlefieldCenter == 0 ? 1.0 : 1.0 / distanceToBattlefieldCenter );

                if ( safestParams < idxParams ) {
                    safestIdx = idx;
                    safestParams = idxParams;
                }
            }

            return safestIdx;
        }();

        // The current position of the archers is not safe, but there is somewhere to retreat
        if ( retreatPositionIndex != -1 ) {
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " retreating from enemy, target cell: " << retreatPositionIndex )

            // The target cell of the movement must be the cell that the unit's head will occupy
            const int32_t moveTargetIdx = getUnitMovementTarget( arena, currentUnit, retreatPositionIndex );

            if ( currentUnit.GetHeadIndex() != moveTargetIdx ) {
                actions.emplace_back( Command::MOVE, currentUnit.GetUID(), moveTargetIdx );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " moving to cell " << moveTargetIdx )
            }
            else {
                // It is impossible to "retreat" to the same position you are already in
                assert( 0 );
            }
        }
        // Archers are blocked and there is nowhere to retreat, they are fighting in melee
        else if ( currentUnit.isHandFighting() ) {
            BattleTargetPair target;
            int32_t bestOutcome = INT32_MIN;

            for ( const int32_t cellIdx : Board::GetAdjacentEnemiesIndexes( currentUnit ) ) {
                const Unit * enemy = Board::GetCell( cellIdx )->GetUnit();
                assert( enemy != nullptr );

                const int32_t archerMeleeDmg = [&currentUnit, enemy]() {
                    if ( currentUnit.Modes( SP_CURSE ) ) {
                        return currentUnit.CalculateMinDamage( *enemy );
                    }

                    if ( currentUnit.Modes( SP_BLESS ) ) {
                        return currentUnit.CalculateMaxDamage( *enemy );
                    }

                    return ( currentUnit.CalculateMinDamage( *enemy ) + currentUnit.CalculateMaxDamage( *enemy ) ) / 2;
                }();

                const int32_t retaliatoryDmg = enemy->EstimateRetaliatoryDamage( archerMeleeDmg );
                const int32_t damageDiff = archerMeleeDmg - retaliatoryDmg;

                if ( bestOutcome < damageDiff ) {
                    bestOutcome = damageDiff;

                    target.unit = enemy;

                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Set melee attack priority on " << enemy->GetName() << ", value: " << damageDiff )
                }
            }

            if ( target.unit ) {
                actions.emplace_back( Command::ATTACK, currentUnit.GetUID(), target.unit->GetUID(), -1, -1, -1 );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " attacking enemy " << target.unit->GetName() << " in melee, outcome: " << bestOutcome )
            }
        }
        // Archers are able to shoot
        else {
            BattleTargetPair target;
            double highestPriority = -1;

            for ( const Unit * enemy : enemies ) {
                assert( enemy != nullptr );

                const auto updateBestTarget = [&target, &highestPriority, enemy]( const double priority, const int32_t targetIdx ) {
                    if ( highestPriority < priority ) {
                        highestPriority = priority;

                        target.cell = targetIdx;
                        target.unit = enemy;

                        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Set shooting attack priority on " << enemy->GetName() << ", value: " << priority )
                    }
                };

                if ( currentUnit.isAbilityPresent( fheroes2::MonsterAbilityType::AREA_SHOT ) ) {
                    const auto calculateAreaShotAttackPriority = [&arena, &currentUnit, enemy]( const int32_t targetIdx ) {
                        double result = 0.0;

                        // Indexes of the head cells of the units are used instead of pointers because the exact result of adding several
                        // floating-point numbers may depend on the order of their addition, so the order of the elements must be deterministic.
                        std::set<int32_t> affectedUnitsIndexes;

                        affectedUnitsIndexes.insert( enemy->GetHeadIndex() );

                        for ( const int32_t cellIdx : Board::GetAroundIndexes( targetIdx ) ) {
                            const Unit * unit = arena.GetTroopBoard( cellIdx );
                            if ( unit == nullptr ) {
                                continue;
                            }

                            affectedUnitsIndexes.insert( unit->GetHeadIndex() );
                        }

                        for ( const int32_t unitIdx : affectedUnitsIndexes ) {
                            const Unit * unit = arena.GetTroopBoard( unitIdx );
                            assert( unit != nullptr );

                            result += unit->evaluateThreatForUnit( currentUnit );
                        }

                        return result;
                    };

                    const int32_t enemyHeadIdx = enemy->GetHeadIndex();
                    assert( enemyHeadIdx != -1 );

                    updateBestTarget( calculateAreaShotAttackPriority( enemyHeadIdx ), enemyHeadIdx );

                    if ( enemy->isWide() ) {
                        const int32_t enemyTailIdx = enemy->GetTailIndex();
                        assert( enemyTailIdx != -1 );

                        updateBestTarget( calculateAreaShotAttackPriority( enemyTailIdx ), enemyTailIdx );
                    }

                    continue;
                }

                updateBestTarget( enemy->evaluateThreatForUnit( currentUnit ), -1 );
            }

            if ( target.unit ) {
                actions.emplace_back( Command::ATTACK, currentUnit.GetUID(), target.unit->GetUID(), -1, target.cell, 0 );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " shooting at enemy " << target.unit->GetName() << ", value: " << highestPriority )
            }
        }

        return actions;
    }

    BattleTargetPair BattlePlanner::meleeUnitOffense( Arena & arena, const Unit & currentUnit ) const
    {
        BattleTargetPair target;

        const PositionValues valuesOfAttackPositions = evaluatePotentialAttackPositions( arena, currentUnit );

        // Current unit can be under the influence of the Hypnotize spell
        const Units enemies( arena.getEnemyForce( _myColor ).getUnits(), &currentUnit );

        // 1. Choose the best target within reach, if any
        {
            MeleeAttackOutcome bestOutcome;

            for ( const Unit * enemy : enemies ) {
                assert( enemy != nullptr );

                const MeleeAttackOutcome outcome = BestAttackOutcome( currentUnit, *enemy, valuesOfAttackPositions );

                if ( !outcome.canAttackImmediately ) {
                    continue;
                }

                if ( IsOutcomeImproved( outcome, bestOutcome ) ) {
                    bestOutcome = outcome;

                    target.cell = outcome.fromIndex;
                    target.unit = enemy;

                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE,
                               "- Set attack priority on " << enemy->GetName() << ", attack value: " << outcome.attackValue
                                                           << ", position value: " << outcome.positionValue )
                }
            }
        }

        if ( target.unit ) {
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " attacking " << target.unit->GetName() << " from cell " << target.cell )

            return target;
        }

        // 2. For units that don't have a target within reach, choose a target depending on distance-based priority
        {
            const Castle * castle = Arena::GetCastle();
            const bool isMoatBuilt = castle && castle->isBuild( BUILD_MOAT );

            // The usual distance between units of different armies at the beginning of a battle is 10-14 tiles. For each tile passed, 20% of the total army value will be
            // lost to make sure that the difference of 4 tiles matters.
            const double attackDistanceModifier = _enemyArmyStrength / 5.0;

            double maxPriority = std::numeric_limits<double>::lowest();

            for ( const Unit * enemy : enemies ) {
                assert( enemy != nullptr );

                const CellDistanceInfo nearestCellInfo = findNearestCellNextToUnit( arena, currentUnit, *enemy );
                if ( nearestCellInfo.idx == -1 ) {
                    continue;
                }

                // Do not pursue faster units that can move away and avoid an engagement
                const uint32_t dist = ( !enemy->isArchers() && isUnitFaster( *enemy, currentUnit ) ? nearestCellInfo.dist + ARENAW + ARENAH : nearestCellInfo.dist );
                const double unitPriority = enemy->evaluateThreatForUnit( currentUnit ) - dist * attackDistanceModifier;

                if ( unitPriority < maxPriority ) {
                    continue;
                }

                maxPriority = unitPriority;

                const Position pos = Position::GetPosition( currentUnit, nearestCellInfo.idx );
                assert( pos.isValidForUnit( currentUnit ) );

                const Indexes path = arena.GetPath( currentUnit, pos );
                assert( !path.empty() );

                // Unit rushes through the moat, step into the moat to get more freedom of action on the next turn
                if ( isMoatBuilt && Board::isMoatIndex( path.back(), currentUnit ) ) {
                    target.cell = path.back();

                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Going after target " << enemy->GetName() << ", stopping in the moat at cell " << target.cell )
                }
                else if ( _cautiousOffensive ) {
                    target.cell = findOptimalPositionForSubsequentAttack( arena, path, currentUnit, enemies );

                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Going after target " << enemy->GetName() << " using a cautious offensive, stopping at cell " << target.cell )
                }
                else {
                    target.cell = path.back();

                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Going after target " << enemy->GetName() << ", stopping at cell " << target.cell )
                }
            }
        }

        if ( target.cell != -1 ) {
            return target;
        }

        // 3. Try to get closer to the castle walls during the siege
        if ( _attackingCastle ) {
            uint32_t shortestDist = UINT32_MAX;

            for ( const int32_t cellIdx : cellsUnderWallsIndexes ) {
                const Position pos = Position::GetPosition( currentUnit, cellIdx );

                if ( !arena.isPositionReachable( currentUnit, pos, false ) ) {
                    continue;
                }

                assert( pos.isValidForUnit( currentUnit ) );

                const uint32_t moveDist = arena.CalculateMoveDistance( currentUnit, pos );
                if ( target.cell == -1 || moveDist < shortestDist ) {
                    shortestDist = moveDist;

                    target.cell = cellIdx;
                }
            }

            if ( target.cell != -1 ) {
                DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " moving towards castle walls, target cell: " << target.cell )
            }
        }

        return target;
    }

    BattleTargetPair BattlePlanner::meleeUnitDefense( Arena & arena, const Unit & currentUnit ) const
    {
        BattleTargetPair target;

        const PositionValues valuesOfAttackPositions = evaluatePotentialAttackPositions( arena, currentUnit );

        const Units friendly( arena.getForce( _myColor ).getUnits(), &currentUnit );
        // Current unit can be under the influence of the Hypnotize spell
        const Units enemies( arena.getEnemyForce( _myColor ).getUnits(), &currentUnit );

        // 1. Cover our archers and attack enemy units blocking them, if there are any. Units whose affiliation has been changed should not cover the archers, because
        // such units will block them instead of covering them.
        if ( currentUnit.GetArmyColor() == _myColor ) {
            const bool isAnyEnemyCanBeAttackedImmediately = std::any_of( enemies.begin(), enemies.end(), [&currentUnit, &valuesOfAttackPositions]( const Unit * enemy ) {
                assert( enemy != nullptr );

                const MeleeAttackOutcome outcome = BestAttackOutcome( currentUnit, *enemy, valuesOfAttackPositions );

                return outcome.canAttackImmediately;
            } );

            // If the army has only one stack of archers, then there is no question about which stack to cover. If the army has three or more stacks of archers, then,
            // most likely, melee units will be able to cover these stacks only partially. In the case of two stacks of archers, the question arises: in which cases we
            // should try to cover both stacks, but partially, and in which cases we should cover just one stack, but completely.
            //
            // In the current implementation, if the value of one of these archer stacks is 2 or more times greater than the value of the other, then it is worth using
            // all melee units to create a more reliable cover for more powerful stack of archers.
            //
            // In the case of two stacks of archers, AI will tend to initially place them on the edges of the battlefield. Thus, for melee units, the maximum distance to
            // the stack of archers will be approximately 5 tiles. To fulfill the above condition, a distance of 5 tiles should give a penalty of 1/3 of the total value
            // of the archer stacks.
            //
            // For example, suppose we have one stack of archers with the value of 200 on the tile with the index of 0, and another stack of archers with the value of 100
            // on the tile with the index of 88. In this case, a melee unit from the tile with the index of 66 should overcome about 5 tiles in order to reach the more
            // powerful stack of archers, but it should overcome just 1 tile to reach the less powerful stack. The penalty for each tile passed in its case will be
            // calculated as (200 + 100) / 15 = 20, and after the penalty is applied, the value of the more powerful stack of archers will be 200 - 20 * 5 = 100, and the
            // value of the less powerful stack will be 100 - 20 * 1 = 80, therefore, this melee unit will go to cover the more powerful stack of archers.
            //
            // However, if the strength of the archer stacks is comparable, for example, 150 vs 100, the penalty for each tile passed will be (150 + 100) / 15 = 16.66,
            // and after the penalty is applied, the value of the more powerful stack of archers will be 150 - 16.66 * 5 = 66,7, and the value of the less powerful stack
            // will be 100 - 16.66 * 1 = 83.34, therefore, this melee unit will go to cover the less powerful stack of archers, and both stacks will be only partially
            // covered.
            const double defenseDistanceModifier = _myRangedUnitsOnly / 15.0;

            double bestArcherValue = std::numeric_limits<double>::lowest();

            for ( const Unit * frnd : friendly ) {
                assert( frnd != nullptr );

                if ( !frnd->isArchers() ) {
                    continue;
                }

                const CellDistanceInfo bestCoverCellInfo = [&arena, &currentUnit, frnd]() -> CellDistanceInfo {
                    const Indexes nearbyIndexes = [&currentUnit, frnd]() {
                        Indexes result;
                        result.reserve( 8 );

                        const std::array<int, 6> priorityDirections = [&currentUnit, frnd]() -> std::array<int, 6> {
                            const bool preferToCoverFromTheSide = [&currentUnit, frnd]() {
                                // If the covering unit is not a wide unit, then using this unit to cover the shooter from the side does not give any advantage
                                if ( !currentUnit.isWide() ) {
                                    return false;
                                }

                                // It is always better to use wide units to cover wide shooters from the sides
                                if ( frnd->isWide() ) {
                                    return true;
                                }

                                assert( Board::isValidIndex( frnd->GetHeadIndex() ) );

                                // If an ordinary shooter is located on a tile that protrudes sideways, then using a wide unit to cover this shooter from the side does
                                // not give any advantage
                                if ( frnd->isReflect() ) {
                                    return ( ( frnd->GetHeadIndex() / ARENAW ) % 2 == 1 );
                                }

                                return ( ( frnd->GetHeadIndex() / ARENAW ) % 2 == 0 );
                            }();

                            if ( preferToCoverFromTheSide ) {
                                if ( frnd->isReflect() ) {
                                    return { TOP_LEFT, BOTTOM_LEFT, LEFT, TOP_RIGHT, BOTTOM_RIGHT, RIGHT };
                                }

                                return { TOP_RIGHT, BOTTOM_RIGHT, RIGHT, TOP_LEFT, BOTTOM_LEFT, LEFT };
                            }

                            if ( frnd->isReflect() ) {
                                return { LEFT, TOP_LEFT, BOTTOM_LEFT, TOP_RIGHT, BOTTOM_RIGHT, RIGHT };
                            }

                            return { RIGHT, TOP_RIGHT, BOTTOM_RIGHT, TOP_LEFT, BOTTOM_LEFT, LEFT };
                        }();

                        for ( const int32_t idx : std::array<int32_t, 2>{ frnd->GetHeadIndex(), frnd->GetTailIndex() } ) {
                            if ( !Board::isValidIndex( idx ) ) {
                                continue;
                            }

                            for ( const int dir : priorityDirections ) {
                                if ( !Board::isValidDirection( idx, dir ) ) {
                                    continue;
                                }

                                const int32_t nearbyIdx = Board::GetIndexDirection( idx, dir );

                                if ( std::find( result.begin(), result.end(), nearbyIdx ) != result.end() ) {
                                    continue;
                                }

                                result.push_back( nearbyIdx );
                            }
                        }

                        return result;
                    }();

                    for ( const int32_t idx : nearbyIndexes ) {
                        const Position pos = Position::GetPosition( currentUnit, idx );
                        if ( pos.GetHead() == nullptr ) {
                            continue;
                        }

                        assert( pos.isValidForUnit( currentUnit ) );
                        assert( Board::GetDistance( pos, frnd->GetPosition() ) == 1 );

                        if ( !arena.isPositionReachable( currentUnit, pos, false ) ) {
                            continue;
                        }

                        return { idx, arena.CalculateMoveDistance( currentUnit, pos ) };
                    }

                    return {};
                }();

                const Indexes adjacentEnemiesIndexes = Board::GetAdjacentEnemiesIndexes( *frnd );

                // If our archer is not blocked by enemy units, but the unit nevertheless cannot cover that archer, then ignore that archer
                if ( bestCoverCellInfo.idx == -1 && adjacentEnemiesIndexes.empty() ) {
                    continue;
                }

                // Either the unit can cover the friendly archer, or the archer is blocked by enemy units, which do not allow our unit to approach. As the distance to
                // estimate the archer's value, we take the smallest of the distance that must be overcome to cover the archer and the distance that must be overcome to
                // approach the nearest of the enemies blocking him.
                const auto [eitherFrndOrAdjEnemyIsReachable, dist] = [&arena, &currentUnit, &bestCoverCellInfo, &adjacentEnemiesIndexes]() {
                    std::pair<bool, uint32_t> result{ bestCoverCellInfo.idx != -1, bestCoverCellInfo.dist };

                    for ( const int idx : adjacentEnemiesIndexes ) {
                        const Unit * enemy = Board::GetCell( idx )->GetUnit();
                        assert( enemy != nullptr );

                        const CellDistanceInfo nearestToEnemyCellInfo = findNearestCellNextToUnit( arena, currentUnit, *enemy );
                        if ( nearestToEnemyCellInfo.idx == -1 ) {
                            continue;
                        }

                        if ( !result.first || nearestToEnemyCellInfo.dist < result.second ) {
                            result = { true, nearestToEnemyCellInfo.dist };
                        }
                    }

                    return result;
                }();

                // If the unit cannot cover the archer or approach any of the enemies blocking that archer, then ignore that archer
                if ( !eitherFrndOrAdjEnemyIsReachable ) {
                    continue;
                }

                // If the unit cannot cover the archer or approach any of the enemies blocking that archer within two turns (according to a rough estimate), but there is
                // at least one enemy unit that can be immediately attacked by this unit, then ignore that archer. Very slow units (such as Hydra) should not waste time
                // covering archers far from them - especially if there are other enemy units nearby worthy of their attention.
                if ( isAnyEnemyCanBeAttackedImmediately && !currentUnit.isFlying() && dist > currentUnit.GetSpeed() * 2 ) {
                    continue;
                }

                const double archerValue = frnd->GetStrength() - dist * defenseDistanceModifier;

                if ( archerValue < bestArcherValue ) {
                    continue;
                }

                bestArcherValue = archerValue;

                target.cell = bestCoverCellInfo.idx;
                target.unit = nullptr;

                // If the archer is blocked by enemy units, it is necessary to attack them immediately, or at least take the best position to attack
                {
                    MeleeAttackOutcome bestOutcome;

                    for ( const int idx : adjacentEnemiesIndexes ) {
                        const Unit * enemy = Board::GetCell( idx )->GetUnit();
                        assert( enemy != nullptr );

                        const MeleeAttackOutcome outcome = BestAttackOutcome( currentUnit, *enemy, valuesOfAttackPositions );

                        if ( IsOutcomeImproved( outcome, bestOutcome ) ) {
                            bestOutcome = outcome;

                            target.cell = outcome.fromIndex;
                            target.unit = outcome.canAttackImmediately ? enemy : nullptr;

                            DEBUG_LOG( DBG_BATTLE, DBG_TRACE,
                                       "- Set attack priority on " << enemy->GetName() << ", attack value: " << outcome.attackValue
                                                                   << ", position value: " << outcome.positionValue )
                        }
                    }
                }

                // If we have reached this point, then the unit should be able to either cover the archer or approach any of the enemies blocking that archer - although
                // perhaps without performing an immediate attack
                assert( Board::isValidIndex( target.cell ) );

                // The unit is going to attack one of the enemy units that blocked the archer, nothing else needs to be done
                if ( target.unit != nullptr ) {
                    continue;
                }

                // It makes sense for a unit that ignores retaliation to attack neighboring enemy units, even if it is covering an archer, since in this case it will not
                // receive unnecessary retaliatory damage (which could affect the duration of the cover). Also, archers with the ability to shoot at area may not always
                // attack enemy units in close proximity to friendly units due to fear of friendly fire, so covering friendly units should help by attacking enemy units
                // next to them.
                if ( !currentUnit.isIgnoringRetaliation() && !frnd->isAbilityPresent( fheroes2::MonsterAbilityType::AREA_SHOT ) ) {
                    continue;
                }

                // If the decision is made to attack one of the neighboring enemy units (if any) while covering the archer, then we should choose the best target
                {
                    int32_t bestAttackValue = 0;

                    for ( const Unit * enemy : enemies ) {
                        assert( enemy != nullptr );

                        if ( !Board::CanAttackTargetFromPosition( currentUnit, *enemy, target.cell ) ) {
                            continue;
                        }

                        const Position pos = Position::GetReachable( currentUnit, target.cell );
                        assert( pos.isValidForUnit( currentUnit ) );

                        const int32_t attackValue = optimalAttackValue( currentUnit, *enemy, pos );
                        if ( bestAttackValue < attackValue ) {
                            bestAttackValue = attackValue;

                            target.unit = enemy;

                            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Set attack priority on " << enemy->GetName() << ", attack value: " << attackValue )
                        }
                    }
                }
            }
        }

        if ( target.cell != -1 ) {
            if ( target.unit ) {
                DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " attacking " << target.unit->GetName() << " from cell " << target.cell )
            }
            else {
                DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " covering friendly archers, moving to cell " << target.cell )
            }

            return target;
        }

        // 2. Otherwise, try to find a suitable target that can be attacked from our half of the battlefield
        {
            MeleeAttackOutcome bestOutcome;

            for ( const Unit * enemy : enemies ) {
                assert( enemy != nullptr );

                const MeleeAttackOutcome outcome = BestAttackOutcome( currentUnit, *enemy, valuesOfAttackPositions, [this, &currentUnit]( const Position & pos ) {
                    return isPositionLocatedInDefendedArea( currentUnit, pos );
                } );

                if ( !Board::isValidIndex( outcome.fromIndex ) ) {
                    continue;
                }

                if ( IsOutcomeImproved( outcome, bestOutcome ) ) {
                    bestOutcome = outcome;

                    target.cell = outcome.fromIndex;
                    target.unit = outcome.canAttackImmediately ? enemy : nullptr;

                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE,
                               "- Set attack priority on " << enemy->GetName() << ", attack value: " << outcome.attackValue
                                                           << ", position value: " << outcome.positionValue )
                }
            }

            if ( target.unit ) {
                DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " attacking " << target.unit->GetName() << " from cell " << target.cell )
            }
        }

        return target;
    }

    Actions BattlePlanner::berserkTurn( Arena & arena, const Unit & currentUnit )
    {
        assert( currentUnit.Modes( SP_BERSERKER ) );

        Actions actions;

        Board * board = Arena::GetBoard();
        assert( board != nullptr );

        const uint32_t currentUnitUID = currentUnit.GetUID();

        const std::vector<Unit *> nearestUnits = board->GetNearestTroops( &currentUnit, {} );
        assert( !nearestUnits.empty() );

        // If the berserker is an archer, then just shoot at the nearest unit
        if ( currentUnit.isArchers() && !currentUnit.isHandFighting() ) {
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " under Berserk spell, will shoot" )

            const Unit * targetUnit = nearestUnits.front();
            assert( targetUnit != nullptr );

            actions.emplace_back( Command::ATTACK, currentUnitUID, targetUnit->GetUID(), -1, -1, 0 );

            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " shooting at enemy " << targetUnit->GetName() )

            return actions;
        }

        BattleTargetPair targetInfo;

        // First, try to find a unit nearby that can be attacked on this turn
        for ( const Unit * nearbyUnit : nearestUnits ) {
            assert( nearbyUnit != nullptr );

            const CellDistanceInfo nearestCellInfo = findNearestCellNextToUnit( arena, currentUnit, *nearbyUnit );
            if ( nearestCellInfo.idx == -1 ) {
                continue;
            }

            if ( !Board::CanAttackTargetFromPosition( currentUnit, *nearbyUnit, nearestCellInfo.idx ) ) {
                continue;
            }

            targetInfo.cell = nearestCellInfo.idx;
            targetInfo.unit = nearbyUnit;

            break;
        }

        // If there is no unit to attack during this turn, then find the nearest one to try to attack it during subsequent turns
        if ( targetInfo.cell == -1 ) {
            for ( const Unit * nearbyUnit : nearestUnits ) {
                assert( nearbyUnit != nullptr );

                const CellDistanceInfo nearestCellInfo = findNearestCellNextToUnit( arena, currentUnit, *nearbyUnit );
                if ( nearestCellInfo.idx == -1 ) {
                    continue;
                }

                targetInfo.cell = nearestCellInfo.idx;

                break;
            }
        }

        // There is no reachable unit in sight, skip the turn
        if ( targetInfo.cell == -1 ) {
            actions.emplace_back( Command::SKIP, currentUnitUID );

            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " skipping the turn" )

            return actions;
        }

        DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " under Berserk spell, target cell: " << targetInfo.cell )

        // The target cell of the movement must be the cell that the unit's head will occupy
        const int32_t moveTargetIdx = getUnitMovementTarget( arena, currentUnit, targetInfo.cell );

        if ( targetInfo.unit ) {
            const Unit * targetUnit = targetInfo.unit;

            actions.emplace_back( Command::ATTACK, currentUnitUID, targetUnit->GetUID(), ( currentUnit.GetHeadIndex() == moveTargetIdx ? -1 : moveTargetIdx ), -1, -1 );

            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " attacking enemy " << targetUnit->GetName() << " from cell " << moveTargetIdx )
        }
        else {
            actions.emplace_back( Command::MOVE, currentUnitUID, moveTargetIdx );

            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " moving to cell " << moveTargetIdx )
        }

        return actions;
    }

    bool BattlePlanner::isPositionLocatedInDefendedArea( const Unit & currentUnit, const Position & pos ) const
    {
        assert( pos.isReflect() == currentUnit.GetPosition().isReflect() && pos.GetHead() != nullptr );

        // Units whose affiliation has been changed are still looking in the direction they originally looked
        const bool reflect = ( currentUnit.GetArmyColor() == _myColor ? currentUnit.isReflect() : !currentUnit.isReflect() );

        const auto checkIdx = [this, reflect]( const int32_t idx ) {
            if ( _defendingCastle ) {
                return Board::isCastleIndex( idx );
            }

            return ( Board::GetDistanceFromBoardEdgeAlongXAxis( idx, reflect ) <= ARENAW / 2 );
        };

        if ( !checkIdx( pos.GetHead()->GetIndex() ) ) {
            return false;
        }

        if ( pos.GetTail() && !checkIdx( pos.GetTail()->GetIndex() ) ) {
            return false;
        }

        return true;
    }

    void Normal::BattleTurn( Arena & arena, const Unit & currentUnit, Actions & actions )
    {
        // Return immediately if our limit of turns has been exceeded
        if ( _battlePlanner.isLimitOfTurnsExceeded( arena, actions ) ) {
            return;
        }

        const Actions & plannedActions = _battlePlanner.planUnitTurn( arena, currentUnit );
        actions.insert( actions.end(), plannedActions.begin(), plannedActions.end() );
    }

    void Normal::battleBegins()
    {
        _battlePlanner.battleBegins();
    }
}
