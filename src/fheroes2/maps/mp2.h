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
#ifndef H2MP2_H
#define H2MP2_H

#include "gamedefs.h"

#define MP2OFFSETDATA 428
#define SIZEOFMP2TILE 20
#define SIZEOFMP2ADDON 15
#define SIZEOFMP2CASTLE 0x46
#define SIZEOFMP2HEROES 0x4c

#define SIZEOFMP2SIGN 0x0a
#define SIZEOFMP2RUMOR 0x09
#define SIZEOFMP2EVENT 0x32
#define SIZEOFMP2RIDDLE 0x8a

namespace MP2
{
    // origin mp2 tile
    struct mp2tile_t
    {
        u16 tileIndex; // tile (ocean, grass, snow, swamp, lava, desert, dirt, wasteland, beach)
        u8 objectName1; // level 1.0
        u8 indexName1; // index level 1.0 or 0xFF
        u8 quantity1; // Bitfield, first 3 bits are flags, rest is used as quantity
        u8 quantity2; // Used as a part of quantity, field size is actually 13 bits. Has most significant bits
        u8 objectName2; // level 2.0
        u8 indexName2; // index level 2.0 or 0xFF
        u8 flags; // Bitfield: 1st bit is passability, last two is tile shape: 0 none, 1 vertical, 2 horizontal, 3 any
        u8 mapObject; // zero or object
        u16 indexAddon; // zero or index addons_t
        u32 editorObjectLink; // Map editor variable: object link
        u32 editorObjectOverlay; // Map editor variable: overlay link
    };

    // origin mp2 addons tile
    struct mp2addon_t
    {
        u16 indexAddon; // zero or next addons_t
        u8 objectNameN1; // level 1.N. Last bit indicates if object is animated. Second-last controls overlay
        u8 indexNameN1; // level 1.N or 0xFF
        u8 quantityN; // Bitfield containing metadata
        u8 objectNameN2; // level 2.N
        u8 indexNameN2; // level 1.N or 0xFF
        u32 editorObjectLink; // Map editor variable: object link
        u32 editorObjectOverlay; // Map editor variable: overlay link
    };

    // origin mp2 castle
    // 0x0046 - size
    struct mp2castle_t
    {
        u8 color; // 00 blue, 01 green, 02 red, 03 yellow, 04 orange, 05 purpl, ff unknown
        bool customBuilding;
        u16 building;
        /*
        0000 0000 0000 0010	Thieve's Guild
        0000 0000 0000 0100	Tavern
        0000 0000 0000 1000	Shipyard
        0000 0000 0001 0000	Well
        0000 0000 1000 0000	Statue
        0000 0001 0000 0000	Left Turret
        0000 0010 0000 0000	Right Turret
        0000 0100 0000 0000	Marketplace
        0000 1000 0000 0000	Farm, Garbage He, Crystal Gar, Waterfall, Orchard, Skull Pile
        0001 0000 0000 0000	Moat
        0010 0000 0000 0000	Fortification, Coliseum, Rainbow, Dungeon, Library, Storm
        */
        u16 dwelling;
        /*
        0000 0000 0000 1000	dweling1
        0000 0000 0001 0000	dweling2
        0000 0000 0010 0000	dweling3
        0000 0000 0100 0000	dweling4
        0000 0000 1000 0000	dweling5
        0000 0001 0000 0000	dweling6
        0000 0010 0000 0000	upgDweling2
        0000 0100 0000 0000	upgDweling3
        0000 1000 0000 0000	upgDweling4
        0001 0000 0000 0000	upgDweling5
        0010 0000 0000 0000	upgDweling6
        */
        u8 magicTower;
        bool customTroops;
        u8 monster1;
        u8 monster2;
        u8 monster3;
        u8 monster4;
        u8 monster5;
        u16 count1;
        u16 count2;
        u16 count3;
        u16 count4;
        u16 count5;
        bool capitan;
        bool customCastleName;
        char castleName[13]; // name + '\0'
        u8 type; // 00 knight, 01 barb, 02 sorc, 03 warl, 04 wiz, 05 necr, 06 rnd
        bool castle;
        u8 allowCastle; // 00 TRUE, 01 FALSE
        u8 unknown[29];
    };

