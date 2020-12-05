/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "battle_army.h"
#include "battle_troop.h"
#include "heroes.h"
#include "settings.h"
#include "speed.h"

#define CAPACITY 16

namespace Battle
{
    bool AllowPart1( const Unit * b, bool f )
    {
        return ( f ? !b->Modes( TR_SKIPMOVE ) || b->Modes( TR_HARDSKIP ) : !b->Modes( TR_SKIPMOVE ) ) && Speed::STANDING != b->GetSpeed( f );
    }

    bool AllowPart2( const Unit * b, bool f )
    {
        return ( f ? b->Modes( TR_SKIPMOVE ) && !b->Modes( TR_HARDSKIP ) : b->Modes( TR_SKIPMOVE ) ) && Speed::STANDING != b->GetSpeed( f );
    }

    Unit * ForceGetCurrentUnitPart( Units & units1, Units & units2, bool part1, bool units1_first, bool orders_mode )
    {
        Units::iterator it1 = part1 ? std::find_if( units1.begin(), units1.end(), [orders_mode]( const Unit * v ) { return AllowPart1( v, orders_mode ); } )
                                    : std::find_if( units1.begin(), units1.end(), [orders_mode]( const Unit * v ) { return AllowPart2( v, orders_mode ); } );
        Units::iterator it2 = part1 ? std::find_if( units2.begin(), units2.end(), [orders_mode]( const Unit * v ) { return AllowPart1( v, orders_mode ); } )
                                    : std::find_if( units2.begin(), units2.end(), [orders_mode]( const Unit * v ) { return AllowPart2( v, orders_mode ); } );
        Unit * result = NULL;

        if ( it1 != units1.end() && it2 != units2.end() ) {
            if ( ( *it1 )->GetSpeed( orders_mode ) == ( *it2 )->GetSpeed( orders_mode ) ) {
                result = units1_first ? *it1 : *it2;
            }
            else if ( part1 || Settings::Get().ExtBattleReverseWaitOrder() ) {
                if ( ( *it1 )->GetSpeed( orders_mode ) > ( *it2 )->GetSpeed( orders_mode ) )
                    result = *it1;
                else if ( ( *it2 )->GetSpeed( orders_mode ) > ( *it1 )->GetSpeed( orders_mode ) )
                    result = *it2;
            }
            else {
                if ( ( *it1 )->GetSpeed( orders_mode ) < ( *it2 )->GetSpeed( orders_mode ) )
                    result = *it1;
                else if ( ( *it2 )->GetSpeed( orders_mode ) < ( *it1 )->GetSpeed( orders_mode ) )
                    result = *it2;
            }
        }
        else if ( it1 != units1.end() )
            result = *it1;
        else if ( it2 != units2.end() )
            result = *it2;

        if ( result && orders_mode ) {
            if ( it1 != units1.end() && result == *it1 )
                units1.erase( it1 );
            else if ( it2 != units2.end() && result == *it2 )
                units2.erase( it2 );
        }

        return result;
    }
}

Battle::Units::Units()
{
    reserve( CAPACITY );
}

Battle::Units::Units( const Units & units, bool filter )
    : std::vector<Unit *>()
{
    reserve( CAPACITY < units.size() ? units.size() : CAPACITY );
    assign( units.begin(), units.end() );
    if ( filter )
        resize( std::distance( begin(), std::remove_if( begin(), end(), []( const Unit * unit ) { return !unit->isValid(); } ) ) );
}

Battle::Units::Units( const Units & units1, const Units & units2 )
{
    const size_t capacity = units1.size() + units2.size();
    reserve( CAPACITY < capacity ? capacity : CAPACITY );
    insert( end(), units1.begin(), units1.end() );
    insert( end(), units2.begin(), units2.end() );
}

Battle::Units::~Units() {}

Battle::Units & Battle::Units::operator=( const Units & units )
{
    reserve( CAPACITY < units.size() ? units.size() : CAPACITY );
    assign( units.begin(), units.end() );

    return *this;
}

struct FastestUnits
{
    bool f;

    FastestUnits( bool v )
        : f( v )
    {}

    bool operator()( const Battle::Unit * t1, const Battle::Unit * t2 )
    {
        return t1->GetSpeed( f ) > t2->GetSpeed( f );
    }
};

struct SlowestUnits
{
    bool f;

    SlowestUnits( bool v )
        : f( v )
    {}

    bool operator()( const Battle::Unit * t1, const Battle::Unit * t2 )
    {
        return t1->GetSpeed( f ) < t2->GetSpeed( f );
    }
};

void Battle::Units::SortSlowest( bool f )
{
    SlowestUnits CompareFunc( f );

    std::sort( begin(), end(), CompareFunc );
}

void Battle::Units::SortFastest( bool f )
{
    FastestUnits CompareFunc( f );

    std::sort( begin(), end(), CompareFunc );
}

void Battle::Units::SortStrongest( void )
{
    std::sort( begin(), end(), Army::StrongestTroop );
}

