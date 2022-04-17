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

#include "game.h"
#include "localevent.h"
#include "logging.h"
#include "screen.h"
#include "settings.h"
#include "tinyconfig.h"
#include "tools.h"

#include <array>
#include <cassert>
#include <map>

namespace
{
    std::array<KeySym, Game::EVENT_LAST> key_events{ KEY_NONE };

    const char * getEventName( const int eventId )
    {
        switch ( eventId ) {
        case Game::EVENT_BUTTON_NEWGAME:
            return "button newgame";
        case Game::EVENT_BUTTON_LOADGAME:
            return "button loadgame";
        case Game::EVENT_BUTTON_HIGHSCORES:
            return "button highscores";
        case Game::EVENT_BUTTON_CREDITS:
            return "button credits";
        case Game::EVENT_BUTTON_STANDARD:
            return "button standard";
        case Game::EVENT_BUTTON_CAMPAIGN:
            return "button campain";
        case Game::EVENT_BUTTON_MULTI:
            return "button multigame";
        case Game::EVENT_BUTTON_SETTINGS:
            return "button settings";
        case Game::EVENT_BUTTON_SELECT:
            return "button select";
        case Game::EVENT_BUTTON_HOTSEAT:
            return "button hotseat";
        case Game::EVENT_BUTTON_HOST:
            return "button host";
        case Game::EVENT_BUTTON_GUEST:
            return "button guest";
        case Game::EVENT_BUTTON_BATTLEONLY:
            return "button battleonly";
        case Game::EVENT_DEFAULT_READY:
            return "default ready";
        case Game::EVENT_DEFAULT_EXIT:
            return "default exit";
        case Game::EVENT_DEFAULT_LEFT:
            return "default left";
        case Game::EVENT_DEFAULT_RIGHT:
            return "default right";
        case Game::EVENT_SYSTEM_FULLSCREEN:
            return "system fullscreen";
        case Game::EVENT_SYSTEM_SCREENSHOT:
            return "system screenshot";
        case Game::EVENT_SLEEP_HERO:
            return "sleep hero";
        case Game::EVENT_END_TURN:
            return "end turn";
        case Game::EVENT_NEXTHERO:
            return "next hero";
        case Game::EVENT_NEXTTOWN:
            return "next town";
        case Game::EVENT_CONTINUE:
            return "continue move";
        case Game::EVENT_SAVEGAME:
            return "save game";
        case Game::EVENT_LOADGAME:
            return "load game";
        case Game::EVENT_FILEOPTIONS:
            return "show file dialog";
        case Game::EVENT_SYSTEMOPTIONS:
            return "show system options";
        case Game::EVENT_PUZZLEMAPS:
            return "show puzzle maps";
        case Game::EVENT_INFOGAME:
            return "show game info";
        case Game::EVENT_DIG_ARTIFACT:
            return "dig artifact";
        case Game::EVENT_CASTSPELL:
            return "cast spell";
        case Game::EVENT_KINGDOM_INFO:
            return "kingdom overview";
        case Game::EVENT_VIEW_WORLD:
            return "view world";
        case Game::EVENT_DEFAULTACTION:
            return "default action";
        case Game::EVENT_BATTLE_CASTSPELL:
            return "battle cast spell";
        case Game::EVENT_BATTLE_RETREAT:
            return "battle retreat";
        case Game::EVENT_BATTLE_SURRENDER:
            return "battle surrender";
        case Game::EVENT_BATTLE_AUTOSWITCH:
            return "battle auto switch";
        case Game::EVENT_BATTLE_OPTIONS:
            return "battle options";
        case Game::EVENT_BATTLE_HARDSKIP:
            return "battle.hard skip";
        case Game::EVENT_BATTLE_SOFTSKIP:
            return "battle soft skip";
        case Game::EVENT_MOVELEFT:
            return "move left";
        case Game::EVENT_MOVERIGHT:
            return "move right";
        case Game::EVENT_MOVETOP:
            return "move top";
        case Game::EVENT_MOVEBOTTOM:
            return "move bottom";
        case Game::EVENT_MOVETOPLEFT:
            return "move top left";
        case Game::EVENT_MOVETOPRIGHT:
            return "move top right";
        case Game::EVENT_MOVEBOTTOMLEFT:
            return "move bottom left";
        case Game::EVENT_MOVEBOTTOMRIGHT:
            return "move bottom right";
        case Game::EVENT_OPENFOCUS:
            return "open focus";
        case Game::EVENT_SCROLLLEFT:
            return "scroll left";
        case Game::EVENT_SCROLLRIGHT:
            return "scroll right";
        case Game::EVENT_SCROLLUP:
            return "scroll up";
        case Game::EVENT_SCROLLDOWN:
            return "scroll down";
        case Game::EVENT_CTRLPANEL:
            return "control panel";
        case Game::EVENT_SHOWRADAR:
            return "show radar";
        case Game::EVENT_SHOWBUTTONS:
            return "show buttons";
        case Game::EVENT_SHOWSTATUS:
            return "show status";
        case Game::EVENT_SHOWICONS:
            return "show icons";
        case Game::EVENT_SPLIT_STACK_BY_HALF:
            return "split stack by half";
        case Game::EVENT_SPLIT_STACK_BY_ONE:
            return "split stack by one";
        case Game::EVENT_JOIN_STACKS:
            return "join stacks";
        case Game::EVENT_UPGRADE_TROOP:
            return "upgrade troop";
        case Game::EVENT_DISMISS_TROOP:
            return "dismiss troop";
        case Game::EVENT_TOWN_DWELLING_LEVEL_1:
            return "town dwelling level 1";
        case Game::EVENT_TOWN_DWELLING_LEVEL_2:
            return "town dwelling level 2";
        case Game::EVENT_TOWN_DWELLING_LEVEL_3:
            return "town dwelling level 3";
        case Game::EVENT_TOWN_DWELLING_LEVEL_4:
            return "town dwelling level 4";
        case Game::EVENT_TOWN_DWELLING_LEVEL_5:
            return "town dwelling level 5";
        case Game::EVENT_TOWN_DWELLING_LEVEL_6:
            return "town dwelling level 6";
        case Game::EVENT_TOWN_WELL:
            return "town well";
        case Game::EVENT_TOWN_MARKETPLACE:
            return "town marketplace";
        case Game::EVENT_TOWN_MAGE_GUILD:
            return "town mageguild";
        case Game::EVENT_TOWN_SHIPYARD:
            return "town shipyard";
        case Game::EVENT_TOWN_THIEVES_GUILD:
            return "town guild";
        case Game::EVENT_TOWN_TAVERN:
            return "town tavern";
        case Game::EVENT_TOWN_JUMP_TO_BUILD_SELECTION:
            return "town build";
        case Game::EVENT_WELL_BUY_ALL_CREATURES:
            return "well buy all creatures";
        case Game::EVENT_NEW_CAMPAIGN_SELECTION_SUCCESSION_WARS:
            return "the succession wars campaign selection";
        case Game::EVENT_NEW_CAMPAIGN_SELECTION_PRICE_OF_LOYALTY:
            return "the price of loyalty campaign selection";
        case Game::EVENT_NEW_ROLAND_CAMPAIGN:
            return "roland campaign";
        case Game::EVENT_NEW_ARCHIBALD_CAMPAIGN:
            return "archibald campaign";
        case Game::EVENT_NEW_PRICE_OF_LOYALTY_CAMPAIGN:
            return "the price of loyalty campaign";
        case Game::EVENT_NEW_VOYAGE_HOME_CAMPAIGN:
            return "voyage home campaign";
        case Game::EVENT_NEW_WIZARDS_ISLE_CAMPAIGN:
            return "wizard's isle campaign";
        case Game::EVENT_NEW_DESCENDANTS_CAMPAIGN:
            return "descendants campaign";
        default:
            // Did you add a new hot key event? Add the logic for it!
            assert( 0 );
            break;
        }
        return nullptr;
    }
}

