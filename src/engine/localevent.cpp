/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2008 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include <SDL_events.h>
#include <SDL_joystick.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>
#include <SDL_version.h>
#include <SDL_video.h>

#if SDL_VERSION_ATLEAST( 2, 0, 0 )

#include <SDL_gamecontroller.h>
#include <SDL_keycode.h>
#include <SDL_touch.h>

#if defined( TARGET_PS_VITA ) || defined( TARGET_NINTENDO_SWITCH ) || defined( ANDROID )
#define TOUCH_SUPPORT
#include <SDL_hints.h>
#endif

#endif

#include "audio.h"
#include "image.h"
#include "localevent.h"
#include "pal.h"
#include "screen.h"
#include "tools.h"

namespace
{
    const uint32_t globalLoopSleepTime{ 1 };

    int getSDLKey( const fheroes2::Key key )
    {
        switch ( key ) {
        case fheroes2::Key::NONE:
        case fheroes2::Key::LAST_KEY:
            return SDLK_UNKNOWN;
        case fheroes2::Key::KEY_BACKSPACE:
            return SDLK_BACKSPACE;
        case fheroes2::Key::KEY_ENTER:
            return SDLK_RETURN;
        case fheroes2::Key::KEY_ESCAPE:
            return SDLK_ESCAPE;
        case fheroes2::Key::KEY_SPACE:
            return SDLK_SPACE;
        case fheroes2::Key::KEY_EXCLAIM:
            return SDLK_EXCLAIM;
        case fheroes2::Key::KEY_DOUBLE_QUOTE:
            return SDLK_QUOTEDBL;
        case fheroes2::Key::KEY_HASH:
            return SDLK_HASH;
        case fheroes2::Key::KEY_DOLLAR:
            return SDLK_DOLLAR;
        case fheroes2::Key::KEY_AMPERSAND:
            return SDLK_AMPERSAND;
        case fheroes2::Key::KEY_QUOTE:
            return SDLK_QUOTE;
        case fheroes2::Key::KEY_LEFT_PARENTHESIS:
            return SDLK_LEFTPAREN;
        case fheroes2::Key::KEY_RIGHT_PARENTHESIS:
            return SDLK_RIGHTPAREN;
        case fheroes2::Key::KEY_ASTERISK:
            return SDLK_ASTERISK;
        case fheroes2::Key::KEY_PLUS:
            return SDLK_PLUS;
        case fheroes2::Key::KEY_COMMA:
            return SDLK_COMMA;
        case fheroes2::Key::KEY_MINUS:
            return SDLK_MINUS;
        case fheroes2::Key::KEY_PERIOD:
            return SDLK_PERIOD;
        case fheroes2::Key::KEY_SLASH:
            return SDLK_SLASH;
        case fheroes2::Key::KEY_COLON:
            return SDLK_COLON;
        case fheroes2::Key::KEY_SEMICOLON:
            return SDLK_SEMICOLON;
        case fheroes2::Key::KEY_LESS:
            return SDLK_LESS;
        case fheroes2::Key::KEY_EQUALS:
            return SDLK_EQUALS;
        case fheroes2::Key::KEY_GREATER:
            return SDLK_GREATER;
        case fheroes2::Key::KEY_QUESTION:
            return SDLK_QUESTION;
        case fheroes2::Key::KEY_AT:
            return SDLK_AT;
        case fheroes2::Key::KEY_LEFT_BRACKET:
            return SDLK_LEFTBRACKET;
        case fheroes2::Key::KEY_BACKSLASH:
            return SDLK_BACKSLASH;
        case fheroes2::Key::KEY_RIGHT_BRACKET:
            return SDLK_RIGHTBRACKET;
        case fheroes2::Key::KEY_CARET:
            return SDLK_CARET;
        case fheroes2::Key::KEY_UNDERSCORE:
            return SDLK_UNDERSCORE;
        case fheroes2::Key::KEY_ALT:
            return SDLK_LALT;
        case fheroes2::Key::KEY_CONTROL:
            return SDLK_LCTRL;
        case fheroes2::Key::KEY_SHIFT:
            return SDLK_LSHIFT;
        case fheroes2::Key::KEY_TAB:
            return SDLK_TAB;
        case fheroes2::Key::KEY_DELETE:
            return SDLK_DELETE;
        case fheroes2::Key::KEY_PAGE_UP:
            return SDLK_PAGEUP;
        case fheroes2::Key::KEY_PAGE_DOWN:
            return SDLK_PAGEDOWN;
        case fheroes2::Key::KEY_F1:
            return SDLK_F1;
        case fheroes2::Key::KEY_F2:
            return SDLK_F2;
        case fheroes2::Key::KEY_F3:
            return SDLK_F3;
        case fheroes2::Key::KEY_F4:
            return SDLK_F4;
        case fheroes2::Key::KEY_F5:
            return SDLK_F5;
        case fheroes2::Key::KEY_F6:
            return SDLK_F6;
        case fheroes2::Key::KEY_F7:
            return SDLK_F7;
        case fheroes2::Key::KEY_F8:
            return SDLK_F8;
        case fheroes2::Key::KEY_F9:
            return SDLK_F9;
        case fheroes2::Key::KEY_F10:
            return SDLK_F10;
        case fheroes2::Key::KEY_F11:
            return SDLK_F11;
        case fheroes2::Key::KEY_F12:
            return SDLK_F12;
        case fheroes2::Key::KEY_LEFT:
            return SDLK_LEFT;
        case fheroes2::Key::KEY_RIGHT:
            return SDLK_RIGHT;
        case fheroes2::Key::KEY_UP:
            return SDLK_UP;
        case fheroes2::Key::KEY_DOWN:
            return SDLK_DOWN;
        case fheroes2::Key::KEY_0:
            return SDLK_0;
        case fheroes2::Key::KEY_1:
            return SDLK_1;
        case fheroes2::Key::KEY_2:
            return SDLK_2;
        case fheroes2::Key::KEY_3:
            return SDLK_3;
        case fheroes2::Key::KEY_4:
            return SDLK_4;
        case fheroes2::Key::KEY_5:
            return SDLK_5;
        case fheroes2::Key::KEY_6:
            return SDLK_6;
        case fheroes2::Key::KEY_7:
            return SDLK_7;
        case fheroes2::Key::KEY_8:
            return SDLK_8;
        case fheroes2::Key::KEY_9:
            return SDLK_9;
        case fheroes2::Key::KEY_A:
            return SDLK_a;
        case fheroes2::Key::KEY_B:
            return SDLK_b;
        case fheroes2::Key::KEY_C:
            return SDLK_c;
        case fheroes2::Key::KEY_D:
            return SDLK_d;
        case fheroes2::Key::KEY_E:
            return SDLK_e;
        case fheroes2::Key::KEY_F:
            return SDLK_f;
        case fheroes2::Key::KEY_G:
            return SDLK_g;
        case fheroes2::Key::KEY_H:
            return SDLK_h;
        case fheroes2::Key::KEY_I:
            return SDLK_i;
        case fheroes2::Key::KEY_J:
            return SDLK_j;
        case fheroes2::Key::KEY_K:
            return SDLK_k;
        case fheroes2::Key::KEY_L:
            return SDLK_l;
        case fheroes2::Key::KEY_M:
            return SDLK_m;
        case fheroes2::Key::KEY_N:
            return SDLK_n;
        case fheroes2::Key::KEY_O:
            return SDLK_o;
        case fheroes2::Key::KEY_P:
            return SDLK_p;
        case fheroes2::Key::KEY_Q:
            return SDLK_q;
        case fheroes2::Key::KEY_R:
            return SDLK_r;
        case fheroes2::Key::KEY_S:
            return SDLK_s;
        case fheroes2::Key::KEY_T:
            return SDLK_t;
        case fheroes2::Key::KEY_U:
            return SDLK_u;
        case fheroes2::Key::KEY_V:
            return SDLK_v;
        case fheroes2::Key::KEY_W:
            return SDLK_w;
        case fheroes2::Key::KEY_X:
            return SDLK_x;
        case fheroes2::Key::KEY_Y:
            return SDLK_y;
        case fheroes2::Key::KEY_Z:
            return SDLK_z;
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        case fheroes2::Key::KEY_PRINT:
            return SDLK_PRINTSCREEN;
        case fheroes2::Key::KEY_KP_0:
            return SDLK_KP_0;
        case fheroes2::Key::KEY_KP_1:
            return SDLK_KP_1;
        case fheroes2::Key::KEY_KP_2:
            return SDLK_KP_2;
        case fheroes2::Key::KEY_KP_3:
            return SDLK_KP_3;
        case fheroes2::Key::KEY_KP_4:
            return SDLK_KP_4;
        case fheroes2::Key::KEY_KP_5:
            return SDLK_KP_5;
        case fheroes2::Key::KEY_KP_6:
            return SDLK_KP_6;
        case fheroes2::Key::KEY_KP_7:
            return SDLK_KP_7;
        case fheroes2::Key::KEY_KP_8:
            return SDLK_KP_8;
        case fheroes2::Key::KEY_KP_9:
            return SDLK_KP_9;
#else
        case fheroes2::Key::KEY_PRINT:
            return SDLK_PRINT;
        case fheroes2::Key::KEY_KP_0:
            return SDLK_KP0;
        case fheroes2::Key::KEY_KP_1:
            return SDLK_KP1;
        case fheroes2::Key::KEY_KP_2:
            return SDLK_KP2;
        case fheroes2::Key::KEY_KP_3:
            return SDLK_KP3;
        case fheroes2::Key::KEY_KP_4:
            return SDLK_KP4;
        case fheroes2::Key::KEY_KP_5:
            return SDLK_KP5;
        case fheroes2::Key::KEY_KP_6:
            return SDLK_KP6;
        case fheroes2::Key::KEY_KP_7:
            return SDLK_KP7;
        case fheroes2::Key::KEY_KP_8:
            return SDLK_KP8;
        case fheroes2::Key::KEY_KP_9:
            return SDLK_KP9;
#endif
        case fheroes2::Key::KEY_KP_PERIOD:
            return SDLK_KP_PERIOD;
        case fheroes2::Key::KEY_KP_DIVIDE:
            return SDLK_KP_DIVIDE;
        case fheroes2::Key::KEY_KP_MULTIPLY:
            return SDLK_KP_MULTIPLY;
        case fheroes2::Key::KEY_KP_MINUS:
            return SDLK_KP_MINUS;
        case fheroes2::Key::KEY_KP_PLUS:
            return SDLK_KP_PLUS;
        case fheroes2::Key::KEY_KP_ENTER:
            return SDLK_KP_ENTER;
        case fheroes2::Key::KEY_KP_EQUALS:
            return SDLK_KP_EQUALS;
        case fheroes2::Key::KEY_HOME:
            return SDLK_HOME;
        case fheroes2::Key::KEY_END:
            return SDLK_END;
        default:
            // Did you add a new key? Add the logic above!
            assert( 0 );
            break;
        }

        return SDLK_UNKNOWN;
    }

