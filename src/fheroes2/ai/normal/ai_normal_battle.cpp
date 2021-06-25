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

#include "ai_normal.h"
#include "artifact.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "battle_cell.h"
#include "battle_command.h"
#include "battle_tower.h"
#include "battle_troop.h"
#include "castle.h"
#include "difficulty.h"
#include "game.h"
#include "heroes.h"
#include "logging.h"
#include "settings.h"
#include "speed.h"

#include <cassert>
#include <cmath>

using namespace Battle;

namespace AI
{
    // Usual distance between units at the start of the battle is 10-14 tiles
    // 20% of maximum value lost for every tile travelled to make sure 4 tiles difference matters
    const double STRENGTH_DISTANCE_FACTOR = 5.0;
    const std::vector<int> underWallsIndicies = { 7, 28, 49, 72, 95 };

    struct MeleeAttackOutcome
    {
        int32_t fromIndex = -1;
        double attackValue = -INT32_MAX;
        double positionValue = -INT32_MAX;
        bool canReach = false;
    };

    bool ValueHasImproved( double primary, double primaryMax, double secondary, double secondaryMax )
    {
        return primaryMax < primary || ( secondaryMax < secondary && std::fabs( primaryMax - primary ) < 0.001 );
    }

    bool IsOutcomeImproved( const MeleeAttackOutcome & newOutcome, const MeleeAttackOutcome & previous )
    {
        // Composite priority criteria:
        // Primary - Enemy is within move range and can be attacked this turn
        // Secondary - Postion quality (to attack from, or protect friendly unit)
        // Tertiary - Enemy unit threat
        return ( newOutcome.canReach && !previous.canReach )
               || ( newOutcome.canReach == previous.canReach
                    && ValueHasImproved( newOutcome.positionValue, previous.positionValue, newOutcome.attackValue, previous.attackValue ) );
    }

    MeleeAttackOutcome BestAttackOutcome( const Arena & arena, const Unit & attacker, const Unit & defender )
    {
        MeleeAttackOutcome bestOutcome;

        const uint32_t currentUnitMoveRange = attacker.GetMoveRange();

        Indexes around = Board::GetAroundIndexes( defender );
        // Shuffle to make equal quality moves a bit unpredictable
        Rand::Shuffle( around );

        for ( const int cell : around ) {
            // Check if we can reach the target and pick best position to attack from
            if ( !arena.hexIsPassable( cell ) )
                continue;

            MeleeAttackOutcome current;
            current.positionValue = Board::GetCell( cell )->GetQuality();
            current.attackValue = Board::OptimalAttackValue( attacker, defender, cell );
            current.canReach = arena.CalculateMoveDistance( cell ) <= currentUnitMoveRange;

            // Pick target if either position has improved or unit is higher value at the same position quality
            if ( IsOutcomeImproved( current, bestOutcome ) ) {
                bestOutcome.attackValue = current.attackValue;
                bestOutcome.positionValue = current.positionValue;
                bestOutcome.fromIndex = cell;
                bestOutcome.canReach = current.canReach;
            }
        }
        return bestOutcome;
    }

    int32_t FindMoveToRetreat( const Indexes & moves, const Unit & currentUnit, const Battle::Units & enemies )
    {
        double lowestThreat = 0.0;
        int32_t targetCell = -1;

        for ( const int moveIndex : moves ) {
            // Skip if this cell has adjacent enemies
            if ( !Board::GetCell( moveIndex )->GetQuality() )
                continue;

            double cellThreatLevel = 0.0;

            for ( const Unit * enemy : enemies ) {
                const uint32_t dist = Board::GetDistance( moveIndex, enemy->GetHeadIndex() );
                const uint32_t range = std::max( 1u, enemy->GetMoveRange() );
                cellThreatLevel += enemy->GetScoreQuality( currentUnit ) * ( 1.0 - static_cast<double>( dist ) / range );
            }

            if ( targetCell == -1 || cellThreatLevel < lowestThreat ) {
                lowestThreat = cellThreatLevel;
                targetCell = moveIndex;
            }
        }
        return targetCell;
    }

