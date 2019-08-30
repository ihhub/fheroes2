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
#include "heroes.h"
#include "game.h"
#include "dialog.h"

void DialogPrimaryOnly(const std::string & name, const std::string & primary)
{
    std::string message = _("%{name} has gained a level.");
    message.append("\n \n");
    message.append(_("%{skill} Skill +1"));
    StringReplace(message, "%{name}", name);
    StringReplace(message, "%{skill}", primary);
    Dialog::Message("", message, Font::BIG, Dialog::OK);
}

int DialogOneSecondary(const std::string & name, const std::string & primary, const Skill::Secondary & sec)
{
    std::string message = _("%{name} has gained a level.");
    message.append("\n \n");
    message.append(_("%{skill} Skill +1"));
    StringReplace(message, "%{name}", name);
    StringReplace(message, "%{skill}", primary);

    message.append("\n \n");
    message.append(_("You have learned %{skill}."));
    StringReplace(message, "%{skill}", sec.GetName());

    const Sprite & sprite_frame = AGG::GetICN(ICN::SECSKILL, 15);
    Surface sf = sprite_frame.GetSurface();

    // sprite
    const Sprite & sprite_skill = AGG::GetICN(ICN::SECSKILL, sec.GetIndexSprite1());
    sprite_skill.Blit(3, 3, sf);
    // text
    Text text_skill(Skill::Secondary::String(sec.Skill()), Font::SMALL);
    text_skill.Blit(3 + (sprite_skill.w() - text_skill.w()) / 2, 6, sf);
    Text text_level(Skill::Level::String(sec.Level()), Font::SMALL);
    text_level.Blit(3 + (sprite_skill.w() - text_level.w()) / 2, sprite_skill.h() - 12, sf);

    Dialog::SpriteInfo("", message, sf);

    return sec.Skill();
}

