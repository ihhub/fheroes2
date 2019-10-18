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

#include "error.h"
#include "display.h"
#include "audio_music.h"
#include "audio_mixer.h"
#include "localevent.h"

#define TAP_DELAY_EMULATE 1050

LocalEvent::LocalEvent() : modes(0), key_value(KEY_NONE), mouse_state(0),
    mouse_button(0), mouse_st(0, 0), redraw_cursor_func(NULL), keyboard_filter_func(NULL),
    clock_delay(TAP_DELAY_EMULATE), loop_delay(1)
{
#ifdef WITHOUT_MOUSE
    emulate_mouse = false;
    emulate_mouse_up = KEY_UP;
    emulate_mouse_down = KEY_DOWN;
    emulate_mouse_left = KEY_LEFT;
    emulate_mouse_right = KEY_RIGHT;
    emulate_mouse_step = 10;
    emulate_press_left = KEY_NONE;
    emulate_press_right = KEY_NONE;
#endif
}

const Point & LocalEvent::GetMousePressLeft(void) const
{
    return mouse_pl;
}

const Point & LocalEvent::GetMousePressMiddle(void) const
{
    return mouse_pm;
}

const Point & LocalEvent::GetMousePressRight(void) const
{
    return mouse_pr;
}

const Point & LocalEvent::GetMouseReleaseLeft(void) const
{
    return mouse_rl;
}

const Point & LocalEvent::GetMouseReleaseMiddle(void) const
{
    return mouse_rm;
}

const Point & LocalEvent::GetMouseReleaseRight(void) const
{
    return mouse_rr;
}

void LocalEvent::SetTapMode(bool f)
{
    if(f)
	SetModes(TAP_MODE);
    else
    {
	ResetModes(TAP_MODE);
	ResetModes(CLOCK_ON);
	clock.Stop();
    }
}

void LocalEvent::SetTapDelayForRightClickEmulation(u32 d)
{
    clock_delay = d < 200 ? TAP_DELAY_EMULATE : d;
}

void LocalEvent::SetMouseOffsetX(s16 x)
{
    SetModes(MOUSE_OFFSET);
    mouse_st.x = x;
}

void LocalEvent::SetMouseOffsetY(s16 y)
{
    SetModes(MOUSE_OFFSET);
    mouse_st.y = y;
}

void LocalEvent::SetModes(flag_t f)
{
    modes |= f;
}

void LocalEvent::ResetModes(flag_t f)
{
    modes &= ~f;
}

void LocalEvent::SetGlobalFilter(bool f)
{
    f ? SetModes(GLOBAL_FILTER) : ResetModes(GLOBAL_FILTER);
}

const char* KeySymGetName(KeySym sym)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    return SDL_GetKeyName(static_cast<SDL_Keycode>(sym));
#else
    return SDL_GetKeyName(static_cast<SDLKey>(sym));
#endif
}

