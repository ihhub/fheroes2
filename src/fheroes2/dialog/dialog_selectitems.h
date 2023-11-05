/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
    Monster selectMonster( const int monsterId );

    int selectHeroes( const int heroId = Heroes::UNKNOWN );

    Artifact selectArtifact( const int artifactId = Artifact::UNKNOWN );

    Spell selectSpell( const int spellId = Spell::NONE );

    Skill::Secondary selectSecondarySkill( const Heroes & hero, const int skillId = Skill::Secondary::UNKNOWN );

    // These functions should be called only from the Editor as they rely on Maps::ObjectInfo structures that are not the same as in-game items.
    int selectHeroType( const int heroType );

    int selectMonsterType( const int monsterType );

    int selectTreasureType( const int resourceType );

    int selectOceanObjectType( const int resourceType );
}

#endif
