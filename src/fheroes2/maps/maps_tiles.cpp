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

#include "maps_tiles.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>

#include "army.h"
#include "army_troop.h"
#include "artifact.h"
#include "castle.h"
#include "game_io.h"
#include "ground.h"
#include "heroes.h"
#include "icn.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles_helper.h" // TODO: This file should not be included
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
#include "pairs.h"
#include "payment.h"
#include "profit.h"
#include "resource.h"
#include "save_format_version.h"
#include "serialize.h"
#include "tools.h"
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

    bool isShadowSprite( const MP2::ObjectIcnType objectIcnType, const uint8_t icnIndex )
    {
        return isValidShadowSprite( MP2::getIcnIdFromObjectIcnType( objectIcnType ), icnIndex );
    }

    bool isValidReefsSprite( const MP2::ObjectIcnType objectIcnType, const uint8_t icnIndex )
    {
        return objectIcnType == MP2::OBJ_ICN_TYPE_X_LOC2 && ObjXlc2::isReefs( icnIndex );
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
        case MP2::OBJ_MINES:
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
            else if ( ObjXlc2::isReefs( icnIndex ) )
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
}

Maps::TilesAddon::TilesAddon( const uint8_t layerType, const uint32_t uid, const MP2::ObjectIcnType objectIcnType, const uint8_t imageIndex,
                              const bool hasObjectAnimation, const bool isMarkedAsRoad )
    : _uid( uid )
    , _layerType( layerType )
    , _objectIcnType( objectIcnType )
    , _imageIndex( imageIndex )
    , _hasObjectAnimation( hasObjectAnimation )
    , _isMarkedAsRoad( isMarkedAsRoad )
{
    // Do nothing.
}

std::string Maps::TilesAddon::String( int lvl ) const
{
    std::ostringstream os;
    os << "--------- Level " << lvl << " --------" << std::endl
       << "UID             : " << _uid << std::endl
       << "ICN object type : " << static_cast<int>( _objectIcnType ) << " (" << ICN::GetString( MP2::getIcnIdFromObjectIcnType( _objectIcnType ) ) << ")" << std::endl
       << "image index     : " << static_cast<int>( _imageIndex ) << std::endl
       << "layer type      : " << static_cast<int>( _layerType ) << " (" << static_cast<int>( _layerType % 4 ) << ")"
       << " - " << getObjectLayerName( _layerType % 4 ) << std::endl
       << "is shadow       : " << ( isShadow( *this ) ? "yes" : "no" ) << std::endl;
    return os.str();
}

bool Maps::TilesAddon::PredicateSortRules1( const Maps::TilesAddon & ta1, const Maps::TilesAddon & ta2 )
{
    return ( ( ta1._layerType % 4 ) > ( ta2._layerType % 4 ) );
}

bool Maps::TilesAddon::isRoad() const
{
    switch ( _objectIcnType ) {
    // road sprite
    case MP2::OBJ_ICN_TYPE_ROAD:
        if ( 1 == _imageIndex || 8 == _imageIndex || 10 == _imageIndex || 11 == _imageIndex || 15 == _imageIndex || 22 == _imageIndex || 23 == _imageIndex
             || 24 == _imageIndex || 25 == _imageIndex || 27 == _imageIndex )
            return false;
        else
            return true;

    // castle or town gate
    case MP2::OBJ_ICN_TYPE_OBJNTOWN:
        if ( 13 == _imageIndex || 29 == _imageIndex || 45 == _imageIndex || 61 == _imageIndex || 77 == _imageIndex || 93 == _imageIndex || 109 == _imageIndex
             || 125 == _imageIndex || 141 == _imageIndex || 157 == _imageIndex || 173 == _imageIndex || 189 == _imageIndex )
            return true;
        break;

    // Random castle or town gate.
    case MP2::OBJ_ICN_TYPE_OBJNTWRD:
        return ( _imageIndex == 13 || _imageIndex == 29 );

    default:
        break;
    }

    return false;
}

bool Maps::TilesAddon::isResource( const TilesAddon & ta )
{
    return ( MP2::OBJ_ICN_TYPE_OBJNRSRC == ta._objectIcnType ) && ( ta._imageIndex % 2 );
}

bool Maps::TilesAddon::isArtifact( const TilesAddon & ta )
{
    // OBJNARTI (skip ultimate)
    return ( MP2::OBJ_ICN_TYPE_OBJNARTI == ta._objectIcnType ) && ( ta._imageIndex > 0x10 ) && ( ta._imageIndex % 2 );
}

bool Maps::TilesAddon::isShadow( const TilesAddon & ta )
{
    return isShadowSprite( ta._objectIcnType, ta._imageIndex );
}

void Maps::Tiles::Init( int32_t index, const MP2::mp2tile_t & mp2 )
{
    tilePassable = DIRECTION_ALL;

    _layerType = mp2.quantity1 & 0x03;
    _metadata[0] = ( ( ( mp2.quantity2 << 8 ) + mp2.quantity1 ) >> 3 );
    _fogColors = Color::ALL;
    _terrainImageIndex = mp2.terrainImageIndex;
    _terrainFlags = mp2.terrainFlags;
    _boatOwnerColor = Color::NONE;

    SetIndex( index );
    SetObject( static_cast<MP2::MapObjectType>( mp2.mapObjectType ) );

    if ( !MP2::doesObjectContainMetadata( _mainObjectType ) ) {
        // No metadata should exist for this object!
        assert( ( ( ( mp2.quantity2 << 8 ) + mp2.quantity1 ) >> 3 ) == 0 );
    }

    addons_level1.clear();
    addons_level2.clear();

    // those bitfields are set by map editor regardless if map object is there
    tileIsRoad = ( ( mp2.objectName1 >> 1 ) & 1 ) && ( ( mp2.objectName1 >> 2 ) == MP2::OBJ_ICN_TYPE_ROAD );

    // If an object has priority 2 (shadow) or 3 (ground) then we put it as an addon.
    if ( mp2.mapObjectType == MP2::OBJ_NONE && ( _layerType >> 1 ) & 1 ) {
        AddonsPushLevel1( mp2 );
    }
    else {
        _uid = mp2.level1ObjectUID;
        _objectIcnType = static_cast<MP2::ObjectIcnType>( mp2.objectName1 >> 2 );
        _imageIndex = mp2.level1IcnImageIndex;
        _hasObjectAnimation = ( mp2.objectName1 & 1 ) != 0;
        _isMarkedAsRoad = ( mp2.objectName1 & 2 ) != 0;
    }

    AddonsPushLevel2( mp2 );
}

Heroes * Maps::Tiles::GetHeroes() const
{
    return MP2::OBJ_HEROES == _mainObjectType && heroID ? world.GetHeroes( heroID - 1 ) : nullptr;
}