    fheroes2::Key getKeyFromSDL( int sdlKey )
    {
        // SDL interprets keyboard Numpad Enter as a separate key. However, in the game we should handle it in the same way as the normal Enter.
        if ( sdlKey == SDLK_KP_ENTER ) {
            sdlKey = SDLK_RETURN;
        }

        static std::map<int, fheroes2::Key> sdlValueToKey;
        if ( sdlValueToKey.empty() ) {
            // The map is empty let's populate it.
            for ( int32_t i = static_cast<int32_t>( fheroes2::Key::NONE ); i < static_cast<int32_t>( fheroes2::Key::LAST_KEY ); ++i ) {
                const fheroes2::Key key = static_cast<fheroes2::Key>( i );
                sdlValueToKey.emplace( getSDLKey( key ), key );
            }
        }

        auto iter = sdlValueToKey.find( sdlKey );
        if ( iter == sdlValueToKey.end() ) {
            return fheroes2::Key::NONE;
        }

        return iter->second;
    }

    int32_t getKeyModifierFromSDL( const int sdlModifier )
    {
        int32_t modifier = fheroes2::KeyModifier::KEY_MODIFIER_NONE;
        if ( sdlModifier & KMOD_CTRL ) {
            modifier |= fheroes2::KeyModifier::KEY_MODIFIER_CTRL;
        }
        if ( sdlModifier & KMOD_SHIFT ) {
            modifier |= fheroes2::KeyModifier::KEY_MODIFIER_SHIFT;
        }
        if ( sdlModifier & KMOD_ALT ) {
            modifier |= fheroes2::KeyModifier::KEY_MODIFIER_ALT;
        }
        if ( sdlModifier & KMOD_CAPS ) {
            modifier |= fheroes2::KeyModifier::KEY_MODIFIER_CAPS;
        }
        if ( sdlModifier & KMOD_NUM ) {
            modifier |= fheroes2::KeyModifier::KEY_MODIFIER_NUM;
        }

        return modifier;
    }

    char getCharacterFromPressedKey( const fheroes2::Key key, const int32_t mod )
    {
        switch ( key ) {
        case fheroes2::Key::KEY_1:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '!' : '1' );
        case fheroes2::Key::KEY_2:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '@' : '2' );
        case fheroes2::Key::KEY_3:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '#' : '3' );
        case fheroes2::Key::KEY_4:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '$' : '4' );
        case fheroes2::Key::KEY_5:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '%' : '5' );
        case fheroes2::Key::KEY_6:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '^' : '6' );
        case fheroes2::Key::KEY_7:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '&' : '7' );
        case fheroes2::Key::KEY_8:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '*' : '8' );
        case fheroes2::Key::KEY_9:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '(' : '9' );
        case fheroes2::Key::KEY_0:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? ')' : '0' );
        case fheroes2::Key::KEY_KP_0:
            if ( fheroes2::KeyModifier::KEY_MODIFIER_NUM & mod )
                return '0';
            break;
        case fheroes2::Key::KEY_KP_1:
            if ( fheroes2::KeyModifier::KEY_MODIFIER_NUM & mod )
                return '1';
            break;
        case fheroes2::Key::KEY_KP_2:
            if ( fheroes2::KeyModifier::KEY_MODIFIER_NUM & mod )
                return '2';
            break;
        case fheroes2::Key::KEY_KP_3:
            if ( fheroes2::KeyModifier::KEY_MODIFIER_NUM & mod )
                return '3';
            break;
        case fheroes2::Key::KEY_KP_4:
            if ( fheroes2::KeyModifier::KEY_MODIFIER_NUM & mod )
                return '4';
            break;
        case fheroes2::Key::KEY_KP_5:
            if ( fheroes2::KeyModifier::KEY_MODIFIER_NUM & mod )
                return '5';
            break;
        case fheroes2::Key::KEY_KP_6:
            if ( fheroes2::KeyModifier::KEY_MODIFIER_NUM & mod )
                return '6';
            break;
        case fheroes2::Key::KEY_KP_7:
            if ( fheroes2::KeyModifier::KEY_MODIFIER_NUM & mod )
                return '7';
            break;
        case fheroes2::Key::KEY_KP_8:
            if ( fheroes2::KeyModifier::KEY_MODIFIER_NUM & mod )
                return '8';
            break;
        case fheroes2::Key::KEY_KP_9:
            if ( fheroes2::KeyModifier::KEY_MODIFIER_NUM & mod )
                return '9';
            break;
        case fheroes2::Key::KEY_MINUS:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '_' : '-' );
        case fheroes2::Key::KEY_EQUALS:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '+' : '=' );
        case fheroes2::Key::KEY_BACKSLASH:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '|' : '\\' );
        case fheroes2::Key::KEY_LEFT_BRACKET:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '{' : '[' );
        case fheroes2::Key::KEY_RIGHT_BRACKET:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '}' : ']' );
        case fheroes2::Key::KEY_SEMICOLON:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? ':' : ';' );
        case fheroes2::Key::KEY_QUOTE:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '"' : '\'' );
        case fheroes2::Key::KEY_COMMA:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '<' : ',' );
        case fheroes2::Key::KEY_PERIOD:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '>' : '.' );
        case fheroes2::Key::KEY_SLASH:
            return ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT & mod ? '?' : '/' );
        case fheroes2::Key::KEY_EXCLAIM:
            return '!';
        case fheroes2::Key::KEY_AT:
            return '@';
        case fheroes2::Key::KEY_HASH:
            return '#';
        case fheroes2::Key::KEY_DOLLAR:
            return '$';
        case fheroes2::Key::KEY_AMPERSAND:
            return '&';
        case fheroes2::Key::KEY_ASTERISK:
            return '*';
        case fheroes2::Key::KEY_LEFT_PARENTHESIS:
            return '(';
        case fheroes2::Key::KEY_RIGHT_PARENTHESIS:
            return ')';
        case fheroes2::Key::KEY_DOUBLE_QUOTE:
            return '"';
        case fheroes2::Key::KEY_PLUS:
            return '+';
        case fheroes2::Key::KEY_COLON:
            return ':';
        case fheroes2::Key::KEY_LESS:
            return '<';
        case fheroes2::Key::KEY_GREATER:
            return '>';
        case fheroes2::Key::KEY_QUESTION:
            return '?';
        case fheroes2::Key::KEY_CARET:
            return '^';
        case fheroes2::Key::KEY_UNDERSCORE:
            return '_';
        case fheroes2::Key::KEY_SPACE:
            return ' ';
        case fheroes2::Key::KEY_A:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'A' : 'a' );
        case fheroes2::Key::KEY_B:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'B' : 'b' );
        case fheroes2::Key::KEY_C:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'C' : 'c' );
        case fheroes2::Key::KEY_D:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'D' : 'd' );
        case fheroes2::Key::KEY_E:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'E' : 'e' );
        case fheroes2::Key::KEY_F:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'F' : 'f' );
        case fheroes2::Key::KEY_G:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'G' : 'g' );
        case fheroes2::Key::KEY_H:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'H' : 'h' );
        case fheroes2::Key::KEY_I:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'I' : 'i' );
        case fheroes2::Key::KEY_J:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'J' : 'j' );
        case fheroes2::Key::KEY_K:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'K' : 'k' );
        case fheroes2::Key::KEY_L:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'L' : 'l' );
        case fheroes2::Key::KEY_M:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'M' : 'm' );
        case fheroes2::Key::KEY_N:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'N' : 'n' );
        case fheroes2::Key::KEY_O:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'O' : 'o' );
        case fheroes2::Key::KEY_P:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'P' : 'p' );
        case fheroes2::Key::KEY_Q:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'Q' : 'q' );
        case fheroes2::Key::KEY_R:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'R' : 'r' );
        case fheroes2::Key::KEY_S:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'S' : 's' );
        case fheroes2::Key::KEY_T:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'T' : 't' );
        case fheroes2::Key::KEY_U:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'U' : 'u' );
        case fheroes2::Key::KEY_V:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'V' : 'v' );
        case fheroes2::Key::KEY_W:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'W' : 'w' );
        case fheroes2::Key::KEY_X:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'X' : 'x' );
        case fheroes2::Key::KEY_Y:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'Y' : 'y' );
        case fheroes2::Key::KEY_Z:
            return ( ( fheroes2::KeyModifier::KEY_MODIFIER_SHIFT | fheroes2::KeyModifier::KEY_MODIFIER_CAPS ) & mod ? 'Z' : 'z' );
        default:
            break;
        }

        return 0;
    }

