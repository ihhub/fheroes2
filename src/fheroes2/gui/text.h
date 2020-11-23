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

#ifndef H2TEXT_H
#define H2TEXT_H

#include <string>
#include <vector>

#include "gamedefs.h"

#include "screen.h"
#include "ui_tool.h"

namespace Font
{
    enum
    {
        SMALL = 0x01,
        BIG = 0x02,
        YELLOW_BIG = 0x04,
        YELLOW_SMALL = 0x08,
        GRAY_BIG = 0x10,
        GRAY_SMALL = 0x20
    };
}
enum
{
    ALIGN_NONE,
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
};

class TextInterface
{
public:
    TextInterface( int ft = Font::BIG );
    virtual ~TextInterface(){};

    virtual void SetText( const std::string & ) = 0;
    virtual void SetFont( int ) = 0;
    virtual void Clear( void ) = 0;

    virtual int w( void ) const = 0;
    virtual int h( void ) const = 0;
    virtual size_t Size( void ) const = 0;

    virtual void Blit( s32, s32, int maxw, fheroes2::Image & sf = fheroes2::Display::instance() ) = 0;

protected:
    int font;
};

class TextAscii : public TextInterface
{
public:
    TextAscii(){};
    TextAscii( const std::string &, int = Font::BIG );

    virtual void SetText( const std::string & ) override;
    virtual void SetFont( int ) override;
    virtual void Clear( void ) override;

    virtual int w( void ) const override;
    virtual int h( void ) const override;
    virtual size_t Size( void ) const override;

    int w( u32, u32 ) const;
    int h( int ) const;

    virtual void Blit( s32, s32, int maxw, fheroes2::Image & sf = fheroes2::Display::instance() ) override;
    static int CharWidth( int, int ft );
    static int CharHeight( int ft );
    static int CharAscent( int ft );
    static int CharDescent( int ft );

private:
    std::string message;
};

#ifdef WITH_TTF
class TextUnicode : public TextInterface
{
public:
    TextUnicode(){};
    TextUnicode( const std::string &, int ft = Font::BIG );
    TextUnicode( const u16 *, size_t, int ft = Font::BIG );

    virtual void SetText( const std::string & ) override;
    virtual void SetFont( int ) override;
    virtual void Clear( void ) override;

    virtual int w( void ) const override;
    virtual int h( void ) const override;
    virtual size_t Size( void ) const override;

    int w( u32, u32 ) const;
    int h( int ) const;

    virtual void Blit( s32, s32, int maxw, fheroes2::Image & sf = fheroes2::Display::instance() ) override;

    static bool isspace( int );
    static int CharWidth( int, int ft );
    static int CharHeight( int ft );

private:
    std::vector<u16> message;
};
#endif

class Text
{
public:
    Text();
    Text( const std::string &, int ft = Font::BIG );
#ifdef WITH_TTF
    Text( const u16 *, size_t, int ft = Font::BIG );
#endif
    Text( const Text & );
    ~Text();

    Text & operator=( const Text & );

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
    void Blit( const Point &, fheroes2::Image & sf = fheroes2::Display::instance() ) const;

    static u32 width( const std::string &, int ft, u32 start = 0, u32 count = 0 );
    static u32 height( const std::string &, int ft, u32 width = 0 );

    // Use this method when you need to find the maximum width of of a string to be fit within given width
    static int32_t getFitWidth( const std::string & text, const int fontId, const int32_t width_ );

protected:
    TextInterface * message;
    u32 gw;
    u32 gh;
};

class TextSprite : protected Text
{
public:
    TextSprite();
    TextSprite( const std::string &, int ft, const Point & pt );
    TextSprite( const std::string &, int ft, s32, s32 );

    void SetPos( s32, s32 );
    void SetText( const std::string & );
    void SetText( const std::string &, int );
    void SetFont( int );

    void Show( void );
    void Hide( void );

    bool isHide( void ) const;
    bool isShow( void ) const;

    int w( void );
    int h( void );

    fheroes2::Rect GetRect( void ) const;

private:
    fheroes2::ImageRestorer _restorer;
    bool hide;
};

class TextBox : protected fheroes2::Rect
{
public:
    TextBox();
    TextBox( const std::string &, int, u32 width );
    TextBox( const std::string &, int, const fheroes2::Rect & );

    void Set( const std::string &, int, u32 width );
    void SetAlign( int type );

    int32_t x() const
    {
        return fheroes2::Rect::x;
    }

    int32_t y() const
    {
        return fheroes2::Rect::y;
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

    void Blit( s32, s32, fheroes2::Image & sf = fheroes2::Display::instance() );
    void Blit( const fheroes2::Point &, fheroes2::Image & sf = fheroes2::Display::instance() );

private:
    void Append( const std::string &, int, u32 );
#ifdef WITH_TTF
    void Append( const std::vector<u16> &, int, u32 );
#endif

    std::list<Text> messages;
    int align;
};

#endif
