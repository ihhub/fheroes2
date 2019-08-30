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
#include <string>
#include "agg.h"
#include "gamedefs.h"
#include "button.h"
#include "cursor.h"
#include "race.h"
#include "settings.h"
#include "maps_fileinfo.h"
#include "dialog_selectscenario.h"
#include "difficulty.h"
#include "dialog.h"
#include "text.h"
#include "kingdom.h"
#include "splitter.h"
#include "world.h"
#include "players.h"
#include "pocketpc.h"
#include "game.h"

void RedrawScenarioStaticInfo(const Rect &);
void RedrawRatingInfo(TextSprite &);
void RedrawDifficultyInfo(const Point & dst, bool label = true);

int Game::SelectScenario(void)
{
    if(Settings::Get().QVGA()) return PocketPC::SelectScenario();
    return SCENARIOINFO;
}

int Game::ScenarioInfo(void)
{
    Settings & conf = Settings::Get();

    AGG::PlayMusic(MUS::MAINMENU);

    MapsFileInfoList lists;
    if(!PrepareMapsFileInfoList(lists, (conf.GameType(Game::TYPE_MULTI))))
    {
	Dialog::Message(_("Warning"), _("No maps available!"), Font::BIG, Dialog::OK);
        return MAINMENU;
    }

    int result = QUITGAME;
    LocalEvent & le = LocalEvent::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Display & display = Display::Get();

    Point top, pointDifficultyInfo, pointOpponentInfo, pointClassInfo;
    Rect rectPanel;
    Button* buttonSelectMaps = NULL;
    Button* buttonOk = NULL;
    Button* buttonCancel = NULL;

    // vector coord difficulty
    Rects coordDifficulty;
    coordDifficulty.reserve(5);

    const Sprite & ngextra = AGG::GetICN(ICN::NGEXTRA, 62);
    Dialog::FrameBorder* frameborder = NULL;

    // image background
    if(conf.QVGA())
    {
	frameborder = new Dialog::FrameBorder(Size(380, 224));
	rectPanel = frameborder->GetArea();

	pointDifficultyInfo = Point(rectPanel.x + 4, rectPanel.y + 24);
	pointOpponentInfo = Point(rectPanel.x + 4, rectPanel.y + 94);
	pointClassInfo = Point(rectPanel.x + 4, rectPanel.y + 148);

	coordDifficulty.push_back(Rect(rectPanel.x + 1, rectPanel.y + 21,   ngextra.w(), ngextra.h()));
	coordDifficulty.push_back(Rect(rectPanel.x + 78, rectPanel.y + 21,  ngextra.w(), ngextra.h()));
	coordDifficulty.push_back(Rect(rectPanel.x + 154, rectPanel.y + 21, ngextra.w(), ngextra.h()));
	coordDifficulty.push_back(Rect(rectPanel.x + 231, rectPanel.y + 21, ngextra.w(), ngextra.h()));
        coordDifficulty.push_back(Rect(rectPanel.x + 308, rectPanel.y + 21, ngextra.w(), ngextra.h()));

	buttonOk = new Button(rectPanel.x + rectPanel.w / 2 - 160, rectPanel.y + rectPanel.h - 30, ICN::NGEXTRA, 66, 67);
	buttonCancel = new Button(rectPanel.x + rectPanel.w / 2 + 60, rectPanel.y + rectPanel.h - 30, ICN::NGEXTRA, 68, 69);

	Text text;
	text.Set(conf.CurrentFileInfo().name, Font::BIG);
	text.Blit(rectPanel.x + (rectPanel.w - text.w()) / 2, rectPanel.y + 5);
    }
    else
    {
	const Sprite &panel = AGG::GetICN(ICN::NGHSBKG, 0);
	const Sprite &back = AGG::GetICN(ICN::HEROES, 0);
	const Point top((display.w() - back.w()) / 2, (display.h() - back.h()) / 2);

	rectPanel = Rect(top.x + 204, top.y + 32, panel.w(), panel.h());
	pointDifficultyInfo = Point(rectPanel.x + 24, rectPanel.y + 93);
	pointOpponentInfo = Point(rectPanel.x + 24, rectPanel.y + 202);
	pointClassInfo = Point(rectPanel.x + 24, rectPanel.y + 282);

	coordDifficulty.push_back(Rect(rectPanel.x + 21, rectPanel.y + 91,  ngextra.w(), ngextra.h()));
	coordDifficulty.push_back(Rect(rectPanel.x + 98, rectPanel.y + 91,  ngextra.w(), ngextra.h()));
	coordDifficulty.push_back(Rect(rectPanel.x + 174, rectPanel.y + 91, ngextra.w(), ngextra.h()));
	coordDifficulty.push_back(Rect(rectPanel.x + 251, rectPanel.y + 91, ngextra.w(), ngextra.h()));
	coordDifficulty.push_back(Rect(rectPanel.x + 328, rectPanel.y + 91, ngextra.w(), ngextra.h()));

	buttonSelectMaps = new Button(rectPanel.x + 309, rectPanel.y + 45, ICN::NGEXTRA, 64, 65);
	buttonOk = new Button(rectPanel.x + 31, rectPanel.y + 380, ICN::NGEXTRA, 66, 67);
	buttonCancel = new Button(rectPanel.x + 287, rectPanel.y + 380, ICN::NGEXTRA, 68, 69);

	back.Blit(top);
    }

    const bool reset_starting_settings = conf.MapsFile().empty() || ! System::IsFile(conf.MapsFile());
    Players & players = conf.GetPlayers();
    Interface::PlayersInfo playersInfo(true, !conf.QVGA(), !conf.QVGA());

    // set first maps settings
    if(reset_starting_settings)
	conf.SetCurrentFileInfo(lists.front());

    playersInfo.UpdateInfo(players, pointOpponentInfo, pointClassInfo);

    RedrawScenarioStaticInfo(rectPanel);
    RedrawDifficultyInfo(pointDifficultyInfo, !conf.QVGA());

    playersInfo.RedrawInfo();

    TextSprite* rating = conf.QVGA() ? NULL : new TextSprite();
    if(rating)
    {
	rating->SetFont(Font::BIG);
	rating->SetPos(rectPanel.x + 166, rectPanel.y + 383);
	RedrawRatingInfo(*rating);
    }

    SpriteMove levelCursor(ngextra);

    switch(conf.GameDifficulty())
    {
	case Difficulty::EASY:		levelCursor.Move(coordDifficulty[0]); break;
	case Difficulty::NORMAL:	levelCursor.Move(coordDifficulty[1]); break;
	case Difficulty::HARD:		levelCursor.Move(coordDifficulty[2]); break;
	case Difficulty::EXPERT:	levelCursor.Move(coordDifficulty[3]); break;
	case Difficulty::IMPOSSIBLE:	levelCursor.Move(coordDifficulty[4]); break;
    }

    if(buttonSelectMaps) buttonSelectMaps->Draw();
    buttonOk->Draw();
    buttonCancel->Draw();

    cursor.Show();
    display.Flip();

    while(le.HandleEvents())
    {
	// press button
	if(buttonSelectMaps)
	le.MousePressLeft(*buttonSelectMaps) ? buttonSelectMaps->PressDraw() : buttonSelectMaps->ReleaseDraw();
	le.MousePressLeft(*buttonOk) ? buttonOk->PressDraw() : buttonOk->ReleaseDraw();
	le.MousePressLeft(*buttonCancel) ? buttonCancel->PressDraw() : buttonCancel->ReleaseDraw();

	// click select
	if(buttonSelectMaps &&
	  (HotKeyPressEvent(Game::EVENT_BUTTON_SELECT) || le.MouseClickLeft(*buttonSelectMaps)))
	{
	    levelCursor.Hide();
	    const Maps::FileInfo* fi = Dialog::SelectScenario(lists);
	    if(fi)
	    {
		conf.SetCurrentFileInfo(*fi);
		playersInfo.UpdateInfo(players, pointOpponentInfo, pointClassInfo);

		cursor.Hide();
		RedrawScenarioStaticInfo(rectPanel);
		RedrawDifficultyInfo(pointDifficultyInfo, !conf.QVGA());
		playersInfo.RedrawInfo();
		if(rating) RedrawRatingInfo(*rating);
		// default difficulty normal
		levelCursor.Move(coordDifficulty[1]);
    		conf.SetGameDifficulty(Difficulty::NORMAL);
		buttonOk->Draw();
		buttonCancel->Draw();
	    }
	    cursor.Show();
	    display.Flip();
	}
	else
	// click cancel
	if(HotKeyPressEvent(EVENT_DEFAULT_EXIT) || le.MouseClickLeft(*buttonCancel))
	{
	    result = MAINMENU;
	    break;
	}
	else
	// click ok
	if(HotKeyPressEvent(EVENT_DEFAULT_READY) || le.MouseClickLeft(*buttonOk))
	{
	    DEBUG(DBG_GAME, DBG_INFO, "select maps: " << conf.MapsFile() << \
		    ", difficulty: " << Difficulty::String(conf.GameDifficulty()));
	    result = STARTGAME;
	    break;
	}
	else
	if(le.MouseClickLeft(rectPanel))
	{
	    const s32 index = coordDifficulty.GetIndex(le.GetMouseCursor());

	    // select difficulty
	    if(0 <= index)
	    {
		cursor.Hide();
		levelCursor.Move(coordDifficulty[index]);
		conf.SetGameDifficulty(index);
		if(rating) RedrawRatingInfo(*rating);
		cursor.Show();
		display.Flip();
	    }
	    else
	    // playersInfo
	    if(playersInfo.QueueEventProcessing())
	    {
		cursor.Hide();
		RedrawScenarioStaticInfo(rectPanel);
		levelCursor.Redraw();
		RedrawDifficultyInfo(pointDifficultyInfo, !conf.QVGA());

		playersInfo.RedrawInfo();
		if(rating) RedrawRatingInfo(*rating);
		buttonOk->Draw();
		buttonCancel->Draw();
		cursor.Show();
		display.Flip();
	    }
	}

	if(le.MousePressRight(rectPanel))
	{
	    if(buttonSelectMaps && le.MousePressRight(*buttonSelectMaps))
		Dialog::Message(_("Scenario"), _("Click here to select which scenario to play."), Font::BIG);
	    else
	    if(0 <= coordDifficulty.GetIndex(le.GetMouseCursor()))
		Dialog::Message(_("Game Difficulty"), _("This lets you change the starting difficulty at which you will play. Higher difficulty levels start you of with fewer resources, and at the higher settings, give extra resources to the computer."), Font::BIG);
	    else
	    if(rating && le.MousePressRight(rating->GetRect()))
		Dialog::Message(_("Difficulty Rating"), _("The difficulty rating reflects a combination of various settings for your game. This number will be applied to your final score."), Font::BIG);
	    else
	    if(le.MousePressRight(*buttonOk))
		Dialog::Message(_("OK"), _("Click to accept these settings and start a new game."), Font::BIG);
	    else
	    if(le.MousePressRight(*buttonCancel))
		Dialog::Message(_("Cancel"), _("Click to return to the main menu."), Font::BIG);
	    else
		playersInfo.QueueEventProcessing();
	}
    }

    cursor.Hide();

    if(result == STARTGAME)
    {
	players.SetStartGame();
	if(conf.ExtGameUseFade()) display.Fade();
	Game::ShowLoadMapsText();
	// Load maps
	std::string lower = StringLower(conf.MapsFile());

	if(lower.size() > 3)
	{
	    std::string ext = lower.substr(lower.size() - 3);

	    if(ext == "mp2" || ext == "mx2")
		result = world.LoadMapMP2(conf.MapsFile()) ? STARTGAME : MAINMENU;
	    else
	    if(ext == "map")
		result = world.LoadMapMAP(conf.MapsFile()) ? STARTGAME : MAINMENU;
	}
	else
	{
	    result = MAINMENU;
	    DEBUG(DBG_GAME, DBG_WARN, conf.MapsFile() << ", " << "unknown map format");
	}
    }

    if(frameborder) delete frameborder;
    if(rating) delete rating;
    if(buttonSelectMaps) delete buttonSelectMaps;
    delete buttonOk;
    delete buttonCancel;

    return result;
}

