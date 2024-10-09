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
        KEY_MODIFIER_CTRL = ( 1 << 0 ),
        KEY_MODIFIER_SHIFT = ( 1 << 1 ),
        KEY_MODIFIER_ALT = ( 1 << 2 ),
        KEY_MODIFIER_CAPS = ( 1 << 3 ),
        KEY_MODIFIER_NUM = ( 1 << 4 )
    };

    const char * KeySymGetName( const Key key );

    size_t InsertKeySym( std::string & res, size_t pos, const Key key, const int32_t mod );
}

class LocalEvent
{
public:
    friend class EventProcessing::EventEngine;

    static LocalEvent & Get();

    static void initEventEngine();

    static int32_t getCurrentKeyModifiers();

    // Only one instance of the class can exist.
    LocalEvent( const LocalEvent & ) = delete;
    LocalEvent & operator=( const LocalEvent & ) = delete;

    // Reset all previous event statuses.
    void reset()
    {
        _actionStates = 0;
    }

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

    bool hasMouseMoved() const
    {
        return ( _actionStates & MOUSE_MOTION ) == MOUSE_MOTION;
    }

    const fheroes2::Point & getMouseCursorPos() const
    {
        return _mouseCursorPos;
    }

    const fheroes2::Point & getMouseLeftButtonPressedPos() const
    {
        return _mousePressLeftPos;
    }

    bool MouseClickLeft();
    bool MouseClickMiddle();
    bool MouseClickRight();

    bool MouseClickLeft( const fheroes2::Rect & area );
    bool MouseClickRight( const fheroes2::Rect & area );

    // The long press event is triggered only once. If this event was triggered (i.e. this method was called
    // and returned true), then after releasing the mouse button, the click event will not be triggered.
    bool MouseLongPressLeft( const fheroes2::Rect & rt );

    bool isMouseWheelUp() const
    {
        return ( _actionStates & MOUSE_WHEEL ) && _mouseWheelMovementOffset.y > 0;
    }

    bool isMouseWheelDown() const
    {
        return ( _actionStates & MOUSE_WHEEL ) && _mouseWheelMovementOffset.y < 0;
    }

    bool isMouseLeftButtonPressed() const
    {
        return ( _actionStates & MOUSE_PRESSED ) && _currentMouseButton == MouseButtonType::MOUSE_BUTTON_LEFT;
    }

    bool isMouseLeftButtonPressedInArea( const fheroes2::Rect & area ) const
    {
        return isMouseLeftButtonPressed() && ( area & _mousePressLeftPos );
    }

    bool isMouseRightButtonPressed() const
    {
        return ( _actionStates & MOUSE_PRESSED ) && _currentMouseButton == MouseButtonType::MOUSE_BUTTON_RIGHT;
    }

    bool isMouseRightButtonPressedInArea( const fheroes2::Rect & area ) const
    {
        return isMouseRightButtonPressed() && ( area & _mousePressRightPos );
    }

    bool isMouseLeftButtonReleased() const
    {
        return ( _actionStates & MOUSE_RELEASED ) && _currentMouseButton == MouseButtonType::MOUSE_BUTTON_LEFT;
    }

    bool isMouseLeftButtonReleasedInArea( const fheroes2::Rect & area ) const
    {
        return isMouseLeftButtonReleased() && ( area & _mouseReleaseLeftPos );
    }

    bool isMouseWheelUpInArea( const fheroes2::Rect & area ) const
    {
        return isMouseWheelUp() && ( area & _mouseCursorPos );
    }

    bool isMouseWheelDownInArea( const fheroes2::Rect & area ) const
    {
        return isMouseWheelDown() && ( area & _mouseCursorPos );
    }

    bool isMouseCursorPosInArea( const fheroes2::Rect & area ) const
    {
        return area & _mouseCursorPos;
    }

    // Returns true if the current mouse event is triggered by the touchpad and not the mouse (in other words,
    // this event is emulated using the touchpad). It is assumed that there is only one mouse in the system,
    // and touchpad events have priority in this sense - that is, as long as the touchpad emulates pressing any
    // mouse button, it is assumed that all mouse events are triggered by the touchpad.
    bool isMouseEventFromTouchpad() const
    {
        return _actionStates & MOUSE_TOUCH;
    }

    bool isAnyKeyPressed() const
    {
        return _actionStates & KEY_PRESSED;
    }

    bool isKeyPressed( const fheroes2::Key key ) const
    {
        return key == _currentKeyboardValue && ( _actionStates & KEY_PRESSED );
    }

    bool isKeyBeingHold() const
    {
        return ( _actionStates & KEY_HOLD ) != 0;
    }

    fheroes2::Key getPressedKeyValue() const
    {
        return _currentKeyboardValue;
    }

    void initController();
    void CloseController();

    void SetControllerPointerSpeed( const int newSpeed )
    {
        if ( newSpeed > 0 ) {
            _controllerPointerSpeed = newSpeed / _constrollerSpeedModifier;
        }
    }

    bool isDragInProgress() const
    {
        return _actionStates & DRAG_ONGOING;
    }

    void registerDrag()
    {
        setStates( DRAG_ONGOING );
    }

private:
    enum class MouseButtonType : uint8_t
    {
        MOUSE_BUTTON_UNKNOWN,
        MOUSE_BUTTON_LEFT,
        MOUSE_BUTTON_MIDDLE,
        MOUSE_BUTTON_RIGHT
    };

