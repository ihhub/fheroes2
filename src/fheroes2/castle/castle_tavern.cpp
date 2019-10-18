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

#include <string>
#include "agg.h"
#include "button.h"
#include "world.h"
#include "cursor.h"
#include "dialog.h"
#include "settings.h"
#include "resource.h"
#include "castle.h"
#include "heroes.h"
#include "kingdom.h"
#include "game.h"
#include "text.h"

void Castle::OpenTavern(void)
{
    const std::string & header = _("A generous tip for the barkeep yields the following rumor:");
    const int system = (Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM);
    const int tavwin = ICN::TAVWIN;
    const std::string & tavern = GetStringBuilding(BUILD_TAVERN);
    const std::string & message = world.GetRumors();

    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    Text text(tavern, Font::BIG);
    const Sprite & s1 = AGG::GetICN(tavwin, 0);
    TextBox box1(header, Font::BIG, BOXAREA_WIDTH);
    TextBox box2(message, Font::BIG, BOXAREA_WIDTH);

    Dialog::FrameBox box(text.h() + 10 + s1.h() + 13 + box1.h() + 20 + box2.h(), true);

    const Rect & pos = box.GetArea();
    Point dst_pt(pos.x, pos.y);

    text.Blit(pos.x + (pos.w - text.w()) / 2, dst_pt.y);

    dst_pt.x = pos.x + (pos.w - s1.w()) / 2;
    dst_pt.y += 10 + text.h();
    s1.Blit(dst_pt);

    dst_pt.x += 3;
    dst_pt.y += 3;

    const Sprite & s20 = AGG::GetICN(tavwin, 1);
    s20.Blit(dst_pt);

    if(const u32 index = ICN::AnimationFrame(tavwin, 0, 0))
    {
	const Sprite & s21 = AGG::GetICN(tavwin, index);
	s21.Blit(dst_pt.x + s21.x(), dst_pt.y + s21.y());
    }

    box1.Blit(pos.x, dst_pt.y + s1.h() + 10);
    box2.Blit(pos.x, dst_pt.y + s1.h() + 10 + box1.h() + 20);

    // button yes
    const Sprite & s4 = AGG::GetICN(system, 5);
    Button buttonYes(pos.x + (pos.w - s4.w()) / 2, pos.y + pos.h - s4.h(), system, 5, 6);

    buttonYes.Draw();

    cursor.Show();
    display.Flip();

    LocalEvent & le = LocalEvent::Get();
    u32 frame = 0;

    // message loop
    while(le.HandleEvents())
    {
        le.MousePressLeft(buttonYes) ? buttonYes.PressDraw() : buttonYes.ReleaseDraw();
        if(le.MouseClickLeft(buttonYes) || HotKeyCloseWindow) break;

        // animation
	if(Game::AnimateInfrequentDelay(Game::CASTLE_TAVERN_DELAY))
	{
	    cursor.Hide();
	    s20.Blit(dst_pt);

	    if(const u32 index = ICN::AnimationFrame(tavwin, 0, frame++))
	    {
		const Sprite & s22 = AGG::GetICN(tavwin, index);
		s22.Blit(dst_pt.x + s22.x(), dst_pt.y + s22.y());
	    }

	    cursor.Show();
	    display.Flip();
	}
    }
}
