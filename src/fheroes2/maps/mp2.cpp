/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "mp2.h"

#include <cassert>
#include <ostream>

#include "direction.h"
#include "ground.h"
#include "icn.h"
#include "logging.h"
#include "settings.h"
#include "translations.h"

int MP2::GetICNObject( const uint8_t tileset )
{
    // First 2 bits are used for flags like animation.
    // TODO: separate these 2 bits and real tile (?) index into 2 variables to avoid this bit shifting operations all over the code.
    switch ( tileset >> 2 ) {
    // reserverd
    case 0:
        return ICN::UNKNOWN;
    // custom: boat sprite
    case 6:
        return ICN::BOAT32;
    // artifact
    case 11:
        return ICN::OBJNARTI;
    // monster
    case 12:
        return ICN::MONS32;
    // castle flags
    case 14:
        return ICN::FLAG32;
    // heroes
    case 21:
        return ICN::MINIHERO;
    // mountains: snow
    case 22:
        return ICN::MTNSNOW;
    // mountains: swamp
    case 23:
        return ICN::MTNSWMP;
    // mountains: lava
    case 24:
        return ICN::MTNLAVA;
    // mountains: desert
    case 25:
        return ICN::MTNDSRT;
    // mountains: dirt
    case 26:
        return ICN::MTNDIRT;
    // mountains: all terrains
    case 27:
        return ICN::MTNMULT;
    // mines
    case 29:
        return ICN::EXTRAOVR;
    // road
    case 30:
        return ICN::ROAD;
    // relief: crck
    case 31:
        return ICN::MTNCRCK;
    // relief: gras
    case 32:
        return ICN::MTNGRAS;
    // trees jungle
    case 33:
        return ICN::TREJNGL;
    // trees evil
    case 34:
        return ICN::TREEVIL;
    // castle and town sprites
    case 35:
        return ICN::OBJNTOWN;
    // castle base
    case 36:
        return ICN::OBJNTWBA;
    // castle shadow
    case 37:
        return ICN::OBJNTWSH;
    // random castle
    case 38:
        return ICN::OBJNTWRD;
    // mine guardians (elementals)
    case 39:
        return ICN::OBJNXTRA;
    // water object
    case 40:
        return ICN::OBJNWAT2;
    // object other
    case 41:
        return ICN::OBJNMUL2;
    // trees snow
    case 42:
        return ICN::TRESNOW;
    // trees trefir
    case 43:
        return ICN::TREFIR;
    // trees
    case 44:
        return ICN::TREFALL;
    // river
    case 45:
        return ICN::STREAM;
    // resource
    case 46:
        return ICN::OBJNRSRC;
    // gras object
    case 48:
        return ICN::OBJNGRA2;
    // trees tredeci
    case 49:
        return ICN::TREDECI;
    // sea object
    case 50:
        return ICN::OBJNWATR;
    // vegetation gras
    case 51:
        return ICN::OBJNGRAS;
    // object on snow
    case 52:
        return ICN::OBJNSNOW;
    // object on swamp
    case 53:
        return ICN::OBJNSWMP;
    // object on lava
    case 54:
        return ICN::OBJNLAVA;
    // object on desert
    case 55:
        return ICN::OBJNDSRT;
    // object on dirt
    case 56:
        return ICN::OBJNDIRT;
    // object on crck
    case 57:
        return ICN::OBJNCRCK;
    // object on lava
    case 58:
        return ICN::OBJNLAV3;
    // object on earth
    case 59:
        return ICN::OBJNMULT;
    //  object on lava
    case 60:
        return ICN::OBJNLAV2;
    // extra objects for loyalty version
    case 61:
        if ( Settings::Get().isPriceOfLoyaltySupported() )
            return ICN::X_LOC1;
        break;
    // extra objects for loyalty version
    case 62:
        if ( Settings::Get().isPriceOfLoyaltySupported() )
            return ICN::X_LOC2;
        break;
    // extra objects for loyalty version
    case 63:
        if ( Settings::Get().isPriceOfLoyaltySupported() )
            return ICN::X_LOC3;
        break;
    default:
        break;
    }

    DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown tileset: " << static_cast<int>( tileset ) )
    return ICN::UNKNOWN;
}

bool MP2::isHiddenForPuzzle( const int terrainType, uint8_t tileset, uint8_t index )
{
    const int icnID = tileset >> 2;
    if ( icnID < 22 || icnID == 46 || ( icnID == 56 && index == 140 ) || ( icnID == 59 && index >= 124 && index <= 137 ) ) {
        return true;
    }

    return isDiggingHoleSprite( terrainType, tileset, index );
}

