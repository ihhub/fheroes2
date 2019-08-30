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

#include "agg.h"
#include "text.h"
#include "button.h"
#include "cursor.h"
#include "settings.h"
#include "monster.h"
#include "morale.h"
#include "speed.h"
#include "luck.h"
#include "army.h"
#include "skill.h"
#include "dialog.h"
#include "game.h"
#include "payment.h"
#include "pocketpc.h"
#include "battle.h"
#include "world.h"

void DrawMonsterStats(const Point &, const Troop &);
void DrawBattleStats(const Point &, const Troop &);

int Dialog::ArmyInfo(const Troop & troop, int flags)
{
    if(Settings::Get().QVGA()) return PocketPC::DialogArmyInfo(troop, flags);
    Display & display = Display::Get();

    const int viewarmy = Settings::Get().ExtGameEvilInterface() ? ICN::VIEWARME : ICN::VIEWARMY;
    const Surface & sprite_dialog = AGG::GetICN(viewarmy, 0);

    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    SpriteBack back(Rect((display.w() - sprite_dialog.w()) / 2, (display.h() - sprite_dialog.h()) / 2, sprite_dialog.w(), sprite_dialog.h()));
    const Rect & pos_rt = back.GetArea();
    sprite_dialog.Blit(pos_rt.x, pos_rt.y, display);

    Point dst_pt;
    Text text;

    dst_pt.x = pos_rt.x + 400;
    dst_pt.y = pos_rt.y + 40;

    DrawMonsterStats(dst_pt, troop);

    if(troop.isBattle())
    {
	dst_pt.x = pos_rt.x + 400;
	dst_pt.y = pos_rt.y + 205;

	DrawBattleStats(dst_pt, troop);
    }

    // name
    text.Set(troop.GetName(), Font::BIG);
    dst_pt.x = pos_rt.x + 140 - text.w() / 2;
    dst_pt.y = pos_rt.y + 40;
    text.Blit(dst_pt);

    // count
    text.Set(GetString(troop.GetCount()));
    dst_pt.x = pos_rt.x + 140 - text.w() / 2;
    dst_pt.y = pos_rt.y + 225;
    text.Blit(dst_pt);

    const Sprite & frame = AGG::GetICN(troop.ICNMonh(), 0);
    frame.Blit(pos_rt.x + (pos_rt.w / 2 - frame.w()) / 2, pos_rt.y + 180 - frame.h());

    // button upgrade
    dst_pt.x = pos_rt.x + 284;
    dst_pt.y = pos_rt.y + 190;
    Button buttonUpgrade(dst_pt.x, dst_pt.y, viewarmy, 5, 6);

    // button dismiss
    dst_pt.x = pos_rt.x + 284;
    dst_pt.y = pos_rt.y + 222;
    Button buttonDismiss(dst_pt.x, dst_pt.y, viewarmy, 1, 2);

    // button exit
    dst_pt.x = pos_rt.x + 415;
    dst_pt.y = pos_rt.y + 225;
    Button buttonExit(dst_pt.x, dst_pt.y, viewarmy, 3, 4);

    if(READONLY & flags)
    {
        buttonDismiss.Press();
        buttonDismiss.SetDisable(true);
    }

    if(!troop.isBattle() && troop.isAllowUpgrade())
    {
        if(UPGRADE & flags)
        {
    	    if(UPGRADE_DISABLE & flags)
    	    {
        	buttonUpgrade.Press();
        	buttonUpgrade.SetDisable(true);
            }
	    else
        	buttonUpgrade.SetDisable(false);
	    buttonUpgrade.Draw();
        }
        else buttonUpgrade.SetDisable(true);
    }
    else buttonUpgrade.SetDisable(true);

    if(BUTTONS & flags)
    {
        if(!troop.isBattle()) buttonDismiss.Draw();
        buttonExit.Draw();
    }

    LocalEvent & le = LocalEvent::Get();
    int result = Dialog::ZERO;

    cursor.Show();
    display.Flip();

    // dialog menu loop
    while(le.HandleEvents())
    {
        if(flags & BUTTONS)
        {
            if(buttonUpgrade.isEnable()) le.MousePressLeft(buttonUpgrade) ? (buttonUpgrade).PressDraw() : (buttonUpgrade).ReleaseDraw();
    	    if(buttonDismiss.isEnable()) le.MousePressLeft(buttonDismiss) ? (buttonDismiss).PressDraw() : (buttonDismiss).ReleaseDraw();
    	    le.MousePressLeft(buttonExit) ? (buttonExit).PressDraw() : (buttonExit).ReleaseDraw();

            // upgrade
            if(buttonUpgrade.isEnable() && le.MouseClickLeft(buttonUpgrade))
            {
		std::string msg = 1.0f != Monster::GetUpgradeRatio() ?
		    _("Your troops can be upgraded, but it will cost you %{ratio} times the difference in cost for each troop, rounded up to next highest number. Do you wish to upgrade them?") :
		    _("Your troops can be upgraded, but it will cost you dearly. Do you wish to upgrade them?");
		StringReplace(msg, "%{ratio}", GetString(Monster::GetUpgradeRatio(), 2));
        	if(Dialog::YES == Dialog::ResourceInfo("", msg,	troop.GetUpgradeCost(), Dialog::YES|Dialog::NO))
		{
		    result = Dialog::UPGRADE;
        	    break;
        	}
    	    }
    	    else
	    // dismiss
            if(buttonDismiss.isEnable() && le.MouseClickLeft(buttonDismiss) &&
        	Dialog::YES == Dialog::Message("", _("Are you sure you want to dismiss this army?"), Font::BIG, Dialog::YES | Dialog::NO))
            {
        	result = Dialog::DISMISS;
        	break;
    	    }
    	    else
	    // exit
    	    if(le.MouseClickLeft(buttonExit) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)){ result = Dialog::CANCEL; break; }
        }
        else
        {
            if(!le.MousePressRight()) break;
        }
    }

    cursor.Hide();
    back.Restore();

    return result;
}

