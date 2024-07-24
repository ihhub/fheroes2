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

#include "maps_tiles.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <type_traits>
#include <utility>

#include "army_troop.h"
#include "castle.h"
#include "game_io.h"
#include "heroes.h"
#include "icn.h"
#include "logging.h"
#include "map_object_info.h"
#include "maps.h"
#include "maps_tiles_helper.h" // TODO: This file should not be included
#include "mp2.h"
#include "pairs.h"
#include "save_format_version.h"
#include "serialize.h"
#include "tools.h"
#include "world.h"
#include "world_object_uid.h"

namespace
{
    bool isValidShadowSprite( const int icn, const uint8_t icnIndex )
    {
        if ( icn == ICN::UNKNOWN ) {
            // Special case when no objects exist.
            return false;
        }

        switch ( icn ) {
        case ICN::MTNDSRT:
        case ICN::MTNGRAS:
        case ICN::MTNLAVA:
        case ICN::MTNMULT:
        case ICN::MTNSNOW:
        case ICN::MTNSWMP: {
            static const std::bitset<256> objMnts1ShadowBitset
                = fheroes2::makeBitsetFromVector<256>( { 0, 5, 11, 17, 21, 26, 32, 38, 42, 45, 49, 52, 55, 59, 62, 65, 68, 71, 74, 75, 79, 80 } );
            return objMnts1ShadowBitset[icnIndex];
        }
        case ICN::MTNCRCK:
        case ICN::MTNDIRT: {
            static const std::bitset<256> objMnts2ShadowBitset = fheroes2::makeBitsetFromVector<256>(
                { 0, 5, 11, 17, 21, 26, 32, 38, 42, 46, 47, 53, 57, 58, 62, 68, 72, 75, 79, 82, 85, 89, 92, 95, 98, 101, 104, 105, 109, 110 } );
            return objMnts2ShadowBitset[icnIndex];
        }
        case ICN::TREDECI:
        case ICN::TREEVIL:
        case ICN::TREFALL:
        case ICN::TREFIR:
        case ICN::TREJNGL:
        case ICN::TRESNOW: {
            static const std::bitset<256> objTreeShadowBitset = fheroes2::makeBitsetFromVector<256>( { 0, 3, 7, 10, 13, 17, 20, 23, 26, 29, 32, 34 } );
            return objTreeShadowBitset[icnIndex];
        }
        case ICN::OBJNCRCK: {
            static const std::bitset<256> objCrckShadowBitset
                = fheroes2::makeBitsetFromVector<256>( { 2, 9, 13, 15, 20, 23, 28, 33, 36, 39, 45, 48, 51, 54, 56, 73, 75, 79, 200, 201, 207, 237 } );
            return objCrckShadowBitset[icnIndex];
        }
        case ICN::OBJNDIRT: {
            static const std::bitset<256> objDirtShadowBitset = fheroes2::makeBitsetFromVector<256>(
                { 0,   1,   5,   6,   14,  47,  52,  59,  62,  65,  68,  70,  72,  75,  78,  81,  84,  87,  91,  94,  97,  100, 103, 111, 114, 117,
                  126, 128, 136, 149, 150, 158, 161, 162, 163, 164, 165, 166, 167, 168, 177, 178, 179, 180, 181, 182, 183, 184, 193, 196, 200 } );
            return objDirtShadowBitset[icnIndex];
        }
        case ICN::OBJNDSRT: {
            static const std::bitset<256> objDsrtShadowBitset = fheroes2::makeBitsetFromVector<256>(
                { 11, 13, 16, 19, 23, 25, 27, 29, 33, 35, 38, 41, 44, 46, 47, 50, 52, 54, 55, 56, 57, 58, 59, 60, 71, 75, 77, 80, 86, 103, 115, 118 } );
            return objDsrtShadowBitset[icnIndex];
        }
        case ICN::OBJNGRA2: {
            static const std::bitset<256> objGra2ShadowBitset = fheroes2::makeBitsetFromVector<256>(
                { 5,  14, 19, 20, 28, 31, 32, 33, 34, 35,  36,  37,  38,  47,  48,  49,  50,  51,  52,  53,  54,  70,  71,  72,  73,  74, 75,
                  76, 77, 78, 79, 80, 81, 82, 83, 91, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 121, 124, 128 } );
            return objGra2ShadowBitset[icnIndex];
        }
        case ICN::OBJNGRAS: {
            static const std::bitset<256> objGrasShadowBitset = fheroes2::makeBitsetFromVector<256>(
                { 0, 4, 29, 32, 36, 39, 42, 44, 46, 48, 50, 76, 79, 82, 88, 92, 94, 98, 102, 105, 108, 111, 113, 120, 124, 128, 134, 138, 141, 143, 145, 147 } );
            return objGrasShadowBitset[icnIndex];
        }
        case ICN::OBJNMUL2: {
            static const std::bitset<256> objMul2ShadowBitset = fheroes2::makeBitsetFromVector<256>(
                { 14,  17,  20,  24,  34,  36,  42,  43,  49,  50,  60,  71,  72,  113, 115, 118, 121, 123, 127, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146,
                  147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 164, 180, 181, 182, 183, 184, 185, 186, 189, 199, 200, 202, 206 } );
            return objMul2ShadowBitset[icnIndex];
        }
        case ICN::OBJNMULT: {
            static const std::bitset<256> objMultShadowBitset = fheroes2::makeBitsetFromVector<256>(
                { 1,  3,  15, 16, 17, 18, 19, 20, 21, 22, 23, 24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54, 57,
                  61, 67, 68, 75, 77, 79, 81, 83, 97, 98, 99, 100, 101, 102, 103, 105, 106, 107, 108, 109, 110, 113, 115, 121, 122, 124, 125, 126, 127, 128, 129, 130 } );
            return objMultShadowBitset[icnIndex];
        }
        case ICN::OBJNSNOW: {
            static const std::bitset<256> objSnowShadowBitset
                = fheroes2::makeBitsetFromVector<256>( { 21,  25,  29,  31,  33,  36,  40,  48,  54,  59,  63,  67,  70,  73,  76,  79,  101, 104, 105, 106, 107,
                                                         108, 109, 110, 111, 120, 121, 122, 123, 124, 125, 126, 127, 137, 140, 142, 144, 148, 193, 203, 207 } );
            return objSnowShadowBitset[icnIndex];
        }
        case ICN::OBJNSWMP: {
            static const std::bitset<256> objSwmpShadowBitset
                = fheroes2::makeBitsetFromVector<256>( { 2,  3,   14,  15,  16,  17,  18,  19,  20,  21,  31,  43,  44,  45,  46,  47,  48,  49, 66,
                                                         83, 125, 127, 130, 132, 136, 141, 163, 170, 175, 178, 195, 197, 202, 204, 207, 211, 215 } );
            return objSwmpShadowBitset[icnIndex];
        }
        case ICN::OBJNWAT2: {
            return icnIndex == 1;
        }
        case ICN::OBJNWATR: {
            static const std::bitset<256> objWatrShadowBitset = fheroes2::makeBitsetFromVector<256>(
                { 12,  13,  14,  15,  16,  17,  18,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,
                  42,  43,  44,  52,  55,  118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 166, 167,
                  168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 184, 188, 189, 190, 191, 192, 193, 194, 240 } );
            return objWatrShadowBitset[icnIndex];
        }
        case ICN::OBJNARTI:
        case ICN::OBJNRSRC:
            return 0 == ( icnIndex % 2 );
        case ICN::OBJNTWRD:
            return icnIndex > 31;
        case ICN::X_LOC1: {
            static const std::bitset<256> objXlc1ShadowBitset = fheroes2::makeBitsetFromVector<256>(
                { 1, 2, 32, 33, 34, 35, 36, 37, 38, 39, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 72, 78, 79, 83, 84, 112, 116, 120, 124, 125, 129, 133 } );
            return objXlc1ShadowBitset[icnIndex];
        }
        case ICN::X_LOC2: {
            static const std::bitset<256> objXlc2ShadowBitset = fheroes2::makeBitsetFromVector<256>(
                { 2, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 47, 48, 49, 50, 51, 52, 53, 54, 55, 83, 84, 85, 86, 87, 88, 89, 90, 91 } );
            return objXlc2ShadowBitset[icnIndex];
        }
        case ICN::X_LOC3: {
            static const std::bitset<256> objXlc3ShadowBitset = fheroes2::makeBitsetFromVector<256>(
                { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,   20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  41,  42,  43,  44,  45,  46, 47,
                  48, 49, 59, 65, 71, 77, 83, 89, 95, 101, 108, 109, 112, 113, 116, 117, 120, 121, 124, 125, 128, 129, 132, 133, 136, 137 } );
            return objXlc3ShadowBitset[icnIndex];
        }
        case ICN::OBJNTOWN: {
            static const std::bitset<256> obTownShadowBitset = fheroes2::makeBitsetFromVector<256>( { 0, 16, 17, 48, 80, 81, 112, 144, 145, 161, 165, 176 } );
            return obTownShadowBitset[icnIndex];
        }
        case ICN::OBJNLAVA: {
            static const std::bitset<256> objLavaShadowBitset = fheroes2::makeBitsetFromVector<256>( { 10, 11, 45, 49, 79, 80, 81, 82, 109, 113, 116 } );
            return objLavaShadowBitset[icnIndex];
        }
        case ICN::OBJNLAV2: {
            static const std::bitset<256> objLav2ShadowBitset
                = fheroes2::makeBitsetFromVector<256>( { 7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 29, 34, 38, 39, 43, 44, 45, 46,
                                                         47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 72, 77, 78 } );
            return objLav2ShadowBitset[icnIndex];
        }
        case ICN::OBJNLAV3: {
            static const std::bitset<256> objLav3ShadowBitset = fheroes2::makeBitsetFromVector<256>(
                { 1,   2,   3,   4,   16,  17,  18,  19,  31,  32,  33,  34,  38,  46,  47,  48,  49,  50,  57,  58,  59,  61,  62,  63,  64,  76,  77,
                  91,  92,  93,  106, 107, 108, 109, 110, 111, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
                  134, 136, 137, 138, 139, 142, 143, 144, 145, 146, 147, 148, 149, 166, 167, 168, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186,
                  187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
                  214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 243 } );
            return objLav3ShadowBitset[icnIndex];
        }
        case ICN::OBJNTWSH:
            return true;
        case ICN::STREAM:
        case ICN::OBJNTWBA:
        case ICN::OBJNXTRA:
        case ICN::ROAD:
        case ICN::EXTRAOVR:
        case ICN::MONS32:
        case ICN::BOAT32:
        case ICN::FLAG32:
        case ICN::MINIHERO:
            return false;
        default:
            // Did you add a new ICN group of objects into the game?
            assert( 0 );
            break;
        }

        return false;
    }

