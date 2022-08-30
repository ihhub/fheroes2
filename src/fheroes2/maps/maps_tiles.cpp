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

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>

#include "agg_image.h"
#include "castle.h"
#include "game.h"
#include "ground.h"
#include "heroes.h"
#include "icn.h"
#ifdef WITH_DEBUG
#include "game_interface.h"
#else
#include "interface_gamearea.h"
#endif
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "monster.h"
#include "monster_anim.h"
#include "mounts.h"
#include "mp2.h"
#include "objcrck.h"
#include "objdirt.h"
#include "objdsrt.h"
#include "objgras.h"
#include "objlava.h"
#include "objmult.h"
#include "objsnow.h"
#include "objswmp.h"
#include "objtown.h"
#include "objwatr.h"
#include "objxloc.h"
#include "race.h"
#include "save_format_version.h"
#include "serialize.h"
#include "settings.h"
#include "spell.h"
#include "til.h"
#include "trees.h"
#include "world.h"

namespace
{
    bool isValidShadowSprite( const int icn, const uint8_t icnIndex )
    {
        if ( icn == 0 ) {
            // Special case when no objects exist.
            return false;
        }

        switch ( icn ) {
        case ICN::MTNDSRT:
        case ICN::MTNGRAS:
        case ICN::MTNLAVA:
        case ICN::MTNMULT:
        case ICN::MTNSNOW:
        case ICN::MTNSWMP:
            return ObjMnts1::isShadow( icnIndex );
        case ICN::MTNCRCK:
        case ICN::MTNDIRT:
            return ObjMnts2::isShadow( icnIndex );
        case ICN::TREDECI:
        case ICN::TREEVIL:
        case ICN::TREFALL:
        case ICN::TREFIR:
        case ICN::TREJNGL:
        case ICN::TRESNOW:
            return ObjTree::isShadow( icnIndex );
        case ICN::OBJNCRCK:
            return ObjCrck::isShadow( icnIndex );
        case ICN::OBJNDIRT:
            return ObjDirt::isShadow( icnIndex );
        case ICN::OBJNDSRT:
            return ObjDsrt::isShadow( icnIndex );
        case ICN::OBJNGRA2:
            return ObjGra2::isShadow( icnIndex );
        case ICN::OBJNGRAS:
            return ObjGras::isShadow( icnIndex );
        case ICN::OBJNMUL2:
            return ObjMul2::isShadow( icnIndex );
        case ICN::OBJNMULT:
            return ObjMult::isShadow( icnIndex );
        case ICN::OBJNSNOW:
            return ObjSnow::isShadow( icnIndex );
        case ICN::OBJNSWMP:
            return ObjSwmp::isShadow( icnIndex );
        case ICN::OBJNWAT2:
            return ObjWat2::isShadow( icnIndex );
        case ICN::OBJNWATR:
            return ObjWatr::isShadow( icnIndex );
        case ICN::OBJNARTI:
        case ICN::OBJNRSRC:
            return 0 == ( icnIndex % 2 );
        case ICN::OBJNTWRD:
            return icnIndex > 31;
        case ICN::X_LOC1:
            return ObjXlc1::isShadow( icnIndex );
        case ICN::X_LOC2:
            return ObjXlc2::isShadow( icnIndex );
        case ICN::X_LOC3:
            return ObjXlc3::isShadow( icnIndex );
        case ICN::OBJNTOWN:
            return ObjTown::isShadow( icnIndex );
        case ICN::OBJNLAVA:
            return ObjLava::isShadow( icnIndex );
        case ICN::OBJNLAV2:
            return ObjLav2::isShadow( icnIndex );
        case ICN::OBJNLAV3:
            return ObjLav3::isShadow( icnIndex );
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
            break;
        }

        // Did you add a new type of objects into the game?
        assert( 0 );
        return false;
    }

    bool isValidReefsSprite( const int icn, const uint8_t icnIndex )
    {
        return icn == ICN::X_LOC2 && ObjXlc2::isReefs( icnIndex );
    }

#if defined( VERIFY_SHADOW_SPRITES )
    // Define VERIFY_SHADOW_SPRITES macro to be able to use these functions.
    bool isShadowImage( const fheroes2::Image & image )
    {
        // The image can't be empty.
        assert( !image.empty() );
        if ( image.empty() )
            return false;

        const uint8_t * data = image.transform();
        const uint8_t * dataEnd = data + image.width() * image.height();

        size_t transformCounter = 0;

        for ( ; data != dataEnd; ++data ) {
            if ( *data == 0 ) {
                return false;
            }
            else if ( *data != 1 ) {
                ++transformCounter;
            }
        }

        if ( transformCounter == 0 ) {
            assert( image.width() == 1 && image.height() == 1 );
            return true;
        }

        return true;
    }

    // Use this function to verify the correctness of data being returned by isValidShadowSprite function.
    void findAllShadowImages()
    {
        static bool completed = false;
        if ( completed ) {
            return;
        }

        const std::vector<int32_t> icnIds
            = { ICN::MTNDSRT,  ICN::MTNGRAS,  ICN::MTNLAVA,  ICN::MTNMULT,  ICN::MTNSNOW,  ICN::MTNSWMP,  ICN::MTNCRCK,  ICN::MTNDIRT,  ICN::TREDECI,
                ICN::TREEVIL,  ICN::TREFALL,  ICN::TREFIR,   ICN::TREJNGL,  ICN::TRESNOW,  ICN::OBJNCRCK, ICN::OBJNDIRT, ICN::OBJNDSRT, ICN::OBJNGRA2,
                ICN::OBJNGRAS, ICN::OBJNMUL2, ICN::OBJNMULT, ICN::OBJNSNOW, ICN::OBJNSWMP, ICN::OBJNWAT2, ICN::OBJNWATR, ICN::OBJNARTI, ICN::OBJNRSRC,
                ICN::OBJNTWRD, ICN::OBJNTWSH, ICN::STREAM,   ICN::OBJNTWBA, ICN::ROAD,     ICN::EXTRAOVR, ICN::X_LOC1,   ICN::X_LOC2,   ICN::X_LOC3,
                ICN::OBJNTOWN, ICN::OBJNLAVA, ICN::OBJNLAV2, ICN::OBJNLAV3, ICN::MONS32 };

        for ( const int32_t icnId : icnIds ) {
            const uint32_t maxIndex = fheroes2::AGG::GetICNCount( icnId );
            assert( maxIndex != 0 );

            std::string output;

            for ( uint32_t i = 0; i < maxIndex; i++ ) {
                const uint32_t startIndex = ICN::AnimationFrame( icnId, i, 0, true );
                const bool hasAnimation = startIndex != 0;
                bool isImageShadow = isShadowImage( fheroes2::AGG::GetICN( icnId, i ) );
                if ( isImageShadow && hasAnimation ) {
                    for ( uint32_t indexOffset = 1;; ++indexOffset ) {
                        const uint32_t animationIndex = ICN::AnimationFrame( icnId, i, indexOffset, true );
                        if ( startIndex == animationIndex ) {
                            break;
                        }

                        if ( !isShadowImage( fheroes2::AGG::GetICN( icnId, animationIndex ) ) ) {
                            isImageShadow = false;
                            break;
                        }
                    }
                }

                if ( isValidShadowSprite( icnId, i ) != isImageShadow ) {
                    output += std::to_string( i );
                    output += ", ";
                }
            }

            if ( output.empty() ) {
                continue;
            }

            VERBOSE_LOG( ICN::GetString( icnId ) << ": " << output )
        }

        completed = true;
    }
#endif

    bool contains( const int base, const int value )
    {
        return ( base & value ) == value;
    }

#ifdef WITH_DEBUG
    const fheroes2::Image & PassableViewSurface( const int passable )
    {
        static std::map<int, fheroes2::Image> imageMap;

        auto iter = imageMap.find( passable );
        if ( iter != imageMap.end() ) {
            return iter->second;
        }

        const int32_t size = 31;
        const uint8_t red = 0xBA;
        const uint8_t green = 0x5A;

        fheroes2::Image sf( size, size );
        sf.reset();

        if ( 0 == passable || Direction::CENTER == passable ) {
            fheroes2::DrawBorder( sf, red );
        }
        else if ( DIRECTION_ALL == passable ) {
            fheroes2::DrawBorder( sf, green );
        }
        else {
            const uint8_t topLeftColor = ( ( passable & Direction::TOP_LEFT ) != 0 ) ? green : red;
            const uint8_t bottomRightColor = ( ( passable & Direction::BOTTOM_RIGHT ) != 0 ) ? green : red;
            const uint8_t topRightColor = ( ( passable & Direction::TOP_RIGHT ) != 0 ) ? green : red;
            const uint8_t bottomLeftColor = ( ( passable & Direction::BOTTOM_LEFT ) != 0 ) ? green : red;
            const uint8_t topColor = ( ( passable & Direction::TOP ) != 0 ) ? green : red;
            const uint8_t bottomColor = ( ( passable & Direction::BOTTOM ) != 0 ) ? green : red;
            const uint8_t leftColor = ( ( passable & Direction::LEFT ) != 0 ) ? green : red;
            const uint8_t rightColor = ( ( passable & Direction::RIGHT ) != 0 ) ? green : red;

            uint8_t * image = sf.image();
            uint8_t * transform = sf.transform();

            // Horizontal
            for ( int32_t i = 0; i < 10; ++i ) {
                *( image + i ) = topLeftColor;
                *( transform + i ) = 0;

                *( image + i + ( size - 1 ) * size ) = bottomLeftColor;
                *( transform + i + ( size - 1 ) * size ) = 0;
            }

            for ( int32_t i = 10; i < 21; ++i ) {
                *( image + i ) = topColor;
                *( transform + i ) = 0;

                *( image + i + ( size - 1 ) * size ) = bottomColor;
                *( transform + i + ( size - 1 ) * size ) = 0;
            }

            for ( int32_t i = 21; i < size; ++i ) {
                *( image + i ) = topRightColor;
                *( transform + i ) = 0;

                *( image + i + ( size - 1 ) * size ) = bottomRightColor;
                *( transform + i + ( size - 1 ) * size ) = 0;
            }

            // Vertical
            for ( int32_t i = 0; i < 10; ++i ) {
                *( image + i * size ) = topLeftColor;
                *( transform + i * size ) = 0;

                *( image + size - 1 + i * size ) = topRightColor;
                *( transform + size - 1 + i * size ) = 0;
            }

            for ( int32_t i = 10; i < 21; ++i ) {
                *( image + i * size ) = leftColor;
                *( transform + i * size ) = 0;

                *( image + size - 1 + i * size ) = rightColor;
                *( transform + size - 1 + i * size ) = 0;
            }

            for ( int32_t i = 21; i < size; ++i ) {
                *( image + i * size ) = bottomLeftColor;
                *( transform + i * size ) = 0;

                *( image + size - 1 + i * size ) = bottomRightColor;
                *( transform + size - 1 + i * size ) = 0;
            }
        }

        return imageMap.try_emplace( passable, std::move( sf ) ).first->second;
    }
#endif

    bool isShortObject( const MP2::MapObjectType objectType )
    {
        // Some objects allow middle moves even being attached to the bottom.
        // These object actually don't have any sprites on tiles above them within addon 2 level objects.
        // TODO: find a better way to do not hardcode values here.

        switch ( objectType ) {
        case MP2::OBJ_HALFLINGHOLE:
        case MP2::OBJN_HALFLINGHOLE:
        case MP2::OBJ_LEANTO:
        case MP2::OBJ_WATERLAKE:
        case MP2::OBJ_TARPIT:
        case MP2::OBJ_MERCENARYCAMP:
        case MP2::OBJN_MERCENARYCAMP:
        case MP2::OBJ_STANDINGSTONES:
        case MP2::OBJ_SHRINE1:
        case MP2::OBJ_SHRINE2:
        case MP2::OBJ_SHRINE3:
        case MP2::OBJ_MAGICGARDEN:
        case MP2::OBJ_RUINS:
        case MP2::OBJN_RUINS:
        case MP2::OBJ_SIGN:
        case MP2::OBJ_IDOL:
        case MP2::OBJ_STONELITHS:
        case MP2::OBJN_STONELITHS:
        case MP2::OBJ_WAGON:
        case MP2::OBJ_WAGONCAMP:
        case MP2::OBJN_WAGONCAMP:
        case MP2::OBJ_GOBLINHUT:
        case MP2::OBJ_FAERIERING:
        case MP2::OBJN_FAERIERING:
        case MP2::OBJ_BARRIER:
        case MP2::OBJ_MAGICWELL:
        case MP2::OBJ_NOTHINGSPECIAL:
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
        case MP2::OBJ_WAGONCAMP:
        case MP2::OBJ_FAERIERING:
        case MP2::OBJ_MINES:
        case MP2::OBJ_SAWMILL:
        case MP2::OBJ_WATERALTAR:
        case MP2::OBJ_AIRALTAR:
        case MP2::OBJ_FIREALTAR:
        case MP2::OBJ_EARTHALTAR:
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

    uint32_t PackTileSpriteIndex( uint32_t index, uint32_t shape ) /* index max: 0x3FFF, shape value: 0, 1, 2, 3 */
    {
        return ( shape << 14 ) | ( 0x3FFF & index );
    }

    bool isDirectRenderingRestricted( const int icnId )
    {
        switch ( icnId ) {
        case ICN::UNKNOWN:
        case ICN::MONS32:
        case ICN::BOAT32:
        case ICN::MINIHERO:
            // Either it is an invalid sprite or a sprite which needs to be divided into tiles in order to properly render it.
            return true;
        default:
            break;
        }

        return false;
    }

    Maps::Addons::const_iterator getAddonWithAnyFlag( const Maps::Addons & addons )
    {
        static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_0918_RELEASE, "Remove this function." );
        Maps::Addons::const_iterator foundAddon = addons.end();
        for ( auto iter = addons.begin(); iter != addons.end(); ++iter ) {
            if ( MP2::GetICNObject( iter->object ) == ICN::FLAG32 ) {
                if ( foundAddon == addons.end() ) {
                    foundAddon = iter;
                }
                else {
                    // The stack of addons contains more than one flag. We have no idea which flag belongs to the actual object.
                    return addons.end();
                }
            }
        }

        return foundAddon;
    }

    uint8_t getColorFromIndex( const int colorIndex )
    {
        static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_0918_RELEASE, "Remove this function." );
        switch ( colorIndex ) {
        case 0:
            return Color::BLUE;
        case 1:
            return Color::GREEN;
        case 2:
            return Color::RED;
        case 3:
            return Color::YELLOW;
        case 4:
            return Color::ORANGE;
        case 5:
            return Color::PURPLE;
        case 6:
            return Color::UNUSED;
        default:
            assert( 0 );
            break;
        }

        return Color::NONE;
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

        return "Uknown layer";
    }
}

