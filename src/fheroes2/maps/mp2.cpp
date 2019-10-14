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

#include <map>
#include <set>

#include "settings.h"
#include "direction.h"
#include "icn.h"
#include "mp2.h"

using namespace MP2;

const std::map<int, int> ICNObjects {
	// reserverd
    {0, ICN::UNKNOWN},

	// manual
	{ 0x11, ICN::TELEPORT1 },
	{ 0x12, ICN::TELEPORT2 },
	{ 0x13, ICN::TELEPORT3 },
	{ 0x14, ICN::FOUNTAIN },
	{ 0x15, ICN::TREASURE },
    
    // artifact
	{ 0x2C, ICN::OBJNARTI },
	{ 0x2D, ICN::OBJNARTI },
	{ 0x2E, ICN::OBJNARTI },
	{ 0x2F, ICN::OBJNARTI },

	// monster
	{ 0x30, ICN::MONS32 },
	{ 0x31, ICN::MONS32 },
	{ 0x32, ICN::MONS32 },
	{ 0x33, ICN::MONS32 },

	// castle flags
	{ 0x38, ICN::FLAG32 },
	{ 0x39, ICN::FLAG32 },
	{ 0x3A, ICN::FLAG32 },
	{ 0x3B, ICN::FLAG32 },

	// heroes
	{ 0x54, ICN::MINIHERO },
	{ 0x55, ICN::MINIHERO },
	{ 0x56, ICN::MINIHERO },
	{ 0x57, ICN::MINIHERO },

	// relief: snow
	{ 0x58, ICN::MTNSNOW },
	{ 0x59, ICN::MTNSNOW },
	{ 0x5A, ICN::MTNSNOW },
	{ 0x5B, ICN::MTNSNOW },

	// relief: swamp
	{ 0x5C, ICN::MTNSWMP },
	{ 0x5D, ICN::MTNSWMP },
	{ 0x5E, ICN::MTNSWMP },
	{ 0x5F, ICN::MTNSWMP },

	// relief: lava
	{ 0x60, ICN::MTNLAVA },
	{ 0x61, ICN::MTNLAVA },
	{ 0x62, ICN::MTNLAVA },
	{ 0x63, ICN::MTNLAVA },

	// relief: desert
	{ 0x64, ICN::MTNDSRT },
	{ 0x65, ICN::MTNDSRT },
	{ 0x66, ICN::MTNDSRT },
	{ 0x67, ICN::MTNDSRT },

	// relief: dirt
	{ 0x68, ICN::MTNDIRT },
	{ 0x69, ICN::MTNDIRT },
	{ 0x6A, ICN::MTNDIRT },
	{ 0x6B, ICN::MTNDIRT },

	// relief: others
	{ 0x6C, ICN::MTNMULT },
	{ 0x6D, ICN::MTNMULT },
	{ 0x6E, ICN::MTNMULT },
	{ 0x6F, ICN::MTNMULT },

	// mines
	{ 0x74, ICN::EXTRAOVR },

	// road
	{ 0x78, ICN::ROAD },
	{ 0x79, ICN::ROAD },
	{ 0x7A, ICN::ROAD },
	{ 0x7B, ICN::ROAD },

	// relief: crck
	{ 0x7C, ICN::MTNCRCK },
	{ 0x7D, ICN::MTNCRCK },
	{ 0x7E, ICN::MTNCRCK },
	{ 0x7F, ICN::MTNCRCK },

	// relief: gras
	{ 0x80, ICN::MTNGRAS },
	{ 0x81, ICN::MTNGRAS },
	{ 0x82, ICN::MTNGRAS },
	{ 0x83, ICN::MTNGRAS },

	// trees jungle
	{ 0x84, ICN::TREJNGL },
	{ 0x85, ICN::TREJNGL },
	{ 0x86, ICN::TREJNGL },
	{ 0x87, ICN::TREJNGL },

	// trees evil
	{ 0x88, ICN::TREEVIL },
	{ 0x89, ICN::TREEVIL },
	{ 0x8A, ICN::TREEVIL },
	{ 0x8B, ICN::TREEVIL },

	// castle and tower
	{ 0x8C, ICN::OBJNTOWN },
	{ 0x8D, ICN::OBJNTOWN },
	{ 0x8E, ICN::OBJNTOWN },
	{ 0x8F, ICN::OBJNTOWN },

	// castle lands
	{ 0x90, ICN::OBJNTWBA },
	{ 0x91, ICN::OBJNTWBA },
	{ 0x92, ICN::OBJNTWBA },
	{ 0x93, ICN::OBJNTWBA },

	// castle shadow
	{ 0x94, ICN::OBJNTWSH },
	{ 0x95, ICN::OBJNTWSH },
	{ 0x96, ICN::OBJNTWSH },
	{ 0x97, ICN::OBJNTWSH },

	// random castle
	{ 0x98, ICN::OBJNTWRD },
	{ 0x99, ICN::OBJNTWRD },
	{ 0x9A, ICN::OBJNTWRD },
	{ 0x9B, ICN::OBJNTWRD },

	// water object
	{ 0xA0, ICN::OBJNWAT2 },
	{ 0xA1, ICN::OBJNWAT2 },
	{ 0xA2, ICN::OBJNWAT2 },
	{ 0xA3, ICN::OBJNWAT2 },

	// object other
	{ 0xA4, ICN::OBJNMUL2 },
	{ 0xA5, ICN::OBJNMUL2 },
	{ 0xA6, ICN::OBJNMUL2 },
	{ 0xA7, ICN::OBJNMUL2 },

	// trees snow
	{ 0xA8, ICN::TRESNOW },
	{ 0xA9, ICN::TRESNOW },
	{ 0xAA, ICN::TRESNOW },
	{ 0xAB, ICN::TRESNOW },

	// trees trefir
	{ 0xAC, ICN::TREFIR },
	{ 0xAD, ICN::TREFIR },
	{ 0xAE, ICN::TREFIR },
	{ 0xAF, ICN::TREFIR },

	// trees
	{ 0xB0, ICN::TREFALL },
	{ 0xB1, ICN::TREFALL },
	{ 0xB2, ICN::TREFALL },
	{ 0xB3, ICN::TREFALL },

	// river
	{ 0xB4, ICN::STREAM },
	{ 0xB5, ICN::STREAM },
	{ 0xB6, ICN::STREAM },
	{ 0xB7, ICN::STREAM },

	// resource
	{ 0xB8, ICN::OBJNRSRC },
	{ 0xB9, ICN::OBJNRSRC },
	{ 0xBA, ICN::OBJNRSRC },
	{ 0xBB, ICN::OBJNRSRC },

	// gras object
	{ 0xC0, ICN::OBJNGRA2 },
	{ 0xC1, ICN::OBJNGRA2 },
	{ 0xC2, ICN::OBJNGRA2 },
	{ 0xC3, ICN::OBJNGRA2 },

	// trees tredeci
	{ 0xC4, ICN::TREDECI },
	{ 0xC5, ICN::TREDECI },
	{ 0xC6, ICN::TREDECI },
	{ 0xC7, ICN::TREDECI },

	// sea object
	{ 0xC8, ICN::OBJNWATR },
	{ 0xC9, ICN::OBJNWATR },
	{ 0xCA, ICN::OBJNWATR },
	{ 0xCB, ICN::OBJNWATR },

	// vegetation gras
	{ 0xCC, ICN::OBJNGRAS },
	{ 0xCD, ICN::OBJNGRAS },
	{ 0xCE, ICN::OBJNGRAS },
	{ 0xCF, ICN::OBJNGRAS },

	// object on snow
	{ 0xD0, ICN::OBJNSNOW },
	{ 0xD1, ICN::OBJNSNOW },
	{ 0xD2, ICN::OBJNSNOW },
	{ 0xD3, ICN::OBJNSNOW },

	// object on swamp
	{ 0xD4, ICN::OBJNSWMP },
	{ 0xD5, ICN::OBJNSWMP },
	{ 0xD6, ICN::OBJNSWMP },
	{ 0xD7, ICN::OBJNSWMP },

	// object on lava
	{ 0xD8, ICN::OBJNLAVA },
	{ 0xD9, ICN::OBJNLAVA },
	{ 0xDA, ICN::OBJNLAVA },
	{ 0xDB, ICN::OBJNLAVA },

	// object on desert
	{ 0xDC, ICN::OBJNDSRT },
	{ 0xDD, ICN::OBJNDSRT },
	{ 0xDE, ICN::OBJNDSRT },
	{ 0xDF, ICN::OBJNDSRT },

	// object on dirt
	{ 0xE0, ICN::OBJNDIRT },
	{ 0xE1, ICN::OBJNDIRT },
	{ 0xE2, ICN::OBJNDIRT },
	{ 0xE3, ICN::OBJNDIRT },

	// object on crck
	{ 0xE4, ICN::OBJNCRCK },
	{ 0xE5, ICN::OBJNCRCK },
	{ 0xE6, ICN::OBJNCRCK },
	{ 0xE7, ICN::OBJNCRCK },

	// object on lava
	{ 0xE8, ICN::OBJNLAV3 },
	{ 0xE9, ICN::OBJNLAV3 },
	{ 0xEA, ICN::OBJNLAV3 },
	{ 0xEB, ICN::OBJNLAV3 },

	// object on earth
	{ 0xEC, ICN::OBJNMULT },
	{ 0xED, ICN::OBJNMULT },
	{ 0xEE, ICN::OBJNMULT },
	{ 0xEF, ICN::OBJNMULT },

	// object on lava
	{ 0xF0, ICN::OBJNLAV2 },
	{ 0xF1, ICN::OBJNLAV2 },
	{ 0xF2, ICN::OBJNLAV2 },
	{ 0xF3, ICN::OBJNLAV2 },

	// extra objects for loyalty version
	{ 0xF4, ICN::X_LOC1 },
	{ 0xF5, ICN::X_LOC1 },
	{ 0xF6, ICN::X_LOC1 },
	{ 0xF7, ICN::X_LOC1 },

	// extra objects for loyalty version
	{ 0xF8, ICN::X_LOC2 },
	{ 0xF9, ICN::X_LOC2 },
	{ 0xFA, ICN::X_LOC2 },
	{ 0xFB, ICN::X_LOC2 },

	// extra objects for loyalty version
	{ 0xFC, ICN::X_LOC3 },
	{ 0xFD, ICN::X_LOC3 },
	{ 0xFE, ICN::X_LOC3 },
	{ 0xFF, ICN::X_LOC3 },
};

