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

namespace Dialog
{
    void DrawSystemInfo(const Rects &);
}

/* return 0x01 - change speed, 0x02 - change sound, 0x04 - change music, 0x08 - change interface, 0x10 - change scroll  */
int Dialog::SystemOptions(void)
{
    Display & display = Display::Get();
    Settings & conf = Settings::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    const int oldcursor = cursor.Themes();
    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder frameborder((display.w() - 300 - BORDERWIDTH * 2) / 2,
	(display.h() - 320 - BORDERWIDTH * 2) / 2 - (conf.QVGA() ? 25 : 0), 300, 315);
    const Rect & area = frameborder.GetArea();

    Rects rects;
    const s32 posx = (area.w - 256) / 2;
    rects.push_back(Rect(area.x + posx,  area.y + 17,  64, 64));
    rects.push_back(Rect(area.x + posx + 92, area.y + 17,  64, 64));
    rects.push_back(Rect(area.x + posx + 184, area.y + 17,  64, 64));
    rects.push_back(Rect(area.x + posx,  area.y + 107, 64, 64));
    rects.push_back(Rect(area.x + posx + 92, area.y + 107, 64, 64));
    rects.push_back(Rect(area.x + posx + 184, area.y + 107, 64, 64));
    rects.push_back(Rect(area.x + posx,  area.y + 197, 64, 64));
    rects.push_back(Rect(area.x + posx + 92, area.y + 197, 64, 64));
    rects.push_back(Rect(area.x + posx + 184, area.y + 197, 64, 64));

    const Rect & rect1 = rects[0];
    const Rect & rect2 = rects[1];
//    const Rect & rect3 = rects[2];
    const Rect & rect4 = rects[3];
    const Rect & rect5 = rects[4];
    const Rect & rect6 = rects[5];
    const Rect & rect7 = rects[6];
    const Rect & rect8 = rects[7];
//    const Rect & rect9 = rects[8];

    Surface back2 = display.GetSurface(Rect(area.x, area.y, area.w, area.h - 30));
    DrawSystemInfo(rects);

    LocalEvent & le = LocalEvent::Get();

    ButtonGroups btnGroups(area, Dialog::OK);
    btnGroups.Draw();

    cursor.Show();
    display.Flip();

    int btnres = Dialog::ZERO;
    int result = 0;
    bool redraw = false;

    // dialog menu loop
    while(btnres == Dialog::ZERO && le.HandleEvents())
    {
	btnres = btnGroups.QueueEventProcessing();

        // set sound volume
        if(conf.Sound() && le.MouseClickLeft(rect1))
        {
    	    conf.SetSoundVolume(10 > conf.SoundVolume() ? conf.SoundVolume() + 1 : 0);
	    redraw = true;
    	    Game::EnvironmentSoundMixer();
    	}

        // set music volume
        if(conf.Music() && le.MouseClickLeft(rect2))
        {
    	    conf.SetMusicVolume(10 > conf.MusicVolume() ? conf.MusicVolume() + 1 : 0);
	    redraw = true;
    	    Music::Volume(Mixer::MaxVolume() * conf.MusicVolume() / 10);
    	}

        // set hero speed
        if(le.MouseClickLeft(rect4))
        {
    	    conf.SetHeroesMoveSpeed(10 > conf.HeroesMoveSpeed() ? conf.HeroesMoveSpeed() + 1 : 0);
    	    result |= 0x01;
	    redraw = true;
	    Game::UpdateHeroesMoveSpeed();
    	}

        // set ai speed
        if(le.MouseClickLeft(rect5))
        {
    	    conf.SetAIMoveSpeed(10 > conf.AIMoveSpeed() ? conf.AIMoveSpeed() + 1 : 0);
    	    result |= 0x01;
	    redraw = true;
	    Game::UpdateHeroesMoveSpeed();
    	}

        // set scroll speed
        if(le.MouseClickLeft(rect6))
        {
    	    conf.SetScrollSpeed(SCROLL_FAST2 > conf.ScrollSpeed() ? conf.ScrollSpeed() << 1 : SCROLL_SLOW);
    	    result |= 0x10;
	    redraw = true;
    	}

        // set interface theme
        if(le.MouseClickLeft(rect7))
        {
    	    conf.SetEvilInterface(!conf.ExtGameEvilInterface());
    	    result |= 0x08;
	    redraw = true;
    	}

        // set interface hide/show
        if(le.MouseClickLeft(rect8) && !conf.QVGA())
        {
    	    conf.SetHideInterface(!conf.ExtGameHideInterface());
    	    result |= 0x04;
	    redraw = true;
    	}

	if(redraw)
	{
    	    cursor.Hide();
    	    back2.Blit(area, display);
	    DrawSystemInfo(rects);
    	    cursor.Show();
    	    display.Flip();
	    redraw = false;
	}
    }

    // restore background
    cursor.Hide();
    cursor.SetThemes(oldcursor);
    cursor.Show();
    display.Flip();

    return result;
}

