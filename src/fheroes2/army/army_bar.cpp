/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "agg.h"
#include "cursor.h"
#include "settings.h"
#include "race.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "world.h"
#include "army.h"
#include "text.h"
#include "army_troop.h"
#include "army_bar.h"

void RedistributeArmy(ArmyTroop & troop1 /* from */, ArmyTroop & troop2 /* to */)
{
    const Army* army1 = troop1.GetArmy();
    const Army* army2 = troop2.GetArmy();

    bool save_last_troop = army1->SaveLastTroop() && army1 != army2;

    if(2 > troop1.GetCount())
    {
        if(!save_last_troop || troop2.isValid())
            Army::SwapTroops(troop1, troop2);
    }
    else
    {
	const u32 free_slots = (army1 == army2 ? 1 : 0) + army2->Size() - army2->GetCount();
	const u32 max_count = save_last_troop ? troop1.GetCount() - 1 : troop1.GetCount();
        u32 redistr_count = troop1.GetCount() / 2;
        const u32 slots = Dialog::ArmySplitTroop((free_slots > max_count ? max_count : free_slots), max_count, redistr_count, save_last_troop);

        switch(slots)
        {
            case 3:
            case 4:
            case 5:
                if(save_last_troop)
                {
                    const Troop troop(troop1, troop1.GetCount() - 1);
                    troop1.SetCount(1);
                    const_cast<Army*>(army2)->SplitTroopIntoFreeSlots(troop, slots);
                }
                else
                {
                    const Troop troop(troop1);
                    troop1.Reset();
                    const_cast<Army*>(army2)->SplitTroopIntoFreeSlots(troop, slots);
                }
                break;

            case 2:
                troop2.Set(troop1, redistr_count);
                troop1.SetCount(troop1.GetCount() - redistr_count);
                break;

            default:
                break;
        }
    }
}

ArmyBar::ArmyBar(Army* ptr, bool mini, bool ro, bool change /* false */)
    : army(NULL), spcursor(AGG::GetICN(ICN::STRIP, 1)), use_mini_sprite(mini), read_only(ro), can_change(change)
{
    if(use_mini_sprite)
	SetBackground(Size(43, 43), RGBA(0, 45, 0));
    else
    {
        const Sprite & sprite = AGG::GetICN(ICN::STRIP, 2);
        SetItemSize(sprite.w(), sprite.h());
    }

    SetArmy(ptr);
}

void ArmyBar::SetArmy(Army* ptr)
{
    if(army && isSelected())
	ResetSelected();

    army = ptr;
    items.clear();

    if(ptr)
	for(u32 ii = 0; ii < ptr->Size(); ++ii)
	    items.push_back(reinterpret_cast<ArmyTroop*>(ptr->GetTroop(ii)));
    
    SetContentItems();
}

bool ArmyBar::isValid(void) const
{
    return army != NULL;
}

void ArmyBar::SetBackground(const Size & sz, const RGBA & fillColor)
{
    if(use_mini_sprite)
    {
        SetItemSize(sz.w, sz.h);

	backsf.Set(sz.w, sz.h, false);
	backsf.Fill(fillColor);
	backsf.DrawBorder(RGBA(0xd0, 0xc0, 0x48));

	spcursor.Set(sz.w, sz.h, true /* transparent */);
	spcursor.DrawBorder(RGBA(0xc0, 0x2c, 0));
    }
}

void ArmyBar::RedrawBackground(const Rect & pos, Surface & dstsf)
{
    if(use_mini_sprite)
        backsf.Blit(pos, dstsf);
    else
	AGG::GetICN(ICN::STRIP, 2).Blit(pos, dstsf);
}

void ArmyBar::RedrawItem(ArmyTroop & troop, const Rect & pos, bool selected, Surface & dstsf)
{
    if(troop.isValid())
    {
        Text text(GetString(troop.GetCount()),(use_mini_sprite ? Font::SMALL : Font::BIG));

	if(use_mini_sprite)
	{
	    const Sprite & mons32 = AGG::GetICN(ICN::MONS32, troop.GetSpriteIndex());
	    Rect srcrt(0, 0, mons32.w(), mons32.h());

            if(mons32.w() > pos.w)
            {
                srcrt.x = (mons32.w() - pos.w) / 2;
                srcrt.w = pos.w;
    	    }

    	    if(mons32.h() > pos.h)
            {
                srcrt.y = (mons32.h() - pos.h) / 2;
        	srcrt.h = pos.h;
            }

	    mons32.Blit(srcrt, pos.x + (pos.w - mons32.w()) / 2, pos.y + pos.h - mons32.h() - 1, dstsf);
	}
	else
	{
	    switch(troop.GetRace())
	    {
                case Race::KNGT: AGG::GetICN(ICN::STRIP, 4).Blit(pos, dstsf);  break;
                case Race::BARB: AGG::GetICN(ICN::STRIP, 5).Blit(pos, dstsf);  break;
                case Race::SORC: AGG::GetICN(ICN::STRIP, 6).Blit(pos, dstsf);  break;
                case Race::WRLK: AGG::GetICN(ICN::STRIP, 7).Blit(pos, dstsf);  break;
                case Race::WZRD: AGG::GetICN(ICN::STRIP, 8).Blit(pos, dstsf);  break;
                case Race::NECR: AGG::GetICN(ICN::STRIP, 9).Blit(pos, dstsf);  break;
                default: AGG::GetICN(ICN::STRIP, 10).Blit(pos, dstsf); break;
	    }

	    const Sprite & spmonh = AGG::GetICN(troop.ICNMonh(), 0);
	    spmonh.Blit(pos.x + spmonh.x(), pos.y + spmonh.y(), dstsf);
	}

	if(! use_mini_sprite)
	{
	    Surface black(Size(text.w() + 4, text.h()), false);
	    black.Fill(ColorBlack);
	    const Point pt(pos.x + pos.w - black.w() - 1, pos.y + pos.h - black.h() - 1);
	    black.Blit(pt.x, pt.y, dstsf);
	    text.Blit(pt.x + 2, pt.y + 1, dstsf);
	}
	else
	    text.Blit(pos.x + pos.w - text.w() - 3, pos.y + pos.h - text.h() - 1, dstsf);

	if(selected)
            spcursor.Move(pos.x, pos.y);
    }
}