KeySym GetKeySym(int key)
{
    switch(key)
    {
	default: break;

	case SDLK_RETURN:	return KEY_RETURN;
	case SDLK_LEFT:		return KEY_LEFT;
	case SDLK_RIGHT:	return KEY_RIGHT;
	case SDLK_UP:		return KEY_UP;
	case SDLK_DOWN:		return KEY_DOWN;

	case SDLK_ESCAPE:	return KEY_ESCAPE;
	case SDLK_BACKSPACE:	return KEY_BACKSPACE;
	case SDLK_EXCLAIM:    	return KEY_EXCLAIM;
	case SDLK_QUOTEDBL:    	return KEY_QUOTEDBL;
	case SDLK_HASH:    	return KEY_HASH;
	case SDLK_DOLLAR:    	return KEY_DOLLAR;
	case SDLK_AMPERSAND:    return KEY_AMPERSAND;
	case SDLK_QUOTE:    	return KEY_QUOTE;
	case SDLK_LEFTPAREN:    return KEY_LEFTPAREN;
	case SDLK_RIGHTPAREN:   return KEY_RIGHTPAREN;
	case SDLK_ASTERISK:     return KEY_ASTERISK;
	case SDLK_PLUS:    	return KEY_PLUS;
	case SDLK_COMMA:    	return KEY_COMMA;
	case SDLK_MINUS:    	return KEY_MINUS;
	case SDLK_PERIOD:    	return KEY_PERIOD;
	case SDLK_SLASH:    	return KEY_SLASH;
	case SDLK_COLON:	return KEY_COLON;
	case SDLK_SEMICOLON:	return KEY_SEMICOLON;
	case SDLK_LESS:		return KEY_LESS;
	case SDLK_EQUALS:	return KEY_EQUALS;
	case SDLK_GREATER:	return KEY_GREATER;
	case SDLK_QUESTION:	return KEY_QUESTION;
	case SDLK_AT:		return KEY_AT;
	case SDLK_LEFTBRACKET:	return KEY_LEFTBRACKET;
	case SDLK_BACKSLASH:	return KEY_BACKSLASH;
	case SDLK_RIGHTBRACKET:	return KEY_RIGHTBRACKET;
	case SDLK_CARET:	return KEY_CARET;
	case SDLK_UNDERSCORE:	return KEY_UNDERSCORE;
	case SDLK_LALT:		return KEY_ALT;
	case SDLK_RALT:		return KEY_ALT;
	case SDLK_LCTRL:	return KEY_CONTROL;
	case SDLK_RCTRL:	return KEY_CONTROL;
	case SDLK_LSHIFT:	return KEY_SHIFT;
	case SDLK_RSHIFT:	return KEY_SHIFT;
	case SDLK_TAB:          return KEY_TAB;
	case SDLK_SPACE:	return KEY_SPACE;
	case SDLK_DELETE:	return KEY_DELETE;
	case SDLK_PAGEUP:	return KEY_PAGEUP;
	case SDLK_PAGEDOWN:	return KEY_PAGEDOWN;
	case SDLK_F1:		return KEY_F1;
	case SDLK_F2:		return KEY_F2;
	case SDLK_F3:		return KEY_F3;
	case SDLK_F4:		return KEY_F4;
	case SDLK_F5:		return KEY_F5;
	case SDLK_F6:		return KEY_F6;
	case SDLK_F7:		return KEY_F7;
	case SDLK_F8:		return KEY_F8;
	case SDLK_F9:		return KEY_F9;
	case SDLK_F10:		return KEY_F10;
	case SDLK_F11:		return KEY_F11;
	case SDLK_F12:		return KEY_F12;
	case SDLK_0:		return KEY_0;
	case SDLK_1:		return KEY_1;
	case SDLK_2:		return KEY_2;
	case SDLK_3:		return KEY_3;
	case SDLK_4:		return KEY_4;
	case SDLK_5:		return KEY_5;
	case SDLK_6:		return KEY_6;
	case SDLK_7:		return KEY_7;
	case SDLK_8:		return KEY_8;
	case SDLK_9:		return KEY_9;
	case SDLK_a:		return KEY_a;
	case SDLK_b:		return KEY_b;
	case SDLK_c:		return KEY_c;
	case SDLK_d:		return KEY_d;
	case SDLK_e:		return KEY_e;
	case SDLK_f:		return KEY_f;
	case SDLK_g:		return KEY_g;
	case SDLK_h:		return KEY_h;
	case SDLK_i:		return KEY_i;
	case SDLK_j:		return KEY_j;
	case SDLK_k:		return KEY_k;
	case SDLK_l:		return KEY_l;
	case SDLK_m:		return KEY_m;
	case SDLK_n:		return KEY_n;
	case SDLK_o:		return KEY_o;
	case SDLK_p:		return KEY_p;
	case SDLK_q:		return KEY_q;
	case SDLK_r:		return KEY_r;
	case SDLK_s:		return KEY_s;
	case SDLK_t:		return KEY_t;
	case SDLK_u:		return KEY_u;
	case SDLK_v:		return KEY_v;
	case SDLK_w:		return KEY_w;
	case SDLK_x:		return KEY_x;
	case SDLK_y:		return KEY_y;
	case SDLK_z:		return KEY_z;

#if SDL_VERSION_ATLEAST(2, 0, 0)
	case SDLK_PRINTSCREEN:	return KEY_PRINT;
	case SDLK_KP_0:		return KEY_KP0;
	case SDLK_KP_1:		return KEY_KP1;
	case SDLK_KP_2:		return KEY_KP2;
	case SDLK_KP_3:		return KEY_KP3;
	case SDLK_KP_4:		return KEY_KP4;
	case SDLK_KP_5:		return KEY_KP5;
	case SDLK_KP_6:		return KEY_KP6;
	case SDLK_KP_7:		return KEY_KP7;
	case SDLK_KP_8:		return KEY_KP8;
	case SDLK_KP_9:		return KEY_KP9;
#else
	case SDLK_PRINT:	return KEY_PRINT;
	case SDLK_KP0:		return KEY_KP0;
	case SDLK_KP1:		return KEY_KP1;
	case SDLK_KP2:		return KEY_KP2;
	case SDLK_KP3:		return KEY_KP3;
	case SDLK_KP4:		return KEY_KP4;
	case SDLK_KP5:		return KEY_KP5;
	case SDLK_KP6:		return KEY_KP6;
	case SDLK_KP7:		return KEY_KP7;
	case SDLK_KP8:		return KEY_KP8;
	case SDLK_KP9:		return KEY_KP9;
#endif

	case SDLK_KP_PERIOD:	return KEY_KP_PERIOD;
	case SDLK_KP_DIVIDE:	return KEY_KP_DIVIDE;
	case SDLK_KP_MULTIPLY:	return KEY_KP_MULTIPLY;
	case SDLK_KP_MINUS:	return KEY_KP_MINUS;
	case SDLK_KP_PLUS:	return KEY_KP_PLUS;
	case SDLK_KP_ENTER:	return KEY_KP_ENTER;
	case SDLK_KP_EQUALS:	return KEY_KP_EQUALS;

#ifdef _WIN32_WCE
	case 0xC1: return KEY_APP01;
	case 0xC2: return KEY_APP02;
	case 0xC3: return KEY_APP03;
	case 0xC4: return KEY_APP04;
	case 0xC5: return KEY_APP05;
	case 0xC6: return KEY_APP06;
	case 0xC7: return KEY_APP07;
	case 0xC8: return KEY_APP08;
	case 0xC9: return KEY_APP09;
	case 0xCA: return KEY_APP10;
	case 0xCB: return KEY_APP11;
	case 0xCC: return KEY_APP12;
	case 0xCD: return KEY_APP13;
	case 0xCE: return KEY_APP14;
	case 0xCF: return KEY_APP15;
#endif
    }

    return KEY_NONE;
}