namespace Game
{
    void HotKeysDefaults( void );
    void HotKeysLoad( const std::string & );
    void KeyboardGlobalFilter( int, int );
}

void Game::HotKeysDefaults( void )
{
    std::fill( key_events.begin(), key_events.end(), KEY_NONE );

    // main menu
    key_events[EVENT_BUTTON_NEWGAME] = KEY_n;
    key_events[EVENT_BUTTON_LOADGAME] = KEY_l;
    key_events[EVENT_BUTTON_HIGHSCORES] = KEY_h;
    key_events[EVENT_BUTTON_CREDITS] = KEY_c;
    key_events[EVENT_BUTTON_STANDARD] = KEY_s;
    key_events[EVENT_BUTTON_CAMPAIGN] = KEY_c;
    key_events[EVENT_BUTTON_MULTI] = KEY_m;
    key_events[EVENT_BUTTON_SETTINGS] = KEY_t;
    key_events[EVENT_BUTTON_SELECT] = KEY_s;
    key_events[EVENT_BUTTON_HOTSEAT] = KEY_h;
    key_events[EVENT_BUTTON_HOST] = KEY_h;
    key_events[EVENT_BUTTON_GUEST] = KEY_g;
    key_events[EVENT_BUTTON_BATTLEONLY] = KEY_b;

    // default
    key_events[EVENT_DEFAULT_READY] = KEY_RETURN;
    key_events[EVENT_DEFAULT_EXIT] = KEY_ESCAPE;
    key_events[EVENT_DEFAULT_LEFT] = KEY_NONE;
    key_events[EVENT_DEFAULT_RIGHT] = KEY_NONE;

    // system
    key_events[EVENT_SYSTEM_FULLSCREEN] = KEY_F4;
    key_events[EVENT_SYSTEM_SCREENSHOT] = KEY_PRINT;

    // battle
    key_events[EVENT_BATTLE_CASTSPELL] = KEY_c;
    key_events[EVENT_BATTLE_RETREAT] = KEY_r;
    key_events[EVENT_BATTLE_SURRENDER] = KEY_s;
    key_events[EVENT_BATTLE_AUTOSWITCH] = KEY_a;
    key_events[EVENT_BATTLE_OPTIONS] = KEY_o;
    key_events[EVENT_BATTLE_HARDSKIP] = KEY_h;
    key_events[EVENT_BATTLE_SOFTSKIP] = KEY_SPACE;

    // sleep hero
    key_events[EVENT_SLEEP_HERO] = KEY_z;
    // end turn
    key_events[EVENT_END_TURN] = KEY_e;
    // next hero
    key_events[EVENT_NEXTHERO] = KEY_h;
    // next town
    key_events[EVENT_NEXTTOWN] = KEY_t;
    // continue (move hero)
    key_events[EVENT_CONTINUE] = KEY_m;
    // save game
    key_events[EVENT_SAVEGAME] = KEY_s;
    // load game
    key_events[EVENT_LOADGAME] = KEY_l;
    // show file dialog
    key_events[EVENT_FILEOPTIONS] = KEY_f;
    // show system options
    key_events[EVENT_SYSTEMOPTIONS] = KEY_o;
    // show puzzle maps
    key_events[EVENT_PUZZLEMAPS] = KEY_p;
    // show game info
    key_events[EVENT_INFOGAME] = KEY_i;
    // dig artifact
    key_events[EVENT_DIG_ARTIFACT] = KEY_d;
    // cast spell
    key_events[EVENT_CASTSPELL] = KEY_c;
    // kingdom overview
    key_events[EVENT_KINGDOM_INFO] = KEY_k;
    // view world
    key_events[EVENT_VIEW_WORLD] = KEY_v;
    // default action
    key_events[EVENT_DEFAULTACTION] = KEY_SPACE;
    // move hero
    key_events[EVENT_MOVELEFT] = KEY_LEFT;
    key_events[EVENT_MOVERIGHT] = KEY_RIGHT;
    key_events[EVENT_MOVETOP] = KEY_UP;
    key_events[EVENT_MOVEBOTTOM] = KEY_DOWN;
    key_events[EVENT_MOVETOPLEFT] = KEY_NONE;
    key_events[EVENT_MOVETOPRIGHT] = KEY_NONE;
    key_events[EVENT_MOVEBOTTOMLEFT] = KEY_NONE;
    key_events[EVENT_MOVEBOTTOMRIGHT] = KEY_NONE;
    // open focus
    key_events[EVENT_OPENFOCUS] = KEY_RETURN;
    // control panel
    key_events[EVENT_CTRLPANEL] = KEY_1;
    key_events[EVENT_SHOWRADAR] = KEY_2;
    key_events[EVENT_SHOWBUTTONS] = KEY_3;
    key_events[EVENT_SHOWSTATUS] = KEY_4;
    key_events[EVENT_SHOWICONS] = KEY_5;
    // system:
    // gamepad scroll bindings
    key_events[EVENT_SCROLLLEFT] = KEY_KP4;
    key_events[EVENT_SCROLLRIGHT] = KEY_KP6;
    key_events[EVENT_SCROLLUP] = KEY_KP8;
    key_events[EVENT_SCROLLDOWN] = KEY_KP2;
    // split
    key_events[EVENT_SPLIT_STACK_BY_HALF] = KEY_SHIFT;
    key_events[EVENT_SPLIT_STACK_BY_ONE] = KEY_CONTROL;
    key_events[EVENT_JOIN_STACKS] = KEY_ALT;

    key_events[EVENT_UPGRADE_TROOP] = KEY_u;
    key_events[EVENT_DISMISS_TROOP] = KEY_d;

    // town + build screen
    key_events[EVENT_TOWN_DWELLING_LEVEL_1] = KEY_1;
    key_events[EVENT_TOWN_DWELLING_LEVEL_2] = KEY_2;
    key_events[EVENT_TOWN_DWELLING_LEVEL_3] = KEY_3;
    key_events[EVENT_TOWN_DWELLING_LEVEL_4] = KEY_4;
    key_events[EVENT_TOWN_DWELLING_LEVEL_5] = KEY_5;
    key_events[EVENT_TOWN_DWELLING_LEVEL_6] = KEY_6;
    key_events[EVENT_TOWN_WELL] = KEY_w;
    key_events[EVENT_TOWN_MAGE_GUILD] = KEY_s;
    key_events[EVENT_TOWN_MARKETPLACE] = KEY_m;
    key_events[EVENT_TOWN_THIEVES_GUILD] = KEY_t;
    key_events[EVENT_TOWN_SHIPYARD] = KEY_n;

    // town screen only
    key_events[EVENT_TOWN_TAVERN] = KEY_r;
    key_events[EVENT_TOWN_JUMP_TO_BUILD_SELECTION] = KEY_b; // also used to build castle, if starting on a village

    key_events[EVENT_NEW_CAMPAIGN_SELECTION_SUCCESSION_WARS] = KEY_o;
    key_events[EVENT_NEW_CAMPAIGN_SELECTION_PRICE_OF_LOYALTY] = KEY_e;

    key_events[EVENT_NEW_ROLAND_CAMPAIGN] = KEY_1;
    key_events[EVENT_NEW_ARCHIBALD_CAMPAIGN] = KEY_2;

    key_events[EVENT_NEW_PRICE_OF_LOYALTY_CAMPAIGN] = KEY_1;
    key_events[EVENT_NEW_VOYAGE_HOME_CAMPAIGN] = KEY_2;
    key_events[EVENT_NEW_WIZARDS_ISLE_CAMPAIGN] = KEY_3;
    key_events[EVENT_NEW_DESCENDANTS_CAMPAIGN] = KEY_4;

    key_events[EVENT_WELL_BUY_ALL_CREATURES] = KEY_m;
}

