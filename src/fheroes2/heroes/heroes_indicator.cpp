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

#include <cmath>
#include "agg.h"
#include "luck.h"
#include "dialog.h"
#include "text.h"
#include "morale.h"
#include "heroes.h"
#include "heroes_indicator.h"

const char* MoraleString(int morale)
{
    switch(morale)
    {
        case Morale::TREASON:
        case Morale::AWFUL:
        case Morale::POOR:
            return _("Bad Morale");

        case Morale::NORMAL:
            return _("Neutral Morale");

        case Morale::GOOD:
        case Morale::GREAT:
        case Morale::BLOOD:
    	    return _("Good Morale");

        default: break;
    }
    return NULL;
}

const char* LuckString(int luck)
{
    switch(luck)
    {
        case Luck::CURSED:
        case Luck::AWFUL:
        case Luck::BAD:
            return _("Bad Luck");

        case Luck::NORMAL:
            return _("Neutral Luck");

        case Luck::GOOD:
        case Luck::GREAT:
        case Luck::IRISH:
            return _("Good Luck");

        default: break;
    }
    return NULL;
}

HeroesIndicator::HeroesIndicator(const Heroes & h) : hero(h)
{
    descriptions.reserve(256);
}

const Rect & HeroesIndicator::GetArea(void) const
{
    return area;
}

const std::string & HeroesIndicator::GetDescriptions(void) const
{
    return descriptions;
}

void HeroesIndicator::SetPos(const Point & pt, bool skip_back)
{
    area.x = pt.x;
    area.y = pt.y;
    if(! skip_back) back.Save(area);
}

LuckIndicator::LuckIndicator(const Heroes & h) : HeroesIndicator(h), luck(Luck::NORMAL)
{
    area.w = 35;
    area.h = 26;
}

void LuckIndicator::Redraw(void)
{
    std::string modificators;
    modificators.reserve(256);
    luck = hero.GetLuckWithModificators(&modificators);

    descriptions.clear();
    descriptions.append(Luck::Description(luck));
    descriptions.append("\n \n");
    descriptions.append(_("Current Modifiers:"));
    descriptions.append("\n \n");

    const Sprite & sprite = AGG::GetICN(ICN::HSICONS, (0 > luck ? 3 : (0 < luck ? 2 : 6)));
    const int inter = 6;
    int count = (0 == luck ? 1 : std::abs(luck));
    s32 cx = area.x + (area.w - (sprite.w() + inter * (count - 1))) / 2;
    s32 cy = area.y + (area.h - sprite.h()) / 2;

    if(modificators.size())
	descriptions.append(modificators);
    else
	descriptions.append(_("None"));

    back.Restore();
    while(count--)
    {
        sprite.Blit(cx, cy);
        cx += inter;
    }
}

void LuckIndicator::QueueEventProcessing(LuckIndicator & indicator)
{
    LocalEvent & le = LocalEvent::Get();

    if(le.MouseClickLeft(indicator.area)) Dialog::Message(LuckString(indicator.luck), indicator.descriptions, Font::BIG, Dialog::OK);
    else
    if(le.MousePressRight(indicator.area)) Dialog::Message(LuckString(indicator.luck), indicator.descriptions, Font::BIG);
}

MoraleIndicator::MoraleIndicator(const Heroes & h) : HeroesIndicator(h), morale(Morale::NORMAL)
{
    area.w = 35;
    area.h = 26;
}

void MoraleIndicator::Redraw(void)
{
    std::string modificators;
    modificators.reserve(256);
    morale = hero.GetMoraleWithModificators(&modificators);

    descriptions.clear();
    descriptions.append(Morale::Description(morale));
    descriptions.append("\n \n");
    descriptions.append(_("Current Modifiers:"));
    descriptions.append("\n \n");

    const Sprite & sprite = AGG::GetICN(ICN::HSICONS, (0 > morale ? 5 : (0 < morale ? 4 : 7)));
    const int inter = 6;
    int count = (0 == morale ? 1 : std::abs(morale));
    s32 cx = area.x + (area.w - (sprite.w() + inter * (count - 1))) / 2;
    s32 cy = area.y + (area.h - sprite.h()) / 2;

    if(modificators.size())
	descriptions.append(modificators);
    else
	descriptions.append(_("None"));

    back.Restore();
    while(count--)
    {
        sprite.Blit(cx, cy);
        cx += inter;
    }
}

void MoraleIndicator::QueueEventProcessing(MoraleIndicator & indicator)
{
    LocalEvent & le = LocalEvent::Get();

    if(le.MouseClickLeft(indicator.area)) Dialog::Message(MoraleString(indicator.morale), indicator.descriptions, Font::BIG, Dialog::OK);
    else
    if(le.MousePressRight(indicator.area)) Dialog::Message(MoraleString(indicator.morale), indicator.descriptions, Font::BIG);
}

ExperienceIndicator::ExperienceIndicator(const Heroes & h) : HeroesIndicator(h)
{
    area.w = 39;
    area.h = 36;

    descriptions = _("Current experience %{exp1} Next level %{exp2}.");
    StringReplace(descriptions, "%{exp1}", hero.GetExperience());
    StringReplace(descriptions, "%{exp2}", hero.GetExperienceFromLevel(hero.GetLevelFromExperience(hero.GetExperience())));
}

void ExperienceIndicator::Redraw(void)
{
    const Sprite & sprite3 = AGG::GetICN(ICN::HSICONS, 1);
    sprite3.Blit(area.x, area.y);

    Text text(GetString(hero.GetExperience()), Font::SMALL);
    text.Blit(area.x + 18 - text.w() / 2, area.y + 23);
}

void ExperienceIndicator::QueueEventProcessing(void)
{
    LocalEvent & le = LocalEvent::Get();

    if(le.MouseClickLeft(area) || le.MousePressRight(area))
    {
	std::string message = _("Level %{level}");
	StringReplace(message, "%{level}", hero.GetLevel());
	Dialog::Message(message, descriptions, Font::BIG, (le.MousePressRight() ? 0 : Dialog::OK));
    }
}

SpellPointsIndicator::SpellPointsIndicator(const Heroes & h) : HeroesIndicator(h)
{
    area.w = 39;
    area.h = 36;

    descriptions = _("%{name} currently has %{point} spell points out of a maximum of %{max}. The maximum number of spell points is 10 times your knowledge. It is occasionally possible to have more than your maximum spell points via special events.");
    StringReplace(descriptions, "%{name}", hero.GetName());
    StringReplace(descriptions, "%{point}", hero.GetSpellPoints());
    StringReplace(descriptions, "%{max}", hero.GetMaxSpellPoints());
}

void SpellPointsIndicator::Redraw(void)
{
    const Sprite & sprite3 = AGG::GetICN(ICN::HSICONS, 8);
    sprite3.Blit(area.x, area.y);

    Text text(GetString(hero.GetSpellPoints()) + "/" + GetString(hero.GetMaxSpellPoints()), Font::SMALL);
    text.Blit(area.x + 18 - text.w() / 2, area.y + 21);
}

void SpellPointsIndicator::QueueEventProcessing(void)
{
    LocalEvent & le = LocalEvent::Get();

    if(le.MouseClickLeft(area) || le.MousePressRight(area))
    {
	Dialog::Message(_("Spell Points"), descriptions, Font::BIG, (le.MousePressRight() ? 0 : Dialog::OK));
    }
}
