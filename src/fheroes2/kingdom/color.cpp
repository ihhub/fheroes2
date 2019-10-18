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

#include <sstream>
#include "players.h"
#include "settings.h"
#include "world.h"
#include "game.h"
#include "color.h"

const char* Color::String(int color)
{
    const char* str_color[] = { "None", _("Blue"), _("Green"), _("Red"), _("Yellow"), _("Orange"), _("Purple"), "uknown" };

    switch(color)
    {
        case Color::BLUE: 	return str_color[1];
        case Color::GREEN: 	return str_color[2];
        case Color::RED:	return str_color[3];
        case Color::YELLOW:	return str_color[4];
	case Color::ORANGE: 	return str_color[5];
	case Color::PURPLE: 	return str_color[6];
	case Color::UNUSED: 	return str_color[7];
    }

    return str_color[0];
}

int Color::GetIndex(int color)
{
    switch(color)
    {
        case BLUE: 	return 0;
        case GREEN: 	return 1;
        case RED:	return 2;
        case YELLOW:	return 3;
	case ORANGE: 	return 4;
	case PURPLE: 	return 5;
	default: break;
    }

    // NONE
    return 6;
}

int Color::Count(int colors)
{
    return CountBits(colors & ALL);
}

int Color::FromInt(int col)
{
    switch(col)
    {
        case BLUE:
        case GREEN:
        case RED:
        case YELLOW:
	case ORANGE:
	case PURPLE:	return col;
	default: break;
    }

    return NONE;
}

int Color::GetFirst(int colors)
{
    if(colors & BLUE) return BLUE;
    else
    if(colors & GREEN) return GREEN;
    else
    if(colors & RED) return RED;
    else
    if(colors & YELLOW) return YELLOW;
    else
    if(colors & ORANGE) return ORANGE;
    else
    if(colors & PURPLE) return PURPLE;

    return NONE;
}

const char* BarrierColor::String(int val)
{
    switch(val)
    {
        case AQUA:	return _("Aqua");
        case BLUE:	return _("Blue");
        case BROWN:	return _("Brown");
        case GOLD:	return _("Gold");
        case GREEN:	return _("Green");
        case ORANGE:	return _("Orange");
        case PURPLE:	return _("Purple");
        case RED:	return _("Red");
        default: break;
    }

    return "None";
}

Colors::Colors(int colors)
{
    reserve(6);

    if(colors & Color::BLUE)	push_back(Color::BLUE);
    if(colors & Color::GREEN)	push_back(Color::GREEN);
    if(colors & Color::RED)	push_back(Color::RED);
    if(colors & Color::YELLOW)	push_back(Color::YELLOW);
    if(colors & Color::ORANGE)	push_back(Color::ORANGE);
    if(colors & Color::PURPLE)	push_back(Color::PURPLE);
}

std::string Colors::String(void) const
{
    std::ostringstream os;

    for(const_iterator
	it = begin(); it != end(); ++it)
	    os << Color::String(*it) << ", ";

    return os.str();
}

bool ColorBase::operator== (int col) const
{
    return color == col;
}

bool ColorBase::isFriends(int col) const
{
    return (col & Color::ALL) && (color == col || Players::isFriends(color, col));
}

void ColorBase::SetColor(int col)
{
    color = Color::FromInt(col);
}

Kingdom & ColorBase::GetKingdom(void) const
{
    return world.GetKingdom(color);
}

StreamBase & operator<< (StreamBase & msg, const ColorBase & col)
{
    return msg << col.color;
}

StreamBase & operator>> (StreamBase & msg, ColorBase & col)
{
    return msg >> col.color;
}