const char * MP2::StringObject( const MapObjectType objectType, const int count )
{
    switch ( objectType ) {
    case OBJ_ZERO:
        return _( "No object" );
    case OBJN_ALCHEMYLAB:
    case OBJ_ALCHEMYLAB:
        return _( "Alchemist Lab" );
    case OBJN_DAEMONCAVE:
    case OBJ_DAEMONCAVE:
        return _( "Daemon Cave" );
    case OBJN_FAERIERING:
    case OBJ_FAERIERING:
        return _( "Faerie Ring" );
    case OBJN_GRAVEYARD:
    case OBJ_GRAVEYARD:
        return _( "Graveyard" );
    case OBJN_DRAGONCITY:
    case OBJ_DRAGONCITY:
        return _( "Dragon City" );
    case OBJN_LIGHTHOUSE:
    case OBJ_LIGHTHOUSE:
        return _( "Lighthouse" );
    case OBJN_WATERWHEEL:
    case OBJ_WATERWHEEL:
        return _n( "Water Wheel", "Water Wheels", count );
    case OBJN_MINES:
    case OBJ_MINES:
        return _( "Mines" );
    case OBJN_OBELISK:
    case OBJ_OBELISK:
        return _( "Obelisk" );
    case OBJN_OASIS:
    case OBJ_OASIS:
        return _( "Oasis" );
    case OBJN_SAWMILL:
    case OBJ_SAWMILL:
        return _( "Sawmill" );
    case OBJN_ORACLE:
    case OBJ_ORACLE:
        return _( "Oracle" );
    case OBJN_DESERTTENT:
    case OBJ_DESERTTENT:
        return _( "Desert Tent" );
    case OBJN_CASTLE:
    case OBJ_CASTLE:
        return _( "Castle" );
    case OBJN_WAGONCAMP:
    case OBJ_WAGONCAMP:
        return _( "Wagon Camp" );
    case OBJN_WINDMILL:
    case OBJ_WINDMILL:
        return _n( "Windmill", "Windmills", count );
    case OBJN_RNDTOWN:
    case OBJ_RNDTOWN:
        return _( "Random Town" );
    case OBJN_RNDCASTLE:
    case OBJ_RNDCASTLE:
        return _( "Random Castle" );
    case OBJN_WATCHTOWER:
    case OBJ_WATCHTOWER:
        return _( "Watch Tower" );
    case OBJN_TREECITY:
    case OBJ_TREECITY:
        return _( "Tree City" );
    case OBJN_TREEHOUSE:
    case OBJ_TREEHOUSE:
        return _( "Tree House" );
    case OBJN_RUINS:
    case OBJ_RUINS:
        return _( "Ruins" );
    case OBJN_FORT:
    case OBJ_FORT:
        return _( "Fort" );
    case OBJN_TRADINGPOST:
    case OBJ_TRADINGPOST:
        return _( "Trading Post" );
    case OBJN_ABANDONEDMINE:
    case OBJ_ABANDONEDMINE:
        return _( "Abandoned Mine" );
    case OBJN_TREEKNOWLEDGE:
    case OBJ_TREEKNOWLEDGE:
        return _( "Tree of Knowledge" );
    case OBJN_DOCTORHUT:
    case OBJ_DOCTORHUT:
        return _( "Witch Doctor's Hut" );
    case OBJN_TEMPLE:
    case OBJ_TEMPLE:
        return _( "Temple" );
    case OBJN_HILLFORT:
    case OBJ_HILLFORT:
        return _( "Hill Fort" );
    case OBJN_HALFLINGHOLE:
    case OBJ_HALFLINGHOLE:
        return _( "Halfling Hole" );
    case OBJN_MERCENARYCAMP:
    case OBJ_MERCENARYCAMP:
        return _( "Mercenary Camp" );
    case OBJN_PYRAMID:
    case OBJ_PYRAMID:
        return _( "Pyramid" );
    case OBJN_CITYDEAD:
    case OBJ_CITYDEAD:
        return _( "City of the Dead" );
    case OBJN_EXCAVATION:
    case OBJ_EXCAVATION:
        return _( "Excavation" );
    case OBJN_SPHINX:
    case OBJ_SPHINX:
        return _( "Sphinx" );
    case OBJN_TROLLBRIDGE:
    case OBJ_TROLLBRIDGE:
        return _( "Troll Bridge" );
    case OBJN_WITCHSHUT:
    case OBJ_WITCHSHUT:
        return _( "Witch's Hut" );
    case OBJN_XANADU:
    case OBJ_XANADU:
        return _( "Xanadu" );
    case OBJN_CAVE:
    case OBJ_CAVE:
        return _( "Cave" );
    case OBJN_MAGELLANMAPS:
    case OBJ_MAGELLANMAPS:
        return _( "Magellan's Maps" );
    case OBJN_DERELICTSHIP:
    case OBJ_DERELICTSHIP:
        return _( "Derelict Ship" );
    case OBJN_SHIPWRECK:
    case OBJ_SHIPWRECK:
        return _( "Shipwreck" );
    case OBJN_OBSERVATIONTOWER:
    case OBJ_OBSERVATIONTOWER:
        return _( "Observation Tower" );
    case OBJN_FREEMANFOUNDRY:
    case OBJ_FREEMANFOUNDRY:
        return _( "Freeman's Foundry" );
    case OBJN_WATERINGHOLE:
    case OBJ_WATERINGHOLE:
        return _( "Watering Hole" );
    case OBJN_ARTESIANSPRING:
    case OBJ_ARTESIANSPRING:
        return _( "Artesian Spring" );
    case OBJN_GAZEBO:
    case OBJ_GAZEBO:
        return _( "Gazebo" );
    case OBJN_ARCHERHOUSE:
    case OBJ_ARCHERHOUSE:
        return _( "Archer's House" );
    case OBJN_PEASANTHUT:
    case OBJ_PEASANTHUT:
        return _( "Peasant Hut" );
    case OBJN_DWARFCOTT:
    case OBJ_DWARFCOTT:
        return _( "Dwarf Cottage" );
    case OBJN_STONELITHS:
    case OBJ_STONELITHS:
        return _( "Stone Liths" );
    case OBJN_MAGICWELL:
    case OBJ_MAGICWELL:
        return _( "Magic Well" );
    case OBJ_HEROES:
        return _( "Heroes" );
    case OBJ_SIGN:
        return _( "Sign" );
    case OBJ_NOTHINGSPECIAL:
    case OBJ_NOTHINGSPECIAL2:
        return _( "Nothing Special" );
    case OBJ_TARPIT:
        return _( "Tar Pit" );
    case OBJ_COAST:
        return _( "Beach" );
    case OBJ_MOUND:
        return _( "Mound" );
    case OBJ_DUNE:
        return _( "Dune" );
    case OBJ_STUMP:
        return _( "Stump" );
    case OBJ_CACTUS:
        return _( "Cactus" );
    case OBJ_TREES:
        return _( "Trees" );
    case OBJ_DEADTREE:
        return _( "Dead Tree" );
    case OBJ_MOUNTS:
        return _( "Mountains" );
    case OBJ_VOLCANO:
        return _( "Volcano" );
    case OBJ_STONES:
        return _( "Rock" );
    case OBJ_FLOWERS:
        return _( "Flowers" );
    case OBJ_WATERLAKE:
        return _( "Water Lake" );
    case OBJ_MANDRAKE:
        return _( "Mandrake" );
    case OBJ_CRATER:
        return _( "Crater" );
    case OBJ_LAVAPOOL:
        return _( "Lava Pool" );
    case OBJ_SHRUB:
        return _( "Shrub" );
    case OBJ_BUOY:
        return _( "Buoy" );
    case OBJN_SKELETON:
    case OBJ_SKELETON:
        return _( "Skeleton" );
    case OBJ_TREASURECHEST:
        return _( "Treasure Chest" );
    case OBJ_WATERCHEST:
        return _( "Sea Chest" );
    case OBJ_CAMPFIRE:
        return _( "Campfire" );
    case OBJ_FOUNTAIN:
        return _( "Fountain" );
    case OBJ_ANCIENTLAMP:
        return _( "Genie Lamp" );
    case OBJ_GOBLINHUT:
        return _( "Goblin Hut" );
    case OBJ_THATCHEDHUT:
        return _( "Thatched Hut" );
    case OBJ_MONSTER:
        return _( "Monster" );
    case OBJ_RESOURCE:
        return _( "Resource" );
    case OBJ_WHIRLPOOL:
        return _( "Whirlpool" );
    case OBJ_ARTIFACT:
        return _( "Artifact" );
    case OBJ_BOAT:
        return _( "Boat" );
    case OBJ_RNDARTIFACT:
        return _( "Random Artifact" );
    case OBJ_RNDRESOURCE:
        return _( "Random Resource" );
    case OBJ_RNDMONSTER1:
        return _( "Random Monster - weak" );
    case OBJ_RNDMONSTER2:
        return _( "Random Monster - medium" );
    case OBJ_RNDMONSTER3:
        return _( "Random Monster - strong" );
    case OBJ_RNDMONSTER4:
        return _( "Random Monster - very strong" );
    case OBJ_STANDINGSTONES:
        return _( "Standing Stones" );
    case OBJ_EVENT:
        return _( "Event" );
    case OBJ_RNDMONSTER:
        return _( "Random Monster" );
    case OBJ_RNDULTIMATEARTIFACT:
        return _( "Random Ultimate Artifact" );
    case OBJ_IDOL:
        return _( "Idol" );
    case OBJ_SHRINE1:
        return _( "Shrine of the First Circle" );
    case OBJ_SHRINE2:
        return _( "Shrine of the Second Circle" );
    case OBJ_SHRINE3:
        return _( "Shrine of the Third Circle" );
    case OBJ_WAGON:
        return _( "Wagon" );
    case OBJ_LEANTO:
        return _( "Lean-To" );
    case OBJ_FLOTSAM:
        return _( "Flotsam" );
    case OBJ_SHIPWRECKSURVIVOR:
        return _( "Shipwreck Survivor" );
    case OBJ_BOTTLE:
        return _( "Bottle" );
    case OBJ_MAGICGARDEN:
        return _n( "Magic Garden", "Magic Gardens", count );
    case OBJ_RNDARTIFACT1:
        return _( "Random Artifact - Treasure" );
    case OBJ_RNDARTIFACT2:
        return _( "Random Artifact - Minor" );
    case OBJ_RNDARTIFACT3:
        return _( "Random Artifact - Major" );
    case OBJN_JAIL:
    case OBJ_JAIL:
        return _( "Jail" );
    case OBJN_TRAVELLERTENT:
    case OBJ_TRAVELLERTENT:
        return _( "Traveller's Tent" );
    case OBJ_BARRIER:
        return _( "Barrier" );
    case OBJN_FIREALTAR:
    case OBJ_FIREALTAR:
        return _( "Fire Summoning Altar" );
    case OBJN_AIRALTAR:
    case OBJ_AIRALTAR:
        return _( "Air Summoning Altar" );
    case OBJN_EARTHALTAR:
    case OBJ_EARTHALTAR:
        return _( "Earth Summoning Altar" );
    case OBJN_WATERALTAR:
    case OBJ_WATERALTAR:
        return _( "Water Summoning Altar" );
    case OBJN_BARROWMOUNDS:
    case OBJ_BARROWMOUNDS:
        return _( "Barrow Mounds" );
    case OBJN_ARENA:
    case OBJ_ARENA:
        return _( "Arena" );
    case OBJN_STABLES:
    case OBJ_STABLES:
        return _( "Stables" );
    case OBJN_ALCHEMYTOWER:
    case OBJ_ALCHEMYTOWER:
        return _( "Alchemist's Tower" );
    case OBJN_HUTMAGI:
    case OBJ_HUTMAGI:
        return _( "Hut of the Magi" );
    case OBJN_EYEMAGI:
    case OBJ_EYEMAGI:
        return _( "Eye of the Magi" );
    case OBJN_MERMAID:
    case OBJ_MERMAID:
        return _( "Mermaid" );
    case OBJN_SIRENS:
    case OBJ_SIRENS:
        return _( "Sirens" );
    case OBJ_REEFS:
        return _( "Reefs" );
    case OBJ_UNKNW_02:
        return "OBJ_UNKNW_02";
    case OBJ_UNKNW_03:
        return "OBJ_UNKNW_03";
    case OBJ_UNKNW_06:
        return "OBJ_UNKNW_06";
    case OBJ_UNKNW_08:
        return "OBJ_UNKNW_08";
    case OBJ_UNKNW_09:
        return "OBJ_UNKNW_09";
    case OBJ_UNKNW_0B:
        return "OBJ_UNKNW_0B";
    case OBJ_UNKNW_0E:
        return "OBJ_UNKNW_0E";
    case OBJ_UNKNW_11:
        return "OBJ_UNKNW_11";
    case OBJ_UNKNW_12:
        return "OBJ_UNKNW_12";
    case OBJ_UNKNW_13:
        return "OBJ_UNKNW_13";
    case OBJ_UNKNW_18:
        return "OBJ_UNKNW_18";
    case OBJ_UNKNW_1B:
        return "OBJ_UNKNW_1B";
    case OBJ_UNKNW_1F:
        return "OBJ_UNKNW_1F";
    case OBJ_UNKNW_21:
        return "OBJ_UNKNW_21";
    case OBJ_UNKNW_26:
        return "OBJ_UNKNW_26";
    case OBJ_UNKNW_27:
        return "OBJ_UNKNW_27";
    case OBJ_UNKNW_29:
        return "OBJ_UNKNW_29";
    case OBJ_UNKNW_2A:
        return "OBJ_UNKNW_2A";
    case OBJ_UNKNW_2B:
        return "OBJ_UNKNW_2B";
    case OBJ_UNKNW_2C:
        return "OBJ_UNKNW_2C";
    case OBJ_UNKNW_2D:
        return "OBJ_UNKNW_2D";
    case OBJ_UNKNW_2E:
        return "OBJ_UNKNW_2E";
    case OBJ_UNKNW_2F:
        return "OBJ_UNKNW_2F";
    case OBJ_UNKNW_32:
        return "OBJ_UNKNW_32";
    case OBJ_UNKNW_33:
        return "OBJ_UNKNW_33";
    case OBJ_UNKNW_34:
        return "OBJ_UNKNW_34";
    case OBJ_UNKNW_35:
        return "OBJ_UNKNW_35";
    case OBJ_UNKNW_36:
        return "OBJ_UNKNW_36";
    case OBJ_UNKNW_37:
        return "OBJ_UNKNW_37";
    case OBJ_UNKNW_41:
        return "OBJ_UNKNW_41";
    case OBJ_UNKNW_42:
        return "OBJ_UNKNW_42";
    case OBJ_UNKNW_43:
        return "OBJ_UNKNW_43";
    case OBJ_UNKNW_4A:
        return "OBJ_UNKNW_4A";
    case OBJ_UNKNW_4B:
        return "OBJ_UNKNW_4B";
    case OBJ_UNKNW_50:
        return "OBJ_UNKNW_50";
    case OBJ_UNKNW_58:
        return "OBJ_UNKNW_58";
    case OBJ_UNKNW_5A:
        return "OBJ_UNKNW_5A";
    case OBJ_UNKNW_5C:
        return "OBJ_UNKNW_5C";
    case OBJ_UNKNW_5D:
        return "OBJ_UNKNW_5D";
    case OBJ_UNKNW_5F:
        return "OBJ_UNKNW_5F";
    case OBJ_UNKNW_62:
        return "OBJ_UNKNW_62";
    case OBJ_UNKNW_79:
        return "OBJ_UNKNW_79";
    case OBJ_UNKNW_7A:
        return "OBJ_UNKNW_7A";
    case OBJ_UNKNW_91:
        return "OBJ_UNKNW_91";
    case OBJ_UNKNW_92:
        return "OBJ_UNKNW_92";
    case OBJ_UNKNW_A1:
        return "OBJ_UNKNW_A1";
    case OBJ_UNKNW_A6:
        return "OBJ_UNKNW_A6";
    case OBJ_UNKNW_AA:
        return "OBJ_UNKNW_AA";
    case OBJ_UNKNW_B2:
        return "OBJ_UNKNW_B2";
    case OBJ_UNKNW_B8:
        return "OBJ_UNKNW_B8";
    case OBJ_UNKNW_B9:
        return "OBJ_UNKNW_B9";
    case OBJ_UNKNW_D1:
        return "OBJ_UNKNW_D1";
    case OBJ_UNKNW_E2:
        return "OBJ_UNKNW_E2";
    case OBJ_UNKNW_E3:
        return "OBJ_UNKNW_E3";
    case OBJ_UNKNW_E4:
        return "OBJ_UNKNW_E4";
    case OBJ_UNKNW_E5:
        return "OBJ_UNKNW_E5";
    case OBJ_UNKNW_E6:
        return "OBJ_UNKNW_E6";
    case OBJ_UNKNW_E7:
        return "OBJ_UNKNW_E7";
    case OBJ_UNKNW_E8:
        return "OBJ_UNKNW_E8";
    case OBJ_UNKNW_F9:
        return "OBJ_UNKNW_F9";
    case OBJ_UNKNW_FA:
        return "OBJ_UNKNW_FA";
    default:
        DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown object: " << static_cast<int>( objectType ) )
        break;
    }

    return nullptr;
}

