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

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "ai_common.h"
#include "ai_planner.h" // IWYU pragma: associated
#include "army.h"
#include "castle.h"
#include "difficulty.h"
#include "game.h"
#include "heroes.h"
#include "kingdom.h"
#include "maps_tiles.h"
#include "monster.h"
#include "payment.h"
#include "race.h"
#include "resource.h"
#include "world.h"
#include "world_regions.h"

namespace
{
    struct BuildOrder
    {
        const BuildingType building = BUILD_NOTHING;
        const uint32_t priority = 1;

        BuildOrder() = default;
        BuildOrder( const BuildingType b, const uint32_t p )
            : building( b )
            , priority( p )
        {}
    };

    const std::vector<BuildOrder> defensiveStructures = { { BUILD_LEFTTURRET, 1 }, { BUILD_RIGHTTURRET, 1 }, { BUILD_MOAT, 1 }, { BUILD_CAPTAIN, 1 } };
    const std::vector<BuildOrder> supportingDefensiveStructures = { { BUILD_MAGEGUILD1, 1 }, { BUILD_SPEC, 2 }, { BUILD_TAVERN, 1 } };

    const std::vector<BuildOrder> & GetIncomeStructures( int type )
    {
        static const std::vector<BuildOrder> standard = { { BUILD_CASTLE, 1 }, { BUILD_STATUE, 1 }, { BUILD_MARKETPLACE, 1 } };
        static const std::vector<BuildOrder> warlock = { { BUILD_CASTLE, 1 }, { BUILD_STATUE, 1 }, { BUILD_MARKETPLACE, 1 }, { BUILD_SPEC, 1 } };

        return ( type == Race::WRLK ) ? warlock : standard;
    }

