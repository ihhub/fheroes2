/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2008 by Josh Matthews <josh@joshmatthews.net>           *
 *   Copyright (C) 2006 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

#include <SDL_events.h>
#include <SDL_version.h>

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#include <SDL_gamecontroller.h>
#include <SDL_touch.h>
#endif

#include "math_base.h"
#include "timing.h"

namespace fheroes2
{
    enum class Key : int32_t
    {
        NONE,
        KEY_BACKSPACE,
        KEY_ENTER, // This is for both Enter keys on keyboards. Check getKeyFromSDL() function for explanation.
        KEY_ESCAPE,
        KEY_SPACE,
        KEY_EXCLAIM,
        KEY_DOUBLE_QUOTE,
        KEY_HASH,
        KEY_DOLLAR,
        KEY_AMPERSAND,
        KEY_QUOTE,
        KEY_LEFT_PARENTHESIS,
        KEY_RIGHT_PARENTHESIS,
        KEY_ASTERISK,
        KEY_PLUS,
        KEY_COMMA,
        KEY_MINUS,
        KEY_PERIOD,
        KEY_SLASH,
        KEY_COLON,
        KEY_SEMICOLON,
        KEY_LESS,
        KEY_EQUALS,
        KEY_GREATER,
        KEY_QUESTION,
        KEY_AT,
        KEY_LEFT_BRACKET,
        KEY_BACKSLASH,
        KEY_RIGHT_BRACKET,
        KEY_CARET,
        KEY_UNDERSCORE,
        KEY_ALT,
        KEY_CONTROL,
        KEY_SHIFT,
        KEY_TAB,
        KEY_DELETE,
        KEY_PAGE_UP,
        KEY_PAGE_DOWN,
        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_UP,
        KEY_DOWN,
        KEY_0,
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,
        KEY_A,
        KEY_B,
        KEY_C,
        KEY_D,
        KEY_E,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_I,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_M,
        KEY_N,
        KEY_O,
        KEY_P,
        KEY_Q,
        KEY_R,
        KEY_S,
        KEY_T,
        KEY_U,
        KEY_V,
        KEY_W,
        KEY_X,
        KEY_Y,
        KEY_Z,
        KEY_PRINT,
        KEY_KP_0,
        KEY_KP_1,
        KEY_KP_2,
        KEY_KP_3,
        KEY_KP_4,
        KEY_KP_5,
        KEY_KP_6,
        KEY_KP_7,
        KEY_KP_8,
        KEY_KP_9,
        KEY_KP_PERIOD,
        KEY_KP_DIVIDE,
        KEY_KP_MULTIPLY,
        KEY_KP_MINUS,
        KEY_KP_PLUS,
        KEY_KP_ENTER,
        KEY_KP_EQUALS,
        KEY_HOME,
        KEY_END,

        // Put all new keys before this line.
        LAST_KEY
    };

    // Key modifier is used for key combination detection.
    enum KeyModifier : int32_t
    {
        KEY_MODIFIER_NONE = 0,
        KEY_MODIFIER_CTRL = 0x1,
        KEY_MODIFIER_SHIFT = 0x2,
        KEY_MODIFIER_ALT = 0x4,
        KEY_MODIFIER_CAPS = 0x8,
        KEY_MODIFIER_NUM = 0x10
    };

    const char * KeySymGetName( const Key key );

    bool PressIntKey( uint32_t max, uint32_t & result );

    size_t InsertKeySym( std::string & res, size_t pos, const Key key, const int32_t mod );
}

class LocalEvent
{
public:
    static LocalEvent & Get();
    static LocalEvent & GetClean(); // reset all previous event statuses and return a reference for events

    void SetMouseMotionGlobalHook( void ( *pf )( int32_t, int32_t ) )
    {
        mouse_motion_hook_func = pf;
    }

    void setGlobalKeyDownEventHook( std::function<void( const fheroes2::Key, const int32_t )> hook )
    {
        _globalKeyDownEventHook = std::move( hook );
    }

    static void SetStateDefaults();

    bool HandleEvents( bool delay = true, bool allowExit = false );

    bool MouseMotion() const
    {
        return ( modes & MOUSE_MOTION ) == MOUSE_MOTION;
    }

    const fheroes2::Point & GetMouseCursor() const
    {
        return mouse_cu;
    }

    const fheroes2::Point & GetMousePressLeft() const
    {
        return mouse_pl;
    }

    void ResetPressLeft()
    {
        mouse_pl = { -1, -1 };
    }

    bool MouseClickLeft();
    bool MouseClickMiddle();
    bool MouseClickRight();

    bool MouseClickLeft( const fheroes2::Rect & rt );
    bool MouseClickRight( const fheroes2::Rect & rt );

    bool MouseWheelUp() const;
    bool MouseWheelDn() const;

    bool MousePressLeft() const;

    bool MousePressLeft( const fheroes2::Rect & rt ) const
    {
        return MousePressLeft() && ( rt & mouse_pl );
    }

    bool MousePressRight() const;

    bool MousePressRight( const fheroes2::Rect & rt ) const
    {
        return MousePressRight() && ( rt & mouse_pr );
    }