bool MP2::isDayLife( const MapObjectType objectType )
{
    // TODO: list day object life
    switch ( objectType ) {
    case OBJ_MAGICWELL:
        return true;

    default:
        break;
    }

    return false;
}

bool MP2::isWeekLife( const MapObjectType objectType )
{
    // TODO: list week object life
    switch ( objectType ) {
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
    case OBJ_HEROES:
        return true;
    default:
        break;
    }

    return false;
}

bool MP2::isMonthLife( const MapObjectType objectType )
{
    return objectType == MP2::OBJ_CASTLE;
}

bool MP2::isBattleLife( const MapObjectType objectType )
{
    switch ( objectType ) {
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
    default:
        break;
    }

    return false;
}

bool MP2::isActionObject( const MapObjectType objectType, const bool locatesOnWater )
{
    if ( locatesOnWater ) {
        return isWaterActionObject( objectType );
    }

    return isActionObject( objectType );
}

bool MP2::isWaterActionObject( const MapObjectType objectType )
{
    switch ( objectType ) {
    case OBJ_WATERCHEST:
    case OBJ_DERELICTSHIP:
    case OBJ_SHIPWRECK:
    case OBJ_WHIRLPOOL:
    case OBJ_BUOY:
    case OBJ_BOTTLE:
    case OBJ_SHIPWRECKSURVIVOR:
    case OBJ_FLOTSAM:
    case OBJ_MAGELLANMAPS:
    case OBJ_COAST:
    case OBJ_MERMAID:
    case OBJ_SIRENS:
    case OBJ_BARRIER:
    case OBJ_MONSTER:
    case OBJ_ARTIFACT:
    case OBJ_RESOURCE:
        return true;
    case OBJ_CASTLE:
    case OBJ_BOAT:
        return false;
    default:
        break;
    }

    // price loyalty: editor allow place other objects
    return Settings::Get().isPriceOfLoyaltySupported() ? isActionObject( objectType ) : false;
}