    const std::vector<BuildOrder> & GetBuildOrder( int type )
    {
        static const std::vector<BuildOrder> genericBuildOrder
            = { { BUILD_CASTLE, 2 },      { BUILD_STATUE, 1 },      { BUILD_MARKETPLACE, 1 }, { DWELLING_UPGRADE7, 1 },   { DWELLING_UPGRADE6, 1 },
                { DWELLING_MONSTER6, 1 }, { DWELLING_UPGRADE5, 1 }, { DWELLING_MONSTER5, 1 }, { DWELLING_UPGRADE4, 1 },   { DWELLING_MONSTER4, 1 },
                { DWELLING_UPGRADE3, 2 }, { DWELLING_MONSTER3, 2 }, { DWELLING_UPGRADE2, 3 }, { DWELLING_MONSTER2, 3 },   { DWELLING_MONSTER1, 4 },
                { BUILD_MAGEGUILD1, 2 },  { BUILD_WEL2, 10 },       { BUILD_TAVERN, 5 },      { BUILD_THIEVESGUILD, 10 }, { BUILD_MAGEGUILD2, 3 },
                { BUILD_MAGEGUILD3, 4 },  { BUILD_MAGEGUILD4, 5 },  { BUILD_MAGEGUILD5, 5 },  { BUILD_SHIPYARD, 4 } };

        static const std::vector<BuildOrder> barbarianBuildOrder
            = { { BUILD_CASTLE, 2 },        { BUILD_STATUE, 1 },      { BUILD_MARKETPLACE, 1 }, { DWELLING_MONSTER6, 1 }, { DWELLING_UPGRADE5, 1 },
                { DWELLING_MONSTER5, 1 },   { DWELLING_UPGRADE4, 1 }, { DWELLING_MONSTER4, 1 }, { DWELLING_MONSTER3, 1 }, { DWELLING_UPGRADE2, 2 },
                { DWELLING_MONSTER2, 2 },   { DWELLING_MONSTER1, 4 }, { BUILD_MAGEGUILD1, 3 },  { BUILD_WEL2, 10 },       { BUILD_TAVERN, 5 },
                { BUILD_THIEVESGUILD, 10 }, { BUILD_MAGEGUILD2, 4 },  { BUILD_MAGEGUILD3, 5 },  { BUILD_MAGEGUILD4, 6 },  { BUILD_MAGEGUILD5, 7 },
                { BUILD_SHIPYARD, 4 } };

        static const std::vector<BuildOrder> sorceressBuildOrder
            = { { BUILD_CASTLE, 2 },        { BUILD_STATUE, 1 },      { BUILD_MARKETPLACE, 1 }, { DWELLING_MONSTER6, 1 }, { DWELLING_MONSTER5, 1 },
                { DWELLING_MONSTER4, 1 },   { BUILD_MAGEGUILD1, 1 },  { DWELLING_MONSTER3, 1 }, { DWELLING_UPGRADE4, 1 }, { DWELLING_UPGRADE3, 2 },
                { DWELLING_UPGRADE2, 5 },   { DWELLING_MONSTER2, 2 }, { BUILD_TAVERN, 2 },      { DWELLING_MONSTER1, 4 }, { BUILD_WEL2, 10 },
                { BUILD_THIEVESGUILD, 10 }, { BUILD_MAGEGUILD2, 3 },  { BUILD_MAGEGUILD3, 4 },  { BUILD_MAGEGUILD4, 5 },  { BUILD_MAGEGUILD5, 5 },
                { BUILD_SHIPYARD, 4 } };

        // De-prioritizing dwelling 5 (you can reach 6 without it), 1 and upgrades of 3 and 4
        // Well, tavern and Archery upgrade are more important
        static const std::vector<BuildOrder> knightBuildOrder
            = { { BUILD_CASTLE, 2 },      { BUILD_STATUE, 1 },      { BUILD_MARKETPLACE, 1 },   { DWELLING_UPGRADE6, 2 }, { DWELLING_MONSTER6, 1 },
                { DWELLING_UPGRADE5, 2 }, { DWELLING_MONSTER5, 2 }, { DWELLING_UPGRADE4, 2 },   { DWELLING_MONSTER4, 1 }, { DWELLING_UPGRADE3, 2 },
                { DWELLING_MONSTER3, 1 }, { DWELLING_UPGRADE2, 1 }, { DWELLING_MONSTER2, 3 },   { DWELLING_MONSTER1, 4 }, { BUILD_WELL, 1 },
                { BUILD_TAVERN, 1 },      { BUILD_MAGEGUILD1, 2 },  { BUILD_MAGEGUILD2, 3 },    { BUILD_MAGEGUILD3, 5 },  { BUILD_MAGEGUILD4, 5 },
                { BUILD_MAGEGUILD5, 5 },  { BUILD_SPEC, 5 },        { BUILD_THIEVESGUILD, 10 }, { BUILD_WEL2, 20 },       { BUILD_SHIPYARD, 4 } };

        // Priority on Dwellings 5/6 and Mage guild level 2
        static const std::vector<BuildOrder> necromancerBuildOrder
            = { { BUILD_CASTLE, 2 },      { BUILD_STATUE, 1 },       { BUILD_MARKETPLACE, 1 }, { DWELLING_UPGRADE6, 1 }, { DWELLING_MONSTER6, 1 },
                { DWELLING_UPGRADE5, 2 }, { DWELLING_MONSTER5, 1 },  { BUILD_MAGEGUILD1, 1 },  { DWELLING_UPGRADE4, 2 }, { DWELLING_MONSTER4, 1 },
                { DWELLING_UPGRADE3, 3 }, { DWELLING_MONSTER3, 3 },  { DWELLING_UPGRADE2, 4 }, { DWELLING_MONSTER2, 2 }, { DWELLING_MONSTER1, 3 },
                { BUILD_MAGEGUILD2, 2 },  { BUILD_THIEVESGUILD, 2 }, { BUILD_WEL2, 8 },        { BUILD_MAGEGUILD3, 4 },  { BUILD_MAGEGUILD4, 5 },
                { BUILD_MAGEGUILD5, 5 },  { BUILD_SHRINE, 10 },      { BUILD_SHIPYARD, 4 } };

        // Priority on Mage tower/guild and library
        static const std::vector<BuildOrder> wizardBuildOrder
            = { { BUILD_CASTLE, 2 },        { BUILD_STATUE, 1 },      { BUILD_MARKETPLACE, 1 }, { DWELLING_UPGRADE6, 1 }, { DWELLING_MONSTER6, 1 },
                { DWELLING_UPGRADE5, 1 },   { DWELLING_MONSTER5, 1 }, { DWELLING_MONSTER4, 1 }, { DWELLING_MONSTER3, 1 }, { DWELLING_MONSTER2, 1 },
                { DWELLING_MONSTER1, 1 },   { BUILD_MAGEGUILD1, 1 },  { DWELLING_UPGRADE3, 4 }, { BUILD_SPEC, 2 },        { BUILD_WEL2, 8 },
                { BUILD_MAGEGUILD2, 3 },    { BUILD_MAGEGUILD3, 4 },  { BUILD_MAGEGUILD4, 4 },  { BUILD_MAGEGUILD5, 4 },  { BUILD_TAVERN, 10 },
                { BUILD_THIEVESGUILD, 10 }, { BUILD_SHIPYARD, 4 } };

        switch ( type ) {
        case Race::KNGT:
            return knightBuildOrder;
        case Race::NECR:
            return necromancerBuildOrder;
        case Race::WZRD:
            return wizardBuildOrder;
        case Race::SORC:
            return sorceressBuildOrder;
        case Race::BARB:
            return barbarianBuildOrder;
        default:
            break;
        }

        return genericBuildOrder;
    }