Maps::TilesAddon::TilesAddon()
    : uniq( 0 )
    , level( 0 )
    , object( 0 )
    , index( 0 )
{}

Maps::TilesAddon::TilesAddon( const uint8_t lv, const uint32_t uid, const uint8_t obj, const uint32_t index_ )
    : uniq( uid )
    , level( lv )
    , object( obj )
    , index( index_ )
{}

std::string Maps::TilesAddon::String( int lvl ) const
{
    std::ostringstream os;
    os << "--------- Level " << lvl << " --------" << std::endl
       << "UID             : " << uniq << std::endl
       << "tileset         : " << static_cast<int>( object ) << " (" << ICN::GetString( MP2::GetICNObject( object ) ) << ")" << std::endl
       << "index           : " << static_cast<int>( index ) << std::endl
       << "level           : " << static_cast<int>( level ) << " (" << static_cast<int>( level % 4 ) << ")"
       << " - " << getObjectLayerName( level % 4 ) << std::endl
       << "shadow          : " << ( isShadow( *this ) ? "true" : "false" ) << std::endl;
    return os.str();
}

bool Maps::TilesAddon::PredicateSortRules1( const Maps::TilesAddon & ta1, const Maps::TilesAddon & ta2 )
{
    return ( ( ta1.level % 4 ) > ( ta2.level % 4 ) );
}

MP2::MapObjectType Maps::Tiles::GetLoyaltyObject( const uint8_t tileset, const uint8_t icnIndex )
{
    switch ( MP2::GetICNObject( tileset ) ) {
    case ICN::X_LOC1:
        if ( icnIndex == 3 )
            return MP2::OBJ_ALCHEMYTOWER;
        else if ( icnIndex < 3 )
            return MP2::OBJN_ALCHEMYTOWER;
        else if ( 70 == icnIndex )
            return MP2::OBJ_ARENA;
        else if ( 3 < icnIndex && icnIndex < 72 )
            return MP2::OBJN_ARENA;
        else if ( 77 == icnIndex )
            return MP2::OBJ_BARROWMOUNDS;
        else if ( 71 < icnIndex && icnIndex < 78 )
            return MP2::OBJN_BARROWMOUNDS;
        else if ( 94 == icnIndex )
            return MP2::OBJ_EARTHALTAR;
        else if ( 77 < icnIndex && icnIndex < 112 )
            return MP2::OBJN_EARTHALTAR;
        else if ( 118 == icnIndex )
            return MP2::OBJ_AIRALTAR;
        else if ( 111 < icnIndex && icnIndex < 120 )
            return MP2::OBJN_AIRALTAR;
        else if ( 127 == icnIndex )
            return MP2::OBJ_FIREALTAR;
        else if ( 119 < icnIndex && icnIndex < 129 )
            return MP2::OBJN_FIREALTAR;
        else if ( 135 == icnIndex )
            return MP2::OBJ_WATERALTAR;
        else if ( 128 < icnIndex && icnIndex < 137 )
            return MP2::OBJN_WATERALTAR;
        break;

    case ICN::X_LOC2:
        if ( icnIndex == 4 )
            return MP2::OBJ_STABLES;
        else if ( icnIndex < 4 )
            return MP2::OBJN_STABLES;
        else if ( icnIndex == 9 )
            return MP2::OBJ_JAIL;
        else if ( 4 < icnIndex && icnIndex < 10 )
            return MP2::OBJN_JAIL;
        else if ( icnIndex == 37 )
            return MP2::OBJ_MERMAID;
        else if ( 9 < icnIndex && icnIndex < 47 )
            return MP2::OBJN_MERMAID;
        else if ( icnIndex == 101 )
            return MP2::OBJ_SIRENS;
        else if ( 46 < icnIndex && icnIndex < 111 )
            return MP2::OBJN_SIRENS;
        else if ( ObjXlc2::isReefs( icnIndex ) )
            return MP2::OBJ_REEFS;
        break;

    case ICN::X_LOC3:
        if ( icnIndex == 30 )
            return MP2::OBJ_HUTMAGI;
        else if ( icnIndex < 32 )
            return MP2::OBJN_HUTMAGI;
        else if ( icnIndex == 50 )
            return MP2::OBJ_EYEMAGI;
        else if ( 31 < icnIndex && icnIndex < 59 )
            return MP2::OBJN_EYEMAGI;
        break;

    default:
        break;
    }

    return MP2::OBJ_ZERO;
}

bool Maps::TilesAddon::isRoad() const
{
    switch ( MP2::GetICNObject( object ) ) {
    // road sprite
    case ICN::ROAD:
        if ( 1 == index || 8 == index || 10 == index || 11 == index || 15 == index || 22 == index || 23 == index || 24 == index || 25 == index || 27 == index )
            return false;
        else
            return true;

    // castle or town gate
    case ICN::OBJNTOWN:
        if ( 13 == index || 29 == index || 45 == index || 61 == index || 77 == index || 93 == index || 109 == index || 125 == index || 141 == index || 157 == index
             || 173 == index || 189 == index )
            return true;
        break;

    // Random castle or town gate.
    case ICN::OBJNTWRD:
        return ( index == 13 || index == 29 );

    default:
        break;
    }

    return false;
}

bool Maps::TilesAddon::isResource( const TilesAddon & ta )
{
    // OBJNRSRC
    return ICN::OBJNRSRC == MP2::GetICNObject( ta.object ) && ( ta.index % 2 );
}

bool Maps::TilesAddon::isArtifact( const TilesAddon & ta )
{
    // OBJNARTI (skip ultimate)
    return ( ICN::OBJNARTI == MP2::GetICNObject( ta.object ) && ( ta.index > 0x10 ) && ( ta.index % 2 ) );
}

int Maps::Tiles::ColorFromBarrierSprite( const uint8_t tileset, const uint8_t icnIndex )
{
    // 60, 66, 72, 78, 84, 90, 96, 102
    return ICN::X_LOC3 == MP2::GetICNObject( tileset ) && 60 <= icnIndex && 102 >= icnIndex ? ( ( icnIndex - 60 ) / 6 ) + 1 : 0;
}

int Maps::Tiles::ColorFromTravellerTentSprite( const uint8_t tileset, const uint8_t icnIndex )
{
    // 110, 114, 118, 122, 126, 130, 134, 138
    return ICN::X_LOC3 == MP2::GetICNObject( tileset ) && 110 <= icnIndex && 138 >= icnIndex ? ( ( icnIndex - 110 ) / 4 ) + 1 : 0;
}

bool Maps::TilesAddon::isShadow( const TilesAddon & ta )
{
    return Tiles::isShadowSprite( ta.object, ta.index );
}

bool Maps::Tiles::isShadowSprite( const int icn, const uint8_t icnIndex )
{
    return isValidShadowSprite( icn, icnIndex );
}

bool Maps::Tiles::isShadowSprite( const uint8_t tileset, const uint8_t icnIndex )
{
    return isShadowSprite( MP2::GetICNObject( tileset ), icnIndex );
}

void Maps::Tiles::UpdateAbandonedMineLeftSprite( uint8_t & tileset, uint8_t & index, const int resource )
{
    if ( ICN::OBJNGRAS == MP2::GetICNObject( tileset ) && 6 == index ) {
        tileset = 128; // MTNGRAS
        index = 82;
    }
    else if ( ICN::OBJNDIRT == MP2::GetICNObject( tileset ) && 8 == index ) {
        tileset = 104; // MTNDIRT
        index = 112;
    }
    else if ( ICN::EXTRAOVR == MP2::GetICNObject( tileset ) && 5 == index ) {
        switch ( resource ) {
        case Resource::ORE:
            index = 0;
            break;
        case Resource::SULFUR:
            index = 1;
            break;
        case Resource::CRYSTAL:
            index = 2;
            break;
        case Resource::GEMS:
            index = 3;
            break;
        case Resource::GOLD:
            index = 4;
            break;
        default:
            break;
        }
    }
}

void Maps::Tiles::UpdateAbandonedMineRightSprite( uint8_t & tileset, uint8_t & index )
{
    if ( ICN::OBJNDIRT == MP2::GetICNObject( tileset ) && index == 9 ) {
        tileset = 104;
        index = 113;
    }
    else if ( ICN::OBJNGRAS == MP2::GetICNObject( tileset ) && index == 7 ) {
        tileset = 128;
        index = 83;
    }
}

std::pair<int, int> Maps::Tiles::ColorRaceFromHeroSprite( const uint32_t heroSpriteIndex )
{
    std::pair<int, int> res;

    if ( 7 > heroSpriteIndex )
        res.first = Color::BLUE;
    else if ( 14 > heroSpriteIndex )
        res.first = Color::GREEN;
    else if ( 21 > heroSpriteIndex )
        res.first = Color::RED;
    else if ( 28 > heroSpriteIndex )
        res.first = Color::YELLOW;
    else if ( 35 > heroSpriteIndex )
        res.first = Color::ORANGE;
    else
        res.first = Color::PURPLE;

    switch ( heroSpriteIndex % 7 ) {
    case 0:
        res.second = Race::KNGT;
        break;
    case 1:
        res.second = Race::BARB;
        break;
    case 2:
        res.second = Race::SORC;
        break;
    case 3:
        res.second = Race::WRLK;
        break;
    case 4:
        res.second = Race::WZRD;
        break;
    case 5:
        res.second = Race::NECR;
        break;
    case 6:
        res.second = Race::RAND;
        break;
    default:
        assert( 0 );
        break;
    }

    return res;
}

void Maps::Tiles::Init( int32_t index, const MP2::mp2tile_t & mp2 )
{
    tilePassable = DIRECTION_ALL;

    _level = mp2.quantity1 & 0x03;
    quantity1 = mp2.quantity1;
    quantity2 = mp2.quantity2;
    additionalMetadata = 0;
    fog_colors = Color::ALL;

    SetTerrain( mp2.surfaceType, mp2.flags );
    SetIndex( index );
    SetObject( static_cast<MP2::MapObjectType>( mp2.mapObjectType ) );

    addons_level1.clear();
    addons_level2.clear();

    // those bitfields are set by map editor regardless if map object is there
    tileIsRoad = ( ( mp2.objectName1 >> 1 ) & 1 ) && ( MP2::GetICNObject( mp2.objectName1 ) == ICN::ROAD );

    // If an object has priority 2 (shadow) or 3 (ground) then we put it as an addon.
    if ( mp2.mapObjectType == MP2::OBJ_ZERO && ( _level >> 1 ) & 1 ) {
        AddonsPushLevel1( mp2 );
    }
    else {
        objectTileset = mp2.objectName1;
        objectIndex = mp2.level1IcnImageIndex;
        uniq = mp2.level1ObjectUID;
    }
    AddonsPushLevel2( mp2 );
}

Heroes * Maps::Tiles::GetHeroes() const
{
    return MP2::OBJ_HEROES == mp2_object && heroID ? world.GetHeroes( heroID - 1 ) : nullptr;
}

void Maps::Tiles::SetHeroes( Heroes * hero )
{
    if ( hero ) {
        hero->SetMapsObject( mp2_object );
        heroID = hero->GetID() + 1;
        SetObject( MP2::OBJ_HEROES );
    }
    else {
        hero = GetHeroes();

        if ( hero ) {
            SetObject( hero->GetMapsObject() );
            hero->SetMapsObject( MP2::OBJ_ZERO );
        }
        else {
            setAsEmpty();
        }

        heroID = 0;
    }
}

fheroes2::Point Maps::Tiles::GetCenter() const
{
    return Maps::GetPoint( _index );
}

MP2::MapObjectType Maps::Tiles::GetObject( bool ignoreObjectUnderHero /* true */ ) const
{
    if ( !ignoreObjectUnderHero && MP2::OBJ_HEROES == mp2_object ) {
        const Heroes * hero = GetHeroes();
        return hero ? hero->GetMapsObject() : MP2::OBJ_ZERO;
    }

    return mp2_object;
}