const std::set<int> ICNLoyaltyObjects {
    0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB,
    0xFC, 0xFD, 0xFE, 0xFF,
};

/* return name icn object */
int MP2::GetICNObject(int type)
{
    if (ICNLoyaltyObjects.find(type) != ICNLoyaltyObjects.end())
    {
        if (Settings::Get().PriceLoyaltyVersion())
            return ICNObjects.at(type);
    }
    else
    {
        if (ICNObjects.find(type) != ICNObjects.end())
            return ICNObjects.at(type);
    }

    DEBUG(DBG_GAME, DBG_WARN, "unknown type: " << static_cast<int>(type));

    return ICN::UNKNOWN;
}

const std::map<int, const char*> StringObjects {
	{ OBJ_ZERO,		        "OBJ_ZERO" },
	{ OBJN_ALCHEMYLAB,		_("Alchemist Lab") },
	{ OBJ_ALCHEMYLAB,		_("Alchemist Lab") },
	{ OBJN_DAEMONCAVE,		_("Daemon Cave") },
	{ OBJ_DAEMONCAVE,		_("Daemon Cave") },
	{ OBJN_FAERIERING,		_("Faerie Ring") },
	{ OBJ_FAERIERING,		_("Faerie Ring") },
	{ OBJN_GRAVEYARD,		_("Graveyard") },
	{ OBJ_GRAVEYARD,		_("Graveyard") },
	{ OBJN_DRAGONCITY,		_("Dragon City") },
	{ OBJ_DRAGONCITY,		_("Dragon City") },
	{ OBJN_LIGHTHOUSE,		_("Light House") },
	{ OBJ_LIGHTHOUSE,		_("Light House") },
	{ OBJN_WATERWHEEL,		_("Water Wheel") },
	{ OBJ_WATERWHEEL,		_("Water Wheel") },
	{ OBJN_MINES,		    _("Mines") },
	{ OBJ_MINES,		    _("Mines") },
	{ OBJN_OBELISK,	    	_("Obelisk") },
	{ OBJ_OBELISK,		    _("Obelisk") },
	{ OBJN_OASIS,		    _("Oasis") },
	{ OBJ_OASIS,		    _("Oasis") },
	{ OBJN_SAWMILL,		    _("Sawmill") },
	{ OBJ_SAWMILL,		    _("Sawmill") },
	{ OBJN_ORACLE,		    _("Oracle") },
	{ OBJ_ORACLE,		    _("Oracle") },
	{ OBJN_DESERTTENT,		_("Desert Tent") },
	{ OBJ_DESERTTENT,		_("Desert Tent") },
	{ OBJN_CASTLE,		    _("Castle") },
	{ OBJ_CASTLE,		    _("Castle") },
	{ OBJN_WAGONCAMP,		_("Wagon Camp") },
	{ OBJ_WAGONCAMP,		_("Wagon Camp") },
	{ OBJN_WINDMILL,		_("Windmill") },
	{ OBJ_WINDMILL,		    _("Windmill") },
	{ OBJN_RNDTOWN,		    _("Random Town") },
	{ OBJ_RNDTOWN,		    _("Random Town") },
	{ OBJN_RNDCASTLE,		_("Random Castle") },
	{ OBJ_RNDCASTLE,		_("Random Castle") },
	{ OBJN_WATCHTOWER,		_("Watch Tower") },
	{ OBJ_WATCHTOWER,		_("Watch Tower") },
	{ OBJN_TREECITY,		_("Tree City") },
	{ OBJ_TREECITY,		    _("Tree City") },
	{ OBJN_TREEHOUSE,		_("Tree House") },
	{ OBJ_TREEHOUSE,		_("Tree House") },
	{ OBJN_RUINS,		    _("Ruins") },
	{ OBJ_RUINS,		    _("Ruins") },
	{ OBJN_FORT,		    _("Fort") },
	{ OBJ_FORT,		        _("Fort") },
	{ OBJN_TRADINGPOST,		_("Trading Post") },
	{ OBJ_TRADINGPOST,		_("Trading Post") },
	{ OBJN_ABANDONEDMINE,	_("Abandoned Mine") },
	{ OBJ_ABANDONEDMINE,	_("Abandoned Mine") },
	{ OBJN_TREEKNOWLEDGE,	_("Tree of Knowledge") },
	{ OBJ_TREEKNOWLEDGE,	_("Tree of Knowledge") },
	{ OBJN_DOCTORHUT,		_("Witch Doctor's Hut") },
	{ OBJ_DOCTORHUT,		_("Witch Doctor's Hut") },
	{ OBJN_TEMPLE,		    _("Temple") },
	{ OBJ_TEMPLE,		    _("Temple") },
	{ OBJN_HILLFORT,		_("Hill Fort") },
	{ OBJ_HILLFORT,		    _("Hill Fort") },
	{ OBJN_HALFLINGHOLE,	_("Halfling Hole") },
	{ OBJ_HALFLINGHOLE,		_("Halfling Hole") },
	{ OBJN_MERCENARYCAMP,	_("Mercenary Camp") },
	{ OBJ_MERCENARYCAMP,	_("Mercenary Camp") },
	{ OBJN_PYRAMID,		    _("Pyramid") },
	{ OBJ_PYRAMID,		    _("Pyramid") },
	{ OBJN_CITYDEAD,		_("City of the Dead") },
	{ OBJ_CITYDEAD,		    _("City of the Dead") },
	{ OBJN_EXCAVATION,		_("Excavation") },
	{ OBJ_EXCAVATION,		_("Excavation") },
	{ OBJN_SPHINX,	    	_("Sphinx") },
	{ OBJ_SPHINX,		    _("Sphinx") },
	{ OBJN_TROLLBRIDGE,		_("Troll Bridge") },
	{ OBJ_TROLLBRIDGE,		_("Troll Bridge") },
	{ OBJN_WITCHSHUT,		_("Witch Hut") },
	{ OBJ_WITCHSHUT,		_("Witch Hut") },
	{ OBJN_XANADU,		    _("Xanadu") },
	{ OBJ_XANADU,		    _("Xanadu") },
	{ OBJN_CAVE,	    	_("Cave") },
	{ OBJ_CAVE,		        _("Cave") },
	{ OBJN_MAGELLANMAPS,	_("Magellan Maps") },
	{ OBJ_MAGELLANMAPS,		_("Magellan Maps") },
	{ OBJN_DERELICTSHIP,	_("Derelict Ship") },
	{ OBJ_DERELICTSHIP,		_("Derelict Ship") },
	{ OBJN_SHIPWRECK,		_("Ship Wreck") },
	{ OBJ_SHIPWRECK,		_("Ship Wreck") },
	{ OBJN_OBSERVATIONTOWER,_("Observation Tower") },
	{ OBJ_OBSERVATIONTOWER,	_("Observation Tower") },
	{ OBJN_FREEMANFOUNDRY,	_("Freeman Foundry") },
	{ OBJ_FREEMANFOUNDRY,	_("Freeman Foundry") },
	{ OBJN_WATERINGHOLE,	_("Watering Hole") },
	{ OBJ_WATERINGHOLE,		_("Watering Hole") },
	{ OBJN_ARTESIANSPRING,	_("Artesian Spring") },
	{ OBJ_ARTESIANSPRING,	_("Artesian Spring") },
	{ OBJN_GAZEBO,	    	_("Gazebo") },
	{ OBJ_GAZEBO,		    _("Gazebo") },
	{ OBJN_ARCHERHOUSE,		_("Archer's House") },
	{ OBJ_ARCHERHOUSE,		_("Archer's House") },
	{ OBJN_PEASANTHUT,		_("Peasant Hut") },
	{ OBJ_PEASANTHUT,		_("Peasant Hut") },
	{ OBJN_DWARFCOTT,		_("Dwarf Cottage") },
	{ OBJ_DWARFCOTT,		_("Dwarf Cottage") },
	{ OBJN_STONELIGHTS,		_("Stone Liths") },
	{ OBJ_STONELIGHTS,		_("Stone Liths") }, // https://sourceforge.net/projects/fheroes2/forums/forum/335991/topic/4605429
	{ OBJN_MAGICWELL,		_("Magic Well") },
	{ OBJ_MAGICWELL,		_("Magic Well") },
	{ OBJ_HEROES,		    _("Heroes") },
	{ OBJ_SIGN,		        _("Sign") },
	{ OBJ_SHRUB2,		    _("Shrub") },
	{ OBJ_NOTHINGSPECIAL,	_("Nothing Special") },
	{ OBJ_TARPIT,		    _("Tar Pit") },
	{ OBJ_COAST,		    _("Coast") },
	{ OBJ_MOUND,		    _("Mound") },
	{ OBJ_DUNE,		        _("Dune") },
	{ OBJ_STUMP,	    	_("Stump") },
	{ OBJ_CACTUS,   		_("Cactus") },
	{ OBJ_TREES,	    	_("Trees") },
	{ OBJ_DEADTREE,	    	_("Dead Tree") },
	{ OBJ_MOUNTS,	    	_("Mountains") },
	{ OBJ_VOLCANO,	    	_("Volcano") },
	{ OBJ_STONES,	    	_("Rock") },
	{ OBJ_FLOWERS,	    	_("Flowers") },
	{ OBJ_WATERLAKE,		_("Water Lake") },
	{ OBJ_MANDRAKE,	    	_("Mandrake") },
	{ OBJ_CRATER,	    	_("Crater") },
	{ OBJ_LAVAPOOL,	    	_("Lava Pool") },
	{ OBJ_SHRUB,	    	_("Shrub") },
	{ OBJ_BUOY,		        _("Buoy") },
	{ OBJN_SKELETON,		_("Skeleton") },
	{ OBJ_SKELETON,		    _("Skeleton") },
	{ OBJ_TREASURECHEST,	_("Treasure Chest") },
	{ OBJ_WATERCHEST,		_("Sea Chest") },
	{ OBJ_CAMPFIRE,		    _("Campfire") },
	{ OBJ_FOUNTAIN,		    _("Fountain") },
	{ OBJ_ANCIENTLAMP,		_("Genie Lamp") },
	{ OBJ_GOBLINHUT,		_("Goblin Hut") },
	{ OBJ_THATCHEDHUT,		_("Thatched Hut") },
	{ OBJ_MONSTER,	    	_("Monster") },
	{ OBJ_RESOURCE,		    _("Resource") },
	{ OBJ_WHIRLPOOL,		_("Whirlpool") },
	{ OBJ_ARTIFACT,		    _("Artifact") },
	{ OBJ_BOAT,		        _("Boat") },
	{ OBJ_RNDARTIFACT,		"Random Artifact" },
	{ OBJ_RNDRESOURCE,		"Random Resource" },
	{ OBJ_RNDMONSTER1,		"OBJ_RNDMONSTER1" },
	{ OBJ_RNDMONSTER2,		"OBJ_RNDMONSTER2" },
	{ OBJ_RNDMONSTER3,		"OBJ_RNDMONSTER3" },
	{ OBJ_RNDMONSTER4,		"OBJ_RNDMONSTER4" },
	{ OBJ_STANDINGSTONES,	_("Standing Stones") },
	{ OBJ_EVENT,		    "OBJ_EVENT" },
	{ OBJ_RNDMONSTER,		"OBJ_RNDMONSTER" },
	{ OBJ_RNDULTIMATEARTIFACT,"OBJ_RNDULTIMATEARTIFACT" },
	{ OBJ_IDOL,		        _("Idol") },
	{ OBJ_SHRINE1,		    _("Shrine of the First Circle") },
	{ OBJ_SHRINE2,  		_("Shrine of the Second Circle") },
	{ OBJ_SHRINE3,	    	_("Shrine of the Third Circle") },
	{ OBJ_WAGON,	    	_("Wagon") },
	{ OBJ_LEANTO,	    	_("Lean To") },
	{ OBJ_FLOTSAM,	    	_("Flotsam") },
	{ OBJ_SHIPWRECKSURVIROR,_("Shipwreck Survivor") },
	{ OBJ_BOTTLE,		    _("Bottle") },
	{ OBJ_MAGICGARDEN,		_("Magic Garden") },
	{ OBJ_RNDARTIFACT1,		"OBJ_RNDARTIFACT1" },
	{ OBJ_RNDARTIFACT2,		"OBJ_RNDARTIFACT2" },
	{ OBJ_RNDARTIFACT3,		"OBJ_RNDARTIFACT3" },
	{ OBJN_JAIL,		    _("Jail") },
	{ OBJ_JAIL,		        _("Jail") },
	{ OBJN_TRAVELLERTENT,	_("Traveller's Tent") },
	{ OBJ_TRAVELLERTENT,	_("Traveller's Tent") },
	{ OBJ_BARRIER,		    _("Barrier") },
	{ OBJN_FIREALTAR,		_("Fire Summoning Altar") },
	{ OBJ_FIREALTAR,		_("Fire Summoning Altar") },
	{ OBJN_AIRALTAR,		_("Air Summoning Altar") },
	{ OBJ_AIRALTAR,		    _("Air Summoning Altar") },
	{ OBJN_EARTHALTAR,		_("Earth Summoning Altar") },
	{ OBJ_EARTHALTAR,		_("Earth Summoning Altar") },
	{ OBJN_WATERALTAR,		_("Water Summoning Altar") },
	{ OBJ_WATERALTAR,		_("Water Summoning Altar") },
	{ OBJN_BARROWMOUNDS,	_("Barrow Mounds") },
	{ OBJ_BARROWMOUNDS,		_("Barrow Mounds") },
	{ OBJN_ARENA,		    _("Arena") },
	{ OBJ_ARENA,		    _("Arena") },
	{ OBJN_STABLES,		    _("Stables") },
	{ OBJ_STABLES,		    _("Stables") },
	{ OBJN_ALCHEMYTOWER,	_("Alchemist's Tower") },
	{ OBJ_ALCHEMYTOWER,		_("Alchemist's Tower") },
	{ OBJN_HUTMAGI,	    	_("Hut of the Magi") },
	{ OBJ_HUTMAGI,		    _("Hut of the Magi") },
	{ OBJN_EYEMAGI,		    _("Eye of the Magi") },
	{ OBJ_EYEMAGI,		    _("Eye of the Magi") },
	{ OBJN_MERMAID,		    _("Mermaid") },
	{ OBJ_MERMAID,		    _("Mermaid") },
	{ OBJN_SIRENS,		    _("Sirens") },
	{ OBJ_SIRENS,		    _("Sirens") },
	{ OBJ_REEFS,		    _("Reefs") },
	{ OBJ_UNKNW_02, 		"OBJ_UNKNW_02" },
	{ OBJ_UNKNW_03,	    	"OBJ_UNKNW_03" },
	{ OBJ_UNKNW_06,	    	"OBJ_UNKNW_06" },
	{ OBJ_UNKNW_08,	    	"OBJ_UNKNW_08" },
	{ OBJ_UNKNW_09,	    	"OBJ_UNKNW_09" },
	{ OBJ_UNKNW_0B,	    	"OBJ_UNKNW_0B" },
	{ OBJ_UNKNW_0E,	    	"OBJ_UNKNW_0E" },
	{ OBJ_UNKNW_11,	    	"OBJ_UNKNW_11" },
	{ OBJ_UNKNW_12,	    	"OBJ_UNKNW_12" },
	{ OBJ_UNKNW_13,	    	"OBJ_UNKNW_13" },
	{ OBJ_UNKNW_18,	    	"OBJ_UNKNW_18" },
	{ OBJ_UNKNW_1B,	    	"OBJ_UNKNW_1B" },
	{ OBJ_UNKNW_1F,	    	"OBJ_UNKNW_1F" },
	{ OBJ_UNKNW_21,	    	"OBJ_UNKNW_21" },
	{ OBJ_UNKNW_26,	    	"OBJ_UNKNW_26" },
	{ OBJ_UNKNW_27,	    	"OBJ_UNKNW_27" },
	{ OBJ_UNKNW_29,	    	"OBJ_UNKNW_29" },
	{ OBJ_UNKNW_2A,	    	"OBJ_UNKNW_2A" },
	{ OBJ_UNKNW_2B,	    	"OBJ_UNKNW_2B" },
	{ OBJ_UNKNW_2C,	    	"OBJ_UNKNW_2C" },
	{ OBJ_UNKNW_2D, 		"OBJ_UNKNW_2D" },
	{ OBJ_UNKNW_2E,	    	"OBJ_UNKNW_2E" },
	{ OBJ_UNKNW_2F,	    	"OBJ_UNKNW_2F" },
	{ OBJ_UNKNW_32,	    	"OBJ_UNKNW_32" },
	{ OBJ_UNKNW_33,	    	"OBJ_UNKNW_33" },
	{ OBJ_UNKNW_34,	    	"OBJ_UNKNW_34" },
	{ OBJ_UNKNW_35,	    	"OBJ_UNKNW_35" },
	{ OBJ_UNKNW_36,	    	"OBJ_UNKNW_36" },
	{ OBJ_UNKNW_37,	    	"OBJ_UNKNW_37" },
	{ OBJ_UNKNW_41,	    	"OBJ_UNKNW_41" },
	{ OBJ_UNKNW_42,	    	"OBJ_UNKNW_42" },
	{ OBJ_UNKNW_43,	    	"OBJ_UNKNW_43" },
	{ OBJ_UNKNW_4A,	    	"OBJ_UNKNW_4A" },
	{ OBJ_UNKNW_4B,	    	"OBJ_UNKNW_4B" },
	{ OBJ_UNKNW_50,	    	"OBJ_UNKNW_50" },
	{ OBJ_UNKNW_58,	    	"OBJ_UNKNW_58" },
	{ OBJ_UNKNW_5A,	    	"OBJ_UNKNW_5A" },
	{ OBJ_UNKNW_5C,	    	"OBJ_UNKNW_5C" },
	{ OBJ_UNKNW_5D,	    	"OBJ_UNKNW_5D" },
	{ OBJ_UNKNW_5F, 		"OBJ_UNKNW_5F" },
	{ OBJ_UNKNW_62, 		"OBJ_UNKNW_62" },
	{ OBJ_UNKNW_79,  		"OBJ_UNKNW_79" },
	{ OBJ_UNKNW_7A, 		"OBJ_UNKNW_7A" },
	{ OBJ_UNKNW_91, 		"OBJ_UNKNW_91" },
	{ OBJ_UNKNW_92, 		"OBJ_UNKNW_92" },
	{ OBJ_UNKNW_A1, 		"OBJ_UNKNW_A1" },
	{ OBJ_UNKNW_A6, 		"OBJ_UNKNW_A6" },
	{ OBJ_UNKNW_AA, 		"OBJ_UNKNW_AA" },
	{ OBJ_UNKNW_B2, 		"OBJ_UNKNW_B2" },
	{ OBJ_UNKNW_B8, 		"OBJ_UNKNW_B8" },
	{ OBJ_UNKNW_B9, 		"OBJ_UNKNW_B9" },
	{ OBJ_UNKNW_D1, 		"OBJ_UNKNW_D1" },
	{ OBJ_UNKNW_E2, 		"OBJ_UNKNW_E2" },
	{ OBJ_UNKNW_E3, 		"OBJ_UNKNW_E3" },
	{ OBJ_UNKNW_E4, 		"OBJ_UNKNW_E4" },
	{ OBJ_UNKNW_E5, 		"OBJ_UNKNW_E5" },
	{ OBJ_UNKNW_E6, 		"OBJ_UNKNW_E6" },
	{ OBJ_UNKNW_E7, 		"OBJ_UNKNW_E7" },
	{ OBJ_UNKNW_E8, 		"OBJ_UNKNW_E8" },
	{ OBJ_UNKNW_F9, 		"OBJ_UNKNW_F9" },
	{ OBJ_UNKNW_FA, 		"OBJ_UNKNW_FA" },
};

