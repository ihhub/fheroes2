/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

namespace
{
    bool isObjectCanBeAction( const MP2::MapObjectType objectType )
    {
        // This is a list of all objects which cannot be an action object.
        switch ( objectType ) {
        case MP2::OBJ_NONE:
        case MP2::OBJ_COAST:
        case MP2::OBJ_NOTHING_SPECIAL:
        case MP2::OBJ_MOSSY_ROCK:
        case MP2::OBJ_TAR_PIT:
        case MP2::OBJ_REEFS:
        case MP2::OBJ_TREES:
        case MP2::OBJ_MOUNTAINS:
        case MP2::OBJ_VOLCANO:
        case MP2::OBJ_FLOWERS:
        case MP2::OBJ_ROCK:
        case MP2::OBJ_WATER_LAKE:
        case MP2::OBJ_MANDRAKE:
        case MP2::OBJ_DEAD_TREE:
        case MP2::OBJ_STUMP:
        case MP2::OBJ_CRATER:
        case MP2::OBJ_CACTUS:
        case MP2::OBJ_MOUND:
        case MP2::OBJ_DUNE:
        case MP2::OBJ_LAVAPOOL:
        case MP2::OBJ_SHRUB:
        case MP2::OBJ_SWAMPY_LAKE:
        case MP2::OBJ_FROZEN_LAKE:
            return false;
        default:
            break;
        }

        return true;
    }
}

int MP2::getIcnIdFromObjectIcnType( const ObjectIcnType objectIcnType )
{
    switch ( objectIcnType ) {
    case OBJ_ICN_TYPE_UNKNOWN:
        return ICN::UNKNOWN;
    case OBJ_ICN_TYPE_BOAT32:
        return ICN::BOAT32;
    case OBJ_ICN_TYPE_OBJNARTI:
        return ICN::OBJNARTI;
    case OBJ_ICN_TYPE_MONS32:
        return ICN::MONS32;
    case OBJ_ICN_TYPE_FLAG32:
        return ICN::FLAG32;
    case OBJ_ICN_TYPE_MINIHERO:
        return ICN::MINIHERO;
    case OBJ_ICN_TYPE_MTNSNOW:
        return ICN::MTNSNOW;
    case OBJ_ICN_TYPE_MTNSWMP:
        return ICN::MTNSWMP;
    case OBJ_ICN_TYPE_MTNLAVA:
        return ICN::MTNLAVA;
    case OBJ_ICN_TYPE_MTNDSRT:
        return ICN::MTNDSRT;
    case OBJ_ICN_TYPE_MTNDIRT:
        return ICN::MTNDIRT;
    case OBJ_ICN_TYPE_MTNMULT:
        return ICN::MTNMULT;
    case OBJ_ICN_TYPE_EXTRAOVR:
        return ICN::EXTRAOVR;
    case OBJ_ICN_TYPE_ROAD:
        return ICN::ROAD;
    case OBJ_ICN_TYPE_MTNCRCK:
        return ICN::MTNCRCK;
    case OBJ_ICN_TYPE_MTNGRAS:
        return ICN::MTNGRAS;
    case OBJ_ICN_TYPE_TREJNGL:
        return ICN::TREJNGL;
    case OBJ_ICN_TYPE_TREEVIL:
        return ICN::TREEVIL;
    case OBJ_ICN_TYPE_OBJNTOWN:
        return ICN::OBJNTOWN;
    case OBJ_ICN_TYPE_OBJNTWBA:
        return ICN::OBJNTWBA;
    case OBJ_ICN_TYPE_OBJNTWSH:
        return ICN::OBJNTWSH;
    case OBJ_ICN_TYPE_OBJNTWRD:
        return ICN::OBJNTWRD;
    case OBJ_ICN_TYPE_OBJNXTRA:
        return ICN::OBJNXTRA;
    case OBJ_ICN_TYPE_OBJNWAT2:
        return ICN::OBJNWAT2;
    case OBJ_ICN_TYPE_OBJNMUL2:
        return ICN::OBJNMUL2;
    case OBJ_ICN_TYPE_TRESNOW:
        return ICN::TRESNOW;
    case OBJ_ICN_TYPE_TREFIR:
        return ICN::TREFIR;
    case OBJ_ICN_TYPE_TREFALL:
        return ICN::TREFALL;
    case OBJ_ICN_TYPE_STREAM:
        return ICN::STREAM;
    case OBJ_ICN_TYPE_OBJNRSRC:
        return ICN::OBJNRSRC;
    case OBJ_ICN_TYPE_OBJNGRA2:
        return ICN::OBJNGRA2;
    case OBJ_ICN_TYPE_TREDECI:
        return ICN::TREDECI;
    case OBJ_ICN_TYPE_OBJNWATR:
        return ICN::OBJNWATR;
    case OBJ_ICN_TYPE_OBJNGRAS:
        return ICN::OBJNGRAS;
    case OBJ_ICN_TYPE_OBJNSNOW:
        return ICN::OBJNSNOW;
    case OBJ_ICN_TYPE_OBJNSWMP:
        return ICN::OBJNSWMP;
    case OBJ_ICN_TYPE_OBJNLAVA:
        return ICN::OBJNLAVA;
    case OBJ_ICN_TYPE_OBJNDSRT:
        return ICN::OBJNDSRT;
    case OBJ_ICN_TYPE_OBJNDIRT:
        return ICN::OBJNDIRT;
    case OBJ_ICN_TYPE_OBJNCRCK:
        return ICN::OBJNCRCK;
    case OBJ_ICN_TYPE_OBJNLAV3:
        return ICN::OBJNLAV3;
    case OBJ_ICN_TYPE_OBJNMULT:
        return ICN::OBJNMULT;
    case OBJ_ICN_TYPE_OBJNLAV2:
        return ICN::OBJNLAV2;
    case OBJ_ICN_TYPE_X_LOC1:
        if ( Settings::Get().isPriceOfLoyaltySupported() ) {
            return ICN::X_LOC1;
        }
        break;
    case OBJ_ICN_TYPE_X_LOC2:
        if ( Settings::Get().isPriceOfLoyaltySupported() ) {
            return ICN::X_LOC2;
        }
        break;
    case OBJ_ICN_TYPE_X_LOC3:
        if ( Settings::Get().isPriceOfLoyaltySupported() ) {
            return ICN::X_LOC3;
        }
        break;
    default:
        break;
    }

    DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown object ICN type: " << static_cast<int>( objectIcnType ) )
    return ICN::UNKNOWN;
}

