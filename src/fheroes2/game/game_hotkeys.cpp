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

#include <SDL_version.h>

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#include <SDL_keycode.h>
#else
#include <SDL_keysym.h>
#endif

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
        DEFAULT_EVENTS,
        MAIN_GAME,
        WORLD_MAP,
        BATTLE,
        CASTLE,
        MONSTER
    };

    const char * getHotKeyCategoryName( const HotKeyCategory category )
    {
        switch ( category ) {
        case HotKeyCategory::DEFAULT_EVENTS:
            return "Default actions";
        case HotKeyCategory::MAIN_GAME:
            return "Main Menu";
        case HotKeyCategory::WORLD_MAP:
            return "World Map";
        case HotKeyCategory::BATTLE:
            return "Battle";
        case HotKeyCategory::CASTLE:
            return "Castle";
        case HotKeyCategory::MONSTER:
            return "Monster";
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

        HotKeyCategory category = HotKeyCategory::DEFAULT_EVENTS;

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
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_NEW_GAME )] = { HotKeyCategory::MAIN_GAME, "new game", fheroes2::Key::KEY_N };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME )] = { HotKeyCategory::MAIN_GAME, "load game", fheroes2::Key::KEY_L };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_HIGHSCORES )] = { HotKeyCategory::MAIN_GAME, "highscores", fheroes2::Key::KEY_H };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_CREDITS )] = { HotKeyCategory::MAIN_GAME, "credits", fheroes2::Key::KEY_C };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_STANDARD )] = { HotKeyCategory::MAIN_GAME, "standard game", fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "campaign game", fheroes2::Key::KEY_C };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MULTI )] = { HotKeyCategory::MAIN_GAME, "multi-player game", fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_SETTINGS )] = { HotKeyCategory::MAIN_GAME, "settings", fheroes2::Key::KEY_T };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_SELECT_MAP )] = { HotKeyCategory::MAIN_GAME, "select map", fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_SMALL )] = { HotKeyCategory::MAIN_GAME, "select small map size", fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_MEDIUM )] = { HotKeyCategory::MAIN_GAME, "select medium map size", fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_LARGE )] = { HotKeyCategory::MAIN_GAME, "select large map size", fheroes2::Key::KEY_L };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_EXTRA_LARGE )]
            = { HotKeyCategory::MAIN_GAME, "select extra large map size", fheroes2::Key::KEY_X };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_ALL )] = { HotKeyCategory::MAIN_GAME, "select all map sizes", fheroes2::Key::KEY_A };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_HOTSEAT )] = { HotKeyCategory::MAIN_GAME, "hotseat game", fheroes2::Key::KEY_H };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_BATTLEONLY )] = { HotKeyCategory::MAIN_GAME, "battle only game", fheroes2::Key::KEY_B };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_NEW_ORIGINAL_CAMPAIGN )]
            = { HotKeyCategory::MAIN_GAME, "choose the original campaign", fheroes2::Key::KEY_O };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MAIN_MENU_NEW_EXPANSION_CAMPAIGN )]
            = { HotKeyCategory::MAIN_GAME, "choose the expansion campaign", fheroes2::Key::KEY_E };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::NEW_ROLAND_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "roland campaign", fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::NEW_ARCHIBALD_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "archibald campaign", fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::NEW_PRICE_OF_LOYALTY_CAMPAIGN )]
            = { HotKeyCategory::MAIN_GAME, "the price of loyalty campaign", fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::NEW_VOYAGE_HOME_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "voyage home campaign", fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::NEW_WIZARDS_ISLE_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "wizard's isle campaign", fheroes2::Key::KEY_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::NEW_DESCENDANTS_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "descendants campaign", fheroes2::Key::KEY_4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_SELECT_FIRST_BONUS )]
            = { HotKeyCategory::MAIN_GAME, "select first campaign bonus", fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_SELECT_SECOND_BONUS )]
            = { HotKeyCategory::MAIN_GAME, "select second campaign bonus", fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_SELECT_THIRD_BONUS )]
            = { HotKeyCategory::MAIN_GAME, "select third campaign bonus", fheroes2::Key::KEY_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_VIEW_INTRO )] = { HotKeyCategory::MAIN_GAME, "view campaign intro", fheroes2::Key::KEY_V };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_SELECT_DIFFICULTY )]
            = { HotKeyCategory::MAIN_GAME, "select campaign difficulty", fheroes2::Key::KEY_D };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAMPAIGN_RESTART_SCENARIO )]
            = { HotKeyCategory::MAIN_GAME, "restart campaign scenario", fheroes2::Key::KEY_R };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_OKAY )] = { HotKeyCategory::DEFAULT_EVENTS, "default okay event", fheroes2::Key::KEY_ENTER };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_CANCEL )] = { HotKeyCategory::DEFAULT_EVENTS, "default cancel event", fheroes2::Key::KEY_ESCAPE };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MOVE_LEFT )] = { HotKeyCategory::DEFAULT_EVENTS, "move left", fheroes2::Key::KEY_LEFT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MOVE_RIGHT )] = { HotKeyCategory::DEFAULT_EVENTS, "move right", fheroes2::Key::KEY_RIGHT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MOVE_TOP )] = { HotKeyCategory::DEFAULT_EVENTS, "move up", fheroes2::Key::KEY_UP };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MOVE_BOTTOM )] = { HotKeyCategory::DEFAULT_EVENTS, "move bottom", fheroes2::Key::KEY_DOWN };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MOVE_TOP_LEFT )] = { HotKeyCategory::DEFAULT_EVENTS, "move top left", fheroes2::Key::KEY_KP_7 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MOVE_TOP_RIGHT )] = { HotKeyCategory::DEFAULT_EVENTS, "move top right", fheroes2::Key::KEY_KP_9 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MOVE_BOTTOM_LEFT )] = { HotKeyCategory::DEFAULT_EVENTS, "move bottom left", fheroes2::Key::KEY_KP_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::MOVE_BOTTOM_RIGHT )] = { HotKeyCategory::DEFAULT_EVENTS, "move bottom right", fheroes2::Key::KEY_KP_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SYSTEM_FULLSCREEN )] = { HotKeyCategory::DEFAULT_EVENTS, "toggle fullscreen", fheroes2::Key::KEY_F4 };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_RETREAT )] = { HotKeyCategory::BATTLE, "retreat from battle", fheroes2::Key::KEY_R };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_SURRENDER )] = { HotKeyCategory::BATTLE, "surrender during battle", fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_AUTO_SWITCH )] = { HotKeyCategory::BATTLE, "toggle battle auto mode", fheroes2::Key::KEY_A };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_AUTO_FINISH )] = { HotKeyCategory::BATTLE, "finish the battle in auto mode", fheroes2::Key::KEY_Q };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_OPTIONS )] = { HotKeyCategory::BATTLE, "battle options", fheroes2::Key::KEY_O };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_SKIP )] = { HotKeyCategory::BATTLE, "skip turn in battle", fheroes2::Key::KEY_SPACE };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::BATTLE_WAIT )] = { HotKeyCategory::BATTLE, "wait in battle", fheroes2::Key::KEY_W };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SAVE_GAME )] = { HotKeyCategory::WORLD_MAP, "save game", fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::NEXT_HERO )] = { HotKeyCategory::WORLD_MAP, "next hero", fheroes2::Key::KEY_H };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CONTINUE_HERO_MOVEMENT )] = { HotKeyCategory::WORLD_MAP, "continue hero movement", fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CAST_SPELL )] = { HotKeyCategory::WORLD_MAP, "cast spell", fheroes2::Key::KEY_C };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SLEEP_HERO )] = { HotKeyCategory::WORLD_MAP, "put hero to sleep", fheroes2::Key::KEY_Z };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::NEXT_TOWN )] = { HotKeyCategory::WORLD_MAP, "next town", fheroes2::Key::KEY_T };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::END_TURN )] = { HotKeyCategory::WORLD_MAP, "end turn", fheroes2::Key::KEY_E };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::FILE_OPTIONS )] = { HotKeyCategory::WORLD_MAP, "file options", fheroes2::Key::KEY_F };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::ADVENTURE_OPTIONS )] = { HotKeyCategory::WORLD_MAP, "adventure options", fheroes2::Key::KEY_A };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SYSTEM_OPTIONS )] = { HotKeyCategory::WORLD_MAP, "system options", fheroes2::Key::KEY_O };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::PUZZLE_MAP )] = { HotKeyCategory::WORLD_MAP, "puzzle map", fheroes2::Key::KEY_P };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SCENARIO_INFORMATION )] = { HotKeyCategory::WORLD_MAP, "scenario information", fheroes2::Key::KEY_I };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DIG_ARTIFACT )] = { HotKeyCategory::WORLD_MAP, "dig for artifact", fheroes2::Key::KEY_D };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::KINGDOM_SUMMARY )] = { HotKeyCategory::WORLD_MAP, "kingdom summary", fheroes2::Key::KEY_K };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::VIEW_WORLD )] = { HotKeyCategory::WORLD_MAP, "view world", fheroes2::Key::KEY_V };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DEFAULT_ACTION )] = { HotKeyCategory::WORLD_MAP, "default action", fheroes2::Key::KEY_SPACE };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::OPEN_FOCUS )] = { HotKeyCategory::WORLD_MAP, "open focus", fheroes2::Key::KEY_ENTER };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::CONTROL_PANEL )] = { HotKeyCategory::WORLD_MAP, "control panel", fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SHOW_RADAR )] = { HotKeyCategory::WORLD_MAP, "show radar", fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SHOW_BUTTONS )] = { HotKeyCategory::WORLD_MAP, "show game buttons", fheroes2::Key::KEY_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SHOW_STATUS )] = { HotKeyCategory::WORLD_MAP, "show status", fheroes2::Key::KEY_4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SHOW_ICONS )] = { HotKeyCategory::WORLD_MAP, "show icons", fheroes2::Key::KEY_5 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SCROLL_LEFT )] = { HotKeyCategory::WORLD_MAP, "scroll left", fheroes2::Key::KEY_KP_4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SCROLL_RIGHT )] = { HotKeyCategory::WORLD_MAP, "scroll right", fheroes2::Key::KEY_KP_6 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SCROLL_UP )] = { HotKeyCategory::WORLD_MAP, "scroll up", fheroes2::Key::KEY_KP_8 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SCROLL_DOWN )] = { HotKeyCategory::WORLD_MAP, "scroll down", fheroes2::Key::KEY_KP_2 };

