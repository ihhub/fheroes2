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
#ifndef H2MP2_H
#define H2MP2_H

#include <cstdint>

namespace MP2
{
    enum MP2Info
    {
        MP2_MAP_INFO_SIZE = 428,
        MP2_ADDON_COUNT_SIZE = 4,

        MP2_ADDON_STRUCTURE_SIZE = 15,
        MP2_CASTLE_STRUCTURE_SIZE = 70,
        MP2_EVENT_STRUCTURE_MIN_SIZE = 50,
        MP2_HEROES_STRUCTURE_SIZE = 76,
        MP2_RIDDLE_STRUCTURE_MIN_SIZE = 137,
        MP2_RUMOR_STRUCTURE_MIN_SIZE = 9,
        MP2_SIGN_STRUCTURE_MIN_SIZE = 10,
        MP2_TILE_STRUCTURE_SIZE = 20,

        MP2_CASTLE_COUNT = 72,
        MP2_CASTLE_POSITION_SIZE = 3,

        MP2_CAPTURE_OBJECT_COUNT = 144,
        MP2_CAPTURE_OBJECT_POSITION_SIZE = 3
    };

    // Tile structure from the original map format.
    struct MP2TileInfo
    {
        // Terrain image index used for terrain tile display on Adventure Map.
        uint16_t terrainImageIndex;

        // Ground (bottom) level object type (first 2 bits) and object tile set (6 bits). Tile set refers to ICN ID.
        uint8_t objectName1;

        // ICN image index (image index for corresponding ICN Id) for ground (bottom) object. 255 means it's an empty object.
        uint8_t bottomIcnImageIndex;

        // First 2 bits correspond to object layer type used to identify the order of rendering on Adventure Map.
        // The third bit is unknown. TODO: find out what the third bit is used for.
        // The last 5 bits are used together with quantity 2 as the value for the object.
        uint8_t quantity1;

        // Used as a part of quantity, field size is actually 13 bits. Has most significant bits.
        uint8_t quantity2;

        // Top level object type (first 2 bits) and object tile set (6 bits). Tile set refers to ICN ID.
        uint8_t objectName2;

        // ICN image index (image index for corresponding ICN Id) for top level object. 255 means it's an empty object.
        uint8_t topIcnImageIndex;

        // First 2 bits responsible for terrain shape (0 - 3).
        // Third, forth and fifth bits belong to tiles of water touching land (beach). There are only two combinations of them (from lowest to highest):
        // 1 0 0
        // 1 0 1
        // These two bit combinations are used to determine where water or land terrain can be drawn on the current tile.
        // Most likely these values are used only in the original Editor and have no use within the game.
        uint8_t terrainFlags;

        // The main object type for the tile. The tile can have multiple objects but the game can display information only about one.
        // Refer to MapObjectType enumeration below.
        uint8_t mapObjectType;

        // Next add-on index. Zero value means it's the last addon chunk.
        uint16_t nextAddonIndex;

        // Ground (bottom) level object UID. An object can allocate more than 1 tile. Each tile could have multiple objects pieces.
        // UID is used to find all pieces/addons which belong to the same object.
        // In Editor first object will have UID as 0. Then second object placed on the map will have UID 0 + number of pieces / tiles per previous object and etc.
        uint32_t level1ObjectUID;

        // Top level object UID. An object can allocate more than 1 tile. Each tile could have multiple objects pieces.
        // UID is used to find all pieces/addons which belong to the same object.
        // In Editor first object will have UID as 0. Then second object placed on the map will have UID 0 + number of pieces / tiles per previous object and etc.
        uint32_t level2ObjectUID;
    };

    // Addon structure from the original map format.
    struct MP2AddonInfo
    {
        // Next add-on index. Zero value means it's the last addon chunk.
        uint16_t nextAddonIndex;

        uint8_t objectNameN1; // level 1.N. Last bit indicates if object is animated. Second-last controls overlay

        // ICN image index (image index for corresponding ICN Id) for ground (bottom) object. 255 means it's an empty object.
        uint8_t bottomIcnImageIndex;

        uint8_t quantityN; // Bitfield containing metadata

        uint8_t objectNameN2; // level 2.N

        // ICN image index (image index for corresponding ICN Id) for top level object. 255 means it's an empty object.
        uint8_t topIcnImageIndex;

        // Ground (bottom) level object UID. An object can allocate more than 1 tile. Each tile could have multiple objects pieces.
        // UID is used to find all pieces/addons which belong to the same object.
        // In Editor first object will have UID as 0. Then second object placed on the map will have UID 0 + number of pieces / tiles per previous object and etc.
        uint32_t level1ObjectUID;

        // Top level object UID. An object can allocate more than 1 tile. Each tile could have multiple objects pieces.
        // UID is used to find all pieces/addons which belong to the same object.
        // In Editor first object will have UID as 0. Then second object placed on the map will have UID 0 + number of pieces / tiles per previous object and etc.
        uint32_t level2ObjectUID;
    };

