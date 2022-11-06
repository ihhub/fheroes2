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

#include "localevent.h"
#include "audio.h"
#include "pal.h"
#include "screen.h"

#include <cassert>
#include <map>

namespace
{
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

    enum KeyMod
    {
        MOD_NONE = KMOD_NONE,
        MOD_CTRL = KMOD_CTRL,
        MOD_SHIFT = KMOD_SHIFT,
        MOD_ALT = KMOD_ALT,
        MOD_CAPS = KMOD_CAPS,
        MOD_NUM = KMOD_NUM
    };

    char getCharacterFromPressedKey( const fheroes2::Key key, const int32_t mod )
    {
        switch ( key ) {
        case fheroes2::Key::KEY_1:
            return ( MOD_SHIFT & mod ? '!' : '1' );
        case fheroes2::Key::KEY_2:
            return ( MOD_SHIFT & mod ? '@' : '2' );
        case fheroes2::Key::KEY_3:
            return ( MOD_SHIFT & mod ? '#' : '3' );
        case fheroes2::Key::KEY_4:
            return ( MOD_SHIFT & mod ? '$' : '4' );
        case fheroes2::Key::KEY_5:
            return ( MOD_SHIFT & mod ? '%' : '5' );
        case fheroes2::Key::KEY_6:
            return ( MOD_SHIFT & mod ? '^' : '6' );
        case fheroes2::Key::KEY_7:
            return ( MOD_SHIFT & mod ? '&' : '7' );
        case fheroes2::Key::KEY_8:
            return ( MOD_SHIFT & mod ? '*' : '8' );
        case fheroes2::Key::KEY_9:
            return ( MOD_SHIFT & mod ? '(' : '9' );
        case fheroes2::Key::KEY_0:
            return ( MOD_SHIFT & mod ? ')' : '0' );
        case fheroes2::Key::KEY_KP_0:
            if ( MOD_NUM & mod )
                return '0';
            break;
        case fheroes2::Key::KEY_KP_1:
            if ( MOD_NUM & mod )
                return '1';
            break;
        case fheroes2::Key::KEY_KP_2:
            if ( MOD_NUM & mod )
                return '2';
            break;
        case fheroes2::Key::KEY_KP_3:
            if ( MOD_NUM & mod )
                return '3';
            break;
        case fheroes2::Key::KEY_KP_4:
            if ( MOD_NUM & mod )
                return '4';
            break;
        case fheroes2::Key::KEY_KP_5:
            if ( MOD_NUM & mod )
                return '5';
            break;
        case fheroes2::Key::KEY_KP_6:
            if ( MOD_NUM & mod )
                return '6';
            break;
        case fheroes2::Key::KEY_KP_7:
            if ( MOD_NUM & mod )
                return '7';
            break;
        case fheroes2::Key::KEY_KP_8:
            if ( MOD_NUM & mod )
                return '8';
            break;
        case fheroes2::Key::KEY_KP_9:
            if ( MOD_NUM & mod )
                return '9';
            break;
        case fheroes2::Key::KEY_MINUS:
            return ( MOD_SHIFT & mod ? '_' : '-' );
        case fheroes2::Key::KEY_EQUALS:
            return ( MOD_SHIFT & mod ? '+' : '=' );
        case fheroes2::Key::KEY_BACKSLASH:
            return ( MOD_SHIFT & mod ? '|' : '\\' );
        case fheroes2::Key::KEY_LEFT_BRACKET:
            return ( MOD_SHIFT & mod ? '{' : '[' );
        case fheroes2::Key::KEY_RIGHT_BRACKET:
            return ( MOD_SHIFT & mod ? '}' : ']' );
        case fheroes2::Key::KEY_SEMICOLON:
            return ( MOD_SHIFT & mod ? ':' : ';' );
        case fheroes2::Key::KEY_QUOTE:
            return ( MOD_SHIFT & mod ? '"' : '\'' );
        case fheroes2::Key::KEY_COMMA:
            return ( MOD_SHIFT & mod ? '<' : ',' );
        case fheroes2::Key::KEY_PERIOD:
            return ( MOD_SHIFT & mod ? '>' : '.' );
        case fheroes2::Key::KEY_SLASH:
            return ( MOD_SHIFT & mod ? '?' : '/' );
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
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'A' : 'a' );
        case fheroes2::Key::KEY_B:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'B' : 'b' );
        case fheroes2::Key::KEY_C:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'C' : 'c' );
        case fheroes2::Key::KEY_D:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'D' : 'd' );
        case fheroes2::Key::KEY_E:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'E' : 'e' );
        case fheroes2::Key::KEY_F:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'F' : 'f' );
        case fheroes2::Key::KEY_G:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'G' : 'g' );
        case fheroes2::Key::KEY_H:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'H' : 'h' );
        case fheroes2::Key::KEY_I:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'I' : 'i' );
        case fheroes2::Key::KEY_J:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'J' : 'j' );
        case fheroes2::Key::KEY_K:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'K' : 'k' );
        case fheroes2::Key::KEY_L:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'L' : 'l' );
        case fheroes2::Key::KEY_M:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'M' : 'm' );
        case fheroes2::Key::KEY_N:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'N' : 'n' );
        case fheroes2::Key::KEY_O:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'O' : 'o' );
        case fheroes2::Key::KEY_P:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'P' : 'p' );
        case fheroes2::Key::KEY_Q:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'Q' : 'q' );
        case fheroes2::Key::KEY_R:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'R' : 'r' );
        case fheroes2::Key::KEY_S:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'S' : 's' );
        case fheroes2::Key::KEY_T:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'T' : 't' );
        case fheroes2::Key::KEY_U:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'U' : 'u' );
        case fheroes2::Key::KEY_V:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'V' : 'v' );
        case fheroes2::Key::KEY_W:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'W' : 'w' );
        case fheroes2::Key::KEY_X:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'X' : 'x' );
        case fheroes2::Key::KEY_Y:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'Y' : 'y' );
        case fheroes2::Key::KEY_Z:
            return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'Z' : 'z' );
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
        return getCharacterFromPressedKey( dPadKeys[currentCharIndex], currentUpper ? MOD_CAPS : MOD_NONE );
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

    Key getKeyFromSDL( int sdlKey )
    {
        // SDL interprets keyboard Numpad Enter as a separate key. However, in the game we should handle it in the same way as the normal Enter.
        if ( sdlKey == SDLK_KP_ENTER ) {
            sdlKey = SDLK_RETURN;
        }

        static std::map<int, Key> sdlValueToKey;
        if ( sdlValueToKey.empty() ) {
            // The map is empty let's populate it.
            for ( int32_t i = static_cast<int32_t>( Key::NONE ); i < static_cast<int32_t>( Key::LAST_KEY ); ++i ) {
                const Key key = static_cast<Key>( i );
                sdlValueToKey.emplace( getSDLKey( key ), key );
            }
        }

        auto iter = sdlValueToKey.find( sdlKey );
        if ( iter == sdlValueToKey.end() ) {
            return Key::NONE;
        }

        return iter->second;
    }
}

