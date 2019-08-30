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

#include <algorithm>
#include <vector>
#include "agg.h"
#include "cursor.h"
#include "settings.h"
#include "text.h"
#include "button.h"
#include "castle.h"
#include "dialog.h"
#include "kingdom.h"
#include "heroes.h"
#include "world.h"
#include "race.h"
#include "game.h"
#include "army_bar.h"
#include "buildinginfo.h"
#include "profit.h"
#include "pocketpc.h"

void RedrawTownSprite(const Rect &, const Castle &);
void RedrawBackground(const Rect &, const Castle &);
void RedrawResourceBar(const Point &, const Funds &);
void RedrawIcons(const Castle & castle, const CastleHeroes & hero, const Point & pt);

enum screen_t { SCREENOUT, SCREENOUT_PREV, SCREENOUT_NEXT, SCREEN1, SCREEN2, SCREEN3, SCREEN4, SCREEN5, SCREEN6 };

screen_t CastleOpenDialog1(Castle &, bool);
screen_t CastleOpenDialog2(Castle &, bool);
screen_t CastleOpenDialog3(Castle &, bool);
screen_t CastleOpenDialog4(Castle &, bool);
screen_t CastleOpenDialog5(Castle &, bool);
screen_t CastleOpenDialog6(Castle &, bool);

class ScreenSwitch
{
public:
    ScreenSwitch(const Castle &, const Rect &, bool);

    void Redraw(void);
    bool QueueEventProcessing(void);

    const Castle & castle;
    const Rect rtScreen1;
    const Rect rtScreen2;
    const Rect rtScreen3;
    const Rect rtScreen4;
    const Rect rtScreen5;
    const Rect rtScreen6;
    const bool readonly;
    screen_t result;
};

ScreenSwitch::ScreenSwitch(const Castle & cstl, const Rect & rt, bool ronly) :
    castle(cstl),
    rtScreen1(rt.x + rt.w - 27, rt.y + 32, 25, 25),
    rtScreen2(rt.x + rt.w - 27, rt.y + 58, 25, 25),
    rtScreen3(rt.x + rt.w - 27, rt.y + 84, 25, 25),
    rtScreen4(rt.x + rt.w - 27, rt.y + 110, 25, 25),
    rtScreen5(rt.x + rt.w - 27, rt.y + 136, 25, 25),
    rtScreen6(rt.x + rt.w - 27, rt.y + 162, 25, 25),
    readonly(ronly),
    result(SCREENOUT)
{
}

void ScreenSwitch::Redraw(void)
{
    AGG::GetICN(ICN::REQUESTS, 20).Blit(rtScreen1.x, rtScreen1.y);

    if(!readonly)
    {
	if(castle.isBuild(BUILD_CASTLE))
	{
	    AGG::GetICN(ICN::REQUESTS, 21).Blit(rtScreen2.x, rtScreen2.y);
	    AGG::GetICN(ICN::REQUESTS, 22).Blit(rtScreen3.x, rtScreen3.y);
	    AGG::GetICN(ICN::REQUESTS, 23).Blit(rtScreen4.x, rtScreen4.y);
	}

	if(castle.GetLevelMageGuild())
	    AGG::GetICN(ICN::REQUESTS, 24).Blit(rtScreen5.x, rtScreen5.y);

	AGG::GetICN(ICN::REQUESTS, 25).Blit(rtScreen6.x, rtScreen6.y);
    }
}

bool ScreenSwitch::QueueEventProcessing(void)
{
    LocalEvent & le = LocalEvent::Get();
    result = SCREENOUT;

    if(le.MouseClickLeft(rtScreen1)) result = SCREEN1;
    else
    if(castle.isBuild(BUILD_CASTLE) && le.MouseClickLeft(rtScreen2)) result = SCREEN2;
    else
    if(castle.isBuild(BUILD_CASTLE) && le.MouseClickLeft(rtScreen3)) result = SCREEN3;
    else
    if(castle.isBuild(BUILD_CASTLE) && le.MouseClickLeft(rtScreen4)) result = SCREEN4;
    else
    if(castle.GetLevelMageGuild() && le.MouseClickLeft(rtScreen5)) result = SCREEN5;
    else
    if(le.MouseClickLeft(rtScreen6)) result = SCREEN6;

    return result != SCREENOUT;
}

int PocketPC::CastleOpenDialog(Castle & castle, bool readonly)
{
    AGG::PlayMusic(MUS::FromRace(castle.GetRace()));

    screen_t screen = CastleOpenDialog1(castle, readonly);
    while(SCREENOUT != screen)
	switch(screen)
	{
	    case SCREEN1: screen = CastleOpenDialog1(castle, readonly); break;
	    case SCREEN2: screen = CastleOpenDialog2(castle, readonly); break;
	    case SCREEN3: screen = CastleOpenDialog3(castle, readonly); break;
	    case SCREEN4: screen = CastleOpenDialog4(castle, readonly); break;
	    case SCREEN5: screen = CastleOpenDialog5(castle, readonly); break;
	    case SCREEN6: screen = CastleOpenDialog6(castle, readonly); break;
	    case SCREENOUT_PREV: return Dialog::PREV;
	    case SCREENOUT_NEXT: return Dialog::NEXT;
	    default: break;
	}
    return Dialog::CANCEL;
}

