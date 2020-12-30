/***************************************************************************
 *   Copyright (C) 2008 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "localevent.h"
#include "audio_mixer.h"
#include "audio_music.h"
#include "error.h"
#include "pal.h"
#include "screen.h"

#define TAP_DELAY_EMULATE 1050

LocalEvent::LocalEvent()
    : modes( 0 )
    , key_value( KEY_NONE )
    , mouse_state( 0 )
    , mouse_button( 0 )
    , mouse_st( 0, 0 )
    , redraw_cursor_func( NULL )
    , keyboard_filter_func( NULL )
    , clock_delay( TAP_DELAY_EMULATE )
    , loop_delay( 1 )
    , _isHiddenWindow( false )
    , _isMusicPaused( false )
    , _isSoundPaused( false )
{}

const Point & LocalEvent::GetMousePressLeft( void ) const
{
    return mouse_pl;
}

const Point & LocalEvent::GetMousePressMiddle( void ) const
{
    return mouse_pm;
}

const Point & LocalEvent::GetMousePressRight( void ) const
{
    return mouse_pr;
}

const Point & LocalEvent::GetMouseReleaseLeft( void ) const
{
    return mouse_rl;
}

const Point & LocalEvent::GetMouseReleaseMiddle( void ) const
{
    return mouse_rm;
}

const Point & LocalEvent::GetMouseReleaseRight( void ) const
{
    return mouse_rr;
}

void LocalEvent::SetTapMode( bool f )
{
    if ( f )
        SetModes( TAP_MODE );
    else {
        ResetModes( TAP_MODE );
        ResetModes( CLOCK_ON );
        clock.Stop();
    }
}

void LocalEvent::SetTapDelayForRightClickEmulation( u32 d )
{
    clock_delay = d < 200 ? TAP_DELAY_EMULATE : d;
}

void LocalEvent::SetMouseOffsetX( int16_t x )
{
    SetModes( MOUSE_OFFSET );
    mouse_st.x = x;
}

void LocalEvent::SetMouseOffsetY( int16_t y )
{
    SetModes( MOUSE_OFFSET );
    mouse_st.y = y;
}

void LocalEvent::SetModes( flag_t f )
{
    modes |= f;
}

void LocalEvent::ResetModes( flag_t f )
{
    modes &= ~f;
}

void LocalEvent::SetGlobalFilter( bool f )
{
    f ? SetModes( GLOBAL_FILTER ) : ResetModes( GLOBAL_FILTER );
}

const char * KeySymGetName( KeySym sym )
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    return SDL_GetKeyName( static_cast<SDL_Keycode>( sym ) );
#else
    return SDL_GetKeyName( static_cast<SDLKey>( sym ) );
#endif
}

KeySym GetKeySym( int key )
{
    switch ( key ) {
    default:
        break;

    case SDLK_RETURN:
        return KEY_RETURN;
    case SDLK_LEFT:
        return KEY_LEFT;
    case SDLK_RIGHT:
        return KEY_RIGHT;
    case SDLK_UP:
        return KEY_UP;
    case SDLK_DOWN:
        return KEY_DOWN;

    case SDLK_ESCAPE:
        return KEY_ESCAPE;
    case SDLK_BACKSPACE:
        return KEY_BACKSPACE;
    case SDLK_EXCLAIM:
        return KEY_EXCLAIM;
    case SDLK_QUOTEDBL:
        return KEY_QUOTEDBL;
    case SDLK_HASH:
        return KEY_HASH;
    case SDLK_DOLLAR:
        return KEY_DOLLAR;
    case SDLK_AMPERSAND:
        return KEY_AMPERSAND;
    case SDLK_QUOTE:
        return KEY_QUOTE;
    case SDLK_LEFTPAREN:
        return KEY_LEFTPAREN;
    case SDLK_RIGHTPAREN:
        return KEY_RIGHTPAREN;
    case SDLK_ASTERISK:
        return KEY_ASTERISK;
    case SDLK_PLUS:
        return KEY_PLUS;
    case SDLK_COMMA:
        return KEY_COMMA;
    case SDLK_MINUS:
        return KEY_MINUS;
    case SDLK_PERIOD:
        return KEY_PERIOD;
    case SDLK_SLASH:
        return KEY_SLASH;
    case SDLK_COLON:
        return KEY_COLON;
    case SDLK_SEMICOLON:
        return KEY_SEMICOLON;
    case SDLK_LESS:
        return KEY_LESS;
    case SDLK_EQUALS:
        return KEY_EQUALS;
    case SDLK_GREATER:
        return KEY_GREATER;
    case SDLK_QUESTION:
        return KEY_QUESTION;
    case SDLK_AT:
        return KEY_AT;
    case SDLK_LEFTBRACKET:
        return KEY_LEFTBRACKET;
    case SDLK_BACKSLASH:
        return KEY_BACKSLASH;
    case SDLK_RIGHTBRACKET:
        return KEY_RIGHTBRACKET;
    case SDLK_CARET:
        return KEY_CARET;
    case SDLK_UNDERSCORE:
        return KEY_UNDERSCORE;
    case SDLK_LALT:
        return KEY_ALT;
    case SDLK_RALT:
        return KEY_ALT;
    case SDLK_LCTRL:
        return KEY_CONTROL;
    case SDLK_RCTRL:
        return KEY_CONTROL;
    case SDLK_LSHIFT:
        return KEY_SHIFT;
    case SDLK_RSHIFT:
        return KEY_SHIFT;
    case SDLK_TAB:
        return KEY_TAB;
    case SDLK_SPACE:
        return KEY_SPACE;
    case SDLK_DELETE:
        return KEY_DELETE;
    case SDLK_PAGEUP:
        return KEY_PAGEUP;
    case SDLK_PAGEDOWN:
        return KEY_PAGEDOWN;
    case SDLK_F1:
        return KEY_F1;
    case SDLK_F2:
        return KEY_F2;
    case SDLK_F3:
        return KEY_F3;
    case SDLK_F4:
        return KEY_F4;
    case SDLK_F5:
        return KEY_F5;
    case SDLK_F6:
        return KEY_F6;
    case SDLK_F7:
        return KEY_F7;
    case SDLK_F8:
        return KEY_F8;
    case SDLK_F9:
        return KEY_F9;
    case SDLK_F10:
        return KEY_F10;
    case SDLK_F11:
        return KEY_F11;
    case SDLK_F12:
        return KEY_F12;
    case SDLK_0:
        return KEY_0;
    case SDLK_1:
        return KEY_1;
    case SDLK_2:
        return KEY_2;
    case SDLK_3:
        return KEY_3;
    case SDLK_4:
        return KEY_4;
    case SDLK_5:
        return KEY_5;
    case SDLK_6:
        return KEY_6;
    case SDLK_7:
        return KEY_7;
    case SDLK_8:
        return KEY_8;
    case SDLK_9:
        return KEY_9;
    case SDLK_a:
        return KEY_a;
    case SDLK_b:
        return KEY_b;
    case SDLK_c:
        return KEY_c;
    case SDLK_d:
        return KEY_d;
    case SDLK_e:
        return KEY_e;
    case SDLK_f:
        return KEY_f;
    case SDLK_g:
        return KEY_g;
    case SDLK_h:
        return KEY_h;
    case SDLK_i:
        return KEY_i;
    case SDLK_j:
        return KEY_j;
    case SDLK_k:
        return KEY_k;
    case SDLK_l:
        return KEY_l;
    case SDLK_m:
        return KEY_m;
    case SDLK_n:
        return KEY_n;
    case SDLK_o:
        return KEY_o;
    case SDLK_p:
        return KEY_p;
    case SDLK_q:
        return KEY_q;
    case SDLK_r:
        return KEY_r;
    case SDLK_s:
        return KEY_s;
    case SDLK_t:
        return KEY_t;
    case SDLK_u:
        return KEY_u;
    case SDLK_v:
        return KEY_v;
    case SDLK_w:
        return KEY_w;
    case SDLK_x:
        return KEY_x;
    case SDLK_y:
        return KEY_y;
    case SDLK_z:
        return KEY_z;

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    case SDLK_PRINTSCREEN:
        return KEY_PRINT;
    case SDLK_KP_0:
        return KEY_KP0;
    case SDLK_KP_1:
        return KEY_KP1;
    case SDLK_KP_2:
        return KEY_KP2;
    case SDLK_KP_3:
        return KEY_KP3;
    case SDLK_KP_4:
        return KEY_KP4;
    case SDLK_KP_5:
        return KEY_KP5;
    case SDLK_KP_6:
        return KEY_KP6;
    case SDLK_KP_7:
        return KEY_KP7;
    case SDLK_KP_8:
        return KEY_KP8;
    case SDLK_KP_9:
        return KEY_KP9;
#else
    case SDLK_PRINT:
        return KEY_PRINT;
    case SDLK_KP0:
        return KEY_KP0;
    case SDLK_KP1:
        return KEY_KP1;
    case SDLK_KP2:
        return KEY_KP2;
    case SDLK_KP3:
        return KEY_KP3;
    case SDLK_KP4:
        return KEY_KP4;
    case SDLK_KP5:
        return KEY_KP5;
    case SDLK_KP6:
        return KEY_KP6;
    case SDLK_KP7:
        return KEY_KP7;
    case SDLK_KP8:
        return KEY_KP8;
    case SDLK_KP9:
        return KEY_KP9;
#endif

    case SDLK_KP_PERIOD:
        return KEY_KP_PERIOD;
    case SDLK_KP_DIVIDE:
        return KEY_KP_DIVIDE;
    case SDLK_KP_MULTIPLY:
        return KEY_KP_MULTIPLY;
    case SDLK_KP_MINUS:
        return KEY_KP_MINUS;
    case SDLK_KP_PLUS:
        return KEY_KP_PLUS;
    case SDLK_KP_ENTER:
        return KEY_RETURN; // repath to the normal Enter
    case SDLK_KP_EQUALS:
        return KEY_KP_EQUALS;
    }

    return KEY_NONE;
}

namespace
{
    class ColorCycling
    {
    public:
        ColorCycling()
            : _counter( 0 )
            , _preRenderDrawing( nullptr )
            , _posRenderDrawing( nullptr )
        {}

        bool applyCycling( std::vector<uint8_t> & palette )
        {
            if ( _preRenderDrawing != nullptr )
                _preRenderDrawing();

            _timer.Stop();
            if ( _timer.Get() >= 220 ) {
                _timer.Start();
                palette = PAL::GetCyclingPalette( _counter++ );
                return true;
            }
            return false;
        }

        void reset()
        {
            _prevDraw.Start();

            if ( _posRenderDrawing != nullptr )
                _posRenderDrawing();
        }

        bool isRedrawRequired()
        {
            _prevDraw.Stop();
            return _prevDraw.Get() >= 220;
        }

        void registerDrawing( void ( *preRenderDrawing )(), void ( *postRenderDrawing )() )
        {
            if ( preRenderDrawing != nullptr )
                _preRenderDrawing = preRenderDrawing;

            if ( postRenderDrawing != nullptr )
                _posRenderDrawing = postRenderDrawing;
        }

    private:
        SDL::Time _timer;
        SDL::Time _prevDraw;
        uint32_t _counter;

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

LocalEvent & LocalEvent::Get( void )
{
    static LocalEvent le;

    return le;
}

void LocalEvent::RegisterCycling( void ( *preRenderDrawing )(), void ( *postRenderDrawing )() ) const
{
    colorCycling.registerDrawing( preRenderDrawing, postRenderDrawing );

    fheroes2::Display::instance().subscribe( ApplyCycling, ResetCycling );
}

void LocalEvent::PauseCycling()
{
    fheroes2::Display::instance().subscribe( NULL, NULL );
}

void LocalEvent::ResumeCycling()
{
    RegisterCycling();
}

LocalEvent & LocalEvent::GetClean()
{
    LocalEvent & le = Get();
    le.ResetModes( KEY_PRESSED );
    le.ResetModes( MOUSE_MOTION );
    le.ResetModes( MOUSE_PRESSED );
    le.ResetModes( CLICK_LEFT );
    le.ResetModes( CLICK_RIGHT );
    le.ResetModes( CLICK_MIDDLE );
    le.ResetModes( KEY_HOLD );
    return le;
}

bool LocalEvent::HandleEvents( bool delay, bool allowExit )
{
    if ( colorCycling.isRedrawRequired() ) {
        fheroes2::Display::instance().render();
    }

    SDL_Event event;

    ResetModes( MOUSE_MOTION );
    ResetModes( KEY_PRESSED );
    ResetModes( CLICK_LEFT );
    ResetModes( CLICK_MIDDLE );
    ResetModes( CLICK_RIGHT );

    mouse_wm = Point();

    while ( SDL_PollEvent( &event ) ) {
        switch ( event.type ) {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        case SDL_WINDOWEVENT:
            if ( Mixer::isValid() ) {
                if ( event.window.event == SDL_WINDOWEVENT_HIDDEN ) {
                    _isHiddenWindow = true;
                    _isMusicPaused = Music::isPaused();
                    _isSoundPaused = Mixer::isPaused( -1 );
                    Mixer::Pause();
                    Music::Pause();
                    loop_delay = 100;
                }
                else if ( event.window.event == SDL_WINDOWEVENT_SHOWN ) {
                    if ( _isHiddenWindow ) {
                        if ( !_isMusicPaused )
                            Music::Resume();
                        if ( !_isSoundPaused )
                            Mixer::Resume();
                        _isHiddenWindow = false;
                    }
                    loop_delay = 1;
                }
            }
            break;
#else
        case SDL_ACTIVEEVENT:
            if ( event.active.state & SDL_APPACTIVE ) {
                if ( Mixer::isValid() ) {
                    // iconify
                    if ( 0 == event.active.gain ) {
                        _isHiddenWindow = true;
                        _isMusicPaused = Music::isPaused();
                        _isSoundPaused = Mixer::isPaused( -1 );
                        Mixer::Pause();
                        Music::Pause();
                        loop_delay = 100;
                    }
                    else {
                        if ( _isHiddenWindow ) {
                            if ( !_isMusicPaused )
                                Music::Resume();
                            if ( !_isSoundPaused )
                                Mixer::Resume();
                            _isHiddenWindow = false;
                        }
                        loop_delay = 1;
                    }
                }
            }
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
#endif

        // exit
        case SDL_QUIT:
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        case SDL_WINDOWEVENT_CLOSE:
#endif
            // Error::Except(__FUNCTION__, "SDL_QUIT");
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

    // emulate press right
    if ( ( modes & TAP_MODE ) && ( modes & CLOCK_ON ) ) {
        clock.Stop();
        if ( clock_delay < clock.Get() ) {
            ResetModes( CLICK_LEFT );
            ResetModes( CLOCK_ON );
            mouse_pr = mouse_cu;
            SetModes( MOUSE_PRESSED );
            mouse_button = SDL_BUTTON_RIGHT;
        }
    }

    if ( delay )
        SDL_Delay( loop_delay );

    return true;
}

bool LocalEvent::MouseMotion( void ) const
{
    return ( modes & MOUSE_MOTION ) == MOUSE_MOTION;
}

bool LocalEvent::MouseMotion( const Rect & rt ) const
{
    return modes & MOUSE_MOTION ? rt & mouse_cu : false;
}

bool LocalEvent::MousePressLeft( void ) const
{
    return ( modes & MOUSE_PRESSED ) && SDL_BUTTON_LEFT == mouse_button;
}

bool LocalEvent::MouseReleaseLeft( void ) const
{
    return !( modes & MOUSE_PRESSED ) && SDL_BUTTON_LEFT == mouse_button;
}

bool LocalEvent::MousePressMiddle( void ) const
{
    return ( modes & MOUSE_PRESSED ) && SDL_BUTTON_MIDDLE == mouse_button;
}

bool LocalEvent::MouseReleaseMiddle( void ) const
{
    return !( modes & MOUSE_PRESSED ) && SDL_BUTTON_MIDDLE == mouse_button;
}

bool LocalEvent::MousePressRight( void ) const
{
    return ( modes & MOUSE_PRESSED ) && SDL_BUTTON_RIGHT == mouse_button;
}

bool LocalEvent::MouseReleaseRight( void ) const
{
    return !( modes & MOUSE_PRESSED ) && SDL_BUTTON_RIGHT == mouse_button;
}

void LocalEvent::HandleKeyboardEvent( SDL_KeyboardEvent & event )
{
    if ( KEY_NONE != GetKeySym( event.keysym.sym ) ) {
        if ( event.type == SDL_KEYDOWN ) {
            SetModes( KEY_PRESSED );
            SetModes( KEY_HOLD );
        }
        else if ( event.type == SDL_KEYUP ) {
            ResetModes( KEY_PRESSED );
            ResetModes( KEY_HOLD );
        }

        key_value = GetKeySym( event.keysym.sym );
    }
}

void LocalEvent::HandleMouseMotionEvent( const SDL_MouseMotionEvent & motion )
{
    mouse_state = motion.state;
    SetModes( MOUSE_MOTION );
    mouse_cu.x = motion.x;
    mouse_cu.y = motion.y;
    if ( modes & MOUSE_OFFSET )
        mouse_cu += mouse_st;
}

void LocalEvent::HandleMouseButtonEvent( const SDL_MouseButtonEvent & button )
{
    button.state == SDL_PRESSED ? SetModes( MOUSE_PRESSED ) : ResetModes( MOUSE_PRESSED );
    mouse_button = button.button;

    mouse_cu.x = button.x;
    mouse_cu.y = button.y;
    if ( modes & MOUSE_OFFSET )
        mouse_cu += mouse_st;

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
            SetModes( CLICK_LEFT );

            // emulate press right
            if ( modes & TAP_MODE ) {
                clock.Start();
                SetModes( CLOCK_ON );
            }
            break;

        case SDL_BUTTON_MIDDLE:
            mouse_pm = mouse_cu;
            SetModes( CLICK_MIDDLE );
            break;

        case SDL_BUTTON_RIGHT:
            mouse_pr = mouse_cu;
            SetModes( CLICK_RIGHT );
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
            SetModes( CLICK_LEFT );
            mouse_rl = mouse_cu;

            // emulate press right
            if ( modes & TAP_MODE ) {
                ResetModes( CLOCK_ON );
            }
            break;

        case SDL_BUTTON_MIDDLE:
            SetModes( CLICK_MIDDLE );
            mouse_rm = mouse_cu;
            break;

        case SDL_BUTTON_RIGHT:
            SetModes( CLICK_RIGHT );
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
#endif

bool LocalEvent::MouseClickLeft( void )
{
    if ( MouseReleaseLeft() && ( CLICK_LEFT & modes ) ) {
        ResetModes( CLICK_LEFT );
        return true;
    }

    return false;
}

bool LocalEvent::MouseClickLeft( const Rect & rt )
{
    // if(MouseReleaseLeft() && (rt & mouse_rl) && (CLICK_LEFT & modes) && ((modes & TAP_MODE) || (rt & mouse_pl)))
    if ( MouseReleaseLeft() && ( rt & mouse_pl ) && ( rt & mouse_rl ) && ( CLICK_LEFT & modes ) ) {
        ResetModes( CLICK_LEFT );
        return true;
    }

    return false;
}

bool LocalEvent::MouseClickMiddle( void )
{
    if ( MouseReleaseMiddle() && ( CLICK_MIDDLE & modes ) ) {
        ResetModes( CLICK_MIDDLE );
        return true;
    }

    return false;
}

bool LocalEvent::MouseClickMiddle( const Rect & rt )
{
    if ( MouseReleaseMiddle() && ( rt & mouse_pm ) && ( rt & mouse_rm ) && ( CLICK_MIDDLE & modes ) ) {
        ResetModes( CLICK_MIDDLE );
        return true;
    }

    return false;
}

bool LocalEvent::MouseClickRight( void )
{
    if ( MouseReleaseRight() && ( CLICK_RIGHT & modes ) ) {
        ResetModes( CLICK_RIGHT );
        return true;
    }

    return false;
}

bool LocalEvent::MouseClickRight( const Rect & rt )
{
    if ( MouseReleaseRight() && ( rt & mouse_pr ) && ( rt & mouse_rr ) && ( CLICK_RIGHT & modes ) ) {
        ResetModes( CLICK_RIGHT );
        return true;
    }

    return false;
}

bool LocalEvent::MouseWheelUp( void ) const
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    return ( modes & MOUSE_WHEEL ) && mouse_wm.y > 0;
#else
    return ( modes & MOUSE_PRESSED ) && SDL_BUTTON_WHEELUP == mouse_button;
#endif
}

bool LocalEvent::MouseWheelDn( void ) const
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    return ( modes & MOUSE_WHEEL ) && mouse_wm.y < 0;
#else
    return ( modes & MOUSE_PRESSED ) && SDL_BUTTON_WHEELDOWN == mouse_button;
#endif
}

bool LocalEvent::MousePressLeft( const Rect & rt ) const
{
    return MousePressLeft() && ( rt & mouse_pl );
}

bool LocalEvent::MousePressLeft( const Point & pt, u32 w, u32 h ) const
{
    return MousePressLeft() && ( Rect( pt.x, pt.y, w, h ) & mouse_pl );
}

bool LocalEvent::MousePressMiddle( const Rect & rt ) const
{
    return MousePressMiddle() && ( rt & mouse_pm );
}

bool LocalEvent::MousePressRight( const Rect & rt ) const
{
    return MousePressRight() && ( rt & mouse_pr );
}

bool LocalEvent::MouseReleaseLeft( const Rect & rt ) const
{
    return MouseReleaseLeft() && ( rt & mouse_rl );
}

bool LocalEvent::MouseReleaseMiddle( const Rect & rt ) const
{
    return MouseReleaseMiddle() && ( rt & mouse_rm );
}

bool LocalEvent::MouseReleaseRight( const Rect & rt ) const
{
    return MouseReleaseRight() && ( rt & mouse_rr );
}

void LocalEvent::ResetPressLeft( void )
{
    mouse_pl.x = -1;
    mouse_pl.y = -1;
}

void LocalEvent::ResetPressRight( void )
{
    mouse_pr.x = -1;
    mouse_pr.y = -1;
}

void LocalEvent::ResetPressMiddle( void )
{
    mouse_pm.x = -1;
    mouse_pm.y = -1;
}

void LocalEvent::ResetReleaseLeft( void )
{
    mouse_rl.x = -1;
    mouse_rl.y = -1;
}

void LocalEvent::ResetReleaseRight( void )
{
    mouse_rr.x = -1;
    mouse_rr.y = -1;
}

void LocalEvent::ResetReleaseMiddle( void )
{
    mouse_rm.x = -1;
    mouse_rm.y = -1;
}

bool LocalEvent::MouseWheelUp( const Rect & rt ) const
{
    return MouseWheelUp() && ( rt & mouse_cu );
}

bool LocalEvent::MouseWheelDn( const Rect & rt ) const
{
    return MouseWheelDn() && ( rt & mouse_cu );
}

bool LocalEvent::MouseCursor( const Rect & rt ) const
{
    return rt & mouse_cu;
}

int LocalEvent::KeyMod( void ) const
{
    return SDL_GetModState();
}

KeySym LocalEvent::KeyValue( void ) const
{
    return key_value;
}

bool LocalEvent::KeyPress( void ) const
{
    return modes & KEY_PRESSED;
}

bool LocalEvent::KeyPress( KeySym key ) const
{
    return key == key_value && ( modes & KEY_PRESSED );
}

void LocalEvent::SetGlobalFilterMouseEvents( void ( *pf )( s32, s32 ) )
{
    redraw_cursor_func = pf;
}

void LocalEvent::SetGlobalFilterKeysEvents( void ( *pf )( int, int ) )
{
    keyboard_filter_func = pf;
}

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
int LocalEvent::GlobalFilterEvents( void * /*userdata*/, SDL_Event * event )
#else
int LocalEvent::GlobalFilterEvents( const SDL_Event * event )
#endif
{
    LocalEvent & le = LocalEvent::Get();

    // motion
    if ( ( le.modes & GLOBAL_FILTER ) && SDL_MOUSEMOTION == event->type ) {
        // redraw cursor
        if ( le.redraw_cursor_func ) {
            if ( le.modes & MOUSE_OFFSET )
                ( *( le.redraw_cursor_func ) )( event->motion.x + le.mouse_st.x, event->motion.y + le.mouse_st.y );
            else
                ( *( le.redraw_cursor_func ) )( event->motion.x, event->motion.y );
        }
    }

    // key
    if ( ( le.modes & GLOBAL_FILTER ) && SDL_KEYDOWN == event->type )

    {
        // key event
        if ( le.keyboard_filter_func )
            ( *( le.keyboard_filter_func ) )( event->key.keysym.sym, event->key.keysym.mod );
    }

    return 1;
}

