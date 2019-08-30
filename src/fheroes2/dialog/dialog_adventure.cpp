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

int Dialog::AdventureOptions(bool enabledig)
{
    Display & display = Display::Get();

    // preload
    const int apanbkg = Settings::Get().ExtGameEvilInterface() ? ICN::APANBKGE : ICN::APANBKG;
    const int apanel  = Settings::Get().ExtGameEvilInterface() ? ICN::APANELE : ICN::APANEL;

    // cursor
    Cursor & cursor = Cursor::Get();
    const int oldcursor = cursor.Themes();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    // image box
    const Sprite &box = AGG::GetICN(apanbkg, 0);
    SpriteBack back(Rect((display.w() - box.w()) / 2, (display.h() - box.h()) / 2, box.w(), box.h()));
    const Point & rb = back.GetPos();
    box.Blit(rb.x, rb.y);

    LocalEvent & le = LocalEvent::Get();

    Button buttonWorld(rb.x + 62, rb.y + 30, apanel, 0, 1);
    Button buttonPuzzle(rb.x + 195, rb.y + 30, apanel, 2, 3);
    Button buttonInfo(rb.x + 62, rb.y + 107, apanel, 4, 5);
    Button buttonDig(rb.x + 195, rb.y + 107, apanel, 6, 7);
    Button buttonCancel(rb.x + 128, rb.y + 184, apanel, 8, 9);

    if(! enabledig) buttonDig.SetDisable(true);

    buttonWorld.Draw();
    buttonPuzzle.Draw();
    buttonInfo.Draw();
    buttonDig.Draw();
    buttonCancel.Draw();

    cursor.Show();
    display.Flip();

    int result = Dialog::ZERO;

    // dialog menu loop
    while(le.HandleEvents())
    {
        le.MousePressLeft(buttonWorld) ? buttonWorld.PressDraw() : buttonWorld.ReleaseDraw();
        le.MousePressLeft(buttonPuzzle) ? buttonPuzzle.PressDraw() : buttonPuzzle.ReleaseDraw();
        le.MousePressLeft(buttonInfo) ? buttonInfo.PressDraw() : buttonInfo.ReleaseDraw();
        le.MousePressLeft(buttonDig) ? buttonDig.PressDraw() : buttonDig.ReleaseDraw();
        le.MousePressLeft(buttonCancel) ? buttonCancel.PressDraw() : buttonCancel.ReleaseDraw();

        if(le.MouseClickLeft(buttonWorld)){ result = Dialog::WORLD; break; }
        if(le.MouseClickLeft(buttonPuzzle)){ result = Dialog::PUZZLE; break; }
        if(le.MouseClickLeft(buttonInfo)){ result = Dialog::INFO; break; }
        if(le.MouseClickLeft(buttonDig) && buttonDig.isEnable()){ result = Dialog::DIG; break; }
        if(le.MouseClickLeft(buttonCancel) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)){ result = Dialog::CANCEL; break; }

	// right info
        if(le.MousePressRight(buttonWorld)) Dialog::Message("", _("View the entire world."), Font::BIG);
        if(le.MousePressRight(buttonPuzzle)) Dialog::Message("", _("View the obelisk puzzle."), Font::BIG);
        if(le.MousePressRight(buttonInfo)) Dialog::Message("", _("View information on the scenario you are currently playing."), Font::BIG);
        if(le.MousePressRight(buttonDig)) Dialog::Message("", _("Dig for the Ultimate Artifact."), Font::BIG);
        if(le.MousePressRight(buttonCancel)) Dialog::Message("", _("Exit this menu without doing anything."), Font::BIG);
    }

    // restore background
    cursor.Hide();
    back.Restore();
    cursor.SetThemes(oldcursor);
    cursor.Show();
    display.Flip();

    return result;
}