LocalEvent::LocalEvent()
    : modes( 0 )
    , key_value( fheroes2::Key::NONE )
    , mouse_button( 0 )
    , redraw_cursor_func( nullptr )
    , keyboard_filter_func( nullptr )
    , loop_delay( 1 )
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
#if defined( TARGET_PS_VITA ) || defined( TARGET_NINTENDO_SWITCH )
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
#endif
}

void LocalEvent::CloseVirtualKeyboard()
{
#if defined( TARGET_PS_VITA )
    dpadInputActive = false;
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
    le.ResetModes( KEY_HOLD );
    return le;
}

bool LocalEvent::HandleEvents( bool delay, bool allowExit )
{
    if ( colorCycling.isRedrawRequired() ) {
        // Looks like there is no explicit rendering so the code for color cycling was executed here.
        if ( delay ) {
            fheroes2::Time timeCheck;
            fheroes2::Display::instance().render();

            if ( timeCheck.getMs() > loop_delay ) {
                // Since rendering took more than waiting time so we should not wait.
                delay = false;
            }
        }
        else {
            fheroes2::Display::instance().render();
        }
    }

    SDL_Event event;

    ResetModes( MOUSE_MOTION );
    ResetModes( MOUSE_RELEASED );
    ResetModes( MOUSE_CLICKED );
    ResetModes( KEY_PRESSED );

    mouse_wm = fheroes2::Point();

    while ( SDL_PollEvent( &event ) ) {
        switch ( event.type ) {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        case SDL_WINDOWEVENT:
            OnSdl2WindowEvent( event );
            break;
#else
        case SDL_ACTIVEEVENT:
            OnActiveEvent( event );
            break;
#endif
        // keyboard
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            HandleKeyboardEvent( event.key );
            break;

        // mouse motion
        case SDL_MOUSEMOTION:
            HandleMouseMotionEvent( event.motion );
            break;

        // mouse button
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            HandleMouseButtonEvent( event.button );
            break;

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
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
            HandleTouchEvent( event.tfinger );
            break;
#endif

        // exit
        case SDL_QUIT:
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        case SDL_WINDOWEVENT_CLOSE:
#endif
            if ( allowExit )
                return false; // try to perform clear exit to catch all memory leaks, for example
            break;

        default:
            break;
        }

        // need for wheel up/down delay
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        // Use HandleMouseWheel instead
#else
        if ( SDL_BUTTON_WHEELDOWN == event.button.button || SDL_BUTTON_WHEELUP == event.button.button )
            break;
#endif
    }

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    if ( _gameController != nullptr ) {
        ProcessControllerAxisMotion();
    }
#endif

    if ( delay )
        SDL_Delay( loop_delay );

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
void LocalEvent::OnSdl2WindowEvent( const SDL_Event & event )
{
    if ( event.window.event == SDL_WINDOWEVENT_FOCUS_LOST ) {
        StopSounds();
    }
    else if ( event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ) {
        // Force display rendering on app activation
        fheroes2::Display::instance().render();

        ResumeSounds();
    }
    else if ( event.window.event == SDL_WINDOWEVENT_RESIZED ) {
        fheroes2::engine().updateScreenParameters();
        fheroes2::Display::instance().render();
    }
}
#else
void LocalEvent::OnActiveEvent( const SDL_Event & event )
{
    if ( event.active.state & SDL_APPINPUTFOCUS ) {
        if ( 0 == event.active.gain ) {
            StopSounds();
        }
        else {
            // Force display rendering on app activation
            fheroes2::Display::instance().render();

            ResumeSounds();
        }
    }
}
#endif

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
void LocalEvent::HandleTouchEvent( const SDL_TouchFingerEvent & event )
{
    if ( event.touchId != 0 )
        return;

    if ( event.type == SDL_FINGERDOWN ) {
        ++_numTouches;
        if ( _numTouches == 1 ) {
            _firstFingerId = event.fingerId;
        }
    }
    else if ( event.type == SDL_FINGERUP ) {
        --_numTouches;
    }

    if ( _firstFingerId == event.fingerId ) {
        const fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Size screenResolution = fheroes2::engine().getCurrentScreenResolution(); // current resolution of screen
        const fheroes2::Size gameSurfaceRes( display.width(), display.height() ); // native game (surface) resolution
        const fheroes2::Rect windowRect = fheroes2::engine().getActiveWindowROI(); // scaled (logical) resolution

        SetModes( MOUSE_MOTION );

        _emulatedPointerPosX
            = static_cast<double>( screenResolution.width * event.x - windowRect.x ) * ( static_cast<double>( gameSurfaceRes.width ) / windowRect.width );
        _emulatedPointerPosY
            = static_cast<double>( screenResolution.height * event.y - windowRect.y ) * ( static_cast<double>( gameSurfaceRes.height ) / windowRect.height );

        mouse_cu.x = static_cast<int32_t>( _emulatedPointerPosX );
        mouse_cu.y = static_cast<int32_t>( _emulatedPointerPosY );

        if ( ( modes & MOUSE_MOTION ) && redraw_cursor_func ) {
            ( *redraw_cursor_func )( mouse_cu.x, mouse_cu.y );
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
    const double deltaTime = _controllerTimer.get() * 1000.0;
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

        if ( ( modes & MOUSE_MOTION ) && redraw_cursor_func ) {
            ( *redraw_cursor_func )( mouse_cu.x, mouse_cu.y );
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
    const fheroes2::Key key = fheroes2::getKeyFromSDL( event.keysym.sym );
    if ( key == fheroes2::Key::NONE ) {
        return;
    }

    if ( event.type == SDL_KEYDOWN ) {
        SetModes( KEY_PRESSED );
        SetModes( KEY_HOLD );
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

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    if ( fheroes2::cursor().isSoftwareEmulation() ) {
        updateEmulatedMousePosition( motion.x, motion.y );
    }
    else {
        mouse_cu.x = motion.x;
        mouse_cu.y = motion.y;
    }
#else
    // SDL1 does not support window scaling.
    mouse_cu.x = motion.x;
    mouse_cu.y = motion.y;
#endif
    _emulatedPointerPosX = mouse_cu.x;
    _emulatedPointerPosY = mouse_cu.y;
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

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    if ( fheroes2::cursor().isSoftwareEmulation() ) {
        updateEmulatedMousePosition( button.x, button.y );
    }
    else {
        mouse_cu.x = button.x;
        mouse_cu.y = button.y;
    }
#else
    // SDL1 does not support window scaling.
    mouse_cu.x = button.x;
    mouse_cu.y = button.y;
#endif
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

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
void LocalEvent::HandleMouseWheelEvent( const SDL_MouseWheelEvent & wheel )
{
    SetModes( MOUSE_WHEEL );
    mouse_rm = mouse_cu;
    mouse_wm.x = wheel.x;
    mouse_wm.y = wheel.y;
}

void LocalEvent::updateEmulatedMousePosition( const int32_t x, const int32_t y )
{
    const fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Rect renderROI = fheroes2::engine().getRenderROI(); // rendering area
    const fheroes2::Size gameSurfaceRes( display.width(), display.height() ); // native game (surface) resolution
    const fheroes2::Rect windowRect = fheroes2::engine().getActiveWindowROI(); // scaled (logical) resolution

    mouse_cu.x = ( x + renderROI.x ) * windowRect.width / ( gameSurfaceRes.width + renderROI.x * 2 );
    mouse_cu.y = ( y + renderROI.y ) * windowRect.height / ( gameSurfaceRes.height + renderROI.y * 2 );
}
#endif

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

int LocalEvent::KeyMod() const
{
    return SDL_GetModState();
}

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
int LocalEvent::GlobalFilterEvents( void * /*userdata*/, SDL_Event * event )
#else
int LocalEvent::GlobalFilterEvents( const SDL_Event * event )
#endif
{
    const LocalEvent & le = LocalEvent::Get();

    if ( SDL_MOUSEMOTION == event->type ) {
        // Redraw cursor.
        if ( le.redraw_cursor_func ) {
            ( *( le.redraw_cursor_func ) )( event->motion.x, event->motion.y );
        }
    }
    else if ( SDL_KEYDOWN == event->type ) {
        // Process key press event.
        if ( le.keyboard_filter_func ) {
            ( *( le.keyboard_filter_func ) )( event->key.keysym.sym, event->key.keysym.mod );
        }
    }

    return 1;
}

void LocalEvent::SetState( const uint32_t type, const bool enable )
{
    // SDL 1 and SDL 2 have different input argument types for event state.
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    SDL_EventState( type, enable ? SDL_ENABLE : SDL_IGNORE );
#else
    SDL_EventState( static_cast<uint8_t>( type ), enable ? SDL_ENABLE : SDL_IGNORE );
#endif
}

void LocalEvent::SetStateDefaults()
{
    SetState( SDL_USEREVENT, true );
    SetState( SDL_KEYDOWN, true );
    SetState( SDL_KEYUP, true );
    SetState( SDL_MOUSEMOTION, true );
    SetState( SDL_MOUSEBUTTONDOWN, true );
    SetState( SDL_MOUSEBUTTONUP, true );
    SetState( SDL_QUIT, true );

    SetState( SDL_JOYAXISMOTION, true );
    SetState( SDL_JOYBUTTONUP, true );
    SetState( SDL_JOYBUTTONDOWN, true );

    SetState( SDL_JOYBALLMOTION, false );
    SetState( SDL_JOYHATMOTION, false );
    SetState( SDL_SYSWMEVENT, false );

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#if defined( TARGET_PS_VITA ) || defined( TARGET_NINTENDO_SWITCH )
    SetState( SDL_FINGERDOWN, true );
    SetState( SDL_FINGERUP, true );
    SetState( SDL_FINGERMOTION, true );
#else
    SetState( SDL_FINGERDOWN, false );
    SetState( SDL_FINGERUP, false );
    SetState( SDL_FINGERMOTION, false );
#endif
#endif

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    SetState( SDL_WINDOWEVENT, true );

    SDL_SetEventFilter( GlobalFilterEvents, nullptr );
#else
    SetState( SDL_ACTIVEEVENT, true );

    SetState( SDL_SYSWMEVENT, false );
    SetState( SDL_VIDEORESIZE, false );
    SetState( SDL_VIDEOEXPOSE, false );

    SDL_SetEventFilter( GlobalFilterEvents );
#endif
}
