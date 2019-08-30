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

#include <cstring>
#include "castle.h"
#include "artifact.h"
#include "race.h"
#include "skill.h"
#include "settings.h"
#include "profit.h"

struct profitstats_t
{
    const char* id;
    cost_t cost;
};

profitstats_t _profits[] = {
    { "castle",  {1000, 0, 0, 0, 0, 0, 0 } },
    { "town",    { 250, 0, 0, 0, 0, 0, 0 } },
    { "statue",  { 250, 0, 0, 0, 0, 0, 0 } },
    { "dungeon", { 500, 0, 0, 0, 0, 0, 0 } },

    { "sawmill",      { 0, 2, 0, 0, 0, 0, 0 } },
    { "alchemylab",   { 0, 0, 1, 0, 0, 0, 0 } },
    { "mine_ore",     { 0, 0, 0, 2, 0, 0, 0 } },
    { "mine_sulfur",  { 0, 0, 0, 0, 1, 0, 0 } },
    { "mine_crystal", { 0, 0, 0, 0, 0, 1, 0 } },
    { "mine_gems",    { 0, 0, 0, 0, 0, 0, 1 } },
    { "mine_gold",    {1000, 0, 0, 0, 0, 0, 0 } },

    { "ultimate_golden_goose", {10000, 0, 0, 0, 0, 0, 0 } },
    { "tax_lien",              { 250, 0, 0, 0, 0, 0, 0 } },
    { "endless_sack_gold",     {1000, 0, 0, 0, 0, 0, 0 } },
    { "endless_bag_gold",      { 750, 0, 0, 0, 0, 0, 0 } },
    { "endless_purse_gold",    { 500, 0, 0, 0, 0, 0, 0 } },
    { "endless_cord_wood",     { 0, 1, 0, 0, 0, 0, 0 } },
    { "endless_vial_mercury",  { 0, 0, 1, 0, 0, 0, 0 } },
    { "endless_cart_ore",      { 0, 0, 0, 1, 0, 0, 0 } },
    { "endless_pouch_sulfur",  { 0, 0, 0, 0, 1, 0, 0 } },
    { "endless_pouch_crystal", { 0, 0, 0, 0, 0, 1, 0 } },
    { "endless_pouch_gems",    { 0, 0, 0, 0, 0, 0, 1 } },

    { NULL, { 0, 0, 0, 0, 0, 0, 0 } },
};

void ProfitConditions::UpdateCosts(const std::string & spec)
{
#ifdef WITH_XML
    // parse profits.xml
    TiXmlDocument doc;
    const TiXmlElement* xml_profits = NULL;

    if(doc.LoadFile(spec.c_str()) &&
        NULL != (xml_profits = doc.FirstChildElement("profits")))
    {
	profitstats_t* ptr = &_profits[0];

        while(ptr->id)
        {
            const TiXmlElement* xml_profit = xml_profits->FirstChildElement(ptr->id);

            if(xml_profit)
        	LoadCostFromXMLElement(ptr->cost, *xml_profit);

            ++ptr;
        }
    }
    else
    VERBOSE(spec << ": " << doc.ErrorDesc());
#endif
}

payment_t ProfitConditions::FromBuilding(u32 building, int race)
{
    payment_t result;
    const char* id = NULL;

    switch(building)
    {
	case BUILD_CASTLE: id = "castle"; break;
	case BUILD_TENT:  id = "town"; break;
	case BUILD_STATUE: id = "statue"; break;
	case BUILD_SPEC: if(race == Race::WRLK) id = "dungeon"; break;
	default: break;
    }

    if(id)
    {
	profitstats_t* ptr = &_profits[0];
	while(ptr->id && std::strcmp(id, ptr->id)) ++ptr;
	if(ptr->id) result = ptr->cost;
    }

    return result;
}

payment_t ProfitConditions::FromArtifact(int artifact)
{
    payment_t result;
    const char* id = NULL;

    switch(artifact)
    {
	case Artifact::TAX_LIEN: id = "tax_lien"; break;
	case Artifact::GOLDEN_GOOSE: id = "ultimate_golden_goose"; break;
	case Artifact::ENDLESS_SACK_GOLD: id = "endless_sack_gold"; break;
	case Artifact::ENDLESS_BAG_GOLD: id = "endless_bag_gold"; break;
	case Artifact::ENDLESS_PURSE_GOLD: id = "endless_purse_gold"; break;
	case Artifact::ENDLESS_POUCH_SULFUR: id = "endless_pouch_sulfur"; break;
	case Artifact::ENDLESS_VIAL_MERCURY: id = "endless_vial_mercury"; break;
	case Artifact::ENDLESS_POUCH_GEMS: id = "endless_pouch_gems"; break;
	case Artifact::ENDLESS_CORD_WOOD: id = "endless_cord_wood"; break;
	case Artifact::ENDLESS_CART_ORE: id = "endless_cart_ore"; break;
	case Artifact::ENDLESS_POUCH_CRYSTAL: id = "endless_pouch_crystal"; break;
	default: break;
    }

    if(id)
    {
	profitstats_t* ptr = &_profits[0];
	while(ptr->id && std::strcmp(id, ptr->id)) ++ptr;
	if(ptr->id) result = ptr->cost;
    }

    return result;
}

payment_t ProfitConditions::FromMine(int type)
{
    payment_t result;
    const char* id = NULL;

    switch(type)
    {
	case Resource::ORE: id = "mine_ore"; break;
	case Resource::WOOD: id = "sawmill"; break;
	case Resource::MERCURY: id = "alchemylab"; break;
	case Resource::SULFUR: id = "mine_sulfur"; break;
	case Resource::CRYSTAL: id = "mine_crystal"; break;
	case Resource::GEMS: id = "mine_gems"; break;
	case Resource::GOLD: id = "mine_gold"; break;
	default: break;
    }

    if(id)
    {
	profitstats_t* ptr = &_profits[0];
	while(ptr->id && std::strcmp(id, ptr->id)) ++ptr;
	if(ptr->id) result = ptr->cost;
    }

    return result;
}
