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

#ifndef H2HEROESIND_H
#define H2HEROESIND_H

#include <string>

#include "gamedefs.h"
#include "image.h"

namespace fheroes2
{
    std::string LuckString( const int luck );
    std::string MoraleString( const int morale );
}

class Heroes;

class HeroesIndicator
{
public:
    explicit HeroesIndicator( const Heroes * h = nullptr );

    const fheroes2::Rect & GetArea( void ) const;
    void SetPos( const fheroes2::Point & );
    void SetHero( const Heroes * hero );

protected:
    const Heroes * hero;
    fheroes2::Rect area;
    fheroes2::ImageRestorer back;
    std::string descriptions;
};

class LuckIndicator : public HeroesIndicator
{
public:
    explicit LuckIndicator( const Heroes * h = nullptr );

    void Redraw( void );
    static void QueueEventProcessing( const LuckIndicator & );

private:
    int luck;
};

class MoraleIndicator : public HeroesIndicator
{
public:
    explicit MoraleIndicator( const Heroes * h = nullptr );

    void Redraw( void );
    static void QueueEventProcessing( const MoraleIndicator & );

private:
    int morale;
};

class ExperienceIndicator : public HeroesIndicator
{
public:
    explicit ExperienceIndicator( const Heroes * h = nullptr );

    void Redraw( void ) const;
    void QueueEventProcessing( void ) const;
};

class SpellPointsIndicator : public HeroesIndicator
{
public:
    explicit SpellPointsIndicator( const Heroes * h = nullptr );

    void Redraw( void ) const;
    void QueueEventProcessing( void ) const;
};

#endif