void Maps::Tiles::SetHeroes( Heroes * hero )
{
    if ( hero ) {
        using HeroIDType = decltype( heroID );
        static_assert( std::is_same_v<HeroIDType, uint8_t>, "Type of heroID has been changed, check the logic below" );

        hero->SetMapsObject( _mainObjectType );

        assert( hero->GetID() >= std::numeric_limits<HeroIDType>::min() && hero->GetID() < std::numeric_limits<HeroIDType>::max() );
        heroID = static_cast<HeroIDType>( hero->GetID() + 1 );

        SetObject( MP2::OBJ_HEROES );
    }
    else {
        hero = GetHeroes();

        if ( hero ) {
            SetObject( hero->GetMapsObject() );
            hero->SetMapsObject( MP2::OBJ_NONE );
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
    if ( !ignoreObjectUnderHero && MP2::OBJ_HEROES == _mainObjectType ) {
        const Heroes * hero = GetHeroes();
        return hero ? hero->GetMapsObject() : MP2::OBJ_NONE;
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
    if ( _objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN && _imageIndex != 255 ) {
        AddonsPushLevel1( TilesAddon( OBJECT_LAYER, _uid, _objectIcnType, _imageIndex, false, false ) );
    }

    SetObject( MP2::OBJ_BOAT );
    _objectIcnType = MP2::OBJ_ICN_TYPE_BOAT32;

    // Left-side sprites have to flipped, add 128 to index
    switch ( direction ) {
    case Direction::TOP:
        _imageIndex = 0;
        break;
    case Direction::TOP_RIGHT:
        _imageIndex = 9;
        break;
    case Direction::RIGHT:
        _imageIndex = 18;
        break;
    case Direction::BOTTOM_RIGHT:
        _imageIndex = 27;
        break;
    case Direction::BOTTOM:
        _imageIndex = 36;
        break;
    case Direction::BOTTOM_LEFT:
        _imageIndex = 27 + 128;
        break;
    case Direction::LEFT:
        _imageIndex = 18 + 128;
        break;
    case Direction::TOP_LEFT:
        _imageIndex = 9 + 128;
        break;
    default:
        _imageIndex = 18;
        break;
    }

    _uid = World::GetUniq();

    using BoatOwnerColorType = decltype( _boatOwnerColor );
    static_assert( std::is_same_v<BoatOwnerColorType, uint8_t>, "Type of _boatOwnerColor has been changed, check the logic below" );

    assert( color >= std::numeric_limits<BoatOwnerColorType>::min() && color <= std::numeric_limits<BoatOwnerColorType>::max() );

    _boatOwnerColor = static_cast<BoatOwnerColorType>( color );
}

int Maps::Tiles::getBoatDirection() const
{
    // Check if it really is a boat
    if ( _objectIcnType != MP2::OBJ_ICN_TYPE_BOAT32 )
        return Direction::UNKNOWN;

    // Left-side sprites have to flipped, add 128 to index
    switch ( _imageIndex ) {
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

    if ( MP2::isActionObject( objectType ) ) {
        return MP2::getActionObjectDirection( objectType );
    }

    if ( ( _objectIcnType == MP2::OBJ_ICN_TYPE_UNKNOWN || _imageIndex == 255 ) || ( ( _layerType >> 1 ) & 1 ) || isShadow() ) {
        // No object exists. Make it fully passable.
        return DIRECTION_ALL;
    }

    if ( isValidReefsSprite( _objectIcnType, _imageIndex ) ) {
        return 0;
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( isValidReefsSprite( addon._objectIcnType, addon._imageIndex ) ) {
            return 0;
        }
    }

    // Objects have fixed passability.
    return DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW;
}

void Maps::Tiles::setInitialPassability()
{
    using TilePassableType = decltype( tilePassable );
    static_assert( std::is_same_v<TilePassableType, uint16_t>, "Type of tilePassable has been changed, check the logic below" );

    const int passability = getOriginalPassability();
    assert( passability >= std::numeric_limits<TilePassableType>::min() && passability <= std::numeric_limits<TilePassableType>::max() );

    tilePassable = static_cast<TilePassableType>( passability );
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
    if ( !isActionObject && ( _objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN ) && _imageIndex < 255 && ( ( _layerType >> 1 ) & 1 ) == 0 && !isShadow() ) {
        // This is a non-action object.
        if ( Maps::isValidDirection( _index, Direction::BOTTOM ) ) {
            const Tiles & bottomTile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::BOTTOM ) );

            // If a bottom tile has the same object ID then this tile is inaccessible.
            std::vector<uint32_t> tileUIDs;
            if ( ( _objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN ) && _imageIndex < 255 && _uid != 0 && ( ( _layerType >> 1 ) & 1 ) == 0 ) {
                tileUIDs.emplace_back( _uid );
            }

            for ( const TilesAddon & addon : addons_level1 ) {
                if ( addon._uid != 0 && ( ( addon._layerType >> 1 ) & 1 ) == 0 ) {
                    tileUIDs.emplace_back( addon._uid );
                }
            }

            for ( const uint32_t objectId : tileUIDs ) {
                if ( bottomTile.doesObjectExist( objectId ) ) {
                    tilePassable = 0;
                    return;
                }
            }

            // If an object locates on land and the bottom tile is water mark the current tile as impassable. It's done for cases that a hero won't be able to
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

                return addon._objectIcnType != MP2::OBJ_ICN_TYPE_ROAD && addon._objectIcnType != MP2::OBJ_ICN_TYPE_STREAM;
            } );

            const bool singleObjectTile = validLevel1ObjectCount == 0 && addons_level2.empty() && ( bottomTile._objectIcnType != _objectIcnType );
            const bool isBottomTileObject = ( ( bottomTile._layerType >> 1 ) & 1 ) == 0;

            // TODO: we might need to simplify the logic below as singleObjectTile might cover most of it.
            if ( !singleObjectTile && !isDetachedObject() && isBottomTileObject && ( bottomTile._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN )
                 && bottomTile._imageIndex < 255 ) {
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
                else if ( bottomTile._mainObjectType != MP2::OBJ_NONE && correctedObjectType != bottomTileObjectType && MP2::isActionObject( correctedObjectType )
                          && isShortObject( correctedObjectType ) && ( bottomTile.getOriginalPassability() & Direction::TOP ) == 0 ) {
                    tilePassable &= ~Direction::BOTTOM;
                }
                else if ( isShortObject( bottomTileObjectType )
                          || ( !bottomTile.containsAnyObjectIcnType( getValidObjectIcnTypes() )
                               && ( isCombinedObject( objectType ) || isCombinedObject( bottomTileObjectType ) ) ) ) {
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
    if ( _uid == uid && ( ( _layerType >> 1 ) & 1 ) == 0 ) {
        return true;
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( addon._uid == uid && ( ( addon._layerType >> 1 ) & 1 ) == 0 ) {
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
    case MP2::OBJ_NONE:
    case MP2::OBJ_COAST:
        return true;
    case MP2::OBJ_BOAT:
        return false;

    default:
        break;
    }

    if ( ( _objectIcnType == MP2::OBJ_ICN_TYPE_UNKNOWN ) || _imageIndex == 255 || ( ( _layerType >> 1 ) & 1 ) == 1 ) {
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
        addons_level1.emplace_back( mt.quantity1, mt.level1ObjectUID, static_cast<MP2::ObjectIcnType>( mt.objectName1 >> 2 ), mt.level1IcnImageIndex, false, false );
    }

    // MP2 "objectName" is a bitfield
    // 6 bits is for object ICN type, 1 bit is isRoad flag, 1 bit is hasAnimation flag
    if ( ( ( mt.objectName1 >> 1 ) & 1 ) && ( ( mt.objectName1 >> 2 ) == MP2::OBJ_ICN_TYPE_ROAD ) ) {
        tileIsRoad = true;
    }
}

void Maps::Tiles::AddonsPushLevel1( const MP2::mp2addon_t & ma )
{
    if ( ma.objectNameN1 != 0 && ma.indexNameN1 != 0xFF ) {
        addons_level1.emplace_back( ma.quantityN, ma.level1ObjectUID, static_cast<MP2::ObjectIcnType>( ma.objectNameN1 >> 2 ), ma.indexNameN1, false, false );
    }
}

void Maps::Tiles::AddonsPushLevel2( const MP2::mp2tile_t & mt )
{
    if ( mt.objectName2 != 0 && mt.level2IcnImageIndex != 0xFF ) {
        // TODO: does level 2 even need level value? Verify it.
        addons_level2.emplace_back( mt.quantity1, mt.level2ObjectUID, static_cast<MP2::ObjectIcnType>( mt.objectName2 >> 2 ), mt.level2IcnImageIndex, false, false );
    }
}

void Maps::Tiles::AddonsPushLevel2( const MP2::mp2addon_t & ma )
{
    if ( ma.objectNameN2 != 0 && ma.indexNameN2 != 0xFF ) {
        // TODO: why do we use the same quantityN member for both level 1 and 2?
        addons_level2.emplace_back( ma.quantityN, ma.level2ObjectUID, static_cast<MP2::ObjectIcnType>( ma.objectNameN2 >> 2 ), ma.indexNameN2, false, false );
    }
}

void Maps::Tiles::AddonsSort()
{
    // Push everything to the container and sort it by level.
    if ( _objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN && _imageIndex != 255 ) {
        addons_level1.emplace_front( _layerType, _uid, _objectIcnType, _imageIndex, false, false );
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
        _uid = highestPriorityAddon._uid;
        _objectIcnType = highestPriorityAddon._objectIcnType;
        _imageIndex = highestPriorityAddon._imageIndex;
        _layerType = highestPriorityAddon._layerType & 0x03;

        addons_level1.pop_back();
    }

    // Level 2 objects don't have any rendering priorities so they should be rendered first in queue first to render.
}

int Maps::Tiles::GetGround() const
{
    // list grounds from GROUND32.TIL
    if ( 30 > _terrainImageIndex )
        return Maps::Ground::WATER;
    if ( 92 > _terrainImageIndex )
        return Maps::Ground::GRASS;
    if ( 146 > _terrainImageIndex )
        return Maps::Ground::SNOW;
    if ( 208 > _terrainImageIndex )
        return Maps::Ground::SWAMP;
    if ( 262 > _terrainImageIndex )
        return Maps::Ground::LAVA;
    if ( 321 > _terrainImageIndex )
        return Maps::Ground::DESERT;
    if ( 361 > _terrainImageIndex )
        return Maps::Ground::DIRT;
    if ( 415 > _terrainImageIndex )
        return Maps::Ground::WASTELAND;

    return Maps::Ground::BEACH;
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
       << "UID             : " << _uid << std::endl
       << "MP2 object type : " << static_cast<int>( objectType ) << " (" << MP2::StringObject( objectType ) << ")" << std::endl
       << "ICN object type : " << static_cast<int>( _objectIcnType ) << " (" << ICN::GetString( MP2::getIcnIdFromObjectIcnType( _objectIcnType ) ) << ")" << std::endl
       << "image index     : " << static_cast<int>( _imageIndex ) << " (animated: " << hasSpriteAnimation() << ")" << std::endl
       << "layer type      : " << static_cast<int>( _layerType ) << " - " << getObjectLayerName( _layerType ) << std::endl
       << "region          : " << _region << std::endl
       << "ground          : " << Ground::String( GetGround() ) << " (isRoad: " << tileIsRoad << ")" << std::endl
       << "shadow          : " << ( isShadowSprite( _objectIcnType, _imageIndex ) ? "true" : "false" ) << std::endl
       << "passable from   : " << ( tilePassable ? Direction::String( tilePassable ) : "nowhere" );

    os << std::endl
       << "metadata value 1: " << _metadata[0] << std::endl
       << "metadata value 2: " << _metadata[1] << std::endl
       << "metadata value 3: " << _metadata[2] << std::endl;

    if ( objectType == MP2::OBJ_BOAT )
        os << "boat owner color: " << Color::String( _boatOwnerColor ) << std::endl;

    for ( const TilesAddon & addon : addons_level1 ) {
        os << addon.String( 1 );
    }

    for ( const TilesAddon & addon : addons_level2 ) {
        os << addon.String( 2 );
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
    case MP2::OBJ_HEROES: {
        const Heroes * hero = GetHeroes();
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

void Maps::Tiles::FixObject()
{
    if ( MP2::OBJ_NONE == _mainObjectType ) {
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

    if ( _objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN && !isShadowSprite( _objectIcnType, _imageIndex ) ) {
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
    if ( fromWater && _mainObjectType != MP2::OBJ_COAST && ( !tileIsWater || _mainObjectType == MP2::OBJ_BOAT ) ) {
        return false;
    }

    // From the ground we can get to the water tile only if this tile contains a certain object.
    if ( !fromWater && tileIsWater && _mainObjectType != MP2::OBJ_SHIPWRECK && _mainObjectType != MP2::OBJ_HEROES && _mainObjectType != MP2::OBJ_BOAT ) {
        return false;
    }

    // Tiles on which allied heroes are located are inaccessible
    if ( _mainObjectType == MP2::OBJ_HEROES ) {
        const Heroes * hero = GetHeroes();
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

    return ( direction & tilePassable ) != 0;
}

void Maps::Tiles::SetObjectPassable( bool pass )
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_TROLL_BRIDGE:
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
    for ( const TilesAddon & addon : addons_level1 ) {
        if ( addon._objectIcnType == MP2::OBJ_ICN_TYPE_STREAM || ( addon._objectIcnType == MP2::OBJ_ICN_TYPE_OBJNMUL2 && addon._imageIndex < 14 ) ) {
            return true;
        }
    }

    return _objectIcnType == MP2::OBJ_ICN_TYPE_STREAM || ( _objectIcnType == MP2::OBJ_ICN_TYPE_OBJNMUL2 && _imageIndex < 14 );
}

bool Maps::Tiles::isShadow() const
{
    return isShadowSprite( _objectIcnType, _imageIndex )
           && addons_level1.size() == static_cast<size_t>( std::count_if( addons_level1.begin(), addons_level1.end(), TilesAddon::isShadow ) );
}

Maps::TilesAddon * Maps::Tiles::getAddonWithFlag( const uint32_t uid )
{
    auto isFlag = [uid]( const TilesAddon & addon ) { return addon._uid == uid && addon._objectIcnType == MP2::OBJ_ICN_TYPE_FLAG32; };

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
    case MP2::OBJ_MAGIC_GARDEN:
        objectSpriteIndex += 128 + 14;
        updateFlag( color, objectSpriteIndex, _uid, false );
        objectSpriteIndex += 7;
        if ( Maps::isValidDirection( _index, Direction::RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::RIGHT ) );
            tile.updateFlag( color, objectSpriteIndex, _uid, false );
        }
        break;

    case MP2::OBJ_WATER_WHEEL:
    case MP2::OBJ_MINES:
        objectSpriteIndex += 128 + 14;
        if ( Maps::isValidDirection( _index, Direction::TOP ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP ) );
            tile.updateFlag( color, objectSpriteIndex, _uid, true );
        }

        objectSpriteIndex += 7;
        if ( Maps::isValidDirection( _index, Direction::TOP_RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP_RIGHT ) );
            tile.updateFlag( color, objectSpriteIndex, _uid, true );
        }
        break;

    case MP2::OBJ_WINDMILL:
    case MP2::OBJ_LIGHTHOUSE:
        objectSpriteIndex += 128 + 42;
        if ( Maps::isValidDirection( _index, Direction::LEFT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) );
            tile.updateFlag( color, objectSpriteIndex, _uid, false );
        }

        objectSpriteIndex += 7;
        updateFlag( color, objectSpriteIndex, _uid, false );
        break;

    case MP2::OBJ_ALCHEMIST_LAB:
        objectSpriteIndex += 21;
        if ( Maps::isValidDirection( _index, Direction::TOP ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP ) );
            tile.updateFlag( color, objectSpriteIndex, _uid, true );
        }
        break;

    case MP2::OBJ_SAWMILL:
        objectSpriteIndex += 28;
        if ( Maps::isValidDirection( _index, Direction::TOP_RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP_RIGHT ) );
            tile.updateFlag( color, objectSpriteIndex, _uid, true );
        }
        break;

    case MP2::OBJ_CASTLE:
        objectSpriteIndex *= 2;
        if ( Maps::isValidDirection( _index, Direction::LEFT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) );
            tile.updateFlag( color, objectSpriteIndex, _uid, true );
        }

        objectSpriteIndex += 1;
        if ( Maps::isValidDirection( _index, Direction::RIGHT ) ) {
            Maps::Tiles & tile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::RIGHT ) );
            tile.updateFlag( color, objectSpriteIndex, _uid, true );
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
        auto isFlag = [uid]( const TilesAddon & addon ) { return addon._uid == uid && addon._objectIcnType == MP2::OBJ_ICN_TYPE_FLAG32; };
        addons_level1.remove_if( isFlag );
        addons_level2.remove_if( isFlag );
        return;
    }

    TilesAddon * addon = getAddonWithFlag( uid );
    if ( addon != nullptr ) {
        // Replace an existing flag.
        addon->_imageIndex = objectSpriteIndex;
    }
    else if ( setOnUpperLayer ) {
        addons_level2.emplace_back( OBJECT_LAYER, uid, MP2::OBJ_ICN_TYPE_FLAG32, objectSpriteIndex, false, false );
    }
    else {
        addons_level1.emplace_back( OBJECT_LAYER, uid, MP2::OBJ_ICN_TYPE_FLAG32, objectSpriteIndex, false, false );
    }
}

void Maps::Tiles::fixTileObjectType( Tiles & tile )
{
    const MP2::MapObjectType originalObjectType = tile.GetObject( false );

    // Left tile of a skeleton on Desert should be marked as non-action tile.
    if ( originalObjectType == MP2::OBJ_SKELETON && tile._objectIcnType == MP2::OBJ_ICN_TYPE_OBJNDSRT && tile._imageIndex == 83 ) {
        tile.SetObject( MP2::OBJ_NON_ACTION_SKELETON );

        // There is no need to check the rest of things as we fixed this object.
        return;
    }

    // Original Editor marks Reefs as Stones. We're fixing this issue by changing the type of the object without changing the content of a tile.
    // This is also required in order to properly calculate Reefs' passability.
    if ( originalObjectType == MP2::OBJ_ROCK && isValidReefsSprite( tile._objectIcnType, tile._imageIndex ) ) {
        tile.SetObject( MP2::OBJ_REEFS );

        // There is no need to check the rest of things as we fixed this object.
        return;
    }

    // Some maps have water tiles with OBJ_COAST, it shouldn't be, replace OBJ_COAST with OBJ_NONE
    if ( originalObjectType == MP2::OBJ_COAST && tile.isWater() ) {
        Heroes * hero = tile.GetHeroes();

        if ( hero ) {
            hero->SetMapsObject( MP2::OBJ_NONE );
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
    if ( tile._objectIcnType == MP2::OBJ_ICN_TYPE_MONS32 ) {
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
        MP2::MapObjectType objectType = getLoyaltyObject( tile._objectIcnType, tile._imageIndex );
        if ( objectType != MP2::OBJ_NONE ) {
            tile.SetObject( objectType );
            break;
        }

        // Add-ons of level 1 shouldn't even exist if no top object is present. However, let's play safe and verify it as well.
        for ( const TilesAddon & addon : tile.addons_level1 ) {
            objectType = getLoyaltyObject( addon._objectIcnType, addon._imageIndex );
            if ( objectType != MP2::OBJ_NONE )
                break;
        }

        if ( objectType != MP2::OBJ_NONE ) {
            tile.SetObject( objectType );
            break;
        }

        for ( const TilesAddon & addon : tile.addons_level2 ) {
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
                                                << static_cast<int>( tile._imageIndex ) )
        break;
    }

    default:
        break;
    }
}

bool Maps::Tiles::isCaptureObjectProtected() const
{
    const MP2::MapObjectType objectType = GetObject( false );

    if ( !MP2::isCaptureObject( objectType ) ) {
        return false;
    }

    if ( MP2::OBJ_CASTLE == objectType ) {
        Castle * castle = world.getCastleEntrance( GetCenter() );
        assert( castle != nullptr );

        if ( castle ) {
            return castle->GetArmy().isValid();
        }

        return false;
    }

    return getTroopFromTile( *this ).isValid();
}

void Maps::Tiles::Remove( uint32_t uniqID )
{
    addons_level1.remove_if( [uniqID]( const Maps::TilesAddon & v ) { return v.isUniq( uniqID ); } );
    addons_level2.remove_if( [uniqID]( const Maps::TilesAddon & v ) { return v.isUniq( uniqID ); } );

    if ( _uid == uniqID ) {
        resetObjectSprite();
        _uid = 0;
    }
}

void Maps::Tiles::replaceObject( const uint32_t objectUid, const MP2::ObjectIcnType originalObjectIcnType, const MP2::ObjectIcnType newObjectIcnType,
                                 const uint8_t originalImageIndex, const uint8_t newImageIndex )
{
    // We can immediately return from the function as only one object per tile can have the same UID.
    for ( TilesAddon & addon : addons_level1 ) {
        if ( addon._uid == objectUid && addon._objectIcnType == originalObjectIcnType && addon._imageIndex == originalImageIndex ) {
            addon._objectIcnType = newObjectIcnType;
            addon._imageIndex = newImageIndex;
            return;
        }
    }

    for ( TilesAddon & addon : addons_level2 ) {
        if ( addon._uid == objectUid && addon._objectIcnType == originalObjectIcnType && addon._imageIndex == originalImageIndex ) {
            addon._objectIcnType = newObjectIcnType;
            addon._imageIndex = newImageIndex;
            return;
        }
    }

    if ( _uid == objectUid && _objectIcnType == originalObjectIcnType && _imageIndex == originalImageIndex ) {
        _objectIcnType = newObjectIcnType;
        _imageIndex = newImageIndex;
    }
}

void Maps::Tiles::updateObjectImageIndex( const uint32_t objectUid, const MP2::ObjectIcnType objectIcnType, const int imageIndexOffset )
{
    // We can immediately return from the function as only one object per tile can have the same UID.
    for ( TilesAddon & addon : addons_level1 ) {
        if ( addon._uid == objectUid && addon._objectIcnType == objectIcnType ) {
            assert( addon._imageIndex + imageIndexOffset >= 0 && addon._imageIndex + imageIndexOffset < 255 );
            addon._imageIndex = static_cast<uint8_t>( addon._imageIndex + imageIndexOffset );
            return;
        }
    }

    for ( TilesAddon & addon : addons_level2 ) {
        if ( addon._uid == objectUid && addon._objectIcnType == objectIcnType ) {
            assert( addon._imageIndex + imageIndexOffset >= 0 && addon._imageIndex + imageIndexOffset < 255 );
            addon._imageIndex = static_cast<uint8_t>( addon._imageIndex + imageIndexOffset );
            return;
        }
    }

    if ( _uid == objectUid && _objectIcnType == objectIcnType ) {
        assert( _imageIndex + imageIndexOffset >= 0 && _imageIndex + imageIndexOffset < 255 );
        _imageIndex = static_cast<uint8_t>( _imageIndex + imageIndexOffset );
    }
}

void Maps::Tiles::RemoveObjectSprite()
{
    switch ( GetObject() ) {
    case MP2::OBJ_MONSTER:
        Remove( _uid );
        break;
    case MP2::OBJ_JAIL:
        RemoveJailSprite();
        tilePassable = DIRECTION_ALL;
        break;
    case MP2::OBJ_ARTIFACT: {
        const uint32_t uidArtifact = getObjectIdByObjectIcnType( MP2::OBJ_ICN_TYPE_OBJNARTI );
        Remove( uidArtifact );

        if ( Maps::isValidDirection( _index, Direction::LEFT ) )
            world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) ).Remove( uidArtifact );
        break;
    }
    case MP2::OBJ_TREASURE_CHEST:
    case MP2::OBJ_RESOURCE: {
        const uint32_t uidResource = getObjectIdByObjectIcnType( MP2::OBJ_ICN_TYPE_OBJNRSRC );
        Remove( uidResource );

        if ( Maps::isValidDirection( _index, Direction::LEFT ) )
            world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) ).Remove( uidResource );
        break;
    }
    case MP2::OBJ_BARRIER:
        tilePassable = DIRECTION_ALL;
        [[fallthrough]];
    default:
        // remove shadow sprite from left cell
        if ( Maps::isValidDirection( _index, Direction::LEFT ) )
            world.GetTiles( Maps::GetDirectionIndex( _index, Direction::LEFT ) ).Remove( _uid );

        Remove( _uid );
        break;
    }
}

