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
    void Normal::BattleTurn( Arena & arena, const Unit & currentUnit, Actions & actions )
    {
        const int myColor = currentUnit.GetColor();

        Board * board = Arena::GetBoard();
        const HeroBase * commander = currentUnit.GetCommander();
        const Force & friendlyForce = arena.GetForce( myColor );
        const Force & enemyForce = arena.GetForce( myColor, true );

        // This should filter out all invalid units
        Units friendly = Units( friendlyForce, true );
        Units enemies = Units( enemyForce, true );

        double myShooterStr = 0;
        double enemyShooterStr = 0;

        for ( Units::const_iterator it = friendly.begin(); it != friendly.end(); ++it ) {
            Unit * unit = *it;

            if ( unit && unit->isArchers() ) {
                DEBUG( DBG_AI, DBG_TRACE, "Friendly shooter: " << unit->GetName() << " count " << unit->GetCount() );
                myShooterStr += currentUnit.GetStrength();
            }
        }

        for ( Units::const_iterator it = enemies.begin(); it != enemies.end(); ++it ) {
            Unit * unit = *it;

            if ( unit && unit->isValid() && unit->isArchers() ) {
                DEBUG( DBG_AI, DBG_TRACE, "Enemy shooter: " << unit->GetName() << " count " << unit->GetCount() );
                enemyShooterStr += currentUnit.GetStrength();
            }
        }

        const Castle * castle = arena.GetCastle();
        if ( castle ) {
            const bool attackerIgnoresCover = arena.GetForce1().GetCommander()->HasArtifact( Artifact::GOLDEN_BOW );
            double towerStr = 0;

            towerStr += arena.GetTower( TWR_LEFT )->GetScoreQuality( currentUnit );
            towerStr += arena.GetTower( TWR_CENTER )->GetScoreQuality( currentUnit );
            towerStr += arena.GetTower( TWR_RIGHT )->GetScoreQuality( currentUnit );

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

        //const Unit * enemy = arena.GetEnemyMaxQuality( myColor );
        //actions.push_back( Battle::Command( MSG_BATTLE_ATTACK, currentUnit.GetUID(), enemy->GetUID(), enemy->GetHeadIndex(), 0 ) );

        actions.push_back( Battle::Command( MSG_BATTLE_END_TURN, currentUnit.GetUID() ) );
    }
}