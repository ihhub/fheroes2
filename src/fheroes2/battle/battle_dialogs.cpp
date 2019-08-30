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

#include "engine.h"
#include "cursor.h"
#include "settings.h"
#include "agg.h"
#include "button.h"
#include "army.h"
#include "text.h"
#include "morale.h"
#include "luck.h"
#include "game.h"
#include "race.h"
#include "heroes.h"
#include "battle_interface.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "world.h"
#include "battle.h"

namespace Battle
{
    void GetSummaryParams(int res1, int res2, const HeroBase &, u32 exp, int &, std::string &);
    void SpeedRedraw(const Point &);
}

void Battle::SpeedRedraw(const Point & dst)
{
    int speed = Settings::Get().BattleSpeed();
    std::string str = _("speed: %{speed}");

    StringReplace(str, "%{speed}", speed);
    Text text(str, Font::SMALL);
    const Sprite & sprite = AGG::GetICN(ICN::CSPANEL, (speed < 3 ? 0 : (speed < 7 ? 1 : 2)));

    sprite.Blit(dst);
    text.Blit(dst.x + (sprite.w() - text.w()) / 2, dst.y + sprite.h() - 15);
}

void Battle::DialogBattleSettings(void)
{
    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    Settings & conf = Settings::Get();

    cursor.Hide();

    const Sprite & dialog = AGG::GetICN((conf.ExtGameEvilInterface() ? ICN::CSPANBKE : ICN::CSPANBKG), 0);

    Rect pos_rt;
    pos_rt.x = (display.w() - dialog.w()) / 2;
    pos_rt.y = (display.h() - dialog.h()) / 2;
    pos_rt.w = dialog.w();
    pos_rt.h = dialog.h();

    SpriteBack back(pos_rt);

    display.FillRect(back.GetArea(), ColorBlack);
    dialog.Blit(pos_rt.x, pos_rt.y);

    Button btn_ok(pos_rt.x + 113, pos_rt.y + 252, (conf.ExtGameEvilInterface() ? ICN::CSPANBTE : ICN::CSPANBTN), 0, 1);

    Rect   opt_speed(pos_rt.x + 36, pos_rt.y + 47, AGG::GetICN(ICN::CSPANEL, 0).w(), AGG::GetICN(ICN::CSPANEL, 0).h());
    Button opt_grid(pos_rt.x + 36, pos_rt.y + 157, ICN::CSPANEL, 8, 9);
    Button opt_shadow_movement(pos_rt.x + 128, pos_rt.y + 157, ICN::CSPANEL, 10, 11);
    Button opt_shadow_cursor(pos_rt.x + 220, pos_rt.y + 157, ICN::CSPANEL, 12, 13);

    btn_ok.Draw();

    if(conf.ExtBattleShowGrid()) opt_grid.Press();
    if(conf.ExtBattleShowMoveShadow()) opt_shadow_movement.Press();
    if(conf.ExtBattleShowMouseShadow()) opt_shadow_cursor.Press();

    SpeedRedraw(opt_speed);

    opt_grid.Draw();
    opt_shadow_movement.Draw();
    opt_shadow_cursor.Draw();

    cursor.Show();
    display.Flip();

    while(le.HandleEvents())
    {
	le.MousePressLeft(btn_ok) ? btn_ok.PressDraw() : btn_ok.ReleaseDraw();

	if(le.MouseClickLeft(opt_speed))
	{
	    conf.SetBattleSpeed((conf.BattleSpeed() + 1) % 11);
	    Game::UpdateBattleSpeed();
	    cursor.Hide();
	    SpeedRedraw(opt_speed);
	    cursor.Show();
	    display.Flip();
	}

	if(le.MouseClickLeft(opt_grid))
	{
	    conf.SetBattleGrid(!conf.ExtBattleShowGrid());
	    cursor.Hide();
	    opt_grid.isPressed() ? opt_grid.Release() : opt_grid.Press();
	    opt_grid.Draw();
	    cursor.Show();
	    display.Flip();
	}

	if(le.MouseClickLeft(opt_shadow_movement))
	{
	    conf.SetBattleMovementShaded(!conf.ExtBattleShowMoveShadow());
	    cursor.Hide();
	    opt_shadow_movement.isPressed() ? opt_shadow_movement.Release() : opt_shadow_movement.Press();
	    opt_shadow_movement.Draw();
	    cursor.Show();
	    display.Flip();
	}

	if(le.MouseClickLeft(opt_shadow_cursor))
	{
	    conf.SetBattleMouseShaded(!conf.ExtBattleShowMouseShadow());
	    cursor.Hide();
	    opt_shadow_cursor.isPressed() ? opt_shadow_cursor.Release() : opt_shadow_cursor.Press();
	    opt_shadow_cursor.Draw();
	    cursor.Show();
	    display.Flip();
	}

        // exit
	if(Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT) || le.MouseClickLeft(btn_ok)) break;
    }

    // restore background
    cursor.Hide();
    back.Restore();
    cursor.Show();
    display.Flip();
}