void Maps::Tiles::RemoveJailSprite()
{
    // remove left sprite
    if ( Maps::isValidDirection( _index, Direction::LEFT ) ) {
        const int32_t left = Maps::GetDirectionIndex( _index, Direction::LEFT );
        world.GetTiles( left ).Remove( _uid );

        // remove left left sprite
        if ( Maps::isValidDirection( left, Direction::LEFT ) )
            world.GetTiles( Maps::GetDirectionIndex( left, Direction::LEFT ) ).Remove( _uid );
    }

    // remove top sprite
    if ( Maps::isValidDirection( _index, Direction::TOP ) ) {
        const int32_t top = Maps::GetDirectionIndex( _index, Direction::TOP );
        Maps::Tiles & topTile = world.GetTiles( top );
        topTile.Remove( _uid );

        if ( topTile.GetObject() == MP2::OBJ_JAIL ) {
            topTile.setAsEmpty();
            topTile.FixObject();
        }

        // remove top left sprite
        if ( Maps::isValidDirection( top, Direction::LEFT ) ) {
            Maps::Tiles & leftTile = world.GetTiles( Maps::GetDirectionIndex( top, Direction::LEFT ) );
            leftTile.Remove( _uid );

            if ( leftTile.GetObject() == MP2::OBJ_JAIL ) {
                leftTile.setAsEmpty();
                leftTile.FixObject();
            }
        }
    }

    Remove( _uid );
}