    bool MouseReleaseLeft() const;

    bool MouseReleaseLeft( const fheroes2::Rect & rt ) const
    {
        return MouseReleaseLeft() && ( rt & mouse_rl );
    }

    bool MouseWheelUp( const fheroes2::Rect & rt ) const
    {
        return MouseWheelUp() && ( rt & mouse_cu );
    }

    bool MouseWheelDn( const fheroes2::Rect & rt ) const
    {
        return MouseWheelDn() && ( rt & mouse_cu );
    }

    bool MouseCursor( const fheroes2::Rect & rt ) const
    {
        return rt & mouse_cu;
    }

    bool KeyPress() const
    {
        return modes & KEY_PRESSED;
    }

    bool KeyPress( fheroes2::Key key ) const
    {
        return key == key_value && ( modes & KEY_PRESSED );
    }

    bool KeyHold() const
    {
        return ( modes & KEY_HOLD ) != 0;
    }

    fheroes2::Key KeyValue() const
    {
        return key_value;
    }

    static int32_t getCurrentKeyModifiers();

    static void RegisterCycling( void ( *preRenderDrawing )() = nullptr, void ( *postRenderDrawing )() = nullptr );

    // These two methods are useful for video playback
    static void PauseCycling();

    static void ResumeCycling()
    {
        RegisterCycling();
    }

    void OpenVirtualKeyboard();
    void CloseVirtualKeyboard();

    void OpenController();
    void CloseController();
    void OpenTouchpad();

    void SetControllerPointerSpeed( const int newSpeed )
    {
        if ( newSpeed > 0 ) {
            _controllerPointerSpeed = newSpeed / CONTROLLER_SPEED_MOD;
        }
    }

private:
    LocalEvent();

    static void SetState( const uint32_t type, const bool enable );

    void HandleMouseMotionEvent( const SDL_MouseMotionEvent & );
    void HandleMouseButtonEvent( const SDL_MouseButtonEvent & );
    void HandleKeyboardEvent( const SDL_KeyboardEvent & );

    static void StopSounds();
    static void ResumeSounds();

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    void HandleMouseWheelEvent( const SDL_MouseWheelEvent & );
    void HandleControllerAxisEvent( const SDL_ControllerAxisEvent & motion );
    void HandleControllerButtonEvent( const SDL_ControllerButtonEvent & button );
    void ProcessControllerAxisMotion();
    void HandleTouchEvent( const SDL_TouchFingerEvent & event );

    static void OnSdl2WindowEvent( const SDL_Event & event );
#else
    void OnActiveEvent( const SDL_Event & event );
#endif

    enum flag_t
    {
        KEY_PRESSED = 0x0001, // key on the keyboard has been pressed
        MOUSE_MOTION = 0x0002, // mouse cursor has been moved
        MOUSE_PRESSED = 0x0004, // mouse button is currently pressed
        MOUSE_RELEASED = 0x0008, // mouse button has just been released
        MOUSE_CLICKED = 0x0010, // mouse button has been clicked
        MOUSE_WHEEL = 0x0020, // mouse wheel has been rotated
        KEY_HOLD = 0x0040 // key on the keyboard is currently being held down
    };

    void SetModes( flag_t f )
    {
        modes |= f;
    }

    void ResetModes( flag_t f )
    {
        modes &= ~f;
    }

    int modes;
    fheroes2::Key key_value;
    int mouse_button;

    fheroes2::Point mouse_pl; // press left
    fheroes2::Point mouse_pm; // press middle
    fheroes2::Point mouse_pr; // press right

    fheroes2::Point mouse_rl; // release left
    fheroes2::Point mouse_rm; // release middle
    fheroes2::Point mouse_rr; // release right

    fheroes2::Point mouse_cu; // point cursor

    fheroes2::Point mouse_wm; // wheel movement

    void ( *mouse_motion_hook_func )( int32_t, int32_t );

    std::function<void( const fheroes2::Key, const int32_t )> _globalKeyDownEventHook;

    uint32_t loop_delay;

    enum
    {
        CONTROLLER_L_DEADZONE = 4000,
        CONTROLLER_R_DEADZONE = 25000
    };

    // used to convert user-friendly pointer speed values into more useable ones
    const double CONTROLLER_SPEED_MOD = 2000000.0;
    double _controllerPointerSpeed = 10.0 / CONTROLLER_SPEED_MOD;
    double _emulatedPointerPosX = 0;
    double _emulatedPointerPosY = 0;

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    // bigger value correndsponds to faster pointer movement speed with bigger stick axis values
    const double CONTROLLER_AXIS_SPEEDUP = 1.03;

    SDL_GameController * _gameController = nullptr;
    SDL_FingerID _firstFingerId = 0;
    fheroes2::Time _controllerTimer;
    int16_t _controllerLeftXAxis = 0;
    int16_t _controllerLeftYAxis = 0;
    int16_t _controllerRightXAxis = 0;
    int16_t _controllerRightYAxis = 0;
    bool _controllerScrollActive = false;
    bool _touchpadAvailable = false;
    int16_t _numTouches = 0;
#endif
};

#endif
