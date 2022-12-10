/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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
#include <map>
#include <set>
#include <type_traits>
#include <utility>

#include "localevent.h"
#include "logging.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "tinyconfig.h"
#include "tools.h"

namespace
{
    enum class HotKeyCategory : uint8_t
    {
        DEFAULT_ACTIONS,
        MAIN_MENU,
        CAMPAIGN,
        WORLD_MAP,
        BATTLE,
        TOWN,
        ARMY
    };

    const char * getHotKeyCategoryName( const HotKeyCategory category )
    {
        switch ( category ) {
        case HotKeyCategory::DEFAULT_ACTIONS:
            return "Default actions";
        case HotKeyCategory::MAIN_MENU:
            return "Main Menu";
        case HotKeyCategory::CAMPAIGN:
            return "Campaign";
        case HotKeyCategory::WORLD_MAP:
            return "World Map";
        case HotKeyCategory::BATTLE:
            return "Battle";
        case HotKeyCategory::TOWN:
            return "Town";
        case HotKeyCategory::ARMY:
            return "Army";
        default:
            // Did you add a new category? Add the logic above!
            assert( 0 );
            break;
        }

        return "";
    }

    struct HotKeyEventInfo
    {
        HotKeyEventInfo() = default;

        HotKeyEventInfo( const HotKeyCategory category_, const char * name_, const fheroes2::Key key_ )
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

