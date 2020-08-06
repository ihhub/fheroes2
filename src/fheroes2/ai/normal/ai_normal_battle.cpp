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
    bool CheckBattleRetreat()
    {
        // FIXME: force AI retreat from battle since battle logic is not implemented
        return true;
    }

    bool CheckCommanderCanSpellcast( const Arena & arena, const HeroBase * commander )
    {
        return commander && commander->HaveSpellBook() && !commander->Modes( Heroes::SPELLCASTED ) && !arena.isSpellcastDisabled();
    }

    void Normal::BattleTurn( Arena & arena, const Unit & currentUnit, Actions & actions )
    {
        // Step 1. Check retreat/surrender condition
        if ( CheckBattleRetreat() ) {
            // Cast maximum damage spell

            actions.push_back( Command( MSG_BATTLE_RETREAT ) );
            actions.push_back( Command( MSG_BATTLE_END_TURN, currentUnit.GetUID() ) );
            return;
        }

        const int difficulty = Settings::Get().GameDifficulty();
        const int myColor = currentUnit.GetColor();

        const HeroBase * commander = currentUnit.GetCommander();
        const Force & friendlyForce = arena.GetForce( myColor );
        const Force & enemyForce = arena.GetForce( myColor, true );

        // This should filter out all invalid units
        const Units friendly( friendlyForce, true );
        const Units enemies( enemyForce, true );
        const size_t enemiesCount = enemies.size();

        // Step 2. Friendly and enemy army analysis
        double myShooterStr = 0;
        double enemyShooterStr = 0;
        double averageEnemyAttack = 0;
        double averageEnemyDefense = 0;

        for ( Units::const_iterator it = friendly.begin(); it != friendly.end(); ++it ) {
            const Unit & unit = **it;

            if ( unit.isArchers() ) {
                DEBUG( DBG_AI, DBG_TRACE, "Friendly shooter: " << unit.GetCount() << " " << unit.GetName() );
                myShooterStr += unit.GetStrength();
            }
        }

        for ( Units::const_iterator it = enemies.begin(); it != enemies.end(); ++it ) {
            const Unit & unit = **it;

            averageEnemyAttack += unit.GetAttack();
            averageEnemyDefense += unit.GetDefense();

            if ( unit.isArchers() ) {
                DEBUG( DBG_AI, DBG_TRACE, "Enemy shooter: " << unit.GetCount() << " " << unit.GetName() );
                enemyShooterStr += unit.GetStrength();
            }
        }
        averageEnemyAttack = ( enemiesCount > 0 ) ? averageEnemyAttack / enemiesCount : 1;
        averageEnemyDefense = ( enemiesCount > 0 ) ? averageEnemyDefense / enemiesCount : 1;

        // Step 3. Add castle siege (and battle arena) modifiers
        const Castle * castle = arena.GetCastle();
        if ( castle ) {
            const bool attackerIgnoresCover = arena.GetForce1().GetCommander()->HasArtifact( Artifact::GOLDEN_BOW );

            double towerStr = arena.GetTower( TWR_LEFT )->GetScoreQuality( currentUnit );
            towerStr += arena.GetTower( TWR_CENTER )->GetScoreQuality( currentUnit );
            towerStr += arena.GetTower( TWR_RIGHT )->GetScoreQuality( currentUnit );
            DEBUG( DBG_AI, DBG_TRACE, "Castle strength: " << towerStr );

            if ( myColor == castle->GetColor() ) {
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
        const bool offensiveTactics = myShooterStr < enemyShooterStr;

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
        if ( currentUnit.isArchers() && !currentUnit.isHandFighting() ) {
            // Ranged unit decision tree
            if ( currentUnit.isHandFighting() ) {
                // Current ranged unit is blocked by the enemy
                int damageDiff = 0;

                // Loop through all adjacent enemy units:
                // 1. Calculate potential damage done
                // 2. Calculate enemy retaliation after
                // 3. Update damageDiff if it's bigger than current value
                // 4. Save target selection

                if ( damageDiff > 0 ) {
                    // attack selected target
                    DEBUG( DBG_AI, DBG_INFO, currentUnit.GetName() << " archer deciding to fight back: " << damageDiff );
                }
                else {
                    // Kiting enemy
                    // Search for a safe spot unit can move away
                    DEBUG( DBG_AI, DBG_INFO, currentUnit.GetName() << " archer kiting enemy" );

                    // Worst case scenario - Skip turn
                }
            }
            else {
                // Loop through all enemy units and calculate threat (to my army, Archers/Flyers/Fast units get bonuses)
                // Attack the highest value unit

                // const Unit * enemy = arena.GetEnemyMaxQuality( myColor );
                // actions.push_back( Battle::Command( MSG_BATTLE_ATTACK, currentUnit.GetUID(), enemy->GetUID(), enemy->GetHeadIndex(), 0 ) );
                DEBUG( DBG_AI, DBG_INFO,
                       currentUnit.GetName() << " archer focusing enemy ..."
                                             << " threat level: ..." );
            }
        }
        else if ( offensiveTactics ) {
            // Melee unit - Offensive action

            // 1. Find highest value enemy unit, save as priority target
            // 2. If priority within reach, attack
            // 3. Otherwise search for another target nearby
            // 4.a. Attack if found, from the tile that is closer to priority target
            // 4.b. Else move to priority target
            DEBUG( DBG_AI, DBG_INFO,
                   currentUnit.GetName() << " melee offense, focus enemy ..."
                                         << " threat level: ..." );
        }
        else {
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

        actions.push_back( Battle::Command( MSG_BATTLE_END_TURN, currentUnit.GetUID() ) );
    }
}
