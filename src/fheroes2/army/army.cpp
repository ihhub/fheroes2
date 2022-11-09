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
#include <cassert>
#include <cmath>
#include <numeric>

#include "agg_image.h"
#include "army.h"
#include "army_ui_helper.h"
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "castle.h"
#include "color.h"
#include "game.h"
#include "heroes.h"
#include "heroes_base.h"
#include "icn.h"
#include "kingdom.h"
#include "logging.h"
#include "luck.h"
#include "maps_tiles.h"
#include "morale.h"
#include "payment.h"
#include "race.h"
#include "rand.h"
#include "screen.h"
#include "serialize.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_text.h"
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

armysize_t ArmyGetSize( uint32_t count )
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
    case ARMY_FEW:
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
    default:
        // Are you passing the correct value?
        assert( 0 );
        break;
    }

    StringReplace( str, "%{monster}", Translation::StringLower( troop.GetMultiName() ) );
    return str;
}

std::string Army::SizeString( uint32_t size )
{
    switch ( ArmyGetSize( size ) ) {
    case ARMY_FEW:
        return _( "army|Few" );
    case ARMY_SEVERAL:
        return _( "army|Several" );
    case ARMY_PACK:
        return _( "army|Pack" );
    case ARMY_LOTS:
        return _( "army|Lots" );
    case ARMY_HORDE:
        return _( "army|Horde" );
    case ARMY_THRONG:
        return _( "army|Throng" );
    case ARMY_SWARM:
        return _( "army|Swarm" );
    case ARMY_ZOUNDS:
        return _( "army|Zounds" );
    case ARMY_LEGION:
        return _( "army|Legion" );
    default:
        // Are you passing the correct value?
        assert( 0 );
        break;
    }

    return {};
}

std::pair<uint32_t, uint32_t> Army::SizeRange( const uint32_t count )
{
    if ( count < ARMY_SEVERAL ) {
        return { ARMY_FEW, ARMY_SEVERAL };
    }
    if ( count < ARMY_PACK ) {
        return { ARMY_SEVERAL, ARMY_PACK };
    }
    if ( count < ARMY_LOTS ) {
        return { ARMY_PACK, ARMY_LOTS };
    }
    if ( count < ARMY_HORDE ) {
        return { ARMY_LOTS, ARMY_HORDE };
    }
    if ( count < ARMY_THRONG ) {
        return { ARMY_HORDE, ARMY_THRONG };
    }
    if ( count < ARMY_SWARM ) {
        return { ARMY_THRONG, ARMY_SWARM };
    }
    if ( count < ARMY_ZOUNDS ) {
        return { ARMY_SWARM, ARMY_ZOUNDS };
    }
    if ( count < ARMY_LEGION ) {
        return { ARMY_ZOUNDS, ARMY_LEGION };
    }
    if ( count < 5000 ) {
        return { ARMY_LEGION, 5000 };
    }

    return { 5000, UINT32_MAX };
}

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

void Troops::PushBack( const Monster & mons, uint32_t count )
{
    push_back( new Troop( mons, count ) );
}

void Troops::PopBack()
{
    if ( !empty() ) {
        delete back();
        pop_back();
    }
}

Troop * Troops::GetTroop( size_t pos )
{
    return pos < size() ? at( pos ) : nullptr;
}

const Troop * Troops::GetTroop( size_t pos ) const
{
    return pos < size() ? at( pos ) : nullptr;
}

void Troops::UpgradeMonsters( const Monster & m )
{
    for ( iterator it = begin(); it != end(); ++it ) {
        if ( **it == m && ( *it )->isValid() ) {
            ( *it )->Upgrade();
        }
    }
}

uint32_t Troops::GetCountMonsters( const Monster & m ) const
{
    uint32_t c = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() && **it == m )
            c += ( *it )->GetCount();

    return c;
}

double Troops::getReinforcementValue( const Troops & reinforcement ) const
{
    // NB items that are added in this vector are all of Troop* type, and not ArmyTroop* type
    // So the GetStrength() computation will be done based on troop strength only (not based on hero bonuses)
    Troops combined( *this );
    const double initialValue = combined.GetStrength();

    combined.Insert( reinforcement.GetOptimized() );
    combined.MergeSameMonsterTroops();
    combined.SortStrongest();

    while ( combined.Size() > Army::maximumTroopCount ) {
        combined.PopBack();
    }

    return combined.GetStrength() - initialValue;
}

bool Troops::isValid() const
{
    for ( const_iterator it = begin(); it != end(); ++it ) {
        if ( ( *it )->isValid() )
            return true;
    }
    return false;
}

uint32_t Troops::GetOccupiedSlotCount() const
{
    uint32_t total = 0;
    for ( const_iterator it = begin(); it != end(); ++it ) {
        if ( ( *it )->isValid() )
            ++total;
    }
    return total;
}

bool Troops::areAllTroopsUnique() const
{
    std::set<int> monsterId;

    for ( const Troop * troop : *this ) {
        assert( troop != nullptr );
        if ( !troop->isValid() ) {
            continue;
        }

        auto [it, inserted] = monsterId.emplace( troop->GetID() );
        if ( !inserted ) {
            return false;
        }
    }

    return true;
}

bool Troops::HasMonster( const Monster & mons ) const
{
    const int monsterID = mons.GetID();
    for ( const_iterator it = begin(); it != end(); ++it ) {
        if ( ( *it )->isValid() && ( *it )->isMonster( monsterID ) ) {
            return true;
        }
    }
    return false;
}

bool Troops::AllTroopsAreUndead() const
{
    for ( const_iterator it = begin(); it != end(); ++it ) {
        if ( ( *it )->isValid() && !( *it )->isUndead() ) {
            return false;
        }
    }

    return true;
}

bool Troops::CanJoinTroop( const Monster & mons ) const
{
    return std::any_of( begin(), end(), [&mons]( const Troop * troop ) { return troop->isMonster( mons.GetID() ); } )
           || std::any_of( begin(), end(), []( const Troop * troop ) { return !troop->isValid(); } );
}