void Battle::GetSummaryParams(int res1, int res2, const HeroBase & hero, u32 exp, int & icn_anim, std::string & msg)
{
    if(res1 & RESULT_WINS)
    {
	icn_anim = ICN::WINCMBT;
	if(res2 & RESULT_SURRENDER)
    	    msg.append(_("The enemy has surrendered!"));
    	else
	if(res2 & RESULT_RETREAT)
    	    msg.append(_("The enemy has fled!"));
	else
	    msg.append(_("A glorious victory!"));

	if(hero.isHeroes())
	{
    	    msg.append("\n");
    	    msg.append(_("For valor in combat, %{name} receives %{exp} experience"));
    	    StringReplace(msg, "%{name}", hero.GetName());
    	    StringReplace(msg, "%{exp}", exp);
    	}
    }
    else
    if(res1 & RESULT_RETREAT)
    {
	icn_anim = ICN::CMBTFLE3;
	msg.append(_("The cowardly %{name} flees from battle."));
    	StringReplace(msg, "%{name}", hero.GetName());
    }
    else
    if(res1 & RESULT_SURRENDER)
    {
	icn_anim = ICN::CMBTSURR;
	msg.append(_("%{name} surrenders to the enemy, and departs in shame."));
    	StringReplace(msg, "%{name}", hero.GetName());
    }
    else
    {
	icn_anim = ICN::CMBTLOS3;
	msg.append(_("Your force suffer a bitter defeat, and %{name} abandons your cause."));
    	StringReplace(msg, "%{name}", hero.GetName());
    }
}

