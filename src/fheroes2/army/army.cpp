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
#include <functional>
#include <math.h>
#include <set>

#include "agg.h"
#include "army.h"
#include "castle.h"
#include "color.h"
#include "game.h"
#include "heroes.h"
#include "heroes_base.h"
#include "kingdom.h"
#include "luck.h"
#include "maps_tiles.h"
#include "morale.h"
#include "payment.h"
#include "race.h"
#include "screen.h"
#include "settings.h"
#include "speed.h"
#include "text.h"
#include "tools.h"
#include "world.h"

enum armysize_t
{
    ARMY_FEW = 1,
    ARMY_SEVERAL = 5,
    ARMY_PACK = 10,
    ARMY_LOTS = 20,
    ARMY_HORDE = 50,
    ARMY_THRONG = 100,
    ARMY_SWARM = 250,
    ARMY_ZOUNDS = 500,
    ARMY_LEGION = 1000
};

armysize_t ArmyGetSize( u32 count )
{
    if ( ARMY_LEGION <= count )
        return ARMY_LEGION;
    else if ( ARMY_ZOUNDS <= count )
        return ARMY_ZOUNDS;
    else if ( ARMY_SWARM <= count )
        return ARMY_SWARM;
    else if ( ARMY_THRONG <= count )
        return ARMY_THRONG;
    else if ( ARMY_HORDE <= count )
        return ARMY_HORDE;
    else if ( ARMY_LOTS <= count )
        return ARMY_LOTS;
    else if ( ARMY_PACK <= count )
        return ARMY_PACK;
    else if ( ARMY_SEVERAL <= count )
        return ARMY_SEVERAL;

    return ARMY_FEW;
}

std::string Army::TroopSizeString( const Troop & troop )
{
    std::string str;

    switch ( ArmyGetSize( troop.GetCount() ) ) {
    default:
        str = _( "A few\n%{monster}" );
        break;
    case ARMY_SEVERAL:
        str = _( "Several\n%{monster}" );
        break;
    case ARMY_PACK:
        str = _( "A pack of\n%{monster}" );
        break;
    case ARMY_LOTS:
        str = _( "Lots of\n%{monster}" );
        break;
    case ARMY_HORDE:
        str = _( "A horde of\n%{monster}" );
        break;
    case ARMY_THRONG:
        str = _( "A throng of\n%{monster}" );
        break;
    case ARMY_SWARM:
        str = _( "A swarm of\n%{monster}" );
        break;
    case ARMY_ZOUNDS:
        str = _( "Zounds of\n%{monster}" );
        break;
    case ARMY_LEGION:
        str = _( "A legion of\n%{monster}" );
        break;
    }

    StringReplace( str, "%{monster}", StringLower( troop.GetMultiName() ) );
    return str;
}

std::string Army::SizeString( u32 size )
{
    const char * str_size[] = {_( "army|Few" ),    _( "army|Several" ), _( "army|Pack" ),   _( "army|Lots" ),  _( "army|Horde" ),
                               _( "army|Throng" ), _( "army|Swarm" ),   _( "army|Zounds" ), _( "army|Legion" )};

    switch ( ArmyGetSize( size ) ) {
    default:
        break;
    case ARMY_SEVERAL:
        return str_size[1];
    case ARMY_PACK:
        return str_size[2];
    case ARMY_LOTS:
        return str_size[3];
    case ARMY_HORDE:
        return str_size[4];
    case ARMY_THRONG:
        return str_size[5];
    case ARMY_SWARM:
        return str_size[6];
    case ARMY_ZOUNDS:
        return str_size[7];
    case ARMY_LEGION:
        return str_size[8];
    }

    return str_size[0];
}

Troops::Troops() {}

Troops::Troops( const Troops & troops )
    : std::vector<Troop *>()
{
    *this = troops;
}

Troops & Troops::operator=( const Troops & rhs )
{
    reserve( rhs.size() );
    for ( const_iterator it = rhs.begin(); it != rhs.end(); ++it )
        push_back( new Troop( **it ) );
    return *this;
}

Troops::~Troops()
{
    for ( iterator it = begin(); it != end(); ++it )
        delete *it;
}

size_t Troops::Size( void ) const
{
    return size();
}

void Troops::Assign( const Troop * it1, const Troop * it2 )
{
    Clean();

    iterator it = begin();

    while ( it != end() && it1 != it2 ) {
        if ( it1->isValid() )
            ( *it )->Set( *it1 );
        ++it;
        ++it1;
    }
}

void Troops::Assign( const Troops & troops )
{
    Clean();

    iterator it1 = begin();
    const_iterator it2 = troops.begin();

    while ( it1 != end() && it2 != troops.end() ) {
        if ( ( *it2 )->isValid() )
            ( *it1 )->Set( **it2 );
        ++it2;
        ++it1;
    }
}

void Troops::Insert( const Troops & troops )
{
    for ( const_iterator it = troops.begin(); it != troops.end(); ++it )
        push_back( new Troop( **it ) );
}

void Troops::PushBack( const Monster & mons, u32 count )
{
    push_back( new Troop( mons, count ) );
}

void Troops::PopBack( void )
{
    if ( size() ) {
        delete back();
        pop_back();
    }
}

Troop * Troops::GetTroop( size_t pos )
{
    return pos < size() ? at( pos ) : NULL;
}

const Troop * Troops::GetTroop( size_t pos ) const
{
    return pos < size() ? at( pos ) : NULL;
}

