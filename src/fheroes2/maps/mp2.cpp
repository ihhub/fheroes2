/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "settings.h"
#include "direction.h"
#include "icn.h"
#include "mp2.h"

/* return name icn object */
int MP2::GetICNObject(int type)
{
    switch(type)
    {
	// reserverd
	case 0:
	    return ICN::UNKNOWN;

	// manual
	case 0x11:
	    return ICN::TELEPORT1;
	case 0x12:
	    return ICN::TELEPORT2;
	case 0x13:
	    return ICN::TELEPORT3;
	case 0x14:
	    return ICN::FOUNTAIN;
	case 0x15:
	    return ICN::TREASURE;

	// artifact
	case 0x2C:
	case 0x2D:
	case 0x2E:
	case 0x2F:
	    return ICN::OBJNARTI;

	// monster
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	    return ICN::MONS32;

	// castle flags
	case 0x38:
	case 0x39:
	case 0x3A:
	case 0x3B:
	    return ICN::FLAG32;

	// heroes
	case 0x54:
	case 0x55:
	case 0x56:
	case 0x57:
	    return ICN::MINIHERO;

	// relief: snow
	case 0x58:
	case 0x59:
	case 0x5A:
	case 0x5B:
	    return ICN::MTNSNOW;

	// relief: swamp
	case 0x5C:
	case 0x5D:
	case 0x5E:
	case 0x5F:
	    return ICN::MTNSWMP;

	// relief: lava
	case 0x60:
	case 0x61:
	case 0x62:
	case 0x63:
	    return ICN::MTNLAVA;

	// relief: desert
	case 0x64:
	case 0x65:
	case 0x66:
	case 0x67:
	    return ICN::MTNDSRT;

	// relief: dirt
	case 0x68:
	case 0x69:
	case 0x6A:
	case 0x6B:
	    return ICN::MTNDIRT;

	// relief: others
	case 0x6C:
	case 0x6D:
	case 0x6E:
	case 0x6F:
	    return ICN::MTNMULT;

	// mines
	case 0x74:
	    return ICN::EXTRAOVR;

	// road
	case 0x78:
	case 0x79:
	case 0x7A:
	case 0x7B:
	    return ICN::ROAD;

	// relief: crck
	case 0x7C:
	case 0x7D:
	case 0x7E:
	case 0x7F:
	    return ICN::MTNCRCK;

	// relief: gras
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	    return ICN::MTNGRAS;

	// trees jungle
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x87:
	    return ICN::TREJNGL;

	// trees evil
	case 0x88:
	case 0x89:
	case 0x8A:
	case 0x8B:
	    return ICN::TREEVIL;

	// castle and tower
	case 0x8C:
	case 0x8D:
	case 0x8E:
	case 0x8F:
	    return ICN::OBJNTOWN;

	// castle lands
	case 0x90:
	case 0x91:
	case 0x92:
	case 0x93:
	    return ICN::OBJNTWBA;

	// castle shadow
	case 0x94:
	case 0x95:
	case 0x96:
	case 0x97:
	    return ICN::OBJNTWSH;

	// random castle
	case 0x98:
	case 0x99:
	case 0x9A:
	case 0x9B:
	    return ICN::OBJNTWRD;

	// water object
	case 0xA0:
	case 0xA1:
	case 0xA2:
	case 0xA3:
	    return ICN::OBJNWAT2;

	// object other
	case 0xA4:
	case 0xA5:
	case 0xA6:
	case 0xA7:
	    return ICN::OBJNMUL2;

	// trees snow
	case 0xA8:
	case 0xA9:
	case 0xAA:
	case 0xAB:
	    return ICN::TRESNOW;

	// trees trefir
	case 0xAC:
	case 0xAD:
	case 0xAE:
	case 0xAF:
	    return ICN::TREFIR;

	// trees
	case 0xB0:
	case 0xB1:
	case 0xB2:
	case 0xB3:
	    return ICN::TREFALL;

	// river
	case 0xB4:
	case 0xB5:
	case 0xB6:
	case 0xB7:
	    return ICN::STREAM;

	// resource
	case 0xB8:
	case 0xB9:
	case 0xBA:
	case 0xBB:
	    return ICN::OBJNRSRC;

	// gras object
	case 0xC0:
	case 0xC1:
	case 0xC2:
	case 0xC3:
	    return ICN::OBJNGRA2;

	// trees tredeci
	case 0xC4:
	case 0xC5:
	case 0xC6:
	case 0xC7:
	    return ICN::TREDECI;

	// sea object
	case 0xC8:
	case 0xC9:
	case 0xCA:
	case 0xCB:
	    return ICN::OBJNWATR;

	// vegetation gras
	case 0xCC:
	case 0xCD:
	case 0xCE:
	case 0xCF:
	    return ICN::OBJNGRAS;

	// object on snow
	case 0xD0:
	case 0xD1:
	case 0xD2:
	case 0xD3:
	    return ICN::OBJNSNOW;

	// object on swamp
	case 0xD4:
	case 0xD5:
	case 0xD6:
	case 0xD7:
	    return ICN::OBJNSWMP;

	// object on lava
	case 0xD8:
	case 0xD9:
	case 0xDA:
	case 0xDB:
	    return ICN::OBJNLAVA;

	// object on desert
	case 0xDC:
	case 0xDD:
	case 0xDE:
	case 0xDF:
	    return ICN::OBJNDSRT;

	// object on dirt
	case 0xE0:
	case 0xE1:
	case 0xE2:
	case 0xE3:
	    return ICN::OBJNDIRT;

	// object on crck
	case 0xE4:
	case 0xE5:
	case 0xE6:
	case 0xE7:
	    return ICN::OBJNCRCK;

	// object on lava
	case 0xE8:
	case 0xE9:
	case 0xEA:
	case 0xEB:
	    return ICN::OBJNLAV3;

	// object on earth
	case 0xEC:
	case 0xED:
	case 0xEE:
	case 0xEF:
	    return ICN::OBJNMULT;
	    
	//  object on lava
	case 0xF0:
	case 0xF1:
	case 0xF2:
	case 0xF3:
	    return ICN::OBJNLAV2;

	// extra objects for loyalty version
	case 0xF4:
	case 0xF5:
	case 0xF6:
	case 0xF7:
	    if(Settings::Get().PriceLoyaltyVersion()) return ICN::X_LOC1;
	    break;

	// extra objects for loyalty version
	case 0xF8:
	case 0xF9:
	case 0xFA:
	case 0xFB:
	    if(Settings::Get().PriceLoyaltyVersion()) return ICN::X_LOC2;
	    break;

	// extra objects for loyalty version
	case 0xFC:
	case 0xFD:
	case 0xFE:
	case 0xFF:
	    if(Settings::Get().PriceLoyaltyVersion()) return ICN::X_LOC3;
	    break;

	default:
	    break;
    }

    DEBUG(DBG_GAME, DBG_WARN, "unknown type: " << static_cast<int>(type));

    return ICN::UNKNOWN;
}

