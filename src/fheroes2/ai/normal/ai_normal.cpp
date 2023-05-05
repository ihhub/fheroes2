/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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

#include "ai_normal.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "army.h"
#include "maps_tiles.h"
#include "pairs.h"
#include "payment.h"
#include "profit.h"
#include "rand.h"

namespace AI
{
    Normal::Normal()
        : _pathfinder( ARMY_ADVANTAGE_LARGE )
    {
        _personality = Rand::Get( AI::WARRIOR, AI::EXPLORER );
    }

    void Normal::resetPathfinder()
    {
        _pathfinder.reset();
    }

    void Normal::revealFog( const Maps::Tiles & tile )
    {
        const MP2::MapObjectType object = tile.GetObject();
        if ( object != MP2::OBJ_NONE )
            _mapObjects.emplace_back( tile.GetIndex(), object );
    }

    double Normal::getTargetArmyStrength( const Maps::Tiles & tile, const MP2::MapObjectType objectType )
    {
        if ( !isMonsterStrengthCacheable( objectType ) ) {
            return Army( tile ).GetStrength();
        }

        const int32_t tileId = tile.GetIndex();

        auto iter = _neutralMonsterStrengthCache.find( tileId );
        if ( iter != _neutralMonsterStrengthCache.end() ) {
            // Cache hit.
            return iter->second;
        }

        auto newEntry = _neutralMonsterStrengthCache.emplace( tileId, Army( tile ).GetStrength() );
        return newEntry.first->second;
    }

    double Normal::getResourcePriorityModifier( const int resource ) const
    {
        // Not all resources are equally valuable: 1 gold does not have the same value as 1 gemstone, so we need to
        // normalize the value of various resources. Let's determine the default relative priority of resources based on
        // the ratio of the amount of resources extracted by the respective mines. For example, if a gold mine produces
        // 1000 gold per day, an ore mine produces 2 units of ore per day, and a gem mine produces 1 gem per day, then
        // the priority of one unit of ore will correspond to the priority of 500 gold, and the priority of one gemstone
        // will correspond to the priority of 1000 gold.
        static const std::map<int, double> defaultResourcePriorities = []() {
            std::map<int, double> result;

            const double goldMineIncome = ProfitConditions::FromMine( Resource::GOLD ).Get( Resource::GOLD );
            assert( goldMineIncome > 0 );

            static_assert( std::is_enum_v<decltype( Resource::ALL )> );
            using ResourceUnderlyingType = std::underlying_type_t<decltype( Resource::ALL )>;
            static_assert( std::numeric_limits<ResourceUnderlyingType>::radix == 2 );

            for ( int i = 0; i < std::numeric_limits<ResourceUnderlyingType>::digits; ++i ) {
                const int res = Resource::ALL & ( 1 << i );
                if ( res == 0 ) {
                    continue;
                }

                const int32_t resMineIncome = ProfitConditions::FromMine( res ).Get( res );
                assert( resMineIncome > 0 );
                if ( resMineIncome <= 0 ) {
                    continue;
                }

                result[res] = goldMineIncome / resMineIncome;
            }

            return result;
        }();

        double prio = 1.0;

        const auto prioIter = defaultResourcePriorities.find( resource );
        if ( prioIter != defaultResourcePriorities.end() ) {
            prio = prioIter->second;
        }
        else {
            // This function was called for an unknown resource, this should never happen
            assert( 0 );
        }

        for ( const BudgetEntry & budget : _budget ) {
            if ( budget.resource != resource ) {
                continue;
            }

            if ( budget.recurringCost ) {
                prio *= 1.5;
            }

            return ( budget.priority ) ? prio * 2.0 : prio;
        }

        return prio;
    }
}
