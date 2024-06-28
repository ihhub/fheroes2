/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "game_hotkeys.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <type_traits>
#include <utility>

#include "battle_arena.h"
#include "dialog.h"
#include "game_interface.h"
#include "game_language.h"
#include "interface_gamearea.h"
#include "localevent.h"
#include "logging.h"
#include "players.h"
#include "settings.h"
#include "system.h"
#include "tinyconfig.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_language.h"

namespace
{
    struct HotKeyEventInfo
    {
        HotKeyEventInfo() = default;

        HotKeyEventInfo( const Game::HotKeyCategory category_, const char * name_, const fheroes2::Key key_ )
            : category( category_ )
            , name( name_ )
            , key( key_ )
        {
            // Do nothing.
        }

        HotKeyEventInfo( const HotKeyEventInfo & ) = default;
        HotKeyEventInfo( HotKeyEventInfo && ) = default;
        ~HotKeyEventInfo() = default;

        HotKeyEventInfo & operator=( const HotKeyEventInfo & ) = default;
        HotKeyEventInfo & operator=( HotKeyEventInfo && ) = default;

        Game::HotKeyCategory category = Game::HotKeyCategory::DEFAULT;

        const char * name = "";

        fheroes2::Key key = fheroes2::Key::NONE;
    };

    constexpr typename std::underlying_type<Game::HotKeyEvent>::type hotKeyEventToInt( Game::HotKeyEvent value )
    {
        return static_cast<typename std::underlying_type<Game::HotKeyEvent>::type>( value );
    }

    std::array<HotKeyEventInfo, hotKeyEventToInt( Game::HotKeyEvent::NO_EVENT )> hotKeyEventInfo;