LocalEvent & LocalEvent::Get(void)
{
    static LocalEvent le;

    return le;
}

bool LocalEvent::HandleEvents(bool delay)
{
    SDL_Event event;

    ResetModes(MOUSE_MOTION);
    ResetModes(KEY_PRESSED);

    while(SDL_PollEvent(&event))
    {
	switch(event.type)
	{
#if SDL_VERSION_ATLEAST(2, 0, 0)
            case SDL_WINDOWEVENT:
                if(Mixer::isValid())
                {
                    if(event.window.event == SDL_WINDOWEVENT_HIDDEN)
                    {
                        Mixer::Pause();
                        Music::Pause();
                        loop_delay = 100;
                    }
                    else
                    if(event.window.event == SDL_WINDOWEVENT_SHOWN)
                    {
                        Mixer::Resume();
                        Music::Resume();
                        loop_delay = 1;
                    }
                }
                break;
#else
	    case SDL_ACTIVEEVENT:
		if(event.active.state & SDL_APPACTIVE)
		{
		    if(Mixer::isValid())
		    {
			//iconify
			if(0 == event.active.gain)
			{
			    Mixer::Pause();
			    Music::Pause();
			    loop_delay = 100;
			}
			else
			{
			    Mixer::Resume();
			    Music::Resume();
			    loop_delay = 1;
			}
		    }
		}
		break;
#endif
	    // keyboard
	    case SDL_KEYDOWN:
	    case SDL_KEYUP:
                HandleKeyboardEvent(event.key);
	    	break;

	    // mouse motion
	    case SDL_MOUSEMOTION:
		HandleMouseMotionEvent(event.motion);
		break;

	    // mouse button
	    case SDL_MOUSEBUTTONDOWN:
	    case SDL_MOUSEBUTTONUP:
		HandleMouseButtonEvent(event.button);
		break;
	
	    // exit
	    case SDL_QUIT:
		Error::Except(__FUNCTION__, "SDL_QUIT");
		return false;

	    default:
		break;
	}

        // need for wheel up/down delay
#if SDL_VERSION_ATLEAST(2, 0, 0)
        if(SDL_MOUSEWHEEL == event.type) break;
#else
        if(SDL_BUTTON_WHEELDOWN == event.button.button || SDL_BUTTON_WHEELUP == event.button.button) break;
#endif
    }

    // emulate press right
    if((modes & TAP_MODE) && (modes & CLOCK_ON))
    {
	clock.Stop();
	if(clock_delay < clock.Get())
	{
	    ResetModes(CLICK_LEFT);
	    ResetModes(CLOCK_ON);
	    mouse_pr = mouse_cu;
	    SetModes(MOUSE_PRESSED);
	    mouse_button = SDL_BUTTON_RIGHT;
	}
    }

    if(delay) SDL_Delay(loop_delay);

    return true;
}