        HotKeyCategory category = HotKeyCategory::DEFAULT_ACTIONS;

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
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_OKAY )] = { HotKeyCategory::DEFAULT_ACTIONS, "default okay event", fheroes2::Key::KEY_ENTER };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_CANCEL )] = { HotKeyCategory::DEFAULT_ACTIONS, "default cancel event", fheroes2::Key::KEY_ESCAPE };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_LEFT )] = { HotKeyCategory::DEFAULT_ACTIONS, "default left", fheroes2::Key::KEY_LEFT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_RIGHT )] = { HotKeyCategory::DEFAULT_ACTIONS, "default right", fheroes2::Key::KEY_RIGHT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_UP )] = { HotKeyCategory::DEFAULT_ACTIONS, "default up", fheroes2::Key::KEY_UP };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_DOWN )] = { HotKeyCategory::DEFAULT_ACTIONS, "default down", fheroes2::Key::KEY_DOWN };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SYSTEM_FULLSCREEN )] = { HotKeyCategory::DEFAULT_ACTIONS, "toggle fullscreen", fheroes2::Key::KEY_F4 };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_NEW_GAME )] = { HotKeyCategory::MAIN_MENU, "new game", fheroes2::Key::KEY_N };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME )] = { HotKeyCategory::MAIN_MENU, "load game", fheroes2::Key::KEY_L };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_HIGHSCORES )] = { HotKeyCategory::MAIN_MENU, "highscores", fheroes2::Key::KEY_H };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_CREDITS )] = { HotKeyCategory::MAIN_MENU, "credits", fheroes2::Key::KEY_C };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_STANDARD )] = { HotKeyCategory::MAIN_MENU, "standard game", fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_CAMPAIGN )] = { HotKeyCategory::MAIN_MENU, "campaign game", fheroes2::Key::KEY_C };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MULTI )] = { HotKeyCategory::MAIN_MENU, "multi-player game", fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_SETTINGS )] = { HotKeyCategory::MAIN_MENU, "settings", fheroes2::Key::KEY_T };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_QUIT )] = { HotKeyCategory::MAIN_MENU, "quit", fheroes2::Key::KEY_Q };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_SELECT_MAP )] = { HotKeyCategory::MAIN_MENU, "select map", fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_SMALL )] = { HotKeyCategory::MAIN_MENU, "select small map size", fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_MEDIUM )] = { HotKeyCategory::MAIN_MENU, "select medium map size", fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_LARGE )] = { HotKeyCategory::MAIN_MENU, "select large map size", fheroes2::Key::KEY_L };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_EXTRA_LARGE )]
            = { HotKeyCategory::MAIN_MENU, "select extra large map size", fheroes2::Key::KEY_X };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_ALL )] = { HotKeyCategory::MAIN_MENU, "select all map sizes", fheroes2::Key::KEY_A };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_HOTSEAT )] = { HotKeyCategory::MAIN_MENU, "hotseat game", fheroes2::Key::KEY_H };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_BATTLEONLY )] = { HotKeyCategory::MAIN_MENU, "battle only game", fheroes2::Key::KEY_B };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_NEW_ORIGINAL_CAMPAIGN )]
            = { HotKeyCategory::MAIN_MENU, "choose the original campaign", fheroes2::Key::KEY_O };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_NEW_EXPANSION_CAMPAIGN )]
            = { HotKeyCategory::MAIN_MENU, "choose the expansion campaign", fheroes2::Key::KEY_E };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_NEW_ROLAND )] = { HotKeyCategory::CAMPAIGN, "roland campaign", fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_NEW_ARCHIBALD )] = { HotKeyCategory::CAMPAIGN, "archibald campaign", fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_NEW_PRICE_OF_LOYALTY )]
            = { HotKeyCategory::CAMPAIGN, "the price of loyalty campaign", fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_NEW_VOYAGE_HOME )] = { HotKeyCategory::CAMPAIGN, "voyage home campaign", fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_NEW_WIZARDS_ISLE )] = { HotKeyCategory::CAMPAIGN, "wizard's isle campaign", fheroes2::Key::KEY_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_NEW_DESCENDANTS )] = { HotKeyCategory::CAMPAIGN, "descendants campaign", fheroes2::Key::KEY_4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_SELECT_FIRST_BONUS )]
            = { HotKeyCategory::CAMPAIGN, "select first campaign bonus", fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_SELECT_SECOND_BONUS )]
            = { HotKeyCategory::CAMPAIGN, "select second campaign bonus", fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_SELECT_THIRD_BONUS )]
            = { HotKeyCategory::CAMPAIGN, "select third campaign bonus", fheroes2::Key::KEY_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_VIEW_INTRO )] = { HotKeyCategory::CAMPAIGN, "view campaign intro", fheroes2::Key::KEY_V };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_SELECT_DIFFICULTY )]
            = { HotKeyCategory::CAMPAIGN, "select campaign difficulty", fheroes2::Key::KEY_D };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_RESTART_SCENARIO )]
            = { HotKeyCategory::CAMPAIGN, "restart campaign scenario", fheroes2::Key::KEY_R };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_MOVE_HERO_LEFT )] = { HotKeyCategory::WORLD_MAP, "move hero left", fheroes2::Key::KEY_KP_4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_MOVE_HERO_RIGHT )] = { HotKeyCategory::WORLD_MAP, "move hero right", fheroes2::Key::KEY_KP_6 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_MOVE_HERO_TOP )] = { HotKeyCategory::WORLD_MAP, "move hero top", fheroes2::Key::KEY_KP_8 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_MOVE_HERO_BOTTOM )] = { HotKeyCategory::WORLD_MAP, "move hero bottom", fheroes2::Key::KEY_KP_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_MOVE_HERO_TOP_LEFT )] = { HotKeyCategory::WORLD_MAP, "move hero top left", fheroes2::Key::KEY_KP_7 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_MOVE_HERO_TOP_RIGHT )] = { HotKeyCategory::WORLD_MAP, "move hero top right", fheroes2::Key::KEY_KP_9 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_MOVE_HERO_BOTTOM_LEFT )]
            = { HotKeyCategory::WORLD_MAP, "move hero bottom left", fheroes2::Key::KEY_KP_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_MOVE_HERO_BOTTOM_RIGHT )]
            = { HotKeyCategory::WORLD_MAP, "move hero bottom right", fheroes2::Key::KEY_KP_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SAVE_GAME )] = { HotKeyCategory::WORLD_MAP, "save game", fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_NEXT_HERO )] = { HotKeyCategory::WORLD_MAP, "next hero", fheroes2::Key::KEY_H };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_CONTINUE_HERO_MOVEMENT )]
            = { HotKeyCategory::WORLD_MAP, "continue hero movement", fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_CAST_SPELL )] = { HotKeyCategory::WORLD_MAP, "cast adventure spell", fheroes2::Key::KEY_C };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SLEEP_HERO )] = { HotKeyCategory::WORLD_MAP, "put hero to sleep", fheroes2::Key::KEY_Z };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_NEXT_TOWN )] = { HotKeyCategory::WORLD_MAP, "next town", fheroes2::Key::KEY_T };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_END_TURN )] = { HotKeyCategory::WORLD_MAP, "end turn", fheroes2::Key::KEY_E };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_FILE_OPTIONS )] = { HotKeyCategory::WORLD_MAP, "file options", fheroes2::Key::KEY_F };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_ADVENTURE_OPTIONS )] = { HotKeyCategory::WORLD_MAP, "adventure options", fheroes2::Key::KEY_A };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SYSTEM_OPTIONS )] = { HotKeyCategory::WORLD_MAP, "system options", fheroes2::Key::KEY_O };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_PUZZLE_MAP )] = { HotKeyCategory::WORLD_MAP, "puzzle map", fheroes2::Key::KEY_P };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SCENARIO_INFORMATION )] = { HotKeyCategory::WORLD_MAP, "scenario information", fheroes2::Key::KEY_I };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_DIG_ARTIFACT )] = { HotKeyCategory::WORLD_MAP, "dig for artifact", fheroes2::Key::KEY_D };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_KINGDOM_SUMMARY )] = { HotKeyCategory::WORLD_MAP, "kingdom summary", fheroes2::Key::KEY_K };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_VIEW_WORLD )] = { HotKeyCategory::WORLD_MAP, "view world", fheroes2::Key::KEY_V };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_DEFAULT_ACTION )] = { HotKeyCategory::WORLD_MAP, "default action", fheroes2::Key::KEY_SPACE };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_OPEN_FOCUS )] = { HotKeyCategory::WORLD_MAP, "open focus", fheroes2::Key::KEY_ENTER };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_TOGGLE_CONTROL_PANEL )] = { HotKeyCategory::WORLD_MAP, "toggle control panel", fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_TOGGLE_RADAR )] = { HotKeyCategory::WORLD_MAP, "toggle radar", fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_TOGGLE_BUTTONS )] = { HotKeyCategory::WORLD_MAP, "toggle buttons", fheroes2::Key::KEY_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_TOGGLE_STATUS )] = { HotKeyCategory::WORLD_MAP, "toggle status", fheroes2::Key::KEY_4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_TOGGLE_ICONS )] = { HotKeyCategory::WORLD_MAP, "toggle icons", fheroes2::Key::KEY_5 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SCROLL_LEFT )] = { HotKeyCategory::WORLD_MAP, "scroll left", fheroes2::Key::KEY_LEFT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SCROLL_RIGHT )] = { HotKeyCategory::WORLD_MAP, "scroll right", fheroes2::Key::KEY_RIGHT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SCROLL_UP )] = { HotKeyCategory::WORLD_MAP, "scroll up", fheroes2::Key::KEY_UP };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_SCROLL_DOWN )] = { HotKeyCategory::WORLD_MAP, "scroll down", fheroes2::Key::KEY_DOWN };