screen_t CastleOpenDialog1(Castle & castle, bool readonly)
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();
    const Settings & conf = Settings::Get();

    CastleHeroes heroes = world.GetHeroes(castle);

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder frameborder(Size(320, 224));
    const Rect & dst_rt = frameborder.GetArea();

    RedrawBackground(dst_rt, castle);

    Text text;
    text.Set(castle.GetName(), Font::YELLOW_SMALL);
    text.Blit(dst_rt.x + (dst_rt.w - text.w()) / 2, dst_rt.y + 3);

    // town icon
    const Sprite & slock = AGG::GetICN(ICN::LOCATORS, 23);
    const Rect rectTown(dst_rt.x, dst_rt.y + 2, slock.w(), slock.h());
    RedrawTownSprite(rectTown, castle);

    // dwelling bar
    DwellingsBar dwellingsBar(castle, Size(43, 43), RGBA(0, 0x2c, 0));
    dwellingsBar.SetPos(dst_rt.x + 2, dst_rt.y + 34);
    dwellingsBar.SetColRows(6, 1);
    dwellingsBar.SetHSpace(2);
    dwellingsBar.Redraw();

    RedrawIcons(castle, heroes, dst_rt);

    const Rect rectSign1(dst_rt.x + 3, dst_rt.y + 80, 41, 41);
    const Rect rectSign2(dst_rt.x + 3, dst_rt.y + 133, 41, 41);

    // castle army bar
    ArmyBar selectArmy1((heroes.Guard() ? & heroes.Guard()->GetArmy() : & castle.GetArmy()), true, readonly);
    selectArmy1.SetColRows(5, 1);
    selectArmy1.SetPos(dst_rt.x + 47, dst_rt.y + 79);
    selectArmy1.SetHSpace(2);
    selectArmy1.Redraw();

    ArmyBar selectArmy2(NULL, true, readonly);
    selectArmy2.SetColRows(5, 1);
    selectArmy2.SetPos(dst_rt.x + 47, dst_rt.y + 124);
    selectArmy2.SetHSpace(2);

    if(heroes.Guest())
    {
        heroes.Guest()->MovePointsScaleFixed();
	selectArmy2.SetArmy(& heroes.Guest()->GetArmy());
	selectArmy2.Redraw();
    }

    const Kingdom & kingdom = castle.GetKingdom();

    // resource bar
    RedrawResourceBar(Point(dst_rt.x + 4, dst_rt.y + 176), kingdom.GetFunds());

    // button swap
    SwapButton buttonSwap(dst_rt.x + 2, dst_rt.y + 113);
    MeetingButton buttonMeeting(dst_rt.x + 26, dst_rt.y + 110);

    if(heroes.Guest() && heroes.Guard() && !readonly)
    {
        buttonSwap.Draw();
        buttonMeeting.Draw();
    }

    const Rect rectExit(dst_rt.x + dst_rt.w - 26, dst_rt.y + 7, 25, 25);
    AGG::GetICN(ICN::TOWNWIND, 12).Blit(rectExit.x, rectExit.y);

    ScreenSwitch screenSwitch(castle, dst_rt, readonly);
    screenSwitch.Redraw();

    // update extra description
    std::string description_castle = castle.GetDescriptionBuilding(BUILD_CASTLE, castle.GetRace());
    {
        payment_t profit = ProfitConditions::FromBuilding(BUILD_CASTLE, castle.GetRace());
        StringReplace(description_castle, "%{count}", profit.gold);
    }

    Button buttonPrev(dst_rt.x + 64, dst_rt.y + 5, ICN::TRADPOST, 3, 4);
    Button buttonNext(dst_rt.x + 245, dst_rt.y + 5, ICN::TRADPOST, 5, 6);
    if(2 > kingdom.GetCastles().size())
    {
	buttonNext.Press();
        buttonPrev.Press();
	buttonNext.SetDisable(true);
        buttonPrev.SetDisable(true);
    }
    buttonNext.Draw();
    buttonPrev.Draw();

    cursor.Show();
    display.Flip();

    while(le.HandleEvents())
    {
        le.MousePressLeft(buttonNext) ? buttonNext.PressDraw() : buttonNext.ReleaseDraw();
        le.MousePressLeft(buttonPrev) ? buttonPrev.PressDraw() : buttonPrev.ReleaseDraw();

	if(!readonly && screenSwitch.QueueEventProcessing() && SCREEN1 != screenSwitch.result)
	    return screenSwitch.result;
        else
        // exit
        if(le.MouseClickLeft(rectExit) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) break;
	else
        if(!readonly && buttonNext.isEnable() && le.MouseClickLeft(buttonNext)) return SCREENOUT_NEXT;
        else
        if(!readonly && buttonPrev.isEnable() && le.MouseClickLeft(buttonPrev)) return SCREENOUT_PREV;
    	else
	if(!readonly && le.MouseClickLeft(rectTown))
	{
	    if(castle.isBuild(BUILD_CASTLE))
		Dialog::Message(castle.GetStringBuilding(BUILD_CASTLE), description_castle, Font::BIG, Dialog::OK);
	    else
	    if(!castle.Modes(Castle::ALLOWCASTLE))
        	Dialog::Message(_("Town"), _("This town may not be upgraded to a castle."), Font::BIG, Dialog::OK);
    	    else
            if(Dialog::OK == castle.DialogBuyCastle(true))
	    {
		// play sound
    		AGG::PlaySound(M82::BUILDTWN);
                castle.BuyBuilding(BUILD_CASTLE);
                cursor.Hide();
                RedrawTownSprite(rectTown, castle);
                cursor.Show();
                display.Flip();
            }
	}
	else
	if(!readonly && le.MouseCursor(dwellingsBar.GetArea()) && dwellingsBar.QueueEventProcessing())
	{
	    cursor.Hide();
	    dwellingsBar.Redraw();
	    selectArmy1.Redraw();
    	    if(selectArmy2.isValid()) selectArmy2.Redraw();
	    RedrawResourceBar(Point(dst_rt.x + 4, dst_rt.y + 176), kingdom.GetFunds());
	    cursor.Show();
	    display.Flip();
	}

	// troops event
        if(heroes.Guest() && selectArmy2.isValid())
        {
	    if((le.MouseCursor(selectArmy1.GetArea()) &&
        	selectArmy1.QueueEventProcessing(selectArmy2)) ||
		(le.MouseCursor(selectArmy2.GetArea()) &&
		selectArmy2.QueueEventProcessing(selectArmy1)))
    	    {
		cursor.Hide();
		RedrawResourceBar(Point(dst_rt.x + 4, dst_rt.y + 176), kingdom.GetFunds());

        	selectArmy1.Redraw();
        	if(selectArmy2.isValid()) selectArmy2.Redraw();
		cursor.Show();
		display.Flip();
	    }
	}
        else
        {
    	    if(le.MouseCursor(selectArmy1.GetArea()) &&
    		selectArmy1.QueueEventProcessing())
	    {
		cursor.Hide();
		RedrawResourceBar(Point(dst_rt.x + 4, dst_rt.y + 176), kingdom.GetFunds());

        	selectArmy1.Redraw();
		cursor.Show();
		display.Flip();
	    }
	}

        if(conf.ExtCastleAllowGuardians() && !readonly)
        {
            Army* army1 = NULL;
            Army* army2 = NULL;

            // swap guest <-> guardian
            if(heroes.Guest() && heroes.Guard())
            {
                if(le.MouseClickLeft(buttonSwap))
                {
                    castle.SwapCastleHeroes(heroes);
                    army1 = &heroes.Guard()->GetArmy();
                    army2 = &heroes.Guest()->GetArmy();
                }
                else
                if(le.MouseClickLeft(buttonMeeting))
                {
                    heroes.Guest()->MeetingDialog(*heroes.Guard());
                }
            }
            else
    	    // move hero to guardian
    	    if(heroes.Guest() && !heroes.Guard() && le.MouseClickLeft(rectSign1))
    	    {
        	if(! heroes.Guest()->GetArmy().CanJoinTroops(castle.GetArmy()))
        	{
            	    // FIXME: correct message
            	    Dialog::Message("Join Error", "Army is full", Font::BIG, Dialog::OK);
        	}
        	else
        	{
            	    castle.SwapCastleHeroes(heroes);
            	    army1 = &heroes.Guard()->GetArmy();
        	}
    	    }
    	    else
    	    // move guardian to hero
    	    if(!heroes.Guest() && heroes.Guard() && le.MouseClickLeft(rectSign2))
    	    {
        	castle.SwapCastleHeroes(heroes);
        	army2 = &heroes.Guest()->GetArmy();
    	    }

    	    if(army1 || army2)
    	    {
        	cursor.Hide();
        	if(selectArmy1.isSelected()) selectArmy1.ResetSelected();
        	if(selectArmy2.isValid() && selectArmy2.isSelected()) selectArmy2.ResetSelected();

        	if(army1 && army2)
        	{
            	    selectArmy1.SetArmy(army1);
            	    selectArmy2.SetArmy(army2);
        	}
        	else
        	if(army1)
        	{
            	    selectArmy1.SetArmy(army1);
            	    selectArmy2.SetArmy(NULL);
        	}
        	else
        	if(army2)
        	{
            	    selectArmy1.SetArmy(&castle.GetArmy());
            	    selectArmy2.SetArmy(army2);
        	}

            	RedrawIcons(castle, heroes, dst_rt);

        	if(heroes.Guest() && heroes.Guard() && !readonly)
        	{
            	    buttonSwap.Draw();
            	    buttonMeeting.Draw();
        	}

            	selectArmy1.Redraw();
            	if(selectArmy2.isValid()) selectArmy2.Redraw();

                cursor.Show();
                display.Flip();
    	    }
        }

	if(!readonly)
	{
	    bool redraw = false;

	    if(heroes.Guard() && le.MouseClickLeft(rectSign1))
	    {
		heroes.Guard()->OpenDialog(false, false);
        	if(selectArmy1.isSelected()) selectArmy1.ResetSelected();
        	if(selectArmy2.isValid() && selectArmy2.isSelected()) selectArmy2.ResetSelected();
		redraw = true;
	    }
	    else
	    if(heroes.Guest() && le.MouseClickLeft(rectSign2))
	    {
		heroes.Guest()->OpenDialog(false, false);
        	if(selectArmy1.isSelected()) selectArmy1.ResetSelected();
        	if(selectArmy2.isValid() && selectArmy2.isSelected()) selectArmy2.ResetSelected();
		redraw = true;
	    }

	    if(redraw)
	    {
                cursor.Hide();
        	selectArmy1.Redraw();
        	if(selectArmy2.isValid()) selectArmy2.Redraw();
                cursor.Show();
		display.Flip();
	    }
	}
    }

    return SCREENOUT;
}

