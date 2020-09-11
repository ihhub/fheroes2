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
using namespace Battle;

namespace AI
{
    bool CheckBattleRetreat( const Units & friendly, const Units & enemies )
    {
        // FIXME: force AI retreat from battle since battle logic is not implemented
        return true;
    }

    bool CheckCommanderCanSpellcast( const Arena & arena, const HeroBase * commander )
    {
        return commander && commander->HaveSpellBook() && !commander->Modes( Heroes::SPELLCASTED ) && !arena.isSpellcastDisabled();
    }

    bool CheckIfUnitIsFaster( const Unit & currentUnit, const Unit & target )
    {
        if ( currentUnit.isFlying() == target.isFlying() )
            return currentUnit.GetSpeed() > target.GetSpeed();
        return currentUnit.isFlying();
    }

    void Normal::BattleTurn( Arena & arena, const Unit & currentUnit, Actions & actions )
    {
        const int difficulty = Settings::Get().GameDifficulty();
        const int myColor = currentUnit.GetColor();

        const HeroBase * commander = currentUnit.GetCommander();
        const Force & friendlyForce = arena.GetForce( myColor );
        const Force & enemyForce = arena.GetForce( myColor, true );

        // This should filter out all invalid units
        const Units friendly( friendlyForce, true );
        const Units enemies( enemyForce, true );
        const size_t enemiesCount = enemies.size();

        // Step 1. Friendly and enemy army analysis
        const Unit * priorityTarget = NULL;
        const Unit * target = NULL;
        int targetCell = -1;

        double myArmyStrength = 0;
        double enemyArmyStrength = 0;
        double myShooterStr = 0;
        double enemyShooterStr = 0;
        double averageAllyDefense = 0;
        double averageEnemyAttack = 0;
        int highestDamageExpected = 0;

        double highestStrength = 0;
        for ( Units::const_iterator it = enemies.begin(); it != enemies.end(); ++it ) {
            const Unit & unit = **it;
            const double unitStr = unit.GetScoreQuality( currentUnit );

            if ( highestStrength < unitStr ) {
                highestStrength = unitStr;
                priorityTarget = *it;
            }

            enemyArmyStrength += unitStr;
            if ( unit.isArchers() ) {
                enemyShooterStr += unitStr;
            }

            const int dmg = unit.CalculateMaxDamage( currentUnit );
            if ( dmg > highestDamageExpected )
                highestDamageExpected = dmg;

            averageEnemyAttack += unit.GetAttack();
        }

        for ( Units::const_iterator it = friendly.begin(); it != friendly.end(); ++it ) {
            const Unit & unit = **it;
            const double unitStr = unit.GetScoreQuality( *priorityTarget );

            myArmyStrength += unitStr;
            if ( unit.isArchers() ) {
                myShooterStr += unitStr;
            }

            averageAllyDefense += unit.GetDefense();
        }

        averageAllyDefense = ( enemiesCount > 0 ) ? averageAllyDefense / enemiesCount : 1;
        averageEnemyAttack = ( enemiesCount > 0 ) ? averageEnemyAttack / enemiesCount : 1;

        // Step 2. Add castle siege (and battle arena) modifiers
        bool defendingCastle = false;
        const Castle * castle = arena.GetCastle();
        if ( castle ) {
            const bool attackerIgnoresCover = arena.GetForce1().GetCommander()->HasArtifact( Artifact::GOLDEN_BOW );

            double towerStr = arena.GetTower( TWR_LEFT )->GetScoreQuality( currentUnit );
            towerStr += arena.GetTower( TWR_CENTER )->GetScoreQuality( currentUnit );
            towerStr += arena.GetTower( TWR_RIGHT )->GetScoreQuality( currentUnit );
            DEBUG( DBG_AI, DBG_TRACE, "Castle strength: " << towerStr );

            if ( myColor == castle->GetColor() ) {
                defendingCastle = true;
                myShooterStr += towerStr;
                if ( !attackerIgnoresCover )
                    enemyShooterStr /= 2;
            }
            else {
                enemyShooterStr += towerStr;
                if ( !attackerIgnoresCover )
                    myShooterStr /= 2;
            }
        }

        DEBUG( DBG_AI, DBG_TRACE, "Comparing shooters: " << myShooterStr << ", vs enemy " << enemyShooterStr );
        const bool defensiveTactics = defendingCastle || myShooterStr > enemyShooterStr;

        // Step 3. Check retreat/surrender condition
        if ( !defendingCastle && commander && CheckBattleRetreat( friendly, enemies ) ) {
            // Cast maximum damage spell

            actions.push_back( Command( MSG_BATTLE_RETREAT ) );
            actions.push_back( Command( MSG_BATTLE_END_TURN, currentUnit.GetUID() ) );
            return;
        }


        // Step 4. Calculate spell heuristics
        if ( CheckCommanderCanSpellcast( arena, commander ) ) {
            // 1. For damage spells - maximum amount of enemy threat lost
            // 2. For buffs - friendly unit strength gained
            // 3. For debuffs - enemy unit threat lost
            // 4. For dispell, resurrect and cure - amount of unit strength recovered
            // 5. For antimagic - based on enemy hero spellcasting abilities multiplied by friendly unit strength

            // 6. Cast best spell with highest heuristic on target pointer saved
        }

        // Step 5. Current unit decision tree
        if ( currentUnit.isArchers() ) {
            // Ranged unit decision tree
            if ( currentUnit.isHandFighting() ) {
                // Current ranged unit is blocked by the enemy

                // force archer to fight back by setting initial expectation to lowest possible (if we're losing battle)
                int bestOutcome = ( myArmyStrength < enemyArmyStrength ) ? -highestDamageExpected : 0;

                const Indexes & adjacentEnemies = Board::GetAdjacentEnemies( currentUnit );
                for ( const int cell : adjacentEnemies ) {
                    const Unit * enemy = Board::GetCell( cell )->GetUnit();
                    const int archerMeleeDmg = currentUnit.GetDamage( *enemy );
                    const int damageDiff = archerMeleeDmg - enemy->CalculateRetaliationDamage( archerMeleeDmg );
                    
                    if ( bestOutcome < damageDiff ) {
                        bestOutcome = damageDiff;
                        target = enemy;
                        targetCell = cell;
                    }
                }

                if ( target && targetCell != -1 ) {
                    // attack selected target
                    DEBUG( DBG_AI, DBG_INFO, currentUnit.GetName() << " archer deciding to fight back: " << bestOutcome );
                    actions.push_back( Battle::Command( MSG_BATTLE_ATTACK, currentUnit.GetUID(), target->GetUID(), targetCell, 0 ) );
                }
                else {
                    // Kiting enemy
                    // Search for a safe spot unit can move away
                    DEBUG( DBG_AI, DBG_INFO, currentUnit.GetName() << " archer kiting enemy" );

                    // Worst case scenario - Skip turn
                }
            }
            else {
                // Normal attack: focus the highest value unit
                if ( priorityTarget ) {
                    actions.push_back( Battle::Command( MSG_BATTLE_ATTACK, currentUnit.GetUID(), priorityTarget->GetUID(), priorityTarget->GetHeadIndex(), 0 ) );
                }
                DEBUG( DBG_AI, DBG_INFO,
                       currentUnit.GetName() << " archer focusing enemy ..."
                                             << " threat level: ..." );
            }
        }
        else {
            if ( defensiveTactics ) {
                // Melee unit - Defensive action

                // Search for enemy units threatening our archers within range
                const bool archersUnderThreat = false;

                if ( archersUnderThreat ) {
                    DEBUG( DBG_AI, DBG_INFO,
                           currentUnit.GetName() << " defending against ..."
                                                 << " threat level: ..." );
                }
                else {
                    // Find best friendly archer and move to them
                    DEBUG( DBG_AI, DBG_INFO, currentUnit.GetName() << " protecting unit ..." );
                }
            }
            else {
                // Melee unit - Offensive action
                uint32_t strength = 0;

                // Loop through all enemy units and calculate threat (to my army, Archers/Flyers/Fast units get bonuses)
                for ( const Unit * enemy : enemies ) {
                    const uint32_t unitStr = enemy->GetScoreQuality( currentUnit );
                    if ( unitStr > strength ) {
                        strength = unitStr;
                        target = enemy;
                    }
                }

                // 1. Find highest value enemy unit, save as priority target
                // 2. If priority within reach, attack
                // 3. Otherwise search for another target nearby
                // 4.a. Attack if found, from the tile that is closer to priority target
                // 4.b. Else move to priority target
                DEBUG( DBG_AI, DBG_INFO,
                       currentUnit.GetName() << " melee offense, focus enemy ..."
                                             << " threat level: ..." );
            }
        }

        actions.push_back( Battle::Command( MSG_BATTLE_END_TURN, currentUnit.GetUID() ) );
    }
}