bool Game::HotKeyPressEvent( int evnt )
{
    const LocalEvent & le = LocalEvent::Get();
    return le.KeyPress() && le.KeyValue() == key_events[evnt];
}

bool Game::HotKeyHoldEvent( const int eventID )
{
    const LocalEvent & le = LocalEvent::Get();
    return le.KeyHold() && le.KeyValue() == key_events[eventID];
}

std::string Game::getHotKeyNameByEventId( const int eventID )
{
    return StringUpper( KeySymGetName( key_events[eventID] ) );
}

void Game::HotKeysLoad( const std::string & hotkeys )
{
    TinyConfig config( '=', '#' );

    if ( config.Load( hotkeys ) ) {
        int ival = 0;

        for ( int evnt = EVENT_NONE + 1; evnt < EVENT_LAST; ++evnt ) {
            const char * name = getEventName( evnt );
            ival = config.IntParams( name );
            if ( ival ) {
                const KeySym sym = GetKeySym( ival );
                key_events[evnt] = sym;
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Event '" << getEventName( evnt ) << "' has key '" << KeySymGetName( sym ) << "'" );
            }
        }
    }
}

void Game::KeyboardGlobalFilter( int sym, int mod )
{
    // system hotkeys
    if ( sym == key_events[EVENT_SYSTEM_FULLSCREEN] && !( ( mod & KMOD_ALT ) || ( mod & KMOD_CTRL ) ) ) {
        fheroes2::engine().toggleFullScreen();
        fheroes2::Display::instance().render();

        Settings & conf = Settings::Get();
        conf.setFullScreen( fheroes2::engine().isFullScreen() );
        conf.Save( Settings::configFileName );
    }
}
