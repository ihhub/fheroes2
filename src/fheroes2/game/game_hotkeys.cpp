/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <ctime>
#include <sstream>

#include "agg.h"
#include "game.h"
#include "game_interface.h"
#include "gamedefs.h"
#include "settings.h"
#include "system.h"
#include "tinyconfig.h"

namespace Game
{
    void HotKeysDefaults( void );
    void HotKeysLoad( const std::string & );
    const char * EventsName( int );
    void KeyboardGlobalFilter( int, int );

    KeySym key_events[EVENT_LAST];
    int key_groups = 0;
}

const char * Game::EventsName( int evnt )
{
    switch ( evnt ) {
    case EVENT_BUTTON_NEWGAME:
        return "button newgame";
    case EVENT_BUTTON_LOADGAME:
        return "button loadgame";
    case EVENT_BUTTON_HIGHSCORES:
        return "button highscores";
    case EVENT_BUTTON_CREDITS:
        return "button credits";
    case EVENT_BUTTON_STANDARD:
        return "button standard";
    case EVENT_BUTTON_CAMPAIN:
        return "button campain";
    case EVENT_BUTTON_MULTI:
        return "button multigame";
    case EVENT_BUTTON_SETTINGS:
        return "button settings";
    case EVENT_BUTTON_SELECT:
        return "button select";
    case EVENT_BUTTON_HOTSEAT:
        return "button hotseat";
    case EVENT_BUTTON_NETWORK:
        return "button network";
    case EVENT_BUTTON_HOST:
        return "button host";
    case EVENT_BUTTON_GUEST:
        return "button guest";
    case EVENT_BUTTON_BATTLEONLY:
        return "button battleonly";

    case EVENT_DEFAULT_READY:
        return "default ready";
    case EVENT_DEFAULT_EXIT:
        return "default exit";
    case EVENT_DEFAULT_LEFT:
        return "default left";
    case EVENT_DEFAULT_RIGHT:
        return "default right";

    case EVENT_SYSTEM_FULLSCREEN:
        return "system fullscreen";
    case EVENT_SYSTEM_SCREENSHOT:
        return "system screenshot";
    case EVENT_SYSTEM_DEBUG1:
        return "system debug1";
    case EVENT_SYSTEM_DEBUG2:
        return "system debug2";

    case EVENT_SLEEPHERO:
        return "sleep hero";
    case EVENT_ENDTURN:
        return "end turn";
    case EVENT_NEXTHERO:
        return "next hero";
    case EVENT_NEXTTOWN:
        return "next town";
    case EVENT_CONTINUE:
        return "continue move";
    case EVENT_SAVEGAME:
        return "save game";
    case EVENT_LOADGAME:
        return "load game";
    case EVENT_FILEOPTIONS:
        return "show file dialog";
    case EVENT_SYSTEMOPTIONS:
        return "show system options";
    case EVENT_PUZZLEMAPS:
        return "show puzzle maps";
    case EVENT_INFOGAME:
        return "show game info";
    case EVENT_DIGARTIFACT:
        return "dig artifact";
    case EVENT_CASTSPELL:
        return "cast spell";
    case EVENT_DEFAULTACTION:
        return "default action";

    case EVENT_BATTLE_CASTSPELL:
        return "battle cast spell";
    case EVENT_BATTLE_RETREAT:
        return "battle retreat";
    case EVENT_BATTLE_SURRENDER:
        return "battle surrender";
    case EVENT_BATTLE_AUTOSWITCH:
        return "battle auto switch";
    case EVENT_BATTLE_OPTIONS:
        return "battle options";
    case EVENT_BATTLE_HARDSKIP:
        return "battle.hard skip";
    case EVENT_BATTLE_SOFTSKIP:
        return "battle soft skip";

    case EVENT_MOVELEFT:
        return "move left";
    case EVENT_MOVERIGHT:
        return "move right";
    case EVENT_MOVETOP:
        return "move top";
    case EVENT_MOVEBOTTOM:
        return "move bottom";
    case EVENT_MOVETOPLEFT:
        return "move top left";
    case EVENT_MOVETOPRIGHT:
        return "move top right";
    case EVENT_MOVEBOTTOMLEFT:
        return "move bottom left";
    case EVENT_MOVEBOTTOMRIGHT:
        return "move bottom right";
    case EVENT_OPENFOCUS:
        return "open focus";
    case EVENT_SCROLLLEFT:
        return "scroll left";
    case EVENT_SCROLLRIGHT:
        return "scroll right";
    case EVENT_SCROLLUP:
        return "scroll up";
    case EVENT_SCROLLDOWN:
        return "scroll down";
    case EVENT_CTRLPANEL:
        return "control panel";
    case EVENT_SHOWRADAR:
        return "show radar";
    case EVENT_SHOWBUTTONS:
        return "show buttons";
    case EVENT_SHOWSTATUS:
        return "show status";
    case EVENT_SHOWICONS:
        return "show icons";
    case EVENT_SWITCHGROUP:
        return "switch group";
    default:
        break;
    }
    return NULL;
}

