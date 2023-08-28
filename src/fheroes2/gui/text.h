/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2TEXT_H
#define H2TEXT_H

#include <cstddef>
#include <cstdint>
#include <list>
#include <string>

#include "image.h"
#include "math_base.h"
#include "screen.h"

namespace Font
{
    enum
    {
        SMALL = 0x01,
        BIG = 0x02,
        YELLOW_BIG = 0x04,
        YELLOW_SMALL = 0x08,
        GRAY_SMALL = 0x10
    };
}
enum
{
    ALIGN_NONE,
    ALIGN_LEFT,
    ALIGN_CENTER
};

class TextAscii;

class Text
{
public:
    Text();
    Text( const std::string &, int ft = Font::BIG );

    Text( const Text & ) = delete;

    ~Text();

    Text & operator=( const Text & ) = delete;

    void Set( const std::string &, int );
    void Set( const std::string & );
    void Set( int );

    void Clear();

    size_t Size() const;

    int w() const
    {
        return static_cast<int>( gw );
    }
    int h() const
    {
        return static_cast<int>( gh );
    }

    void Blit( int32_t ax, int32_t ay, fheroes2::Image & dst = fheroes2::Display::instance() ) const;
    void Blit( int32_t ax, int32_t ay, int maxw, fheroes2::Image & dst = fheroes2::Display::instance() ) const;
    void Blit( const fheroes2::Point & dst_pt, fheroes2::Image & dst = fheroes2::Display::instance() ) const;

    // Use this method when you need to find the maximum width of a string to be fit within given width
    static int32_t getFitWidth( const std::string & text, const int fontId, const int32_t width_ );

protected:
    TextAscii * message;
    uint32_t gw;
    uint32_t gh;
};

class TextSprite : protected Text
{
public:
    TextSprite();
    TextSprite( const std::string &, int ft, int32_t, int32_t );

    void SetPos( int32_t, int32_t );
    void SetText( const std::string & );
    void SetText( const std::string &, int );
    void SetFont( int );

    void Show();
    void Hide();

    bool isShow() const
    {
        return !hide;
    }

    int w() const
    {
        return gw;
    }

    int h() const
    {
        return gh + 5;
    }

    fheroes2::Rect GetRect() const;

private:
    fheroes2::ImageRestorer _restorer;
    bool hide;
};

class TextBox : protected fheroes2::Rect
{
public:
    TextBox( const std::string &, int, uint32_t width_ );
    TextBox( const std::string &, int, const fheroes2::Rect & );

    void Set( const std::string &, int, uint32_t width_ );

    void SetAlign( int type )
    {
        align = type;
    }

    int32_t w() const
    {
        return fheroes2::Rect::width;
    }

    int32_t h() const
    {
        return fheroes2::Rect::height;
    }

    size_t row() const
    {
        return messages.size();
    }

    void Blit( int32_t, int32_t, fheroes2::Image & sf = fheroes2::Display::instance() );

private:
    void Append( const std::string &, int, uint32_t );

    std::list<Text> messages;
    int align;
};

#endif
