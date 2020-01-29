/***************************************************************************
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2EDITOR_DIALOGS_H
#define H2EDITOR_DIALOGS_H

#include "artifact.h"
#include "heroes.h"
#include "monster.h"
#include "skill.h"
#include "spell.h"

namespace Dialog
{
    Monster SelectMonster( int id = Monster::UNKNOWN );
    int SelectHeroes( int cur = Heroes::UNKNOWN );
    Artifact SelectArtifact( int id = Artifact::UNKNOWN );
    Spell SelectSpell( int id = Spell::NONE );
    Skill::Secondary SelectSecondarySkill( void );
}

#endif
