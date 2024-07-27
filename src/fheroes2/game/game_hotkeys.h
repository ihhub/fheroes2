/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2024                                             *
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

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace fheroes2
{
    enum class Key : int32_t;
}

namespace Game
{
    enum class HotKeyEvent : int32_t
    {
        NONE,

        DEFAULT_OKAY,
        DEFAULT_CANCEL,
        DEFAULT_LEFT,
        DEFAULT_RIGHT,
        DEFAULT_UP,
        DEFAULT_DOWN,

        GLOBAL_TOGGLE_FULLSCREEN,
        GLOBAL_TOGGLE_TEXT_SUPPORT_MODE,

        MAIN_MENU_NEW_GAME,
        MAIN_MENU_LOAD_GAME,
        MAIN_MENU_HIGHSCORES,
        MAIN_MENU_CREDITS,
        MAIN_MENU_STANDARD,
        MAIN_MENU_CAMPAIGN,
        MAIN_MENU_MULTI,
        MAIN_MENU_SETTINGS,
        MAIN_MENU_QUIT,
        MAIN_MENU_SELECT_MAP,
        MAIN_MENU_MAP_SIZE_SMALL,
        MAIN_MENU_MAP_SIZE_MEDIUM,
        MAIN_MENU_MAP_SIZE_LARGE,
        MAIN_MENU_MAP_SIZE_EXTRA_LARGE,
        MAIN_MENU_MAP_SIZE_ALL,
        MAIN_MENU_HOTSEAT,
        MAIN_MENU_BATTLEONLY,
        MAIN_MENU_NEW_ORIGINAL_CAMPAIGN,
        MAIN_MENU_NEW_EXPANSION_CAMPAIGN,

        EDITOR_MAIN_MENU,
        EDITOR_NEW_MAP_MENU,
        EDITOR_LOAD_MAP_MENU,
        EDITOR_FROM_SCRATCH_MAP_MENU,
        EDITOR_RANDOM_MAP_MENU,
        EDITOR_UNDO_LAST_ACTION,
        EDITOR_REDO_LAST_ACTION,
        EDITOR_TO_GAME_MAIN_MENU,
        EDITOR_TOGGLE_PASSABILITY,

        CAMPAIGN_ROLAND,
        CAMPAIGN_ARCHIBALD,
        CAMPAIGN_PRICE_OF_LOYALTY,
        CAMPAIGN_VOYAGE_HOME,
        CAMPAIGN_WIZARDS_ISLE,
        CAMPAIGN_DESCENDANTS,
        CAMPAIGN_SELECT_FIRST_BONUS,
        CAMPAIGN_SELECT_SECOND_BONUS,
        CAMPAIGN_SELECT_THIRD_BONUS,
        CAMPAIGN_VIEW_INTRO,
        CAMPAIGN_SELECT_DIFFICULTY,
        CAMPAIGN_RESTART_SCENARIO,

        // Move the hero if there is a selected one, otherwise scroll the adventure map.
        WORLD_LEFT,
        WORLD_RIGHT,
        WORLD_UP,
        WORLD_DOWN,
        WORLD_UP_LEFT,
        WORLD_UP_RIGHT,
        WORLD_DOWN_LEFT,
        WORLD_DOWN_RIGHT,
        //
        WORLD_SAVE_GAME,
        WORLD_NEXT_HERO,
        WORLD_QUICK_SELECT_HERO,
        WORLD_START_HERO_MOVEMENT,
        WORLD_CAST_SPELL,
        WORLD_SLEEP_HERO,
        WORLD_NEXT_TOWN,
        WORLD_END_TURN,
        WORLD_FILE_OPTIONS,
        WORLD_ADVENTURE_OPTIONS,
        WORLD_PUZZLE_MAP,
        WORLD_SCENARIO_INFORMATION,
        WORLD_DIG_ARTIFACT,
        WORLD_VIEW_WORLD,
        WORLD_KINGDOM_SUMMARY,
        WORLD_DEFAULT_ACTION,
        WORLD_OPEN_FOCUS,
        WORLD_SYSTEM_OPTIONS,
        // Scroll the adventure map regardless of the selected object.
        WORLD_SCROLL_LEFT,
        WORLD_SCROLL_RIGHT,
        WORLD_SCROLL_UP,
        WORLD_SCROLL_DOWN,
        //
        WORLD_TOGGLE_CONTROL_PANEL,
        WORLD_TOGGLE_RADAR,
        WORLD_TOGGLE_BUTTONS,
        WORLD_TOGGLE_STATUS,
        WORLD_TOGGLE_ICONS,

#if defined( WITH_DEBUG )
        // This hotkey is only for debug mode as of now.
        WORLD_TRANSFER_CONTROL_TO_AI,
#endif

        BATTLE_RETREAT,
        BATTLE_SURRENDER,
        BATTLE_AUTO_SWITCH,
        BATTLE_AUTO_FINISH,
        BATTLE_OPTIONS,
        BATTLE_SKIP,
        BATTLE_CAST_SPELL,

        TOWN_DWELLING_LEVEL_1,
        TOWN_DWELLING_LEVEL_2,
        TOWN_DWELLING_LEVEL_3,
        TOWN_DWELLING_LEVEL_4,
        TOWN_DWELLING_LEVEL_5,
        TOWN_DWELLING_LEVEL_6,
        TOWN_WELL,
        TOWN_MARKETPLACE,
        TOWN_MAGE_GUILD,
        TOWN_SHIPYARD,
        TOWN_THIEVES_GUILD,
        TOWN_TAVERN,
        // Also used to build a castle in the town.
        TOWN_CONSTRUCTION,
        TOWN_WELL_BUY_ALL,

        ARMY_SPLIT_STACK_BY_HALF,
        ARMY_SPLIT_STACK_BY_ONE,
        ARMY_JOIN_STACKS,
        ARMY_UPGRADE_TROOP,
        ARMY_DISMISS,

        // WARNING! Put all new event only above this line. No adding in between.
        NO_EVENT,
    };

    enum class HotKeyCategory : uint8_t
    {
        DEFAULT,
        GLOBAL,
        MAIN_MENU,
        CAMPAIGN,
        WORLD_MAP,
        BATTLE,
        TOWN,
        ARMY,
        EDITOR,
    };

    bool HotKeyPressEvent( const HotKeyEvent eventID );
    bool HotKeyHoldEvent( const HotKeyEvent eventID );

    fheroes2::Key getHotKeyForEvent( const HotKeyEvent eventID );
    void setHotKeyForEvent( const HotKeyEvent eventID, const fheroes2::Key key );

    inline bool HotKeyCloseWindow()
    {
        return HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) || HotKeyPressEvent( HotKeyEvent::DEFAULT_OKAY );
    }

    std::string getHotKeyNameByEventId( const HotKeyEvent eventID );

    const char * getHotKeyEventNameByEventId( const HotKeyEvent eventID );

    std::vector<std::pair<HotKeyEvent, HotKeyCategory>> getAllHotKeyEvents();

    void globalKeyDownEvent( const fheroes2::Key key, const int32_t modifier );

    void HotKeysLoad( const std::string & filename );

    void HotKeySave();

    const char * getHotKeyCategoryName( const HotKeyCategory category );
}
