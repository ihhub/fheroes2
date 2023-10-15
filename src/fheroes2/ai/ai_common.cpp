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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "ai.h"
#include "army.h"
#include "army_troop.h"
#include "castle.h"
#include "color.h"
#include "difficulty.h"
#include "game.h"
#include "kingdom.h"
#include "logging.h"
#include "normal/ai_normal.h"
#include "payment.h"
#include "resource.h"
#include "resource_trading.h"
#include "settings.h"

namespace AI
{
    Base & Get( AI_TYPE /* type */ )
    {
        static AI::Normal normal;
        return normal;
    }

    bool BuildIfPossible( Castle & castle, const building_t building )
    {
        switch ( castle.CheckBuyBuilding( building ) ) {
        case LACK_RESOURCES: {
            const Funds payment = PaymentConditions::BuyBuilding( castle.GetRace(), building );

            if ( !tradeAtMarketplace( castle.GetKingdom(), payment ) ) {
                return false;
            }

            break;
        }
        case ALLOW_BUILD:
            break;
        default:
            return false;
        }

        const bool result = castle.BuyBuilding( building );
        assert( result );

        return result;
    }

    bool BuildIfEnoughFunds( Castle & castle, const building_t building, const uint32_t fundsMultiplier )
    {
        if ( fundsMultiplier < 1 || fundsMultiplier > 99 ) {
            return false;
        }

        const Kingdom & kingdom = castle.GetKingdom();

        const Funds fundsAvailable = kingdom.GetFunds();
        const Funds fundsRequired = PaymentConditions::BuyBuilding( castle.GetRace(), building );

        if ( fundsAvailable < fundsRequired * fundsMultiplier ) {
            // It makes sense to try to trade on the market only if we currently do not have enough resources to build this building
            if ( castle.CheckBuyBuilding( building ) != LACK_RESOURCES ) {
                return false;
            }

            const std::optional<Funds> marketplaceTransaction = calculateMarketplaceTransaction( kingdom, fundsRequired );
            if ( !marketplaceTransaction ) {
                return false;
            }

            assert( marketplaceTransaction->GetValidItemsCount() > 0 );

            const Funds fundsRequiredWithTransaction = fundsRequired + *marketplaceTransaction;
            assert( [&fundsRequiredWithTransaction]() {
                bool valid = true;

                Resource::forEach( Resource::ALL, [&fundsRequiredWithTransaction, &valid]( const int res ) {
                    // When exchanging resources for gold, we can get a little more gold than we need. Since the resources that we buy are indicated with a minus sign in
                    // the trade transaction, then we can get a negative amount of gold required to build a building after applying this transaction to the building
                    // construction requirements. For example, if 500 gold is needed to build a building, the kingdom has 0 gold, but we sell 19 units of wood for 25 gold
                    // per unit and 1 unit of mercury for 50 gold, then we will get 525 units of gold. Thus, after the construction of the building, taking into account
                    // the trade transaction, we will have more gold than before its construction (25 gold vs 0 gold).
                    if ( res == Resource::GOLD ) {
                        return;
                    }

                    if ( fundsRequiredWithTransaction.Get( res ) >= 0 ) {
                        return;
                    }

                    valid = false;
                } );

                return valid;
            }() );

            // Make sure that after the construction of the building, taking into account the trade transaction, we still have a sufficient supply of resources
            if ( fundsAvailable < fundsRequiredWithTransaction * fundsMultiplier ) {
                return false;
            }
        }

        return BuildIfPossible( castle, building );
    }