void ArmyBar::ResetSelected(void)
{
    Cursor::Get().Hide();
    spcursor.Hide();
    Interface::ItemsActionBar<ArmyTroop>::ResetSelected();
}

void ArmyBar::Redraw(Surface & dstsf)
{
    Cursor::Get().Hide();
    spcursor.Hide();
    Interface::ItemsActionBar<ArmyTroop>::Redraw(dstsf);
}

bool ArmyBar::ActionBarCursor(const Point & cursor, ArmyTroop & troop, const Rect & pos)
{
    if(isSelected())
    {
	ArmyTroop* troop2 = GetSelectedItem();

	if(&troop == troop2)
	{
	    msg = _("View %{name}");
	    StringReplace(msg, "%{name}", troop.GetName());
	}
	else
	if(! troop.isValid())
	{
    	    msg = _("Move or right click Redistribute %{name}");
    	    StringReplace(msg, "%{name}", troop2->GetName());
	}
	else
	if(troop.GetID() == troop2->GetID())
	{
	    msg = _("Combine %{name} armies");
	    StringReplace(msg, "%{name}", troop.GetName());
	}
	else
	{
	    msg = _("Exchange %{name2} with %{name}");
	    StringReplace(msg, "%{name}", troop.GetName());
	    StringReplace(msg, "%{name2}", troop2->GetName());
	}
    }
    else
    if(troop.isValid())
    {
        msg = _("Select %{name}");
        StringReplace(msg, "%{name}", troop.GetName());
    }

    // drag drop - redistribute troops
    LocalEvent & le = LocalEvent::Get();
    ArmyTroop* troop_p = GetItem(le.GetMousePressLeft());

    if(!troop.isValid() && troop_p && troop_p->isValid())
    {
/*
	const Rect* pos_p = GetItemPos(le.GetMousePressLeft());
	const Point offset(Point(le.GetMousePressLeft()) - *pos_p);
	Surface sf(pos.w, pos.h);
	RedrawItem(*troop_p, Rect(0, 0, pos.w, pos.h), false, sf);
	sf.GrayScale();
	SpriteCursor sp;
	sp.SetSprite(sf);
	while(le.HandleEvents() && le.MousePressLeft()){ Cursor::Get().Hide(); sp.Move(Point(le.GetMouseCursor()) - offset); Cursor::Get().Show(); Display::Get().Flip(); DELAY(1); };
	Cursor::Get().Hide();
	sp.Hide();
*/
	while(le.HandleEvents() && le.MousePressLeft()){ Cursor::Get().Show(); Display::Get().Flip(); DELAY(1); };
	ArmyTroop* troop_r = GetItem(le.GetMouseReleaseLeft());

	if(troop_r && !troop_r->isValid())
	{
	    RedistributeArmy(*troop_p, *troop_r);
	    if(isSelected()) ResetSelected();
	    le.ResetPressLeft();
	    return true;
	}
	le.ResetPressLeft();
    }

    return false;
}

bool ArmyBar::ActionBarCursor(const Point & cursor, ArmyTroop & troop1, const Rect & pos1, ArmyTroop & troop2 /* selected */, const Rect & pos2)
{
    bool save_last_troop = troop2.GetArmy()->SaveLastTroop();

    if(troop1.isValid())
    {
	if(troop1.GetID() != troop2.GetID())
	{
	    msg = _("Exchange %{name2} with %{name}");
	    StringReplace(msg, "%{name}", troop1.GetName());
	    StringReplace(msg, "%{name2}", troop2.GetName());
	}
	else
	if(save_last_troop)
	    msg = _("Cannot move last troop");
	else
	{
	    msg = _("Combine %{name} armies");
	    StringReplace(msg, "%{name}", troop1.GetName());
	}
    }
    else
    if(save_last_troop)
        msg = _("Cannot move last troop");
    else
    {
    	msg = _("Move or right click Redistribute %{name}");
    	StringReplace(msg, "%{name}", troop2.GetName());
    }

    return false;
}