screen_t CastleOpenDialog2(Castle & castle, bool readonly)
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder frameborder(Size(320, 224));
    const Rect & dst_rt = frameborder.GetArea();

    const Rect rectExit(dst_rt.x + dst_rt.w - 26, dst_rt.y + 7, 25, 25);
    AGG::GetICN(ICN::TOWNWIND, 12).Blit(rectExit.x, rectExit.y);

    ScreenSwitch screenSwitch(castle, dst_rt, readonly);
    screenSwitch.Redraw();

    BuildingInfo dwelling1(castle, DWELLING_MONSTER1);
    dwelling1.SetPos(dst_rt.x + 2, dst_rt.y + 2);
    dwelling1.Redraw();

    BuildingInfo dwelling2(castle, DWELLING_MONSTER2);
    dwelling2.SetPos(dst_rt.x + 141, dst_rt.y + 2);
    dwelling2.Redraw();

    BuildingInfo dwelling3(castle, DWELLING_MONSTER3);
    dwelling3.SetPos(dst_rt.x + 2, dst_rt.y + 76);
    dwelling3.Redraw();

    BuildingInfo dwelling4(castle, DWELLING_MONSTER4);
    dwelling4.SetPos(dst_rt.x + 141, dst_rt.y + 76);
    dwelling4.Redraw();

    BuildingInfo dwelling5(castle, DWELLING_MONSTER5);
    dwelling5.SetPos(dst_rt.x + 2, dst_rt.y + 150);
    dwelling5.Redraw();

    BuildingInfo dwelling6(castle, DWELLING_MONSTER6);
    dwelling6.SetPos(dst_rt.x + 141, dst_rt.y + 150);
    dwelling6.Redraw();

    cursor.Show();
    display.Flip();

    while(le.HandleEvents())
    {
	if(!readonly && screenSwitch.QueueEventProcessing() && SCREEN2 != screenSwitch.result)
	    return screenSwitch.result;
        else
        // exit
        if(le.MouseClickLeft(rectExit) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) break;

	if(le.MouseCursor(dwelling1.GetArea()) && dwelling1.QueueEventProcessing()) { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(dwelling1()); return SCREEN1; }
	else
	if(le.MouseCursor(dwelling2.GetArea()) && dwelling2.QueueEventProcessing()) { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(dwelling2()); return SCREEN1; }
	else
	if(le.MouseCursor(dwelling3.GetArea()) && dwelling3.QueueEventProcessing()) { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(dwelling3()); return SCREEN1; }
	else
	if(le.MouseCursor(dwelling4.GetArea()) && dwelling4.QueueEventProcessing()) { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(dwelling4()); return SCREEN1; }
	else
	if(le.MouseCursor(dwelling5.GetArea()) && dwelling5.QueueEventProcessing()) { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(dwelling5()); return SCREEN1; }
	else
	if(le.MouseCursor(dwelling6.GetArea()) && dwelling6.QueueEventProcessing()) { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(dwelling6()); return SCREEN1; }
    }
    return SCREENOUT;
}