const char* MP2::StringObject(int object)
{
    if (StringObjects.find(object) != StringObjects.end())
        return StringObjects.at(object);

	DEBUG(DBG_GAME, DBG_WARN, "unknown object: " << static_cast<int>(object));
    return NULL;
}

const std::set<int> DayLifeObjects {
    // FIXME: list day object life

    OBJ_MAGICWELL,
};

bool MP2::isDayLife(int obj)
{
    return DayLifeObjects.find(obj) != DayLifeObjects.end();
}

const std::set<int> WeekLifeObjects {
	OBJ_STABLES,
	OBJ_MAGICGARDEN,
	OBJ_WATERWHEEL,
	OBJ_WINDMILL,
	OBJ_ARTESIANSPRING,

	// join army
	OBJ_WATCHTOWER,
	OBJ_EXCAVATION,
	OBJ_CAVE,
	OBJ_TREEHOUSE,
	OBJ_ARCHERHOUSE,
	OBJ_GOBLINHUT,
	OBJ_DWARFCOTT,
	OBJ_HALFLINGHOLE,
	OBJ_PEASANTHUT,
	OBJ_THATCHEDHUT,

    // recruit army
	OBJ_RUINS,
	OBJ_TREECITY,
	OBJ_WAGONCAMP,
	OBJ_DESERTTENT,
	OBJ_WATERALTAR,
	OBJ_AIRALTAR,
	OBJ_FIREALTAR,
	OBJ_EARTHALTAR,
	OBJ_BARROWMOUNDS,

    // battle and recruit army
	OBJ_DRAGONCITY,
	OBJ_CITYDEAD,
	OBJ_TROLLBRIDGE,
	// for AI
	OBJ_COAST,
	// for AI
	OBJ_CASTLE,
};