    // An object type could be action and non-action. If both parts are present the difference between them must be 128.
    enum MapObjectType : uint16_t
    {
        // This section defines all types of NON-action objects which are present in the original game.
        // If the object by nature is an action object name it with prefix OBJ_NON_ACTION_.
        // Otherwise, name it with prefix OBJ_.
        OBJ_NONE = 0, // No object exist.
        OBJ_NON_ACTION_ALCHEMIST_LAB = 1,
        OBJ_NON_ACTION_SIGN = 2, // Never set in maps.
        OBJ_NON_ACTION_BUOY = 3, // Never set in maps.
        OBJ_NON_ACTION_SKELETON = 4, // Never set in maps.
        OBJ_NON_ACTION_DAEMON_CAVE = 5,
        OBJ_NON_ACTION_TREASURE_CHEST = 6, // Never set in maps.
        OBJ_NON_ACTION_FAERIE_RING = 7,
        OBJ_NON_ACTION_CAMPFIRE = 8, // Never set in maps.
        OBJ_NON_ACTION_FOUNTAIN = 9, // Never set in maps.
        OBJ_NON_ACTION_GAZEBO = 10,
        OBJ_NON_ACTION_GENIE_LAMP = 11, // Never set in maps.
        OBJ_NON_ACTION_GRAVEYARD = 12,
        OBJ_NON_ACTION_ARCHER_HOUSE = 13,
        OBJ_NON_ACTION_GOBLIN_HUT = 14, // Never set in maps.
        OBJ_NON_ACTION_DWARF_COTTAGE = 15,
        OBJ_NON_ACTION_PEASANT_HUT = 16,
        OBJ_NON_ACTION_STABLES = 17, // Never set in maps. Based on given information this was a monster dwelling called Log Cabin but set by the engine as Stables.
        OBJ_NON_ACTION_ALCHEMIST_TOWER = 18, // Never set in maps. Based on given information this was Road but set by the engine as Alchemist Tower.
        OBJ_NON_ACTION_EVENT = 19, // Never set in maps.
        OBJ_NON_ACTION_DRAGON_CITY = 20,
        OBJ_NON_ACTION_LIGHTHOUSE = 21,
        OBJ_NON_ACTION_WATER_WHEEL = 22,
        OBJ_NON_ACTION_MINE = 23,
        OBJ_NON_ACTION_MONSTER = 24, // Never set in maps.
        OBJ_NON_ACTION_OBELISK = 25,
        OBJ_NON_ACTION_OASIS = 26,
        OBJ_NON_ACTION_RESOURCE = 27, // Never set in maps.
        OBJ_COAST = 28, // This is the only object in non-action section which is considered as an action object (for some cases).
        OBJ_NON_ACTION_SAWMILL = 29,
        OBJ_NON_ACTION_ORACLE = 30,
        OBJ_NON_ACTION_SHRINE_FIRST_CIRCLE = 31, // Never set in maps.
        OBJ_NON_ACTION_SHIPWRECK = 32,
        OBJ_NON_ACTION_SEA_CHEST = 33, // Never set in maps.
        OBJ_NON_ACTION_DESERT_TENT = 34,
        OBJ_NON_ACTION_CASTLE = 35,
        OBJ_NON_ACTION_STONE_LITHS = 36,
        OBJ_NON_ACTION_WAGON_CAMP = 37,
        OBJ_NON_ACTION_HUT_OF_MAGI = 38, // Never set in maps. Based on given information this was a Well object.
        OBJ_NON_ACTION_WHIRLPOOL = 39, // Never set in maps.
        OBJ_NON_ACTION_WINDMILL = 40,
        OBJ_NON_ACTION_ARTIFACT = 41, // Never set in maps.
        OBJ_NON_ACTION_MERMAID = 42, // Never set in maps. Based on given information this was a Hero.
        OBJ_NON_ACTION_BOAT = 43, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_ULTIMATE_ARTIFACT = 44, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_ARTIFACT = 45, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_RESOURCE = 46, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_MONSTER = 47, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_TOWN = 48,
        OBJ_NON_ACTION_RANDOM_CASTLE = 49,
        OBJ_NON_ACTION_EYE_OF_MAGI = 50, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_MONSTER_WEAK = 51, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_MONSTER_MEDIUM = 52, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_MONSTER_STRONG = 53, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_MONSTER_VERY_STRONG = 54, // Never set in maps.
        OBJ_NON_ACTION_HERO = 55, // Never set in maps. This type is used for any types of heroes, including random.
        OBJ_NOTHING_SPECIAL = 56,
        OBJ_MOSSY_ROCK = 57, // It is a Rock with moss for Swamp terrain. ICN::OBJNSWMP, images 138-139. In the original game it has no name.
        OBJ_NON_ACTION_WATCH_TOWER = 58,
        OBJ_NON_ACTION_TREE_HOUSE = 59,
        OBJ_NON_ACTION_TREE_CITY = 60,
        OBJ_NON_ACTION_RUINS = 61,
        OBJ_NON_ACTION_FORT = 62,
        OBJ_NON_ACTION_TRADING_POST = 63,
        OBJ_NON_ACTION_ABANDONED_MINE = 64,
        OBJ_NON_ACTION_SIRENS = 65, // Originally it was Thatched Hut which is replaced by Peasant Hut for all original maps.
        OBJ_NON_ACTION_STANDING_STONES = 66, // Never set in maps.
        OBJ_NON_ACTION_IDOL = 67, // Never set in maps.
        OBJ_NON_ACTION_TREE_OF_KNOWLEDGE = 68,
        OBJ_NON_ACTION_WITCH_DOCTORS_HUT = 69,
        OBJ_NON_ACTION_TEMPLE = 70,
        OBJ_NON_ACTION_HILL_FORT = 71,
        OBJ_NON_ACTION_HALFLING_HOLE = 72,
        OBJ_NON_ACTION_MERCENARY_CAMP = 73,
        OBJ_NON_ACTION_SHRINE_SECOND_CIRCLE = 74, // Never set in maps.
        OBJ_NON_ACTION_SHRINE_THIRD_CIRCLE = 75, // Never set in maps.
        OBJ_NON_ACTION_PYRAMID = 76,
        OBJ_NON_ACTION_CITY_OF_DEAD = 77,
        OBJ_NON_ACTION_EXCAVATION = 78,
        OBJ_NON_ACTION_SPHINX = 79,
        OBJ_NON_ACTION_WAGON = 80, // Never set in maps.
        OBJ_TAR_PIT = 81,
        OBJ_NON_ACTION_ARTESIAN_SPRING = 82,
        OBJ_NON_ACTION_TROLL_BRIDGE = 83,
        OBJ_NON_ACTION_WATERING_HOLE = 84,
        OBJ_NON_ACTION_WITCHS_HUT = 85,
        OBJ_NON_ACTION_XANADU = 86,
        OBJ_NON_ACTION_CAVE = 87,
        OBJ_NON_ACTION_LEAN_TO = 88, // Never set in maps.
        OBJ_NON_ACTION_MAGELLANS_MAPS = 89,
        OBJ_NON_ACTION_FLOTSAM = 90, // Never set in maps.
        OBJ_NON_ACTION_DERELICT_SHIP = 91,
        OBJ_NON_ACTION_SHIPWRECK_SURVIVOR = 92, // Never set in maps.
        OBJ_NON_ACTION_BOTTLE = 93, // Never set in maps.
        OBJ_NON_ACTION_MAGIC_WELL = 94,
        OBJ_NON_ACTION_MAGIC_GARDEN = 95, // Never set in maps.
        OBJ_NON_ACTION_OBSERVATION_TOWER = 96,
        OBJ_NON_ACTION_FREEMANS_FOUNDRY = 97,
        OBJ_REEFS = 98, // Never used within the original Editor.
        OBJ_TREES = 99,
        OBJ_MOUNTAINS = 100,
        OBJ_VOLCANO = 101,
        OBJ_FLOWERS = 102,
        OBJ_ROCK = 103,
        OBJ_WATER_LAKE = 104,
        OBJ_MANDRAKE = 105,
        OBJ_DEAD_TREE = 106,
        OBJ_STUMP = 107,
        OBJ_CRATER = 108,
        OBJ_CACTUS = 109,
        OBJ_MOUND = 110,
        OBJ_DUNE = 111,
        OBJ_LAVAPOOL = 112,
        OBJ_SHRUB = 113,
        OBJ_NON_ACTION_ARENA = 114, // Never set in maps.
        OBJ_NON_ACTION_BARROW_MOUNDS = 115, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_ARTIFACT_TREASURE = 116, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_ARTIFACT_MINOR = 117, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_ARTIFACT_MAJOR = 118, // Never set in maps.
        OBJ_NON_ACTION_BARRIER = 119, // Never set in maps.
        OBJ_NON_ACTION_TRAVELLER_TENT = 120,
        OBJ_NON_ACTION_EXPANSION_DWELLING = 121,
        OBJ_NON_ACTION_EXPANSION_OBJECT = 122,
        OBJ_NON_ACTION_JAIL = 123,
        OBJ_NON_ACTION_FIRE_ALTAR = 124, // Never set in maps.
        OBJ_NON_ACTION_AIR_ALTAR = 125, // Never set in maps.
        OBJ_NON_ACTION_EARTH_ALTAR = 126, // Never set in maps.
        OBJ_NON_ACTION_WATER_ALTAR = 127, // Never set in maps.

