/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <cassert>
#include <cstdint>
#include <limits>
#include <list>
#include <optional>
#include <ostream>
#include <type_traits>
#include <utility>

#include "army_troop.h"
#include "artifact.h"
#include "color.h"
#include "logging.h"
#include "maps_tiles.h"
#include "monster.h"
#include "mp2.h"
#include "pairs.h"
#include "payment.h"
#include "profit.h"
#include "rand.h"
#include "resource.h"
#include "skill.h"
#include "spell.h"
#include "tools.h"
#include "week.h"
#include "world.h"

bool Maps::Tiles::QuantityIsValid() const
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_ARTIFACT:
    case MP2::OBJ_RESOURCE:
    case MP2::OBJ_CAMPFIRE:
    case MP2::OBJ_FLOTSAM:
    case MP2::OBJ_SHIPWRECK_SURVIVOR:
    case MP2::OBJ_TREASURE_CHEST:
    case MP2::OBJ_SEA_CHEST:
        return true;

    case MP2::OBJ_PYRAMID:
        return getSpellFromTile( *this ).isValid();

    case MP2::OBJ_SHIPWRECK:
    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_DERELICT_SHIP:
    case MP2::OBJ_WATER_WHEEL:
    case MP2::OBJ_WINDMILL:
    case MP2::OBJ_LEAN_TO:
    case MP2::OBJ_MAGIC_GARDEN:
        return quantity2 != 0;

    case MP2::OBJ_SKELETON:
        return getArtifactFromTile( *this ) != Artifact::UNKNOWN;

    case MP2::OBJ_WAGON:
        return getArtifactFromTile( *this ) != Artifact::UNKNOWN || quantity2 != 0;

    case MP2::OBJ_DAEMON_CAVE:
        return QuantityVariant() != 0;

    default:
        break;
    }

    return false;
}

int Maps::Tiles::QuantityVariant() const
{
    return quantity2 >> 4;
}

int Maps::Tiles::QuantityExt() const
{
    return 0x0f & quantity2;
}

void Maps::Tiles::QuantitySetVariant( int variant )
{
    quantity2 &= 0x0f;
    quantity2 |= variant << 4;
}

void Maps::Tiles::QuantitySetExt( int ext )
{
    quantity2 &= 0xf0;
    quantity2 |= ( 0x0f & ext );
}

Skill::Secondary Maps::Tiles::QuantitySkill() const
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_ARTIFACT:
        switch ( QuantityVariant() ) {
        case 4:
            return Skill::Secondary( Skill::Secondary::LEADERSHIP, Skill::Level::BASIC );
        case 5:
            return Skill::Secondary( Skill::Secondary::WISDOM, Skill::Level::BASIC );
        default:
            break;
        }
        break;

    case MP2::OBJ_WITCHS_HUT:
        return Skill::Secondary( quantity1, Skill::Level::BASIC );

    default:
        break;
    }

    return Skill::Secondary();
}

void Maps::Tiles::QuantitySetSkill( int skill )
{
    using Quantity1Type = decltype( quantity1 );
    static_assert( std::is_same_v<Quantity1Type, uint8_t>, "Type of quantity1 has been changed, check the logic below" );

    switch ( GetObject( false ) ) {
    case MP2::OBJ_WITCHS_HUT:
        assert( skill >= std::numeric_limits<Quantity1Type>::min() && skill <= std::numeric_limits<Quantity1Type>::max() );

        quantity1 = static_cast<Quantity1Type>( skill );
        break;

    default:
        break;
    }
}

void Maps::Tiles::QuantitySetSpell( int spell )
{
    using Quantity1Type = decltype( quantity1 );
    static_assert( std::is_same_v<Quantity1Type, uint8_t>, "Type of quantity1 has been changed, check the logic below" );

    switch ( GetObject( false ) ) {
    case MP2::OBJ_ARTIFACT:
    case MP2::OBJ_SHRINE_FIRST_CIRCLE:
    case MP2::OBJ_SHRINE_SECOND_CIRCLE:
    case MP2::OBJ_SHRINE_THIRD_CIRCLE:
    case MP2::OBJ_PYRAMID:
        assert( spell >= std::numeric_limits<Quantity1Type>::min() && spell <= std::numeric_limits<Quantity1Type>::max() );

        quantity1 = static_cast<Quantity1Type>( spell );
        break;

    default:
        break;
    }
}

void Maps::Tiles::QuantitySetArtifact( int art )
{
    using Quantity1Type = decltype( quantity1 );
    static_assert( std::is_same_v<Quantity1Type, uint8_t>, "Type of quantity1 has been changed, check the logic below" );

    assert( art >= std::numeric_limits<Quantity1Type>::min() && art <= std::numeric_limits<Quantity1Type>::max() );

    quantity1 = static_cast<Quantity1Type>( art );
}

void Maps::Tiles::QuantitySetResource( int res, uint32_t count )
{
    using Quantity1Type = decltype( quantity1 );
    using Quantity2Type = decltype( quantity2 );
    static_assert( std::is_same_v<Quantity1Type, uint8_t> && std::is_same_v<Quantity2Type, uint8_t>,
                   "Types of tile's quantities have been changed, check the logic below" );

    assert( res >= std::numeric_limits<Quantity1Type>::min() && res <= std::numeric_limits<Quantity1Type>::max() );

    quantity1 = static_cast<Quantity1Type>( res );

    if ( res == Resource::GOLD ) {
        count = count / 100;
    }

    assert( count >= std::numeric_limits<Quantity2Type>::min() && count <= std::numeric_limits<Quantity2Type>::max() );

    quantity2 = static_cast<Quantity2Type>( count );
}