void Dialog::DrawSystemInfo(const Rects & rects)
{
    Display & display = Display::Get();
    Settings & conf = Settings::Get();

    std::string str;
    Text text;

    Surface black(Size(65, 65), false);
    black.Fill(ColorBlack);

    // sound
    const Sprite & sprite1 = AGG::GetICN(ICN::SPANEL, conf.Sound() ? 1 : 0);
    const Rect & rect1 = rects[0];
    sprite1.Blit(rect1);
    str = _("sound");
    str.append(" ");
    if(conf.Sound() && conf.SoundVolume())
	str.append(GetString(conf.SoundVolume()));
    else
	str.append(_("off"));
    text.Set(str, Font::SMALL);
    text.Blit(rect1.x + (rect1.w - text.w()) / 2, rect1.y + rect1.h + 5);

    // music
    const Sprite & sprite2 = AGG::GetICN(ICN::SPANEL, conf.Music() ? 3 : 2);
    const Rect & rect2 = rects[1];
    sprite2.Blit(rect2);
    str = _("music");
    str.append(" ");
    if(conf.Music() && conf.MusicVolume())
	str.append(GetString(conf.MusicVolume()));
    else
	str.append(_("off"));
    text.Set(str);
    text.Blit(rect2.x + (rect2.w - text.w()) / 2, rect2.y + rect2.h + 5);

    // unused
    //const Sprite & sprite3 = AGG::GetICN(ICN::SPANEL, 17);
    const Rect & rect3 = rects[2];
    black.Blit(rect3, display);
    str.clear();
    str = "unused";
    text.Set(str);
    text.Blit(rect3.x + (rect3.w - text.w()) / 2, rect3.y + rect3.h + 5);

    // hero move speed
    const u32 is4 = conf.HeroesMoveSpeed() ? (conf.HeroesMoveSpeed() < 9 ? (conf.HeroesMoveSpeed() < 7 ? (conf.HeroesMoveSpeed() < 4 ? 4 : 5) : 6) : 7) : 9;
    const Sprite & sprite4 = AGG::GetICN(ICN::SPANEL, is4);
    const Rect & rect4 = rects[3];
    sprite4.Blit(rect4);
    str = _("hero speed");
    str.append(" ");
    if(conf.HeroesMoveSpeed())
	str.append(GetString(conf.HeroesMoveSpeed()));
    else
	str.append(_("off"));
    text.Set(str);
    text.Blit(rect4.x + (rect4.w - text.w()) / 2, rect4.y + rect4.h + 5);

    // ai move speed
    const u32 is5 = conf.AIMoveSpeed() ? (conf.AIMoveSpeed() < 9 ? (conf.AIMoveSpeed() < 7 ? (conf.AIMoveSpeed() < 4 ? 4 : 5) : 6) : 7) : 9;
    const Sprite & sprite5 = AGG::GetICN(ICN::SPANEL, is5);
    const Rect & rect5 = rects[4];
    sprite5.Blit(rect5);
    str = _("ai speed");
    str.append(" ");
    if(conf.AIMoveSpeed())
	str.append(GetString(conf.AIMoveSpeed()));
    else
	str.append(_("off"));
    text.Set(str);
    text.Blit(rect5.x + (rect5.w - text.w()) / 2, rect5.y + rect5.h + 5);

    // scroll speed
    const u32 is6 = (conf.ScrollSpeed() < SCROLL_FAST2 ? (conf.ScrollSpeed() < SCROLL_FAST1 ? (conf.ScrollSpeed() < SCROLL_NORMAL ? 4 : 5) : 6) : 7);
    const Sprite & sprite6 = AGG::GetICN(ICN::SPANEL, is6);
    const Rect & rect6 = rects[5];
    sprite6.Blit(rect6);
    str = _("scroll speed");
    str.append(" ");
    str.append(GetString(conf.ScrollSpeed()));
    text.Set(str);
    text.Blit(rect6.x + (rect6.w - text.w()) / 2, rect6.y + rect6.h + 5);

    // interface themes
    const Sprite & sprite7 = AGG::GetICN(ICN::SPANEL, (conf.ExtGameEvilInterface() ? 17 : 16));
    const Rect & rect7 = rects[6];
    sprite7.Blit(rect7);
    str.clear();
    str = _("Interface");
    str += ": ";
    if(conf.ExtGameEvilInterface())
	str += _("Evil");
    else
	str += _("Good");
    text.Set(str);
    text.Blit(rect7.x + (rect7.w - text.w()) / 2, rect7.y + rect7.h + 5);

    // interface show/hide
    const Sprite & sprite8 = AGG::GetICN(ICN::SPANEL, 16);
    const Sprite & sprite81 = AGG::GetICN(ICN::ESPANEL, 4);
    const Rect & rect8 = rects[7];
    str.clear();
    str = _("Interface");
    str += ": ";
    if(conf.ExtGameHideInterface())
    {
	sprite81.Blit(rect8, display);
	str += _("Hide");
    }
    else
    {
	sprite8.Blit(rect8, display);
	sprite81.Blit(Rect(13, 13, 38, 38), rect8.x + 13, rect8.y + 13);
	str += _("Show");
    }
    text.Set(str);
    text.Blit(rect8.x + (rect8.w - text.w()) / 2, rect8.y + rect8.h + 5);

    // unused
    //const Sprite & sprite9 = AGG::GetICN(ICN::SPANEL, 17);
    const Rect & rect9 = rects[8];
    black.Blit(rect9, display);
    str.clear();
    str = "unused";
    text.Set(str);
    text.Blit(rect9.x + (rect9.w - text.w()) / 2, rect9.y + rect9.h + 5);
}