void Troops::UpgradeMonsters( const Monster & m )
{
    for ( iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() && **it == m )
            ( *it )->Upgrade();
}

u32 Troops::GetCountMonsters( const Monster & m ) const
{
    u32 c = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() && **it == m )
            c += ( *it )->GetCount();

    return c;
}

bool Troops::isValid( void ) const
{
    for ( const_iterator it = begin(); it != end(); ++it ) {
        if ( ( *it )->isValid() )
            return true;
    }
    return false;
}

u32 Troops::GetCount( void ) const
{
    uint32_t total = 0;
    for ( const_iterator it = begin(); it != end(); ++it ) {
        if ( ( *it )->isValid() )
            ++total;
    }
    return total;
}

bool Troops::HasMonster( const Monster & mons ) const
{
    const int monsterID = mons.GetID();
    for ( const_iterator it = begin(); it != end(); ++it ) {
        if ( ( *it )->isMonster( monsterID ) )
            return true;
    }
    return false;
}

bool Troops::AllTroopsIsRace( int race ) const
{
    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() && ( *it )->GetRace() != race )
            return false;

    return true;
}

bool Troops::AllTroopsAreUndead() const
{
    for ( const_iterator it = begin(); it != end(); ++it ) {
        if ( ( *it )->isValid() && !( *it )->isUndead() )
            return false;
    }

    return true;
}

bool Troops::CanJoinTroop( const Monster & mons ) const
{
    const_iterator it = std::find_if( begin(), end(), [&mons]( const Troop * troop ) { return troop->isMonster( mons.GetID() ); } );
    if ( it == end() )
        it = std::find_if( begin(), end(), []( const Troop * troop ) { return !troop->isValid(); } );

    return it != end();
}

bool Troops::JoinTroop( const Monster & mons, u32 count )
{
    if ( mons.isValid() && count ) {
        iterator it = std::find_if( begin(), end(), [&mons]( const Troop * troop ) { return troop->isMonster( mons.GetID() ); } );
        if ( it == end() )
            it = std::find_if( begin(), end(), []( const Troop * troop ) { return !troop->isValid(); } );

        if ( it != end() ) {
            if ( ( *it )->isValid() )
                ( *it )->SetCount( ( *it )->GetCount() + count );
            else
                ( *it )->Set( mons, count );

            DEBUG( DBG_GAME, DBG_INFO, std::dec << count << " " << ( *it )->GetName() );
            return true;
        }
    }

    return false;
}

bool Troops::JoinTroop( const Troop & troop )
{
    return troop.isValid() ? JoinTroop( troop(), troop.GetCount() ) : false;
}

bool Troops::CanJoinTroops( const Troops & troops2 ) const
{
    if ( this == &troops2 )
        return false;

    Army troops1;
    troops1.Insert( *this );

    for ( const_iterator it = troops2.begin(); it != troops2.end(); ++it )
        if ( ( *it )->isValid() && !troops1.JoinTroop( **it ) )
            return false;

    return true;
}

void Troops::JoinTroops( Troops & troops2 )
{
    if ( this == &troops2 )
        return;

    for ( iterator it = troops2.begin(); it != troops2.end(); ++it )
        if ( ( *it )->isValid() ) {
            JoinTroop( **it );
            ( *it )->Reset();
        }
}

u32 Troops::GetUniqueCount( void ) const
{
    std::set<int> monsters;

    for ( size_t idx = 0; idx < size(); ++idx ) {
        const Troop * troop = operator[]( idx );
        if ( troop && troop->isValid() )
            monsters.insert( troop->GetID() );
    }

    return monsters.size();
}

double Troops::GetStrength() const
{
    double strength = 0;
    for ( const Troop * troop : *this ) {
        if ( troop && troop->isValid() )
            strength += troop->GetStrength();
    }
    return strength;
}

u32 Troops::GetAttack( void ) const
{
    u32 res = 0;
    u32 count = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() ) {
            res += static_cast<Monster *>( *it )->GetAttack();
            ++count;
        }

    return count ? res / count : 0;
}

u32 Troops::GetDefense( void ) const
{
    u32 res = 0;
    u32 count = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() ) {
            res += static_cast<Monster *>( *it )->GetDefense();
            ++count;
        }

    return count ? res / count : 0;
}

u32 Troops::GetHitPoints( void ) const
{
    u32 res = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() )
            res += ( *it )->GetHitPoints();

    return res;
}

u32 Troops::GetDamageMin( void ) const
{
    u32 res = 0;
    u32 count = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() ) {
            res += ( *it )->GetDamageMin();
            ++count;
        }

    return count ? res / count : 0;
}

u32 Troops::GetDamageMax( void ) const
{
    u32 res = 0;
    u32 count = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() ) {
            res += ( *it )->GetDamageMax();
            ++count;
        }

    return count ? res / count : 0;
}

void Troops::Clean( void )
{
    std::for_each( begin(), end(), []( Troop * troop ) { troop->Reset(); } );
}

void Troops::UpgradeTroops( const Castle & castle )
{
    for ( iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() ) {
            payment_t payment = ( *it )->GetUpgradeCost();
            Kingdom & kingdom = castle.GetKingdom();

            if ( castle.GetRace() == ( *it )->GetRace() && castle.isBuild( ( *it )->GetUpgrade().GetDwelling() ) && kingdom.AllowPayment( payment ) ) {
                kingdom.OddFundsResource( payment );
                ( *it )->Upgrade();
            }
        }
}