void Maps::Tiles::RestoreAbandonedMine( Tiles & tile, const int resource )
{
    assert( tile.GetObject( false ) == MP2::OBJ_ABANDONED_MINE );
    assert( tile._uid != 0 );

    const payment_t info = ProfitConditions::FromMine( resource );
    std::optional<uint32_t> resourceCount;

    switch ( resource ) {
    case Resource::ORE:
        resourceCount = fheroes2::checkedCast<uint32_t>( info.ore );
        break;
    case Resource::SULFUR:
        resourceCount = fheroes2::checkedCast<uint32_t>( info.sulfur );
        break;
    case Resource::CRYSTAL:
        resourceCount = fheroes2::checkedCast<uint32_t>( info.crystal );
        break;
    case Resource::GEMS:
        resourceCount = fheroes2::checkedCast<uint32_t>( info.gems );
        break;
    case Resource::GOLD:
        resourceCount = fheroes2::checkedCast<uint32_t>( info.gold );
        break;
    default:
        assert( 0 );
        break;
    }

    assert( resourceCount.has_value() && resourceCount > 0U );

    setResourceOnTile( tile, resource, resourceCount.value() );

    auto restoreLeftSprite = [resource]( MP2::ObjectIcnType & objectIcnType, uint8_t & imageIndex ) {
        if ( MP2::OBJ_ICN_TYPE_OBJNGRAS == objectIcnType && imageIndex == 6 ) {
            objectIcnType = MP2::OBJ_ICN_TYPE_MTNGRAS;
            imageIndex = 82;
        }
        else if ( MP2::OBJ_ICN_TYPE_OBJNDIRT == objectIcnType && imageIndex == 8 ) {
            objectIcnType = MP2::OBJ_ICN_TYPE_MTNDIRT;
            imageIndex = 112;
        }
        else if ( MP2::OBJ_ICN_TYPE_EXTRAOVR == objectIcnType && imageIndex == 5 ) {
            switch ( resource ) {
            case Resource::ORE:
                imageIndex = 0;
                break;
            case Resource::SULFUR:
                imageIndex = 1;
                break;
            case Resource::CRYSTAL:
                imageIndex = 2;
                break;
            case Resource::GEMS:
                imageIndex = 3;
                break;
            case Resource::GOLD:
                imageIndex = 4;
                break;
            default:
                break;
            }
        }
    };

    auto restoreRightSprite = []( MP2::ObjectIcnType & objectIcnType, uint8_t & imageIndex ) {
        if ( MP2::OBJ_ICN_TYPE_OBJNDIRT == objectIcnType && imageIndex == 9 ) {
            objectIcnType = MP2::OBJ_ICN_TYPE_MTNDIRT;
            imageIndex = 113;
        }
        else if ( MP2::OBJ_ICN_TYPE_OBJNGRAS == objectIcnType && imageIndex == 7 ) {
            objectIcnType = MP2::OBJ_ICN_TYPE_MTNGRAS;
            imageIndex = 83;
        }
    };

    auto restoreMineObjectType = [&tile]( int directionVector ) {
        if ( Maps::isValidDirection( tile._index, directionVector ) ) {
            Tiles & mineTile = world.GetTiles( Maps::GetDirectionIndex( tile._index, directionVector ) );
            if ( ( mineTile.GetObject() == MP2::OBJ_NON_ACTION_ABANDONED_MINE )
                 && ( mineTile._uid == tile._uid || mineTile.FindAddonLevel1( tile._uid ) || mineTile.FindAddonLevel2( tile._uid ) ) ) {
                mineTile.SetObject( MP2::OBJ_NON_ACTION_MINES );
            }
        }
    };

    restoreLeftSprite( tile._objectIcnType, tile._imageIndex );
    for ( TilesAddon & addon : tile.addons_level1 ) {
        if ( addon._uid == tile._uid ) {
            restoreLeftSprite( addon._objectIcnType, addon._imageIndex );
        }
    }

    if ( Maps::isValidDirection( tile._index, Direction::RIGHT ) ) {
        Tiles & rightTile = world.GetTiles( Maps::GetDirectionIndex( tile._index, Direction::RIGHT ) );

        if ( rightTile._uid == tile._uid ) {
            restoreRightSprite( rightTile._objectIcnType, rightTile._imageIndex );
        }

        TilesAddon * addon = rightTile.FindAddonLevel1( tile._uid );

        if ( addon ) {
            restoreRightSprite( addon->_objectIcnType, addon->_imageIndex );
        }
    }

    restoreMineObjectType( Direction::LEFT );
    restoreMineObjectType( Direction::RIGHT );
    restoreMineObjectType( Direction::TOP );
    restoreMineObjectType( Direction::TOP_LEFT );
    restoreMineObjectType( Direction::TOP_RIGHT );
}

