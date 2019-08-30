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
#include "settings.h"
#include "battle_cell.h"
#include "battle_troop.h"
#include "battle_tower.h"

Battle::Tower::Tower(const Castle & castle, int twr) : Unit(Troop(Monster::ARCHER, 0), -1, false),
    type(twr), color(castle.GetColor()), bonus(0), valid(true)
{
    count += castle.CountBuildings();
    count += castle.GetLevelMageGuild() - 1;

    if(count > 20) count = 20;
    if(TWR_CENTER != type) count /= 2;
    if(count == 0) count = 1;
    bonus = castle.GetLevelMageGuild();

    SetModes(CAP_TOWER);
}

const char* Battle::Tower::GetName(void) const
{
    switch(type)
    {
	case TWR_LEFT:	return _("Left Turret");
	case TWR_RIGHT:	return _("Right Turret");

	default: break;
    }

    return _("Ballista");
}

bool Battle::Tower::isValid(void) const
{
    return valid;
}

u32 Battle::Tower::GetType(void) const
{
    return type;
}

u32 Battle::Tower::GetBonus(void) const
{
    return bonus;
}

u32 Battle::Tower::GetAttack(void) const
{
    return Unit::GetAttack() + bonus;
}

int Battle::Tower::GetColor(void) const
{
    return color;
}

Point Battle::Tower::GetPortPosition(void) const
{
    Point res;

    switch(type)
    {
	case TWR_LEFT:	res = Point(410, 70); break;
	case TWR_RIGHT:	res = Point(410, 320); break;
	case TWR_CENTER:res = Point(560, 170); break;
	default: break;
    }

    if(Settings::Get().QVGA())
    {
	res.x /= 2;
	res.y /= 2;
    }

    return res;
}

void Battle::Tower::SetDestroy(void)
{
    switch(type)
    {
	case TWR_LEFT:	Board::GetCell(19)->SetObject(1); break;
	case TWR_RIGHT:	Board::GetCell(85)->SetObject(1); break;
	default: break;
    }
    valid = false;
}

std::string Battle::Tower::GetInfo(const Castle & cstl)
{
    const char* tmpl = _("The %{name} fires with the strength of %{count} Archers");
    const char* addn = _("each with a +%{attack} bonus to their attack skill.");

    std::vector<int> towers;
    std::string msg;

    if(cstl.isBuild(BUILD_CASTLE))
    {
	towers.push_back(TWR_CENTER);

	if(cstl.isBuild(BUILD_LEFTTURRET)) towers.push_back(TWR_LEFT);
	if(cstl.isBuild(BUILD_RIGHTTURRET)) towers.push_back(TWR_RIGHT);

	for(std::vector<int>::const_iterator
	    it = towers.begin(); it != towers.end(); ++it)
	{
    	    Tower twr = Tower(cstl, *it);

    	    msg.append(tmpl);
    	    StringReplace(msg, "%{name}", twr.GetName());
    	    StringReplace(msg, "%{count}", twr.GetCount());

	    if(twr.GetBonus())
	    {
		msg.append(", ");
		msg.append(addn);
    		StringReplace(msg, "%{attack}", twr.GetBonus());
	    }
	    else
		msg.append(".");

	    if((it + 1) != towers.end())
    		msg.append("\n \n");
	}
    }

    return msg;
}