bool LocalEvent::MouseMotion(void) const
{
    return modes & MOUSE_MOTION;
}

bool LocalEvent::MouseMotion(const Rect &rt) const
{
    return modes & MOUSE_MOTION ? rt & mouse_cu : false;
}

bool LocalEvent::MousePressLeft(void) const
{
    return (modes & MOUSE_PRESSED) && SDL_BUTTON_LEFT == mouse_button;
}

bool LocalEvent::MouseReleaseLeft(void) const
{
    return !(modes & MOUSE_PRESSED) && SDL_BUTTON_LEFT == mouse_button;
}

bool LocalEvent::MousePressMiddle(void) const
{
    return (modes & MOUSE_PRESSED) && SDL_BUTTON_MIDDLE  == mouse_button;
}

bool LocalEvent::MouseReleaseMiddle(void) const
{
    return !(modes & MOUSE_PRESSED) && SDL_BUTTON_MIDDLE  == mouse_button;
}

bool LocalEvent::MousePressRight(void) const
{
    return (modes & MOUSE_PRESSED) && SDL_BUTTON_RIGHT == mouse_button;
}

bool LocalEvent::MouseReleaseRight(void) const
{
    return !(modes & MOUSE_PRESSED) && SDL_BUTTON_RIGHT == mouse_button;
}

void LocalEvent::HandleKeyboardEvent(SDL_KeyboardEvent & event)
{
    if(KEY_NONE != GetKeySym(event.keysym.sym))
    {
	(event.type == SDL_KEYDOWN) ? SetModes(KEY_PRESSED) : ResetModes(KEY_PRESSED);

#ifdef WITHOUT_MOUSE
	if(emulate_mouse && EmulateMouseAction(GetKeySym(event.keysym.sym))) return;
#endif

	key_value = GetKeySym(event.keysym.sym);
    }
}

void LocalEvent::HandleMouseMotionEvent(const SDL_MouseMotionEvent & motion)
{
    mouse_state = motion.state;
    SetModes(MOUSE_MOTION);
    mouse_cu.x = motion.x;
    mouse_cu.y = motion.y;
    if(modes & MOUSE_OFFSET) mouse_cu += mouse_st;
}

