/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "buildinginfo.h"
#include "payment.h"
#include "settings.h"

payment_t PaymentConditions::BuyBuilding( const int race, const uint32_t build )
{
    return BuildingInfo::GetCost( build, race );
}

payment_t PaymentConditions::BuyBoat()
{
    return { 0, 10, 0, 0, 0, 0, 1000 };
}

payment_t PaymentConditions::BuySpellBook( const int shrine )
{
    switch ( shrine ) {
    case 1:
        return { 0, 0, 0, 0, 0, 0, 1250 };
    case 2:
        return { 0, 0, 0, 0, 0, 0, 1000 };
    case 3:
        return { 0, 0, 0, 0, 0, 0, 750 };
    default:
        break;
    }

    return { 0, 0, 0, 0, 0, 0, 500 };
}

payment_t PaymentConditions::RecruitHero( const int level )
{
    int gold = 2500;

    // level price
    if ( Settings::Get().ExtHeroRecruitCostDependedFromLevel() && level > 1 ) {
        gold += ( level - 1 ) * 500;
    }
    return { 0, 0, 0, 0, 0, 0, gold };
}

payment_t PaymentConditions::ForAlchemist()
{
    return { 0, 0, 0, 0, 0, 0, 750 };
}