const char* MP2::StringObject(int object)
{
    switch(object)
    {
        case OBJ_ZERO:			return "OBJ_ZERO";
        case OBJN_ALCHEMYLAB:
        case OBJ_ALCHEMYLAB:		return _("Alchemist Lab");
        case OBJN_DAEMONCAVE:
        case OBJ_DAEMONCAVE:		return _("Daemon Cave");
        case OBJN_FAERIERING:
        case OBJ_FAERIERING:		return _("Faerie Ring");
        case OBJN_GRAVEYARD:
        case OBJ_GRAVEYARD:		return _("Graveyard");
        case OBJN_DRAGONCITY:
        case OBJ_DRAGONCITY:		return _("Dragon City");
        case OBJN_LIGHTHOUSE:
        case OBJ_LIGHTHOUSE:		return _("Light House");
        case OBJN_WATERWHEEL:
        case OBJ_WATERWHEEL:		return _("Water Wheel");
        case OBJN_MINES:
        case OBJ_MINES:			return _("Mines");
        case OBJN_OBELISK:
        case OBJ_OBELISK:		return _("Obelisk");
        case OBJN_OASIS:
        case OBJ_OASIS:			return _("Oasis");
        case OBJN_SAWMILL:
        case OBJ_SAWMILL:		return _("Sawmill");
        case OBJN_ORACLE:
        case OBJ_ORACLE:		return _("Oracle");
        case OBJN_DESERTTENT:
        case OBJ_DESERTTENT:		return _("Desert Tent");
        case OBJN_CASTLE:
        case OBJ_CASTLE:		return _("Castle");
        case OBJN_WAGONCAMP:
        case OBJ_WAGONCAMP:		return _("Wagon Camp");
        case OBJN_WINDMILL:
        case OBJ_WINDMILL:		return _("Windmill");
        case OBJN_RNDTOWN:
        case OBJ_RNDTOWN:		return _("Random Town");
        case OBJN_RNDCASTLE:
        case OBJ_RNDCASTLE:		return _("Random Castle");
        case OBJN_WATCHTOWER:
        case OBJ_WATCHTOWER:            return _("Watch Tower");
        case OBJN_TREECITY:
        case OBJ_TREECITY:		return _("Tree City");
        case OBJN_TREEHOUSE:
        case OBJ_TREEHOUSE:             return _("Tree House");
        case OBJN_RUINS:
        case OBJ_RUINS:			return _("Ruins");
        case OBJN_FORT:
        case OBJ_FORT:			return _("Fort");
        case OBJN_TRADINGPOST:
        case OBJ_TRADINGPOST:		return _("Trading Post");
        case OBJN_ABANDONEDMINE:
        case OBJ_ABANDONEDMINE:		return _("Abandoned Mine");
        case OBJN_TREEKNOWLEDGE:
        case OBJ_TREEKNOWLEDGE:		return _("Tree of Knowledge");
        case OBJN_DOCTORHUT:
        case OBJ_DOCTORHUT:		return _("Witch Doctor's Hut");
        case OBJN_TEMPLE:
        case OBJ_TEMPLE:		return _("Temple");
        case OBJN_HILLFORT:
        case OBJ_HILLFORT:		return _("Hill Fort");
        case OBJN_HALFLINGHOLE:
        case OBJ_HALFLINGHOLE:		return _("Halfling Hole");
        case OBJN_MERCENARYCAMP:
        case OBJ_MERCENARYCAMP:		return _("Mercenary Camp");
        case OBJN_PYRAMID:
        case OBJ_PYRAMID:		return _("Pyramid");
        case OBJN_CITYDEAD:
        case OBJ_CITYDEAD:		return _("City of the Dead");
        case OBJN_EXCAVATION:
        case OBJ_EXCAVATION:		return _("Excavation");
        case OBJN_SPHINX:
        case OBJ_SPHINX:		return _("Sphinx");
        case OBJN_TROLLBRIDGE:
        case OBJ_TROLLBRIDGE:		return _("Troll Bridge");
        case OBJN_WITCHSHUT:
        case OBJ_WITCHSHUT:		return _("Witch Hut");
        case OBJN_XANADU:
        case OBJ_XANADU:		return _("Xanadu");
        case OBJN_CAVE:
        case OBJ_CAVE:			return _("Cave");
        case OBJN_MAGELLANMAPS:
        case OBJ_MAGELLANMAPS:		return _("Magellan Maps");
        case OBJN_DERELICTSHIP:
        case OBJ_DERELICTSHIP:		return _("Derelict Ship");
        case OBJN_SHIPWRECK:
        case OBJ_SHIPWRECK:		return _("Ship Wreck");
        case OBJN_OBSERVATIONTOWER:
        case OBJ_OBSERVATIONTOWER:	return _("Observation Tower");
        case OBJN_FREEMANFOUNDRY:
        case OBJ_FREEMANFOUNDRY:	return _("Freeman Foundry");
        case OBJN_WATERINGHOLE:
        case OBJ_WATERINGHOLE:          return _("Watering Hole");
        case OBJN_ARTESIANSPRING:
        case OBJ_ARTESIANSPRING:        return _("Artesian Spring");
        case OBJN_GAZEBO:
        case OBJ_GAZEBO:		return _("Gazebo");
        case OBJN_ARCHERHOUSE:
        case OBJ_ARCHERHOUSE:		return _("Archer's House");
        case OBJN_PEASANTHUT:
        case OBJ_PEASANTHUT:		return _("Peasant Hut");
        case OBJN_DWARFCOTT:
        case OBJ_DWARFCOTT:		return _("Dwarf Cottage");
        case OBJN_STONELIGHTS:
        case OBJ_STONELIGHTS:		return _("Stone Liths");	// https://sourceforge.net/projects/fheroes2/forums/forum/335991/topic/4605429
        case OBJN_MAGICWELL:
        case OBJ_MAGICWELL:             return _("Magic Well");
        case OBJ_HEROES:		return _("Heroes");
        case OBJ_SIGN:			return _("Sign");
        case OBJ_SHRUB2:		return _("Shrub");
        case OBJ_NOTHINGSPECIAL:	return _("Nothing Special");
        case OBJ_TARPIT:		return _("Tar Pit");
        case OBJ_COAST:			return _("Coast");
        case OBJ_MOUND:			return _("Mound");
        case OBJ_DUNE:			return _("Dune");
	case OBJ_STUMP:			return _("Stump");
	case OBJ_CACTUS:		return _("Cactus");
        case OBJ_TREES:			return _("Trees");
        case OBJ_DEADTREE:		return _("Dead Tree");
        case OBJ_MOUNTS:		return _("Mountains");
        case OBJ_VOLCANO:		return _("Volcano");
        case OBJ_STONES:		return _("Rock");
        case OBJ_FLOWERS:		return _("Flowers");
        case OBJ_WATERLAKE:		return _("Water Lake");
        case OBJ_MANDRAKE:		return _("Mandrake");
        case OBJ_CRATER:		return _("Crater");
        case OBJ_LAVAPOOL:		return _("Lava Pool");
        case OBJ_SHRUB:			return _("Shrub");
        case OBJ_BUOY:			return _("Buoy");
        case OBJN_SKELETON:
        case OBJ_SKELETON:		return _("Skeleton");
        case OBJ_TREASURECHEST:		return _("Treasure Chest");
        case OBJ_WATERCHEST:		return _("Sea Chest");
        case OBJ_CAMPFIRE:		return _("Campfire");
        case OBJ_FOUNTAIN:		return _("Fountain");
        case OBJ_ANCIENTLAMP:		return _("Genie Lamp");
        case OBJ_GOBLINHUT:		return _("Goblin Hut");
        case OBJ_THATCHEDHUT:		return _("Thatched Hut");
        case OBJ_MONSTER:		return _("Monster");
        case OBJ_RESOURCE:		return _("Resource");
        case OBJ_WHIRLPOOL:		return _("Whirlpool");
        case OBJ_ARTIFACT:		return _("Artifact");
        case OBJ_BOAT:			return _("Boat");
        case OBJ_RNDARTIFACT:		return "Random Artifact";
        case OBJ_RNDRESOURCE:		return "Random Resource";
        case OBJ_RNDMONSTER1:           return "OBJ_RNDMONSTER1";
        case OBJ_RNDMONSTER2:           return "OBJ_RNDMONSTER2";
        case OBJ_RNDMONSTER3:           return "OBJ_RNDMONSTER3";
        case OBJ_RNDMONSTER4:           return "OBJ_RNDMONSTER4";
        case OBJ_STANDINGSTONES:        return _("Standing Stones");
        case OBJ_EVENT:                 return "OBJ_EVENT";
        case OBJ_RNDMONSTER:            return "OBJ_RNDMONSTER";
        case OBJ_RNDULTIMATEARTIFACT:   return "OBJ_RNDULTIMATEARTIFACT";
        case OBJ_IDOL:                  return _("Idol");
        case OBJ_SHRINE1:               return _("Shrine of the First Circle");
        case OBJ_SHRINE2:               return _("Shrine of the Second Circle");
        case OBJ_SHRINE3:               return _("Shrine of the Third Circle");
        case OBJ_WAGON:                 return _("Wagon");
        case OBJ_LEANTO:                return _("Lean To");
        case OBJ_FLOTSAM:               return _("Flotsam");
        case OBJ_SHIPWRECKSURVIROR:     return _("Shipwreck Survivor");
        case OBJ_BOTTLE:                return _("Bottle");
        case OBJ_MAGICGARDEN:           return _("Magic Garden");
        case OBJ_RNDARTIFACT1:          return "OBJ_RNDARTIFACT1";
        case OBJ_RNDARTIFACT2:          return "OBJ_RNDARTIFACT2";
        case OBJ_RNDARTIFACT3:          return "OBJ_RNDARTIFACT3";

	case OBJN_JAIL:
	case OBJ_JAIL:			return _("Jail");
	case OBJN_TRAVELLERTENT:
	case OBJ_TRAVELLERTENT:		return _("Traveller's Tent");
	case OBJ_BARRIER:		return _("Barrier");

	case OBJN_FIREALTAR:
	case OBJ_FIREALTAR:		return _("Fire Summoning Altar");
	case OBJN_AIRALTAR:
	case OBJ_AIRALTAR:		return _("Air Summoning Altar");
	case OBJN_EARTHALTAR:
	case OBJ_EARTHALTAR:		return _("Earth Summoning Altar");
	case OBJN_WATERALTAR:
	case OBJ_WATERALTAR:		return _("Water Summoning Altar");
	case OBJN_BARROWMOUNDS:
	case OBJ_BARROWMOUNDS:		return _("Barrow Mounds");
	case OBJN_ARENA:
	case OBJ_ARENA:			return _("Arena");
	case OBJN_STABLES:
	case OBJ_STABLES:		return _("Stables");
	case OBJN_ALCHEMYTOWER:
	case OBJ_ALCHEMYTOWER:		return _("Alchemist's Tower");
	case OBJN_HUTMAGI:
	case OBJ_HUTMAGI:		return _("Hut of the Magi");
	case OBJN_EYEMAGI:
	case OBJ_EYEMAGI:		return _("Eye of the Magi");
	case OBJN_MERMAID:
	case OBJ_MERMAID:		return _("Mermaid");
	case OBJN_SIRENS:
	case OBJ_SIRENS:		return _("Sirens");
	case OBJ_REEFS:			return _("Reefs");

	case OBJ_UNKNW_02:		return "OBJ_UNKNW_02";
	case OBJ_UNKNW_03:		return "OBJ_UNKNW_03";
	case OBJ_UNKNW_06:		return "OBJ_UNKNW_06";
	case OBJ_UNKNW_08:		return "OBJ_UNKNW_08";
	case OBJ_UNKNW_09:		return "OBJ_UNKNW_09";
	case OBJ_UNKNW_0B:		return "OBJ_UNKNW_0B";
	case OBJ_UNKNW_0E:		return "OBJ_UNKNW_0E";
	case OBJ_UNKNW_11:		return "OBJ_UNKNW_11";
	case OBJ_UNKNW_12:		return "OBJ_UNKNW_12";
	case OBJ_UNKNW_13:		return "OBJ_UNKNW_13";
	case OBJ_UNKNW_18:		return "OBJ_UNKNW_18";
	case OBJ_UNKNW_1B:		return "OBJ_UNKNW_1B";
	case OBJ_UNKNW_1F:		return "OBJ_UNKNW_1F";
	case OBJ_UNKNW_21:		return "OBJ_UNKNW_21";
	case OBJ_UNKNW_26:		return "OBJ_UNKNW_26";
	case OBJ_UNKNW_27:		return "OBJ_UNKNW_27";
	case OBJ_UNKNW_29:		return "OBJ_UNKNW_29";
	case OBJ_UNKNW_2A:		return "OBJ_UNKNW_2A";
	case OBJ_UNKNW_2B:		return "OBJ_UNKNW_2B";
	case OBJ_UNKNW_2C:		return "OBJ_UNKNW_2C";
	case OBJ_UNKNW_2D:		return "OBJ_UNKNW_2D";
	case OBJ_UNKNW_2E:		return "OBJ_UNKNW_2E";
	case OBJ_UNKNW_2F:		return "OBJ_UNKNW_2F";
	case OBJ_UNKNW_32:		return "OBJ_UNKNW_32";
	case OBJ_UNKNW_33:		return "OBJ_UNKNW_33";
	case OBJ_UNKNW_34:		return "OBJ_UNKNW_34";
	case OBJ_UNKNW_35:		return "OBJ_UNKNW_35";
	case OBJ_UNKNW_36:		return "OBJ_UNKNW_36";
	case OBJ_UNKNW_37:		return "OBJ_UNKNW_37";
	case OBJ_UNKNW_41:		return "OBJ_UNKNW_41";
	case OBJ_UNKNW_42:		return "OBJ_UNKNW_42";
	case OBJ_UNKNW_43:		return "OBJ_UNKNW_43";
	case OBJ_UNKNW_4A:		return "OBJ_UNKNW_4A";
	case OBJ_UNKNW_4B:		return "OBJ_UNKNW_4B";
	case OBJ_UNKNW_50:		return "OBJ_UNKNW_50";
	case OBJ_UNKNW_58:		return "OBJ_UNKNW_58";
	case OBJ_UNKNW_5A:		return "OBJ_UNKNW_5A";
	case OBJ_UNKNW_5C:		return "OBJ_UNKNW_5C";
	case OBJ_UNKNW_5D:		return "OBJ_UNKNW_5D";
	case OBJ_UNKNW_5F:		return "OBJ_UNKNW_5F";
	case OBJ_UNKNW_62:		return "OBJ_UNKNW_62";
	case OBJ_UNKNW_79:		return "OBJ_UNKNW_79";
	case OBJ_UNKNW_7A:		return "OBJ_UNKNW_7A";
	case OBJ_UNKNW_91:		return "OBJ_UNKNW_91";
	case OBJ_UNKNW_92:		return "OBJ_UNKNW_92";
	case OBJ_UNKNW_A1:		return "OBJ_UNKNW_A1";
	case OBJ_UNKNW_A6:		return "OBJ_UNKNW_A6";
	case OBJ_UNKNW_AA:		return "OBJ_UNKNW_AA";
	case OBJ_UNKNW_B2:		return "OBJ_UNKNW_B2";
	case OBJ_UNKNW_B8:		return "OBJ_UNKNW_B8";
	case OBJ_UNKNW_B9:		return "OBJ_UNKNW_B9";
	case OBJ_UNKNW_D1:		return "OBJ_UNKNW_D1";
	case OBJ_UNKNW_E2:		return "OBJ_UNKNW_E2";
	case OBJ_UNKNW_E3:		return "OBJ_UNKNW_E3";
	case OBJ_UNKNW_E4:		return "OBJ_UNKNW_E4";
	case OBJ_UNKNW_E5:		return "OBJ_UNKNW_E5";
	case OBJ_UNKNW_E6:		return "OBJ_UNKNW_E6";
	case OBJ_UNKNW_E7:		return "OBJ_UNKNW_E7";
	case OBJ_UNKNW_E8:		return "OBJ_UNKNW_E8";
	case OBJ_UNKNW_F9:		return "OBJ_UNKNW_F9";
	case OBJ_UNKNW_FA:		return "OBJ_UNKNW_FA";

	default:
	    DEBUG(DBG_GAME, DBG_WARN, "unknown object: " << static_cast<int>(object));
	    break;
    }
    
    return NULL;
}