void Game::HotKeysDefaults( void )
{
    std::fill( &key_events[0], &key_events[EVENT_LAST], KEY_NONE );

    // main menu
    key_events[EVENT_BUTTON_NEWGAME] = KEY_n;
    key_events[EVENT_BUTTON_LOADGAME] = KEY_l;
    key_events[EVENT_BUTTON_HIGHSCORES] = KEY_h;
    key_events[EVENT_BUTTON_CREDITS] = KEY_c;
    key_events[EVENT_BUTTON_STANDARD] = KEY_s;
    key_events[EVENT_BUTTON_CAMPAIN] = KEY_c;
    key_events[EVENT_BUTTON_MULTI] = KEY_m;
    key_events[EVENT_BUTTON_SETTINGS] = KEY_t;
    key_events[EVENT_BUTTON_SELECT] = KEY_s;
    key_events[EVENT_BUTTON_HOTSEAT] = KEY_h;
    key_events[EVENT_BUTTON_NETWORK] = KEY_n;
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
    key_events[EVENT_SYSTEM_DEBUG1] = KEY_NONE;
    key_events[EVENT_SYSTEM_DEBUG2] = KEY_NONE;

    // battle
    key_events[EVENT_BATTLE_CASTSPELL] = KEY_c;
    key_events[EVENT_BATTLE_RETREAT] = KEY_ESCAPE;
    key_events[EVENT_BATTLE_SURRENDER] = KEY_s;
    key_events[EVENT_BATTLE_AUTOSWITCH] = KEY_a;
    key_events[EVENT_BATTLE_OPTIONS] = KEY_o;
    key_events[EVENT_BATTLE_HARDSKIP] = KEY_h;
    key_events[EVENT_BATTLE_SOFTSKIP] = KEY_SPACE;

    // sleep hero
    key_events[EVENT_SLEEPHERO] = KEY_z;
    // end turn
    key_events[EVENT_ENDTURN] = KEY_e;
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
    key_events[EVENT_DIGARTIFACT] = KEY_d;
    // cast spell
    key_events[EVENT_CASTSPELL] = KEY_c;
    // default action
    key_events[EVENT_DEFAULTACTION] = KEY_SPACE;
    // move hero
    key_events[EVENT_MOVELEFT] = KEY_LEFT;
    key_events[EVENT_MOVERIGHT] = KEY_RIGHT;
    key_events[EVENT_MOVETOP] = KEY_UP;
    key_events[EVENT_MOVEBOTTOM] = KEY_DOWN;
    // key_events[EVENT_MOVEBOTTOM] = KEVENT_MOVETOPLEFT] = KEY_NONE;
    // key_events[EVENT_MOVEBOTTOM] = KEVENT_MOVETOPRIGHT] = KEY_NONE;
    // key_events[EVENT_MOVEBOTTOM] = KEVENT_MOVEBOTTOMLEFT] = KEY_NONE;
    // key_events[EVENT_MOVEBOTTOM] = KEVENT_MOVEBOTTOMRIGHT] = KEY_NONE;
    // open focus
    // key_events[EVENT_OPENFOCUS] = KEY_NONE;
    // scroll
    // key_events[EVENT_SCROLLLEFT] = KEY_NONE;
    // key_events[EVENT_SCROLLRIGHT] = KEY_NONE;
    // key_events[EVENT_SCROLLUP] = KEY_NONE;
    // key_events[EVENT_SCROLLDOWN] = KEY_NONE;
    // control panel
    key_events[EVENT_CTRLPANEL] = KEY_1;
    key_events[EVENT_SHOWRADAR] = KEY_2;
    key_events[EVENT_SHOWBUTTONS] = KEY_3;
    key_events[EVENT_SHOWSTATUS] = KEY_4;
    key_events[EVENT_SHOWICONS] = KEY_5;
    // system:
    // switch group
    // key_events[EVENT_SWITCHGROUP] = KEY_NONE;
    // split
    key_events[EVENT_STACKSPLIT_SHIFT] = KEY_SHIFT;
    key_events[EVENT_STACKSPLIT_CTRL] = KEY_CONTROL;
}