        OBJ_ACTION_OBJECT_TYPE = 128, // NEVER use this object type to set in maps. This entry is used to determine if an object is action type.

        // This section defines all types of action objects which are present in the original game.
        // If the object by nature is an action object name it with prefix OBJ_.
        // Otherwise, name it with prefix OBJ_ACTON_.
        // The value of the object must be: non-action object value + OBJ_ACTION_OBJECT_TYPE.
        OBJ_ALCHEMIST_LAB = OBJ_NON_ACTION_ALCHEMIST_LAB + OBJ_ACTION_OBJECT_TYPE,
        OBJ_SIGN = OBJ_NON_ACTION_SIGN + OBJ_ACTION_OBJECT_TYPE,
        OBJ_BUOY = OBJ_NON_ACTION_BUOY + OBJ_ACTION_OBJECT_TYPE,
        OBJ_SKELETON = OBJ_NON_ACTION_SKELETON + OBJ_ACTION_OBJECT_TYPE,
        OBJ_DAEMON_CAVE = OBJ_NON_ACTION_DAEMON_CAVE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_TREASURE_CHEST = OBJ_NON_ACTION_TREASURE_CHEST + OBJ_ACTION_OBJECT_TYPE,
        OBJ_FAERIE_RING = OBJ_NON_ACTION_FAERIE_RING + OBJ_ACTION_OBJECT_TYPE,
        OBJ_CAMPFIRE = OBJ_NON_ACTION_CAMPFIRE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_FOUNTAIN = OBJ_NON_ACTION_FOUNTAIN + OBJ_ACTION_OBJECT_TYPE,
        OBJ_GAZEBO = OBJ_NON_ACTION_GAZEBO + OBJ_ACTION_OBJECT_TYPE,
        OBJ_GENIE_LAMP = OBJ_NON_ACTION_GENIE_LAMP + OBJ_ACTION_OBJECT_TYPE,
        OBJ_GRAVEYARD = OBJ_NON_ACTION_GRAVEYARD + OBJ_ACTION_OBJECT_TYPE,
        OBJ_ARCHER_HOUSE = OBJ_NON_ACTION_ARCHER_HOUSE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_GOBLIN_HUT = OBJ_NON_ACTION_GOBLIN_HUT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_DWARF_COTTAGE = OBJ_NON_ACTION_DWARF_COTTAGE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_PEASANT_HUT = OBJ_NON_ACTION_PEASANT_HUT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_STABLES = OBJ_NON_ACTION_STABLES + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ALCHEMIST_TOWER = OBJ_NON_ACTION_ALCHEMIST_TOWER + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_EVENT = OBJ_NON_ACTION_EVENT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_DRAGON_CITY = OBJ_NON_ACTION_DRAGON_CITY + OBJ_ACTION_OBJECT_TYPE,
        OBJ_LIGHTHOUSE = OBJ_NON_ACTION_LIGHTHOUSE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_WATER_WHEEL = OBJ_NON_ACTION_WATER_WHEEL + OBJ_ACTION_OBJECT_TYPE,
        OBJ_MINE = OBJ_NON_ACTION_MINE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_MONSTER = OBJ_NON_ACTION_MONSTER + OBJ_ACTION_OBJECT_TYPE,
        OBJ_OBELISK = OBJ_NON_ACTION_OBELISK + OBJ_ACTION_OBJECT_TYPE,
        OBJ_OASIS = OBJ_NON_ACTION_OASIS + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RESOURCE = OBJ_NON_ACTION_RESOURCE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_ACTION_COAST = OBJ_COAST + OBJ_ACTION_OBJECT_TYPE, // Never set in maps as Coast is not an action object.
        OBJ_SAWMILL = OBJ_NON_ACTION_SAWMILL + OBJ_ACTION_OBJECT_TYPE,
        OBJ_ORACLE = OBJ_NON_ACTION_ORACLE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_SHRINE_FIRST_CIRCLE = OBJ_NON_ACTION_SHRINE_FIRST_CIRCLE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_SHIPWRECK = OBJ_NON_ACTION_SHIPWRECK + OBJ_ACTION_OBJECT_TYPE,
        OBJ_SEA_CHEST = OBJ_NON_ACTION_SEA_CHEST + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_DESERT_TENT = OBJ_NON_ACTION_DESERT_TENT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_CASTLE = OBJ_NON_ACTION_CASTLE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_STONE_LITHS = OBJ_NON_ACTION_STONE_LITHS + OBJ_ACTION_OBJECT_TYPE,
        OBJ_WAGON_CAMP = OBJ_NON_ACTION_WAGON_CAMP + OBJ_ACTION_OBJECT_TYPE,
        OBJ_HUT_OF_MAGI = OBJ_NON_ACTION_HUT_OF_MAGI + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_WHIRLPOOL = OBJ_NON_ACTION_WHIRLPOOL + OBJ_ACTION_OBJECT_TYPE,
        OBJ_WINDMILL = OBJ_NON_ACTION_WINDMILL + OBJ_ACTION_OBJECT_TYPE,
        OBJ_ARTIFACT = OBJ_NON_ACTION_ARTIFACT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_MERMAID = OBJ_NON_ACTION_MERMAID + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_BOAT = OBJ_NON_ACTION_BOAT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RANDOM_ULTIMATE_ARTIFACT = OBJ_NON_ACTION_RANDOM_ULTIMATE_ARTIFACT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RANDOM_ARTIFACT = OBJ_NON_ACTION_RANDOM_ARTIFACT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RANDOM_RESOURCE = OBJ_NON_ACTION_RANDOM_RESOURCE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RANDOM_MONSTER = OBJ_NON_ACTION_RANDOM_MONSTER + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RANDOM_TOWN = OBJ_NON_ACTION_RANDOM_TOWN + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RANDOM_CASTLE = OBJ_NON_ACTION_RANDOM_CASTLE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_EYE_OF_MAGI = OBJ_NON_ACTION_EYE_OF_MAGI + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_RANDOM_MONSTER_WEAK = OBJ_NON_ACTION_RANDOM_MONSTER_WEAK + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RANDOM_MONSTER_MEDIUM = OBJ_NON_ACTION_RANDOM_MONSTER_MEDIUM + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RANDOM_MONSTER_STRONG = OBJ_NON_ACTION_RANDOM_MONSTER_STRONG + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RANDOM_MONSTER_VERY_STRONG = OBJ_NON_ACTION_RANDOM_MONSTER_VERY_STRONG + OBJ_ACTION_OBJECT_TYPE,
        OBJ_HERO = OBJ_NON_ACTION_HERO + OBJ_ACTION_OBJECT_TYPE, // This type is used for any types of heroes, including random.
        OBJ_ACTION_NOTHING_SPECIAL = OBJ_NOTHING_SPECIAL + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_MOSSY_ROCK = OBJ_MOSSY_ROCK + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_WATCH_TOWER = OBJ_NON_ACTION_WATCH_TOWER + OBJ_ACTION_OBJECT_TYPE,
        OBJ_TREE_HOUSE = OBJ_NON_ACTION_TREE_HOUSE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_TREE_CITY = OBJ_NON_ACTION_TREE_CITY + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RUINS = OBJ_NON_ACTION_RUINS + OBJ_ACTION_OBJECT_TYPE,
        OBJ_FORT = OBJ_NON_ACTION_FORT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_TRADING_POST = OBJ_NON_ACTION_TRADING_POST + OBJ_ACTION_OBJECT_TYPE,
        OBJ_ABANDONED_MINE = OBJ_NON_ACTION_ABANDONED_MINE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_SIRENS = OBJ_NON_ACTION_SIRENS + OBJ_ACTION_OBJECT_TYPE, // Originally it was Thatched Hut which is replaced by Peasant Hut for all original maps.
        OBJ_STANDING_STONES = OBJ_NON_ACTION_STANDING_STONES + OBJ_ACTION_OBJECT_TYPE,
        OBJ_IDOL = OBJ_NON_ACTION_IDOL + OBJ_ACTION_OBJECT_TYPE,
        OBJ_TREE_OF_KNOWLEDGE = OBJ_NON_ACTION_TREE_OF_KNOWLEDGE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_WITCH_DOCTORS_HUT = OBJ_NON_ACTION_WITCH_DOCTORS_HUT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_TEMPLE = OBJ_NON_ACTION_TEMPLE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_HILL_FORT = OBJ_NON_ACTION_HILL_FORT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_HALFLING_HOLE = OBJ_NON_ACTION_HALFLING_HOLE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_MERCENARY_CAMP = OBJ_NON_ACTION_MERCENARY_CAMP + OBJ_ACTION_OBJECT_TYPE,
        OBJ_SHRINE_SECOND_CIRCLE = OBJ_NON_ACTION_SHRINE_SECOND_CIRCLE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_SHRINE_THIRD_CIRCLE = OBJ_NON_ACTION_SHRINE_THIRD_CIRCLE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_PYRAMID = OBJ_NON_ACTION_PYRAMID + OBJ_ACTION_OBJECT_TYPE,
        OBJ_CITY_OF_DEAD = OBJ_NON_ACTION_CITY_OF_DEAD + OBJ_ACTION_OBJECT_TYPE,
        OBJ_EXCAVATION = OBJ_NON_ACTION_EXCAVATION + OBJ_ACTION_OBJECT_TYPE,
        OBJ_SPHINX = OBJ_NON_ACTION_SPHINX + OBJ_ACTION_OBJECT_TYPE,
        OBJ_WAGON = OBJ_NON_ACTION_WAGON + OBJ_ACTION_OBJECT_TYPE,
        OBJ_ACTION_TAR_PIT = OBJ_TAR_PIT + OBJ_ACTION_OBJECT_TYPE, // Never set in maps. This is not an action object.
        OBJ_ARTESIAN_SPRING = OBJ_NON_ACTION_ARTESIAN_SPRING + OBJ_ACTION_OBJECT_TYPE,
        OBJ_TROLL_BRIDGE = OBJ_NON_ACTION_TROLL_BRIDGE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_WATERING_HOLE = OBJ_NON_ACTION_WATERING_HOLE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_WITCHS_HUT = OBJ_NON_ACTION_WITCHS_HUT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_XANADU = OBJ_NON_ACTION_XANADU + OBJ_ACTION_OBJECT_TYPE,
        OBJ_CAVE = OBJ_NON_ACTION_CAVE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_LEAN_TO = OBJ_NON_ACTION_LEAN_TO + OBJ_ACTION_OBJECT_TYPE,
        OBJ_MAGELLANS_MAPS = OBJ_NON_ACTION_MAGELLANS_MAPS + OBJ_ACTION_OBJECT_TYPE,
        OBJ_FLOTSAM = OBJ_NON_ACTION_FLOTSAM + OBJ_ACTION_OBJECT_TYPE,
        OBJ_DERELICT_SHIP = OBJ_NON_ACTION_DERELICT_SHIP + OBJ_ACTION_OBJECT_TYPE,
        OBJ_SHIPWRECK_SURVIVOR = OBJ_NON_ACTION_SHIPWRECK_SURVIVOR + OBJ_ACTION_OBJECT_TYPE,
        OBJ_BOTTLE = OBJ_NON_ACTION_BOTTLE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_MAGIC_WELL = OBJ_NON_ACTION_MAGIC_WELL + OBJ_ACTION_OBJECT_TYPE,
        OBJ_MAGIC_GARDEN = OBJ_NON_ACTION_MAGIC_GARDEN + OBJ_ACTION_OBJECT_TYPE,
        OBJ_OBSERVATION_TOWER = OBJ_NON_ACTION_OBSERVATION_TOWER + OBJ_ACTION_OBJECT_TYPE,
        OBJ_FREEMANS_FOUNDRY = OBJ_NON_ACTION_FREEMANS_FOUNDRY + OBJ_ACTION_OBJECT_TYPE,
        OBJ_ACTION_REEFS = OBJ_REEFS + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_TREES = OBJ_TREES + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_MOUNTAINS = OBJ_MOUNTAINS + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_VOLCANO = OBJ_VOLCANO + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_FLOWERS = OBJ_FLOWERS + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_ROCK = OBJ_ROCK + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_WATER_LAKE = OBJ_WATER_LAKE + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_MANDRAKE = OBJ_MANDRAKE + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_DEAD_TREE = OBJ_DEAD_TREE + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_STUMP = OBJ_STUMP + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_CRATER = OBJ_CRATER + OBJ_ACTION_OBJECT_TYPE, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_ACTION_CACTUS = OBJ_CACTUS + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_MOUND = OBJ_MOUND + OBJ_ACTION_OBJECT_TYPE, // Never set in maps
        OBJ_ACTION_DUNE = OBJ_DUNE + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_LAVAPOOL = OBJ_LAVAPOOL + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ACTION_SHRUB = OBJ_SHRUB + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_ARENA = OBJ_NON_ACTION_ARENA + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_BARROW_MOUNDS = OBJ_NON_ACTION_BARROW_MOUNDS + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_RANDOM_ARTIFACT_TREASURE = OBJ_NON_ACTION_RANDOM_ARTIFACT_TREASURE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RANDOM_ARTIFACT_MINOR = OBJ_NON_ACTION_RANDOM_ARTIFACT_MINOR + OBJ_ACTION_OBJECT_TYPE,
        OBJ_RANDOM_ARTIFACT_MAJOR = OBJ_NON_ACTION_RANDOM_ARTIFACT_MAJOR + OBJ_ACTION_OBJECT_TYPE,
        OBJ_BARRIER = OBJ_NON_ACTION_BARRIER + OBJ_ACTION_OBJECT_TYPE,
        OBJ_TRAVELLER_TENT = OBJ_NON_ACTION_TRAVELLER_TENT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_EXPANSION_DWELLING = OBJ_NON_ACTION_EXPANSION_DWELLING + OBJ_ACTION_OBJECT_TYPE,
        OBJ_EXPANSION_OBJECT = OBJ_NON_ACTION_EXPANSION_OBJECT + OBJ_ACTION_OBJECT_TYPE,
        OBJ_JAIL = OBJ_NON_ACTION_JAIL + OBJ_ACTION_OBJECT_TYPE,
        OBJ_FIRE_ALTAR = OBJ_NON_ACTION_FIRE_ALTAR + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_AIR_ALTAR = OBJ_NON_ACTION_AIR_ALTAR + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_EARTH_ALTAR = OBJ_NON_ACTION_EARTH_ALTAR + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.
        OBJ_WATER_ALTAR = OBJ_NON_ACTION_WATER_ALTAR + OBJ_ACTION_OBJECT_TYPE, // Never set in maps.

