/***************************************************************************
 *   Copyright (C) 2006 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *   Copyright (C) 2008 by Josh Matthews <josh@joshmatthews.net>           *
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
#ifndef H2LOCALEVENT_H
#define H2LOCALEVENT_H

#include "rect.h"
#include "thread.h"
#include "types.h"

enum KeySym
{
    KEY_NONE = -1,

    KEY_UNKNOWN = SDLK_UNKNOWN,

    KEY_BACKSPACE = SDLK_BACKSPACE,
    KEY_RETURN = SDLK_RETURN,
    KEY_ESCAPE = SDLK_ESCAPE,
    KEY_SPACE = SDLK_SPACE,
    KEY_EXCLAIM = SDLK_EXCLAIM,
    KEY_QUOTEDBL = SDLK_QUOTEDBL,
    KEY_HASH = SDLK_HASH,
    KEY_DOLLAR = SDLK_DOLLAR,
    KEY_AMPERSAND = SDLK_AMPERSAND,
    KEY_QUOTE = SDLK_QUOTE,
    KEY_LEFTPAREN = SDLK_LEFTPAREN,
    KEY_RIGHTPAREN = SDLK_RIGHTPAREN,
    KEY_ASTERISK = SDLK_ASTERISK,
    KEY_PLUS = SDLK_PLUS,
    KEY_COMMA = SDLK_COMMA,
    KEY_MINUS = SDLK_MINUS,
    KEY_PERIOD = SDLK_PERIOD,
    KEY_SLASH = SDLK_SLASH,
    KEY_COLON = SDLK_COLON,
    KEY_SEMICOLON = SDLK_SEMICOLON,
    KEY_LESS = SDLK_LESS,
    KEY_EQUALS = SDLK_EQUALS,
    KEY_GREATER = SDLK_GREATER,
    KEY_QUESTION = SDLK_QUESTION,
    KEY_AT = SDLK_AT,
    KEY_LEFTBRACKET = SDLK_LEFTBRACKET,
    KEY_BACKSLASH = SDLK_BACKSLASH,
    KEY_RIGHTBRACKET = SDLK_RIGHTBRACKET,
    KEY_CARET = SDLK_CARET,
    KEY_UNDERSCORE = SDLK_UNDERSCORE,
    KEY_ALT = SDLK_LALT,
    KEY_CONTROL = SDLK_LCTRL,
    KEY_SHIFT = SDLK_LSHIFT,
    KEY_TAB = SDLK_TAB,
    KEY_DELETE = SDLK_DELETE,
    KEY_PAGEUP = SDLK_PAGEUP,
    KEY_PAGEDOWN = SDLK_PAGEDOWN,
    KEY_F1 = SDLK_F1,
    KEY_F2 = SDLK_F2,
    KEY_F3 = SDLK_F3,
    KEY_F4 = SDLK_F4,
    KEY_F5 = SDLK_F5,
    KEY_F6 = SDLK_F6,
    KEY_F7 = SDLK_F7,
    KEY_F8 = SDLK_F8,
    KEY_F9 = SDLK_F9,
    KEY_F10 = SDLK_F10,
    KEY_F11 = SDLK_F11,
    KEY_F12 = SDLK_F12,
    KEY_LEFT = SDLK_LEFT,
    KEY_RIGHT = SDLK_RIGHT,
    KEY_UP = SDLK_UP,
    KEY_DOWN = SDLK_DOWN,
    KEY_0 = SDLK_0,
    KEY_1 = SDLK_1,
    KEY_2 = SDLK_2,
    KEY_3 = SDLK_3,
    KEY_4 = SDLK_4,
    KEY_5 = SDLK_5,
    KEY_6 = SDLK_6,
    KEY_7 = SDLK_7,
    KEY_8 = SDLK_8,
    KEY_9 = SDLK_9,
    KEY_a = SDLK_a,
    KEY_b = SDLK_b,
    KEY_c = SDLK_c,
    KEY_d = SDLK_d,
    KEY_e = SDLK_e,
    KEY_f = SDLK_f,
    KEY_g = SDLK_g,
    KEY_h = SDLK_h,
    KEY_i = SDLK_i,
    KEY_j = SDLK_j,
    KEY_k = SDLK_k,
    KEY_l = SDLK_l,
    KEY_m = SDLK_m,
    KEY_n = SDLK_n,
    KEY_o = SDLK_o,
    KEY_p = SDLK_p,
    KEY_q = SDLK_q,
    KEY_r = SDLK_r,
    KEY_s = SDLK_s,
    KEY_t = SDLK_t,
    KEY_u = SDLK_u,
    KEY_v = SDLK_v,
    KEY_w = SDLK_w,
    KEY_x = SDLK_x,
    KEY_y = SDLK_y,
    KEY_z = SDLK_z,

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    KEY_PRINT = SDLK_PRINTSCREEN,
    KEY_KP0 = SDLK_KP_0,
    KEY_KP1 = SDLK_KP_1,
    KEY_KP2 = SDLK_KP_2,
    KEY_KP3 = SDLK_KP_3,
    KEY_KP4 = SDLK_KP_4,
    KEY_KP5 = SDLK_KP_5,
    KEY_KP6 = SDLK_KP_6,
    KEY_KP7 = SDLK_KP_7,
    KEY_KP8 = SDLK_KP_8,
    KEY_KP9 = SDLK_KP_9,
#else
    KEY_PRINT = SDLK_PRINT,
    KEY_KP0 = SDLK_KP0,
    KEY_KP1 = SDLK_KP1,
    KEY_KP2 = SDLK_KP2,
    KEY_KP3 = SDLK_KP3,
    KEY_KP4 = SDLK_KP4,
    KEY_KP5 = SDLK_KP5,
    KEY_KP6 = SDLK_KP6,
    KEY_KP7 = SDLK_KP7,
    KEY_KP8 = SDLK_KP8,
    KEY_KP9 = SDLK_KP9,
#endif

    KEY_KP_PERIOD = SDLK_KP_PERIOD,
    KEY_KP_DIVIDE = SDLK_KP_DIVIDE,
    KEY_KP_MULTIPLY = SDLK_KP_MULTIPLY,
    KEY_KP_MINUS = SDLK_KP_MINUS,
    KEY_KP_PLUS = SDLK_KP_PLUS,
    KEY_KP_ENTER = SDLK_KP_ENTER,
    KEY_KP_EQUALS = SDLK_KP_EQUALS,

    KEY_LAST
};

const char * KeySymGetName( KeySym );
KeySym GetKeySym( int );

class LocalEvent
{
public:
    static LocalEvent & Get( void );
    static LocalEvent & GetClean(); // reset all previous event statuses and return a reference for events

    void SetGlobalFilterMouseEvents( void ( *pf )( s32, s32 ) );
    void SetGlobalFilterKeysEvents( void ( *pf )( int, int ) );
    void SetGlobalFilter( bool );
    void SetTapMode( bool );
    void SetTapDelayForRightClickEmulation( u32 );
    void SetMouseOffsetX( int16_t );
    void SetMouseOffsetY( int16_t );

    static void SetStateDefaults( void );
    static void SetState( u32 type, bool enable );
    static int GetState( u32 type );

    bool HandleEvents( bool delay = true, bool allowExit = false );

    bool MouseMotion( void ) const;
    bool MouseMotion( const Rect & rt ) const;

    const Point & GetMouseCursor( void )
    {
        return mouse_cu;
    }

    const Point & GetMousePressLeft( void ) const;
    const Point & GetMousePressMiddle( void ) const;
    const Point & GetMousePressRight( void ) const;
    const Point & GetMouseReleaseLeft( void ) const;
    const Point & GetMouseReleaseMiddle( void ) const;
    const Point & GetMouseReleaseRight( void ) const;

    void ResetPressLeft( void );
    void ResetPressRight( void );
    void ResetPressMiddle( void );

    void ResetReleaseLeft( void );
    void ResetReleaseRight( void );
    void ResetReleaseMiddle( void );

    bool MouseClickLeft( void );
    bool MouseClickMiddle( void );
    bool MouseClickRight( void );

    bool MouseClickLeft( const Rect & rt );
    bool MouseClickMiddle( const Rect & rt );
    bool MouseClickRight( const Rect & rt );

    bool MouseWheelUp( void ) const;
    bool MouseWheelDn( void ) const;

    bool MousePressLeft( void ) const;
    bool MousePressLeft( const Rect & rt ) const;
    bool MousePressLeft( const Point & pt, u32 w, u32 h ) const;
    bool MousePressMiddle( void ) const;
    bool MousePressMiddle( const Rect & rt ) const;
    bool MousePressRight( void ) const;
    bool MousePressRight( const Rect & rt ) const;

    bool MouseReleaseLeft( void ) const;
    bool MouseReleaseLeft( const Rect & rt ) const;
    bool MouseReleaseMiddle( void ) const;
    bool MouseReleaseMiddle( const Rect & rt ) const;
    bool MouseReleaseRight( void ) const;
    bool MouseReleaseRight( const Rect & rt ) const;

    bool MouseWheelUp( const Rect & rt ) const;
    bool MouseWheelDn( const Rect & rt ) const;

    bool MouseCursor( const Rect & rt ) const;

    bool KeyPress( void ) const;
    bool KeyPress( KeySym key ) const;

    bool KeyHold() const
    {
        return ( modes & KEY_HOLD ) != 0;
    }

    KeySym KeyValue( void ) const;
    int KeyMod( void ) const;

    void RegisterCycling( void ( *preRenderDrawing )() = nullptr, void ( *postRenderDrawing )() = nullptr ) const;

    // These two methods are useful for video playback
    void PauseCycling();
    void ResumeCycling();

private:
    LocalEvent();

    void HandleMouseMotionEvent( const SDL_MouseMotionEvent & );
    void HandleMouseButtonEvent( const SDL_MouseButtonEvent & );
    void HandleKeyboardEvent( SDL_KeyboardEvent & );

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    void HandleMouseWheelEvent( const SDL_MouseWheelEvent & );
    static int GlobalFilterEvents( void *, SDL_Event * );
#else
    static int GlobalFilterEvents( const SDL_Event * );
#endif

    enum flag_t
    {
        KEY_PRESSED = 0x0001,
        MOUSE_MOTION = 0x0002,
        MOUSE_PRESSED = 0x0004,
        GLOBAL_FILTER = 0x0008,
        CLICK_LEFT = 0x0010, // either there is a click on left button or it was just released
        CLICK_RIGHT = 0x0020, // either there is a click on right button or it was just released
        CLICK_MIDDLE = 0x0040, // either there is a click on middle button or it was just released
        TAP_MODE = 0x0080,
        MOUSE_OFFSET = 0x0100,
        CLOCK_ON = 0x0200,
        MOUSE_WHEEL = 0x0400,
        KEY_HOLD = 0x0800
    };

    void SetModes( flag_t );
    void ResetModes( flag_t );

    int modes;
    KeySym key_value;
    int mouse_state;
    int mouse_button;

    Point mouse_st; // mouse offset for pocketpc

    Point mouse_pl; // press left
    Point mouse_pm; // press middle
    Point mouse_pr; // press right

    Point mouse_rl; // release left
    Point mouse_rm; // release middle
    Point mouse_rr; // release right

    Point mouse_cu; // point cursor

    Point mouse_wm; // wheel movement

    void ( *redraw_cursor_func )( s32, s32 );
    void ( *keyboard_filter_func )( int, int );

    SDL::Time clock;
    u32 clock_delay;
    int loop_delay;

    // These members are used for restoring music and sounds when an user reopens the window
    bool _isHiddenWindow;
    bool _isMusicPaused;
    bool _isSoundPaused;
};

#endif