bool MP2::isActionObject( const MapObjectType objectType )
{
    // check if first bit is set
    if ( objectType < 128 ) {
        return false;
    }
    switch ( objectType ) {
    case OBJ_EVENT:
    case OBJN_STABLES:
    case OBJN_ALCHEMYTOWER:
    case OBJ_UNKNW_E2:
    case OBJ_UNKNW_E3:
    case OBJ_UNKNW_E4:
    case OBJ_UNKNW_E5:
    case OBJ_UNKNW_E6:
    case OBJ_UNKNW_E7:
    case OBJ_UNKNW_E8:
    case OBJ_UNKNW_F9:
    case OBJ_UNKNW_FA:
    case OBJ_UNKNW_91:
    case OBJ_UNKNW_92:
    case OBJ_UNKNW_9C:
    case OBJ_UNKNW_A1:
    case OBJ_UNKNW_AA:
    case OBJ_UNKNW_B2:
    case OBJ_UNKNW_B8:
    case OBJ_UNKNW_B9:
    case OBJ_UNKNW_D1:
    case OBJ_REEFS:
        return false;
    default:
        break;
    }

    return true;
}

MP2::MapObjectType MP2::getBaseActionObjectType( const MapObjectType objectType )
{
    switch ( objectType ) {
    case OBJN_ALCHEMYLAB:
        return OBJ_ALCHEMYLAB;
    case OBJN_SKELETON:
        return OBJ_SKELETON;
    case OBJN_DAEMONCAVE:
        return OBJ_DAEMONCAVE;
    case OBJN_FAERIERING:
        return OBJ_FAERIERING;
    case OBJN_GAZEBO:
        return OBJ_GAZEBO;
    case OBJN_GRAVEYARD:
        return OBJ_GRAVEYARD;
    case OBJN_ARCHERHOUSE:
        return OBJ_ARCHERHOUSE;
    case OBJN_DWARFCOTT:
        return OBJ_DWARFCOTT;
    case OBJN_PEASANTHUT:
        return OBJ_PEASANTHUT;
    case OBJN_DRAGONCITY:
        return OBJ_DRAGONCITY;
    case OBJN_LIGHTHOUSE:
        return OBJ_LIGHTHOUSE;
    case OBJN_WATERWHEEL:
        return OBJ_WATERWHEEL;
    case OBJN_MINES:
        return OBJ_MINES;
    case OBJN_OBELISK:
        return OBJ_OBELISK;
    case OBJN_OASIS:
        return OBJ_OASIS;
    case OBJN_SAWMILL:
        return OBJ_SAWMILL;
    case OBJN_ORACLE:
        return OBJ_ORACLE;
    case OBJN_SHIPWRECK:
        return OBJ_SHIPWRECK;
    case OBJN_DESERTTENT:
        return OBJ_DESERTTENT;
    case OBJN_CASTLE:
        return OBJ_CASTLE;
    case OBJN_STONELITHS:
        return OBJ_STONELITHS;
    case OBJN_WAGONCAMP:
        return OBJ_WAGONCAMP;
    case OBJN_WINDMILL:
        return OBJ_WINDMILL;
    case OBJN_RNDTOWN:
        return OBJ_RNDTOWN;
    case OBJN_RNDCASTLE:
        return OBJ_RNDCASTLE;
    case OBJN_WATCHTOWER:
        return OBJ_WATCHTOWER;
    case OBJN_TREEHOUSE:
        return OBJ_TREEHOUSE;
    case OBJN_TREECITY:
        return OBJ_TREECITY;
    case OBJN_RUINS:
        return OBJ_RUINS;
    case OBJN_FORT:
        return OBJ_FORT;
    case OBJN_TRADINGPOST:
        return OBJ_TRADINGPOST;
    case OBJN_ABANDONEDMINE:
        return OBJ_ABANDONEDMINE;
    case OBJN_TREEKNOWLEDGE:
        return OBJ_TREEKNOWLEDGE;
    case OBJN_DOCTORHUT:
        return OBJ_DOCTORHUT;
    case OBJN_TEMPLE:
        return OBJ_TEMPLE;
    case OBJN_HILLFORT:
        return OBJ_HILLFORT;
    case OBJN_HALFLINGHOLE:
        return OBJ_HALFLINGHOLE;
    case OBJN_MERCENARYCAMP:
        return OBJ_MERCENARYCAMP;
    case OBJN_PYRAMID:
        return OBJ_PYRAMID;
    case OBJN_CITYDEAD:
        return OBJ_CITYDEAD;
    case OBJN_EXCAVATION:
        return OBJ_EXCAVATION;
    case OBJN_SPHINX:
        return OBJ_SPHINX;
    case OBJN_ARTESIANSPRING:
        return OBJ_ARTESIANSPRING;
    case OBJN_TROLLBRIDGE:
        return OBJ_TROLLBRIDGE;
    case OBJN_WATERINGHOLE:
        return OBJ_WATERINGHOLE;
    case OBJN_WITCHSHUT:
        return OBJ_WITCHSHUT;
    case OBJN_XANADU:
        return OBJ_XANADU;
    case OBJN_CAVE:
        return OBJ_CAVE;
    case OBJN_MAGELLANMAPS:
        return OBJ_MAGELLANMAPS;
    case OBJN_DERELICTSHIP:
        return OBJ_DERELICTSHIP;
    case OBJN_MAGICWELL:
        return OBJ_MAGICWELL;
    case OBJN_OBSERVATIONTOWER:
        return OBJ_OBSERVATIONTOWER;
    case OBJN_FREEMANFOUNDRY:
        return OBJ_FREEMANFOUNDRY;
    case OBJN_ARENA:
        return OBJ_ARENA;
    case OBJN_BARROWMOUNDS:
        return OBJ_BARROWMOUNDS;
    case OBJN_MERMAID:
        return OBJ_MERMAID;
    case OBJN_SIRENS:
        return OBJ_SIRENS;
    case OBJN_HUTMAGI:
        return OBJ_HUTMAGI;
    case OBJN_EYEMAGI:
        return OBJ_EYEMAGI;
    case OBJN_TRAVELLERTENT:
        return OBJ_TRAVELLERTENT;
    case OBJN_JAIL:
        return OBJ_JAIL;
    case OBJN_FIREALTAR:
        return OBJ_FIREALTAR;
    case OBJN_AIRALTAR:
        return OBJ_AIRALTAR;
    case OBJN_EARTHALTAR:
        return OBJ_EARTHALTAR;
    case OBJN_WATERALTAR:
        return OBJ_WATERALTAR;
    case OBJN_ALCHEMYTOWER:
        return OBJ_ALCHEMYTOWER;
    case OBJN_STABLES:
        return OBJ_STABLES;
    default:
        break;
    }

    return objectType;
}