Troop * Troops::GetFirstValid( void )
{
    iterator it = std::find_if( begin(), end(), []( const Troop * troop ) { return troop->isValid(); } );
    return it == end() ? NULL : *it;
}

Troop * Troops::GetWeakestTroop( void )
{
    iterator first, last, lowest;

    first = begin();
    last = end();

    while ( first != last )
        if ( ( *first )->isValid() )
            break;
        else
            ++first;

    if ( first == end() )
        return NULL;
    lowest = first;

    if ( first != last )
        while ( ++first != last )
            if ( ( *first )->isValid() && Army::WeakestTroop( *first, *lowest ) )
                lowest = first;

    return *lowest;
}

Troop * Troops::GetSlowestTroop( void )
{
    iterator first, last, lowest;

    first = begin();
    last = end();

    while ( first != last )
        if ( ( *first )->isValid() )
            break;
        else
            ++first;

    if ( first == end() )
        return NULL;
    lowest = first;

    if ( first != last )
        while ( ++first != last )
            if ( ( *first )->isValid() && Army::SlowestTroop( *first, *lowest ) )
                lowest = first;

    return *lowest;
}

void Troops::MergeTroops()
{
    for ( size_t slot = 0; slot < size(); ++slot ) {
        Troop * troop = at( slot );
        if ( !troop || !troop->isValid() )
            continue;

        const int id = troop->GetID();
        for ( size_t secondary = slot + 1; secondary < size(); ++secondary ) {
            Troop * secondaryTroop = at( secondary );
            if ( secondaryTroop && secondaryTroop->isValid() && id == secondaryTroop->GetID() ) {
                troop->SetCount( troop->GetCount() + secondaryTroop->GetCount() );
                secondaryTroop->Reset();
            }
        }
    }
}

Troops Troops::GetOptimized( void ) const
{
    Troops result;
    result.reserve( size() );

    for ( const_iterator it1 = begin(); it1 != end(); ++it1 )
        if ( ( *it1 )->isValid() ) {
            const int monsterId = ( *it1 )->GetID();
            iterator it2 = std::find_if( result.begin(), result.end(), [monsterId]( const Troop * troop ) { return troop->isMonster( monsterId ); } );

            if ( it2 == result.end() )
                result.push_back( new Troop( **it1 ) );
            else
                ( *it2 )->SetCount( ( *it2 )->GetCount() + ( *it1 )->GetCount() );
        }

    return result;
}

void Troops::SortStrongest()
{
    std::sort( begin(), end(), Army::StrongestTroop );
}

void Troops::ArrangeForBattle( bool upgrade )
{
    const Troops & priority = GetOptimized();

    switch ( priority.size() ) {
    case 1: {
        const Monster & m = *priority.back();
        const u32 count = priority.back()->GetCount();

        Clean();

        if ( 49 < count ) {
            const u32 c = count / 5;
            at( 0 )->Set( m, c );
            at( 1 )->Set( m, c );
            at( 2 )->Set( m, c + count - ( c * 5 ) );
            at( 3 )->Set( m, c );
            at( 4 )->Set( m, c );

            if ( upgrade && at( 2 )->isAllowUpgrade() )
                at( 2 )->Upgrade();
        }
        else if ( 20 < count ) {
            const u32 c = count / 3;
            at( 1 )->Set( m, c );
            at( 2 )->Set( m, c + count - ( c * 3 ) );
            at( 3 )->Set( m, c );

            if ( upgrade && at( 2 )->isAllowUpgrade() )
                at( 2 )->Upgrade();
        }
        else
            at( 2 )->Set( m, count );
        break;
    }
    case 2: {
        // TODO: need modify army for 2 troops
        Assign( priority );
        break;
    }
    case 3: {
        // TODO: need modify army for 3 troops
        Assign( priority );
        break;
    }
    case 4: {
        // TODO: need modify army for 4 troops
        Assign( priority );
        break;
    }
    case 5: {
        // possible change orders monster
        // store
        Assign( priority );
        break;
    }
    default:
        break;
    }
}

