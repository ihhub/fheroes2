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
#ifndef H2DISPLAY_H
#define H2DISPLAY_H

#include <string>
#include "surface.h"
class Texture;

class Display : public Surface
{
public:
    ~Display();

    static Display &    Get(void);

    Size        GetSize(void) const;

    std::string	GetInfo(void) const;
    Size	GetMaxMode(bool enable_rotate) const;

    void	SetVideoMode(int w, int h, bool);
    void	SetCaption(const char*);
    void	SetIcons(Surface &);

    void	Flip(void);
    void	Present(void);
    void        Clear(void);
    void        ToggleFullScreen(void);

    void	Fade(int delay = 500);
    void	Fade(const Surface &, const Surface &, const Point &, int level, int delay);
    void	Rise(int delay = 500);
    void	Rise(const Surface &, const Surface &, const Point &, int level, int delay);

    static void HideCursor(void);
    static void ShowCursor(void);

    Surface	GetSurface(void) const;
    Surface	GetSurface(const Rect & rt) const;

protected:
    friend class Texture;

    bool	isDisplay(void) const;

    Display();

#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_Window*         window;
    SDL_Renderer*	renderer;
#endif
};

#if SDL_VERSION_ATLEAST(2, 0, 0)
class TextureTarget
{
public:
    TextureTarget();

    void Fill(const RGBA &);
    void FillRect(const Rect &, const RGBA &);
};

class Texture
{
public:
    Texture();
    Texture(const Surface &);
    Texture(const Texture &);
    ~Texture();

    Texture &	operator= (const Texture &);
    Size	GetSize(void) const;

    void	Blit(Display &) const;
    void        Blit(s32 dx, s32 dy, Display &) const;
    void        Blit(const Point & dstpt, Display &) const;
    void        Blit(const Rect & srcrt, s32 dx, s32 dy, Display &) const;
    void        Blit(const Rect & srcrt, const Point & dstpt, Display &) const;

protected:
    SDL_Texture*        texture;
    int*		counter;
};
#else
class Texture : public Surface
{
public:
    Texture(const Surface &);
};
#endif

#endif
