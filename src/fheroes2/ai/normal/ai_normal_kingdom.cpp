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
        const int color = kingdom.GetColor();
        KingdomHeroes & heroes = kingdom.GetHeroes();
        KingdomCastles & castles = kingdom.GetCastles();

        if ( kingdom.isLoss() || color == Color::NONE ) {
            kingdom.LossPostActions();
            return;
        }

        if ( !Settings::Get().MusicMIDI() )
            AGG::PlayMusic( MUS::COMPUTER );

        Interface::StatusWindow & status = Interface::Basic::Get().GetStatusWindow();

        // indicator
        status.RedrawTurnProgress( 0 );

        size_t heroLimit = Maps::XLARGE > world.w() ? ( Maps::LARGE > world.w() ? 2 : 3 ) : 4;
        if ( _personality == EXPLORER )
            heroLimit++;

        // Scan visible map

        status.RedrawTurnProgress( 1 );

        // Buy heroes, adjust roles

        VecHeroes sortedHeroList = heroes;
        // Sort

        status.RedrawTurnProgress( 2 );

        size_t heroesMovedCount = 0;
        for ( VecHeroes::iterator it = sortedHeroList.begin(); it != sortedHeroList.end(); ++it ) {
            if ( *it ) {
                HeroTurn( **it );

                heroesMovedCount++;
                status.RedrawTurnProgress( 2 + ( 7 * heroesMovedCount / sortedHeroList.size() ) );
            }
        }

        status.RedrawTurnProgress( 9 );

        for ( KingdomCastles::iterator it = castles.begin(); it != castles.end(); ++it ) {
            if ( *it ) {
                CastleTurn( **it );
            }
        }
    }
}