bool MP2::isHiddenForPuzzle( const int terrainType, const ObjectIcnType objectIcnType, uint8_t index )
{
    switch ( objectIcnType ) {
    case OBJ_ICN_TYPE_UNKNOWN:
    case OBJ_ICN_TYPE_UNUSED_1:
    case OBJ_ICN_TYPE_UNUSED_2:
    case OBJ_ICN_TYPE_UNUSED_3:
    case OBJ_ICN_TYPE_UNUSED_4:
    case OBJ_ICN_TYPE_UNUSED_5:
    case OBJ_ICN_TYPE_BOAT32:
    case OBJ_ICN_TYPE_UNUSED_7:
    case OBJ_ICN_TYPE_UNUSED_8:
    case OBJ_ICN_TYPE_UNUSED_9:
    case OBJ_ICN_TYPE_OBJNHAUN:
    case OBJ_ICN_TYPE_OBJNARTI:
    case OBJ_ICN_TYPE_MONS32:
    case OBJ_ICN_TYPE_UNUSED_13:
    case OBJ_ICN_TYPE_FLAG32:
    case OBJ_ICN_TYPE_UNUSED_15:
    case OBJ_ICN_TYPE_UNUSED_16:
    case OBJ_ICN_TYPE_UNUSED_17:
    case OBJ_ICN_TYPE_UNUSED_18:
    case OBJ_ICN_TYPE_UNUSED_19:
    case OBJ_ICN_TYPE_MINIMON:
    case OBJ_ICN_TYPE_MINIHERO:
    case OBJ_ICN_TYPE_OBJNRSRC:
        return true;
    case OBJ_ICN_TYPE_OBJNMULT:
        // Campfire.
        return ( index >= 124 && index <= 137 );
    default:
        // TODO: verify whether we need to show pickup objects in water.
        break;
    }

    return isDiggingHoleSprite( terrainType, objectIcnType, index );
}