#if defined( TARGET_PS_VITA )
    const int totalCharactersDPad = 38;
    bool dpadInputActive = false;
    bool currentUpper = false;
    int currentCharIndex = 0;

    const fheroes2::Key dPadKeys[totalCharactersDPad] = {
        // lowercase letters
        fheroes2::Key::KEY_A, fheroes2::Key::KEY_B, fheroes2::Key::KEY_C, fheroes2::Key::KEY_D, fheroes2::Key::KEY_E, fheroes2::Key::KEY_F, fheroes2::Key::KEY_G,
        fheroes2::Key::KEY_H, fheroes2::Key::KEY_I, fheroes2::Key::KEY_J, fheroes2::Key::KEY_K, fheroes2::Key::KEY_L, fheroes2::Key::KEY_M, fheroes2::Key::KEY_N,
        fheroes2::Key::KEY_O, fheroes2::Key::KEY_P, fheroes2::Key::KEY_Q, fheroes2::Key::KEY_R, fheroes2::Key::KEY_S, fheroes2::Key::KEY_T, fheroes2::Key::KEY_U,
        fheroes2::Key::KEY_V, fheroes2::Key::KEY_W, fheroes2::Key::KEY_X, fheroes2::Key::KEY_Y, fheroes2::Key::KEY_Z,
        // space, underscore
        fheroes2::Key::KEY_SPACE, fheroes2::Key::KEY_UNDERSCORE,
        // nums
        fheroes2::Key::KEY_0, fheroes2::Key::KEY_1, fheroes2::Key::KEY_2, fheroes2::Key::KEY_3, fheroes2::Key::KEY_4, fheroes2::Key::KEY_5, fheroes2::Key::KEY_6,
        fheroes2::Key::KEY_7, fheroes2::Key::KEY_8, fheroes2::Key::KEY_9 };

    char GetCurrentDPadChar()
    {
        return getCharacterFromPressedKey( dPadKeys[currentCharIndex],
                                           currentUpper ? fheroes2::KeyModifier::KEY_MODIFIER_CAPS : fheroes2::KeyModifier::KEY_MODIFIER_NONE );
    }

    fheroes2::Key KeySymFromChar( const char c )
    {
        switch ( c ) {
        case '!':
            return fheroes2::Key::KEY_EXCLAIM;
        case '"':
            return fheroes2::Key::KEY_DOUBLE_QUOTE;
        case '#':
            return fheroes2::Key::KEY_HASH;
        case '$':
            return fheroes2::Key::KEY_DOLLAR;
        case '&':
            return fheroes2::Key::KEY_AMPERSAND;
        case '\'':
            return fheroes2::Key::KEY_QUOTE;
        case '(':
            return fheroes2::Key::KEY_LEFT_PARENTHESIS;
        case ')':
            return fheroes2::Key::KEY_RIGHT_PARENTHESIS;
        case '*':
            return fheroes2::Key::KEY_ASTERISK;
        case '+':
            return fheroes2::Key::KEY_PLUS;
        case ',':
            return fheroes2::Key::KEY_COMMA;
        case '-':
            return fheroes2::Key::KEY_MINUS;
        case '.':
            return fheroes2::Key::KEY_PERIOD;
        case '/':
            return fheroes2::Key::KEY_SLASH;
        case ':':
            return fheroes2::Key::KEY_COLON;
        case ';':
            return fheroes2::Key::KEY_SEMICOLON;
        case '<':
            return fheroes2::Key::KEY_LESS;
        case '=':
            return fheroes2::Key::KEY_EQUALS;
        case '>':
            return fheroes2::Key::KEY_GREATER;
        case '?':
            return fheroes2::Key::KEY_QUESTION;
        case '@':
            return fheroes2::Key::KEY_AT;
        case '[':
            return fheroes2::Key::KEY_LEFT_BRACKET;
        case '\\':
            return fheroes2::Key::KEY_BACKSLASH;
        case ']':
            return fheroes2::Key::KEY_RIGHT_BRACKET;
        case '^':
            return fheroes2::Key::KEY_CARET;
        case '_':
            return fheroes2::Key::KEY_UNDERSCORE;
        case ' ':
            return fheroes2::Key::KEY_SPACE;
        case 'a':
            return fheroes2::Key::KEY_A;
        case 'b':
            return fheroes2::Key::KEY_B;
        case 'c':
            return fheroes2::Key::KEY_C;
        case 'd':
            return fheroes2::Key::KEY_D;
        case 'e':
            return fheroes2::Key::KEY_E;
        case 'f':
            return fheroes2::Key::KEY_F;
        case 'g':
            return fheroes2::Key::KEY_G;
        case 'h':
            return fheroes2::Key::KEY_H;
        case 'i':
            return fheroes2::Key::KEY_I;
        case 'j':
            return fheroes2::Key::KEY_J;
        case 'k':
            return fheroes2::Key::KEY_K;
        case 'l':
            return fheroes2::Key::KEY_L;
        case 'm':
            return fheroes2::Key::KEY_M;
        case 'n':
            return fheroes2::Key::KEY_N;
        case 'o':
            return fheroes2::Key::KEY_O;
        case 'p':
            return fheroes2::Key::KEY_P;
        case 'q':
            return fheroes2::Key::KEY_Q;
        case 'r':
            return fheroes2::Key::KEY_R;
        case 's':
            return fheroes2::Key::KEY_S;
        case 't':
            return fheroes2::Key::KEY_T;
        case 'u':
            return fheroes2::Key::KEY_U;
        case 'v':
            return fheroes2::Key::KEY_V;
        case 'w':
            return fheroes2::Key::KEY_W;
        case 'x':
            return fheroes2::Key::KEY_X;
        case 'y':
            return fheroes2::Key::KEY_Y;
        case 'z':
            return fheroes2::Key::KEY_Z;
        case '0':
            return fheroes2::Key::KEY_0;
        case '1':
            return fheroes2::Key::KEY_1;
        case '2':
            return fheroes2::Key::KEY_2;
        case '3':
            return fheroes2::Key::KEY_3;
        case '4':
            return fheroes2::Key::KEY_4;
        case '5':
            return fheroes2::Key::KEY_5;
        case '6':
            return fheroes2::Key::KEY_6;
        case '7':
            return fheroes2::Key::KEY_7;
        case '8':
            return fheroes2::Key::KEY_8;
        case '9':
            return fheroes2::Key::KEY_9;
        default:
            break;
        }
        return fheroes2::Key::NONE;
    }

    void SetCurrentDPadCharIndex( char currentChar )
    {
        if ( currentChar >= 'A' && currentChar <= 'Z' ) {
            currentUpper = true;
            currentChar += 32;
        }

        const fheroes2::Key keySym = KeySymFromChar( currentChar );
        for ( int i = 0; i < totalCharactersDPad; ++i ) {
            if ( dPadKeys[i] == keySym ) {
                currentCharIndex = i;
                return;
            }
        }

        currentCharIndex = 0;
    }