#if defined( WITH_DEBUG )
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WORLD_TRANSFER_CONTROL_TO_AI )]
            = { HotKeyCategory::WORLD_MAP, "transfer control to ai", fheroes2::Key::KEY_F8 };
#endif

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_RETREAT )] = { HotKeyCategory::BATTLE, "retreat from battle", fheroes2::Key::KEY_R };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_SURRENDER )] = { HotKeyCategory::BATTLE, "surrender during battle", fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_AUTO_SWITCH )] = { HotKeyCategory::BATTLE, "toggle battle auto mode", fheroes2::Key::KEY_A };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_AUTO_FINISH )] = { HotKeyCategory::BATTLE, "finish the battle in auto mode", fheroes2::Key::KEY_Q };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_OPTIONS )] = { HotKeyCategory::BATTLE, "battle options", fheroes2::Key::KEY_O };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_SKIP )] = { HotKeyCategory::BATTLE, "skip turn in battle", fheroes2::Key::KEY_SPACE };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_CAST_SPELL )] = { HotKeyCategory::BATTLE, "cast battle spell", fheroes2::Key::KEY_C };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_1 )] = { HotKeyCategory::TOWN, "town dwelling level 1", fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_2 )] = { HotKeyCategory::TOWN, "town dwelling level 2", fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_3 )] = { HotKeyCategory::TOWN, "town dwelling level 3", fheroes2::Key::KEY_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_4 )] = { HotKeyCategory::TOWN, "town dwelling level 4", fheroes2::Key::KEY_4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_5 )] = { HotKeyCategory::TOWN, "town dwelling level 5", fheroes2::Key::KEY_5 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_6 )] = { HotKeyCategory::TOWN, "town dwelling level 6", fheroes2::Key::KEY_6 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_WELL )] = { HotKeyCategory::TOWN, "well", fheroes2::Key::KEY_W };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_MAGE_GUILD )] = { HotKeyCategory::TOWN, "mage guild", fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_MARKETPLACE )] = { HotKeyCategory::TOWN, "marketplace", fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_THIEVES_GUILD )] = { HotKeyCategory::TOWN, "thieves guild", fheroes2::Key::KEY_T };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_SHIPYARD )] = { HotKeyCategory::TOWN, "shipyard", fheroes2::Key::KEY_N };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_TAVERN )] = { HotKeyCategory::TOWN, "tavern", fheroes2::Key::KEY_R };
        // It is also used to build castle in a town.
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_JUMP_TO_BUILD_SELECTION )] = { HotKeyCategory::TOWN, "castle construction", fheroes2::Key::KEY_B };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_WELL_BUY_ALL_CREATURES )] = { HotKeyCategory::TOWN, "buy all monsters in well", fheroes2::Key::KEY_M };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::ARMY_SPLIT_STACK_BY_HALF )] = { HotKeyCategory::ARMY, "split stack by half", fheroes2::Key::KEY_SHIFT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::ARMY_SPLIT_STACK_BY_ONE )] = { HotKeyCategory::ARMY, "split stack by one", fheroes2::Key::KEY_CONTROL };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::ARMY_JOIN_STACKS )] = { HotKeyCategory::ARMY, "join stacks", fheroes2::Key::KEY_ALT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::ARMY_UPGRADE_TROOP )] = { HotKeyCategory::ARMY, "upgrade troop", fheroes2::Key::KEY_U };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::ARMY_DISMISS_TROOP )] = { HotKeyCategory::ARMY, "dismiss troop", fheroes2::Key::KEY_D };
    }

    std::string getHotKeyFileContent()
    {
        std::ostringstream os;
        os << "# fheroes2 hotkey file (saved by version " << Settings::GetVersion() << ")" << std::endl;
        os << std::endl;

        HotKeyCategory currentCategory = hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::NONE ) + 1].category;
        os << "# " << getHotKeyCategoryName( currentCategory ) << ':' << std::endl;

