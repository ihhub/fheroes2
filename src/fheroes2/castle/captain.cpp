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

#include "castle.h"
#include "luck.h"
#include "morale.h"
#include "race.h"
#include "agg.h"
#include "settings.h"
#include "captain.h"

Captain::Captain(Castle & cstl) : HeroBase(HeroBase::CAPTAIN, cstl.GetRace()), home(cstl)
{
    SetCenter(home.GetCenter());
}

bool Captain::isValid(void) const
{
    return home.isBuild(BUILD_CAPTAIN);
}

int Captain::GetAttack(void) const
{
    return attack + GetAttackModificator(NULL);
}

int Captain::GetDefense(void) const
{
    return defense + GetDefenseModificator(NULL);
}

int Captain::GetPower(void) const
{
   return power + GetPowerModificator(NULL);
}

int Captain::GetKnowledge(void) const
{
    return knowledge + GetKnowledgeModificator(NULL);
}

int Captain::GetMorale(void) const
{
    int result = Morale::NORMAL;

    // global modificator
    result += GetMoraleModificator(NULL);

    // result
    if(result < Morale::AWFUL)  return Morale::TREASON;
    else
    if(result < Morale::POOR)   return Morale::AWFUL;
    else
    if(result < Morale::NORMAL) return Morale::POOR;
    else
    if(result < Morale::GOOD)   return Morale::NORMAL;
    else
    if(result < Morale::GREAT)  return Morale::GOOD;
    else
    if(result < Morale::BLOOD)  return Morale::GREAT;

    return Morale::BLOOD;
}

int Captain::GetLuck(void) const
{
    int result = Luck::NORMAL;

    // global modificator
    result += GetLuckModificator(NULL);

    // result
    if(result < Luck::AWFUL)    return Luck::CURSED;
    else
    if(result < Luck::BAD)      return Luck::AWFUL;
    else
    if(result < Luck::NORMAL)   return Luck::BAD;
    else
    if(result < Luck::GOOD)     return Luck::NORMAL;
    else
    if(result < Luck::GREAT)    return Luck::GOOD;
    else
    if(result < Luck::IRISH)    return Luck::GREAT;

    return Luck::IRISH;
}

int Captain::GetRace(void) const
{
    return home.GetRace();
}

int Captain::GetColor(void) const
{
    return home.GetColor();
}

const std::string & Captain::GetName(void) const
{
    return home.GetName();
}

int Captain::GetType(void) const
{
    return HeroBase::CAPTAIN;
}

int Captain::GetLevelSkill(int) const
{
    return 0;
}

u32 Captain::GetSecondaryValues(int) const
{
    return 0;
}

const Army & Captain::GetArmy(void) const
{
    return home.GetArmy();
}

Army & Captain::GetArmy(void)
{
    return home.GetArmy();
}

u32 Captain::GetMaxSpellPoints(void) const
{
    return knowledge * 10;
}

int Captain::GetControl(void) const
{
    return home.GetControl();
}

s32 Captain::GetIndex(void) const
{
    return home.GetIndex();
}

void Captain::ActionAfterBattle(void)
{
    SetSpellPoints(GetMaxSpellPoints());
}

void Captain::ActionPreBattle(void)
{
    SetSpellPoints(GetMaxSpellPoints());
}

const Castle* Captain::inCastle(void) const
{
    return &home;
}

Surface Captain::GetPortrait(int type) const
{
    switch(type)
    {
        case PORT_BIG:
            switch(GetRace())
            {
                case Race::KNGT:        return AGG::GetICN(ICN::PORT0090, 0);
                case Race::BARB:        return AGG::GetICN(ICN::PORT0091, 0);
                case Race::SORC:        return AGG::GetICN(ICN::PORT0092, 0);
                case Race::WRLK:        return AGG::GetICN(ICN::PORT0093, 0);
                case Race::WZRD:        return AGG::GetICN(ICN::PORT0094, 0);
                case Race::NECR:        return AGG::GetICN(ICN::PORT0095, 0);
                default: break;
            }
            break;

        case PORT_MEDIUM:
        case PORT_SMALL:
            switch(GetRace())
            {
                case Race::KNGT:        return AGG::GetICN(ICN::MINICAPT, 0);
                case Race::BARB:        return AGG::GetICN(ICN::MINICAPT, 1);
                case Race::SORC:        return AGG::GetICN(ICN::MINICAPT, 2);
                case Race::WRLK:        return AGG::GetICN(ICN::MINICAPT, 3);
                case Race::WZRD:        return AGG::GetICN(ICN::MINICAPT, 4);
                case Race::NECR:        return AGG::GetICN(ICN::MINICAPT, 5);
                default: break;
            }
            break;
    }

    return Surface();
}

void Captain::PortraitRedraw(s32 px, s32 py, int type, Surface & dstsf) const
{
    GetPortrait(type).Blit(px, py, dstsf);
}