        // IMPORTANT!!! Do not use any of unused entries for new objects. Add new entries below following the instruction.

        // NEVER use this object type in the Editor!
        // This object type is used to separate objects from the original game from Resurrection expansion's objects.
        OBJ_RESURRECTION_OBJECT_TYPE = 256,

        // This section defines all types of NON-action objects which are not present in the original game.
        // If the object by nature is an action object name it with prefix OBJ_NON_ACTION_.
        // Otherwise, name it with prefix OBJ_.
        OBJ_SWAMPY_LAKE = 257,
        OBJ_FROZEN_LAKE = 258,

        // This section defines all types of action objects which are not present in the original game.
        // If the object by nature is an action object name it with prefix OBJ_.
        // Otherwise, name it with prefix OBJ_ACTON_.
        // The value of the object must be: non-action object value + OBJ_ACTION_OBJECT_TYPE.
        OBJ_ACTION_SWAMPY_LAKE = OBJ_SWAMPY_LAKE + OBJ_ACTION_OBJECT_TYPE,
        OBJ_ACTION_FROZEN_LAKE = OBJ_FROZEN_LAKE + OBJ_ACTION_OBJECT_TYPE,
    };

    enum ObjectIcnType : uint8_t
    {
        OBJ_ICN_TYPE_UNKNOWN, // Object does not exist.
        OBJ_ICN_TYPE_UNUSED_1, // Unused
        OBJ_ICN_TYPE_UNUSED_2, // Unused
        OBJ_ICN_TYPE_UNUSED_3, // Unused
        OBJ_ICN_TYPE_UNUSED_4, // Unused
        OBJ_ICN_TYPE_UNUSED_5, // Unused
        OBJ_ICN_TYPE_BOAT32, // TODO: this is incorrect type for boats set by mistake. Fix it.
        OBJ_ICN_TYPE_UNUSED_7, // Unused
        OBJ_ICN_TYPE_UNUSED_8, // Unused
        OBJ_ICN_TYPE_UNUSED_9, // Unused
        OBJ_ICN_TYPE_OBJNHAUN, // Flying ghosts over an object (mine).
        OBJ_ICN_TYPE_OBJNARTI, // Artifacts
        OBJ_ICN_TYPE_MONS32, // MON32.icn corresponds to static monsters while we use dynamic monster animation from MINIMON.icn.
        OBJ_ICN_TYPE_UNUSED_13, // Unused
        OBJ_ICN_TYPE_FLAG32, // Flags usually used for castles.
        OBJ_ICN_TYPE_UNUSED_15, // Unused
        OBJ_ICN_TYPE_UNUSED_16, // Unused
        OBJ_ICN_TYPE_UNUSED_17, // Unused
        OBJ_ICN_TYPE_UNUSED_18, // Unused
        OBJ_ICN_TYPE_UNUSED_19, // Unused
        OBJ_ICN_TYPE_MINIMON, // Somehow it is unused but we need to use it properly.
        OBJ_ICN_TYPE_MINIHERO, // Heroes which are set in the original Editor.
        OBJ_ICN_TYPE_MTNSNOW, // Snow mountains.
        OBJ_ICN_TYPE_MTNSWMP, // Swamp mountains.
        OBJ_ICN_TYPE_MTNLAVA, // Lava mountains.
        OBJ_ICN_TYPE_MTNDSRT, // Desert mountains.
        OBJ_ICN_TYPE_MTNDIRT, // Dirt mountains.
        OBJ_ICN_TYPE_MTNMULT, // All terrain mountains.
        OBJ_ICN_TYPE_UNUSED_28, // Unused
        OBJ_ICN_TYPE_EXTRAOVR, // Extra overlay for mines.
        OBJ_ICN_TYPE_ROAD, // Roads.
        OBJ_ICN_TYPE_MTNCRCK, // Cracked desert mountains.
        OBJ_ICN_TYPE_MTNGRAS, // Grass mountains.
        OBJ_ICN_TYPE_TREJNGL, // Jungle trees.
        OBJ_ICN_TYPE_TREEVIL, // Evil trees.
        OBJ_ICN_TYPE_OBJNTOWN, // Towns and castles.
        OBJ_ICN_TYPE_OBJNTWBA, // Town basement.
        OBJ_ICN_TYPE_OBJNTWSH, // Town shadows.
        OBJ_ICN_TYPE_OBJNTWRD, // Random town.
        OBJ_ICN_TYPE_OBJNXTRA, // Elementals as guardians.
        OBJ_ICN_TYPE_OBJNWAT2, // Coastal water objects.
        OBJ_ICN_TYPE_OBJNMUL2, // Miscellaneous ground objects.
        OBJ_ICN_TYPE_TRESNOW, // Snow trees.
        OBJ_ICN_TYPE_TREFIR, // Fir-trees during Summer.
        OBJ_ICN_TYPE_TREFALL, // Fir-trees during Autumn.
        OBJ_ICN_TYPE_STREAM, // River streams.
        OBJ_ICN_TYPE_OBJNRSRC, // Resources.
        OBJ_ICN_TYPE_UNUSED_47, // Unused
        OBJ_ICN_TYPE_OBJNGRA2, // Grass objects.
        OBJ_ICN_TYPE_TREDECI, // Deciduous trees.
        OBJ_ICN_TYPE_OBJNWATR, // Water objects.
        OBJ_ICN_TYPE_OBJNGRAS, // Non-action grass objects.
        OBJ_ICN_TYPE_OBJNSNOW, // Snow objects.
        OBJ_ICN_TYPE_OBJNSWMP, // Swamp objects.
        OBJ_ICN_TYPE_OBJNLAVA, // Lava objects.
        OBJ_ICN_TYPE_OBJNDSRT, // Desert objects.
        OBJ_ICN_TYPE_OBJNDIRT, // Dirt objects.
        OBJ_ICN_TYPE_OBJNCRCK, // Crack desert objects.
        OBJ_ICN_TYPE_OBJNLAV3, // Animated lava objects.
        OBJ_ICN_TYPE_OBJNMULT, // Miscellaneous ground objects.
        OBJ_ICN_TYPE_OBJNLAV2, // Animated lava objects.
        OBJ_ICN_TYPE_X_LOC1, // Objects from The Price of Loyalty expansion.
        OBJ_ICN_TYPE_X_LOC2, // Objects from The Price of Loyalty expansion.
        OBJ_ICN_TYPE_X_LOC3 // Objects from The Price of Loyalty expansion.