void LocalEvent::HandleMouseButtonEvent(const SDL_MouseButtonEvent & button)
{
    button.state == SDL_PRESSED ? SetModes(MOUSE_PRESSED) : ResetModes(MOUSE_PRESSED);
    mouse_button = button.button;

    mouse_cu.x = button.x;
    mouse_cu.y = button.y;
    if(modes & MOUSE_OFFSET) mouse_cu += mouse_st;

    if(modes & MOUSE_PRESSED)
	switch(button.button)
	{
#if SDL_VERSION_ATLEAST(2, 0, 0)
            case SDL_BUTTON_X1:
            case SDL_BUTTON_X2:
#else
	    case SDL_BUTTON_WHEELDOWN:
	    case SDL_BUTTON_WHEELUP:
#endif
		mouse_pm = mouse_cu;
		break;

	    case SDL_BUTTON_LEFT:
		mouse_pl = mouse_cu;
		SetModes(CLICK_LEFT);

		// emulate press right
		if(modes & TAP_MODE){ clock.Start(); SetModes(CLOCK_ON); }
		break;

	    case SDL_BUTTON_MIDDLE:
		mouse_pm = mouse_cu;
		SetModes(CLICK_MIDDLE);
		break;


	    case SDL_BUTTON_RIGHT:
		mouse_pr = mouse_cu;
		SetModes(CLICK_RIGHT);
		break;

	    default:
		break;
	}
    else
	switch(button.button)
	{
#if SDL_VERSION_ATLEAST(2, 0, 0)
            case SDL_BUTTON_X1:
            case SDL_BUTTON_X2:
#else
	    case SDL_BUTTON_WHEELDOWN:
	    case SDL_BUTTON_WHEELUP:
#endif
		mouse_rm = mouse_cu;
		break;

	    case SDL_BUTTON_LEFT:
		mouse_rl = mouse_cu;

		// emulate press right
		if(modes & TAP_MODE){ ResetModes(CLOCK_ON); }
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

bool LocalEvent::MouseClickLeft(void)
{
    if(MouseReleaseLeft() && (CLICK_LEFT & modes))
    {
	ResetModes(CLICK_LEFT);
	return true;
    }

    return false;
}

bool LocalEvent::MouseClickLeft(const Rect &rt)
{
    //if(MouseReleaseLeft() && (rt & mouse_rl) && (CLICK_LEFT & modes) && ((modes & TAP_MODE) || (rt & mouse_pl)))
    if(MouseReleaseLeft() && (rt & mouse_pl) && (rt & mouse_rl) && (CLICK_LEFT & modes))
    {
	ResetModes(CLICK_LEFT);
	return true;
    }

    return false;
}

bool LocalEvent::MouseClickMiddle(void)
{
    if(MouseReleaseMiddle() && (CLICK_MIDDLE & modes))
    {
	ResetModes(CLICK_MIDDLE);
	return true;
    }

    return false;
}

bool LocalEvent::MouseClickMiddle(const Rect &rt)
{
    if(MouseReleaseMiddle() && (rt & mouse_pm) && (rt & mouse_rm) && (CLICK_MIDDLE & modes))
    {
	ResetModes(CLICK_MIDDLE);
	return true;
    }

    return false;
}

bool LocalEvent::MouseClickRight(void)
{
    if(MouseReleaseRight() && (CLICK_RIGHT & modes))
    {
	ResetModes(CLICK_RIGHT);
	return true;
    }

    return false;
}

bool LocalEvent::MouseClickRight(const Rect &rt)
{
    if(MouseReleaseRight() && (rt & mouse_pr) && (rt & mouse_rr) && (CLICK_RIGHT & modes))
    {
	ResetModes(CLICK_RIGHT);
	return true;
    }

    return false;
}

bool LocalEvent::MouseWheelUp(void) const
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    return (modes & MOUSE_PRESSED) && SDL_BUTTON_X1 == mouse_button;
#else
    return (modes & MOUSE_PRESSED) && SDL_BUTTON_WHEELUP == mouse_button;
#endif
}

bool LocalEvent::MouseWheelDn(void) const
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    return (modes & MOUSE_PRESSED) && SDL_BUTTON_X2 == mouse_button;
#else
    return (modes & MOUSE_PRESSED) && SDL_BUTTON_WHEELDOWN == mouse_button;
#endif
}

bool LocalEvent::MousePressLeft(const Rect &rt) const
{
    return MousePressLeft() && (rt & mouse_pl);
}

bool LocalEvent::MousePressLeft(const Point &pt, u32 w, u32 h) const
{
    return MousePressLeft() && (Rect(pt.x, pt.y, w, h) & mouse_pl);
}

bool LocalEvent::MousePressMiddle(const Rect &rt) const
{
    return MousePressMiddle() && (rt & mouse_pm);
}

bool LocalEvent::MousePressRight(const Rect &rt) const
{
    return MousePressRight() && (rt & mouse_pr);
}

bool LocalEvent::MouseReleaseLeft(const Rect &rt) const
{
    return MouseReleaseLeft() && (rt & mouse_rl);
}

bool LocalEvent::MouseReleaseMiddle(const Rect &rt) const
{
    return MouseReleaseMiddle() && (rt & mouse_rm);
}

bool LocalEvent::MouseReleaseRight(const Rect &rt) const
{
    return MouseReleaseRight() && (rt & mouse_rr);
}

void LocalEvent::ResetPressLeft(void)
{
    mouse_pl.x = -1;
    mouse_pl.y = -1;
}