#endif

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    std::set<uint32_t> eventTypeStatus;

    void setEventProcessingState( const uint32_t eventType, const bool enable )
    {
        eventTypeStatus.emplace( eventType );
        SDL_EventState( eventType, ( enable ? SDL_ENABLE : SDL_IGNORE ) );
    }
#else
    std::set<uint8_t> eventTypeStatus;

    void setEventProcessingState( const uint8_t eventType, const bool enable )
    {
        eventTypeStatus.emplace( eventType );
        SDL_EventState( eventType, ( enable ? SDL_ENABLE : SDL_IGNORE ) );
    }
#endif
}

// Custom button mapping for Nintendo Switch
#if defined( TARGET_NINTENDO_SWITCH )
#undef SDL_CONTROLLER_BUTTON_A
#undef SDL_CONTROLLER_BUTTON_B
#undef SDL_CONTROLLER_BUTTON_DPAD_LEFT
#undef SDL_CONTROLLER_BUTTON_DPAD_RIGHT
#undef SDL_CONTROLLER_BUTTON_DPAD_UP
#undef SDL_CONTROLLER_BUTTON_DPAD_DOWN
#define SDL_CONTROLLER_BUTTON_A 1
#define SDL_CONTROLLER_BUTTON_B 0
#define SDL_CONTROLLER_BUTTON_DPAD_LEFT 13
#define SDL_CONTROLLER_BUTTON_DPAD_RIGHT 14
#define SDL_CONTROLLER_BUTTON_DPAD_UP 11
#define SDL_CONTROLLER_BUTTON_DPAD_DOWN 12

enum SwitchJoyconKeys
{
    SWITCH_BUTTON_Y = 2,
    SWITCH_BUTTON_X = 3,
    SWITCH_BUTTON_MINUS = 4,
    SWITCH_BUTTON_PLUS = 6,
    SWITCH_BUTTON_L = 9,
    SWITCH_BUTTON_R = 10
};

#endif

namespace fheroes2
{
    const char * KeySymGetName( const Key key )
    {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        return SDL_GetKeyName( static_cast<SDL_Keycode>( getSDLKey( key ) ) );
#else
        return SDL_GetKeyName( static_cast<SDLKey>( getSDLKey( key ) ) );
#endif
    }

    bool PressIntKey( uint32_t max, uint32_t & result )
    {
        const LocalEvent & le = LocalEvent::Get();

        if ( le.KeyPress( fheroes2::Key::KEY_BACKSPACE ) ) {
            result /= 10;
            return true;
        }

        if ( !le.KeyPress() ) {
            // No key is pressed.
            return false;
        }

        if ( le.KeyValue() >= fheroes2::Key::KEY_0 && le.KeyValue() <= fheroes2::Key::KEY_9 ) {
            if ( max <= result ) {
                // We reached the maximum.
                return true;
            }

            result *= 10;

            result += static_cast<uint32_t>( static_cast<int32_t>( le.KeyValue() ) - static_cast<int32_t>( fheroes2::Key::KEY_0 ) );

            if ( result > max ) {
                result = max;
            }

            return true;
        }

        if ( le.KeyValue() >= fheroes2::Key::KEY_KP_0 && le.KeyValue() <= fheroes2::Key::KEY_KP_9 ) {
            if ( max <= result ) {
                // We reached the maximum.
                return true;
            }

            result *= 10;

            result += static_cast<uint32_t>( static_cast<int32_t>( le.KeyValue() ) - static_cast<int32_t>( fheroes2::Key::KEY_KP_0 ) );

            if ( result > max ) {
                result = max;
            }

            return true;
        }

        return false;
    }

    size_t InsertKeySym( std::string & res, size_t pos, const Key key, const int32_t mod )
    {
#if defined( TARGET_PS_VITA )
        (void)mod;

        // input with D-Pad
        if ( res.size() ) {
            SetCurrentDPadCharIndex( res.back() );
        }
        else {
            currentUpper = true;
            currentCharIndex = 0;
        }

        switch ( key ) {
        // delete char
        case fheroes2::Key::KEY_KP_4: {
            if ( !res.empty() && pos ) {
                res.resize( res.size() - 1 );
                --pos;
            }
            break;
        }
        // add new char
        case fheroes2::Key::KEY_KP_6: {
            currentUpper = res.empty();
            currentCharIndex = 0;

            const char c = GetCurrentDPadChar();
            if ( c )
                res.push_back( c );

            ++pos;
            break;
        }
        // next char
        case fheroes2::Key::KEY_KP_2: {
            ++currentCharIndex;
            if ( currentCharIndex >= totalCharactersDPad )
                currentCharIndex = 0;

            if ( !res.empty() ) {
                res.resize( res.size() - 1 );
            }
            else {
                ++pos;
            }

            const char c = GetCurrentDPadChar();
            if ( c )
                res.push_back( c );

            break;
        }
        // previous char
        case fheroes2::Key::KEY_KP_8: {
            --currentCharIndex;
            if ( currentCharIndex < 0 )
                currentCharIndex = totalCharactersDPad - 1;

            if ( !res.empty() ) {
                res.resize( res.size() - 1 );
            }
            else {
                ++pos;
            }

            const char c = GetCurrentDPadChar();
            if ( c )
                res.push_back( c );

            break;
        }
        // switch uppler/lowercase
        case fheroes2::Key::KEY_SHIFT: {
            currentUpper = !currentUpper;

            if ( !res.empty() ) {
                res.resize( res.size() - 1 );
            }
            else {
                ++pos;
            }

            const char c = GetCurrentDPadChar();
            if ( c )
                res.push_back( c );

            break;
        }

        default:
            break;
        }
#else
        switch ( key ) {
        case fheroes2::Key::KEY_BACKSPACE:
            if ( !res.empty() && pos ) {
                if ( pos >= res.size() )
                    res.resize( res.size() - 1 );
                else
                    res.erase( pos - 1, 1 );
                --pos;
            }
            break;
        case fheroes2::Key::KEY_DELETE:
            if ( !res.empty() ) {
                if ( pos < res.size() )
                    res.erase( pos, 1 );
            }
            break;

        case fheroes2::Key::KEY_LEFT:
            if ( pos )
                --pos;
            break;
        case fheroes2::Key::KEY_RIGHT:
            if ( pos < res.size() )
                ++pos;
            break;
        case fheroes2::Key::KEY_HOME:
            pos = 0;
            break;
        case fheroes2::Key::KEY_END:
            pos = res.size();
            break;

        default: {
            char c = getCharacterFromPressedKey( key, mod );

            if ( c ) {
                res.insert( pos, 1, c );
                ++pos;
            }
        }
        }
#endif

        return pos;
    }
}

LocalEvent::LocalEvent()
    : modes( 0 )
    , key_value( fheroes2::Key::NONE )
    , mouse_button( 0 )
{}

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
void LocalEvent::OpenController()
{
    for ( int i = 0; i < SDL_NumJoysticks(); ++i ) {
        if ( SDL_IsGameController( i ) ) {
            _gameController = SDL_GameControllerOpen( i );
            if ( _gameController != nullptr ) {
                fheroes2::cursor().enableSoftwareEmulation( true );
                break;
            }
        }
    }
}

void LocalEvent::CloseController()
{
    if ( SDL_GameControllerGetAttached( _gameController ) ) {
        SDL_GameControllerClose( _gameController );
        _gameController = nullptr;
    }
}

void LocalEvent::OpenTouchpad()
{
#if defined( TOUCH_SUPPORT )
    const int touchNumber = SDL_GetNumTouchDevices();
    if ( touchNumber > 0 ) {
        fheroes2::cursor().enableSoftwareEmulation( true );
#if SDL_VERSION_ATLEAST( 2, 0, 10 )
        SDL_SetHint( SDL_HINT_TOUCH_MOUSE_EVENTS, "0" );
#endif
    }
#endif
}

#else
void LocalEvent::OpenController()
{
    // Do nothing.
}
void LocalEvent::CloseController()
{
    // Do nothing.
}

