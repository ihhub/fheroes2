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

#include <algorithm>
#include "agg.h"
#include "text.h"
#include "settings.h"
#include "cursor.h"
#include "button.h"
#include "dialog.h"
#include "interface_list.h"

class SettingsListBox : public Interface::ListBox<u32>
{
public:
    SettingsListBox(const Point & pt, bool f) : Interface::ListBox<u32>(pt), readonly(f) {};

    void RedrawItem(const u32 &, s32, s32, bool);
    void RedrawBackground(const Point &);

    void ActionCurrentUp(void){};
    void ActionCurrentDn(void){};
    void ActionListDoubleClick(u32 &);
    void ActionListSingleClick(u32 &);
    void ActionListPressRight(u32 &){};

    bool readonly;
};

void SettingsListBox::RedrawItem(const u32 & item, s32 ox, s32 oy, bool current)
{
    const Settings & conf = Settings::Get();

    const Sprite & cell = AGG::GetICN(ICN::CELLWIN, 1);
    const Sprite & mark = AGG::GetICN(ICN::CELLWIN, 2);

    cell.Blit(ox, oy);
    if(conf.ExtModes(item)) mark.Blit(ox + 3, oy + 2);

    TextBox msg(conf.ExtName(item), Font::SMALL, 250);
    msg.SetAlign(ALIGN_LEFT);

    if(1 < msg.row())
	msg.Blit(ox + cell.w() + 5, oy - 1);
    else
	msg.Blit(ox + cell.w() + 5, oy + 4);
}

void SettingsListBox::RedrawBackground(const Point & top)
{
    const Settings & conf = Settings::Get();

    const int window_h = conf.QVGA() ? 224 : 400;
    const int ah = window_h - 54;

    AGG::GetICN(ICN::STONEBAK, 0).Blit(Rect(15, 25, 280, ah), top.x + 15, top.y + 25);

    for(int ii = 1; ii < (window_h / 25); ++ii)
	AGG::GetICN(ICN::DROPLISL, 11).Blit(top.x + 295, top.y + 35 + (19 * ii));

    AGG::GetICN(ICN::DROPLISL, 10).Blit(top.x + 295, top.y + 46);
    AGG::GetICN(ICN::DROPLISL, 12).Blit(top.x + 295, top.y + ah - 14);
}

void SettingsListBox::ActionListDoubleClick(u32 & item)
{
    ActionListSingleClick(item);
}

void SettingsListBox::ActionListSingleClick(u32 & item)
{
    Settings & conf = Settings::Get();

    if(!readonly || conf.CanChangeInGame(item))
    {
	conf.ExtModes(item) ? conf.ExtResetModes(item) : conf.ExtSetModes(item);

	// depends
	switch(item)
	{
	    case Settings::WORLD_1HERO_HIRED_EVERY_WEEK:
		conf.ExtResetModes(Settings::CASTLE_1HERO_HIRED_EVERY_WEEK);
		break;

	    case Settings::CASTLE_1HERO_HIRED_EVERY_WEEK:
		conf.ExtResetModes(Settings::WORLD_1HERO_HIRED_EVERY_WEEK);
		break;

	    case Settings::GAME_AUTOSAVE_BEGIN_DAY:
		if(conf.ExtModes(Settings::GAME_AUTOSAVE_BEGIN_DAY))
		    conf.ExtSetModes(Settings::GAME_AUTOSAVE_ON);
		else
		    conf.ExtResetModes(Settings::GAME_AUTOSAVE_ON);
		break;

	    case Settings::WORLD_GUARDIAN_TWO_DEFENSE:
		if(conf.ExtModes(Settings::WORLD_GUARDIAN_TWO_DEFENSE))
		    conf.ExtSetModes(Settings::WORLD_ALLOW_SET_GUARDIAN);
		else
		    conf.ExtResetModes(Settings::WORLD_ALLOW_SET_GUARDIAN);
		break;

	    case Settings::WORLD_NEW_VERSION_WEEKOF:
		if(conf.ExtModes(Settings::WORLD_NEW_VERSION_WEEKOF))
		    conf.ExtSetModes(Settings::WORLD_BAN_WEEKOF);
		else
		    conf.ExtResetModes(Settings::WORLD_BAN_WEEKOF);
		break;

	    default: break;
	}
    }
}

