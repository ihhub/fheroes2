/***************************************************************************
 *   Copyright (C) 2013 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <QRegExp>
#include <QDomElement>

#include "engine.h"
#include "global.h"

namespace Default
{
    int		_resourceGoldMin	= 500;
    int		_resourceGoldMax	= 1000;
    int		_resourceWoodOreMin	= 5;
    int		_resourceWoodOreMax	= 10;
    int		_resourceOtherMin	= 3;
    int		_resourceOtherMax	= 6;

    int & resourceGoldMin(void) { return _resourceGoldMin; }
    int & resourceGoldMax(void) { return _resourceGoldMax; }
    int & resourceWoodOreMin(void) { return _resourceWoodOreMin; }
    int & resourceWoodOreMax(void) { return _resourceWoodOreMax; }
    int & resourceOtherMin(void) { return _resourceOtherMin; }
    int & resourceOtherMax(void) { return _resourceOtherMax; }

    MonsterStat	_monsters[] = {
	{     0,   0,   0,   0,   0,  Speed::Standing,   0,     0, { 0, 0, 0, 0, 0, 0, 0} /* wood, mercury, ore, sulfur, crystal, gems, gold */},

	{     1,   1,   1,   1,   1,  Speed::VerySlow,  12,     0, { 0, 0, 0, 0, 0, 0, 20 } },
	{     5,   3,   2,   3,  10,  Speed::VerySlow,   8,    12, { 0, 0, 0, 0, 0, 0, 150 } },
	{     5,   3,   2,   3,  10,   Speed::Average,   8,    24, { 0, 0, 0, 0, 0, 0, 200 } },
	{     5,   9,   3,   4,  15,   Speed::Average,   5,     0, { 0, 0, 0, 0, 0, 0, 200 } },
	{     5,   9,   3,   4,  20,      Speed::Fast,   5,     0, { 0, 0, 0, 0, 0, 0, 250 } },
	{     7,   9,   4,   6,  25,   Speed::Average,   4,     0, { 0, 0, 0, 0, 0, 0, 250 } },
	{     7,   9,   4,   6,  30,      Speed::Fast,   4,     0, { 0, 0, 0, 0, 0, 0, 300 } },
	{    10,   9,   5,  10,  30,  Speed::VeryFast,   3,     0, { 0, 0, 0, 0, 0, 0, 300 } },
	{    10,   9,   5,  10,  40, Speed::UltraFast,   3,     0, { 0, 0, 0, 0, 0, 0, 375 } },
	{    11,  12,  10,  20,  50,      Speed::Fast,   2,     0, { 0, 0, 0, 0, 0, 0, 600 } },
	{    11,  12,  10,  20,  65,  Speed::VeryFast,   2,     0, { 0, 0, 0, 0, 0, 0, 1000 } },

	{     3,   1,   1,   2,   3,   Speed::Average,  10,     0, { 0, 0, 0, 0, 0, 0, 40 } },
	{     3,   4,   2,   3,  10,  Speed::VerySlow,   8,     8, { 0, 0, 0, 0, 0, 0, 140 } },
	{     3,   4,   3,   4,  15,      Speed::Slow,   8,    16, { 0, 0, 0, 0, 0, 0, 175 } },
	{     6,   2,   3,   5,  20,  Speed::VeryFast,   5,     0, { 0, 0, 0, 0, 0, 0, 200 } },
	{     9,   5,   4,   6,  40,  Speed::VerySlow,   4,     0, { 0, 0, 0, 0, 0, 0, 300 } },
	{     9,   5,   5,   7,  60,   Speed::Average,   4,     0, { 0, 0, 0, 0, 0, 0, 500 } },
	{    10,   5,   5,   7,  40,   Speed::Average,   3,     8, { 0, 0, 0, 0, 0, 0, 600 } },
	{    10,   5,   7,   9,  40,      Speed::Fast,   3,    16, { 0, 0, 0, 0, 0, 0, 700 } },
	{    12,   9,  12,  24,  80,      Speed::Fast,   2,     0, { 0, 0, 0, 0, 1, 0, 750 } },

	{     4,   2,   1,   2,   2,   Speed::Average,   8,     0, { 0, 0, 0, 0, 0, 0, 50 } },
	{     6,   5,   2,   4,  20,  Speed::VerySlow,   6,     0, { 0, 0, 0, 0, 0, 0, 200 } },
	{     6,   6,   2,   4,  20,   Speed::Average,   6,     0, { 0, 0, 0, 0, 0, 0, 250 } },
	{     4,   3,   2,   3,  15,   Speed::Average,   4,    24, { 0, 0, 0, 0, 0, 0, 250 } },
	{     5,   5,   2,   3,  15,  Speed::VeryFast,   4,    24, { 0, 0, 0, 0, 0, 0, 300 } },
	{     7,   5,   5,   8,  25,      Speed::Fast,   3,     8, { 0, 0, 0, 0, 0, 0, 350 } },
	{     7,   7,   5,   8,  25,  Speed::VeryFast,   3,    16, { 0, 0, 0, 0, 0, 0, 400 } },
	{    10,   9,   7,  14,  40,      Speed::Fast,   2,     0, { 0, 0, 0, 0, 0, 0, 500 } },
	{    12,  10,  20,  40, 100, Speed::UltraFast,   1,     0, { 0, 1, 0, 0, 0, 0, 1500 } },

	{     3,   1,   1,   2,   5,   Speed::Average,   8,     8, { 0, 0, 0, 0, 0, 0, 60 } },
	{     4,   7,   2,   3,  15,  Speed::VeryFast,   6,     0, { 0, 0, 0, 0, 0, 0, 200 } },
	{     6,   6,   3,   5,  25,   Speed::Average,   4,     0, { 0, 0, 0, 0, 0, 0, 300 } },
	{     9,   8,   5,  10,  35,   Speed::Average,   3,     0, { 0, 0, 0, 0, 0, 0, 400 } },
	{     9,   8,   5,  10,  45,  Speed::VeryFast,   3,     0, { 0, 0, 0, 0, 0, 0, 500 } },
	{     8,   9,   6,  12,  75,  Speed::VerySlow,   2,     0, { 0, 0, 0, 0, 0, 0, 800 } },
	{    12,  12,  25,  50, 200,   Speed::Average,   1,     0, { 0, 0, 0, 1, 0, 0, 3000 } },
	{    13,  13,  25,  50, 250,      Speed::Fast,   1,     0, { 0, 0, 0, 1, 0, 0, 3500 } }, 
	{    14,  14,  25,  50, 300,  Speed::VeryFast,   1,     0, { 0, 0, 0, 2, 0, 0, 4000 } },

	{     2,   1,   1,   3,   3,      Speed::Slow,   8,    12, { 0, 0, 0, 0, 0, 0, 50 } },
	{     5,   4,   2,   3,  15,  Speed::VeryFast,   6,     0, { 0, 0, 0, 0, 0, 0, 150 } },
	{     5,  10,   4,   5,  30,  Speed::VerySlow,   4,     0, { 0, 0, 0, 0, 0, 0, 300 } },
	{     7,  10,   4,   5,  35,      Speed::Slow,   4,     0, { 0, 0, 0, 0, 0, 0, 350 } },
	{     7,   7,   4,   8,  40,   Speed::Average,   3,     0, { 0, 0, 0, 0, 0, 0, 400 } },
	{    11,   7,   7,   9,  30,      Speed::Fast,   2,    12, { 0, 0, 0, 0, 0, 0, 600 } },
	{    12,   8,   7,   9,  35,  Speed::VeryFast,   2,    24, { 0, 0, 0, 0, 0, 0, 700 } },
	{    13,  10,  20,  30, 150,   Speed::Average,   1,     0, { 0, 0, 0, 0, 0, 1, 2000 } },
	{    15,  15,  20,  30, 300,  Speed::VeryFast,   1,    24, { 0, 0, 0, 0, 0, 2, 5000 } },

	{     4,   3,   2,   3,   4,   Speed::Average,   8,     0, { 0, 0, 0, 0, 0, 0, 75 } },
	{     5,   2,   2,   3,  15,  Speed::VerySlow,   6,     0, { 0, 0, 0, 0, 0, 0, 150 } },
	{     5,   2,   2,   3,  25,   Speed::Average,   6,     0, { 0, 0, 0, 0, 0, 0, 200 } },
	{     6,   6,   3,   4,  25,   Speed::Average,   4,     0, { 0, 0, 0, 0, 0, 0, 250 } },
	{     6,   6,   3,   4,  30,      Speed::Fast,   4,     0, { 0, 0, 0, 0, 0, 0, 300 } },
	{     8,   6,   5,   7,  30,   Speed::Average,   3,     0, { 0, 0, 0, 0, 0, 0, 500 } },
	{     8,   6,   5,   7,  40,      Speed::Fast,   3,     0, { 0, 0, 0, 0, 0, 0, 650 } },
	{     7,  12,   8,  10,  25,      Speed::Fast,   2,    12, { 0, 0, 0, 0, 0, 0, 750 } },
	{     7,  13,   8,  10,  35,  Speed::VeryFast,   2,    24, { 0, 0, 0, 0, 0, 0, 900 } }, 
	{    11,   9,  25,  45, 150,   Speed::Average,   1,     0, { 0, 0, 0, 0, 0, 0, 1500 } },

	{     6,   1,   1,   2,   4,      Speed::Fast,   4,     0, { 0, 0, 0, 0, 0, 0, 50 } },
	{     7,   6,   2,   5,  20,  Speed::VeryFast,   4,     0, { 0, 0, 0, 0, 0, 0, 200 } },
	{     8,   7,   4,   6,  20,      Speed::Fast,   4,     0, { 0, 0, 0, 0, 0, 0, 1000 } },
	{    10,   9,  20,  30,  50,  Speed::VeryFast,   4,     0, { 0, 0, 0, 0, 0, 1, 650 } },
	{     8,   9,   6,  10,  35,   Speed::Average,   4,     0, { 0, 0, 0, 0, 0, 0, 500 } },
	{     8,   8,   4,   5,  50,      Speed::Slow,   4,     0, { 0, 0, 0, 0, 0, 0, 500 } },
	{     7,   7,   2,   8,  35,  Speed::VeryFast,   4,     0, { 0, 0, 0, 0, 0, 0, 500 } },
	{     8,   6,   4,   6,  40,      Speed::Fast,   4,     0, { 0, 0, 0, 0, 0, 0, 500 } },
	{     6,   8,   3,   7,  45,   Speed::Average,   4,     0, { 0, 0, 0, 0, 0, 0, 500 } },

	{     0,   0,   0,   0,   0,  Speed::Standing,   0,     0, { 0, 0, 0, 0, 0, 0, 0} },
	{     0,   0,   0,   0,   0,  Speed::Standing,   0,     0, { 0, 0, 0, 0, 0, 0, 0} },
	{     0,   0,   0,   0,   0,  Speed::Standing,   0,     0, { 0, 0, 0, 0, 0, 0, 0} },
	{     0,   0,   0,   0,   0,  Speed::Standing,   0,     0, { 0, 0, 0, 0, 0, 0, 0} },
	{     0,   0,   0,   0,   0,  Speed::Standing,   0,     0, { 0, 0, 0, 0, 0, 0, 0} },

	{     0,   0,   0,   0,   0,  Speed::Standing,   0,     0, { 0, 0, 0, 0, 0, 0, 0} } };

    MonsterStat & monsterStat(int type)
    {
	const int count = sizeof(_monsters) / sizeof(_monsters[0]);
	return count > type ? _monsters[type] : _monsters[Monster::Unknown];
    }
}