void OpenTouchpad()
{
    // Do nothing.
}
#endif

void LocalEvent::OpenVirtualKeyboard()
{
#if defined( TARGET_PS_VITA )
    dpadInputActive = true;
#elif defined( ANDROID )
    // Here we should use SDL_StartTextInput() call to open a keyboard.
#endif
}

void LocalEvent::CloseVirtualKeyboard()
{
#if defined( TARGET_PS_VITA )
    dpadInputActive = false;
#elif defined( ANDROID )
    // Here we should use SDL_StopTextInput() call to close a keyboard.
#endif
}

namespace
{
    class ColorCycling
    {
    public:
        ColorCycling()
            : _counter( 0 )
            , _isPaused( false )
            , _preRenderDrawing( nullptr )
            , _posRenderDrawing( nullptr )
        {}

        bool applyCycling( std::vector<uint8_t> & palette )
        {
            if ( _preRenderDrawing != nullptr )
                _preRenderDrawing();

            if ( _timer.getMs() >= 220 ) {
                _timer.reset();
                palette = PAL::GetCyclingPalette( _counter );
                ++_counter;
                return true;
            }
            return false;
        }

        void reset()
        {
            _prevDraw.reset();

            if ( _posRenderDrawing != nullptr )
                _posRenderDrawing();
        }

        bool isRedrawRequired() const
        {
            return !_isPaused && _prevDraw.getMs() >= 220;
        }

        void registerDrawing( void ( *preRenderDrawing )(), void ( *postRenderDrawing )() )
        {
            if ( preRenderDrawing != nullptr )
                _preRenderDrawing = preRenderDrawing;

            if ( postRenderDrawing != nullptr )
                _posRenderDrawing = postRenderDrawing;
        }

        void pause()
        {
            _isPaused = true;
        }

        void resume()
        {
            _isPaused = false;
            _prevDraw.reset();
            _timer.reset();
        }

    private:
        fheroes2::Time _timer;
        fheroes2::Time _prevDraw;
        uint32_t _counter;
        bool _isPaused;

        void ( *_preRenderDrawing )();
        void ( *_posRenderDrawing )();
    };

    ColorCycling colorCycling;

    bool ApplyCycling( std::vector<uint8_t> & palette )
    {
        return colorCycling.applyCycling( palette );
    }

    void ResetCycling()
    {
        colorCycling.reset();
    }
}

LocalEvent & LocalEvent::Get()
{
    static LocalEvent le;

    return le;
}

void LocalEvent::RegisterCycling( void ( *preRenderDrawing )(), void ( *postRenderDrawing )() )
{
    colorCycling.registerDrawing( preRenderDrawing, postRenderDrawing );
    colorCycling.resume();

    fheroes2::Display::instance().subscribe( ApplyCycling, ResetCycling );
}

void LocalEvent::PauseCycling()
{
    colorCycling.pause();
    fheroes2::Display::instance().subscribe( nullptr, nullptr );
}

LocalEvent & LocalEvent::GetClean()
{
    LocalEvent & le = Get();

    le.ResetModes( KEY_PRESSED );
    le.ResetModes( MOUSE_MOTION );
    le.ResetModes( MOUSE_PRESSED );
    le.ResetModes( MOUSE_RELEASED );
    le.ResetModes( MOUSE_CLICKED );
    le.ResetModes( MOUSE_WHEEL );
    le.ResetModes( KEY_HOLD );

    return le;
}

bool LocalEvent::HandleEvents( const bool sleepAfterEventProcessing, const bool allowExit /* = false */ )
{
    // Event processing might be computationally heavy.
    // We want to make sure that we do not slow down by going into sleep mode when it is not needed.
    const fheroes2::Time eventProcessingTimer;

    // We can have more than one event which requires rendering. We must render only once and only when sleeping is excepted.
    fheroes2::Rect renderRoi;

    // Mouse area must be updated only once so we will use only the latest area for rendering.
    _mouseCursorRenderArea = {};

    fheroes2::Display & display = fheroes2::Display::instance();

    if ( colorCycling.isRedrawRequired() ) {
        // To maintain color cycling animation we need to render the whole frame with an updated palette.
        renderRoi = { 0, 0, display.width(), display.height() };
    }

    SDL_Event event;

    // We shouldn't reset the MOUSE_PRESSED and KEY_HOLD here because these are "lasting" states
    ResetModes( KEY_PRESSED );
    ResetModes( MOUSE_MOTION );
    ResetModes( MOUSE_RELEASED );
    ResetModes( MOUSE_CLICKED );
    ResetModes( MOUSE_WHEEL );

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    while ( SDL_PollEvent( &event ) ) {
        switch ( event.type ) {
        case SDL_WINDOWEVENT:
            if ( event.window.event == SDL_WINDOWEVENT_CLOSE ) {
                // A special case since we need to exit the loop.
                if ( allowExit ) {
                    // Try to perform clear exit to catch all memory leaks, for example.
                    return false;
                }
                break;
            }
            if ( HandleWindowEvent( event.window ) ) {
                renderRoi = { 0, 0, display.width(), display.height() };
            }
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            HandleKeyboardEvent( event.key );
            break;
        case SDL_MOUSEMOTION:
            HandleMouseMotionEvent( event.motion );
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            HandleMouseButtonEvent( event.button );
            break;
        case SDL_MOUSEWHEEL:
            HandleMouseWheelEvent( event.wheel );
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if ( _gameController != nullptr ) {
                const SDL_GameController * removedController = SDL_GameControllerFromInstanceID( event.jdevice.which );
                if ( removedController == _gameController ) {
                    SDL_GameControllerClose( _gameController );
                    _gameController = nullptr;
                }
            }
            break;
        case SDL_CONTROLLERDEVICEADDED:
            if ( _gameController == nullptr ) {
                _gameController = SDL_GameControllerOpen( event.jdevice.which );
                if ( _gameController != nullptr ) {
                    fheroes2::cursor().enableSoftwareEmulation( true );
                }
            }
            break;
        case SDL_CONTROLLERAXISMOTION:
            HandleControllerAxisEvent( event.caxis );
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            HandleControllerButtonEvent( event.cbutton );
            break;
        case SDL_FINGERDOWN:
        case SDL_FINGERUP:
        case SDL_FINGERMOTION:
#if defined( TOUCH_SUPPORT )
            HandleTouchEvent( event.tfinger );
#endif
            break;
        case SDL_RENDER_TARGETS_RESET:
            // We need to just update the screen. This event usually happens when we switch between fullscreen and windowed modes.
            renderRoi = { 0, 0, display.width(), display.height() };
            break;
        case SDL_RENDER_DEVICE_RESET:
            HandleRenderDeviceResetEvent();
            renderRoi = { 0, 0, display.width(), display.height() };
            break;
        case SDL_TEXTINPUT:
            // Keyboard events on Android should be processed here. Use event.text.text to extract text input.
            break;
        case SDL_TEXTEDITING:
            // An event when a user pressed a button on a keyboard. Not all buttons are supported. This event should be used mainly on Android devices.
            break;
        case SDL_QUIT:
            if ( allowExit ) {
                // Try to perform clear exit to catch all memory leaks, for example.
                return false;
            }
            break;
        default:
            // If this assertion blows up then we included an event type but we didn't add logic for it.
            assert( eventTypeStatus.count( event.type ) == 0 );

            // This is a new event type which we do not handle. It might have been added in a newer version of SDL.
            break;
        }
    }
#else
    while ( SDL_PollEvent( &event ) ) {
        switch ( event.type ) {
        case SDL_ACTIVEEVENT:
            if ( HandleActiveEvent( event.active ) ) {
                renderRoi = { 0, 0, display.width(), display.height() };
            }
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            HandleKeyboardEvent( event.key );
            break;
        case SDL_MOUSEMOTION:
            HandleMouseMotionEvent( event.motion );
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            HandleMouseButtonEvent( event.button );
            break;
        case SDL_QUIT:
            if ( allowExit ) {
                // Try to perform clear exit to catch all memory leaks, for example.
                return false;
            }
            break;
        default:
            // If this assertion blows up then we included an event type but we didn't add logic for it.
            assert( eventTypeStatus.count( event.type ) == 0 );

            // This is a new event type which we do not handle. It might have been added in a newer version of SDL.
            break;
        }

        if ( SDL_BUTTON_WHEELDOWN == event.button.button || SDL_BUTTON_WHEELUP == event.button.button )
            break;
    }
#endif

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    if ( _gameController != nullptr ) {
        ProcessControllerAxisMotion();
    }
#endif

    renderRoi = fheroes2::getBoundaryRect( renderRoi, _mouseCursorRenderArea );

    if ( sleepAfterEventProcessing ) {
        if ( renderRoi != fheroes2::Rect() ) {
            display.render( renderRoi );
        }

        // Make sure not to delay any further if the processing time within this function was more than the expected waiting time.
        if ( eventProcessingTimer.getMs() < globalLoopSleepTime ) {
            static_assert( globalLoopSleepTime == 1, "Make sure that you sleep for the difference between times since you change the sleep time." );
            SDL_Delay( globalLoopSleepTime );
        }
    }
    else {
        // Since rendering is going to be just after the call of this method we need to update rendering area only.
        if ( renderRoi != fheroes2::Rect() ) {
            display.updateNextRenderRoi( renderRoi );
        }
    }

    return true;
}