bool MP2::isWeekLife(int obj)
{
    return WeekLifeObjects.find(obj) != WeekLifeObjects.end();
}

const std::set<int> MonthLifeObjects {
    // FIXME: list month object life
};

bool MP2::isMonthLife(int obj)
{
    return MonthLifeObjects.find(obj) != MonthLifeObjects.end();
}

const std::set<int> BattleLifeObjects {
    // FIXME: list battle object life

	// luck modificators
    OBJ_IDOL,
	OBJ_FOUNTAIN,
	OBJ_FAERIERING,
	OBJ_PYRAMID,

	// morale modificators
	OBJ_BUOY,
	OBJ_OASIS,
	OBJ_TEMPLE,
	OBJ_WATERINGHOLE,
	OBJ_GRAVEYARD,
	OBJ_DERELICTSHIP,
	OBJ_SHIPWRECK,
	OBJ_MERMAID,
};

bool MP2::isBattleLife(int obj)
{
    return BattleLifeObjects.find(obj) != BattleLifeObjects.end();
}

bool MP2::isActionObject(int obj, bool water)
{
    if (water) 
        return isWaterObject(obj);

    return isGroundObject(obj);
}


const std::map<int, bool> WaterObjects {
	{ OBJ_WATERCHEST, true },
	{ OBJ_DERELICTSHIP, true },
	{ OBJ_SHIPWRECK, true },
	{ OBJ_WHIRLPOOL, true },
	{ OBJ_BUOY, true },
	{ OBJ_BOTTLE, true },
	{ OBJ_SHIPWRECKSURVIROR, true },
	{ OBJ_FLOTSAM, true },
	{ OBJ_MAGELLANMAPS, true },
	{ OBJ_COAST, true },

	{ OBJ_MERMAID, true },
	{ OBJ_SIRENS, true },
	{ OBJ_BARRIER, true },

    // hack (bug: #3142729)
	{ OBJ_MONSTER, true },
	{ OBJ_ARTIFACT, true },
	{ OBJ_RESOURCE, true },

	{ OBJ_CASTLE, false },
	{ OBJ_BOAT, false },
};

