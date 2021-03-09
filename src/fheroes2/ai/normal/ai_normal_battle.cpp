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
#include "battle_board.h"
#include "battle_catapult.h"
#include "battle_cell.h"
#include "battle_command.h"
#include "battle_tower.h"
#include "battle_troop.h"
#include "castle.h"
#include "difficulty.h"
#include "game.h"
#include "heroes.h"
#include "logging.h"

#include <cassert>

using namespace Battle;

namespace AI
{
    // Usual distance between units at the start of the battle is 10-14 tiles
    // 20% of maximum value lost for every tile travelled to make sure 4 tiles difference matters
    const double STRENGTH_DISTANCE_FACTOR = 5.0;
    const std::vector<int> underWallsIndicies = {7, 28, 49, 72, 95};

    int32_t correctAttackTarget( const Position & target, const int32_t from )
    {
        const Cell * tail = target.GetTail();
        if ( tail && Board::isNearIndexes( from, tail->GetIndex() ) ) {
            return tail->GetIndex();
        }
        const Cell * head = target.GetHead();
        return head ? head->GetIndex() : -1;
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

        const Force & enemyForce = arena.GetForce( _myColor, true );

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, currentUnit.GetName() << " start their turn. Side: " << _myColor );

        // When we have in 10 times stronger army than the enemy we could consider it as an overpowered and we most likely will win.
        const bool myOverpoweredArmy = _myArmyStrength > _enemyArmyStrength * 10;
        const double enemyArcherRatio = _enemyShooterStr / _enemyArmyStrength;