    void initializeHotKeyEvents()
    {
        // Make sure that event name is unique!
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_OKAY )]
            = { Game::HotKeyCategory::DEFAULT, gettext_noop( "hotkey|default okay event" ), fheroes2::Key::KEY_ENTER };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_CANCEL )]
            = { Game::HotKeyCategory::DEFAULT, gettext_noop( "hotkey|default cancel event" ), fheroes2::Key::KEY_ESCAPE };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_LEFT )]
            = { Game::HotKeyCategory::DEFAULT, gettext_noop( "hotkey|default left" ), fheroes2::Key::KEY_LEFT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_RIGHT )]
            = { Game::HotKeyCategory::DEFAULT, gettext_noop( "hotkey|default right" ), fheroes2::Key::KEY_RIGHT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_UP )]
            = { Game::HotKeyCategory::DEFAULT, gettext_noop( "hotkey|default up" ), fheroes2::Key::KEY_UP };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_DOWN )]
            = { Game::HotKeyCategory::DEFAULT, gettext_noop( "hotkey|default down" ), fheroes2::Key::KEY_DOWN };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::GLOBAL_TOGGLE_FULLSCREEN )]
            = { Game::HotKeyCategory::GLOBAL, gettext_noop( "hotkey|toggle fullscreen" ), fheroes2::Key::KEY_F4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::GLOBAL_TOGGLE_TEXT_SUPPORT_MODE )]
            = { Game::HotKeyCategory::GLOBAL, gettext_noop( "hotkey|toggle text support mode" ), fheroes2::Key::KEY_F10 };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_NEW_GAME )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|new game" ), fheroes2::Key::KEY_N };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|load game" ), fheroes2::Key::KEY_L };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_HIGHSCORES )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|high scores" ), fheroes2::Key::KEY_H };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_CREDITS )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|credits" ), fheroes2::Key::KEY_C };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_STANDARD )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|standard game" ), fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_CAMPAIGN )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|campaign game" ), fheroes2::Key::KEY_C };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MULTI )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|multi-player game" ), fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_SETTINGS )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|settings" ), fheroes2::Key::KEY_T };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_QUIT )] = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|quit" ), fheroes2::Key::KEY_Q };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_SELECT_MAP )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|select map" ), fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_SMALL )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|select small map size" ), fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_MEDIUM )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|select medium map size" ), fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_LARGE )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|select large map size" ), fheroes2::Key::KEY_L };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_EXTRA_LARGE )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|select extra large map size" ), fheroes2::Key::KEY_X };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_ALL )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|select all map sizes" ), fheroes2::Key::KEY_A };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_HOTSEAT )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|hot seat game" ), fheroes2::Key::KEY_H };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_BATTLEONLY )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|battle only game" ), fheroes2::Key::KEY_B };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_NEW_ORIGINAL_CAMPAIGN )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|choose the original campaign" ), fheroes2::Key::KEY_O };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_NEW_EXPANSION_CAMPAIGN )]
            = { Game::HotKeyCategory::MAIN_MENU, gettext_noop( "hotkey|choose the expansion campaign" ), fheroes2::Key::KEY_E };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::EDITOR_MAIN_MENU )]
            = { Game::HotKeyCategory::EDITOR, gettext_noop( "hotkey|map editor main menu" ), fheroes2::Key::KEY_E };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::EDITOR_NEW_MAP_MENU )]
            = { Game::HotKeyCategory::EDITOR, gettext_noop( "hotkey|new map menu" ), fheroes2::Key::KEY_N };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::EDITOR_LOAD_MAP_MENU )]
            = { Game::HotKeyCategory::EDITOR, gettext_noop( "hotkey|load map menu" ), fheroes2::Key::KEY_L };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::EDITOR_FROM_SCRATCH_MAP_MENU )]
            = { Game::HotKeyCategory::EDITOR, gettext_noop( "hotkey|new map from scratch" ), fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::EDITOR_RANDOM_MAP_MENU )]
            = { Game::HotKeyCategory::EDITOR, gettext_noop( "hotkey|new random map" ), fheroes2::Key::KEY_R };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::EDITOR_UNDO_LAST_ACTION )]
            = { Game::HotKeyCategory::EDITOR, gettext_noop( "hotkey|undo last action" ), fheroes2::Key::KEY_U };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::EDITOR_REDO_LAST_ACTION )]
            = { Game::HotKeyCategory::EDITOR, gettext_noop( "hotkey|redo last action" ), fheroes2::Key::KEY_R };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::EDITOR_TO_GAME_MAIN_MENU )]
            = { Game::HotKeyCategory::EDITOR, gettext_noop( "hotkey|open game main menu" ), fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::EDITOR_TOGGLE_PASSABILITY )]
            = { Game::HotKeyCategory::EDITOR, gettext_noop( "hotkey|toggle passability" ), fheroes2::Key::KEY_P };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_ROLAND )]
            = { Game::HotKeyCategory::CAMPAIGN, gettext_noop( "hotkey|roland campaign" ), fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_ARCHIBALD )]
            = { Game::HotKeyCategory::CAMPAIGN, gettext_noop( "hotkey|archibald campaign" ), fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_PRICE_OF_LOYALTY )]
            = { Game::HotKeyCategory::CAMPAIGN, gettext_noop( "hotkey|price of loyalty campaign" ), fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_VOYAGE_HOME )]
            = { Game::HotKeyCategory::CAMPAIGN, gettext_noop( "hotkey|voyage home campaign" ), fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_WIZARDS_ISLE )]
            = { Game::HotKeyCategory::CAMPAIGN, gettext_noop( "hotkey|wizard's isle campaign" ), fheroes2::Key::KEY_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_DESCENDANTS )]
            = { Game::HotKeyCategory::CAMPAIGN, gettext_noop( "hotkey|descendants campaign" ), fheroes2::Key::KEY_4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_SELECT_FIRST_BONUS )]
            = { Game::HotKeyCategory::CAMPAIGN, gettext_noop( "hotkey|select first campaign bonus" ), fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_SELECT_SECOND_BONUS )]
            = { Game::HotKeyCategory::CAMPAIGN, gettext_noop( "hotkey|select second campaign bonus" ), fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_SELECT_THIRD_BONUS )]
            = { Game::HotKeyCategory::CAMPAIGN, gettext_noop( "hotkey|select third campaign bonus" ), fheroes2::Key::KEY_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_VIEW_INTRO )]
            = { Game::HotKeyCategory::CAMPAIGN, gettext_noop( "hotkey|view campaign intro" ), fheroes2::Key::KEY_V };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_SELECT_DIFFICULTY )]
            = { Game::HotKeyCategory::CAMPAIGN, gettext_noop( "hotkey|select campaign difficulty" ), fheroes2::Key::KEY_D };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_RESTART_SCENARIO )]
            = { Game::HotKeyCategory::CAMPAIGN, gettext_noop( "hotkey|restart campaign scenario" ), fheroes2::Key::KEY_R };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_LEFT )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|world map left" ), fheroes2::Key::KEY_KP_4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_RIGHT )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|world map right" ), fheroes2::Key::KEY_KP_6 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_UP )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|world map up" ), fheroes2::Key::KEY_KP_8 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_DOWN )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|world map down" ), fheroes2::Key::KEY_KP_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_UP_LEFT )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|world map up left" ), fheroes2::Key::KEY_KP_7 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_UP_RIGHT )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|world map up right" ), fheroes2::Key::KEY_KP_9 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_DOWN_LEFT )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|world map down left" ), fheroes2::Key::KEY_KP_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_DOWN_RIGHT )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|world map down right" ), fheroes2::Key::KEY_KP_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SAVE_GAME )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|save game" ), fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_NEXT_HERO )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|next hero" ), fheroes2::Key::KEY_H };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_QUICK_SELECT_HERO )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|change to hero under cursor" ), fheroes2::Key::KEY_LEFT_SHIFT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_START_HERO_MOVEMENT )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|start hero movement" ), fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_CAST_SPELL )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|cast adventure spell" ), fheroes2::Key::KEY_C };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SLEEP_HERO )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|put hero to sleep" ), fheroes2::Key::KEY_Z };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_NEXT_TOWN )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|next town" ), fheroes2::Key::KEY_T };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_END_TURN )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|end turn" ), fheroes2::Key::KEY_E };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_FILE_OPTIONS )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|file options" ), fheroes2::Key::KEY_F };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_ADVENTURE_OPTIONS )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|adventure options" ), fheroes2::Key::KEY_A };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_PUZZLE_MAP )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|puzzle map" ), fheroes2::Key::KEY_P };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SCENARIO_INFORMATION )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|scenario information" ), fheroes2::Key::KEY_I };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_DIG_ARTIFACT )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|dig for artifact" ), fheroes2::Key::KEY_D };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_VIEW_WORLD )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|view world" ), fheroes2::Key::KEY_V };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_KINGDOM_SUMMARY )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|kingdom summary" ), fheroes2::Key::KEY_K };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_DEFAULT_ACTION )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|default action" ), fheroes2::Key::KEY_SPACE };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_OPEN_FOCUS )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|open focus" ), fheroes2::Key::KEY_ENTER };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SYSTEM_OPTIONS )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|system options" ), fheroes2::Key::KEY_O };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SCROLL_LEFT )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|scroll left" ), fheroes2::Key::KEY_LEFT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SCROLL_RIGHT )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|scroll right" ), fheroes2::Key::KEY_RIGHT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SCROLL_UP )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|scroll up" ), fheroes2::Key::KEY_UP };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SCROLL_DOWN )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|scroll down" ), fheroes2::Key::KEY_DOWN };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_TOGGLE_CONTROL_PANEL )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|toggle control panel" ), fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_TOGGLE_RADAR )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|toggle radar" ), fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_TOGGLE_BUTTONS )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|toggle buttons" ), fheroes2::Key::KEY_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_TOGGLE_STATUS )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|toggle status" ), fheroes2::Key::KEY_4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_TOGGLE_ICONS )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|toggle icons" ), fheroes2::Key::KEY_5 };