bool MP2::isWaterObject(int obj)
{
    if (WaterObjects.find(obj) != WaterObjects.end())
        return WaterObjects.at(obj);

    // price loyalty: editor allow place other objects
    return Settings::Get().PriceLoyaltyVersion() ? isGroundObject(obj) : false;
}

const std::set<int> GroundObjects {
	OBJ_TREASURECHEST,
	OBJ_ALCHEMYLAB,
	OBJ_SIGN,
	OBJ_SKELETON,
	OBJ_DAEMONCAVE,
	OBJ_FAERIERING,
	OBJ_CAMPFIRE,
	OBJ_FOUNTAIN,
	OBJ_GAZEBO,
	OBJ_ANCIENTLAMP,
	OBJ_GRAVEYARD,
	OBJ_ARCHERHOUSE,
	OBJ_GOBLINHUT,
	OBJ_DWARFCOTT,
	OBJ_PEASANTHUT,
	OBJ_THATCHEDHUT,
	OBJ_DRAGONCITY,
	OBJ_LIGHTHOUSE,
	OBJ_WATERWHEEL,
	OBJ_MINES,
	OBJ_OBELISK,
	OBJ_OASIS,
	OBJ_RESOURCE,
	OBJ_SAWMILL,
	OBJ_ORACLE,
	OBJ_SHIPWRECK,
	OBJ_DESERTTENT,
	OBJ_STONELIGHTS,
	OBJ_WAGONCAMP,
	OBJ_WINDMILL,
	OBJ_ARTIFACT,
	OBJ_WATCHTOWER,
	OBJ_TREEHOUSE,
	OBJ_TREECITY,
	OBJ_RUINS,
	OBJ_FORT,
	OBJ_TRADINGPOST,
	OBJ_ABANDONEDMINE,
	OBJ_STANDINGSTONES,
	OBJ_IDOL,
	OBJ_TREEKNOWLEDGE,
	OBJ_DOCTORHUT,
	OBJ_TEMPLE,
	OBJ_HILLFORT,
	OBJ_HALFLINGHOLE,
	OBJ_MERCENARYCAMP,
	OBJ_WATERINGHOLE,
	OBJ_SHRINE1,
	OBJ_SHRINE2,
	OBJ_SHRINE3,
	OBJ_PYRAMID,
	OBJ_CITYDEAD,
	OBJ_EXCAVATION,
	OBJ_SPHINX,
	OBJ_WAGON,
	OBJ_ARTESIANSPRING,
	OBJ_TROLLBRIDGE,
	OBJ_WITCHSHUT,
	OBJ_XANADU,
	OBJ_CAVE,
	OBJ_LEANTO,
	OBJ_MAGICWELL,
	OBJ_MAGICGARDEN,
	OBJ_OBSERVATIONTOWER,
	OBJ_FREEMANFOUNDRY,

	OBJ_MONSTER,
	OBJ_CASTLE,
	OBJ_BOAT,

	OBJ_BARRIER,
	OBJ_TRAVELLERTENT,
	OBJ_FIREALTAR,
	OBJ_AIRALTAR,
	OBJ_EARTHALTAR,
	OBJ_WATERALTAR,
	OBJ_BARROWMOUNDS,
	OBJ_ARENA,
	OBJ_JAIL,
	OBJ_STABLES,
	OBJ_ALCHEMYTOWER,
	OBJ_HUTMAGI,
	OBJ_EYEMAGI,
};

