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
#ifdef WITH_DEBUG
#include "text.h"
#endif
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
        case ICN::ROAD:
        case ICN::EXTRAOVR:
        case ICN::MONS32:
        case ICN::BOAT32:
        case ICN::FLAG32:
            return false;
        default:
            break;
        }

        // Did you add a new type of objects into the game?
        assert( 0 );
        return false;
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

            VERBOSE_LOG( ICN::GetString( icnId ) << ": " << output );
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

        return imageMap.emplace( passable, std::move( sf ) ).first->second;
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
    os << "----------------" << lvl << "--------" << std::endl
       << "uniq            : " << uniq << std::endl
       << "tileset         : " << static_cast<int>( object ) << ", (" << ICN::GetString( MP2::GetICNObject( object ) ) << ")" << std::endl
       << "index           : " << static_cast<int>( index ) << std::endl
       << "level           : " << static_cast<int>( level ) << ", (" << static_cast<int>( level % 4 ) << ")" << std::endl
       << "shadow          : " << isShadow( *this ) << std::endl;
    return os.str();
}

Maps::TilesAddon::TilesAddon( const Maps::TilesAddon & ta )
    : uniq( ta.uniq )
    , level( ta.level )
    , object( ta.object )
    , index( ta.index )
{}

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
        else if ( 110 < icnIndex && icnIndex < 136 )
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
    // from sprite road
    case ICN::ROAD:
        if ( 1 == index || 8 == index || 10 == index || 11 == index || 15 == index || 22 == index || 23 == index || 24 == index || 25 == index || 27 == index )
            return false;
        else
            return true;

    // castle and tower (gate)
    case ICN::OBJNTOWN:
        if ( 13 == index || 29 == index || 45 == index || 61 == index || 77 == index || 93 == index || 109 == index || 125 == index || 141 == index || 157 == index
             || 173 == index || 189 == index )
            return true;
        break;

        // castle lands (gate)
    case ICN::OBJNTWBA:
        if ( 7 == index || 17 == index || 27 == index || 37 == index || 47 == index || 57 == index || 67 == index || 77 == index )
            return true;
        break;

    default:
        break;
    }

    return false;
}

bool Maps::TilesAddon::hasSpriteAnimation() const
{
    return object & 1;
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

bool Maps::TilesAddon::isFlag32( const TilesAddon & ta )
{
    return ICN::FLAG32 == MP2::GetICNObject( ta.object );
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

void Maps::Tiles::UpdateAbandoneMineLeftSprite( uint8_t & tileset, uint8_t & index, const int resource )
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

void Maps::Tiles::UpdateAbandoneMineRightSprite( uint8_t & tileset, uint8_t & index )
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
    }

    return res;
}

/* Maps::Addons */
void Maps::Addons::Remove( u32 uniq )
{
    remove_if( [uniq]( const TilesAddon & v ) { return v.isUniq( uniq ); } );
}

u32 PackTileSpriteIndex( u32 index, u32 shape ) /* index max: 0x3FFF, shape value: 0, 1, 2, 3 */
{
    return ( shape << 14 ) | ( 0x3FFF & index );
}

Maps::Tiles::Tiles()
    : _index( 0 )
    , pack_sprite_index( 0 )
    , uniq( 0 )
    , objectTileset( 0 )
    , objectIndex( 255 )
    , mp2_object( 0 )
    , tilePassable( DIRECTION_ALL )
    , fog_colors( Color::ALL )
    , quantity1( 0 )
    , quantity2( 0 )
    , quantity3( 0 )
{}

void Maps::Tiles::Init( s32 index, const MP2::mp2tile_t & mp2 )
{
    tilePassable = DIRECTION_ALL;

    _level = mp2.quantity1 & 0x03;
    quantity1 = mp2.quantity1;
    quantity2 = mp2.quantity2;
    quantity3 = 0;
    fog_colors = Color::ALL;

    SetTile( mp2.tileIndex, mp2.flags );
    SetIndex( index );
    SetObject( static_cast<MP2::MapObjectType>( mp2.mapObject ) );

    addons_level1.clear();
    addons_level2.clear();

    // those bitfields are set by map editor regardless if map object is there
    tileIsRoad = ( mp2.objectName1 >> 1 ) & 1;

    // If an object has priority 2 (shadow) or 3 (ground) then we put it as an addon.
    if ( mp2.mapObject == MP2::OBJ_ZERO && ( _level >> 1 ) & 1 ) {
        AddonsPushLevel1( mp2 );
    }
    else {
        objectTileset = mp2.objectName1;
        objectIndex = mp2.indexName1;
        uniq = mp2.level1ObjectUID;
    }
    AddonsPushLevel2( mp2 );
}

Heroes * Maps::Tiles::GetHeroes( void ) const
{
    return MP2::OBJ_HEROES == mp2_object && heroID ? world.GetHeroes( heroID - 1 ) : nullptr;
}

