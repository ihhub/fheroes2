/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#include <cstdint>

#include "monster.h"

enum class WeekName : int
{
    UNNAMED,

    // A regular week (Week Of ...)
    SQUIRREL,
    RABBIT,
    GOPHER,
    BADGER,
    RAT,
    EAGLE,
    WEASEL,
    RAVEN,
    MONGOOSE,
    DOG,
    AARDVARK,
    LIZARD,
    TORTOISE,
    HEDGEHOG,
    CONDOR,

    // A regular first week of the month (Month Of ...)
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

    // The Week of a monster (the Month of a monster, if it's the first week of the month)
    MONSTERS,

    // The Month of the Plague
    PLAGUE
};

struct Week
{
    Week( const WeekName type = WeekName::UNNAMED, const Monster::MonsterType monster = Monster::UNKNOWN )
        : _week( type )
        , _monster( monster )
    {}

    WeekName GetType() const
    {
        return _week;
    }

    Monster::MonsterType GetMonster() const
    {
        return _monster;
    }

    const char * GetName() const;

    static Week RandomWeek( const bool isNewMonth, const uint32_t weekSeed );

private:
    WeekName _week;
    Monster::MonsterType _monster;
};

#endif