void LocalEvent::StopSounds()
{
    Audio::Mute();
}

void LocalEvent::ResumeSounds()
{
    Audio::Unmute();
}

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
void LocalEvent::HandleMouseWheelEvent( const SDL_MouseWheelEvent & wheel )
{
    SetModes( MOUSE_WHEEL );
    mouse_rm = mouse_cu;
    mouse_wm.x = wheel.x;
    mouse_wm.y = wheel.y;
}

void LocalEvent::HandleTouchEvent( const SDL_TouchFingerEvent & event )
{
    switch ( event.type ) {
    case SDL_FINGERDOWN:
        if ( !_fingerIds.first ) {
            _fingerIds.first = event.fingerId;
        }
        else if ( !_fingerIds.second ) {
            _fingerIds.second = event.fingerId;
        }
        else {
            // Gestures of more than two fingers are not supported, ignore
            return;
        }

        break;
    case SDL_FINGERUP:
    case SDL_FINGERMOTION:
        if ( !std::apply( [&event]( const auto... fingerId ) { return ( ( event.fingerId == fingerId ) || ... ); }, _fingerIds ) ) {
            // An event from an unknown finger, ignore
            return;
        }

        break;
    default:
        // Unknown event, this should never happen
        assert( 0 );
        return;
    }

    // Single-finger gesture, simulate the mouse movement and the left mouse button operation
    if ( event.fingerId == _fingerIds.first && !_fingerIds.second ) {
        const fheroes2::Display & display = fheroes2::Display::instance();

#if defined( TARGET_PS_VITA ) || defined( TARGET_NINTENDO_SWITCH )
        // TODO: verify where it is even needed to do such weird woodoo magic for these targets.
        const fheroes2::Size screenResolution = fheroes2::engine().getCurrentScreenResolution(); // current resolution of screen
        const fheroes2::Rect windowRect = fheroes2::engine().getActiveWindowROI(); // scaled (logical) resolution

        _emulatedPointerPosX = static_cast<double>( screenResolution.width * event.x - windowRect.x ) * ( static_cast<double>( display.width() ) / windowRect.width );
        _emulatedPointerPosY = static_cast<double>( screenResolution.height * event.y - windowRect.y ) * ( static_cast<double>( display.height() ) / windowRect.height );
#else
        _emulatedPointerPosX = static_cast<double>( event.x ) * display.width();
        _emulatedPointerPosY = static_cast<double>( event.y ) * display.height();
#endif

        mouse_cu.x = static_cast<int32_t>( _emulatedPointerPosX );
        mouse_cu.y = static_cast<int32_t>( _emulatedPointerPosY );

        SetModes( MOUSE_MOTION );

        if ( _globalMouseMotionEventHook ) {
            _mouseCursorRenderArea = _globalMouseMotionEventHook( mouse_cu.x, mouse_cu.y );
        }

        if ( event.type == SDL_FINGERDOWN ) {
            mouse_pl = mouse_cu;

            SetModes( MOUSE_PRESSED );
        }
        else if ( event.type == SDL_FINGERUP ) {
            mouse_rl = mouse_cu;

            ResetModes( MOUSE_PRESSED );
            SetModes( MOUSE_RELEASED );
            SetModes( MOUSE_CLICKED );
        }

        mouse_button = SDL_BUTTON_LEFT;
    }
    // Two-finger gesture, simulate the right mouse button operation in the coordinates of the last single-finger gesture.
    // This gesture will continue to be processed as long as the second finger touches the screen, even if the first finger
    // is no longer doing so.
    else if ( event.fingerId == _fingerIds.second ) {
        if ( event.type == SDL_FINGERDOWN ) {
            mouse_pr = mouse_cu;

            SetModes( MOUSE_PRESSED );
        }
        else if ( event.type == SDL_FINGERUP ) {
            mouse_rr = mouse_cu;

            ResetModes( MOUSE_PRESSED );
            SetModes( MOUSE_RELEASED );
            SetModes( MOUSE_CLICKED );
        }

        mouse_button = SDL_BUTTON_RIGHT;
    }

    // The finger no longer touches the screen, reset its state
    if ( event.type == SDL_FINGERUP ) {
        if ( event.fingerId == _fingerIds.first ) {
            _fingerIds.first.reset();
        }
        else if ( event.fingerId == _fingerIds.second ) {
            _fingerIds.second.reset();
        }
        else {
            // An event from an unknown finger, this should never happen
            assert( 0 );
        }
    }
}

void LocalEvent::HandleControllerAxisEvent( const SDL_ControllerAxisEvent & motion )
{
    if ( motion.axis == SDL_CONTROLLER_AXIS_LEFTX ) {
        if ( std::abs( motion.value ) > CONTROLLER_L_DEADZONE )
            _controllerLeftXAxis = motion.value;
        else
            _controllerLeftXAxis = 0;
    }
    else if ( motion.axis == SDL_CONTROLLER_AXIS_LEFTY ) {
        if ( std::abs( motion.value ) > CONTROLLER_L_DEADZONE )
            _controllerLeftYAxis = motion.value;
        else
            _controllerLeftYAxis = 0;
    }
    else if ( motion.axis == SDL_CONTROLLER_AXIS_RIGHTX ) {
        if ( std::abs( motion.value ) > CONTROLLER_R_DEADZONE )
            _controllerRightXAxis = motion.value;
        else
            _controllerRightXAxis = 0;
    }
    else if ( motion.axis == SDL_CONTROLLER_AXIS_RIGHTY ) {
        if ( std::abs( motion.value ) > CONTROLLER_R_DEADZONE )
            _controllerRightYAxis = motion.value;
        else
            _controllerRightYAxis = 0;
    }
}

void LocalEvent::HandleControllerButtonEvent( const SDL_ControllerButtonEvent & button )
{
    if ( button.state == SDL_PRESSED )
        SetModes( KEY_PRESSED );
    else if ( button.state == SDL_RELEASED )
        ResetModes( KEY_PRESSED );

    if ( button.button == SDL_CONTROLLER_BUTTON_A || button.button == SDL_CONTROLLER_BUTTON_B ) {
        if ( modes & KEY_PRESSED ) {
            SetModes( MOUSE_PRESSED );
        }
        else {
            ResetModes( MOUSE_PRESSED );
            SetModes( MOUSE_RELEASED );
            SetModes( MOUSE_CLICKED );
        }

        if ( button.button == SDL_CONTROLLER_BUTTON_A ) {
            if ( modes & KEY_PRESSED ) {
                mouse_pl = mouse_cu;
            }
            else {
                mouse_rl = mouse_cu;
            }

            mouse_button = SDL_BUTTON_LEFT;
        }
        else if ( button.button == SDL_CONTROLLER_BUTTON_B ) {
            if ( modes & KEY_PRESSED ) {
                mouse_pr = mouse_cu;
            }
            else {
                mouse_rr = mouse_cu;
            }

            mouse_button = SDL_BUTTON_RIGHT;
        }

        ResetModes( KEY_PRESSED );
    }
    else if ( modes & KEY_PRESSED ) {
#if defined( TARGET_PS_VITA )
        if ( dpadInputActive ) {
            if ( button.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER || button.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER ) {
                key_value = fheroes2::Key::KEY_SHIFT;
            }
            else if ( button.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT ) {
                key_value = fheroes2::Key::KEY_KP_4;
            }
            else if ( button.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT ) {
                key_value = fheroes2::Key::KEY_KP_6;
            }
            else if ( button.button == SDL_CONTROLLER_BUTTON_DPAD_UP ) {
                key_value = fheroes2::Key::KEY_KP_8;
            }
            else if ( button.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN ) {
                key_value = fheroes2::Key::KEY_KP_2;
            }
            return;
        }
#endif
        if ( button.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN ) {
            key_value = fheroes2::Key::KEY_SPACE;
        }
        else if ( button.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER ) {
            key_value = fheroes2::Key::KEY_H;
        }
        else if ( button.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER ) {
            key_value = fheroes2::Key::KEY_T;
        }
        else if ( button.button == SDL_CONTROLLER_BUTTON_X ) {
            key_value = fheroes2::Key::KEY_E;
        }
        else if ( button.button == SDL_CONTROLLER_BUTTON_Y ) {
            key_value = fheroes2::Key::KEY_C;
        }
        else if ( button.button == SDL_CONTROLLER_BUTTON_BACK ) {
            key_value = fheroes2::Key::KEY_F;
        }
        else if ( button.button == SDL_CONTROLLER_BUTTON_START ) {
            key_value = fheroes2::Key::KEY_ENTER;
        }
#if defined( TARGET_NINTENDO_SWITCH )
        // Custom button mapping for Nintendo Switch
        if ( button.button == SWITCH_BUTTON_Y ) {
            key_value = fheroes2::Key::KEY_ENTER;
        }
        else if ( button.button == SWITCH_BUTTON_X ) {
            key_value = fheroes2::Key::KEY_ESCAPE;
        }
        else if ( button.button == SWITCH_BUTTON_R ) {
            key_value = fheroes2::Key::KEY_T;
        }
        else if ( button.button == SWITCH_BUTTON_L ) {
            key_value = fheroes2::Key::KEY_H;
        }
        else if ( button.button == SWITCH_BUTTON_MINUS ) {
            key_value = fheroes2::Key::KEY_E;
        }
        else if ( button.button == SWITCH_BUTTON_PLUS ) {
            key_value = fheroes2::Key::KEY_C;
        }
#endif
    }
}