void Maps::Tiles::SetObject( const MP2::MapObjectType objectType )
{
    mp2_object = objectType;
    world.resetPathfinder();
}

void Maps::Tiles::setBoat( int direction )
{
    if ( objectTileset != 0 && objectIndex != 255 ) {
        AddonsPushLevel1( TilesAddon( OBJECT_LAYER, uniq, objectTileset, objectIndex ) );
    }
    SetObject( MP2::OBJ_BOAT );
    objectTileset = ICN::BOAT32;

    // Left-side sprites have to flipped, add 128 to index
    switch ( direction ) {
    case Direction::TOP:
        objectIndex = 0;
        break;
    case Direction::TOP_RIGHT:
        objectIndex = 9;
        break;
    case Direction::RIGHT:
        objectIndex = 18;
        break;
    case Direction::BOTTOM_RIGHT:
        objectIndex = 27;
        break;
    case Direction::BOTTOM:
        objectIndex = 36;
        break;
    case Direction::BOTTOM_LEFT:
        objectIndex = 27 + 128;
        break;
    case Direction::LEFT:
        objectIndex = 18 + 128;
        break;
    case Direction::TOP_LEFT:
        objectIndex = 9 + 128;
        break;
    default:
        objectIndex = 18;
        break;
    }

    uniq = World::GetUniq();
}

int Maps::Tiles::getBoatDirection() const
{
    // Check if it really is a boat
    if ( objectTileset != ICN::BOAT32 )
        return Direction::UNKNOWN;

    // Left-side sprites have to flipped, add 128 to index
    switch ( objectIndex ) {
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

void Maps::Tiles::SetTerrain( uint32_t sprite_index, uint32_t shape )
{
    pack_sprite_index = PackTileSpriteIndex( sprite_index, shape );
}

const fheroes2::Image & Maps::Tiles::GetTileSurface() const
{
    return fheroes2::AGG::GetTIL( TIL::GROUND32, TileSpriteIndex(), TileSpriteShape() );
}

int Maps::Tiles::getOriginalPassability() const
{
    const MP2::MapObjectType objectType = GetObject( false );

    if ( MP2::isActionObject( objectType ) ) {
        return MP2::getActionObjectDirection( objectType );
    }

    if ( ( objectTileset == 0 || objectIndex == 255 ) || ( ( _level >> 1 ) & 1 ) || isShadow() ) {
        // No object exists. Make it fully passable.
        return DIRECTION_ALL;
    }

    if ( isValidReefsSprite( MP2::GetICNObject( objectTileset ), objectIndex ) ) {
        return 0;
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( isValidReefsSprite( MP2::GetICNObject( addon.object ), addon.index ) ) {
            return 0;
        }
    }

    // Objects have fixed passability.
    return DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW;
}

void Maps::Tiles::setInitialPassability()
{
    tilePassable = getOriginalPassability();
}

void Maps::Tiles::updatePassability()
{
    if ( !Maps::isValidDirection( _index, Direction::LEFT ) ) {
        tilePassable &= ~( Direction::LEFT | Direction::TOP_LEFT | Direction::BOTTOM_LEFT );
    }
    if ( !Maps::isValidDirection( _index, Direction::RIGHT ) ) {
        tilePassable &= ~( Direction::RIGHT | Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT );
    }
    if ( !Maps::isValidDirection( _index, Direction::TOP ) ) {
        tilePassable &= ~( Direction::TOP | Direction::TOP_LEFT | Direction::TOP_RIGHT );
    }
    if ( !Maps::isValidDirection( _index, Direction::BOTTOM ) ) {
        tilePassable &= ~( Direction::BOTTOM | Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT );
    }

    const MP2::MapObjectType objectType = GetObject( false );
    const bool isActionObject = MP2::isActionObject( objectType );
    if ( !isActionObject && objectTileset > 0 && objectIndex < 255 && ( ( _level >> 1 ) & 1 ) == 0 && !isShadow() ) {
        // This is a non-action object.
        if ( Maps::isValidDirection( _index, Direction::BOTTOM ) ) {
            const Tiles & bottomTile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::BOTTOM ) );

            // If a bottom tile has the same object ID then this tile is inaccessible.
            std::vector<uint32_t> tileUIDs;
            if ( objectTileset > 0 && objectIndex < 255 && uniq != 0 && ( ( _level >> 1 ) & 1 ) == 0 ) {
                tileUIDs.emplace_back( uniq );
            }

            for ( const TilesAddon & addon : addons_level1 ) {
                if ( addon.uniq != 0 && ( ( addon.level >> 1 ) & 1 ) == 0 ) {
                    tileUIDs.emplace_back( addon.uniq );
                }
            }

            for ( const uint32_t objectId : tileUIDs ) {
                if ( bottomTile.doesObjectExist( objectId ) ) {
                    tilePassable = 0;
                    return;
                }
            }

            // If an object locates on land and the bottom tile is water mark the current tile as impassible. It's done for cases that a hero won't be able to
            // disembark on the tile.
            if ( !isWater() && bottomTile.isWater() ) {
                tilePassable = 0;
                return;
            }

            // Count how many objects are there excluding shadows, roads and river streams.
            const std::ptrdiff_t validLevel1ObjectCount = std::count_if( addons_level1.begin(), addons_level1.end(), []( const TilesAddon & addon ) {
                if ( TilesAddon::isShadow( addon ) ) {
                    return false;
                }

                const int icnType = MP2::GetICNObject( addon.object );
                return icnType != ICN::ROAD && icnType != ICN::STREAM;
            } );

            const bool singleObjectTile = validLevel1ObjectCount == 0 && addons_level2.empty() && ( bottomTile.objectTileset >> 2 ) != ( objectTileset >> 2 );
            const bool isBottomTileObject = ( ( bottomTile._level >> 1 ) & 1 ) == 0;

            // TODO: we might need to simplify the logic below as singleObjectTile might cover most of it.
            if ( !singleObjectTile && !isDetachedObject() && isBottomTileObject && bottomTile.objectTileset > 0 && bottomTile.objectIndex < 255 ) {
                const MP2::MapObjectType bottomTileObjectType = bottomTile.GetObject( false );
                const bool isBottomTileActionObject = MP2::isActionObject( bottomTileObjectType );
                const MP2::MapObjectType correctedObjectType = MP2::getBaseActionObjectType( bottomTileObjectType );

                if ( isBottomTileActionObject ) {
                    if ( ( MP2::getActionObjectDirection( bottomTileObjectType ) & Direction::TOP ) == 0 ) {
                        if ( isShortObject( bottomTileObjectType ) ) {
                            tilePassable &= ~Direction::BOTTOM;
                        }
                        else {
                            tilePassable = 0;
                            return;
                        }
                    }
                }
                else if ( bottomTile.mp2_object != MP2::OBJ_ZERO && correctedObjectType != bottomTileObjectType && MP2::isActionObject( correctedObjectType )
                          && isShortObject( correctedObjectType ) && ( bottomTile.getOriginalPassability() & Direction::TOP ) == 0 ) {
                    tilePassable &= ~Direction::BOTTOM;
                }
                else if ( isShortObject( bottomTileObjectType )
                          || ( !bottomTile.containsTileSet( getValidTileSets() ) && ( isCombinedObject( objectType ) || isCombinedObject( bottomTileObjectType ) ) ) ) {
                    tilePassable &= ~Direction::BOTTOM;
                }
                else {
                    tilePassable = 0;
                    return;
                }
            }
        }
        else {
            tilePassable = 0;
            return;
        }
    }

    // Left side.
    if ( ( tilePassable & Direction::TOP_LEFT ) && Maps::isValidDirection( _index, Direction::LEFT ) ) {
        const Tiles & leftTile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) );
        const bool leftTileTallObject = leftTile.isTallObject();
        if ( leftTileTallObject && ( leftTile.getOriginalPassability() & Direction::TOP ) == 0 ) {
            tilePassable &= ~Direction::TOP_LEFT;
        }
    }

    // Right side.
    if ( ( tilePassable & Direction::TOP_RIGHT ) && Maps::isValidDirection( _index, Direction::RIGHT ) ) {
        const Tiles & rightTile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::RIGHT ) );
        const bool rightTileTallObject = rightTile.isTallObject();
        if ( rightTileTallObject && ( rightTile.getOriginalPassability() & Direction::TOP ) == 0 ) {
            tilePassable &= ~Direction::TOP_RIGHT;
        }
    }
}

bool Maps::Tiles::doesObjectExist( const uint32_t uid ) const
{
    if ( uniq == uid && ( ( _level >> 1 ) & 1 ) == 0 ) {
        return true;
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( addon.uniq == uid && ( ( addon.level >> 1 ) & 1 ) == 0 ) {
            return true;
        }
    }

    return false;
}

void Maps::Tiles::UpdateRegion( uint32_t newRegionID )
{
    if ( tilePassable ) {
        _region = newRegionID;
    }
    else {
        _region = REGION_NODE_BLOCKED;
    }
}

bool Maps::Tiles::isClearGround() const
{
    const MP2::MapObjectType objectType = GetObject( true );

    switch ( objectType ) {
    case MP2::OBJ_ZERO:
    case MP2::OBJ_COAST:
        return true;
    case MP2::OBJ_BOAT:
        return false;

    default:
        break;
    }

    if ( objectTileset == 0 || objectIndex == 255 || ( ( _level >> 1 ) & 1 ) == 1 ) {
        if ( MP2::isActionObject( objectType, isWater() ) ) {
            return false;
        }
        // No objects are here.
        return true;
    }

    return false;
}

void Maps::Tiles::AddonsPushLevel1( const MP2::mp2tile_t & mt )
{
    if ( mt.objectName1 != 0 && mt.level1IcnImageIndex != 0xFF ) {
        addons_level1.emplace_back( mt.quantity1, mt.level1ObjectUID, mt.objectName1, mt.level1IcnImageIndex );
    }

    // MP2 "objectName" is a bitfield
    // 6 bits is ICN tileset id, 1 bit is isRoad flag, 1 bit is hasAnimation flag
    if ( ( ( mt.objectName1 >> 1 ) & 1 ) && ( MP2::GetICNObject( mt.objectName1 ) == ICN::ROAD ) )
        tileIsRoad = true;
}

void Maps::Tiles::AddonsPushLevel1( const MP2::mp2addon_t & ma )
{
    if ( ma.objectNameN1 != 0 && ma.indexNameN1 != 0xFF ) {
        addons_level1.emplace_back( ma.quantityN, ma.level1ObjectUID, ma.objectNameN1, ma.indexNameN1 );
    }
}

void Maps::Tiles::AddonsPushLevel1( const TilesAddon & ta )
{
    addons_level1.emplace_back( ta );
}

void Maps::Tiles::AddonsPushLevel2( const MP2::mp2tile_t & mt )
{
    if ( mt.objectName2 != 0 && mt.level2IcnImageIndex != 0xFF ) {
        // TODO: does level 2 even need level value? Verify it.
        addons_level2.emplace_back( mt.quantity1, mt.level2ObjectUID, mt.objectName2, mt.level2IcnImageIndex );
    }
}

void Maps::Tiles::AddonsPushLevel2( const MP2::mp2addon_t & ma )
{
    if ( ma.objectNameN2 != 0 && ma.indexNameN2 != 0xFF ) {
        // TODO: why do we use the same quantityN member for both level 1 and 2?
        addons_level2.emplace_back( ma.quantityN, ma.level2ObjectUID, ma.objectNameN2, ma.indexNameN2 );
    }
}

void Maps::Tiles::AddonsSort()
{
    // Push everything to the container and sort it by level.
    if ( objectTileset != 0 && objectIndex < 255 ) {
        addons_level1.emplace_front( _level, uniq, objectTileset, objectIndex );
    }

    // Some original maps have issues with identifying tiles as roads. This code fixes it. It's not an ideal solution but works fine in most of cases.
    if ( !tileIsRoad ) {
        for ( const TilesAddon & addon : addons_level1 ) {
            if ( addon.isRoad() ) {
                tileIsRoad = true;
                break;
            }
        }
    }

    addons_level1.sort( TilesAddon::PredicateSortRules1 );

    if ( !addons_level1.empty() ) {
        const TilesAddon & highestPriorityAddon = addons_level1.back();
        uniq = highestPriorityAddon.uniq;
        objectTileset = highestPriorityAddon.object;
        objectIndex = highestPriorityAddon.index;
        _level = highestPriorityAddon.level & 0x03;

        addons_level1.pop_back();
    }

    // Level 2 objects don't have any rendering priorities so they should be rendered first in queue first to render.
}

int Maps::Tiles::GetGround() const
{
    const uint32_t index = TileSpriteIndex();

    // list grounds from GROUND32.TIL
    if ( 30 > index )
        return Maps::Ground::WATER;
    else if ( 92 > index )
        return Maps::Ground::GRASS;
    else if ( 146 > index )
        return Maps::Ground::SNOW;
    else if ( 208 > index )
        return Maps::Ground::SWAMP;
    else if ( 262 > index )
        return Maps::Ground::LAVA;
    else if ( 321 > index )
        return Maps::Ground::DESERT;
    else if ( 361 > index )
        return Maps::Ground::DIRT;
    else if ( 415 > index )
        return Maps::Ground::WASTELAND;

    return Maps::Ground::BEACH;
}