    bool isReefs( const uint8_t index )
    {
        return index >= 111 && index <= 135;
    }

    bool isValidReefsSprite( const MP2::ObjectIcnType objectIcnType, const uint8_t icnIndex )
    {
        return objectIcnType == MP2::OBJ_ICN_TYPE_X_LOC2 && isReefs( icnIndex );
    }

    bool isShortObject( const MP2::MapObjectType objectType )
    {
        // Some objects allow middle moves even being attached to the bottom.
        // These object actually don't have any sprites on tiles above them within addon 2 level objects.
        // TODO: find a better way to do not hardcode values here.

        switch ( objectType ) {
        case MP2::OBJ_HALFLING_HOLE:
        case MP2::OBJ_NON_ACTION_HALFLING_HOLE:
        case MP2::OBJ_LEAN_TO:
        case MP2::OBJ_WATER_LAKE:
        case MP2::OBJ_TAR_PIT:
        case MP2::OBJ_MERCENARY_CAMP:
        case MP2::OBJ_NON_ACTION_MERCENARY_CAMP:
        case MP2::OBJ_STANDING_STONES:
        case MP2::OBJ_SHRINE_FIRST_CIRCLE:
        case MP2::OBJ_SHRINE_SECOND_CIRCLE:
        case MP2::OBJ_SHRINE_THIRD_CIRCLE:
        case MP2::OBJ_MAGIC_GARDEN:
        case MP2::OBJ_RUINS:
        case MP2::OBJ_NON_ACTION_RUINS:
        case MP2::OBJ_SIGN:
        case MP2::OBJ_IDOL:
        case MP2::OBJ_STONE_LITHS:
        case MP2::OBJ_NON_ACTION_STONE_LITHS:
        case MP2::OBJ_WAGON:
        case MP2::OBJ_WAGON_CAMP:
        case MP2::OBJ_NON_ACTION_WAGON_CAMP:
        case MP2::OBJ_GOBLIN_HUT:
        case MP2::OBJ_FAERIE_RING:
        case MP2::OBJ_NON_ACTION_FAERIE_RING:
        case MP2::OBJ_BARRIER:
        case MP2::OBJ_MAGIC_WELL:
        case MP2::OBJ_NOTHING_SPECIAL:
            return true;
        default:
            break;
        }

        return false;
    }

    bool isDetachedObjectType( const MP2::MapObjectType objectType )
    {
        // Some objects do not take into account other objects below them.
        switch ( objectType ) {
        case MP2::OBJ_CASTLE:
        case MP2::OBJ_WAGON_CAMP:
        case MP2::OBJ_FAERIE_RING:
        case MP2::OBJ_MINE:
        case MP2::OBJ_SAWMILL:
        case MP2::OBJ_WATER_ALTAR:
        case MP2::OBJ_AIR_ALTAR:
        case MP2::OBJ_FIRE_ALTAR:
        case MP2::OBJ_EARTH_ALTAR:
            return true;
        default:
            break;
        }

        return false;
    }

    bool isCombinedObject( const MP2::MapObjectType objectType )
    {
        // Trees allow bottom and top movements but they don't allow the same for other trees.
        switch ( objectType ) {
        case MP2::OBJ_TREES:
        case MP2::OBJ_CRATER:
            return true;
        default:
            break;
        }

        return false;
    }

    const char * getObjectLayerName( const uint8_t level )
    {
        switch ( level ) {
        case Maps::OBJECT_LAYER:
            return "Object layer";
        case Maps::BACKGROUND_LAYER:
            return "Background layer";
        case Maps::SHADOW_LAYER:
            return "Shadow layer";
        case Maps::TERRAIN_LAYER:
            return "Terrain layer";
        default:
            assert( 0 );
            break;
        }

        return "Unknown layer";
    }

    MP2::MapObjectType getLoyaltyObject( const MP2::ObjectIcnType objectIcnType, const uint8_t icnIndex )
    {
        switch ( objectIcnType ) {
        case MP2::OBJ_ICN_TYPE_X_LOC1:
            if ( icnIndex == 3 )
                return MP2::OBJ_ALCHEMIST_TOWER;
            else if ( icnIndex < 3 )
                return MP2::OBJ_NON_ACTION_ALCHEMIST_TOWER;
            else if ( 70 == icnIndex )
                return MP2::OBJ_ARENA;
            else if ( 3 < icnIndex && icnIndex < 72 )
                return MP2::OBJ_NON_ACTION_ARENA;
            else if ( 77 == icnIndex )
                return MP2::OBJ_BARROW_MOUNDS;
            else if ( 71 < icnIndex && icnIndex < 78 )
                return MP2::OBJ_NON_ACTION_BARROW_MOUNDS;
            else if ( 94 == icnIndex )
                return MP2::OBJ_EARTH_ALTAR;
            else if ( 77 < icnIndex && icnIndex < 112 )
                return MP2::OBJ_NON_ACTION_EARTH_ALTAR;
            else if ( 118 == icnIndex )
                return MP2::OBJ_AIR_ALTAR;
            else if ( 111 < icnIndex && icnIndex < 120 )
                return MP2::OBJ_NON_ACTION_AIR_ALTAR;
            else if ( 127 == icnIndex )
                return MP2::OBJ_FIRE_ALTAR;
            else if ( 119 < icnIndex && icnIndex < 129 )
                return MP2::OBJ_NON_ACTION_FIRE_ALTAR;
            else if ( 135 == icnIndex )
                return MP2::OBJ_WATER_ALTAR;
            else if ( 128 < icnIndex && icnIndex < 137 )
                return MP2::OBJ_NON_ACTION_WATER_ALTAR;
            break;

        case MP2::OBJ_ICN_TYPE_X_LOC2:
            if ( icnIndex == 4 )
                return MP2::OBJ_STABLES;
            else if ( icnIndex < 4 )
                return MP2::OBJ_NON_ACTION_STABLES;
            else if ( icnIndex == 9 )
                return MP2::OBJ_JAIL;
            else if ( 4 < icnIndex && icnIndex < 10 )
                return MP2::OBJ_NON_ACTION_JAIL;
            else if ( icnIndex == 37 )
                return MP2::OBJ_MERMAID;
            else if ( 9 < icnIndex && icnIndex < 47 )
                return MP2::OBJ_NON_ACTION_MERMAID;
            else if ( icnIndex == 101 )
                return MP2::OBJ_SIRENS;
            else if ( 46 < icnIndex && icnIndex < 111 )
                return MP2::OBJ_NON_ACTION_SIRENS;
            else if ( isReefs( icnIndex ) )
                return MP2::OBJ_REEFS;
            break;

        case MP2::OBJ_ICN_TYPE_X_LOC3:
            if ( icnIndex == 30 )
                return MP2::OBJ_HUT_OF_MAGI;
            else if ( icnIndex < 32 )
                return MP2::OBJ_NON_ACTION_HUT_OF_MAGI;
            else if ( icnIndex == 50 )
                return MP2::OBJ_EYE_OF_MAGI;
            else if ( 31 < icnIndex && icnIndex < 59 )
                return MP2::OBJ_NON_ACTION_EYE_OF_MAGI;
            break;

        default:
            break;
        }

        return MP2::OBJ_NONE;
    }

    bool isSpriteRoad( const MP2::ObjectIcnType objectIcnType, const uint8_t imageIndex )
    {
        switch ( objectIcnType ) {
        case MP2::OBJ_ICN_TYPE_ROAD: {
            static const std::set<uint8_t> allowedIndecies{ 0, 2, 3, 4, 5, 6, 7, 9, 12, 13, 14, 16, 17, 18, 19, 20, 21, 26, 28, 29, 30, 31 };
            return ( allowedIndecies.count( imageIndex ) == 1 );
        }
        case MP2::OBJ_ICN_TYPE_OBJNTOWN: {
            static const std::set<uint8_t> allowedIndecies{ 13, 29, 45, 61, 77, 93, 109, 125, 141, 157, 173, 189 };
            return ( allowedIndecies.count( imageIndex ) == 1 );
        }
        case MP2::OBJ_ICN_TYPE_OBJNTWRD: {
            static const std::set<uint8_t> allowedIndecies{ 13, 29 };
            return ( allowedIndecies.count( imageIndex ) == 1 );
        }
        default:
            break;
        }

        return false;
    }

    bool isAddonShadow( const Maps::TilesAddon & ta )
    {
        return isValidShadowSprite( MP2::getIcnIdFromObjectIcnType( ta._objectIcnType ), ta._imageIndex );
    }

    void getAddonInfo( const Maps::TilesAddon & addon, std::ostringstream & os )
    {
        os << "UID             : " << addon._uid << std::endl
           << "ICN object type : " << static_cast<int>( addon._objectIcnType ) << " (" << ICN::GetString( MP2::getIcnIdFromObjectIcnType( addon._objectIcnType ) ) << ")"
           << std::endl
           << "image index     : " << static_cast<int>( addon._imageIndex ) << std::endl
           << "layer type      : " << static_cast<int>( addon._layerType ) << " - " << getObjectLayerName( addon._layerType ) << std::endl
           << "is shadow       : " << ( isAddonShadow( addon ) ? "yes" : "no" ) << std::endl;
    }