void Maps::Tiles::ClearFog( const int colors )
{
    _fogColors &= ~colors;

    // The fog might be cleared even without the hero's movement - for example, the hero can gain a new level of Scouting
    // skill by picking up a Treasure Chest from a nearby tile or buying a map in a Magellan's Maps object using the space
    // bar button. Reset the pathfinder(s) to make the newly discovered tiles immediately available for this hero.
    world.resetPathfinder();
}

void Maps::Tiles::updateFogDirectionsInArea( const fheroes2::Point & minPos, const fheroes2::Point & maxPos, const int32_t color )
{
    assert( ( minPos.x <= maxPos.x ) && ( minPos.y <= maxPos.y ) );

    const int32_t worldWidth = world.w();
    const int32_t worldHeight = world.w();

    // Do not get over the world borders.
    const int32_t minX = std::max( minPos.x, 0 );
    const int32_t minY = std::max( minPos.y, 0 );
    // Add extra 1 to reach the given maxPos point.
    const int32_t maxX = std::min( maxPos.x + 1, worldWidth );
    const int32_t maxY = std::min( maxPos.y + 1, worldHeight );

    // Fog data range is 1 tile bigger from each side as for the fog directions we have to check all tiles around each tile in the area.
    const int32_t fogMinX = std::max( minX - 1, 0 );
    const int32_t fogMinY = std::max( minY - 1, 0 );
    const int32_t fogMaxX = std::min( maxX + 1, worldWidth );
    const int32_t fogMaxY = std::min( maxY + 1, worldHeight );

    const int32_t fogDataWidth = maxX - minX + 2;
    const int32_t fogDataSize = fogDataWidth * ( maxY - minY + 2 );

    // A vector to cache 'isFog()' data. This vector type is not <bool> as using std::vector<uint8_t> gives the higher performance.
    // 1 is for the 'true' state, 0 is for the 'false' state.
    std::vector<uint8_t> fogData( fogDataSize, 1 );

    // Set the 'fogData' index offset from the tile index.
    const int32_t fogDataOffset = 1 - minX + ( 1 - minY ) * fogDataWidth;

    // Cache the 'fogData' data for the given area to use it in fog direction calculation.
    // The loops run only within the world area, if 'fogData' area includes tiles outside the world borders we do not update them as the are already set to 1.
    for ( int32_t y = fogMinY; y < fogMaxY; ++y ) {
        const int32_t fogTileOffsetY = y * worldWidth;
        const int32_t fogDataOffsetY = y * fogDataWidth + fogDataOffset;

        for ( int32_t x = fogMinX; x < fogMaxX; ++x ) {
            fogData[x + fogDataOffsetY] = world.GetTiles( x + fogTileOffsetY ).isFog( color ) ? 1 : 0;
        }
    }

    // Set the 'fogData' index offset from the tile index for the TOP LEFT direction from the tile.
    const int32_t topLeftDirectionOffset = -1 - fogDataWidth;

#ifndef NDEBUG
    // Cache the maximum border for fogDataIndex corresponding the "CENTER" tile to use in assertion. Should be removed if assertion is removed.
    const int32_t centerfogDataIndexLimit = fogDataSize + topLeftDirectionOffset;
#endif

    // Calculate fog directions using the cached 'isFog' data.
    for ( int32_t y = minY; y < maxY; ++y ) {
        const int32_t fogCenterDataOffsetY = y * fogDataWidth + fogDataOffset;

        for ( int32_t x = minX; x < maxX; ++x ) {
            Maps::Tiles & tile = world.GetTiles( x, y );

            int32_t fogDataIndex = x + fogCenterDataOffsetY;

            if ( fogData[fogDataIndex] == 0 ) {
                // For the tile is without fog we set the UNKNOWN direction.
                tile._setFogDirection( Direction::UNKNOWN );
            }
            else {
                // The tile is under the fog so its CENTER direction for fog is true.
                uint16_t fogDirection = Direction::CENTER;

                // 'fogDataIndex' should not get out of maximum 'fogData' vector index after all increments.
                assert( fogDataIndex < centerfogDataIndexLimit );

                // Check all tiles around for 'fogData' starting from the top left direction and if it is true then logically add the direction to 'fogDirection'.
                fogDataIndex += topLeftDirectionOffset;

                assert( fogDataIndex >= 0 );

                if ( fogData[fogDataIndex] == 1 ) {
                    fogDirection |= Direction::TOP_LEFT;
                }

                ++fogDataIndex;

                if ( fogData[fogDataIndex] == 1 ) {
                    fogDirection |= Direction::TOP;
                }

                ++fogDataIndex;

                if ( fogData[fogDataIndex] == 1 ) {
                    fogDirection |= Direction::TOP_RIGHT;
                }

                // Set index to the left direction tile of the next fog data raw.
                fogDataIndex += fogDataWidth - 2;

                if ( fogData[fogDataIndex] == 1 ) {
                    fogDirection |= Direction::LEFT;
                }

                // Skip the center tile as it was already checked.
                fogDataIndex += 2;

                if ( fogData[fogDataIndex] == 1 ) {
                    fogDirection |= Direction::RIGHT;
                }

                fogDataIndex += fogDataWidth - 2;

                if ( fogData[fogDataIndex] == 1 ) {
                    fogDirection |= Direction::BOTTOM_LEFT;
                }

                ++fogDataIndex;

                if ( fogData[fogDataIndex] == 1 ) {
                    fogDirection |= Direction::BOTTOM;
                }

                ++fogDataIndex;

                if ( fogData[fogDataIndex] == 1 ) {
                    fogDirection |= Direction::BOTTOM_RIGHT;
                }

                tile._setFogDirection( fogDirection );
            }
        }
    }
}

void Maps::Tiles::updateTileById( Maps::Tiles & tile, const uint32_t uid, const uint8_t newIndex )
{
    Maps::TilesAddon * addon = tile.FindAddonLevel1( uid );
    if ( addon != nullptr ) {
        addon->_imageIndex = newIndex;
    }
    else if ( tile._uid == uid ) {
        tile._imageIndex = newIndex;
    }
}

void Maps::Tiles::updateEmpty()
{
    if ( _mainObjectType == MP2::OBJ_NONE ) {
        setAsEmpty();
    }
}

