/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#include "army.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <utility>

#include "army_troop.h"
#include "army_ui_helper.h"
#include "artifact.h"
#include "artifact_info.h"
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "castle.h"
#include "color.h"
#include "heroes.h"
#include "heroes_base.h"
#include "kingdom.h"
#include "logging.h"
#include "luck.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "morale.h"
#include "mp2.h"
#include "race.h"
#include "rand.h"
#include "resource.h"
#include "screen.h"
#include "serialize.h"
#include "settings.h"
#include "skill.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

namespace fheroes2
{
    class Image;
}

namespace
{
    enum class ArmySize : uint32_t
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

    ArmySize getArmySize( const uint32_t count )
    {
        const ArmySize countAsEnum = static_cast<ArmySize>( count );

        if ( ArmySize::ARMY_LEGION <= countAsEnum ) {
            return ArmySize::ARMY_LEGION;
        }
        if ( ArmySize::ARMY_ZOUNDS <= countAsEnum ) {
            return ArmySize::ARMY_ZOUNDS;
        }
        if ( ArmySize::ARMY_SWARM <= countAsEnum ) {
            return ArmySize::ARMY_SWARM;
        }
        if ( ArmySize::ARMY_THRONG <= countAsEnum ) {
            return ArmySize::ARMY_THRONG;
        }
        if ( ArmySize::ARMY_HORDE <= countAsEnum ) {
            return ArmySize::ARMY_HORDE;
        }
        if ( ArmySize::ARMY_LOTS <= countAsEnum ) {
            return ArmySize::ARMY_LOTS;
        }
        if ( ArmySize::ARMY_PACK <= countAsEnum ) {
            return ArmySize::ARMY_PACK;
        }
        if ( ArmySize::ARMY_SEVERAL <= countAsEnum ) {
            return ArmySize::ARMY_SEVERAL;
        }
        return ArmySize::ARMY_FEW;
    }

    std::pair<uint32_t, uint32_t> getNumberOfMonstersInStartingArmy( const Monster & monster )
    {
        switch ( monster.GetMonsterLevel() ) {
        case 1:
            switch ( monster.GetID() ) {
            case Monster::PEASANT:
                return { 30, 50 };
            case Monster::GOBLIN:
                return { 15, 25 };
            case Monster::SPRITE:
                return { 10, 20 };
            default:
                return { 6, 10 };
            }
        case 2:
            switch ( monster.GetID() ) {
            case Monster::ARCHER:
            case Monster::ORC:
                return { 3, 5 };
            default:
                return { 2, 4 };
            }
        default:
            assert( 0 );
            break;
        }

        return { 0, 0 };
    }
}

std::string Army::TroopSizeString( const Troop & troop )
{
    std::string str;

    switch ( getArmySize( troop.GetCount() ) ) {
    case ArmySize::ARMY_FEW:
        str = _( "A few\n%{monster}" );
        break;
    case ArmySize::ARMY_SEVERAL:
        str = _( "Several\n%{monster}" );
        break;
    case ArmySize::ARMY_PACK:
        str = _( "A pack of\n%{monster}" );
        break;
    case ArmySize::ARMY_LOTS:
        str = _( "Lots of\n%{monster}" );
        break;
    case ArmySize::ARMY_HORDE:
        str = _( "A horde of\n%{monster}" );
        break;
    case ArmySize::ARMY_THRONG:
        str = _( "A throng of\n%{monster}" );
        break;
    case ArmySize::ARMY_SWARM:
        str = _( "A swarm of\n%{monster}" );
        break;
    case ArmySize::ARMY_ZOUNDS:
        str = _( "Zounds...\n%{monster}" );
        break;
    case ArmySize::ARMY_LEGION:
        str = _( "A legion of\n%{monster}" );
        break;
    default:
        // Are you passing the correct value?
        assert( 0 );
        break;
    }

    StringReplaceWithLowercase( str, "%{monster}", troop.GetMultiName() );
    return str;
}

