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

#include <cstring>
#include "castle.h"
#include "race.h"
#include "buildinginfo.h"
#include "settings.h"
#include "payment.h"

struct paymentstats_t
{
    const char* id;
    cost_t cost;
};

paymentstats_t _payments[] = {
    { "buy_boat",       { 1000,10, 0, 0, 0, 0, 0 } },
    { "buy_spell_book", {  500, 0, 0, 0, 0, 0, 0 } },
    { "buy_spell_book_from_shrine1", {  1250, 0, 0, 0, 0, 0, 0 } },
    { "buy_spell_book_from_shrine2", {  1000, 0, 0, 0, 0, 0, 0 } },
    { "buy_spell_book_from_shrine3", {   750, 0, 0, 0, 0, 0, 0 } },
    { "recruit_hero",   { 2500, 0, 0, 0, 0, 0, 0 } },
    { "recruit_level",   { 500, 0, 0, 0, 0, 0, 0 } },
    { "alchemist_payment", { 750, 0, 0, 0, 0, 0, 0 } },

    { NULL, { 0, 0, 0, 0, 0, 0, 0 } },
};

#ifdef WITH_XML
void LoadCostFromXMLElement(cost_t & cost, const TiXmlElement & element)
{
    int value;

    element.Attribute("gold", &value); cost.gold = value;
    element.Attribute("wood", &value); cost.wood = value;
    element.Attribute("mercury", &value); cost.mercury = value;
    element.Attribute("ore", &value); cost.ore = value;
    element.Attribute("sulfur", &value); cost.sulfur = value;
    element.Attribute("crystal", &value); cost.crystal = value;
    element.Attribute("gems", &value); cost.gems = value;
}
#endif

void PaymentConditions::UpdateCosts(const std::string & spec)
{
#ifdef WITH_XML
    // parse payments.xml
    TiXmlDocument doc;
    const TiXmlElement* xml_payments = NULL;

    if(doc.LoadFile(spec.c_str()) &&
        NULL != (xml_payments = doc.FirstChildElement("payments")))
    {
	paymentstats_t* ptr = &_payments[0];

        while(ptr->id)
        {
            const TiXmlElement* xml_payment = xml_payments->FirstChildElement(ptr->id);

            if(xml_payment)
        	LoadCostFromXMLElement(ptr->cost, *xml_payment);

            ++ptr;
        }
    }
    else
    VERBOSE(spec << ": " << doc.ErrorDesc());
#endif
}

payment_t PaymentConditions::BuyBuilding(int race, u32 build)
{
    return BuildingInfo::GetCost(build, race);
}

payment_t PaymentConditions::BuyBoat(void)
{
    payment_t result;
    paymentstats_t* ptr = &_payments[0];

    while(ptr->id && std::strcmp("buy_boat", ptr->id)) ++ptr;
    if(ptr->id) result = ptr->cost;

    return result;
}

payment_t PaymentConditions::BuySpellBook(int shrine)
{
    payment_t result;
    paymentstats_t* ptr = &_payments[0];
    const char* skey = NULL;

    switch(shrine)
    {
	case 1:  skey = "buy_spell_book_from_shrine1"; break;
	case 2:  skey = "buy_spell_book_from_shrine2"; break;
	case 3:  skey = "buy_spell_book_from_shrine3"; break;
	default: skey = "buy_spell_book"; break;
    }

    while(ptr->id && std::strcmp(skey, ptr->id)) ++ptr;
    if(ptr->id) result = ptr->cost;

    return result;
}

payment_t PaymentConditions::RecruitHero(int level)
{
    payment_t result;
    paymentstats_t* ptr = &_payments[0];
    while(ptr->id && std::strcmp("recruit_hero", ptr->id)) ++ptr;
    if(ptr->id) result = ptr->cost;

    // level price
    if(Settings::Get().ExtHeroRecruitCostDependedFromLevel())
    {
	ptr = &_payments[0];
	while(ptr->id && std::strcmp("recruit_level", ptr->id)) ++ptr;
	if(ptr && 1 < level)
	{
	    if(ptr->cost.gold) result.gold += (level - 1) * ptr->cost.gold;
	    if(ptr->cost.wood) result.wood += (level - 1) * ptr->cost.wood;
	    if(ptr->cost.mercury) result.mercury += (level - 1) * ptr->cost.mercury;
	    if(ptr->cost.ore) result.ore += (level - 1) * ptr->cost.ore;
	    if(ptr->cost.sulfur) result.sulfur += (level - 1) * ptr->cost.sulfur;
	    if(ptr->cost.crystal) result.crystal += (level - 1) * ptr->cost.crystal;
	    if(ptr->cost.gems) result.gems += (level - 1) * ptr->cost.gems;
	}
    }

    return result;
}

payment_t PaymentConditions::ForAlchemist(int arts)
{
    payment_t result;
    paymentstats_t* ptr = &_payments[0];
    while(ptr->id && std::strcmp("alchemist_payment", ptr->id)) ++ptr;
    if(ptr->id) result = ptr->cost;

    return result;
}
