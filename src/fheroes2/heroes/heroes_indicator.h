/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#ifndef H2HEROESIND_H
#define H2HEROESIND_H

#include <string>

#include "image.h"
#include "luck.h"
#include "math_base.h"
#include "morale.h"
#include "screen.h"

namespace fheroes2
{
    std::string LuckString( const int luck );
    std::string MoraleString( const int morale );
}

class Heroes;

class HeroesIndicator
{
public:
    explicit HeroesIndicator( const Heroes * hero = nullptr )
        : _hero( hero )
        , _back( fheroes2::Display::instance() )
    {
        // Do nothing.
    }

    const fheroes2::Rect & GetArea() const;
    void SetPos( const fheroes2::Point & pt );
    void SetHero( const Heroes * hero );

protected:
    const Heroes * _hero;
    fheroes2::Rect area;
    fheroes2::ImageRestorer _back;
    std::string descriptions;
};

class LuckIndicator : public HeroesIndicator
{
public:
    explicit LuckIndicator( const Heroes * hero = nullptr )
        : HeroesIndicator( hero )
    {
        area.width = 35;
        area.height = 26;
    }

    void Redraw();
    static void QueueEventProcessing( const LuckIndicator & indicator );

private:
    int luck{ Luck::NORMAL };
};

class MoraleIndicator : public HeroesIndicator
{
public:
    explicit MoraleIndicator( const Heroes * hero = nullptr )
        : HeroesIndicator( hero )
    {
        area.width = 35;
        area.height = 26;
    }

    void Redraw();
    static void QueueEventProcessing( const MoraleIndicator & indicator );

private:
    int morale{ Morale::NORMAL };
};

class ExperienceIndicator : public HeroesIndicator
{
public:
    explicit ExperienceIndicator( const Heroes * hero = nullptr );

    void Redraw() const;
    void QueueEventProcessing() const;
};

class SpellPointsIndicator : public HeroesIndicator
{
public:
    explicit SpellPointsIndicator( const Heroes * hero = nullptr );

    void Redraw() const;
    void QueueEventProcessing() const;
};

#endif
