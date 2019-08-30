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

namespace Font { enum { SMALL = 0x01, BIG = 0x02, YELLOW_BIG = 0x04, YELLOW_SMALL = 0x08 }; }
enum { ALIGN_NONE, ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };

class TextInterface
{
public:
    TextInterface(int ft = Font::BIG);
    virtual ~TextInterface(){};

    virtual void	SetText(const std::string &) = 0;
    virtual void	SetFont(int) = 0;
    virtual void	Clear(void) = 0;

    virtual int		w(void) const = 0;
    virtual int		h(void) const = 0;
    virtual size_t	Size(void) const = 0;

    virtual void	Blit(s32, s32, int maxw, Surface & sf = Display::Get()) = 0;

    int			font;
};


class TextAscii : public TextInterface
{
public:
    TextAscii() {};
    TextAscii(const std::string &, int = Font::BIG);

    void	SetText(const std::string &);
    void	SetFont(int);
    void	Clear(void);

    int		w(void) const;
    int		w(u32, u32) const;
    int		h(void) const;
    int		h(int) const;
    size_t	Size(void) const;

    void	Blit(s32, s32, int maxw, Surface & sf = Display::Get());
    static int	CharWidth(int, int ft);
    static int	CharHeight(int ft);
    static int	CharAscent(int ft);
    static int	CharDescent(int ft);

private:
    std::string message;
};

#ifdef WITH_TTF
class TextUnicode : public TextInterface
{
public:
    TextUnicode() {};
    TextUnicode(const std::string &, int ft = Font::BIG);
    TextUnicode(const u16*, size_t, int ft = Font::BIG);

    void	SetText(const std::string &);
    void	SetFont(int);
    void	Clear(void);

    int		w(void) const;
    int		w(u32, u32) const;
    int		h(void) const;
    int		h(int) const;
    size_t	Size(void) const;

    void	Blit(s32, s32, int maxw, Surface & sf = Display::Get());

    static bool isspace(int);
    static int	CharWidth(int, int ft);
    static int	CharHeight(int ft);
    static int	CharAscent(int ft);
    static int	CharDescent(int ft);

private:
    std::vector<u16> message;
};
#endif

class Text
{
public:
    Text();
    Text(const std::string &, int ft = Font::BIG);
#ifdef WITH_TTF
    Text(const u16*, size_t, int ft = Font::BIG);
#endif
    Text(const Text &);
    ~Text();

    Text & operator= (const Text &);

    void	Set(const std::string &, int);
    void	Set(const std::string &);
    void	Set(int);

    void	Clear(void);
    size_t	Size(void) const;

    int		w(void) const{ return gw; }
    int		h(void) const{ return gh; }

    void	Blit(s32, s32, Surface & sf = Display::Get()) const;
    void	Blit(s32, s32, int maxw, Surface & sf = Display::Get()) const;
    void	Blit(const Point &, Surface & sf = Display::Get()) const;

    static u32	width(const std::string &, int ft, u32 start = 0, u32 count = 0);
    static u32	height(const std::string &, int ft, u32 width = 0);

protected:
    TextInterface*	message;
    u32			gw;
    u32			gh;
};

class TextSprite : protected Text
{
public:
    TextSprite();
    TextSprite(const std::string &, int ft, const Point & pt);
    TextSprite(const std::string &, int ft, s32, s32);

    void	SetPos(const Point & pt){ SetPos(pt.x, pt.y); }
    void	SetPos(s32, s32);
    void	SetText(const std::string &);
    void	SetText(const std::string &, int);
    void	SetFont(int);

    void	Show(void);
    void	Hide(void);

    bool	isHide(void) const;
    bool	isShow(void) const;

    int		w(void);
    int		h(void);

    const Rect &
		GetRect(void) const;
private:
    SpriteBack	back;
    bool	hide;
};

class TextBox : protected Rect
{
public:
    TextBox();
    TextBox(const std::string &, int, u32 width);
    TextBox(const std::string &, int, const Rect &);

    void	Set(const std::string &, int, u32 width);
    void	SetAlign(int type);

    const Rect & GetRect(void) const { return *this; }
    s32		x(void) const { return Rect::x; }
    s32		y(void) const { return Rect::y; }
    int		w(void) const { return Rect::w; }
    int		h(void) const { return Rect::h; }
    u32		row(void) const { return messages.size(); }

    void	Blit(s32, s32, Surface & sf = Display::Get());
    void	Blit(const Point &, Surface & sf = Display::Get());

private:
    void	Append(const std::string &, int, u32);
#ifdef WITH_TTF
    void	Append(const std::vector<u16> &, int, u32);
#endif

    std::list<Text>	messages;
    int			align;
};

#endif
