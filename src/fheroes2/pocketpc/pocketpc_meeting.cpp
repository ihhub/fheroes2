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
#include "cursor.h"
#include "text.h"
#include "button.h"
#include "dialog.h"
#include "heroes.h"
#include "game.h"
#include "heroes_indicator.h"
#include "army_bar.h"
#include "world.h"
#include "kingdom.h"
#include "pocketpc.h"

void PocketPC::HeroesMeeting(Heroes & hero1, Heroes & hero2)
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder frameborder(Size(320, 236));
    const Rect & dst_rt = frameborder.GetArea();

    // portrait
    AGG::GetICN(ICN::BRCREST, 6).Blit(dst_rt.x + 4, dst_rt.y + 4, display);
    hero1.PortraitRedraw(dst_rt.x + 8, dst_rt.y + 8, PORT_MEDIUM, display);

    AGG::GetICN(ICN::BRCREST, 6).Blit(dst_rt.x + 4, dst_rt.y + 118, display);
    hero2.PortraitRedraw(dst_rt.x + 8, dst_rt.y + 122, PORT_MEDIUM, display);

    // art bar
    ArtifactsBar selectArtifacts1(&hero1, true, false);
    selectArtifacts1.SetColRows(7, 2);
    selectArtifacts1.SetHSpace(2);
    selectArtifacts1.SetVSpace(2);
    selectArtifacts1.SetContent(hero1.GetBagArtifacts());
    selectArtifacts1.SetPos(dst_rt.x + 68, dst_rt.y + 2);
    selectArtifacts1.Redraw();

    ArtifactsBar selectArtifacts2(&hero2, true, false);
    selectArtifacts2.SetColRows(7, 2);
    selectArtifacts2.SetHSpace(2);
    selectArtifacts2.SetVSpace(2);
    selectArtifacts2.SetContent(hero2.GetBagArtifacts());
    selectArtifacts2.SetPos(dst_rt.x + 68, dst_rt.y + 164);
    selectArtifacts2.Redraw();

    // army bar
    ArmyBar selectArmy1(&hero1.GetArmy(), true, false);
    selectArmy1.SetColRows(5, 1);
    selectArmy1.SetPos(dst_rt.x + 68, dst_rt.y + 74);
    selectArmy1.SetHSpace(2);
    selectArmy1.Redraw();

    ArmyBar selectArmy2(&hero2.GetArmy(), true, false);
    selectArmy2.SetColRows(5, 1);
    selectArmy2.SetPos(dst_rt.x + 68, dst_rt.y + 119);
    selectArmy2.SetHSpace(2);
    selectArmy2.Redraw();

    const Rect rectExit(dst_rt.x + dst_rt.w - 25, dst_rt.y + (dst_rt.h - 25) / 2, 25, 25);
    AGG::GetICN(ICN::TOWNWIND, 12).Blit(rectExit.x + 4, rectExit.y + 4);

    cursor.Show();
    display.Flip();

    while(le.HandleEvents())
    {
        // exit
        if(le.MouseClickLeft(rectExit) || HotKeyCloseWindow) break;

        if((le.MouseCursor(selectArmy1.GetArea()) &&
            selectArmy1.QueueEventProcessing(selectArmy2)) ||
           (le.MouseCursor(selectArmy2.GetArea()) &&
            selectArmy2.QueueEventProcessing(selectArmy1)))
        {
            cursor.Hide();

    	    if(selectArtifacts1.isSelected()) selectArtifacts1.ResetSelected();
            else
            if(selectArtifacts2.isSelected()) selectArtifacts2.ResetSelected();

            selectArmy1.Redraw();
            selectArmy2.Redraw();

            cursor.Show();
            display.Flip();
	}

        if((le.MouseCursor(selectArtifacts1.GetArea()) &&
            selectArtifacts1.QueueEventProcessing(selectArtifacts2)) ||
           (le.MouseCursor(selectArtifacts2.GetArea()) &&
            selectArtifacts2.QueueEventProcessing(selectArtifacts1)))
        {
            cursor.Hide();

    	    if(selectArmy1.isSelected()) selectArmy1.ResetSelected();
            else
            if(selectArmy2.isSelected()) selectArmy2.ResetSelected();

            selectArtifacts1.Redraw();
            selectArtifacts2.Redraw();

            cursor.Show();
            display.Flip();
    	}
    }
}