void LocalEvent::ResetPressRight(void)
{
    mouse_pr.x = -1;
    mouse_pr.y = -1;
}

void LocalEvent::ResetPressMiddle(void)
{
    mouse_pm.x = -1;
    mouse_pm.y = -1;
}

void LocalEvent::ResetReleaseLeft(void)
{
    mouse_rl.x = -1;
    mouse_rl.y = -1;
}

void LocalEvent::ResetReleaseRight(void)
{
    mouse_rr.x = -1;
    mouse_rr.y = -1;
}

void LocalEvent::ResetReleaseMiddle(void)
{
    mouse_rm.x = -1;
    mouse_rm.y = -1;
}

bool LocalEvent::MouseWheelUp(const Rect &rt) const
{
    return MouseWheelUp() && (rt & mouse_cu);
}

bool LocalEvent::MouseWheelDn(const Rect &rt) const
{
    return MouseWheelDn() && (rt & mouse_cu);
}

bool LocalEvent::MouseCursor(const Rect &rt) const
{
    return rt & mouse_cu;
}

const Point & LocalEvent::GetMouseCursor(void)
{
#ifdef WITHOUT_MOUSE
    if(!emulate_mouse)
#endif
    {
	int x, y;

	SDL_PumpEvents();
	SDL_GetMouseState(&x, &y);

	mouse_cu.x = x;
	mouse_cu.y = y;
    }

    if(modes & MOUSE_OFFSET) mouse_cu += mouse_st;

    return mouse_cu;
}

int LocalEvent::KeyMod(void) const
{
    return SDL_GetModState();
}

KeySym LocalEvent::KeyValue(void) const
{
    return key_value;
}

bool LocalEvent::KeyPress(void) const
{
    return modes & KEY_PRESSED;
}

bool LocalEvent::KeyPress(KeySym key) const
{
    return key == key_value && (modes & KEY_PRESSED);
}

void LocalEvent::SetGlobalFilterMouseEvents(void (*pf)(s32, s32))
{
    redraw_cursor_func = pf;
}

void LocalEvent::SetGlobalFilterKeysEvents(void (*pf)(int, int))
{
    keyboard_filter_func = pf;
}

#if SDL_VERSION_ATLEAST(2, 0, 0)
int LocalEvent::GlobalFilterEvents(void* userdata, SDL_Event* event)
#else
int LocalEvent::GlobalFilterEvents(const SDL_Event *event)
#endif
{
    LocalEvent & le = LocalEvent::Get();

    // motion
    if((le.modes & GLOBAL_FILTER) && SDL_MOUSEMOTION == event->type)
    {
        // redraw cursor
        if(le.redraw_cursor_func)
	{
	    if(le.modes & MOUSE_OFFSET)
    		(*(le.redraw_cursor_func))(event->motion.x + le.mouse_st.x, event->motion.y + le.mouse_st.y);
    	    else
		(*(le.redraw_cursor_func))(event->motion.x, event->motion.y);
	}
    }

    // key
    if((le.modes & GLOBAL_FILTER) && SDL_KEYDOWN == event->type)

    {
        // key event
        if(le.keyboard_filter_func)
    	    (*(le.keyboard_filter_func))(event->key.keysym.sym, event->key.keysym.mod);
    }

    return 1;
}

void LocalEvent::SetState(u32 type, bool enable)
{
    SDL_EventState(type, enable ? SDL_ENABLE : SDL_IGNORE);
}

int LocalEvent::GetState(u32 type)
{
    return SDL_EventState(type, SDL_QUERY);
}
         
void LocalEvent::SetStateDefaults(void)
{
    SetState(SDL_USEREVENT, true);
    SetState(SDL_KEYDOWN, true);
    SetState(SDL_KEYUP, true);
    SetState(SDL_MOUSEMOTION, true);
    SetState(SDL_MOUSEBUTTONDOWN, true);
    SetState(SDL_MOUSEBUTTONUP, true);
    SetState(SDL_QUIT, true);

    SetState(SDL_JOYAXISMOTION, false);
    SetState(SDL_JOYBALLMOTION, false);
    SetState(SDL_JOYHATMOTION, false);
    SetState(SDL_JOYBUTTONUP, false);
    SetState(SDL_JOYBUTTONDOWN, false);
    SetState(SDL_SYSWMEVENT, false);

#if SDL_VERSION_ATLEAST(2, 0, 0)
    SetState(SDL_WINDOWEVENT, true);

    SDL_SetEventFilter(GlobalFilterEvents, NULL);
#else
    SetState(SDL_ACTIVEEVENT, true);

    SetState(SDL_SYSWMEVENT, false);
    SetState(SDL_VIDEORESIZE, false);
    SetState(SDL_VIDEOEXPOSE, false);

    SDL_SetEventFilter(GlobalFilterEvents);
#endif
}

