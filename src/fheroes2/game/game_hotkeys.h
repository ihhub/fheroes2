/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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

        MAIN_MENU_NEW_GAME,
        MAIN_MENU_LOAD_GAME,
        MAIN_MENU_HIGHSCORES,
        MAIN_MENU_CREDITS,
        MAIN_MENU_STANDARD,
        MAIN_MENU_CAMPAIGN,
        MAIN_MENU_MULTI,
        MAIN_MENU_SETTINGS,
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
        NEW_ROLAND_CAMPAIGN,
        NEW_ARCHIBALD_CAMPAIGN,
        NEW_PRICE_OF_LOYALTY_CAMPAIGN,
        NEW_VOYAGE_HOME_CAMPAIGN,
        NEW_WIZARDS_ISLE_CAMPAIGN,
        NEW_DESCENDANTS_CAMPAIGN,
        CAMPAIGN_SELECT_FIRST_BONUS,
        CAMPAIGN_SELECT_SECOND_BONUS,
        CAMPAIGN_SELECT_THIRD_BONUS,
        CAMPAIGN_VIEW_INTRO,
        CAMPAIGN_SELECT_DIFFICULTY,
        CAMPAIGN_RESTART_SCENARIO,

        DEFAULT_OKAY,
        DEFAULT_CANCEL,
        MOVE_LEFT,
        MOVE_RIGHT,
        MOVE_TOP,
        MOVE_BOTTOM,
        MOVE_TOP_LEFT,
        MOVE_TOP_RIGHT,
        MOVE_BOTTOM_LEFT,
        MOVE_BOTTOM_RIGHT,
        SYSTEM_FULLSCREEN,

        SAVE_GAME,
        NEXT_HERO,
        CONTINUE_HERO_MOVEMENT,
        CAST_SPELL,
        SLEEP_HERO,
        NEXT_TOWN,
        END_TURN,
        FILE_OPTIONS,
        ADVENTURE_OPTIONS,
        PUZZLE_MAP,
        SCENARIO_INFORMATION,
        DIG_ARTIFACT,
        VIEW_WORLD,
        KINGDOM_SUMMARY,
        DEFAULT_ACTION,
        OPEN_FOCUS,
        SYSTEM_OPTIONS,
        SCROLL_LEFT,
        SCROLL_RIGHT,
        SCROLL_UP,
        SCROLL_DOWN,
        CONTROL_PANEL,
        SHOW_RADAR,
        SHOW_BUTTONS,
        SHOW_STATUS,
        SHOW_ICONS,

#if defined( WITH_DEBUG )
        // This hotkey is only for debug mode as of now.
        TRANSFER_CONTROL_TO_AI,
#endif

        BATTLE_RETREAT,
        BATTLE_SURRENDER,
        BATTLE_AUTO_SWITCH,
        BATTLE_AUTO_FINISH,
        BATTLE_OPTIONS,
        BATTLE_SKIP,
        BATTLE_WAIT,

        SPLIT_STACK_BY_HALF,
        SPLIT_STACK_BY_ONE,
        JOIN_STACKS,
        UPGRADE_TROOP,
        DISMISS_TROOP,

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
        // town screen exclusive, not applied to build screen!
        TOWN_TAVERN,
        TOWN_JUMP_TO_BUILD_SELECTION,
        WELL_BUY_ALL_CREATURES,

        // WARNING! Put all new event only above this line. No adding in between.
        NO_EVENT,
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

    std::string getHotKeyEventNameByEventId( const HotKeyEvent eventID );

    std::vector<Game::HotKeyEvent> getAllHotKeyEvents();

    void globalKeyboardEvent( const fheroes2::Key key, const int32_t modifier );

    void HotKeysLoad( const std::string & filename );

    void HotKeySave();
}
