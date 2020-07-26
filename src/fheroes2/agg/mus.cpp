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

#include <iomanip>
#include <sstream>
#include <string>

#include "ground.h"
#include "mp2.h"
#include "mus.h"
#include "race.h"
#include "settings.h"

namespace MUS
{
    const struct
    {
        int type;
        const char * string;
    } musmap[] = {{UNUSED, ""},
                  {DATATRACK, ""},
                  {BATTLE1, "Battle 1"},
                  {BATTLE2, "Battle 2"},
                  {BATTLE3, "Battle 3"},
                  {SORCERESS, "Sorceress Castle"},
                  {WARLOCK, "Warlock Castle"},
                  {NECROMANCER, "Necromancer Castle"},
                  {KNIGHT, "Knight Castle"},
                  {BARBARIAN, "Barbarian Castle"},
                  {WIZARD, "Wizard Castle"},
                  {LAVA, "Lava Theme"},
                  {WASTELAND, "Wasteland Theme"},
                  {DESERT, "Desert Theme"},
                  {SNOW, "Snow Theme"},
                  {SWAMP, "Swamp Theme"},
                  {OCEAN, "Ocean Theme"},
                  {DIRT, "Dirt Theme"},
                  {GRASS, "Grass Theme"},
                  {LOSTGAME, "Lost Game"},
                  {NEW_WEEK, "New Week"},
                  {NEW_MONTH, "New Month"},
                  {ARCHIBALD, "Archibald Campaign"},
                  {PUZZLE, "Map Puzzle"},
                  {ROLAND, "Roland Campaign"},
                  {CARAVANS, "25"},
                  {CARAVANS_2, "26"},
                  {CARAVANS_3, "27"},
                  {COMPUTER_TURN, "AI Turn"},
                  {BATTLEWIN, "Battle Won"},
                  {BATTLELOSE, "Battle Lost"},
                  {DUNGEON, "Dungeon"},
                  {WATERSPRING, "Waterspring"},
                  {ARABIAN, "Arabian"},
                  {HILLFORT, "Hillfort"},
                  {TREEHOUSE, "Treehouse"},
                  {DEMONCAVE, "Demoncave"},
                  {EXPERIENCE, "Experience"},
                  {SKILL, "Skill"},
                  {WATCHTOWER, "Watchtower"},
                  {XANADU, "Xanadu"},
                  {ULTIMATE_ARTIFACT, "Ultimate Artifact"},
                  {MAINMENU, "Main Menu"},
                  {VICTORY, "Scenario Victory"},
                  {UNKNOWN, "UNKNOWN"}};

    const std::string GetString( int mus, bool longName )
    {
        std::stringstream sstream;
        if ( longName ) {
            sstream << std::setw( 2 ) << std::setfill( '0' ) << mus;
            sstream << " " << ( UNUSED <= mus && UNKNOWN > mus ? musmap[mus].string : musmap[UNKNOWN].string ) << ".ogg";
        }
        else {
            // GOG version format, data track was ignored there so 02 becomes 01
            sstream << "homm2_" << std::setw( 2 ) << std::setfill( '0' ) << mus - 1 << ".ogg";
        }

        return sstream.str();
    }
}

int MUS::FromGround( int ground )
{
    switch ( ground ) {
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
        break;
    }

    return UNKNOWN;
}

int MUS::FromRace( int race )
{
    switch ( race ) {
    case Race::KNGT:
        return KNIGHT;
    case Race::BARB:
        return BARBARIAN;
    case Race::SORC:
        return SORCERESS;
    case Race::WRLK:
        return WARLOCK;
    case Race::WZRD:
        return WIZARD;
    case Race::NECR:
        return NECROMANCER;
    default:
        break;
    }

    return UNKNOWN;
}

int MUS::FromMapObject( int object )
{
    if ( Settings::Get().MusicMIDI() )
        return MUS::UNKNOWN;

    switch ( object ) {
    case MP2::OBJ_PYRAMID:
    case MP2::OBJ_DRAGONCITY:
    case MP2::OBJ_CITYDEAD:
    case MP2::OBJ_TROLLBRIDGE:
        return MUS::DUNGEON;

    case MP2::OBJ_ARTESIANSPRING:
    case MP2::OBJ_MAGICWELL:
    case MP2::OBJ_ORACLE:
        return MUS::WATERSPRING;

    case MP2::OBJ_DESERTTENT: // Changed OG selection to something more appropriate
    case MP2::OBJ_SPHINX:
    case MP2::OBJ_ANCIENTLAMP:
        return MUS::ARABIAN;

    case MP2::OBJ_HILLFORT:
        return MUS::HILLFORT;

    case MP2::OBJ_TREEHOUSE:
    case MP2::OBJ_TREECITY:
    case MP2::OBJ_WAGONCAMP:
        return MUS::TREEHOUSE;

    case MP2::OBJ_DAEMONCAVE:
        return MUS::DEMONCAVE;

    case MP2::OBJ_GAZEBO:
    case MP2::OBJ_TREEKNOWLEDGE:
    case MP2::OBJ_WITCHSHUT:
        return MUS::EXPERIENCE;

    case MP2::OBJ_FORT:
    case MP2::OBJ_MERCENARYCAMP:
    case MP2::OBJ_DOCTORHUT:
    case MP2::OBJ_STANDINGSTONES:
        return MUS::SKILL;

    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_SHIPWRECK:
    case MP2::OBJ_DERELICTSHIP:
    case MP2::OBJ_ABANDONEDMINE:
    case MP2::OBJ_MAGELLANMAPS:
    case MP2::OBJ_WATCHTOWER:
        return MUS::WATCHTOWER;

    case MP2::OBJ_XANADU:
    case MP2::OBJ_LIGHTHOUSE:
        return MUS::XANADU;

    default:
        return MUS::UNKNOWN;
    }
}

int MUS::GetBattleRandom( void )
{
    switch ( Rand::Get( 1, 3 ) ) {
    case 1:
        return BATTLE1;
    case 2:
        return BATTLE2;
    case 3:
        return BATTLE3;
    default:
        break;
    }
    return UNKNOWN;
}