const char * MP2::StringObject( MapObjectType objectType, const int count )
{
    if ( ( objectType & OBJ_ACTION_OBJECT_TYPE ) == OBJ_ACTION_OBJECT_TYPE ) {
        objectType = static_cast<MapObjectType>( objectType & ~OBJ_ACTION_OBJECT_TYPE );
    }

    switch ( objectType ) {
    case OBJ_NONE:
        return _( "No object" );
    case OBJ_NON_ACTION_ALCHEMIST_LAB:
        return _( "Alchemist Lab" );
    case OBJ_NON_ACTION_SIGN:
        return _( "Sign" );
    case OBJ_NON_ACTION_BUOY:
        return _( "Buoy" );
    case OBJ_NON_ACTION_SKELETON:
        return _( "Skeleton" );
    case OBJ_NON_ACTION_DAEMON_CAVE:
        return _( "Daemon Cave" );
    case OBJ_NON_ACTION_TREASURE_CHEST:
        return _( "Treasure Chest" );
    case OBJ_NON_ACTION_FAERIE_RING:
        return _( "Faerie Ring" );
    case OBJ_NON_ACTION_CAMPFIRE:
        return _( "Campfire" );
    case OBJ_NON_ACTION_FOUNTAIN:
        return _( "Fountain" );
    case OBJ_NON_ACTION_GAZEBO:
        return _( "Gazebo" );
    case OBJ_NON_ACTION_GENIE_LAMP:
        return _( "Genie Lamp" );
    case OBJ_NON_ACTION_GRAVEYARD:
        return _( "Graveyard" );
    case OBJ_NON_ACTION_ARCHER_HOUSE:
        return _( "Archer's House" );
    case OBJ_NON_ACTION_GOBLIN_HUT:
        return _( "Goblin Hut" );
    case OBJ_NON_ACTION_DWARF_COTTAGE:
        return _( "Dwarf Cottage" );
    case OBJ_NON_ACTION_PEASANT_HUT:
        return _( "Peasant Hut" );
    case OBJ_NON_ACTION_STABLES:
        return _( "Stables" );
    case OBJ_NON_ACTION_ALCHEMIST_TOWER:
        return _( "Alchemist's Tower" );
    case OBJ_NON_ACTION_EVENT:
        return _( "Event" );
    case OBJ_NON_ACTION_DRAGON_CITY:
        return _( "Dragon City" );
    case OBJ_NON_ACTION_LIGHTHOUSE:
        return _n( "Lighthouse", "Lighthouses", count );
    case OBJ_NON_ACTION_WATER_WHEEL:
        return _n( "Water Wheel", "Water Wheels", count );
    case OBJ_NON_ACTION_MINE:
        return _( "Mine" );
    case OBJ_NON_ACTION_MONSTER:
        return _( "Monster" );
    case OBJ_NON_ACTION_OBELISK:
        return _( "Obelisk" );
    case OBJ_NON_ACTION_OASIS:
        return _( "Oasis" );
    case OBJ_NON_ACTION_RESOURCE:
        return _( "Resource" );
    case OBJ_COAST:
        return _( "Beach" );
    case OBJ_NON_ACTION_SAWMILL:
        return _( "Sawmill" );
    case OBJ_NON_ACTION_ORACLE:
        return _( "Oracle" );
    case OBJ_NON_ACTION_SHRINE_FIRST_CIRCLE:
        return _( "Shrine of the First Circle" );
    case OBJ_NON_ACTION_SHIPWRECK:
        return _( "Shipwreck" );
    case OBJ_NON_ACTION_SEA_CHEST:
        return _( "Sea Chest" );
    case OBJ_NON_ACTION_DESERT_TENT:
        return _( "Desert Tent" );
    case OBJ_NON_ACTION_CASTLE:
        return _( "Castle" );
    case OBJ_NON_ACTION_STONE_LITHS:
        return _( "Stone Liths" );
    case OBJ_NON_ACTION_WAGON_CAMP:
        return _( "Wagon Camp" );
    case OBJ_NON_ACTION_HUT_OF_MAGI:
        return _( "Hut of the Magi" );
    case OBJ_NON_ACTION_WHIRLPOOL:
        return _( "Whirlpool" );
    case OBJ_NON_ACTION_WINDMILL:
        return _n( "Windmill", "Windmills", count );
    case OBJ_NON_ACTION_ARTIFACT:
        return _( "Artifact" );
    case OBJ_NON_ACTION_MERMAID:
        return _( "Mermaid" );
    case OBJ_NON_ACTION_BOAT:
        return _( "Boat" );
    case OBJ_NON_ACTION_RANDOM_ULTIMATE_ARTIFACT:
        return _( "Random Ultimate Artifact" );
    case OBJ_NON_ACTION_RANDOM_ARTIFACT:
        return _( "Random Artifact" );
    case OBJ_NON_ACTION_RANDOM_RESOURCE:
        return _( "Random Resource" );
    case OBJ_NON_ACTION_RANDOM_MONSTER:
        return _( "Random Monster" );
    case OBJ_NON_ACTION_RANDOM_TOWN:
        return _( "Random Town" );
    case OBJ_NON_ACTION_RANDOM_CASTLE:
        return _( "Random Castle" );
    case OBJ_NON_ACTION_EYE_OF_MAGI:
        return _( "Eye of the Magi" );
    case OBJ_NON_ACTION_RANDOM_MONSTER_WEAK:
        return _( "Random Monster - weak" );
    case OBJ_NON_ACTION_RANDOM_MONSTER_MEDIUM:
        return _( "Random Monster - medium" );
    case OBJ_NON_ACTION_RANDOM_MONSTER_STRONG:
        return _( "Random Monster - strong" );
    case OBJ_NON_ACTION_RANDOM_MONSTER_VERY_STRONG:
        return _( "Random Monster - very strong" );
    case OBJ_NON_ACTION_HERO:
        return _( "Hero" );
    case OBJ_NOTHING_SPECIAL:
        return _( "Nothing Special" );
    case OBJ_MOSSY_ROCK:
        return _( "Mossy Rock" );
    case OBJ_NON_ACTION_WATCH_TOWER:
        return _( "Watch Tower" );
    case OBJ_NON_ACTION_TREE_HOUSE:
        return _( "Tree House" );
    case OBJ_NON_ACTION_TREE_CITY:
        return _( "Tree City" );
    case OBJ_NON_ACTION_RUINS:
        return _( "Ruins" );
    case OBJ_NON_ACTION_FORT:
        return _( "Fort" );
    case OBJ_NON_ACTION_TRADING_POST:
        return _( "Trading Post" );
    case OBJ_NON_ACTION_ABANDONED_MINE:
        return _( "Abandoned Mine" );
    case OBJ_NON_ACTION_SIRENS:
        return _( "Sirens" );
    case OBJ_NON_ACTION_STANDING_STONES:
        return _( "Standing Stones" );
    case OBJ_NON_ACTION_IDOL:
        return _( "Idol" );
    case OBJ_NON_ACTION_TREE_OF_KNOWLEDGE:
        return _( "Tree of Knowledge" );
    case OBJ_NON_ACTION_WITCH_DOCTORS_HUT:
        return _( "Witch Doctor's Hut" );
    case OBJ_NON_ACTION_TEMPLE:
        return _( "Temple" );
    case OBJ_NON_ACTION_HILL_FORT:
        return _( "Hill Fort" );
    case OBJ_NON_ACTION_HALFLING_HOLE:
        return _( "Halfling Hole" );
    case OBJ_NON_ACTION_MERCENARY_CAMP:
        return _( "Mercenary Camp" );
    case OBJ_NON_ACTION_SHRINE_SECOND_CIRCLE:
        return _( "Shrine of the Second Circle" );
    case OBJ_NON_ACTION_SHRINE_THIRD_CIRCLE:
        return _( "Shrine of the Third Circle" );
    case OBJ_NON_ACTION_PYRAMID:
        return _( "Pyramid" );
    case OBJ_NON_ACTION_CITY_OF_DEAD:
        return _( "City of the Dead" );
    case OBJ_NON_ACTION_EXCAVATION:
        return _( "Excavation" );
    case OBJ_NON_ACTION_SPHINX:
        return _( "Sphinx" );
    case OBJ_NON_ACTION_WAGON:
        return _( "Wagon" );
    case OBJ_TAR_PIT:
        return _( "Tar Pit" );
    case OBJ_NON_ACTION_ARTESIAN_SPRING:
        return _( "Artesian Spring" );
    case OBJ_NON_ACTION_TROLL_BRIDGE:
        return _( "Troll Bridge" );
    case OBJ_NON_ACTION_WATERING_HOLE:
        return _( "Watering Hole" );
    case OBJ_NON_ACTION_WITCHS_HUT:
        return _( "Witch's Hut" );
    case OBJ_NON_ACTION_XANADU:
        return _( "Xanadu" );
    case OBJ_NON_ACTION_CAVE:
        return _( "Cave" );
    case OBJ_NON_ACTION_LEAN_TO:
        return _( "Lean-To" );
    case OBJ_NON_ACTION_MAGELLANS_MAPS:
        return _( "Magellan's Maps" );
    case OBJ_NON_ACTION_FLOTSAM:
        return _( "Flotsam" );
    case OBJ_NON_ACTION_DERELICT_SHIP:
        return _( "Derelict Ship" );
    case OBJ_NON_ACTION_SHIPWRECK_SURVIVOR:
        return _( "Shipwreck Survivor" );
    case OBJ_NON_ACTION_BOTTLE:
        return _( "Bottle" );
    case OBJ_NON_ACTION_MAGIC_WELL:
        return _( "Magic Well" );
    case OBJ_NON_ACTION_MAGIC_GARDEN:
        return _n( "Magic Garden", "Magic Gardens", count );
    case OBJ_NON_ACTION_OBSERVATION_TOWER:
        return _( "Observation Tower" );
    case OBJ_NON_ACTION_FREEMANS_FOUNDRY:
        return _( "Freeman's Foundry" );
    case OBJ_REEFS:
        return _( "Reefs" );
    case OBJ_TREES:
        return _( "Trees" );
    case OBJ_MOUNTAINS:
        return _( "Mountains" );
    case OBJ_VOLCANO:
        return _( "Volcano" );
    case OBJ_FLOWERS:
        return _( "Flowers" );
    case OBJ_ROCK:
        return _( "Rock" );
    case OBJ_WATER_LAKE:
        return _( "Water Lake" );
    case OBJ_MANDRAKE:
        return _( "Mandrake" );
    case OBJ_DEAD_TREE:
        return _( "Dead Tree" );
    case OBJ_STUMP:
        return _( "Stump" );
    case OBJ_CRATER:
        return _( "Crater" );
    case OBJ_CACTUS:
        return _( "Cactus" );
    case OBJ_MOUND:
        return _( "Mound" );
    case OBJ_DUNE:
        return _( "Dune" );
    case OBJ_LAVAPOOL:
        return _( "Lava Pool" );
    case OBJ_SHRUB:
        return _( "Shrub" );
    case OBJ_NON_ACTION_ARENA:
        return _( "Arena" );
    case OBJ_NON_ACTION_BARROW_MOUNDS:
        return _( "Barrow Mounds" );
    case OBJ_NON_ACTION_RANDOM_ARTIFACT_TREASURE:
        return _( "Random Artifact - Treasure" );
    case OBJ_NON_ACTION_RANDOM_ARTIFACT_MINOR:
        return _( "Random Artifact - Minor" );
    case OBJ_NON_ACTION_RANDOM_ARTIFACT_MAJOR:
        return _( "Random Artifact - Major" );
    case OBJ_NON_ACTION_BARRIER:
        return _( "Barrier" );
    case OBJ_NON_ACTION_TRAVELLER_TENT:
        return _( "Traveller's Tent" );
    case OBJ_NON_ACTION_EXPANSION_DWELLING:
        return "Expansion Dwelling";
    case OBJ_NON_ACTION_EXPANSION_OBJECT:
        return "Expansion Object";
    case OBJ_NON_ACTION_JAIL:
        return _( "Jail" );
    case OBJ_NON_ACTION_FIRE_ALTAR:
        return _( "Fire Summoning Altar" );
    case OBJ_NON_ACTION_AIR_ALTAR:
        return _( "Air Summoning Altar" );
    case OBJ_NON_ACTION_EARTH_ALTAR:
        return _( "Earth Summoning Altar" );
    case OBJ_NON_ACTION_WATER_ALTAR:
        return _( "Water Summoning Altar" );
    case OBJ_SWAMPY_LAKE:
        return _( "Swampy Lake" );
    case OBJ_FROZEN_LAKE:
        return _( "Frozen Lake" );
    default:
        // Did you add a new object type? Add the logic above!
        assert( 0 );
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown object type: " << static_cast<int>( objectType ) )
        break;
    }

    return nullptr;
}