bool Game::HotKeyPressEvent( int evnt )
{
    LocalEvent & le = LocalEvent::Get();
    return le.KeyPress() && le.KeyValue() == key_events[evnt];
}

bool Game::HotKeyHoldEvent( const int eventID )
{
    LocalEvent & le = LocalEvent::Get();
    return le.KeyHold() && le.KeyValue() == key_events[eventID];
}

void Game::HotKeysLoad( const std::string & hotkeys )
{
    TinyConfig config( '=', '#' );

    if ( config.Load( hotkeys.c_str() ) ) {
        int ival = 0;

        for ( int evnt = EVENT_NONE; evnt < EVENT_LAST; ++evnt ) {
            const char * name = EventsName( evnt );
            if ( name ) {
                ival = config.IntParams( name );
                if ( ival ) {
                    const KeySym sym = GetKeySym( ival );
                    key_events[evnt] = sym;
                    DEBUG( DBG_GAME, DBG_INFO, "events: " << EventsName( evnt ) << ", key: " << KeySymGetName( sym ) );
                }
            }
        }
    }
}

void Game::KeyboardGlobalFilter( int sym, int mod )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // system hotkeys
    if ( sym == key_events[EVENT_SYSTEM_FULLSCREEN] && !( ( mod & KMOD_ALT ) || ( mod & KMOD_CTRL ) ) ) {
        Cursor::Get().Hide();
        fheroes2::engine().toggleFullScreen();
        Cursor::Get().Show();
        display.render();
    }
//     else if ( sym == key_events[EVENT_SYSTEM_SCREENSHOT] ) {
//         std::ostringstream stream;
//         stream << System::ConcatePath( Settings::GetSaveDir(), "screenshot_" ) << std::time( 0 );
//
// #ifndef WITH_IMAGE
//         stream << ".bmp";
// #else
//         stream << ".png";
// #endif
//         if ( display.Save( stream.str().c_str() ) )
//             DEBUG( DBG_GAME, DBG_INFO, "save: " << stream.str() );
//     }
    else
        // reserved
        if ( sym == key_events[EVENT_SWITCHGROUP] )
        ++key_groups;
    else if ( sym == key_events[EVENT_SYSTEM_DEBUG1] ) {
        Interface::Basic::Get().EventDebug1();
    }
    else if ( sym == key_events[EVENT_SYSTEM_DEBUG2] ) {
        Interface::Basic::Get().EventDebug2();
    }
}