uint32_t Maps::Tiles::QuantityGold() const
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_ARTIFACT:
        switch ( QuantityVariant() ) {
        case 1:
            return 2000;
        case 2:
            return 2500;
        case 3:
            return 3000;
        default:
            break;
        }
        break;

    case MP2::OBJ_RESOURCE:
    case MP2::OBJ_MAGIC_GARDEN:
    case MP2::OBJ_WATER_WHEEL:
    case MP2::OBJ_TREE_OF_KNOWLEDGE:
        return quantity1 == Resource::GOLD ? 100 * quantity2 : 0;

    case MP2::OBJ_FLOTSAM:
    case MP2::OBJ_CAMPFIRE:
    case MP2::OBJ_SEA_CHEST:
    case MP2::OBJ_TREASURE_CHEST:
    case MP2::OBJ_DERELICT_SHIP:
    case MP2::OBJ_GRAVEYARD:
        return 100 * quantity2;

    case MP2::OBJ_DAEMON_CAVE:
        switch ( QuantityVariant() ) {
        case 2:
        case 4:
            return 2500;
        default:
            break;
        }
        break;

    case MP2::OBJ_SHIPWRECK:
        switch ( QuantityVariant() ) {
        case 1:
            return 1000;
        case 2:
        case 4:
            // Case 4 gives 2000 gold and an artifact.
            return 2000;
        case 3:
            return 5000;
        default:
            break;
        }
        break;

    default:
        break;
    }

    return 0;
}

ResourceCount Maps::Tiles::QuantityResourceCount() const
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_ARTIFACT:
        switch ( QuantityVariant() ) {
        case 1:
            return ResourceCount( Resource::GOLD, QuantityGold() );
        case 2:
            return ResourceCount( Resource::getResourceTypeFromIconIndex( QuantityExt() - 1 ), 3 );
        case 3:
            return ResourceCount( Resource::getResourceTypeFromIconIndex( QuantityExt() - 1 ), 5 );
        default:
            break;
        }
        break;

    case MP2::OBJ_SEA_CHEST:
    case MP2::OBJ_TREASURE_CHEST:
        return ResourceCount( Resource::GOLD, QuantityGold() );

    case MP2::OBJ_FLOTSAM:
        return ResourceCount( Resource::WOOD, quantity1 );

    default:
        break;
    }

    return ResourceCount( quantity1, Resource::GOLD == quantity1 ? QuantityGold() : quantity2 );
}

Funds Maps::Tiles::QuantityFunds() const
{
    const ResourceCount & rc = QuantityResourceCount();

    switch ( GetObject( false ) ) {
    case MP2::OBJ_ARTIFACT:
        switch ( QuantityVariant() ) {
        case 1:
            return Funds( rc );
        case 2:
        case 3:
            return Funds( Resource::GOLD, QuantityGold() ) + Funds( rc );
        default:
            break;
        }
        break;

    case MP2::OBJ_CAMPFIRE:
        return Funds( Resource::GOLD, QuantityGold() ) + Funds( rc );

    case MP2::OBJ_FLOTSAM:
        return Funds( Resource::GOLD, QuantityGold() ) + Funds( Resource::WOOD, quantity1 );

    case MP2::OBJ_SEA_CHEST:
    case MP2::OBJ_TREASURE_CHEST:
    case MP2::OBJ_DERELICT_SHIP:
    case MP2::OBJ_SHIPWRECK:
    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_DAEMON_CAVE:
        return Funds( Resource::GOLD, QuantityGold() );

    default:
        break;
    }

    return Funds( rc );
}

void Maps::Tiles::QuantitySetColor( int col )
{
    using Quantity1Type = decltype( quantity1 );
    static_assert( std::is_same_v<Quantity1Type, uint8_t>, "Type of quantity1 has been changed, check the logic below" );

    switch ( GetObject( false ) ) {
    case MP2::OBJ_BARRIER:
    case MP2::OBJ_TRAVELLER_TENT:
        assert( col >= std::numeric_limits<Quantity1Type>::min() && col <= std::numeric_limits<Quantity1Type>::max() );

        quantity1 = static_cast<Quantity1Type>( col );
        break;

    default:
        world.CaptureObject( GetIndex(), col );
        break;
    }
}

int Maps::Tiles::QuantityColor() const
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_BARRIER:
    case MP2::OBJ_TRAVELLER_TENT:
        return quantity1;

    default:
        return world.ColorCapturedObject( GetIndex() );
    }
}

Troop Maps::Tiles::QuantityTroop() const
{
    return MP2::isCaptureObject( GetObject( false ) ) ? world.GetCapturedObject( GetIndex() ).GetTroop() : Troop( getMonsterFromTile( *this ), MonsterCount() );
}

void Maps::Tiles::QuantityReset()
{
    // TODO: don't modify first 2 bits of quantity1.
    quantity1 = 0;
    quantity2 = 0;

    switch ( GetObject( false ) ) {
    case MP2::OBJ_SKELETON:
    case MP2::OBJ_WAGON:
    case MP2::OBJ_ARTIFACT:
    case MP2::OBJ_SHIPWRECK_SURVIVOR:
    case MP2::OBJ_SEA_CHEST:
    case MP2::OBJ_TREASURE_CHEST:
    case MP2::OBJ_SHIPWRECK:
    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_DAEMON_CAVE:
        QuantitySetArtifact( Artifact::UNKNOWN );
        break;

    default:
        break;
    }

    if ( MP2::isPickupObject( _mainObjectType ) )
        setAsEmpty();
}