bool MP2::isDayLife( const MapObjectType objectType )
{
    // Only one object on Adventure Map restores every day and this is Magic Well.
    return ( objectType == OBJ_MAGIC_WELL );
}

bool MP2::isWeekLife( const MapObjectType objectType )
{
    // TODO: list week object life
    switch ( objectType ) {
    case OBJ_STABLES:
    case OBJ_MAGIC_GARDEN:
    case OBJ_WATER_WHEEL:
    case OBJ_WINDMILL:
    case OBJ_ARTESIAN_SPRING:
    // join army
    case OBJ_WATCH_TOWER:
    case OBJ_EXCAVATION:
    case OBJ_CAVE:
    case OBJ_TREE_HOUSE:
    case OBJ_ARCHER_HOUSE:
    case OBJ_GOBLIN_HUT:
    case OBJ_DWARF_COTTAGE:
    case OBJ_HALFLING_HOLE:
    case OBJ_PEASANT_HUT:
    // recruit army
    case OBJ_RUINS:
    case OBJ_TREE_CITY:
    case OBJ_WAGON_CAMP:
    case OBJ_DESERT_TENT:
    case OBJ_WATER_ALTAR:
    case OBJ_AIR_ALTAR:
    case OBJ_FIRE_ALTAR:
    case OBJ_EARTH_ALTAR:
    case OBJ_BARROW_MOUNDS:
    // battle and recruit army
    case OBJ_DRAGON_CITY:
    case OBJ_CITY_OF_DEAD:
    case OBJ_TROLL_BRIDGE:
    // for AI
    case OBJ_HERO:
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
    case OBJ_FAERIE_RING:
    case OBJ_PYRAMID:
    // morale modificators
    case OBJ_BUOY:
    case OBJ_OASIS:
    case OBJ_TEMPLE:
    case OBJ_WATERING_HOLE:
    case OBJ_GRAVEYARD:
    case OBJ_DERELICT_SHIP:
    case OBJ_SHIPWRECK:
    case OBJ_MERMAID:
        return true;
    default:
        break;
    }

    return false;
}