void Troops::JoinStrongest( Troops & troops2, bool saveLast )
{
    if ( this == &troops2 )
        return;

    // validate the size (can be different from ARMYMAXTROOPS)
    if ( troops2.size() < size() )
        troops2.resize( size() );

    // first try to keep units in the same slots
    for ( size_t slot = 0; slot < size(); ++slot ) {
        Troop * leftTroop = at( slot );
        Troop * rightTroop = troops2[slot];
        if ( rightTroop && rightTroop->isValid() ) {
            if ( !leftTroop->isValid() ) {
                // if slot is empty, simply move the unit
                leftTroop->Set( *rightTroop );
                rightTroop->Reset();
            }
            else if ( leftTroop->GetID() == rightTroop->GetID() ) {
                // check if we can merge them
                leftTroop->SetCount( leftTroop->GetCount() + rightTroop->GetCount() );
                rightTroop->Reset();
            }
        }
    }

    // there's still unmerged units left and there's empty room for them
    for ( size_t slot = 0; slot < troops2.size(); ++slot ) {
        Troop * rightTroop = troops2[slot];
        if ( rightTroop && rightTroop->isValid() && JoinTroop( *rightTroop ) ) {
            rightTroop->Reset();
        }
    }

    // if there's more units than slots, start optimizing
    if ( troops2.GetCount() ) {
        Troops rightPriority = troops2.GetOptimized();
        troops2.Clean();
        // strongest at the end
        std::sort( rightPriority.begin(), rightPriority.end(), Army::WeakestTroop );

        // 1. Merge any remaining stacks to free some space
        MergeTroops();

        // 2. Fill empty slots with best troops (if there are any)
        uint32_t count = GetCount();
        while ( count < ARMYMAXTROOPS && rightPriority.size() ) {
            JoinTroop( *rightPriority.back() );
            rightPriority.PopBack();
            ++count;
        }

        // 3. Swap weakest and strongest unit until there's no left
        while ( rightPriority.size() ) {
            Troop * weakest = GetWeakestTroop();

            if ( !weakest || Army::StrongestTroop( weakest, rightPriority.back() ) ) {
                // we're done processing if second army units are weaker
                break;
            }

            Army::SwapTroops( *weakest, *rightPriority.back() );
            std::sort( rightPriority.begin(), rightPriority.end(), Army::WeakestTroop );
        }

        // 4. The rest goes back to second army
        while ( rightPriority.size() ) {
            troops2.JoinTroop( *rightPriority.back() );
            rightPriority.PopBack();
        }
    }

    // save weakest unit to army2 (for heroes)
    if ( saveLast && !troops2.isValid() ) {
        Troop * weakest = GetWeakestTroop();

        if ( weakest && weakest->isValid() ) {
            troops2.JoinTroop( *weakest, 1 );
            weakest->SetCount( weakest->GetCount() - 1 );
        }
    }
}

void Troops::KeepOnlyWeakest( Troops & troops2, bool save_last )
{
    if ( this == &troops2 )
        return;

    Troops priority = GetOptimized();
    priority.reserve( ARMYMAXTROOPS * 2 );

    const Troops & priority2 = troops2.GetOptimized();
    priority.Insert( priority2 );

    Clean();
    troops2.Clean();

    // sort: strongest
    std::sort( priority.begin(), priority.end(), Army::StrongestTroop );

    // weakest to army
    while ( size() < priority.size() ) {
        JoinTroop( *priority.back() );
        priority.PopBack();
    }

    // save half weak of strongest to army
    if ( save_last && !isValid() ) {
        Troop & last = *priority.back();
        u32 count = last.GetCount() / 2;
        JoinTroop( last, last.GetCount() - count );
        last.SetCount( count );
    }

    // strongest to army2
    while ( priority.size() ) {
        troops2.JoinTroop( *priority.back() );
        priority.PopBack();
    }
}

void Troops::DrawMons32Line( int32_t cx, int32_t cy, uint32_t width, uint32_t first, uint32_t count, uint32_t drawPower, bool compact, bool isScouteView ) const
{
    if ( isValid() ) {
        if ( 0 == count )
            count = GetCount();

        const uint32_t chunk = width / count;
        cx += chunk / 2;

        Text text;
        text.Set( Font::SMALL );

        for ( const_iterator it = begin(); it != end(); ++it ) {
            if ( ( *it )->isValid() ) {
                if ( 0 == first && count ) {
                    const uint32_t spriteIndex = ( *it )->GetSpriteIndex();
                    const fheroes2::Sprite & monster = fheroes2::AGG::GetICN( ICN::MONS32, spriteIndex );
                    const int offsetY = !compact ? 30 - monster.height() : ( monster.height() < 35 ) ? 35 - monster.height() : 0;

                    fheroes2::Blit( monster, fheroes2::Display::instance(), cx - monster.width() / 2 + monster.x(), cy + offsetY + monster.y() );

                    const std::string countText
                        = isScouteView ? Game::CountScoute( ( *it )->GetCount(), drawPower, compact ) : Game::CountThievesGuild( ( *it )->GetCount(), drawPower );

                    text.Set( countText );
                    if ( compact )
                        text.Blit( cx + monster.width() / 2, cy + 23 );
                    else
                        text.Blit( cx - text.w() / 2, cy + 29 );

                    cx += chunk;
                    --count;
                }
                else
                    --first;
            }
        }
    }
}

void Troops::SplitTroopIntoFreeSlots( const Troop & troop, u32 slots )
{
    if ( slots && slots <= ( Size() - GetCount() ) ) {
        u32 chunk = troop.GetCount() / slots;
        u32 limits = slots;
        std::vector<iterator> iters;

        for ( iterator it = begin(); it != end(); ++it )
            if ( !( *it )->isValid() && limits ) {
                iters.push_back( it );
                ( *it )->Set( troop.GetMonster(), chunk );
                --limits;
            }

        u32 last = troop.GetCount() - chunk * slots;

        for ( std::vector<iterator>::iterator it = iters.begin(); it != iters.end(); ++it )
            if ( last ) {
                ( **it )->SetCount( ( **it )->GetCount() + 1 );
                --last;
            }
    }
}

void Troops::AssignToFirstFreeSlot( const Troop & troop, const uint32_t splitCount )
{
    for ( iterator it = begin(); it != end(); ++it ) {
        if ( ( *it )->isValid() )
            continue;

        ( *it )->Set( troop.GetMonster(), splitCount );
        break;
    }
}

Army::Army( HeroBase * s )
    : commander( s )
    , combat_format( true )
    , color( Color::NONE )
{
    reserve( ARMYMAXTROOPS );
    for ( u32 ii = 0; ii < ARMYMAXTROOPS; ++ii )
        push_back( new ArmyTroop( this ) );
}