void Dialog::ExtSettings(bool readonly)
{
    Display & display = Display::Get();
    const Settings & conf = Settings::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    const int window_h = conf.QVGA() ? 224 : 400;
    Dialog::FrameBorder frameborder(Size(320, window_h));
    const Rect & area = frameborder.GetArea();

    Text text("FHeroes2 Settings", Font::YELLOW_BIG);
    text.Blit(area.x + (area.w - text.w()) / 2, area.y + 6);

    std::vector<u32> states;
    states.reserve(64);

    states.push_back(Settings::GAME_SAVE_REWRITE_CONFIRM);
    states.push_back(Settings::GAME_ALSO_CONFIRM_AUTOSAVE);
    states.push_back(Settings::GAME_REMEMBER_LAST_FOCUS);
    states.push_back(Settings::GAME_SHOW_SYSTEM_INFO);
    states.push_back(Settings::GAME_EVIL_INTERFACE);
    states.push_back(Settings::GAME_BATTLE_SHOW_GRID);
    states.push_back(Settings::GAME_BATTLE_SHOW_MOUSE_SHADOW);
    states.push_back(Settings::GAME_BATTLE_SHOW_MOVE_SHADOW);
    states.push_back(Settings::GAME_BATTLE_SHOW_DAMAGE);

    if(! conf.QVGA())
    {
	states.push_back(Settings::GAME_CASTLE_FLASH_BUILDING);
	states.push_back(Settings::GAME_HIDE_INTERFACE);
    }

    if(!conf.PocketPC())
	states.push_back(Settings::GAME_DYNAMIC_INTERFACE);

    states.push_back(Settings::GAME_AUTOSAVE_ON);
    states.push_back(Settings::GAME_AUTOSAVE_BEGIN_DAY);

    if(conf.VideoMode().w == 640 && conf.VideoMode().h == 480)
	states.push_back(Settings::GAME_USE_FADE);

#ifdef BUILD_RELEASE
    states.push_back(Settings::GAME_SHOW_SDL_LOGO);
#endif
    states.push_back(Settings::GAME_CONTINUE_AFTER_VICTORY);
    states.push_back(Settings::WORLD_SHOW_VISITED_CONTENT);
    states.push_back(Settings::WORLD_ABANDONED_MINE_RANDOM);
    states.push_back(Settings::WORLD_SAVE_MONSTER_BATTLE);
    states.push_back(Settings::WORLD_ALLOW_SET_GUARDIAN);
    states.push_back(Settings::WORLD_GUARDIAN_TWO_DEFENSE);
    states.push_back(Settings::WORLD_EXT_OBJECTS_CAPTURED);
    states.push_back(Settings::WORLD_NOREQ_FOR_ARTIFACTS);
    states.push_back(Settings::WORLD_SCOUTING_EXTENDED);
    states.push_back(Settings::WORLD_ARTSPRING_SEPARATELY_VISIT);
    states.push_back(Settings::WORLD_ARTIFACT_CRYSTAL_BALL);
    states.push_back(Settings::WORLD_ONLY_FIRST_MONSTER_ATTACK);
    states.push_back(Settings::WORLD_EYE_EAGLE_AS_SCHOLAR);
    states.push_back(Settings::WORLD_BAN_WEEKOF);
    states.push_back(Settings::WORLD_NEW_VERSION_WEEKOF);
    states.push_back(Settings::WORLD_BAN_PLAGUES);
    states.push_back(Settings::WORLD_BAN_MONTHOF_MONSTERS);
    states.push_back(Settings::WORLD_STARTHERO_LOSSCOND4HUMANS);
    states.push_back(Settings::WORLD_1HERO_HIRED_EVERY_WEEK);
    states.push_back(Settings::CASTLE_1HERO_HIRED_EVERY_WEEK);
    states.push_back(Settings::WORLD_DWELLING_ACCUMULATE_UNITS);
    states.push_back(Settings::WORLD_USE_UNIQUE_ARTIFACTS_ML);
    states.push_back(Settings::WORLD_USE_UNIQUE_ARTIFACTS_RS);
    states.push_back(Settings::WORLD_USE_UNIQUE_ARTIFACTS_PS);
    states.push_back(Settings::WORLD_USE_UNIQUE_ARTIFACTS_SS);
    states.push_back(Settings::WORLD_DISABLE_BARROW_MOUNDS);
    states.push_back(Settings::HEROES_BUY_BOOK_FROM_SHRINES);
    states.push_back(Settings::HEROES_LEARN_SPELLS_WITH_DAY);
    states.push_back(Settings::HEROES_COST_DEPENDED_FROM_LEVEL);
    states.push_back(Settings::HEROES_REMEMBER_POINTS_RETREAT);
    states.push_back(Settings::HEROES_SURRENDERING_GIVE_EXP);
    states.push_back(Settings::HEROES_RECALCULATE_MOVEMENT);
    states.push_back(Settings::HEROES_PATROL_ALLOW_PICKUP);
    states.push_back(Settings::HEROES_AUTO_MOVE_BATTLE_DST);
    states.push_back(Settings::HEROES_TRANSCRIBING_SCROLLS);
    states.push_back(Settings::HEROES_ALLOW_BANNED_SECSKILLS);
    states.push_back(Settings::HEROES_ARENA_ANY_SKILLS);

    if(! conf.QVGA())
	states.push_back(Settings::CASTLE_ALLOW_BUY_FROM_WELL);

    states.push_back(Settings::CASTLE_ALLOW_GUARDIANS);
    states.push_back(Settings::CASTLE_MAGEGUILD_POINTS_TURN);
    states.push_back(Settings::CASTLE_ALLOW_RECRUITS_SPECIAL);

    states.push_back(Settings::UNIONS_ALLOW_HERO_MEETINGS);
    states.push_back(Settings::UNIONS_ALLOW_CASTLE_VISITING);

    states.push_back(Settings::BATTLE_SOFT_WAITING);
    states.push_back(Settings::BATTLE_OBJECTS_ARCHERS_PENALTY);
    states.push_back(Settings::BATTLE_MERGE_ARMIES);
    states.push_back(Settings::BATTLE_ARCHMAGE_RESIST_BAD_SPELL);
    states.push_back(Settings::BATTLE_MAGIC_TROOP_RESIST);
    states.push_back(Settings::BATTLE_SKIP_INCREASE_DEFENSE);
    states.push_back(Settings::BATTLE_REVERSE_WAIT_ORDER);

    if(conf.PocketPC())
    {
	states.push_back(Settings::POCKETPC_HIDE_CURSOR);
	states.push_back(Settings::POCKETPC_TAP_MODE);
	states.push_back(Settings::POCKETPC_LOW_MEMORY);
	states.push_back(Settings::POCKETPC_DRAG_DROP_SCROLL);
    }

    SettingsListBox listbox(area, readonly);

    const int ah = window_h - 60;

    listbox.RedrawBackground(area);
    listbox.SetScrollButtonUp(ICN::DROPLISL, 6, 7, Point(area.x + 295, area.y + 25));
    listbox.SetScrollButtonDn(ICN::DROPLISL, 8, 9, Point(area.x + 295, area.y + ah + 5));
    listbox.SetScrollSplitter(AGG::GetICN(ICN::DROPLISL, 13), Rect(area.x + 300, area.y + 49, 12, ah - 43));
    listbox.SetAreaMaxItems(ah / 40);
    listbox.SetAreaItems(Rect(area.x + 10, area.y + 30, 290, ah + 5));
    listbox.SetListContent(states);
    listbox.Redraw();

    LocalEvent & le = LocalEvent::Get();

    ButtonGroups btnGroups(area, Dialog::OK|Dialog::CANCEL);
    btnGroups.Draw();

    cursor.Show();
    display.Flip();

    // message loop
    int result = Dialog::ZERO;
    while(result == Dialog::ZERO && le.HandleEvents())
    {
	result = btnGroups.QueueEventProcessing();

	listbox.QueueEventProcessing();

	if(!cursor.isVisible())
	{
	    listbox.Redraw();
	    cursor.Show();
	    display.Flip();
	}
    }

    // store
    if(result == Dialog::OK)
    {
	le.SetTapMode(conf.ExtPocketTapMode());
	Settings::Get().BinarySave();
    }
}