    void OptimizeTroopsOrder( Army & army )
    {
        if ( !army.isValid() ) {
            return;
        }

        // Optimize troops placement in case of a battle
        army.MergeSameMonsterTroops();

        // Validate and pick the troops
        std::vector<Troop> archers;
        std::vector<Troop> others;

        for ( size_t slot = 0; slot < Army::maximumTroopCount; ++slot ) {
            const Troop * troop = army.GetTroop( slot );
            if ( troop && troop->isValid() ) {
                if ( troop->isArchers() ) {
                    archers.push_back( *troop );
                }
                else {
                    others.push_back( *troop );
                }
            }
        }

        // Sort troops by tactical priority. For melee:
        // 1. Faster units first
        // 2. Flyers first
        // 3. Finally if unit type and speed is same, compare by strength
        std::sort( others.begin(), others.end(), []( const Troop & left, const Troop & right ) {
            if ( left.GetSpeed() == right.GetSpeed() ) {
                if ( left.isFlying() == right.isFlying() ) {
                    return left.GetStrength() < right.GetStrength();
                }
                return right.isFlying();
            }
            return left.GetSpeed() < right.GetSpeed();
        } );

        // Archers sorted purely by strength.
        std::sort( archers.begin(), archers.end(), []( const Troop & left, const Troop & right ) { return left.GetStrength() < right.GetStrength(); } );

        std::vector<size_t> slotOrder = { 2, 1, 3, 0, 4 };
        switch ( archers.size() ) {
        case 1:
            slotOrder = { 0, 2, 1, 3, 4 };
            break;
        case 2:
        case 3:
            slotOrder = { 0, 4, 2, 1, 3 };
            break;
        case 4:
            slotOrder = { 0, 4, 2, 3, 1 };
            break;
        case 5:
            slotOrder = { 0, 4, 1, 2, 3 };
            break;
        default:
            break;
        }

        // Re-arrange troops in army
        army.Clean();
        for ( const size_t slot : slotOrder ) {
            if ( !archers.empty() ) {
                army.GetTroop( slot )->Set( archers.back() );
                archers.pop_back();
            }
            else if ( !others.empty() ) {
                army.GetTroop( slot )->Set( others.back() );
                others.pop_back();
            }
            else {
                break;
            }
        }

        if ( Difficulty::allowAIToSplitWeakStacks( Game::getDifficulty() ) ) {
            // Complicate the task of a potential attacker
            army.splitStackOfWeakestUnitsIntoFreeSlots();
        }
    }

    bool CanPurchaseHero( const Kingdom & kingdom )
    {
        if ( kingdom.GetCountCastle() == 0 ) {
            return false;
        }

        if ( kingdom.GetColor() == Settings::Get().CurrentColor() ) {
            // This is the AI's current turn.
            return kingdom.AllowPayment( PaymentConditions::RecruitHero() );
        }

        // This is not the current turn for the AI so we need to roughly calculate the possible future income on the next day.
        return kingdom.AllowPayment( PaymentConditions::RecruitHero() - kingdom.GetIncome() );
    }

