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

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <numeric>
#include <ostream>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "ai.h"
#include "ai_normal.h"
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
#include "logging.h"
#include "monster_info.h"
#include "rand.h"
#include "settings.h"
#include "speed.h"

using namespace Battle;

namespace AI
{
    // Usual distance between units at the start of the battle is 10-14 tiles
    // 20% of maximum value lost for every tile travelled to make sure 4 tiles difference matters
    const double STRENGTH_DISTANCE_FACTOR = 5.0;
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
        // Primary - Enemy is within move range and can be attacked this turn
        // Secondary - Position quality (to attack from, or protect friendly unit)
        // Tertiary - Enemy unit threat
        return ( newOutcome.canAttackImmediately && !previous.canAttackImmediately )
               || ( newOutcome.canAttackImmediately == previous.canAttackImmediately
                    && ValueHasImproved( newOutcome.positionValue, previous.positionValue, newOutcome.attackValue, previous.attackValue ) );
    }

    MeleeAttackOutcome BestAttackOutcome( Arena & arena, const Unit & attacker, const Unit & defender, const Rand::DeterministicRandomGenerator & randomGenerator )
    {
        MeleeAttackOutcome bestOutcome;

        Indexes nearbyIndexes = Board::GetAroundIndexes( defender );
        // Shuffle to make equal quality moves a bit unpredictable
        randomGenerator.Shuffle( nearbyIndexes );

        // Check if we can reach the target and pick best position to attack from
        for ( const int32_t nearbyIdx : nearbyIndexes ) {
            const Position pos = Position::GetPosition( attacker, nearbyIdx );

            if ( !arena.isPositionReachable( attacker, pos, false ) ) {
                continue;
            }

            assert( pos.GetHead() != nullptr && ( !attacker.isWide() || pos.GetTail() != nullptr ) );

            MeleeAttackOutcome current;
            current.positionValue = Board::GetCell( nearbyIdx )->GetQuality();
            current.attackValue = Board::OptimalAttackValue( attacker, defender, nearbyIdx );
            current.canAttackImmediately = Board::CanAttackTargetFromPosition( attacker, defender, nearbyIdx );

            // Pick target if either position has improved or unit is higher value at the same position quality
            if ( IsOutcomeImproved( current, bestOutcome ) ) {
                bestOutcome.attackValue = current.attackValue;
                bestOutcome.positionValue = current.positionValue;
                bestOutcome.fromIndex = nearbyIdx;
                bestOutcome.canAttackImmediately = current.canAttackImmediately;
            }
        }
        return bestOutcome;
    }

    int32_t findOptimalPositionForSubsequentAttack( const Indexes & path, const Unit & currentUnit, const Battle::Units & enemies )
    {
        double lowestThreat = 0.0;
        int32_t targetIdx = -1;

        for ( const int stepIdx : path ) {
            const Position pos = Position::GetReachable( currentUnit, stepIdx );
            // If this step of the path is not reachable, then the following steps are also not reachable, there is no point in further checks
            if ( pos.GetHead() == nullptr ) {
                break;
            }

            double posThreatLevel = 0.0;

            for ( const Unit * enemy : enemies ) {
                assert( enemy != nullptr );

                // Archers and Flyers are always threatening, skip
                if ( enemy->isFlying() || ( enemy->isArchers() && !enemy->isHandFighting() ) ) {
                    continue;
                }

                // Also consider the next turn, even if the enemy unit has already acted during the current turn
                const uint32_t enemyAttackRange = enemy->GetSpeed( false, true ) + 1;

                if ( Board::GetDistance( pos, enemy->GetPosition() ) <= enemyAttackRange ) {
                    posThreatLevel += enemy->GetScoreQuality( currentUnit );
                }
            }

            // We need to get as close to the target as possible (taking into account the threat level)
            if ( targetIdx == -1 || posThreatLevel < lowestThreat || std::fabs( posThreatLevel - lowestThreat ) < 0.001 ) {
                lowestThreat = posThreatLevel;
                targetIdx = stepIdx;
            }
        }

        return targetIdx;
    }

    std::pair<int32_t, uint32_t> findNearestCellNextToUnit( Arena & arena, const Unit & currentUnit, const Unit & target )
    {
        std::pair<int32_t, uint32_t> result = { -1, UINT32_MAX };

        for ( const int32_t nearbyIdx : Board::GetAroundIndexes( target ) ) {
            const Position pos = Position::GetPosition( currentUnit, nearbyIdx );

            if ( !arena.isPositionReachable( currentUnit, pos, false ) ) {
                continue;
            }

            assert( pos.GetHead() != nullptr && ( !currentUnit.isWide() || pos.GetTail() != nullptr ) );

            const uint32_t dist = arena.CalculateMoveDistance( currentUnit, pos );
            if ( result.first == -1 || dist < result.second ) {
                result = { nearbyIdx, dist };
            }
        }

        return result;
    }

    int32_t getUnitMovementTarget( const Unit & currentUnit, const int32_t idx )
    {
        // First try to find the position that is reachable on the current turn
        Position pos = Position::GetReachable( currentUnit, idx );

        // If there is no such position, then use an abstract position corresponding to the specified index
        if ( pos.GetHead() == nullptr ) {
            pos = Position::GetPosition( currentUnit, idx );
        }

        assert( pos.GetHead() != nullptr && ( !currentUnit.isWide() || pos.GetTail() != nullptr ) );

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
        return _myArmyStrength * Difficulty::GetAIRetreatRatio( Game::getDifficulty() ) < _enemyArmyStrength;
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

                actions.emplace_back( CommandType::MSG_BATTLE_AUTO_SWITCH, currentColor );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO, Color::String( currentColor ) << " has used up the limit of turns without deaths, auto battle is turned off" )
            }
            // Otherwise the attacker's hero should retreat
            else {
                assert( arena.CanRetreatOpponent( currentColor ) && arena.GetCurrentCommander() != nullptr );

                actions.emplace_back( CommandType::MSG_BATTLE_RETREAT );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO,
                           Color::String( currentColor ) << " has used up the limit of turns without deaths, " << arena.GetCurrentCommander()->GetName() << " retreats" )
            }

            return true;
        }

        return false;
    }

    Actions BattlePlanner::planUnitTurn( Arena & arena, const Unit & currentUnit )
    {
        if ( currentUnit.Modes( SP_BERSERKER ) != 0 ) {
            return berserkTurn( arena, currentUnit );
        }

        Actions actions;

        // Step 1. Analyze current battle state and update variables
        analyzeBattleState( arena, currentUnit );

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, currentUnit.GetName() << " start their turn. Side: " << _myColor )

        // Step 2. Check retreat/surrender condition
        const Heroes * actualHero = dynamic_cast<const Heroes *>( _commander );
        if ( actualHero && arena.CanRetreatOpponent( _myColor ) && checkRetreatCondition( *actualHero ) ) {
            if ( isCommanderCanSpellcast( arena, _commander ) ) {
                // Cast maximum damage spell
                const SpellSelection & bestSpell = selectBestSpell( arena, currentUnit, true );

                if ( bestSpell.spellID != -1 ) {
                    actions.emplace_back( CommandType::MSG_BATTLE_CAST, bestSpell.spellID, bestSpell.cell );
                }
            }

            actions.emplace_back( CommandType::MSG_BATTLE_RETREAT );
            return actions;
        }

        // Step 3. Calculate spell heuristics
        if ( isCommanderCanSpellcast( arena, _commander ) ) {
            const SpellSelection & bestSpell = selectBestSpell( arena, currentUnit, false );

            if ( bestSpell.spellID != -1 ) {
                actions.emplace_back( CommandType::MSG_BATTLE_CAST, bestSpell.spellID, bestSpell.cell );
                return actions;
            }
        }

        // Step 4. Current unit decision tree
        const size_t actionsSize = actions.size();
        Arena::GetBoard()->SetPositionQuality( currentUnit );

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
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " melee phase end, target cell is " << target.cell )

            if ( target.cell != -1 ) {
                // The target cell of the movement must be the cell that the unit's head will occupy
                const int32_t moveTargetIdx = getUnitMovementTarget( currentUnit, target.cell );

                if ( currentUnit.GetHeadIndex() != moveTargetIdx ) {
                    actions.emplace_back( CommandType::MSG_BATTLE_MOVE, currentUnit.GetUID(), moveTargetIdx );

                    DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " moving to cell " << moveTargetIdx )
                }

                if ( target.unit ) {
                    const int32_t optimalTargetIdx = Board::OptimalAttackTarget( currentUnit, *target.unit, target.cell );

                    actions.emplace_back( CommandType::MSG_BATTLE_ATTACK, currentUnit.GetUID(), target.unit->GetUID(), optimalTargetIdx,
                                          Board::GetDirection( target.cell, optimalTargetIdx ) );

                    DEBUG_LOG( DBG_BATTLE, DBG_INFO,
                               currentUnit.GetName() << " melee offense, focus enemy " << target.unit->GetName()
                                                     << " threat level: " << target.unit->GetScoreQuality( currentUnit ) )
                }
            }
            // Else skip the turn
        }

        // No action was taken, skip the turn
        if ( actions.size() == actionsSize ) {
            actions.emplace_back( CommandType::MSG_BATTLE_SKIP, currentUnit.GetUID() );
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
        _myShooterStr = 0;
        _enemyShooterStr = 0;
        _enemyRangedUnitsOnly = 0;
        _enemyAverageSpeed = 0;
        _enemySpellStrength = 0;
        _considerRetreat = false;
        _randomGenerator = &arena.GetRandomGenerator();
        assert( _randomGenerator );
        // TODO : this pointer will dangle as we don't set it to nullptr when the Battle instance is deleted

        if ( enemyForce.empty() )
            return;

        double sumEnemyStr = 0.0;
        for ( const Unit * unitPtr : enemyForce ) {
            if ( !unitPtr || !unitPtr->isValid() )
                continue;

            const Unit & unit = *unitPtr;
            const double unitStr = unit.GetStrength();

            _enemyArmyStrength += unitStr;
            if ( unit.isArchers() && !unit.isImmovable() ) {
                _enemyRangedUnitsOnly += unitStr;
            }

            // average speed is weighted by troop strength
            const uint32_t speed = unit.GetSpeed( false, true );
            _enemyAverageSpeed += speed * unitStr;
            sumEnemyStr += unitStr;
        }
        _enemyShooterStr += _enemyRangedUnitsOnly;

        if ( sumEnemyStr > 0.0 ) {
            _enemyAverageSpeed /= sumEnemyStr;
        }

        uint32_t initialUnitCount = 0;
        double sumArmyStr = 0.0;
        for ( const Unit * unitPtr : friendlyForce ) {
            // Do not check isValid() here to handle dead troops
            if ( !unitPtr )
                continue;

            const Unit & unit = *unitPtr;
            const uint32_t count = unit.GetCount();
            const uint32_t dead = unit.GetDead();

            // Count all valid troops in army (both alive and dead)
            if ( count > 0 || dead > 0 ) {
                ++initialUnitCount;
            }

            const double unitStr = unit.GetStrength();

            // average speed is weighted by troop strength
            const uint32_t speed = unit.GetSpeed( false, true );
            _myArmyAverageSpeed += speed * unitStr;
            sumArmyStr += unitStr;

            // Dead unit: trigger retreat condition and skip strength calculation
            if ( count == 0 && dead > 0 ) {
                _considerRetreat = true;
                continue;
            }
            _myArmyStrength += unitStr;
            if ( unit.isArchers() && !unit.isImmovable() ) {
                _myShooterStr += unitStr;
            }
        }
        if ( sumArmyStr > 0.0 ) {
            _myArmyAverageSpeed /= sumArmyStr;
        }
        _considerRetreat = _considerRetreat || initialUnitCount < 4;

        // Add castle siege (and battle arena) modifiers
        _attackingCastle = false;
        _defendingCastle = false;
        const Castle * castle = Arena::GetCastle();
        // Mark as castle siege only if any tower is present. If no towers present then nothing to defend and most likely all walls are destroyed as well.
        if ( castle != nullptr && Arena::isAnyTowerPresent() ) {
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
                _myShooterStr += towerStr;
                if ( !attackerIgnoresCover )
                    _enemyShooterStr /= 1.5;
            }
            else {
                _attackingCastle = true;
                _enemyShooterStr += towerStr;
                if ( !attackerIgnoresCover )
                    _myShooterStr /= 1.5;
            }
        }

        // Calculate each hero spell strength and add it to shooter values after castle modifiers were applied
        if ( _commander && _myShooterStr > 1 ) {
            _myShooterStr += BattlePlanner::commanderMaximumSpellDamageValue( *_commander );
        }
        const HeroBase * enemyCommander = arena.getEnemyCommander( _myColor );
        if ( enemyCommander ) {
            _enemySpellStrength = enemyCommander->GetMagicStrategicValue( _myArmyStrength );
            _enemyShooterStr += BattlePlanner::commanderMaximumSpellDamageValue( *enemyCommander );
        }

        double overPowerRatio = 10; // for melee creatures
        if ( currentUnit.isFlying() ) {
            overPowerRatio = 6;
        }

        // When we have in X times stronger army than the enemy we could consider it as an overpowered and we most likely will win.
        const bool myOverpoweredArmy = _myArmyStrength > _enemyArmyStrength * overPowerRatio;
        const double enemyArcherRatio = _enemyShooterStr / _enemyArmyStrength;

        const double enemyArcherThreshold = 0.66;
        _defensiveTactics = _myShooterStr > _enemyShooterStr && ( _defendingCastle || enemyArcherRatio < enemyArcherThreshold ) && !myOverpoweredArmy;

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE,
                   "Tactic " << _defensiveTactics << " chosen. Archers: " << _myShooterStr << ", vs enemy " << _enemyShooterStr << " ratio is " << enemyArcherRatio )
    }

    Actions BattlePlanner::archerDecision( Arena & arena, const Unit & currentUnit ) const
    {
        Actions actions;

        // Current unit can be under the influence of the Hypnotize spell
        const Units enemies( arena.getEnemyForce( _myColor ).getUnits(), &currentUnit );

        // Assess the current threat level and decide whether to retreat to another position or attack a
        // specific unit in order to increase the field for maneuver in the future
        const BattleTargetPair immediateDangerAssessmentResult = [&arena, &currentUnit, &enemies]() -> BattleTargetPair {
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

                        assert( headIdx != -1 && ( !unitToRemove.isWide() || tailIdx != -1 ) );

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

                        assert( headIdx != -1 && ( !unitToRestore->isWide() || tailIdx != -1 ) );

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

                    const auto isPositionReachableForEnemy = [enemy]( const Position & position ) {
                        const uint32_t enemySpeed = enemy->GetSpeed( false, true );

                        // Blinded or paralyzed unit is considered harmless
                        if ( enemySpeed == Speed::STANDING ) {
                            return false;
                        }

                        // The potential event of enemy's good morale is not taken into account here
                        for ( const int32_t idx : Board::GetAroundIndexes( position ) ) {
                            const Position enemyPos = Position::GetReachable( *enemy, idx, enemySpeed );
                            if ( enemyPos.GetHead() == nullptr ) {
                                continue;
                            }

                            assert( !enemy->isWide() || enemyPos.GetTail() != nullptr );

                            return true;
                        }

                        return false;
                    };

                    for ( auto & [position, characteristics] : potentialPositions ) {
                        assert( position.GetHead() != nullptr );

                        if ( isPositionReachableForEnemy( position ) ) {
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
                    return {};
                }

                // If the current position is in danger, then we need to see if we can completely destroy some threatening
                // enemy stack to increase the field for maneuver in the future
                const Unit * priorityTarget = nullptr;
                const bool isCurrentUnitHandFighting = currentUnit.isHandFighting();

                for ( const int32_t enemyIdx : characteristics.threateningEnemiesIndexes ) {
                    const Unit * enemy = arena.GetTroopBoard( enemyIdx );
                    assert( enemy != nullptr );

                    // If archers are fighting in melee, then we cannot consider distant enemy stacks as potential targets
                    if ( isCurrentUnitHandFighting ) {
                        if ( !Unit::isHandFighting( currentUnit, *enemy ) ) {
                            continue;
                        }
                    }
                    else {
                        // If archers are not fighting in melee, then there should be no enemy units near them
                        assert( !Unit::isHandFighting( currentUnit, *enemy ) );
                    }

                    // The event of bad luck is deliberately not taken into account here, so that it does not look like
                    // cheating, because a human player in a similar situation does not know about this event in advance
                    const uint32_t guaranteedDamage = currentUnit.Modes( SP_BLESS ) ? currentUnit.CalculateMaxDamage( *enemy ) : currentUnit.CalculateMinDamage( *enemy );
                    const uint32_t guaranteedKills = enemy->HowManyWillBeKilled( guaranteedDamage );

                    assert( guaranteedKills <= enemy->GetCount() );

                    if ( guaranteedKills != enemy->GetCount() ) {
                        continue;
                    }

                    // If we can completely destroy multiple enemy stacks, then we need to choose the one that is able to inflict maximum damage on us
                    if ( priorityTarget != nullptr && enemy->CalculateMaxDamage( currentUnit ) < priorityTarget->CalculateMaxDamage( currentUnit ) ) {
                        continue;
                    }

                    priorityTarget = enemy;
                }

                // If we find such an enemy stack, then we need to see if we will suffer any potential losses after the attack of the enemy stacks
                // threatening us (without taking into account this enemy stack, because it should already be dead at the time of the attack)
                if ( priorityTarget ) {
                    const uint32_t potentialEnemyDamage
                        = std::accumulate( characteristics.threateningEnemiesIndexes.begin(), characteristics.threateningEnemiesIndexes.end(), static_cast<uint32_t>( 0 ),
                                           [&arena, &currentUnit, priorityTarget]( const uint32_t total, const int32_t enemyIdx ) {
                                               const Unit * enemy = arena.GetTroopBoard( enemyIdx );
                                               assert( enemy != nullptr );

                                               // The potential event of enemy's good luck is not taken into account here
                                               return enemy == priorityTarget ? total : total + enemy->CalculateMaxDamage( currentUnit );
                                           } );

                    // If we don't suffer any losses, then instead of retreating, we designate this enemy stack as a priority target
                    if ( currentUnit.HowManyWillBeKilled( potentialEnemyDamage ) == 0 ) {
                        return { -1, priorityTarget };
                    }
                }
            }

            // The current position is in danger, and there is no priority target, let's try to find a position to retreat
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

            return { safestIdx, nullptr };
        }();

        // The current position of the archers is not safe, but there is somewhere to retreat
        if ( immediateDangerAssessmentResult.cell != -1 ) {
            const int32_t retreatIdx = immediateDangerAssessmentResult.cell;

            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " archer retreats from enemy, target cell is " << retreatIdx )

            // The target cell of the movement must be the cell that the unit's head will occupy
            const int32_t moveTargetIdx = getUnitMovementTarget( currentUnit, retreatIdx );

            if ( currentUnit.GetHeadIndex() != moveTargetIdx ) {
                actions.emplace_back( CommandType::MSG_BATTLE_MOVE, currentUnit.GetUID(), moveTargetIdx );

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

            int bestOutcome = INT_MIN;

            const auto evaluateEnemyTarget = [&currentUnit, &target, &bestOutcome]( const Unit * enemy ) {
                assert( enemy != nullptr );

                const int archerMeleeDmg = currentUnit.GetDamage( *enemy );
                const int damageDiff = archerMeleeDmg - enemy->EstimateRetaliatoryDamage( archerMeleeDmg );

                if ( bestOutcome < damageDiff ) {
                    bestOutcome = damageDiff;

                    target.unit = enemy;

                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Set melee attack priority on " << enemy->GetName() << " value " << damageDiff )
                }
            };

            // There is a priority target, attack it
            if ( immediateDangerAssessmentResult.unit != nullptr ) {
                evaluateEnemyTarget( immediateDangerAssessmentResult.unit );
            }

            // Either there is no priority target, or the priority target is not suitable according to the results of its evaluation,
            // choose the most suitable target in the usual way
            if ( target.unit == nullptr ) {
                for ( const int cellIdx : Board::GetAdjacentEnemies( currentUnit ) ) {
                    evaluateEnemyTarget( Board::GetCell( cellIdx )->GetUnit() );
                }
            }

            if ( target.unit ) {
                actions.emplace_back( CommandType::MSG_BATTLE_ATTACK, currentUnit.GetUID(), target.unit->GetUID(), -1, -1 );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " archer attacks enemy " << target.unit->GetName() << " in melee, outcome: " << bestOutcome )
            }
        }
        // Archers are able to shoot
        else {
            BattleTargetPair target;

            double highestPriority = -1;

            const auto evaluateEnemyTarget = [&arena, &currentUnit, &target, &highestPriority]( const Unit * enemy ) {
                assert( enemy != nullptr );

                const auto updateBestTarget = [&target, &highestPriority, enemy]( const double priority, const int32_t targetIdx ) {
                    if ( highestPriority < priority ) {
                        highestPriority = priority;

                        target.cell = targetIdx;
                        target.unit = enemy;

                        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Set distance attack priority on " << enemy->GetName() << " value " << priority )
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

                            result += unit->GetScoreQuality( currentUnit );
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

                    return;
                }

                updateBestTarget( enemy->GetScoreQuality( currentUnit ), -1 );
            };

            // There is a priority target, attack it
            if ( immediateDangerAssessmentResult.unit != nullptr ) {
                evaluateEnemyTarget( immediateDangerAssessmentResult.unit );
            }

            // Either there is no priority target, or a shot at it does more harm than good, choose the most suitable target in the usual way
            if ( target.unit == nullptr ) {
                std::for_each( enemies.begin(), enemies.end(), evaluateEnemyTarget );
            }

            if ( target.unit ) {
                actions.emplace_back( CommandType::MSG_BATTLE_ATTACK, currentUnit.GetUID(), target.unit->GetUID(), target.cell, 0 );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " archer shoots at enemy " << target.unit->GetName() << " value: " << highestPriority )
            }
        }

        return actions;
    }

    BattleTargetPair BattlePlanner::meleeUnitOffense( Arena & arena, const Unit & currentUnit ) const
    {
        BattleTargetPair target;

        const Castle * castle = Arena::GetCastle();
        const bool isMoatBuilt = castle && castle->isBuild( BUILD_MOAT );
        // Current unit can be under the influence of the Hypnotize spell
        const Units enemies( arena.getEnemyForce( _myColor ).getUnits(), &currentUnit );

        double attackHighestValue = -_enemyArmyStrength;
        double attackPositionValue = -_enemyArmyStrength;

        for ( const Unit * enemy : enemies ) {
            const MeleeAttackOutcome & outcome = BestAttackOutcome( arena, currentUnit, *enemy, *_randomGenerator );

            if ( outcome.canAttackImmediately && ValueHasImproved( outcome.positionValue, attackPositionValue, outcome.attackValue, attackHighestValue ) ) {
                attackHighestValue = outcome.attackValue;
                attackPositionValue = outcome.positionValue;
                target.cell = outcome.fromIndex;
                target.unit = enemy;
            }
        }

        // For walking units that don't have a target within reach, pick based on distance priority
        if ( target.unit == nullptr ) {
            const double attackDistanceModifier = _enemyArmyStrength / STRENGTH_DISTANCE_FACTOR;
            double maxMovePriority = attackDistanceModifier * ARENASIZE * -1;

            for ( const Unit * enemy : enemies ) {
                // move node pair consists of move hex index and distance
                const std::pair<int, uint32_t> move = findNearestCellNextToUnit( arena, currentUnit, *enemy );

                // Skip unit if no path found
                if ( move.first == -1 ) {
                    continue;
                }

                // Do not chase faster units that can move away and avoid an engagement
                const uint32_t distance = ( !enemy->isArchers() && isUnitFaster( *enemy, currentUnit ) ? move.second + ARENAW + ARENAH : move.second );

                const double unitPriority = enemy->GetScoreQuality( currentUnit ) - distance * attackDistanceModifier;
                if ( unitPriority > maxMovePriority ) {
                    maxMovePriority = unitPriority;

                    const Position pos = Position::GetPosition( currentUnit, move.first );
                    assert( pos.GetHead() != nullptr && ( !currentUnit.isWide() || pos.GetTail() != nullptr ) );

                    const Indexes path = arena.GetPath( currentUnit, pos );

                    // Normally this shouldn't happen
                    if ( path.empty() ) {
                        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Arena::GetPath() returned an empty path to cell " << move.first << " for " << currentUnit.GetName() << "!" )
                    }
                    // Unit rushes through the moat, step into the moat to get more freedom of action on the next turn
                    else if ( isMoatBuilt && Board::isMoatIndex( path.back(), currentUnit ) ) {
                        target.cell = path.back();

                        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Going after target " << enemy->GetName() << " stopping in the moat at " << target.cell )
                    }
                    else {
                        target.cell = findOptimalPositionForSubsequentAttack( path, currentUnit, enemies );

                        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Going after target " << enemy->GetName() << " stopping at " << target.cell )
                    }
                }
            }
        }
        else {
            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, currentUnit.GetName() << " attacking " << target.unit->GetName() << " at " << target.cell )
        }

        // Walkers: move closer to the castle walls during siege
        if ( _attackingCastle && target.cell == -1 ) {
            uint32_t shortestDist = UINT32_MAX;

            for ( const int32_t cellIdx : cellsUnderWallsIndexes ) {
                const Position pos = Position::GetPosition( currentUnit, cellIdx );

                if ( !arena.isPositionReachable( currentUnit, pos, false ) ) {
                    continue;
                }

                assert( pos.GetHead() != nullptr && ( !currentUnit.isWide() || pos.GetTail() != nullptr ) );

                const uint32_t dist = arena.CalculateMoveDistance( currentUnit, pos );
                if ( target.cell == -1 || dist < shortestDist ) {
                    shortestDist = dist;
                    target.cell = cellIdx;
                }
            }

            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " moving towards castle walls, target cell is " << target.cell )
        }

        return target;
    }

    BattleTargetPair BattlePlanner::meleeUnitDefense( Arena & arena, const Unit & currentUnit ) const
    {
        BattleTargetPair target;

        const Units friendly( arena.getForce( _myColor ).getUnits(), &currentUnit );
        // Current unit can be under the influence of the Hypnotize spell
        const Units enemies( arena.getEnemyForce( _myColor ).getUnits(), &currentUnit );

        const int myHeadIndex = currentUnit.GetHeadIndex();

        const double defenceDistanceModifier = _myArmyStrength / STRENGTH_DISTANCE_FACTOR;

        const auto isDefensivePosition = [this, &currentUnit]( const int32_t index ) {
            return ( !_defendingCastle && Board::DistanceFromOriginX( index, currentUnit.isReflect() ) <= ARENAW / 2 )
                   || ( _defendingCastle && Board::isCastleIndex( index ) );
        };

        // 1. Check if there's a target within our half of the battlefield
        MeleeAttackOutcome attackOption;
        for ( const Unit * enemy : enemies ) {
            const MeleeAttackOutcome & outcome = BestAttackOutcome( arena, currentUnit, *enemy, *_randomGenerator );

            // Allow to move only within our half of the battlefield. If in castle make sure to stay inside.
            if ( !isDefensivePosition( outcome.fromIndex ) )
                continue;

            if ( IsOutcomeImproved( outcome, attackOption ) ) {
                attackOption.attackValue = outcome.attackValue;
                attackOption.positionValue = outcome.positionValue;
                attackOption.canAttackImmediately = outcome.canAttackImmediately;
                target.cell = outcome.fromIndex;
                target.unit = outcome.canAttackImmediately ? enemy : nullptr;
            }
        }

        // 2. Check if our archer units are under threat - overwrite target and protect
        MeleeAttackOutcome protectOption;
        for ( const Unit * unitToDefend : friendly ) {
            if ( !unitToDefend->isArchers() ) {
                continue;
            }

            const std::pair<int, uint32_t> move = findNearestCellNextToUnit( arena, currentUnit, *unitToDefend );
            const uint32_t distanceToUnit = ( move.first != -1 ) ? move.second : Board::GetDistance( myHeadIndex, unitToDefend->GetHeadIndex() );
            const double archerValue = unitToDefend->GetStrength() - distanceToUnit * defenceDistanceModifier;

            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, unitToDefend->GetName() << " archer value " << archerValue << " distance: " << distanceToUnit )

            // 3. Search for enemy units blocking our archers within range move
            const Indexes & adjacentEnemies = Board::GetAdjacentEnemies( *unitToDefend );
            for ( const int cell : adjacentEnemies ) {
                const Unit * enemy = Board::GetCell( cell )->GetUnit();
                if ( !enemy ) {
                    DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Board::GetAdjacentEnemies() returned a cell " << cell << " that does not contain a unit!" )

                    continue;
                }

                MeleeAttackOutcome outcome = BestAttackOutcome( arena, currentUnit, *enemy, *_randomGenerator );
                outcome.positionValue = archerValue;

                DEBUG_LOG( DBG_BATTLE, DBG_TRACE, " - Found enemy, cell " << cell << " threat " << outcome.attackValue )

                if ( IsOutcomeImproved( outcome, protectOption ) ) {
                    protectOption.attackValue = outcome.attackValue;
                    protectOption.positionValue = archerValue;
                    protectOption.canAttackImmediately = outcome.canAttackImmediately;
                    target.cell = outcome.fromIndex;
                    target.unit = outcome.canAttackImmediately ? enemy : nullptr;

                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, " - Target selected " << enemy->GetName() << " cell " << target.cell << " archer value " << archerValue )
                }
            }

            // 4. No enemies found anywhere - move in closer to the friendly ranged unit
            if ( !target.unit && protectOption.positionValue < archerValue ) {
                target.cell = move.first;
                protectOption.positionValue = archerValue;
            }
        }

        if ( target.unit ) {
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " defending against " << target.unit->GetName() << " threat level: " << protectOption.attackValue )
        }
        else if ( target.cell != -1 ) {
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " protecting friendly archer, moving to " << target.cell )
        }
        else if ( !isDefensivePosition( currentUnit.GetHeadIndex() ) ) {
            // When there's nothing to do on our half; we're likely dealing with enemy's archers
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " on the enemy half of the battlefield; switch to offense" )
            target = meleeUnitOffense( arena, currentUnit );
        }

        return target;
    }

    Actions BattlePlanner::berserkTurn( Arena & arena, const Unit & currentUnit )
    {
        assert( currentUnit.Modes( SP_BERSERKER ) );

        Actions actions;

        Board * board = Arena::GetBoard();
        assert( board != nullptr );

        const std::vector<Unit *> nearestUnits = board->GetNearestTroops( &currentUnit, {} );
        // Normally this shouldn't happen
        if ( nearestUnits.empty() ) {
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Board::GetNearestTroops() returned an empty result for " << currentUnit.GetName() << "!" )

            return actions;
        }

        const uint32_t currentUnitUID = currentUnit.GetUID();

        // If the berserker is an archer, then just shoot at the nearest unit
        if ( currentUnit.isArchers() && !currentUnit.isHandFighting() ) {
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " under Berserk spell, will shoot" )

            const Unit * targetUnit = nearestUnits.front();
            assert( targetUnit != nullptr );

            actions.emplace_back( CommandType::MSG_BATTLE_ATTACK, currentUnitUID, targetUnit->GetUID(), -1, 0 );

            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " archer shoots at enemy " << targetUnit->GetName() )

            return actions;
        }

        BattleTargetPair targetInfo;
        std::map<const Unit *, Indexes> aroundIndexesCache;

        // First, try to find a unit nearby that can be attacked on this turn
        for ( const Unit * nearbyUnit : nearestUnits ) {
            assert( nearbyUnit != nullptr );

            const auto cacheItemIter = aroundIndexesCache.try_emplace( nearbyUnit, Board::GetAroundIndexes( *nearbyUnit ) ).first;
            assert( cacheItemIter != aroundIndexesCache.end() );

            uint32_t shortestDist = UINT32_MAX;

            for ( const int32_t cellIdx : cacheItemIter->second ) {
                if ( !Board::CanAttackTargetFromPosition( currentUnit, *nearbyUnit, cellIdx ) ) {
                    continue;
                }

                const Position pos = Position::GetReachable( currentUnit, cellIdx );
                assert( pos.GetHead() != nullptr && ( !currentUnit.isWide() || pos.GetTail() != nullptr ) );

                const uint32_t dist = arena.CalculateMoveDistance( currentUnit, pos );
                if ( targetInfo.cell == -1 || dist < shortestDist ) {
                    shortestDist = dist;

                    targetInfo.cell = cellIdx;
                    targetInfo.unit = nearbyUnit;
                }
            }

            if ( targetInfo.cell != -1 ) {
                break;
            }
        }

        // If there is no unit to attack during this turn, then find the nearest one to try to attack it during subsequent turns
        if ( targetInfo.cell == -1 ) {
            for ( const Unit * nearbyUnit : nearestUnits ) {
                assert( nearbyUnit != nullptr );

                const auto cacheItemIter = aroundIndexesCache.find( nearbyUnit );
                assert( cacheItemIter != aroundIndexesCache.end() );

                uint32_t shortestDist = UINT32_MAX;

                for ( const int32_t cellIdx : cacheItemIter->second ) {
                    const Position pos = Position::GetPosition( currentUnit, cellIdx );

                    if ( !arena.isPositionReachable( currentUnit, pos, false ) ) {
                        continue;
                    }

                    assert( pos.GetHead() != nullptr && ( !currentUnit.isWide() || pos.GetTail() != nullptr ) );

                    const uint32_t dist = arena.CalculateMoveDistance( currentUnit, pos );
                    if ( targetInfo.cell == -1 || dist < shortestDist ) {
                        shortestDist = dist;

                        targetInfo.cell = cellIdx;
                    }
                }
            }
        }

        // There is no reachable unit in sight, skip the turn
        if ( targetInfo.cell == -1 ) {
            actions.emplace_back( CommandType::MSG_BATTLE_SKIP, currentUnitUID );

            return actions;
        }

        DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " under Berserk spell, target cell is " << targetInfo.cell )

        // The target cell of the movement must be the cell that the unit's head will occupy
        const int32_t moveTargetIdx = getUnitMovementTarget( currentUnit, targetInfo.cell );

        if ( currentUnit.GetHeadIndex() != moveTargetIdx ) {
            actions.emplace_back( CommandType::MSG_BATTLE_MOVE, currentUnitUID, moveTargetIdx );

            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " moving to cell " << moveTargetIdx )
        }

        if ( targetInfo.unit ) {
            const Unit * targetUnit = targetInfo.unit;

            actions.emplace_back( CommandType::MSG_BATTLE_ATTACK, currentUnitUID, targetUnit->GetUID(), -1, -1 );

            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " melee offense, focus enemy " << targetUnit->GetName() )
        }

        return actions;
    }

    void Normal::BattleTurn( Arena & arena, const Unit & currentUnit, Actions & actions )
    {
        // Return immediately if our limit of turns has been exceeded
        if ( _battlePlanner.isLimitOfTurnsExceeded( arena, actions ) ) {
            return;
        }

        const Actions & plannedActions = _battlePlanner.planUnitTurn( arena, currentUnit );
        actions.insert( actions.end(), plannedActions.begin(), plannedActions.end() );

        // Do not end the turn if we only cast a spell
        if ( plannedActions.size() != 1 || !plannedActions.front().isType( CommandType::MSG_BATTLE_CAST ) ) {
            actions.emplace_back( CommandType::MSG_BATTLE_END_TURN, currentUnit.GetUID() );
        }
    }

    void Normal::battleBegins()
    {
        _battlePlanner.battleBegins();
    }
}