void Battle::Units::SortWeakest( void )
{
    std::sort( begin(), end(), Army::WeakestTroop );
}

void Battle::Units::SortArchers( void )
{
    std::sort( begin(), end(), Army::ArchersFirst );
}

Battle::Unit * Battle::Units::FindUID( u32 pid )
{
    iterator it = std::find_if( begin(), end(), [pid]( const Unit * unit ) { return unit->isUID( pid ); } );

    return it == end() ? NULL : *it;
}

Battle::Unit * Battle::Units::FindMode( u32 mod )
{
    iterator it = std::find_if( begin(), end(), [mod]( const Unit * unit ) { return unit->Modes( mod ); } );

    return it == end() ? NULL : *it;
}

Battle::Force::Force( Army & parent, bool opposite )
    : army( parent )
{
    uids.reserve( army.Size() );

    for ( u32 index = 0; index < army.Size(); ++index ) {
        const Troop * troop = army.GetTroop( index );
        const u32 position = army.isSpreadFormat() ? index * 22 : 22 + index * 11;
        u32 uid = 0;

        if ( troop && troop->isValid() ) {
            push_back( new Unit( *troop, ( opposite ? position + 10 : position ), opposite ) );
            back()->SetArmy( army );
            uid = back()->GetUID();
        }

        uids.push_back( uid );
    }
}

Battle::Force::~Force()
{
    for ( iterator it = begin(); it != end(); ++it )
        delete *it;
}

const HeroBase * Battle::Force::GetCommander( void ) const
{
    return army.GetCommander();
}

HeroBase * Battle::Force::GetCommander( void )
{
    return army.GetCommander();
}

int Battle::Force::GetColor( void ) const
{
    return army.GetColor();
}

int Battle::Force::GetControl( void ) const
{
    return army.GetControl();
}

bool Battle::Force::isValid( void ) const
{
    return end() != std::find_if( begin(), end(), []( const Unit * unit ) { return unit->isValid(); } );
}

uint32_t Battle::Force::GetSurrenderCost( void ) const
{
    double res = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() ) {
            const payment_t & payment = ( *it )->GetCost();
            res += payment.gold;
        }

    const HeroBase * commander = GetCommander();
    if ( commander ) {
        const Artifact art( Artifact::STATESMAN_QUILL );
        double mod = commander->HasArtifact( art ) ? art.ExtraValue() / 100.0 : 0.5;

        switch ( commander->GetLevelSkill( Skill::Secondary::DIPLOMACY ) ) {
        case Skill::Level::BASIC:
            mod *= 0.8;
            break;
        case Skill::Level::ADVANCED:
            mod *= 0.6;
            break;
        case Skill::Level::EXPERT:
            mod *= 0.4;
            break;
        }
        res *= mod;
    }
    // Total cost should always be at least 1 gold
    return res >= 1 ? static_cast<uint32_t>( res + 0.5 ) : 1;
}

void Battle::Force::NewTurn( void )
{
    if ( GetCommander() )
        GetCommander()->ResetModes( Heroes::SPELLCASTED );

    std::for_each( begin(), end(), []( Unit * unit ) { unit->NewTurn(); } );
}

bool isUnitFirst( const Battle::Unit * last, bool part1, int army2_color )
{
    return ( !last && part1 ) || ( last && army2_color == last->GetColor() );
}

void Battle::Force::UpdateOrderUnits( const Force & army1, const Force & army2, Units & orders )
{
    orders.clear();
    Unit * last = NULL;

    if ( 1 ) {
        Units units1( army1, true );
        Units units2( army2, true );

        units1.SortFastest( true );
        units2.SortFastest( true );

        while ( NULL != ( last = ForceGetCurrentUnitPart( units1, units2, true, isUnitFirst( last, true, army2.GetColor() ), true ) ) )
            orders.push_back( last );
    }

    if ( Settings::Get().ExtBattleSoftWait() ) {
        Units units1( army1, true );
        Units units2( army2, true );

        if ( Settings::Get().ExtBattleReverseWaitOrder() ) {
            units1.SortFastest( true );
            units2.SortFastest( true );
        }
        else {
            units1.SortSlowest( true );
            units2.SortSlowest( true );
        }

        while ( NULL != ( last = ForceGetCurrentUnitPart( units1, units2, false, isUnitFirst( last, false, army2.GetColor() ), true ) ) )
            orders.push_back( last );
    }
}

Battle::Unit * Battle::Force::GetCurrentUnit( const Force & army1, const Force & army2, Unit * last, bool part1 )
{
    Units units1( army1, true );
    Units units2( army2, true );

    if ( part1 || Settings::Get().ExtBattleReverseWaitOrder() ) {
        units1.SortFastest( false );
        units2.SortFastest( false );
    }
    else {
        units1.SortSlowest( false );
        units2.SortSlowest( false );
    }

    Unit * result = ForceGetCurrentUnitPart( units1, units2, part1, isUnitFirst( last, part1, army2.GetColor() ), false );

    return result && result->isValid() && result->GetSpeed() > Speed::STANDING ? result : NULL;
}