void LocalEvent::ProcessControllerAxisMotion()
{
    const double deltaTime = _controllerTimer.getS() * 1000.0;
    _controllerTimer.reset();

    if ( _controllerLeftXAxis != 0 || _controllerLeftYAxis != 0 ) {
        SetModes( MOUSE_MOTION );

        const int32_t xSign = ( _controllerLeftXAxis > 0 ) - ( _controllerLeftXAxis < 0 );
        const int32_t ySign = ( _controllerLeftYAxis > 0 ) - ( _controllerLeftYAxis < 0 );

        _emulatedPointerPosX += pow( std::abs( _controllerLeftXAxis ), CONTROLLER_AXIS_SPEEDUP ) * xSign * deltaTime * _controllerPointerSpeed;
        _emulatedPointerPosY += pow( std::abs( _controllerLeftYAxis ), CONTROLLER_AXIS_SPEEDUP ) * ySign * deltaTime * _controllerPointerSpeed;

        const fheroes2::Display & display = fheroes2::Display::instance();

        if ( _emulatedPointerPosX < 0 )
            _emulatedPointerPosX = 0;
        else if ( _emulatedPointerPosX >= display.width() )
            _emulatedPointerPosX = display.width() - 1;

        if ( _emulatedPointerPosY < 0 )
            _emulatedPointerPosY = 0;
        else if ( _emulatedPointerPosY >= display.height() )
            _emulatedPointerPosY = display.height() - 1;

        mouse_cu.x = static_cast<int32_t>( _emulatedPointerPosX );
        mouse_cu.y = static_cast<int32_t>( _emulatedPointerPosY );

        if ( _globalMouseMotionEventHook ) {
            _mouseCursorRenderArea = _globalMouseMotionEventHook( mouse_cu.x, mouse_cu.y );
        }
    }

    // map scroll with right stick
    if ( _controllerRightXAxis != 0 || _controllerRightYAxis != 0 ) {
        _controllerScrollActive = true;
        SetModes( KEY_PRESSED );

        if ( _controllerRightXAxis < 0 )
            key_value = fheroes2::Key::KEY_KP_4;
        else if ( _controllerRightXAxis > 0 )
            key_value = fheroes2::Key::KEY_KP_6;
        else if ( _controllerRightYAxis < 0 )
            key_value = fheroes2::Key::KEY_KP_8;
        else if ( _controllerRightYAxis > 0 )
            key_value = fheroes2::Key::KEY_KP_2;
    }
    else if ( _controllerScrollActive ) {
        ResetModes( KEY_PRESSED );
        _controllerScrollActive = false;
    }
}

bool LocalEvent::HandleWindowEvent( const SDL_WindowEvent & event )
{
    if ( event.event == SDL_WINDOWEVENT_FOCUS_LOST ) {
        StopSounds();
        return false;
    }

    if ( event.event == SDL_WINDOWEVENT_FOCUS_GAINED ) {
        ResumeSounds();
        return true;
    }

    return ( event.event == SDL_WINDOWEVENT_RESIZED );
}

void LocalEvent::HandleRenderDeviceResetEvent()
{
    // All textures has to be recreated. The only way to do it is to reset everything and render it back.
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Image temp( display.width(), display.height() );
    if ( display.singleLayer() ) {
        temp._disableTransformLayer();
    }

    fheroes2::Copy( display, temp );
    display.release();
    display.resize( temp.width(), temp.height() );
    fheroes2::Copy( temp, display );
}
#else
bool LocalEvent::HandleActiveEvent( const SDL_ActiveEvent & event )
{
    if ( event.state & SDL_APPINPUTFOCUS ) {
        if ( 0 == event.gain ) {
            StopSounds();
        }
        else {
            ResumeSounds();

            return true;
        }
    }

    return false;
}
#endif

bool LocalEvent::MousePressLeft() const
{
    return ( modes & MOUSE_PRESSED ) && SDL_BUTTON_LEFT == mouse_button;
}

bool LocalEvent::MouseReleaseLeft() const
{
    return ( modes & MOUSE_RELEASED ) && SDL_BUTTON_LEFT == mouse_button;
}

bool LocalEvent::MousePressRight() const
{
    return ( modes & MOUSE_PRESSED ) && SDL_BUTTON_RIGHT == mouse_button;
}

void LocalEvent::HandleKeyboardEvent( const SDL_KeyboardEvent & event )
{
    const fheroes2::Key key = getKeyFromSDL( event.keysym.sym );
    if ( key == fheroes2::Key::NONE ) {
        return;
    }

    if ( event.type == SDL_KEYDOWN ) {
        SetModes( KEY_PRESSED );
        SetModes( KEY_HOLD );

        if ( _globalKeyDownEventHook ) {
            _globalKeyDownEventHook( key, getKeyModifierFromSDL( event.keysym.mod ) );
        }
    }
    else if ( event.type == SDL_KEYUP ) {
        ResetModes( KEY_PRESSED );
        ResetModes( KEY_HOLD );
    }

    key_value = key;
}

void LocalEvent::HandleMouseMotionEvent( const SDL_MouseMotionEvent & motion )
{
    SetModes( MOUSE_MOTION );
    mouse_cu.x = motion.x;
    mouse_cu.y = motion.y;
    _emulatedPointerPosX = mouse_cu.x;
    _emulatedPointerPosY = mouse_cu.y;

    if ( _globalMouseMotionEventHook ) {
        _mouseCursorRenderArea = _globalMouseMotionEventHook( motion.x, motion.y );
    }
}

void LocalEvent::HandleMouseButtonEvent( const SDL_MouseButtonEvent & button )
{
    if ( button.state == SDL_PRESSED ) {
        SetModes( MOUSE_PRESSED );
    }
    else {
        ResetModes( MOUSE_PRESSED );
        SetModes( MOUSE_RELEASED );
        SetModes( MOUSE_CLICKED );
    }

    mouse_button = button.button;

    mouse_cu.x = button.x;
    mouse_cu.y = button.y;
    _emulatedPointerPosX = mouse_cu.x;
    _emulatedPointerPosY = mouse_cu.y;

    if ( modes & MOUSE_PRESSED )
        switch ( button.button ) {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#else
        case SDL_BUTTON_WHEELDOWN:
        case SDL_BUTTON_WHEELUP:
            mouse_pm = mouse_cu;
            break;
#endif
        case SDL_BUTTON_LEFT:
            mouse_pl = mouse_cu;
            break;

        case SDL_BUTTON_MIDDLE:
            mouse_pm = mouse_cu;
            break;

        case SDL_BUTTON_RIGHT:
            mouse_pr = mouse_cu;
            break;

        default:
            break;
        }
    else // mouse button released
        switch ( button.button ) {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#else
        case SDL_BUTTON_WHEELDOWN:
        case SDL_BUTTON_WHEELUP:
            mouse_rm = mouse_cu;
            break;
#endif

        case SDL_BUTTON_LEFT:
            mouse_rl = mouse_cu;
            break;

        case SDL_BUTTON_MIDDLE:
            mouse_rm = mouse_cu;
            break;

        case SDL_BUTTON_RIGHT:
            mouse_rr = mouse_cu;
            break;

        default:
            break;
        }
}