bool MP2::isDayLife(int obj)
{
    // FIXME: list day object life
    switch(obj)
    {
	case OBJ_MAGICWELL:
	    return true;

	default: break;
    }

    return false;
}

bool MP2::isWeekLife(int obj)
{
    // FIXME: list week object life
    switch(obj)
    {
	case OBJ_STABLES:
        case OBJ_MAGICGARDEN:
        case OBJ_WATERWHEEL:
        case OBJ_WINDMILL:
        case OBJ_ARTESIANSPRING:

	// join army
        case OBJ_WATCHTOWER:
        case OBJ_EXCAVATION:
        case OBJ_CAVE:
        case OBJ_TREEHOUSE:
        case OBJ_ARCHERHOUSE:
        case OBJ_GOBLINHUT:
        case OBJ_DWARFCOTT:
        case OBJ_HALFLINGHOLE:
        case OBJ_PEASANTHUT:
        case OBJ_THATCHEDHUT:

        // recruit army
        case OBJ_RUINS:
        case OBJ_TREECITY:
        case OBJ_WAGONCAMP:
        case OBJ_DESERTTENT:
        case OBJ_WATERALTAR:
        case OBJ_AIRALTAR:
        case OBJ_FIREALTAR:
        case OBJ_EARTHALTAR:
        case OBJ_BARROWMOUNDS:

        // battle and recruit army
        case OBJ_DRAGONCITY:
        case OBJ_CITYDEAD:
        case OBJ_TROLLBRIDGE:

	// for AI
	case OBJ_COAST:
	// for AI
        case OBJ_CASTLE:

	    return true;

	default: break;
    }

    return false;
}