void Maps::Tiles::RedrawEmptyTile( fheroes2::Image & dst, const fheroes2::Point & mp, const Interface::GameArea & area )
{
    if ( mp.y == -1 && mp.x >= 0 && mp.x < world.w() ) { // top first row
        area.DrawTile( dst, fheroes2::AGG::GetTIL( TIL::STON, 20 + ( mp.x % 4 ), 0 ), mp );
    }
    else if ( mp.x == world.w() && mp.y >= 0 && mp.y < world.h() ) { // right first row
        area.DrawTile( dst, fheroes2::AGG::GetTIL( TIL::STON, 24 + ( mp.y % 4 ), 0 ), mp );
    }
    else if ( mp.y == world.h() && mp.x >= 0 && mp.x < world.w() ) { // bottom first row
        area.DrawTile( dst, fheroes2::AGG::GetTIL( TIL::STON, 28 + ( mp.x % 4 ), 0 ), mp );
    }
    else if ( mp.x == -1 && mp.y >= 0 && mp.y < world.h() ) { // left first row
        area.DrawTile( dst, fheroes2::AGG::GetTIL( TIL::STON, 32 + ( mp.y % 4 ), 0 ), mp );
    }
    else {
        area.DrawTile( dst, fheroes2::AGG::GetTIL( TIL::STON, ( std::abs( mp.y ) % 4 ) * 4 + std::abs( mp.x ) % 4, 0 ), mp );
    }
}

void Maps::Tiles::RedrawPassable( fheroes2::Image & dst, const Interface::GameArea & area ) const
{
#ifdef WITH_DEBUG
    if ( 0 == tilePassable || DIRECTION_ALL != tilePassable ) {
        area.BlitOnTile( dst, PassableViewSurface( tilePassable ), 0, 0, Maps::GetPoint( _index ), false, 255 );
    }
#else
    (void)dst;
    (void)area;
#endif
}

void Maps::Tiles::redrawBottomLayerObjects( fheroes2::Image & dst, bool isPuzzleDraw, const Interface::GameArea & area, const uint8_t level ) const
{
    assert( level <= 0x03 );

    const fheroes2::Point & mp = Maps::GetPoint( _index );

    // Since the original game stores information about objects in a very weird way and this is how it is implemented for us we need to do the following procedure:
    // - run through all bottom objects first which are stored in the addon stack
    // - check the main object which is on the tile

    // Some addons must be rendered after the main object on the tile. This applies for flags.
    std::vector<const TilesAddon *> postRenderingAddon;

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( ( addon.level & 0x03 ) != level ) {
            continue;
        }

        if ( isPuzzleDraw && MP2::isHiddenForPuzzle( GetGround(), addon.object, addon.index ) ) {
            continue;
        }

        const int icn = MP2::GetICNObject( addon.object );
        if ( icn == ICN::FLAG32 ) {
            postRenderingAddon.emplace_back( &addon );
            continue;
        }

        renderAddonObject( dst, area, mp, addon );
    }

    if ( objectTileset != 0 && ( _level & 0x03 ) == level && ( !isPuzzleDraw || !MP2::isHiddenForPuzzle( GetGround(), objectTileset, objectIndex ) ) ) {
        renderMainObject( dst, area, mp );
    }

    for ( const TilesAddon * addon : postRenderingAddon ) {
        assert( addon != nullptr );

        renderAddonObject( dst, area, mp, *addon );
    }
}

void Maps::Tiles::renderAddonObject( fheroes2::Image & output, const Interface::GameArea & area, const fheroes2::Point & offset, const TilesAddon & addon )
{
    assert( addon.object != 0 && addon.index != 255 );

    const int icn = MP2::GetICNObject( addon.object );
    if ( isDirectRenderingRestricted( icn ) ) {
        return;
    }

    const uint8_t alphaValue = area.getObjectAlphaValue( addon.uniq );

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, addon.index );

    // Ideally we need to check that the image is within a tile area. However, flags are among those for which this rule doesn't apply.
    if ( icn == ICN::FLAG32 ) {
        assert( sprite.width() <= TILEWIDTH && sprite.height() <= TILEWIDTH );
    }
    else {
        assert( sprite.x() >= 0 && sprite.width() + sprite.x() <= TILEWIDTH && sprite.y() >= 0 && sprite.height() + sprite.y() <= TILEWIDTH );
    }

    area.BlitOnTile( output, sprite, sprite.x(), sprite.y(), offset, false, alphaValue );

    const uint32_t animationIndex = ICN::AnimationFrame( icn, addon.index, Game::MapsAnimationFrame() );
    if ( animationIndex > 0 ) {
        const fheroes2::Sprite & animationSprite = fheroes2::AGG::GetICN( icn, animationIndex );

        // If this assertion blows up we are trying to render an image bigger than a tile. Render this object properly as heroes or monsters!
        assert( animationSprite.x() >= 0 && animationSprite.width() + animationSprite.x() <= TILEWIDTH && animationSprite.y() >= 0
                && animationSprite.height() + animationSprite.y() <= TILEWIDTH );

        area.BlitOnTile( output, animationSprite, animationSprite.x(), animationSprite.y(), offset, false, alphaValue );
    }
}

void Maps::Tiles::renderMainObject( fheroes2::Image & output, const Interface::GameArea & area, const fheroes2::Point & offset ) const
{
    assert( objectTileset != 0 && objectIndex != 255 );

    const int mainObjectIcn = MP2::GetICNObject( objectTileset );
    if ( isDirectRenderingRestricted( mainObjectIcn ) ) {
        return;
    }

    const uint8_t mainObjectAlphaValue = area.getObjectAlphaValue( uniq );

    const fheroes2::Sprite & mainObjectSprite = fheroes2::AGG::GetICN( mainObjectIcn, objectIndex );

    // If this assertion blows up we are trying to render an image bigger than a tile. Render this object properly as heroes or monsters!
    assert( mainObjectSprite.x() >= 0 && mainObjectSprite.width() + mainObjectSprite.x() <= TILEWIDTH && mainObjectSprite.y() >= 0
            && mainObjectSprite.height() + mainObjectSprite.y() <= TILEWIDTH );

    area.BlitOnTile( output, mainObjectSprite, mainObjectSprite.x(), mainObjectSprite.y(), offset, false, mainObjectAlphaValue );

    // Render possible animation image.
    // TODO: quantity2 is used in absolutely incorrect way! Fix all the logic for it. As of now (quantity2 != 0) expression is used only for Magic Garden.
    const uint32_t mainObjectAnimationIndex = ICN::AnimationFrame( mainObjectIcn, objectIndex, Game::MapsAnimationFrame(), quantity2 != 0 );
    if ( mainObjectAnimationIndex > 0 ) {
        const fheroes2::Sprite & animationSprite = fheroes2::AGG::GetICN( mainObjectIcn, mainObjectAnimationIndex );

        // If this assertion blows up we are trying to render an image bigger than a tile. Render this object properly as heroes or monsters!
        assert( animationSprite.x() >= 0 && animationSprite.width() + animationSprite.x() <= TILEWIDTH && animationSprite.y() >= 0
                && animationSprite.height() + animationSprite.y() <= TILEWIDTH );

        area.BlitOnTile( output, animationSprite, animationSprite.x(), animationSprite.y(), offset, false, mainObjectAlphaValue );
    }
}

void Maps::Tiles::drawByIcnId( fheroes2::Image & output, const Interface::GameArea & area, const int32_t icnId ) const
{
    const fheroes2::Point & tileOffset = Maps::GetPoint( _index );

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( MP2::GetICNObject( addon.object ) == icnId ) {
            renderAddonObject( output, area, tileOffset, addon );
        }
    }

    if ( MP2::GetICNObject( objectTileset ) == icnId ) {
        renderMainObject( output, area, tileOffset );
    }

    for ( const TilesAddon & addon : addons_level2 ) {
        if ( MP2::GetICNObject( addon.object ) == icnId ) {
            renderAddonObject( output, area, tileOffset, addon );
        }
    }
}

std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> Maps::Tiles::getMonsterSpritesPerTile() const
{
    assert( GetObject() == MP2::OBJ_MONSTER );

    std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> output;

    const Monster & monster = QuantityMonster();
    const std::pair<uint32_t, uint32_t> spriteIndicies = GetMonsterSpriteIndices( *this, monster.GetSpriteIndex() );

    const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( ICN::MINI_MONSTER_IMAGE, spriteIndicies.first );
    const fheroes2::Point monsterSpriteOffset( monsterSprite.x() + 16, monsterSprite.y() + 30 );

    fheroes2::DivideImageBySquares( monsterSpriteOffset, monsterSprite, TILEWIDTH, false, output );

    if ( spriteIndicies.second > 0 ) {
        const fheroes2::Sprite & secondaryMonsterSprite = fheroes2::AGG::GetICN( ICN::MINI_MONSTER_IMAGE, spriteIndicies.second );
        const fheroes2::Point secondaryMonsterSpriteOffset( secondaryMonsterSprite.x() + 16, secondaryMonsterSprite.y() + 30 );

        fheroes2::DivideImageBySquares( secondaryMonsterSpriteOffset, secondaryMonsterSprite, TILEWIDTH, false, output );
    }

    return output;
}

std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> Maps::Tiles::getMonsterShadowSpritesPerTile() const
{
    assert( GetObject() == MP2::OBJ_MONSTER );

    std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> output;

    const Monster & monster = QuantityMonster();
    const std::pair<uint32_t, uint32_t> spriteIndicies = GetMonsterSpriteIndices( *this, monster.GetSpriteIndex() );

    const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( ICN::MINI_MONSTER_SHADOW, spriteIndicies.first );
    const fheroes2::Point monsterSpriteOffset( monsterSprite.x() + 16, monsterSprite.y() + 30 );

    fheroes2::DivideImageBySquares( monsterSpriteOffset, monsterSprite, TILEWIDTH, false, output );

    if ( spriteIndicies.second > 0 ) {
        const fheroes2::Sprite & secondaryMonsterSprite = fheroes2::AGG::GetICN( ICN::MINI_MONSTER_SHADOW, spriteIndicies.second );
        const fheroes2::Point secondaryMonsterSpriteOffset( secondaryMonsterSprite.x() + 16, secondaryMonsterSprite.y() + 30 );

        fheroes2::DivideImageBySquares( secondaryMonsterSpriteOffset, secondaryMonsterSprite, TILEWIDTH, false, output );
    }

    return output;
}

std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> Maps::Tiles::getBoatSpritesPerTile() const
{
    // TODO: combine both boat image generation for heroes and empty boats.
    assert( GetObject() == MP2::OBJ_BOAT );

    const uint32_t spriteIndex = ( objectIndex == 255 ) ? 18 : objectIndex;

    const bool isReflected = ( spriteIndex > 128 );

    const fheroes2::Sprite & boatSprite = fheroes2::AGG::GetICN( ICN::BOAT32, spriteIndex % 128 );

    const fheroes2::Point boatSpriteOffset( ( isReflected ? ( TILEWIDTH + 1 - boatSprite.x() - boatSprite.width() ) : boatSprite.x() ), boatSprite.y() + TILEWIDTH - 11 );

    std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> output;
    fheroes2::DivideImageBySquares( boatSpriteOffset, boatSprite, TILEWIDTH, isReflected, output );

    return output;
}

std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> Maps::Tiles::getBoatShadowSpritesPerTile() const
{
    assert( GetObject() == MP2::OBJ_BOAT );

    std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> output;

    // TODO: boat shadow logic is more complex than this and it is not directly depend on spriteIndex. Find the proper logic and fix it!
    const uint32_t spriteIndex = ( objectIndex == 255 ) ? 18 : objectIndex;

    const fheroes2::Sprite & boatShadowSprite = fheroes2::AGG::GetICN( ICN::BOATSHAD, spriteIndex % 128 );
    const fheroes2::Point boatShadowSpriteOffset( boatShadowSprite.x(), TILEWIDTH + boatShadowSprite.y() - 11 );

    // Shadows cannot be flipped so flip flag is always false.
    fheroes2::DivideImageBySquares( boatShadowSpriteOffset, boatShadowSprite, TILEWIDTH, false, output );

    return output;
}

std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> Maps::Tiles::getMineGuardianSpritesPerTile() const
{
    assert( GetObject( false ) == MP2::OBJ_MINES );

    std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> output;

    const int32_t spellID = Maps::getSpellIdFromTile( *this );
    switch ( spellID ) {
    case Spell::SETEGUARDIAN:
    case Spell::SETAGUARDIAN:
    case Spell::SETFGUARDIAN:
    case Spell::SETWGUARDIAN: {
        static_assert( Spell::SETAGUARDIAN - Spell::SETEGUARDIAN == 1 && Spell::SETFGUARDIAN - Spell::SETEGUARDIAN == 2 && Spell::SETWGUARDIAN - Spell::SETEGUARDIAN == 3,
                       "Why are you changing the order of spells?! Be extremely careful of what you are doing" );
        const fheroes2::Sprite & image = fheroes2::AGG::GetICN( ICN::OBJNXTRA, spellID - Spell::SETEGUARDIAN );
        fheroes2::DivideImageBySquares( { image.x(), image.y() }, image, TILEWIDTH, false, output );
        break;
    }
    default:
        break;
    }

    return output;
}