bool LocalEvent::MouseClickLeft()
{
    if ( ( modes & MOUSE_CLICKED ) && SDL_BUTTON_LEFT == mouse_button ) {
        ResetModes( MOUSE_RELEASED );
        ResetModes( MOUSE_CLICKED );

        return true;
    }

    return false;
}

bool LocalEvent::MouseClickLeft( const fheroes2::Rect & rt )
{
    if ( ( modes & MOUSE_CLICKED ) && SDL_BUTTON_LEFT == mouse_button && ( rt & mouse_pl ) && ( rt & mouse_rl ) ) {
        ResetModes( MOUSE_RELEASED );
        ResetModes( MOUSE_CLICKED );

        return true;
    }

    return false;
}

bool LocalEvent::MouseClickMiddle()
{
    if ( ( modes & MOUSE_CLICKED ) && SDL_BUTTON_MIDDLE == mouse_button ) {
        ResetModes( MOUSE_RELEASED );
        ResetModes( MOUSE_CLICKED );

        return true;
    }

    return false;
}

bool LocalEvent::MouseClickRight()
{
    if ( ( modes & MOUSE_CLICKED ) && SDL_BUTTON_RIGHT == mouse_button ) {
        ResetModes( MOUSE_RELEASED );
        ResetModes( MOUSE_CLICKED );

        return true;
    }

    return false;
}

bool LocalEvent::MouseClickRight( const fheroes2::Rect & rt )
{
    if ( ( modes & MOUSE_CLICKED ) && SDL_BUTTON_RIGHT == mouse_button && ( rt & mouse_pr ) && ( rt & mouse_rr ) ) {
        ResetModes( MOUSE_RELEASED );
        ResetModes( MOUSE_CLICKED );

        return true;
    }

    return false;
}

bool LocalEvent::MouseWheelUp() const
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    return ( modes & MOUSE_WHEEL ) && mouse_wm.y > 0;
#else
    return ( modes & MOUSE_PRESSED ) && SDL_BUTTON_WHEELUP == mouse_button;
#endif
}

bool LocalEvent::MouseWheelDn() const
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    return ( modes & MOUSE_WHEEL ) && mouse_wm.y < 0;
#else
    return ( modes & MOUSE_PRESSED ) && SDL_BUTTON_WHEELDOWN == mouse_button;
#endif
}

int32_t LocalEvent::getCurrentKeyModifiers()
{
    return getKeyModifierFromSDL( SDL_GetModState() );
}

void LocalEvent::setEventProcessingStates()
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
// The list below is based on event types which require >= SDL 2.0.5. Is there a reason why you want to compile with an older SDL version?
#if !SDL_VERSION_ATLEAST( 2, 0, 5 )
#error Minimal suppported SDL version is 2.0.5.
#endif

    // Full list of events and their requirements can be found at https://wiki.libsdl.org/SDL_EventType
    setEventProcessingState( SDL_QUIT, true );
    // TODO: we don't process this event. Add the logic.
    setEventProcessingState( SDL_APP_TERMINATING, false );
    // TODO: we don't process this event. Add the logic.
    setEventProcessingState( SDL_APP_LOWMEMORY, false );
    // TODO: we don't process this event. Add the logic.
    setEventProcessingState( SDL_APP_WILLENTERBACKGROUND, false );
    // TODO: we don't process this event. Add the logic.
    setEventProcessingState( SDL_APP_DIDENTERBACKGROUND, false );
    // TODO: we don't process this event. Add the logic.
    setEventProcessingState( SDL_APP_WILLENTERFOREGROUND, false );
    // TODO: we don't process this event. Add the logic.
    setEventProcessingState( SDL_APP_DIDENTERFOREGROUND, false );
    // SDL_LOCALECHANGED is supported from SDL 2.0.14
    // TODO: we don't process this event. Add the logic.
    setEventProcessingState( SDL_DISPLAYEVENT, false );
    setEventProcessingState( SDL_WINDOWEVENT, true );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_SYSWMEVENT, false );
    setEventProcessingState( SDL_KEYDOWN, true );
    setEventProcessingState( SDL_KEYUP, true );
    // SDL_TEXTINPUT and SDL_TEXTEDITING are enabled and disabled by SDL_StartTextInput() and SDL_StopTextInput() functions.
    // Do not enable them here.
    setEventProcessingState( SDL_TEXTEDITING, false );
    setEventProcessingState( SDL_TEXTINPUT, false );
    setEventProcessingState( SDL_KEYMAPCHANGED, false ); // supported from SDL 2.0.4
    // SDL_TEXTEDITING_EXT is supported only from SDL 2.0.22
    setEventProcessingState( SDL_MOUSEMOTION, true );
    setEventProcessingState( SDL_MOUSEBUTTONDOWN, true );
    setEventProcessingState( SDL_MOUSEBUTTONUP, true );
    setEventProcessingState( SDL_MOUSEWHEEL, true );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_JOYAXISMOTION, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_JOYBALLMOTION, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_JOYHATMOTION, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_JOYBUTTONDOWN, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_JOYBUTTONUP, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_JOYDEVICEADDED, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_JOYDEVICEREMOVED, false );
    setEventProcessingState( SDL_CONTROLLERAXISMOTION, true );
    setEventProcessingState( SDL_CONTROLLERBUTTONDOWN, true );
    setEventProcessingState( SDL_CONTROLLERBUTTONUP, true );
    setEventProcessingState( SDL_CONTROLLERDEVICEADDED, true );
    setEventProcessingState( SDL_CONTROLLERDEVICEREMOVED, true );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_CONTROLLERDEVICEREMAPPED, false );
    // SDL_CONTROLLERTOUCHPADDOWN is supported from SDL 2.0.14
    // SDL_CONTROLLERTOUCHPADMOTION is supported from SDL 2.0.14
    // SDL_CONTROLLERTOUCHPADUP is supported from SDL 2.0.14
    // SDL_CONTROLLERSENSORUPDATE is supported from SDL 2.0.14
    setEventProcessingState( SDL_FINGERDOWN, true );
    setEventProcessingState( SDL_FINGERUP, true );
    setEventProcessingState( SDL_FINGERMOTION, true );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_DOLLARGESTURE, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_DOLLARRECORD, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_MULTIGESTURE, false );
    // We do not support clipboard within the engine.
    setEventProcessingState( SDL_CLIPBOARDUPDATE, false );
    // We do not support drag and drop capability for the application.
    setEventProcessingState( SDL_DROPFILE, false );
    setEventProcessingState( SDL_DROPTEXT, false );
    setEventProcessingState( SDL_DROPBEGIN, false ); // supported from SDL 2.0.5
    setEventProcessingState( SDL_DROPCOMPLETE, false ); // supported from SDL 2.0.5
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_AUDIODEVICEADDED, false ); // supported from SDL 2.0.4
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_AUDIODEVICEREMOVED, false ); // supported from SDL 2.0.4
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_SENSORUPDATE, false );
    setEventProcessingState( SDL_RENDER_TARGETS_RESET, true ); // supported from SDL 2.0.2
    setEventProcessingState( SDL_RENDER_DEVICE_RESET, true ); // supported from SDL 2.0.4
    // SDL_POLLSENTINEL is supported from SDL 2.0.?
    // We do not support custom user events as of now.
    setEventProcessingState( SDL_USEREVENT, false );
#else
    setEventProcessingState( SDL_ACTIVEEVENT, true );
    setEventProcessingState( SDL_KEYDOWN, true );
    setEventProcessingState( SDL_KEYUP, true );
    setEventProcessingState( SDL_MOUSEMOTION, true );
    setEventProcessingState( SDL_MOUSEBUTTONDOWN, true );
    setEventProcessingState( SDL_MOUSEBUTTONUP, true );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_JOYAXISMOTION, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_JOYBALLMOTION, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_JOYHATMOTION, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_JOYBUTTONDOWN, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_JOYBUTTONUP, false );
    setEventProcessingState( SDL_QUIT, true );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_SYSWMEVENT, false );
    // SDL_EVENT_RESERVEDA is not in use.
    // SDL_EVENT_RESERVEDB is not in use.
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_VIDEORESIZE, false );
    // TODO: verify why disabled processing of this event.
    setEventProcessingState( SDL_VIDEOEXPOSE, false );
    // SDL_EVENT_RESERVED2 - SDL_EVENT_RESERVED7 are not in use.
    // We do not support custom user events as of now.
    setEventProcessingState( SDL_USEREVENT, false );
#endif
}