bool MP2::isMonthLife(int obj)
{
    // FIXME: list month object life
    switch(obj)
    {
	default: break;
    }

    return false;
}

bool MP2::isBattleLife(int obj)
{
    // FIXME: list battle object life
    switch(obj)
    {
	// luck modificators
	case OBJ_IDOL:
	case OBJ_FOUNTAIN:
	case OBJ_FAERIERING:
	case OBJ_PYRAMID:

	// morale modificators
	case OBJ_BUOY:
	case OBJ_OASIS:
	case OBJ_TEMPLE:
	case OBJ_WATERINGHOLE:
	case OBJ_GRAVEYARD:
        case OBJ_DERELICTSHIP:
        case OBJ_SHIPWRECK:
	case OBJ_MERMAID:

	    return true;

	default: break;
    }

    return false;
}

bool MP2::isActionObject(int obj, bool water)
{
    if(water) return isWaterObject(obj);

    return isGroundObject(obj);
}

bool MP2::isWaterObject(int obj)
{
    switch(obj)
    {
    	    case OBJ_WATERCHEST:
	    case OBJ_DERELICTSHIP:
	    case OBJ_SHIPWRECK:
	    case OBJ_WHIRLPOOL:
	    case OBJ_BUOY:
	    case OBJ_BOTTLE:
	    case OBJ_SHIPWRECKSURVIROR:
	    case OBJ_FLOTSAM:
	    case OBJ_MAGELLANMAPS:
	    case OBJ_COAST:

    	    case OBJ_MERMAID:
    	    case OBJ_SIRENS:
    	    case OBJ_BARRIER:

    	    // hack (bug: #3142729)
	    case OBJ_MONSTER:
	    case OBJ_ARTIFACT:
	    case OBJ_RESOURCE:

            return true;

    	    case OBJ_CASTLE:
    	    case OBJ_BOAT:
            return false;

        default: break;
    }

    // price loyalty: editor allow place other objects
    return Settings::Get().PriceLoyaltyVersion() ? isGroundObject(obj) : false;
}