#if defined( WITH_DEBUG )
        std::set<const char *> duplicationStringVerifier;
#endif

        for ( int32_t eventId = hotKeyEventToInt( Game::HotKeyEvent::NONE ) + 1; eventId < hotKeyEventToInt( Game::HotKeyEvent::NO_EVENT ); ++eventId ) {
            if ( currentCategory != hotKeyEventInfo[eventId].category ) {
                currentCategory = hotKeyEventInfo[eventId].category;
                os << std::endl;
                os << "# " << getHotKeyCategoryName( currentCategory ) << ':' << std::endl;
            }

            assert( strlen( hotKeyEventInfo[eventId].name ) > 0 );
#if defined( WITH_DEBUG )
            const bool isUnique = duplicationStringVerifier.emplace( hotKeyEventInfo[eventId].name ).second;
            assert( isUnique );
#endif

            os << hotKeyEventInfo[eventId].name << " = " << StringUpper( KeySymGetName( hotKeyEventInfo[eventId].key ) ) << std::endl;
        }

        return os.str();
    }
}

bool Game::HotKeyPressEvent( const HotKeyEvent eventID )
{
    const LocalEvent & le = LocalEvent::Get();
    return le.KeyPress() && le.KeyValue() == hotKeyEventInfo[hotKeyEventToInt( eventID )].key;
}

bool Game::HotKeyHoldEvent( const HotKeyEvent eventID )
{
    const LocalEvent & le = LocalEvent::Get();
    return le.KeyHold() && le.KeyValue() == hotKeyEventInfo[hotKeyEventToInt( eventID )].key;
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

std::string Game::getHotKeyEventNameByEventId( const HotKeyEvent eventID )
{
    return hotKeyEventInfo[hotKeyEventToInt( eventID )].name;
}

std::vector<Game::HotKeyEvent> Game::getAllHotKeyEvents()
{
    std::vector<Game::HotKeyEvent> events;
    events.reserve( hotKeyEventInfo.size() - 2 );

    for ( size_t i = 1; i < hotKeyEventInfo.size() - 1; ++i ) {
        events.emplace_back( static_cast<Game::HotKeyEvent>( i ) );
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
            std::map<std::string, fheroes2::Key> nameToKey;
            for ( int32_t i = static_cast<int32_t>( fheroes2::Key::NONE ); i < static_cast<int32_t>( fheroes2::Key::LAST_KEY ); ++i ) {
                const fheroes2::Key key = static_cast<fheroes2::Key>( i );
                nameToKey.try_emplace( StringUpper( KeySymGetName( key ) ), key );
            }

            for ( int eventId = hotKeyEventToInt( HotKeyEvent::NONE ) + 1; eventId < hotKeyEventToInt( HotKeyEvent::NO_EVENT ); ++eventId ) {
                const char * eventName = hotKeyEventInfo[eventId].name;
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
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Event '" << hotKeyEventInfo[eventId].name << "' has key '" << value << "'" )
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
    if ( key == hotKeyEventInfo[hotKeyEventToInt( HotKeyEvent::SYSTEM_FULLSCREEN )].key && !( modifier & fheroes2::KeyModifier::KEY_MODIFIER_ALT )
         && !( modifier & fheroes2::KeyModifier::KEY_MODIFIER_CTRL ) ) {
        Settings & conf = Settings::Get();
        conf.setFullScreen( !fheroes2::engine().isFullScreen() );
        conf.Save( Settings::configFileName );
    }
}
