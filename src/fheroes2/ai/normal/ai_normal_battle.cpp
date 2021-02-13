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

    bool isHeroWorthSaving( const Heroes * hero )
    {
        return hero && ( hero->GetLevel() > 2 || !hero->GetBagArtifacts().empty() );
    }

    bool CheckCommanderCanSpellcast( const Arena & arena, const HeroBase * commander )
    {
        return commander && commander->HaveSpellBook() && !commander->Modes( Heroes::SPELLCASTED ) && !arena.isSpellcastDisabled();
    }

    bool CheckBattleRetreat( double myArmy, double enemy )
    {
        // FIXME: more sophisticated logic to see if remaining units are under threat
        // Consider taking speed/turn order into account as well
        // Pass in ( const Units & friendly, const Units & enemies ) instead

        // Retreat if remaining army strength is 10% of enemy's army
        return myArmy * 10 < enemy;
    }

    bool CheckIfUnitIsFaster( const Unit & currentUnit, const Unit & target )
    {
        if ( currentUnit.isFlying() == target.isFlying() )
            return currentUnit.GetSpeed() > target.GetSpeed();
        return currentUnit.isFlying();
    }

    void ForceSpellcastBeforeRetreat( Arena & arena, const HeroBase * commander, Actions & actions )
    {
        if ( CheckCommanderCanSpellcast( arena, commander ) ) {
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
                actions.push_back( Battle::Command( MSG_BATTLE_CAST, bestSpell, targetIdx ) );
            }
        }
    }

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
            // 1, 5 or 4 -> 3, 2, 5
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

    void Normal::BattleTurn( Arena & arena, const Unit & currentUnit, Actions & actions )
    {
        if ( currentUnit.Modes( SP_BERSERKER ) != 0 ) {
            berserkTurn( arena, currentUnit, actions );
            return;
        }

        const int myColor = currentUnit.GetColor();
        const int myHeadIndex = currentUnit.GetHeadIndex();
        const uint32_t currentUnitMoveRange = currentUnit.isFlying() ? MAXU16 : currentUnit.GetSpeed();

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, currentUnit.GetName() << " start their turn. Side: " << myColor );

        const HeroBase * commander = currentUnit.GetCommander();
        const Force & friendlyForce = arena.GetForce( myColor );
        const Force & enemyForce = arena.GetForce( myColor, true );

        // This should filter out all invalid units
        const Units friendly( friendlyForce, true );
        const Units enemies( enemyForce, true );
        // const size_t enemiesCount = enemies.size();

        // Step 1. Friendly and enemy army analysis
        double myArmyStrength = 0;
        double enemyArmyStrength = 0;
        double myShooterStr = 0;
        double enemyShooterStr = 0;
        // double averageAllyDefense = 0;
        // double averageEnemyAttack = 0;
        int highestDamageExpected = 0;

        for ( Units::const_iterator it = enemies.begin(); it != enemies.end(); ++it ) {
            const Unit & unit = **it;
            const double unitStr = unit.GetStrength();

            enemyArmyStrength += unitStr;
            if ( unit.isArchers() ) {
                enemyShooterStr += unitStr;
            }

            const int dmg = unit.CalculateMaxDamage( currentUnit );
            if ( dmg > highestDamageExpected )
                highestDamageExpected = dmg;

            // averageEnemyAttack += unit.GetAttack();
        }

        for ( Units::const_iterator it = friendly.begin(); it != friendly.end(); ++it ) {
            const Unit & unit = **it;
            const double unitStr = unit.GetStrength();

            myArmyStrength += unitStr;
            if ( unit.isArchers() ) {
                myShooterStr += unitStr;
            }

            // averageAllyDefense += unit.GetDefense();
        }

        const double enemyArcherRatio = enemyShooterStr / enemyArmyStrength;
        // Will be used for better unit strength heuristic
        // averageAllyDefense = ( enemiesCount > 0 ) ? averageAllyDefense / enemiesCount : 1;
        // averageEnemyAttack = ( enemiesCount > 0 ) ? averageEnemyAttack / enemiesCount : 1;

        // Step 2. Add castle siege (and battle arena) modifiers
        bool attackingCastle = false;
        bool defendingCastle = false;
        const Castle * castle = Arena::GetCastle();
        if ( castle ) {
            const bool attackerIgnoresCover = arena.GetForce1().GetCommander()->HasArtifact( Artifact::GOLDEN_BOW );

            auto getTowerStrength = [&currentUnit]( const Tower * tower ) { return ( tower && tower->isValid() ) ? tower->GetScoreQuality( currentUnit ) : 0; };

            double towerStr = getTowerStrength( Arena::GetTower( TWR_CENTER ) );
            towerStr += getTowerStrength( Arena::GetTower( TWR_LEFT ) );
            towerStr += getTowerStrength( Arena::GetTower( TWR_RIGHT ) );
            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "- Castle strength: " << towerStr );

            if ( myColor == castle->GetColor() ) {
                defendingCastle = true;
                myShooterStr += towerStr;
                if ( !attackerIgnoresCover )
                    enemyShooterStr /= 2;
            }
            else {
                attackingCastle = true;
                enemyShooterStr += towerStr;
                if ( !attackerIgnoresCover )
                    myShooterStr /= 2;
            }
        }

        // When we have in 10 times stronger army than the enemy we could consider it as an overpowered and we most likely will win.
        const bool myOverpoweredArmy = myArmyStrength > enemyArmyStrength * 10;

        const bool defensiveTactics = enemyArcherRatio < 0.75 && ( defendingCastle || myShooterStr > enemyShooterStr ) && !myOverpoweredArmy;
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE,
                   "Tactic " << defensiveTactics << " chosen. Archers: " << myShooterStr << ", vs enemy " << enemyShooterStr << " ratio is " << enemyArcherRatio );

        const double attackDistanceModifier = enemyArmyStrength / STRENGTH_DISTANCE_FACTOR;
        const double defenceDistanceModifier = myArmyStrength / STRENGTH_DISTANCE_FACTOR;

        // Step 3. Check retreat/surrender condition
        const Heroes * actualHero = dynamic_cast<const Heroes *>( commander );
        if ( actualHero && arena.CanRetreatOpponent( myColor ) && isHeroWorthSaving( actualHero ) && CheckBattleRetreat( myArmyStrength, enemyArmyStrength ) ) {
            // Cast maximum damage spell
            ForceSpellcastBeforeRetreat( arena, commander, actions );

            actions.push_back( Command( MSG_BATTLE_RETREAT ) );
            actions.push_back( Command( MSG_BATTLE_END_TURN, currentUnit.GetUID() ) );
            return;
        }

        // Step 4. Calculate spell heuristics

        // Hero should conserve spellpoints if fighting against monsters or AI and has advantage
        if ( !( myOverpoweredArmy && enemyForce.GetControl() == CONTROL_AI ) && CheckCommanderCanSpellcast( arena, commander ) ) {
            // 1. For damage spells - maximum amount of enemy threat lost
            // 2. For buffs - friendly unit strength gained
            // 3. For debuffs - enemy unit threat lost
            // 4. For dispell, resurrect and cure - amount of unit strength recovered
            // 5. For antimagic - based on enemy hero spellcasting abilities multiplied by friendly unit strength

            // 6. Cast best spell with highest heuristic on target pointer saved

            // Temporary: force damage spell
            ForceSpellcastBeforeRetreat( arena, commander, actions );
        }

        // Step 5. Current unit decision tree
        const size_t actionsSize = actions.size();
        const Unit * target = NULL;
        int targetCell = -1;

        if ( currentUnit.isArchers() ) {
            // Ranged unit decision tree
            if ( currentUnit.isHandFighting() ) {
                // Current ranged unit is blocked by the enemy

                // force archer to fight back by setting initial expectation to lowest possible (if we're losing battle)
                int bestOutcome = ( myArmyStrength < enemyArmyStrength ) ? -highestDamageExpected : 0;
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
                        if ( canOutrunEnemy && !CheckIfUnitIsFaster( currentUnit, *enemy ) )
                            canOutrunEnemy = false;
                    }
                    else {
                        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Board::GetAdjacentEnemies returned a cell " << cell << " that does not contain a unit!" );
                    }
                }

                if ( target && targetCell != -1 ) {
                    // attack selected target
                    DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " archer deciding to fight back: " << bestOutcome );
                    actions.push_back( Battle::Command( MSG_BATTLE_ATTACK, currentUnit.GetUID(), target->GetUID(), targetCell, 0 ) );
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
                    actions.push_back( Battle::Command( MSG_BATTLE_ATTACK, currentUnit.GetUID(), target->GetUID(), target->GetHeadIndex(), 0 ) );

                    DEBUG_LOG( DBG_BATTLE, DBG_INFO,
                               currentUnit.GetName() << " archer focusing enemy " << target->GetName() << " threat level: " << target->GetScoreQuality( currentUnit ) );
                }
            }
        }
        else {
            // Melee unit decision tree (both flyers and walkers)
            Board & board = *Battle::Arena::GetBoard();
            board.SetPositionQuality( currentUnit );

            if ( defensiveTactics ) {
                // Melee unit - Defensive action
                double maxArcherValue = defenceDistanceModifier * ARENASIZE * -1;
                double maxEnemyValue = attackDistanceModifier * ARENASIZE * -1;
                double maxEnemyThreat = 0;

                // 1. Check if there's a target within our half of the battlefield
                for ( const Unit * enemy : enemies ) {
                    const std::pair<int, uint32_t> move = arena.CalculateMoveToUnit( *enemy );

                    // Allow to move only within our half of the battlefield. If in castle make sure to stay inside.
                    const bool isSafeToMove = ( !defendingCastle && move.second <= ARENAW / 2 ) || ( defendingCastle && Board::isCastleIndex( move.first ) );

                    if ( move.first != -1 && isSafeToMove ) {
                        const double enemyValue = enemy->GetStrength() - move.second * attackDistanceModifier;

                        // Pick highest value unit if there's multiple
                        if ( maxEnemyValue < enemyValue ) {
                            maxEnemyValue = enemyValue;
                            target = enemy;
                            targetCell = move.first;
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
                                const bool hadAnotherTarget = target != NULL;

                                DEBUG_LOG( DBG_BATTLE, DBG_TRACE, " - Found enemy, cell " << cell << " threat " << enemyThreat << " distance " << moveToEnemy.second );

                                // Composite priority criteria:
                                // Primary - Enemy is within move range
                                // Secondary - Archer unit value
                                // Tertiary - Enemy unit threat
                                if ( ( canReach != hadAnotherTarget && canReach )
                                     || ( canReach == hadAnotherTarget
                                          && ( maxArcherValue < archerValue
                                               || ( std::fabs( maxArcherValue - archerValue ) < 0.001 && maxEnemyThreat < enemyThreat ) ) ) ) {
                                    targetCell = moveToEnemy.first;
                                    target = enemy;
                                    maxArcherValue = archerValue;
                                    maxEnemyThreat = enemyThreat;
                                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE,
                                               " - Target selected " << enemy->GetName() << " cell " << targetCell << " archer value " << archerValue );
                                }
                            }
                            else {
                                DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Board::GetAdjacentEnemies returned a cell " << cell << " that does not contain a unit!" );
                            }
                        }

                        // 4. No enemies found anywhere - move in closer to the friendly ranged unit
                        if ( !target && maxArcherValue < archerValue ) {
                            targetCell = move.first;
                            maxArcherValue = archerValue;
                        }
                    }
                }

                if ( target ) {
                    DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " defending against " << target->GetName() << " threat level: " << maxEnemyThreat );
                }
                else if ( targetCell != -1 ) {
                    DEBUG_LOG( DBG_BATTLE, DBG_INFO, currentUnit.GetName() << " protecting friendly archer, moving to " << targetCell );
                }
            }
            else {
                // Melee unit - Offensive action
                double maxPriority = attackDistanceModifier * ARENASIZE * -1;
                int highestValue = 0;

                for ( const Unit * enemy : enemies ) {
                    const Indexes & around = Board::GetAroundIndexes( *enemy );
                    for ( const int cell : around ) {
                        const int quality = Board::GetCell( cell )->GetQuality();
                        const uint32_t dist = arena.CalculateMoveDistance( cell );
                        if ( arena.hexIsPassable( cell ) && dist <= currentUnitMoveRange && highestValue < quality ) {
                            highestValue = quality;
                            target = enemy;
                            targetCell = cell;
                            break;
                        }
                    }

                    // For walking units that don't have a target within reach, pick based on distance priority
                    if ( target == NULL ) {
                        // move node pair consists of move hex index and distance
                        const std::pair<int, uint32_t> move = arena.CalculateMoveToUnit( *enemy );

                        const double unitPriority = enemy->GetScoreQuality( currentUnit ) - move.second * attackDistanceModifier;
                        if ( unitPriority > maxPriority ) {
                            maxPriority = unitPriority;
                            targetCell = move.first;
                        }
                    }
                }

                // Walkers: move closer to the castle walls during siege
                if ( attackingCastle && targetCell == -1 ) {
                    uint32_t shortestDist = MAXU16;

                    for ( const int wallIndex : underWallsIndicies ) {
                        const uint32_t dist = arena.CalculateMoveDistance( wallIndex );
                        if ( dist < shortestDist ) {
                            shortestDist = dist;
                            targetCell = wallIndex;
                        }
                    }
                    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "Walker unit moving towards castle walls " << currentUnit.GetName() << " cell " << targetCell );
                }
            }

            // Melee unit final stage - action target should be determined already, add actions to the queue
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, "Melee phase end, targetCell is " << targetCell );

            if ( targetCell != -1 ) {
                if ( myHeadIndex != targetCell )
                    actions.push_back( Battle::Command( MSG_BATTLE_MOVE, currentUnit.GetUID(), targetCell ) );

                if ( target ) {
                    actions.push_back( Battle::Command( MSG_BATTLE_ATTACK, currentUnit.GetUID(), target->GetUID(), target->GetHeadIndex(), 0 ) );
                    DEBUG_LOG( DBG_BATTLE, DBG_INFO,
                               currentUnit.GetName() << " melee offense, focus enemy " << target->GetName()
                                                     << " threat level: " << target->GetScoreQuality( currentUnit ) );
                }
            }
            // else skip
        }

        // no action was taken - skip
        if ( actions.size() == actionsSize ) {
            actions.push_back( Command( MSG_BATTLE_SKIP, currentUnit.GetUID(), true ) );
        }

        actions.push_back( Battle::Command( MSG_BATTLE_END_TURN, currentUnit.GetUID() ) );
    }

    void Normal::berserkTurn( Arena & arena, const Unit & currentUnit, Actions & actions )
    {
        assert( currentUnit.Modes( SP_BERSERKER ) );

        Board & board = *Battle::Arena::GetBoard();
        const uint32_t currentUnitUID = currentUnit.GetUID();

        const std::vector<Unit *> nearestUnits = board.GetNearestTroops( &currentUnit, std::vector<Unit *>() );
        if ( !nearestUnits.empty() ) {
            for ( size_t i = 0; i < nearestUnits.size(); ++i ) {
                const uint32_t targetUnitUID = nearestUnits[i]->GetUID();
                const int32_t targetUnitHead = nearestUnits[i]->GetHeadIndex();
                if ( currentUnit.isArchers() && !currentUnit.isHandFighting() ) {
                    actions.push_back( Battle::Command( MSG_BATTLE_ATTACK, currentUnitUID, targetUnitUID, targetUnitHead, 0 ) );
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
                            actions.push_back( Battle::Command( MSG_BATTLE_MOVE, currentUnitUID, targetCell ) );

                        actions.push_back( Battle::Command( MSG_BATTLE_ATTACK, currentUnitUID, targetUnitUID, targetUnitHead, 0 ) );
                        break;
                    }
                }
            }
        }

        actions.push_back( Battle::Command( MSG_BATTLE_END_TURN, currentUnitUID ) );
    }
}
