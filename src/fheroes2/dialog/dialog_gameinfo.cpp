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
#include "icn.h"
#include "text.h"
#include "settings.h"
#include "cursor.h"
#include "button.h"
#include "maps.h"
#include "game.h"
#include "game_over.h"
#include "difficulty.h"
#include "dialog.h"

void Dialog::GameInfo(void)
{
    // FIXME: QVGA version
    if(Settings::Get().QVGA())
    {
       Dialog::Message("", _("For the QVGA version is not available."), Font::SMALL, Dialog::OK);
       return;
    }

    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    Settings & conf = Settings::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    const Sprite & dlg = AGG::GetICN(ICN::SCENIBKG, 0);

    SpriteBack back(Rect((display.w() - dlg.w()) / 2, (display.h() - dlg.h()) / 2, dlg.w(), dlg.h()));
    const Point & pt = back.GetPos();
    dlg.Blit(pt);

    TextBox text;

    text.Set(conf.MapsName(), Font::BIG, 350);
    text.Blit(pt.x + 52, pt.y + 30);

    text.Set(_("Map\nDifficulty"), Font::SMALL, 80);
    text.Blit(pt.x + 50, pt.y + 54);

    text.Set(_("Game\nDifficulty"), Font::SMALL, 80);
    text.Blit(pt.x + 140, pt.y + 54);

    text.Set(_("Rating"), Font::SMALL, 80);
    text.Blit(pt.x + 230, pt.y + 61);

    text.Set(_("Map Size"), Font::SMALL, 80);
    text.Blit(pt.x + 322, pt.y + 61);

    text.Set(Difficulty::String(conf.MapsDifficulty()), Font::SMALL, 80);
    text.Blit(pt.x + 50, pt.y + 80);

    text.Set(Difficulty::String(conf.GameDifficulty()), Font::SMALL, 80);
    text.Blit(pt.x + 140, pt.y + 80);

    text.Set(GetString(Game::GetRating()) + " %", Font::SMALL, 80);
    text.Blit(pt.x + 230, pt.y + 80);

    text.Set(Maps::SizeString(conf.MapsSize().w), Font::SMALL, 80);
    text.Blit(pt.x + 322, pt.y + 80);

    text.Set(conf.MapsDescription(), Font::SMALL, 350);
    text.Blit(pt.x + 52, pt.y + 105);

    text.Set(_("Opponents"), Font::SMALL, 350);
    text.Blit(pt.x + 52, pt.y + 150);

    text.Set(_("Class"), Font::SMALL, 350);
    text.Blit(pt.x + 52, pt.y + 225);

    Interface::PlayersInfo playersInfo(true, true, false);

    playersInfo.UpdateInfo(conf.GetPlayers(), Point(pt.x + 40, pt.y + 165), Point(pt.x + 40, pt.y + 240));
    playersInfo.RedrawInfo(true);

    text.Set(_("Victory\nConditions"), Font::SMALL, 80);
    text.Blit(pt.x + 40, pt.y + 345);

    text.Set(GameOver::GetActualDescription(conf.ConditionWins()), Font::SMALL, 272);
    text.Blit(pt.x + 130, pt.y + 348);

    text.Set(_("Loss\nConditions"), Font::SMALL, 80);
    text.Blit(pt.x + 40, pt.y + 390);

    text.Set(GameOver::GetActualDescription(conf.ConditionLoss()), Font::SMALL, 272);
    text.Blit(pt.x + 130, pt.y + 396);

    text.Set("score: " + GetString(Game::GetGameOverScores()), Font::YELLOW_SMALL, 80);
    text.Blit(pt.x + 415 - text.w(), pt.y + 434);

    Button buttonOk(pt.x + 180, pt.y + 425, ICN::SYSTEM, 1, 2);
    Button buttonCfg(pt.x + 50, pt.y + 425, ICN::BTNCONFIG, 0, 1);

    buttonOk.Draw();
    buttonCfg.Draw();

    cursor.Show();
    display.Flip();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while(le.HandleEvents())
    {
	le.MousePressLeft(buttonOk) ? buttonOk.PressDraw() : buttonOk.ReleaseDraw();
	le.MousePressLeft(buttonCfg) ? buttonCfg.PressDraw() : buttonCfg.ReleaseDraw();

        if(le.MouseClickLeft(buttonCfg))
	{
	    Dialog::ExtSettings(true);
	    Cursor::Get().Show();
	    Display::Get().Flip();
	}

        if(le.MouseClickLeft(buttonOk) ||
	   HotKeyCloseWindow) break;
    }

    cursor.Hide();
    back.Restore();
}
