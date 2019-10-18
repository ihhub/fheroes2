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

#include <string>
#include <sstream>
#include <iomanip>
#include "race.h"
#include "ground.h"
#include "mus.h"
#include "settings.h"
#include "mp2.h"

namespace MUS
{
    const struct
    {
	int type;
	const char* string;
    } musmap[] = {
        { UNUSED,       ""                    },
        { DATATRACK,    ""                    },
        { BATTLE1,      "Battle (1)"          },
        { BATTLE2,      "Battle (2)"          },
        { BATTLE3,      "Battle (3)"          },
        { BARBARIAN,    "Barbarian Castle"    },
        { SORCERESS,    "Sorceress Castle"    },
        { WARLOCK,      "Warlock Castle"      },
        { WIZARD,       "Wizard Castle"       },
        { NECROMANCER,  "Necromancer Castle"  },
        { KNIGHT,       "Knight Castle"       },
        { LAVA,         "Lava Theme"          },
        { WASTELAND,    "Wasteland Theme"     },
        { DESERT,       "Desert Theme"        },
        { SNOW,         "Snow Theme"          },
        { SWAMP,        "Swamp Theme"         },
        { BEACH,        "Ocean Theme"         },
        { DIRT,         "Dirt Theme"          },
        { GRASS,        "Grass Theme"         },
        { LOSTGAME,     "Lost Game"           },
        { WEEK1,        "Week (1)"            },
        { WEEK2_MONTH1, "Week (2) Month (1)"  },
        { MONTH2,       "Month (2)"           },
        { PUZZLE,       "Map Puzzle"          },
        { ROLAND,       "Roland's Campaign"   },
        { CARAVANS,     "25"                  },
        { CARAVANS_2,   "26"                  },
        { CARAVANS_3,   "27"                  },
        { COMPUTER,     "28"                  },
        { BATTLEWIN,    "29"                  },
        { BATTLELOSE,   "30"                  },
        { DEATH,        "31"                  },
        { WATERSPRING,  "32"                  },
        { ARABIAN,      "33"                  },
        { NOMADTENTS,   "34"                  },
        { TREEHOUSE,    "35"                  },
        { DEMONCAVE,    "36"                  },
        { EXPERIENCE,   "37"                  },
        { SKILL,        "38"                  },
        { WATCHTOWER,   "39"                  },
        { EVENT15,      "40"                  },
        { NEWS,         "41"                  },
        { MAINMENU,     "Main Menu"           },
        { VICTORY,      "Scenario Victory"    },
        { UNKNOWN,      "UNKNOWN"             }
    };
    
    const std::string GetString(int mus, bool shortname)
    {
      std::stringstream sstream;
      sstream << std::setw(2) << std::setfill('0') << (int)mus;
      if(shortname)
      sstream << ".ogg";
      else
      sstream << " " <<
	(UNUSED <= mus && UNKNOWN > mus ? musmap[mus].string : musmap[UNKNOWN].string) << ".ogg";
      return sstream.str();
    }
}

int MUS::FromGround(int ground)
{
    switch(ground)
    {
        case Maps::Ground::DESERT:	return DESERT;
        case Maps::Ground::SNOW:	return SNOW;
        case Maps::Ground::SWAMP:	return SWAMP;
        case Maps::Ground::WASTELAND:	return WASTELAND;
        case Maps::Ground::BEACH:	return BEACH;
        case Maps::Ground::LAVA:	return LAVA;
        case Maps::Ground::DIRT:	return DIRT;
        case Maps::Ground::GRASS:	return GRASS;
        case Maps::Ground::WATER:	return BEACH;
        default: break;
    }

    return UNKNOWN;
}

int MUS::FromRace(int race)
{
    switch(race)
    {
        case Race::KNGT:	return KNIGHT;
        case Race::BARB:	return BARBARIAN;
        case Race::SORC:	return SORCERESS;
        case Race::WRLK:	return WARLOCK;
        case Race::WZRD:	return WIZARD;
        case Race::NECR:	return NECROMANCER;
        default: break;
    }

    return UNKNOWN;
}

int MUS::FromMapObject(int object)
{
    if(Settings::Get().MusicMIDI())
        return MUS::UNKNOWN;
    
    switch(object)
    {
        case MP2::OBJ_WITCHSHUT:
        case MP2::OBJ_FORT:
        case MP2::OBJ_MERCENARYCAMP:
        case MP2::OBJ_DOCTORHUT:
        case MP2::OBJ_STANDINGSTONES:
            return MUS::SKILL;

        case MP2::OBJ_GAZEBO:
        case MP2::OBJ_TREEKNOWLEDGE:
            return MUS::EXPERIENCE;

        case MP2::OBJ_DAEMONCAVE:
            return MUS::DEMONCAVE;

        case MP2::OBJ_TREEHOUSE:
        case MP2::OBJ_TREECITY:
            return MUS::TREEHOUSE;

        case MP2::OBJ_WATCHTOWER:
            return MUS::WATCHTOWER;

        case MP2::OBJ_DESERTTENT:
            return MUS::NOMADTENTS;

        case MP2::OBJ_ARTESIANSPRING:
            return MUS::WATERSPRING;

        case MP2::OBJ_SPHINX:
            return MUS::ARABIAN;

        case MP2::OBJ_EVENT:
            return MUS::NEWS;
            
        default:
            return MUS::UNKNOWN;
    }
}

int MUS::GetBattleRandom(void)
{
    switch(Rand::Get(1, 3))
    {
	case 1:	return BATTLE1;
	case 2:	return BATTLE2;
	case 3:	return BATTLE3;
	default: break;
    }
    return UNKNOWN;
}
