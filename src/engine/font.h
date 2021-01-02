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

#ifndef H2FONT_H
#define H2FONT_H

#include <string>
#include <vector>

#include "image.h"
#include "types.h"

#ifdef WITH_TTF
#include <SDL_ttf.h>

class RGBA;

class FontTTF
{
public:
    FontTTF();
    ~FontTTF();

    TTF_Font * operator()( void ) const
    {
        return ptr;
    }

    static void Init( void );
    static void Quit( void );

    bool Open( const std::string &, int size );
    bool isValid( void ) const;
    void SetStyle( int );

    int Height( void ) const;
    int Ascent( void ) const;
    int Descent( void ) const;
    int LineSkip( void ) const;

    // Surface RenderText( const std::string &, const RGBA &, bool solid /* or blended */ );
    // Surface RenderChar( char, const RGBA &, bool solid /* or blended */ );
    // Surface RenderUnicodeText( const std::vector<u16> &, const RGBA &, bool solid /* or blended */ );
    // Surface RenderUnicodeChar( u16, const RGBA &, bool solid /* or blended */ );

protected:
    TTF_Font * ptr;

private:
    FontTTF( const FontTTF & ) {}
    FontTTF & operator=( const FontTTF & )
    {
        return *this;
    }
};

#endif

class FontPSF
{
public:
    FontPSF( const std::string & filePath, const fheroes2::Size & size );

    fheroes2::Image RenderText( const std::string & text, const uint8_t color ) const;

private:
    const std::vector<uint8_t> _data;
    const fheroes2::Size _size;
};
#endif
