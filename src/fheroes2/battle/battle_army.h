/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#pragma once

#include <cassert>
#include <cstdint>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <vector>

#include "army.h"
#include "battle_troop.h"
#include "bitmodes.h"
#include "monster.h"

class HeroBase;

namespace Battle
{
    class TroopsUidGenerator;

    class Units : public std::vector<Unit *>
    {
    public:
        enum class FilterType
        {
            REMOVE_INVALID_UNITS,
            REMOVE_INVALID_UNITS_AND_SPECIFIED_UNIT,
            REMOVE_INVALID_UNITS_AND_UNITS_THAT_CHANGED_SIDES
        };

        static constexpr std::integral_constant<FilterType, FilterType::REMOVE_INVALID_UNITS> REMOVE_INVALID_UNITS{};
        static constexpr std::integral_constant<FilterType, FilterType::REMOVE_INVALID_UNITS_AND_SPECIFIED_UNIT> REMOVE_INVALID_UNITS_AND_SPECIFIED_UNIT{};
        static constexpr std::integral_constant<FilterType, FilterType::REMOVE_INVALID_UNITS_AND_UNITS_THAT_CHANGED_SIDES>
            REMOVE_INVALID_UNITS_AND_UNITS_THAT_CHANGED_SIDES{};

        Units();

        // Creates a shallow copy of 'units' (only pointers are copied) by applying a filter according to the specified tag
        template <FilterType filterType, typename... Types>
        Units( const Units & units, std::integral_constant<FilterType, filterType> /* tag */, const Types... params )
        {
            reserve( units.size() );

            const auto filterPredicateGenerator = []( const auto... filterParams ) {
                if constexpr ( filterType == FilterType::REMOVE_INVALID_UNITS ) {
                    static_assert( sizeof...( filterParams ) == 0 );

                    return []( const Unit * unit ) {
                        assert( unit != nullptr );

                        return unit->isValid();
                    };
                }
                else if constexpr ( filterType == FilterType::REMOVE_INVALID_UNITS_AND_SPECIFIED_UNIT ) {
                    static_assert( sizeof...( filterParams ) == 1 );

                    return [unitToRemove = std::get<0>( std::tie( filterParams... ) )]( const Unit * unit ) {
                        assert( unit != nullptr );

                        return unit->isValid() && unit != unitToRemove;
                    };
                }
                else if constexpr ( filterType == FilterType::REMOVE_INVALID_UNITS_AND_UNITS_THAT_CHANGED_SIDES ) {
                    static_assert( sizeof...( filterParams ) == 0 );

                    return []( const Unit * unit ) {
                        assert( unit != nullptr );

                        return unit->isValid() && unit->GetColor() == unit->GetCurrentColor();
                    };
                }
                else {
                    // The build should fail because this lambda does not meet the requirements of UnaryPredicate
                    return []() { assert( 0 ); };
                }
            };

            std::copy_if( units.begin(), units.end(), std::back_inserter( *this ), filterPredicateGenerator( params... ) );
        }

        Units( const Units & ) = delete;

        virtual ~Units() = default;

        Units & operator=( const Units & ) = delete;

        Unit * FindMode( const uint32_t mod ) const;
        Unit * FindUID( const uint32_t uid ) const;

        void SortFastest();
    };

    class Force : public Units, public BitModes
    {
    public:
        Force( Army & parent, bool opposite, TroopsUidGenerator & generator );

        Force( const Force & ) = delete;

        ~Force() override;

        Force & operator=( const Force & ) = delete;

        HeroBase * GetCommander();
        const HeroBase * GetCommander() const;

        const Units & getUnits() const;

        bool isValid( const bool considerBattlefieldArmy = true ) const;
        bool HasMonster( const Monster & ) const;

        uint32_t GetDeadHitPoints() const;
        uint32_t GetDeadCounts() const;

        int GetColor() const;
        int GetControl() const;

        // Returns the cost of surrender (in units of gold) for the current army on the battlefield
        uint32_t GetSurrenderCost() const;

        Troops GetKilledTroops() const;

        bool animateIdleUnits() const;
        void resetIdleAnimation() const;

        void NewTurn();
        void SyncArmyCount();

    private:
        Army & army;
        std::vector<uint32_t> uids;
    };
}