        const bool defensiveTactics = enemyArcherRatio < 0.75 && ( _defendingCastle || _myShooterStr > _enemyShooterStr ) && !myOverpoweredArmy;
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE,
                   "Tactic " << defensiveTactics << " chosen. Archers: " << _myShooterStr << ", vs enemy " << _enemyShooterStr << " ratio is " << enemyArcherRatio );

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

        // Hero should conserve spellpoints if fighting against monsters or AI and has advantage
        if ( !( myOverpoweredArmy && enemyForce.GetControl() == CONTROL_AI ) && isCommanderCanSpellcast( arena, _commander ) ) {
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
            if ( defensiveTactics ) {
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
                    actions.emplace_back( MSG_BATTLE_ATTACK, currentUnit.GetUID(), target.unit->GetUID(), correctAttackTarget( target.unit->GetPosition(), target.cell ),
                                          0 );
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
        _commander = currentUnit.GetCommander();
        _myColor = currentUnit.GetColor();

        const Force & friendlyForce = arena.GetForce( _myColor );
        const Force & enemyForce = arena.GetForce( _myColor, true );

        // Friendly and enemy army analysis
        _myArmyStrength = 0;
        _enemyArmyStrength = 0;
        _myShooterStr = 0;
        _enemyShooterStr = 0;
        _highestDamageExpected = 0;
        _considerRetreat = false;

        for ( Unit * unitPtr : enemyForce ) {
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
        }

        uint32_t initialUnitCount = 0;
        for ( Unit * unitPtr : friendlyForce ) {
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
            const bool attackerIgnoresCover = arena.GetForce1().GetCommander()->HasArtifact( Artifact::GOLDEN_BOW );

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
    }

    Actions BattlePlanner::archerDecision( Arena & arena, const Unit & currentUnit )
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
                double lowestThreat = _enemyArmyStrength;

                const Indexes & moves = arena.getAllAvailableMoves( currentUnit.GetMoveRange() );
                for ( const int moveIndex : moves ) {
                    if ( !Board::GetCell( moveIndex )->GetQuality() ) {
                        double cellThreatLevel = 0.0;

                        for ( const Unit * enemy : enemies ) {
                            const double ratio = static_cast<double>( Board::GetDistance( moveIndex, enemy->GetHeadIndex() ) ) / std::max( 1u, enemy->GetMoveRange() );
                            cellThreatLevel += enemy->GetScoreQuality( currentUnit ) * ( 1.0 - ratio );
                        }

                        if ( cellThreatLevel < lowestThreat ) {
                            lowestThreat = cellThreatLevel;
                            target.cell = moveIndex;
                        }
                    }
                }

                if ( target.cell != -1 ) {
                    actions.emplace_back( MSG_BATTLE_MOVE, currentUnit.GetUID(), target.cell );
                    DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " archer kiting enemy, moves to " << target.cell << " threat is " << lowestThreat );
                }
                else {
                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, currentUnit.GetName() << " archer couldn't find a good hex to move out of " << moves.size() );
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

    BattleTargetPair BattlePlanner::meleeUnitOffense( Arena & arena, const Unit & currentUnit )
    {
        BattleTargetPair target;
        const Units enemies( arena.GetForce( _myColor, true ), true );

        const uint32_t currentUnitMoveRange = currentUnit.GetMoveRange();
        const double attackDistanceModifier = _enemyArmyStrength / STRENGTH_DISTANCE_FACTOR;

        double maxPriority = attackDistanceModifier * ARENASIZE * -1;
        int highestValue = 0;

        for ( const Unit * enemy : enemies ) {
            const Indexes & around = Board::GetAroundIndexes( *enemy );
            for ( const int cell : around ) {
                const int quality = Board::GetCell( cell )->GetQuality();
                const uint32_t dist = arena.CalculateMoveDistance( cell );
                if ( arena.hexIsPassable( cell ) && dist <= currentUnitMoveRange && highestValue < quality ) {
                    highestValue = quality;
                    target.unit = enemy;
                    target.cell = cell;
                    break;
                }
            }

            // For walking units that don't have a target within reach, pick based on distance priority
            if ( target.unit == nullptr ) {
                // move node pair consists of move hex index and distance
                const std::pair<int, uint32_t> move = arena.CalculateMoveToUnit( *enemy );
                // Do not chase after faster units that might kite away and avoid engagement
                const uint32_t distance = ( !enemy->isArchers() && isUnitFaster( *enemy, currentUnit ) ) ? move.second + ARENAW + ARENAH : move.second;

                const double unitPriority = enemy->GetScoreQuality( currentUnit ) - distance * attackDistanceModifier;
                if ( unitPriority > maxPriority ) {
                    maxPriority = unitPriority;
                    target.cell = move.first;
                }
            }
            else {
                DEBUG_LOG( DBG_BATTLE, DBG_TRACE, currentUnit.GetName() << " is attacking " << target.unit->GetName() << " at " << target.cell );
            }
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

    BattleTargetPair BattlePlanner::meleeUnitDefense( Arena & arena, const Unit & currentUnit )
    {
        BattleTargetPair target;

        const Units friendly( arena.GetForce( _myColor ), true );
        const Units enemies( arena.GetForce( _myColor, true ), true );

        const uint32_t currentUnitMoveRange = currentUnit.GetMoveRange();
        const int myHeadIndex = currentUnit.GetHeadIndex();

        const double attackDistanceModifier = _enemyArmyStrength / STRENGTH_DISTANCE_FACTOR;
        const double defenceDistanceModifier = _myArmyStrength / STRENGTH_DISTANCE_FACTOR;

        double maxArcherValue = defenceDistanceModifier * ARENASIZE * -1;
        double maxEnemyValue = attackDistanceModifier * ARENASIZE * -1;
        double maxEnemyThreat = 0;

        // 1. Check if there's a target within our half of the battlefield
        for ( const Unit * enemy : enemies ) {
            const std::pair<int, uint32_t> move = arena.CalculateMoveToUnit( *enemy );

            // Allow to move only within our half of the battlefield. If in castle make sure to stay inside.
            const bool isSafeToMove = ( !_defendingCastle && move.second <= ARENAW / 2 ) || ( _defendingCastle && Board::isCastleIndex( move.first ) );

            if ( move.first != -1 && isSafeToMove ) {
                const double enemyValue = enemy->GetStrength() - move.second * attackDistanceModifier;

                // Pick highest value unit if there's multiple
                if ( maxEnemyValue < enemyValue ) {
                    maxEnemyValue = enemyValue;
                    target.unit = enemy;
                    target.cell = move.first;
                }
            }
        }

        // 2. Check if our archer units are under threat - overwrite target and protect
        for ( const Unit * unitToDefend : friendly ) {
            if ( unitToDefend->GetUID() != currentUnit.GetUID() && unitToDefend->isArchers() ) {
                const int headIndexToDefend = unitToDefend->GetHeadIndex();
                const std::pair<int, uint32_t> move = arena.CalculateMoveToUnit( *unitToDefend );
                const uint32_t distanceToUnit = ( move.first != -1 ) ? move.second : Board::GetDistance( myHeadIndex, headIndexToDefend );
                const double archerValue = unitToDefend->GetStrength() - distanceToUnit * defenceDistanceModifier;

                DEBUG_LOG( DBG_AI, DBG_TRACE, unitToDefend->GetName() << " archer value " << archerValue << " distance: " << distanceToUnit );

                // 3. Search for enemy units blocking our archers within range move
                const Indexes & adjacentEnemies = Board::GetAdjacentEnemies( *unitToDefend );
                for ( const int cell : adjacentEnemies ) {
                    const Unit * enemy = Board::GetCell( cell )->GetUnit();
                    if ( enemy ) {
                        const double enemyThreat = enemy->GetScoreQuality( currentUnit );
                        const std::pair<int, uint32_t> moveToEnemy = arena.CalculateMoveToUnit( *enemy );
                        const bool canReach = moveToEnemy.first != -1 && moveToEnemy.second <= currentUnitMoveRange;
                        const bool hadAnotherTarget = target.unit != NULL;

                        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, " - Found enemy, cell " << cell << " threat " << enemyThreat << " distance " << moveToEnemy.second );

                        // Composite priority criteria:
                        // Primary - Enemy is within move range
                        // Secondary - Archer unit value
                        // Tertiary - Enemy unit threat
                        if ( ( canReach != hadAnotherTarget && canReach )
                             || ( canReach == hadAnotherTarget
                                  && ( maxArcherValue < archerValue || ( std::fabs( maxArcherValue - archerValue ) < 0.001 && maxEnemyThreat < enemyThreat ) ) ) ) {
                            target.cell = moveToEnemy.first;
                            target.unit = enemy;
                            maxArcherValue = archerValue;
                            maxEnemyThreat = enemyThreat;
                            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, " - Target selected " << enemy->GetName() << " cell " << target.cell << " archer value " << archerValue );
                        }
                    }
                    else {
                        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Board::GetAdjacentEnemies returned a cell " << cell << " that does not contain a unit!" );
                    }
                }

                // 4. No enemies found anywhere - move in closer to the friendly ranged unit
                if ( !target.unit && maxArcherValue < archerValue ) {
                    target.cell = move.first;
                    maxArcherValue = archerValue;
                }
            }
        }

        if ( target.unit ) {
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " defending against " << target.unit->GetName() << " threat level: " << maxEnemyThreat );
        }
        else if ( target.cell != -1 ) {
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " protecting friendly archer, moving to " << target.cell );
        }

        return target;
    }

    Actions BattlePlanner::berserkTurn( Arena & arena, const Unit & currentUnit )
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
