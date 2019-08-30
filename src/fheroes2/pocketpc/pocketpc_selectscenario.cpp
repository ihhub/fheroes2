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
#include "button.h"
#include "cursor.h"
#include "difficulty.h"
#include "settings.h"
#include "dialog.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "game.h"
#include "dialog_selectscenario.h"
#include "text.h"
#include "tools.h"
#include "pocketpc.h"

int PocketPC::SelectScenario(void)
{
    Settings & conf = Settings::Get();
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    const Sprite &sprite = AGG::GetICN(ICN::HEROES, 0);
    Rect src_rt((sprite.w() - display.w()) / 2, 0, display.w(), display.h());
    sprite.Blit(src_rt, 0, 0);

    MapsFileInfoList all;
    if(!PrepareMapsFileInfoList(all, false))
    {
        Dialog::Message(_("Warning"), _("No maps available!"), Font::BIG, Dialog::OK);
        return Game::MAINMENU;
    }

    MapsFileInfoList small;
    MapsFileInfoList medium;
    MapsFileInfoList large;
    MapsFileInfoList xlarge;

    small.reserve(all.size());
    medium.reserve(all.size());
    large.reserve(all.size());
    xlarge.reserve(all.size());

    for(MapsFileInfoList::iterator cur = all.begin(); cur != all.end(); ++ cur)
    {
	switch((*cur).size_w)
	{
    	    case Maps::SMALL:	small.push_back(*cur); break;
    	    case Maps::MEDIUM:	medium.push_back(*cur); break;
    	    case Maps::LARGE:	large.push_back(*cur); break;
    	    case Maps::XLARGE:	xlarge.push_back(*cur); break;
	    default: continue;
	}
    }

    Dialog::FrameBorder frameborder(Size(320, 224));
    const Rect & rt = frameborder.GetArea();

    ButtonGroups btnGroups(rt, Dialog::OK|Dialog::CANCEL);

    Button buttonSelectSmall(rt.x + 7, rt.y + 12, ICN::REQUESTS, 9, 10);
    Button buttonSelectMedium(rt.x + 69, rt.y + 12, ICN::REQUESTS, 11, 12);
    Button buttonSelectLarge(rt.x + 131, rt.y + 12, ICN::REQUESTS, 13, 14);
    Button buttonSelectXLarge(rt.x + 193, rt.y + 12, ICN::REQUESTS, 15, 16);
    Button buttonSelectAll(rt.x + 255, rt.y + 12, ICN::REQUESTS, 17, 18);

    if(all.empty()) btnGroups.DisableButton1(true);
    if(small.empty()) buttonSelectSmall.SetDisable(true);
    if(medium.empty()) buttonSelectMedium.SetDisable(true);
    if(large.empty()) buttonSelectLarge.SetDisable(true);
    if(xlarge.empty()) buttonSelectXLarge.SetDisable(true);

    ScenarioListBox listbox(rt);

    listbox.RedrawBackground(rt);
    listbox.SetScrollButtonUp(ICN::REQUESTS, 5, 6, Point(rt.x + 285, rt.y + 40));
    listbox.SetScrollButtonDn(ICN::REQUESTS, 7, 8, Point(rt.x + 285, rt.y + 175));
    listbox.SetScrollSplitter(AGG::GetICN(ICN::ESCROLL, 3), Rect(rt.x + 286, rt.y + 58, 12, 114));
    listbox.SetAreaMaxItems(8);
    listbox.SetAreaItems(Rect(rt.x + 17, rt.y + 37, 266, 156));
    listbox.SetListContent(all);

    listbox.Redraw();

    btnGroups.Draw();

    buttonSelectSmall.Draw();
    buttonSelectMedium.Draw();
    buttonSelectLarge.Draw();
    buttonSelectXLarge.Draw();
    buttonSelectAll.Draw();

    u32 result = Dialog::ZERO;

    cursor.Show();
    display.Flip();

    while(result == Dialog::ZERO && le.HandleEvents())
    {
	le.MousePressLeft(buttonSelectSmall) && buttonSelectSmall.isEnable() ? buttonSelectSmall.PressDraw() : buttonSelectSmall.ReleaseDraw();
	le.MousePressLeft(buttonSelectMedium) && buttonSelectMedium.isEnable() ? buttonSelectMedium.PressDraw() : buttonSelectMedium.ReleaseDraw();
	le.MousePressLeft(buttonSelectLarge) && buttonSelectLarge.isEnable() ? buttonSelectLarge.PressDraw() : buttonSelectLarge.ReleaseDraw();
	le.MousePressLeft(buttonSelectXLarge) && buttonSelectXLarge.isEnable() ? buttonSelectXLarge.PressDraw() : buttonSelectXLarge.ReleaseDraw();
	le.MousePressLeft(buttonSelectAll) ? buttonSelectAll.PressDraw() : buttonSelectAll.ReleaseDraw();

	listbox.QueueEventProcessing();
	result = btnGroups.QueueEventProcessing();

	if((le.MouseClickLeft(buttonSelectSmall) || le.KeyPress(KEY_s)) &&
		buttonSelectSmall.isEnable() && buttonSelectSmall.isEnable())
	{
	    listbox.SetListContent(small);
	    cursor.Hide();
	}
	else
	if((le.MouseClickLeft(buttonSelectMedium) || le.KeyPress(KEY_m)) &&
		buttonSelectMedium.isEnable() && buttonSelectMedium.isEnable())
	{
	    listbox.SetListContent(medium);
	    cursor.Hide();
	}
	else
	if((le.MouseClickLeft(buttonSelectLarge) || le.KeyPress(KEY_l)) &&
		buttonSelectLarge.isEnable() && buttonSelectLarge.isEnable())
	{
	    listbox.SetListContent(large);
	    cursor.Hide();
	}
	else
	if((le.MouseClickLeft(buttonSelectXLarge) || le.KeyPress(KEY_x)) &&
		buttonSelectXLarge.isEnable() && buttonSelectXLarge.isEnable())
	{
	    listbox.SetListContent(xlarge);
	    cursor.Hide();
	}
	else
	if(le.MouseClickLeft(buttonSelectAll) || le.KeyPress(KEY_a))
	{
	    listbox.SetListContent(all);
	    cursor.Hide();
	}

	if(!cursor.isVisible())
	{
	    listbox.Redraw();
	    cursor.Show();
	    display.Flip();
	}
    }

    if(Dialog::OK == result)
    {
	conf.SetCurrentFileInfo(listbox.GetCurrent());
    	conf.SetGameDifficulty(Difficulty::NORMAL);

	return Game::SCENARIOINFO;
    }

    return Game::MAINMENU;
}