void Maps::Tiles::setAsEmpty()
{
    // If an object is removed we should validate if this tile a potential candidate to be a coast.
    // Check if this tile is not water and it has neighbouring water tiles.
    if ( isWater() ) {
        SetObject( MP2::OBJ_NONE );
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

    SetObject( isCoast ? MP2::OBJ_COAST : MP2::OBJ_NONE );
}

uint32_t Maps::Tiles::getObjectIdByObjectIcnType( const MP2::ObjectIcnType objectIcnType ) const
{
    if ( _objectIcnType == objectIcnType ) {
        return _uid;
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( addon._objectIcnType == objectIcnType ) {
            return addon._uid;
        }
    }

    for ( const TilesAddon & addon : addons_level2 ) {
        if ( addon._objectIcnType == objectIcnType ) {
            return addon._uid;
        }
    }

    return 0;
}

std::vector<MP2::ObjectIcnType> Maps::Tiles::getValidObjectIcnTypes() const
{
    std::vector<MP2::ObjectIcnType> objectIcnTypes;

    if ( _objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN ) {
        objectIcnTypes.emplace_back( _objectIcnType );
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        // If this assertion blows up then you put an empty object into an addon which makes no sense!
        assert( addon._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN );

        objectIcnTypes.emplace_back( addon._objectIcnType );
    }

    for ( const TilesAddon & addon : addons_level2 ) {
        // If this assertion blows up then you put an empty object into an addon which makes no sense!
        assert( addon._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN );

        objectIcnTypes.emplace_back( addon._objectIcnType );
    }

    return objectIcnTypes;
}

bool Maps::Tiles::containsAnyObjectIcnType( const std::vector<MP2::ObjectIcnType> & objectIcnTypes ) const
{
    for ( const MP2::ObjectIcnType objectIcnType : objectIcnTypes ) {
        if ( _objectIcnType == objectIcnType ) {
            return true;
        }

        for ( const TilesAddon & addon : addons_level1 ) {
            if ( addon._objectIcnType == objectIcnType ) {
                return true;
            }
        }

        for ( const TilesAddon & addon : addons_level2 ) {
            if ( addon._objectIcnType == objectIcnType ) {
                return true;
            }
        }
    }

    return false;
}

bool Maps::Tiles::containsSprite( const MP2::ObjectIcnType objectIcnType, const uint32_t imageIdx ) const
{
    if ( _objectIcnType == objectIcnType && imageIdx == _imageIndex ) {
        return true;
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( addon._objectIcnType == objectIcnType && imageIdx == _imageIndex ) {
            return true;
        }
    }

    for ( const TilesAddon & addon : addons_level2 ) {
        if ( addon._objectIcnType == objectIcnType && imageIdx == _imageIndex ) {
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
    if ( _objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN && _imageIndex < 255 && _uid != 0 && ( ( _layerType >> 1 ) & 1 ) == 0 ) {
        tileUIDs.emplace_back( _uid );
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( addon._uid != 0 && ( ( addon._layerType >> 1 ) & 1 ) == 0 ) {
            tileUIDs.emplace_back( addon._uid );
        }
    }

    for ( const TilesAddon & addon : addons_level2 ) {
        if ( addon._uid != 0 && ( ( addon._layerType >> 1 ) & 1 ) == 0 ) {
            tileUIDs.emplace_back( addon._uid );
        }
    }

    const Tiles & topTile = world.GetTiles( Maps::GetDirectionIndex( _index, Direction::TOP ) );
    for ( const uint32_t tileUID : tileUIDs ) {
        if ( topTile._uid == tileUID && !isShadowSprite( topTile._objectIcnType, topTile._imageIndex ) ) {
            return true;
        }

        for ( const TilesAddon & addon : topTile.addons_level1 ) {
            if ( addon._uid == tileUID && !TilesAddon::isShadow( addon ) ) {
                return true;
            }
        }

        for ( const TilesAddon & addon : topTile.addons_level2 ) {
            if ( addon._uid == tileUID && !TilesAddon::isShadow( addon ) ) {
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
        uids.insert( addon._uid );
    }

    for ( const TilesAddon & addon : tile.getLevel2Addons() ) {
        uids.insert( addon._uid );
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
    if ( _uid == objectUID ) {
        return ( ( _layerType >> 1 ) & 1 ) == 0;
    }

    for ( const TilesAddon & addon : addons_level1 ) {
        if ( addon._uid == objectUID ) {
            return ( ( addon._layerType >> 1 ) & 1 ) == 0;
        }
    }

    return false;
}

void Maps::Tiles::swap( TilesAddon & addon ) noexcept
{
    std::swap( addon._objectIcnType, _objectIcnType );
    std::swap( addon._imageIndex, _imageIndex );
    std::swap( addon._uid, _uid );
    std::swap( addon._layerType, _layerType );
    std::swap( addon._hasObjectAnimation, _hasObjectAnimation );
    std::swap( addon._isMarkedAsRoad, _isMarkedAsRoad );
}

StreamBase & Maps::operator<<( StreamBase & msg, const TilesAddon & ta )
{
    using ObjectIcnTypeUnderlyingType = std::underlying_type_t<decltype( ta._objectIcnType )>;

    return msg << ta._layerType << ta._uid << static_cast<ObjectIcnTypeUnderlyingType>( ta._objectIcnType ) << ta._hasObjectAnimation << ta._isMarkedAsRoad
               << ta._imageIndex;
}

StreamBase & Maps::operator>>( StreamBase & msg, TilesAddon & ta )
{
    msg >> ta._layerType >> ta._uid;

    using ObjectIcnTypeUnderlyingType = std::underlying_type_t<decltype( ta._objectIcnType )>;
    static_assert( std::is_same_v<ObjectIcnTypeUnderlyingType, uint8_t>, "Type of _objectIcnType has been changed, check the logic below" );

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1001_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1001_RELEASE ) {
        ObjectIcnTypeUnderlyingType objectIcnType = MP2::OBJ_ICN_TYPE_UNKNOWN;
        msg >> objectIcnType;

        ta._objectIcnType = static_cast<MP2::ObjectIcnType>( objectIcnType >> 2 );

        ta._hasObjectAnimation = ( objectIcnType & 1 ) != 0;
        ta._isMarkedAsRoad = ( objectIcnType & 2 ) != 0;
    }
    else {
        ObjectIcnTypeUnderlyingType objectIcnType = MP2::OBJ_ICN_TYPE_UNKNOWN;
        msg >> objectIcnType;

        ta._objectIcnType = static_cast<MP2::ObjectIcnType>( objectIcnType );

        msg >> ta._hasObjectAnimation >> ta._isMarkedAsRoad;
    }

    msg >> ta._imageIndex;

    return msg;
}

StreamBase & Maps::operator<<( StreamBase & msg, const Tiles & tile )
{
    using ObjectIcnTypeUnderlyingType = std::underlying_type_t<decltype( tile._objectIcnType )>;
    using MainObjectTypeUnderlyingType = std::underlying_type_t<decltype( tile._mainObjectType )>;

    return msg << tile._index << tile._terrainImageIndex << tile._terrainFlags << tile.tilePassable << tile._uid
               << static_cast<ObjectIcnTypeUnderlyingType>( tile._objectIcnType ) << tile._hasObjectAnimation << tile._isMarkedAsRoad << tile._imageIndex
               << static_cast<MainObjectTypeUnderlyingType>( tile._mainObjectType ) << tile._fogColors << tile._metadata << tile.heroID << tile.tileIsRoad
               << tile.addons_level1 << tile.addons_level2 << tile._layerType << tile._boatOwnerColor;
}

StreamBase & Maps::operator>>( StreamBase & msg, Tiles & tile )
{
    msg >> tile._index;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE2_1001_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE2_1001_RELEASE ) {
        // In old save format terrain information is stored in a very fuzzy way.
        uint16_t temp = 0;
        msg >> temp;

        tile._terrainImageIndex = ( temp & 0x3FFF );
        tile._terrainFlags = ( temp >> 14 );
    }
    else {
        msg >> tile._terrainImageIndex >> tile._terrainFlags;
    }

    msg >> tile.tilePassable >> tile._uid;

    using ObjectIcnTypeUnderlyingType = std::underlying_type_t<decltype( tile._objectIcnType )>;
    static_assert( std::is_same_v<ObjectIcnTypeUnderlyingType, uint8_t>, "Type of _objectIcnType has been changed, check the logic below" );

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1001_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1001_RELEASE ) {
        ObjectIcnTypeUnderlyingType objectIcnType = MP2::OBJ_ICN_TYPE_UNKNOWN;
        msg >> objectIcnType;

        tile._objectIcnType = static_cast<MP2::ObjectIcnType>( objectIcnType >> 2 );

        tile._hasObjectAnimation = ( objectIcnType & 1 ) != 0;
        tile._isMarkedAsRoad = ( objectIcnType & 2 ) != 0;
    }
    else {
        ObjectIcnTypeUnderlyingType objectIcnType = MP2::OBJ_ICN_TYPE_UNKNOWN;
        msg >> objectIcnType;

        tile._objectIcnType = static_cast<MP2::ObjectIcnType>( objectIcnType );

        msg >> tile._hasObjectAnimation >> tile._isMarkedAsRoad;
    }

    msg >> tile._imageIndex;

    using MainObjectTypeUnderlyingType = std::underlying_type_t<decltype( tile._mainObjectType )>;
    static_assert( std::is_same_v<MainObjectTypeUnderlyingType, uint8_t>, "Type of _mainObjectType has been changed, check the logic below" );

    MainObjectTypeUnderlyingType mainObjectType = MP2::OBJ_NONE;
    msg >> mainObjectType;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE1_1001_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE1_1001_RELEASE ) {
        mainObjectType = Tiles::convertOldMainObjectType( mainObjectType );
    }

    tile._mainObjectType = static_cast<MP2::MapObjectType>( mainObjectType );

    msg >> tile._fogColors;
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1004_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1004_RELEASE ) {
        uint8_t quantity1 = 0;
        uint8_t quantity2 = 0;
        uint32_t additionalMetadata = 0;

        msg >> quantity1 >> quantity2 >> additionalMetadata;

        world.setOldTileQuantityData( tile.GetIndex(), quantity1, quantity2, additionalMetadata );
    }
    else {
        // We want to verify the size of array being present in the file.
        std::vector<uint32_t> temp;
        msg >> temp;

        if ( tile._metadata.size() != temp.size() ) {
            // This is a corrupted file!
            assert( 0 );
        }
        else {
            std::copy_n( temp.begin(), tile._metadata.size(), tile._metadata.begin() );
        }
    }

    msg >> tile.heroID >> tile.tileIsRoad >> tile.addons_level1 >> tile.addons_level2 >> tile._layerType;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1002_RELEASE, "Remove the check below." );
    if ( Game::GetVersionOfCurrentSaveFile() >= FORMAT_VERSION_1002_RELEASE ) {
        msg >> tile._boatOwnerColor;
    }

    return msg;
}

