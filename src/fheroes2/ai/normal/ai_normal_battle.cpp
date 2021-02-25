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

    void Normal::HeroesPreBattle( HeroBase & hero )
    {
        // Optimize troops placement before the battle
        Army & army = hero.GetArmy();

        std::vector<Troop> archers;
        std::vector<Troop> others;

        // Validate and pick the troops
        for ( size_t slot = 0; slot < ARMYMAXTROOPS; ++slot ) {
            Troop * troop = army.GetTroop( slot );
            if ( troop && troop->isValid() ) {
                if ( troop->isArchers() ) {
                    archers.push_back( *troop );
                }
                else {
                    others.push_back( *troop );
                }
            }
        }

        // Sort troops by tactical priority. For melee:
        // 1. Faster units first
        // 2. Flyers first
        // 3. Finally if unit type and speed is same, compare by strength
        std::sort( others.begin(), others.end(), []( const Troop & left, const Troop & right ) {
            if ( left.GetSpeed() == right.GetSpeed() ) {
                if ( left.isFlying() == right.isFlying() ) {
                    return left.GetStrength() < right.GetStrength();
                }
                return right.isFlying();
            }
            return left.GetSpeed() < right.GetSpeed();
        } );

        // Archers sorted purely by strength.
        std::sort( archers.begin(), archers.end(), []( const Troop & left, const Troop & right ) { return left.GetStrength() < right.GetStrength(); } );

        std::vector<size_t> slotOrder = {2, 1, 3, 0, 4};
        switch ( archers.size() ) {
        case 1:
            slotOrder = {0, 2, 1, 3, 4};
            break;
        case 2:
            slotOrder = {0, 4, 2, 1, 3};
            break;
        case 3:
            slotOrder = {0, 4, 2, 1, 3};
            break;
        case 4:
            slotOrder = {0, 4, 2, 3, 1};
            break;
        case 5:
            slotOrder = {0, 4, 1, 2, 3};
            break;
        default:
            break;
        }

        // Re-arrange troops in army
        army.Clean();
        for ( const size_t slot : slotOrder ) {
            if ( !archers.empty() ) {
                army.GetTroop( slot )->Set( archers.back() );
                archers.pop_back();
            }
            else if ( !others.empty() ) {
                army.GetTroop( slot )->Set( others.back() );
                others.pop_back();
            }
            else {
                break;
            }
        }
    }

    bool BattlePlanner::isHeroWorthSaving( const Heroes * hero ) const
    {
        return hero && ( hero->GetLevel() > 2 || !hero->GetBagArtifacts().empty() );
    }

    bool BattlePlanner::isCommanderCanSpellcast( const Arena & arena, const HeroBase * commander ) const
    {
        return commander && ( !commander->isControlHuman() || Settings::Get().BattleAutoSpellcast() ) && commander->HaveSpellBook()
               && !commander->Modes( Heroes::SPELLCASTED ) && !arena.isSpellcastDisabled();
    }

    bool BattlePlanner::checkRetreatCondition( double myArmy, double enemy ) const
    {
        // FIXME: more sophisticated logic to see if remaining units are under threat
        // Consider taking speed/turn order into account as well
        // Pass in ( const Units & friendly, const Units & enemies ) instead

        // Retreat if remaining army strength is 10% of enemy's army
        return myArmy * 10 < enemy;
    }

    bool BattlePlanner::isUnitFaster( const Unit & currentUnit, const Unit & target ) const
    {
        if ( currentUnit.isFlying() == target.isFlying() )
            return currentUnit.GetSpeed() > target.GetSpeed();
        return currentUnit.isFlying();
    }

    Actions BattlePlanner::forceSpellcastBeforeRetreat( Arena & arena, const HeroBase * commander )
    {
        Actions result;
        if ( !isCommanderCanSpellcast( arena, commander ) ) {
            return result;
        }

        const std::vector<Spell> allSpells = commander->GetSpells();
        int bestSpell = -1;
        double bestHeuristic = 0;
        int targetIdx = -1;

        const Units friendly( arena.GetForce( commander->GetColor() ), true );
        const Units enemies( arena.GetForce( commander->GetColor(), true ), true );

        const int spellPower = commander->GetPower();
        for ( const Spell & spell : allSpells ) {
            if ( !commander->HaveSpellPoints( spell ) )
                continue;

            // TODO: add mass spells
            if ( spell.isCombat() && spell.isDamage() && spell.isSingleTarget() ) {
                const uint32_t totalDamage = spell.Damage() * spellPower;
                for ( const Unit * enemy : enemies ) {
                    const double spellHeuristic
                        = enemy->GetMonsterStrength() * enemy->HowManyWillKilled( totalDamage * ( 100 - enemy->GetMagicResist( spell, spellPower ) ) / 100 );

                    if ( spellHeuristic > bestHeuristic ) {
                        bestHeuristic = spellHeuristic;
                        bestSpell = spell.GetID();
                        targetIdx = enemy->GetHeadIndex();
                    }
                }
            }
        }

        if ( bestSpell != -1 ) {
            result.emplace_back( MSG_BATTLE_CAST, bestSpell, targetIdx );
        }
        return result;
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
        const HeroBase * commander = currentUnit.GetCommander();

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, currentUnit.GetName() << " start their turn. Side: " << _myColor );

        // When we have in 10 times stronger army than the enemy we could consider it as an overpowered and we most likely will win.
        const bool myOverpoweredArmy = _myArmyStrength > _enemyArmyStrength * 10;
        const double enemyArcherRatio = _enemyShooterStr / _enemyArmyStrength;

        const bool defensiveTactics = enemyArcherRatio < 0.75 && ( _defendingCastle || _myShooterStr > _enemyShooterStr ) && !myOverpoweredArmy;
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE,
                   "Tactic " << defensiveTactics << " chosen. Archers: " << _myShooterStr << ", vs enemy " << _enemyShooterStr << " ratio is " << enemyArcherRatio );

        // Step 2. Check retreat/surrender condition
        const Heroes * actualHero = dynamic_cast<const Heroes *>( commander );
        if ( actualHero && arena.CanRetreatOpponent( _myColor ) && isHeroWorthSaving( actualHero ) && checkRetreatCondition( _myArmyStrength, _enemyArmyStrength ) ) {
            // Cast maximum damage spell
            actions = forceSpellcastBeforeRetreat( arena, commander );

            actions.emplace_back( MSG_BATTLE_RETREAT );
            actions.emplace_back( MSG_BATTLE_END_TURN, currentUnit.GetUID() );
            return actions;
        }

        // Step 3. Calculate spell heuristics

        // Hero should conserve spellpoints if fighting against monsters or AI and has advantage
        if ( !( myOverpoweredArmy && enemyForce.GetControl() == CONTROL_AI ) && isCommanderCanSpellcast( arena, commander ) ) {
            // 1. For damage spells - maximum amount of enemy threat lost
            // 2. For buffs - friendly unit strength gained
            // 3. For debuffs - enemy unit threat lost
            // 4. For dispell, resurrect and cure - amount of unit strength recovered
            // 5. For antimagic - based on enemy hero spellcasting abilities multiplied by friendly unit strength

            // 6. Cast best spell with highest heuristic on target pointer saved

            // Temporary: force damage spell
            actions = forceSpellcastBeforeRetreat( arena, commander );
        }

        // Step 4. Current unit decision tree
        const size_t actionsSize = actions.size();

        if ( currentUnit.isArchers() ) {
            const Actions & archerActions = archerDecision( arena, currentUnit );
            actions.insert( actions.end(), archerActions.begin(), archerActions.end() );
        }
        else {
            // Melee unit decision tree (both flyers and walkers)
            Board & board = *Battle::Arena::GetBoard();
            board.SetPositionQuality( currentUnit );

            // Determine unit target/cell to move
            BattleTargetPair target;

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
                    actions.emplace_back( MSG_BATTLE_ATTACK, currentUnit.GetUID(), target.unit->GetUID(), target.unit->GetHeadIndex(), 0 );
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
        _myColor = currentUnit.GetColor();
        const Force & friendlyForce = arena.GetForce( _myColor );
        const Force & enemyForce = arena.GetForce( _myColor, true );

        // This should filter out all invalid units
        const Units friendly( friendlyForce, true );
        const Units enemies( enemyForce, true );

        // Friendly and enemy army analysis
        _myArmyStrength = 0;
        _enemyArmyStrength = 0;
        _myShooterStr = 0;
        _enemyShooterStr = 0;
        _highestDamageExpected = 0;

        for ( Units::const_iterator it = enemies.begin(); it != enemies.end(); ++it ) {
            const Unit & unit = **it;
            const double unitStr = unit.GetStrength();

            _enemyArmyStrength += unitStr;
            if ( unit.isArchers() ) {
                _enemyShooterStr += unitStr;
            }

            const int dmg = unit.CalculateMaxDamage( currentUnit );
            if ( dmg > _highestDamageExpected )
                _highestDamageExpected = dmg;
        }

        for ( Units::const_iterator it = friendly.begin(); it != friendly.end(); ++it ) {
            const Unit & unit = **it;
            const double unitStr = unit.GetStrength();

            _myArmyStrength += unitStr;
            if ( unit.isArchers() ) {
                _myShooterStr += unitStr;
            }
        }

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
        const Unit * target = NULL;
        int targetCell = -1;

        if ( currentUnit.isHandFighting() ) {
            // Current ranged unit is blocked by the enemy

            // force archer to fight back by setting initial expectation to lowest possible (if we're losing battle)
            int bestOutcome = ( _myArmyStrength < _enemyArmyStrength ) ? -_highestDamageExpected : 0;
            bool canOutrunEnemy = true;

            const Indexes & adjacentEnemies = Board::GetAdjacentEnemies( currentUnit );
            for ( const int cell : adjacentEnemies ) {
                const Unit * enemy = Board::GetCell( cell )->GetUnit();
                if ( enemy ) {
                    const int archerMeleeDmg = currentUnit.GetDamage( *enemy );
                    const int damageDiff = archerMeleeDmg - enemy->CalculateRetaliationDamage( archerMeleeDmg );

                    if ( bestOutcome < damageDiff ) {
                        bestOutcome = damageDiff;
                        target = enemy;
                        targetCell = cell;
                    }

                    // try to determine if it's worth running away (canOutrunEnemy stays true ONLY if all enemies are slower)
                    if ( canOutrunEnemy && !isUnitFaster( currentUnit, *enemy ) )
                        canOutrunEnemy = false;
                }
                else {
                    DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Board::GetAdjacentEnemies returned a cell " << cell << " that does not contain a unit!" );
                }
            }

            if ( target && targetCell != -1 ) {
                // attack selected target
                DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " archer deciding to fight back: " << bestOutcome );
                actions.emplace_back( MSG_BATTLE_ATTACK, currentUnit.GetUID(), target->GetUID(), targetCell, 0 );
            }
            else if ( canOutrunEnemy ) {
                // Kiting enemy
                // Search for a safe spot unit can move away
                DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " archer kiting enemy" );
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
                    target = enemy;
                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Set priority on " << enemy->GetName() << " value " << attackPriority );
                }
            }

            if ( target ) {
                actions.emplace_back( MSG_BATTLE_ATTACK, currentUnit.GetUID(), target->GetUID(), target->GetHeadIndex(), 0 );

                DEBUG_LOG( DBG_BATTLE, DBG_INFO,
                           currentUnit.GetName() << " archer focusing enemy " << target->GetName() << " threat level: " << target->GetScoreQuality( currentUnit ) );
            }
        }

        return actions;
    }

    BattleTargetPair BattlePlanner::meleeUnitOffense( Arena & arena, const Unit & currentUnit )
    {
        BattleTargetPair target;
        const Units enemies( arena.GetForce( _myColor, true ), true );

        const uint32_t currentUnitMoveRange = currentUnit.isFlying() ? MAXU16 : currentUnit.GetSpeed();
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
        }

        // Walkers: move closer to the castle walls during siege
        if ( _attackingCastle && target.cell == -1 ) {
            uint32_t shortestDist = MAXU16;

            for ( const int wallIndex : underWallsIndicies ) {
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

        const uint32_t currentUnitMoveRange = currentUnit.isFlying() ? MAXU16 : currentUnit.GetSpeed();
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

        actions.emplace_back( MSG_BATTLE_END_TURN, currentUnit.GetUID() );
    }
}