void Battle::Arena::DialogBattleSummary(const Result & res) const
{
    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    Settings & conf = Settings::Get();

    const Troops killed1 = army1->GetKilledTroops();
    const Troops killed2 = army2->GetKilledTroops();

    cursor.Hide();
    cursor.SetThemes(Cursor::POINTER);

    std::string msg;
    int icn_anim = ICN::UNKNOWN;

    if((res.army1 & RESULT_WINS) && army1->GetCommander() && army1->GetCommander()->isControlHuman())
    {
    	GetSummaryParams(res.army1, res.army2, *army1->GetCommander(), res.exp1, icn_anim, msg);
	if(conf.Music()) AGG::PlayMusic(MUS::BATTLEWIN, false);
    }
    else
    if((res.army2 & RESULT_WINS) && army2->GetCommander() && army2->GetCommander()->isControlHuman())
    {
    	GetSummaryParams(res.army2, res.army1, *army2->GetCommander(), res.exp2, icn_anim, msg);
	if(conf.Music()) AGG::PlayMusic(MUS::BATTLEWIN, false);
    }
    else
    if(army1->GetCommander() && army1->GetCommander()->isControlHuman())
    {
    	GetSummaryParams(res.army1, res.army2, *army1->GetCommander(), res.exp1, icn_anim, msg);
	if(conf.Music()) AGG::PlayMusic(MUS::BATTLELOSE, false);
    }
    else
    if(army2->GetCommander() && army2->GetCommander()->isControlHuman())
    {
    	GetSummaryParams(res.army2, res.army1, *army2->GetCommander(), res.exp2, icn_anim, msg);
	if(conf.Music()) AGG::PlayMusic(MUS::BATTLELOSE, false);
    }
    else
    // AI move
    if(army1->GetCommander() && army1->GetCommander()->isControlAI())
    {
	// AI wins
	if(res.army1 & RESULT_WINS)
	{
	    icn_anim = ICN::CMBTLOS3;
	    msg.append(_("Your force suffer a bitter defeat."));
	}
	else
	// Human wins
	if(res.army2 & RESULT_WINS)
	{
	    icn_anim = ICN::WINCMBT;
	    msg.append(_("A glorious victory!"));
	}
    }

    const Sprite & dialog = AGG::GetICN((conf.ExtGameEvilInterface() ? ICN::WINLOSEE : ICN::WINLOSE), 0);

    Rect pos_rt;
    pos_rt.x = (display.w() - dialog.w()) / 2;
    pos_rt.y = (display.h() - (conf.QVGA() ? 224 : dialog.h())) / 2;
    pos_rt.w = dialog.w();
    pos_rt.h = conf.QVGA() ? 224 : dialog.h();

    SpriteBack back(pos_rt);

    if(conf.QVGA())
    {
	dialog.Blit(Rect(0, 232, pos_rt.w, 224), pos_rt.x, pos_rt.y);
	dialog.Blit(Rect(0, 0, pos_rt.w, 30), pos_rt.x, pos_rt.y);
    }
    else
	dialog.Blit(pos_rt.x, pos_rt.y);

    const int anime_ox = 47;
    const int anime_oy = 36;

    if(!conf.QVGA())
    {
	const Sprite & sprite1 = AGG::GetICN(icn_anim, 0);
	const Sprite & sprite2 = AGG::GetICN(icn_anim, 1);

	sprite1.Blit(pos_rt.x + anime_ox + sprite1.x(), pos_rt.y + anime_oy + sprite1.y());
	sprite2.Blit(pos_rt.x + anime_ox + sprite2.x(), pos_rt.y + anime_oy + sprite2.y());
    }

    Button btn_ok(pos_rt.x + 121, pos_rt.y + (conf.QVGA() ? 176 : 410), (conf.ExtGameEvilInterface() ? ICN::WINCMBBE : ICN::WINCMBTB), 0, 1);

    TextBox box(msg, Font::BIG, 270);
    box.Blit(pos_rt.x + 25, pos_rt.y + (conf.QVGA() ? 20 : 175));

    // battlefield casualties
    Text text(_("Battlefield Casualties"), Font::SMALL);
    text.Blit(pos_rt.x + (pos_rt.w - text.w()) / 2, pos_rt.y + (conf.QVGA() ? 58 : 270));

    // attacker
    text.Set(_("Attacker"), Font::SMALL);
    text.Blit(pos_rt.x + (pos_rt.w - text.w()) / 2, pos_rt.y + (conf.QVGA() ? 70 : 285));

    if(killed1.isValid())
	Army::DrawMons32Line(killed1, pos_rt.x + 25, pos_rt.y + (conf.QVGA() ? 83 : 303), 270);
    else
    {
	text.Set("None", Font::SMALL);
	text.Blit(pos_rt.x + (pos_rt.w - text.w()) / 2, pos_rt.y + (conf.QVGA() ? 80 : 300));
    }

    // defender
    text.Set(_("Defender"), Font::SMALL);
    text.Blit(pos_rt.x + (pos_rt.w - text.w()) / 2, pos_rt.y + (conf.QVGA() ? 120 : 345));

    if(killed2.isValid())
	Army::DrawMons32Line(killed2, pos_rt.x + 25, pos_rt.y + (conf.QVGA() ? 138 : 363), 270);
    else
    {
	text.Set("None", Font::SMALL);
	text.Blit(pos_rt.x + (pos_rt.w - text.w()) / 2, pos_rt.y + (conf.QVGA() ? 135 : 360));
    }

    btn_ok.Draw();

    cursor.Show();
    display.Flip();

    u32 frame = 0;

    while(le.HandleEvents())
    {
	le.MousePressLeft(btn_ok) ? btn_ok.PressDraw() : btn_ok.ReleaseDraw();

        // exit
	if(HotKeyCloseWindow || le.MouseClickLeft(btn_ok)) break;

        // animation
	if(!conf.QVGA() && Game::AnimateInfrequentDelay(Game::BATTLE_DIALOG_DELAY))
        {
	    if(0 == frame || 1 != ICN::AnimationFrame(icn_anim, 1, frame))
	    {
		const Sprite & sprite1 = AGG::GetICN(icn_anim, 0);
		const Sprite & sprite2 = AGG::GetICN(icn_anim, ICN::AnimationFrame(icn_anim, 1, frame));

		cursor.Hide();
		sprite1.Blit(pos_rt.x + anime_ox + sprite1.x(), pos_rt.y + anime_oy + sprite1.y());
		sprite2.Blit(pos_rt.x + anime_ox + sprite2.x(), pos_rt.y + anime_oy + sprite2.y());
		cursor.Show();
		display.Flip();
		++frame;
	    }
	}
    }

    // restore background
    cursor.Hide();
    back.Restore();
    cursor.Show();
    display.Flip();
}