    // origin mp2 heroes
    // 0x004c - size
    struct mp2heroes_t
    {
        u8 unknown;
        bool customTroops;
        u8 monster1; // 0xff none
        u8 monster2; // 0xff none
        u8 monster3; // 0xff none
        u8 monster4; // 0xff none
        u8 monster5; // 0xff none
        u16 countMonter1;
        u16 countMonter2;
        u16 countMonter3;
        u16 countMonter4;
        u16 countMonter5;
        u8 customPortrate;
        u8 portrate;
        u8 artifact1; // 0xff none
        u8 artifact2; // 0xff none
        u8 artifact3; // 0xff none
        u8 unknown2; // 0
        u32 exerience;
        bool customSkill;
        u8 skill1; // 0xff none
        u8 skill2; // pathfinding, archery, logistic, scouting,
        u8 skill3; // diplomacy, navigation, leadership, wisdom,
        u8 skill4; // mysticism, luck, ballistics, eagle, necromance, estate
        u8 skill5;
        u8 skill6;
        u8 skill7;
        u8 skill8;
        u8 skillLevel1;
        u8 skillLevel2;
        u8 skillLevel3;
        u8 skillLevel4;
        u8 skillLevel5;
        u8 skillLevel6;
        u8 skillLevel7;
        u8 skillLevel8;
        u8 unknown3; // 0
        u8 customName;
        char name[13]; // name + '\0'
        bool patrol;
        u8 countSquare; // for patrol
        u8 unknown4[15]; // 0
    };

    // origin mp2 sign or bottle
    struct mp2info_t
    {
        u8 id; // 0x01
        u8 zero[8]; // 8 byte 0x00
        char text; // message  + '/0'
    };

    // origin mp2 event for coord
    struct mp2eventcoord_t
    {
        u8 id; // 0x01
        u32 wood;
        u32 mercury;
        u32 ore;
        u32 sulfur;
        u32 crystal;
        u32 gems;
        u32 golds;
        u16 artifact; // 0xffff - none
        bool computer; // allow events for computer
        bool cancel; // cancel event after first visit
        u8 zero[10]; // 10 byte 0x00
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
        u8 id; // 0x00
        u32 wood;
        u32 mercury;
        u32 ore;
        u32 sulfur;
        u32 crystal;
        u32 gems;
        u32 golds;
        u16 artifact; // always 0xffff - none
        u16 computer; // allow events for computer
        u16 first; // day of first occurent
        u16 subsequent; // subsequent occurrences
        u8 zero[6]; // 6 byte 0x00 and end 0x01
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
        u8 id; // 0x00
        u8 zero[7]; // 7 byte 0x00
        char text; // message + '/0'
    };

    // origin mp2 riddle sphinx
    struct mp2riddle_t
    {
        u8 id; // 0x00
        u32 wood;
        u32 mercury;
        u32 ore;
        u32 sulfur;
        u32 crystal;
        u32 gems;
        u32 golds;
        u16 artifact; // 0xffff - none
        u8 count; // count answers (1, 8)
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

    ///////////////////////////////////////////////////////////////////////////////
    // First bit indicates if you can interact with object
    enum
    {
        OBJ_ZERO = 0x00,
        OBJN_ALCHEMYLAB = 0x01,
        OBJ_UNKNW_02 = 0x02,
        OBJ_UNKNW_03 = 0x03,
        OBJN_SKELETON = 0x04,
        OBJN_DAEMONCAVE = 0x05,
        OBJ_UNKNW_06 = 0x06,
        OBJN_FAERIERING = 0x07,
        OBJ_UNKNW_08 = 0x08,
        OBJ_UNKNW_09 = 0x09,
        OBJN_GAZEBO = 0x0A,
        OBJ_UNKNW_0B = 0x0B,
        OBJN_GRAVEYARD = 0x0C,
        OBJN_ARCHERHOUSE = 0x0D,
        OBJ_UNKNW_0E = 0x0E,
        OBJN_DWARFCOTT = 0x0F,

        OBJN_PEASANTHUT = 0x10,
        OBJ_UNKNW_11 = 0x11,
        OBJ_UNKNW_12 = 0x12,
        OBJ_UNKNW_13 = 0x13,
        OBJN_DRAGONCITY = 0x14,
        OBJN_LIGHTHOUSE = 0x15,
        OBJN_WATERWHEEL = 0x16,
        OBJN_MINES = 0x17,
        OBJ_UNKNW_18 = 0x18,
        OBJN_OBELISK = 0x19,
        OBJN_OASIS = 0x1A,
        OBJ_UNKNW_1B = 0x1B,
        OBJ_COAST = 0x1C,
        OBJN_SAWMILL = 0x1D,
        OBJN_ORACLE = 0x1E,
        OBJ_UNKNW_1F = 0x1F,