uint8_t Maps::Tiles::convertOldMainObjectType( const uint8_t mainObjectType )
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE1_1001_RELEASE, "Remove this method." );

    if ( mainObjectType == 128 ) {
        // This is an old Sea Chest object type.
        return MP2::OBJ_SEA_CHEST;
    }
    if ( mainObjectType == 235 ) {
        // This is an old non-action Stables object type.
        return MP2::OBJ_NON_ACTION_STABLES;
    }
    if ( mainObjectType == 241 ) {
        // This is an old action Stables object type.
        return MP2::OBJ_STABLES;
    }
    if ( mainObjectType == 234 ) {
        // This is an old non-action Alchemist Tower object type.
        return MP2::OBJ_NON_ACTION_ALCHEMIST_TOWER;
    }
    if ( mainObjectType == 240 ) {
        // This is an old action Alchemist Tower object type.
        return MP2::OBJ_ALCHEMIST_TOWER;
    }
    if ( mainObjectType == 118 ) {
        // This is an old non-action The Hut of Magi object type.
        return MP2::OBJ_NON_ACTION_HUT_OF_MAGI;
    }
    if ( mainObjectType == 238 ) {
        // This is an old action The Hut of Magi object type.
        return MP2::OBJ_HUT_OF_MAGI;
    }
    if ( mainObjectType == 119 ) {
        // This is an old non-action The Eye of Magi object type.
        return MP2::OBJ_NON_ACTION_EYE_OF_MAGI;
    }
    if ( mainObjectType == 239 ) {
        // This is an old action The Eye of Magi object type.
        return MP2::OBJ_EYE_OF_MAGI;
    }
    if ( mainObjectType == 233 ) {
        // This is an old non-action Reefs object type.
        return MP2::OBJ_REEFS;
    }
    if ( mainObjectType == 65 ) {
        // This is an old non-action Thatched Hut object type.
        return MP2::OBJ_NON_ACTION_PEASANT_HUT;
    }
    if ( mainObjectType == 193 ) {
        // This is an old action Thatched Hut object type.
        return MP2::OBJ_PEASANT_HUT;
    }
    if ( mainObjectType == 117 ) {
        // This is an old non-action Sirens object type.
        return MP2::OBJ_NON_ACTION_SIRENS;
    }
    if ( mainObjectType == 237 ) {
        // This is an old action Sirens object type.
        return MP2::OBJ_SIRENS;
    }
    if ( mainObjectType == 116 ) {
        // This is an old non-action Mermaid object type.
        return MP2::OBJ_NON_ACTION_MERMAID;
    }
    if ( mainObjectType == 236 ) {
        // This is an old non-action Mermaid object type.
        return MP2::OBJ_MERMAID;
    }

    return mainObjectType;
}