StreamBase & Battle::operator<<( StreamBase & msg, const Force & f )
{
    msg << static_cast<const BitModes &>( f ) << static_cast<u32>( f.size() );

    for ( Force::const_iterator it = f.begin(); it != f.end(); ++it )
        msg << ( *it )->GetUID() << **it;

    return msg;
}

StreamBase & Battle::operator>>( StreamBase & msg, Force & f )
{
    u32 size = 0;
    u32 uid = 0;

    msg >> static_cast<BitModes &>( f ) >> size;

    for ( u32 ii = 0; ii < size; ++ii ) {
        msg >> uid;
        Unit * b = f.FindUID( uid );
        if ( b )
            msg >> *b;
    }

    return msg;
}

Troops Battle::Force::GetKilledTroops( void ) const
{
    Troops killed;

    for ( const_iterator it = begin(); it != end(); ++it ) {
        const Unit & b = ( **it );
        killed.PushBack( b, b.GetDead() );
    }

    return killed;
}

bool Battle::Force::animateIdleUnits()
{
    bool redrawNeeded = false;

    for ( Force::iterator it = begin(); it != end(); ++it ) {
        Unit & unit = **it;

        // check if alive and not paralyzed
        if ( unit.isValid() && !unit.Modes( SP_BLIND | IS_PARALYZE_MAGIC ) ) {
            if ( unit.isIdling() ) {
                if ( unit.isFinishAnimFrame() ) {
                    redrawNeeded = unit.SwitchAnimation( Monster_Info::STATIC ) || redrawNeeded;
                }
                else {
                    unit.IncreaseAnimFrame();
                    redrawNeeded = true;
                }
            }
            // checkIdleDelay() sets and checks unit's internal timer if we're ready to switch to next one
            else if ( unit.GetAnimationState() == Monster_Info::STATIC && unit.checkIdleDelay() ) {
                redrawNeeded = unit.SwitchAnimation( Monster_Info::IDLE ) || redrawNeeded;
            }
        }
    }
    return redrawNeeded;
}

void Battle::Force::resetIdleAnimation()
{
    for ( Force::iterator it = begin(); it != end(); ++it ) {
        Unit & unit = **it;

        // check if alive and not paralyzed
        if ( unit.isValid() && !unit.Modes( SP_BLIND | IS_PARALYZE_MAGIC ) ) {
            if ( unit.GetAnimationState() == Monster_Info::STATIC )
                unit.checkIdleDelay();
        }
    }
}

bool Battle::Force::HasMonster( const Monster & mons ) const
{
    return end() != std::find_if( begin(), end(), [&mons]( const Unit * unit ) { return unit->isMonster( mons.GetID() ); } );
}

u32 Battle::Force::GetDeadCounts( void ) const
{
    u32 res = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        res += ( *it )->GetDead();

    return res;
}

u32 Battle::Force::GetDeadHitPoints( void ) const
{
    u32 res = 0;

    for ( const_iterator it = begin(); it != end(); ++it ) {
        res += static_cast<Monster *>( *it )->GetHitPoints() * ( *it )->GetDead();
    }

    return res;
}

void Battle::Force::SyncArmyCount( bool checkResurrected )
{
    // A special case: when every creature was resurrected by Ressurection
    // In this case we have to find the weakest troop and set it to 1
    Troop * restoredTroop = nullptr;
    if ( checkResurrected ) {
        bool isAllDead = true;
        std::vector<Troop *> armyMonsters;
        for ( u32 index = 0; index < army.Size(); ++index ) {
            Troop * troop = army.GetTroop( index );

            if ( troop && troop->isValid() ) {
                const Unit * unit = FindUID( uids.at( index ) );
                if ( unit ) {
                    if ( unit->GetDead() > 0 ) {
                        if ( unit->GetDead() < troop->GetCount() ) {
                            isAllDead = false;
                            break;
                        }
                    }
                    else {
                        isAllDead = false;
                        break;
                    }
                }

                armyMonsters.push_back( troop );
            }
        }

        if ( isAllDead && !armyMonsters.empty() ) {
            std::sort( armyMonsters.begin(), armyMonsters.end(), []( const Troop * one, const Troop * two ) { return one->GetStrength() < two->GetStrength(); } );

            restoredTroop = armyMonsters.front();
        }
    }

    for ( u32 index = 0; index < army.Size(); ++index ) {
        Troop * troop = army.GetTroop( index );

        if ( troop && troop->isValid() ) {
            const Unit * unit = FindUID( uids.at( index ) );

            if ( unit ) {
                if ( unit->GetDead() )
                    troop->SetCount( unit->GetDead() > troop->GetCount() ? 0 : troop->GetCount() - unit->GetDead() );
                else if ( unit->GetID() == Monster::GHOST )
                    troop->SetCount( unit->GetCount() );
            }
        }
    }

    if ( restoredTroop != nullptr ) {
        restoredTroop->SetCount( 1 );
    }
}
