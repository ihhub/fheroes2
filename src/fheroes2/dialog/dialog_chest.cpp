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
#include "settings.h"
#include "cursor.h"
#include "button.h"
#include "world.h"
#include "game.h"
#include "dialog.h"

bool Dialog::SelectGoldOrExp(const std::string & header, const std::string & message, u32 gold, u32 expr, const  Heroes & hero)
{
    Display & display = Display::Get();
    const int system = Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM;

    // cursor
    Cursor & cursor = Cursor::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    const Sprite & sprite_gold = AGG::GetICN(ICN::RESOURCE, 6);
    const Sprite & sprite_expr = AGG::GetICN(ICN::EXPMRL, 4);

    Point pt;
    TextBox box1(header, Font::YELLOW_BIG, BOXAREA_WIDTH);
    TextBox box2(message, Font::BIG, BOXAREA_WIDTH);

    Text text;
    text.Set(GetString(gold) + " " + "(" + "total: " + GetString(world.GetKingdom(hero.GetColor()).GetFunds().Get(Resource::GOLD)) + ")", Font::SMALL);

    const int spacer = Settings::Get().QVGA() ? 5 : 10;
    FrameBox box(box1.h() + spacer + box2.h() + spacer + sprite_expr.h() + 2 + text.h(), true);

    pt.x = box.GetArea().x + box.GetArea().w / 2 - AGG::GetICN(system, 9).w() - 20;
    pt.y = box.GetArea().y + box.GetArea().h - AGG::GetICN(system, 5).h();
    Button button_yes(pt.x, pt.y, system, 5, 6);

    pt.x = box.GetArea().x + box.GetArea().w / 2 + 20;
    pt.y = box.GetArea().y + box.GetArea().h - AGG::GetICN(system, 7).h();
    Button button_no(pt.x, pt.y, system, 7, 8);

    Rect pos = box.GetArea();

    if(header.size()) box1.Blit(pos);
    pos.y += box1.h() + spacer;

    if(message.size()) box2.Blit(pos);
    pos.y += box2.h() + spacer;

    pos.y += sprite_expr.h();
    // sprite1
    pos.x = box.GetArea().x + box.GetArea().w / 2 - sprite_gold.w() - 30;
    sprite_gold.Blit(pos.x, pos.y - sprite_gold.h());
    // text
    text.Blit(pos.x + (sprite_gold.w() - text.w()) / 2, pos.y + 2);

    // sprite2
    pos.x = box.GetArea().x + box.GetArea().w / 2 + 30;
    sprite_expr.Blit(pos.x, pos.y - sprite_expr.h());
    // text
    text.Set(GetString(expr) + " " + "(" + "need: " + GetString(Heroes::GetExperienceFromLevel(hero.GetLevel()) - hero.GetExperience()) + ")", Font::SMALL);
    text.Blit(pos.x + (sprite_expr.w() - text.w()) / 2, pos.y + 2);

    button_yes.Draw();
    button_no.Draw();

    cursor.Show();
    display.Flip();
    LocalEvent & le = LocalEvent::Get();
    bool result = false;

    // message loop
    while(le.HandleEvents())
    {
	le.MousePressLeft(button_yes) ? button_yes.PressDraw() : button_yes.ReleaseDraw();
	le.MousePressLeft(button_no) ? button_no.PressDraw() : button_no.ReleaseDraw();

        if(Game::HotKeyPressEvent(Game::EVENT_DEFAULT_READY) || le.MouseClickLeft(button_yes)){ result = true; break; }
        if(Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT) || le.MouseClickLeft(button_no)){ result = false; break; }
    }

    cursor.Hide();

    return result;
}