Army::Army( const Maps::Tiles & t )
    : commander( NULL )
    , combat_format( true )
    , color( Color::NONE )
{
    reserve( ARMYMAXTROOPS );
    for ( u32 ii = 0; ii < ARMYMAXTROOPS; ++ii )
        push_back( new ArmyTroop( this ) );

    setFromTile( t );
}

Army::~Army()
{
    for ( iterator it = begin(); it != end(); ++it )
        delete *it;
    clear();
}

void Army::setFromTile( const Maps::Tiles & tile )
{
    Reset();

    const bool isCaptureObject = MP2::isCaptureObject( tile.GetObject() );
    if ( isCaptureObject )
        color = tile.QuantityColor();

    switch ( tile.GetObject( false ) ) {
    case MP2::OBJ_PYRAMID:
        at( 0 )->Set( Monster::ROYAL_MUMMY, 10 );
        at( 1 )->Set( Monster::VAMPIRE_LORD, 10 );
        at( 2 )->Set( Monster::ROYAL_MUMMY, 10 );
        at( 3 )->Set( Monster::VAMPIRE_LORD, 10 );
        at( 4 )->Set( Monster::ROYAL_MUMMY, 10 );
        break;

    case MP2::OBJ_GRAVEYARD:
        at( 0 )->Set( Monster::MUTANT_ZOMBIE, 100 );
        ArrangeForBattle( false );
        break;

    case MP2::OBJ_SHIPWRECK:
        at( 0 )->Set( Monster::GHOST, tile.GetQuantity2() );
        ArrangeForBattle( false );
        break;

    case MP2::OBJ_DERELICTSHIP:
        at( 0 )->Set( Monster::SKELETON, 200 );
        ArrangeForBattle( false );
        break;

    case MP2::OBJ_ARTIFACT:
        switch ( tile.QuantityVariant() ) {
        case 6:
            at( 0 )->Set( Monster::ROGUE, 50 );
            break;
        case 7:
            at( 0 )->Set( Monster::GENIE, 1 );
            break;
        case 8:
            at( 0 )->Set( Monster::PALADIN, 1 );
            break;
        case 9:
            at( 0 )->Set( Monster::CYCLOPS, 1 );
            break;
        case 10:
            at( 0 )->Set( Monster::PHOENIX, 1 );
            break;
        case 11:
            at( 0 )->Set( Monster::GREEN_DRAGON, 1 );
            break;
        case 12:
            at( 0 )->Set( Monster::TITAN, 1 );
            break;
        case 13:
            at( 0 )->Set( Monster::BONE_DRAGON, 1 );
            break;
        default:
            break;
        }
        ArrangeForBattle( false );
        break;

        // case MP2::OBJ_ABANDONEDMINE:
        //    at(0) = Troop(t);
        //    ArrangeForBattle(false);
        //    break;

    case MP2::OBJ_CITYDEAD:
        at( 0 )->Set( Monster::ZOMBIE, 20 );
        at( 1 )->Set( Monster::VAMPIRE_LORD, 5 );
        at( 2 )->Set( Monster::POWER_LICH, 5 );
        at( 3 )->Set( Monster::VAMPIRE_LORD, 5 );
        at( 4 )->Set( Monster::ZOMBIE, 20 );
        break;

    case MP2::OBJ_TROLLBRIDGE:
        at( 0 )->Set( Monster::TROLL, 4 );
        at( 1 )->Set( Monster::WAR_TROLL, 4 );
        at( 2 )->Set( Monster::TROLL, 4 );
        at( 3 )->Set( Monster::WAR_TROLL, 4 );
        at( 4 )->Set( Monster::TROLL, 4 );
        break;

    case MP2::OBJ_DRAGONCITY:
        at( 0 )->Set( Monster::GREEN_DRAGON, 3 );
        at( 1 )->Set( Monster::RED_DRAGON, 2 );
        at( 2 )->Set( Monster::BLACK_DRAGON, 1 );
        break;

    case MP2::OBJ_DAEMONCAVE:
        at( 0 )->Set( Monster::EARTH_ELEMENT, 2 );
        at( 1 )->Set( Monster::EARTH_ELEMENT, 2 );
        at( 2 )->Set( Monster::EARTH_ELEMENT, 2 );
        at( 3 )->Set( Monster::EARTH_ELEMENT, 2 );
        break;

    default:
        if ( isCaptureObject ) {
            CapturedObject & co = world.GetCapturedObject( tile.GetIndex() );
            Troop & troop = co.GetTroop();

            switch ( co.GetSplit() ) {
            case 3:
                if ( 3 > troop.GetCount() )
                    at( 0 )->Set( co.GetTroop() );
                else {
                    at( 0 )->Set( troop(), troop.GetCount() / 3 );
                    at( 4 )->Set( troop(), troop.GetCount() / 3 );
                    at( 2 )->Set( troop(), troop.GetCount() - at( 4 )->GetCount() - at( 0 )->GetCount() );
                }
                break;

            case 5:
                if ( 5 > troop.GetCount() )
                    at( 0 )->Set( co.GetTroop() );
                else {
                    at( 0 )->Set( troop(), troop.GetCount() / 5 );
                    at( 1 )->Set( troop(), troop.GetCount() / 5 );
                    at( 3 )->Set( troop(), troop.GetCount() / 5 );
                    at( 4 )->Set( troop(), troop.GetCount() / 5 );
                    at( 2 )->Set( troop(), troop.GetCount() - at( 0 )->GetCount() - at( 1 )->GetCount() - at( 3 )->GetCount() - at( 4 )->GetCount() );
                }
                break;

            default:
                at( 0 )->Set( co.GetTroop() );
                break;
            }
        }
        else {
            MapMonster * map_troop = NULL;
            if ( tile.GetObject() == MP2::OBJ_MONSTER )
                map_troop = dynamic_cast<MapMonster *>( world.GetMapObject( tile.GetObjectUID() ) );

            Troop troop = map_troop ? map_troop->QuantityTroop() : tile.QuantityTroop();

            at( 0 )->Set( troop );
            if ( troop.isValid() )
                ArrangeForBattle( true );
        }
        break;
    }
}

