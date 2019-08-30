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

const char* Difficulty::String(int difficulty)
{
    const char* str_difficulty[] = { _("difficulty|Easy"), _("difficulty|Normal"), _("difficulty|Hard"), _("difficulty|Expert"), _("difficulty|Impossible"), "Unknown" };

    switch(difficulty)
    {
        case Difficulty::EASY:		return str_difficulty[0];
        case Difficulty::NORMAL:	return str_difficulty[1];
	case Difficulty::HARD: 		return str_difficulty[2];
	case Difficulty::EXPERT: 	return str_difficulty[3];
	case Difficulty::IMPOSSIBLE: 	return str_difficulty[4];
	default: break;
    }

    return str_difficulty[5];
}