    bool Build( Castle & castle, const std::vector<BuildOrder> & buildOrderList, const uint32_t multiplier = 1 )
    {
        const int gameDifficulty = Game::getDifficulty();
        const bool isGameCampaign = Game::isCampaign();

        for ( const BuildOrder & order : buildOrderList ) {
            if ( !Difficulty::allowAIToBuildCastleBuilding( gameDifficulty, isGameCampaign, order.building ) ) {
                continue;
            }

            const uint32_t fundsMultiplier = order.priority * multiplier;

            if ( fundsMultiplier == 1 ) {
                if ( AI::BuildIfPossible( castle, order.building ) ) {
                    return true;
                }
            }
            else if ( AI::BuildIfEnoughFunds( castle, order.building, fundsMultiplier ) ) {
                return true;
            }
        }

        return false;
    }

    bool CastleDevelopment( Castle & castle, int safetyFactor, int spellLevel )
    {
        if ( !Difficulty::allowAIToDevelopCastlesOnDay( Game::getDifficulty(), Game::isCampaign(), world.CountDay() ) ) {
            return false;
        }

        if ( castle.isCastle() && !castle.isBuild( BUILD_WELL ) && world.CountDay() > 6 ) {
            // If you can't build Well, you won't be able to build anything else
            return AI::BuildIfPossible( castle, BUILD_WELL );
        }

        if ( Build( castle, GetIncomeStructures( castle.GetRace() ) ) ) {
            return true;
        }

        const size_t neighbourRegions = world.getRegion( world.getTile( castle.GetIndex() ).GetRegion() ).getNeighboursCount();
        const bool islandOrPeninsula = neighbourRegions < 3;

        // Force building a shipyard, +1 to cost check since we can have 0 neighbours
        if ( islandOrPeninsula && AI::BuildIfEnoughFunds( castle, BUILD_SHIPYARD, static_cast<uint32_t>( neighbourRegions + 1 ) ) ) {
            return true;
        }

        if ( Build( castle, GetBuildOrder( castle.GetRace() ) ) ) {
            return true;
        }

        if ( castle.GetLevelMageGuild() < spellLevel && safetyFactor > 0 ) {
            static const std::vector<BuildOrder> magicGuildUpgrades
                = { { BUILD_MAGEGUILD2, 2 }, { BUILD_MAGEGUILD3, 2 }, { BUILD_MAGEGUILD4, 1 }, { BUILD_MAGEGUILD5, 1 } };
            if ( Build( castle, magicGuildUpgrades ) ) {
                return true;
            }
        }

        // Check whether it is worth buying a boat
        const bool buyBoat = [&castle, islandOrPeninsula]() {
            // Check if it is possible to build a boat in this castle at all (except for the solvency check)
            if ( !castle.AllowBuyBoat( false ) ) {
                return false;
            }

            Kingdom & kingdom = castle.GetKingdom();

            // If there are no active heroes in the kingdom, then there will be no one to use the boat
            if ( kingdom.GetHeroes().empty() ) {
                return false;
            }

            // Perhaps the kingdom already has the necessary supply of resources
            const Funds requiredFunds = PaymentConditions::BuyBoat() * ( islandOrPeninsula ? 2 : 4 );
            if ( kingdom.AllowPayment( requiredFunds ) ) {
                return true;
            }

            // Even if the kingdom does not have the necessary supply of these resources right now, there may be enough resources of another type available to get the
            // missing resources as a result of resource exchange
            if ( !AI::calculateMarketplaceTransaction( kingdom, requiredFunds ) ) {
                return false;
            }

            // There are resources available to buy a boat right now
            if ( kingdom.AllowPayment( PaymentConditions::BuyBoat() ) ) {
                return true;
            }

            // There are no available resources, but it is possible to make a resource exchange
            if ( AI::tradeAtMarketplace( kingdom, PaymentConditions::BuyBoat() ) ) {
                return true;
            }

            assert( 0 );
            return false;
        }();

        // If yes, then buy a boat
        if ( buyBoat ) {
            castle.BuyBoat();
        }

        if ( Build( castle, defensiveStructures, 10 ) ) {
            return true;
        }

        return Build( castle, supportingDefensiveStructures, 10 );
    }
}

