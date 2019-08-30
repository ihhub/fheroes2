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
#include "settings.h"
#include "text.h"
#include "button.h"
#include "game.h"
#include "dialog.h"
#include "army.h"
#include "army_troop.h"
#include "pocketpc.h"

void DrawMonsterStats(const Point &, const Troop &);
void DrawBattleStats(const Point &, const Troop &);

int PocketPC::DialogArmyInfo(const Troop & troop, u32 flags)
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder frameborder(Size(320, 224));
    const Rect & dst_rt = frameborder.GetArea();

    // name
    Text text;
    text.Set(troop.GetName(), Font::BIG);
    text.Blit(dst_rt.x + (dst_rt.w - text.w()) / 2, dst_rt.y + 10);

    const Sprite & frame = AGG::GetICN(troop.ICNMonh(), 0);
    frame.Blit(dst_rt.x + 50 - frame.w() / 2, dst_rt.y + 145 - frame.h());

    text.Set(GetString(troop.GetCount()));
    text.Blit(dst_rt.x + 50 - text.w() / 2, dst_rt.y + 150);

    // stats
    DrawMonsterStats(Point(dst_rt.x + 200, dst_rt.y + 40), troop);

    if(troop.isBattle())
        DrawBattleStats(Point(dst_rt.x + 160, dst_rt.y + 160), troop);

    Button buttonDismiss(dst_rt.x + dst_rt.w / 2 - 160, dst_rt.y + dst_rt.h - 30, ICN::VIEWARMY, 1, 2);
    Button buttonUpgrade(dst_rt.x + dst_rt.w / 2 - 60, dst_rt.y + dst_rt.h - 30, ICN::VIEWARMY, 5, 6);
    Button buttonExit(dst_rt.x + dst_rt.w / 2 + 60, dst_rt.y + dst_rt.h - 30, ICN::VIEWARMY, 3, 4);

    if(Dialog::READONLY & flags)
    {
        buttonDismiss.Press();
        buttonDismiss.SetDisable(true);
    }

    if(! troop.isBattle() && troop.isAllowUpgrade())
    {
	if(Dialog::UPGRADE & flags)
        {
	    if(Dialog::UPGRADE_DISABLE & flags)
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

    if(! troop.isBattle()) buttonDismiss.Draw();
    buttonExit.Draw();

    cursor.Show();
    display.Flip();

    while(le.HandleEvents())
    {
        if(buttonUpgrade.isEnable()) le.MousePressLeft(buttonUpgrade) ? (buttonUpgrade).PressDraw() : (buttonUpgrade).ReleaseDraw();
        if(buttonDismiss.isEnable()) le.MousePressLeft(buttonDismiss) ? (buttonDismiss).PressDraw() : (buttonDismiss).ReleaseDraw();
        le.MousePressLeft(buttonExit) ? (buttonExit).PressDraw() : (buttonExit).ReleaseDraw();

        if(buttonUpgrade.isEnable() && le.MouseClickLeft(buttonUpgrade)) return Dialog::UPGRADE;
        else
        if(buttonDismiss.isEnable() && le.MouseClickLeft(buttonDismiss)) return Dialog::DISMISS;
        else
        if(le.MouseClickLeft(buttonExit) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) return Dialog::CANCEL;
    }

    return Dialog::ZERO;
}
