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

#ifndef H2RECRUITS_H
#define H2RECRUITS_H

#include <cstdint>
#include <utility>

#include "heroes.h"

class Heroes;
class StreamBase;

struct Recruit
{
    Recruit()
        : id( Heroes::UNKNOWN )
        , surrenderDay( 0 )
    {}

    Recruit( const Heroes & hero, const uint32_t heroSurrenderDay )
        : id( hero.GetID() )
        , surrenderDay( heroSurrenderDay )
    {}

    explicit Recruit( const Heroes & hero )
        : Recruit( hero, 0 )
    {}

    int id;
    uint32_t surrenderDay;
};

class Recruits : public std::pair<Recruit, Recruit>
{
public:
    Recruits();

    void Reset();

    int GetID1() const;
    int GetID2() const;

    Heroes * GetHero1() const;
    Heroes * GetHero2() const;

    uint32_t getSurrenderDayOfHero1() const;
    uint32_t getSurrenderDayOfHero2() const;

    void SetHero1( const Heroes * hero );
    void SetHero2( const Heroes * hero );

    void SetHero2Tmp( const Heroes * hero, const uint32_t heroSurrenderDay );

    void appendSurrenderedHero( const Heroes & hero, const uint32_t heroSurrenderDay );
};

StreamBase & operator<<( StreamBase & msg, const Recruit & recruit );
StreamBase & operator>>( StreamBase & msg, Recruit & recruit );

#endif