    std::string getAddonInfo( const Maps::TilesAddon & addon, const int lvl )
    {
        std::ostringstream os;
        os << "--------- Level " << lvl << " --------" << std::endl;
        getAddonInfo( addon, os );
        return os.str();
    }
}

void Maps::Tiles::Init( int32_t index, const MP2::MP2TileInfo & mp2 )
{
    _tilePassabilityDirections = DIRECTION_ALL;

    _metadata[0] = ( ( ( mp2.quantity2 << 8 ) + mp2.quantity1 ) >> 3 );
    _fogColors = Color::ALL;
    _terrainImageIndex = mp2.terrainImageIndex;
    _terrainFlags = mp2.terrainFlags;
    _boatOwnerColor = Color::NONE;
    _index = index;

    SetObject( static_cast<MP2::MapObjectType>( mp2.mapObjectType ) );

    if ( !MP2::doesObjectContainMetadata( _mainObjectType ) && ( _metadata[0] != 0 ) ) {
        // No metadata should exist for non-action objects.
        // Some maps have invalid format. Even if this metadata is set here, it will later be reset during world map loading.
        DEBUG_LOG( DBG_GAME, DBG_WARN,
                   "Metadata present for non action object " << MP2::StringObject( _mainObjectType ) << " at tile " << _index << ". Metadata value " << _metadata[0] )
    }

    _addonBottomLayer.clear();
    _addonTopLayer.clear();

    const MP2::ObjectIcnType bottomObjectIcnType = static_cast<MP2::ObjectIcnType>( mp2.objectName1 >> 2 );

    const uint8_t layerType = ( mp2.quantity1 & 0x03 );

    // In the original Editor the road bit is set even if no road exist.
    // It is important to verify the existence of a road without relying on this bit.
    if ( isSpriteRoad( bottomObjectIcnType, mp2.bottomIcnImageIndex ) ) {
        _isTileMarkedAsRoad = true;
    }

    if ( mp2.mapObjectType == MP2::OBJ_NONE && ( layerType == Maps::ObjectLayerType::SHADOW_LAYER || layerType == Maps::ObjectLayerType::TERRAIN_LAYER ) ) {
        // If an object sits on shadow or terrain layer then we should put it as a bottom layer add-on.
        if ( bottomObjectIcnType != MP2::ObjectIcnType::OBJ_ICN_TYPE_UNKNOWN ) {
            _addonBottomLayer.emplace_back( layerType, mp2.level1ObjectUID, bottomObjectIcnType, mp2.bottomIcnImageIndex );
        }
    }
    else {
        _mainAddon._layerType = layerType;
        _mainAddon._uid = mp2.level1ObjectUID;
        _mainAddon._objectIcnType = bottomObjectIcnType;
        _mainAddon._imageIndex = mp2.bottomIcnImageIndex;
    }

    const MP2::ObjectIcnType topObjectIcnType = static_cast<MP2::ObjectIcnType>( mp2.objectName2 >> 2 );
    if ( topObjectIcnType != MP2::ObjectIcnType::OBJ_ICN_TYPE_UNKNOWN ) {
        // Top layer objects do not have any internal structure (layers) so all of them should have the same internal layer.
        // TODO: remove layer type for top layer objects.
        _addonTopLayer.emplace_back( OBJECT_LAYER, mp2.level2ObjectUID, topObjectIcnType, mp2.topIcnImageIndex );
    }
}

void Maps::Tiles::setTerrain( const uint16_t terrainImageIndex, const bool horizontalFlip, const bool verticalFlip )
{
    _terrainFlags = ( verticalFlip ? 1 : 0 ) + ( horizontalFlip ? 2 : 0 );

    const int newGround = Ground::getGroundByImageIndex( terrainImageIndex );

    // TODO: Remove all objects from map when changing water to land and vice-versa in both 'Map_Format' and 'tiles'.

    if ( ( _isTileMarkedAsRoad || isStream() ) && ( newGround != Ground::WATER ) && Maps::Ground::doesTerrainImageIndexContainEmbeddedObjects( terrainImageIndex ) ) {
        // There cannot be extra objects under the roads and streams.
        _terrainImageIndex = Ground::getRandomTerrainImageIndex( Ground::getGroundByImageIndex( terrainImageIndex ), false );

        return;
    }

    _terrainImageIndex = terrainImageIndex;
}

Heroes * Maps::Tiles::getHero() const
{
    return MP2::OBJ_HERO == _mainObjectType && Heroes::isValidId( _occupantHeroId ) ? world.GetHeroes( _occupantHeroId ) : nullptr;
}

void Maps::Tiles::setHero( Heroes * hero )
{
    if ( hero ) {
        using HeroIDType = decltype( _occupantHeroId );
        static_assert( std::is_same_v<HeroIDType, uint8_t>, "Type of heroID has been changed, check the logic below" );

        hero->setObjectTypeUnderHero( _mainObjectType );

        assert( hero->GetID() >= std::numeric_limits<HeroIDType>::min() && hero->GetID() < std::numeric_limits<HeroIDType>::max() );
        _occupantHeroId = static_cast<HeroIDType>( hero->GetID() );

        SetObject( MP2::OBJ_HERO );
    }
    else {
        hero = getHero();

        if ( hero ) {
            SetObject( hero->getObjectTypeUnderHero() );
            hero->setObjectTypeUnderHero( MP2::OBJ_NONE );
        }
        else {
            updateObjectType();
        }

        _occupantHeroId = Heroes::UNKNOWN;
    }
}

fheroes2::Point Maps::Tiles::GetCenter() const
{
    return Maps::GetPoint( _index );
}

MP2::MapObjectType Maps::Tiles::GetObject( bool ignoreObjectUnderHero /* true */ ) const
{
    if ( !ignoreObjectUnderHero && MP2::OBJ_HERO == _mainObjectType ) {
        const Heroes * hero = getHero();
        return hero ? hero->getObjectTypeUnderHero() : MP2::OBJ_NONE;
    }

    return _mainObjectType;
}

void Maps::Tiles::SetObject( const MP2::MapObjectType objectType )
{
    _mainObjectType = objectType;

    world.resetPathfinder();
}

void Maps::Tiles::setBoat( const int direction, const int color )
{
    if ( _mainAddon._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN ) {
        // It is important to preserve the order of objects for rendering purposes. Therefore, the main object should go to the front of objects.
        _addonBottomLayer.emplace_front( _mainAddon );
    }

    // If this assertion blows up then you are trying to put a boat on land!
    assert( isWater() );

    SetObject( MP2::OBJ_BOAT );
    _mainAddon._objectIcnType = MP2::OBJ_ICN_TYPE_BOAT32;

    switch ( direction ) {
    case Direction::TOP:
        _mainAddon._imageIndex = 0;
        break;
    case Direction::TOP_RIGHT:
        _mainAddon._imageIndex = 9;
        break;
    case Direction::RIGHT:
        _mainAddon._imageIndex = 18;
        break;
    case Direction::BOTTOM_RIGHT:
        _mainAddon._imageIndex = 27;
        break;
    case Direction::BOTTOM:
        _mainAddon._imageIndex = 36;
        break;
    // Left-side sprites have to be flipped, add 128 to index.
    case Direction::BOTTOM_LEFT:
        _mainAddon._imageIndex = 27 + 128;
        break;
    case Direction::LEFT:
        _mainAddon._imageIndex = 18 + 128;
        break;
    case Direction::TOP_LEFT:
        _mainAddon._imageIndex = 9 + 128;
        break;
    default:
        _mainAddon._imageIndex = 18;
        break;
    }

    _mainAddon._uid = getNewObjectUID();

    using BoatOwnerColorType = decltype( _boatOwnerColor );
    static_assert( std::is_same_v<BoatOwnerColorType, uint8_t>, "Type of _boatOwnerColor has been changed, check the logic below" );

    assert( color >= std::numeric_limits<BoatOwnerColorType>::min() && color <= std::numeric_limits<BoatOwnerColorType>::max() );

    _boatOwnerColor = static_cast<BoatOwnerColorType>( color );
}

int Maps::Tiles::getBoatDirection() const
{
    // Check if it really is a boat
    if ( _mainAddon._objectIcnType != MP2::OBJ_ICN_TYPE_BOAT32 )
        return Direction::UNKNOWN;

    // Left-side sprites have to flipped, add 128 to index
    switch ( _mainAddon._imageIndex ) {
    case 0:
        return Direction::TOP;
    case 9:
        return Direction::TOP_RIGHT;
    case 18:
        return Direction::RIGHT;
    case 27:
        return Direction::BOTTOM_RIGHT;
    case 36:
        return Direction::BOTTOM;
    case 27 + 128:
        return Direction::BOTTOM_LEFT;
    case 18 + 128:
        return Direction::LEFT;
    case 9 + 128:
        return Direction::TOP_LEFT;
    default:
        break;
    }

    return Direction::UNKNOWN;
}

int Maps::Tiles::getOriginalPassability() const
{
    const MP2::MapObjectType objectType = GetObject( false );

    if ( MP2::isOffGameActionObject( objectType ) ) {
        return MP2::getActionObjectDirection( objectType );
    }

    if ( _mainAddon._objectIcnType == MP2::OBJ_ICN_TYPE_UNKNOWN || _mainAddon.isPassabilityTransparent() || isShadow() ) {
        // No object exists. Make it fully passable.
        return DIRECTION_ALL;
    }

    if ( isValidReefsSprite( _mainAddon._objectIcnType, _mainAddon._imageIndex ) ) {
        return 0;
    }

    for ( const TilesAddon & addon : _addonBottomLayer ) {
        if ( isValidReefsSprite( addon._objectIcnType, addon._imageIndex ) ) {
            return 0;
        }
    }

    // Objects have fixed passability.
    return DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW;
}