bool MP2::isGroundObject(int obj)
{
    switch(obj)
    {
    	    case OBJ_TREASURECHEST:
    	    case OBJ_ALCHEMYLAB:
    	    case OBJ_SIGN:
    	    case OBJ_SKELETON:
    	    case OBJ_DAEMONCAVE:
    	    case OBJ_FAERIERING:
    	    case OBJ_CAMPFIRE:
    	    case OBJ_FOUNTAIN:
    	    case OBJ_GAZEBO:
    	    case OBJ_ANCIENTLAMP:
    	    case OBJ_GRAVEYARD:
    	    case OBJ_ARCHERHOUSE:
    	    case OBJ_GOBLINHUT:
    	    case OBJ_DWARFCOTT:
    	    case OBJ_PEASANTHUT:
    	    case OBJ_THATCHEDHUT:
    	    case OBJ_DRAGONCITY:
    	    case OBJ_LIGHTHOUSE:
    	    case OBJ_WATERWHEEL:
    	    case OBJ_MINES:
	    case OBJ_OBELISK:
	    case OBJ_OASIS:
	    case OBJ_RESOURCE:
	    case OBJ_SAWMILL:
	    case OBJ_ORACLE:
	    case OBJ_SHIPWRECK:
	    case OBJ_DESERTTENT:
	    case OBJ_STONELIGHTS:
	    case OBJ_WAGONCAMP:
	    case OBJ_WINDMILL:
	    case OBJ_ARTIFACT:
	    case OBJ_WATCHTOWER:
	    case OBJ_TREEHOUSE:
	    case OBJ_TREECITY:
	    case OBJ_RUINS:
	    case OBJ_FORT:
    	    case OBJ_TRADINGPOST:
    	    case OBJ_ABANDONEDMINE:
    	    case OBJ_STANDINGSTONES:
    	    case OBJ_IDOL:
    	    case OBJ_TREEKNOWLEDGE:
    	    case OBJ_DOCTORHUT:
    	    case OBJ_TEMPLE:
    	    case OBJ_HILLFORT:
    	    case OBJ_HALFLINGHOLE:
    	    case OBJ_MERCENARYCAMP:
    	    case OBJ_WATERINGHOLE:
	    case OBJ_SHRINE1:
    	    case OBJ_SHRINE2:
    	    case OBJ_SHRINE3:
    	    case OBJ_PYRAMID:
    	    case OBJ_CITYDEAD:
    	    case OBJ_EXCAVATION:
    	    case OBJ_SPHINX:
    	    case OBJ_WAGON:
    	    case OBJ_ARTESIANSPRING:
    	    case OBJ_TROLLBRIDGE:
    	    case OBJ_WITCHSHUT:
    	    case OBJ_XANADU:
    	    case OBJ_CAVE:
    	    case OBJ_LEANTO:
    	    case OBJ_MAGICWELL:
    	    case OBJ_MAGICGARDEN:
    	    case OBJ_OBSERVATIONTOWER:
    	    case OBJ_FREEMANFOUNDRY:
    	    
    	    case OBJ_MONSTER:
    	    case OBJ_CASTLE:
    	    case OBJ_BOAT:

	    case OBJ_BARRIER:
	    case OBJ_TRAVELLERTENT:
	    case OBJ_FIREALTAR:
	    case OBJ_AIRALTAR:
	    case OBJ_EARTHALTAR:
	    case OBJ_WATERALTAR:
	    case OBJ_BARROWMOUNDS:
	    case OBJ_ARENA:
	    case OBJ_JAIL:
	    case OBJ_STABLES:
	    case OBJ_ALCHEMYTOWER:
	    case OBJ_HUTMAGI:
	    case OBJ_EYEMAGI:
		return true;

	    default: break;
    }

    return false;
}