    int32_t FindNextTurnAttackMove( const Indexes & moves, const Unit & currentUnit, const Battle::Units & enemies )
    {
        double lowestThreat = 0.0;
        int32_t targetCell = -1;

        for ( const int moveIndex : moves ) {
            double cellThreatLevel = 0.0;

            for ( const Unit * enemy : enemies ) {
                // Archers and Flyers are always threatning, skip
                if ( enemy->isFlying() || ( enemy->isArchers() && !enemy->isHandFighting() ) )
                    continue;

                if ( Board::GetDistance( moveIndex, enemy->GetHeadIndex() ) <= enemy->GetMoveRange() + 1 ) {
                    cellThreatLevel += enemy->GetScoreQuality( currentUnit );
                }
            }

            // Also allow to move up closer if there's still no threat
            if ( targetCell == -1 || cellThreatLevel < lowestThreat || std::fabs( cellThreatLevel ) < 0.001 ) {
                lowestThreat = cellThreatLevel;
                targetCell = moveIndex;
            }
        }
        return targetCell;
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
        // Retreat if remaining army strength is a fraction of enemy's
        // Consider taking speed/turn order into account in the future
        const double ratio = Difficulty::GetAIRetreatRatio( Game::getDifficulty() );
        return _considerRetreat && _myArmyStrength * ratio < _enemyArmyStrength && !hero.isControlHuman() && isHeroWorthSaving( hero );
    }

    bool BattlePlanner::isUnitFaster( const Unit & currentUnit, const Unit & target ) const
    {
        if ( currentUnit.isFlying() == target.isFlying() )
            return currentUnit.GetSpeed() > target.GetSpeed();
        return currentUnit.isFlying();
    }

    Actions BattlePlanner::planUnitTurn( Arena & arena, const Unit & currentUnit )
    {
        if ( currentUnit.Modes( SP_BERSERKER ) != 0 ) {
            return berserkTurn( arena, currentUnit );
        }

        Actions actions;

        // Step 1. Analyze current battle state and update variables
        analyzeBattleState( arena, currentUnit );

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, currentUnit.GetName() << " start their turn. Side: " << _myColor );

        // Step 2. Check retreat/surrender condition
        const Heroes * actualHero = dynamic_cast<const Heroes *>( _commander );
        if ( actualHero && arena.CanRetreatOpponent( _myColor ) && checkRetreatCondition( *actualHero ) ) {
            if ( isCommanderCanSpellcast( arena, _commander ) ) {
                // Cast maximum damage spell
                const SpellSeletion & bestSpell = selectBestSpell( arena, true );

                if ( bestSpell.spellID != -1 ) {
                    actions.emplace_back( MSG_BATTLE_CAST, bestSpell.spellID, bestSpell.cell );
                }
            }

            actions.emplace_back( MSG_BATTLE_RETREAT );
            actions.emplace_back( MSG_BATTLE_END_TURN, currentUnit.GetUID() );
            return actions;
        }

        // Step 3. Calculate spell heuristics
        if ( isCommanderCanSpellcast( arena, _commander ) ) {
            const SpellSeletion & bestSpell = selectBestSpell( arena, false );

            if ( bestSpell.spellID != -1 ) {
                actions.emplace_back( MSG_BATTLE_CAST, bestSpell.spellID, bestSpell.cell );
                return actions;
            }
        }

        // Step 4. Current unit decision tree
        const size_t actionsSize = actions.size();
        Battle::Arena::GetBoard()->SetPositionQuality( currentUnit );

        if ( currentUnit.isArchers() ) {
            const Actions & archerActions = archerDecision( arena, currentUnit );
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
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, "Melee phase end, targetCell is " << target.cell );