void DrawMonsterStats(const Point & dst, const Troop & troop)
{
    Point dst_pt;
    Text text;
    bool pda = Settings::Get().QVGA();

    // attack
    text.Set(std::string(_("Attack")) + ":");
    dst_pt.x = dst.x - text.w();
    dst_pt.y = dst.y;
    text.Blit(dst_pt);

    const int ox = 10;

    text.Set(troop.GetAttackString());
    dst_pt.x = dst.x + ox;
    text.Blit(dst_pt);

    // defense
    text.Set(std::string(_("Defense")) + ":");
    dst_pt.x = dst.x - text.w();
    dst_pt.y += (pda ? 14 : 18);
    text.Blit(dst_pt);

    text.Set(troop.GetDefenseString());
    dst_pt.x = dst.x + ox;
    text.Blit(dst_pt);

    // shot
    if(troop.isArchers())
    {
	std::string message = troop.isBattle() ? _("Shots Left") : _("Shots");
	message.append(":");
	text.Set(message);
	dst_pt.x = dst.x - text.w();
	dst_pt.y += (pda ? 14 : 18);
	text.Blit(dst_pt);

	text.Set(troop.GetShotString());
	dst_pt.x = dst.x + ox;
	text.Blit(dst_pt);
    }

    // damage
    text.Set(std::string(_("Damage")) + ":");
    dst_pt.x = dst.x - text.w();
    dst_pt.y += (pda ? 14 : 18);
    text.Blit(dst_pt);

    if(troop().GetDamageMin() != troop().GetDamageMax())
	text.Set(GetString(troop().GetDamageMin()) + " - " + GetString(troop().GetDamageMax()));
    else
	text.Set(GetString(troop().GetDamageMin()));
    dst_pt.x = dst.x + ox;
    text.Blit(dst_pt);

    // hp
    text.Set(std::string(_("Hit Points")) + ":");
    dst_pt.x = dst.x - text.w();
    dst_pt.y += (pda ? 14 : 18);
    text.Blit(dst_pt);

    text.Set(GetString(troop().GetHitPoints()));
    dst_pt.x = dst.x + ox;
    text.Blit(dst_pt);

    if(troop.isBattle())
    {
	text.Set(std::string(_("Hit Points Left")) + ":");
	dst_pt.x = dst.x - text.w();
	dst_pt.y += (pda ? 14 : 18);
	text.Blit(dst_pt);

	text.Set(GetString(troop.GetHitPointsLeft()));
	dst_pt.x = dst.x + ox;
	text.Blit(dst_pt);
    }

    // speed
    text.Set(std::string(_("Speed")) + ":");
    dst_pt.x = dst.x - text.w();
    dst_pt.y += (pda ? 14 : 18);
    text.Blit(dst_pt);

    text.Set(troop.GetSpeedString());
    dst_pt.x = dst.x + ox;
    text.Blit(dst_pt);

    // morale
    text.Set(std::string(_("Morale")) + ":");
    dst_pt.x = dst.x - text.w();
    dst_pt.y += (pda ? 14 : 18);
    text.Blit(dst_pt);

    text.Set(Morale::String(troop.GetMorale()));
    dst_pt.x = dst.x + ox;
    text.Blit(dst_pt);

    // luck
    text.Set(std::string(_("Luck")) + ":");
    dst_pt.x = dst.x - text.w();
    dst_pt.y += (pda ? 14 : 18);
    text.Blit(dst_pt);

    text.Set(Luck::String(troop.GetLuck()));
    dst_pt.x = dst.x + ox;
    text.Blit(dst_pt);
}