        OBJN_SHIPWRECK = 0x20,
        OBJ_UNKNW_21 = 0x21,
        OBJN_DESERTTENT = 0x22,
        OBJN_CASTLE = 0x23,
        OBJN_STONELITHS = 0x24,
        OBJN_WAGONCAMP = 0x25,
        OBJ_UNKNW_26 = 0x26,
        OBJ_UNKNW_27 = 0x27,
        OBJN_WINDMILL = 0x28,
        OBJ_UNKNW_29 = 0x29,
        OBJ_UNKNW_2A = 0x2A,
        OBJ_UNKNW_2B = 0x2B,
        OBJ_UNKNW_2C = 0x2C,
        OBJ_UNKNW_2D = 0x2D,
        OBJ_UNKNW_2E = 0x2E,
        OBJ_UNKNW_2F = 0x2F,

        OBJN_RNDTOWN = 0x30,
        OBJN_RNDCASTLE = 0x31,
        OBJ_UNKNW_32 = 0x32,
        OBJ_UNKNW_33 = 0x33,
        OBJ_UNKNW_34 = 0x34,
        OBJ_UNKNW_35 = 0x35,
        OBJ_UNKNW_36 = 0x36,
        OBJ_UNKNW_37 = 0x37,
        OBJ_NOTHINGSPECIAL = 0x38,
        OBJ_NOTHINGSPECIAL2 = 0x39,
        OBJN_WATCHTOWER = 0x3A,
        OBJN_TREEHOUSE = 0x3B,
        OBJN_TREECITY = 0x3C,
        OBJN_RUINS = 0x3D,
        OBJN_FORT = 0x3E,
        OBJN_TRADINGPOST = 0x3F,

        OBJN_ABANDONEDMINE = 0x40,
        OBJ_UNKNW_41 = 0x41,
        OBJ_UNKNW_42 = 0x42,
        OBJ_UNKNW_43 = 0x43,
        OBJN_TREEKNOWLEDGE = 0x44,
        OBJN_DOCTORHUT = 0x45,
        OBJN_TEMPLE = 0x46,
        OBJN_HILLFORT = 0x47,
        OBJN_HALFLINGHOLE = 0x48,
        OBJN_MERCENARYCAMP = 0x49,
        OBJ_UNKNW_4A = 0x4A,
        OBJ_UNKNW_4B = 0x4B,
        OBJN_PYRAMID = 0x4C,
        OBJN_CITYDEAD = 0x4D,
        OBJN_EXCAVATION = 0x4E,
        OBJN_SPHINX = 0x4F,

        OBJ_UNKNW_50 = 0x50,
        OBJ_TARPIT = 0x51,
        OBJN_ARTESIANSPRING = 0x52,
        OBJN_TROLLBRIDGE = 0x53,
        OBJN_WATERINGHOLE = 0x54,
        OBJN_WITCHSHUT = 0x55,
        OBJN_XANADU = 0x56,
        OBJN_CAVE = 0x57,
        OBJ_UNKNW_58 = 0x58,
        OBJN_MAGELLANMAPS = 0x59,
        OBJ_UNKNW_5A = 0x5A,
        OBJN_DERELICTSHIP = 0x5B,
        OBJ_UNKNW_5C = 0x5C,
        OBJ_UNKNW_5D = 0x5D,
        OBJN_MAGICWELL = 0x5E,
        OBJ_UNKNW_5F = 0x5F,

        OBJN_OBSERVATIONTOWER = 0x60,
        OBJN_FREEMANFOUNDRY = 0x61,
        OBJ_UNKNW_62 = 0x62,
        OBJ_TREES = 0x63,
        OBJ_MOUNTS = 0x64,
        OBJ_VOLCANO = 0x65,
        OBJ_FLOWERS = 0x66,
        OBJ_STONES = 0x67,
        OBJ_WATERLAKE = 0x68,
        OBJ_MANDRAKE = 0x69,
        OBJ_DEADTREE = 0x6A,
        OBJ_STUMP = 0x6B,
        OBJ_CRATER = 0x6C,
        OBJ_CACTUS = 0x6D,
        OBJ_MOUND = 0x6E,
        OBJ_DUNE = 0x6F,