screen_t CastleOpenDialog3(Castle & castle, bool readonly)
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder frameborder(Size(320, 224));
    const Rect & dst_rt = frameborder.GetArea();

    const Rect rectExit(dst_rt.x + dst_rt.w - 26, dst_rt.y + 7, 25, 25);
    AGG::GetICN(ICN::TOWNWIND, 12).Blit(rectExit.x, rectExit.y);

    ScreenSwitch screenSwitch(castle, dst_rt, readonly);
    screenSwitch.Redraw();

    building_t level = BUILD_NOTHING;
    switch(castle.GetLevelMageGuild())
    {
        case 0: level = BUILD_MAGEGUILD1; break;
        case 1: level = BUILD_MAGEGUILD2; break;
        case 2: level = BUILD_MAGEGUILD3; break;
        case 3: level = BUILD_MAGEGUILD4; break;
        default:level = BUILD_MAGEGUILD5; break;
    }

    BuildingInfo building1(castle, level);
    building1.SetPos(dst_rt.x + 2, dst_rt.y + 2);
    building1.Redraw();

    BuildingInfo building2(castle, (castle.GetRace() == Race::NECR ? BUILD_SHRINE : BUILD_TAVERN));
    building2.SetPos(dst_rt.x + 141, dst_rt.y + 2);
    building2.Redraw();

    BuildingInfo building3(castle, BUILD_THIEVESGUILD);
    building3.SetPos(dst_rt.x + 2, dst_rt.y + 76);
    building3.Redraw();

    BuildingInfo building4(castle, BUILD_SHIPYARD);
    building4.SetPos(dst_rt.x + 141, dst_rt.y + 76);
    building4.Redraw();

    BuildingInfo building5(castle, BUILD_STATUE);
    building5.SetPos(dst_rt.x + 2, dst_rt.y + 150);
    building5.Redraw();

    BuildingInfo building6(castle, BUILD_MARKETPLACE);
    building6.SetPos(dst_rt.x + 141, dst_rt.y + 150);
    building6.Redraw();

    cursor.Show();
    display.Flip();

    while(le.HandleEvents())
    {
	if(!readonly && screenSwitch.QueueEventProcessing() && SCREEN3 != screenSwitch.result)
	    return screenSwitch.result;
        else
        // exit
        if(le.MouseClickLeft(rectExit) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) break;

	if(le.MouseCursor(building1.GetArea()) && building1.QueueEventProcessing())
	    { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(building1()); return SCREEN1; }
	else
	if(le.MouseCursor(building2.GetArea()) && building2.QueueEventProcessing())
	    { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(building2()); return SCREEN1; }
	else
	if(le.MouseCursor(building3.GetArea()) && building3.QueueEventProcessing())
	    { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(building3()); return SCREEN1; }
	else
	if(le.MouseCursor(building4.GetArea()) && building4.QueueEventProcessing())
	    { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(building4()); return SCREEN1; }
	else
	if(le.MouseCursor(building5.GetArea()) && building5.QueueEventProcessing())
	    { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(building5()); return SCREEN1; }
	else
	if(le.MouseCursor(building6.GetArea()) && building6.QueueEventProcessing())
	    { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(building6()); return SCREEN1; }
    }
    return SCREENOUT;
}