void Maps::Tiles::quantityIntoMetadata( const uint8_t quantityValue1, const uint8_t quantityValue2, const uint32_t additionalMetadata )
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1004_RELEASE, "Remove this method." );

    // The object could be under a hero so ignore hero.
    const MP2::MapObjectType objectType = GetObject( false );

    if ( !MP2::isActionObject( objectType ) ) {
        // A non-action object has no metadata.
        return;
    }

    // Old format contained Gold values divided by 100 due to uint8_t limitation. We don't have such limitation anymore.

    switch ( objectType ) {
    // Alchemist Lab, Sawmill and Mines have first value as a resource type and the second value as resource count per day.
    case MP2::OBJ_ALCHEMIST_LAB:
    case MP2::OBJ_MINES:
    case MP2::OBJ_SAWMILL:
        _metadata[0] = quantityValue1;
        if ( quantityValue1 == Resource::GOLD ) {
            _metadata[1] = quantityValue2 * 100;
        }
        else {
            _metadata[1] = quantityValue2;
        }

        if ( _metadata[1] == 0 ) {
            // This is a broken mine from old saves. Let's try to correct income.
            if ( Funds{ static_cast<int>( _metadata[0] ), 1 }.GetValidItemsCount() == 1 ) {
                const payment_t income = ProfitConditions::FromMine( static_cast<int>( _metadata[0] ) );
                _metadata[1] = income.Get( static_cast<int>( _metadata[0] ) );
            }
            else {
                // This is definitely not a mine.
                SetObject( MP2::OBJ_NONE );
            }
        }

        _metadata[2] = additionalMetadata;
        break;

    // Abandoned mine was mixed with Mines in the old save formats.
    case MP2::OBJ_ABANDONED_MINE:
        if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1003_RELEASE ) {
            _metadata[0] = quantityValue1;
            _metadata[1] = quantityValue2;
            _metadata[2] = additionalMetadata;
        }
        else {
            _metadata[0] = ( static_cast<uint32_t>( quantityValue1 ) << 8 ) + quantityValue2;
            _metadata[2] = additionalMetadata;
        }
        break;

    // Monster dwellings always store only one value - the number of monsters.
    case MP2::OBJ_AIR_ALTAR:
    case MP2::OBJ_ARCHER_HOUSE:
    case MP2::OBJ_BARROW_MOUNDS:
    case MP2::OBJ_CAVE:
    case MP2::OBJ_CITY_OF_DEAD:
    case MP2::OBJ_DESERT_TENT:
    case MP2::OBJ_DRAGON_CITY:
    case MP2::OBJ_DWARF_COTTAGE:
    case MP2::OBJ_EARTH_ALTAR:
    case MP2::OBJ_EXCAVATION:
    case MP2::OBJ_FIRE_ALTAR:
    case MP2::OBJ_GOBLIN_HUT:
    case MP2::OBJ_HALFLING_HOLE:
    case MP2::OBJ_PEASANT_HUT:
    case MP2::OBJ_RUINS:
    case MP2::OBJ_TREE_CITY:
    case MP2::OBJ_TREE_HOUSE:
    case MP2::OBJ_TROLL_BRIDGE:
    case MP2::OBJ_WAGON_CAMP:
    case MP2::OBJ_WATCH_TOWER:
    case MP2::OBJ_WATER_ALTAR:
        _metadata[0] = ( static_cast<uint32_t>( quantityValue1 ) << 8 ) + quantityValue2;
        break;

    // Genie's Lamp must have some monsters inside otherwise this object should not exist on Adventure Map.
    case MP2::OBJ_GENIE_LAMP:
        _metadata[0] = ( static_cast<uint32_t>( quantityValue1 ) << 8 ) + quantityValue2;
        assert( _metadata[0] > 0 );
        break;

    // Shrines as well as Pyramid always contain one type of spell.
    case MP2::OBJ_SHRINE_FIRST_CIRCLE:
    case MP2::OBJ_SHRINE_SECOND_CIRCLE:
    case MP2::OBJ_SHRINE_THIRD_CIRCLE:
    case MP2::OBJ_PYRAMID:
        _metadata[0] = quantityValue1;
        break;

    // Monster object store the number of monsters (which must be bigger than 0) and join condition type.
    case MP2::OBJ_MONSTER:
        _metadata[0] = ( static_cast<uint32_t>( quantityValue1 ) << 8 ) + quantityValue2;
        _metadata[2] = additionalMetadata;
        break;

    // Resource contains the type and the amount.
    case MP2::OBJ_RESOURCE:
        _metadata[0] = quantityValue1;
        if ( quantityValue1 == Resource::GOLD ) {
            _metadata[1] = quantityValue2 * 100;
        }
        else {
            _metadata[1] = quantityValue2;
        }
        break;

    // Barrier and Traveler's Tent contain color.
    case MP2::OBJ_BARRIER:
    case MP2::OBJ_TRAVELLER_TENT:
        _metadata[0] = quantityValue1;
        break;

    // Tree of Knowledge contains either nothing for free level up or the amount of required resources.
    case MP2::OBJ_TREE_OF_KNOWLEDGE:
        _metadata[0] = quantityValue1;
        if ( quantityValue1 == Resource::GOLD ) {
            _metadata[1] = quantityValue2 * 100;
        }
        else {
            _metadata[1] = quantityValue2;
        }
        break;

    // Witch's Hut contains a basic level of  a secondary skill.
    case MP2::OBJ_WITCHS_HUT:
        _metadata[0] = quantityValue1;
        break;

    // Magic Garden and Water Wheel either contain nothing when it was visited or some resources.
    case MP2::OBJ_MAGIC_GARDEN:
    case MP2::OBJ_WATER_WHEEL:
        _metadata[0] = quantityValue1;
        if ( quantityValue1 == Resource::GOLD ) {
            _metadata[1] = quantityValue2 * 100;
        }
        else {
            _metadata[1] = quantityValue2;
        }
        break;

    // Skeleton contains an artifact.
    case MP2::OBJ_SKELETON:
        _metadata[0] = quantityValue1;
        break;

    // Lean-To contains one resource type and its amount.
    case MP2::OBJ_LEAN_TO:
        _metadata[0] = quantityValue1;
        if ( quantityValue1 == Resource::GOLD ) {
            _metadata[1] = quantityValue2 * 100;
        }
        else {
            _metadata[1] = quantityValue2;
        }
        break;

    // Wagon can contain either an artifact or a resource.
    case MP2::OBJ_WAGON:
        if ( quantityValue2 > 0 ) {
            _metadata[0] = Artifact::UNKNOWN;
            _metadata[1] = quantityValue1;
            if ( quantityValue1 == Resource::GOLD ) {
                _metadata[2] = quantityValue2 * 100;
            }
            else {
                _metadata[2] = quantityValue2;
            }
        }
        else {
            _metadata[0] = quantityValue1;
        }
        break;

    // Flotsam can contain Wood and Gold.
    case MP2::OBJ_FLOTSAM:
        _metadata[0] = quantityValue1;
        _metadata[1] = quantityValue2 * 100;
        break;

    // Treasure and Sea Chests can contain an artifact and gold.
    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_SEA_CHEST:
    case MP2::OBJ_TREASURE_CHEST:
        _metadata[0] = quantityValue1;
        _metadata[1] = quantityValue2 * 100;
        break;

    // Derelict Ship always has only Gold.
    case MP2::OBJ_DERELICT_SHIP:
        _metadata[0] = quantityValue1;
        if ( quantityValue1 == Resource::GOLD ) {
            _metadata[1] = quantityValue2 * 100;
        }
        else {
            _metadata[1] = quantityValue2;
        }
        break;

    // Daemon Cave is tricky: it can contain experience, gold and artifact.
    case MP2::OBJ_DAEMON_CAVE:
        _metadata[0] = quantityValue1;
        _metadata[1] = ( 0x0f & quantityValue2 ) * 100;
        _metadata[2] = ( quantityValue2 >> 4 );
        break;

    // Campfire contains some random resources and gold which has the same value as resource but multiplied by 100.
    case MP2::OBJ_CAMPFIRE:
        _metadata[0] = quantityValue1;
        _metadata[1] = quantityValue2;
        break;

    // Windmill contains some resources.
    case MP2::OBJ_WINDMILL:
        _metadata[0] = quantityValue1;
        if ( quantityValue1 == Resource::GOLD ) {
            _metadata[1] = quantityValue2 * 100;
        }
        else {
            _metadata[1] = quantityValue2;
        }
        break;

    // Artifact contains artifact ID, possible resources and condition to grab it.
    case MP2::OBJ_ARTIFACT:
        _metadata[0] = quantityValue1;
        _metadata[1] = ( 0x0f & quantityValue2 );
        _metadata[2] = ( quantityValue2 >> 4 );
        break;

    // Shipwreck Survivor has an artifact.
    case MP2::OBJ_SHIPWRECK_SURVIVOR:
        _metadata[0] = quantityValue1;
        break;

    // Shipwreck contains Gold, Artifact and winning conditions. However, old format did not store the amount of Gold, we need to add it.
    case MP2::OBJ_SHIPWRECK:
        _metadata[0] = quantityValue1;
        _metadata[2] = ( quantityValue2 >> 4 );
        switch ( static_cast<ShipwreckCaptureCondition>( _metadata[2] ) ) {
        case ShipwreckCaptureCondition::EMPTY:
            // 103 is old Artifact::UNKNOWN value.
            assert( _metadata[0] == 103 );
            break;
        case ShipwreckCaptureCondition::FIGHT_10_GHOSTS_AND_GET_1000_GOLD:
            _metadata[1] = 1000;
            break;
        case ShipwreckCaptureCondition::FIGHT_15_GHOSTS_AND_GET_2000_GOLD:
            _metadata[1] = 2000;
            break;
        case ShipwreckCaptureCondition::FIGHT_25_GHOSTS_AND_GET_5000_GOLD:
            _metadata[1] = 5000;
            break;
        case ShipwreckCaptureCondition::FIGHT_50_GHOSTS_AND_GET_2000_GOLD_WITH_ARTIFACT:
            _metadata[1] = 2000;
            break;
        default:
            // This is an invalid case!
            assert( 0 );
            break;
        }

        break;

    // These objects should not have any metadata.
    case MP2::OBJ_ACTION_CACTUS:
    case MP2::OBJ_ACTION_COAST:
    case MP2::OBJ_ACTION_CRATER:
    case MP2::OBJ_ACTION_DEAD_TREE:
    case MP2::OBJ_ACTION_DUNE:
    case MP2::OBJ_ACTION_FLOWERS:
    case MP2::OBJ_ACTION_LAVAPOOL:
    case MP2::OBJ_ACTION_MANDRAKE:
    case MP2::OBJ_ACTION_MOSSY_ROCK:
    case MP2::OBJ_ACTION_MOUND:
    case MP2::OBJ_ACTION_MOUNTAINS:
    case MP2::OBJ_ACTION_NOTHING_SPECIAL:
    case MP2::OBJ_ACTION_REEFS:
    case MP2::OBJ_ACTION_ROCK:
    case MP2::OBJ_ACTION_SHRUB:
    case MP2::OBJ_ACTION_STUMP:
    case MP2::OBJ_ACTION_TAR_PIT:
    case MP2::OBJ_ACTION_TREES:
    case MP2::OBJ_ACTION_VOLCANO:
    case MP2::OBJ_ACTION_WATER_LAKE:
    case MP2::OBJ_ALCHEMIST_TOWER:
    case MP2::OBJ_ARENA:
    case MP2::OBJ_ARTESIAN_SPRING:
    case MP2::OBJ_BOAT:
    case MP2::OBJ_BUOY:
    case MP2::OBJ_EYE_OF_MAGI:
    case MP2::OBJ_FAERIE_RING:
    case MP2::OBJ_FORT:
    case MP2::OBJ_FOUNTAIN:
    case MP2::OBJ_FREEMANS_FOUNDRY:
    case MP2::OBJ_GAZEBO:
    case MP2::OBJ_HILL_FORT:
    case MP2::OBJ_HUT_OF_MAGI:
    case MP2::OBJ_IDOL:
    case MP2::OBJ_LIGHTHOUSE:
    case MP2::OBJ_MAGELLANS_MAPS:
    case MP2::OBJ_MAGIC_WELL:
    case MP2::OBJ_MERCENARY_CAMP:
    case MP2::OBJ_MERMAID:
    case MP2::OBJ_OASIS:
    case MP2::OBJ_OBELISK:
    case MP2::OBJ_OBSERVATION_TOWER:
    case MP2::OBJ_ORACLE:
    case MP2::OBJ_SIRENS:
    case MP2::OBJ_STABLES:
    case MP2::OBJ_STANDING_STONES:
    case MP2::OBJ_STONE_LITHS:
    case MP2::OBJ_TEMPLE:
    case MP2::OBJ_TRADING_POST:
    case MP2::OBJ_WATERING_HOLE:
    case MP2::OBJ_WHIRLPOOL:
    case MP2::OBJ_WITCH_DOCTORS_HUT:
    case MP2::OBJ_XANADU:
        break;

    // Metadata for these objects is stored outside this class.
    case MP2::OBJ_BOTTLE:
    case MP2::OBJ_CASTLE:
    case MP2::OBJ_EVENT:
    case MP2::OBJ_HEROES:
    case MP2::OBJ_JAIL:
    case MP2::OBJ_SIGN:
    case MP2::OBJ_SPHINX:
        assert( MP2::doesObjectNeedExtendedMetadata( objectType ) );
        break;

    // These objects must not even exist in a save file.
    case MP2::OBJ_EXPANSION_DWELLING:
    case MP2::OBJ_EXPANSION_OBJECT:
    case MP2::OBJ_RANDOM_ARTIFACT:
    case MP2::OBJ_RANDOM_ARTIFACT_MAJOR:
    case MP2::OBJ_RANDOM_ARTIFACT_MINOR:
    case MP2::OBJ_RANDOM_ARTIFACT_TREASURE:
    case MP2::OBJ_RANDOM_CASTLE:
    case MP2::OBJ_RANDOM_MONSTER:
    case MP2::OBJ_RANDOM_MONSTER_MEDIUM:
    case MP2::OBJ_RANDOM_MONSTER_STRONG:
    case MP2::OBJ_RANDOM_MONSTER_VERY_STRONG:
    case MP2::OBJ_RANDOM_MONSTER_WEAK:
    case MP2::OBJ_RANDOM_RESOURCE:
    case MP2::OBJ_RANDOM_TOWN:
    case MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT:
        assert( 0 );
        break;

    default:
        // Did you add a new action object on Adventure Map? Add the logic!
        assert( 0 );
        break;
    }
}

void Maps::Tiles::fixOldArtifactIDs()
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE1_1005_RELEASE, "Remove this method." );

    // The object could be under a hero so ignore hero.
    const MP2::MapObjectType objectType = GetObject( false );

    switch ( objectType ) {
    case MP2::OBJ_ARTIFACT:
        assert( _metadata[0] < 103 );
        ++_metadata[0];
        break;
    case MP2::OBJ_DAEMON_CAVE:
    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_SEA_CHEST:
    case MP2::OBJ_SHIPWRECK:
    case MP2::OBJ_SHIPWRECK_SURVIVOR:
    case MP2::OBJ_SKELETON:
    case MP2::OBJ_TREASURE_CHEST:
    case MP2::OBJ_WAGON:
        if ( _metadata[0] == 103 ) {
            _metadata[0] = Artifact::UNKNOWN;
        }
        else {
            ++_metadata[0];
        }
        break;
    default:
        break;
    }
}
