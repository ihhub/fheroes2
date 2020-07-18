/***************************************************************************
 *   Copyright (C) 2008 by Josh Matthews <josh@joshmatthews.net>           *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#ifndef H2MUS_H
#define H2MUS_H

#include "gamedefs.h"

namespace MUS
{
    enum
    {
        UNUSED,
        DATATRACK,
        BATTLE1,
        BATTLE2,
        BATTLE3,
        SORCERESS,
        WARLOCK,
        NECROMANCER,
        KNIGHT,
        BARBARIAN,
        WIZARD,
        LAVA,
        WASTELAND,
        DESERT,
        SNOW,
        SWAMP,
        OCEAN,
        DIRT,
        GRASS,
        LOSTGAME,
        NEW_WEEK,
        NEW_MONTH,
        ARCHIBALD,
        PUZZLE,
        ROLAND,
        CARAVANS,
        CARAVANS_2,
        CARAVANS_3,
        COMPUTER_TURN,
        BATTLEWIN,
        BATTLELOSE,
        DUNGEON,
        WATERSPRING,
        ARABIAN,
        HILLFORT,
        TREEHOUSE,
        DEMONCAVE,
        EXPERIENCE,
        SKILL,
        WATCHTOWER,
        XANADU,
        ULTIMATE_ARTIFACT,
        MAINMENU,
        VICTORY,

        UNKNOWN
    };

    const std::string GetString( int, bool longName = false );

    int FromGround( int );
    int FromRace( int );
    int FromMapObject( int );

    int GetBattleRandom( void );
}

#endif