bool Army::isFullHouse( void ) const
{
    return GetCount() == size();
}

void Army::SetSpreadFormat( bool f )
{
    combat_format = f;
}

bool Army::isSpreadFormat( void ) const
{
    return combat_format;
}

int Army::GetColor( void ) const
{
    return GetCommander() ? GetCommander()->GetColor() : color;
}

void Army::SetColor( int cl )
{
    color = cl;
}

int Army::GetRace( void ) const
{
    std::vector<int> races;
    races.reserve( size() );

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() )
            races.push_back( ( *it )->GetRace() );

    std::sort( races.begin(), races.end() );
    races.resize( std::distance( races.begin(), std::unique( races.begin(), races.end() ) ) );

    if ( races.empty() ) {
        DEBUG( DBG_GAME, DBG_WARN, "empty" );
        return Race::NONE;
    }

    return 1 < races.size() ? Race::MULT : races[0];
}

int Army::GetLuck( void ) const
{
    return GetCommander() ? GetCommander()->GetLuck() : GetLuckModificator( NULL );
}

int Army::GetLuckModificator( std::string * ) const
{
    return Luck::NORMAL;
}

int Army::GetMorale( void ) const
{
    return GetCommander() ? GetCommander()->GetMorale() : GetMoraleModificator( NULL );
}

// TODO:: need optimize
int Army::GetMoraleModificator( std::string * strs ) const
{
    int result = Morale::NORMAL;

    // different race penalty
    u32 count = 0;
    u32 count_kngt = 0;
    u32 count_barb = 0;
    u32 count_sorc = 0;
    u32 count_wrlk = 0;
    u32 count_wzrd = 0;
    u32 count_necr = 0;
    u32 count_bomg = 0;
    bool ghost_present = false;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() ) {
            switch ( ( *it )->GetRace() ) {
            case Race::KNGT:
                ++count_kngt;
                break;
            case Race::BARB:
                ++count_barb;
                break;
            case Race::SORC:
                ++count_sorc;
                break;
            case Race::WRLK:
                ++count_wrlk;
                break;
            case Race::WZRD:
                ++count_wzrd;
                break;
            case Race::NECR:
                ++count_necr;
                break;
            case Race::NONE:
                ++count_bomg;
                break;
            default:
                break;
            }
            if ( ( *it )->GetID() == Monster::GHOST )
                ghost_present = true;
        }

    u32 r = Race::MULT;
    if ( count_kngt ) {
        ++count;
        r = Race::KNGT;
    }
    if ( count_barb ) {
        ++count;
        r = Race::BARB;
    }
    if ( count_sorc ) {
        ++count;
        r = Race::SORC;
    }
    if ( count_wrlk ) {
        ++count;
        r = Race::WRLK;
    }
    if ( count_wzrd ) {
        ++count;
        r = Race::WZRD;
    }
    if ( count_necr ) {
        ++count;
        r = Race::NECR;
    }
    if ( count_bomg ) {
        ++count;
        r = Race::NONE;
    }
    const u32 uniq_count = GetUniqueCount();

    switch ( count ) {
    case 2:
    case 0:
        break;
    case 1:
        if ( 0 == count_necr && !ghost_present ) {
            if ( 1 < uniq_count ) {
                ++result;
                if ( strs ) {
                    std::string str = _( "All %{race} troops +1" );
                    StringReplace( str, "%{race}", Race::String( r ) );
                    strs->append( str );
                    strs->append( "\n" );
                }
            }
        }
        else {
            return 0;
        }
        break;
    case 3:
        result -= 1;
        if ( strs ) {
            strs->append( _( "Troops of 3 alignments -1" ) );
            strs->append( "\n" );
        }
        break;
    case 4:
        result -= 2;
        if ( strs ) {
            strs->append( _( "Troops of 4 alignments -2" ) );
            strs->append( "\n" );
        }
        break;
    default:
        result -= 3;
        if ( strs ) {
            strs->append( _( "Troops of 5 alignments -3" ) );
            strs->append( "\n" );
        }
        break;
    }

    // undead in life group
    if ( ( 1 < uniq_count && ( count_necr || ghost_present ) && ( count_kngt || count_barb || count_sorc || count_wrlk || count_wzrd || count_bomg ) ) ||
         // or artifact Arm Martyr
         ( GetCommander() && GetCommander()->HasArtifact( Artifact::ARM_MARTYR ) ) ) {
        result -= 1;
        if ( strs ) {
            strs->append( _( "Some undead in groups -1" ) );
            strs->append( "\n" );
        }
    }

    return result;
}

u32 Army::GetAttack( void ) const
{
    u32 res = 0;
    u32 count = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() ) {
            res += ( *it )->GetAttack();
            ++count;
        }

    return count ? res / count : 0;
}