#if defined( WITH_DEBUG )
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_TRANSFER_CONTROL_TO_AI )]
            = { Game::HotKeyCategory::WORLD_MAP, gettext_noop( "hotkey|transfer control to ai" ), fheroes2::Key::KEY_F8 };
#endif

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_RETREAT )]
            = { Game::HotKeyCategory::BATTLE, gettext_noop( "hotkey|retreat from battle" ), fheroes2::Key::KEY_R };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_SURRENDER )]
            = { Game::HotKeyCategory::BATTLE, gettext_noop( "hotkey|surrender during battle" ), fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_AUTO_SWITCH )]
            = { Game::HotKeyCategory::BATTLE, gettext_noop( "hotkey|toggle battle auto mode" ), fheroes2::Key::KEY_A };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_AUTO_FINISH )]
            = { Game::HotKeyCategory::BATTLE, gettext_noop( "hotkey|finish the battle in auto mode" ), fheroes2::Key::KEY_Q };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_OPTIONS )]
            = { Game::HotKeyCategory::BATTLE, gettext_noop( "hotkey|battle options" ), fheroes2::Key::KEY_O };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_SKIP )]
            = { Game::HotKeyCategory::BATTLE, gettext_noop( "hotkey|skip turn in battle" ), fheroes2::Key::KEY_SPACE };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_CAST_SPELL )]
            = { Game::HotKeyCategory::BATTLE, gettext_noop( "hotkey|cast battle spell" ), fheroes2::Key::KEY_C };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_1 )]
            = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|dwelling level 1" ), fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_2 )]
            = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|dwelling level 2" ), fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_3 )]
            = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|dwelling level 3" ), fheroes2::Key::KEY_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_4 )]
            = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|dwelling level 4" ), fheroes2::Key::KEY_4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_5 )]
            = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|dwelling level 5" ), fheroes2::Key::KEY_5 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_6 )]
            = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|dwelling level 6" ), fheroes2::Key::KEY_6 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_WELL )] = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|well" ), fheroes2::Key::KEY_W };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_MARKETPLACE )]
            = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|marketplace" ), fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_MAGE_GUILD )]
            = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|mage guild" ), fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_SHIPYARD )] = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|shipyard" ), fheroes2::Key::KEY_N };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_THIEVES_GUILD )]
            = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|thieves guild" ), fheroes2::Key::KEY_T };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_TAVERN )] = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|tavern" ), fheroes2::Key::KEY_R };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_CONSTRUCTION )]
            = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|construction screen" ), fheroes2::Key::KEY_B };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_WELL_BUY_ALL )]
            = { Game::HotKeyCategory::TOWN, gettext_noop( "hotkey|buy all monsters in well" ), fheroes2::Key::KEY_M };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::ARMY_SPLIT_STACK_BY_HALF )]
            = { Game::HotKeyCategory::ARMY, gettext_noop( "hotkey|split stack by half" ), fheroes2::Key::KEY_LEFT_SHIFT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::ARMY_SPLIT_STACK_BY_ONE )]
            = { Game::HotKeyCategory::ARMY, gettext_noop( "hotkey|split stack by one" ), fheroes2::Key::KEY_LEFT_CONTROL };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::ARMY_JOIN_STACKS )]
            = { Game::HotKeyCategory::ARMY, gettext_noop( "hotkey|join stacks" ), fheroes2::Key::KEY_LEFT_ALT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::ARMY_UPGRADE_TROOP )]
            = { Game::HotKeyCategory::ARMY, gettext_noop( "hotkey|upgrade troop" ), fheroes2::Key::KEY_U };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::ARMY_DISMISS )]
            = { Game::HotKeyCategory::ARMY, gettext_noop( "hotkey|dismiss hero or troop" ), fheroes2::Key::KEY_D };
    }

    std::string getHotKeyFileContent()
    {
        std::ostringstream os;
        os << "# fheroes2 hotkey file (saved by version " << Settings::GetVersion() << ")" << std::endl;
        os << std::endl;

        Game::HotKeyCategory currentCategory = hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::NONE ) + 1].category;
        os << "# " << getHotKeyCategoryName( currentCategory ) << ':' << std::endl;

