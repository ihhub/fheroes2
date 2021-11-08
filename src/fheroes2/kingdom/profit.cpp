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

#include "castle.h"
#include "profit.h"
#include "race.h"

payment_t ProfitConditions::FromBuilding( const uint32_t building, const int race )
{
    switch ( building ) {
    case BUILD_CASTLE:
        return { 0, 0, 0, 0, 0, 0, 1000 };
    case BUILD_TENT:
    case BUILD_STATUE:
        return { 0, 0, 0, 0, 0, 0, 250 };
    case BUILD_SPEC:
        if ( race == Race::WRLK )
            return { 0, 0, 0, 0, 0, 0, 500 };
        break;
    default:
        break;
    }

    return {};
}

payment_t ProfitConditions::FromArtifact( const int artifact )
{
    switch ( artifact ) {
    case Artifact::TAX_LIEN:
        return { 0, 0, 0, 0, 0, 0, 250 };
    case Artifact::GOLDEN_GOOSE:
        return { 0, 0, 0, 0, 0, 0, 10000 };
    case Artifact::ENDLESS_SACK_GOLD:
        return { 0, 0, 0, 0, 0, 0, 1000 };
    case Artifact::ENDLESS_BAG_GOLD:
        return { 0, 0, 0, 0, 0, 0, 750 };
    case Artifact::ENDLESS_PURSE_GOLD:
        return { 0, 0, 0, 0, 0, 0, 500 };
    case Artifact::ENDLESS_POUCH_SULFUR:
        return { 0, 0, 0, 1, 0, 0, 0 };
    case Artifact::ENDLESS_VIAL_MERCURY:
        return { 0, 1, 0, 0, 0, 0, 0 };
    case Artifact::ENDLESS_POUCH_GEMS:
        return { 0, 0, 0, 0, 0, 1, 0 };
    case Artifact::ENDLESS_CORD_WOOD:
        return { 0, 1, 0, 0, 0, 0, 0 };
    case Artifact::ENDLESS_CART_ORE:
        return { 1, 0, 0, 0, 0, 0, 0 };
    case Artifact::ENDLESS_POUCH_CRYSTAL:
        return { 0, 0, 0, 0, 1, 0, 0 };
    default:
        break;
    }

    return {};
}

payment_t ProfitConditions::FromMine( const int type )
{
    switch ( type ) {
    case Resource::ORE:
        return { 2, 0, 0, 0, 0, 0, 0 };
    case Resource::WOOD:
        return { 0, 2, 0, 0, 0, 0, 0 };
    case Resource::MERCURY:
        return { 0, 0, 1, 0, 0, 0, 0 };
    case Resource::SULFUR:
        return { 0, 0, 0, 1, 0, 0, 0 };
    case Resource::CRYSTAL:
        return { 0, 0, 0, 0, 1, 0, 0 };
    case Resource::GEMS:
        return { 0, 0, 0, 0, 0, 1, 0 };
    case Resource::GOLD:
        return { 0, 0, 0, 0, 0, 0, 1000 };
    default:
        break;
    }

    return {};
}