bool MP2::isInGameActionObject( const MapObjectType objectType, const bool accessedFromWater )
{
    if ( accessedFromWater ) {
        return isWaterActionObject( objectType );
    }

    return isInGameActionObject( objectType );
}

bool MP2::isWaterActionObject( const MapObjectType objectType )
{
    switch ( objectType ) {
    // These are the types of objects that can be placed on water tiles by the original editor and,
    // therefore, should be accessible to the hero who is on board the boat (yes, artifacts too).
    case OBJ_ARTIFACT:
    case OBJ_BOTTLE:
    case OBJ_BUOY:
    case OBJ_COAST:
    case OBJ_DERELICT_SHIP:
    case OBJ_FLOTSAM:
    // Heroes cannot be placed on water by the original editor, but they can board a boat
    case OBJ_HERO:
    case OBJ_MAGELLANS_MAPS:
    case OBJ_MERMAID:
    case OBJ_SEA_CHEST:
    case OBJ_SHIPWRECK:
    case OBJ_SHIPWRECK_SURVIVOR:
    case OBJ_SIRENS:
    case OBJ_WHIRLPOOL:
        return true;

    case OBJ_BOAT:
    case OBJ_CASTLE:
        return false;

    default:
        break;
    }

    // Here we would have to return false, but some map editors allow to place arbitrary objects
    // on water tiles, so we have to work with this.
    return isInGameActionObject( objectType );
}