screen_t CastleOpenDialog4(Castle & castle, bool readonly)
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder frameborder(Size(320, 224));
    const Rect & dst_rt = frameborder.GetArea();

    const Rect rectExit(dst_rt.x + dst_rt.w - 26, dst_rt.y + 7, 25, 25);
    AGG::GetICN(ICN::TOWNWIND, 12).Blit(rectExit.x, rectExit.y);

    ScreenSwitch screenSwitch(castle, dst_rt, readonly);
    screenSwitch.Redraw();

    BuildingInfo building1(castle, BUILD_WELL);
    building1.SetPos(dst_rt.x + 2, dst_rt.y + 2);
    building1.Redraw();

    BuildingInfo building2(castle, BUILD_WEL2);
    building2.SetPos(dst_rt.x + 141, dst_rt.y + 2);
    building2.Redraw();

    BuildingInfo building3(castle, BUILD_SPEC);
    building3.SetPos(dst_rt.x + 2, dst_rt.y + 76);
    building3.Redraw();

    BuildingInfo building4(castle, BUILD_LEFTTURRET);
    building4.SetPos(dst_rt.x + 141, dst_rt.y + 76);
    building4.Redraw();

    BuildingInfo building5(castle, BUILD_RIGHTTURRET);
    building5.SetPos(dst_rt.x + 2, dst_rt.y + 150);
    building5.Redraw();

    BuildingInfo building6(castle, BUILD_MOAT);
    building6.SetPos(dst_rt.x + 141, dst_rt.y + 150);
    building6.Redraw();

    cursor.Show();
    display.Flip();

    while(le.HandleEvents())
    {
	if(!readonly && screenSwitch.QueueEventProcessing() && SCREEN4 != screenSwitch.result)
	    return screenSwitch.result;
        else
        // exit
        if(le.MouseClickLeft(rectExit) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) break;

	if(le.MouseCursor(building1.GetArea()) && building1.QueueEventProcessing()) { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(building1()); return SCREEN1; }
	else
	if(le.MouseCursor(building2.GetArea()) && building2.QueueEventProcessing()) { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(building2()); return SCREEN1; }
	else
	if(le.MouseCursor(building3.GetArea()) && building3.QueueEventProcessing()) { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(building3()); return SCREEN1; }
	else
	if(le.MouseCursor(building4.GetArea()) && building4.QueueEventProcessing()) { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(building4()); return SCREEN1; }
	else
	if(le.MouseCursor(building5.GetArea()) && building5.QueueEventProcessing()) { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(building5()); return SCREEN1; }
	else
	if(le.MouseCursor(building6.GetArea()) && building6.QueueEventProcessing()) { AGG::PlaySound(M82::BUILDTWN); castle.BuyBuilding(building6()); return SCREEN1; }
    }
    return SCREENOUT;
}