        OBJ_LAVAPOOL = 0x70,
        OBJ_SHRUB = 0x71,
        OBJN_ARENA = 0x72,
        OBJN_BARROWMOUNDS = 0x73,
        OBJN_MERMAID = 0x74,
        OBJN_SIRENS = 0x75,
        OBJN_HUTMAGI = 0x76,
        OBJN_EYEMAGI = 0x77,
        OBJN_TRAVELLERTENT = 0x78,
        OBJ_UNKNW_79 = 0x79,
        OBJ_UNKNW_7A = 0x7A,
        OBJN_JAIL = 0x7B,
        OBJN_FIREALTAR = 0x7C,
        OBJN_AIRALTAR = 0x7D,
        OBJN_EARTHALTAR = 0x7E,
        OBJN_WATERALTAR = 0x7F,

        OBJ_WATERCHEST = 0x80,
        OBJ_ALCHEMYLAB = 0x81,
        OBJ_SIGN = 0x82,
        OBJ_BUOY = 0x83,
        OBJ_SKELETON = 0x84,
        OBJ_DAEMONCAVE = 0x85,
        OBJ_TREASURECHEST = 0x86,
        OBJ_FAERIERING = 0x87,
        OBJ_CAMPFIRE = 0x88,
        OBJ_FOUNTAIN = 0x89,
        OBJ_GAZEBO = 0x8A,
        OBJ_ANCIENTLAMP = 0x8B,
        OBJ_GRAVEYARD = 0x8C,
        OBJ_ARCHERHOUSE = 0x8D,
        OBJ_GOBLINHUT = 0x8E,
        OBJ_DWARFCOTT = 0x8F,

        OBJ_PEASANTHUT = 0x90,
        OBJ_UNKNW_91 = 0x91,
        OBJ_UNKNW_92 = 0x92,
        OBJ_EVENT = 0x93,
        OBJ_DRAGONCITY = 0x94,
        OBJ_LIGHTHOUSE = 0x95,
        OBJ_WATERWHEEL = 0x96,
        OBJ_MINES = 0x97,
        OBJ_MONSTER = 0x98,
        OBJ_OBELISK = 0x99,
        OBJ_OASIS = 0x9A,
        OBJ_RESOURCE = 0x9B,
        OBJ_UNKNW_9C = 0x9C,
        OBJ_SAWMILL = 0x9D,
        OBJ_ORACLE = 0x9E,
        OBJ_SHRINE1 = 0x9F,

        OBJ_SHIPWRECK = 0xA0,
        OBJ_UNKNW_A1 = 0xA1,
        OBJ_DESERTTENT = 0xA2,
        OBJ_CASTLE = 0xA3,
        OBJ_STONELITHS = 0xA4,
        OBJ_WAGONCAMP = 0xA5,
        OBJ_UNKNW_A6 = 0xA6,
        OBJ_WHIRLPOOL = 0xA7,
        OBJ_WINDMILL = 0xA8,
        OBJ_ARTIFACT = 0xA9,
        OBJ_UNKNW_AA = 0xAA,
        OBJ_BOAT = 0xAB,
        OBJ_RNDULTIMATEARTIFACT = 0xAC,
        OBJ_RNDARTIFACT = 0xAD,
        OBJ_RNDRESOURCE = 0xAE,
        OBJ_RNDMONSTER = 0xAF,

        OBJ_RNDTOWN = 0xB0,
        OBJ_RNDCASTLE = 0xB1,
        OBJ_UNKNW_B2 = 0xB2,
        OBJ_RNDMONSTER1 = 0xB3,
        OBJ_RNDMONSTER2 = 0xB4,
        OBJ_RNDMONSTER3 = 0xB5,
        OBJ_RNDMONSTER4 = 0xB6,
        OBJ_HEROES = 0xB7,
        OBJ_UNKNW_B8 = 0xB8,
        OBJ_UNKNW_B9 = 0xB9,
        OBJ_WATCHTOWER = 0xBA,
        OBJ_TREEHOUSE = 0xBB,
        OBJ_TREECITY = 0xBC,
        OBJ_RUINS = 0xBD,
        OBJ_FORT = 0xBE,
        OBJ_TRADINGPOST = 0xBF,