bool MP2::isQuantityObject(int obj)
{
    switch(obj)
    {
        case OBJ_SKELETON:
        case OBJ_WAGON:
        case OBJ_ARTIFACT:
        case OBJ_RESOURCE:
        case OBJ_MAGICGARDEN:
        case OBJ_WATERWHEEL:
        case OBJ_WINDMILL:
        case OBJ_LEANTO:
        case OBJ_CAMPFIRE:
        case OBJ_FLOTSAM:
        case OBJ_SHIPWRECKSURVIROR:
        case OBJ_TREASURECHEST:
        case OBJ_WATERCHEST:
        case OBJ_DERELICTSHIP:
        case OBJ_SHIPWRECK:
        case OBJ_GRAVEYARD:
        case OBJ_PYRAMID:
        case OBJ_DAEMONCAVE:
        case OBJ_ABANDONEDMINE:
            return true;

        default: break;
    }

    if(isPickupObject(obj)) return true;

    return false;
}

bool MP2::isCaptureObject(int obj)
{
    switch(obj)
    {
        case OBJ_MINES:
	case OBJ_ABANDONEDMINE:
        case OBJ_ALCHEMYLAB:
        case OBJ_SAWMILL:
        case OBJ_LIGHTHOUSE:
        case OBJ_CASTLE:
	    return true;

        case OBJ_WATERWHEEL:
        case OBJ_WINDMILL:
        case OBJ_MAGICGARDEN:
           return Settings::Get().ExtWorldExtObjectsCaptured();

	default: break;
    }

    return false;
}

