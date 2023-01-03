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
        SIZEOFMP2RIDDLE = 138
    };

    // Tile structure from the original map format.
    struct mp2tile_t
    {
        uint16_t surfaceType; // Tile index representing a type of surface: ocean, grass, snow, swamp, lava, desert, dirt, wasteland, beach.

        uint8_t objectName1; // Ground (bottom) level object type (first 2 bits) and object tile set (6 bits). Tile set refers to ICN ID.
        uint8_t level1IcnImageIndex; // ICN image index (image index for corresponding ICN Id) for ground (bottom) object. 255 means it's an empty object.
        uint8_t quantity1; // Bitfield, first 3 bits are flags, rest is used as quantity
        uint8_t quantity2; // Used as a part of quantity, field size is actually 13 bits. Has most significant bits
        uint8_t objectName2; // Top level object type (first 2 bits) and object tile set (6 bits). Tile set refers to ICN ID.
        uint8_t level2IcnImageIndex; // ICN image index (image index for corresponding ICN Id) for top level object. 255 means it's an empty object.

        // First 2 bits responsible for tile shape (0 - 3). Subsequent bits are still unknown. Possible values are 1 and 5. They are set only for tiles with transition
        // between land and sea. They can be related to passabilities.
        uint8_t flags;

        uint8_t mapObjectType; // Object type. Please refer to MapObjectType enumeration.

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

    // origin mp2 riddle sphinx
    struct mp2riddle_t
    {
        uint8_t id; // 0x00
        uint32_t wood;
        uint32_t mercury;
        uint32_t ore;
        uint32_t sulfur;
        uint32_t crystal;
        uint32_t gems;
        uint32_t golds;
        uint16_t artifact; // 0xffff - none
        uint8_t count; // count answers (1, 8)
        char answer1[13];
        char answer2[13];
        char answer3[13];
        char answer4[13];
        char answer5[13];
        char answer6[13];
        char answer7[13];
        char answer8[13];
        char text; // message + '/0'
    };

    // An object type could be action and non-action. If both parts are present the difference between them must be 128.
    // TODO: right now some PoL objects do not follow this procedure leading to hacks in the code. We must fix it!
    enum MapObjectType : uint8_t
    {
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
        OBJ_NON_ACTION_UNUSED_17 = 17, // Never set in maps. Based on given information this was a monster dwelling called Log Cabin.
        OBJ_NON_ACTION_UNUSED_18 = 18, // Never set in maps. Based on given information this was Road.
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
        OBJ_NON_ACTION_UNUSED_33 = 33, // Never set in maps. Based on given information this was a Sea Chest object.
        OBJ_NON_ACTION_DESERT_TENT = 34,
        OBJ_NON_ACTION_CASTLE = 35,
        OBJ_NON_ACTION_STONE_LITHS = 36,
        OBJ_NON_ACTION_WAGON_CAMP = 37,
        OBJ_NON_ACTION_UNUSED_38 = 38, // Never set in maps. Based on given information this was a Well object.
        OBJ_NON_ACTION_WHIRLPOOL = 39, // Never set in maps.
        OBJ_NON_ACTION_WINDMILL = 40,
        OBJ_NON_ACTION_ARTIFACT = 41, // Never set in maps.
        OBJ_NON_ACTION_UNUSED_42 = 42, // Never set in maps. Based on given information this was a Hero.
        OBJ_NON_ACTION_BOAT = 43, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_ULTIMATE_ARTIFACT = 44, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_ARTIFACT = 45, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_RESOURCE = 46, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_MONSTER = 47, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_TOWN = 48,
        OBJ_NON_ACTION_RANDOM_CASTLE = 49,
        OBJ_NON_ACTION_UNUSED_50 = 50, // Never set in maps. Not in use at all.
        OBJ_NON_ACTION_RANDOM_MONSTER_WEAK = 51, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_MONSTER_MEDIUM = 52, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_MONSTER_STRONG = 53, // Never set in maps.
        OBJ_NON_ACTION_RANDOM_MONSTER_VERY_STRONG = 54, // Never set in maps.
        OBJ_NON_ACTION_HEROES = 55, // Never set in maps. TODO: this suppose to be a random hero.
        OBJ_NON_ACTION_NOTHING_SPECIAL = 56,
        OBJ_NON_ACTION_UNUSED_57 = 57, // TODO: it is used in some maps. Verify what it is.
        OBJ_NON_ACTION_WATCH_TOWER = 58,
        OBJ_NON_ACTION_TREE_HOUSE = 59,
        OBJ_NON_ACTION_TREE_CITY = 60,
        OBJ_NON_ACTION_RUINS = 61,
        OBJ_NON_ACTION_FORT = 62,
        OBJ_NON_ACTION_TRADING_POST = 63,
        OBJ_NON_ACTION_ABANDONED_MINE = 64,
        OBJ_NON_ACTION_THATCHED_HUT = 65, // Never set in maps. TODO: it is a peasant hut! It is used at least in one map. Alternative name is Dwarf Cabin.
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
        OBJ_NON_ACTION_TAR_PIT = 81,
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
        OBJ_STREAM = 98, // Never set in maps. Not in use within the original Editor.
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
        OBJ_NON_ACTION_ARENA = 114, // Never set in maps. TODO: check the correct ID for this object.
        OBJ_NON_ACTION_BARROW_MOUNDS = 115, // Never set in maps. TODO: check the correct ID for this object.
        OBJ_NON_ACTION_MERMAID = 116, // Never set in maps. TODO: check the correct ID for this object.
        OBJ_NON_ACTION_SIRENS = 117, // Never set in maps. TODO: check the correct ID for this object.
        OBJ_NON_ACTION_HUT_OF_MAGI = 118, // Never set in maps. TODO: check the correct ID for this object.
        OBJ_NON_ACTION_EYE_OF_MAGI = 119, // Never set in maps. TODO: check the correct ID for this object.
        OBJ_NON_ACTION_TRAVELLER_TENT = 120,
        OBJ_NON_ACTION_EXPANSION_DWELLING = 121,
        OBJ_NON_ACTION_EXPANSION_OBJECT = 122,
        OBJ_NON_ACTION_JAIL = 123,
        OBJ_NON_ACTION_FIRE_ALTAR = 124, // Never set in maps. TODO: check the correct ID for this object.
        OBJ_NON_ACTION_AIR_ALTAR = 125, // Never set in maps. TODO: check the correct ID for this object.
        OBJ_NON_ACTION_EARTH_ALTAR = 126, // Never set in maps. TODO: check the correct ID for this object.
        OBJ_NON_ACTION_WATER_ALTAR = 127, // Never set in maps. TODO: check the correct ID for this object.

        OBJ_WATERCHEST, // Never set in maps. TODO: this is wrong ID for this object.
        OBJ_ALCHEMIST_LAB,
        OBJ_SIGN,
        OBJ_BUOY,
        OBJ_SKELETON,
        OBJ_DAEMON_CAVE,
        OBJ_TREASURE_CHEST,
        OBJ_FAERIE_RING,
        OBJ_CAMPFIRE,
        OBJ_FOUNTAIN,
        OBJ_GAZEBO,
        OBJ_GENIE_LAMP,
        OBJ_GRAVEYARD,
        OBJ_ARCHER_HOUSE,
        OBJ_GOBLIN_HUT,
        OBJ_DWARF_COTTAGE,
        OBJ_PEASANT_HUT,
        OBJ_UNUSED_17, // Never set in maps.
        OBJ_UNUSED_18, // Never set in maps.
        OBJ_EVENT,
        OBJ_DRAGON_CITY,
        OBJ_LIGHTHOUSE,
        OBJ_WATER_WHEEL,
        OBJ_MINES,
        OBJ_MONSTER,
        OBJ_OBELISK,
        OBJ_OASIS,
        OBJ_RESOURCE,
        OBJ_ACTION_COAST, // Never set in maps as Coast is not an action object.
        OBJ_SAWMILL,
        OBJ_ORACLE,
        OBJ_SHRINE_FIRST_CIRCLE,
        OBJ_SHIPWRECK,
        OBJ_UNUSED_33, // Never set in maps.
        OBJ_DESERT_TENT,
        OBJ_CASTLE,
        OBJ_STONE_LITHS,
        OBJ_WAGON_CAMP,
        OBJ_UNUSED_38, // Never set in maps.
        OBJ_WHIRLPOOL,
        OBJ_WINDMILL,
        OBJ_ARTIFACT,
        OBJ_UNUSED_42, // Never set in maps.
        OBJ_BOAT,
        OBJ_RANDOM_ULTIMATE_ARTIFACT,
        OBJ_RANDOM_ARTIFACT,
        OBJ_RANDOM_RESOURCE,
        OBJ_RANDOM_MONSTER,
        OBJ_RANDOM_TOWN,
        OBJ_RANDOM_CASTLE,
        OBJ_UNUSED_50, // Never set in maps.
        OBJ_RANDOM_MONSTER_WEAK,
        OBJ_RANDOM_MONSTER_MEDIUM,
        OBJ_RANDOM_MONSTER_STRONG,
        OBJ_RANDOM_MONSTER_VERY_STRONG,
        OBJ_HEROES, // TODO: this suppose to be a random hero.
        OBJ_NOTHING_SPECIAL, // Never set in maps.
        OBJ_UNUSED_57, // Never set in maps.
        OBJ_WATCH_TOWER,
        OBJ_TREE_HOUSE,
        OBJ_TREE_CITY,
        OBJ_RUINS,
        OBJ_FORT,
        OBJ_TRADING_POST,
        OBJ_ABANDONED_MINE,
        OBJ_THATCHED_HUT, // TODO: it is a peasant hut! It is used at least in one map. Alternative name is Dwarf Cabin.
        OBJ_STANDING_STONES,
        OBJ_IDOL,
        OBJ_TREE_OF_KNOWLEDGE,
        OBJ_WITCH_DOCTORS_HUT,
        OBJ_TEMPLE,
        OBJ_HILL_FORT,
        OBJ_HALFLING_HOLE,
        OBJ_MERCENARY_CAMP,
        OBJ_SHRINE_SECOND_CIRCLE,
        OBJ_SHRINE_THIRD_CIRCLE,
        OBJ_PYRAMID,
        OBJ_CITY_OF_DEAD,
        OBJ_EXCAVATION,
        OBJ_SPHINX,
        OBJ_WAGON,
        OBJ_TAR_PIT, // Never set in maps. This is not an action object.
        OBJ_ARTESIAN_SPRING,
        OBJ_TROLL_BRIDGE,
        OBJ_WATERING_HOLE,
        OBJ_WITCHS_HUT,
        OBJ_XANADU,
        OBJ_CAVE,
        OBJ_LEAN_TO,
        OBJ_MAGELLANS_MAPS,
        OBJ_FLOTSAM,
        OBJ_DERELICT_SHIP,
        OBJ_SHIPWRECK_SURVIVOR,
        OBJ_BOTTLE,
        OBJ_MAGIC_WELL,
        OBJ_MAGIC_GARDEN,
        OBJ_OBSERVATION_TOWER,
        OBJ_FREEMANS_FOUNDRY,
        OBJ_ACTION_UNUSED_226, // Never set in maps.
        OBJ_ACTION_UNUSED_227, // Never set in maps.
        OBJ_ACTION_UNUSED_228, // Never set in maps.
        OBJ_ACTION_UNUSED_229, // Never set in maps.
        OBJ_ACTION_UNUSED_230, // Never set in maps.
        OBJ_ACTION_UNUSED_231, // Never set in maps.
        OBJ_ACTION_UNUSED_232, // Never set in maps.
        OBJ_REEFS, // Never set in maps. TODO: verify the correct ID for this object as it should not exist in action objects section.
        OBJ_NON_ACTION_ALCHEMIST_TOWER, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_NON_ACTION_STABLES, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_MERMAID, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_SIRENS, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_HUT_OF_MAGI, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_EYE_OF_MAGI, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_ALCHEMIST_TOWER, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_STABLES, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_ARENA, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_BARROW_MOUNDS, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_RANDOM_ARTIFACT_TREASURE,
        OBJ_RANDOM_ARTIFACT_MINOR,
        OBJ_RANDOM_ARTIFACT_MAJOR,
        OBJ_BARRIER,
        OBJ_TRAVELLER_TENT,
        OBJ_EXPANSION_DWELLING,
        OBJ_EXPANSION_OBJECT,
        OBJ_JAIL,
        OBJ_FIRE_ALTAR, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_AIR_ALTAR, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_EARTH_ALTAR, // Never set in maps. TODO: verify the correct ID for this object.
        OBJ_WATER_ALTAR // Never set in maps. TODO: verify the correct ID for this object.

        // IMPORTANT!!! Do not use any of unused entries for new objects. Add new entries just above this line.
    };

    // Return Icn ID related to this tileset value.
    int GetICNObject( const uint8_t tileset );

    const char * StringObject( const MapObjectType objectType, const int count = 1 );

    bool isHiddenForPuzzle( const int terrainType, uint8_t tileset, uint8_t index );

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

    bool getDiggingHoleSprite( const int terrainType, uint8_t & tileSet, uint8_t & index );
    bool isDiggingHoleSprite( const int terrainType, const uint8_t tileSet, const uint8_t index );
}

#endif