bool MP2::isQuantityObject( const MapObjectType objectType )
{
    switch ( objectType ) {
    case OBJ_SKELETON:
    case OBJ_WAGON:
    case OBJ_MAGICGARDEN:
    case OBJ_WATERWHEEL:
    case OBJ_WINDMILL:
    case OBJ_LEANTO:
    case OBJ_CAMPFIRE:
    case OBJ_FLOTSAM:
    case OBJ_SHIPWRECKSURVIVOR:
    case OBJ_WATERCHEST:
    case OBJ_DERELICTSHIP:
    case OBJ_SHIPWRECK:
    case OBJ_GRAVEYARD:
    case OBJ_PYRAMID:
    case OBJ_DAEMONCAVE:
    case OBJ_ABANDONEDMINE:
        return true;
    default:
        break;
    }

    if ( isPickupObject( objectType ) )
        return true;

    return false;
}

bool MP2::isCaptureObject( const MapObjectType objectType )
{
    switch ( objectType ) {
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
    default:
        break;
    }

    return false;
}

bool MP2::isPickupObject( const MapObjectType objectType )
{
    switch ( objectType ) {
    case OBJ_WATERCHEST:
    case OBJ_SHIPWRECKSURVIVOR:
    case OBJ_FLOTSAM:
    case OBJ_BOTTLE:
    case OBJ_TREASURECHEST:
    case OBJ_ANCIENTLAMP:
    case OBJ_CAMPFIRE:
    case OBJ_RESOURCE:
    case OBJ_ARTIFACT:
        return true;
    default:
        break;
    }

    return false;
}