int Battle::Arena::DialogBattleHero(const HeroBase & hero, bool buttons) const
{
    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    Settings & conf = Settings::Get();

    cursor.Hide();
    cursor.SetThemes(Cursor::POINTER);

    const bool readonly = current_color != hero.GetColor() || !buttons;
    const Sprite & dialog = AGG::GetICN((conf.ExtGameEvilInterface() ? ICN::VGENBKGE : ICN::VGENBKG), 0);

    Rect pos_rt;
    pos_rt.x = (display.w() - dialog.w()) / 2;
    pos_rt.y = (display.h() - dialog.h()) / 2;
    pos_rt.w = dialog.w();
    pos_rt.h = dialog.h();

    SpriteBack back(pos_rt);

    dialog.Blit(pos_rt.x, pos_rt.y);
    hero.PortraitRedraw(pos_rt.x + 27, pos_rt.y + 42, PORT_BIG, display);

    int col = (Color::NONE == hero.GetColor() ? 1 : Color::GetIndex(hero.GetColor()) + 1);
    AGG::GetICN(ICN::VIEWGEN, col).Blit(pos_rt.x + 148, pos_rt.y + 36);

    Point tp(pos_rt);

    std::string str;
    Text text;
    text.Set(Font::SMALL);
    str = _("%{name} the %{race}");
    StringReplace(str, "%{name}", hero.GetName());
    StringReplace(str, "%{race}", Race::String(hero.GetRace()));
    text.Set(str);
    tp.x = pos_rt.x + (pos_rt.w - text.w()) / 2;
    tp.y += 10;
    text.Blit(tp);
    str = _("Attack") + std::string(": ") + GetString(hero.GetAttack());
    text.Set(str);
    tp.x = pos_rt.x + 205 - text.w() / 2;
    tp.y = pos_rt.y + 40;
    text.Blit(tp);
    str = _("Defense") + std::string(": ") + GetString(hero.GetDefense());
    text.Set(str);
    tp.x = pos_rt.x + 205 - text.w() / 2;
    tp.y = pos_rt.y + 51;
    text.Blit(tp);
    str = _("Spell Power") + std::string(": ") + GetString(hero.GetPower());
    text.Set(str);
    tp.x = pos_rt.x + 205 - text.w() / 2;
    tp.y = pos_rt.y + 62;
    text.Blit(tp);
    str = _("Knowledge") + std::string(": ") + GetString(hero.GetKnowledge());
    text.Set(str);
    tp.x = pos_rt.x + 205 - text.w() / 2;
    tp.y = pos_rt.y + 73;
    text.Blit(tp);
    str = _("Morale") + std::string(": ") + Morale::String(hero.GetMorale());
    text.Set(str);
    tp.x = pos_rt.x + 205 - text.w() / 2;
    tp.y = pos_rt.y + 84;
    text.Blit(tp);
    str = _("Luck") + std::string(": ") + Luck::String(hero.GetLuck());
    text.Set(str);
    tp.x = pos_rt.x + 205 - text.w() / 2;
    tp.y = pos_rt.y + 95;
    text.Blit(tp);
    str = _("Spell Points") + std::string(": ") + GetString(hero.GetSpellPoints()) + "/" + GetString(hero.GetMaxSpellPoints());
    text.Set(str);
    tp.x = pos_rt.x + 205 - text.w() / 2;
    tp.y = pos_rt.y + 117;
    text.Blit(tp);

    Button btnCast(pos_rt.x + 30, pos_rt.y + 148, ICN::VIEWGEN, 9, 10);
    Button btnRetreat(pos_rt.x + 89, pos_rt.y + 148, ICN::VIEWGEN, 11, 12);
    Button btnSurrender(pos_rt.x + 148, pos_rt.y + 148, ICN::VIEWGEN, 13, 14);
    Button btnClose(pos_rt.x + 207, pos_rt.y + 148, ICN::VIEWGEN, 15, 16);

    btnCast.SetDisable(readonly || !hero.HaveSpellBook() || hero.Modes(Heroes::SPELLCASTED));
    btnRetreat.SetDisable(readonly || !CanRetreatOpponent(hero.GetColor()));
    btnSurrender.SetDisable(readonly || !CanSurrenderOpponent(hero.GetColor()));

    btnCast.Draw();
    btnRetreat.Draw();
    btnSurrender.Draw();
    btnClose.Draw();

    if(!conf.QVGA())
    {
	Surface shadow(btnCast, false);
	shadow.Fill(ColorBlack);
	shadow.SetAlphaMod(80);
	if(btnCast.isDisable()) shadow.Blit(btnCast, display);
	if(btnRetreat.isDisable()) shadow.Blit(btnRetreat, display);
	if(btnSurrender.isDisable()) shadow.Blit(btnSurrender, display);
    }

    int result = 0;

    cursor.Show();
    display.Flip();

    while(le.HandleEvents() && !result)
    {
	btnCast.isEnable() && le.MousePressLeft(btnCast) ? btnCast.PressDraw() : btnCast.ReleaseDraw();
	btnRetreat.isEnable() && le.MousePressLeft(btnRetreat) ? btnRetreat.PressDraw() : btnRetreat.ReleaseDraw();
	btnSurrender.isEnable() && le.MousePressLeft(btnSurrender) ? btnSurrender.PressDraw() : btnSurrender.ReleaseDraw();
	le.MousePressLeft(btnClose) ? btnClose.PressDraw() : btnClose.ReleaseDraw();

	if(!buttons && !le.MousePressRight()) break;

	if(Game::HotKeyPressEvent(Game::EVENT_BATTLE_CASTSPELL) ||
		(btnCast.isEnable() && le.MouseClickLeft(btnCast))) result = 1;

	if(Game::HotKeyPressEvent(Game::EVENT_BATTLE_RETREAT) ||
		(btnRetreat.isEnable() && le.MouseClickLeft(btnRetreat))) result = 2;

	if(Game::HotKeyPressEvent(Game::EVENT_BATTLE_SURRENDER) ||
		(btnSurrender.isEnable() && le.MouseClickLeft(btnSurrender))) result = 3;

	if(le.MousePressRight(btnCast))
	    Dialog::Message(_("Cast Spell"), _("Cast a magical spell. You may only cast one spell per combat round. The round is reset when every creature has had a turn"), Font::BIG);
	else
	if(le.MousePressRight(btnRetreat))
	    Dialog::Message(_("Retreat"), _("Retreat your hero, abandoning your creatures. Your hero will be available for you to recruit again, however, the hero will have only a novice hero's forces."), Font::BIG);
	else
	if(le.MousePressRight(btnSurrender))
	    Dialog::Message(_("Surrender"), _("Surrendering costs gold. However if you pay the ransom, the hero and all of his or her surviving creatures will be available to recruit again."), Font::BIG);
	else
	if(le.MousePressRight(btnClose))
	    Dialog::Message(_("Cancel"), _("Return to the battle."), Font::BIG);

        // exit
	if(HotKeyCloseWindow || le.MouseClickLeft(btnClose)) break;
    }

    cursor.Hide();
    back.Restore();
    cursor.Show();
    display.Flip();

    return result;
}