void Maps::Tiles::redrawTopLayerExtraObjects( fheroes2::Image & dst, const bool isPuzzleDraw, const Interface::GameArea & area ) const
{
    if ( isPuzzleDraw ) {
        // Extra objects should not be shown on Puzzle Map as they are temporary objects appearing under specific conditions like flags.
        return;
    }

    // Ghost animation is unique and can be rendered in multiple cases.
    bool renderFlyingGhosts = false;

    const MP2::MapObjectType objectType = GetObject( false );
    if ( objectType == MP2::OBJ_ABANDONEDMINE ) {
        renderFlyingGhosts = true;
    }
    else if ( objectType == MP2::OBJ_MINES ) {
        const int32_t spellID = Maps::getSpellIdFromTile( *this );

        switch ( spellID ) {
        case Spell::NONE:
            // No spell exists. Nothing we need to render.
        case Spell::SETEGUARDIAN:
        case Spell::SETAGUARDIAN:
        case Spell::SETFGUARDIAN:
        case Spell::SETWGUARDIAN:
            // The logic for these spells is done while rending the bottom layer. Nothing should be done here.
            break;
        case Spell::HAUNT:
            renderFlyingGhosts = true;
            break;
        default:
            // Did you add a new spell for mines? Add the rendering for it above!
            assert( 0 );
            break;
        }
    }

    if ( renderFlyingGhosts ) {
        // This sprite is bigger than TILEWIDTH but rendering is correct for heroes and boats.
        // TODO: consider adding this sprite as a part of an addon.
        const fheroes2::Sprite & image = fheroes2::AGG::GetICN( ICN::OBJNHAUN, Game::MapsAnimationFrame() % 15 );

        const uint8_t alphaValue = area.getObjectAlphaValue( uniq );

        area.BlitOnTile( dst, image, image.x(), image.y(), Maps::GetPoint( _index ), false, alphaValue );
    }
}

void Maps::Tiles::redrawTopLayerObject( fheroes2::Image & dst, const bool isPuzzleDraw, const Interface::GameArea & area, const TilesAddon & addon ) const
{
    if ( isPuzzleDraw && MP2::isHiddenForPuzzle( GetGround(), addon.object, addon.index ) ) {
        return;
    }

    renderAddonObject( dst, area, Maps::GetPoint( _index ), addon );
}

Maps::TilesAddon * Maps::Tiles::FindAddonLevel1( uint32_t uniq1 )
{
    Addons::iterator it = std::find_if( addons_level1.begin(), addons_level1.end(), [uniq1]( const TilesAddon & v ) { return v.isUniq( uniq1 ); } );

    return it != addons_level1.end() ? &( *it ) : nullptr;
}

Maps::TilesAddon * Maps::Tiles::FindAddonLevel2( uint32_t uniq2 )
{
    Addons::iterator it = std::find_if( addons_level2.begin(), addons_level2.end(), [uniq2]( const TilesAddon & v ) { return v.isUniq( uniq2 ); } );

    return it != addons_level2.end() ? &( *it ) : nullptr;
}

std::string Maps::Tiles::String() const
{
    std::ostringstream os;

    const MP2::MapObjectType objectType = GetObject();

    os << "******* Tile info *******" << std::endl
       << "Tile index      : " << _index << ", "
       << "point: (" << GetCenter().x << ", " << GetCenter().y << ")" << std::endl
       << "UID             : " << uniq << std::endl
       << "MP2 object type : " << static_cast<int>( objectType ) << " (" << MP2::StringObject( objectType ) << ")" << std::endl
       << "tileset         : " << static_cast<int>( objectTileset ) << " (" << ICN::GetString( MP2::GetICNObject( objectTileset ) ) << ")" << std::endl
       << "object index    : " << static_cast<int>( objectIndex ) << " (animated: " << hasSpriteAnimation() << ")" << std::endl
       << "level           : " << static_cast<int>( _level ) << " - " << getObjectLayerName( _level ) << std::endl
       << "region          : " << _region << std::endl
       << "ground          : " << Ground::String( GetGround() ) << " (isRoad: " << tileIsRoad << ")" << std::endl
       << "shadow          : " << ( isShadowSprite( objectTileset, objectIndex ) ? "true" : "false" ) << std::endl
       << "passable from   : " << ( tilePassable ? Direction::String( tilePassable ) : "nowhere" );

    os << std::endl
       << "quantity 1      : " << static_cast<int>( quantity1 ) << std::endl
       << "quantity 2      : " << static_cast<int>( quantity2 ) << std::endl
       << "add. metadata   : " << additionalMetadata << std::endl;

    for ( const TilesAddon & addon : addons_level1 ) {
        os << addon.String( 1 );
    }

    for ( const TilesAddon & addon : addons_level2 ) {
        os << addon.String( 2 );
    }

    os << "--- Extra information ---" << std::endl;

    switch ( objectType ) {
    case MP2::OBJ_RUINS:
    case MP2::OBJ_TREECITY:
    case MP2::OBJ_WAGONCAMP:
    case MP2::OBJ_DESERTTENT:
    case MP2::OBJ_TROLLBRIDGE:
    case MP2::OBJ_DRAGONCITY:
    case MP2::OBJ_CITYDEAD:
    case MP2::OBJ_WATCHTOWER:
    case MP2::OBJ_EXCAVATION:
    case MP2::OBJ_CAVE:
    case MP2::OBJ_TREEHOUSE:
    case MP2::OBJ_ARCHERHOUSE:
    case MP2::OBJ_GOBLINHUT:
    case MP2::OBJ_DWARFCOTT:
    case MP2::OBJ_HALFLINGHOLE:
    case MP2::OBJ_PEASANTHUT:
    case MP2::OBJ_THATCHEDHUT:
    case MP2::OBJ_MONSTER:
        os << "monster count   : " << MonsterCount() << std::endl;
        break;
    case MP2::OBJ_HEROES: {
        const Heroes * hero = GetHeroes();
        if ( hero )
            os << hero->String();
        break;
    }
    case MP2::OBJN_CASTLE:
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

void Maps::Tiles::FixObject()
{
    if ( MP2::OBJ_ZERO == mp2_object ) {
        if ( std::any_of( addons_level1.begin(), addons_level1.end(), TilesAddon::isArtifact ) )
            SetObject( MP2::OBJ_ARTIFACT );
        else if ( std::any_of( addons_level1.begin(), addons_level1.end(), TilesAddon::isResource ) )
            SetObject( MP2::OBJ_RESOURCE );
    }
}

bool Maps::Tiles::GoodForUltimateArtifact() const
{
    if ( isWater() || !isPassableFrom( Direction::CENTER, false, true, 0 ) ) {
        return false;
    }

    if ( objectTileset != 0 && !isShadowSprite( objectTileset, objectIndex ) ) {
        return false;
    }

    if ( static_cast<size_t>( std::count_if( addons_level1.begin(), addons_level1.end(), TilesAddon::isShadow ) ) != addons_level1.size() ) {
        return false;
    }

    if ( static_cast<size_t>( std::count_if( addons_level2.begin(), addons_level2.end(), TilesAddon::isShadow ) ) != addons_level2.size() ) {
        return false;
    }

    return true;
}

bool Maps::Tiles::isPassableFrom( const int direction, const bool fromWater, const bool skipFog, const int heroColor ) const
{
    if ( !skipFog && isFog( heroColor ) ) {
        return false;
    }

    const bool tileIsWater = isWater();

    // From the water we can get either to the coast tile or to the water tile (provided there is no boat on this tile).
    if ( fromWater && mp2_object != MP2::OBJ_COAST && ( !tileIsWater || mp2_object == MP2::OBJ_BOAT ) ) {
        return false;
    }

    // From the ground we can get to the water tile only if this tile contains a certain object.
    if ( !fromWater && tileIsWater && mp2_object != MP2::OBJ_SHIPWRECK && mp2_object != MP2::OBJ_HEROES && mp2_object != MP2::OBJ_BOAT ) {
        return false;
    }

    return ( direction & tilePassable ) != 0;
}

void Maps::Tiles::SetObjectPassable( bool pass )
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_TROLLBRIDGE:
        if ( pass )
            tilePassable |= Direction::TOP_LEFT;
        else
            tilePassable &= ~Direction::TOP_LEFT;
        break;

    default:
        break;
    }
}

bool Maps::Tiles::isStream() const
{
    for ( auto it = addons_level1.begin(); it != addons_level1.end(); ++it ) {
        const int icn = MP2::GetICNObject( it->object );
        if ( icn == ICN::STREAM || ( icn == ICN::OBJNMUL2 && it->index < 14 ) )
            return true;
    }
    const int tileICN = MP2::GetICNObject( objectTileset );
    return tileICN == ICN::STREAM || ( tileICN == ICN::OBJNMUL2 && objectIndex < 14 );
}

bool Maps::Tiles::isShadow() const
{
    return isShadowSprite( objectTileset, objectIndex )
           && addons_level1.size() == static_cast<size_t>( std::count_if( addons_level1.begin(), addons_level1.end(), TilesAddon::isShadow ) );
}

Maps::TilesAddon * Maps::Tiles::getAddonWithFlag( const uint32_t uid )
{
    auto isFlag = [uid]( const TilesAddon & addon ) { return addon.uniq == uid && MP2::GetICNObject( addon.object ) == ICN::FLAG32; };

    auto iter = std::find_if( addons_level1.begin(), addons_level1.end(), isFlag );
    if ( iter != addons_level1.end() ) {
        return &( *iter );
    }

    iter = std::find_if( addons_level2.begin(), addons_level2.end(), isFlag );
    if ( iter != addons_level2.end() ) {
        return &( *iter );
    }

    return nullptr;
}

void Maps::Tiles::setOwnershipFlag( const MP2::MapObjectType objectType, const int color )
{
    // All flags in FLAG32.ICN are actually the same except the fact of having different offset.
    // 14, 21
    uint8_t objectSpriteIndex = 0;

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
        // Neutral / gray flag.
        objectSpriteIndex = 6;
        break;
    default:
        // Did you add a new color type? Add logic above!
        assert( 0 );
        break;
    }

    switch ( objectType ) {
    case MP2::OBJ_MAGICGARDEN:
        objectSpriteIndex += 128 + 14;
        updateFlag( color, objectSpriteIndex, uniq, false );
        objectSpriteIndex += 7;
        if ( Maps::isValidDirection( _index, Direction::RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::RIGHT ) );
            tile.updateFlag( color, objectSpriteIndex, uniq, false );
        }
        break;

    case MP2::OBJ_WATERWHEEL:
    case MP2::OBJ_MINES:
        objectSpriteIndex += 128 + 14;
        if ( Maps::isValidDirection( _index, Direction::TOP ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP ) );
            tile.updateFlag( color, objectSpriteIndex, uniq, true );
        }

        objectSpriteIndex += 7;
        if ( Maps::isValidDirection( _index, Direction::TOP_RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP_RIGHT ) );
            tile.updateFlag( color, objectSpriteIndex, uniq, true );
        }
        break;

    case MP2::OBJ_WINDMILL:
    case MP2::OBJ_LIGHTHOUSE:
        objectSpriteIndex += 128 + 42;
        if ( Maps::isValidDirection( _index, Direction::LEFT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) );
            tile.updateFlag( color, objectSpriteIndex, uniq, false );
        }

        objectSpriteIndex += 7;
        updateFlag( color, objectSpriteIndex, uniq, false );
        break;

    case MP2::OBJ_ALCHEMYLAB:
        objectSpriteIndex += 21;
        if ( Maps::isValidDirection( _index, Direction::TOP ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP ) );
            tile.updateFlag( color, objectSpriteIndex, uniq, true );
        }
        break;

    case MP2::OBJ_SAWMILL:
        objectSpriteIndex += 28;
        if ( Maps::isValidDirection( _index, Direction::TOP_RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP_RIGHT ) );
            tile.updateFlag( color, objectSpriteIndex, uniq, true );
        }
        break;

    case MP2::OBJ_CASTLE:
        objectSpriteIndex *= 2;
        if ( Maps::isValidDirection( _index, Direction::LEFT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) );
            tile.updateFlag( color, objectSpriteIndex, uniq, true );
        }

        objectSpriteIndex += 1;
        if ( Maps::isValidDirection( _index, Direction::RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::RIGHT ) );
            tile.updateFlag( color, objectSpriteIndex, uniq, true );
        }
        break;

    default:
        break;
    }
}