std::string Army::SizeString( uint32_t size )
{
    switch ( getArmySize( size ) ) {
    case ArmySize::ARMY_FEW:
        return _( "army|Few" );
    case ArmySize::ARMY_SEVERAL:
        return _( "army|Several" );
    case ArmySize::ARMY_PACK:
        return _( "army|Pack" );
    case ArmySize::ARMY_LOTS:
        return _( "army|Lots" );
    case ArmySize::ARMY_HORDE:
        return _( "army|Horde" );
    case ArmySize::ARMY_THRONG:
        return _( "army|Throng" );
    case ArmySize::ARMY_SWARM:
        return _( "army|Swarm" );
    case ArmySize::ARMY_ZOUNDS:
        return _( "army|Zounds" );
    case ArmySize::ARMY_LEGION:
        return _( "army|Legion" );
    default:
        // Are you passing the correct value?
        assert( 0 );
        break;
    }

    return {};
}

Troops::Troops( const Troops & troops )
    : std::vector<Troop *>()
{
    reserve( troops.size() );

    for ( const Troop * troop : troops ) {
        assert( troop != nullptr );

        push_back( new Troop( *troop ) );
    }
}

Troops::~Troops()
{
    std::for_each( begin(), end(), []( Troop * troop ) {
        assert( troop != nullptr );

        delete troop;
    } );
}