void Maps::Tiles::QuantityUpdate( bool isFirstLoad )
{
    // TODO: don't modify first 2 bits of quantity1.
    switch ( GetObject( false ) ) {
    case MP2::OBJ_WITCHS_HUT:
        QuantitySetSkill( Skill::Secondary::RandForWitchsHut() );
        break;

    case MP2::OBJ_SHRINE_FIRST_CIRCLE:
        QuantitySetSpell( Rand::Get( 1 ) ? Spell::RandCombat( 1 ).GetID() : Spell::RandAdventure( 1 ).GetID() );
        break;

    case MP2::OBJ_SHRINE_SECOND_CIRCLE:
        QuantitySetSpell( Rand::Get( 1 ) ? Spell::RandCombat( 2 ).GetID() : Spell::RandAdventure( 2 ).GetID() );
        break;

    case MP2::OBJ_SHRINE_THIRD_CIRCLE:
        QuantitySetSpell( Rand::Get( 1 ) ? Spell::RandCombat( 3 ).GetID() : Spell::RandAdventure( 3 ).GetID() );
        break;

    case MP2::OBJ_SKELETON: {
        Rand::Queue percents( 2 );
        // 80%: empty
        percents.Push( 0, 80 );
        // 20%: artifact 1 or 2 or 3
        percents.Push( 1, 20 );

        if ( percents.Get() )
            QuantitySetArtifact( Artifact::Rand( Artifact::ART_LEVEL_ALL_NORMAL ) );
        else
            QuantityReset();
        break;
    }

    case MP2::OBJ_WAGON: {
        quantity2 = 0;

        Rand::Queue percents( 3 );
        // 20%: empty
        percents.Push( 0, 20 );
        // 10%: artifact 1 or 2
        percents.Push( 1, 10 );
        // 50%: resource
        percents.Push( 2, 50 );

        switch ( percents.Get() ) {
        case 1:
            QuantitySetArtifact( Artifact::Rand( Rand::Get( 1 ) ? Artifact::ART_LEVEL_TREASURE : Artifact::ART_LEVEL_MINOR ) );
            break;
        case 2:
            QuantitySetResource( Resource::Rand( false ), Rand::Get( 2, 5 ) );
            break;
        default:
            QuantityReset();
            break;
        }
        break;
    }

    case MP2::OBJ_ARTIFACT: {
        const int art = Artifact::FromMP2IndexSprite( _imageIndex ).GetID();

        if ( Artifact::UNKNOWN != art ) {
            if ( art == Artifact::SPELL_SCROLL ) {
                static_assert( std::is_same_v<decltype( quantity1 ), uint8_t> && std::is_same_v<decltype( quantity2 ), uint8_t>,
                               "Types of tile's quantities have been changed, check the bitwise arithmetic below" );
                static_assert( Spell::FIREBALL < Spell::SETWGUARDIAN, "The order of spell IDs has been changed, check the logic below" );

                // Spell id of a spell scroll is represented by 2 low-order bits of quantity2 and 5 high-order bits of quantity1 plus one, and cannot be random
                const int spell
                    = std::clamp( ( ( quantity2 & 0x03 ) << 5 ) + ( quantity1 >> 3 ) + 1, static_cast<int>( Spell::FIREBALL ), static_cast<int>( Spell::SETWGUARDIAN ) );

                QuantitySetVariant( 15 );
                QuantitySetSpell( spell );
            }
            else {
                // 0: 70% none
                // 1,2,3 - 2000g, 2500g+3res, 3000g+5res,
                // 4,5 - need to have skill wisdom or leadership,
                // 6 - 50 rogues, 7 - 1 gin, 8,9,10,11,12,13 - 1 monster level4,
                // 15 - spell
                int cond = Rand::Get( 1, 10 ) < 4 ? Rand::Get( 1, 13 ) : 0;

                QuantitySetVariant( cond );
                QuantitySetArtifact( art );

                if ( cond == 2 || cond == 3 ) {
                    // TODO: why do we use icon ICN index instead of map ICN index?
                    QuantitySetExt( Resource::getIconIcnIndex( Resource::Rand( false ) ) + 1 );
                }
            }
        }
        break;
    }

    case MP2::OBJ_RESOURCE: {
        int resourceType = Resource::UNKNOWN;

        if ( _objectIcnType == MP2::OBJ_ICN_TYPE_OBJNRSRC ) {
            // The resource is located at the top.
            resourceType = Resource::FromIndexSprite( _imageIndex );
        }
        else {
            for ( TilesAddon & addon : addons_level1 ) {
                if ( addon._objectIcnType == MP2::OBJ_ICN_TYPE_OBJNRSRC ) {
                    resourceType = Resource::FromIndexSprite( addon._imageIndex );
                    // If this happens we are in trouble. It looks like that map maker put the resource under an object which is impossible to do.
                    // Let's swap the addon and main tile objects
                    std::swap( addon._objectIcnType, _objectIcnType );
                    std::swap( addon._imageIndex, _imageIndex );
                    std::swap( addon._uid, _uid );
                    std::swap( addon._layerType, _layerType );
                    std::swap( addon._hasObjectAnimation, _hasObjectAnimation );
                    std::swap( addon._isMarkedAsRoad, _isMarkedAsRoad );

                    break;
                }
            }
        }
        uint32_t count = 0;

        switch ( resourceType ) {
        case Resource::GOLD:
            count = 100 * Rand::Get( 5, 10 );
            break;
        case Resource::WOOD:
        case Resource::ORE:
            count = Rand::Get( 5, 10 );
            break;
        case Resource::MERCURY:
        case Resource::SULFUR:
        case Resource::CRYSTAL:
        case Resource::GEMS:
            count = Rand::Get( 3, 6 );
            break;
        default:
            // Some maps have broken resources being put which ideally we need to correct. Let's make them 0 Wood.
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Tile " << _index << " contains unknown resource type. Object ICN type " << _objectIcnType << ", image index " << _imageIndex )
            resourceType = Resource::WOOD;
            count = 0;
            break;
        }

        QuantitySetResource( resourceType, count );
        break;
    }

    case MP2::OBJ_CAMPFIRE:
        // 4-6 rnd resource and + 400-600 gold
        QuantitySetResource( Resource::Rand( false ), Rand::Get( 4, 6 ) );
        break;

    case MP2::OBJ_MAGIC_GARDEN:
        // 5 gems or 500 gold
        if ( Rand::Get( 1 ) )
            QuantitySetResource( Resource::GEMS, 5 );
        else
            QuantitySetResource( Resource::GOLD, 500 );
        break;

    case MP2::OBJ_WATER_WHEEL:
        // first week 500 gold, next week 1000 gold
        QuantitySetResource( Resource::GOLD, ( 0 == world.CountDay() ? 500 : 1000 ) );
        break;

    case MP2::OBJ_WINDMILL: {
        int res = Resource::WOOD;
        while ( res == Resource::WOOD ) {
            res = Resource::Rand( false );
        }

        // 2 rnd resource
        QuantitySetResource( res, 2 );
        break;
    }

    case MP2::OBJ_LEAN_TO:
        // 1-4 rnd resource
        QuantitySetResource( Resource::Rand( false ), Rand::Get( 1, 4 ) );
        break;

    case MP2::OBJ_FLOTSAM: {
        switch ( Rand::Get( 1, 4 ) ) {
        // 25%: empty
        default:
            break;
        // 25%: 500 gold + 10 wood
        case 1:
            QuantitySetResource( Resource::GOLD, 500 );
            quantity1 = 10;
            break;
        // 25%: 200 gold + 5 wood
        case 2:
            QuantitySetResource( Resource::GOLD, 200 );
            quantity1 = 5;
            break;
        // 25%: 5 wood
        case 3:
            quantity1 = 5;
            break;
        }
        break;
    }

    case MP2::OBJ_SHIPWRECK_SURVIVOR: {
        Rand::Queue percents( 3 );
        // 55%: artifact 1
        percents.Push( 1, 55 );
        // 30%: artifact 2
        percents.Push( 1, 30 );
        // 15%: artifact 3
        percents.Push( 1, 15 );

        // variant
        switch ( percents.Get() ) {
        case 1:
            QuantitySetArtifact( Artifact::Rand( Artifact::ART_LEVEL_TREASURE ) );
            break;
        case 2:
            QuantitySetArtifact( Artifact::Rand( Artifact::ART_LEVEL_MINOR ) );
            break;
        default:
            QuantitySetArtifact( Artifact::Rand( Artifact::ART_LEVEL_MAJOR ) );
            break;
        }
        break;
    }

    case MP2::OBJ_SEA_CHEST: {
        Rand::Queue percents( 3 );
        // 20% - empty
        percents.Push( 0, 20 );
        // 70% - 1500 gold
        percents.Push( 1, 70 );
        // 10% - 1000 gold + art
        percents.Push( 2, 10 );

        int art = Artifact::UNKNOWN;
        uint32_t gold = 0;

        // variant
        switch ( percents.Get() ) {
        default:
            break; // empty
        case 1:
            gold = 1500;
            break;
        case 2:
            gold = 1000;
            art = Artifact::Rand( Artifact::ART_LEVEL_TREASURE );
            break;
        }

        QuantitySetResource( Resource::GOLD, gold );
        QuantitySetArtifact( art );
        break;
    }

    case MP2::OBJ_TREASURE_CHEST:
        if ( isWater() ) {
            SetObject( MP2::OBJ_SEA_CHEST );
            QuantityUpdate();
        }
        else {
            Rand::Queue percents( 4 );
            // 31% - 2000 gold or 1500 exp
            percents.Push( 1, 31 );
            // 32% - 1500 gold or 1000 exp
            percents.Push( 2, 32 );
            // 32% - 1000 gold or 500 exp
            percents.Push( 3, 32 );
            // 5% - art
            percents.Push( 4, 5 );

            int art = Artifact::UNKNOWN;
            uint32_t gold = 0;

            // variant
            switch ( percents.Get() ) {
            case 1:
                gold = 2000;
                break;
            case 2:
                gold = 1500;
                break;
            case 3:
                gold = 1000;
                break;
            default:
                art = Artifact::Rand( Artifact::ART_LEVEL_TREASURE );
                break;
            }

            QuantitySetResource( Resource::GOLD, gold );
            QuantitySetArtifact( art );
        }
        break;

    case MP2::OBJ_DERELICT_SHIP:
        QuantitySetResource( Resource::GOLD, 5000 );
        break;

    case MP2::OBJ_SHIPWRECK: {
        Rand::Queue percents( 4 );
        // 40% - 10ghost(1000g)
        percents.Push( 1, 40 );
        // 30% - 15 ghost(2000g)
        percents.Push( 2, 30 );
        // 20% - 25ghost(5000g)
        percents.Push( 3, 20 );
        // 10% - 50ghost(2000g+art)
        percents.Push( 4, 10 );

        int cond = percents.Get();

        QuantitySetVariant( cond );
        QuantitySetArtifact( cond == 4 ? Artifact::Rand( Artifact::ART_LEVEL_ALL_NORMAL ) : Artifact::UNKNOWN );
        break;
    }

    case MP2::OBJ_GRAVEYARD:
        // 1000 gold + art
        QuantitySetResource( Resource::GOLD, 1000 );
        QuantitySetArtifact( Artifact::Rand( Artifact::ART_LEVEL_ALL_NORMAL ) );
        break;

    case MP2::OBJ_PYRAMID: {
        // random spell level 5
        const Spell & spell = Rand::Get( 1 ) ? Spell::RandCombat( 5 ) : Spell::RandAdventure( 5 );
        QuantitySetSpell( spell.GetID() );
        break;
    }

    case MP2::OBJ_DAEMON_CAVE: {
        // 1000 exp or 1000 exp + 2500 gold or 1000 exp + art or (-2500 or remove hero)
        const int cond = Rand::Get( 1, 4 );
        QuantitySetVariant( cond );
        QuantitySetArtifact( cond == 3 ? Artifact::Rand( Artifact::ART_LEVEL_ALL_NORMAL ) : Artifact::UNKNOWN );
        break;
    }

    case MP2::OBJ_TREE_OF_KNOWLEDGE:
        // variant: 10 gems, 2000 gold or free
        switch ( Rand::Get( 1, 3 ) ) {
        case 1:
            QuantitySetResource( Resource::GEMS, 10 );
            break;
        case 2:
            QuantitySetResource( Resource::GOLD, 2000 );
            break;
        default:
            break;
        }
        break;

    case MP2::OBJ_BARRIER:
        QuantitySetColor( getColorFromBarrierSprite( _objectIcnType, _imageIndex ) );
        break;

    case MP2::OBJ_TRAVELLER_TENT:
        QuantitySetColor( getColorFromTravellerTentSprite( _objectIcnType, _imageIndex ) );
        break;

    case MP2::OBJ_ALCHEMIST_LAB: {
        const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::MERCURY ).mercury );
        assert( resourceCount.has_value() && resourceCount > 0U );

        QuantitySetResource( Resource::MERCURY, resourceCount.value() );
        break;
    }

    case MP2::OBJ_SAWMILL: {
        const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::WOOD ).wood );
        assert( resourceCount.has_value() && resourceCount > 0U );

        QuantitySetResource( Resource::WOOD, resourceCount.value() );
        break;
    }

    case MP2::OBJ_MINES: {
        switch ( _imageIndex ) {
        case 0: {
            const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::ORE ).ore );
            assert( resourceCount.has_value() && resourceCount > 0U );

            QuantitySetResource( Resource::ORE, resourceCount.value() );
            break;
        }
        case 1: {
            const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::SULFUR ).sulfur );
            assert( resourceCount.has_value() && resourceCount > 0U );

            QuantitySetResource( Resource::SULFUR, resourceCount.value() );
            break;
        }
        case 2: {
            const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::CRYSTAL ).crystal );
            assert( resourceCount.has_value() && resourceCount > 0U );

            QuantitySetResource( Resource::CRYSTAL, resourceCount.value() );
            break;
        }
        case 3: {
            const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::GEMS ).gems );
            assert( resourceCount.has_value() && resourceCount > 0U );

            QuantitySetResource( Resource::GEMS, resourceCount.value() );
            break;
        }
        case 4: {
            const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::GOLD ).gold );
            assert( resourceCount.has_value() && resourceCount > 0U );

            QuantitySetResource( Resource::GOLD, resourceCount.value() );
            break;
        }
        default:
            break;
        }
        break;
    }

    case MP2::OBJ_ABANDONED_MINE:
        // The number of Ghosts is set when loading the map and does not change anymore
        if ( isFirstLoad ) {
            MonsterSetCount( Rand::Get( 30, 60 ) );
        }
        break;

    case MP2::OBJ_BOAT:
        _objectIcnType = MP2::OBJ_ICN_TYPE_BOAT32;
        _imageIndex = 18;
        break;

    case MP2::OBJ_EVENT:
        resetObjectSprite();
        break;

    case MP2::OBJ_RANDOM_ARTIFACT:
    case MP2::OBJ_RANDOM_ARTIFACT_TREASURE:
    case MP2::OBJ_RANDOM_ARTIFACT_MINOR:
    case MP2::OBJ_RANDOM_ARTIFACT_MAJOR:
        // modify rnd artifact sprite
        UpdateRNDArtifactSprite( *this );
        QuantityUpdate();
        break;

    case MP2::OBJ_RANDOM_RESOURCE:
        // modify rnd resource sprite
        UpdateRNDResourceSprite( *this );
        QuantityUpdate();
        break;

    case MP2::OBJ_MONSTER:
        if ( world.CountWeek() > 1 )
            UpdateMonsterPopulation( *this );
        else
            UpdateMonsterInfo( *this );
        break;

    case MP2::OBJ_RANDOM_MONSTER:
    case MP2::OBJ_RANDOM_MONSTER_WEAK:
    case MP2::OBJ_RANDOM_MONSTER_MEDIUM:
    case MP2::OBJ_RANDOM_MONSTER_STRONG:
    case MP2::OBJ_RANDOM_MONSTER_VERY_STRONG:
        // modify rnd monster sprite
        UpdateMonsterInfo( *this );
        break;

    case MP2::OBJ_GENIE_LAMP:
        // The number of Genies is set when loading the map and does not change anymore
        if ( isFirstLoad ) {
            MonsterSetCount( Rand::Get( 2, 4 ) );
        }
        break;

    case MP2::OBJ_WATCH_TOWER:
    case MP2::OBJ_EXCAVATION:
    case MP2::OBJ_CAVE:
    case MP2::OBJ_TREE_HOUSE:
    case MP2::OBJ_ARCHER_HOUSE:
    case MP2::OBJ_GOBLIN_HUT:
    case MP2::OBJ_DWARF_COTTAGE:
    case MP2::OBJ_HALFLING_HOLE:
    case MP2::OBJ_PEASANT_HUT:
    // recruit dwelling
    case MP2::OBJ_RUINS:
    case MP2::OBJ_TREE_CITY:
    case MP2::OBJ_WAGON_CAMP:
    case MP2::OBJ_DESERT_TENT:
    case MP2::OBJ_TROLL_BRIDGE:
    case MP2::OBJ_DRAGON_CITY:
    case MP2::OBJ_CITY_OF_DEAD:
    case MP2::OBJ_WATER_ALTAR:
    case MP2::OBJ_AIR_ALTAR:
    case MP2::OBJ_FIRE_ALTAR:
    case MP2::OBJ_EARTH_ALTAR:
    case MP2::OBJ_BARROW_MOUNDS:
        UpdateDwellingPopulation( *this, isFirstLoad );
        break;
    default:
        break;
    }
}

