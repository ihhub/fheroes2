/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2019 - 2022                                             *
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

#include <list>
#include <string>

#include "screen.h"
#include "types.h"

namespace Font
{
    enum
    {
        SMALL = 0x01,
        BIG = 0x02,
        YELLOW_BIG = 0x04,
        YELLOW_SMALL = 0x08,
        GRAY_BIG = 0x10,
        GRAY_SMALL = 0x20,
        WHITE_LARGE = 0x40
    };
}
enum
{
    ALIGN_NONE,
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
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

    void Clear( void );
    size_t Size( void ) const;

    int w( void ) const
    {
        return static_cast<int>( gw );
    }
    int h( void ) const
    {
        return static_cast<int>( gh );
    }

    void Blit( s32, s32, fheroes2::Image & sf = fheroes2::Display::instance() ) const;
    void Blit( s32, s32, int maxw, fheroes2::Image & sf = fheroes2::Display::instance() ) const;
    void Blit( const fheroes2::Point &, fheroes2::Image & sf = fheroes2::Display::instance() ) const;

    static int32_t getCharacterWidth( const uint8_t character, const int fontType );

    // Use this method when you need to find the maximum width of of a string to be fit within given width
    static int32_t getFitWidth( const std::string & text, const int fontId, const int32_t width_ );

protected:
    TextAscii * message;
    u32 gw;
    u32 gh;
};

class TextSprite : protected Text
{
public:
    TextSprite();
    TextSprite( const std::string &, int ft, s32, s32 );

    void SetPos( s32, s32 );
    void SetText( const std::string & );
    void SetText( const std::string &, int );
    void SetFont( int );

    void Show( void );
    void Hide( void );

    bool isShow( void ) const;

    int w() const;
    int h() const;

    fheroes2::Rect GetRect( void ) const;

private:
    fheroes2::ImageRestorer _restorer;
    bool hide;
};

class TextBox : protected fheroes2::Rect
{
public:
    TextBox() = delete;
    TextBox( const std::string &, int, uint32_t width_ );
    TextBox( const std::string &, int, const fheroes2::Rect & );

    void Set( const std::string &, int, uint32_t width_ );
    void SetAlign( int type );

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

    void Blit( s32, s32, fheroes2::Image & sf = fheroes2::Display::instance() );

private:
    void Append( const std::string &, int, u32 );

    std::list<Text> messages;
    int align;
};

#endif
