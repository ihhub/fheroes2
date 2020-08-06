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
#include "heroes.h"
#include "maps.h"
#include "mp2.h"
#include "world.h"

namespace AI
{
    namespace
    {
        const int temporaryHeroScanDist = 15;
    }

    bool MoveHero( Heroes & hero )
    {
        // FIXME: Very basic set up, targets and priorities should be fed from AI Kingdom
        const int heroIndex = hero.GetIndex();

        // Maps::GetAroundIndexes should sort tiles internally
        const Maps::Indexes & seenTiles = Maps::GetAroundIndexes( heroIndex, temporaryHeroScanDist, true );
        for ( auto it = seenTiles.begin(); it != seenTiles.end(); ++it ) {
            if ( HeroesValidObject( hero, *it ) && hero.GetPath().Calculate( *it ) ) {
                HeroesMove( hero );
                return true;
            }
        }

        hero.SetModes( AI::HERO_WAITING );
        return false;
    }

    void Normal::HeroTurn( Heroes & hero )
    {
        hero.ResetModes( AI::HERO_WAITING | AI::HERO_MOVED | AI::HERO_SKIP_TURN );

        while ( hero.MayStillMove() && !hero.Modes( AI::HERO_WAITING | AI::HERO_MOVED ) ) {
            MoveHero( hero );
        }

        if ( !hero.MayStillMove() ) {
            hero.SetModes( AI::HERO_MOVED );
        }
    }
}