uint32_t Maps::Tiles::MonsterCount() const
{
    static_assert( std::is_same_v<decltype( quantity1 ), uint8_t> && std::is_same_v<decltype( quantity2 ), uint8_t>,
                   "Types of tile's quantities have been changed, check the logic below" );

    // TODO: avoid this hacky way of storing data.
    return ( static_cast<uint32_t>( quantity1 ) << 8 ) + quantity2;
}

void Maps::Tiles::MonsterSetCount( uint32_t count )
{
    static_assert( std::is_same_v<decltype( quantity1 ), uint8_t> && std::is_same_v<decltype( quantity2 ), uint8_t>,
                   "Types of tile's quantities have been changed, check the logic below" );

    if ( count > UINT16_MAX ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "The number of monsters for tile " << _index << " is " << count << ", which is more than " << UINT16_MAX )

        count = UINT16_MAX;
    }

    // TODO: avoid this hacky way of storing data.
    quantity1 = ( count >> 8 ) & 0xFF;
    quantity2 = count & 0xFF;
}

void Maps::Tiles::PlaceMonsterOnTile( Tiles & tile, const Monster & mons, const uint32_t count )
{
    tile.SetObject( MP2::OBJ_MONSTER );

    // If there was another object sprite here (shadow for example) push it down to Addons,
    // except when there is already MONS32.ICN here.
    if ( tile._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN && tile._objectIcnType != MP2::OBJ_ICN_TYPE_MONS32 && tile._imageIndex != 255 ) {
        // Push object sprite to Level 1 Addons preserving the Layer Type.
        tile.AddonsPushLevel1( TilesAddon( tile._layerType, tile._uid, tile._objectIcnType, tile._imageIndex, false, false ) );

        // Set unique UID for placed monster.
        tile._uid = World::GetUniq();
        tile._objectIcnType = MP2::OBJ_ICN_TYPE_MONS32;
    }

    using TileImageIndexType = decltype( tile._imageIndex );
    static_assert( std::is_same_v<TileImageIndexType, uint8_t>, "Type of _imageIndex has been changed, check the logic below" );

    const uint32_t monsSpriteIndex = mons.GetSpriteIndex();
    assert( monsSpriteIndex >= std::numeric_limits<TileImageIndexType>::min() && monsSpriteIndex <= std::numeric_limits<TileImageIndexType>::max() );

    tile._imageIndex = static_cast<TileImageIndexType>( monsSpriteIndex );

    const bool setDefinedCount = ( count > 0 );

    if ( setDefinedCount ) {
        tile.MonsterSetCount( count );
    }
    else {
        tile.MonsterSetCount( mons.GetRNDSize() );
    }

    if ( mons.GetID() == Monster::GHOST || mons.isElemental() ) {
        // Ghosts and elementals never join hero's army.
        setMonsterOnTileJoinCondition( tile, Monster::JOIN_CONDITION_SKIP );
    }
    else if ( setDefinedCount || ( world.GetWeekType().GetType() == WeekName::MONSTERS && world.GetWeekType().GetMonster() == mons.GetID() ) ) {
        // Wandering monsters with the number of units specified by the map designer are always considered as "hostile" and always join only for money.

        // Monsters will be willing to join for some amount of money.
        setMonsterOnTileJoinCondition( tile, Monster::JOIN_CONDITION_MONEY );
    }
    else {
        // 20% chance for join
        if ( 3 > Rand::Get( 1, 10 ) ) {
            setMonsterOnTileJoinCondition( tile, Monster::JOIN_CONDITION_FREE );
        }
        else {
            setMonsterOnTileJoinCondition( tile, Monster::JOIN_CONDITION_MONEY );
        }
    }
}

