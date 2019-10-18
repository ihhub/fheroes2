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

#ifndef H2PAYMENT_H
#define H2PAYMENT_H

#include <string>
#include "resource.h"

typedef Funds payment_t;

enum { INCOME_CAPTURED = 0x01, INCOME_CASTLES = 0x02, INCOME_ARTIFACTS = 0x04, INCOME_HEROSKILLS = 0x08, INCOME_ALL = 0xFF };

namespace PaymentConditions
{
    payment_t BuyBuilding(int race, u32 build);
    payment_t BuyBoat(void);
    payment_t BuySpellBook(int shrine = 0);
    payment_t RecruitHero(int level);
    payment_t ForAlchemist(int arts);

    void UpdateCosts(const std::string &);
}

#endif