void Maps::Tiles::setInitialPassability()
{
    using TilePassableType = decltype( _tilePassabilityDirections );
    static_assert( std::is_same_v<TilePassableType, uint16_t>, "Type of tilePassable has been changed, check the logic below" );

    const int passability = getOriginalPassability();
    assert( passability >= std::numeric_limits<TilePassableType>::min() && passability <= std::numeric_limits<TilePassableType>::max() );

    _tilePassabilityDirections = static_cast<TilePassableType>( passability );
}

void Maps::Tiles::updatePassability()
{
    // Get object type but ignore heroes as they are "temporary" objects.
    const MP2::MapObjectType objectType = GetObject( false );

    if ( !MP2::isOffGameActionObject( objectType ) && ( _mainAddon._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN ) && !_mainAddon.isPassabilityTransparent()
         && !isShadow() ) {
        // This is a non-action object.

        if ( !Maps::isValidDirection( _index, Direction::BOTTOM ) ) {
            // This object "touches" the bottom part of the map. Mark is as inaccessible.
            _tilePassabilityDirections = 0;
            return;
        }

        const Tiles & bottomTile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::BOTTOM ) );
        // If an object locates on land and the bottom tile is water mark the current tile as impassable. It's done for cases that a hero won't be able to
        // disembark on the tile.
        if ( !isWater() && bottomTile.isWater() ) {
            _tilePassabilityDirections = 0;
            return;
        }

        // If a bottom tile has the same object ID then this tile must be marked as inaccessible because it is a continuation of the same object.
        std::vector<uint32_t> tileUIDs;

        // If this assertion blows up then the object is not set properly. An object must have a valid UID!
        assert( _mainAddon._uid != 0 );
        tileUIDs.emplace_back( _mainAddon._uid );

        for ( const TilesAddon & addon : _addonBottomLayer ) {
            if ( !addon.isPassabilityTransparent() ) {
                // If this assertion blows up then the object is not set properly. An object must have a valid UID!
                assert( addon._uid != 0 );
                tileUIDs.emplace_back( addon._uid );
            }
        }

        for ( const uint32_t objectId : tileUIDs ) {
            if ( bottomTile.doesObjectExist( objectId ) ) {
                _tilePassabilityDirections = 0;
                return;
            }
        }

        // Count how many objects are there excluding shadows, roads and river streams.
        const std::ptrdiff_t validBottomLayerObjects = std::count_if( _addonBottomLayer.begin(), _addonBottomLayer.end(), []( const TilesAddon & addon ) {
            if ( isAddonShadow( addon ) ) {
                return false;
            }

            return addon._objectIcnType != MP2::OBJ_ICN_TYPE_ROAD && addon._objectIcnType != MP2::OBJ_ICN_TYPE_STREAM;
        } );

        const bool singleObjectTile = ( validBottomLayerObjects == 0 ) && _addonTopLayer.empty() && ( bottomTile._mainAddon._objectIcnType != _mainAddon._objectIcnType );

        // TODO: we might need to simplify the logic below as singleObjectTile might cover most of it.
        if ( !singleObjectTile && !isDetachedObject() && !bottomTile._mainAddon.isPassabilityTransparent()
             && ( bottomTile._mainAddon._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN ) ) {
            const MP2::MapObjectType bottomTileObjectType = bottomTile.GetObject( false );
            const MP2::MapObjectType correctedObjectType = MP2::getBaseActionObjectType( bottomTileObjectType );

            if ( MP2::isOffGameActionObject( bottomTileObjectType ) ) {
                if ( ( MP2::getActionObjectDirection( bottomTileObjectType ) & Direction::TOP ) == 0 ) {
                    if ( isShortObject( bottomTileObjectType ) ) {
                        _tilePassabilityDirections &= ~Direction::BOTTOM;
                    }
                    else {
                        _tilePassabilityDirections = 0;
                        return;
                    }
                }
            }
            else if ( bottomTile._mainObjectType != MP2::OBJ_NONE && correctedObjectType != bottomTileObjectType && MP2::isOffGameActionObject( correctedObjectType )
                      && isShortObject( correctedObjectType ) && ( bottomTile.getOriginalPassability() & Direction::TOP ) == 0 ) {
                _tilePassabilityDirections &= ~Direction::BOTTOM;
            }
            else if ( isShortObject( bottomTileObjectType )
                      || ( !bottomTile.containsAnyObjectIcnType( getValidObjectIcnTypes() )
                           && ( isCombinedObject( objectType ) || isCombinedObject( bottomTileObjectType ) ) ) ) {
                _tilePassabilityDirections &= ~Direction::BOTTOM;
            }
            else {
                _tilePassabilityDirections = 0;
                return;
            }
        }
    }

    // Left side.
    if ( ( _tilePassabilityDirections & Direction::TOP_LEFT ) && Maps::isValidDirection( _index, Direction::LEFT ) ) {
        const Tiles & leftTile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) );
        const bool leftTileTallObject = leftTile.isTallObject();
        if ( leftTileTallObject && ( leftTile.getOriginalPassability() & Direction::TOP ) == 0 ) {
            _tilePassabilityDirections &= ~Direction::TOP_LEFT;
        }
    }

    // Right side.
    if ( ( _tilePassabilityDirections & Direction::TOP_RIGHT ) && Maps::isValidDirection( _index, Direction::RIGHT ) ) {
        const Tiles & rightTile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::RIGHT ) );
        const bool rightTileTallObject = rightTile.isTallObject();
        if ( rightTileTallObject && ( rightTile.getOriginalPassability() & Direction::TOP ) == 0 ) {
            _tilePassabilityDirections &= ~Direction::TOP_RIGHT;
        }
    }
}

bool Maps::Tiles::doesObjectExist( const uint32_t uid ) const
{
    if ( _mainAddon._uid == uid && !_mainAddon.isPassabilityTransparent() ) {
        return true;
    }

    return std::any_of( _addonBottomLayer.cbegin(), _addonBottomLayer.cend(),
                        [uid]( const TilesAddon & addon ) { return addon._uid == uid && !addon.isPassabilityTransparent(); } );
}

void Maps::Tiles::UpdateRegion( uint32_t newRegionID )
{
    if ( _tilePassabilityDirections ) {
        _region = newRegionID;
    }
    else {
        _region = REGION_NODE_BLOCKED;
    }
}

void Maps::Tiles::pushBottomLayerAddon( const MP2::MP2AddonInfo & ma )
{
    const MP2::ObjectIcnType objectIcnType = static_cast<MP2::ObjectIcnType>( ma.objectNameN1 >> 2 );
    if ( objectIcnType == MP2::ObjectIcnType::OBJ_ICN_TYPE_UNKNOWN ) {
        // No object exist.
        return;
    }

    // In the original Editor the road bit is set even if no road exist.
    // It is important to verify the existence of a road without relying on this bit.
    if ( isSpriteRoad( objectIcnType, ma.bottomIcnImageIndex ) ) {
        _isTileMarkedAsRoad = true;
    }

    _addonBottomLayer.emplace_back( static_cast<uint8_t>( ma.quantityN & 0x03 ), ma.level1ObjectUID, objectIcnType, ma.bottomIcnImageIndex );
}

void Maps::Tiles::pushTopLayerAddon( const MP2::MP2AddonInfo & ma )
{
    const MP2::ObjectIcnType objectIcnType = static_cast<MP2::ObjectIcnType>( ma.objectNameN2 >> 2 );
    if ( objectIcnType == MP2::ObjectIcnType::OBJ_ICN_TYPE_UNKNOWN ) {
        // No object exist.
        return;
    }

    // Top layer objects do not have any internal structure (layers) so all of them should have the same internal layer.
    // TODO: remove layer type for top layer objects.
    _addonTopLayer.emplace_back( OBJECT_LAYER, ma.level2ObjectUID, objectIcnType, ma.topIcnImageIndex );
}

void Maps::Tiles::pushBottomLayerAddon( TilesAddon ta )
{
    if ( isSpriteRoad( ta._objectIcnType, ta._imageIndex ) ) {
        _isTileMarkedAsRoad = true;
    }

    _addonBottomLayer.emplace_back( ta );
}

void Maps::Tiles::AddonsSort()
{
    if ( _addonBottomLayer.empty() ) {
        // Nothing to sort.
        return;
    }

    // Push everything to the container and sort it by level.
    if ( _mainAddon._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN ) {
        _addonBottomLayer.emplace_front( _mainAddon );
    }

    // Sort by internal layers.
    _addonBottomLayer.sort( []( const auto & left, const auto & right ) { return ( left._layerType > right._layerType ); } );

    if ( !_addonBottomLayer.empty() ) {
        TilesAddon & highestPriorityAddon = _addonBottomLayer.back();
        std::swap( highestPriorityAddon, _mainAddon );

        // If this assertion blows up then you are not storing correct values for layer type!
        assert( _mainAddon._layerType <= TERRAIN_LAYER );

        _addonBottomLayer.pop_back();
    }

    // Top layer objects don't have any rendering priorities so they should be rendered first in queue first to render.
}

Maps::TilesAddon * Maps::Tiles::getBottomLayerAddon( const uint32_t uid )
{
    auto it = std::find_if( _addonBottomLayer.begin(), _addonBottomLayer.end(), [uid]( const TilesAddon & v ) { return v._uid == uid; } );

    return it != _addonBottomLayer.end() ? &( *it ) : nullptr;
}

Maps::TilesAddon * Maps::Tiles::getTopLayerAddon( const uint32_t uid )
{
    auto it = std::find_if( _addonTopLayer.begin(), _addonTopLayer.end(), [uid]( const TilesAddon & v ) { return v._uid == uid; } );

    return it != _addonTopLayer.end() ? &( *it ) : nullptr;
}