QDomElement & operator<< (QDomElement & el, const DefaultValues & dv)
{
    Q_UNUSED(dv);

    QDomElement resourcesElem = el.ownerDocument().createElement("resources");
    el.appendChild(resourcesElem);

    resourcesElem.setAttribute("goldMin", Default::_resourceGoldMin);
    resourcesElem.setAttribute("goldMax", Default::_resourceGoldMax);
    resourcesElem.setAttribute("woodOreMin", Default::_resourceWoodOreMin);
    resourcesElem.setAttribute("woodOreMax", Default::_resourceWoodOreMax);
    resourcesElem.setAttribute("otherMin", Default::_resourceOtherMin);
    resourcesElem.setAttribute("otherMax", Default::_resourceOtherMax);

    QDomElement monstersElem = el.ownerDocument().createElement("monsters");
    el.appendChild(monstersElem);

    for(int type = Monster::None; type < Monster::Unknown; ++type)
    {
	MonsterStat & stat = Default::_monsters[type];
	QString tagName = Monster::transcribe(type).toLower().replace(QRegExp(" "), "_");

	QDomElement monsterElem = el.ownerDocument().createElement(tagName);
	monstersElem.appendChild(monsterElem);

	monsterElem.setAttribute("attack", stat.attack);
	monsterElem.setAttribute("defense", stat.defense);
	monsterElem.setAttribute("damageMin", stat.damageMin);
	monsterElem.setAttribute("damageMax", stat.damageMax);
	monsterElem.setAttribute("hp", stat.hp);
	monsterElem.setAttribute("speed", stat.speed);
	monsterElem.setAttribute("grown", stat.grown);
	monsterElem.setAttribute("shots", stat.shots);

	if(stat.cost.ore) monsterElem.setAttribute("costOre", stat.cost.ore);
	if(stat.cost.wood) monsterElem.setAttribute("costWood", stat.cost.wood);
	if(stat.cost.mercury) monsterElem.setAttribute("costMercury", stat.cost.mercury);
	if(stat.cost.sulfur) monsterElem.setAttribute("costSulfur", stat.cost.sulfur);
	if(stat.cost.crystal) monsterElem.setAttribute("costCrystal", stat.cost.crystal);
	if(stat.cost.gems) monsterElem.setAttribute("costGems", stat.cost.gems);
	if(stat.cost.gold) monsterElem.setAttribute("costGold", stat.cost.gold);
    }

    return el;
}

