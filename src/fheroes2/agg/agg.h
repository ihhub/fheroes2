/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2AGG_H
#define H2AGG_H

#include <vector>
#include <utility>

#include "gamedefs.h"
#include "icn.h"
#include "til.h"
#include "m82.h"
#include "mus.h"
#include "xmi.h"
#include "sprite.h"

class ICNSprite : public std::pair<Surface, Surface> /* first: image with out alpha, second: shadow with alpha */
{
public:
    ICNSprite() {}
    ICNSprite(const Surface & sf1, const Surface & sf2) : std::pair<Surface, Surface>(sf1, sf2) {}

    bool   isValid(void) const;
    Sprite CreateSprite(bool reflect, bool shadow) const;
    Surface First(void) { return first; }
    Surface Second(void) { return second; }

    Point      offset;
};

namespace AGG
{	
    bool	Init(void);
    void	Quit(void);

    int		PutICN(const Sprite &, bool init_reflect = false);
    Sprite	GetICN(int icn, u32 index, bool reflect = false);
    u32		GetICNCount(int icn);
    Surface	GetTIL(int til, u32 index, u32 shape);
    Surface	GetLetter(u32 ch, u32 ft);
#ifdef WITH_TTF
    Surface	GetUnicodeLetter(u32 ch, u32 ft);
    u32		GetFontHeight(bool small);
#endif
    void	LoadLOOPXXSounds(const std::vector<int> &);
    void	PlaySound(int m82);
    void	PlayMusic(int mus, bool loop = true);
    void	ResetMixer(void);

    RGBA	GetPaletteColor(u32 index);
    ICNSprite   RenderICNSprite(int, u32);
    void        RenderICNSprite(int icn, u32 index, const Rect & srt, const Point & dpt, Surface & dst);
}

#endif