bool MP2::isGroundObject(int obj)
{
    return GroundObjects.find(obj) != GroundObjects.end();
}

const std::set<int> QuantityObjects {
	OBJ_SKELETON,
	OBJ_WAGON,
	OBJ_ARTIFACT,
	OBJ_RESOURCE,
	OBJ_MAGICGARDEN,
	OBJ_WATERWHEEL,
	OBJ_WINDMILL,
	OBJ_LEANTO,
	OBJ_CAMPFIRE,
	OBJ_FLOTSAM,
	OBJ_SHIPWRECKSURVIROR,
	OBJ_TREASURECHEST,
	OBJ_WATERCHEST,
	OBJ_DERELICTSHIP,
	OBJ_SHIPWRECK,
	OBJ_GRAVEYARD,
	OBJ_PYRAMID,
	OBJ_DAEMONCAVE,
	OBJ_ABANDONEDMINE,
};

bool MP2::isQuantityObject(int obj)
{
    return (QuantityObjects.find(obj) != QuantityObjects.end()) || isPickupObject(obj);
}

const std::map<int, bool> CaptureObjects {
	{ OBJ_MINES,	        true },
	{ OBJ_ABANDONEDMINE,	true },
	{ OBJ_ALCHEMYLAB,	    true },
	{ OBJ_SAWMILL,	        true },
	{ OBJ_LIGHTHOUSE,	    true },
	{ OBJ_CASTLE,	        true },

	{ OBJ_WATERWHEEL,	Settings::Get().ExtWorldExtObjectsCaptured() },
	{ OBJ_WINDMILL,	    Settings::Get().ExtWorldExtObjectsCaptured() },
	{ OBJ_MAGICGARDEN,	Settings::Get().ExtWorldExtObjectsCaptured() },
};