std::string Maps::Tiles::String() const
{
    std::ostringstream os;

    const MP2::MapObjectType objectType = GetObject();

    os << "******* Tile info *******" << std::endl
       << "Tile index      : " << _index << ", "
       << "point: (" << GetCenter().x << ", " << GetCenter().y << ")" << std::endl
       << "MP2 object type : " << static_cast<int>( objectType ) << " (" << MP2::StringObject( objectType ) << ")" << std::endl;

    getAddonInfo( _mainAddon, os );

    os << "region          : " << _region << std::endl
       << "ground          : " << Ground::String( GetGround() ) << " (isRoad: " << _isTileMarkedAsRoad << ")" << std::endl
       << "ground img index: " << _terrainImageIndex << ", image flags: " << static_cast<int>( _terrainFlags ) << std::endl
       << "passable from   : " << ( _tilePassabilityDirections ? Direction::String( _tilePassabilityDirections ) : "nowhere" ) << std::endl
       << "metadata value 1: " << _metadata[0] << std::endl
       << "metadata value 2: " << _metadata[1] << std::endl
       << "metadata value 3: " << _metadata[2] << std::endl;

    if ( objectType == MP2::OBJ_BOAT )
        os << "boat owner color: " << Color::String( _boatOwnerColor ) << std::endl;

    for ( const TilesAddon & addon : _addonBottomLayer ) {
        os << getAddonInfo( addon, 1 );
    }

    for ( const TilesAddon & addon : _addonTopLayer ) {
        os << getAddonInfo( addon, 2 );
    }

    os << "--- Extra information ---" << std::endl;

    switch ( objectType ) {
    case MP2::OBJ_RUINS:
    case MP2::OBJ_TREE_CITY:
    case MP2::OBJ_WAGON_CAMP:
    case MP2::OBJ_DESERT_TENT:
    case MP2::OBJ_TROLL_BRIDGE:
    case MP2::OBJ_DRAGON_CITY:
    case MP2::OBJ_CITY_OF_DEAD:
    case MP2::OBJ_WATCH_TOWER:
    case MP2::OBJ_EXCAVATION:
    case MP2::OBJ_CAVE:
    case MP2::OBJ_TREE_HOUSE:
    case MP2::OBJ_ARCHER_HOUSE:
    case MP2::OBJ_GOBLIN_HUT:
    case MP2::OBJ_DWARF_COTTAGE:
    case MP2::OBJ_HALFLING_HOLE:
    case MP2::OBJ_PEASANT_HUT:
    case MP2::OBJ_MONSTER:
        os << "monster count   : " << getMonsterCountFromTile( *this ) << std::endl;
        break;
    case MP2::OBJ_HERO: {
        const Heroes * hero = getHero();
        if ( hero )
            os << hero->String();
        break;
    }
    case MP2::OBJ_NON_ACTION_CASTLE:
    case MP2::OBJ_CASTLE: {
        const Castle * castle = world.getCastle( GetCenter() );
        if ( castle )
            os << castle->String();
        break;
    }
    default: {
        const MapsIndexes & v = Maps::getMonstersProtectingTile( _index );
        if ( !v.empty() ) {
            os << "protection      : ";
            for ( const int32_t index : v ) {
                os << index << ", ";
            }
            os << std::endl;
        }
        break;
    }
    }

    if ( MP2::isCaptureObject( GetObject( false ) ) ) {
        const CapturedObject & co = world.GetCapturedObject( _index );

        os << "capture color   : " << Color::String( co.objcol.second ) << std::endl;
        if ( co.guardians.isValid() ) {
            os << "capture guard   : " << co.guardians.GetName() << std::endl << "capture count   : " << co.guardians.GetCount() << std::endl;
        }
    }

    os << "*************************" << std::endl;

    return os.str();
}

bool Maps::Tiles::GoodForUltimateArtifact() const
{
    if ( isWater() || !isPassableFrom( Direction::CENTER, false, true, 0 ) ) {
        return false;
    }

    if ( _mainAddon._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN && !isAddonShadow( _mainAddon ) ) {
        return false;
    }

    if ( static_cast<size_t>( std::count_if( _addonBottomLayer.begin(), _addonBottomLayer.end(), isAddonShadow ) ) != _addonBottomLayer.size() ) {
        return false;
    }

    if ( static_cast<size_t>( std::count_if( _addonTopLayer.begin(), _addonTopLayer.end(), isAddonShadow ) ) != _addonTopLayer.size() ) {
        return false;
    }

    return true;
}

bool Maps::Tiles::isPassabilityTransparent() const
{
    for ( const TilesAddon & addon : _addonBottomLayer ) {
        if ( !addon.isPassabilityTransparent() ) {
            return false;
        }
    }

    return _mainAddon.isPassabilityTransparent();
}

bool Maps::Tiles::isPassableFrom( const int direction, const bool fromWater, const bool skipFog, const int heroColor ) const
{
    if ( !skipFog && isFog( heroColor ) ) {
        return false;
    }

    const bool tileIsWater = isWater();

    // From the water we can get either to the coast tile or to the water tile (provided there is no boat on this tile).
    if ( fromWater && _mainObjectType != MP2::OBJ_COAST && ( !tileIsWater || _mainObjectType == MP2::OBJ_BOAT ) ) {
        return false;
    }

    // From the ground we can get to the water tile only if this tile contains a certain object.
    if ( !fromWater && tileIsWater && _mainObjectType != MP2::OBJ_SHIPWRECK && _mainObjectType != MP2::OBJ_HERO && _mainObjectType != MP2::OBJ_BOAT ) {
        return false;
    }

    // Tiles on which allied heroes are located are inaccessible
    if ( _mainObjectType == MP2::OBJ_HERO ) {
        const Heroes * hero = getHero();
        assert( hero != nullptr );

        if ( hero->GetColor() != heroColor && hero->isFriends( heroColor ) ) {
            return false;
        }
    }

    // Tiles on which the entrances to the allied castles are located are inaccessible
    if ( _mainObjectType == MP2::OBJ_CASTLE ) {
        const Castle * castle = world.getCastleEntrance( GetCenter() );

        if ( castle && castle->GetColor() != heroColor && castle->isFriends( heroColor ) ) {
            return false;
        }
    }

    return ( direction & _tilePassabilityDirections ) != 0;
}

void Maps::Tiles::SetObjectPassable( bool pass )
{
    if ( GetObject( false ) == MP2::OBJ_TROLL_BRIDGE ) {
        if ( pass ) {
            _tilePassabilityDirections |= Direction::TOP_LEFT;
        }
        else {
            _tilePassabilityDirections &= ~Direction::TOP_LEFT;
        }
    }
}

bool Maps::Tiles::isStream() const
{
    for ( const TilesAddon & addon : _addonBottomLayer ) {
        if ( addon._objectIcnType == MP2::OBJ_ICN_TYPE_STREAM
             || ( addon._objectIcnType == MP2::OBJ_ICN_TYPE_OBJNMUL2
                  && ( addon._imageIndex < 14 || ( addon._imageIndex > 217 && addon._imageIndex < ( 218 + 14 ) ) ) ) ) {
            return true;
        }
    }

    return _mainAddon._objectIcnType == MP2::OBJ_ICN_TYPE_STREAM
           || ( _mainAddon._objectIcnType == MP2::OBJ_ICN_TYPE_OBJNMUL2
                && ( _mainAddon._imageIndex < 14 || ( _mainAddon._imageIndex > 217 && _mainAddon._imageIndex < ( 218 + 14 ) ) ) );
}

bool Maps::Tiles::isShadow() const
{
    return isAddonShadow( _mainAddon )
           && _addonBottomLayer.size() == static_cast<size_t>( std::count_if( _addonBottomLayer.begin(), _addonBottomLayer.end(), isAddonShadow ) );
}

Maps::TilesAddon * Maps::Tiles::getAddonWithFlag( const uint32_t uid )
{
    const auto isFlag = [uid]( const TilesAddon & addon ) { return addon._uid == uid && addon._objectIcnType == MP2::OBJ_ICN_TYPE_FLAG32; };

    auto iter = std::find_if( _addonBottomLayer.begin(), _addonBottomLayer.end(), isFlag );
    if ( iter != _addonBottomLayer.end() ) {
        return &( *iter );
    }

    iter = std::find_if( _addonTopLayer.begin(), _addonTopLayer.end(), isFlag );
    if ( iter != _addonTopLayer.end() ) {
        return &( *iter );
    }

    return nullptr;
}

