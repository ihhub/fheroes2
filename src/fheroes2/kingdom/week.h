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

#ifndef H2WEEK_H
#define H2WEEK_H

#include "monster.h"

enum class WeekName : int
{
    UNNAMED,
    PLAGUE,
    ANT,
    GRASSHOPPER,
    DRAGONFLY,
    SPIDER,
    BUTTERFLY,
    BUMBLEBEE,
    LOCUST,
    EARTHWORM,
    HORNET,
    BEETLE,
    SQUIRREL,
    RABBIT,
    GOPHER,
    BADGER,
    EAGLE,
    WEASEL,
    RAVEN,
    MONGOOSE,
    AARDVARK,
    LIZARD,
    TORTOISE,
    HEDGEHOG,
    CONDOR,
    MONSTERS // week of "monster"
};

class World;

struct Week
{
    Week( const WeekName type = WeekName::UNNAMED, const Monster::monster_t monster = Monster::UNKNOWN )
        : _week( type )
        , _monster( monster )
    {}

    WeekName GetType() const
    {
        return _week;
    }

    Monster::monster_t GetMonster() const
    {
        return _monster;
    }

    const char * GetName() const;

    static Week RandomWeek( const World & world, const bool isNewMonth, const uint32_t weekSeed );

    friend StreamBase & operator>>( StreamBase & stream, Week & week );

private:
    WeekName _week;
    Monster::monster_t _monster;
};

StreamBase & operator>>( StreamBase & stream, Week & week );
StreamBase & operator<<( StreamBase & stream, const Week & week );

#endif