    std::optional<Funds> calculateMarketplaceTransaction( const Kingdom & kingdom, const Funds & fundsToObtain )
    {
        const uint32_t marketplaceCount = kingdom.GetCountMarketplace();
        if ( marketplaceCount == 0 ) {
            return {};
        }

        std::map<int, int32_t> plannedBalance;
        {
            const Funds fundsDiff = kingdom.GetFunds() - fundsToObtain;

            Resource::forEach( Resource::ALL, [&plannedBalance, &fundsDiff]( const int res ) {
                const auto [dummy, inserted] = plannedBalance.try_emplace( res, fundsDiff.Get( res ) );
                if ( !inserted ) {
                    assert( 0 );
                }
            } );

            assert( !plannedBalance.empty() );
        }

        Funds plannedTransaction;

        const auto exchangeResources = [marketplaceCount, &plannedBalance, &plannedTransaction]( const int fromRes, const int toRes ) {
            assert( fromRes != toRes );

            auto fromResBalanceIter = plannedBalance.find( fromRes );
            auto toResBalanceIter = plannedBalance.find( toRes );

            assert( fromResBalanceIter != plannedBalance.end() && toResBalanceIter != plannedBalance.end() );

            auto & [fromResDummy, fromResBalanceAmount] = *fromResBalanceIter;
            auto & [toResDummy, toResBalanceAmount] = *toResBalanceIter;

            assert( fromResBalanceAmount > 0 && toResBalanceAmount < 0 );

            const int32_t tradeCost = fheroes2::getTradeCost( marketplaceCount, fromRes, toRes );
            assert( tradeCost > 0 );

            int32_t * fromResTransactionAmount = plannedTransaction.GetPtr( fromRes );
            int32_t * toResTransactionAmount = plannedTransaction.GetPtr( toRes );

            assert( fromResTransactionAmount != nullptr && toResTransactionAmount != nullptr && fromResTransactionAmount != toResTransactionAmount );

            // If resources are exchanged for gold, then by giving away one unit of the resource we get several units of gold
            if ( toRes == Resource::GOLD ) {
                fromResBalanceAmount -= 1;
                toResBalanceAmount += tradeCost;

                // Since the planned transaction must be deducted from the funds of the kingdom, the volume of exchanged resources should be positive, and the volume of
                // acquired resources should be negative
                *fromResTransactionAmount += 1;
                *toResTransactionAmount -= tradeCost;
            }
            // Otherwise (when exchanging gold for a resource or a resource for a resource) by giving away several units of a resource, we get only one unit of another
            // resource
            else {
                fromResBalanceAmount -= tradeCost;
                toResBalanceAmount += 1;

                // Since the planned transaction must be deducted from the funds of the kingdom, the volume of exchanged resources should be positive, and the volume of
                // acquired resources should be negative
                *fromResTransactionAmount += tradeCost;
                *toResTransactionAmount -= 1;
            }

            assert( fromResBalanceAmount >= 0 );
        };

        for ( const auto & [missingRes, missingResAmount] : plannedBalance ) {
            if ( missingResAmount >= 0 ) {
                continue;
            }

            while ( missingResAmount < 0 ) {
                // The exchange of resources takes place one unit at a time. Each time, the resource whose relative stock is currently maximum is selected for exchange
                // (that is, the resource whose current stock can be exchanged for the maximum amount of the target resource).

                int resToExchange = Resource::UNKNOWN;
                int32_t maxPotentialRevenue = 0;

                for ( const auto & [res, resAmount] : plannedBalance ) {
                    if ( res == missingRes ) {
                        continue;
                    }

                    if ( resAmount <= 0 ) {
                        continue;
                    }

                    const int32_t tradeCost = fheroes2::getTradeCost( marketplaceCount, res, missingRes );
                    assert( tradeCost > 0 );

                    const int32_t potentialRevenue = ( missingRes == Resource::GOLD ? resAmount * tradeCost : resAmount / tradeCost );
                    if ( maxPotentialRevenue < potentialRevenue ) {
                        resToExchange = res;
                        maxPotentialRevenue = potentialRevenue;
                    }
                }

                if ( resToExchange == Resource::UNKNOWN ) {
                    return {};
                }

                exchangeResources( resToExchange, missingRes );
            }

            // If we exchange resources for gold, we can get a little more than the amount we need. In other cases, we should get exactly the amount we need.
            assert( missingRes == Resource::GOLD ? missingResAmount >= 0 : missingResAmount == 0 );
        }

        // If the trade is successful, then we should not have any "debts" in the planned balance...
        assert( std::none_of( plannedBalance.cbegin(), plannedBalance.cend(), []( const auto & item ) { return ( item.second < 0 ); } ) );

        // ... and the kingdom should be able to conduct the planned transaction
        assert( kingdom.AllowPayment( plannedTransaction ) );

        return plannedTransaction;
    }

    bool tradeAtMarketplace( Kingdom & kingdom, const Funds & fundsToObtain )
    {
        const std::optional<Funds> transaction = calculateMarketplaceTransaction( kingdom, fundsToObtain );
        if ( !transaction ) {
            DEBUG_LOG( DBG_AI, DBG_TRACE,
                       Color::String( kingdom.GetColor() ) << " having funds " << kingdom.GetFunds().String() << " failed to obtain funds " << fundsToObtain.String() )

            return false;
        }

        assert( kingdom.AllowPayment( *transaction ) );

        DEBUG_LOG( DBG_AI, DBG_INFO,
                   Color::String( kingdom.GetColor() ) << " having funds " << kingdom.GetFunds().String() << " obtains funds " << fundsToObtain.String()
                                                       << " through a transaction " << transaction->String() )

        kingdom.OddFundsResource( *transaction );

        return true;
    }
}