u32 Army::GetDefense( void ) const
{
    u32 res = 0;
    u32 count = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() ) {
            res += ( *it )->GetDefense();
            ++count;
        }

    return count ? res / count : 0;
}

double Army::GetStrength( void ) const
{
    double res = 0;
    const uint32_t archery = ( commander ) ? commander->GetSecondaryValues( Skill::Secondary::ARCHERY ) : 0;
    // Hero bonus calculation is slow, cache it
    const int bonusAttack = ( commander ? commander->GetAttack() : 0 );
    const int bonusDefense = ( commander ? commander->GetDefense() : 0 );
    const int armyMorale = GetMorale();
    const int armyLuck = GetLuck();

    for ( const_iterator it = begin(); it != end(); ++it ) {
        const Troop * troop = *it;
        if ( troop != NULL && troop->isValid() ) {
            double strength = troop->GetStrengthWithBonus( bonusAttack, bonusDefense );

            if ( archery > 0 && troop->isArchers() ) {
                strength *= sqrt( 1 + static_cast<double>( archery ) / 100 );
            }

            // GetMorale checks if unit is affected by it
            if ( troop->isAffectedByMorale() )
                strength *= 1 + ( ( armyMorale < 0 ) ? armyMorale / 12.0 : armyMorale / 24.0 );

            strength *= 1 + armyLuck / 24.0;

            res += strength;
        }
    }

    // hero spell STR
    // composition

    return res;
}

double Army::getReinforcementValue( const Troops & reinforcement ) const
{
    // NB items that are added in this vector are all of Troop* type, and not ArmyTroop* type
    // So the GetStrength() computation will be done based on troop strength only (not based on hero bonuses)
    Troops combined( *this );
    const double initialValue = combined.GetStrength();

    combined.Insert( reinforcement.GetOptimized() );
    combined.MergeTroops();
    combined.SortStrongest();

    while ( combined.Size() > ARMYMAXTROOPS ) {
        combined.PopBack();
    }

    return combined.GetStrength() - initialValue;
}

void Army::Reset( bool soft )
{
    Troops::Clean();

    if ( commander && commander->isHeroes() ) {
        const Monster mons1( commander->GetRace(), DWELLING_MONSTER1 );

        if ( soft ) {
            const Monster mons2( commander->GetRace(), DWELLING_MONSTER2 );

            switch ( mons1.GetID() ) {
            case Monster::PEASANT:
                JoinTroop( mons1, Rand::Get( 30, 50 ) );
                break;
            case Monster::GOBLIN:
                JoinTroop( mons1, Rand::Get( 15, 25 ) );
                break;
            case Monster::SPRITE:
                JoinTroop( mons1, Rand::Get( 10, 20 ) );
                break;
            default:
                JoinTroop( mons1, Rand::Get( 6, 10 ) );
                break;
            }

            if ( Rand::Get( 1, 10 ) != 1 ) {
                switch ( mons2.GetID() ) {
                case Monster::ARCHER:
                case Monster::ORC:
                    JoinTroop( mons2, Rand::Get( 3, 5 ) );
                    break;
                default:
                    JoinTroop( mons2, Rand::Get( 2, 4 ) );
                    break;
                }
            }
        }
        else {
            JoinTroop( mons1, 1 );
        }
    }
}

void Army::SetCommander( HeroBase * c )
{
    commander = c;
}

HeroBase * Army::GetCommander( void )
{
    return ( !commander || ( commander->isCaptain() && !commander->isValid() ) ) ? NULL : commander;
}

const Castle * Army::inCastle( void ) const
{
    return commander ? commander->inCastle() : NULL;
}

const HeroBase * Army::GetCommander( void ) const
{
    return ( !commander || ( commander->isCaptain() && !commander->isValid() ) ) ? NULL : commander;
}

int Army::GetControl( void ) const
{
    return commander ? commander->GetControl() : ( color == Color::NONE ? CONTROL_AI : Players::GetPlayerControl( color ) );
}

std::string Army::String( void ) const
{
    std::ostringstream os;

    os << "color(" << Color::String( commander ? commander->GetColor() : color ) << "), ";

    if ( GetCommander() )
        os << "commander(" << GetCommander()->GetName() << "), ";

    os << ": ";

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() )
            os << std::dec << ( *it )->GetCount() << " " << ( *it )->GetName() << ", ";

    return os.str();
}

void Army::JoinStrongestFromArmy( Army & army2 )
{
    bool save_last = army2.commander && army2.commander->isHeroes();
    JoinStrongest( army2, save_last );
}

void Army::KeepOnlyWeakestTroops( Army & army2 )
{
    bool save_last = commander && commander->isHeroes();
    KeepOnlyWeakest( army2, save_last );
}

u32 Army::ActionToSirens( void )
{
    u32 res = 0;

    for ( iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() ) {
            const u32 kill = ( *it )->GetCount() * 30 / 100;

            if ( kill ) {
                ( *it )->SetCount( ( *it )->GetCount() - kill );
                res += kill * static_cast<Monster *>( *it )->GetHitPoints();
            }
        }

    return res;
}

bool Army::isStrongerThan( const Army & target, double safetyRatio ) const
{
    if ( !target.isValid() )
        return true;

    const double str1 = GetStrength();
    const double str2 = target.GetStrength() * safetyRatio;

    DEBUG( DBG_GAME, DBG_TRACE, "Comparing troops: " << str1 << " versus " << str2 );

    return str1 > str2;
}