void Maps::Tiles::setOwnershipFlag( const MP2::MapObjectType objectType, const int color )
{
    // All flags in FLAG32.ICN are actually the same except the fact of having different offset.
    // Set the default value for the UNUSED color.
    uint8_t objectSpriteIndex = 6;

    switch ( color ) {
    case Color::NONE:
        // No flag. Just ignore it.
        break;
    case Color::BLUE:
        objectSpriteIndex = 0;
        break;
    case Color::GREEN:
        objectSpriteIndex = 1;
        break;
    case Color::RED:
        objectSpriteIndex = 2;
        break;
    case Color::YELLOW:
        objectSpriteIndex = 3;
        break;
    case Color::ORANGE:
        objectSpriteIndex = 4;
        break;
    case Color::PURPLE:
        objectSpriteIndex = 5;
        break;
    case Color::UNUSED:
        // Neutral (gray) flag. Index '6' is already set.
        break;
    default:
        // Did you add a new color type? Add logic above!
        assert( 0 );
        break;
    }

    switch ( objectType ) {
    case MP2::OBJ_MAGIC_GARDEN:
        objectSpriteIndex += 128 + 14;
        updateFlag( color, objectSpriteIndex, _mainAddon._uid, false );
        objectSpriteIndex += 7;
        if ( Maps::isValidDirection( _index, Direction::RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::RIGHT ) );
            tile.updateFlag( color, objectSpriteIndex, _mainAddon._uid, false );
        }
        break;

    case MP2::OBJ_WATER_WHEEL:
    case MP2::OBJ_MINE:
        objectSpriteIndex += 128 + 14;
        if ( Maps::isValidDirection( _index, Direction::TOP ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP ) );
            tile.updateFlag( color, objectSpriteIndex, _mainAddon._uid, true );
        }

        objectSpriteIndex += 7;
        if ( Maps::isValidDirection( _index, Direction::TOP_RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP_RIGHT ) );
            tile.updateFlag( color, objectSpriteIndex, _mainAddon._uid, true );
        }
        break;

    case MP2::OBJ_WINDMILL:
    case MP2::OBJ_LIGHTHOUSE:
        objectSpriteIndex += 128 + 42;
        if ( Maps::isValidDirection( _index, Direction::LEFT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) );
            tile.updateFlag( color, objectSpriteIndex, _mainAddon._uid, false );
        }

        objectSpriteIndex += 7;
        updateFlag( color, objectSpriteIndex, _mainAddon._uid, false );
        break;

    case MP2::OBJ_ALCHEMIST_LAB:
        objectSpriteIndex += 21;
        if ( Maps::isValidDirection( _index, Direction::TOP ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP ) );
            tile.updateFlag( color, objectSpriteIndex, _mainAddon._uid, true );
        }
        break;

    case MP2::OBJ_SAWMILL:
        objectSpriteIndex += 28;
        if ( Maps::isValidDirection( _index, Direction::TOP_RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP_RIGHT ) );
            tile.updateFlag( color, objectSpriteIndex, _mainAddon._uid, true );
        }
        break;

    case MP2::OBJ_CASTLE:
        objectSpriteIndex *= 2;
        if ( Maps::isValidDirection( _index, Direction::LEFT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) );
            tile.updateFlag( color, objectSpriteIndex, _mainAddon._uid, true );
        }

        objectSpriteIndex += 1;
        if ( Maps::isValidDirection( _index, Direction::RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::RIGHT ) );
            tile.updateFlag( color, objectSpriteIndex, _mainAddon._uid, true );
        }
        break;

    default:
        break;
    }
}

void Maps::Tiles::removeOwnershipFlag( const MP2::MapObjectType objectType )
{
    setOwnershipFlag( objectType, Color::NONE );
}

void Maps::Tiles::updateFlag( const int color, const uint8_t objectSpriteIndex, const uint32_t uid, const bool setOnUpperLayer )
{
    // Flag deletion or installation must be done in relation to object UID as flag is attached to the object.
    if ( color == Color::NONE ) {
        const auto isFlag = [uid]( const TilesAddon & addon ) { return addon._uid == uid && addon._objectIcnType == MP2::OBJ_ICN_TYPE_FLAG32; };
        _addonBottomLayer.remove_if( isFlag );
        _addonTopLayer.remove_if( isFlag );
        return;
    }

    TilesAddon * addon = getAddonWithFlag( uid );
    if ( addon != nullptr ) {
        // Replace an existing flag.
        addon->_imageIndex = objectSpriteIndex;
    }
    else if ( setOnUpperLayer ) {
        _addonTopLayer.emplace_back( OBJECT_LAYER, uid, MP2::OBJ_ICN_TYPE_FLAG32, objectSpriteIndex );
    }
    else {
        _addonBottomLayer.emplace_back( OBJECT_LAYER, uid, MP2::OBJ_ICN_TYPE_FLAG32, objectSpriteIndex );
    }
}

void Maps::Tiles::_updateRoadFlag()
{
    _isTileMarkedAsRoad = isSpriteRoad( _mainAddon._objectIcnType, _mainAddon._imageIndex );

    if ( _isTileMarkedAsRoad ) {
        return;
    }

    for ( const TilesAddon & addon : _addonBottomLayer ) {
        if ( isSpriteRoad( addon._objectIcnType, addon._imageIndex ) ) {
            _isTileMarkedAsRoad = true;
            return;
        }
    }
}

void Maps::Tiles::fixMP2MapTileObjectType( Tiles & tile )
{
    const MP2::MapObjectType originalObjectType = tile.GetObject( false );

    // Left tile of a skeleton on Desert should be marked as non-action tile.
    if ( originalObjectType == MP2::OBJ_SKELETON && tile._mainAddon._objectIcnType == MP2::OBJ_ICN_TYPE_OBJNDSRT && tile._mainAddon._imageIndex == 83 ) {
        tile.SetObject( MP2::OBJ_NON_ACTION_SKELETON );

        // There is no need to check the rest of things as we fixed this object.
        return;
    }

    // Oasis object has 2 top tiles being marked as part of bottom object layer while in reality they should be at the top level.
    if ( originalObjectType == MP2::OBJ_NON_ACTION_OASIS && tile._mainAddon._objectIcnType == MP2::OBJ_ICN_TYPE_OBJNDSRT
         && ( tile._mainAddon._imageIndex == 105 || tile._mainAddon._imageIndex == 106 ) ) {
        tile._addonTopLayer.emplace_back();
        std::swap( tile._addonTopLayer.back(), tile._mainAddon );

        return;
    }

    // Original Editor marks Reefs as Stones. We're fixing this issue by changing the type of the object without changing the content of a tile.
    // This is also required in order to properly calculate Reefs' passability.
    if ( originalObjectType == MP2::OBJ_ROCK && isValidReefsSprite( tile._mainAddon._objectIcnType, tile._mainAddon._imageIndex ) ) {
        tile.SetObject( MP2::OBJ_REEFS );

        // There is no need to check the rest of things as we fixed this object.
        return;
    }

    // Some maps have water tiles with OBJ_COAST, it shouldn't be, replace OBJ_COAST with OBJ_NONE
    if ( originalObjectType == MP2::OBJ_COAST && tile.isWater() ) {
        Heroes * hero = tile.getHero();

        if ( hero ) {
            hero->setObjectTypeUnderHero( MP2::OBJ_NONE );
        }
        else {
            tile.SetObject( MP2::OBJ_NONE );
        }

        // There is no need to check the rest of things as we fixed this object.
        return;
    }

    // On some maps (apparently created by some non-standard editors), the object type on tiles with random monsters does not match the index
    // of the monster placeholder sprite. While this engine looks at the object type when placing an actual monster on a tile, the original
    // HoMM2 apparently looks at the placeholder sprite, so we need to keep them in sync.
    if ( tile._mainAddon._objectIcnType == MP2::OBJ_ICN_TYPE_MONS32 ) {
        MP2::MapObjectType monsterObjectType = originalObjectType;

        const uint8_t originalObjectSpriteIndex = tile.GetObjectSpriteIndex();
        switch ( originalObjectSpriteIndex ) {
        // Random monster placeholder "MON"
        case 66:
            monsterObjectType = MP2::OBJ_RANDOM_MONSTER;
            break;
        // Random monster placeholder "MON 1"
        case 67:
            monsterObjectType = MP2::OBJ_RANDOM_MONSTER_WEAK;
            break;
        // Random monster placeholder "MON 2"
        case 68:
            monsterObjectType = MP2::OBJ_RANDOM_MONSTER_MEDIUM;
            break;
        // Random monster placeholder "MON 3"
        case 69:
            monsterObjectType = MP2::OBJ_RANDOM_MONSTER_STRONG;
            break;
        // Random monster placeholder "MON 4"
        case 70:
            monsterObjectType = MP2::OBJ_RANDOM_MONSTER_VERY_STRONG;
            break;
        default:
            break;
        }

        if ( monsterObjectType != originalObjectType ) {
            tile.SetObject( monsterObjectType );

            DEBUG_LOG( DBG_GAME, DBG_WARN,
                       "Invalid object type index " << tile._index << ": type " << MP2::StringObject( originalObjectType ) << ", object sprite index "
                                                    << static_cast<int>( originalObjectSpriteIndex ) << ", corrected type " << MP2::StringObject( monsterObjectType ) )

            // There is no need to check the rest of things as we fixed this object.
            return;
        }
    }

    // Fix The Price of Loyalty objects even if the map is The Succession Wars type.
    switch ( originalObjectType ) {
    case MP2::OBJ_NON_ACTION_EXPANSION_DWELLING:
    case MP2::OBJ_NON_ACTION_EXPANSION_OBJECT:
    case MP2::OBJ_EXPANSION_DWELLING:
    case MP2::OBJ_EXPANSION_OBJECT: {
        // The type of expansion action object or dwelling is stored in object metadata.
        // However, we just ignore it.
        MP2::MapObjectType objectType = getLoyaltyObject( tile._mainAddon._objectIcnType, tile._mainAddon._imageIndex );
        if ( objectType != MP2::OBJ_NONE ) {
            tile.SetObject( objectType );
            break;
        }

        // Add-ons of level 1 shouldn't even exist if no top object is present. However, let's play safe and verify it as well.
        for ( const TilesAddon & addon : tile._addonBottomLayer ) {
            objectType = getLoyaltyObject( addon._objectIcnType, addon._imageIndex );
            if ( objectType != MP2::OBJ_NONE )
                break;
        }

        if ( objectType != MP2::OBJ_NONE ) {
            tile.SetObject( objectType );
            break;
        }

        for ( const TilesAddon & addon : tile._addonTopLayer ) {
            objectType = getLoyaltyObject( addon._objectIcnType, addon._imageIndex );
            if ( objectType != MP2::OBJ_NONE )
                break;
        }

        if ( objectType != MP2::OBJ_NONE ) {
            tile.SetObject( objectType );
            break;
        }

        DEBUG_LOG( DBG_GAME, DBG_WARN,
                   "Invalid object type index " << tile._index << ": type " << MP2::StringObject( originalObjectType ) << ", icn ID "
                                                << static_cast<int>( tile._mainAddon._imageIndex ) )
        break;
    }

    default:
        break;
    }
}