void Maps::Tiles::UpdateMonsterInfo( Tiles & tile )
{
    Monster mons;

    if ( MP2::OBJ_MONSTER == tile.GetObject() ) {
        mons = Monster( tile._imageIndex + 1 ); // ICN::MONS32 start from PEASANT
    }
    else {
        switch ( tile.GetObject() ) {
        case MP2::OBJ_RANDOM_MONSTER:
            mons = Monster::Rand( Monster::LevelType::LEVEL_ANY ).GetID();
            break;
        case MP2::OBJ_RANDOM_MONSTER_WEAK:
            mons = Monster::Rand( Monster::LevelType::LEVEL_1 ).GetID();
            break;
        case MP2::OBJ_RANDOM_MONSTER_MEDIUM:
            mons = Monster::Rand( Monster::LevelType::LEVEL_2 ).GetID();
            break;
        case MP2::OBJ_RANDOM_MONSTER_STRONG:
            mons = Monster::Rand( Monster::LevelType::LEVEL_3 ).GetID();
            break;
        case MP2::OBJ_RANDOM_MONSTER_VERY_STRONG:
            mons = Monster::Rand( Monster::LevelType::LEVEL_4 ).GetID();
            break;
        default:
            break;
        }

        // fixed random sprite
        tile.SetObject( MP2::OBJ_MONSTER );

        using TileImageIndexType = decltype( tile._imageIndex );
        static_assert( std::is_same_v<TileImageIndexType, uint8_t>, "Type of _imageIndex has been changed, check the logic below" );

        assert( mons.GetID() > std::numeric_limits<TileImageIndexType>::min() && mons.GetID() <= std::numeric_limits<TileImageIndexType>::max() );

        tile._imageIndex = static_cast<TileImageIndexType>( mons.GetID() - 1 ); // ICN::MONS32 starts from PEASANT
    }

    uint32_t count = 0;

    // update count (mp2 format)
    if ( tile.quantity1 || tile.quantity2 ) {
        count = tile.quantity2;
        count <<= 8;
        count |= tile.quantity1;
        count >>= 3;
    }

    PlaceMonsterOnTile( tile, mons, count );
}