#if defined( WITH_DEBUG )
        std::set<const char *> duplicationStringVerifier;
#endif

        const fheroes2::LanguageSwitcher languageSwitcher( fheroes2::SupportedLanguage::English );

        for ( int32_t eventId = hotKeyEventToInt( Game::HotKeyEvent::NONE ) + 1; eventId < hotKeyEventToInt( Game::HotKeyEvent::NO_EVENT ); ++eventId ) {
            if ( currentCategory != hotKeyEventInfo[eventId].category ) {
                currentCategory = hotKeyEventInfo[eventId].category;
                os << std::endl;
                os << "# " << getHotKeyCategoryName( currentCategory ) << ':' << std::endl;
            }

            const char * eventName = _( hotKeyEventInfo[eventId].name );
            assert( strlen( eventName ) > 0 );
#if defined( WITH_DEBUG )
            const bool isUnique = duplicationStringVerifier.emplace( eventName ).second;
            assert( isUnique );
#endif

            os << eventName << " = " << StringUpper( KeySymGetName( hotKeyEventInfo[eventId].key ) ) << std::endl;
        }

        return os.str();
    }
}

bool Game::HotKeyPressEvent( const HotKeyEvent eventID )
{
    const LocalEvent & le = LocalEvent::Get();
    if ( le.isAnyKeyPressed() ) {
        // We should disable the fast scroll, because the cursor might be on one of the borders when a dialog gets dismissed.
        Interface::AdventureMap::Get().getGameArea().setFastScrollStatus( false );
    }
    return le.isAnyKeyPressed() && le.getPressedKeyValue() == hotKeyEventInfo[hotKeyEventToInt( eventID )].key;
}

