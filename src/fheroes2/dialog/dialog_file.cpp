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
#include "game.h"
#include "dialog.h"

int Dialog::FileOptions(void)
{
    Display & display = Display::Get();
    // preload
    const int cpanbkg = Settings::Get().ExtGameEvilInterface() ? ICN::CPANBKGE : ICN::CPANBKG;
    const int cpanel  = Settings::Get().ExtGameEvilInterface() ? ICN::CPANELE : ICN::CPANEL;

    // cursor
    Cursor & cursor = Cursor::Get();
    const int oldcursor = cursor.Themes();
    cursor.Hide();
    cursor.SetThemes(Cursor::POINTER);

    // image box
    const Sprite &box = AGG::GetICN(cpanbkg, 0);

    SpriteBack back(Rect((display.w() - box.w()) / 2, (display.h() - box.h()) / 2, box.w(), box.h()));
    const Point & rb = back.GetPos();
    box.Blit(rb.x, rb.y);

    LocalEvent & le = LocalEvent::Get();

    Button buttonNew(rb.x + 62, rb.y + 31, cpanel, 0, 1);
    Button buttonLoad(rb.x + 195, rb.y + 31, cpanel, 2, 3);
    Button buttonSave(rb.x + 62, rb.y + 107, cpanel, 4, 5);
    Button buttonQuit(rb.x + 195, rb.y + 107, cpanel, 6, 7);
    Button buttonCancel(rb.x + 128, rb.y + 184, cpanel, 8, 9);

    buttonNew.Draw();
    buttonLoad.Draw();
    buttonSave.Draw();
    buttonQuit.Draw();
    buttonCancel.Draw();

    cursor.Show();
    display.Flip();

    int result = Game::QUITGAME;

    // dialog menu loop
    while(le.HandleEvents())
    {
        le.MousePressLeft(buttonNew) ? buttonNew.PressDraw() : buttonNew.ReleaseDraw();
        le.MousePressLeft(buttonLoad) ? buttonLoad.PressDraw() : buttonLoad.ReleaseDraw();
        le.MousePressLeft(buttonSave) ? buttonSave.PressDraw() : buttonSave.ReleaseDraw();
        le.MousePressLeft(buttonQuit) ? buttonQuit.PressDraw() : buttonQuit.ReleaseDraw();
        le.MousePressLeft(buttonCancel) ? buttonCancel.PressDraw() : buttonCancel.ReleaseDraw();

        if(le.MouseClickLeft(buttonNew)) { result = Game::NEWGAME;  break; }
        if(le.MouseClickLeft(buttonLoad)){ result = Game::LOADGAME; break; }
        if(le.MouseClickLeft(buttonSave)){ result = Game::SAVEGAME; break; }
        if(le.MouseClickLeft(buttonQuit)){ result = Game::QUITGAME; break; }
        if(le.MouseClickLeft(buttonCancel) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)){ result = Game::CANCEL; break; }
    }

    // restore background
    cursor.Hide();
    back.Restore();
    cursor.SetThemes(oldcursor);
    cursor.Show();
    display.Flip();

    return result;
}