void Maps::Tiles::UpdateDwellingPopulation( Tiles & tile, bool isFirstLoad )
{
    uint32_t count = isFirstLoad ? 0 : tile.MonsterCount();
    const MP2::MapObjectType objectType = tile.GetObject( false );

    switch ( objectType ) {
    // join monsters
    case MP2::OBJ_HALFLING_HOLE:
        count += isFirstLoad ? Rand::Get( 20, 40 ) : Rand::Get( 5, 10 );
        break;
    case MP2::OBJ_PEASANT_HUT:
        count += isFirstLoad ? Rand::Get( 20, 50 ) : Rand::Get( 5, 10 );
        break;
    case MP2::OBJ_EXCAVATION:
    case MP2::OBJ_TREE_HOUSE:
        count += isFirstLoad ? Rand::Get( 10, 25 ) : Rand::Get( 4, 8 );
        break;
    case MP2::OBJ_CAVE:
        count += isFirstLoad ? Rand::Get( 10, 20 ) : Rand::Get( 3, 6 );
        break;
    case MP2::OBJ_GOBLIN_HUT:
        count += isFirstLoad ? Rand::Get( 15, 40 ) : Rand::Get( 3, 6 );
        break;

    case MP2::OBJ_TREE_CITY:
        count += isFirstLoad ? Rand::Get( 20, 40 ) : Rand::Get( 10, 20 );
        break;

    case MP2::OBJ_WATCH_TOWER:
        count += isFirstLoad ? Rand::Get( 7, 10 ) : Rand::Get( 2, 4 );
        break;
    case MP2::OBJ_ARCHER_HOUSE:
        count += isFirstLoad ? Rand::Get( 10, 25 ) : Rand::Get( 2, 4 );
        break;
    case MP2::OBJ_DWARF_COTTAGE:
        count += isFirstLoad ? Rand::Get( 10, 20 ) : Rand::Get( 3, 6 );
        break;
    case MP2::OBJ_WAGON_CAMP:
        count += isFirstLoad ? Rand::Get( 30, 50 ) : Rand::Get( 3, 6 );
        break;
    case MP2::OBJ_DESERT_TENT:
        count += isFirstLoad ? Rand::Get( 10, 20 ) : Rand::Get( 1, 3 );
        break;
    case MP2::OBJ_RUINS:
        count += isFirstLoad ? Rand::Get( 3, 5 ) : Rand::Get( 1, 3 );
        break;
    case MP2::OBJ_WATER_ALTAR:
    case MP2::OBJ_AIR_ALTAR:
    case MP2::OBJ_FIRE_ALTAR:
    case MP2::OBJ_EARTH_ALTAR:
    case MP2::OBJ_BARROW_MOUNDS:
        count += Rand::Get( 2, 5 );
        break;

    case MP2::OBJ_TROLL_BRIDGE:
    case MP2::OBJ_CITY_OF_DEAD:
        count = isFirstLoad ? Rand::Get( 4, 6 ) : ( Color::NONE == tile.QuantityColor() ) ? count : count + Rand::Get( 1, 3 );
        break;

    case MP2::OBJ_DRAGON_CITY:
        count = isFirstLoad ? 2 : ( Color::NONE == tile.QuantityColor() ) ? count : count + 1;
        break;

    default:
        break;
    }

    if ( count ) {
        tile.MonsterSetCount( count );
    }
}