bool MP2::isCaptureObject(int obj)
{
    return CaptureObjects.find(obj) != CaptureObjects.end() ? CaptureObjects.at(obj) : false;
}

const std::set<int> PickupObjects {
    OBJ_WATERCHEST,
	OBJ_SHIPWRECKSURVIROR,
	OBJ_FLOTSAM,
	OBJ_BOTTLE,
	OBJ_TREASURECHEST,
	OBJ_ANCIENTLAMP,
	OBJ_CAMPFIRE,
	OBJ_RESOURCE,
	OBJ_ARTIFACT,
};

bool MP2::isPickupObject(int obj)
{
    return PickupObjects.find(obj) != PickupObjects.end();
}

const std::set<int> MoveObjects {
    OBJ_STONELIGHTS,
    OBJ_WHIRLPOOL,
};

bool MP2::isMoveObject(int obj)
{
    return MoveObjects.find(obj) != MoveObjects.end();
}

const std::set<int> RemoveObjects {
    OBJ_MONSTER,
    OBJ_BARRIER,
};

bool MP2::isRemoveObject(int obj)
{
    return RemoveObjects.find(obj) != RemoveObjects.end() || isPickupObject(obj);
}

const std::set<int> NeedStayFrontObjects {
    OBJ_WATERCHEST,
	OBJ_SHIPWRECKSURVIROR,
	OBJ_FLOTSAM,
	OBJ_BOTTLE,
	OBJ_TREASURECHEST,
	OBJ_ANCIENTLAMP,
	OBJ_CAMPFIRE,
	OBJ_MONSTER,
	OBJ_RESOURCE,
	OBJ_ARTIFACT,
	OBJ_HEROES,
	OBJ_BOAT,
	OBJ_BARRIER,
	OBJ_JAIL,
	OBJ_SHIPWRECK,
	OBJ_BUOY,
};

