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

#include "gamedefs.h"
#include "agg.h"
#include "text.h"
#include "cursor.h"
#include "dialog.h"
#include "button.h"
#include "settings.h"
#include "pocketpc.h"
#include "game.h"

#define NEWGAME_DEFAULT 1
#define LOADGAME_DEFAULT 5
#define HIGHSCORES_DEFAULT 9
#define CREDITS_DEFAULT 13
#define QUIT_DEFAULT 17

int Game::MainMenu(void)
{
    Mixer::Pause();
    AGG::PlayMusic(MUS::MAINMENU);

    Settings & conf = Settings::Get();

    conf.SetGameType(TYPE_MENU);
    if(conf.QVGA()) return PocketPC::MainMenu();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Display & display = Display::Get();
    display.Fill(ColorBlack);

    // image background
    const Sprite &sprite = AGG::GetICN(ICN::HEROES, 0);
    const Point top((display.w() - sprite.w()) / 2, (display.h() - sprite.h()) / 2);
    sprite.Blit(top);

    LocalEvent & le = LocalEvent::Get();

    const Sprite & s1 = AGG::GetICN(ICN::BTNSHNGL, NEWGAME_DEFAULT);
    const Sprite & s2 = AGG::GetICN(ICN::BTNSHNGL, LOADGAME_DEFAULT);
    const Sprite & s3 = AGG::GetICN(ICN::BTNSHNGL, HIGHSCORES_DEFAULT);
    const Sprite & s4 = AGG::GetICN(ICN::BTNSHNGL, CREDITS_DEFAULT);
    const Sprite & s5 = AGG::GetICN(ICN::BTNSHNGL, QUIT_DEFAULT);

    Button buttonNewGame(top.x + s1.x(), top.y + s1.y(), ICN::BTNSHNGL, NEWGAME_DEFAULT, NEWGAME_DEFAULT + 2);
    Button buttonLoadGame(top.x + s2.x(), top.y + s2.y(), ICN::BTNSHNGL, LOADGAME_DEFAULT, LOADGAME_DEFAULT + 2);
    Button buttonHighScores(top.x + s3.x(), top.y + s3.y(), ICN::BTNSHNGL, HIGHSCORES_DEFAULT, HIGHSCORES_DEFAULT + 2);
    Button buttonCredits(top.x + s4.x(), top.y + s4.y(), ICN::BTNSHNGL, CREDITS_DEFAULT, CREDITS_DEFAULT + 2);
    Button buttonQuit(top.x + s5.x(), top.y + s5.y(), ICN::BTNSHNGL, QUIT_DEFAULT, QUIT_DEFAULT + 2);

    const Point lt_pt(0, 0);

    const Sprite & lantern10 = AGG::GetICN(ICN::SHNGANIM, 0);
    lantern10.Blit(top.x + lantern10.x(), top.y + lantern10.y());

    const Sprite & lantern11 = AGG::GetICN(ICN::SHNGANIM, ICN::AnimationFrame(ICN::SHNGANIM, 0, 0));
    lantern11.Blit(top.x + lantern11.x(), top.y + lantern11.y());

    buttonNewGame.Draw();
    buttonLoadGame.Draw();
    buttonHighScores.Draw();
    buttonCredits.Draw();
    buttonQuit.Draw();

    cursor.Show();
    display.Flip();

    u32 lantern_frame = 0;

    struct ButtonInfo
    {
        u32 frame;
        Button &button;
        bool isOver;
        bool wasOver;
    } buttons[] = {
        { NEWGAME_DEFAULT, buttonNewGame, false, false },
        { LOADGAME_DEFAULT, buttonLoadGame, false, false },
        { HIGHSCORES_DEFAULT, buttonHighScores, false, false },
        { CREDITS_DEFAULT, buttonCredits, false, false },
        { QUIT_DEFAULT, buttonQuit, false, false }
    };

    for(u32 i = 0; le.MouseMotion() && i < ARRAY_COUNT(buttons); i++)
    {
        cursor.Hide();
        const Sprite & sprite = AGG::GetICN(ICN::BTNSHNGL, buttons[i].frame);
        sprite.Blit(top.x + sprite.x(), top.y + sprite.y());
        cursor.Show();
    }

    // mainmenu loop
    while(le.HandleEvents())
    {
        for(u32 i = 0; i < ARRAY_COUNT(buttons); i++)
        {
            buttons[i].wasOver = buttons[i].isOver;

            if(le.MousePressLeft(buttons[i].button))
                buttons[i].button.PressDraw();
            else buttons[i].button.ReleaseDraw();

            buttons[i].isOver = le.MouseCursor(buttons[i].button);

            if((!buttons[i].isOver && buttons[i].wasOver) ||
               (buttons[i].isOver && !buttons[i].wasOver))
            {
                u32 frame = buttons[i].frame;

                if(buttons[i].isOver && !buttons[i].wasOver)
                    frame++;

                cursor.Hide();
                const Sprite & sprite = AGG::GetICN(ICN::BTNSHNGL, frame);
                sprite.Blit(top.x + sprite.x(), top.y + sprite.y());
                cursor.Show();
            }
        }

	if(HotKeyPressEvent(EVENT_BUTTON_NEWGAME) || le.MouseClickLeft(buttonNewGame)) return NEWGAME;
	else
	if(HotKeyPressEvent(EVENT_BUTTON_LOADGAME) || le.MouseClickLeft(buttonLoadGame)) return LOADGAME;
	else
	if(HotKeyPressEvent(EVENT_BUTTON_HIGHSCORES) || le.MouseClickLeft(buttonHighScores)) return HIGHSCORES;
	else
	if(HotKeyPressEvent(EVENT_BUTTON_CREDITS) || le.MouseClickLeft(buttonCredits)) return CREDITS;
	else
	if(HotKeyPressEvent(EVENT_DEFAULT_EXIT) || le.MouseClickLeft(buttonQuit))
	{ if(conf.ExtGameUseFade()) display.Fade(); return QUITGAME; }

	// right info
	if(le.MousePressRight(buttonQuit)) Dialog::Message(_("Quit"), _("Quit Heroes of Might and Magic and return to the operating system."), Font::BIG);
	else
	if(le.MousePressRight(buttonLoadGame)) Dialog::Message(_("Load Game"), _("Load a previously saved game."), Font::BIG);
	else
	if(le.MousePressRight(buttonCredits)) Dialog::Message(_("Credits"), _("View the credits screen."), Font::BIG);
	else
	if(le.MousePressRight(buttonHighScores)) Dialog::Message(_("High Scores"), _("View the high score screen."), Font::BIG);
	else
	if(le.MousePressRight(buttonNewGame)) Dialog::Message(_("New Game"), _("Start a single or multi-player game."), Font::BIG);

	if(AnimateInfrequentDelay(MAIN_MENU_DELAY))
	{
	    cursor.Hide();
	    const Sprite & lantern12 = AGG::GetICN(ICN::SHNGANIM, ICN::AnimationFrame(ICN::SHNGANIM, 0, lantern_frame++));
	    lantern12.Blit(top.x + lantern12.x(), top.y + lantern12.y());
	    cursor.Show();
	    display.Flip();
	}
    }

    return QUITGAME;
}