bool MP2::isInGameActionObject( const MapObjectType objectType )
{
    if ( ( objectType & OBJ_ACTION_OBJECT_TYPE ) != OBJ_ACTION_OBJECT_TYPE ) {
        // It is not an action object.
        return false;
    }

    if ( objectType == OBJ_EVENT ) {
        // This is the only edge case object type inherited from the original game.
        return false;
    }

    return isObjectCanBeAction( static_cast<MapObjectType>( objectType & ~OBJ_ACTION_OBJECT_TYPE ) );
}

bool MP2::isOffGameActionObject( const MapObjectType objectType )
{
    return ( objectType & OBJ_ACTION_OBJECT_TYPE ) == OBJ_ACTION_OBJECT_TYPE;
}

MP2::MapObjectType MP2::getBaseActionObjectType( const MapObjectType objectType )
{
    if ( ( objectType & OBJ_ACTION_OBJECT_TYPE ) == OBJ_ACTION_OBJECT_TYPE ) {
        // This is an action object which is considered as base.
        return objectType;
    }

    if ( !isObjectCanBeAction( objectType ) ) {
        return objectType;
    }

    return static_cast<MapObjectType>( objectType | OBJ_ACTION_OBJECT_TYPE );
}

bool MP2::isValuableResourceObject( const MapObjectType objectType )
{
    // Sort things in alphabetical order for better readability.
    switch ( objectType ) {
    case OBJ_CAMPFIRE:
    case OBJ_DAEMON_CAVE:
    case OBJ_DERELICT_SHIP:
    case OBJ_FLOTSAM:
    case OBJ_GRAVEYARD:
    case OBJ_LEAN_TO:
    case OBJ_MAGIC_GARDEN:
    case OBJ_PYRAMID:
    case OBJ_SEA_CHEST:
    case OBJ_SHIPWRECK:
    case OBJ_SHIPWRECK_SURVIVOR:
    case OBJ_SKELETON:
    case OBJ_WAGON:
    case OBJ_WATER_WHEEL:
    case OBJ_WINDMILL:
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
    // Sort things in alphabetical order for better readability.
    switch ( objectType ) {
    case OBJ_ALCHEMIST_LAB:
    case OBJ_CASTLE:
    case OBJ_LIGHTHOUSE:
    case OBJ_MINE:
    case OBJ_SAWMILL:
        return true;
    default:
        break;
    }

    return false;
}

bool MP2::isPickupObject( const MapObjectType objectType )
{
    // Sort things in alphabetical order for better readability.
    switch ( objectType ) {
    case OBJ_ARTIFACT:
    case OBJ_BOTTLE:
    case OBJ_CAMPFIRE:
    case OBJ_FLOTSAM:
    case OBJ_GENIE_LAMP:
    case OBJ_RESOURCE:
    case OBJ_SEA_CHEST:
    case OBJ_SHIPWRECK_SURVIVOR:
    case OBJ_TREASURE_CHEST:
        return true;
    default:
        break;
    }

    return false;
}

bool MP2::isArtifactObject( const MapObjectType objectType )
{
    // Sort things in alphabetical order for better readability.
    switch ( objectType ) {
    case OBJ_ARTIFACT:
    case OBJ_DAEMON_CAVE:
    case OBJ_GRAVEYARD:
    case OBJ_SEA_CHEST:
    case OBJ_SHIPWRECK:
    case OBJ_SHIPWRECK_SURVIVOR:
    case OBJ_SKELETON:
    case OBJ_TREASURE_CHEST:
    case OBJ_WAGON:
        return true;
    default:
        break;
    }

    return false;
}

bool MP2::isBattleMandatoryifObjectIsProtected( const MapObjectType objectType )
{
    // Sort things in alphabetical order for better readability.
    switch ( objectType ) {
    // If the artifact is guarded by rogues, it is impossible to refuse a fight.
    case OBJ_ARTIFACT:
    case OBJ_MONSTER:
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
    case OBJ_STONE_LITHS:
    case OBJ_WHIRLPOOL:
    // Sign messages are useless for AI, but they are harmless for fog discovery purposes
    case OBJ_SIGN:
        return true;
    default:
        break;
    }

    // Action objects in general should be avoided for fog discovery purposes, because
    // they may be guarded or may require wasting resources
    return !isInGameActionObject( objectType );
}

bool MP2::isNeedStayFront( const MapObjectType objectType )
{
    // Sort things in alphabetical order for better readability.
    switch ( objectType ) {
    case OBJ_BARRIER:
    case OBJ_BOAT:
    case OBJ_BUOY:
    case OBJ_HERO:
    case OBJ_JAIL:
    case OBJ_MERMAID:
    case OBJ_MONSTER:
    case OBJ_SHIPWRECK:
    case OBJ_SIRENS:
    case OBJ_SKELETON:
        return true;
    default:
        break;
    }

    return isPickupObject( objectType );
}

int MP2::getActionObjectDirection( const MapObjectType objectType )
{
    switch ( objectType ) {
    case OBJ_ARTIFACT:
    case OBJ_BARRIER:
    case OBJ_BOAT:
    case OBJ_BOTTLE:
    case OBJ_BUOY:
    case OBJ_CAMPFIRE:
    case OBJ_COAST:
    case OBJ_EVENT:
    case OBJ_FLOTSAM:
    case OBJ_GENIE_LAMP:
    case OBJ_HERO:
    case OBJ_JAIL:
    case OBJ_MONSTER:
    case OBJ_RANDOM_ARTIFACT:
    case OBJ_RANDOM_ARTIFACT_MAJOR:
    case OBJ_RANDOM_ARTIFACT_MINOR:
    case OBJ_RANDOM_ARTIFACT_TREASURE:
    case OBJ_RANDOM_CASTLE:
    case OBJ_RANDOM_MONSTER:
    case OBJ_RANDOM_MONSTER_MEDIUM:
    case OBJ_RANDOM_MONSTER_STRONG:
    case OBJ_RANDOM_MONSTER_VERY_STRONG:
    case OBJ_RANDOM_MONSTER_WEAK:
    case OBJ_RANDOM_TOWN:
    case OBJ_RANDOM_ULTIMATE_ARTIFACT:
    case OBJ_RANDOM_RESOURCE:
    case OBJ_RESOURCE:
    case OBJ_SEA_CHEST:
    case OBJ_SHIPWRECK_SURVIVOR:
    case OBJ_SKELETON:
    case OBJ_TREASURE_CHEST:
    case OBJ_WHIRLPOOL:
        return DIRECTION_ALL;
    case OBJ_ABANDONED_MINE:
    case OBJ_AIR_ALTAR:
    case OBJ_ALCHEMIST_LAB:
    case OBJ_ALCHEMIST_TOWER:
    case OBJ_ARCHER_HOUSE:
    case OBJ_ARENA:
    case OBJ_ARTESIAN_SPRING:
    case OBJ_BARROW_MOUNDS:
    case OBJ_CASTLE:
    case OBJ_CAVE:
    case OBJ_CITY_OF_DEAD:
    case OBJ_DAEMON_CAVE:
    case OBJ_DERELICT_SHIP:
    case OBJ_DESERT_TENT:
    case OBJ_DRAGON_CITY:
    case OBJ_DWARF_COTTAGE:
    case OBJ_EARTH_ALTAR:
    case OBJ_EXCAVATION:
    case OBJ_EYE_OF_MAGI:
    case OBJ_FAERIE_RING:
    case OBJ_FIRE_ALTAR:
    case OBJ_FORT:
    case OBJ_FOUNTAIN:
    case OBJ_FREEMANS_FOUNDRY:
    case OBJ_GAZEBO:
    case OBJ_GOBLIN_HUT:
    case OBJ_GRAVEYARD:
    case OBJ_HALFLING_HOLE:
    case OBJ_HILL_FORT:
    case OBJ_HUT_OF_MAGI:
    case OBJ_IDOL:
    case OBJ_LEAN_TO:
    case OBJ_LIGHTHOUSE:
    case OBJ_MAGELLANS_MAPS:
    case OBJ_MAGIC_GARDEN:
    case OBJ_MAGIC_WELL:
    case OBJ_MERCENARY_CAMP:
    case OBJ_MERMAID:
    case OBJ_MINE:
    case OBJ_OASIS:
    case OBJ_OBELISK:
    case OBJ_OBSERVATION_TOWER:
    case OBJ_ORACLE:
    case OBJ_PEASANT_HUT:
    case OBJ_PYRAMID:
    case OBJ_RUINS:
    case OBJ_SAWMILL:
    case OBJ_SHIPWRECK:
    case OBJ_SHRINE_FIRST_CIRCLE:
    case OBJ_SHRINE_SECOND_CIRCLE:
    case OBJ_SHRINE_THIRD_CIRCLE:
    case OBJ_SIGN:
    case OBJ_SIRENS:
    case OBJ_SPHINX:
    case OBJ_STABLES:
    case OBJ_STANDING_STONES:
    case OBJ_STONE_LITHS:
    case OBJ_TEMPLE:
    case OBJ_TRADING_POST:
    case OBJ_TRAVELLER_TENT:
    case OBJ_TREE_CITY:
    case OBJ_TREE_HOUSE:
    case OBJ_TREE_OF_KNOWLEDGE:
    case OBJ_TROLL_BRIDGE:
    case OBJ_WAGON:
    case OBJ_WAGON_CAMP:
    case OBJ_WATCH_TOWER:
    case OBJ_WATER_ALTAR:
    case OBJ_WATER_WHEEL:
    case OBJ_WATERING_HOLE:
    case OBJ_WINDMILL:
    case OBJ_WITCH_DOCTORS_HUT:
    case OBJ_WITCHS_HUT:
    case OBJ_XANADU:
        return DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW;
    default:
        // Did you add a new action object? Please add its passability!
        assert( 0 );
        break;
    }

    return Direction::UNKNOWN;
}

bool MP2::getDiggingHoleSprite( const int terrainType, ObjectIcnType & objectIcnType, uint8_t & index )
{
    switch ( terrainType ) {
    case Maps::Ground::DESERT:
        objectIcnType = OBJ_ICN_TYPE_OBJNDSRT;
        index = 68;
        return true;
    case Maps::Ground::SNOW:
        objectIcnType = OBJ_ICN_TYPE_OBJNSNOW;
        index = 11;
        return true;
    case Maps::Ground::SWAMP:
        objectIcnType = OBJ_ICN_TYPE_OBJNSWMP;
        index = 86;
        return true;
    case Maps::Ground::WASTELAND:
        objectIcnType = OBJ_ICN_TYPE_OBJNCRCK;
        index = 70;
        return true;
    case Maps::Ground::LAVA:
        objectIcnType = OBJ_ICN_TYPE_OBJNLAVA;
        index = 26;
        return true;
    case Maps::Ground::DIRT:
        objectIcnType = OBJ_ICN_TYPE_OBJNDIRT;
        index = 140;
        return true;
    case Maps::Ground::BEACH:
    case Maps::Ground::GRASS:
        // Beach doesn't have its digging hole so we use it from Grass terrain.
        objectIcnType = OBJ_ICN_TYPE_OBJNGRA2;
        index = 9;
        return true;
    case Maps::Ground::WATER:
        // It is not possible to dig on water. Remember this!
        objectIcnType = OBJ_ICN_TYPE_UNKNOWN;
        index = 255;
        return false;
    default:
        // Did you add a new terrain type? Add the logic above!
        assert( 0 );

        objectIcnType = OBJ_ICN_TYPE_UNKNOWN;
        index = 255;
        break;
    }

    return false;
}

bool MP2::isDiggingHoleSprite( const int terrainType, const ObjectIcnType objectIcnType, const uint8_t index )
{
    ObjectIcnType terrainObjectIcnType = OBJ_ICN_TYPE_UNKNOWN;
    uint8_t correctIndex = 0;

    if ( !getDiggingHoleSprite( terrainType, terrainObjectIcnType, correctIndex ) ) {
        return false;
    }

    return ( objectIcnType == terrainObjectIcnType ) && ( index == correctIndex );
}

bool MP2::doesObjectNeedExtendedMetadata( const MP2::MapObjectType type )
{
    switch ( type ) {
    case OBJ_BOTTLE:
    case OBJ_CASTLE:
    case OBJ_EVENT:
    case OBJ_HERO:
    case OBJ_JAIL:
    case OBJ_RANDOM_CASTLE:
    case OBJ_RANDOM_TOWN:
    case OBJ_SIGN:
    case OBJ_SPHINX:
        return true;
    default:
        break;
    }

    return false;
}

bool MP2::doesObjectContainMetadata( const MP2::MapObjectType type )
{
    if ( doesObjectNeedExtendedMetadata( type ) ) {
        // UID for an extended metadata data chunk which should be read outside Tiles object.
        return true;
    }

    switch ( type ) {
    // Only spell scrolls require additional metadata to store spell ID.
    case OBJ_ARTIFACT:
    // Traveller's tent and barriers contain a color of the object. As of now we don't use it at all.
    case OBJ_BARRIER:
    case OBJ_TRAVELLER_TENT:
    // Expansion object and dwellings contain ID to identify object type. As of now we don't use it at all.
    case OBJ_EXPANSION_DWELLING:
    case OBJ_EXPANSION_OBJECT:
    // The number of monsters.
    case OBJ_MONSTER:
    case OBJ_RANDOM_MONSTER:
    case OBJ_RANDOM_MONSTER_MEDIUM:
    case OBJ_RANDOM_MONSTER_STRONG:
    case OBJ_RANDOM_MONSTER_VERY_STRONG:
    case OBJ_RANDOM_MONSTER_WEAK:
    // Ultimate artifact contains radius value to be placed by the engine.
    case OBJ_RANDOM_ULTIMATE_ARTIFACT:
        return true;
    default:
        break;
    }

    return false;
}