void AI::Planner::CastlePreBattle( Castle & castle )
{
    Heroes * hero = world.GetHero( castle );
    if ( hero == nullptr ) {
        return;
    }

    Army & army = hero->GetArmy();

    if ( !army.ArrangeForCastleDefense( castle.GetArmy() ) ) {
        return;
    }

    // Optimization cannot be performed if we have not received any reinforcements from the garrison, otherwise the actual placement of units during the battle will
    // differ from that observed by the enemy player before the start of the battle
    OptimizeTroopsOrder( army );
}

void AI::Planner::updateKingdomBudget( const Kingdom & kingdom )
{
    // clean up first
    for ( BudgetEntry & budgetEntry : _budget ) {
        budgetEntry.reset();
    }

    const Funds & kindgomFunds = kingdom.GetFunds();
    Funds requirements;
    for ( const Castle * castle : kingdom.GetCastles() ) {
        if ( !castle ) {
            continue;
        }

        const int race = castle->GetRace();
        const std::vector<BuildOrder> & buildOrder = GetBuildOrder( race );
        for ( const BuildOrder & order : buildOrder ) {
            const BuildingStatus status = castle->CheckBuyBuilding( order.building );
            if ( status == BuildingStatus::LACK_RESOURCES ) {
                Funds missing = PaymentConditions::BuyBuilding( race, order.building ) - kindgomFunds;

                requirements = requirements.max( missing );
            }
        }

        if ( castle->isBuild( DWELLING_MONSTER6 ) ) {
            Funds bestUnitCost = Monster( race, DWELLING_MONSTER6 ).GetUpgrade().GetCost();
            for ( BudgetEntry & budgetEntry : _budget ) {
                if ( bestUnitCost.Get( budgetEntry.resource ) > 0 ) {
                    budgetEntry.recurringCost = true;
                }
            }
        }
    }

    for ( BudgetEntry & budgetEntry : _budget ) {
        budgetEntry.missing = requirements.Get( budgetEntry.resource );

        if ( budgetEntry.missing ) {
            budgetEntry.priority = true;
        }
    }
}

void AI::Planner::CastleTurn( Castle & castle, const bool defensiveStrategy )
{
    if ( defensiveStrategy ) {
        const Funds & kingdomFunds = castle.GetKingdom().GetFunds();

        // If the castle is potentially under threat, then it makes sense to try to hire the maximum number of troops so that the enemy cannot hire them even if he
        // captures the castle, therefore, it is worth starting with hiring.
        castle.recruitBestAvailable( kingdomFunds );

        Army & garrison = castle.GetArmy();

        // Then we try to upgrade the existing units in the castle garrison...
        garrison.UpgradeTroops( castle );

        // ... and then we try to hire troops again, because after upgrading the existing troops, there could be a place for new units.
        castle.recruitBestAvailable( kingdomFunds );

        OptimizeTroopsOrder( garrison );

        // Avoid building monster dwellings when defensive as they might fall into enemy's hands. Instead, try to build defensive structures if there is at least some
        // kind of garrison in the castle.
        if ( castle.GetActualArmy().getTotalCount() > 0 ) {
            Build( castle, defensiveStructures );
            Build( castle, supportingDefensiveStructures );
        }
    }
    else {
        const uint32_t regionID = world.getTile( castle.GetIndex() ).GetRegion();
        const RegionStats & stats = _regions[regionID];

        CastleDevelopment( castle, stats.safetyFactor, stats.spellLevel );
    }
}
