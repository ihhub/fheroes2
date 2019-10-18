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

#include <sstream>
#include <string>

#include "tools.h"
#include "types.h"
#include "system.h"
#include "error.h"
#include "display.h"

#if SDL_VERSION_ATLEAST(2, 0, 0)
Display::Display() : window(NULL), renderer(NULL) {}
#else
Display::Display() {}
#endif

Display::~Display()
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if(renderer)
	SDL_DestroyRenderer(renderer);

    if(window)
        SDL_DestroyWindow(window);

    FreeSurface(*this);
#else
#endif
}

void Display::SetVideoMode(int w, int h, bool fullscreen)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    u32 flags = SDL_WINDOW_SHOWN;
    if(fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;
        
    if(renderer)
	SDL_DestroyRenderer(renderer);
        
    if(window)
        SDL_DestroyWindow(window);

    window = SDL_CreateWindow("", 0, 0, w, h, flags);
    renderer = SDL_CreateRenderer(window, -1, System::GetRenderFlags());

    if(! renderer)
        Error::Except(__FUNCTION__, SDL_GetError());

    Set(w, h, false);
    Fill(RGBA(0, 0, 0));
#else
    u32 flags = System::GetRenderFlags();

    if(fullscreen)
	flags |= SDL_FULLSCREEN;

    surface = SDL_SetVideoMode(w, h, 0, flags);

    if(! surface)
	Error::Except(__FUNCTION__, SDL_GetError());
#endif
}

Size Display::GetSize(void) const
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if(window)
    {
        int dw, dh;
        SDL_GetWindowSize(window, &dw, &dh);
        return Size(dw, dh);
    }

    return Size(0, 0);
#else
    return Size(w(), h());
#endif
}

void Display::Flip(void)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer, surface);

    if(tx)
    {
	if(0 != SDL_SetRenderTarget(renderer, NULL))
	{
    	    ERROR(SDL_GetError());
	}
	else
	{
	    if(0 != SDL_RenderCopy(renderer, tx, NULL, NULL))
	    {
    		ERROR(SDL_GetError());
	    }
	    else
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyTexture(tx);
    }
    else
    	ERROR(SDL_GetError());
#else
    SDL_Flip(surface);
#endif
}

void Display::Present(void)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_RenderPresent(renderer);
#else
    SDL_Flip(surface);
#endif
}

void Display::ToggleFullScreen(void)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if(window)
    {
        u32 flags = SDL_GetWindowFlags(window);

        // toggle FullScreen
        if(flags & SDL_WINDOW_FULLSCREEN)
            flags &= ~SDL_WINDOW_FULLSCREEN;

        SDL_SetWindowFullscreen(window, flags);
    }
#else
    SDL_WM_ToggleFullScreen(surface);
#endif
}

void Display::SetCaption(const char* str)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if(window)
        SDL_SetWindowTitle(window, str);
#else
    SDL_WM_SetCaption(str, NULL);
#endif
}

void Display::SetIcons(Surface & icons)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_SetWindowIcon(window, icons());
#else
    SDL_WM_SetIcon(icons(), NULL);
#endif
}

Size Display::GetMaxMode(bool rotate) const
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    int disp = 0;
    int num = SDL_GetNumDisplayModes(disp);
    SDL_DisplayMode mode;
    int max = 0;
    int cur = 0;

    for(int ii = 0; ii < num; ++ii)
    {
        SDL_GetDisplayMode(disp, ii, &mode);

        if(max < mode.w * mode.h)
        {
            max = mode.w * mode.h;
            cur = ii;
        }
    }
    
    SDL_GetDisplayMode(disp, cur, &mode);
    Size result = Size(mode.w, mode.h);

    if(rotate && result.w < result.h)
        std::swap(result.w, result.h);

    return result;
#else
    Size result;
    SDL_Rect** modes = SDL_ListModes(NULL, SDL_ANYFORMAT);

    if(modes == (SDL_Rect **) 0 ||
	modes == (SDL_Rect **) -1)
    {
        ERROR("GetMaxMode: " << "no modes available");
    }
    else
    {
	int max = 0;
	int cur = 0;

	for(int ii = 0; modes[ii]; ++ii)
	{
	    if(max < modes[ii]->w * modes[ii]->h)
	    {
		max = modes[ii]->w * modes[ii]->h;
		cur = ii;
	    }
	}

	result.w = modes[cur]->w;
	result.h = modes[cur]->h;

	if(rotate && result.w < result.h)
	    std::swap(result.w, result.h);
    }

    return result;
#endif
}

std::string Display::GetInfo(void) const
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    std::ostringstream os;
    os << "Display::GetInfo: " <<
            GetString(GetSize()) << ", " <<
            "driver: " << SDL_GetCurrentVideoDriver();
    return os.str();
#else
    std::ostringstream os;
    char namebuf[12];

    os << "Display::" << "GetInfo: " <<
	GetString(GetSize()) << ", " <<
	"driver: " << SDL_VideoDriverName(namebuf, 12);

    return os.str();
#endif
}