            if ( target.cell != -1 ) {
                if ( currentUnit.GetHeadIndex() != target.cell )
                    actions.emplace_back( MSG_BATTLE_MOVE, currentUnit.GetUID(), target.cell );

                if ( target.unit ) {
                    actions.emplace_back( MSG_BATTLE_ATTACK, currentUnit.GetUID(), target.unit->GetUID(),
                                          Board::OptimalAttackTarget( currentUnit, *target.unit, target.cell ), 0 );
                    DEBUG_LOG( DBG_BATTLE, DBG_INFO,
                               currentUnit.GetName() << " melee offense, focus enemy " << target.unit->GetName()
                                                     << " threat level: " << target.unit->GetScoreQuality( currentUnit ) );
                }
            }
            // else skip
        }

        // no action was taken - skip
        if ( actions.size() == actionsSize ) {
            actions.emplace_back( MSG_BATTLE_SKIP, currentUnit.GetUID(), true );
        }

        return actions;
    }

    void BattlePlanner::analyzeBattleState( Arena & arena, const Unit & currentUnit )
    {
        _myColor = currentUnit.GetCurrentColor();
        _commander = arena.GetCommander( _myColor );

        const Force & friendlyForce = arena.GetForce( _myColor );
        const Force & enemyForce = arena.GetForce( _myColor, true );

        // Friendly and enemy army analysis
        _myArmyStrength = 0;
        _enemyArmyStrength = 0;
        _myShooterStr = 0;
        _enemyShooterStr = 0;
        _enemyAverageSpeed = 0;
        _enemySpellStrength = 0;
        _highestDamageExpected = 0;
        _considerRetreat = false;

        if ( enemyForce.empty() )
            return;

        uint32_t slowestUnitSpeed = Speed::INSTANT;
        for ( const Unit * unitPtr : enemyForce ) {
            if ( !unitPtr || !unitPtr->isValid() )
                continue;

            const Unit & unit = *unitPtr;
            const double unitStr = unit.GetStrength();

            _enemyArmyStrength += unitStr;
            if ( unit.isArchers() ) {
                _enemyShooterStr += unitStr;
            }

            const int dmg = unit.CalculateMaxDamage( currentUnit );
            if ( dmg > _highestDamageExpected )
                _highestDamageExpected = dmg;

            const uint32_t speed = unit.GetSpeed();
            _enemyAverageSpeed += speed;
            if ( speed < slowestUnitSpeed )
                slowestUnitSpeed = speed;
        }
        if ( enemyForce.size() > 2 ) {
            _enemyAverageSpeed -= slowestUnitSpeed;
        }
        _enemyAverageSpeed /= enemyForce.size();

        uint32_t initialUnitCount = 0;
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
            // Dead unit: trigger retreat condition and skip strength calculation
            if ( count == 0 && dead > 0 ) {
                _considerRetreat = true;
                continue;
            }

            const double unitStr = unit.GetStrength();
            _myArmyStrength += unitStr;
            if ( unit.isArchers() ) {
                _myShooterStr += unitStr;
            }
        }
        _considerRetreat = _considerRetreat || initialUnitCount < 4;

        // Add castle siege (and battle arena) modifiers
        _attackingCastle = false;
        _defendingCastle = false;
        const Castle * castle = Arena::GetCastle();
        if ( castle ) {
            const bool attackerIgnoresCover = arena.GetForce1().GetCommander()->HasArtifact( Artifact::GOLDEN_BOW ) > 0;

            auto getTowerStrength = [&currentUnit]( const Tower * tower ) { return ( tower && tower->isValid() ) ? tower->GetScoreQuality( currentUnit ) : 0; };

            double towerStr = getTowerStrength( Arena::GetTower( TWR_CENTER ) );
            towerStr += getTowerStrength( Arena::GetTower( TWR_LEFT ) );
            towerStr += getTowerStrength( Arena::GetTower( TWR_RIGHT ) );
            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Castle strength: " << towerStr );

            if ( _myColor == castle->GetColor() ) {
                _defendingCastle = true;
                _myShooterStr += towerStr;
                if ( !attackerIgnoresCover )
                    _enemyShooterStr /= 2;
            }
            else {
                _attackingCastle = true;
                _enemyShooterStr += towerStr;
                if ( !attackerIgnoresCover )
                    _myShooterStr /= 2;
            }
        }

        // TODO: replace this hacky code for archers
        // Calculate each hero spell strength and add it to shooter values after castle modifiers were applied
        if ( _commander && _myShooterStr > 1 ) {
            _myShooterStr += _commander->GetSpellcastStrength( _myArmyStrength );
        }
        const HeroBase * enemyCommander = arena.GetCommander( _myColor, true );
        if ( enemyCommander ) {
            _enemySpellStrength = enemyCommander->GetSpellcastStrength( _enemyArmyStrength );
            _enemyShooterStr += _enemySpellStrength;
        }

        // When we have in 10 times stronger army than the enemy we could consider it as an overpowered and we most likely will win.
        const bool myOverpoweredArmy = _myArmyStrength > _enemyArmyStrength * 10;
        const double enemyArcherRatio = _enemyShooterStr / _enemyArmyStrength;

        _defensiveTactics = enemyArcherRatio < 0.75 && ( _defendingCastle || _myShooterStr > _enemyShooterStr ) && !myOverpoweredArmy;
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE,
                   "Tactic " << _defensiveTactics << " chosen. Archers: " << _myShooterStr << ", vs enemy " << _enemyShooterStr << " ratio is " << enemyArcherRatio );
    }

    Actions BattlePlanner::archerDecision( Arena & arena, const Unit & currentUnit ) const
    {
        Actions actions;
        const Units enemies( arena.GetForce( _myColor, true ), true );
        BattleTargetPair target;

        if ( currentUnit.isHandFighting() ) {
            // Current ranged unit is blocked by the enemy

            // Force archer to fight back by setting initial expectation to lowest possible (if we're losing battle)
            int bestOutcome = ( _myArmyStrength < _enemyArmyStrength ) ? -_highestDamageExpected : 0;

            const Indexes & adjacentEnemies = Board::GetAdjacentEnemies( currentUnit );
            for ( const int cell : adjacentEnemies ) {
                const Unit * enemy = Board::GetCell( cell )->GetUnit();
                if ( enemy ) {
                    const int archerMeleeDmg = currentUnit.GetDamage( *enemy );
                    const int damageDiff = archerMeleeDmg - enemy->CalculateRetaliationDamage( archerMeleeDmg );

                    if ( bestOutcome < damageDiff ) {
                        bestOutcome = damageDiff;
                        target.unit = enemy;
                        target.cell = cell;
                    }
                }
                else {
                    DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Board::GetAdjacentEnemies returned a cell " << cell << " that does not contain a unit!" );
                }
            }

            if ( target.unit && target.cell != -1 ) {
                // Melee attack selected target
                DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " archer deciding to fight back: " << bestOutcome );
                actions.emplace_back( MSG_BATTLE_ATTACK, currentUnit.GetUID(), target.unit->GetUID(), target.cell, 0 );
            }
            else {
                // Kiting enemy: Search for a safe spot unit can move to
                target.cell = FindMoveToRetreat( arena.getAllAvailableMoves( currentUnit.GetMoveRange() ), currentUnit, enemies );

                if ( target.cell != -1 ) {
                    actions.emplace_back( MSG_BATTLE_MOVE, currentUnit.GetUID(), target.cell );
                    DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " archer kiting enemy, moving to " << target.cell );
                }
            }
            // Worst case scenario - Skip turn
        }
        else {
            // Normal ranged attack: focus the highest value unit
            double highestStrength = 0;

            for ( const Unit * enemy : enemies ) {
                const double attackPriority = enemy->GetScoreQuality( currentUnit );

                if ( highestStrength < attackPriority ) {
                    highestStrength = attackPriority;
                    target.unit = enemy;
                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Set priority on " << enemy->GetName() << " value " << attackPriority );
                }
            }

            if ( target.unit ) {
                actions.emplace_back( MSG_BATTLE_ATTACK, currentUnit.GetUID(), target.unit->GetUID(), target.unit->GetHeadIndex(), 0 );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO,
                           currentUnit.GetName() << " archer focusing enemy " << target.unit->GetName()
                                                 << " threat level: " << target.unit->GetScoreQuality( currentUnit ) );
            }
        }

        return actions;
    }

    BattleTargetPair BattlePlanner::meleeUnitOffense( Arena & arena, const Unit & currentUnit ) const
    {
        BattleTargetPair target;
        const Units enemies( arena.GetForce( _myColor, true ), true );

        double attackHighestValue = -_enemyArmyStrength;
        double attackPositionValue = -_enemyArmyStrength;

        for ( const Unit * enemy : enemies ) {
            const MeleeAttackOutcome & outcome = BestAttackOutcome( arena, currentUnit, *enemy );

            if ( outcome.canReach && ValueHasImproved( outcome.positionValue, attackPositionValue, outcome.attackValue, attackHighestValue ) ) {
                attackHighestValue = outcome.attackValue;
                attackPositionValue = outcome.positionValue;
                target.cell = outcome.fromIndex;
                target.unit = enemy;
            }
        }

        // For walking units that don't have a target within reach, pick based on distance priority
        if ( target.unit == nullptr ) {
            const uint32_t currentUnitMoveRange = currentUnit.GetMoveRange();
            const double attackDistanceModifier = _enemyArmyStrength / STRENGTH_DISTANCE_FACTOR;
            double maxMovePriority = attackDistanceModifier * ARENASIZE * -1;

            for ( const Unit * enemy : enemies ) {
                // move node pair consists of move hex index and distance
                const std::pair<int, uint32_t> move = arena.CalculateMoveToUnit( *enemy );

                if ( move.first == -1 ) // Skip unit if no path found
                    continue;

                // Do not chase after faster units that might kite away and avoid engagement
                const uint32_t distance = ( !enemy->isArchers() && isUnitFaster( *enemy, currentUnit ) ) ? move.second + ARENAW + ARENAH : move.second;

                const double unitPriority = enemy->GetScoreQuality( currentUnit ) - distance * attackDistanceModifier;
                if ( unitPriority > maxMovePriority ) {
                    maxMovePriority = unitPriority;

                    const Indexes & path = arena.CalculateTwoMoveOverlap( move.first, currentUnitMoveRange );
                    if ( !path.empty() ) {
                        target.cell = FindNextTurnAttackMove( path, currentUnit, enemies );
                        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "Going after target " << enemy->GetName() << " stopping at " << target.cell );
                    }
                    else {
                        target.cell = move.first;
                    }
                }
            }
        }
        else {
            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, currentUnit.GetName() << " is attacking " << target.unit->GetName() << " at " << target.cell );
        }

        // Walkers: move closer to the castle walls during siege
        if ( _attackingCastle && target.cell == -1 ) {
            uint32_t shortestDist = MAXU16;

            for ( const int wallIndex : underWallsIndicies ) {
                if ( !arena.hexIsPassable( wallIndex ) ) {
                    continue;
                }

                const uint32_t dist = arena.CalculateMoveDistance( wallIndex );
                if ( dist < shortestDist ) {
                    shortestDist = dist;
                    target.cell = wallIndex;
                }
            }
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, "Walker unit moving towards castle walls " << currentUnit.GetName() << " cell " << target.cell );
        }

        return target;
    }

    BattleTargetPair BattlePlanner::meleeUnitDefense( Arena & arena, const Unit & currentUnit ) const
    {
        BattleTargetPair target;

        const Units friendly( arena.GetForce( _myColor ), true );
        const Units enemies( arena.GetForce( _myColor, true ), true );

        const int myHeadIndex = currentUnit.GetHeadIndex();

        const double defenceDistanceModifier = _myArmyStrength / STRENGTH_DISTANCE_FACTOR;

        // 1. Check if there's a target within our half of the battlefield
        MeleeAttackOutcome attackOption;
        for ( const Unit * enemy : enemies ) {
            const MeleeAttackOutcome & outcome = BestAttackOutcome( arena, currentUnit, *enemy );

            // Allow to move only within our half of the battlefield. If in castle make sure to stay inside.
            if ( ( !_defendingCastle && Board::DistanceFromOriginX( outcome.fromIndex, currentUnit.isReflect() ) > ARENAW / 2 )
                 || ( _defendingCastle && !Board::isCastleIndex( outcome.fromIndex ) ) )
                continue;

            if ( IsOutcomeImproved( outcome, attackOption ) ) {
                attackOption.attackValue = outcome.attackValue;
                attackOption.positionValue = outcome.positionValue;
                target.cell = outcome.fromIndex;

                if ( outcome.canReach ) {
                    attackOption.canReach = true;
                    target.unit = enemy;
                }
            }
        }

        // 2. Check if our archer units are under threat - overwrite target and protect
        MeleeAttackOutcome protectOption;
        for ( const Unit * unitToDefend : friendly ) {
            if ( unitToDefend->GetUID() == currentUnit.GetUID() || !unitToDefend->isArchers() ) {
                continue;
            }

            const std::pair<int, uint32_t> move = arena.CalculateMoveToUnit( *unitToDefend );
            const uint32_t distanceToUnit = ( move.first != -1 ) ? move.second : Board::GetDistance( myHeadIndex, unitToDefend->GetHeadIndex() );
            const double archerValue = unitToDefend->GetStrength() - distanceToUnit * defenceDistanceModifier;

            DEBUG_LOG( DBG_AI, DBG_TRACE, unitToDefend->GetName() << " archer value " << archerValue << " distance: " << distanceToUnit );

            // 3. Search for enemy units blocking our archers within range move
            const Indexes & adjacentEnemies = Board::GetAdjacentEnemies( *unitToDefend );
            for ( const int cell : adjacentEnemies ) {
                const Unit * enemy = Board::GetCell( cell )->GetUnit();
                if ( !enemy ) {
                    DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Board::GetAdjacentEnemies returned a cell " << cell << " that does not contain a unit!" );
                    continue;
                }

                MeleeAttackOutcome outcome = BestAttackOutcome( arena, currentUnit, *enemy );
                outcome.positionValue = archerValue;

                DEBUG_LOG( DBG_BATTLE, DBG_TRACE, " - Found enemy, cell " << cell << " threat " << outcome.attackValue );

                if ( IsOutcomeImproved( outcome, protectOption ) ) {
                    protectOption.attackValue = outcome.attackValue;
                    protectOption.positionValue = archerValue;
                    target.cell = outcome.fromIndex;

                    if ( outcome.canReach ) {
                        protectOption.canReach = true;
                        target.unit = enemy;
                    }
                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, " - Target selected " << enemy->GetName() << " cell " << target.cell << " archer value " << archerValue );
                }
            }

            // 4. No enemies found anywhere - move in closer to the friendly ranged unit
            if ( !target.unit && protectOption.positionValue < archerValue ) {
                target.cell = move.first;
                protectOption.positionValue = archerValue;
            }
        }

        if ( target.unit ) {
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " defending against " << target.unit->GetName() << " threat level: " << protectOption.attackValue );
        }
        else if ( target.cell != -1 ) {
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " protecting friendly archer, moving to " << target.cell );
        }

        return target;
    }

    Actions BattlePlanner::berserkTurn( Arena & arena, const Unit & currentUnit ) const
    {
        assert( currentUnit.Modes( SP_BERSERKER ) );
        Actions actions;

        Board & board = *Arena::GetBoard();
        const uint32_t currentUnitUID = currentUnit.GetUID();

        const std::vector<Unit *> nearestUnits = board.GetNearestTroops( &currentUnit, std::vector<Unit *>() );
        if ( !nearestUnits.empty() ) {
            for ( size_t i = 0; i < nearestUnits.size(); ++i ) {
                const uint32_t targetUnitUID = nearestUnits[i]->GetUID();
                const int32_t targetUnitHead = nearestUnits[i]->GetHeadIndex();
                if ( currentUnit.isArchers() && !currentUnit.isHandFighting() ) {
                    actions.emplace_back( MSG_BATTLE_ATTACK, currentUnitUID, targetUnitUID, targetUnitHead, 0 );
                    break;
                }
                else {
                    int targetCell = -1;
                    const Indexes & around = Board::GetAroundIndexes( *nearestUnits[i] );
                    for ( const int cell : around ) {
                        if ( arena.hexIsPassable( cell ) ) {
                            targetCell = cell;
                            break;
                        }
                    }

                    if ( targetCell != -1 ) {
                        if ( currentUnit.GetHeadIndex() != targetCell )
                            actions.emplace_back( MSG_BATTLE_MOVE, currentUnitUID, targetCell );

                        actions.emplace_back( MSG_BATTLE_ATTACK, currentUnitUID, targetUnitUID, targetUnitHead, 0 );
                        break;
                    }
                }
            }
        }

        actions.emplace_back( MSG_BATTLE_END_TURN, currentUnitUID );
        return actions;
    }

    void Normal::BattleTurn( Arena & arena, const Unit & currentUnit, Actions & actions )
    {
        const Actions & plannedActions = _battlePlanner.planUnitTurn( arena, currentUnit );
        actions.insert( actions.end(), plannedActions.begin(), plannedActions.end() );
        // Do not end the turn if we only cast a spell
        if ( plannedActions.size() != 1 || !plannedActions.front().isType( MSG_BATTLE_CAST ) )
            actions.emplace_back( MSG_BATTLE_END_TURN, currentUnit.GetUID() );
    }
}
