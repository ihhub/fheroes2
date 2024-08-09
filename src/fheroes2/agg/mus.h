/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2008 by Josh Matthews <josh@joshmatthews.net>           *
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

#include <string>

namespace MUS
{
    enum MusicTrack : int
    {
        UNUSED,

        DATATRACK, // not in use
        BATTLE1,
        BATTLE2,
        BATTLE3,
        SORCERESS_CASTLE,
        WARLOCK_CASTLE,
        NECROMANCER_CASTLE,
        KNIGHT_CASTLE,
        BARBARIAN_CASTLE,
        WIZARD_CASTLE,
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
        ARCHIBALD_CAMPAIGN_SCREEN,
        PUZZLE,
        ROLAND_CAMPAIGN_SCREEN,
        CARAVANS, // not in use
        CARAVANS_2, // not in use
        CARAVANS_3, // not in use
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

        // IMPORTANT!!! Put all new entries above this line.
        UNKNOWN
    };

    enum class ExternalMusicNamingScheme : int
    {
        MAPPED,
        DOS_VERSION,
        WIN_VERSION
    };

    std::string getFileName( const int musicTrackId, const ExternalMusicNamingScheme namingScheme, const char * fileExtension );

    int FromGround( const int groundType );
    int FromRace( const int race );

    int GetBattleRandom();
}

#endif
