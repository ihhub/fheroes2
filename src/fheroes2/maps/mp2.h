/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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
        MP2OFFSETDATA = 428,
        SIZEOFMP2TILE = 20,
        SIZEOFMP2ADDON = 15,
        SIZEOFMP2CASTLE = 70, // Refer to Castle::LoadFromMP2() method for more information.
        SIZEOFMP2HEROES = 76,
        SIZEOFMP2SIGN = 10,
        SIZEOFMP2RUMOR = 9,
        SIZEOFMP2EVENT = 50,
        SIZEOFMP2RIDDLE = 137
    };

    // Tile structure from the original map format.
    struct mp2tile_t
    {
        // Terrain image index used for terrain tile display on Adventure Map.
        uint16_t terrainImageIndex;

        uint8_t objectName1; // Ground (bottom) level object type (first 2 bits) and object tile set (6 bits). Tile set refers to ICN ID.
        uint8_t level1IcnImageIndex; // ICN image index (image index for corresponding ICN Id) for ground (bottom) object. 255 means it's an empty object.

        // First 2 bits correspond to object layer type used to identify the order of rendering on Adventure Map.
        // The third bit is unknown. TODO: find out what the third bit is used for.
        // The last 5 bits are used together with quantity 2 as the value for the object.
        uint8_t quantity1;

        uint8_t quantity2; // Used as a part of quantity, field size is actually 13 bits. Has most significant bits
        uint8_t objectName2; // Top level object type (first 2 bits) and object tile set (6 bits). Tile set refers to ICN ID.
        uint8_t level2IcnImageIndex; // ICN image index (image index for corresponding ICN Id) for top level object. 255 means it's an empty object.

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

        uint16_t nextAddonIndex; // Next add-on index. Zero value means it's the last addon chunk.

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
    struct mp2addon_t
    {
        uint16_t nextAddonIndex; // Next add-on index. Zero value means it's the last addon chunk.

        uint8_t objectNameN1; // level 1.N. Last bit indicates if object is animated. Second-last controls overlay
        uint8_t indexNameN1; // level 1.N or 0xFF
        uint8_t quantityN; // Bitfield containing metadata
        uint8_t objectNameN2; // level 2.N
        uint8_t indexNameN2; // level 1.N or 0xFF

        // Ground (bottom) level object UID. An object can allocate more than 1 tile. Each tile could have multiple objects pieces.
        // UID is used to find all pieces/addons which belong to the same object.
        // In Editor first object will have UID as 0. Then second object placed on the map will have UID 0 + number of pieces / tiles per previous object and etc.
        uint32_t level1ObjectUID;

        // Top level object UID. An object can allocate more than 1 tile. Each tile could have multiple objects pieces.
        // UID is used to find all pieces/addons which belong to the same object.
        // In Editor first object will have UID as 0. Then second object placed on the map will have UID 0 + number of pieces / tiles per previous object and etc.
        uint32_t level2ObjectUID;
    };

    // origin mp2 heroes
    // 0x004c - size
    struct mp2heroes_t
    {
        uint8_t unknown;
        bool customTroops;
        uint8_t monster1; // 0xff none
        uint8_t monster2; // 0xff none
        uint8_t monster3; // 0xff none
        uint8_t monster4; // 0xff none
        uint8_t monster5; // 0xff none
        uint16_t countMonter1;
        uint16_t countMonter2;
        uint16_t countMonter3;
        uint16_t countMonter4;
        uint16_t countMonter5;
        uint8_t customPortrate;
        uint8_t portrate;
        uint8_t artifact1; // 0xff none
        uint8_t artifact2; // 0xff none
        uint8_t artifact3; // 0xff none
        uint8_t unknown2; // 0
        uint32_t exerience;
        bool customSkill;
        uint8_t skill1; // 0xff none
        uint8_t skill2; // pathfinding, archery, logistic, scouting,
        uint8_t skill3; // diplomacy, navigation, leadership, wisdom,
        uint8_t skill4; // mysticism, luck, ballistics, eagle, necromance, estate
        uint8_t skill5;
        uint8_t skill6;
        uint8_t skill7;
        uint8_t skill8;
        uint8_t skillLevel1;
        uint8_t skillLevel2;
        uint8_t skillLevel3;
        uint8_t skillLevel4;
        uint8_t skillLevel5;
        uint8_t skillLevel6;
        uint8_t skillLevel7;
        uint8_t skillLevel8;
        uint8_t unknown3; // 0
        uint8_t customName;
        char name[13]; // name + '\0'
        bool patrol;
        uint8_t countSquare; // for patrol
        uint8_t unknown4[15]; // 0
    };

    // origin mp2 sign or bottle
    struct mp2info_t
    {
        uint8_t id; // 0x01
        uint8_t zero[8]; // 8 byte 0x00
        char text; // message  + '/0'
    };

    // origin mp2 event for coord
    struct mp2eventcoord_t
    {
        uint8_t id; // 0x01
        uint32_t wood;
        uint32_t mercury;
        uint32_t ore;
        uint32_t sulfur;
        uint32_t crystal;
        uint32_t gems;
        uint32_t golds;
        uint16_t artifact; // 0xffff - none
        bool computer; // allow events for computer
        bool cancel; // cancel event after first visit
        uint8_t zero[10]; // 10 byte 0x00
        bool blue;
        bool green;
        bool red;
        bool yellow;
        bool orange;
        bool purple;
        char text; // message + '/0'
    };

    // origin mp2 event for day
    struct mp2eventday_t
    {
        uint8_t id; // 0x00
        uint32_t wood;
        uint32_t mercury;
        uint32_t ore;
        uint32_t sulfur;
        uint32_t crystal;
        uint32_t gems;
        uint32_t golds;
        uint16_t artifact; // always 0xffff - none
        uint16_t computer; // allow events for computer
        uint16_t first; // day of first occurent
        uint16_t subsequent; // subsequent occurrences
        uint8_t zero[6]; // 6 byte 0x00 and end 0x01
        bool blue;
        bool green;
        bool red;
        bool yellow;
        bool orange;
        bool purple;
        char text; // message + '/0'
    };

    // origin mp2 rumor
    struct mp2rumor_t
    {
        uint8_t id; // 0x00
        uint8_t zero[7]; // 7 byte 0x00
        char text; // message + '/0'
    };

    // An object type could be action and non-action. If both parts are present the difference between them must be 128.
    enum MapObjectType : uint8_t
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
        OBJ_NON_ACTION_MINES = 23,
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
        OBJ_NON_ACTION_HEROES = 55, // Never set in maps. This type is used for any types of heroes, including random.
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
        OBJ_REEFS = 98, // Never set in maps. Not in use within the original Editor.
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
        OBJ_MINES = OBJ_NON_ACTION_MINES + OBJ_ACTION_OBJECT_TYPE,
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
        OBJ_HEROES, // This type is used for any types of heroes, including random.
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
        OBJ_WATER_ALTAR = OBJ_NON_ACTION_WATER_ALTAR + OBJ_ACTION_OBJECT_TYPE // Never set in maps.

        // IMPORTANT!!! Do not use any of unused entries for new objects. Add new entries below following the instruction.

        // This section defines all types of NON-action objects which are not present in the original game.
        // If the object by nature is an action object name it with prefix OBJ_NON_ACTION_.
        // Otherwise, name it with prefix OBJ_.

        // This section defines all types of action objects which are not present in the original game.
        // If the object by nature is an action object name it with prefix OBJ_.
        // Otherwise, name it with prefix OBJ_ACTON_.
        // The value of the object must be: non-action object value + OBJ_ACTION_OBJECT_TYPE.
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

        // IMPORTANT!!! If you want to add new types use UNUSED entries first and only then add new entries in this enumeration.
    };

    int getIcnIdFromObjectIcnType( const uint8_t objectIcnType );

    const char * StringObject( MapObjectType objectType, const int count = 1 );

    bool isHiddenForPuzzle( const int terrainType, uint8_t nonCorrectedObjectIcnType, uint8_t index );

    // The method check whether the object is an action object depending on its location. For example, castle can't be located on water.
    bool isActionObject( const MapObjectType objectType, const bool locatesOnWater );

    // The method checks if the object is an action independent form its location.
    bool isActionObject( const MapObjectType objectType );

    // Returns proper object type if the object is an action object. Otherwise it returns the object type itself.
    MapObjectType getBaseActionObjectType( const MapObjectType objectType );

    bool isWaterActionObject( const MapObjectType objectType );

    bool isQuantityObject( const MapObjectType objectType );
    bool isCaptureObject( const MapObjectType objectType );
    bool isPickupObject( const MapObjectType objectType );
    bool isArtifactObject( const MapObjectType objectType );
    bool isHeroUpgradeObject( const MapObjectType objectType );
    bool isMonsterDwelling( const MapObjectType objectType );
    bool isAbandonedMine( const MapObjectType objectType );
    bool isProtectedObject( const MapObjectType objectType );
    // Returns true if this object can be safely visited by AI for fog discovery purposes.
    bool isSafeForFogDiscoveryObject( const MapObjectType objectType );

    bool isNeedStayFront( const MapObjectType objectType );

    bool isDayLife( const MapObjectType objectType );
    bool isWeekLife( const MapObjectType objectType );
    bool isMonthLife( const MapObjectType objectType );
    bool isBattleLife( const MapObjectType objectType );

    // Make sure that you pass a valid action object.
    int getActionObjectDirection( const MapObjectType objectType );

    bool getDiggingHoleSprite( const int terrainType, uint8_t & objectIcnType, uint8_t & index );
    bool isDiggingHoleSprite( const int terrainType, const uint8_t objectIcnType, const uint8_t index );
}

#endif
