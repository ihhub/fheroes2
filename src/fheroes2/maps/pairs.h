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

#ifndef H2PAIRS_H_
#define H2PAIRS_H_

#include <utility>
#include "mp2.h"
#include "color.h"
#include "resource.h"

class IndexDistance : public std::pair<s32, u32>
{
    public:
    IndexDistance() : std::pair<s32, u32>(-1, 0) {};
    IndexDistance(s32 i, u32 d) : std::pair<s32, u32>(i, d) {};

    static bool Shortest(const IndexDistance & id1, const IndexDistance & id2){ return id1.second < id2.second; };
    static bool Longest(const IndexDistance & id1, const IndexDistance & id2){ return id1.second > id2.second; };
};

StreamBase & operator>> (StreamBase &, IndexDistance &);

class IndexObject : public std::pair<s32, int>
{
    public:
    IndexObject() : std::pair<s32, int>(-1, MP2::OBJ_ZERO) {};
    IndexObject(s32 index, int object) : std::pair<s32, int>(index, object) {};

    bool isIndex(s32 index) const { return index == first; };
    bool isObject(int object) const { return object == second; };
};

StreamBase & operator>> (StreamBase &, IndexObject &);

class ObjectColor : public std::pair<int, int>
{
    public:
    ObjectColor() : std::pair<int, int>(MP2::OBJ_ZERO, Color::NONE) {};
    ObjectColor(int object, int color) : std::pair<int, int>(object, color) {};

    bool isObject(int object) const { return object == first; };
    bool isColor(int colors) const { return colors & second; };
};

StreamBase & operator>> (StreamBase &, ObjectColor &);

class ResourceCount : public std::pair<int, u32>
{
    public:
    ResourceCount() : std::pair<int, u32>(Resource::UNKNOWN, 0) {};
    ResourceCount(int res, u32 count) : std::pair<int, u32>(res, count) {};

    bool isResource(int res) const { return res == first; };
    bool isValid(void) const { return (first & Resource::ALL) && second; };
};

StreamBase & operator>> (StreamBase &, ResourceCount &);

/*
template<class T>
struct map_data_compare : public std::binary_function<typename T::value_type,
                                                      typename T::mapped_type, bool>
{
    bool operator() (const typename T::value_type & p, const typename T::mapped_type & i) const
    {
        return p.second == i;
    }
};
*/

#endif