bool Troops::JoinTroop( const Monster & mons, uint32_t count, bool emptySlotFirst )
{
    if ( !mons.isValid() || count == 0 ) {
        return false;
    }

    auto findEmptySlot = []( const Troop * troop ) { return !troop->isValid(); };
    auto findMonster = [&mons]( const Troop * troop ) { return troop->isValid() && troop->isMonster( mons.GetID() ); };

    iterator it = emptySlotFirst ? std::find_if( begin(), end(), findEmptySlot ) : std::find_if( begin(), end(), findMonster );
    if ( it == end() ) {
        it = emptySlotFirst ? std::find_if( begin(), end(), findMonster ) : std::find_if( begin(), end(), findEmptySlot );
    }

    if ( it != end() ) {
        if ( ( *it )->isValid() )
            ( *it )->SetCount( ( *it )->GetCount() + count );
        else
            ( *it )->Set( mons, count );

        DEBUG_LOG( DBG_GAME, DBG_INFO, std::dec << count << " " << ( *it )->GetName() )
        return true;
    }

    return false;
}

bool Troops::JoinTroop( const Troop & troop )
{
    if ( !troop.isValid() ) {
        return false;
    }

    return JoinTroop( troop.GetMonster(), troop.GetCount(), false );
}

bool Troops::CanJoinTroops( const Troops & troops2 ) const
{
    if ( this == &troops2 )
        return false;

    Troops troops1;
    troops1.Insert( *this );

    for ( const_iterator it = troops2.begin(); it != troops2.end(); ++it ) {
        if ( ( *it )->isValid() && !troops1.JoinTroop( **it ) ) {
            return false;
        }
    }

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

void Troops::MoveTroops( Troops & from, const int monsterIdToKeep )
{
    assert( this != &from );

    // Combine troops of the same type in one slot to leave more room for new troops to join
    MergeSameMonsterTroops();

    assert( isValid() && from.isValid() );

    if ( monsterIdToKeep != Monster::UNKNOWN ) {
        // Find a troop stack in the destination stack set consisting of monsters we need to keep
        const auto keepIter = std::find_if( begin(), end(), [monsterIdToKeep]( const Troop * troop ) {
            assert( troop != nullptr );

            return troop->isValid() && troop->GetID() == monsterIdToKeep;
        } );

        // Find a troop stack in the source stack set consisting of monsters of a different type than the monster we need to keep
        const auto xchgIter = std::find_if( from.begin(), from.end(), [monsterIdToKeep]( const Troop * troop ) {
            assert( troop != nullptr );

            return troop->isValid() && troop->GetID() != monsterIdToKeep;
        } );

        // If we found both, then exchange the monster to keep in the destination stack set with the found troop stack in the
        // source stack set to concentrate all the monsters to keep in the source stack set
        if ( keepIter != end() && xchgIter != from.end() ) {
            Troop * keep = *keepIter;
            Troop * xchg = *xchgIter;

            const uint32_t count = keep->GetCount();

            keep->Reset();

            if ( !JoinTroop( *xchg ) ) {
                assert( 0 );
            }

            xchg->Reset();

            if ( !from.JoinTroop( Monster( monsterIdToKeep ), count, false ) ) {
                assert( 0 );
            }
        }
    }

    auto moveTroops = [this, &from, monsterIdToKeep]( const bool ignoreMonstersToKeep ) {
        uint32_t stacksLeft = from.GetOccupiedSlotCount();

        for ( Troop * troop : from ) {
            assert( troop != nullptr );

            if ( troop->isEmpty() ) {
                continue;
            }

            if ( ignoreMonstersToKeep && troop->GetID() == monsterIdToKeep ) {
                continue;
            }

            if ( stacksLeft == 1 ) {
                // This is the last valid troop stack in the source stack set, try to join all but one monsters from this stack and
                // then stop in any case
                if ( JoinTroop( troop->GetMonster(), troop->GetCount() - 1, false ) ) {
                    troop->SetCount( 1 );
                }

                break;
            }

            assert( stacksLeft > 1 );

            if ( JoinTroop( *troop ) ) {
                troop->Reset();

                --stacksLeft;
            }
        }
    };

    // First of all, try to move all the troop stacks except the stacks of monsters that we need to keep
    moveTroops( true );

    // Then, try to move as much of monsters to keep as we can (except the last one), if there is still a place for them
    moveTroops( false );
}

bool Troops::AllTroopsAreTheSame() const
{
    int firstMonsterId = Monster::UNKNOWN;
    for ( const Troop * troop : *this ) {
        if ( troop->isValid() ) {
            if ( firstMonsterId == Monster::UNKNOWN ) {
                firstMonsterId = troop->GetID();
            }
            else if ( troop->GetID() != firstMonsterId ) {
                return false;
            }
        }
    }
    return true;
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

void Troops::Clean()
{
    std::for_each( begin(), end(), []( Troop * troop ) { troop->Reset(); } );
}

void Troops::UpgradeTroops( const Castle & castle )
{
    for ( iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() ) {
            payment_t payment = ( *it )->GetTotalUpgradeCost();
            Kingdom & kingdom = castle.GetKingdom();

            if ( castle.GetRace() == ( *it )->GetRace() && castle.isBuild( ( *it )->GetUpgrade().GetDwelling() ) && kingdom.AllowPayment( payment ) ) {
                kingdom.OddFundsResource( payment );
                ( *it )->Upgrade();
            }
        }
}

Troop * Troops::GetFirstValid()
{
    iterator it = std::find_if( begin(), end(), []( const Troop * troop ) { return troop->isValid(); } );
    return it == end() ? nullptr : *it;
}

Troop * Troops::getBestMatchToCondition( const std::function<bool( const Troop *, const Troop * )> & condition ) const
{
    const_iterator bestMatch = std::find_if( begin(), end(), []( const Troop * troop ) {
        assert( troop != nullptr );

        return troop->isValid();
    } );

    if ( bestMatch == end() ) {
        return nullptr;
    }

    const_iterator iter = bestMatch + 1;

    while ( iter != end() ) {
        assert( *iter != nullptr );

        if ( ( *iter )->isValid() && condition( *iter, *bestMatch ) ) {
            bestMatch = iter;
        }

        ++iter;
    }

    return *bestMatch;
}

Troop * Troops::GetWeakestTroop() const
{
    return getBestMatchToCondition( Army::WeakestTroop );
}

Troop * Troops::GetSlowestTroop() const
{
    return getBestMatchToCondition( Army::SlowestTroop );
}

void Troops::MergeSameMonsterTroops()
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

Troops Troops::GetOptimized() const
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

void Troops::JoinStrongest( Troops & giverArmy, const bool keepAtLeastOneSlotForGiver )
{
    if ( this == &giverArmy )
        return;

    // validate the size (can be different from maximumTroopCount)
    if ( giverArmy.size() < size() )
        giverArmy.resize( size() );

    // first try to keep units in the same slots
    for ( size_t slot = 0; slot < size(); ++slot ) {
        Troop * leftTroop = at( slot );
        Troop * rightTroop = giverArmy[slot];
        if ( rightTroop && rightTroop->isValid() ) {
            assert( leftTroop != nullptr );

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
    for ( size_t slot = 0; slot < giverArmy.size(); ++slot ) {
        Troop * rightTroop = giverArmy[slot];
        if ( rightTroop && JoinTroop( rightTroop->GetMonster(), rightTroop->GetCount(), false ) ) {
            rightTroop->Reset();
        }
    }

    // if there's more units than slots, start optimizing
    if ( giverArmy.GetOccupiedSlotCount() > 0 ) {
        Troops rightPriority = giverArmy.GetOptimized();
        giverArmy.Clean();
        // strongest at the end
        std::sort( rightPriority.begin(), rightPriority.end(), Army::WeakestTroop );

        // 1. Merge any remaining stacks to free some space
        MergeSameMonsterTroops();

        // 2. Fill empty slots with best troops (if there are any)
        size_t count = GetOccupiedSlotCount();
        while ( count < Army::maximumTroopCount && !rightPriority.empty() ) {
            if ( !JoinTroop( *rightPriority.back() ) ) {
                // Something is wrong with calculation of free monster slots. Check the logic!
                assert( 0 );
            }
            rightPriority.PopBack();
            ++count;
        }

        // 3. Swap weakest and strongest unit until there's no left
        while ( !rightPriority.empty() ) {
            Troop * weakest = GetWeakestTroop();

            if ( !weakest || Army::StrongestTroop( weakest, rightPriority.back() ) ) {
                // we're done processing if second army units are weaker
                break;
            }

            Army::SwapTroops( *weakest, *rightPriority.back() );
            std::sort( rightPriority.begin(), rightPriority.end(), Army::WeakestTroop );
        }

        // 4. The rest goes back to second army
        while ( !rightPriority.empty() ) {
            if ( !giverArmy.JoinTroop( *rightPriority.back() ) ) {
                // We somehow cannot give the same army back.
                assert( 0 );
            }
            rightPriority.PopBack();
        }
    }

    if ( !keepAtLeastOneSlotForGiver || giverArmy.isValid() ) {
        // Either the giver army does no need extra army or it already has some.
        return;
    }

    Troop * weakest = GetWeakestTroop();
    if ( weakest == nullptr || !weakest->isValid() ) {
        // How is possible that after we have merged 2 armies no monsters are present?
        assert( 0 );
        return;
    }

    // First check if the weakest troop is actually worth to keep.
    const double weakestStrength = weakest->GetStrength();
    const double totalArmyStrength = GetStrength();
    // The weakest army should not be more than 5% from the overall army strength.
    const double strengthLimit = totalArmyStrength / 20;

    if ( weakestStrength < strengthLimit ) {
        // The weakest troop is less than limit. Just kick this weakling out.
        if ( !giverArmy.JoinTroop( *weakest, weakest->GetCount(), false ) ) {
            // The army is empty and we cannot add more troops?
            assert( 0 );
        }

        weakest->Reset();
    }
    else {
        uint32_t acceptableCount = static_cast<uint32_t>( strengthLimit / weakestStrength * weakest->GetCount() );
        assert( acceptableCount <= weakest->GetCount() );
        if ( acceptableCount > weakest->GetCount() / 2 ) {
            // No more than half.
            acceptableCount = weakest->GetCount() / 2;
        }

        if ( acceptableCount < 1 ) {
            acceptableCount = 1;
        }

        if ( !giverArmy.JoinTroop( *weakest, acceptableCount, false ) ) {
            // The army is empty and we cannot add more troops?
            assert( 0 );
        }

        weakest->SetCount( weakest->GetCount() - acceptableCount );
    }

    // Make sure that this hero can survive an attack by splitting a single stack of monsters into multiple.
    Troop * firstValidStack = giverArmy.GetFirstValid();
    assert( firstValidStack != nullptr );

    if ( firstValidStack->GetCount() > 1 ) {
        const uint32_t stackCount = std::min( static_cast<uint32_t>( giverArmy.size() ), firstValidStack->GetCount() );

        Troop temp( *firstValidStack );
        firstValidStack->Reset();

        giverArmy.addNewTroopsToFreeSlots( temp, stackCount );
    }

    // Make it less predictable to guess where troops would be. It makes human players to suffer by constantly adjusting the position of their troops.
    if ( giverArmy.GetOccupiedSlotCount() < giverArmy.size() ) {
        Rand::Shuffle( giverArmy );
    }
}

void Troops::SplitTroopIntoFreeSlots( const Troop & troop, const Troop & selectedSlot, const uint32_t slots )
{
    const iterator selectedSlotIterator = std::find( begin(), end(), &selectedSlot );

    // this means the selected slot is actually not part of the army, which is not the intended logic
    if ( selectedSlotIterator == end() ) {
        return;
    }

    addNewTroopsToFreeSlots( troop, slots );
}

void Troops::addNewTroopsToFreeSlots( const Troop & troop, uint32_t maxSlots )
{
    if ( maxSlots < 1 || GetOccupiedSlotCount() >= Size() ) {
        assert( 0 );
        return;
    }

    if ( maxSlots > Size() - GetOccupiedSlotCount() ) {
        maxSlots = static_cast<uint32_t>( Size() ) - GetOccupiedSlotCount();
    }

    const uint32_t chunk = troop.GetCount() / maxSlots;
    uint32_t remainingCount = troop.GetCount() % maxSlots;
    uint32_t remainingSlots = maxSlots;

    auto TryCreateTroopChunk = [&remainingSlots, &remainingCount, chunk, &troop]( Troop & newTroop ) {
        if ( remainingSlots <= 0 )
            return;

        if ( !newTroop.isValid() ) {
            newTroop.Set( troop.GetMonster(), remainingCount > 0 ? chunk + 1 : chunk );
            --remainingSlots;

            if ( remainingCount > 0 )
                --remainingCount;
        }
    };

    for ( size_t i = 0; i < Size(); ++i ) {
        TryCreateTroopChunk( *GetTroop( i ) );
    }
}

bool Troops::mergeWeakestTroopsIfNeeded()
{
    if ( !isFullHouse() ) {
        return true;
    }

    std::map<int, uint32_t> monsterIdVsSlotCount;
    std::map<int, double> monsterIdVsStrength;

    for ( const Troop * troop : *this ) {
        assert( troop != nullptr );
        if ( !troop->isValid() ) {
            continue;
        }

        const int id = troop->GetID();

        if ( monsterIdVsStrength.count( id ) > 0 ) {
            monsterIdVsStrength[id] += troop->GetStrength();
            ++monsterIdVsSlotCount[id];
        }
        else {
            monsterIdVsSlotCount.emplace( id, 1 );
            monsterIdVsStrength.emplace( id, troop->GetStrength() );
        }
    }

    if ( monsterIdVsStrength.size() == size() ) {
        return false;
    }

    assert( monsterIdVsSlotCount.size() == monsterIdVsStrength.size() );

    std::vector<std::pair<int, double>> sortedMultiSlotMonsterIdVsStrength;
    for ( const auto & [id, slotCount] : monsterIdVsSlotCount ) {
        if ( slotCount > 1 ) {
            sortedMultiSlotMonsterIdVsStrength.emplace_back( id, monsterIdVsStrength[id] );
        }
    }

    assert( !sortedMultiSlotMonsterIdVsStrength.empty() );

    std::sort( sortedMultiSlotMonsterIdVsStrength.begin(), sortedMultiSlotMonsterIdVsStrength.end(),
               []( const auto & left, const auto & right ) { return left.second < right.second; } );

    const int weakestMonsterToMerge = sortedMultiSlotMonsterIdVsStrength.front().first;

    // Reset the last slot.
    uint32_t monsterCount = 0;

    for ( Troop * troop : *this ) {
        assert( troop != nullptr );
        if ( !troop->isValid() ) {
            continue;
        }

        if ( troop->GetID() == weakestMonsterToMerge ) {
            monsterCount += troop->GetCount();
            troop->Reset();
        }
    }

    addNewTroopsToFreeSlots( Troop( weakestMonsterToMerge, monsterCount ), monsterIdVsSlotCount[weakestMonsterToMerge] - 1 );

    return true;
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

void Troops::JoinAllTroopsOfType( const Troop & targetTroop )
{
    const int troopID = targetTroop.GetID();
    const int totalMonsterCount = GetCountMonsters( troopID );

    for ( iterator it = begin(); it != end(); ++it ) {
        Troop * troop = *it;
        if ( !troop->isValid() || troop->GetID() != troopID )
            continue;

        if ( troop == &targetTroop ) {
            troop->SetCount( totalMonsterCount );
        }
        else {
            troop->Reset();
        }
    }
}

Army::Army( HeroBase * s )
    : commander( s )
    , combat_format( true )
    , color( Color::NONE )
{
    reserve( maximumTroopCount );
    for ( size_t i = 0; i < maximumTroopCount; ++i )
        push_back( new ArmyTroop( this ) );
}

Army::Army( const Maps::Tiles & t )
    : commander( nullptr )
    , combat_format( true )
    , color( Color::NONE )
{
    reserve( maximumTroopCount );
    for ( size_t i = 0; i < maximumTroopCount; ++i )
        push_back( new ArmyTroop( this ) );

    setFromTile( t );
}

Army::~Army()
{
    for ( iterator it = begin(); it != end(); ++it )
        delete *it;
    clear();
}

const Troops & Army::getTroops() const
{
    return *this;
}

void Army::setFromTile( const Maps::Tiles & tile )
{
    Reset();

    const bool isCaptureObject = MP2::isCaptureObject( tile.GetObject( false ) );
    if ( isCaptureObject )
        color = tile.QuantityColor();

    switch ( tile.GetObject( false ) ) {
    case MP2::OBJ_PYRAMID:
        at( 0 )->Set( Monster::VAMPIRE_LORD, 10 );
        at( 1 )->Set( Monster::ROYAL_MUMMY, 10 );
        at( 2 )->Set( Monster::ROYAL_MUMMY, 10 );
        at( 3 )->Set( Monster::ROYAL_MUMMY, 10 );
        at( 4 )->Set( Monster::VAMPIRE_LORD, 10 );
        break;

    case MP2::OBJ_GRAVEYARD:
        at( 0 )->Set( Monster::MUTANT_ZOMBIE, 20 );
        at( 1 )->Set( Monster::MUTANT_ZOMBIE, 20 );
        at( 2 )->Set( Monster::MUTANT_ZOMBIE, 20 );
        at( 3 )->Set( Monster::MUTANT_ZOMBIE, 20 );
        at( 4 )->Set( Monster::MUTANT_ZOMBIE, 20 );
        break;

    case MP2::OBJ_SHIPWRECK: {
        uint32_t count = 0;

        switch ( tile.QuantityVariant() ) {
        case 0:
            // Shipwreck guardians were defeated.
            return;
        case 1:
            count = 10;
            break;
        case 2:
            count = 15;
            break;
        case 3:
            count = 25;
            break;
        case 4:
            count = 50;
            break;
        default:
            assert( 0 );
            break;
        }

        ArrangeForBattle( Monster::GHOST, count, tile.GetIndex(), false );

        break;
    }

    case MP2::OBJ_DERELICTSHIP:
        ArrangeForBattle( Monster::SKELETON, 200, tile.GetIndex(), false );
        break;

    case MP2::OBJ_ARTIFACT:
        switch ( tile.QuantityVariant() ) {
        case 6:
            ArrangeForBattle( Monster::ROGUE, 50, tile.GetIndex(), false );
            break;
        case 7:
            ArrangeForBattle( Monster::GENIE, 1, tile.GetIndex(), false );
            break;
        case 8:
            ArrangeForBattle( Monster::PALADIN, 1, tile.GetIndex(), false );
            break;
        case 9:
            ArrangeForBattle( Monster::CYCLOPS, 1, tile.GetIndex(), false );
            break;
        case 10:
            ArrangeForBattle( Monster::PHOENIX, 1, tile.GetIndex(), false );
            break;
        case 11:
            ArrangeForBattle( Monster::GREEN_DRAGON, 1, tile.GetIndex(), false );
            break;
        case 12:
            ArrangeForBattle( Monster::TITAN, 1, tile.GetIndex(), false );
            break;
        case 13:
            ArrangeForBattle( Monster::BONE_DRAGON, 1, tile.GetIndex(), false );
            break;
        default:
            break;
        }
        break;

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

    case MP2::OBJ_DRAGONCITY: {
        uint32_t monsterCount = 1;
        if ( Settings::Get().isCampaignGameType() ) {
            const Campaign::ScenarioVictoryCondition victoryCondition = Campaign::getCurrentScenarioVictoryCondition();
            if ( victoryCondition == Campaign::ScenarioVictoryCondition::CAPTURE_DRAGON_CITY ) {
                monsterCount = 2;
            }
        }

        at( 0 )->Set( Monster::GREEN_DRAGON, monsterCount );
        at( 1 )->Set( Monster::GREEN_DRAGON, monsterCount );
        at( 2 )->Set( Monster::GREEN_DRAGON, monsterCount );
        at( 3 )->Set( Monster::RED_DRAGON, monsterCount );
        at( 4 )->Set( Monster::BLACK_DRAGON, monsterCount );
        break;
    }

    case MP2::OBJ_DAEMONCAVE:
        at( 0 )->Set( Monster::EARTH_ELEMENT, 2 );
        at( 1 )->Set( Monster::EARTH_ELEMENT, 2 );
        at( 2 )->Set( Monster::EARTH_ELEMENT, 2 );
        at( 3 )->Set( Monster::EARTH_ELEMENT, 2 );
        break;

    case MP2::OBJ_ABANDONEDMINE: {
        const Troop & troop = world.GetCapturedObject( tile.GetIndex() ).GetTroop();
        assert( troop.isValid() );

        ArrangeForBattle( troop.GetMonster(), troop.GetCount(), tile.GetIndex(), false );

        break;
    }

    default:
        if ( isCaptureObject ) {
            CapturedObject & capturedObject = world.GetCapturedObject( tile.GetIndex() );
            const Troop & troop = capturedObject.GetTroop();

            if ( troop.isValid() ) {
                ArrangeForBattle( troop.GetMonster(), troop.GetCount(), capturedObject.GetSplit() );
            }
        }
        else {
            const Troop troop = tile.QuantityTroop();

            if ( troop.isValid() ) {
                ArrangeForBattle( troop.GetMonster(), troop.GetCount(), tile.GetIndex(), true );
            }
        }
        break;
    }
}

int Army::GetColor() const
{
    const HeroBase * currentCommander = GetCommander();
    return currentCommander != nullptr ? currentCommander->GetColor() : color;
}

int Army::GetLuck() const
{
    const HeroBase * currentCommander = GetCommander();
    return currentCommander != nullptr ? currentCommander->GetLuck() : GetLuckModificator( nullptr );
}

int Army::GetLuckModificator( std::string * strs ) const
{
    int result = Luck::NORMAL;

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle ) {
        result += castle->GetLuckModificator( strs );
    }

    return result;
}

int Army::GetMorale() const
{
    const HeroBase * currentCommander = GetCommander();
    return currentCommander != nullptr ? currentCommander->GetMorale() : GetMoraleModificator( nullptr );
}

int Army::GetMoraleModificator( std::string * strs ) const
{
    // different race penalty
    std::set<int> races;
    bool hasUndead = false;
    bool allUndead = true;

    for ( const Troop * troop : *this )
        if ( troop->isValid() ) {
            races.insert( troop->GetRace() );
            hasUndead = hasUndead || troop->isUndead();
            allUndead = allUndead && troop->isUndead();
        }

    if ( allUndead )
        return Morale::NORMAL;

    int result = Morale::NORMAL;

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle ) {
        result += castle->GetMoraleModificator( strs );
    }

    // artifact "Arm of the Martyr" adds the undead morale penalty
    hasUndead = hasUndead || ( GetCommander() && GetCommander()->GetBagArtifacts().isArtifactCursePresent( fheroes2::ArtifactCurseType::UNDEAD_MORALE_PENALTY ) );

    const int count = static_cast<int>( races.size() );
    switch ( count ) {
    case 0:
    case 2:
        break;
    case 1:
        if ( !hasUndead && !AllTroopsAreTheSame() ) { // presence of undead discards "All %{race} troops +1" bonus
            ++result;
            if ( strs ) {
                std::string str = _( "All %{race} troops +1" );
                StringReplace( str, "%{race}", *races.begin() == Race::NONE ? _( "Multiple" ) : Race::String( *races.begin() ) );
                strs->append( str );
                *strs += '\n';
            }
        }
        break;
    default:
        const int penalty = count - 2;
        result -= penalty;
        if ( strs ) {
            std::string str = _( "Troops of %{count} alignments -%{penalty}" );
            StringReplace( str, "%{count}", count );
            StringReplace( str, "%{penalty}", penalty );
            strs->append( str );
            *strs += '\n';
        }
        break;
    }

    // undead in life group
    if ( hasUndead ) {
        result -= 1;
        if ( strs ) {
            strs->append( _( "Some undead in group -1" ) );
            *strs += '\n';
        }
    }

    return result;
}

double Army::GetStrength() const
{
    double result = 0;
    const uint32_t archery = ( commander != nullptr ) ? commander->GetSecondaryValues( Skill::Secondary::ARCHERY ) : 0;
    // Hero bonus calculation is slow, cache it
    const int bonusAttack = ( commander ? commander->GetAttack() : 0 );
    const int bonusDefense = ( commander ? commander->GetDefense() : 0 );
    const int armyMorale = GetMorale();
    const int armyLuck = GetLuck();

    for ( const_iterator it = begin(); it != end(); ++it ) {
        const Troop * troop = *it;
        if ( troop != nullptr && troop->isValid() ) {
            double strength = troop->GetStrengthWithBonus( bonusAttack, bonusDefense );

            if ( archery > 0 && troop->isArchers() ) {
                strength *= sqrt( 1 + static_cast<double>( archery ) / 100 );
            }

            // GetMorale checks if unit is affected by it
            if ( troop->isAffectedByMorale() )
                strength *= 1 + ( ( armyMorale < 0 ) ? armyMorale / 12.0 : armyMorale / 24.0 );

            strength *= 1 + armyLuck / 24.0;

            result += strength;
        }
    }

    if ( commander ) {
        result += commander->GetMagicStrategicValue( result );
    }

    return result;
}

void Army::Reset( const bool soft /* = false */ )
{
    Troops::Clean();

    if ( commander && commander->isHeroes() ) {
        const Monster mons1( commander->GetRace(), DWELLING_MONSTER1 );

        if ( soft ) {
            const Monster mons2( commander->GetRace(), DWELLING_MONSTER2 );

            switch ( mons1.GetID() ) {
            case Monster::PEASANT:
                JoinTroop( mons1, Rand::Get( 30, 50 ), false );
                break;
            case Monster::GOBLIN:
                JoinTroop( mons1, Rand::Get( 15, 25 ), false );
                break;
            case Monster::SPRITE:
                JoinTroop( mons1, Rand::Get( 10, 20 ), false );
                break;
            default:
                JoinTroop( mons1, Rand::Get( 6, 10 ), false );
                break;
            }

            if ( Rand::Get( 1, 10 ) != 1 ) {
                switch ( mons2.GetID() ) {
                case Monster::ARCHER:
                case Monster::ORC:
                    JoinTroop( mons2, Rand::Get( 3, 5 ), false );
                    break;
                default:
                    JoinTroop( mons2, Rand::Get( 2, 4 ), false );
                    break;
                }
            }
        }
        else {
            JoinTroop( mons1, 1, false );
        }
    }
}

HeroBase * Army::GetCommander()
{
    return ( !commander || ( commander->isCaptain() && !commander->isValid() ) ) ? nullptr : commander;
}

const Castle * Army::inCastle() const
{
    return commander ? commander->inCastle() : nullptr;
}

const HeroBase * Army::GetCommander() const
{
    return ( !commander || ( commander->isCaptain() && !commander->isValid() ) ) ? nullptr : commander;
}

int Army::GetControl() const
{
    return commander ? commander->GetControl() : ( color == Color::NONE ? CONTROL_AI : Players::GetPlayerControl( color ) );
}

uint32_t Army::getTotalCount() const
{
    return std::accumulate( begin(), end(), 0u, []( const uint32_t count, const Troop * troop ) { return troop->isValid() ? count + troop->GetCount() : count; } );
}

std::string Army::String() const
{
    std::ostringstream os;

    os << "color(" << Color::String( commander ? commander->GetColor() : color ) << "), ";

    if ( GetCommander() )
        os << "commander(" << GetCommander()->GetName() << ")";

    os << ": ";

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isValid() )
            os << std::dec << ( *it )->GetCount() << " " << ( *it )->GetName() << ", ";

    return os.str();
}

void Army::JoinStrongestFromArmy( Army & giver )
{
    const bool saveLast = ( giver.commander != nullptr ) && giver.commander->isHeroes();
    JoinStrongest( giver, saveLast );
}

uint32_t Army::ActionToSirens() const
{
    uint32_t experience = 0;

    for ( Troop * troop : *this ) {
        assert( troop != nullptr );

        if ( !troop->isValid() ) {
            continue;
        }

        const uint32_t troopCount = troop->GetCount();
        if ( troopCount == 1 ) {
            // Sirens do not affect 1 troop stack.
            continue;
        }

        // 30% of stack troops will be gone.
        uint32_t troopToRemove = troopCount * 3 / 10;
        if ( troopToRemove == 0 ) {
            // At least one unit must go, even if it's more than 30%.
            troopToRemove = 1;
        }

        troop->SetCount( troopCount - troopToRemove );
        experience += troopToRemove * static_cast<Monster *>( troop )->GetHitPoints();
    }

    return experience;
}

bool Army::isStrongerThan( const Army & target, double safetyRatio ) const
{
    if ( !target.isValid() )
        return true;

    const double str1 = GetStrength();
    const double str2 = target.GetStrength() * safetyRatio;

    DEBUG_LOG( DBG_GAME, DBG_TRACE, "Comparing troops: " << str1 << " versus " << str2 )

    return str1 > str2;
}

bool Army::isMeleeDominantArmy() const
{
    double meleeInfantry = 0;
    double other = 0;

    for ( const Troop * troop : *this ) {
        if ( troop != nullptr && troop->isValid() ) {
            if ( !troop->isArchers() && !troop->isFlying() ) {
                meleeInfantry += troop->GetStrength();
            }
            else {
                other += troop->GetStrength();
            }
        }
    }
    return meleeInfantry > other;
}

// draw MONS32 sprite in line, first valid = 0, count = 0
void Army::drawMiniMonsLine( const Troops & troops, int32_t cx, int32_t cy, uint32_t width, uint32_t first, uint32_t count )
{
    fheroes2::drawMiniMonsters( troops, cx, cy, width, first, count, Skill::Level::EXPERT, false, true, fheroes2::Display::instance() );
}

void Army::DrawMonsterLines( const Troops & troops, int32_t posX, int32_t posY, uint32_t lineWidth, uint32_t drawType, bool compact, bool isScouteView )
{
    const uint32_t count = troops.GetOccupiedSlotCount();
    const int offsetX = lineWidth / 6;
    const int offsetY = compact ? 31 : 49;

    fheroes2::Image & output = fheroes2::Display::instance();

    if ( count < 3 ) {
        fheroes2::drawMiniMonsters( troops, posX + offsetX, posY + offsetY / 2 + 1, lineWidth * 2 / 3, 0, 0, drawType, compact, isScouteView, output );
    }
    else {
        const int firstLineTroopCount = 2;
        const int secondLineTroopCount = count - firstLineTroopCount;
        const int secondLineWidth = secondLineTroopCount == 2 ? lineWidth * 2 / 3 : lineWidth;

        fheroes2::drawMiniMonsters( troops, posX + offsetX, posY, lineWidth * 2 / 3, 0, firstLineTroopCount, drawType, compact, isScouteView, output );
        fheroes2::drawMiniMonsters( troops, posX, posY + offsetY, secondLineWidth, firstLineTroopCount, secondLineTroopCount, drawType, compact, isScouteView, output );
    }
}

NeutralMonsterJoiningCondition Army::GetJoinSolution( const Heroes & hero, const Maps::Tiles & tile, const Troop & troop )
{
    // Check for creature alliance/bane campaign awards, campaign only and of course, for human players
    // creature alliance -> if we have an alliance with the appropriate creature (inc. players) they will join for free
    // creature curse/bane -> same as above but all of them will flee even if you have just 1 peasant
    if ( Settings::Get().isCampaignGameType() && hero.isControlHuman() ) {
        const std::vector<Campaign::CampaignAwardData> campaignAwards = Campaign::CampaignSaveData::Get().getObtainedCampaignAwards();

        for ( size_t i = 0; i < campaignAwards.size(); ++i ) {
            const bool isAlliance = campaignAwards[i]._type == Campaign::CampaignAwardData::TYPE_CREATURE_ALLIANCE;
            const bool isCurse = campaignAwards[i]._type == Campaign::CampaignAwardData::TYPE_CREATURE_CURSE;

            if ( !isAlliance && !isCurse )
                continue;

            Monster monster( campaignAwards[i]._subType );
            while ( true ) {
                if ( troop.GetID() == monster.GetID() ) {
                    if ( isAlliance ) {
                        return { NeutralMonsterJoiningCondition::Reason::Alliance, troop.GetCount(),
                                 Campaign::CampaignAwardData::getAllianceJoiningMessage( monster.GetID() ),
                                 Campaign::CampaignAwardData::getAllianceFleeingMessage( monster.GetID() ) };
                    }
                    else {
                        return { NeutralMonsterJoiningCondition::Reason::Bane, troop.GetCount(), nullptr,
                                 Campaign::CampaignAwardData::getBaneFleeingMessage( monster.GetID() ) };
                    }
                }

                // try to cycle through the creature's upgrades
                if ( !monster.isAllowUpgrade() )
                    break;

                monster = monster.GetUpgrade();
            }
        }
    }

    if ( hero.GetBagArtifacts().isArtifactCursePresent( fheroes2::ArtifactCurseType::NO_JOINING_ARMIES ) ) {
        return { NeutralMonsterJoiningCondition::Reason::None, 0, nullptr, nullptr };
    }

    if ( Maps::isMonsterOnTileJoinConditionSkip( tile ) || !troop.isValid() ) {
        return { NeutralMonsterJoiningCondition::Reason::None, 0, nullptr, nullptr };
    }

    // Neutral monsters don't care about hero's stats. Ignoring hero's stats makes hero's army strength be smaller in eyes of neutrals and they won't join so often.
    const double armyStrengthRatio = Troops( hero.GetArmy().getTroops() ).GetStrength() / troop.GetStrength();

    // The ability to accept monsters (a free slot or a stack of monsters of the same type) is a
    // mandatory condition for their joining in accordance with the mechanics of the original game
    if ( armyStrengthRatio > 2 && hero.GetArmy().CanJoinTroop( troop ) ) {
        if ( Maps::isMonsterOnTileJoinConditionFree( tile ) ) {
            return { NeutralMonsterJoiningCondition::Reason::Free, troop.GetCount(), nullptr, nullptr };
        }

        if ( hero.HasSecondarySkill( Skill::Secondary::DIPLOMACY ) ) {
            const uint32_t amountToJoin = Monster::GetCountFromHitPoints( troop, troop.GetHitPoints() * hero.GetSecondaryValues( Skill::Secondary::DIPLOMACY ) / 100 );

            // The ability to hire the entire stack of monsters is a mandatory condition for their joining
            // due to hero's Diplomacy skill in accordance with the mechanics of the original game
            if ( amountToJoin > 0 && hero.GetKingdom().AllowPayment( payment_t( Resource::GOLD, troop.GetTotalCost().gold ) ) ) {
                return { NeutralMonsterJoiningCondition::Reason::ForMoney, amountToJoin, nullptr, nullptr };
            }
        }
    }

    if ( armyStrengthRatio > 5 && !hero.isControlAI() ) {
        // ... surely flee before us
        return { NeutralMonsterJoiningCondition::Reason::RunAway, 0, nullptr, nullptr };
    }

    return { NeutralMonsterJoiningCondition::Reason::None, 0, nullptr, nullptr };
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

void Army::SwapTroops( Troop & t1, Troop & t2 )
{
    std::swap( t1, t2 );
}

bool Army::SaveLastTroop() const
{
    return commander && commander->isHeroes() && 1 == GetOccupiedSlotCount();
}

Monster Army::GetStrongestMonster() const
{
    Monster monster( Monster::UNKNOWN );
    for ( const Troop * troop : *this ) {
        if ( troop->isValid() && troop->GetMonster().GetMonsterStrength() > monster.GetMonsterStrength() ) {
            monster = troop->GetID();
        }
    }
    return monster;
}

void Army::resetInvalidMonsters() const
{
    for ( Troop * troop : *this ) {
        if ( troop->GetID() != Monster::UNKNOWN && !troop->isValid() ) {
            troop->Set( Monster::UNKNOWN, 0 );
        }
    }
}

void Army::ArrangeForCastleDefense( Army & garrison )
{
    assert( this != &garrison );
    // This method is designed to take reinforcements only from the garrison, because
    // it can leave the garrison empty
    assert( garrison.commander == nullptr || garrison.commander->isCaptain() );

    // There are no troops in the garrison
    if ( !garrison.isValid() ) {
        return;
    }

    // Create and fill a temporary container for convenient sorting of garrison troops
    std::vector<Troop *> garrisonTroops;

    garrisonTroops.reserve( garrison.Size() );

    for ( size_t i = 0; i < garrison.Size(); ++i ) {
        Troop * troop = garrison.GetTroop( i );
        assert( troop != nullptr );

        if ( troop->isValid() ) {
            garrisonTroops.push_back( troop );
        }
    }

    // Sort the garrison troops by their strength (most powerful stacks first)
    std::sort( garrisonTroops.begin(), garrisonTroops.end(), StrongestTroop );

    // Try to reinforce this army with garrison troops (most powerful stacks first)
    for ( Troop * troop : garrisonTroops ) {
        if ( JoinTroop( *troop ) ) {
            troop->Reset();
        }
    }
}

void Army::ArrangeForWhirlpool()
{
    // Make an "optimized" version first (each unit type occupies just one slot)
    const Troops optimizedTroops = GetOptimized();
    assert( optimizedTroops.Size() > 0 && optimizedTroops.Size() <= maximumTroopCount );

    // Already a full house, there is no room for further optimization
    if ( optimizedTroops.Size() == maximumTroopCount ) {
        return;
    }

    Assign( optimizedTroops );

    // Look for a troop consisting of the weakest units
    Troop * troopOfWeakestUnits = nullptr;

    for ( Troop * troop : *this ) {
        assert( troop != nullptr );

        if ( !troop->isValid() ) {
            continue;
        }

        if ( troopOfWeakestUnits == nullptr || troopOfWeakestUnits->GetMonsterStrength() > troop->GetMonsterStrength() ) {
            troopOfWeakestUnits = troop;
        }
    }

    assert( troopOfWeakestUnits != nullptr );
    assert( troopOfWeakestUnits->GetCount() > 0 );

    // There is already just one unit in this troop, let's leave it as it is
    if ( troopOfWeakestUnits->GetCount() == 1 ) {
        return;
    }

    // Move one unit from this troop's slot...
    troopOfWeakestUnits->SetCount( troopOfWeakestUnits->GetCount() - 1 );

    // To the separate slot
    auto emptySlot = std::find_if( begin(), end(), []( const Troop * troop ) { return troop->isEmpty(); } );
    assert( emptySlot != end() );

    ( *emptySlot )->Set( Monster( troopOfWeakestUnits->GetID() ), 1 );
}

void Army::ArrangeForBattle( const Monster & monster, const uint32_t monstersCount, const uint32_t stacksCount )
{
    assert( stacksCount > 0 && stacksCount <= size() && size() == maximumTroopCount );
    assert( std::all_of( begin(), end(), []( const Troop * troop ) { return troop->isEmpty(); } ) );

    size_t stacks;

    for ( stacks = stacksCount; stacks > 0; --stacks ) {
        if ( monstersCount / stacks > 0 ) {
            break;
        }
    }

    assert( stacks > 0 );

    const uint32_t quotient = monstersCount / static_cast<uint32_t>( stacks );
    const uint32_t remainder = monstersCount % static_cast<uint32_t>( stacks );

    assert( quotient > 0 );

    const size_t shift = ( size() - stacksCount ) / 2;

    for ( size_t i = 0; i < stacks; ++i ) {
        at( i + shift )->Set( monster, i < remainder ? quotient + 1 : quotient );
    }

    assert( std::accumulate( begin(), end(), 0U, []( const uint32_t count, const Troop * troop ) { return troop->isValid() ? count + troop->GetCount() : count; } )
            == monstersCount );
}

void Army::ArrangeForBattle( const Monster & monster, const uint32_t monstersCount, const int32_t tileIndex, const bool allowUpgrade )
{
    uint32_t stacksCount = 0;

    // Archers should always be divided into as many stacks as possible
    if ( monster.isArchers() ) {
        stacksCount = maximumTroopCount;
    }
    else {
        std::mt19937 seededGen( world.GetMapSeed() + static_cast<uint32_t>( tileIndex ) );

        stacksCount = Rand::GetWithGen( 3, 5, seededGen );
    }

    ArrangeForBattle( monster, monstersCount, stacksCount );

    if ( allowUpgrade ) {
        assert( size() % 2 == 1 );

        // An upgraded stack can be located only in the center
        Troop * troopToUpgrade = at( size() / 2 );
        assert( troopToUpgrade != nullptr );

        if ( troopToUpgrade->isValid() && troopToUpgrade->isAllowUpgrade() ) {
            std::mt19937 seededGen( world.GetMapSeed() + static_cast<uint32_t>( tileIndex ) + static_cast<uint32_t>( monster.GetID() ) );

            // 50% chance to get an upgraded stack
            if ( Rand::GetWithGen( 0, 1, seededGen ) == 1 ) {
                troopToUpgrade->Upgrade();
            }
        }
    }
}

StreamBase & operator<<( StreamBase & msg, const Army & army )
{
    msg << static_cast<uint32_t>( army.size() );

    // Army: fixed size
    for ( Army::const_iterator it = army.begin(); it != army.end(); ++it )
        msg << **it;

    return msg << army.combat_format << army.color;
}

StreamBase & operator>>( StreamBase & msg, Army & army )
{
    uint32_t armysz;
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
    army.commander = nullptr;

    return msg;
}