int DialogSelectSecondary(const std::string & name, const std::string & primary, const Skill::Secondary & sec1, const Skill::Secondary & sec2, Heroes & hero)
{
    std::string header = _("%{name} has gained a level.");
    header.append("\n \n");
    header.append(_("%{skill} Skill +1"));
    StringReplace(header, "%{name}", name);
    StringReplace(header, "%{skill}", primary);

    Display & display = Display::Get();
    const int system = Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM;

    // cursor
    Cursor & cursor = Cursor::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    const Sprite &sprite_frame = AGG::GetICN(ICN::SECSKILL, 15);
    const Sprite & sprite_skill1 = AGG::GetICN(ICN::SECSKILL, sec1.GetIndexSprite1());
    const Sprite & sprite_skill2 = AGG::GetICN(ICN::SECSKILL, sec2.GetIndexSprite1());

    Point pt;
    std::string message = _("You may learn either:");
    message.append("\n%{skill1}\n");
    message.append(" ");
    message.append(_("or"));
    message.append(" ");
    message.append("\n%{skill2}");
    StringReplace(message, "%{skill1}", sec1.GetName());
    StringReplace(message, "%{skill2}", sec2.GetName());

    TextBox box1(header, Font::BIG, BOXAREA_WIDTH);
    TextBox box2(message, Font::BIG, BOXAREA_WIDTH);
    const int spacer = Settings::Get().QVGA() ? 5 : 10;

    Dialog::FrameBox box(box1.h() + spacer + box2.h() + 10 + sprite_frame.h(), true);

    pt.x = box.GetArea().x + box.GetArea().w / 2 - AGG::GetICN(system, 9).w() - 20;
    pt.y = box.GetArea().y + box.GetArea().h - AGG::GetICN(system, 9).h();
    Button button_learn1(pt.x, pt.y, system, 9, 10);

    pt.x = box.GetArea().x + box.GetArea().w / 2 + 20;
    pt.y = box.GetArea().y + box.GetArea().h - AGG::GetICN(system, 9).h();
    Button button_learn2(pt.x, pt.y, system, 9, 10);

    Rect pos = box.GetArea();

    if(header.size()) box1.Blit(pos);
    pos.y += box1.h() + spacer;

    if(message.size()) box2.Blit(pos);
    pos.y += box2.h() + spacer;

    // sprite1
    pos.x = box.GetArea().x + box.GetArea().w / 2 - sprite_frame.w() - 20;
    sprite_frame.Blit(pos);
    pos.x += 3;
    Rect rect_image1(pos, sprite_skill1.w(), sprite_skill1.h());
    sprite_skill1.Blit(pos.x, pos.y + 3);

    Text text;
    // text
    text.Set(Skill::Secondary::String(sec1.Skill()), Font::SMALL);
    text.Blit(pos.x + (sprite_skill1.w() - text.w()) / 2, pos.y + 5);
    text.Set(Skill::Level::String(sec1.Level()), Font::SMALL);
    text.Blit(pos.x + (sprite_skill1.w() - text.w()) / 2, pos.y + sprite_skill1.h() - 12);

    // sprite2
    pos.x = box.GetArea().x + box.GetArea().w / 2 + 20;
    sprite_frame.Blit(pos);
    pos.x += 3;
    Rect rect_image2(pos, sprite_skill2.w(), sprite_skill2.h());
    sprite_skill2.Blit(pos.x, pos.y + 3);
    // text
    Text name_skill2(Skill::Secondary::String(sec2.Skill()), Font::SMALL);
    name_skill2.Blit(pos.x + (sprite_skill2.w() - name_skill2.w()) / 2, pos.y + 5);
    Text name_level2(Skill::Level::String(sec2.Level()), Font::SMALL);
    name_level2.Blit(pos.x + (sprite_skill2.w() - name_level2.w()) / 2, pos.y + sprite_skill2.h() - 12);

    // hero button
    pt.x = box.GetArea().x + box.GetArea().w / 2 - 18;
    pt.y = box.GetArea().y + box.GetArea().h - 36;
    Button button_hero(pt.x, pt.y, (Settings::Get().ExtGameEvilInterface() ? ICN::ADVEBTNS : ICN::ADVBTNS), 0, 1);
    text.Set(GetString(HEROESMAXSKILL) +"/" + GetString(hero.GetSecondarySkills().Count()), Font::BIG);
    text.Blit(box.GetArea().x + (box.GetArea().w - text.w()) / 2, pt.y - 15);

    button_learn1.Draw();
    button_learn2.Draw();
    button_hero.Draw();

    cursor.Show();
    display.Flip();
    LocalEvent & le = LocalEvent::Get();

    // message loop
    while(le.HandleEvents())
    {
	le.MousePressLeft(button_learn1) ? button_learn1.PressDraw() : button_learn1.ReleaseDraw();
	le.MousePressLeft(button_learn2) ? button_learn2.PressDraw() : button_learn2.ReleaseDraw();
	le.MousePressLeft(button_hero) ? button_hero.PressDraw() : button_hero.ReleaseDraw();

        if(le.MouseClickLeft(button_learn1) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_LEFT)) return sec1.Skill();
        else
	if(le.MouseClickLeft(button_learn2) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_RIGHT)) return sec2.Skill();
	else
	if(le.MouseClickLeft(button_hero) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_READY))
	    { hero.OpenDialog(true /* read only */, false); cursor.Show(); display.Flip(); }

	if(le.MouseClickLeft(rect_image1))
	    { cursor.Hide(); Dialog::SecondarySkillInfo(sec1); cursor.Show(); display.Flip(); }
	else
	if(le.MouseClickLeft(rect_image2))
	    { cursor.Hide(); Dialog::SecondarySkillInfo(sec2); cursor.Show(); display.Flip(); }

	if(le.MousePressRight(rect_image1))
	    { cursor.Hide(); Dialog::SecondarySkillInfo(sec1, false); cursor.Show(); display.Flip(); }
	else
	if(le.MousePressRight(rect_image2))
	    { cursor.Hide(); Dialog::SecondarySkillInfo(sec2, false); cursor.Show(); display.Flip(); }
    }

    cursor.Hide();
    return Skill::Secondary::UNKNOWN;
}

int Dialog::LevelUpSelectSkill(const std::string & name, const std::string & primary, const Skill::Secondary & sec1, const Skill::Secondary & sec2, Heroes & hero)
{
    int result = Skill::Secondary::UNKNOWN;

    if(Skill::Secondary::UNKNOWN == sec1.Skill() && Skill::Secondary::UNKNOWN == sec2.Skill())
	DialogPrimaryOnly(name, primary);
    else
    if(Skill::Secondary::UNKNOWN == sec1.Skill() || Skill::Secondary::UNKNOWN == sec2.Skill())
	result = DialogOneSecondary(name, primary, (Skill::Secondary::UNKNOWN == sec2.Skill() ? sec1 : sec2));
    else
	result = DialogSelectSecondary(name, primary, sec1, sec2, hero);

    return result;
}

