/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2022                                                    *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "battle.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "battle_troop.h"
#include "heroes.h"
#include "monster_anim.h"
#include "settings.h"
#include "speed.h"

namespace
{
    const size_t unitSizeCapacity = 16;
}

Battle::Units::Units()
{
    reserve( unitSizeCapacity );
}

Battle::Units::Units( const Units & units, const bool filter )
    : std::vector<Unit *>()
{
    reserve( unitSizeCapacity < units.size() ? units.size() : unitSizeCapacity );
    assign( units.begin(), units.end() );

    if ( filter ) {
        erase( std::remove_if( begin(), end(), []( const Unit * unit ) { return !unit->isValid(); } ), end() );
    }
}

void Battle::Units::SortSlowest()
{
    std::stable_sort( begin(), end(), Army::SlowestTroop );
}

void Battle::Units::SortFastest()
{
    std::stable_sort( begin(), end(), Army::FastestTroop );
}

void Battle::Units::SortArchers( void )
{
    std::sort( begin(), end(), []( const Troop * t1, const Troop * t2 ) { return t1->isArchers() && !t2->isArchers(); } );
}

Battle::Unit * Battle::Units::FindUID( uint32_t pid ) const
{
    const_iterator it = std::find_if( begin(), end(), [pid]( const Unit * unit ) { return unit->isUID( pid ); } );

    return it == end() ? nullptr : *it;
}

Battle::Unit * Battle::Units::FindMode( uint32_t mod ) const
{
    const_iterator it = std::find_if( begin(), end(), [mod]( const Unit * unit ) { return unit->Modes( mod ); } );

    return it == end() ? nullptr : *it;
}

Battle::Force::Force( Army & parent, bool opposite, const Rand::DeterministicRandomGenerator & randomGenerator, TroopsUidGenerator & generator )
    : army( parent )
{
    uids.reserve( army.Size() );

    for ( u32 index = 0; index < army.Size(); ++index ) {
        const Troop * troop = army.GetTroop( index );
        const u32 position = army.isSpreadFormat() ? index * 22 : 22 + index * 11;
        u32 uid = 0;

        if ( troop && troop->isValid() ) {
            push_back( new Unit( *troop, opposite ? position + 10 : position, opposite, randomGenerator, generator.GetUnique() ) );
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

const Battle::Units & Battle::Force::getUnits() const
{
    return *this;
}

int Battle::Force::GetColor( void ) const
{
    return army.GetColor();
}

int Battle::Force::GetControl( void ) const
{
    return army.GetControl();
}

bool Battle::Force::isValid( const bool considerBattlefieldArmy /* = true */ ) const
{
    // Consider the state of the army on the battlefield (including resurrected units, summoned units, etc)
    if ( considerBattlefieldArmy ) {
        return std::any_of( begin(), end(), []( const Unit * unit ) { return unit->isValid(); } );
    }

    // Consider only the state of the original army
    for ( uint32_t index = 0; index < army.Size(); ++index ) {
        const Troop * troop = army.GetTroop( index );

        if ( troop && troop->isValid() ) {
            const Unit * unit = FindUID( uids.at( index ) );

            if ( unit && unit->GetDead() < unit->GetInitialCount() ) {
                return true;
            }
        }
    }

    return false;
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
        double mod = commander->hasArtifact( art ) ? art.ExtraValue() / 100.0 : 0.5;

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
    return std::any_of( begin(), end(), [&mons]( const Unit * unit ) { return unit->isMonster( mons.GetID() ); } );
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

void Battle::Force::SyncArmyCount()
{
    for ( u32 index = 0; index < army.Size(); ++index ) {
        Troop * troop = army.GetTroop( index );

        if ( troop && troop->isValid() ) {
            const Unit * unit = FindUID( uids.at( index ) );

            if ( unit ) {
                troop->SetCount( unit->GetDead() > unit->GetInitialCount() ? 0 : unit->GetInitialCount() - unit->GetDead() );
            }
        }
    }
}