bool Game::HotKeyHoldEvent( const HotKeyEvent eventID )
{
    const LocalEvent & le = LocalEvent::Get();
    return le.isKeyBeingHold() && le.getPressedKeyValue() == hotKeyEventInfo[hotKeyEventToInt( eventID )].key;
}

fheroes2::Key Game::getHotKeyForEvent( const HotKeyEvent eventID )
{
    return hotKeyEventInfo[hotKeyEventToInt( eventID )].key;
}

void Game::setHotKeyForEvent( const HotKeyEvent eventID, const fheroes2::Key key )
{
    hotKeyEventInfo[hotKeyEventToInt( eventID )].key = key;
}

std::string Game::getHotKeyNameByEventId( const HotKeyEvent eventID )
{
    return StringUpper( KeySymGetName( hotKeyEventInfo[hotKeyEventToInt( eventID )].key ) );
}

const char * Game::getHotKeyEventNameByEventId( const HotKeyEvent eventID )
{
    return hotKeyEventInfo[hotKeyEventToInt( eventID )].name;
}

std::vector<std::pair<Game::HotKeyEvent, Game::HotKeyCategory>> Game::getAllHotKeyEvents()
{
    std::vector<std::pair<Game::HotKeyEvent, Game::HotKeyCategory>> events;
    events.reserve( hotKeyEventInfo.size() - 1 );

    const bool disableEditor = !Settings::Get().isPriceOfLoyaltySupported();

    for ( size_t i = 1; i < hotKeyEventInfo.size(); ++i ) {
        if ( disableEditor && ( hotKeyEventInfo[i].category == HotKeyCategory::EDITOR ) ) {
            continue;
        }

        events.emplace_back( static_cast<Game::HotKeyEvent>( i ), hotKeyEventInfo[i].category );
    }

    return events;
}

void Game::HotKeysLoad( const std::string & filename )
{
    initializeHotKeyEvents();

    bool isFilePresent = System::IsFile( filename );
    if ( isFilePresent ) {
        TinyConfig config( '=', '#' );
        isFilePresent = config.Load( filename );

        if ( isFilePresent ) {
            std::map<std::string, fheroes2::Key, std::less<>> nameToKey;
            for ( int32_t i = static_cast<int32_t>( fheroes2::Key::NONE ); i < static_cast<int32_t>( fheroes2::Key::LAST_KEY ); ++i ) {
                const fheroes2::Key key = static_cast<fheroes2::Key>( i );
                nameToKey.try_emplace( StringUpper( KeySymGetName( key ) ), key );
            }

            const fheroes2::LanguageSwitcher languageSwitcher( fheroes2::SupportedLanguage::English );

            for ( int eventId = hotKeyEventToInt( HotKeyEvent::NONE ) + 1; eventId < hotKeyEventToInt( HotKeyEvent::NO_EVENT ); ++eventId ) {
                const char * eventName = _( hotKeyEventInfo[eventId].name );
                std::string value = config.StrParams( eventName );
                if ( value.empty() ) {
                    continue;
                }

                value = StringUpper( value );
                auto foundKey = nameToKey.find( value );
                if ( foundKey == nameToKey.end() ) {
                    continue;
                }

                hotKeyEventInfo[eventId].key = foundKey->second;
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Event '" << eventName << "' has key '" << value << "'" )
            }
        }
    }

    HotKeySave();
}

