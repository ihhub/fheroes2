/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2POSITION_H
#define H2POSITION_H

#include "gamedefs.h"

class MapPosition
{
public:
    MapPosition(const Point & = Point(-1, -1));

    bool operator== (s32) const;

    const Point &	GetCenter(void) const;
    s32			GetIndex(void) const;

    void		SetCenter(const Point &);
    void		SetIndex(s32);

    bool		isPosition(const Point &) const;

protected:
    friend StreamBase & operator<< (StreamBase &, const MapPosition &);
    friend StreamBase & operator>> (StreamBase &, MapPosition &);

    Point	center;
};

StreamBase & operator<< (StreamBase &, const MapPosition &);
StreamBase & operator>> (StreamBase &, MapPosition &);

#endif
