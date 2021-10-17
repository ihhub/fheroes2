/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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
#include <cstring>

#include "castle.h"
#include "profit.h"
#include "race.h"

enum Profit : size_t
{
    GOLD250 = 0,
    GOLD500 = 1,
    GOLD750 = 2,
    GOLD1000 = 3,
    GOLD10000 = 4,
    WOOD1 = 5,
    WOOD2 = 6,
    MERCURY1 = 7,
    ORE1 = 8,
    ORE2 = 9,
    SULFUR1 = 10,
    CRYSTAL1 = 11,
    GEMS1 = 12,
    NONE = 13
};

constexpr std::array<cost_t, Profit::NONE + 1> profits = { { { 250, 0, 0, 0, 0, 0, 0 },
                                                             { 500, 0, 0, 0, 0, 0, 0 },
                                                             { 750, 0, 0, 0, 0, 0, 0 },
                                                             { 1000, 0, 0, 0, 0, 0, 0 },
                                                             { 10000, 0, 0, 0, 0, 0, 0 },
                                                             { 0, 1, 0, 0, 0, 0, 0 },
                                                             { 0, 2, 0, 0, 0, 0, 0 },
                                                             { 0, 0, 1, 0, 0, 0, 0 },
                                                             { 0, 0, 0, 1, 0, 0, 0 },
                                                             { 0, 0, 0, 2, 0, 0, 0 },
                                                             { 0, 0, 0, 0, 1, 0, 0 },
                                                             { 0, 0, 0, 0, 0, 1, 0 },
                                                             { 0, 0, 0, 0, 0, 0, 1 },
                                                             { 0, 0, 0, 0, 0, 0, 0 } } };

cost_t ProfitConditions::FromBuilding( const uint32_t building, const int race )
{
    switch ( building ) {
    case BUILD_CASTLE:
        return profits[Profit::GOLD1000];
    case BUILD_TENT:
    case BUILD_STATUE:
        return profits[Profit::GOLD250];
    case BUILD_SPEC:
        if ( race == Race::WRLK )
            return profits[Profit::GOLD500];
    default:
        break;
    }
    return profits[Profit::NONE];
}

cost_t ProfitConditions::FromArtifact( const int artifact )
{
    switch ( artifact ) {
    case Artifact::TAX_LIEN:
        return profits[Profit::GOLD250];
    case Artifact::GOLDEN_GOOSE:
        return profits[Profit::GOLD10000];
    case Artifact::ENDLESS_SACK_GOLD:
        return profits[Profit::GOLD1000];
    case Artifact::ENDLESS_BAG_GOLD:
        return profits[Profit::GOLD750];
    case Artifact::ENDLESS_PURSE_GOLD:
        return profits[Profit::GOLD500];
    case Artifact::ENDLESS_POUCH_SULFUR:
        return profits[Profit::SULFUR1];
    case Artifact::ENDLESS_VIAL_MERCURY:
        return profits[Profit::MERCURY1];
    case Artifact::ENDLESS_POUCH_GEMS:
        return profits[Profit::GEMS1];
    case Artifact::ENDLESS_CORD_WOOD:
        return profits[Profit::WOOD1];
    case Artifact::ENDLESS_CART_ORE:
        return profits[Profit::ORE1];
    case Artifact::ENDLESS_POUCH_CRYSTAL:
        return profits[Profit::CRYSTAL1];
    default:
        break;
    }

    return profits[Profit::NONE];
}

cost_t ProfitConditions::FromMine( const int type )
{
    switch ( type ) {
    case Resource::ORE:
        return profits[Profit::ORE2];
    case Resource::WOOD:
        return profits[Profit::WOOD2];
    case Resource::MERCURY:
        return profits[Profit::MERCURY1];
    case Resource::SULFUR:
        return profits[Profit::SULFUR1];
    case Resource::CRYSTAL:
        return profits[Profit::CRYSTAL1];
    case Resource::GEMS:
        return profits[Profit::GEMS1];
    case Resource::GOLD:
        return profits[Profit::GOLD1000];
    default:
        break;
    }

    return profits[Profit::NONE];
}
