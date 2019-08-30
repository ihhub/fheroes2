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
#include "text.h"
#include "world.h"
#include "button.h"
#include "cursor.h"
#include "castle.h"
#include "kingdom.h"
#include "settings.h"
#include "game.h"
#include "dialog.h"
#include "payment.h"

int Dialog::BuyBoat(bool enable)
{
    Display & display = Display::Get();

    const int system = Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM;

    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    Resource::BoxSprite rbs(PaymentConditions::BuyBoat(), BOXAREA_WIDTH);

    const Sprite & sprite = AGG::GetICN(ICN::BOATWIND, 0);
    Text text(_("Build a new ship:"), Font::BIG);
    const int spacer = Settings::Get().QVGA() ? 5 : 10;

    Dialog::FrameBox box(text.h() + spacer + sprite.h() + spacer + text.h() + spacer + rbs.GetArea().h - 20, true);

    const Rect & box_rt = box.GetArea();
    Point dst_pt;
    dst_pt.x = box_rt.x + (box_rt.w - text.w()) / 2;
    dst_pt.y = box_rt.y;
    text.Blit(dst_pt);

    dst_pt.x = box_rt.x + (box_rt.w - sprite.w()) / 2;
    dst_pt.y = box_rt.y + text.h() + spacer;
    sprite.Blit(dst_pt);

    text.Set(_("Resource cost:"), Font::BIG);
    dst_pt.x = box_rt.x + (box_rt.w - text.w()) / 2;
    dst_pt.y = dst_pt.y + sprite.h() + spacer;
    text.Blit(dst_pt);

    rbs.SetPos(box_rt.x, dst_pt.y + spacer);
    rbs.Redraw();

    // buttons
    dst_pt.x = box_rt.x;
    dst_pt.y = box_rt.y + box_rt.h - AGG::GetICN(system, 1).h();
    Button button1(dst_pt.x, dst_pt.y, system, 1, 2);

    dst_pt.x = box_rt.x + box_rt.w - AGG::GetICN(system, 3).w();
    dst_pt.y = box_rt.y + box_rt.h - AGG::GetICN(system, 3).h();
    Button button2(dst_pt.x, dst_pt.y, system, 3, 4);

    if(!enable)
    {
	button1.Press();
	button1.SetDisable(true);
    }

    button1.Draw();
    button2.Draw();

    cursor.Show();
    display.Flip();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while(le.HandleEvents())
    {
        if(button1.isEnable()) le.MousePressLeft(button1) ? button1.PressDraw() : button1.ReleaseDraw();
        le.MousePressLeft(button2) ? button2.PressDraw() : button2.ReleaseDraw();

        if(button1.isEnable() &&
	    (Game::HotKeyPressEvent(Game::EVENT_DEFAULT_READY) ||le.MouseClickLeft(button1))) return Dialog::OK;

        if(Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT) ||
    	    le.MouseClickLeft(button2)) return Dialog::CANCEL;
    }

    return Dialog::ZERO;
}