bool MP2::isPickupObject(int obj)
{
    switch(obj)
    {
	case OBJ_WATERCHEST:
	case OBJ_SHIPWRECKSURVIROR:
        case OBJ_FLOTSAM:
        case OBJ_BOTTLE:
        case OBJ_TREASURECHEST:
        case OBJ_ANCIENTLAMP:
	case OBJ_CAMPFIRE:
        case OBJ_RESOURCE:
        case OBJ_ARTIFACT:
	    return true;

	default: break;
    }

    return false;
}

bool MP2::isMoveObject(int obj)
{
    switch(obj)
    {
	    case OBJ_STONELIGHTS:
	    case OBJ_WHIRLPOOL:
	    return true;

	default: break;
    }

    return false;
}

bool MP2::isRemoveObject(int obj)
{
    switch(obj)
    {
        case OBJ_MONSTER:
        case OBJ_BARRIER:
	    return true;

	default: break;
    }

    return isPickupObject(obj);
}

bool MP2::isNeedStayFront(int obj)
{
    switch(obj)
    {
        case OBJ_WATERCHEST:
        case OBJ_SHIPWRECKSURVIROR:
        case OBJ_FLOTSAM:
        case OBJ_BOTTLE:
        case OBJ_TREASURECHEST:
        case OBJ_ANCIENTLAMP:
        case OBJ_CAMPFIRE:
        case OBJ_MONSTER:
        case OBJ_RESOURCE:
        case OBJ_ARTIFACT:
        case OBJ_HEROES:
        case OBJ_BOAT:
        case OBJ_BARRIER:
        case OBJ_JAIL:
	case OBJ_SHIPWRECK:
	case OBJ_BUOY:
	    return true;

	default: break;
    }

    return false;
}