u32 Game::GetStep4Player(u32 current, u32 width, u32 count)
{
    return current * width * KINGDOMMAX / count + (width * (KINGDOMMAX - count) / (2 * count));
}

void RedrawScenarioStaticInfo(const Rect & rt)
{
    Settings & conf = Settings::Get();

    if(conf.QVGA())
    {
	const Sprite & background = AGG::GetICN(ICN::STONEBAK, 0);
	background.Blit(Rect(0, 0, rt.w, rt.h), rt);

	Text text;
	text.Set(conf.CurrentFileInfo().name, Font::BIG);
	text.Blit(rt.x + (rt.w - text.w()) / 2, rt.y + 5);
    }
    else
    {
	// image panel
	const Sprite &panel = AGG::GetICN(ICN::NGHSBKG, 0);
	panel.Blit(rt);

	// text scenario
	Text text(_("Scenario:"), Font::BIG);
        text.Blit(rt.x + (rt.w - text.w()) / 2, rt.y + 20);

	// maps name
	text.Set(conf.MapsName());
	text.Blit(rt.x + (rt.w - text.w()) / 2, rt.y + 46);

	// text game difficulty
	text.Set(_("Game Difficulty:"));
	text.Blit(rt.x + (rt.w - text.w()) / 2, rt.y + 75);

	// text opponents
	text.Set(_("Opponents:"), Font::BIG);
	text.Blit(rt.x + (rt.w - text.w()) / 2, rt.y + 181);

	// text class
	text.Set(_("Class:"), Font::BIG);
	text.Blit(rt.x + (rt.w - text.w()) / 2, rt.y + 262);
    }
}

void RedrawDifficultyInfo(const Point & dst, bool label)
{
    for(u32 current = Difficulty::EASY; current <= Difficulty::IMPOSSIBLE; ++current)
    {
    	const Sprite & sprite = AGG::GetICN(ICN::NGHSBKG, 0);
    	Rect src_rt(24, 94, 65, 65);
    	u32 offset = current * (src_rt.w + 12);
    	src_rt.x = src_rt.x + offset;
	sprite.Blit(src_rt, dst.x + offset, dst.y);

	if(label)
	{
	    Text text(Difficulty::String(current), Font::SMALL);
	    text.Blit(dst.x + offset + (src_rt.w - text.w()) / 2, dst.y + src_rt.h + 5);
	}
    }
}

void RedrawRatingInfo(TextSprite & sprite)
{
    sprite.Hide();
    std::string str(_("Rating %{rating}%"));
    StringReplace(str, "%{rating}", Game::GetRating());
    sprite.SetText(str);
    sprite.Show();
}
