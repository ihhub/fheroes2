/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

#include "resource_trading.h"

#include <array>
#include <cassert>
#include <cstddef>

#include "resource.h"

namespace
{
    // Resources for trade are divided into 3 categories:
    // - common such as Ore and Wood
    // - rare such as Mercury, Sulfur, Crystal and Gems
    // - Gold
    //
    // So we could have the following 8 combinations:
    // common --> common
    // common --> rare
    // common --> gold
    // rare   --> common
    // rare   --> rare
    // rare   --> gold
    // gold   --> common
    // gold   --> rare
    //
    // However, common --> common and rare --> rare should have the same conversion ratio as these are within category.
    // So are left with 7 combinations.
    const size_t maxMarketplaces{ 9 };

    const std::array<uint32_t, maxMarketplaces> resourceToResource{ 10, 7, 5, 4, 4, 3, 3, 3, 2 };
    const std::array<uint32_t, maxMarketplaces> commonToRare{ 20, 14, 10, 8, 7, 6, 5, 5, 4 };
    const std::array<uint32_t, maxMarketplaces> rareToCommon{ 5, 4, 3, 2, 2, 2, 2, 2, 1 };
    const std::array<uint32_t, maxMarketplaces> commonToGold{ 25, 37, 50, 62, 74, 87, 100, 112, 124 };
    const std::array<uint32_t, maxMarketplaces> rareToGold{ 50, 74, 100, 124, 149, 175, 200, 224, 249 };
    const std::array<uint32_t, maxMarketplaces> goldToCommon{ 2500, 1667, 1250, 1000, 834, 715, 625, 556, 500 };
    const std::array<uint32_t, maxMarketplaces> goldToRare{ 5000, 3334, 2500, 2000, 1667, 1429, 1250, 1112, 1000 };
}

namespace fheroes2
{
    uint32_t getTradeCost( uint32_t marketplaceCount, const int resourceFrom, const int resourceTo )
    {
        if ( marketplaceCount == 0 ) {
            assert( 0 );
            return 0;
        }

        if ( resourceFrom == resourceTo || resourceFrom == Resource::UNKNOWN || resourceTo == Resource::UNKNOWN ) {
            // The resource is not selected (Resource::UNKNOWN) or resources are the same.
            // What are we trying to achieve?
            return 0;
        }

        if ( marketplaceCount > 9 ) {
            marketplaceCount = 9;
        }

        --marketplaceCount;

        switch ( resourceFrom ) {
        case Resource::GOLD: {
            switch ( resourceTo ) {
            case Resource::WOOD:
            case Resource::ORE:
                return goldToCommon[marketplaceCount];
            case Resource::MERCURY:
            case Resource::SULFUR:
            case Resource::CRYSTAL:
            case Resource::GEMS:
                return goldToRare[marketplaceCount];
            default:
                assert( 0 );
            }
            return 0;
        }

        case Resource::WOOD:
        case Resource::ORE: {
            switch ( resourceTo ) {
            case Resource::WOOD:
            case Resource::ORE:
                return resourceToResource[marketplaceCount];
            case Resource::GOLD:
                return commonToGold[marketplaceCount];
            case Resource::MERCURY:
            case Resource::SULFUR:
            case Resource::CRYSTAL:
            case Resource::GEMS:
                return commonToRare[marketplaceCount];
            default:
                assert( 0 );
            }
            return 0;
        }

        case Resource::MERCURY:
        case Resource::SULFUR:
        case Resource::CRYSTAL:
        case Resource::GEMS: {
            switch ( resourceTo ) {
            case Resource::WOOD:
            case Resource::ORE:
                return rareToCommon[marketplaceCount];
            case Resource::GOLD:
                return rareToGold[marketplaceCount];
            case Resource::MERCURY:
            case Resource::SULFUR:
            case Resource::CRYSTAL:
            case Resource::GEMS:
                return resourceToResource[marketplaceCount];
            default:
                assert( 0 );
            }
            return 0;
        }

        default:
            assert( 0 );
            return 0;
        }
    }
}
