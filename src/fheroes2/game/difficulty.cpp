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

#include "difficulty.h"
#include "translations.h"

const std::string & Difficulty::String( int difficulty )
{
    static const std::string str_difficulty[]
        = {_( "difficulty|Easy" ), _( "difficulty|Normal" ), _( "difficulty|Hard" ), _( "difficulty|Expert" ), _( "difficulty|Impossible" ), "Unknown"};

    switch ( difficulty ) {
    case Difficulty::EASY:
        return str_difficulty[0];
    case Difficulty::NORMAL:
        return str_difficulty[1];
    case Difficulty::HARD:
        return str_difficulty[2];
    case Difficulty::EXPERT:
        return str_difficulty[3];
    case Difficulty::IMPOSSIBLE:
        return str_difficulty[4];
    default:
        break;
    }

    return str_difficulty[5];
}

int Difficulty::GetScoutingBonus( int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::NORMAL:
        return 1;
    case Difficulty::HARD:
        return 2;
    case Difficulty::EXPERT:
        return 3;
    case Difficulty::IMPOSSIBLE:
        return 4;
    default:
        break;
    }
    return 0;
}

double Difficulty::GetGoldIncomeBonus( int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::EASY:
        return 0.75;
    case Difficulty::HARD:
        return 1.29;
    case Difficulty::EXPERT:
        return 1.45;
    case Difficulty::IMPOSSIBLE:
        return 1.6;
    default:
        break;
    }
    return 1.0;
}

double Difficulty::GetUnitGrowthBonus( int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::HARD:
        return 1.2;
    case Difficulty::EXPERT:
        return 1.32;
    case Difficulty::IMPOSSIBLE:
        return 1.44;
    default:
        break;
    }
    return 1.0;
}

int Difficulty::GetHeroMovementBonus( int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::EXPERT:
    case Difficulty::IMPOSSIBLE:
        return 75;
    default:
        break;
    }
    return 0;
}

double Difficulty::GetAIRetreatRatio( int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::NORMAL:
        return 100.0 / 7.5;
    case Difficulty::HARD: // fall-through
    case Difficulty::EXPERT:
        return 100.0 / 8.5;
    case Difficulty::IMPOSSIBLE:
        return 100.0 / 10.0;
    default:
        break;
    }
    return 100.0 / 6.0;
}