screen_t CastleOpenDialog5(Castle & castle, bool readonly)
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder frameborder(Size(320, 224));
    const Rect & dst_rt = frameborder.GetArea();

    RowSpells spells1(Point(dst_rt.x + 38, dst_rt.y + 220 - 44), castle, 1);
    RowSpells spells2(Point(dst_rt.x + 38, dst_rt.y + 220 - 44 * 2), castle, 2);
    RowSpells spells3(Point(dst_rt.x + 38, dst_rt.y + 220 - 44 * 3), castle, 3);
    RowSpells spells4(Point(dst_rt.x + 38, dst_rt.y + 220 - 44 * 4), castle, 4);
    RowSpells spells5(Point(dst_rt.x + 38, dst_rt.y + 220 - 44 * 5), castle, 5);

    spells1.Redraw();
    spells2.Redraw();
    spells3.Redraw();
    spells4.Redraw();
    spells5.Redraw();

    // magic book sprite
    const Heroes* hero = castle.GetHeroes().GuestFirst();
    bool need_buy_book = hero && !hero->HaveSpellBook() && castle.GetLevelMageGuild();
    const Rect book_pos(dst_rt.x + 250, dst_rt.y + 5, 32, 32);
    if(need_buy_book)
    {
	AGG::GetICN(ICN::ARTFX, 81).Blit(book_pos);
	Text text(_("buy"), Font::SMALL);
	text.Blit(book_pos.x + (book_pos.w - text.w()) / 2, book_pos.y + book_pos.h - 12);
    }

    // buttons
    const Rect rectExit(dst_rt.x + dst_rt.w - 26, dst_rt.y + 7, 25, 25);
    AGG::GetICN(ICN::TOWNWIND, 12).Blit(rectExit.x, rectExit.y);

    ScreenSwitch screenSwitch(castle, dst_rt, readonly);
    screenSwitch.Redraw();

    cursor.Show();
    display.Flip();

    while(le.HandleEvents())
    {
	if(!readonly && screenSwitch.QueueEventProcessing() && SCREEN5 != screenSwitch.result)
	    return screenSwitch.result;
        else
        // exit
        if(le.MouseClickLeft(rectExit) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) break;
	else
	if(need_buy_book && le.MouseClickLeft(book_pos)) { const_cast<Heroes*>(hero)->BuySpellBook(&castle); return SCREEN1; }

        spells1.QueueEventProcessing();
        spells2.QueueEventProcessing();
        spells3.QueueEventProcessing();
        spells4.QueueEventProcessing();
        spells5.QueueEventProcessing();
    }

    return SCREENOUT;
}

