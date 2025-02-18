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

#pragma once

#include <cstdint>
#include <utility>

class IStreamBase;
class OStreamBase;

class Heroes;

class Recruit
{
public:
    Recruit();
    Recruit( const Heroes & hero, const uint32_t surrenderDay );
    explicit Recruit( const Heroes & hero );

    int getID() const
    {
        return _id;
    }

    uint32_t getSurrenderDay() const
    {
        return _surrenderDay;
    }

private:
    int _id;
    uint32_t _surrenderDay;

    friend OStreamBase & operator<<( OStreamBase & stream, const Recruit & recruit );
    friend IStreamBase & operator>>( IStreamBase & stream, Recruit & recruit );
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

    void SetHero1( Heroes * hero );
    void SetHero2( Heroes * hero );

    void appendSurrenderedHero( Heroes & hero, const uint32_t heroSurrenderDay );
};

OStreamBase & operator<<( OStreamBase & stream, const Recruit & recruit );
IStreamBase & operator>>( IStreamBase & stream, Recruit & recruit );
