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

#ifndef H2TYPES_H
#define H2TYPES_H

#include "SDL.h"

typedef Sint8		s8;
typedef Uint8		u8;
typedef Sint16		s16;
typedef Uint16		u16;
typedef Sint32		s32;
typedef Uint32		u32;

#define MAXU16   		0xFFFF
#define MAXU32   		0xFFFFFFFF

#define ARRAY_COUNT(A)       	sizeof(A) / sizeof(A[0])
#define ARRAY_COUNT_END(A)   	A + ARRAY_COUNT(A)

#define DELAY(X)		SDL_Delay(X)

#if defined __SYMBIAN32__
#define PATH_MAX FILENAME_MAX
namespace std
{
    int		c_abs(int x);
    float	c_abs(float x);
    double	c_abs(double x);
    int		c_isspace(char c);

#define isspace(c) 		c_isspace(c)
#define abs(x) 			c_abs(x)
}
using namespace std;
#endif

#if defined __MINGW32CE__
#undef PATH_MAX
#define PATH_MAX 		255
#endif

#endif
