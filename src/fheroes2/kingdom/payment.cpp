/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "payment.h"
#include "buildinginfo.h"

payment_t PaymentConditions::BuyBuilding( int race, uint32_t build )
{
    return BuildingInfo::GetCost( build, race );
}

payment_t PaymentConditions::BuyBoat()
{
    return payment_t( cost_t{ 1000, 10, 0, 0, 0, 0, 0 } );
}

payment_t PaymentConditions::BuySpellBook( int shrine )
{
    switch ( shrine ) {
    case 1:
        return payment_t( cost_t{ 1250, 0, 0, 0, 0, 0, 0 } );
    case 2:
        return payment_t( cost_t{ 1000, 0, 0, 0, 0, 0, 0 } );
    case 3:
        return payment_t( cost_t{ 750, 0, 0, 0, 0, 0, 0 } );
    default:
        break;
    }

    return payment_t( cost_t{ 500, 0, 0, 0, 0, 0, 0 } );
}

payment_t PaymentConditions::RecruitHero()
{
    return payment_t( cost_t{ 2500, 0, 0, 0, 0, 0, 0 } );
}

payment_t PaymentConditions::ForAlchemist()
{
    return payment_t( cost_t{ 750, 0, 0, 0, 0, 0, 0 } );
}