screen_t CastleOpenDialog6(Castle & castle, bool readonly)
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder frameborder(Size(320, 224));
    const Rect & dst_rt = frameborder.GetArea();

    // tavern
    Point dst_pt;
    if(castle.isBuild(BUILD_TAVERN))
    {
	Text text;
	text.Set(castle.GetStringBuilding(BUILD_TAVERN), Font::SMALL);
	text.Blit(dst_rt.x + (dst_rt.w - text.w()) / 2, dst_rt.y + 3);

	TextBox box1(_("A generous tip for the barkeep yields the following rumor:"), Font::SMALL, 186);
	TextBox box2(world.GetRumors(), Font::SMALL, 186);

	box1.Blit(dst_rt.x + 67, dst_rt.y + 120);
	box2.Blit(dst_rt.x + 67, dst_rt.y + 130 + box1.h());

	const Sprite & s1 = AGG::GetICN(ICN::TAVWIN, 0);
	dst_pt = Point(dst_rt.x + (dst_rt.w - s1.w()) / 2, dst_rt.y + 18);
	s1.Blit(dst_pt);

	const Sprite & s20 = AGG::GetICN(ICN::TAVWIN, 1);
	s20.Blit(dst_pt.x + 3, dst_pt.y + 3);
    }

    Rect rectRecruit1, rectRecruit2, rectCaptain;
    Heroes* hero1 = NULL;
    Heroes* hero2 = NULL;

    if(castle.isBuild(BUILD_CASTLE))
    {
	// hero
	const Sprite & crest = AGG::GetICN(ICN::BRCREST, 6);
	rectRecruit1 = Rect(dst_rt.x + 4, dst_rt.y + 18, crest.w(), crest.h());
	rectRecruit2 = Rect(dst_rt.x + 4, dst_rt.y + 77, crest.w(), crest.h());
	rectCaptain = Rect(dst_rt.x + 4, dst_rt.y + 136, crest.w(), crest.h());

	hero1 = castle.GetKingdom().GetRecruits().GetHero1();
	hero2 = castle.GetKingdom().GetRecruits().GetHero2();

	crest.Blit(rectRecruit1);
	if(hero1) hero1->PortraitRedraw(rectRecruit1.x + 4, rectRecruit1.y + 4, PORT_MEDIUM, display);

	crest.Blit(rectRecruit2);
        if(hero2) hero2->PortraitRedraw(rectRecruit2.x + 4, rectRecruit2.y + 4, PORT_MEDIUM, display);

	// captain
	crest.Blit(rectCaptain);
	Surface port = castle.GetCaptain().GetPortrait(PORT_BIG);
	if(port.isValid())
    	    port.Blit(Rect((port.w() - 50) / 2, 15, 50, 47), rectCaptain.x + 4, rectCaptain.y + 4, display);
    }

    // shipyard, shieves guild, marketplace
    const Sprite & spriteX = AGG::GetICN(ICN::CASLXTRA, 1);
    const Rect rt1(dst_rt.x + 180, dst_rt.y + 180, spriteX.w(), spriteX.h());
    if(castle.isBuild(BUILD_MARKETPLACE))
    {
	Text txt(castle.GetStringBuilding(BUILD_MARKETPLACE), Font::SMALL);
	spriteX.Blit(rt1);
	txt.Blit(rt1.x + (rt1.w - txt.w()) / 2, rt1.y + 1);
    }
    const Rect rt2(dst_rt.x + 180, dst_rt.y + 195, spriteX.w(), spriteX.h());
    if(castle.isBuild(BUILD_THIEVESGUILD))
    {
	Text txt(castle.GetStringBuilding(BUILD_THIEVESGUILD), Font::SMALL);
	spriteX.Blit(rt2);
	txt.Blit(rt2.x + (rt2.w - txt.w()) / 2, rt2.y + 1);
    }
    const Rect rt3(dst_rt.x + 180, dst_rt.y + 210, spriteX.w(), spriteX.h());
    if(castle.isBuild(BUILD_SHIPYARD))
    {
	Text txt(castle.GetStringBuilding(BUILD_SHIPYARD), Font::SMALL);
	spriteX.Blit(rt3);
	txt.Blit(rt3.x + (rt3.w - txt.w()) / 2, rt3.y + 1);
    }

    // buttons
    const Rect rectExit(dst_rt.x + dst_rt.w - 26, dst_rt.y + 7, 25, 25);
    AGG::GetICN(ICN::TOWNWIND, 12).Blit(rectExit.x, rectExit.y);

    ScreenSwitch screenSwitch(castle, dst_rt, readonly);
    screenSwitch.Redraw();

    u32 frame = 0;
    
    cursor.Show();
    display.Flip();

    while(le.HandleEvents())
    {
	if(!readonly && screenSwitch.QueueEventProcessing() && SCREEN6 != screenSwitch.result)
	    return screenSwitch.result;
        else
        // exit
        if(le.MouseClickLeft(rectExit) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) break;
	else
	if(hero1 && le.MouseClickLeft(rectRecruit1) &&
	    Dialog::OK == castle.DialogBuyHero(hero1))
	{
    	    castle.RecruitHero(hero1);
	    return SCREEN1;
        }
	else
	if(hero2 && le.MouseClickLeft(rectRecruit2) &&
	    Dialog::OK == castle.DialogBuyHero(hero2))
	{
    	    castle.RecruitHero(hero2);
	    return SCREEN1;
        }
	else
	if(le.MouseClickLeft(rectCaptain))
	{
	    BuildingInfo b(castle, BUILD_CAPTAIN);
	    if(castle.isBuild(BUILD_CAPTAIN))
		Dialog::Message(b.GetName(), b.GetDescription(), Font::SMALL, Dialog::OK);
	    else
	    if(b.DialogBuyBuilding(true))
	    {
		AGG::PlaySound(M82::BUILDTWN);
		castle.BuyBuilding(b());
		return SCREEN1;
	    }
	}
	else
	// show marketplace
    	if(castle.isBuild(BUILD_MARKETPLACE) && le.MouseClickLeft(rt1)) Dialog::Marketplace();
	else
        // buy boat
	if(castle.isBuild(BUILD_SHIPYARD) && le.MouseClickLeft(rt3) &&
	    Dialog::OK == Dialog::BuyBoat(castle.AllowBuyBoat())) castle.BuyBoat();
	else
	// show thieves guild
	if(castle.isBuild(BUILD_THIEVESGUILD) && le.MouseClickLeft(rt2))
	{ PocketPC::ThievesGuild(false); return SCREEN1; }


        // animation
        if(castle.isBuild(BUILD_TAVERN) && Game::AnimateInfrequentDelay(Game::CASTLE_TAVERN_DELAY))
        {
            cursor.Hide();
	    const Sprite & s20 = AGG::GetICN(ICN::TAVWIN, 1);
            s20.Blit(dst_pt.x + 3, dst_pt.y + 3);
            if(u32 index = ICN::AnimationFrame(ICN::TAVWIN, 0, frame++))
            {
        	const Sprite & s22 = AGG::GetICN(ICN::TAVWIN, index);
                s22.Blit(dst_pt.x + s22.x() + 3, dst_pt.y + s22.y() + 3);
            }
    	    cursor.Show();
    	    display.Flip();
        }
    }

    return SCREENOUT;
}