    enum class KeyboardEventState : uint8_t
    {
        KEY_UNKNOWN,
        KEY_DOWN,
        KEY_UP
    };

    enum class ControllerAxisType : uint8_t
    {
        CONTROLLER_AXIS_UNKNOWN,
        CONTROLLER_AXIS_LEFT_X,
        CONTROLLER_AXIS_LEFT_Y,
        CONTROLLER_AXIS_RIGHT_X,
        CONTROLLER_AXIS_RIGHT_Y
    };

    enum class ControllerButtonType : uint8_t
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
        CONTROLLER_BUTTON_START,
        CONTROLLER_BUTTON_GUIDE
    };

    enum class TouchFingerEventType : uint8_t
    {
        FINGER_EVENT_UNKNOWN,
        FINGER_EVENT_DOWN,
        FINGER_EVENT_UP,
        FINGER_EVENT_MOTION
    };

    enum : uint32_t
    {
        NO_EVENT = 0,
        // Key on the keyboard has been pressed
        KEY_PRESSED = ( 1 << 0 ),
        // Mouse cursor has been moved
        MOUSE_MOTION = ( 1 << 1 ),
        // Mouse button is currently pressed
        MOUSE_PRESSED = ( 1 << 2 ),
        // Mouse button has just been released
        MOUSE_RELEASED = ( 1 << 3 ),
        // Mouse wheel has been rotated
        MOUSE_WHEEL = ( 1 << 4 ),
        // Current mouse event has been triggered by the touchpad
        MOUSE_TOUCH = ( 1 << 5 ),
        // Key on the keyboard is currently being held down
        KEY_HOLD = ( 1 << 6 ),
        // Some UI component registered the start of drag motion
        DRAG_ONGOING = ( 1 << 7 ),
    };

    enum : int16_t
    {
        CONTROLLER_L_DEADZONE = 4000,
        CONTROLLER_R_DEADZONE = 25000
    };

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

    std::unique_ptr<EventProcessing::EventEngine> _engine;

    uint32_t _actionStates{ NO_EVENT };
    fheroes2::Key _currentKeyboardValue{ fheroes2::Key::NONE };
    MouseButtonType _currentMouseButton{ MouseButtonType::MOUSE_BUTTON_UNKNOWN };

    fheroes2::Point _mousePressLeftPos;
    fheroes2::Point _mousePressMiddlePos;
    fheroes2::Point _mousePressRightPos;

    fheroes2::Point _mouseReleaseLeftPos;
    fheroes2::Point _mouseReleaseMiddlePos;
    fheroes2::Point _mouseReleaseRightPos;

    fheroes2::Point _mouseCursorPos;

    fheroes2::Point _mouseWheelMovementOffset;

    LongPressDelay _mouseButtonLongPressDelay;

    std::function<fheroes2::Rect( const int32_t, const int32_t )> _globalMouseMotionEventHook;
    std::function<void( const fheroes2::Key, const int32_t )> _globalKeyDownEventHook;

    fheroes2::Rect _mouseCursorRenderArea;

    // used to convert user-friendly pointer speed values into more usable ones
    const double _constrollerSpeedModifier{ 2000000.0 };
    double _controllerPointerSpeed{ 10.0 / _constrollerSpeedModifier };
    fheroes2::PointBase2D<double> _emulatedPointerPos;

    // bigger value corresponds to faster pointer movement speed with bigger stick axis values
    const double _controllerAxisSpeedup{ 1.03 };
    const double _controllerTriggerCursorSpeedup{ 2.0 };

    fheroes2::Time _controllerTimer;
    int16_t _controllerLeftXAxis{ 0 };
    int16_t _controllerLeftYAxis{ 0 };
    int16_t _controllerRightXAxis{ 0 };
    int16_t _controllerRightYAxis{ 0 };
    bool _controllerScrollActive{ false };

    // IDs of currently active (touching the touchpad) fingers, if any. These IDs consist of a touch device id and a finger id.
    std::pair<std::optional<std::pair<int64_t, int64_t>>, std::optional<std::pair<int64_t, int64_t>>> _fingerIds;
    // Is the two-finger gesture currently being processed
    bool _isTwoFingerGestureInProgress = false;

    static void StopSounds();
    static void ResumeSounds();

    static void onRenderDeviceResetEvent();

    LocalEvent();

    void onMouseMotionEvent( fheroes2::Point position );
    void onMouseButtonEvent( const bool isPressed, const MouseButtonType buttonType, fheroes2::Point position );
    void onKeyboardEvent( const fheroes2::Key key, const int32_t keyModifier, const KeyboardEventState keyState );
    void onMouseWheelEvent( fheroes2::Point position );
    void onControllerAxisEvent( const ControllerAxisType axisType, const int16_t value );
    void onControllerButtonEvent( const bool isPressed, const ControllerButtonType buttonType );
    void onTouchFingerEvent( const TouchFingerEventType eventType, const int64_t touchId, const int64_t fingerId, fheroes2::PointBase2D<float> position );

    void ProcessControllerAxisMotion();

    void setStates( const uint32_t states )
    {
        _actionStates |= states;
    }

    void resetStates( const uint32_t states )
    {
        _actionStates &= ~states;
    }
};

#endif
