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

#include "agg.h"
#include "ai_normal.h"
#include "game_interface.h"
#include "kingdom.h"
#include "mus.h"
#include "world.h"

namespace AI
{
    void Normal::KingdomTurn( Kingdom & kingdom )
    {
        const int difficulty = Settings::Get().GameDifficulty();
        const int color = kingdom.GetColor();

        if ( kingdom.isLoss() || color == Color::NONE ) {
            kingdom.LossPostActions();
            return;
        }

        // reset indicator
        Interface::StatusWindow & status = Interface::Basic::Get().GetStatusWindow();
        status.RedrawTurnProgress( 0 );

        AGG::PlayMusic( MUS::COMPUTER_TURN );
        KingdomHeroes & heroes = kingdom.GetHeroes();
        KingdomCastles & castles = kingdom.GetCastles();

        // Step 1. Scan visible map (based on game difficulty), add goals and threats
        status.RedrawTurnProgress( 1 );

        // Step 2. Update AI variables and recalculate resource budget
        int combinedHeroStrength = 0;
        for ( auto it = heroes.begin(); it != heroes.end(); ++it ) {
            if ( *it ) {
                combinedHeroStrength += ( *it )->GetArmy().GetStrength();
            }
        }

        size_t heroLimit = Maps::XLARGE > world.w() ? ( Maps::LARGE > world.w() ? 2 : 3 ) : 4;
        if ( _personality == EXPLORER )
            heroLimit++;

        // Step 3. Buy new heroes, adjust roles, sort heroes based on priority or strength
        if ( heroes.size() < heroLimit && castles.size() ) {
            Recruits & rec = kingdom.GetRecruits();

            // FIXME: Pick appropriate castle to buy hero from
            Heroes * hero = castles.front()->GetHeroes().Guest();
            if ( !hero ) {
                castles.front()->RecruitHero( rec.GetHero1() );
            }
        }
        VecHeroes sortedHeroList = heroes;

        status.RedrawTurnProgress( 2 );

        // Step 4. Move heroes until they have nothing to do (HERO_WAITING or HERO_MOVED state)
        size_t heroesMovedCount = 0;
        for ( auto it = sortedHeroList.begin(); it != sortedHeroList.end(); ++it ) {
            if ( *it ) {
                HeroTurn( **it );

                if ( ( *it )->Modes( HERO_MOVED ) ) {
                    ++heroesMovedCount;
                    status.RedrawTurnProgress( 2 + ( 7 * heroesMovedCount / sortedHeroList.size() ) );
                }
            }
        }

        // Step 5. Repeat process (maybe there was a path unlocked by a stronger hero)
        for ( auto it = sortedHeroList.begin(); it != sortedHeroList.end(); ++it ) {
            if ( *it && !( *it )->Modes( HERO_MOVED ) ) {
                HeroTurn( **it );
                ++heroesMovedCount;
                status.RedrawTurnProgress( 2 + ( 7 * heroesMovedCount / sortedHeroList.size() ) );
            }
        }

        status.RedrawTurnProgress( 9 );

        // Step 6. Castle development according to kingdom budget
        for ( KingdomCastles::iterator it = castles.begin(); it != castles.end(); ++it ) {
            if ( *it ) {
                CastleTurn( **it );
            }
        }
    }
}
