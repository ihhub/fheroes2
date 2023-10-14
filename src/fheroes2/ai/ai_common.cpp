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
#include <vector>

#include "ai.h"
#include "army.h"
#include "army_troop.h"
#include "castle.h"
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

    bool BuildIfPossible( Castle & castle, const int building )
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

    bool BuildIfEnoughFunds( Castle & castle, const int building, const int fundsMultiplier )
    {
        if ( fundsMultiplier < 1 || fundsMultiplier > 99 ) {
            return false;
        }

        const Kingdom & kingdom = castle.GetKingdom();
        const uint32_t marketplaceCount = kingdom.GetCountMarketplace();

        const Funds fundsAvailable = kingdom.GetFunds();
        const Funds fundsRequired = PaymentConditions::BuyBuilding( castle.GetRace(), building );

        if ( marketplaceCount == 0 ) {
            // If there are no markets in the kingdom, then the exchange of resources is impossible. Compare resources directly.
            if ( fundsAvailable < fundsRequired * fundsMultiplier ) {
                return false;
            }
        }
        else {
            const auto fundsToGold = [marketplaceCount]( const Funds & funds, const bool useResourcePurchaseRate ) {
                int32_t result = 0;

                Resource::forEach( Resource::ALL, [marketplaceCount, &funds, useResourcePurchaseRate, &result]( const int res ) {
                    const int32_t amount = funds.Get( res );
                    if ( amount == 0 ) {
                        return;
                    }

                    if ( res == Resource::GOLD ) {
                        result += amount;

                        return;
                    }

                    const int32_t tradeCost = useResourcePurchaseRate ? fheroes2::getTradeCost( marketplaceCount, Resource::GOLD, res )
                                                                      : fheroes2::getTradeCost( marketplaceCount, res, Resource::GOLD );
                    assert( tradeCost > 0 );

                    result += amount * tradeCost;
                } );

                return result;
            };

            // If there are markets in the kingdom, then an exchange of resources is possible. For comparison, we convert resources into money as follows: the resources
            // available to the kingdom are converted into money at the resource sale rate, and the resources needed to buy a building are converted into money at the
            // resource purchase rate.
            if ( fundsToGold( fundsAvailable, false ) < fundsToGold( fundsRequired, true ) * fundsMultiplier ) {
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

    bool tradeAtMarketplace( Kingdom & kingdom, const Funds & fundsToObtain )
    {
        const uint32_t marketplaceCount = kingdom.GetCountMarketplace();
        if ( marketplaceCount == 0 ) {
            return false;
        }

        static const std::vector<int> resourcePriorities{ Resource::WOOD,    Resource::ORE,  Resource::MERCURY, Resource::SULFUR,
                                                          Resource::CRYSTAL, Resource::GEMS, Resource::GOLD };

        Funds plannedBalance = kingdom.GetFunds() - fundsToObtain;
        Funds plannedPayment;

        bool tradeFailed = false;

        Resource::forEach( Resource::ALL, [marketplaceCount, &plannedBalance, &plannedPayment, &tradeFailed]( const int res ) {
            int32_t & missingResAmount = *( plannedBalance.GetPtr( res ) );
            if ( missingResAmount >= 0 ) {
                return;
            }

            for ( const int resForSale : resourcePriorities ) {
                int32_t & saleResAmount = *( plannedBalance.GetPtr( resForSale ) );

                if ( saleResAmount <= 0 ) {
                    continue;
                }

                const int32_t tradeCost = fheroes2::getTradeCost( marketplaceCount, resForSale, res );
                assert( tradeCost > 0 );

                // If resources are sold for gold, then by giving away one unit of the resource we get several units of gold
                if ( res == Resource::GOLD ) {
                    const int32_t amountToSell
                        = std::min( ( -missingResAmount ) % tradeCost == 0 ? ( -missingResAmount ) / tradeCost : ( -missingResAmount ) / tradeCost + 1, saleResAmount );
                    assert( amountToSell > 0 );

                    saleResAmount -= amountToSell;
                    missingResAmount += amountToSell * tradeCost;

                    assert( saleResAmount >= 0 );

                    // Since Kingdom::OddFundsResource() will be used, the volume of resources sold should be positive, and the volume of resources bought should be
                    // negative
                    *( plannedPayment.GetPtr( resForSale ) ) += amountToSell;
                    *( plannedPayment.GetPtr( res ) ) -= amountToSell * tradeCost;
                }
                // Otherwise (when exchanging gold for a resource or a resource for a resource) by giving away several units of a resource, we get only one unit of
                // another resource
                else {
                    const int32_t amountToBuy = std::min( -missingResAmount, saleResAmount / tradeCost );
                    assert( amountToBuy >= 0 );

                    if ( amountToBuy == 0 ) {
                        continue;
                    }

                    saleResAmount -= amountToBuy * tradeCost;
                    missingResAmount += amountToBuy;

                    assert( saleResAmount >= 0 );

                    // Since Kingdom::OddFundsResource() will be used, the volume of resources sold should be positive, and the volume of resources bought should be
                    // negative
                    *( plannedPayment.GetPtr( resForSale ) ) += amountToBuy * tradeCost;
                    *( plannedPayment.GetPtr( res ) ) -= amountToBuy;
                }

                if ( missingResAmount >= 0 ) {
                    // If we exchange resources for gold, we can get a little more than the amount we need. In other cases, we should get exactly the amount we need.
                    assert( res == Resource::GOLD || missingResAmount == 0 );

                    return;
                }
            }

            tradeFailed = true;
        } );

        if ( tradeFailed ) {
            DEBUG_LOG( DBG_AI, DBG_TRACE, Color::String( kingdom.GetColor() ) << " failed to obtain funds " << fundsToObtain.String() )

            return false;
        }

        // If the trade is successful, then we should not have any "debts" in the planned balance...
        assert( [&plannedBalance]() {
            bool valid = true;

            Resource::forEach( Resource::ALL, [&plannedBalance, &valid]( const int res ) {
                if ( plannedBalance.Get( res ) < 0 ) {
                    valid = false;
                }
            } );

            return valid;
        }() );

        // ... and the kingdom should be able to conduct an appropriate financial transaction
        assert( kingdom.AllowPayment( plannedPayment ) );

        DEBUG_LOG( DBG_AI, DBG_INFO,
                   Color::String( kingdom.GetColor() ) << " obtained funds " << fundsToObtain.String() << " through a trading operation " << plannedPayment.String() )

        kingdom.OddFundsResource( plannedPayment );

        return true;
    }
}