#ifdef WITHOUT_MOUSE
void LocalEvent::ToggleEmulateMouse(void)
{
    emulate_mouse = emulate_mouse ? false : true;
}

void LocalEvent::SetEmulateMouse(bool f)
{
    emulate_mouse = f;
    if(f) mouse_cu = Point(0, 0);
}

void LocalEvent::SetEmulateMouseUpKey(KeySym k)
{
    emulate_mouse_up = k;
}

void LocalEvent::SetEmulateMouseDownKey(KeySym k)
{
    emulate_mouse_down = k;
}

void LocalEvent::SetEmulateMouseLeftKey(KeySym k)
{
    emulate_mouse_left = k;
}

void LocalEvent::SetEmulateMouseRightKey(KeySym k)
{
    emulate_mouse_right = k;
}
                                
void LocalEvent::SetEmulateMouseStep(u8 s)
{
    emulate_mouse_step = s;
}

void LocalEvent::SetEmulatePressLeftKey(KeySym k)
{
    emulate_press_left = k;
}

void LocalEvent::SetEmulatePressRightKey(KeySym k)
{
    emulate_press_right = k;
}

bool LocalEvent::EmulateMouseAction(KeySym key)
{
    if((key == emulate_mouse_up ||
	key == emulate_mouse_down ||
	key == emulate_mouse_left ||
	key == emulate_mouse_right ||
	key == emulate_press_left ||
	key == emulate_press_right))
    {
	if(emulate_mouse_up == key)
	{
	    mouse_cu.y -= emulate_mouse_step;
	    SetModes(MOUSE_MOTION);
	}
	else
	if(emulate_mouse_down == key)
	{
	    mouse_cu.y += emulate_mouse_step;
	    SetModes(MOUSE_MOTION);
	}
	else
	if(emulate_mouse_left == key)
	{
	    mouse_cu.x -= emulate_mouse_step;
	    SetModes(MOUSE_MOTION);
	}
	else
	if(emulate_mouse_right == key)
	{
	    mouse_cu.x += emulate_mouse_step;
	    SetModes(MOUSE_MOTION);
	}

	if(mouse_cu.x < 0) mouse_cu.x = 0;
	if(mouse_cu.y < 0) mouse_cu.y = 0;
	if(mouse_cu.x > Display::Get().w()) mouse_cu.x = Display::Get().w();
	if(mouse_cu.y > Display::Get().h()) mouse_cu.y = Display::Get().h();

	if(emulate_press_left == key)
	{
	    if(modes & KEY_PRESSED)
	    {
		mouse_pl = mouse_cu;
		SetModes(MOUSE_PRESSED);
		SetModes(CLICK_LEFT);
	    }
	    else
	    {
		mouse_rl = mouse_cu;
		ResetModes(MOUSE_PRESSED);
	    }
	    mouse_button = SDL_BUTTON_LEFT;
	}
	else
	if(emulate_press_right == key)
	{
	    if(modes & KEY_PRESSED)
	    {
		mouse_pr = mouse_cu;
		SetModes(MOUSE_PRESSED);
	    }
	    else
	    {
		mouse_rr = mouse_cu;
		ResetModes(MOUSE_PRESSED);
	    }
	    mouse_button = SDL_BUTTON_RIGHT;
	}

    	if((modes & MOUSE_MOTION) && redraw_cursor_func)
	{
	    if(modes & MOUSE_OFFSET)
    		(*(redraw_cursor_func))(mouse_cu.x + mouse_st.x, mouse_cu.y + mouse_st.y);
    	    else
		(*(redraw_cursor_func))(mouse_cu.x, mouse_cu.y);
	}

	ResetModes(KEY_PRESSED);

	return true;
    }

    return false;
}

#endif
