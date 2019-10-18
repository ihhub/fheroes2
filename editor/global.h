/***************************************************************************
 *   Copyright (C) 2013 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef _EDITOR_GLOBAL_H_
#define _EDITOR_GLOBAL_H_

class QDomElement;
struct MonsterStat;

namespace Default
{
    int &		resourceGoldMin(void);
    int &		resourceGoldMax(void);
    int &		resourceWoodOreMin(void);
    int &		resourceWoodOreMax(void);
    int &		resourceOtherMin(void);
    int &		resourceOtherMax(void);

    MonsterStat &	monsterStat(int);
}

struct DefaultValues
{
};

QDomElement & operator<< (QDomElement &, const DefaultValues &);
QDomElement & operator>> (QDomElement &, DefaultValues &);

#define FH2ENGINE_VERSION_3269		3269
#define FH2ENGINE_CURRENT_VERSION	FH2ENGINE_VERSION_3269
#define FH2ENGINE_LAST_VERSION		FH2ENGINE_VERSION_3269

#endif
