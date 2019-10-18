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
#ifndef H2RESOURCE_H
#define H2RESOURCE_H

#include "gamedefs.h"

struct cost_t
{
    u16		gold;
    u8		wood;
    u8		mercury;
    u8		ore;
    u8		sulfur;
    u8		crystal;
    u8		gems;
};

#define COST_NONE { 0, 0, 0, 0, 0, 0 ,0 }

class ResourceCount;

#ifdef WITH_XML
class TiXmlElement;                                                                                                     
void   LoadCostFromXMLElement(cost_t &, const TiXmlElement &);
#endif

namespace Resource
{
    enum
    {
	UNKNOWN = 0x00,
        WOOD	= 0x01,
        MERCURY	= 0x02,
        ORE	= 0x04,
        SULFUR	= 0x08,
        CRYSTAL	= 0x10,
        GEMS	= 0x20,
        GOLD	= 0x40,
	ALL	= WOOD | MERCURY | ORE | SULFUR | CRYSTAL | GEMS | GOLD
    };
}

class Funds
{
public:
	Funds();
	Funds(s32 _ore, s32 _wood, s32 _mercury, s32 _sulfur, s32 _crystal, s32 _gems, s32 _gold);
	Funds(int rs, u32 count);
	Funds(const cost_t &);
	Funds(const ResourceCount &);

	Funds operator+ (const Funds &) const;
	Funds operator* (u32 mul) const;
	Funds operator- (const Funds &) const;
	Funds & operator+= (const Funds &);
	Funds & operator*= (u32 mul);
	Funds & operator-= (const Funds &);
	Funds & operator= (const cost_t &);

	s32  Get(int rs) const;
	s32* GetPtr(int rs);

	bool operator< (const Funds &) const;
	bool operator<= (const Funds &) const;
	bool operator> (const Funds &) const;
	bool operator>= (const Funds &) const;

	int GetValidItems(void) const;
	u32 GetValidItemsCount(void) const;

	void Reset(void);
	std::string String(void) const;

        s32 wood;
        s32 mercury;
        s32 ore;
        s32 sulfur;
        s32 crystal;
        s32 gems;
        s32 gold;
};

StreamBase & operator<< (StreamBase &, const cost_t &);
StreamBase & operator>> (StreamBase &, cost_t &);
StreamBase & operator<< (StreamBase &, const Funds &);
StreamBase & operator>> (StreamBase &, Funds &);

namespace Resource
{
    const char*	String(int resource);
    int		Rand(bool with_gold = false);

    /* return index sprite objnrsrc.icn */
    u32		GetIndexSprite(int resource);
    int		FromIndexSprite(u32 index);

    /* return index sprite resource.icn */
    u32		GetIndexSprite2(int resource);
    int		FromIndexSprite2(u32 index);

    class BoxSprite : protected Rect
    {
    public:
	BoxSprite(const Funds &, u32);

	const Rect &	GetArea(void) const;
	void		SetPos(s32, s32);
	void		Redraw(void) const;

	const Funds rs;
    };
}

#endif
