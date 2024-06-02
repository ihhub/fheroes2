/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "math_base.h"
#include "timing.h"

namespace EventProcessing
{
    class EventEngine;
}

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
    friend class EventProcessing::EventEngine;

    static LocalEvent & Get();

    static void initEventEngine();

    static int32_t getCurrentKeyModifiers();

    // Reset all previous event statuses.
    void reset();

    void setGlobalMouseMotionEventHook( std::function<fheroes2::Rect( const int32_t, const int32_t )> hook )
    {
        _globalMouseMotionEventHook = std::move( hook );
    }

    void setGlobalKeyDownEventHook( std::function<void( const fheroes2::Key, const int32_t )> hook )
    {
        _globalKeyDownEventHook = std::move( hook );
    }

    // Return false when event handling should be stopped, true otherwise.
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

    bool MouseWheelUp() const
    {
        return ( modes & MOUSE_WHEEL ) && mouse_wm.y > 0;
    }

    bool MouseWheelDn() const
    {
        return ( modes & MOUSE_WHEEL ) && mouse_wm.y < 0;
    }

    bool MousePressLeft() const
    {
        return ( modes & MOUSE_PRESSED ) && MOUSE_BUTTON_LEFT == mouse_button;
    }

    bool MousePressLeft( const fheroes2::Rect & rt ) const
    {
        return MousePressLeft() && ( rt & mouse_pl );
    }

    bool MousePressRight() const
    {
        return ( modes & MOUSE_PRESSED ) && MOUSE_BUTTON_RIGHT == mouse_button;
    }

    bool MousePressRight( const fheroes2::Rect & rt ) const
    {
        return MousePressRight() && ( rt & mouse_pr );
    }

    bool MouseReleaseLeft() const
    {
        return ( modes & MOUSE_RELEASED ) && MOUSE_BUTTON_LEFT == mouse_button;
    }

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

    void OpenController();
    void CloseController();

    void SetControllerPointerSpeed( const int newSpeed )
    {
        if ( newSpeed > 0 ) {
            _controllerPointerSpeed = newSpeed / CONTROLLER_SPEED_MOD;
        }
    }

    bool isDragInProgress() const
    {
        return modes & DRAG_ONGOING;
    }

    void registerDrag()
    {
        SetModes( DRAG_ONGOING );
    }

private:
    static void StopSounds();
    static void ResumeSounds();

    static void onRenderDeviceResetEvent();

    LocalEvent();

    void onMouseMotionEvent( fheroes2::Point position );
    void onMouseButtonEvent( const bool isPressed, const int buttonType, fheroes2::Point position );
    void onKeyboardEvent( const fheroes2::Key key, const int32_t keyModifier, const uint8_t keyState );
    void onMouseWheelEvent( fheroes2::Point position );
    void onControllerAxisEvent( const uint8_t axisType, const int16_t value );
    void onControllerButtonEvent( const bool isPressed, const int buttonType );
    void onTouchFingerEvent( const uint8_t eventType, const int64_t touchId, const int64_t fingerId, fheroes2::PointBase2D<float> position );

    void ProcessControllerAxisMotion();

    void SetModes( const uint32_t f )
    {
        modes |= f;
    }

    void ResetModes( const uint32_t f )
    {
        modes &= ~f;
    }

    enum MouseButtonType : int
    {
        MOUSE_BUTTON_UNKNOWN,
        MOUSE_BUTTON_LEFT,
        MOUSE_BUTTON_MIDDLE,
        MOUSE_BUTTON_RIGHT
    };

    enum KeyboardEventState : uint8_t
    {
        KEY_UNKNOWN,
        KEY_DOWN,
        KEY_UP
    };

    enum ControllerAxisType : uint8_t
    {
        CONTROLLER_AXIS_UNKNOWN,
        CONTROLLER_AXIS_LEFT_X,
        CONTROLLER_AXIS_LEFT_Y,
        CONTROLLER_AXIS_RIGHT_X,
        CONTROLLER_AXIS_RIGHT_Y
    };

    enum ControllerButtonType : int
    {
        CONTROLLER_BUTTON_UNKNOWN,
        CONTROLLER_BUTTON_A,
        CONTROLLER_BUTTON_B,
        CONTROLLER_BUTTON_X,
        CONTROLLER_BUTTON_Y,
        CONTROLLER_BUTTON_RIGHT_SHOULDER,
        CONTROLLER_BUTTON_LEFT_SHOULDER,
        CONTROLLER_BUTTON_DPAD_UP,
        CONTROLLER_BUTTON_DPAD_DOWN,
        CONTROLLER_BUTTON_DPAD_RIGHT,
        CONTROLLER_BUTTON_DPAD_LEFT,
        CONTROLLER_BUTTON_BACK,
        CONTROLLER_BUTTON_START
    };

    enum TouchFingerEventType
    {
        FINGER_EVENT_UNKNOWN,
        FINGER_EVENT_DOWN,
        FINGER_EVENT_UP,
        FINGER_EVENT_MOTION
    };

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
        // Some UI component registered the start of drag motion
        DRAG_ONGOING = 0x0080,
    };

    enum
    {
        CONTROLLER_L_DEADZONE = 4000,
        CONTROLLER_R_DEADZONE = 25000
    };

    std::unique_ptr<EventProcessing::EventEngine> _engine;

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

    fheroes2::Time _controllerTimer;
    int16_t _controllerLeftXAxis = 0;
    int16_t _controllerLeftYAxis = 0;
    int16_t _controllerRightXAxis = 0;
    int16_t _controllerRightYAxis = 0;
    bool _controllerScrollActive = false;

    // IDs of currently active (touching the touchpad) fingers, if any. These IDs consist of a touch device id and a finger id.
    std::pair<std::optional<std::pair<int64_t, int64_t>>, std::optional<std::pair<int64_t, int64_t>>> _fingerIds;
    // Is the two-finger gesture currently being processed
    bool _isTwoFingerGestureInProgress = false;
};

#endif