bool MP2::isArtifactObject( const MapObjectType objectType )
{
    switch ( objectType ) {
    case OBJ_ARTIFACT:
    case OBJ_WAGON:
    case OBJ_SKELETON:
    case OBJ_DAEMONCAVE:
    case OBJ_WATERCHEST:
    case OBJ_TREASURECHEST:
    case OBJ_SHIPWRECKSURVIVOR:
    case OBJ_SHIPWRECK:
    case OBJ_GRAVEYARD:
        return true;
    default:
        break;
    }

    return false;
}

bool MP2::isHeroUpgradeObject( const MapObjectType objectType )
{
    switch ( objectType ) {
    case OBJ_GAZEBO:
    case OBJ_TREEKNOWLEDGE:
    case OBJ_MERCENARYCAMP:
    case OBJ_FORT:
    case OBJ_STANDINGSTONES:
    case OBJ_DOCTORHUT:
    case OBJ_SHRINE1:
    case OBJ_SHRINE2:
    case OBJ_SHRINE3:
    case OBJ_WITCHSHUT:
    case OBJ_XANADU:
        return true;
    default:
        break;
    }

    return false;
}

bool MP2::isMonsterDwelling( const MapObjectType objectType )
{
    switch ( objectType ) {
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
    case OBJ_RUINS:
    case OBJ_TREECITY:
    case OBJ_WAGONCAMP:
    case OBJ_DESERTTENT:
    case OBJ_WATERALTAR:
    case OBJ_AIRALTAR:
    case OBJ_FIREALTAR:
    case OBJ_EARTHALTAR:
    case OBJ_BARROWMOUNDS:
    case OBJ_CITYDEAD:
    case OBJ_TROLLBRIDGE:
    case OBJ_DRAGONCITY:
        return true;
    default:
        break;
    }

    return false;
}