bool MP2::isNeedStayFront(int obj)
{
    return NeedStayFrontObjects.find(obj) != NeedStayFrontObjects.end();
}

const std::set<int> ClearGroundObjects {
    OBJ_ZERO,
    OBJ_COAST,
};

bool MP2::isClearGroundObject(int obj)
{
    return ClearGroundObjects.find(obj) != ClearGroundObjects.end();
}

const std::map<int, int> ObjectDirections {

	{ OBJ_JAIL,	            DIRECTION_ALL },
	{ OBJ_BARRIER,	        DIRECTION_ALL },

	{ OBJ_SHIPWRECK,	    Direction::CENTER | Direction::LEFT | DIRECTION_BOTTOM_ROW },

	{ OBJ_DERELICTSHIP,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_TROLLBRIDGE,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_ARCHERHOUSE,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_DOCTORHUT,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_DWARFCOTT,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_THATCHEDHUT,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_FOUNTAIN,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_IDOL,	            DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_LIGHTHOUSE,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_OBELISK,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_SIGN,	            DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_WATCHTOWER,   	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_WITCHSHUT,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_GAZEBO,   	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_MAGICWELL,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_OBSERVATIONTOWER,	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_PEASANTHUT,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_STONELIGHTS,  	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_STANDINGSTONES,	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_GOBLINHUT,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_SHRINE1,      	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_SHRINE2,       	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_SHRINE3,      	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_TREEHOUSE,    	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_ARTESIANSPRING,	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_SKELETON,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_TREEKNOWLEDGE,	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_ORACLE,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_OASIS,        	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_LEANTO,       	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_MAGICGARDEN,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_WAGON,        	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_TRAVELLERTENT,	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_ALCHEMYTOWER,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_HUTMAGI,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_EYEMAGI,      	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_MERCENARYCAMP,	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_WINDMILL,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_WATERINGHOLE, 	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_TRADINGPOST,  	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_EXCAVATION, 	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_DESERTTENT,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_DAEMONCAVE,   	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_PYRAMID,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_FORT,	            DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_RUINS,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_HILLFORT,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_FREEMANFOUNDRY,	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_SAWMILL,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_TREECITY,     	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_SPHINX,          	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_TEMPLE,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_FAERIERING,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_BARROWMOUNDS, 	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_STABLES,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_ABANDONEDMINE,	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_MINES,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_ALCHEMYLAB,   	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_CAVE,	            DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_CITYDEAD,     	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_GRAVEYARD,    	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_DRAGONCITY,   	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_XANADU,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_HALFLINGHOLE, 	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_WAGONCAMP,    	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_WATERALTAR,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_AIRALTAR,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_FIREALTAR,    	DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_EARTHALTAR,	    DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_ARENA,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_SIRENS,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },
	{ OBJ_MERMAID,	        DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW },

	{ OBJ_WATERWHEEL,	    Direction::CENTER | Direction::RIGHT | DIRECTION_BOTTOM_ROW },
	{ OBJ_MAGELLANMAPS,	    Direction::CENTER | Direction::LEFT | DIRECTION_BOTTOM_ROW },
	{ OBJ_CASTLE,	        Direction::CENTER | Direction::BOTTOM },
};

int MP2::GetObjectDirect(int obj)
{
    return ObjectDirections.find(obj) != ObjectDirections.end() ? ObjectDirections.at(obj) : DIRECTION_ALL;
}