        // IMPORTANT!!! If you want to add new types use UNUSED entries only.
    };

    int getIcnIdFromObjectIcnType( const ObjectIcnType objectIcnType );

    const char * StringObject( MapObjectType objectType, const int count = 1 );

    bool isHiddenForPuzzle( const int terrainType, const ObjectIcnType objectIcnType, uint8_t index );

    // The method checks whether the object is an action object depending on whether it is accessed from water or from land.
    // Use it only during actual gameplay. Event object is not considered as an action object.
    // For example, castle can't be accessed from water.
    //
    // TODO: make a separate function to determine whether the object is an action object depending on its location and not
    // TODO: on where it is accessed from.
    bool isInGameActionObject( const MapObjectType objectType, const bool accessedFromWater );

    // The method checks if the object is an action object regardless of where it is accessed from.
    // Use it only during actual gameplay. Event object is not considered as an action object.
    bool isInGameActionObject( const MapObjectType objectType );

    // The method checks if the object is an action object regardless of where it is accessed from.
    // Use it only for cases when gameplay is not active, like Editor and map initialization.
    bool isOffGameActionObject( const MapObjectType objectType );

    // The method checks if the object is an action object if it is accessed from water.
    // Use it only during actual gameplay. Event object is not considered as an action object.
    bool isWaterActionObject( const MapObjectType objectType );