void LocalEvent::SetState( u32 type, bool enable )
{
    SDL_EventState( type, enable ? SDL_ENABLE : SDL_IGNORE );
}

int LocalEvent::GetState( u32 type )
{
    return SDL_EventState( type, SDL_QUERY );
}

void LocalEvent::SetStateDefaults( void )
{
    SetState( SDL_USEREVENT, true );
    SetState( SDL_KEYDOWN, true );
    SetState( SDL_KEYUP, true );
    SetState( SDL_MOUSEMOTION, true );
    SetState( SDL_MOUSEBUTTONDOWN, true );
    SetState( SDL_MOUSEBUTTONUP, true );
    SetState( SDL_QUIT, true );

    SetState( SDL_JOYAXISMOTION, false );
    SetState( SDL_JOYBALLMOTION, false );
    SetState( SDL_JOYHATMOTION, false );
    SetState( SDL_JOYBUTTONUP, false );
    SetState( SDL_JOYBUTTONDOWN, false );
    SetState( SDL_SYSWMEVENT, false );

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    SetState( SDL_WINDOWEVENT, true );

    SDL_SetEventFilter( GlobalFilterEvents, NULL );
#else
    SetState( SDL_ACTIVEEVENT, true );

    SetState( SDL_SYSWMEVENT, false );
    SetState( SDL_VIDEORESIZE, false );
    SetState( SDL_VIDEOEXPOSE, false );

    SDL_SetEventFilter( GlobalFilterEvents );
#endif
}
