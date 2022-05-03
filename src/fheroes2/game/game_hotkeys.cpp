/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
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
#include "localevent.h"
#include "logging.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "tinyconfig.h"
#include "tools.h"

#include <array>
#include <cassert>
#include <fstream>
#include <map>
#include <set>

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

        HotKeyEventInfo( const HotKeyCategory category_, const char * name_, const KeySym key_ )
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

        KeySym key = KEY_NONE;
    };

    std::array<HotKeyEventInfo, fheroes2::enumToUnderlying( Game::HotKeyEvent::NO_EVENT )> hotKeyEventInfo;

    void initializeHotKeyEvents()
    {
        // Make sure that event name is unique!
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_NEW_GAME )] = { HotKeyCategory::MAIN_GAME, "new game", KEY_n };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME )] = { HotKeyCategory::MAIN_GAME, "load game", KEY_l };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_HIGHSCORES )] = { HotKeyCategory::MAIN_GAME, "highscores", KEY_h };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_CREDITS )] = { HotKeyCategory::MAIN_GAME, "credits", KEY_c };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_STANDARD )] = { HotKeyCategory::MAIN_GAME, "standard game", KEY_s };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "campaign game", KEY_c };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_MULTI )] = { HotKeyCategory::MAIN_GAME, "multi-player game", KEY_m };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_SETTINGS )] = { HotKeyCategory::MAIN_GAME, "settings", KEY_t };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_SELECT_MAP )] = { HotKeyCategory::MAIN_GAME, "select map", KEY_s };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_HOTSEAT )] = { HotKeyCategory::MAIN_GAME, "hotseat game", KEY_h };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_BATTLEONLY )] = { HotKeyCategory::MAIN_GAME, "battle only game", KEY_b };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_NEW_CAMPAIGN_SELECTION_SUCCESSION_WARS )] = { HotKeyCategory::MAIN_GAME, "the succession wars campaign selection", KEY_o };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MAIN_MENU_NEW_CAMPAIGN_SELECTION_PRICE_OF_LOYALTY )] = { HotKeyCategory::MAIN_GAME, "the price of loyalty campaign selection", KEY_e };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::NEW_ROLAND_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "roland campaign", KEY_1 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::NEW_ARCHIBALD_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "archibald campaign", KEY_2 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::NEW_PRICE_OF_LOYALTY_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "the price of loyalty campaign", KEY_1 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::NEW_VOYAGE_HOME_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "voyage home campaign", KEY_2 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::NEW_WIZARDS_ISLE_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "wizard's isle campaign", KEY_3 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::NEW_DESCENDANTS_CAMPAIGN )] = { HotKeyCategory::MAIN_GAME, "descendants campaign", KEY_4 };

        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::DEFAULT_READY )] = { HotKeyCategory::DEFAULT_EVENTS, "default okay event", KEY_RETURN };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::DEFAULT_EXIT )] = { HotKeyCategory::DEFAULT_EVENTS, "default cancel event", KEY_ESCAPE };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::DEFAULT_LEFT )] = { HotKeyCategory::DEFAULT_EVENTS, "left selection", KEY_NONE };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::DEFAULT_RIGHT )] = { HotKeyCategory::DEFAULT_EVENTS, "right selection", KEY_NONE };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MOVE_LEFT )] = { HotKeyCategory::DEFAULT_EVENTS, "move left", KEY_LEFT };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MOVE_RIGHT )] = { HotKeyCategory::DEFAULT_EVENTS, "move right", KEY_RIGHT };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MOVE_TOP )] = { HotKeyCategory::DEFAULT_EVENTS, "move up", KEY_UP };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MOVE_BOTTOM )] = { HotKeyCategory::DEFAULT_EVENTS, "move bottom", KEY_DOWN };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MOVE_TOP_LEFT )] = { HotKeyCategory::DEFAULT_EVENTS, "move top bottom", KEY_NONE };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MOVE_TOP_RIGHT )] = { HotKeyCategory::DEFAULT_EVENTS, "move top right", KEY_NONE };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MOVE_BOTTOM_LEFT )] = { HotKeyCategory::DEFAULT_EVENTS, "move bottom left", KEY_NONE };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::MOVE_BOTTOM_RIGHT )] = { HotKeyCategory::DEFAULT_EVENTS, "move bottom right", KEY_NONE };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SYSTEM_FULLSCREEN )] = { HotKeyCategory::DEFAULT_EVENTS, "toggle fullscreen", KEY_F4 };

        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::BATTLE_RETREAT )] = { HotKeyCategory::BATTLE, "retreat from battle", KEY_r };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::BATTLE_SURRENDER )] = { HotKeyCategory::BATTLE, "surrender during battle", KEY_s };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::BATTLE_AUTOSWITCH )] = { HotKeyCategory::BATTLE, "toggle battle auto mode", KEY_a };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::BATTLE_OPTIONS )] = { HotKeyCategory::BATTLE, "battle options", KEY_o };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::BATTLE_SKIP )] = { HotKeyCategory::BATTLE, "skip turn in battle", KEY_SPACE };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::BATTLE_WAIT )] = { HotKeyCategory::BATTLE, "wait in battle", KEY_w };

        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SAVE_GAME )] = { HotKeyCategory::WORLD_MAP, "save game", KEY_s };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::NEXT_HERO )] = { HotKeyCategory::WORLD_MAP, "next hero", KEY_h };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::CONTINUE_HERO_MOVEMENT )] = { HotKeyCategory::WORLD_MAP, "continue hero movement", KEY_m };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::CAST_SPELL )] = { HotKeyCategory::WORLD_MAP, "cast spell", KEY_c };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SLEEP_HERO )] = { HotKeyCategory::WORLD_MAP, "put hero to sleep", KEY_z };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::NEXT_TOWN )] = { HotKeyCategory::WORLD_MAP, "next town", KEY_t };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::END_TURN )] = { HotKeyCategory::WORLD_MAP, "end turn", KEY_e };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::FILE_OPTIONS )] = { HotKeyCategory::WORLD_MAP, "file options", KEY_f };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SYSTEM_OPTIONS )] = { HotKeyCategory::WORLD_MAP, "system options", KEY_o };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::PUZZLE_MAP )] = { HotKeyCategory::WORLD_MAP, "puzzle map", KEY_p };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SCENARIO_INFORMATION )] = { HotKeyCategory::WORLD_MAP, "scenario information", KEY_i };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::DIG_ARTIFACT )] = { HotKeyCategory::WORLD_MAP, "dig for artifact", KEY_d };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::KINGDOM_SUMMARY )] = { HotKeyCategory::WORLD_MAP, "kingdom summary", KEY_k };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::VIEW_WORLD )] = { HotKeyCategory::WORLD_MAP, "view world", KEY_v };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::DEFAULT_ACTION )] = { HotKeyCategory::WORLD_MAP, "default action", KEY_SPACE };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::OPEN_FOCUS )] = { HotKeyCategory::WORLD_MAP, "open focus", KEY_RETURN };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::CONTROL_PANEL )] = { HotKeyCategory::WORLD_MAP, "control panel", KEY_1 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SHOW_RADAR )] = { HotKeyCategory::WORLD_MAP, "show radar", KEY_2 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SHOW_BUTTONS )] = { HotKeyCategory::WORLD_MAP, "show game buttons", KEY_3 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SHOW_STATUS )] = { HotKeyCategory::WORLD_MAP, "show status", KEY_4 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SHOW_ICONS )] = { HotKeyCategory::WORLD_MAP, "show icons", KEY_5 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SCROLL_LEFT )] = { HotKeyCategory::WORLD_MAP, "scroll left", KEY_KP4 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SCROLL_RIGHT )] = { HotKeyCategory::WORLD_MAP, "scroll right", KEY_KP6 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SCROLL_UP )] = { HotKeyCategory::WORLD_MAP, "scroll up", KEY_KP8 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SCROLL_DOWN )] = { HotKeyCategory::WORLD_MAP, "scroll down", KEY_KP2 };

        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SPLIT_STACK_BY_HALF )] = { HotKeyCategory::MONSTER, "split stack by half", KEY_SHIFT };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::SPLIT_STACK_BY_ONE )] = { HotKeyCategory::MONSTER, "split stack by one", KEY_CONTROL };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::JOIN_STACKS )] = { HotKeyCategory::MONSTER, "join stacks", KEY_ALT };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::UPGRADE_TROOP )] = { HotKeyCategory::MONSTER, "upgrade troop", KEY_u };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::DISMISS_TROOP )] = { HotKeyCategory::MONSTER, "dismiss troop", KEY_d };

        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_1 )] = { HotKeyCategory::CASTLE, "town dwelling level 1", KEY_1 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_2 )] = { HotKeyCategory::CASTLE, "town dwelling level 2", KEY_2 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_3 )] = { HotKeyCategory::CASTLE, "town dwelling level 3", KEY_3 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_4 )] = { HotKeyCategory::CASTLE, "town dwelling level 4", KEY_4 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_5 )] = { HotKeyCategory::CASTLE, "town dwelling level 5", KEY_5 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_6 )] = { HotKeyCategory::CASTLE, "town dwelling level 6", KEY_6 };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_WELL )] = { HotKeyCategory::CASTLE, "well", KEY_w };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_MAGE_GUILD )] = { HotKeyCategory::CASTLE, "mage guild", KEY_s };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_MARKETPLACE )] = { HotKeyCategory::CASTLE, "marketplace", KEY_m };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_THIEVES_GUILD )] = { HotKeyCategory::CASTLE, "thieves guild", KEY_t };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_SHIPYARD )] = { HotKeyCategory::CASTLE, "shipyard", KEY_n };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_TAVERN )] = { HotKeyCategory::CASTLE, "tavern", KEY_r };
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::TOWN_JUMP_TO_BUILD_SELECTION )] = { HotKeyCategory::CASTLE, "castle construction", KEY_b }; // also used to build castle in a town
        hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::WELL_BUY_ALL_CREATURES )] = { HotKeyCategory::CASTLE, "buy all monsters in well", KEY_m };
    }

    std::string getHotKeyFileContent()
    {
        std::ostringstream os;
        os << "# fheroes2 hotkey file (saved by version " << Settings::GetVersion() << ")" << std::endl;
        os << std::endl;

        HotKeyCategory currentCategory = hotKeyEventInfo[fheroes2::enumToUnderlying( Game::HotKeyEvent::NONE ) + 1].category;
        os << "# " << getHotKeyCategoryName( currentCategory ) << ':' << std::endl;

#if defined( WITH_DEBUG )
        std::set<const char *> duplicationStringVerifier;
#endif

        for ( int32_t eventId = fheroes2::enumToUnderlying( Game::HotKeyEvent::NONE ) + 1; eventId < fheroes2::enumToUnderlying( Game::HotKeyEvent::NO_EVENT ); ++eventId ) {
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
    return le.KeyPress() && le.KeyValue() == hotKeyEventInfo[fheroes2::enumToUnderlying( eventID )].key;
}

bool Game::HotKeyHoldEvent( const HotKeyEvent eventID )
{
    const LocalEvent & le = LocalEvent::Get();
    return le.KeyHold() && le.KeyValue() == hotKeyEventInfo[fheroes2::enumToUnderlying( eventID )].key;
}

std::string Game::getHotKeyNameByEventId( const HotKeyEvent eventID )
{
    return StringUpper( KeySymGetName( hotKeyEventInfo[fheroes2::enumToUnderlying( eventID )].key ) );
}

void Game::HotKeysLoad( std::string filename )
{
    initializeHotKeyEvents();

    TinyConfig config( '=', '#' );

    if ( config.Load( filename ) ) {
        std::map<std::string, KeySym> nameToKey;
        for ( int32_t i = KEY_NONE + 1; i < KEY_LAST; ++i ) {
            const KeySym key = static_cast<KeySym>( i );
            nameToKey.emplace( StringUpper( KeySymGetName( key ) ), key );
        }

        for ( int eventId = fheroes2::enumToUnderlying( HotKeyEvent::NONE ) + 1; eventId < fheroes2::enumToUnderlying( HotKeyEvent::NO_EVENT ); ++eventId ) {
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
    else {
        filename = System::ConcatePath( System::GetConfigDirectory( "fheroes2" ), "fheroes2.key" );
        std::fstream file;
        file.open( filename.data(), std::fstream::out | std::fstream::trunc );
        if ( !file )
            return;

        const std::string & data = getHotKeyFileContent();
        file.write( data.data(), data.size() );
    }
}

void Game::KeyboardGlobalFilter( int sym, int mod )
{
    // system hotkeys
    if ( sym == hotKeyEventInfo[fheroes2::enumToUnderlying( HotKeyEvent::SYSTEM_FULLSCREEN )].key && !( ( mod & KMOD_ALT ) || ( mod & KMOD_CTRL ) ) ) {
        fheroes2::engine().toggleFullScreen();
        fheroes2::Display::instance().render();

        Settings & conf = Settings::Get();
        conf.setFullScreen( fheroes2::engine().isFullScreen() );
        conf.Save( Settings::configFileName );
    }
}
