/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include "battle_army.h"

#include <algorithm>
#include <cassert>
#include <cstddef>

#include "army_troop.h"
#include "artifact.h"
#include "artifact_info.h"
#include "battle_arena.h"
#include "battle_cell.h"
#include "battle_troop.h"
#include "heroes.h"
#include "heroes_base.h"
#include "monster_anim.h"
#include "resource.h"
#include "skill.h"

namespace
{
    const size_t unitSizeCapacity = 16;
}

Battle::Units::Units()
{
    reserve( unitSizeCapacity );
}

Battle::Units::Units( const Units & units, const bool isRemoveInvalidUnits )
{
    reserve( unitSizeCapacity < units.size() ? units.size() : unitSizeCapacity );
    assign( units.begin(), units.end() );

    if ( isRemoveInvalidUnits ) {
        erase( std::remove_if( begin(), end(),
                               []( const Unit * unit ) {
                                   assert( unit != nullptr );

                                   return !unit->isValid();
                               } ),
               end() );
    }
}

Battle::Units::Units( const Units & units, const Unit * unitToRemove )
{
    reserve( unitSizeCapacity < units.size() ? units.size() : unitSizeCapacity );
    assign( units.begin(), units.end() );

    erase( std::remove_if( begin(), end(),
                           [unitToRemove]( const Unit * unit ) {
                               assert( unit != nullptr );

                               return !unit->isValid() || unit == unitToRemove;
                           } ),
           end() );
}

void Battle::Units::SortFastest()
{
    // It is important to maintain the initial order of units having the same speed for the proper operation of the unit turn queue
    std::stable_sort( begin(), end(), Army::FastestTroop );
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

Battle::Force::Force( Army & parent, bool opposite, TroopsUidGenerator & generator )
    : army( parent )
{
    uids.reserve( army.Size() );

    for ( size_t i = 0; i < army.Size(); ++i ) {
        const Troop * troop = army.GetTroop( i );

        if ( troop == nullptr || !troop->isValid() ) {
            uids.push_back( 0 );

            continue;
        }

        int32_t idx = army.isSpreadFormation() ? static_cast<int32_t>( i ) * 22 : 22 + static_cast<int32_t>( i ) * 11;

        if ( opposite ) {
            idx += ( troop->isWide() ? 9 : 10 );
        }
        else if ( troop->isWide() ) {
            idx += 1;
        }

        Position pos;
        pos.Set( idx, troop->isWide(), opposite );

        assert( pos.GetHead() != nullptr && ( !troop->isWide() || pos.GetTail() != nullptr ) );

        push_back( new Unit( *troop, pos, opposite, generator.GetUnique() ) );
        back()->SetArmy( army );

        uids.push_back( back()->GetUID() );
    }
}

Battle::Force::~Force()
{
    std::for_each( begin(), end(), []( Unit * unit ) {
        assert( unit != nullptr );

        delete unit;
    } );
}

const HeroBase * Battle::Force::GetCommander() const
{
    return army.GetCommander();
}

HeroBase * Battle::Force::GetCommander()
{
    return army.GetCommander();
}

const Battle::Units & Battle::Force::getUnits() const
{
    return *this;
}

int Battle::Force::GetColor() const
{
    return army.GetColor();
}

int Battle::Force::GetControl() const
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
    for ( size_t index = 0; index < army.Size(); ++index ) {
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

uint32_t Battle::Force::GetSurrenderCost() const
{
    double res = 0;

    // Consider only the units from the original army
    for ( size_t index = 0; index < army.Size(); ++index ) {
        const Troop * troop = army.GetTroop( index );

        if ( troop && troop->isValid() ) {
            const Unit * unit = FindUID( uids.at( index ) );

            if ( unit && unit->isValid() ) {
                res += unit->GetSurrenderCost().gold;
            }
        }
    }

    const HeroBase * commander = GetCommander();

    if ( commander ) {
        const std::vector<int32_t> costReductionPercent
            = commander->GetBagArtifacts().getTotalArtifactMultipliedPercent( fheroes2::ArtifactBonusType::SURRENDER_COST_REDUCTION_PERCENT );
        double mod = 0.5;
        if ( !costReductionPercent.empty() ) {
            mod = 1;
            for ( const int32_t value : costReductionPercent ) {
                mod = mod * value / 100;
            }
        }

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
        default:
            break;
        }

        res *= mod;
    }

    // Total cost should always be at least 1 gold
    return res >= 1 ? static_cast<uint32_t>( res + 0.5 ) : 1;
}

void Battle::Force::NewTurn()
{
    if ( GetCommander() )
        GetCommander()->ResetModes( Heroes::SPELLCASTED );

    std::for_each( begin(), end(), []( Unit * unit ) { unit->NewTurn(); } );
}

Troops Battle::Force::GetKilledTroops() const
{
    Troops result;

    for ( const Unit * unit : *this ) {
        assert( unit != nullptr );

        result.PushBack( *unit, std::min( unit->GetInitialCount(), unit->GetDead() ) );
    }

    return result;
}

bool Battle::Force::animateIdleUnits() const
{
    bool redrawNeeded = false;

    for ( Unit * unit : *this ) {
        // Check if unit is alive.
        if ( unit->isValid() ) {
            if ( unit->isIdling() ) {
                // Go to 'STATIC' animation state if idle animation is over or if unit is blinded or paralyzed.
                if ( unit->isFinishAnimFrame() || unit->isImmovable() ) {
                    redrawNeeded = unit->SwitchAnimation( Monster_Info::STATIC ) || redrawNeeded;
                }
                else {
                    unit->IncreaseAnimFrame();
                    redrawNeeded = true;
                }
            }
            // checkIdleDelay() sets and checks unit's internal timer if we're ready to switch to next one.
            // Do not start idle animations for paralyzed or blinded units.
            else if ( unit->GetAnimationState() == Monster_Info::STATIC && !unit->isImmovable() && unit->checkIdleDelay() ) {
                redrawNeeded = unit->SwitchAnimation( Monster_Info::IDLE ) || redrawNeeded;
            }
        }
    }
    return redrawNeeded;
}

void Battle::Force::resetIdleAnimation() const
{
    for ( Unit * unit : *this ) {
        // Check if unit is alive, not paralyzed or blinded and is in 'STATIC' animation state.
        if ( unit->isValid() && unit->GetAnimationState() == Monster_Info::STATIC && !unit->isImmovable() ) {
            unit->checkIdleDelay();
        }
    }
}

bool Battle::Force::HasMonster( const Monster & mons ) const
{
    return std::any_of( begin(), end(), [&mons]( const Unit * unit ) { return unit->isMonster( mons.GetID() ); } );
}

uint32_t Battle::Force::GetDeadCounts() const
{
    uint32_t res = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        res += ( *it )->GetDead();

    return res;
}

uint32_t Battle::Force::GetDeadHitPoints() const
{
    uint32_t res = 0;

    for ( const_iterator it = begin(); it != end(); ++it ) {
        res += static_cast<Monster *>( *it )->GetHitPoints() * ( *it )->GetDead();
    }

    return res;
}

void Battle::Force::SyncArmyCount()
{
    for ( uint32_t index = 0; index < army.Size(); ++index ) {
        Troop * troop = army.GetTroop( index );

        if ( troop && troop->isValid() ) {
            const Unit * unit = FindUID( uids.at( index ) );

            if ( unit ) {
                troop->SetCount( unit->GetDead() > unit->GetInitialCount() ? 0 : unit->GetInitialCount() - unit->GetDead() );
            }
        }
    }
}