Sprite GetModesSprite(u32 mod)
{
    switch(mod)
    {
	case Battle::SP_BLOODLUST:	return AGG::GetICN(ICN::SPELLINF, 9);
	case Battle::SP_BLESS:		return AGG::GetICN(ICN::SPELLINF, 3);
	case Battle::SP_HASTE:		return AGG::GetICN(ICN::SPELLINF, 0);
	case Battle::SP_SHIELD:		return AGG::GetICN(ICN::SPELLINF, 10);
	case Battle::SP_STONESKIN:	return AGG::GetICN(ICN::SPELLINF, 13);
	case Battle::SP_DRAGONSLAYER:	return AGG::GetICN(ICN::SPELLINF, 8);
	case Battle::SP_STEELSKIN:	return AGG::GetICN(ICN::SPELLINF, 14);
	case Battle::SP_ANTIMAGIC:	return AGG::GetICN(ICN::SPELLINF, 12);
	case Battle::SP_CURSE:		return AGG::GetICN(ICN::SPELLINF, 4);
	case Battle::SP_SLOW:		return AGG::GetICN(ICN::SPELLINF, 1);
	case Battle::SP_BERSERKER:	return AGG::GetICN(ICN::SPELLINF, 5);
	case Battle::SP_HYPNOTIZE:	return AGG::GetICN(ICN::SPELLINF, 7);
	case Battle::SP_BLIND:		return AGG::GetICN(ICN::SPELLINF, 2);
	case Battle::SP_PARALYZE:	return AGG::GetICN(ICN::SPELLINF, 6);
	case Battle::SP_STONE:		return AGG::GetICN(ICN::SPELLINF, 11);
	default: break;
    }

    return Sprite();
}

void DrawBattleStats(const Point & dst, const Troop & b)
{
    const u32 modes[] = {
	Battle::SP_BLOODLUST, Battle::SP_BLESS, Battle::SP_HASTE, Battle::SP_SHIELD, Battle::SP_STONESKIN,
	Battle::SP_DRAGONSLAYER, Battle::SP_STEELSKIN, Battle::SP_ANTIMAGIC, Battle::SP_CURSE, Battle::SP_SLOW,
	Battle::SP_BERSERKER, Battle::SP_HYPNOTIZE, Battle::SP_BLIND, Battle::SP_PARALYZE, Battle::SP_STONE
    };

    // accumulate width
    u32 ow = 0;

    for(u32 ii = 0; ii < ARRAY_COUNT(modes); ++ii)
	if(b.isModes(modes[ii]))
	{
	    const Sprite & sprite = GetModesSprite(modes[ii]);
	    if(sprite.isValid()) ow += sprite.w() + 4;
	}

    ow -= 4;
    ow = dst.x - ow / 2;

    Text text;

    // blit centered
    for(u32 ii = 0; ii < ARRAY_COUNT(modes); ++ii)
	if(b.isModes(modes[ii]))
	{
	    const Sprite & sprite = GetModesSprite(modes[ii]);
	    if(sprite.isValid())
	    {
		sprite.Blit(ow, dst.y);

		const u32 duration = b.GetAffectedDuration(modes[ii]);
		if(duration)
		{
		    text.Set(GetString(duration), Font::SMALL);
		    text.Blit(ow + (sprite.w() - text.w()) / 2, dst.y + sprite.h() + 1);
		}

		ow += sprite.w() + 4;
	    }
	}
}

