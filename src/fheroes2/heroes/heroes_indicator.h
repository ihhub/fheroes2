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

#ifndef H2HEROESIND_H
#define H2HEROESIND_H

#include <string>

#include "image.h"
#include "luck.h"
#include "math_base.h"
#include "morale.h"

namespace fheroes2
{
    std::string LuckString( const int luck );
    std::string MoraleString( const int morale );
}

class Heroes;

class HeroesIndicator
{
public:
    explicit HeroesIndicator( const Heroes * hero );

    const fheroes2::Rect & GetArea() const;

    void SetPos( const fheroes2::Point & pt );

protected:
    const Heroes * _hero;
    fheroes2::Rect _area;
    fheroes2::ImageRestorer _back;
    std::string _description;
};

class LuckIndicator : public HeroesIndicator
{
public:
    explicit LuckIndicator( const Heroes * hero )
        : HeroesIndicator( hero )
    {
        _area.width = 35;
        _area.height = 26;
    }

    void Redraw();

    // Redraws only the background under this indicator, but not the indicator itself
    void redrawOnlyBackground()
    {
        _back.restore();
    }

    static void QueueEventProcessing( const LuckIndicator & indicator );

private:
    int _luck{ Luck::NORMAL };
};

class MoraleIndicator : public HeroesIndicator
{
public:
    explicit MoraleIndicator( const Heroes * hero )
        : HeroesIndicator( hero )
    {
        _area.width = 35;
        _area.height = 26;
    }

    void Redraw();

    // Redraws only the background under this indicator, but not the indicator itself
    void redrawOnlyBackground()
    {
        _back.restore();
    }

    static void QueueEventProcessing( const MoraleIndicator & indicator );

private:
    int _morale{ Morale::NORMAL };
};

class ExperienceIndicator : public HeroesIndicator
{
public:
    explicit ExperienceIndicator( const Heroes * hero );

    void Redraw() const;
    void QueueEventProcessing() const;

    // Set if default value is used. Use this method only in Editor!
    void setDefaultState( const bool isDefault )
    {
        _isDefault = isDefault;
    }

private:
    // This state is used in Editor to show that default value is used.
    bool _isDefault{ false };
};

class SpellPointsIndicator : public HeroesIndicator
{
public:
    explicit SpellPointsIndicator( const Heroes * hero );

    void Redraw() const;
    void QueueEventProcessing() const;

    // Set if default value is used. Use this method only in Editor!
    void setDefaultState( const bool isDefault )
    {
        _isDefault = isDefault;
    }

private:
    // This state is used in Editor to show that default value is used.
    bool _isDefault{ false };
};

#endif
