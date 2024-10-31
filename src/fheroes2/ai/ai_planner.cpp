/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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

#include "ai_planner.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

#include "army.h"
#include "heroes.h"
#include "kingdom.h"
#include "maps_tiles.h"
#include "mp2.h"
#include "profit.h"
#include "resource.h"
#include "route.h"
#include "world.h"
#include "world_pathfinding.h"

AI::Planner & AI::Planner::Get()
{
    static Planner ai;
    return ai;
}

void AI::Planner::resetPathfinder()
{
    _pathfinder.reset();
}

void AI::Planner::revealFog( const Maps::Tile & tile, const Kingdom & kingdom )
{
    const MP2::MapObjectType object = tile.getMainObjectType();
    if ( !MP2::isInGameActionObject( object ) ) {
        return;
    }

    updateMapActionObjectCache( tile.GetIndex() );
    updatePriorityAttackTarget( kingdom, tile );

    // If this is an action object and one of AI heroes is moving,
    // we have to stop him because the new object might be more valuable than the current target.
    const VecHeroes & heroes = kingdom.GetHeroes();
    for ( Heroes * hero : heroes ) {
        if ( hero == nullptr ) {
            // How is it even possible?
            assert( 0 );
            continue;
        }

        if ( hero->isMoveEnabled() ) {
            hero->GetPath().Truncate();
            break;
        }
    }
}

double AI::Planner::getTileArmyStrength( const Maps::Tile & tile )
{
    const auto [iter, inserted] = _tileArmyStrengthValues.try_emplace( tile.GetIndex(), 0.0 );
    if ( inserted ) {
        // Creating an Army instance is a relatively heavy operation, so cache it to speed up calculations
        static Army tileArmy;
        tileArmy.setFromTile( tile );

        iter->second = tileArmy.GetStrength();
    }

    return iter->second;
}

double AI::Planner::getResourcePriorityModifier( const int resource, const bool isMine ) const
{
    // Not all resources are equally valuable: 1 gold does not have the same value as 1 gemstone, so we need to
    // normalize the value of various resources.

    // For mines, let's determine the default relative priority based on the ratio of the amount of resources
    // extracted by these mines. For example, if a gold mine produces 1000 gold per day, an ore mine produces 2
    // units of ore per day, and a gem mine produces 1 gem per day, then the priority of one unit of ore will
    // correspond to the priority of 500 gold, and the priority of one gemstone will correspond to the priority
    // of 1000 gold. Evaluate the resources from mines in proportion to the amount of resources that these mines
    // bring in 2 days (mines should be more valuable than just resource piles).
    static const std::map<int, double> minePriorities = []() {
        std::map<int, double> result;

        const double goldMineIncome = ProfitConditions::FromMine( Resource::GOLD ).Get( Resource::GOLD );
        assert( goldMineIncome > 0 );

        Resource::forEach( Resource::ALL, [&result, goldMineIncome]( const int res ) {
            const int32_t resMineIncome = ProfitConditions::FromMine( res ).Get( res );
            assert( resMineIncome > 0 );
            if ( resMineIncome <= 0 ) {
                return;
            }

            result[res] = goldMineIncome / resMineIncome * 2;
        } );

        return result;
    }();

    // For one-time resource sources (such as piles, chests, campfires and so on), let's determine the default
    // relative priority based on the ratio of the usual amount of resources in these sources.
    static const std::map<int, double> pilePriorities = { // The amount of gold on the map is usually ~500-1500
                                                          { Resource::GOLD, 1 },
                                                          // The amount of wood and ore on the map is usually ~5-10
                                                          { Resource::WOOD, 125 },
                                                          { Resource::ORE, 125 },
                                                          // The amount of other resources on the map is usually ~2-5
                                                          { Resource::MERCURY, 250 },
                                                          { Resource::SULFUR, 250 },
                                                          { Resource::CRYSTAL, 250 },
                                                          { Resource::GEMS, 250 } };

    const std::map<int, double> & resourcePriorities = isMine ? minePriorities : pilePriorities;

    double prio = 1.0;

    const auto prioIter = resourcePriorities.find( resource );
    if ( prioIter != resourcePriorities.end() ) {
        prio = prioIter->second;
    }
    else {
        // This function has been called for an unknown resource, this should never happen
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

double AI::Planner::getFundsValueBasedOnPriority( const Funds & funds ) const
{
    double value = 0;

    Resource::forEach( funds.GetValidItems(), [this, &funds, &value]( const int res ) {
        const int amount = funds.Get( res );
        if ( amount <= 0 ) {
            return;
        }

        value += amount * getResourcePriorityModifier( res, false );
    } );

    return value;
}

void AI::Planner::updateMapActionObjectCache( const int mapIndex )
{
    const MP2::MapObjectType objectType = world.getTile( mapIndex ).getMainObjectType();

    if ( !MP2::isInGameActionObject( objectType ) ) {
        _mapActionObjects.erase( mapIndex );

        return;
    }

    if ( const auto [iter, inserted] = _mapActionObjects.try_emplace( mapIndex, objectType ); !inserted ) {
        iter->second = objectType;
    }
}
