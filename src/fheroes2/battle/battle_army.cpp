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
#include "monster_anim.h"
#include "settings.h"
#include "speed.h"

#define CAPACITY 16

namespace Battle
{
    bool AllowPart1( const Unit * b )
    {
        return !b->Modes( TR_SKIPMOVE ) && b->GetSpeed() > Speed::STANDING;
    }

    bool AllowPart2( const Unit * b )
    {
        return b->Modes( TR_SKIPMOVE ) && b->GetSpeed() > Speed::STANDING;
    }

    Unit * ForceGetCurrentUnitPart( Units & units1, Units & units2, bool part1, bool units1_first, bool orders_mode )
    {
        auto allowPartFunc = part1 ? AllowPart1 : AllowPart2;
        Units::iterator it1 = std::find_if( units1.begin(), units1.end(), allowPartFunc );
        Units::iterator it2 = std::find_if( units2.begin(), units2.end(), allowPartFunc );
        Unit * result = nullptr;

        if ( it1 != units1.end() && it2 != units2.end() ) {
            if ( ( *it1 )->GetSpeed() == ( *it2 )->GetSpeed() ) {
                result = units1_first ? *it1 : *it2;
            }
            else if ( part1 || Settings::Get().ExtBattleReverseWaitOrder() ) {
                if ( ( *it1 )->GetSpeed() > ( *it2 )->GetSpeed() )
                    result = *it1;
                else if ( ( *it2 )->GetSpeed() > ( *it1 )->GetSpeed() )
                    result = *it2;
            }
            else {
                if ( ( *it1 )->GetSpeed() < ( *it2 )->GetSpeed() )
                    result = *it1;
                else if ( ( *it2 )->GetSpeed() < ( *it1 )->GetSpeed() )
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

Battle::Unit * Battle::Units::FindUID( u32 pid )
{
    iterator it = std::find_if( begin(), end(), [pid]( const Unit * unit ) { return unit->isUID( pid ); } );

    return it == end() ? nullptr : *it;
}

Battle::Unit * Battle::Units::FindMode( u32 mod )
{
    iterator it = std::find_if( begin(), end(), [mod]( const Unit * unit ) { return unit->Modes( mod ); } );

    return it == end() ? nullptr : *it;
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
    return std::any_of( begin(), end(), []( const Unit * unit ) { return unit->isValid(); } );
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

void Battle::Force::UpdateOrderUnits( const Force & army1, const Force & army2, const Unit * activeUnit, int preferredColor, const Units & orderHistory, Units & orders )
{
    orders.clear();
    orders.insert( orders.end(), orderHistory.begin(), orderHistory.end() );

    {
        Units units1( army1, true );
        Units units2( army2, true );

        units1.SortFastest();
        units2.SortFastest();

        Unit * unit = nullptr;

        while ( ( unit = ForceGetCurrentUnitPart( units1, units2, true, preferredColor != army2.GetColor(), true ) ) != nullptr ) {
            if ( unit != activeUnit && unit->isValid() ) {
                preferredColor = unit->GetArmyColor() == army1.GetColor() ? army2.GetColor() : army1.GetColor();

                orders.push_back( unit );
            }
        }
    }

    if ( Settings::Get().ExtBattleSoftWait() ) {
        Units units1( army1, true );
        Units units2( army2, true );

        if ( Settings::Get().ExtBattleReverseWaitOrder() ) {
            units1.SortFastest();
            units2.SortFastest();
        }
        else {
            std::reverse( units1.begin(), units1.end() );
            std::reverse( units2.begin(), units2.end() );

            units1.SortSlowest();
            units2.SortSlowest();
        }

        Unit * unit = nullptr;

        while ( ( unit = ForceGetCurrentUnitPart( units1, units2, false, preferredColor != army2.GetColor(), true ) ) != nullptr ) {
            if ( unit != activeUnit && unit->isValid() ) {
                preferredColor = unit->GetArmyColor() == army1.GetColor() ? army2.GetColor() : army1.GetColor();

                orders.push_back( unit );
            }
        }
    }
}

Battle::Unit * Battle::Force::GetCurrentUnit( const Force & army1, const Force & army2, bool part1, int preferredColor )
{
    Units units1( army1, true );
    Units units2( army2, true );

    if ( part1 || Settings::Get().ExtBattleReverseWaitOrder() ) {
        units1.SortFastest();
        units2.SortFastest();
    }
    else {
        std::reverse( units1.begin(), units1.end() );
        std::reverse( units2.begin(), units2.end() );

        units1.SortSlowest();
        units2.SortSlowest();
    }

    Unit * result = ForceGetCurrentUnitPart( units1, units2, part1, preferredColor != army2.GetColor(), false );

    return result && result->isValid() ? result : nullptr;
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