bool Maps::Tiles::removeObjectPartsByUID( const uint32_t objectUID )
{
    bool isObjectPartRemoved = false;
    if ( _mainAddon._uid == objectUID ) {
        _mainAddon = {};

        isObjectPartRemoved = true;
    }

    size_t addonCountBefore = _addonBottomLayer.size();
    _addonBottomLayer.remove_if( [objectUID]( const Maps::TilesAddon & v ) { return v._uid == objectUID; } );
    if ( addonCountBefore != _addonBottomLayer.size() ) {
        isObjectPartRemoved = true;
    }

    addonCountBefore = _addonTopLayer.size();
    _addonTopLayer.remove_if( [objectUID]( const Maps::TilesAddon & v ) { return v._uid == objectUID; } );
    if ( addonCountBefore != _addonTopLayer.size() ) {
        isObjectPartRemoved = true;
    }

    if ( isObjectPartRemoved ) {
        // Since an object part was removed we have to update main object type.
        updateObjectType();

        setInitialPassability();
        updatePassability();

        if ( Heroes::isValidId( _occupantHeroId ) ) {
            Heroes * hero = world.GetHeroes( _occupantHeroId );
            if ( hero != nullptr ) {
                hero->setObjectTypeUnderHero( _mainObjectType );

                SetObject( MP2::OBJ_HERO );
            }
        }

        // TODO: since we remove an object we need to check whether this tile contains any object with additional metadata.
        //       If it doesn't contain we need to reset metadata.
    }

    return isObjectPartRemoved;
}

void Maps::Tiles::removeObjects( const MP2::ObjectIcnType objectIcnType )
{
    _addonBottomLayer.remove_if( [objectIcnType]( const Maps::TilesAddon & addon ) { return addon._objectIcnType == objectIcnType; } );
    _addonTopLayer.remove_if( [objectIcnType]( const Maps::TilesAddon & addon ) { return addon._objectIcnType == objectIcnType; } );

    if ( _mainAddon._objectIcnType == objectIcnType ) {
        resetObjectSprite();
        _mainAddon._uid = 0;
    }

    _updateRoadFlag();
}

void Maps::Tiles::replaceObject( const uint32_t objectUid, const MP2::ObjectIcnType originalObjectIcnType, const MP2::ObjectIcnType newObjectIcnType,
                                 const uint8_t originalImageIndex, const uint8_t newImageIndex )
{
    // We can immediately return from the function as only one object per tile can have the same UID.
    for ( TilesAddon & addon : _addonBottomLayer ) {
        if ( addon._uid == objectUid && addon._objectIcnType == originalObjectIcnType && addon._imageIndex == originalImageIndex ) {
            addon._objectIcnType = newObjectIcnType;
            addon._imageIndex = newImageIndex;
            return;
        }
    }

    for ( TilesAddon & addon : _addonTopLayer ) {
        if ( addon._uid == objectUid && addon._objectIcnType == originalObjectIcnType && addon._imageIndex == originalImageIndex ) {
            addon._objectIcnType = newObjectIcnType;
            addon._imageIndex = newImageIndex;
            return;
        }
    }

    if ( _mainAddon._uid == objectUid && _mainAddon._objectIcnType == originalObjectIcnType && _mainAddon._imageIndex == originalImageIndex ) {
        _mainAddon._objectIcnType = newObjectIcnType;
        _mainAddon._imageIndex = newImageIndex;
    }
}

void Maps::Tiles::updateObjectImageIndex( const uint32_t objectUid, const MP2::ObjectIcnType objectIcnType, const int imageIndexOffset )
{
    // We can immediately return from the function as only one object per tile can have the same UID.
    for ( TilesAddon & addon : _addonBottomLayer ) {
        if ( addon._uid == objectUid && addon._objectIcnType == objectIcnType ) {
            assert( addon._imageIndex + imageIndexOffset >= 0 && addon._imageIndex + imageIndexOffset < 255 );
            addon._imageIndex = static_cast<uint8_t>( addon._imageIndex + imageIndexOffset );
            return;
        }
    }

    for ( TilesAddon & addon : _addonTopLayer ) {
        if ( addon._uid == objectUid && addon._objectIcnType == objectIcnType ) {
            assert( addon._imageIndex + imageIndexOffset >= 0 && addon._imageIndex + imageIndexOffset < 255 );
            addon._imageIndex = static_cast<uint8_t>( addon._imageIndex + imageIndexOffset );
            return;
        }
    }

    if ( _mainAddon._uid == objectUid && _mainAddon._objectIcnType == objectIcnType ) {
        assert( _mainAddon._imageIndex + imageIndexOffset >= 0 && _mainAddon._imageIndex + imageIndexOffset < 255 );
        _mainAddon._imageIndex = static_cast<uint8_t>( _mainAddon._imageIndex + imageIndexOffset );
    }
}

void Maps::Tiles::ClearFog( const int colors )
{
    _fogColors &= ~colors;

    // The fog might be cleared even without the hero's movement - for example, the hero can gain a new level of Scouting
    // skill by picking up a Treasure Chest from a nearby tile or buying a map in a Magellan's Maps object using the space
    // bar button. Reset the pathfinder(s) to make the newly discovered tiles immediately available for this hero.
    world.resetPathfinder();
}

void Maps::Tiles::updateTileObjectIcnIndex( Maps::Tiles & tile, const uint32_t uid, const uint8_t newIndex )
{
    Maps::TilesAddon * addon = tile.getBottomLayerAddon( uid );
    if ( addon != nullptr ) {
        addon->_imageIndex = newIndex;
    }
    else if ( tile._mainAddon._uid == uid ) {
        tile._mainAddon._imageIndex = newIndex;
    }

    tile._updateRoadFlag();
}

void Maps::Tiles::updateObjectType()
{
    // After removing an object there could be an object part in the main addon.
    MP2::MapObjectType objectType = Maps::getObjectTypeByIcn( _mainAddon._objectIcnType, _mainAddon._imageIndex );
    if ( MP2::isOffGameActionObject( objectType ) ) {
        // Set object type only when this is an interactive object type to make sure that interaction can be done.
        SetObject( objectType );
        return;
    }

    // And sometimes even in the bottom layer addons.
    // Take a note that we iterate object parts from back to front as the latest object part has higher priority.
    if ( objectType == MP2::OBJ_NONE ) {
        for ( auto iter = _addonBottomLayer.rbegin(); iter != _addonBottomLayer.rend(); ++iter ) {
            const MP2::MapObjectType type = Maps::getObjectTypeByIcn( iter->_objectIcnType, iter->_imageIndex );
            if ( type == MP2::OBJ_NONE ) {
                continue;
            }

            if ( MP2::isOffGameActionObject( type ) ) {
                // Set object type only when this is an interactive object type to make sure that interaction can be done.
                SetObject( type );
                return;
            }

            if ( objectType == MP2::OBJ_NONE ) {
                objectType = type;
            }
        }
    }

    // Or object part can be in the top layer addons.
    // Take a note that we iterate object parts from back to front as the latest object part has higher priority.
    for ( auto iter = _addonTopLayer.rbegin(); iter != _addonTopLayer.rend(); ++iter ) {
        const MP2::MapObjectType type = Maps::getObjectTypeByIcn( iter->_objectIcnType, iter->_imageIndex );

        if ( type != MP2::OBJ_NONE ) {
            SetObject( type );
            return;
        }
    }

    // Top objects do not have object type while bottom object do.
    if ( objectType != MP2::OBJ_NONE ) {
        SetObject( objectType );
        return;
    }

    // If an object is removed we should validate if this tile a potential candidate to be a coast.
    // Check if this tile is not water and it has neighbouring water tiles.
    if ( isWater() ) {
        assert( objectType == MP2::OBJ_NONE );
        SetObject( objectType );
        return;
    }

    const Indexes tileIndices = Maps::getAroundIndexes( _index, 1 );
    for ( const int tileIndex : tileIndices ) {
        if ( tileIndex < 0 ) {
            // Invalid tile index.
            continue;
        }

        if ( world.GetTiles( tileIndex ).isWater() ) {
            SetObject( MP2::OBJ_COAST );
            return;
        }
    }

    assert( objectType == MP2::OBJ_NONE );
    SetObject( objectType );
}

uint32_t Maps::Tiles::getObjectIdByObjectIcnType( const MP2::ObjectIcnType objectIcnType ) const
{
    if ( _mainAddon._objectIcnType == objectIcnType ) {
        return _mainAddon._uid;
    }

    for ( const TilesAddon & addon : _addonBottomLayer ) {
        if ( addon._objectIcnType == objectIcnType ) {
            return addon._uid;
        }
    }

    for ( const TilesAddon & addon : _addonTopLayer ) {
        if ( addon._objectIcnType == objectIcnType ) {
            return addon._uid;
        }
    }

    return 0;
}

std::vector<MP2::ObjectIcnType> Maps::Tiles::getValidObjectIcnTypes() const
{
    std::vector<MP2::ObjectIcnType> objectIcnTypes;

    if ( _mainAddon._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN ) {
        objectIcnTypes.emplace_back( _mainAddon._objectIcnType );
    }

    for ( const TilesAddon & addon : _addonBottomLayer ) {
        // If this assertion blows up then you put an empty object into an addon which makes no sense!
        assert( addon._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN );

        objectIcnTypes.emplace_back( addon._objectIcnType );
    }

    for ( const TilesAddon & addon : _addonTopLayer ) {
        // If this assertion blows up then you put an empty object into an addon which makes no sense!
        assert( addon._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN );

        objectIcnTypes.emplace_back( addon._objectIcnType );
    }

    return objectIcnTypes;
}

bool Maps::Tiles::containsAnyObjectIcnType( const std::vector<MP2::ObjectIcnType> & objectIcnTypes ) const
{
    for ( const MP2::ObjectIcnType objectIcnType : objectIcnTypes ) {
        if ( _mainAddon._objectIcnType == objectIcnType ) {
            return true;
        }

        for ( const TilesAddon & addon : _addonBottomLayer ) {
            if ( addon._objectIcnType == objectIcnType ) {
                return true;
            }
        }

        for ( const TilesAddon & addon : _addonTopLayer ) {
            if ( addon._objectIcnType == objectIcnType ) {
                return true;
            }
        }
    }

    return false;
}