bool ArmyBar::ActionBarSingleClick(const Point & cursor, ArmyTroop & troop, const Rect & pos)
{
    if(isSelected())
    {
	ArmyTroop* troop2 = GetSelectedItem();

	// combine
        if(troop.GetID() == troop2->GetID())
        {
	    troop.SetCount(troop.GetCount() + troop2->GetCount());
	    troop2->Reset();
        }
        // exchange
        else
	    Army::SwapTroops(troop, *troop2);

        return false; // reset cursor
    }
    else
    if(troop.isValid())
    {
        if(! read_only) // select
        {
            Cursor::Get().Hide();
            spcursor.Hide();
        }
    }
    else
    {
	
        if(can_change) // add troop
	{
	    int cur = Monster::UNKNOWN;

	    if(army->GetCommander())
		switch(army->GetCommander()->GetRace())
    	    {
		case Race::KNGT: cur = Monster::PEASANT; break;
		case Race::BARB: cur = Monster::GOBLIN; break;
		case Race::SORC: cur = Monster::SPRITE; break;
		case Race::WRLK: cur = Monster::CENTAUR; break;
		case Race::WZRD: cur = Monster::HALFLING; break;
		case Race::NECR: cur = Monster::SKELETON; break;
		default: break;
    	    }

	    const Monster mons = Dialog::SelectMonster(cur);

	    if(mons.isValid())
	    {
		u32 count = 1;

		if(Dialog::SelectCount("Set Count", 1, 500000, count))
		    troop.Set(mons, count);
    	    }
	}

        return false;
    }

    return true;
}

bool ArmyBar::ActionBarSingleClick(const Point & cursor, ArmyTroop & troop1, const Rect & pos1, ArmyTroop & troop2 /* selected */, const Rect & pos2)
{
    if(troop2.GetArmy()->SaveLastTroop())
    {
	if(troop1.isValid())
	    Army::SwapTroops(troop1, troop2);
    }
    else
    {
	if(! troop1.isValid())
	    Army::SwapTroops(troop1, troop2);
	else
	if(troop1.isValid() && troop1.GetID() == troop2.GetID())
	{
	    troop1.SetCount(troop1.GetCount() + troop2.GetCount());
	    troop2.Reset();
	}
	else
	    Army::SwapTroops(troop1, troop2);
    }

    return false; // reset cursor
}

bool ArmyBar::ActionBarDoubleClick(const Point & cursor, ArmyTroop & troop, const Rect & pos)
{
    ArmyTroop* troop2 = GetSelectedItem();

    if(&troop == troop2)
    {
	int flags = (read_only || army->SaveLastTroop() ? Dialog::READONLY | Dialog::BUTTONS : Dialog::BUTTONS);
	const Castle* castle = army->inCastle();

	if(troop.isAllowUpgrade() &&
	    // allow upgrade
	    castle && castle->GetRace() == troop.GetRace() && castle->isBuild(troop.GetUpgrade().GetDwelling()))
	{
	    flags |= Dialog::UPGRADE;

	    if(! world.GetKingdom(army->GetColor()).AllowPayment(troop.GetUpgradeCost()))
		flags |= Dialog::UPGRADE_DISABLE;
	}

	switch(Dialog::ArmyInfo(troop, flags))
	{
    	    case Dialog::UPGRADE:
    		world.GetKingdom(army->GetColor()).OddFundsResource(troop.GetUpgradeCost());
		troop.Upgrade();
		break;

	    case Dialog::DISMISS:
            	troop.Reset();
		break;

	    default: break;
	}
    }

    ResetSelected();

    return true;
}

bool ArmyBar::ActionBarPressRight(const Point & cursor, ArmyTroop & troop, const Rect & pos)
{
    if(troop.isValid())
    {
	ResetSelected();

        if(can_change && ! army->SaveLastTroop())
            troop.Reset();
        else
	    Dialog::ArmyInfo(troop, 0);
    }
    // empty troops - redistribute troops
    if(isSelected())
    {
	ArmyTroop & troop2 = *GetSelectedItem();
	ResetSelected();

        RedistributeArmy(troop2, troop);
    }

    return true;
}

bool ArmyBar::ActionBarPressRight(const Point & cursor, ArmyTroop & troop1, const Rect & pos1, ArmyTroop & troop2 /* selected */, const Rect & pos2)
{
    ResetSelected();

    if(troop1.isValid())
	Dialog::ArmyInfo(troop1, 0);
    else
        RedistributeArmy(troop2, troop1);

    return true;
}

bool ArmyBar::QueueEventProcessing(std::string* str)
{
    msg.clear();
    bool res = Interface::ItemsActionBar<ArmyTroop>::QueueEventProcessing();
    if(str) *str = msg;
    return res;
}

bool ArmyBar::QueueEventProcessing(ArmyBar & bar, std::string* str)
{
    msg.clear();
    bool res = Interface::ItemsActionBar<ArmyTroop>::QueueEventProcessing(bar);
    if(str) *str = msg;
    return res;
}