bool MP2::isProtectedObject( const MapObjectType objectType )
{
    switch ( objectType ) {
    case OBJ_MONSTER:
    case OBJ_ARTIFACT:
    case OBJ_DERELICTSHIP:
    case OBJ_SHIPWRECK:
    case OBJ_GRAVEYARD:
    case OBJ_PYRAMID:
    case OBJ_DAEMONCAVE:
    case OBJ_ABANDONEDMINE:
    case OBJ_CITYDEAD:
    case OBJ_TROLLBRIDGE:
    case OBJ_DRAGONCITY:
        return true;
    default:
        break;
    }

    return isCaptureObject( objectType );
}

bool MP2::isSafeForFogDiscoveryObject( const MapObjectType objectType )
{
    switch ( objectType ) {
    // Stone liths and whirlpools are mandatory because they open access to new tiles
    case OBJ_STONELITHS:
    case OBJ_WHIRLPOOL:
    // Sign messages are useless for AI, but they are harmless for fog discovery purposes
    case OBJ_SIGN:
        return true;
    default:
        break;
    }

    // Action objects in general should be avoided for fog discovery purposes, because
    // they may be guarded or may require wasting resources
    return !isActionObject( objectType );
}

bool MP2::isAbandonedMine( const MapObjectType objectType )
{
    return objectType == MP2::OBJN_ABANDONEDMINE || objectType == MP2::OBJ_ABANDONEDMINE;
}