void Maps::Tiles::correctOldSaveOwnershipFlag()
{
    // Old saves prior 0.9.18 release contain flags in different incorrect places. This method attempts to fix it but there is no guarantee that it will always work.
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_0918_RELEASE, "Remove this method." );

    int ownerColor = Color::NONE;

    switch ( mp2_object ) {
    case MP2::OBJ_LIGHTHOUSE:
    case MP2::OBJ_WINDMILL:
    case MP2::OBJ_MAGICGARDEN: {
        if ( removeOldFlag( addons_level1, 42, ownerColor ) ) {
            setOwnershipFlag( mp2_object, ownerColor );
        }
        break;
    }

    case MP2::OBJ_WATERWHEEL: {
        if ( removeOldFlag( addons_level1, 14, ownerColor ) ) {
            setOwnershipFlag( mp2_object, ownerColor );
        }
        break;
    }
    case MP2::OBJ_MINES: {
        if ( removeOldFlag( addons_level2, 14, ownerColor ) ) {
            setOwnershipFlag( mp2_object, ownerColor );
        }
        break;
    }

    case MP2::OBJ_ALCHEMYLAB: {
        if ( Maps::isValidDirection( _index, Direction::TOP ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP ) );
            if ( removeOldFlag( tile.addons_level2, 21, ownerColor ) ) {
                setOwnershipFlag( mp2_object, ownerColor );
            }
        }
        break;
    }

    case MP2::OBJ_SAWMILL: {
        if ( Maps::isValidDirection( _index, Direction::TOP_RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP_RIGHT ) );
            if ( removeOldFlag( tile.addons_level2, 21, ownerColor ) ) {
                setOwnershipFlag( mp2_object, ownerColor );
            }
        }
        break;
    }

    default:
        // Castles do not need flag conversion as they come with flags from map format.
        break;
    }
}

bool Maps::Tiles::removeOldFlag( Addons & addons, const uint8_t startIndex, int & ownerColor )
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_0918_RELEASE, "Remove this method." );

    Maps::Addons::const_iterator foundAddon = getAddonWithAnyFlag( addons );
    if ( foundAddon == addons.end() ) {
        return false;
    }

    const int colorIndex = foundAddon->index - startIndex;
    if ( colorIndex >= 0 && colorIndex <= 6 ) {
        addons.erase( foundAddon );
        ownerColor = getColorFromIndex( colorIndex );
        return true;
    }

    return false;
}

void Maps::Tiles::correctDiggingHoles()
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_0918_RELEASE, "Remove this method." );
    uint8_t obj = 0;
    uint32_t idx = 0;
    switch ( GetGround() ) {
    case Maps::Ground::WASTELAND:
        obj = 0xE4; // ICN::OBJNCRCK
        idx = 70;
        break;
    case Maps::Ground::DIRT:
        obj = 0xE0; // ICN::OBJNDIRT
        idx = 140;
        break;
    case Maps::Ground::DESERT:
        obj = 0xDC; // ICN::OBJNDSRT
        idx = 68;
        break;
    case Maps::Ground::LAVA:
        obj = 0xD8; // ICN::OBJNLAVA
        idx = 26;
        break;
    default:
        // Previous implementation was using incorrect digging holes for my terrains. We aren't going to fix it for old saves.
        obj = 0xC0; // ICN::OBJNGRA2
        idx = 9;
        break;
    }

    for ( TilesAddon & addon : addons_level1 ) {
        if ( addon.object == obj && addon.index == idx ) {
            addon.level = BACKGROUND_LAYER;
        }
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
        auto isFlag = [uid]( const TilesAddon & addon ) { return addon.uniq == uid && MP2::GetICNObject( addon.object ) == ICN::FLAG32; };
        addons_level1.remove_if( isFlag );
        addons_level2.remove_if( isFlag );
        return;
    }

    const uint8_t objectType = 0x38;

    TilesAddon * addon = getAddonWithFlag( uid );
    if ( addon != nullptr ) {
        // Replace an existing flag.
        addon->index = objectSpriteIndex;
    }
    else if ( setOnUpperLayer ) {
        addons_level2.emplace_back( OBJECT_LAYER, uid, objectType, objectSpriteIndex );
    }
    else {
        addons_level1.emplace_back( OBJECT_LAYER, uid, objectType, objectSpriteIndex );
    }
}

void Maps::Tiles::fixTileObjectType( Tiles & tile )
{
    const MP2::MapObjectType originalObjectType = tile.GetObject( false );
    const int originalICN = MP2::GetICNObject( tile.objectTileset );

    // Left tile of a skeleton on Desert should be mark as non-action tile.
    if ( originalObjectType == MP2::OBJ_SKELETON && originalICN == ICN::OBJNDSRT && tile.objectIndex == 83 ) {
        tile.SetObject( MP2::OBJN_SKELETON );

        // There is no need to check the rest of things as we fixed this object.
        return;
    }

    // Original Editor marks Reefs as Stones. We're fixing this issue by changing the type of the object without changing the content of a tile.
    // This is also required in order to properly calculate Reefs' passbility.
    if ( originalObjectType == MP2::OBJ_STONES && isValidReefsSprite( originalICN, tile.objectIndex ) ) {
        tile.SetObject( MP2::OBJ_REEFS );

        // There is no need to check the rest of things as we fixed this object.
        return;
    }

    // Some maps have water tiles with OBJ_COAST, it shouldn't be, replace OBJ_COAST with OBJ_ZERO
    if ( originalObjectType == MP2::OBJ_COAST && tile.isWater() ) {
        Heroes * hero = tile.GetHeroes();

        if ( hero ) {
            hero->SetMapsObject( MP2::OBJ_ZERO );
        }
        else {
            tile.SetObject( MP2::OBJ_ZERO );
        }

        // There is no need to check the rest of things as we fixed this object.
        return;
    }

    // On some maps (apparently created by some non-standard editor), the object type on tiles with random monsters does not match the index
    // of the monster placeholder sprite. While this engine looks at the object type when placing an actual monster on a tile, the original
    // HoMM2 apparently looks at the placeholder sprite, so we need to keep them in sync.
    if ( originalICN == ICN::MONS32 ) {
        MP2::MapObjectType monsterObjectType = originalObjectType;

        const uint8_t originalObjectSpriteIndex = tile.GetObjectSpriteIndex();
        switch ( originalObjectSpriteIndex ) {
        // Random monster placeholder "MON"
        case 66:
            monsterObjectType = MP2::OBJ_RNDMONSTER;
            break;
        // Random monster placeholder "MON 1"
        case 67:
            monsterObjectType = MP2::OBJ_RNDMONSTER1;
            break;
        // Random monster placeholder "MON 2"
        case 68:
            monsterObjectType = MP2::OBJ_RNDMONSTER2;
            break;
        // Random monster placeholder "MON 3"
        case 69:
            monsterObjectType = MP2::OBJ_RNDMONSTER3;
            break;
        // Random monster placeholder "MON 4"
        case 70:
            monsterObjectType = MP2::OBJ_RNDMONSTER4;
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
    case MP2::OBJ_UNKNW_79:
    case MP2::OBJ_UNKNW_7A:
    case MP2::OBJ_UNKNW_F9:
    case MP2::OBJ_UNKNW_FA: {
        MP2::MapObjectType objectType = Maps::Tiles::GetLoyaltyObject( tile.objectTileset, tile.objectIndex );
        if ( objectType != MP2::OBJ_ZERO ) {
            tile.SetObject( objectType );
            break;
        }

        // Add-ons of level 1 shouldn't even exist if no top object. However, let's play safe and verify it as well.
        for ( const TilesAddon & addon : tile.addons_level1 ) {
            objectType = Maps::Tiles::GetLoyaltyObject( addon.object, addon.index );
            if ( objectType != MP2::OBJ_ZERO )
                break;
        }

        if ( objectType != MP2::OBJ_ZERO ) {
            tile.SetObject( objectType );
            break;
        }

        for ( const TilesAddon & addon : tile.addons_level2 ) {
            objectType = Maps::Tiles::GetLoyaltyObject( addon.object, addon.index );
            if ( objectType != MP2::OBJ_ZERO )
                break;
        }

        if ( objectType != MP2::OBJ_ZERO ) {
            tile.SetObject( objectType );
            break;
        }

        DEBUG_LOG( DBG_GAME, DBG_WARN,
                   "Invalid object type index " << tile._index << ": type " << MP2::StringObject( originalObjectType ) << ", icn ID "
                                                << static_cast<int>( tile.objectIndex ) )
        break;
    }

    default:
        break;
    }
}

bool Maps::Tiles::isCaptureObjectProtected() const
{
    const MP2::MapObjectType objectType = GetObject( false );

    if ( MP2::isCaptureObject( objectType ) ) {
        if ( MP2::OBJ_CASTLE == objectType ) {
            Castle * castle = world.getCastleEntrance( GetCenter() );
            if ( castle )
                return castle->GetArmy().isValid();
        }
        else
            return QuantityTroop().isValid();
    }

    return false;
}

void Maps::Tiles::Remove( uint32_t uniqID )
{
    addons_level1.remove_if( [uniqID]( const Maps::TilesAddon & v ) { return v.isUniq( uniqID ); } );
    addons_level2.remove_if( [uniqID]( const Maps::TilesAddon & v ) { return v.isUniq( uniqID ); } );

    if ( uniq == uniqID ) {
        resetObjectSprite();
        uniq = 0;
    }
}

void Maps::Tiles::ReplaceObjectSprite( uint32_t uniqID, uint8_t rawTileset, uint8_t newTileset, uint8_t indexToReplace, uint8_t newIndex )
{
    for ( Addons::iterator it = addons_level1.begin(); it != addons_level1.end(); ++it ) {
        if ( it->uniq == uniqID && ( it->object >> 2 ) == rawTileset && it->index == indexToReplace ) {
            it->object = newTileset;
            it->index = newIndex;
        }
    }
    for ( Addons::iterator it2 = addons_level2.begin(); it2 != addons_level2.end(); ++it2 ) {
        if ( it2->uniq == uniqID && ( it2->object >> 2 ) == rawTileset && it2->index == indexToReplace ) {
            it2->object = newTileset;
            it2->index = newIndex;
        }
    }

    if ( uniq == uniqID && ( objectTileset >> 2 ) == rawTileset && objectIndex == indexToReplace ) {
        objectTileset = newTileset;
        objectIndex = newIndex;
    }
}

void Maps::Tiles::UpdateObjectSprite( uint32_t uniqID, uint8_t rawTileset, uint8_t newTileset, int indexChange )
{
    for ( Addons::iterator it = addons_level1.begin(); it != addons_level1.end(); ++it ) {
        if ( it->uniq == uniqID && ( it->object >> 2 ) == rawTileset ) {
            it->object = newTileset;
            it->index = it->index + indexChange;
        }
    }
    for ( Addons::iterator it2 = addons_level2.begin(); it2 != addons_level2.end(); ++it2 ) {
        if ( it2->uniq == uniqID && ( it2->object >> 2 ) == rawTileset ) {
            it2->object = newTileset;
            it2->index = it2->index + indexChange;
        }
    }

    if ( uniq == uniqID && ( objectTileset >> 2 ) == rawTileset ) {
        objectTileset = newTileset;
        objectIndex += indexChange;
    }
}

void Maps::Tiles::RemoveObjectSprite()
{
    switch ( GetObject() ) {
    case MP2::OBJ_MONSTER:
        Remove( uniq );
        break;
    case MP2::OBJ_JAIL:
        RemoveJailSprite();
        tilePassable = DIRECTION_ALL;
        break;
    case MP2::OBJ_ARTIFACT: {
        const uint32_t uidArtifact = getObjectIdByICNType( ICN::OBJNARTI );
        Remove( uidArtifact );

        if ( Maps::isValidDirection( _index, Direction::LEFT ) )
            world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) ).Remove( uidArtifact );
        break;
    }
    case MP2::OBJ_TREASURECHEST:
    case MP2::OBJ_RESOURCE: {
        const uint32_t uidResource = getObjectIdByICNType( ICN::OBJNRSRC );
        Remove( uidResource );

        if ( Maps::isValidDirection( _index, Direction::LEFT ) )
            world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) ).Remove( uidResource );
        break;
    }
    case MP2::OBJ_BARRIER:
        tilePassable = DIRECTION_ALL;
        // fall-through
    default:
        // remove shadow sprite from left cell
        if ( Maps::isValidDirection( _index, Direction::LEFT ) )
            world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) ).Remove( uniq );

        Remove( uniq );
        break;
    }
}

void Maps::Tiles::RemoveJailSprite()
{
    // remove left sprite
    if ( Maps::isValidDirection( _index, Direction::LEFT ) ) {
        const int32_t left = Maps::GetDirectionIndex( _index, Direction::LEFT );
        world.GetTiles( left ).Remove( uniq );

        // remove left left sprite
        if ( Maps::isValidDirection( left, Direction::LEFT ) )
            world.GetTiles( Maps::GetDirectionIndex( left, Direction::LEFT ) ).Remove( uniq );
    }

    // remove top sprite
    if ( Maps::isValidDirection( _index, Direction::TOP ) ) {
        const int32_t top = Maps::GetDirectionIndex( _index, Direction::TOP );
        Maps::Tiles & topTile = world.GetTiles( top );
        topTile.Remove( uniq );

        if ( topTile.GetObject() == MP2::OBJ_JAIL ) {
            topTile.setAsEmpty();
            topTile.FixObject();
        }

        // remove top left sprite
        if ( Maps::isValidDirection( top, Direction::LEFT ) ) {
            Maps::Tiles & leftTile = world.GetTiles( Maps::GetDirectionIndex( top, Direction::LEFT ) );
            leftTile.Remove( uniq );

            if ( leftTile.GetObject() == MP2::OBJ_JAIL ) {
                leftTile.setAsEmpty();
                leftTile.FixObject();
            }
        }
    }

    Remove( uniq );
}