    // Returns proper object type if the object is an action object. Otherwise it returns the object type itself.
    MapObjectType getBaseActionObjectType( const MapObjectType objectType );

    bool isValuableResourceObject( const MapObjectType objectType );
    bool isCaptureObject( const MapObjectType objectType );
    bool isPickupObject( const MapObjectType objectType );
    bool isArtifactObject( const MapObjectType objectType );
    // Returns true if it is impossible to refuse a fight when visiting a protected object of this type.
    bool isBattleMandatoryifObjectIsProtected( const MapObjectType objectType );
    // Returns true if this object can be safely visited by AI for fog discovery purposes.
    bool isSafeForFogDiscoveryObject( const MapObjectType objectType );

    bool isNeedStayFront( const MapObjectType objectType );

    bool isDayLife( const MapObjectType objectType );
    bool isWeekLife( const MapObjectType objectType );
    bool isMonthLife( const MapObjectType objectType );
    bool isBattleLife( const MapObjectType objectType );

    // Make sure that you pass a valid action object.
    int getActionObjectDirection( const MapObjectType objectType );

    bool getDiggingHoleSprite( const int terrainType, ObjectIcnType & objectIcnType, uint8_t & index );
    bool isDiggingHoleSprite( const int terrainType, const ObjectIcnType objectIcnType, const uint8_t index );

    // Only specific objects from the original MP2 format require extended metadata.
    // These objects store a UID in their initial metadata (quantity1 and quantity2 values).
    bool doesObjectNeedExtendedMetadata( const MP2::MapObjectType type );

    // Only specific objects from the original MP2 format contain metadata (quantity1 and quantity2 values).
    bool doesObjectContainMetadata( const MP2::MapObjectType type );
}

#endif
