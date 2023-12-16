/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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
#include <optional>
#include <string>
#include <utility>

#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <SDL_touch.h>

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
        KEY_LEFT_ALT,
        KEY_RIGHT_ALT,
        KEY_LEFT_CONTROL,
        KEY_RIGHT_CONTROL,
        KEY_LEFT_SHIFT,
        KEY_RIGHT_SHIFT,
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

    void setGlobalMouseMotionEventHook( std::function<fheroes2::Rect( const int32_t, const int32_t )> hook )
    {
        _globalMouseMotionEventHook = std::move( hook );
    }

    void setGlobalKeyDownEventHook( std::function<void( const fheroes2::Key, const int32_t )> hook )
    {
        _globalKeyDownEventHook = std::move( hook );
    }

    static void setEventProcessingStates();

    bool HandleEvents( const bool sleepAfterEventProcessing = true, const bool allowExit = false );

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

    bool MouseClickLeft();
    bool MouseClickMiddle();
    bool MouseClickRight();

    bool MouseClickLeft( const fheroes2::Rect & rt );
    bool MouseClickRight( const fheroes2::Rect & rt );

    // The long press event is triggered only once. If this event was triggered (i.e. this method was called
    // and returned true), then after releasing the mouse button, the click event will not be triggered.
    bool MouseLongPressLeft( const fheroes2::Rect & rt );

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

    // Returns true if the current mouse event is triggered by the touchpad and not the mouse (in other words,
    // this event is emulated using the touchpad). It is assumed that there is only one mouse in the system,
    // and touchpad events have priority in this sense - that is, as long as the touchpad emulates pressing any
    // mouse button, it is assumed that all mouse events are triggered by the touchpad.
    bool MouseEventFromTouchpad() const
    {
        return modes & MOUSE_TOUCH;
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

    void OpenController();
    void CloseController();

    static void OpenTouchpad();

    void SetControllerPointerSpeed( const int newSpeed )
    {
        if ( newSpeed > 0 ) {
            _controllerPointerSpeed = newSpeed / CONTROLLER_SPEED_MOD;
        }
    }

private:
    LocalEvent();

    void HandleMouseMotionEvent( const SDL_MouseMotionEvent & );
    void HandleMouseButtonEvent( const SDL_MouseButtonEvent & );
    void HandleKeyboardEvent( const SDL_KeyboardEvent & );

    static void StopSounds();
    static void ResumeSounds();

    void HandleMouseWheelEvent( const SDL_MouseWheelEvent & );
    void HandleControllerAxisEvent( const SDL_ControllerAxisEvent & motion );
    void HandleControllerButtonEvent( const SDL_ControllerButtonEvent & button );
    void HandleTouchEvent( const SDL_TouchFingerEvent & event );

    // Returns true if frame rendering is required.
    static bool HandleWindowEvent( const SDL_WindowEvent & event );
    static void HandleRenderDeviceResetEvent();

    void ProcessControllerAxisMotion();

    enum : uint32_t
    {
        // Key on the keyboard has been pressed
        KEY_PRESSED = 0x0001,
        // Mouse cursor has been moved
        MOUSE_MOTION = 0x0002,
        // Mouse button is currently pressed
        MOUSE_PRESSED = 0x0004,
        // Mouse button has just been released
        MOUSE_RELEASED = 0x0008,
        // Mouse wheel has been rotated
        MOUSE_WHEEL = 0x0010,
        // Current mouse event has been triggered by the touchpad
        MOUSE_TOUCH = 0x0020,
        // Key on the keyboard is currently being held down
        KEY_HOLD = 0x0040,
    };

    enum
    {
        CONTROLLER_L_DEADZONE = 4000,
        CONTROLLER_R_DEADZONE = 25000
    };

    void SetModes( const uint32_t f )
    {
        modes |= f;
    }

    void ResetModes( const uint32_t f )
    {
        modes &= ~f;
    }

    uint32_t modes;
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

    class LongPressDelay final : public fheroes2::TimeDelay
    {
    public:
        using TimeDelay::TimeDelay;

        void reset()
        {
            TimeDelay::reset();

            _triggered = false;
        }

        bool isTriggered() const
        {
            return _triggered;
        }

        void setTriggered()
        {
            _triggered = true;
        }

    private:
        bool _triggered{ false };
    };

    LongPressDelay _mouseButtonLongPressDelay;

    std::function<fheroes2::Rect( const int32_t, const int32_t )> _globalMouseMotionEventHook;
    std::function<void( const fheroes2::Key, const int32_t )> _globalKeyDownEventHook;

    fheroes2::Rect _mouseCursorRenderArea;

    // used to convert user-friendly pointer speed values into more usable ones
    const double CONTROLLER_SPEED_MOD = 2000000.0;
    double _controllerPointerSpeed = 10.0 / CONTROLLER_SPEED_MOD;
    double _emulatedPointerPosX = 0;
    double _emulatedPointerPosY = 0;

    // bigger value corresponds to faster pointer movement speed with bigger stick axis values
    const double CONTROLLER_AXIS_SPEEDUP = 1.03;
    const double CONTROLLER_TRIGGER_CURSOR_SPEEDUP = 2.0;

    SDL_GameController * _gameController = nullptr;
    fheroes2::Time _controllerTimer;
    int16_t _controllerLeftXAxis = 0;
    int16_t _controllerLeftYAxis = 0;
    int16_t _controllerRightXAxis = 0;
    int16_t _controllerRightYAxis = 0;
    bool _controllerScrollActive = false;

    // IDs of currently active (touching the touchpad) fingers, if any. These IDs consist of a touch device id and a finger id.
    std::pair<std::optional<std::pair<SDL_TouchID, SDL_FingerID>>, std::optional<std::pair<SDL_TouchID, SDL_FingerID>>> _fingerIds;
    // Is the two-finger gesture currently being processed
    bool _isTwoFingerGestureInProgress = false;
};

#endif
