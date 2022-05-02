/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
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

#include <string>

namespace Game
{
    enum HotKeyEvent : int32_t
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
        MAIN_MENU_HOTSEAT,
        MAIN_MENU_BATTLEONLY,
        MAIN_MENU_NEW_CAMPAIGN_SELECTION_SUCCESSION_WARS,
        MAIN_MENU_NEW_CAMPAIGN_SELECTION_PRICE_OF_LOYALTY,
        NEW_ROLAND_CAMPAIGN,
        NEW_ARCHIBALD_CAMPAIGN,
        NEW_PRICE_OF_LOYALTY_CAMPAIGN,
        NEW_VOYAGE_HOME_CAMPAIGN,
        NEW_WIZARDS_ISLE_CAMPAIGN,
        NEW_DESCENDANTS_CAMPAIGN,

        DEFAULT_READY,
        DEFAULT_EXIT,
        DEFAULT_LEFT,
        DEFAULT_RIGHT,
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

        BATTLE_RETREAT,
        BATTLE_SURRENDER,
        BATTLE_AUTOSWITCH,
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

    std::string getHotKeyNameByEventId( const HotKeyEvent eventID );

    void KeyboardGlobalFilter( int sym, int mod );

    void HotKeysLoad( std::string filename );
}

#define HotKeyCloseWindow ( Game::HotKeyPressEvent( Game::DEFAULT_EXIT ) || Game::HotKeyPressEvent( Game::DEFAULT_READY ) )