bool Battle::DialogBattleSurrender(const HeroBase & hero, u32 cost)
{
    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    Settings & conf = Settings::Get();

    cursor.Hide();
    cursor.SetThemes(Cursor::POINTER);

    const Sprite & dialog = AGG::GetICN(conf.ExtGameEvilInterface() ? ICN::SURDRBKE : ICN::SURDRBKG, 0);

    Rect pos_rt;
    pos_rt.x = (display.w() - dialog.w() + 16) / 2;
    pos_rt.y = (display.h() - dialog.h() + 16) / 2;
    pos_rt.w = dialog.w();
    pos_rt.h = dialog.h();

    SpriteBack back(pos_rt);

    dialog.Blit(pos_rt.x, pos_rt.y);

    const int icn = conf.ExtGameEvilInterface() ? ICN::SURRENDE : ICN::SURRENDR;

    Button btnAccept(pos_rt.x + 90, pos_rt.y + 150, icn, 0, 1);
    Button btnDecline(pos_rt.x + 295, pos_rt.y + 150, icn, 2, 3);
    Button btnMarket(pos_rt.x + (pos_rt.w - 16) / 2, pos_rt.y + 145, (conf.ExtGameEvilInterface() ? ICN::ADVEBTNS : ICN::ADVBTNS), 4, 5);
    const Kingdom & kingdom = world.GetKingdom(hero.GetColor());

    if(! kingdom.AllowPayment(payment_t(Resource::GOLD, cost)))
    {
	btnAccept.Press();
    	btnAccept.SetDisable(true);
    }

    if(kingdom.GetCountMarketplace())
    {
	if(kingdom.AllowPayment(payment_t(Resource::GOLD, cost)))
    	    btnMarket.SetDisable(true);
	else
	{
    	    std::string msg = _("Not enough gold (%{gold})");
    	    StringReplace(msg, "%{gold}", cost - kingdom.GetFunds().Get(Resource::GOLD));
	    Text text(msg, Font::SMALL);
	    text.Blit(btnMarket.x + (btnMarket.w - text.w()) / 2, btnMarket.y - 15);
    	    btnMarket.Draw();
	}
    }

    btnAccept.Draw();
    btnDecline.Draw();

    const Sprite & window = AGG::GetICN(icn, 4);
    window.Blit(pos_rt.x + 54, pos_rt.y + 30);
    hero.PortraitRedraw(pos_rt.x + 58, pos_rt.y + 38, PORT_BIG, display);

    std::string str = _("%{name} states:");
    StringReplace(str, "%{name}", hero.GetName());
    Text text(str, Font::BIG);
    text.Blit(pos_rt.x + 320 - text.w() / 2, pos_rt.y + 30);

    str = _("I will accept your surrender and grant you and your troops safe passage for the price of %{price} gold.");
    StringReplace(str, "%{price}", cost);

    TextBox box(str, Font::BIG, 275);
    box.Blit(pos_rt.x + 175, pos_rt.y + 50);
    bool result = false;

    cursor.Show();
    display.Flip();

    while(le.HandleEvents() && !result)
    {
	if(btnAccept.isEnable())
	le.MousePressLeft(btnAccept) ? btnAccept.PressDraw() : btnAccept.ReleaseDraw();
	le.MousePressLeft(btnDecline) ? btnDecline.PressDraw() : btnDecline.ReleaseDraw();

	if(btnMarket.isEnable())
	le.MousePressLeft(btnMarket) ? btnMarket.PressDraw() : btnMarket.ReleaseDraw();

	if(btnAccept.isEnable() && le.MouseClickLeft(btnAccept)) result = true;

        if(btnMarket.isEnable() && le.MouseClickLeft(btnMarket))
        {
            Dialog::Marketplace(false);

            if(kingdom.AllowPayment(payment_t(Resource::GOLD, cost)))
	    {
		btnAccept.Release();
    		btnAccept.SetDisable(false);
	    }
        }

        // exit
	if(Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT) || le.MouseClickLeft(btnDecline)) break;
    }

    cursor.Hide();
    back.Restore();
    cursor.Show();
    display.Flip();

    return result;
}