bool Maps::Tiles::containsSprite( const MP2::ObjectIcnType objectIcnType, const uint32_t imageIdx ) const
{
    if ( _mainAddon._objectIcnType == objectIcnType && imageIdx == _mainAddon._imageIndex ) {
        return true;
    }

    if ( std::any_of( _addonBottomLayer.cbegin(), _addonBottomLayer.cend(),
                      [objectIcnType, imageIdx]( const TilesAddon & addon ) { return addon._objectIcnType == objectIcnType && imageIdx == addon._imageIndex; } ) ) {
        return true;
    }

    return std::any_of( _addonTopLayer.cbegin(), _addonTopLayer.cend(),
                        [objectIcnType, imageIdx]( const TilesAddon & addon ) { return addon._objectIcnType == objectIcnType && imageIdx == addon._imageIndex; } );
}

bool Maps::Tiles::isTallObject() const
{
    // TODO: possibly cache the output of the method as right now it's in average twice.
    if ( !Maps::isValidDirection( _index, Direction::TOP ) ) {
        // Nothing above so this object can't be tall.
        return false;
    }

    std::vector<uint32_t> tileUIDs;
    if ( _mainAddon._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN && _mainAddon._uid != 0 && !_mainAddon.isPassabilityTransparent() ) {
        tileUIDs.emplace_back( _mainAddon._uid );
    }

    for ( const TilesAddon & addon : _addonBottomLayer ) {
        if ( addon._uid != 0 && !addon.isPassabilityTransparent() ) {
            tileUIDs.emplace_back( addon._uid );
        }
    }

    for ( const TilesAddon & addon : _addonTopLayer ) {
        if ( addon._uid != 0 && !addon.isPassabilityTransparent() ) {
            tileUIDs.emplace_back( addon._uid );
        }
    }

    const Tiles & topTile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP ) );
    for ( const uint32_t tileUID : tileUIDs ) {
        if ( topTile._mainAddon._uid == tileUID && !isAddonShadow( topTile._mainAddon ) ) {
            return true;
        }

        for ( const TilesAddon & addon : topTile._addonBottomLayer ) {
            if ( addon._uid == tileUID && !isAddonShadow( addon ) ) {
                return true;
            }
        }

        for ( const TilesAddon & addon : topTile._addonTopLayer ) {
            if ( addon._uid == tileUID && !isAddonShadow( addon ) ) {
                return true;
            }
        }
    }

    return false;
}

int32_t Maps::Tiles::getIndexOfMainTile( const Maps::Tiles & tile )
{
    const MP2::MapObjectType objectType = tile.GetObject( false );
    const MP2::MapObjectType correctedObjectType = MP2::getBaseActionObjectType( objectType );

    if ( correctedObjectType == objectType ) {
        // Nothing to do.
        return tile._index;
    }

    assert( correctedObjectType > objectType );

    // It's unknown whether object type belongs to bottom layer or ground. Create a list of UIDs starting from bottom layer.
    std::set<uint32_t> uids;
    uids.insert( tile.GetObjectUID() );

    for ( const TilesAddon & addon : tile.getBottomLayerAddons() ) {
        uids.insert( addon._uid );
    }

    for ( const TilesAddon & addon : tile.getTopLayerAddons() ) {
        uids.insert( addon._uid );
    }

    const int32_t tileIndex = tile.GetIndex();
    const int32_t mapWidth = world.w();

    // This is non-main tile of an action object. We have to find the main tile.
    // Since we don't want to care about the size of every object in the game we should find tiles in a certain radius.
    const int32_t radiusOfSearch = 3;

    // Main tile is usually at the bottom of the object so let's start from there. Also there are no objects having tiles below more than 1 row.
    for ( int32_t y = radiusOfSearch; y >= -1; --y ) {
        const int32_t offsetX = tileIndex + y * mapWidth;
        for ( int32_t x = -radiusOfSearch; x <= radiusOfSearch; ++x ) {
            const int32_t index = offsetX + x;
            if ( Maps::isValidAbsIndex( index ) ) {
                const Maps::Tiles & foundTile = world.GetTiles( index );
                if ( foundTile.GetObject( false ) != correctedObjectType ) {
                    continue;
                }

                if ( foundTile.GetObjectUID() != 0 && uids.count( foundTile.GetObjectUID() ) > 0 ) {
                    return foundTile._index;
                }
            }
        }
    }

    // Most likely we have a broken object put by an editor.
    DEBUG_LOG( DBG_GAME, DBG_TRACE, "Tile " << tileIndex << " of type " << MP2::StringObject( objectType ) << " has no parent tile." )
    return -1;
}

bool Maps::Tiles::isDetachedObject() const
{
    const MP2::MapObjectType objectType = GetObject( false );
    if ( isDetachedObjectType( objectType ) ) {
        return true;
    }

    const MP2::MapObjectType correctedObjectType = MP2::getBaseActionObjectType( objectType );
    if ( !isDetachedObjectType( correctedObjectType ) ) {
        return false;
    }

    const int32_t mainTileIndex = Maps::Tiles::getIndexOfMainTile( *this );
    if ( mainTileIndex == -1 ) {
        return false;
    }

    const uint32_t objectUID = world.GetTiles( mainTileIndex ).GetObjectUID();
    if ( _mainAddon._uid == objectUID ) {
        return !_mainAddon.isPassabilityTransparent();
    }

    for ( const TilesAddon & addon : _addonBottomLayer ) {
        if ( addon._uid == objectUID ) {
            return !addon.isPassabilityTransparent();
        }
    }

    return false;
}

StreamBase & Maps::operator<<( StreamBase & msg, const TilesAddon & ta )
{
    using ObjectIcnTypeUnderlyingType = std::underlying_type_t<decltype( ta._objectIcnType )>;

    return msg << ta._layerType << ta._uid << static_cast<ObjectIcnTypeUnderlyingType>( ta._objectIcnType ) << ta._imageIndex;
}

StreamBase & Maps::operator>>( StreamBase & msg, TilesAddon & ta )
{
    msg >> ta._layerType;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE2_1009_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE2_1009_RELEASE ) {
        ta._layerType = ( ta._layerType & 0x03 );
    }

    msg >> ta._uid;

    using ObjectIcnTypeUnderlyingType = std::underlying_type_t<decltype( ta._objectIcnType )>;
    static_assert( std::is_same_v<ObjectIcnTypeUnderlyingType, uint8_t>, "Type of _objectIcnType has been changed, check the logic below" );

    ObjectIcnTypeUnderlyingType objectIcnType = MP2::OBJ_ICN_TYPE_UNKNOWN;
    msg >> objectIcnType;

    ta._objectIcnType = static_cast<MP2::ObjectIcnType>( objectIcnType );

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE2_1009_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE2_1009_RELEASE ) {
        bool temp;
        msg >> temp >> temp;
    }

    msg >> ta._imageIndex;

    return msg;
}

StreamBase & Maps::operator<<( StreamBase & msg, const Tiles & tile )
{
    using ObjectIcnTypeUnderlyingType = std::underlying_type_t<decltype( tile._mainAddon._objectIcnType )>;
    using MainObjectTypeUnderlyingType = std::underlying_type_t<decltype( tile._mainObjectType )>;

    // TODO: use operator<<() for _mainAddon.
    return msg << tile._index << tile._terrainImageIndex << tile._terrainFlags << tile._tilePassabilityDirections << tile._mainAddon._uid
               << static_cast<ObjectIcnTypeUnderlyingType>( tile._mainAddon._objectIcnType ) << tile._mainAddon._imageIndex
               << static_cast<MainObjectTypeUnderlyingType>( tile._mainObjectType ) << tile._fogColors << tile._metadata << tile._occupantHeroId
               << tile._isTileMarkedAsRoad << tile._addonBottomLayer << tile._addonTopLayer << tile._mainAddon._layerType << tile._boatOwnerColor;
}

StreamBase & Maps::operator>>( StreamBase & msg, Tiles & tile )
{
    msg >> tile._index >> tile._terrainImageIndex >> tile._terrainFlags >> tile._tilePassabilityDirections >> tile._mainAddon._uid;

    using ObjectIcnTypeUnderlyingType = std::underlying_type_t<decltype( tile._mainAddon._objectIcnType )>;
    static_assert( std::is_same_v<ObjectIcnTypeUnderlyingType, uint8_t>, "Type of _objectIcnType has been changed, check the logic below" );

    ObjectIcnTypeUnderlyingType objectIcnType = MP2::OBJ_ICN_TYPE_UNKNOWN;
    msg >> objectIcnType;

    tile._mainAddon._objectIcnType = static_cast<MP2::ObjectIcnType>( objectIcnType );

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE2_1009_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE2_1009_RELEASE ) {
        bool temp;
        msg >> temp >> temp;
    }

    msg >> tile._mainAddon._imageIndex;

    using MainObjectTypeUnderlyingType = std::underlying_type_t<decltype( tile._mainObjectType )>;
    static_assert( std::is_same_v<MainObjectTypeUnderlyingType, uint16_t>, "Type of _mainObjectType has been changed, check the logic below" );

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE3_1100_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE3_1100_RELEASE ) {
        uint8_t mainObjectType = static_cast<uint8_t>( MP2::OBJ_NONE );
        msg >> mainObjectType;

        tile._mainObjectType = static_cast<MP2::MapObjectType>( mainObjectType );
    }
    else {
        MainObjectTypeUnderlyingType mainObjectType = MP2::OBJ_NONE;
        msg >> mainObjectType;

        tile._mainObjectType = static_cast<MP2::MapObjectType>( mainObjectType );
    }

    return msg >> tile._fogColors >> tile._metadata >> tile._occupantHeroId >> tile._isTileMarkedAsRoad >> tile._addonBottomLayer >> tile._addonTopLayer
           >> tile._mainAddon._layerType >> tile._boatOwnerColor;
}