void Game::HotKeySave()
{
    // Save the latest information into the file.
    const std::string filename = System::concatPath( System::GetConfigDirectory( "fheroes2" ), "fheroes2.key" );

    std::fstream file( filename.data(), std::fstream::out | std::fstream::trunc );
    if ( !file ) {
        ERROR_LOG( "Unable to open hotkey settings file " << filename )
        return;
    }

    const std::string & data = getHotKeyFileContent();
    file.write( data.data(), data.size() );
}

void Game::globalKeyDownEvent( const fheroes2::Key key, const int32_t modifier )
{
    if ( ( modifier & fheroes2::KeyModifier::KEY_MODIFIER_ALT ) || ( modifier & fheroes2::KeyModifier::KEY_MODIFIER_CTRL ) ) {
        return;
    }

    Settings & conf = Settings::Get();

    if ( key == hotKeyEventInfo[hotKeyEventToInt( HotKeyEvent::GLOBAL_TOGGLE_FULLSCREEN )].key ) {
        conf.setFullScreen( !conf.FullScreen() );
        conf.Save( Settings::configFileName );
    }
    else if ( key == hotKeyEventInfo[hotKeyEventToInt( HotKeyEvent::GLOBAL_TOGGLE_TEXT_SUPPORT_MODE )].key ) {
        conf.setTextSupportMode( !conf.isTextSupportModeEnabled() );
        conf.Save( Settings::configFileName );
    }
#if defined( WITH_DEBUG )
    else if ( key == hotKeyEventInfo[hotKeyEventToInt( HotKeyEvent::WORLD_TRANSFER_CONTROL_TO_AI )].key ) {
        static bool recursiveCall = false;

        if ( !recursiveCall ) {
            class RecursionGuard
            {
            public:
                explicit RecursionGuard( bool & rc )
                    : _recursiveCall( rc )
                {
                    _recursiveCall = true;
                }

                RecursionGuard( const RecursionGuard & ) = delete;

                ~RecursionGuard()
                {
                    _recursiveCall = false;
                }

                RecursionGuard & operator=( const RecursionGuard & ) = delete;

            private:
                bool & _recursiveCall;
            };

            const RecursionGuard recursionGuard( recursiveCall );

            Player * player = Settings::Get().GetPlayers().GetCurrent();

            // Do not allow to transfer control to/from AI during battle
            if ( player && ( player->isControlHuman() || player->isAIAutoControlMode() ) && Battle::GetArena() == nullptr ) {
                if ( player->isAIAutoControlMode() ) {
                    if ( fheroes2::showStandardTextMessage( _( "Warning" ),
                                                            _( "Do you want to regain control from AI? The effect will take place only on the next turn." ),
                                                            Dialog::YES | Dialog::NO )
                         == Dialog::YES ) {
                        player->setAIAutoControlMode( false );
                    }
                }
                else {
                    if ( fheroes2::showStandardTextMessage( _( "Warning" ),
                                                            _( "Do you want to transfer control from you to the AI? The effect will take place only on the next turn." ),
                                                            Dialog::YES | Dialog::NO )
                         == Dialog::YES ) {
                        player->setAIAutoControlMode( true );
                    }
                }
            }
        }
    }
#endif
}

const char * Game::getHotKeyCategoryName( const HotKeyCategory category )
{
    switch ( category ) {
    case HotKeyCategory::DEFAULT:
        return gettext_noop( "Default Actions" );
    case HotKeyCategory::GLOBAL:
        return gettext_noop( "Global Actions" );
    case HotKeyCategory::MAIN_MENU:
        return gettext_noop( "Main Menu" );
    case HotKeyCategory::CAMPAIGN:
        return gettext_noop( "Campaign" );
    case HotKeyCategory::WORLD_MAP:
        return gettext_noop( "World Map" );
    case HotKeyCategory::BATTLE:
        return gettext_noop( "Battle Screen" );
    case HotKeyCategory::TOWN:
        return gettext_noop( "Town Screen" );
    case HotKeyCategory::ARMY:
        return gettext_noop( "Army Actions" );
    case HotKeyCategory::EDITOR:
        return gettext_noop( "Editor" );
    default:
        // Did you add a new category? Add the logic above!
        assert( 0 );
        break;
    }

    return "";
}
