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

payment_t ProfitConditions::FromBuilding( uint32_t building, int race )
{
    switch ( building ) {
    case BUILD_CASTLE:
        return payment_t( cost_t{ 1000, 0, 0, 0, 0, 0, 0 } );
    case BUILD_TENT:
    case BUILD_STATUE:
        return payment_t( cost_t{ 250, 0, 0, 0, 0, 0, 0 } );
    case BUILD_SPEC:
        if ( race == Race::WRLK )
            return payment_t( cost_t{ 500, 0, 0, 0, 0, 0, 0 } );
        break;
    default:
        break;
    }

    return {};
}

payment_t ProfitConditions::FromArtifact( int artifact )
{
    switch ( artifact ) {
    case Artifact::TAX_LIEN:
        return payment_t( cost_t{ 250, 0, 0, 0, 0, 0, 0 } );
    case Artifact::GOLDEN_GOOSE:
        return payment_t( cost_t{ 10000, 0, 0, 0, 0, 0, 0 } );
    case Artifact::ENDLESS_SACK_GOLD:
        return payment_t( cost_t{ 1000, 0, 0, 0, 0, 0, 0 } );
    case Artifact::ENDLESS_BAG_GOLD:
        return payment_t( cost_t{ 750, 0, 0, 0, 0, 0, 0 } );
    case Artifact::ENDLESS_PURSE_GOLD:
        return payment_t( cost_t{ 500, 0, 0, 0, 0, 0, 0 } );
    case Artifact::ENDLESS_POUCH_SULFUR:
        return payment_t( cost_t{ 0, 0, 0, 0, 1, 0, 0 } );
    case Artifact::ENDLESS_VIAL_MERCURY:
        return payment_t( cost_t{ 0, 0, 1, 0, 0, 0, 0 } );
    case Artifact::ENDLESS_POUCH_GEMS:
        return payment_t( cost_t{ 0, 0, 0, 0, 0, 0, 1 } );
    case Artifact::ENDLESS_CORD_WOOD:
        return payment_t( cost_t{ 0, 1, 0, 0, 0, 0, 0 } );
    case Artifact::ENDLESS_CART_ORE:
        return payment_t( cost_t{ 0, 0, 0, 1, 0, 0, 0 } );
    case Artifact::ENDLESS_POUCH_CRYSTAL:
        return payment_t( cost_t{ 0, 0, 0, 0, 0, 1, 0 } );
    default:
        break;
    }

    return {};
}

payment_t ProfitConditions::FromMine( int type )
{
    switch ( type ) {
    case Resource::ORE:
        return payment_t( cost_t{ 0, 0, 0, 2, 0, 0, 0 } );
    case Resource::WOOD:
        return payment_t( cost_t{ 0, 2, 0, 0, 0, 0, 0 } );
    case Resource::MERCURY:
        return payment_t( cost_t{ 0, 0, 1, 0, 0, 0, 0 } );
    case Resource::SULFUR:
        return payment_t( cost_t{ 0, 0, 0, 0, 1, 0, 0 } );
    case Resource::CRYSTAL:
        return payment_t( cost_t{ 0, 0, 0, 0, 0, 1, 0 } );
    case Resource::GEMS:
        return payment_t( cost_t{ 0, 0, 0, 0, 0, 0, 1 } );
    case Resource::GOLD:
        return payment_t( cost_t{ 1000, 0, 0, 0, 0, 0, 0 } );
    default:
        break;
    }

    return {};
}