bool MP2::isNeedStayFront( const MapObjectType objectType )
{
    switch ( objectType ) {
    case OBJ_MONSTER:
    case OBJ_HEROES:
    case OBJ_BOAT:
    case OBJ_BARRIER:
    case OBJ_JAIL:
    case OBJ_BUOY:
    case OBJ_SKELETON:
    case OBJ_MERMAID:
    case OBJ_SIRENS:
    case OBJ_SHIPWRECK:
        return true;
    default:
        break;
    }

    return isPickupObject( objectType );
}

int MP2::getActionObjectDirection( const MapObjectType objectType )
{
    switch ( objectType ) {
    case OBJ_JAIL:
    case OBJ_BARRIER:
    case OBJ_ARTIFACT:
    case OBJ_RESOURCE:
    case OBJ_TREASURECHEST:
    case OBJ_MONSTER:
    case OBJ_ANCIENTLAMP:
    case OBJ_CAMPFIRE:
    case OBJ_SHIPWRECKSURVIVOR:
    case OBJ_FLOTSAM:
    case OBJ_WATERCHEST:
    case OBJ_BUOY:
    case OBJ_WHIRLPOOL:
    case OBJ_BOTTLE:
    case OBJ_COAST:
    case OBJ_BOAT:
    case OBJ_HEROES:
        return DIRECTION_ALL;
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
    case OBJ_STONELITHS:
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
    case OBJ_WATERWHEEL:
    case OBJ_MAGELLANMAPS:
    case OBJ_SHIPWRECK:
        return DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW;
    case OBJ_CASTLE:
        return Direction::CENTER | Direction::BOTTOM;
    default:
        // Did you add a new action object? Please add its passability!
        assert( 0 );
        break;
    }

    return Direction::UNKNOWN;
}

bool MP2::getDiggingHoleSprite( const int terrainType, uint8_t & tileSet, uint32_t & index )
{
    switch ( terrainType ) {
    case Maps::Ground::DESERT:
        tileSet = 0xDC; // ICN::OBJNDSRT
        index = 68;
        return true;
    case Maps::Ground::SNOW:
        tileSet = 208; // ICN::OBJNSNOW
        index = 11;
        return true;
    case Maps::Ground::SWAMP:
        tileSet = 212; // ICN::OBJNSWMP
        index = 86;
        return true;
    case Maps::Ground::WASTELAND:
        tileSet = 0xE4; // ICN::OBJNCRCK
        index = 70;
        return true;
    case Maps::Ground::LAVA:
        tileSet = 0xD8; // ICN::OBJNLAVA
        index = 26;
        return true;
    case Maps::Ground::DIRT:
        tileSet = 0xE0; // ICN::OBJNDIRT
        index = 140;
        return true;
    case Maps::Ground::BEACH:
    case Maps::Ground::GRASS:
        // Beach doesn't have its digging hole so we use it from Grass terrain.
        tileSet = 0xC0; // ICN::OBJNGRA2
        index = 9;
        return true;
    case Maps::Ground::WATER:
        // It is not possible to dig on water. Remember this!
        tileSet = 0;
        index = 255;
        return false;
    default:
        // Did you add a new terrain type? Add the logic above!
        assert( 0 );

        tileSet = 0;
        index = 255;
        break;
    }

    return false;
}

bool MP2::isDiggingHoleSprite( const int terrainType, const uint8_t tileSet, const uint32_t index )
{
    uint8_t correctTileSet = 0;
    uint32_t correctIndex = 0;

    if ( !getDiggingHoleSprite( terrainType, correctTileSet, correctIndex ) ) {
        return false;
    }

    return ( ( tileSet >> 2 ) << 2 ) == correctTileSet && index == correctIndex;
}