Surface Display::GetSurface(const Rect & rt) const
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
/*
    SDL_Rect srcrect = SDLRect(rt);
    SDL_Surface *sf = SDL_CreateRGBSurface(0, rt.w, rt.h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    Surface res;


    if(! sf)
        ERROR(SDL_GetError());
    else
    {
        if(0 != SDL_RenderReadPixels(renderer, &srcrect, SDL_PIXELFORMAT_ARGB8888, sf->pixels, sf->pitch))
            ERROR(SDL_GetError());

        res.Set(sf);
    }
*/
    return Surface::GetSurface(rt);
#else
    Surface res(rt, GetFormat());
    Blit(rt, Point(0, 0), res);
    return res; //Surface(SDL_DisplayFormat(res()));
#endif
}

void Display::Clear(void)
{
    Fill(ColorBlack);
}

/* hide system cursor */
void Display::HideCursor(void)
{
    SDL_ShowCursor(SDL_DISABLE);
}

/* show system cursor */
void Display::ShowCursor(void)
{
    SDL_ShowCursor(SDL_ENABLE);
}

void Display::Fade(const Surface & top, const Surface & back, const Point & pt, int level, int delay)
{
    Surface shadow = top.GetSurface();
    int alpha = 255;
    const int step = 10;
    const int min = step + 5;
    const int delay2 = (delay * step) / (alpha - min);

    while(alpha > min + level)
    {
	back.Blit(*this);
	shadow.SetAlphaMod(alpha);
	shadow.Blit(*this);
	Flip();
	alpha -= step;
	DELAY(delay2);
    }
}

void Display::Fade(int delay)
{
    Surface top = GetSurface();
    Surface back(GetSize(), false);
    back.Fill(ColorBlack);
    Fade(top, back, Point(0, 0), 5, delay);
    Blit(back);
    Flip();
}

void Display::Rise(const Surface & top, const Surface & back, const Point & pt, int level, int delay)
{
    Surface shadow = top.GetSurface();
    int alpha = 0;
    const int step = 10;
    const int max = level - step;
    const int delay2 = (delay * step) / max;

    while(alpha < max)
    {
	back.Blit(*this);
	shadow.SetAlphaMod(alpha);
	shadow.Blit(*this);
        Flip();
	alpha += step;
	DELAY(delay2);
    }
}

void Display::Rise(int delay)
{
    Surface top = GetSurface();
    Surface back(GetSize(), false);
    back.Fill(ColorBlack);
    Rise(top, back, Point(0, 0), 250, delay);
    Blit(top);
    Flip();
}

/* get video display */
Display & Display::Get(void)
{
    static Display inside;
    return inside;
}


bool Display::isDisplay(void) const
{
    return true;
}

Surface Display::GetSurface(void) const
{
    return GetSurface(Rect(Point(0, 0), GetSize()));
}

#if SDL_VERSION_ATLEAST(2, 0, 0)
Texture::Texture() : texture(NULL), counter(NULL)
{
    counter = new int;
    *counter = 0;
}

Texture::Texture(const Surface & sf) : texture(NULL), counter(NULL)
{
    Display & display = Display::Get();
    texture = SDL_CreateTextureFromSurface(display.renderer, sf());

    if(!texture)
	ERROR(SDL_GetError());

    counter = new int;
    *counter = 1;
}

Texture::~Texture()
{
    if(1 < *counter)
	*counter -= 1;
    else
    {
	SDL_DestroyTexture(texture);
	delete counter;
    }
}

Texture::Texture(const Texture & tx) : texture(tx.texture), counter(tx.counter)
{
    if(texture)
	*counter += 1;
    else
    {
	counter = new int;
	*counter = 0;
    }
}

Texture & Texture::operator= (const Texture & tx)
{
    if(this == &tx)
	return *this;

    if(1 < *counter)
	*counter -= 1;
    else
    {
	SDL_DestroyTexture(texture);
	delete counter;
    }

    texture = tx.texture;
    counter = tx.counter;

    if(texture)
	*counter += 1;
    else
    {
	counter = new int;
	*counter = 0;
    }

    return *this;
}

Size Texture::GetSize(void) const
{
    int tw, th;
    SDL_QueryTexture(texture, NULL, NULL, &tw, &th);
    return Size(tw, th);
}

void Texture::Blit(Display & display) const
{
    Blit(Rect(Point(0, 0), GetSize()), Point(0, 0), display);
}

void Texture::Blit(s32 dx, s32 dy, Display & display) const
{
    Blit(Rect(Point(0, 0), GetSize()), Point(dx, dy), display);
}

void Texture::Blit(const Point & dstpt, Display & display) const
{
    Blit(Rect(Point(0, 0), GetSize()), dstpt, display);
}

void Texture::Blit(const Rect & srcrt, s32 dx, s32 dy, Display & display) const
{
    Blit(srcrt, Point(dx, dy), display);
}

void Texture::Blit(const Rect & srt, const Point & dpt, Display & display) const
{
    SDL_Rect srcrt = SDLRect(srt);
    SDL_Rect dstrt = SDLRect(dpt.x, dpt.y, srt.w, srt.h);

    if(0 != SDL_SetRenderTarget(display.renderer, NULL))
    {
    	ERROR(SDL_GetError());
    }
    else
    {
	if(0 != SDL_RenderCopy(display.renderer, texture, &srcrt, &dstrt))
    	    ERROR(SDL_GetError());
    }
}

#else
Texture::Texture(const Surface & sf)
{
    Set(SDL_DisplayFormatAlpha(sf()));
    //Set(SDL_DisplayFormat(sf()));
}
#endif