void RedrawResourceBar(const Point & dst, const Funds & rs)
{
    AGG::GetICN(ICN::STONEBAK, 0).Blit(Rect(0, 0, 312, 13), dst.x, dst.y + 30);

    const Sprite & ore = AGG::GetICN(ICN::RESOURCE, 2);
    const Sprite & wood = AGG::GetICN(ICN::RESOURCE, 0);
    const Sprite & mercury = AGG::GetICN(ICN::RESOURCE, 1);
    const Sprite & sulfur = AGG::GetICN(ICN::RESOURCE, 3);
    const Sprite & crystal = AGG::GetICN(ICN::RESOURCE, 4);
    const Sprite & gems = AGG::GetICN(ICN::RESOURCE, 5);
    const Sprite & gold = AGG::GetICN(ICN::RESOURCE, 6);

    Text text;

    ore.Blit(dst.x + 22 - ore.w() / 2, dst.y + 34 - ore.h());
    text.Set(GetString(rs.ore), Font::SMALL);
    text.Blit(dst.x + 22 - text.w() / 2, dst.y + 33);

    wood.Blit(dst.x + 68 - wood.w() / 2, dst.y + 34 - wood.h());
    text.Set(GetString(rs.wood), Font::SMALL);
    text.Blit(dst.x + 68 - text.w() / 2, dst.y + 33);

    mercury.Blit(dst.x + 114 - mercury.w() / 2, dst.y + 34 - mercury.h());
    text.Set(GetString(rs.mercury), Font::SMALL);
    text.Blit(dst.x + 114 - text.w() / 2, dst.y + 33);

    sulfur.Blit(dst.x + 160 - sulfur.w() / 2, dst.y + 34 - sulfur.h());
    text.Set(GetString(rs.sulfur), Font::SMALL);
    text.Blit(dst.x + 160 - text.w() / 2, dst.y + 33);

    crystal.Blit(dst.x + 206 - crystal.w() / 2, dst.y + 34 - crystal.h());
    text.Set(GetString(rs.crystal), Font::SMALL);
    text.Blit(dst.x + 206 - text.w() / 2, dst.y + 33);

    gems.Blit(dst.x + 252 - gems.w() / 2, dst.y + 34 - gems.h());
    text.Set(GetString(rs.gems), Font::SMALL);
    text.Blit(dst.x + 252 - text.w() / 2, dst.y + 33);

    gold.Blit(Rect(0, 0, 40, gold.h()), dst.x + 292 - 20, dst.y + 34 - gold.h());
    text.Set(GetString(rs.gold), Font::SMALL);
    text.Blit(dst.x + 292 - text.w() / 2, dst.y + 33);
}

void RedrawBackground(const Rect & rt, const Castle & castle)
{
    switch(castle.GetRace())
    {
	case Race::KNGT: AGG::GetICN(ICN::TOWNBKG0, 0).Blit(Rect(148, 0, rt.w, 123), rt.x, rt.y); break;
	case Race::BARB: AGG::GetICN(ICN::TOWNBKG1, 0).Blit(Rect(142, 0, rt.w, 123), rt.x, rt.y); break;
	case Race::SORC: AGG::GetICN(ICN::TOWNBKG2, 0).Blit(Rect(218, 0, rt.w, 123), rt.x, rt.y); break;
	case Race::WRLK: AGG::GetICN(ICN::TOWNBKG3, 0).Blit(Rect(300, 0, rt.w, 123), rt.x, rt.y); break;
	case Race::WZRD: AGG::GetICN(ICN::TOWNBKG4, 0).Blit(Rect(150, 0, rt.w, 123), rt.x, rt.y); break;
	case Race::NECR: AGG::GetICN(ICN::TOWNBKG5, 0).Blit(Rect(0, 0, rt.w, 123), rt.x, rt.y); break;
	default: break;
    }
}

void RedrawTownSprite(const Rect & rt, const Castle & castle)
{
    const Sprite & slock = AGG::GetICN(ICN::LOCATORS, 23);
    slock.Blit(rt.x, rt.y);
    switch(castle.GetRace())
    {
        case Race::KNGT: AGG::GetICN(ICN::LOCATORS, castle.isCastle() ?  9 : 15).Blit(rt.x + 4, rt.y + 4); break;
        case Race::BARB: AGG::GetICN(ICN::LOCATORS, castle.isCastle() ?  10 : 16).Blit(rt.x + 4, rt.y + 4); break;
        case Race::SORC: AGG::GetICN(ICN::LOCATORS, castle.isCastle() ?  11 : 17).Blit(rt.x + 4, rt.y + 4); break;
        case Race::WRLK: AGG::GetICN(ICN::LOCATORS, castle.isCastle() ?  12 : 18).Blit(rt.x + 4, rt.y + 4); break;
        case Race::WZRD: AGG::GetICN(ICN::LOCATORS, castle.isCastle() ?  13 : 19).Blit(rt.x + 4, rt.y + 4); break;
        case Race::NECR: AGG::GetICN(ICN::LOCATORS, castle.isCastle() ?  14 : 20).Blit(rt.x + 4, rt.y + 4); break;
        default: break;
    }
    if(! castle.AllowBuild()) AGG::GetICN(ICN::LOCATORS, 24).Blit(rt.x + 43, rt.y + 5);
}