#if defined( WITH_DEBUG )
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TRANSFER_CONTROL_TO_AI )] = { HotKeyCategory::WORLD_MAP, "transfer control to ai", fheroes2::Key::KEY_F8 };
#endif

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SPLIT_STACK_BY_HALF )] = { HotKeyCategory::MONSTER, "split stack by half", fheroes2::Key::KEY_SHIFT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::SPLIT_STACK_BY_ONE )] = { HotKeyCategory::MONSTER, "split stack by one", fheroes2::Key::KEY_CONTROL };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::JOIN_STACKS )] = { HotKeyCategory::MONSTER, "join stacks", fheroes2::Key::KEY_ALT };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::UPGRADE_TROOP )] = { HotKeyCategory::MONSTER, "upgrade troop", fheroes2::Key::KEY_U };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::DISMISS_TROOP )] = { HotKeyCategory::MONSTER, "dismiss troop", fheroes2::Key::KEY_D };

        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_1 )] = { HotKeyCategory::CASTLE, "town dwelling level 1", fheroes2::Key::KEY_1 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_2 )] = { HotKeyCategory::CASTLE, "town dwelling level 2", fheroes2::Key::KEY_2 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_3 )] = { HotKeyCategory::CASTLE, "town dwelling level 3", fheroes2::Key::KEY_3 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_4 )] = { HotKeyCategory::CASTLE, "town dwelling level 4", fheroes2::Key::KEY_4 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_5 )] = { HotKeyCategory::CASTLE, "town dwelling level 5", fheroes2::Key::KEY_5 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_6 )] = { HotKeyCategory::CASTLE, "town dwelling level 6", fheroes2::Key::KEY_6 };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_WELL )] = { HotKeyCategory::CASTLE, "well", fheroes2::Key::KEY_W };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_MAGE_GUILD )] = { HotKeyCategory::CASTLE, "mage guild", fheroes2::Key::KEY_S };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_MARKETPLACE )] = { HotKeyCategory::CASTLE, "marketplace", fheroes2::Key::KEY_M };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_THIEVES_GUILD )] = { HotKeyCategory::CASTLE, "thieves guild", fheroes2::Key::KEY_T };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_SHIPYARD )] = { HotKeyCategory::CASTLE, "shipyard", fheroes2::Key::KEY_N };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_TAVERN )] = { HotKeyCategory::CASTLE, "tavern", fheroes2::Key::KEY_R };
        // It is also used to build castle in a town.
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::TOWN_JUMP_TO_BUILD_SELECTION )] = { HotKeyCategory::CASTLE, "castle construction", fheroes2::Key::KEY_B };
        hotKeyEventInfo[hotKeyEventToInt( Game::HotKeyEvent::WELL_BUY_ALL_CREATURES )] = { HotKeyCategory::CASTLE, "buy all monsters in well", fheroes2::Key::KEY_M };
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
    const std::string filename = System::ConcatePath( System::GetConfigDirectory( "fheroes2" ), "fheroes2.key" );

    std::fstream file( filename.data(), std::fstream::out | std::fstream::trunc );
    if ( !file ) {
        ERROR_LOG( "Unable to open hotkey settings file " << filename )
        return;
    }

    const std::string & data = getHotKeyFileContent();
    file.write( data.data(), data.size() );
}

void Game::KeyboardGlobalFilter( int sdlKey, int mod )
{
    if ( fheroes2::getKeyFromSDL( sdlKey ) == hotKeyEventInfo[hotKeyEventToInt( HotKeyEvent::SYSTEM_FULLSCREEN )].key
         && !( ( mod & KMOD_ALT ) || ( mod & KMOD_CTRL ) ) ) {
        Settings & conf = Settings::Get();
        conf.setFullScreen( !fheroes2::engine().isFullScreen() );
        conf.Save( Settings::configFileName );
    }
}