QDomElement & operator>> (QDomElement & el, DefaultValues & dv)
{
    Q_UNUSED(dv);

    QDomElement resourcesElem = el.firstChildElement("resources").toElement();

    if(! resourcesElem.isNull())
    {
	if(resourcesElem.hasAttribute("goldMin")) Default::_resourceGoldMin = resourcesElem.attribute("goldMin").toInt();
	if(resourcesElem.hasAttribute("goldMax")) Default::_resourceGoldMax = resourcesElem.attribute("goldMax").toInt();
	if(resourcesElem.hasAttribute("woodOreMin")) Default::_resourceWoodOreMin = resourcesElem.attribute("woodOreMin").toInt();
	if(resourcesElem.hasAttribute("woodOreMax")) Default::_resourceWoodOreMax = resourcesElem.attribute("woodOreMax").toInt();
	if(resourcesElem.hasAttribute("otherMin")) Default::_resourceOtherMin = resourcesElem.attribute("otherMin").toInt();
    }

    QDomElement monstersElem = el.firstChildElement("monsters").toElement();

    if(! monstersElem.isNull())
    {
	for(int type = Monster::None; type < Monster::Unknown; ++type)
	{
	    MonsterStat & stat = Default::_monsters[type];
	    QString tagName = Monster::transcribe(type).toLower().replace(QRegExp(" "), "_");

	    QDomElement monsterElem = el.firstChildElement(tagName).toElement();

	    if(! monsterElem.isNull())
	    {
		if(monsterElem.hasAttribute("attack")) stat.attack = monsterElem.attribute("attack").toInt();
		if(monsterElem.hasAttribute("defense")) stat.defense = monsterElem.attribute("defense").toInt();
		if(monsterElem.hasAttribute("damageMin")) stat.damageMin = monsterElem.attribute("damageMin").toInt();
		if(monsterElem.hasAttribute("damageMax")) stat.damageMax = monsterElem.attribute("damageMax").toInt();
		if(monsterElem.hasAttribute("hp")) stat.hp = monsterElem.attribute("hp").toInt();
		if(monsterElem.hasAttribute("speed")) stat.speed = monsterElem.attribute("speed").toInt();
		if(monsterElem.hasAttribute("grown")) stat.grown = monsterElem.attribute("grown").toInt();
		if(monsterElem.hasAttribute("shots")) stat.shots = monsterElem.attribute("shots").toInt();

		if(monsterElem.hasAttribute("costOre")) stat.cost.ore = monsterElem.attribute("costOre").toInt();
		if(monsterElem.hasAttribute("costWood")) stat.cost.wood = monsterElem.attribute("costWood").toInt();
		if(monsterElem.hasAttribute("costMercury")) stat.cost.mercury = monsterElem.attribute("costMercury").toInt();
		if(monsterElem.hasAttribute("costSulfur")) stat.cost.sulfur = monsterElem.attribute("costSulfur").toInt();
		if(monsterElem.hasAttribute("costCrystal")) stat.cost.crystal = monsterElem.attribute("costCrystal").toInt();
		if(monsterElem.hasAttribute("costGems")) stat.cost.gems = monsterElem.attribute("costGems").toInt();
		if(monsterElem.hasAttribute("costGold")) stat.cost.gold = monsterElem.attribute("costGold").toInt();
	    }
	}
    }

    return el;
}