void Maps::Tiles::UpdateMonsterPopulation( Tiles & tile )
{
    const Troop & troop = tile.QuantityTroop();
    const uint32_t troopCount = troop.GetCount();

    if ( troopCount == 0 ) {
        tile.MonsterSetCount( troop.GetRNDSize() );
    }
    else {
        const uint32_t bonusUnit = ( Rand::Get( 1, 7 ) <= ( troopCount % 7 ) ) ? 1 : 0;
        tile.MonsterSetCount( troopCount * 8 / 7 + bonusUnit );
    }
}

namespace Maps
{
    void setSpellOnTile( Tiles & tile, const int32_t spellId )
    {
        tile.setAdditionalMetadata( spellId );
    }

    int32_t getSpellIdFromTile( const Tiles & tile )
    {
        return tile.getAdditionalMetadata();
    }

    void setMonsterOnTileJoinCondition( Tiles & tile, const int32_t condition )
    {
        tile.setAdditionalMetadata( condition );
    }

    bool isMonsterOnTileJoinConditionSkip( const Tiles & tile )
    {
        return tile.GetObject() == MP2::OBJ_MONSTER && tile.getAdditionalMetadata() == Monster::JOIN_CONDITION_SKIP;
    }

    bool isMonsterOnTileJoinConditionFree( const Tiles & tile )
    {
        return tile.GetObject() == MP2::OBJ_MONSTER && tile.getAdditionalMetadata() == Monster::JOIN_CONDITION_FREE;
    }

