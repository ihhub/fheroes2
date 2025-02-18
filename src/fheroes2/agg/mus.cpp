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

#include "mus.h"

#include <array>
#include <cassert>
#include <string>

#include "ground.h"
#include "race.h"
#include "rand.h"

namespace
{
    struct MusMapItem
    {
        int type;
        const char * string;
    };

    void addTrackId( std::string & output, const int musicTrackId )
    {
        if ( musicTrackId < 10 ) {
            output += '0';
        }

        output += std::to_string( musicTrackId );
    }

    const std::array<MusMapItem, 45> musmap = { { { MUS::UNUSED, "" },
                                                  { MUS::DATATRACK, "" },
                                                  { MUS::BATTLE1, "Battle 1" },
                                                  { MUS::BATTLE2, "Battle 2" },
                                                  { MUS::BATTLE3, "Battle 3" },
                                                  { MUS::SORCERESS_CASTLE, "Sorceress Castle" },
                                                  { MUS::WARLOCK_CASTLE, "Warlock Castle" },
                                                  { MUS::NECROMANCER_CASTLE, "Necromancer Castle" },
                                                  { MUS::KNIGHT_CASTLE, "Knight Castle" },
                                                  { MUS::BARBARIAN_CASTLE, "Barbarian Castle" },
                                                  { MUS::WIZARD_CASTLE, "Wizard Castle" },
                                                  { MUS::LAVA, "Lava Theme" },
                                                  { MUS::WASTELAND, "Wasteland Theme" },
                                                  { MUS::DESERT, "Desert Theme" },
                                                  { MUS::SNOW, "Snow Theme" },
                                                  { MUS::SWAMP, "Swamp Theme" },
                                                  { MUS::OCEAN, "Ocean Theme" },
                                                  { MUS::DIRT, "Dirt Theme" },
                                                  { MUS::GRASS, "Grass Theme" },
                                                  { MUS::LOSTGAME, "Lost Game" },
                                                  { MUS::NEW_WEEK, "New Week" },
                                                  { MUS::NEW_MONTH, "New Month" },
                                                  { MUS::ARCHIBALD_CAMPAIGN_SCREEN, "Archibald Campaign" },
                                                  { MUS::PUZZLE, "Map Puzzle" },
                                                  { MUS::ROLAND_CAMPAIGN_SCREEN, "Roland Campaign" },
                                                  { MUS::CARAVANS, "25" },
                                                  { MUS::CARAVANS_2, "26" },
                                                  { MUS::CARAVANS_3, "27" },
                                                  { MUS::COMPUTER_TURN, "AI Turn" },
                                                  { MUS::BATTLEWIN, "Battle Won" },
                                                  { MUS::BATTLELOSE, "Battle Lost" },
                                                  { MUS::DUNGEON, "Dungeon" },
                                                  { MUS::WATERSPRING, "Waterspring" },
                                                  { MUS::ARABIAN, "Arabian" },
                                                  { MUS::HILLFORT, "Hillfort" },
                                                  { MUS::TREEHOUSE, "Treehouse" },
                                                  { MUS::DEMONCAVE, "Demoncave" },
                                                  { MUS::EXPERIENCE, "Experience" },
                                                  { MUS::SKILL, "Skill" },
                                                  { MUS::WATCHTOWER, "Watchtower" },
                                                  { MUS::XANADU, "Xanadu" },
                                                  { MUS::ULTIMATE_ARTIFACT, "Ultimate Artifact" },
                                                  { MUS::MAINMENU, "Main Menu" },
                                                  { MUS::VICTORY, "Scenario Victory" },
                                                  { MUS::UNKNOWN, "UNKNOWN" } } };
}

namespace MUS
{
    std::string getFileName( const int musicTrackId, const ExternalMusicNamingScheme namingScheme, const char * fileExtension )
    {
        assert( fileExtension != nullptr );

        if ( musicTrackId <= UNUSED || musicTrackId > UNKNOWN ) {
            // You are passing an invalid music track ID!
            assert( 0 );
            return {};
        }

        if ( namingScheme == ExternalMusicNamingScheme::MAPPED ) {
            std::string output;
            addTrackId( output, musicTrackId );
            output += ' ';

            output += musmap[musicTrackId].string;
            output += fileExtension;
            return output;
        }

        if ( namingScheme == ExternalMusicNamingScheme::DOS_VERSION ) {
            std::string output( "homm2_" );

            // GOG version format, data track was ignored there so 02 becomes 01
            addTrackId( output, musicTrackId - 1 );
            output += fileExtension;
            return output;
        }

        if ( namingScheme == ExternalMusicNamingScheme::WIN_VERSION ) {
            std::string output( "Track" );
            addTrackId( output, musicTrackId );
            output += fileExtension;
            return output;
        }

        // Did you add a new type of music?
        assert( 0 );
        return {};
    }

    int FromGround( const int groundType )
    {
        switch ( groundType ) {
        case Maps::Ground::DESERT:
            return DESERT;
        case Maps::Ground::SNOW:
            return SNOW;
        case Maps::Ground::SWAMP:
            return SWAMP;
        case Maps::Ground::WASTELAND:
            return WASTELAND;
        case Maps::Ground::BEACH:
            return OCEAN;
        case Maps::Ground::LAVA:
            return LAVA;
        case Maps::Ground::DIRT:
            return DIRT;
        case Maps::Ground::GRASS:
            return GRASS;
        case Maps::Ground::WATER:
            return OCEAN;
        default:
            // Did you add a new ground type? Add the track for it!
            assert( 0 );
            break;
        }

        return UNKNOWN;
    }

    int FromRace( const int race )
    {
        switch ( race ) {
        case Race::KNGT:
            return KNIGHT_CASTLE;
        case Race::BARB:
            return BARBARIAN_CASTLE;
        case Race::SORC:
            return SORCERESS_CASTLE;
        case Race::WRLK:
            return WARLOCK_CASTLE;
        case Race::WZRD:
            return WIZARD_CASTLE;
        case Race::NECR:
            return NECROMANCER_CASTLE;
        default:
            // Did you add a new race? Add an appropriate music theme for it!
            assert( 0 );
            break;
        }

        return UNKNOWN;
    }

    int GetBattleRandom()
    {
        switch ( Rand::Get( 1, 3 ) ) {
        case 1:
            return BATTLE1;
        case 2:
            return BATTLE2;
        case 3:
            return BATTLE3;
        default:
            // How is it even possible?
            assert( 0 );
            break;
        }

        return UNKNOWN;
    }
}