        OBJ_ABANDONEDMINE = 0xC0,
        OBJ_THATCHEDHUT = 0xC1,
        OBJ_STANDINGSTONES = 0xC2,
        OBJ_IDOL = 0xC3,
        OBJ_TREEKNOWLEDGE = 0xC4,
        OBJ_DOCTORHUT = 0xC5,
        OBJ_TEMPLE = 0xC6,
        OBJ_HILLFORT = 0xC7,
        OBJ_HALFLINGHOLE = 0xC8,
        OBJ_MERCENARYCAMP = 0xC9,
        OBJ_SHRINE2 = 0xCA,
        OBJ_SHRINE3 = 0xCB,
        OBJ_PYRAMID = 0xCC,
        OBJ_CITYDEAD = 0xCD,
        OBJ_EXCAVATION = 0xCE,
        OBJ_SPHINX = 0xCF,

        OBJ_WAGON = 0xD0,
        OBJ_UNKNW_D1 = 0xD1,
        OBJ_ARTESIANSPRING = 0xD2,
        OBJ_TROLLBRIDGE = 0xD3,
        OBJ_WATERINGHOLE = 0xD4,
        OBJ_WITCHSHUT = 0xD5,
        OBJ_XANADU = 0xD6,
        OBJ_CAVE = 0xD7,
        OBJ_LEANTO = 0xD8,
        OBJ_MAGELLANMAPS = 0xD9,
        OBJ_FLOTSAM = 0xDA,
        OBJ_DERELICTSHIP = 0xDB,
        OBJ_SHIPWRECKSURVIROR = 0xDC,
        OBJ_BOTTLE = 0xDD,
        OBJ_MAGICWELL = 0xDE,
        OBJ_MAGICGARDEN = 0xDF,

        OBJ_OBSERVATIONTOWER = 0xE0,
        OBJ_FREEMANFOUNDRY = 0xE1,
        OBJ_UNKNW_E2 = 0xE2,
        OBJ_UNKNW_E3 = 0xE3,
        OBJ_UNKNW_E4 = 0xE4,
        OBJ_UNKNW_E5 = 0xE5,
        OBJ_UNKNW_E6 = 0xE6,
        OBJ_UNKNW_E7 = 0xE7,
        OBJ_UNKNW_E8 = 0xE8,
        OBJ_REEFS = 0xE9,
        OBJN_ALCHEMYTOWER = 0xEA,
        OBJN_STABLES = 0xEB,
        OBJ_MERMAID = 0xEC,
        OBJ_SIRENS = 0xED,
        OBJ_HUTMAGI = 0xEE,
        OBJ_EYEMAGI = 0xEF,

        OBJ_ALCHEMYTOWER = 0xF0,
        OBJ_STABLES = 0xF1,
        OBJ_ARENA = 0xF2,
        OBJ_BARROWMOUNDS = 0xF3,
        OBJ_RNDARTIFACT1 = 0xF4,
        OBJ_RNDARTIFACT2 = 0xF5,
        OBJ_RNDARTIFACT3 = 0xF6,
        OBJ_BARRIER = 0xF7,
        OBJ_TRAVELLERTENT = 0xF8,
        OBJ_UNKNW_F9 = 0xF9,
        OBJ_UNKNW_FA = 0xFA,
        OBJ_JAIL = 0xFB,
        OBJ_FIREALTAR = 0xFC,
        OBJ_AIRALTAR = 0xFD,
        OBJ_EARTHALTAR = 0xFE,
        OBJ_WATERALTAR = 0xFF
    };

    int GetICNObject( int tileset );
    const char * StringObject( int object );

    bool isHiddenForPuzzle( uint8_t tileset, uint8_t index );
    bool isActionObject( int obj, bool water );
    bool isGroundObject( int obj );
    bool isWaterObject( int obj );
    bool isQuantityObject( int obj );
    bool isCaptureObject( int obj );
    bool isPickupObject( int obj );
    bool isArtifactObject( int obj );
    bool isHeroUpgradeObject( int obj );
    bool isMonsterDwelling( int obj );
    bool isRemoveObject( int obj );
    bool isMoveObject( int obj );
    bool isAbandonedMine( int obj );
    bool isProtectedObject( int obj );

    bool isNeedStayFront( int obj );
    bool isClearGroundObject( int obj );

    bool isDayLife( int obj );
    bool isWeekLife( int obj );
    bool isMonthLife( int obj );
    bool isBattleLife( int obj );

    int GetObjectDirect( int obj );
}

#endif