int Dialog::ArmyJoinFree(const Troop & troop, Heroes & hero)
{
    Display & display = Display::Get();
    const Settings & conf = Settings::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    int oldthemes = cursor.Themes();
    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    std::string message = _("A group of %{monster} with a desire for greater glory wish to join you.\nDo you accept?");
    StringReplace(message, "%{monster}", StringLower(troop.GetMultiName()));

    TextBox textbox(message, Font::BIG, BOXAREA_WIDTH);
    const int buttons = Dialog::YES | Dialog::NO;
    int posy = 0;

    FrameBox box(10 + textbox.h() + 10, buttons);
    const Rect & pos = box.GetArea();

    posy = pos.y + 10;
    textbox.Blit(pos.x, posy);

    ButtonGroups btnGroups(pos, buttons);
    Button btnHeroes(pos.x + pos.w / 2 - 20, pos.y + pos.h - 35, (conf.ExtGameEvilInterface() ? ICN::ADVEBTNS : ICN::ADVBTNS), 0, 1);

    if(hero.GetArmy().GetCount() < hero.GetArmy().Size() || hero.GetArmy().HasMonster(troop))
	btnHeroes.SetDisable(true);
    else
    {
	//TextBox textbox2(_("Not room in\nthe garrison"), Font::SMALL, 100);
	//textbox2.Blit(btnHeroes.x - 35, btnHeroes.y - 30);
	btnHeroes.Draw();
	btnGroups.DisableButton1(true);
    }

    btnGroups.Draw();
    cursor.Show();
    display.Flip();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    int result = Dialog::ZERO;

    while(result == Dialog::ZERO && le.HandleEvents())
    {
	if(btnHeroes.isEnable())
    	    le.MousePressLeft(btnHeroes) ? btnHeroes.PressDraw() : btnHeroes.ReleaseDraw();

        if(!buttons && !le.MousePressRight()) break;

        result = btnGroups.QueueEventProcessing();

	if(btnHeroes.isEnable() && le.MouseClickLeft(btnHeroes))
	{
	    hero.OpenDialog(false, false);

	    if(hero.GetArmy().GetCount() < hero.GetArmy().Size())
	    {
    		btnGroups.DisableButton1(false);
		btnGroups.Draw();
	    }

	    cursor.Show();
	    display.Flip();
	}
    }

    cursor.Hide();
    cursor.SetThemes(oldthemes);
    cursor.Show();

    return result;
}

