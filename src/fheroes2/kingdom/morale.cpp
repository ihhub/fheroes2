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

#include "gamedefs.h"
#include "morale.h"

const char* Morale::String(int morale)
{
    const char* str_morale[] = { "Unknown", _("morale|Treason"), _("morale|Awful"), _("morale|Poor"), _("morale|Normal"), _("morale|Good"), _("morale|Great"), _("morale|Blood!") };

    switch(morale)
    {
        case Morale::TREASON:	return str_morale[1];
        case Morale::AWFUL:	return str_morale[2];
        case Morale::POOR:	return str_morale[3];
        case Morale::NORMAL:	return str_morale[4];
        case Morale::GOOD:	return str_morale[5];
        case Morale::GREAT:	return str_morale[6];
        case Morale::BLOOD:	return str_morale[7];
	default: break;
    }

    return str_morale[0];
}

const char* Morale::Description(int morale)
{
    const char* str_desc_morale[] = { "Unknown",
	_("Bad morale may cause your armies to freeze in combat."),
	_("Neutral morale means your armies will never be blessed with extra attacks or freeze in combat."),
	_("Good morale may give your armies extra attacks in combat.") };

    switch(morale)
    {
        case Morale::TREASON:
        case Morale::AWFUL:
        case Morale::POOR:	return str_desc_morale[1];
        case Morale::NORMAL:	return str_desc_morale[2];
        case Morale::GOOD:
        case Morale::GREAT:
        case Morale::BLOOD:	return str_desc_morale[3];
	default: break;
    }

    return str_desc_morale[0];
}