void Maps::Tiles::UpdateAbandonedMineSprite( Tiles & tile )
{
    if ( tile.uniq ) {
        const int type = tile.QuantityResourceCount().first;

        Tiles::UpdateAbandonedMineLeftSprite( tile.objectTileset, tile.objectIndex, type );
        for ( Addons::iterator it = tile.addons_level1.begin(); it != tile.addons_level1.end(); ++it )
            Tiles::UpdateAbandonedMineLeftSprite( it->object, it->index, type );

        if ( Maps::isValidDirection( tile._index, Direction::RIGHT ) ) {
            Tiles & tile2 = world.GetTiles( Maps::GetDirectionIndex( tile._index, Direction::RIGHT ) );
            TilesAddon * mines = tile2.FindAddonLevel1( tile.uniq );

            if ( mines )
                Tiles::UpdateAbandonedMineRightSprite( mines->object, mines->index );

            if ( tile2.GetObject() == MP2::OBJN_ABANDONEDMINE ) {
                tile2.SetObject( MP2::OBJN_MINES );
                Tiles::UpdateAbandonedMineRightSprite( tile2.objectTileset, tile2.objectIndex );
            }
        }
    }

    if ( Maps::isValidDirection( tile._index, Direction::LEFT ) ) {
        Tiles & tile2 = world.GetTiles( Maps::GetDirectionIndex( tile._index, Direction::LEFT ) );
        if ( tile2.GetObject() == MP2::OBJN_ABANDONEDMINE )
            tile2.SetObject( MP2::OBJN_MINES );
    }

    if ( Maps::isValidDirection( tile._index, Direction::TOP ) ) {
        Tiles & tile2 = world.GetTiles( Maps::GetDirectionIndex( tile._index, Direction::TOP ) );
        if ( tile2.GetObject() == MP2::OBJN_ABANDONEDMINE )
            tile2.SetObject( MP2::OBJN_MINES );

        if ( Maps::isValidDirection( tile2._index, Direction::LEFT ) ) {
            Tiles & tile3 = world.GetTiles( Maps::GetDirectionIndex( tile2._index, Direction::LEFT ) );
            if ( tile3.GetObject() == MP2::OBJN_ABANDONEDMINE )
                tile3.SetObject( MP2::OBJN_MINES );
        }

        if ( Maps::isValidDirection( tile2._index, Direction::RIGHT ) ) {
            Tiles & tile3 = world.GetTiles( Maps::GetDirectionIndex( tile2._index, Direction::RIGHT ) );
            if ( tile3.GetObject() == MP2::OBJN_ABANDONEDMINE )
                tile3.SetObject( MP2::OBJN_MINES );
        }
    }
}

void Maps::Tiles::UpdateRNDArtifactSprite( Tiles & tile )
{
    Artifact art;

    switch ( tile.GetObject() ) {
    case MP2::OBJ_RNDARTIFACT:
        art = Artifact::Rand( Artifact::ART_LEVEL123 );
        break;
    case MP2::OBJ_RNDARTIFACT1:
        art = Artifact::Rand( Artifact::ART_LEVEL1 );
        break;
    case MP2::OBJ_RNDARTIFACT2:
        art = Artifact::Rand( Artifact::ART_LEVEL2 );
        break;
    case MP2::OBJ_RNDARTIFACT3:
        art = Artifact::Rand( Artifact::ART_LEVEL3 );
        break;
    default:
        return;
    }

    if ( !art.isValid() ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown artifact" )
        return;
    }

    tile.SetObject( MP2::OBJ_ARTIFACT );

    uint32_t uidArtifact = tile.getObjectIdByICNType( ICN::OBJNARTI );
    if ( uidArtifact == 0 ) {
        uidArtifact = tile.uniq;
    }

    updateTileById( tile, uidArtifact, art.IndexSprite() );

    // replace artifact shadow
    if ( Maps::isValidDirection( tile._index, Direction::LEFT ) ) {
        updateTileById( world.GetTiles( Maps::GetDirectionIndex( tile._index, Direction::LEFT ) ), uidArtifact, art.IndexSprite() - 1 );
    }
}

void Maps::Tiles::UpdateRNDResourceSprite( Tiles & tile )
{
    tile.SetObject( MP2::OBJ_RESOURCE );

    const uint32_t resourceSprite = Resource::GetIndexSprite( Resource::Rand( true ) );

    uint32_t uidResource = tile.getObjectIdByICNType( ICN::OBJNRSRC );
    if ( uidResource == 0 ) {
        uidResource = tile.uniq;
    }

    updateTileById( tile, uidResource, resourceSprite );

    // Replace shadow of the resource.
    if ( Maps::isValidDirection( tile._index, Direction::LEFT ) ) {
        updateTileById( world.GetTiles( Maps::GetDirectionIndex( tile._index, Direction::LEFT ) ), uidResource, resourceSprite - 1 );
    }
}

std::pair<uint32_t, uint32_t> Maps::Tiles::GetMonsterSpriteIndices( const Tiles & tile, uint32_t monsterIndex )
{
    const int tileIndex = tile._index;
    int attackerIndex = -1;

    // scan for a hero around
    for ( const int32_t idx : ScanAroundObject( tileIndex, MP2::OBJ_HEROES, false ) ) {
        const Heroes * hero = world.GetTiles( idx ).GetHeroes();
        assert( hero != nullptr );

        // hero is going to attack monsters on this tile
        if ( hero->GetAttackedMonsterTileIndex() == tileIndex ) {
            attackerIndex = idx;
            break;
        }
    }

    std::pair<uint32_t, uint32_t> spriteIndices( monsterIndex * 9, 0 );

    // draw an attacking sprite if there is an attacking hero nearby
    if ( attackerIndex != -1 ) {
        spriteIndices.first += 7;

        switch ( Maps::GetDirection( tileIndex, attackerIndex ) ) {
        case Direction::TOP_LEFT:
        case Direction::LEFT:
        case Direction::BOTTOM_LEFT:
            spriteIndices.first += 1;
            break;
        default:
            break;
        }
    }
    else {
        const fheroes2::Point & mp = Maps::GetPoint( tileIndex );
        const std::array<uint8_t, 15> & monsterAnimationSequence = fheroes2::getMonsterAnimationSequence();
        spriteIndices.second = monsterIndex * 9 + 1 + monsterAnimationSequence[( Game::MapsAnimationFrame() + mp.x * mp.y ) % monsterAnimationSequence.size()];
    }
    return spriteIndices;
}

bool Maps::Tiles::isFogAllAround( const int color ) const
{
    const int32_t center = GetIndex();
    const fheroes2::Point mp = Maps::GetPoint( center );
    const int32_t width = world.w();
    const int32_t height = world.h();

    // Verify all tiles around the current one with radius of 2 to cover moving hero case as well.
    for ( int32_t y = -2; y < 3; ++y ) {
        const int32_t offsetY = mp.y + y;
        if ( offsetY < 0 || offsetY >= height )
            continue;

        const int32_t centerY = center + y * width;

        for ( int32_t x = -2; x < 3; ++x ) {
            if ( x == 0 && y == 0 )
                continue;

            const int32_t offsetX = mp.x + x;
            if ( offsetX < 0 || offsetX >= width )
                continue;

            if ( !world.GetTiles( centerY + x ).isFog( color ) ) {
                return false;
            }
        }
    }

    return true;
}

int Maps::Tiles::GetFogDirections( int color ) const
{
    int around = 0;
    const Directions & directions = Direction::All();

    for ( Directions::const_iterator it = directions.begin(); it != directions.end(); ++it )
        if ( !Maps::isValidDirection( _index, *it ) || world.GetTiles( Maps::GetDirectionIndex( _index, *it ) ).isFog( color ) )
            around |= *it;

    if ( isFog( color ) )
        around |= Direction::CENTER;

    return around;
}

void Maps::Tiles::drawFog( fheroes2::Image & dst, int color, const Interface::GameArea & area ) const
{
    const fheroes2::Point & mp = Maps::GetPoint( _index );

    const int around = GetFogDirections( color );

    // TIL::CLOF32
    if ( DIRECTION_ALL == around ) {
        const fheroes2::Image & sf = fheroes2::AGG::GetTIL( TIL::CLOF32, ( mp.x + mp.y ) % 4, 0 );
        area.DrawTile( dst, sf, mp );
    }
    else {
        uint32_t index = 0;
        bool revert = false;

        if ( ( around & Direction::CENTER ) && !( around & ( Direction::TOP | Direction::BOTTOM | Direction::LEFT | Direction::RIGHT ) ) ) {
            index = 10;
        }
        else if ( ( contains( around, Direction::CENTER | Direction::TOP ) ) && !( around & ( Direction::BOTTOM | Direction::LEFT | Direction::RIGHT ) ) ) {
            index = 6;
        }
        else if ( ( contains( around, Direction::CENTER | Direction::RIGHT ) ) && !( around & ( Direction::TOP | Direction::BOTTOM | Direction::LEFT ) ) ) {
            index = 7;
        }
        else if ( ( contains( around, Direction::CENTER | Direction::LEFT ) ) && !( around & ( Direction::TOP | Direction::BOTTOM | Direction::RIGHT ) ) ) {
            index = 7;
            revert = true;
        }
        else if ( ( contains( around, Direction::CENTER | Direction::BOTTOM ) ) && !( around & ( Direction::TOP | Direction::LEFT | Direction::RIGHT ) ) ) {
            index = 8;
        }
        else if ( ( contains( around, DIRECTION_CENTER_COL ) ) && !( around & ( Direction::LEFT | Direction::RIGHT ) ) ) {
            index = 9;
        }
        else if ( ( contains( around, DIRECTION_CENTER_ROW ) ) && !( around & ( Direction::TOP | Direction::BOTTOM ) ) ) {
            index = 29;
        }
        else if ( around == ( DIRECTION_ALL & ( ~Direction::TOP_RIGHT ) ) ) {
            index = 15;
        }
        else if ( around == ( DIRECTION_ALL & ( ~Direction::TOP_LEFT ) ) ) {
            index = 15;
            revert = true;
        }
        else if ( around == ( DIRECTION_ALL & ( ~Direction::BOTTOM_RIGHT ) ) ) {
            index = 22;
        }
        else if ( around == ( DIRECTION_ALL & ( ~Direction::BOTTOM_LEFT ) ) ) {
            index = 22;
            revert = true;
        }
        else if ( around == ( DIRECTION_ALL & ( ~( Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT ) ) ) ) {
            index = 16;
        }
        else if ( around == ( DIRECTION_ALL & ( ~( Direction::TOP_LEFT | Direction::BOTTOM_LEFT ) ) ) ) {
            index = 16;
            revert = true;
        }
        else if ( around == ( DIRECTION_ALL & ( ~( Direction::TOP_RIGHT | Direction::BOTTOM_LEFT ) ) ) ) {
            index = 17;
        }
        else if ( around == ( DIRECTION_ALL & ( ~( Direction::TOP_LEFT | Direction::BOTTOM_RIGHT ) ) ) ) {
            index = 17;
            revert = true;
        }
        else if ( around == ( DIRECTION_ALL & ( ~( Direction::TOP_LEFT | Direction::TOP_RIGHT ) ) ) ) {
            index = 18;
        }
        else if ( around == ( DIRECTION_ALL & ( ~( Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT ) ) ) ) {
            index = 23;
        }
        else if ( around == ( DIRECTION_ALL & ( ~DIRECTION_TOP_RIGHT_CORNER ) ) ) {
            index = 13;
        }
        else if ( around == ( DIRECTION_ALL & ( ~DIRECTION_TOP_LEFT_CORNER ) ) ) {
            index = 13;
            revert = true;
        }
        else if ( around == ( DIRECTION_ALL & ( ~DIRECTION_BOTTOM_RIGHT_CORNER ) ) ) {
            index = 14;
        }
        else if ( around == ( DIRECTION_ALL & ( ~DIRECTION_BOTTOM_LEFT_CORNER ) ) ) {
            index = 14;
            revert = true;
        }
        else if ( contains( around, Direction::CENTER | Direction::LEFT | Direction::BOTTOM_LEFT | Direction::BOTTOM )
                  && !( around & ( Direction::TOP | Direction::RIGHT ) ) ) {
            index = 11;
        }
        else if ( contains( around, Direction::CENTER | Direction::RIGHT | Direction::BOTTOM_RIGHT | Direction::BOTTOM )
                  && !( around & ( Direction::TOP | Direction::LEFT ) ) ) {
            index = 11;
            revert = true;
        }
        else if ( contains( around, Direction::CENTER | Direction::LEFT | Direction::TOP_LEFT | Direction::TOP )
                  && !( around & ( Direction::BOTTOM | Direction::RIGHT ) ) ) {
            index = 12;
        }
        else if ( contains( around, Direction::CENTER | Direction::RIGHT | Direction::TOP_RIGHT | Direction::TOP )
                  && !( around & ( Direction::BOTTOM | Direction::LEFT ) ) ) {
            index = 12;
            revert = true;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP | Direction::TOP_LEFT )
                  && !( around & ( Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT | Direction::TOP_RIGHT ) ) ) {
            index = 19;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP | Direction::TOP_RIGHT )
                  && !( around & ( Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT | Direction::TOP_LEFT ) ) ) {
            index = 19;
            revert = true;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP | Direction::BOTTOM_LEFT )
                  && !( around & ( Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT | Direction::TOP_LEFT ) ) ) {
            index = 20;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP | Direction::BOTTOM_RIGHT )
                  && !( around & ( Direction::TOP_RIGHT | Direction::BOTTOM_LEFT | Direction::TOP_LEFT ) ) ) {
            index = 20;
            revert = true;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP )
                  && !( around & ( Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT | Direction::BOTTOM_LEFT | Direction::TOP_LEFT ) ) ) {
            index = 22;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::BOTTOM_LEFT ) && !( around & ( Direction::TOP | Direction::BOTTOM_RIGHT ) ) ) {
            index = 24;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::BOTTOM_RIGHT ) && !( around & ( Direction::TOP | Direction::BOTTOM_LEFT ) ) ) {
            index = 24;
            revert = true;
        }
        else if ( contains( around, DIRECTION_CENTER_COL | Direction::LEFT | Direction::TOP_LEFT ) && !( around & ( Direction::RIGHT | Direction::BOTTOM_LEFT ) ) ) {
            index = 25;
        }
        else if ( contains( around, DIRECTION_CENTER_COL | Direction::RIGHT | Direction::TOP_RIGHT ) && !( around & ( Direction::LEFT | Direction::BOTTOM_RIGHT ) ) ) {
            index = 25;
            revert = true;
        }
        else if ( contains( around, DIRECTION_CENTER_COL | Direction::BOTTOM_LEFT | Direction::LEFT ) && !( around & ( Direction::RIGHT | Direction::TOP_LEFT ) ) ) {
            index = 26;
        }
        else if ( contains( around, DIRECTION_CENTER_COL | Direction::BOTTOM_RIGHT | Direction::RIGHT ) && !( around & ( Direction::LEFT | Direction::TOP_RIGHT ) ) ) {
            index = 26;
            revert = true;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | Direction::TOP_LEFT | Direction::TOP ) && !( around & ( Direction::BOTTOM | Direction::TOP_RIGHT ) ) ) {
            index = 30;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | Direction::TOP_RIGHT | Direction::TOP ) && !( around & ( Direction::BOTTOM | Direction::TOP_LEFT ) ) ) {
            index = 30;
            revert = true;
        }
        else if ( contains( around, Direction::CENTER | Direction::BOTTOM | Direction::LEFT )
                  && !( around & ( Direction::TOP | Direction::RIGHT | Direction::BOTTOM_LEFT ) ) ) {
            index = 27;
        }
        else if ( contains( around, Direction::CENTER | Direction::BOTTOM | Direction::RIGHT )
                  && !( around & ( Direction::TOP | Direction::TOP_LEFT | Direction::LEFT | Direction::BOTTOM_RIGHT ) ) ) {
            index = 27;
            revert = true;
        }
        else if ( contains( around, Direction::CENTER | Direction::LEFT | Direction::TOP )
                  && !( around & ( Direction::TOP_LEFT | Direction::RIGHT | Direction::BOTTOM | Direction::BOTTOM_RIGHT ) ) ) {
            index = 28;
        }
        else if ( contains( around, Direction::CENTER | Direction::RIGHT | Direction::TOP )
                  && !( around & ( Direction::TOP_RIGHT | Direction::LEFT | Direction::BOTTOM | Direction::BOTTOM_LEFT ) ) ) {
            index = 28;
            revert = true;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | Direction::TOP ) && !( around & ( Direction::BOTTOM | Direction::TOP_LEFT | Direction::TOP_RIGHT ) ) ) {
            index = 31;
        }
        else if ( contains( around, DIRECTION_CENTER_COL | Direction::RIGHT ) && !( around & ( Direction::LEFT | Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT ) ) ) {
            index = 32;
        }
        else if ( contains( around, DIRECTION_CENTER_COL | Direction::LEFT ) && !( around & ( Direction::RIGHT | Direction::TOP_LEFT | Direction::BOTTOM_LEFT ) ) ) {
            index = 32;
            revert = true;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | Direction::BOTTOM ) && !( around & ( Direction::TOP | Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT ) ) ) {
            index = 33;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW ) && !( around & Direction::TOP ) ) {
            index = ( _index % 2 ) ? 0 : 1;
        }
        else if ( contains( around, DIRECTION_CENTER_ROW | DIRECTION_TOP_ROW ) && !( around & Direction::BOTTOM ) ) {
            index = ( _index % 2 ) ? 4 : 5;
        }
        else if ( contains( around, DIRECTION_CENTER_COL | DIRECTION_LEFT_COL ) && !( around & Direction::RIGHT ) ) {
            index = ( _index % 2 ) ? 2 : 3;
        }
        else if ( contains( around, DIRECTION_CENTER_COL | DIRECTION_RIGHT_COL ) && !( around & Direction::LEFT ) ) {
            index = ( _index % 2 ) ? 2 : 3;
            revert = true;
        }
        else {
            // unknown
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid direction for fog: " << around )
            const fheroes2::Image & sf = fheroes2::AGG::GetTIL( TIL::CLOF32, ( mp.x + mp.y ) % 4, 0 );
            area.DrawTile( dst, sf, mp );
            return;
        }

        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::CLOP32, index );
        area.BlitOnTile( dst, sprite, ( revert ? sprite.x() + TILEWIDTH - sprite.width() : sprite.x() ), sprite.y(), mp, revert, 255 );
    }
}