    Monster getMonsterFromTile( const Tiles & tile )
    {
        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_WATCH_TOWER:
            return { Monster::ORC };
        case MP2::OBJ_EXCAVATION:
            return { Monster::SKELETON };
        case MP2::OBJ_CAVE:
            return { Monster::CENTAUR };
        case MP2::OBJ_TREE_HOUSE:
            return { Monster::SPRITE };
        case MP2::OBJ_ARCHER_HOUSE:
            return { Monster::ARCHER };
        case MP2::OBJ_GOBLIN_HUT:
            return { Monster::GOBLIN };
        case MP2::OBJ_DWARF_COTTAGE:
            return { Monster::DWARF };
        case MP2::OBJ_HALFLING_HOLE:
            return { Monster::HALFLING };
        case MP2::OBJ_PEASANT_HUT:
            return { Monster::PEASANT };
        case MP2::OBJ_RUINS:
            return { Monster::MEDUSA };
        case MP2::OBJ_TREE_CITY:
            return { Monster::SPRITE };
        case MP2::OBJ_WAGON_CAMP:
            return { Monster::ROGUE };
        case MP2::OBJ_DESERT_TENT:
            return { Monster::NOMAD };
        case MP2::OBJ_TROLL_BRIDGE:
            return { Monster::TROLL };
        case MP2::OBJ_DRAGON_CITY:
            return { Monster::RED_DRAGON };
        case MP2::OBJ_CITY_OF_DEAD:
            return { Monster::POWER_LICH };
        case MP2::OBJ_GENIE_LAMP:
            return { Monster::GENIE };
        case MP2::OBJ_ABANDONED_MINE:
            return { Monster::GHOST };
        // Price of Loyalty
        case MP2::OBJ_WATER_ALTAR:
            return { Monster::WATER_ELEMENT };
        case MP2::OBJ_AIR_ALTAR:
            return { Monster::AIR_ELEMENT };
        case MP2::OBJ_FIRE_ALTAR:
            return { Monster::FIRE_ELEMENT };
        case MP2::OBJ_EARTH_ALTAR:
            return { Monster::EARTH_ELEMENT };
        case MP2::OBJ_BARROW_MOUNDS:
            return { Monster::GHOST };

        case MP2::OBJ_MONSTER:
            return { tile.GetObjectSpriteIndex() + 1 };
        default:
            break;
        }

        if ( MP2::isCaptureObject( tile.GetObject( false ) ) ) {
            return { world.GetCapturedObject( tile.GetIndex() ).GetTroop().GetID() };
        }

        return { Monster::UNKNOWN };
    }

    Spell getSpellFromTile( const Tiles & tile )
    {
        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_ARTIFACT:
            if ( tile.QuantityVariant() == 15 ) {
                return { tile.GetQuantity1() };
            }
            return { Spell::NONE };

        case MP2::OBJ_SHRINE_FIRST_CIRCLE:
        case MP2::OBJ_SHRINE_SECOND_CIRCLE:
        case MP2::OBJ_SHRINE_THIRD_CIRCLE:
        case MP2::OBJ_PYRAMID:
            return { tile.GetQuantity1() };

        default:
            break;
        }

        return { Spell::NONE };
    }

    Artifact getArtifactFromTile( const Tiles & tile )
    {
        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_WAGON:
            return Artifact( tile.GetQuantity2() ? static_cast<int>( Artifact::UNKNOWN ) : tile.GetQuantity1() );

        case MP2::OBJ_SKELETON:
        case MP2::OBJ_DAEMON_CAVE:
        case MP2::OBJ_SEA_CHEST:
        case MP2::OBJ_TREASURE_CHEST:
        case MP2::OBJ_SHIPWRECK_SURVIVOR:
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
            return Artifact( tile.GetQuantity1() );

        case MP2::OBJ_ARTIFACT:
            if ( tile.QuantityVariant() == 15 ) {
                Artifact art( Artifact::SPELL_SCROLL );
                art.SetSpell( getSpellFromTile( tile ).GetID() );
                return art;
            }
            else
                return Artifact( tile.GetQuantity1() );

        default:
            break;
        }

        return Artifact( Artifact::UNKNOWN );
    }
}
