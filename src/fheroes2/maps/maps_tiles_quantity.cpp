/***************************************************************************
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "artifact.h"
#include "difficulty.h"
#include "maps_tiles.h"
#include "pairs.h"
#include "settings.h"
#include "skill.h"
#include "spell.h"
#include "world.h"

bool Maps::Tiles::QuantityIsValid( void ) const
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_ARTIFACT:
    case MP2::OBJ_RESOURCE:
    case MP2::OBJ_CAMPFIRE:
    case MP2::OBJ_FLOTSAM:
    case MP2::OBJ_SHIPWRECKSURVIROR:
    case MP2::OBJ_TREASURECHEST:
    case MP2::OBJ_WATERCHEST:
    case MP2::OBJ_ABANDONEDMINE:
        return true;

    case MP2::OBJ_PYRAMID:
        return QuantitySpell().isValid();

    case MP2::OBJ_SHIPWRECK:
    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_DERELICTSHIP:
    case MP2::OBJ_WATERWHEEL:
    case MP2::OBJ_WINDMILL:
    case MP2::OBJ_LEANTO:
    case MP2::OBJ_MAGICGARDEN:
        return quantity2;

    case MP2::OBJ_SKELETON:
        return QuantityArtifact() != Artifact::UNKNOWN;

    case MP2::OBJ_WAGON:
        return QuantityArtifact() != Artifact::UNKNOWN || quantity2;

    case MP2::OBJ_DAEMONCAVE:
        return QuantityVariant();

    default:
        break;
    }

    return false;
}

int Maps::Tiles::QuantityVariant( void ) const
{
    return quantity2 >> 4;
}

int Maps::Tiles::QuantityExt( void ) const
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

Skill::Secondary Maps::Tiles::QuantitySkill( void ) const
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

    case MP2::OBJ_WITCHSHUT:
        return Skill::Secondary( quantity1, Skill::Level::BASIC );

    default:
        break;
    }

    return Skill::Secondary();
}

void Maps::Tiles::QuantitySetSkill( int skill )
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_WITCHSHUT:
        quantity1 = skill;
        break;

    default:
        break;
    }
}

Spell Maps::Tiles::QuantitySpell( void ) const
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_ARTIFACT:
        return Spell( QuantityVariant() == 15 ? quantity1 : static_cast<int>( Spell::NONE ) );

    case MP2::OBJ_SHRINE1:
    case MP2::OBJ_SHRINE2:
    case MP2::OBJ_SHRINE3:
    case MP2::OBJ_PYRAMID:
        return Spell( quantity1 );

    default:
        break;
    }

    return Spell( Spell::NONE );
}

void Maps::Tiles::QuantitySetSpell( int spell )
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_ARTIFACT:
    case MP2::OBJ_SHRINE1:
    case MP2::OBJ_SHRINE2:
    case MP2::OBJ_SHRINE3:
    case MP2::OBJ_PYRAMID:
        quantity1 = spell;
        break;

    default:
        break;
    }
}

Artifact Maps::Tiles::QuantityArtifact( void ) const
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_WAGON:
        return Artifact( quantity2 ? static_cast<int>( Artifact::UNKNOWN ) : quantity1 );

    case MP2::OBJ_SKELETON:
    case MP2::OBJ_DAEMONCAVE:
    case MP2::OBJ_WATERCHEST:
    case MP2::OBJ_TREASURECHEST:
    case MP2::OBJ_SHIPWRECKSURVIROR:
    case MP2::OBJ_SHIPWRECK:
    case MP2::OBJ_GRAVEYARD:
        return Artifact( quantity1 );

    case MP2::OBJ_ARTIFACT:
        if ( QuantityVariant() == 15 ) {
            Artifact art( Artifact::SPELL_SCROLL );
            art.SetSpell( QuantitySpell().GetID() );
            return art;
        }
        else
            return Artifact( quantity1 );

    default:
        break;
    }

    return Artifact( Artifact::UNKNOWN );
}

void Maps::Tiles::QuantitySetArtifact( int art )
{
    quantity1 = art;
}

void Maps::Tiles::QuantitySetResource( int res, u32 count )
{
    quantity1 = res;
    quantity2 = res == Resource::GOLD ? count / 100 : count;
}

u32 Maps::Tiles::QuantityGold( void ) const
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
    case MP2::OBJ_MAGICGARDEN:
    case MP2::OBJ_WATERWHEEL:
    case MP2::OBJ_TREEKNOWLEDGE:
        return quantity1 == Resource::GOLD ? 100 * quantity2 : 0;

    case MP2::OBJ_FLOTSAM:
    case MP2::OBJ_CAMPFIRE:
    case MP2::OBJ_WATERCHEST:
    case MP2::OBJ_TREASURECHEST:
    case MP2::OBJ_DERELICTSHIP:
    case MP2::OBJ_GRAVEYARD:
        return 100 * quantity2;

    case MP2::OBJ_DAEMONCAVE:
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
            return 2000;
        case 3:
            return 5000;
        default:
            break;
        }

    default:
        break;
    }

    return 0;
}

ResourceCount Maps::Tiles::QuantityResourceCount( void ) const
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_ARTIFACT:
        switch ( QuantityVariant() ) {
        case 1:
            return ResourceCount( Resource::GOLD, QuantityGold() );
        case 2:
            return ResourceCount( Resource::FromIndexSprite2( QuantityExt() - 1 ), 3 );
        case 3:
            return ResourceCount( Resource::FromIndexSprite2( QuantityExt() - 1 ), 5 );
        default:
            break;
        }
        break;

    case MP2::OBJ_WATERCHEST:
    case MP2::OBJ_TREASURECHEST:
        return ResourceCount( Resource::GOLD, QuantityGold() );

    case MP2::OBJ_FLOTSAM:
        return ResourceCount( Resource::WOOD, quantity1 );

    default:
        break;
    }

    return ResourceCount( quantity1, Resource::GOLD == quantity1 ? QuantityGold() : quantity2 );
}

Funds Maps::Tiles::QuantityFunds( void ) const
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

    case MP2::OBJ_WATERCHEST:
    case MP2::OBJ_TREASURECHEST:
    case MP2::OBJ_DERELICTSHIP:
    case MP2::OBJ_SHIPWRECK:
    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_DAEMONCAVE:
        return Funds( Resource::GOLD, QuantityGold() );

    default:
        break;
    }

    return Funds( rc );
}

void Maps::Tiles::QuantitySetColor( int col )
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_BARRIER:
    case MP2::OBJ_TRAVELLERTENT:
        quantity1 = col;
        break;

    default:
        world.CaptureObject( GetIndex(), col );
        break;
    }
}

int Maps::Tiles::QuantityColor( void ) const
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_BARRIER:
    case MP2::OBJ_TRAVELLERTENT:
        return quantity1;

    default:
        return world.ColorCapturedObject( GetIndex() );
    }
}

Monster Maps::Tiles::QuantityMonster( void ) const
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_WATCHTOWER:
        return Monster( Monster::ORC );
    case MP2::OBJ_EXCAVATION:
        return Monster( Monster::SKELETON );
    case MP2::OBJ_CAVE:
        return Monster( Monster::CENTAUR );
    case MP2::OBJ_TREEHOUSE:
        return Monster( Monster::SPRITE );
    case MP2::OBJ_ARCHERHOUSE:
        return Monster( Monster::ARCHER );
    case MP2::OBJ_GOBLINHUT:
        return Monster( Monster::GOBLIN );
    case MP2::OBJ_DWARFCOTT:
        return Monster( Monster::DWARF );
    case MP2::OBJ_HALFLINGHOLE:
        return Monster( Monster::HALFLING );
    case MP2::OBJ_PEASANTHUT:
    case MP2::OBJ_THATCHEDHUT:
        return Monster( Monster::PEASANT );

    case MP2::OBJ_RUINS:
        return Monster( Monster::MEDUSA );
    case MP2::OBJ_TREECITY:
        return Monster( Monster::SPRITE );
    case MP2::OBJ_WAGONCAMP:
        return Monster( Monster::ROGUE );
    case MP2::OBJ_DESERTTENT:
        return Monster( Monster::NOMAD );

    case MP2::OBJ_TROLLBRIDGE:
        return Monster( Monster::TROLL );
    case MP2::OBJ_DRAGONCITY:
        return Monster( Monster::RED_DRAGON );
    case MP2::OBJ_CITYDEAD:
        return Monster( Monster::LICH );

    case MP2::OBJ_ANCIENTLAMP:
        return Monster( Monster::GENIE );

    // loyalty version
    case MP2::OBJ_WATERALTAR:
        return Monster( Monster::WATER_ELEMENT );
    case MP2::OBJ_AIRALTAR:
        return Monster( Monster::AIR_ELEMENT );
    case MP2::OBJ_FIREALTAR:
        return Monster( Monster::FIRE_ELEMENT );
    case MP2::OBJ_EARTHALTAR:
        return Monster( Monster::EARTH_ELEMENT );
    case MP2::OBJ_BARROWMOUNDS:
        return Monster( Monster::GHOST );

    case MP2::OBJ_MONSTER:
        return Monster( objectIndex + 1 );

    default:
        break;
    }

    return MP2::isCaptureObject( GetObject( false ) ) ? world.GetCapturedObject( GetIndex() ).GetTroop() : Monster( Monster::UNKNOWN );
}

Troop Maps::Tiles::QuantityTroop( void ) const
{
    return MP2::isCaptureObject( GetObject( false ) ) ? world.GetCapturedObject( GetIndex() ).GetTroop() : Troop( QuantityMonster(), MonsterCount() );
}

void Maps::Tiles::QuantityReset( void )
{
    quantity1 = 0;
    quantity2 = 0;

    switch ( GetObject( false ) ) {
    case MP2::OBJ_SKELETON:
    case MP2::OBJ_WAGON:
    case MP2::OBJ_ARTIFACT:
    case MP2::OBJ_SHIPWRECKSURVIROR:
    case MP2::OBJ_WATERCHEST:
    case MP2::OBJ_TREASURECHEST:
    case MP2::OBJ_SHIPWRECK:
    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_DAEMONCAVE:
        QuantitySetArtifact( Artifact::UNKNOWN );
        break;

    default:
        break;
    }

    if ( MP2::isPickupObject( mp2_object ) )
        SetObject( MP2::OBJ_ZERO );
}

void Maps::Tiles::QuantityUpdate( bool isFirstLoad )
{
    switch ( GetObject( false ) ) {
    case MP2::OBJ_WITCHSHUT:
        QuantitySetSkill( Skill::Secondary::RandForWitchsHut() );
        break;

    case MP2::OBJ_SHRINE1:
        QuantitySetSpell( Rand::Get( 1 ) ? Spell::RandCombat( 1 )() : Spell::RandAdventure( 1 )() );
        break;

    case MP2::OBJ_SHRINE2:
        QuantitySetSpell( Rand::Get( 1 ) ? Spell::RandCombat( 2 )() : Spell::RandAdventure( 2 )() );
        break;

    case MP2::OBJ_SHRINE3:
        QuantitySetSpell( Rand::Get( 1 ) ? Spell::RandCombat( 3 )() : Spell::RandAdventure( 3 )() );
        break;

    case MP2::OBJ_SKELETON: {
        Rand::Queue percents( 2 );
        // 80%: empty
        percents.Push( 0, 80 );
        // 20%: artifact 1 or 2 or 3
        percents.Push( 1, 20 );

        if ( percents.Get() )
            QuantitySetArtifact( Artifact::Rand( Artifact::ART_LEVEL123 ) );
        else
            QuantityReset();
    } break;

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
            QuantitySetArtifact( Artifact::Rand( Rand::Get( 1 ) ? Artifact::ART_LEVEL1 : Artifact::ART_LEVEL2 ) );
            break;
        case 2:
            QuantitySetResource( Resource::Rand(), Rand::Get( 2, 5 ) );
            break;
        default:
            QuantityReset();
            break;
        }
    } break;

    case MP2::OBJ_ARTIFACT: {
        const int art = Artifact::FromMP2IndexSprite( objectIndex ).GetID();

        if ( Artifact::UNKNOWN != art ) {
            if ( art == Artifact::SPELL_SCROLL ) {
                QuantitySetVariant( 15 );
                // spell from origin mp2
                QuantitySetSpell( 1 + ( quantity2 * 256 + quantity1 ) / 8 );
            }
            else {
                // 0: 70% none
                // 1,2,3 - 2000g, 2500g+3res, 3000g+5res,
                // 4,5 - need have skill wisard or leadership,
                // 6 - 50 rogues, 7 - 1 gin, 8,9,10,11,12,13 - 1 monster level4,
                // 15 - spell
                int cond = Rand::Get( 1, 10 ) < 4 ? Rand::Get( 1, 13 ) : 0;

                QuantitySetVariant( cond );
                QuantitySetArtifact( art );

                if ( cond == 2 || cond == 3 )
                    QuantitySetExt( Resource::GetIndexSprite2( Resource::Rand() ) + 1 );
            }
        }
    } break;

    case MP2::OBJ_RESOURCE: {
        const int res = Resource::FromIndexSprite( objectIndex );
        u32 count = 0;

        switch ( res ) {
        case Resource::GOLD:
            count = 100 * Rand::Get( 5, 10 );
            break;
        case Resource::WOOD:
        case Resource::ORE:
            count = Rand::Get( 5, 10 );
            break;
        default:
            count = Rand::Get( 3, 6 );
            break;
        }

        QuantitySetResource( res, count );
    } break;

    case MP2::OBJ_CAMPFIRE: {
        // 4-6 rnd resource and + 400-600 gold
        QuantitySetResource( Resource::Rand(), Rand::Get( 4, 6 ) );
    } break;

    case MP2::OBJ_MAGICGARDEN:
        // 5 gems or 500 gold
        if ( Rand::Get( 1 ) )
            QuantitySetResource( Resource::GEMS, 5 );
        else
            QuantitySetResource( Resource::GOLD, 500 );
        break;

    case MP2::OBJ_WATERWHEEL:
        // first week 500 gold, next week 1000 gold
        QuantitySetResource( Resource::GOLD, ( 0 == world.CountDay() ? 500 : 1000 ) );
        break;

    case MP2::OBJ_WINDMILL: {
        int res = Resource::WOOD;
        // except: wood, bugs: #3117478
        while ( res == Resource::WOOD )
            res = Resource::Rand();

        // 2 rnd resource
        QuantitySetResource( res, 2 );
    } break;

    case MP2::OBJ_LEANTO:
        // 1-4 rnd resource
        QuantitySetResource( Resource::Rand(), Rand::Get( 1, 4 ) );
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
    } break;

    case MP2::OBJ_SHIPWRECKSURVIROR: {
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
            QuantitySetArtifact( Artifact::Rand( Artifact::ART_LEVEL1 ) );
            break;
        case 2:
            QuantitySetArtifact( Artifact::Rand( Artifact::ART_LEVEL2 ) );
            break;
        default:
            QuantitySetArtifact( Artifact::Rand( Artifact::ART_LEVEL3 ) );
            break;
        }
    } break;

    case MP2::OBJ_WATERCHEST: {
        Rand::Queue percents( 3 );
        // 20% - empty
        percents.Push( 0, 20 );
        // 70% - 1500 gold
        percents.Push( 1, 70 );
        // 10% - 1000 gold + art
        percents.Push( 2, 10 );

        int art = Artifact::UNKNOWN;
        u32 gold = 0;

        // variant
        switch ( percents.Get() ) {
        default:
            break; // empty
        case 1:
            gold = 1500;
            break;
        case 2:
            gold = 1000;
            art = Artifact::Rand( Artifact::ART_LEVEL1 );
            break;
        }

        QuantitySetResource( Resource::GOLD, gold );
        QuantitySetArtifact( art );
    } break;

    case MP2::OBJ_TREASURECHEST:
        if ( isWater() ) {
            SetObject( MP2::OBJ_WATERCHEST );
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
            u32 gold = 0;

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
                art = Artifact::Rand( Artifact::ART_LEVEL1 );
                break;
            }

            QuantitySetResource( Resource::GOLD, gold );
            QuantitySetArtifact( art );
        }
        break;

    case MP2::OBJ_DERELICTSHIP:
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
        QuantitySetArtifact( cond == 4 ? Artifact::Rand( Artifact::ART_LEVEL123 ) : Artifact::UNKNOWN );
    } break;

    case MP2::OBJ_GRAVEYARD:
        // 1000 gold + art
        QuantitySetResource( Resource::GOLD, 1000 );
        QuantitySetArtifact( Artifact::Rand( Artifact::ART_LEVEL123 ) );
        break;

    case MP2::OBJ_PYRAMID: {
        // random spell level 5
        const Spell & spell = Rand::Get( 1 ) ? Spell::RandCombat( 5 ) : Spell::RandAdventure( 5 );
        QuantitySetSpell( spell() );
    } break;

    case MP2::OBJ_DAEMONCAVE: {
        // 1000 exp or 1000 exp + 2500 gold or 1000 exp + art or (-2500 or remove hero)
        int cond = Rand::Get( 1, 4 );
        QuantitySetVariant( cond );
        QuantitySetArtifact( cond == 3 ? Artifact::Rand( Artifact::ART_LEVEL123 ) : Artifact::UNKNOWN );
    } break;

    case MP2::OBJ_TREEKNOWLEDGE:
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

    case MP2::OBJ_BARRIER: {
        QuantitySetColor( Tiles::ColorFromBarrierSprite( objectTileset, objectIndex ) );
    } break;

    case MP2::OBJ_TRAVELLERTENT: {
        QuantitySetColor( Tiles::ColorFromTravellerTentSprite( objectTileset, objectIndex ) );
    } break;

    case MP2::OBJ_ALCHEMYLAB:
        QuantitySetResource( Resource::MERCURY, 1 );
        break;

    case MP2::OBJ_SAWMILL:
        QuantitySetResource( Resource::WOOD, 2 );
        break;

    case MP2::OBJ_MINES: {
        switch ( objectIndex ) {
        case 0:
            QuantitySetResource( Resource::ORE, 2 );
            break;
        case 1:
            QuantitySetResource( Resource::SULFUR, 1 );
            break;
        case 2:
            QuantitySetResource( Resource::CRYSTAL, 1 );
            break;
        case 3:
            QuantitySetResource( Resource::GEMS, 1 );
            break;
        case 4:
            QuantitySetResource( Resource::GOLD, 1000 );
            break;
        default:
            break;
        }
    } break;

    case MP2::OBJ_ABANDONEDMINE: {
        Troop & troop = world.GetCapturedObject( GetIndex() ).GetTroop();

        // I checked in Heroes II: min 3 x 13, and max 3 x 15
        troop.Set( Monster::GHOST, 3 * Rand::Get( 13, 15 ) );

        if ( !Settings::Get().ExtWorldAbandonedMineRandom() )
            QuantitySetResource( Resource::GOLD, 1000 );
        else
            switch ( Rand::Get( 1, 5 ) ) {
            case 1:
                QuantitySetResource( Resource::ORE, 2 );
                break;
            case 2:
                QuantitySetResource( Resource::SULFUR, 1 );
                break;
            case 3:
                QuantitySetResource( Resource::CRYSTAL, 1 );
                break;
            case 4:
                QuantitySetResource( Resource::GEMS, 1 );
                break;
            default:
                QuantitySetResource( Resource::GOLD, 1000 );
                break;
            }
    } break;

    case MP2::OBJ_BOAT:
        objectTileset = 27;
        objectIndex = 18;
        break;

    case MP2::OBJ_EVENT:
        resetObjectSprite();
        break;

    case MP2::OBJ_RNDARTIFACT:
    case MP2::OBJ_RNDARTIFACT1:
    case MP2::OBJ_RNDARTIFACT2:
    case MP2::OBJ_RNDARTIFACT3:
        // modify rnd artifact sprite
        UpdateRNDArtifactSprite( *this );
        QuantityUpdate();
        break;

    case MP2::OBJ_RNDRESOURCE:
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

    case MP2::OBJ_RNDMONSTER:
    case MP2::OBJ_RNDMONSTER1:
    case MP2::OBJ_RNDMONSTER2:
    case MP2::OBJ_RNDMONSTER3:
    case MP2::OBJ_RNDMONSTER4:
        // modify rnd monster sprite
        UpdateMonsterInfo( *this );
        break;

    case MP2::OBJ_ANCIENTLAMP:
        // Genies in the lamp do not accumulate
        if ( isFirstLoad )
            MonsterSetCount( Rand::Get( 2, 4 ) );
        break;

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
    // recruit dwelling
    case MP2::OBJ_RUINS:
    case MP2::OBJ_TREECITY:
    case MP2::OBJ_WAGONCAMP:
    case MP2::OBJ_DESERTTENT:
    case MP2::OBJ_TROLLBRIDGE:
    case MP2::OBJ_DRAGONCITY:
    case MP2::OBJ_CITYDEAD:
    case MP2::OBJ_WATERALTAR:
    case MP2::OBJ_AIRALTAR:
    case MP2::OBJ_FIREALTAR:
    case MP2::OBJ_EARTHALTAR:
        UpdateDwellingPopulation( *this, isFirstLoad );
        break;

    case MP2::OBJ_BARROWMOUNDS:
        if ( !Settings::Get().ExtWorldDisableBarrowMounds() )
            UpdateDwellingPopulation( *this, isFirstLoad );
        break;

    default:
        break;
    }
}

int Maps::Tiles::MonsterJoinCondition( void ) const
{
    return mp2_object == MP2::OBJ_MONSTER ? ( 0x03 & quantity3 ) : 0;
}

void Maps::Tiles::MonsterSetJoinCondition( int cond )
{
    quantity3 &= 0xFC;
    quantity3 |= ( cond & 0x03 );
}

void Maps::Tiles::MonsterSetFixedCount( void )
{
    quantity3 |= 0x80;
}

bool Maps::Tiles::MonsterFixedCount( void ) const
{
    return mp2_object == MP2::OBJ_MONSTER ? quantity3 & 0x80 : 0;
}

bool Maps::Tiles::MonsterJoinConditionSkip( void ) const
{
    return Monster::JOIN_CONDITION_SKIP == MonsterJoinCondition();
}

bool Maps::Tiles::MonsterJoinConditionMoney( void ) const
{
    return Monster::JOIN_CONDITION_MONEY == MonsterJoinCondition();
}

bool Maps::Tiles::MonsterJoinConditionFree( void ) const
{
    return Monster::JOIN_CONDITION_FREE == MonsterJoinCondition();
}

bool Maps::Tiles::MonsterJoinConditionForce( void ) const
{
    return Monster::JOIN_CONDITION_FORCE == MonsterJoinCondition();
}

u32 Maps::Tiles::MonsterCount( void ) const
{
    return ( static_cast<u32>( quantity1 ) << 8 ) | quantity2;
}

void Maps::Tiles::MonsterSetCount( u32 count )
{
    quantity1 = count >> 8;
    quantity2 = 0x00FF & count;
}

void Maps::Tiles::PlaceMonsterOnTile( Tiles & tile, const Monster & mons, u32 count )
{
    tile.SetObject( MP2::OBJ_MONSTER );

    // if there was another sprite here (shadow for example) push it down to Addons
    if ( tile.objectTileset != 0 && tile.objectIndex != 255 ) {
        tile.AddonsPushLevel1( TilesAddon( 0, tile.uniq, tile.objectTileset, tile.objectIndex ) );
    }
    // replace sprite with the one for the new monster
    tile.uniq = 0;
    tile.objectTileset = 48; // MONS32.ICN
    tile.objectIndex = mons.GetSpriteIndex();

    if ( count ) {
        tile.MonsterSetFixedCount();
        tile.MonsterSetCount( count );
    }
    else {
        tile.MonsterSetCount( mons.GetRNDSize( true ) );
    }

    // skip join
    if ( mons.GetID() == Monster::GHOST || mons.isElemental() )
        tile.MonsterSetJoinCondition( Monster::JOIN_CONDITION_SKIP );
    else
        // fixed count: for money
        if ( tile.MonsterFixedCount() ||
             // month of monster
             ( world.GetWeekType().GetType() == Week::MONSTERS && world.GetWeekType().GetMonster() == mons.GetID() ) )
        tile.MonsterSetJoinCondition( Monster::JOIN_CONDITION_MONEY );
    else {
        // 20% chance for join
        if ( 3 > Rand::Get( 1, 10 ) )
            tile.MonsterSetJoinCondition( Monster::JOIN_CONDITION_FREE );
        else
            tile.MonsterSetJoinCondition( Monster::JOIN_CONDITION_MONEY );
    }
}

void Maps::Tiles::UpdateMonsterInfo( Tiles & tile )
{
    Monster mons;

    if ( MP2::OBJ_MONSTER == tile.GetObject() ) {
        mons = Monster( tile.objectIndex + 1 ); // ICN::MONS32 start from PEASANT
    }
    else {
        switch ( tile.GetObject() ) {
        case MP2::OBJ_RNDMONSTER:
            mons = Monster::Rand().GetID();
            break;
        case MP2::OBJ_RNDMONSTER1:
            mons = Monster::Rand( Monster::LEVEL1 ).GetID();
            break;
        case MP2::OBJ_RNDMONSTER2:
            mons = Monster::Rand( Monster::LEVEL2 ).GetID();
            break;
        case MP2::OBJ_RNDMONSTER3:
            mons = Monster::Rand( Monster::LEVEL3 ).GetID();
            break;
        case MP2::OBJ_RNDMONSTER4:
            mons = Monster::Rand( Monster::LEVEL4 ).GetID();
            break;
        default:
            break;
        }

        // fixed random sprite
        tile.SetObject( MP2::OBJ_MONSTER );
        tile.objectIndex = mons.GetID() - 1; // ICN::MONS32 start from PEASANT
    }

    u32 count = 0;

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
    const int obj = tile.GetObject( false );

    switch ( obj ) {
    // join monsters
    case MP2::OBJ_HALFLINGHOLE:
        count += isFirstLoad ? Rand::Get( 20, 40 ) : Rand::Get( 5, 10 );
        break;
    case MP2::OBJ_PEASANTHUT:
    case MP2::OBJ_THATCHEDHUT:
        count += isFirstLoad ? Rand::Get( 20, 50 ) : Rand::Get( 5, 10 );
        break;
    case MP2::OBJ_EXCAVATION:
    case MP2::OBJ_TREEHOUSE:
        count += isFirstLoad ? Rand::Get( 10, 25 ) : Rand::Get( 4, 8 );
        break;
    case MP2::OBJ_CAVE:
        count += isFirstLoad ? Rand::Get( 10, 20 ) : Rand::Get( 3, 6 );
        break;
    case MP2::OBJ_GOBLINHUT:
        count += isFirstLoad ? Rand::Get( 15, 40 ) : Rand::Get( 3, 6 );
        break;

    case MP2::OBJ_TREECITY:
        count += isFirstLoad ? Rand::Get( 20, 40 ) : Rand::Get( 10, 20 );
        break;

    case MP2::OBJ_WATCHTOWER:
        count += isFirstLoad ? Rand::Get( 7, 10 ) : Rand::Get( 2, 4 );
        break;
    case MP2::OBJ_ARCHERHOUSE:
        count += isFirstLoad ? Rand::Get( 10, 25 ) : Rand::Get( 2, 4 );
        break;
    case MP2::OBJ_DWARFCOTT:
        count += isFirstLoad ? Rand::Get( 10, 20 ) : Rand::Get( 3, 6 );
        break;
    case MP2::OBJ_WAGONCAMP:
        count += isFirstLoad ? Rand::Get( 30, 50 ) : Rand::Get( 3, 6 );
        break;
    case MP2::OBJ_DESERTTENT:
        count += isFirstLoad ? Rand::Get( 10, 20 ) : Rand::Get( 1, 3 );
        break;
    case MP2::OBJ_RUINS:
        count += isFirstLoad ? Rand::Get( 3, 5 ) : Rand::Get( 1, 3 );
        break;
    case MP2::OBJ_WATERALTAR:
    case MP2::OBJ_AIRALTAR:
    case MP2::OBJ_FIREALTAR:
    case MP2::OBJ_EARTHALTAR:
    case MP2::OBJ_BARROWMOUNDS:
        count += Rand::Get( 2, 5 );
        break;

    case MP2::OBJ_TROLLBRIDGE:
    case MP2::OBJ_CITYDEAD:
        count = isFirstLoad ? Rand::Get( 4, 6 ) : ( Color::NONE == tile.QuantityColor() ) ? count : count + Rand::Get( 1, 3 );
        break;

    case MP2::OBJ_DRAGONCITY:
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

    if ( 0 == troop.GetCount() )
        tile.MonsterSetCount( troop.GetRNDSize( false ) );
    else if ( !tile.MonsterFixedCount() )
        tile.MonsterSetCount( troop.GetCount() * 8 / 7 );
}