bool Army::ArmyStrongerThanEnemy( const Army & army1, const Army & army2 )
{
    return army1.isStrongerThan( army2 );
}

void Army::DrawMons32LineWithScoute( const Troops & troops, s32 cx, s32 cy, u32 width, u32 first, u32 count, u32 scoute )
{
    troops.DrawMons32Line( cx, cy, width, first, count, scoute, false, true );
}

/* draw MONS32 sprite in line, first valid = 0, count = 0 */
void Army::DrawMons32Line( const Troops & troops, s32 cx, s32 cy, u32 width, u32 first, u32 count )
{
    troops.DrawMons32Line( cx, cy, width, first, count, Skill::Level::EXPERT, false, true );
}

void Army::DrawMonsterLines( const Troops & troops, int32_t posX, int32_t posY, uint32_t lineWidth, uint32_t drawType, bool compact, bool isScouteView )
{
    const uint32_t count = troops.GetCount();
    const int offsetX = lineWidth / 6;
    const int offsetY = compact ? 29 : 50;

    const bool useSingleLine = count < 4;

    if ( useSingleLine ) {
        troops.DrawMons32Line( posX, posY + offsetY / 2, lineWidth, 0, 0, drawType, compact, isScouteView );
    }
    else {
        const int firstLineTroopCount = 2;
        const int secondLineTroopCount = count - firstLineTroopCount;
        const int secondLineWidth = secondLineTroopCount == 2 ? lineWidth * 2 / 3 : lineWidth;

        troops.DrawMons32Line( posX + offsetX, posY, lineWidth * 2 / 3, 0, firstLineTroopCount, drawType, compact, isScouteView );
        troops.DrawMons32Line( posX, posY + offsetY, secondLineWidth, firstLineTroopCount, secondLineTroopCount, drawType, compact, isScouteView );
    }
}

JoinCount Army::GetJoinSolution( const Heroes & hero, const Maps::Tiles & tile, const Troop & troop )
{
    MapMonster * map_troop = NULL;
    if ( tile.GetObject() == MP2::OBJ_MONSTER )
        map_troop = dynamic_cast<MapMonster *>( world.GetMapObject( tile.GetObjectUID() ) );

    const u32 ratios = troop.isValid() ? hero.GetArmy().GetStrength() / troop.GetStrength() : 0;
    const bool check_extra_condition = !hero.HasArtifact( Artifact::HIDEOUS_MASK );

    const bool join_skip = map_troop ? map_troop->JoinConditionSkip() : tile.MonsterJoinConditionSkip();
    const bool join_free = map_troop ? map_troop->JoinConditionFree() : tile.MonsterJoinConditionFree();
    // force join for campain and others...
    const bool join_force = map_troop ? map_troop->JoinConditionForce() : tile.MonsterJoinConditionForce();

    if ( !join_skip && ( ( check_extra_condition && ratios >= 2 ) || join_force ) ) {
        if ( join_free || join_force )
            return JoinCount( JOIN_FREE, troop.GetCount() );
        else if ( hero.HasSecondarySkill( Skill::Secondary::DIPLOMACY ) ) {
            // skill diplomacy
            const u32 to_join = Monster::GetCountFromHitPoints( troop, troop.GetHitPoints() * hero.GetSecondaryValues( Skill::Secondary::DIPLOMACY ) / 100 );

            if ( to_join )
                return JoinCount( JOIN_COST, to_join );
        }
    }

    if ( ratios >= 5 && !hero.isControlAI() ) {
        // ... surely flee before us
        return JoinCount( JOIN_FLEE, 0 );
    }

    return JoinCount( JOIN_NONE, 0 );
}

bool Army::WeakestTroop( const Troop * t1, const Troop * t2 )
{
    return t1->GetStrength() < t2->GetStrength();
}

bool Army::StrongestTroop( const Troop * t1, const Troop * t2 )
{
    return t1->GetStrength() > t2->GetStrength();
}

bool Army::SlowestTroop( const Troop * t1, const Troop * t2 )
{
    return t1->GetSpeed() < t2->GetSpeed();
}

bool Army::FastestTroop( const Troop * t1, const Troop * t2 )
{
    return t1->GetSpeed() > t2->GetSpeed();
}

bool Army::ArchersFirst( const Troop * t1, const Troop * t2 )
{
    return t1->isArchers() > t2->isArchers();
}

void Army::SwapTroops( Troop & t1, Troop & t2 )
{
    std::swap( t1, t2 );
}

bool Army::SaveLastTroop( void ) const
{
    return commander && commander->isHeroes() && 1 == GetCount();
}

StreamBase & operator<<( StreamBase & msg, const Army & army )
{
    msg << static_cast<u32>( army.size() );

    // Army: fixed size
    for ( Army::const_iterator it = army.begin(); it != army.end(); ++it )
        msg << **it;

    return msg << army.combat_format << army.color;
}

StreamBase & operator>>( StreamBase & msg, Army & army )
{
    u32 armysz;
    msg >> armysz;

    for ( Army::iterator it = army.begin(); it != army.end(); ++it )
        msg >> **it;

    msg >> army.combat_format >> army.color;

    // set army
    for ( Army::iterator it = army.begin(); it != army.end(); ++it ) {
        ArmyTroop * troop = static_cast<ArmyTroop *>( *it );
        if ( troop )
            troop->SetArmy( army );
    }

    // set later from owner (castle, heroes)
    army.commander = NULL;

    return msg;
}