void Maps::Tiles::updateTileById( Maps::Tiles & tile, const uint32_t uid, const uint8_t newIndex )
{
    Maps::TilesAddon * addon = tile.FindAddonLevel1( uid );
    if ( addon != nullptr ) {
        addon->index = newIndex;
    }
    else if ( tile.uniq == uid ) {
        tile.objectIndex = newIndex;
    }
}

void Maps::Tiles::updateEmpty()
{
    if ( mp2_object == MP2::OBJ_ZERO ) {
        setAsEmpty();
    }
}

void Maps::Tiles::setAsEmpty()
{
    // If an object is removed we should validate if this tile a potential candidate to be a coast.
    // Check if this tile is not water and it has neighbouring water tiles.
    if ( isWater() ) {
        SetObject( MP2::OBJ_ZERO );
        return;
    }

    bool isCoast = false;

    const Indexes tileIndices = Maps::getAroundIndexes( _index, 1 );
    for ( const int tileIndex : tileIndices ) {
        if ( tileIndex < 0 ) {
            // Invalid tile index.
            continue;
        }

        if ( world.GetTiles( tileIndex ).isWater() ) {
            isCoast = true;
            break;
        }
    }

    SetObject( isCoast ? MP2::OBJ_COAST : MP2::OBJ_ZERO );
}

uint32_t Maps::Tiles::getObjectIdByICNType( const int icnId ) const
{
    if ( MP2::GetICNObject( objectTileset ) == icnId ) {
        return uniq;
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( MP2::GetICNObject( addon.object ) == icnId ) {
            return addon.uniq;
        }
    }

    for ( const TilesAddon & addon : addons_level2 ) {
        if ( MP2::GetICNObject( addon.object ) == icnId ) {
            return addon.uniq;
        }
    }

    return 0;
}

std::vector<uint8_t> Maps::Tiles::getValidTileSets() const
{
    std::vector<uint8_t> tileSets;

    if ( objectTileset != 0 ) {
        tileSets.emplace_back( static_cast<uint8_t>( objectTileset >> 2 ) );
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( addon.object != 0 ) {
            tileSets.emplace_back( static_cast<uint8_t>( addon.object >> 2 ) );
        }
    }

    for ( const TilesAddon & addon : addons_level2 ) {
        if ( addon.object != 0 ) {
            tileSets.emplace_back( static_cast<uint8_t>( addon.object >> 2 ) );
        }
    }

    return tileSets;
}

bool Maps::Tiles::containsTileSet( const std::vector<uint8_t> & tileSets ) const
{
    for ( const uint8_t tileSetId : tileSets ) {
        if ( ( objectTileset >> 2 ) == tileSetId ) {
            return true;
        }

        for ( const TilesAddon & addon : addons_level1 ) {
            if ( ( addon.object >> 2 ) == tileSetId ) {
                return true;
            }
        }

        for ( const TilesAddon & addon : addons_level2 ) {
            if ( ( addon.object >> 2 ) == tileSetId ) {
                return true;
            }
        }
    }

    return false;
}

bool Maps::Tiles::containsSprite( uint8_t tileSetId, const uint32_t objectIdx ) const
{
    tileSetId = tileSetId >> 2;

    if ( ( objectTileset >> 2 ) == tileSetId && objectIdx == objectIndex ) {
        return true;
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( ( addon.object >> 2 ) == tileSetId && objectIdx == objectIndex ) {
            return true;
        }
    }

    for ( const TilesAddon & addon : addons_level2 ) {
        if ( ( addon.object >> 2 ) == tileSetId && objectIdx == objectIndex ) {
            return true;
        }
    }

    return false;
}

bool Maps::Tiles::isTallObject() const
{
    // TODO: possibly cache the output of the method as right now it's in average twice.
    if ( !Maps::isValidDirection( _index, Direction::TOP ) ) {
        // Nothing above so this object can't be tall.
        return false;
    }

    std::vector<uint32_t> tileUIDs;
    if ( objectTileset > 0 && objectIndex < 255 && uniq != 0 && ( ( _level >> 1 ) & 1 ) == 0 ) {
        tileUIDs.emplace_back( uniq );
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( addon.uniq != 0 && ( ( addon.level >> 1 ) & 1 ) == 0 ) {
            tileUIDs.emplace_back( addon.uniq );
        }
    }

    for ( const TilesAddon & addon : addons_level2 ) {
        if ( addon.uniq != 0 && ( ( addon.level >> 1 ) & 1 ) == 0 ) {
            tileUIDs.emplace_back( addon.uniq );
        }
    }

    const Tiles & topTile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP ) );
    for ( const uint32_t tileUID : tileUIDs ) {
        if ( topTile.uniq == tileUID && !topTile.isShadowSprite( topTile.objectTileset, topTile.objectIndex ) ) {
            return true;
        }

        for ( const TilesAddon & addon : topTile.addons_level1 ) {
            if ( addon.uniq == tileUID && !TilesAddon::isShadow( addon ) ) {
                return true;
            }
        }

        for ( const TilesAddon & addon : topTile.addons_level2 ) {
            if ( addon.uniq == tileUID && !TilesAddon::isShadow( addon ) ) {
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

    for ( const TilesAddon & addon : tile.getLevel1Addons() ) {
        uids.insert( addon.uniq );
    }

    for ( const TilesAddon & addon : tile.getLevel2Addons() ) {
        uids.insert( addon.uniq );
    }

    const int32_t tileIndex = tile.GetIndex();
    const int32_t mapWidth = world.w();

    // This is non-main tile of an action object. We have to find the main tile.
    // Since we don't want to care about the size of every object in the game we should find tiles in a certain radius.
    const int32_t radiusOfSearch = 3;

    // Main tile is usually at the bottom of the object so let's start from there. Also there are no objects having tiles below more than 1 row.
    for ( int32_t y = radiusOfSearch; y >= -1; --y ) {
        for ( int32_t x = -radiusOfSearch; x <= radiusOfSearch; ++x ) {
            const int32_t index = tileIndex + y * mapWidth + x;
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
    if ( uniq == objectUID ) {
        return ( ( _level >> 1 ) & 1 ) == 0;
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( addon.uniq == objectUID ) {
            return ( ( addon.level >> 1 ) & 1 ) == 0;
        }
    }

    return false;
}

StreamBase & Maps::operator<<( StreamBase & msg, const TilesAddon & ta )
{
    return msg << ta.level << ta.uniq << ta.object << ta.index;
}

StreamBase & Maps::operator>>( StreamBase & msg, TilesAddon & ta )
{
    msg >> ta.level >> ta.uniq >> ta.object >> ta.index;

    return msg;
}

StreamBase & Maps::operator<<( StreamBase & msg, const Tiles & tile )
{
    static_assert( sizeof( uint8_t ) == sizeof( MP2::MapObjectType ), "Incorrect type for writing MP2::MapObjectType object" );

    return msg << tile._index << tile.pack_sprite_index << tile.tilePassable << tile.uniq << tile.objectTileset << tile.objectIndex
               << static_cast<uint8_t>( tile.mp2_object ) << tile.fog_colors << tile.quantity1 << tile.quantity2 << tile.additionalMetadata << tile.heroID
               << tile.tileIsRoad << tile.addons_level1 << tile.addons_level2 << tile._level;
}

StreamBase & Maps::operator>>( StreamBase & msg, Tiles & tile )
{
    msg >> tile._index >> tile.pack_sprite_index >> tile.tilePassable >> tile.uniq >> tile.objectTileset >> tile.objectIndex;

    static_assert( sizeof( uint8_t ) == sizeof( MP2::MapObjectType ), "Incorrect type for reading MP2::MapObjectType object" );
    uint8_t objectType = MP2::OBJ_ZERO;
    msg >> objectType;
    tile.mp2_object = static_cast<MP2::MapObjectType>( objectType );

    msg >> tile.fog_colors >> tile.quantity1 >> tile.quantity2;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE_0918_RELEASE, "Remove the check below." );
    if ( Game::GetLoadVersion() < FORMAT_VERSION_PRE_0918_RELEASE ) {
        // Old additional metadata was stored in uint8_t format.
        uint8_t temp;
        msg >> temp;
        tile.additionalMetadata = temp;
    }
    else {
        msg >> tile.additionalMetadata;
    }

    msg >> tile.heroID >> tile.tileIsRoad >> tile.addons_level1 >> tile.addons_level2 >> tile._level;

    return msg;
}