void Troops::Assign( const Troop * itbeg, const Troop * itend )
{
    Clean();

    iterator it = begin();

    while ( it != end() && itbeg != itend ) {
        if ( itbeg->isValid() ) {
            ( *it )->Set( *itbeg );
        }

        ++it;
        ++itbeg;
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
    if ( empty() ) {
        return;
    }

    delete back();

    pop_back();
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

uint32_t Troops::GetCountMonsters( const Monster & mons ) const
{
    const int monsterId = mons.GetID();

    uint32_t result = 0;

    for ( const Troop * troop : *this ) {
        assert( troop != nullptr );

        if ( troop->isValid() && troop->isMonster( monsterId ) ) {
            result += troop->GetCount();
        }
    }

    return result;
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

        if ( auto [dummy, inserted] = monsterId.emplace( troop->GetID() ); !inserted ) {
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

bool Troops::JoinTroop( const Monster & mons, const uint32_t count, const bool emptySlotFirst )
{
    if ( !mons.isValid() || count == 0 ) {
        return false;
    }

    const auto findBestMatch = [this]( const auto higherPriorityPredicate, const auto lowerPriorityPredicate ) {
        const auto iter = std::find_if( begin(), end(), higherPriorityPredicate );
        if ( iter != end() ) {
            return iter;
        }

        return std::find_if( begin(), end(), lowerPriorityPredicate );
    };

    const auto isSlotEmpty = []( const Troop * troop ) {
        assert( troop != nullptr );

        return troop->isEmpty();
    };
    const auto isSameMonster = [&mons]( const Troop * troop ) {
        assert( troop != nullptr );

        return troop->isValid() && troop->isMonster( mons.GetID() );
    };

    const auto iter = emptySlotFirst ? findBestMatch( isSlotEmpty, isSameMonster ) : findBestMatch( isSameMonster, isSlotEmpty );
    if ( iter == end() ) {
        return false;
    }

    if ( ( *iter )->isValid() ) {
        ( *iter )->SetCount( ( *iter )->GetCount() + count );
    }
    else {
        ( *iter )->Set( mons, count );
    }

    return true;
}

bool Troops::JoinTroop( const Troop & troop )
{
    if ( !troop.isValid() ) {
        return false;
    }

    return JoinTroop( troop.GetMonster(), troop.GetCount(), false );
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

uint32_t Troops::getTotalHP() const
{
    uint32_t hp = 0;
    for ( const Troop * troop : *this ) {
        if ( troop && troop->isValid() )
            hp += troop->GetCount() * troop->GetHitPoints();
    }
    return hp;
}

void Troops::Clean()
{
    std::for_each( begin(), end(), []( Troop * troop ) { troop->Reset(); } );
}

void Troops::UpgradeTroops( const Castle & castle ) const
{
    for ( Troop * troop : *this ) {
        assert( troop != nullptr );
        if ( !troop->isValid() ) {
            continue;
        }

        if ( !troop->isAllowUpgrade() ) {
            continue;
        }

        Kingdom & kingdom = castle.GetKingdom();
        if ( castle.GetRace() != troop->GetRace() ) {
            continue;
        }

        if ( !castle.isBuild( troop->GetUpgrade().GetDwelling() ) ) {
            continue;
        }

        const Funds payment = troop->GetTotalUpgradeCost();
        if ( kingdom.AllowPayment( payment ) ) {
            kingdom.OddFundsResource( payment );
            troop->Upgrade();
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
        assert( troop != nullptr );

        if ( !troop->isValid() ) {
            continue;
        }

        const int monsterId = troop->GetID();

        for ( size_t otherSlot = slot + 1; otherSlot < size(); ++otherSlot ) {
            Troop * otherTroop = at( otherSlot );
            assert( otherTroop != nullptr );

            if ( otherTroop->isValid() && otherTroop->isMonster( monsterId ) ) {
                troop->SetCount( troop->GetCount() + otherTroop->GetCount() );

                otherTroop->Reset();
            }
        }
    }
}

bool Troops::MergeSameMonsterOnce()
{
    for ( size_t slot = 0; slot < size(); ++slot ) {
        Troop * troop = at( slot );
        assert( troop != nullptr );

        if ( !troop->isValid() ) {
            continue;
        }

        const int monsterId = troop->GetID();

        for ( size_t otherSlot = slot + 1; otherSlot < size(); ++otherSlot ) {
            Troop * otherTroop = at( otherSlot );
            assert( otherTroop != nullptr );

            if ( otherTroop->isValid() && otherTroop->isMonster( monsterId ) ) {
                troop->SetCount( troop->GetCount() + otherTroop->GetCount() );

                otherTroop->Reset();

                return true;
            }
        }
    }

    return false;
}

Troops Troops::GetOptimized() const
{
    Troops result;
    result.reserve( size() );

    for ( const Troop * troop : *this ) {
        assert( troop != nullptr );

        if ( !troop->isValid() ) {
            continue;
        }

        const int monsterId = troop->GetID();
        const const_iterator iter
            = std::find_if( result.begin(), result.end(), [monsterId]( const Troop * resultTroop ) { return resultTroop->isMonster( monsterId ); } );

        if ( iter == result.end() ) {
            result.push_back( new Troop( *troop ) );
        }
        else {
            Troop * resultTroop = *iter;
            assert( resultTroop != nullptr );

            resultTroop->SetCount( resultTroop->GetCount() + troop->GetCount() );
        }
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
        // Either the giver army does not need an extra army or it already has some.
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
    const double totalArmyStrength = Troops::GetStrength();

    assert( totalArmyStrength >= weakestStrength );

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

    const auto TryCreateTroopChunk = [&remainingSlots, &remainingCount, chunk, &troop]( Troop & newTroop ) {
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

void Troops::splitStackOfWeakestUnitsIntoFreeSlots()
{
    const uint32_t occupiedSlotCount = GetOccupiedSlotCount();

    if ( occupiedSlotCount == size() ) {
        // Nothing to do as all slots are being occupied.
        return;
    }

    // Look for a stack consisting of the weakest units
    Troop * stackOfWeakestUnits = nullptr;

    for ( Troop * troop : *this ) {
        assert( troop != nullptr );

        if ( !troop->isValid() ) {
            continue;
        }

        if ( stackOfWeakestUnits == nullptr || stackOfWeakestUnits->GetMonsterStrength() > troop->GetMonsterStrength() ) {
            stackOfWeakestUnits = troop;
        }
    }

    assert( stackOfWeakestUnits != nullptr );
    assert( stackOfWeakestUnits->GetCount() > 0 );

    const uint32_t count = std::min( static_cast<uint32_t>( size() ) - occupiedSlotCount, stackOfWeakestUnits->GetCount() - 1 );
    if ( count == 0 ) {
        return;
    }

    stackOfWeakestUnits->SetCount( stackOfWeakestUnits->GetCount() - count );

    addNewTroopsToFreeSlots( { stackOfWeakestUnits->GetMonster(), count }, count );
}

void Troops::AssignToFirstFreeSlot( const Troop & troopToAssign, const uint32_t count ) const
{
    for ( Troop * troop : *this ) {
        assert( troop != nullptr );

        if ( troop->isValid() ) {
            continue;
        }

        troop->Set( troopToAssign.GetMonster(), count );
        break;
    }
}

void Troops::JoinAllTroopsOfType( const Troop & targetTroop ) const
{
    const int troopID = targetTroop.GetID();
    const int totalMonsterCount = GetCountMonsters( troopID );

    for ( Troop * troop : *this ) {
        assert( troop != nullptr );

        if ( !troop->isValid() || troop->GetID() != troopID ) {
            continue;
        }

        if ( troop == &targetTroop ) {
            troop->SetCount( totalMonsterCount );
        }
        else {
            troop->Reset();
        }
    }
}

Army::Army( HeroBase * cmdr /* = nullptr */ )
    : commander( cmdr )
    , _isSpreadCombatFormation( true )
    , color( Color::NONE )
{
    reserve( maximumTroopCount );

    for ( size_t i = 0; i < maximumTroopCount; ++i ) {
        push_back( new ArmyTroop( this ) );
    }
}

Army::Army( const Maps::Tile & tile )
    : commander( nullptr )
    , _isSpreadCombatFormation( true )
    , color( Color::NONE )
{
    reserve( maximumTroopCount );

    for ( size_t i = 0; i < maximumTroopCount; ++i ) {
        push_back( new ArmyTroop( this ) );
    }

    setFromTile( tile );
}

const Troops & Army::getTroops() const
{
    return *this;
}

void Army::setFromTile( const Maps::Tile & tile )
{
    assert( commander == nullptr );

    Troops::Clean();

    const bool isCaptureObject = MP2::isCaptureObject( tile.getMainObjectType( false ) );
    if ( isCaptureObject ) {
        color = getColorFromTile( tile );
    }
    else {
        color = Color::NONE;
    }

    switch ( tile.getMainObjectType( false ) ) {
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

        switch ( getShipwreckCaptureCondition( tile ) ) {
        case Maps::ShipwreckCaptureCondition::EMPTY:
            // Shipwreck guardians were defeated.
            return;
        case Maps::ShipwreckCaptureCondition::FIGHT_10_GHOSTS_AND_GET_1000_GOLD:
            count = 10;
            break;
        case Maps::ShipwreckCaptureCondition::FIGHT_15_GHOSTS_AND_GET_2000_GOLD:
            count = 15;
            break;
        case Maps::ShipwreckCaptureCondition::FIGHT_25_GHOSTS_AND_GET_5000_GOLD:
            count = 25;
            break;
        case Maps::ShipwreckCaptureCondition::FIGHT_50_GHOSTS_AND_GET_2000_GOLD_WITH_ARTIFACT:
            count = 50;
            break;
        default:
            assert( 0 );
            break;
        }

        ArrangeForBattle( Monster::GHOST, count, tile.GetIndex(), false );

        break;
    }

    case MP2::OBJ_DERELICT_SHIP:
        ArrangeForBattle( Monster::SKELETON, 200, tile.GetIndex(), false );
        break;

    case MP2::OBJ_ARTIFACT:
        switch ( getArtifactCaptureCondition( tile ) ) {
        case Maps::ArtifactCaptureCondition::FIGHT_50_ROGUES:
            ArrangeForBattle( Monster::ROGUE, 50, tile.GetIndex(), false );
            break;
        case Maps::ArtifactCaptureCondition::FIGHT_1_GENIE:
            ArrangeForBattle( Monster::GENIE, 1, tile.GetIndex(), false );
            break;
        case Maps::ArtifactCaptureCondition::FIGHT_1_PALADIN:
            ArrangeForBattle( Monster::PALADIN, 1, tile.GetIndex(), false );
            break;
        case Maps::ArtifactCaptureCondition::FIGHT_1_CYCLOPS:
            ArrangeForBattle( Monster::CYCLOPS, 1, tile.GetIndex(), false );
            break;
        case Maps::ArtifactCaptureCondition::FIGHT_1_PHOENIX:
            ArrangeForBattle( Monster::PHOENIX, 1, tile.GetIndex(), false );
            break;
        case Maps::ArtifactCaptureCondition::FIGHT_1_GREEN_DRAGON:
            ArrangeForBattle( Monster::GREEN_DRAGON, 1, tile.GetIndex(), false );
            break;
        case Maps::ArtifactCaptureCondition::FIGHT_1_TITAN:
            ArrangeForBattle( Monster::TITAN, 1, tile.GetIndex(), false );
            break;
        case Maps::ArtifactCaptureCondition::FIGHT_1_BONE_DRAGON:
            ArrangeForBattle( Monster::BONE_DRAGON, 1, tile.GetIndex(), false );
            break;
        default:
            break;
        }
        break;

    case MP2::OBJ_CITY_OF_DEAD:
        at( 0 )->Set( Monster::ZOMBIE, 20 );
        at( 1 )->Set( Monster::VAMPIRE_LORD, 5 );
        at( 2 )->Set( Monster::POWER_LICH, 5 );
        at( 3 )->Set( Monster::VAMPIRE_LORD, 5 );
        at( 4 )->Set( Monster::ZOMBIE, 20 );
        break;

    case MP2::OBJ_TROLL_BRIDGE:
        at( 0 )->Set( Monster::TROLL, 4 );
        at( 1 )->Set( Monster::WAR_TROLL, 4 );
        at( 2 )->Set( Monster::TROLL, 4 );
        at( 3 )->Set( Monster::WAR_TROLL, 4 );
        at( 4 )->Set( Monster::TROLL, 4 );
        break;

    case MP2::OBJ_DRAGON_CITY: {
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

    case MP2::OBJ_DAEMON_CAVE:
        at( 0 )->Set( Monster::EARTH_ELEMENT, 2 );
        at( 1 )->Set( Monster::EARTH_ELEMENT, 2 );
        at( 2 )->Set( Monster::EARTH_ELEMENT, 2 );
        at( 3 )->Set( Monster::EARTH_ELEMENT, 2 );
        break;

    default:
        if ( isCaptureObject ) {
            const Troop & troop = world.GetCapturedObject( tile.GetIndex() ).GetTroop();

            if ( troop.isValid() ) {
                ArrangeForBattle( troop.GetMonster(), troop.GetCount(), tile.GetIndex(), false );
            }
        }
        else {
            const Troop troop = getTroopFromTile( tile );

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
                StringReplace( str, "%{race}", *races.begin() == Race::NONE ? _( "NeutralRaceTroops|Neutral" ) : Race::String( *races.begin() ) );
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
            strs->append( _( "Some undead in army -1" ) );
            *strs += '\n';
        }
    }

    return result;
}

double Army::GetStrength() const
{
    double result = 0;

    const uint32_t heroArchery = ( commander != nullptr ) ? commander->GetSecondarySkillValue( Skill::Secondary::ARCHERY ) : 0;

    const int bonusAttack = ( commander ? commander->GetAttack() : 0 );
    const int bonusDefense = ( commander ? commander->GetDefense() : 0 );
    const int armyMorale = GetMorale();
    const int armyLuck = GetLuck();

    bool troopsExist = false;

    for ( const Troop * troop : *this ) {
        assert( troop != nullptr );

        if ( troop->isEmpty() ) {
            continue;
        }

        troopsExist = true;

        double strength = troop->GetStrengthWithBonus( bonusAttack, bonusDefense );

        if ( heroArchery > 0 && troop->isArchers() ) {
            strength *= sqrt( 1 + static_cast<double>( heroArchery ) / 100 );
        }

        if ( troop->isAffectedByMorale() ) {
            strength *= 1 + ( ( armyMorale < 0 ) ? armyMorale / 12.0 : armyMorale / 24.0 );
        }

        strength *= 1 + armyLuck / 24.0;

        result += strength;
    }

    if ( commander != nullptr && troopsExist ) {
        result += commander->GetMagicStrategicValue( result );
    }

    return result;
}

void Army::Reset( const bool defaultArmy /* = false */ )
{
    Troops::Clean();

    if ( commander == nullptr || !commander->isHeroes() ) {
        return;
    }

    const int race = commander->GetRace();
    // Sometimes heroes created solely for the purpose of showing an avatar may not have a race
    if ( race == Race::NONE ) {
        return;
    }

    if ( !defaultArmy ) {
        JoinTroop( { race, DWELLING_MONSTER1 }, 1, false );

        return;
    }

    const auto joinMonsters = [this]( const Monster & monster ) {
        const auto [min, max] = getNumberOfMonstersInStartingArmy( monster );

        if ( !JoinTroop( monster, Rand::Get( min, max ), false ) ) {
            assert( 0 );
        }
    };

    joinMonsters( { race, DWELLING_MONSTER1 } );

    if ( Rand::Get( 1, 10 ) == 1 ) {
        return;
    }

    joinMonsters( { race, DWELLING_MONSTER2 } );
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
    return std::accumulate( begin(), end(), static_cast<uint32_t>( 0 ),
                            []( const uint32_t count, const Troop * troop ) { return troop->isValid() ? count + troop->GetCount() : count; } );
}

std::string Army::String() const
{
    std::ostringstream os;

    os << "color(" << Color::String( GetColor() ) << "), strength(" << GetStrength() << "), ";

    if ( const HeroBase * cmdr = GetCommander(); cmdr != nullptr ) {
        os << "commander(" << GetCommander()->GetName() << ")";
    }
    else {
        os << "commander(None)";
    }

    os << ": ";

    for ( const Troop * troop : *this ) {
        assert( troop != nullptr );

        if ( !troop->isValid() ) {
            continue;
        }

        os << std::dec << troop->GetCount() << " " << troop->GetName() << ", ";
    }

    return os.str();
}

void Army::JoinStrongestFromArmy( Army & giver )
{
    const bool saveLast = ( giver.commander != nullptr ) && giver.commander->isHeroes();
    JoinStrongest( giver, saveLast );
}

void Army::MoveTroops( Army & from, const int monsterIdToKeep )
{
    assert( this != &from );

    // Combine troops of the same type in one slot to leave more room for new troops to join
    MergeSameMonsterTroops();

    // Heroes need to have at least one occupied slot in their army, while garrisons do not need this
    const bool fromHero = from.GetCommander() && from.GetCommander()->isHeroes();
#ifndef NDEBUG
    const bool toHero = GetCommander() && GetCommander()->isHeroes();
#endif

    assert( ( !fromHero || from.isValid() ) && ( !toHero || isValid() ) );

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

    const auto moveTroops = [this, &from, monsterIdToKeep, fromHero]( const bool ignoreMonstersToKeep ) {
        uint32_t stacksLeft = from.GetOccupiedSlotCount();

        for ( Troop * troop : from ) {
            assert( troop != nullptr );

            if ( troop->isEmpty() ) {
                continue;
            }

            if ( ignoreMonstersToKeep && troop->GetID() == monsterIdToKeep ) {
                continue;
            }

            if ( fromHero ) {
                // If the source stack set belongs to a hero and this is the last valid troop stack in it,
                // then try to join all but one monsters from this stack and then stop in any case
                if ( stacksLeft == 1 ) {
                    if ( JoinTroop( troop->GetMonster(), troop->GetCount() - 1, false ) ) {
                        troop->SetCount( 1 );
                    }

                    break;
                }

                assert( stacksLeft > 1 );
            }

            if ( JoinTroop( *troop ) ) {
                troop->Reset();

                --stacksLeft;
            }
        }
    };

    // First of all, try to move all the troop stacks except the stacks of monsters that we need to keep
    moveTroops( true );

    // Then, try to move as much of monsters to keep as we can (except the last one, if we are moving them
    // from the stack set that belongs to a hero), if there is still a place for them
    moveTroops( false );
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

bool Army::isStrongerThan( const Army & target, double safetyRatio /* = 1.0 */ ) const
{
    if ( !target.isValid() ) {
        return true;
    }

    const double armyStrength = Army::GetStrength();
    const double targetStrength = target.GetStrength() * safetyRatio;

    DEBUG_LOG( DBG_GAME, DBG_TRACE, "Comparing troops: " << armyStrength << " versus " << targetStrength )

    return armyStrength > targetStrength;
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

void Army::drawSingleDetailedMonsterLine( const Troops & troops, int32_t cx, int32_t cy, int32_t width )
{
    fheroes2::drawMiniMonsters( troops, cx, cy, width, 0, 0, false, true, false, 0, fheroes2::Display::instance() );
}

void Army::drawMultipleMonsterLines( const Troops & troops, int32_t posX, int32_t posY, int32_t lineWidth, bool isCompact, const bool isDetailedView,
                                     const bool isGarrisonView /* = false */, const uint32_t thievesGuildsCount /* = 0 */ )
{
    const uint32_t count = troops.GetOccupiedSlotCount();
    const int offsetX = lineWidth / 6;
    const int offsetY = isCompact ? 31 : 49;

    fheroes2::Image & output = fheroes2::Display::instance();

    if ( count < 3 ) {
        fheroes2::drawMiniMonsters( troops, posX + offsetX, posY + offsetY / 2 + 1, lineWidth * 2 / 3, 0, 0, isCompact, isDetailedView, isGarrisonView,
                                    thievesGuildsCount, output );
    }
    else {
        const int firstLineTroopCount = 2;
        const int secondLineTroopCount = count - firstLineTroopCount;
        const int secondLineWidth = secondLineTroopCount == 2 ? lineWidth * 2 / 3 : lineWidth;

        fheroes2::drawMiniMonsters( troops, posX + offsetX, posY, lineWidth * 2 / 3, 0, firstLineTroopCount, isCompact, isDetailedView, isGarrisonView,
                                    thievesGuildsCount, output );
        fheroes2::drawMiniMonsters( troops, posX, posY + offsetY, secondLineWidth, firstLineTroopCount, secondLineTroopCount, isCompact, isDetailedView, isGarrisonView,
                                    thievesGuildsCount, output );
    }
}

NeutralMonsterJoiningCondition Army::GetJoinSolution( const Heroes & hero, const Maps::Tile & tile, const Troop & troop )
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
            const uint32_t amountToJoin
                = Monster::GetCountFromHitPoints( troop, troop.GetHitPoints() * hero.GetSecondarySkillValue( Skill::Secondary::DIPLOMACY ) / 100 );

            // The ability to hire the entire stack of monsters is a mandatory condition for their joining
            // due to hero's Diplomacy skill in accordance with the mechanics of the original game
            if ( amountToJoin > 0 && hero.GetKingdom().AllowPayment( Funds( Resource::GOLD, troop.GetTotalCost().gold ) ) ) {
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

bool Army::ArrangeForCastleDefense( Army & garrison )
{
    assert( this != &garrison );
    assert( size() == maximumTroopCount && garrison.size() == maximumTroopCount );
    // This method is designed to reinforce only the armies of heroes
    assert( commander != nullptr && commander->isHeroes() );
    // This method is designed to take reinforcements only from the garrison, because
    // it can leave the garrison empty
    assert( garrison.commander == nullptr || garrison.commander->isCaptain() );

    bool result = false;

    // If the guest hero's army is controlled by AI, then try to squeeze as many garrison troops in as possible
    if ( isControlAI() ) {
        // Create and fill a temporary container for convenient sorting of garrison troops
        std::vector<Troop *> garrisonTroops;
        garrisonTroops.reserve( garrison.Size() );

        for ( Troop * troop : garrison ) {
            assert( troop != nullptr );

            if ( troop->isValid() ) {
                garrisonTroops.push_back( troop );
            }
        }

        // Sort the garrison troops by their strength (most powerful stacks first)
        std::sort( garrisonTroops.begin(), garrisonTroops.end(), StrongestTroop );

        // Try to reinforce the guest hero's army with garrison troops (most powerful stacks first)
        for ( Troop * troop : garrisonTroops ) {
            if ( JoinTroop( *troop ) ) {
                troop->Reset();

                result = true;

                continue;
            }

            // If there is no space for another garrison stack, we will try to combine some existing stacks...
            if ( MergeSameMonsterOnce() ) {
                // ... and try again
                if ( JoinTroop( *troop ) ) {
                    troop->Reset();

                    result = true;
                }
                else {
                    assert( 0 );
                }
            }
        }

        assert( size() == maximumTroopCount );

        return result;
    }

    // Otherwise, try to move the garrison troops to exactly the same slots of the guest hero's army, provided
    // that these slots are empty
    for ( size_t i = 0; i < maximumTroopCount; ++i ) {
        Troop * troop = GetTroop( i );
        Troop * garrisonTroop = garrison.GetTroop( i );
        assert( troop != nullptr && garrisonTroop != nullptr );

        if ( !garrisonTroop->isValid() ) {
            continue;
        }

        if ( troop->isValid() ) {
            continue;
        }

        troop->Set( garrisonTroop->GetMonster(), garrisonTroop->GetCount() );

        garrisonTroop->Reset();

        result = true;
    }

    return result;
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

    assert( std::accumulate( begin(), end(), static_cast<uint32_t>( 0 ),
                             []( const uint32_t count, const Troop * troop ) { return troop->isValid() ? count + troop->GetCount() : count; } )
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

OStreamBase & operator<<( OStreamBase & stream, const Army & army )
{
    stream.put32( static_cast<uint32_t>( army.size() ) );

    std::for_each( army.begin(), army.end(), [&stream]( const Troop * troop ) {
        assert( troop != nullptr );

        stream << *troop;
    } );

    return stream << army._isSpreadCombatFormation << army.color;
}

IStreamBase & operator>>( IStreamBase & stream, Army & army )
{
    if ( const uint32_t size = stream.get32(); army.size() != size ) {
        // Most likely the save file is corrupted.
        stream.setFail();

        std::for_each( army.begin(), army.end(), []( Troop * troop ) {
            assert( troop != nullptr );

            troop->Reset();
        } );
    }
    else {
        std::for_each( army.begin(), army.end(), [&stream]( Troop * troop ) {
            assert( troop != nullptr );

            stream >> *troop;
        } );
    }

    stream >> army._isSpreadCombatFormation >> army.color;

    assert( std::all_of( army.begin(), army.end(), [&army]( const Troop * troop ) {
        const ArmyTroop * armyTroop = dynamic_cast<const ArmyTroop *>( troop );

        return armyTroop != nullptr && armyTroop->GetArmy() == &army;
    } ) );

    // Will be set later by the owner (castle or hero)
    army.commander = nullptr;

    return stream;
}