bool MP2::isClearGroundObject(int obj)
{
    switch(obj)
    {
	case OBJ_ZERO:
	case OBJ_COAST:
	    return true;

	default: break;
    }

    return false;
}

int MP2::GetObjectDirect(int obj)
{
    switch(obj)
    {
        case OBJ_JAIL:
        case OBJ_BARRIER:
	    return DIRECTION_ALL;

        case OBJ_SHIPWRECK:
            return Direction::CENTER | Direction::LEFT | DIRECTION_BOTTOM_ROW;

        case OBJ_DERELICTSHIP:
        case OBJ_TROLLBRIDGE:
        case OBJ_ARCHERHOUSE:
        case OBJ_DOCTORHUT:
        case OBJ_DWARFCOTT:
        case OBJ_THATCHEDHUT:
        case OBJ_FOUNTAIN:
        case OBJ_IDOL:
        case OBJ_LIGHTHOUSE:
        case OBJ_OBELISK:
        case OBJ_SIGN:
        case OBJ_WATCHTOWER:
        case OBJ_WITCHSHUT:
        case OBJ_GAZEBO:
        case OBJ_MAGICWELL:
        case OBJ_OBSERVATIONTOWER:
        case OBJ_PEASANTHUT:
        case OBJ_STONELIGHTS:
        case OBJ_STANDINGSTONES:
        case OBJ_GOBLINHUT:
        case OBJ_SHRINE1:
        case OBJ_SHRINE2:
        case OBJ_SHRINE3:
        case OBJ_TREEHOUSE:
        case OBJ_ARTESIANSPRING:
        case OBJ_SKELETON:
        case OBJ_TREEKNOWLEDGE:
        case OBJ_ORACLE:
        case OBJ_OASIS:
        case OBJ_LEANTO:
        case OBJ_MAGICGARDEN:
        case OBJ_WAGON:
        case OBJ_TRAVELLERTENT:
        case OBJ_ALCHEMYTOWER:
        case OBJ_HUTMAGI:
        case OBJ_EYEMAGI:
        case OBJ_MERCENARYCAMP:
        case OBJ_WINDMILL:
        case OBJ_WATERINGHOLE:
        case OBJ_TRADINGPOST:
        case OBJ_EXCAVATION:
        case OBJ_DESERTTENT:
        case OBJ_DAEMONCAVE:
        case OBJ_PYRAMID:
        case OBJ_FORT:
        case OBJ_RUINS:
        case OBJ_HILLFORT:
        case OBJ_FREEMANFOUNDRY:
        case OBJ_SAWMILL:
        case OBJ_TREECITY:
        case OBJ_SPHINX:
        case OBJ_TEMPLE:
        case OBJ_FAERIERING:
        case OBJ_BARROWMOUNDS:
        case OBJ_STABLES:
        case OBJ_ABANDONEDMINE:
        case OBJ_MINES:
        case OBJ_ALCHEMYLAB:
        case OBJ_CAVE:
        case OBJ_CITYDEAD:
        case OBJ_GRAVEYARD:
        case OBJ_DRAGONCITY:
        case OBJ_XANADU:
        case OBJ_HALFLINGHOLE:
        case OBJ_WAGONCAMP:
        case OBJ_WATERALTAR:
        case OBJ_AIRALTAR:
        case OBJ_FIREALTAR:
        case OBJ_EARTHALTAR:
        case OBJ_ARENA:
        case OBJ_SIRENS:
        case OBJ_MERMAID:
            return DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW;

        case OBJ_WATERWHEEL:
            return Direction::CENTER | Direction::RIGHT | DIRECTION_BOTTOM_ROW;

        case OBJ_MAGELLANMAPS:
            return Direction::CENTER | Direction::LEFT | DIRECTION_BOTTOM_ROW;

        case OBJ_CASTLE:
            return Direction::CENTER | Direction::BOTTOM;

        default: break;
    }

    return DIRECTION_ALL;
}