void Maps::Tiles::SetHeroes( Heroes * hero )
{
    if ( hero ) {
        hero->SetMapsObject( static_cast<MP2::MapObjectType>( mp2_object ) );
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

fheroes2::Point Maps::Tiles::GetCenter( void ) const
{
    return Maps::GetPoint( _index );
}

MP2::MapObjectType Maps::Tiles::GetObject( bool ignoreObjectUnderHero /* true */ ) const
{
    if ( !ignoreObjectUnderHero && MP2::OBJ_HEROES == mp2_object ) {
        const Heroes * hero = GetHeroes();
        return hero ? hero->GetMapsObject() : MP2::OBJ_ZERO;
    }

    return static_cast<MP2::MapObjectType>( mp2_object );
}

void Maps::Tiles::SetObject( const MP2::MapObjectType objectType )
{
    mp2_object = objectType;
    world.resetPathfinder();
}

void Maps::Tiles::setBoat( int direction )
{
    if ( objectTileset != 0 && objectIndex != 255 ) {
        AddonsPushLevel1( TilesAddon( 0, uniq, objectTileset, objectIndex ) );
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

void Maps::Tiles::resetObjectSprite()
{
    objectTileset = 0;
    objectIndex = 255;
}

void Maps::Tiles::SetTile( u32 sprite_index, u32 shape )
{
    pack_sprite_index = PackTileSpriteIndex( sprite_index, shape );
}

u32 Maps::Tiles::TileSpriteIndex( void ) const
{
    return pack_sprite_index & 0x3FFF;
}

u32 Maps::Tiles::TileSpriteShape( void ) const
{
    return pack_sprite_index >> 14;
}

const fheroes2::Image & Maps::Tiles::GetTileSurface( void ) const
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

            if ( isWater() != bottomTile.isWater() ) {
                // If object is bordering water then it must be marked as not passable.
                tilePassable = 0;
                return;
            }

            const bool isBottomTileObject = ( ( bottomTile._level >> 1 ) & 1 ) == 0;

            if ( !isDetachedObject() && isBottomTileObject && bottomTile.objectTileset > 0 && bottomTile.objectIndex < 255 ) {
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
                else if ( bottomTile.mp2_object != 0 && correctedObjectType != bottomTileObjectType && MP2::isActionObject( correctedObjectType )
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

uint32_t Maps::Tiles::GetRegion() const
{
    return _region;
}

void Maps::Tiles::UpdateRegion( uint32_t newRegionID )
{
    if ( tilePassable ) {
        _region = newRegionID;
    }
}

u32 Maps::Tiles::GetObjectUID() const
{
    return uniq;
}

int Maps::Tiles::GetPassable( void ) const
{
    return tilePassable;
}

bool Maps::Tiles::isClearGround() const
{
    const MP2::MapObjectType objectType = GetObject( true );

    switch ( objectType ) {
    case MP2::OBJ_ZERO:
    case MP2::OBJ_COAST:
        return true;

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
    if ( mt.objectName1 && mt.indexName1 < 0xFF ) {
        addons_level1.emplace_back( mt.quantity1, mt.level1ObjectUID, mt.objectName1, mt.indexName1 );
    }

    // MP2 "objectName" is a bitfield
    // 6 bits is ICN tileset id, 1 bit isRoad flag, 1 bit hasAnimation flag
    if ( ( mt.objectName1 >> 1 ) & 1 )
        tileIsRoad = true;
}

void Maps::Tiles::AddonsPushLevel1( const MP2::mp2addon_t & ma )
{
    if ( ma.objectNameN1 && ma.indexNameN1 < 0xFF ) {
        addons_level1.emplace_back( ma.quantityN, ma.level1ObjectUID, ma.objectNameN1, ma.indexNameN1 );
    }
}

void Maps::Tiles::AddonsPushLevel1( const TilesAddon & ta )
{
    addons_level1.emplace_back( ta );
}

void Maps::Tiles::AddonsPushLevel2( const MP2::mp2tile_t & mt )
{
    if ( mt.objectName2 && mt.indexName2 < 0xFF ) {
        addons_level2.emplace_back( mt.quantity1, mt.level2ObjectUID, mt.objectName2, mt.indexName2 );
    }
}

void Maps::Tiles::AddonsPushLevel2( const MP2::mp2addon_t & ma )
{
    if ( ma.objectNameN2 && ma.indexNameN2 < 0xFF ) {
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

int Maps::Tiles::GetGround( void ) const
{
    const u32 index = TileSpriteIndex();

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

bool Maps::Tiles::isWater( void ) const
{
    return 30 > TileSpriteIndex();
}

void Maps::Tiles::RedrawTile( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const
{
    const fheroes2::Point & mp = Maps::GetPoint( _index );

    if ( !( visibleTileROI & mp ) )
        return;

    area.DrawTile( dst, GetTileSurface(), mp );
}

void Maps::Tiles::RedrawEmptyTile( fheroes2::Image & dst, const fheroes2::Point & mp, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area )
{
    if ( !( visibleTileROI & mp ) ) {
        return;
    }

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

void Maps::Tiles::RedrawAddon( fheroes2::Image & dst, const Addons & addon, const fheroes2::Rect & visibleTileROI, bool isPuzzleDraw,
                               const Interface::GameArea & area ) const
{
    if ( addon.empty() ) {
        return;
    }

    const fheroes2::Point & mp = Maps::GetPoint( _index );

    if ( !( visibleTileROI & mp ) )
        return;

    for ( Addons::const_iterator it = addon.begin(); it != addon.end(); ++it ) {
        const u8 index = ( *it ).index;
        const int icn = MP2::GetICNObject( ( *it ).object );

        if ( ICN::UNKNOWN != icn && ICN::MINIHERO != icn && ICN::MONS32 != icn && ( !isPuzzleDraw || !MP2::isHiddenForPuzzle( it->object, index ) ) ) {
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, index );
            area.BlitOnTile( dst, sprite, sprite.x(), sprite.y(), mp );

            // possible animation
            const uint32_t animationIndex = ICN::AnimationFrame( icn, index, Game::MapsAnimationFrame(), quantity2 != 0 );
            if ( animationIndex ) {
                area.BlitOnTile( dst, fheroes2::AGG::GetICN( icn, animationIndex ), mp );
            }
        }
    }
}

void Maps::Tiles::RedrawBottom( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, bool isPuzzleDraw, const Interface::GameArea & area ) const
{
    RedrawAddon( dst, addons_level1, visibleTileROI, isPuzzleDraw, area );
}

void Maps::Tiles::RedrawPassable( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const
{
#ifdef WITH_DEBUG
    const fheroes2::Point & mp = Maps::GetPoint( _index );

    if ( ( visibleTileROI & mp ) && ( 0 == tilePassable || DIRECTION_ALL != tilePassable ) ) {
        area.BlitOnTile( dst, PassableViewSurface( tilePassable ), 0, 0, mp );
    }
#else
    (void)dst;
    (void)visibleTileROI;
    (void)area;
#endif
}

void Maps::Tiles::RedrawObjects( fheroes2::Image & dst, bool isPuzzleDraw, const Interface::GameArea & area ) const
{
    const MP2::MapObjectType objectType = GetObject();

    // monsters and boats will be drawn later, on top of everything else
    // hero object is accepted here since it replaces what was there originally
    if ( objectType != MP2::OBJ_BOAT && objectType != MP2::OBJ_MONSTER && ( !isPuzzleDraw || !MP2::isHiddenForPuzzle( objectTileset, objectIndex ) ) ) {
        const int icn = MP2::GetICNObject( objectTileset );

        if ( ICN::UNKNOWN != icn ) {
            const fheroes2::Point & mp = Maps::GetPoint( _index );

            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, objectIndex );
            area.BlitOnTile( dst, sprite, sprite.x(), sprite.y(), mp );

            // possible animation
            const uint32_t animationIndex = ICN::AnimationFrame( icn, objectIndex, Game::MapsAnimationFrame(), quantity2 != 0 );
            if ( animationIndex ) {
                const fheroes2::Sprite & animationSprite = fheroes2::AGG::GetICN( icn, animationIndex );

                area.BlitOnTile( dst, animationSprite, mp );
            }
        }
    }
}

void Maps::Tiles::RedrawMonster( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const
{
    const fheroes2::Point & mp = Maps::GetPoint( _index );

    if ( !( visibleTileROI & mp ) )
        return;

    const Monster & monster = QuantityMonster();
    const std::pair<uint32_t, uint32_t> spriteIndicies = GetMonsterSpriteIndices( *this, monster.GetSpriteIndex() );

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MINIMON, spriteIndicies.first );
    area.BlitOnTile( dst, sprite, sprite.x() + 16, sprite.y() + 30, mp );

    if ( spriteIndicies.second ) {
        const fheroes2::Sprite & animatedSprite = fheroes2::AGG::GetICN( ICN::MINIMON, spriteIndicies.second );
        area.BlitOnTile( dst, animatedSprite, animatedSprite.x() + 16, animatedSprite.y() + 30, mp );
    }
}

void Maps::Tiles::RedrawBoatShadow( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const
{
    const fheroes2::Point & mp = Maps::GetPoint( _index );

    if ( !( visibleTileROI & mp ) )
        return;

    const uint32_t spriteIndex = ( objectIndex == 255 ) ? 18 : objectIndex;

    const Game::ObjectFadeAnimation::FadeTask & fadeTask = Game::ObjectFadeAnimation::GetFadeTask();
    const uint8_t alpha
        = ( MP2::OBJ_BOAT == fadeTask.object && ( ( fadeTask.fadeOut && fadeTask.fromIndex == _index ) || ( fadeTask.fadeIn && fadeTask.toIndex == _index ) ) )
              ? fadeTask.alpha
              : 255;

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::BOATSHAD, spriteIndex % 128 );
    area.BlitOnTile( dst, sprite, sprite.x(), TILEWIDTH + sprite.y() - 11, mp, ( spriteIndex > 128 ), alpha );
}

void Maps::Tiles::RedrawBoat( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const
{
    const fheroes2::Point & mp = Maps::GetPoint( _index );

    if ( !( visibleTileROI & mp ) )
        return;

    const uint32_t spriteIndex = ( objectIndex == 255 ) ? 18 : objectIndex;

    const Game::ObjectFadeAnimation::FadeTask & fadeTask = Game::ObjectFadeAnimation::GetFadeTask();
    const uint8_t alpha
        = ( MP2::OBJ_BOAT == fadeTask.object && ( ( fadeTask.fadeOut && fadeTask.fromIndex == _index ) || ( fadeTask.fadeIn && fadeTask.toIndex == _index ) ) )
              ? fadeTask.alpha
              : 255;

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::BOAT32, spriteIndex % 128 );
    area.BlitOnTile( dst, sprite, sprite.x(), TILEWIDTH + sprite.y() - 11, mp, ( spriteIndex > 128 ), alpha );
}

bool Interface::SkipRedrawTileBottom4Hero( const uint8_t tileset, const uint8_t icnIndex, const int passable )
{
    const int icn = MP2::GetICNObject( tileset );
    switch ( icn ) {
    case ICN::UNKNOWN:
    case ICN::MINIHERO:
    case ICN::MONS32:
        return true;

    // whirlpool
    case ICN::OBJNWATR:
        return ( icnIndex >= 202 && icnIndex <= 225 ) || icnIndex == 69;

    // river delta
    case ICN::OBJNMUL2:
        return icnIndex < 14;

    case ICN::OBJNTWSH:
    case ICN::OBJNTWBA:
    case ICN::ROAD:
    case ICN::STREAM:
        return true;

    case ICN::OBJNCRCK:
        return ( icnIndex == 58 || icnIndex == 59 || icnIndex == 64 || icnIndex == 65 || icnIndex == 188 || icnIndex == 189 || ( passable & DIRECTION_TOP_ROW ) );

    case ICN::OBJNDIRT:
    case ICN::OBJNDSRT:
    case ICN::OBJNGRA2:
    case ICN::OBJNGRAS:
    case ICN::OBJNLAVA:
    case ICN::OBJNSNOW:
    case ICN::OBJNSWMP:
        return ( passable & DIRECTION_TOP_ROW ) != 0;

    default:
        break;
    }

    return Maps::Tiles::isShadowSprite( icn, icnIndex );
}

void Maps::Tiles::RedrawBottom4Hero( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const
{
    const fheroes2::Point & mp = Maps::GetPoint( _index );

    if ( !( visibleTileROI & mp ) )
        return;

    for ( Addons::const_iterator it = addons_level1.begin(); it != addons_level1.end(); ++it ) {
        const uint8_t object = it->object;
        const uint8_t index = it->index;
        if ( !Interface::SkipRedrawTileBottom4Hero( object, index, tilePassable ) ) {
            const int icn = MP2::GetICNObject( object );

            area.BlitOnTile( dst, fheroes2::AGG::GetICN( icn, index ), mp );

            // possible anime
            if ( it->object & 1 ) {
                area.BlitOnTile( dst, fheroes2::AGG::GetICN( icn, ICN::AnimationFrame( icn, index, Game::MapsAnimationFrame(), quantity2 != 0 ) ), mp );
            }
        }
    }
}

void Maps::Tiles::RedrawTop( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const bool isPuzzleDraw, const Interface::GameArea & area ) const
{
    const fheroes2::Point & mp = Maps::GetPoint( _index );

    if ( !( visibleTileROI & mp ) )
        return;

    const MP2::MapObjectType objectType = GetObject( false );
    // animate objects
    if ( objectType == MP2::OBJ_ABANDONEDMINE ) {
        area.BlitOnTile( dst, fheroes2::AGG::GetICN( ICN::OBJNHAUN, Game::MapsAnimationFrame() % 15 ), mp );
    }
    else if ( objectType == MP2::OBJ_MINES ) {
        const uint8_t spellID = quantity3;
        if ( spellID == Spell::HAUNT ) {
            area.BlitOnTile( dst, fheroes2::AGG::GetICN( ICN::OBJNHAUN, Game::MapsAnimationFrame() % 15 ), mp );
        }
        else if ( spellID >= Spell::SETEGUARDIAN && spellID <= Spell::SETWGUARDIAN ) {
            area.BlitOnTile( dst, fheroes2::AGG::GetICN( ICN::OBJNXTRA, spellID - Spell::SETEGUARDIAN ), TILEWIDTH, 0, mp );
        }
    }

    RedrawAddon( dst, addons_level2, visibleTileROI, isPuzzleDraw, area );
}

void Maps::Tiles::RedrawTopFromBottom( fheroes2::Image & dst, const Interface::GameArea & area ) const
{
    if ( !Maps::isValidDirection( _index, Direction::BOTTOM ) ) {
        return;
    }
    const Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::BOTTOM ) );
    const fheroes2::Point & mp = Maps::GetPoint( tile._index );
    for ( const Maps::TilesAddon & addon : tile.addons_level2 ) {
        const int icn = MP2::GetICNObject( addon.object );
        if ( icn == ICN::FLAG32 ) {
            area.BlitOnTile( dst, fheroes2::AGG::GetICN( icn, addon.index ), mp );
        }
    }
}

void Maps::Tiles::RedrawTop4Hero( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, bool skip_ground, const Interface::GameArea & area ) const
{
    const fheroes2::Point & mp = Maps::GetPoint( _index );

    if ( ( visibleTileROI & mp ) && !addons_level2.empty() ) {
        for ( Addons::const_iterator it = addons_level2.begin(); it != addons_level2.end(); ++it ) {
            if ( skip_ground && MP2::isActionObject( static_cast<MP2::MapObjectType>( ( *it ).object ) ) )
                continue;

            const uint8_t object = ( *it ).object;
            const uint8_t index = ( *it ).index;
            const int icn = MP2::GetICNObject( object );

            if ( ICN::HighlyObjectSprite( icn, index ) ) {
                area.BlitOnTile( dst, fheroes2::AGG::GetICN( icn, index ), mp );

                // possible anime
                if ( object & 1 ) {
                    area.BlitOnTile( dst, fheroes2::AGG::GetICN( icn, ICN::AnimationFrame( icn, index, Game::MapsAnimationFrame() ) ), mp );
                }
            }
        }
    }
}

Maps::TilesAddon * Maps::Tiles::FindAddonLevel1( u32 uniq1 )
{
    Addons::iterator it = std::find_if( addons_level1.begin(), addons_level1.end(), [uniq1]( const TilesAddon & v ) { return v.isUniq( uniq1 ); } );

    return it != addons_level1.end() ? &( *it ) : nullptr;
}

Maps::TilesAddon * Maps::Tiles::FindAddonLevel2( u32 uniq2 )
{
    Addons::iterator it = std::find_if( addons_level2.begin(), addons_level2.end(), [uniq2]( const TilesAddon & v ) { return v.isUniq( uniq2 ); } );

    return it != addons_level2.end() ? &( *it ) : nullptr;
}

std::string Maps::Tiles::String( void ) const
{
    std::ostringstream os;

    os << "----------------:>>>>>>>>" << std::endl
       << "Tile index      : " << _index << ", "
       << "point: (" << GetCenter().x << ", " << GetCenter().y << ")" << std::endl
       << "uniq            : " << uniq << std::endl
       << "mp2 object      : " << GetObject() << ", (" << MP2::StringObject( GetObject() ) << ")" << std::endl
       << "tileset         : " << static_cast<int>( objectTileset ) << ", (" << ICN::GetString( MP2::GetICNObject( objectTileset ) ) << ")" << std::endl
       << "object index    : " << static_cast<int>( objectIndex ) << ", (animated: " << hasSpriteAnimation() << ")" << std::endl
       << "level           : " << static_cast<int>( _level ) << std::endl
       << "region          : " << _region << std::endl
       << "ground          : " << Ground::String( GetGround() ) << ", (isRoad: " << tileIsRoad << ")" << std::endl
       << "shadow          : " << isShadowSprite( objectTileset, objectIndex ) << std::endl
       << "passable        : " << ( tilePassable ? Direction::String( tilePassable ) : "false" );

    os << std::endl
       << "quantity 1      : " << static_cast<int>( quantity1 ) << std::endl
       << "quantity 2      : " << static_cast<int>( quantity2 ) << std::endl
       << "quantity 3      : " << static_cast<int>( quantity3 ) << std::endl;

    for ( Addons::const_iterator it = addons_level1.begin(); it != addons_level1.end(); ++it )
        os << ( *it ).String( 1 );

    for ( Addons::const_iterator it = addons_level2.begin(); it != addons_level2.end(); ++it )
        os << ( *it ).String( 2 );

    os << "----------------I--------" << std::endl;

    // extra obj info
    switch ( GetObject() ) {
        // dwelling
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
        os << "count           : " << MonsterCount() << std::endl;
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
        const MapsIndexes & v = Maps::GetTilesUnderProtection( _index );
        if ( !v.empty() ) {
            os << "protection      : ";
            for ( MapsIndexes::const_iterator it = v.begin(); it != v.end(); ++it )
                os << *it << ", ";
            os << std::endl;
        }
        break;
    }
    }

    if ( MP2::isCaptureObject( GetObject( false ) ) ) {
        const CapturedObject & co = world.GetCapturedObject( _index );

        os << "capture color   : " << Color::String( co.objcol.second ) << std::endl;
        if ( co.guardians.isValid() ) {
            os << "capture guard   : " << co.guardians.GetName() << std::endl << "capture caunt   : " << co.guardians.GetCount() << std::endl;
        }
    }

    os << "----------------:<<<<<<<<" << std::endl;
    return os.str();
}

void Maps::Tiles::FixObject( void )
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
    if ( isWater() || !isPassable( Direction::CENTER, false, true, 0 ) ) {
        return false;
    }

    if ( objectTileset == 0 || isShadowSprite( objectTileset, objectIndex ) ) {
        return addons_level1.size() == static_cast<size_t>( std::count_if( addons_level1.begin(), addons_level1.end(), TilesAddon::isShadow ) );
    }

    return false;
}

bool Maps::Tiles::validateWaterRules( bool fromWater ) const
{
    const bool tileIsWater = isWater();
    if ( fromWater )
        return mp2_object == MP2::OBJ_COAST || ( tileIsWater && mp2_object != MP2::OBJ_BOAT );

    // if we're not in water but tile is; allow movement in three cases
    if ( tileIsWater )
        return mp2_object == MP2::OBJ_SHIPWRECK || mp2_object == MP2::OBJ_HEROES || mp2_object == MP2::OBJ_BOAT;

    return true;
}

bool Maps::Tiles::isPassable( int direct, bool fromWater, bool skipfog, const int heroColor ) const
{
    if ( !skipfog && isFog( heroColor ) )
        return false;

    if ( !validateWaterRules( fromWater ) )
        return false;

    return ( direct & tilePassable ) != 0;
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

/* check road */
bool Maps::Tiles::isRoad() const
{
    return tileIsRoad || mp2_object == MP2::OBJ_CASTLE;
}

bool Maps::Tiles::isStream( void ) const
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

bool Maps::Tiles::hasSpriteAnimation() const
{
    return objectTileset & 1;
}

bool Maps::Tiles::isObject( const MP2::MapObjectType objectType ) const
{
    return objectType == mp2_object;
}

uint8_t Maps::Tiles::GetObjectTileset() const
{
    return objectTileset;
}

uint8_t Maps::Tiles::GetObjectSpriteIndex() const
{
    return objectIndex;
}

Maps::TilesAddon * Maps::Tiles::FindFlags( void )
{
    Addons::iterator it = std::find_if( addons_level1.begin(), addons_level1.end(), TilesAddon::isFlag32 );

    if ( it == addons_level1.end() ) {
        it = std::find_if( addons_level2.begin(), addons_level2.end(), TilesAddon::isFlag32 );
        return addons_level2.end() != it ? &( *it ) : nullptr;
    }

    return addons_level1.end() != it ? &( *it ) : nullptr;
}

void Maps::Tiles::removeFlags()
{
    addons_level1.remove_if( TilesAddon::isFlag32 );
    addons_level2.remove_if( TilesAddon::isFlag32 );
}

void Maps::Tiles::CaptureFlags32( const MP2::MapObjectType objectType, int col )
{
    u32 index = 0;

    switch ( col ) {
    case Color::BLUE:
        index = 0;
        break;
    case Color::GREEN:
        index = 1;
        break;
    case Color::RED:
        index = 2;
        break;
    case Color::YELLOW:
        index = 3;
        break;
    case Color::ORANGE:
        index = 4;
        break;
    case Color::PURPLE:
        index = 5;
        break;
    default:
        index = 6;
        break;
    }

    switch ( objectType ) {
    case MP2::OBJ_WINDMILL:
        index += 42;
        CorrectFlags32( col, index, false );
        break;
    case MP2::OBJ_WATERWHEEL:
        index += 14;
        CorrectFlags32( col, index, false );
        break;
    case MP2::OBJ_MAGICGARDEN:
        index += 42;
        CorrectFlags32( col, index, false );
        break;

    case MP2::OBJ_MINES:
        index += 14;
        CorrectFlags32( col, index, true );
        break;
    case MP2::OBJ_LIGHTHOUSE:
        index += 42;
        CorrectFlags32( col, index, false );
        break;

    case MP2::OBJ_ALCHEMYLAB: {
        index += 21;
        if ( Maps::isValidDirection( _index, Direction::TOP ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP ) );
            tile.CorrectFlags32( col, index, true );
        }
        break;
    }

    case MP2::OBJ_SAWMILL: {
        index += 28;
        if ( Maps::isValidDirection( _index, Direction::TOP_RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP_RIGHT ) );
            tile.CorrectFlags32( col, index, true );
        }
        break;
    }

    case MP2::OBJ_CASTLE: {
        index *= 2;
        if ( Maps::isValidDirection( _index, Direction::LEFT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) );
            tile.CorrectFlags32( col, index, true );
        }

        index += 1;
        if ( Maps::isValidDirection( _index, Direction::RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::RIGHT ) );
            tile.CorrectFlags32( col, index, true );
        }
        break;
    }

    default:
        break;
    }
}

void Maps::Tiles::CorrectFlags32( const int col, const u32 index, const bool up )
{
    if ( col == Color::NONE ) {
        removeFlags();
        return;
    }

    TilesAddon * taddon = FindFlags();

    // replace flag
    if ( taddon )
        taddon->index = index;
    else if ( up )
        // or new flag
        addons_level2.emplace_back( TilesAddon::UPPER, World::GetUniq(), 0x38, index );
    else
        // or new flag
        addons_level1.emplace_back( TilesAddon::UPPER, World::GetUniq(), 0x38, index );
}

void Maps::Tiles::FixedPreload( Tiles & tile )
{
    // fix skeleton: left position
    if ( MP2::GetICNObject( tile.objectTileset ) == ICN::OBJNDSRT && tile.objectIndex == 83 ) {
        tile.SetObject( MP2::OBJN_SKELETON );
    }

    // fix price loyalty objects.
    if ( Settings::Get().isPriceOfLoyaltySupported() )
        switch ( tile.GetObject() ) {
        case MP2::OBJ_UNKNW_79:
        case MP2::OBJ_UNKNW_7A:
        case MP2::OBJ_UNKNW_F9:
        case MP2::OBJ_UNKNW_FA: {
            MP2::MapObjectType objectType = Maps::Tiles::GetLoyaltyObject( tile.objectTileset, tile.objectIndex );
            if ( objectType == MP2::OBJ_ZERO ) {
                // if nothing was found it means there's overlay tile on top of the object; search for it
                for ( auto it = tile.addons_level2.begin(); it != tile.addons_level2.end(); ++it ) {
                    objectType = Maps::Tiles::GetLoyaltyObject( it->object, it->index );
                    if ( objectType != MP2::OBJ_ZERO )
                        break;
                }
            }

            if ( MP2::OBJ_ZERO != objectType )
                tile.SetObject( objectType );
            else {
                DEBUG_LOG( DBG_GAME, DBG_WARN, "invalid expansion object at index: " << tile._index );
            }
            break;
        }

        default:
            break;
        }
}

/* true: if protection or has guardians */
bool Maps::Tiles::CaptureObjectIsProtection( void ) const
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

void Maps::Tiles::Remove( u32 uniqID )
{
    if ( !addons_level1.empty() )
        addons_level1.Remove( uniqID );
    if ( !addons_level2.empty() )
        addons_level2.Remove( uniqID );

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

void Maps::Tiles::RemoveObjectSprite( void )
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

void Maps::Tiles::RemoveJailSprite( void )
{
    // remove left sprite
    if ( Maps::isValidDirection( _index, Direction::LEFT ) ) {
        const s32 left = Maps::GetDirectionIndex( _index, Direction::LEFT );
        world.GetTiles( left ).Remove( uniq );

        // remove left left sprite
        if ( Maps::isValidDirection( left, Direction::LEFT ) )
            world.GetTiles( Maps::GetDirectionIndex( left, Direction::LEFT ) ).Remove( uniq );
    }

    // remove top sprite
    if ( Maps::isValidDirection( _index, Direction::TOP ) ) {
        const s32 top = Maps::GetDirectionIndex( _index, Direction::TOP );
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

void Maps::Tiles::UpdateAbandoneMineSprite( Tiles & tile )
{
    if ( tile.uniq ) {
        const int type = tile.QuantityResourceCount().first;

        Tiles::UpdateAbandoneMineLeftSprite( tile.objectTileset, tile.objectIndex, type );
        for ( Addons::iterator it = tile.addons_level1.begin(); it != tile.addons_level1.end(); ++it )
            Tiles::UpdateAbandoneMineLeftSprite( it->object, it->index, type );

        if ( Maps::isValidDirection( tile._index, Direction::RIGHT ) ) {
            Tiles & tile2 = world.GetTiles( Maps::GetDirectionIndex( tile._index, Direction::RIGHT ) );
            TilesAddon * mines = tile2.FindAddonLevel1( tile.uniq );

            if ( mines )
                Tiles::UpdateAbandoneMineRightSprite( mines->object, mines->index );

            if ( tile2.GetObject() == MP2::OBJN_ABANDONEDMINE ) {
                tile2.SetObject( MP2::OBJN_MINES );
                Tiles::UpdateAbandoneMineRightSprite( tile2.objectTileset, tile2.objectIndex );
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
        DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown artifact" );
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

void Maps::Tiles::ClearFog( int colors )
{
    fog_colors &= ~colors;
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

void Maps::Tiles::RedrawFogs( fheroes2::Image & dst, int color, const Interface::GameArea & area ) const
{
    const fheroes2::Point & mp = Maps::GetPoint( _index );

    const int around = GetFogDirections( color );

    // TIL::CLOF32
    if ( DIRECTION_ALL == around ) {
        const fheroes2::Image & sf = fheroes2::AGG::GetTIL( TIL::CLOF32, ( mp.x + mp.y ) % 4, 0 );
        area.DrawTile( dst, sf, mp );
    }
    else {
        u32 index = 0;
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
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid direction for fog: " << around );
            const fheroes2::Image & sf = fheroes2::AGG::GetTIL( TIL::CLOF32, ( mp.x + mp.y ) % 4, 0 );
            area.DrawTile( dst, sf, mp );
            return;
        }

        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::CLOP32, index );
        area.BlitOnTile( dst, sprite, ( revert ? sprite.x() + TILEWIDTH - sprite.width() : sprite.x() ), sprite.y(), mp, revert );
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
    // Check if this tile is not water and it have neighbouring water tiles.
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

    return 0;
}

std::vector<uint8_t> Maps::Tiles::getValidTileSets() const
{
    std::vector<uint8_t> tileSets;

    if ( objectTileset != 0 ) {
        tileSets.emplace_back( objectTileset >> 2 );
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( addon.object != 0 ) {
            tileSets.emplace_back( addon.object >> 2 );
        }
    }

    for ( const TilesAddon & addon : addons_level2 ) {
        if ( addon.object != 0 ) {
            tileSets.emplace_back( addon.object >> 2 );
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

    // This is non-main tile of an action object. We have to find the main tile.
    // Since we don't want to care about the size of every object in the game we should find tiles in a certain radius.
    const int32_t radiusOfSearch = 3;

    // It's unknown whether object type belongs to bottom layer or ground. Create a list of UIDs starting from bottom layer.
    std::vector<uint32_t> uids;
    const Maps::Addons & level2Addons = tile.getLevel2Addons();
    const Maps::Addons & level1Addons = tile.getLevel1Addons();

    for ( auto iter = level2Addons.rbegin(); iter != level2Addons.rend(); ++iter ) {
        if ( iter->uniq != 0 ) {
            uids.emplace_back( iter->uniq );
        }
    }

    if ( tile.GetObjectUID() != 0 ) {
        uids.emplace_back( tile.GetObjectUID() );
    }

    for ( auto iter = level1Addons.rbegin(); iter != level1Addons.rend(); ++iter ) {
        if ( iter->uniq != 0 ) {
            uids.emplace_back( iter->uniq );
        }
    }

    const int32_t tileIndex = tile.GetIndex();
    const int32_t mapWidth = world.w();

    assert( correctedObjectType > objectType );

    for ( int32_t y = -radiusOfSearch; y <= radiusOfSearch; ++y ) {
        for ( int32_t x = -radiusOfSearch; x <= radiusOfSearch; ++x ) {
            const int32_t index = tileIndex + y * mapWidth + x;
            if ( Maps::isValidAbsIndex( index ) ) {
                const Maps::Tiles & foundTile = world.GetTiles( index );
                if ( std::find( uids.begin(), uids.end(), foundTile.GetObjectUID() ) != uids.end() && foundTile.GetObject( false ) == correctedObjectType ) {
                    return foundTile._index;
                }
            }
        }
    }

    // Most likely we have a broken object put by an editor.
    DEBUG_LOG( DBG_GAME, DBG_WARN, "Tile " << tileIndex << " of type " << MP2::StringObject( objectType ) << " has no parent tile." );
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
    return msg << tile._index << tile.pack_sprite_index << tile.tilePassable << tile.uniq << tile.objectTileset << tile.objectIndex << tile.mp2_object << tile.fog_colors
               << tile.quantity1 << tile.quantity2 << tile.quantity3 << tile.heroID << tile.tileIsRoad << tile.addons_level1 << tile.addons_level2 << tile._level;
}

StreamBase & Maps::operator>>( StreamBase & msg, Tiles & tile )
{
    msg >> tile._index >> tile.pack_sprite_index >> tile.tilePassable >> tile.uniq >> tile.objectTileset >> tile.objectIndex >> tile.mp2_object >> tile.fog_colors
        >> tile.quantity1 >> tile.quantity2 >> tile.quantity3 >> tile.heroID >> tile.tileIsRoad >> tile.addons_level1 >> tile.addons_level2 >> tile._level;

    return msg;
}