int Dialog::ArmyJoinWithCost(const Troop & troop, u32 join, u32 gold, Heroes & hero)
{
    Display & display = Display::Get();
    const Settings & conf = Settings::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    int oldthemes = cursor.Themes();
    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    std::string message;

    if(troop.GetCount() == 1)
	message = _("The creature is swayed by your diplomatic tongue, and offers to join your army for the sum of %{gold} gold.\nDo you accept?");
    else
    {
        message = _("The creatures are swayed by your diplomatic\ntongue, and make you an offer:\n \n");

        if(join != troop.GetCount())
    	    message += _("%{offer} of the %{total} %{monster} will join your army, and the rest will leave you alone, for the sum of %{gold} gold.\nDo you accept?");
        else
    	    message += _("All %{offer} of the %{monster} will join your army for the sum of %{gold} gold.\nDo you accept?");
    }

    StringReplace(message, "%{offer}", join);
    StringReplace(message, "%{total}", troop.GetCount());
    StringReplace(message, "%{monster}", StringLower(troop.GetPluralName(join)));
    StringReplace(message, "%{gold}", gold);

    TextBox textbox(message, Font::BIG, BOXAREA_WIDTH);
    const int buttons = Dialog::YES | Dialog::NO;
    const Sprite & sprite = AGG::GetICN(ICN::RESOURCE, 6);
    int posy = 0;
    Text text;

    message = _("(Rate: %{percent})");
    StringReplace(message, "%{percent}", troop.GetMonster().GetCost().gold * join * 100 / gold);
    text.Set(message, Font::BIG);

    FrameBox box(10 + textbox.h() + 10 + text.h() + 40 + sprite.h() + 10, buttons);
    const Rect & pos = box.GetArea();

    posy = pos.y + 10;
    textbox.Blit(pos.x, posy);

    posy += textbox.h() + 10;
    text.Blit(pos.x + (pos.w - text.w()) / 2, posy);


    posy += text.h() + 40;
    sprite.Blit(pos.x + (pos.w - sprite.w()) / 2, posy);

    TextSprite tsTotal(GetString(gold) + " " + "(" + "total: " + GetString(world.GetKingdom(hero.GetColor()).GetFunds().Get(Resource::GOLD)) + ")", Font::SMALL,
	    pos.x + (pos.w - text.w()) / 2, posy + sprite.h() + 5);
    tsTotal.Show();

    ButtonGroups btnGroups(pos, buttons);
    Button btnMarket(pos.x + pos.w / 2 - 60 - 36, posy, (conf.ExtGameEvilInterface() ? ICN::ADVEBTNS : ICN::ADVBTNS), 4, 5);
    Button btnHeroes(pos.x + pos.w / 2 + 60, posy, (conf.ExtGameEvilInterface() ? ICN::ADVEBTNS : ICN::ADVBTNS), 0, 1);
    const Kingdom & kingdom = hero.GetKingdom();

    if(! kingdom.AllowPayment(payment_t(Resource::GOLD, gold)))
	btnGroups.DisableButton1(true);

    TextSprite tsEnough;

    if(kingdom.GetCountMarketplace())
    {
	if(kingdom.AllowPayment(payment_t(Resource::GOLD, gold)))
	    btnMarket.SetDisable(true);
	else
	{
	    std::string msg = _("Not enough gold (%{gold})");
	    StringReplace(msg, "%{gold}", gold - kingdom.GetFunds().Get(Resource::GOLD));
	    tsEnough.SetText(msg, Font::YELLOW_SMALL);
	    tsEnough.SetPos(btnMarket.x - 25, btnMarket.y - 17);
	    tsEnough.Show();
	    btnMarket.Draw();
	}
    }

    if(hero.GetArmy().GetCount() < hero.GetArmy().Size() || hero.GetArmy().HasMonster(troop))
	btnHeroes.SetDisable(true);
    else
    {
	TextBox textbox2(_("Not room in\nthe garrison"), Font::SMALL, 100);
	textbox2.Blit(btnHeroes.x - 35, btnHeroes.y - 30);
	btnHeroes.Draw();

	btnGroups.DisableButton1(true);
    }

    btnGroups.Draw();
    cursor.Show();
    display.Flip();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    int result = Dialog::ZERO;

    while(result == Dialog::ZERO && le.HandleEvents())
    {
	if(btnMarket.isEnable())
    	    le.MousePressLeft(btnMarket) ? btnMarket.PressDraw() : btnMarket.ReleaseDraw();

	if(btnHeroes.isEnable())
    	    le.MousePressLeft(btnHeroes) ? btnHeroes.PressDraw() : btnHeroes.ReleaseDraw();

        if(!buttons && !le.MousePressRight()) break;

        result = btnGroups.QueueEventProcessing();

	if(btnMarket.isEnable() && le.MouseClickLeft(btnMarket))
	{
	    Marketplace(false);

	    cursor.Hide();
	    tsTotal.Hide();
	    tsTotal.SetText(GetString(gold) + " " + "(" + "total: " + GetString(world.GetKingdom(hero.GetColor()).GetFunds().Get(Resource::GOLD)) + ")");
	    tsTotal.Show();

	    if(kingdom.AllowPayment(payment_t(Resource::GOLD, gold)))
	    {
		tsEnough.Hide();
    		btnGroups.DisableButton1(false);
		btnGroups.Draw();
	    }
	    else
	    {
		tsEnough.Hide();
		std::string msg = _("Not enough gold (%{gold})");
		StringReplace(msg, "%{gold}", gold - kingdom.GetFunds().Get(Resource::GOLD));
		tsEnough.SetText(msg, Font::SMALL);
		tsEnough.Show();
	    }

	    cursor.Show();
	    display.Flip();
	}
	else
	if(btnHeroes.isEnable() && le.MouseClickLeft(btnHeroes))
	{
	    hero.OpenDialog(false, false);

	    if(hero.GetArmy().GetCount() < hero.GetArmy().Size())
	    {
    		btnGroups.DisableButton1(false);
		btnGroups.Draw();
	    }

	    cursor.Show();
	    display.Flip();
	}
    }

    cursor.Hide();
    cursor.SetThemes(oldthemes);
    cursor.Show();

    return result;
}
