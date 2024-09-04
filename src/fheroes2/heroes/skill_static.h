/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2SKILL_STATIC_H
#define H2SKILL_STATIC_H

#include <cstdint>

namespace Skill
{
    struct ValuesPerLevel
    {
        uint16_t basic;
        uint16_t advanced;
        uint16_t expert;
    };

    struct PrimarySkillValues
    {
        uint8_t attack;
        uint8_t defense;
        uint8_t power;
        uint8_t knowledge;
    };

    struct SecondarySkillValues
    {
        uint8_t archery;
        uint8_t ballistics;
        uint8_t diplomacy;
        uint8_t eagleeye;
        uint8_t estates;
        uint8_t leadership;
        uint8_t logistics;
        uint8_t luck;
        uint8_t mysticism;
        uint8_t navigation;
        uint8_t necromancy;
        uint8_t pathfinding;
        uint8_t scouting;
        uint8_t wisdom;
    };

    struct FactionProperties
    {
        const char * id;
        PrimarySkillValues captainPrimarySkills;
        PrimarySkillValues heroInitialPrimarySkills;
        uint8_t initialSpell;
        SecondarySkillValues heroInitialSecondarySkills;
        uint8_t boundaryBetweenLowAndHighLevels;
        PrimarySkillValues weightsOfPrimarySkillsForLowLevels;
        PrimarySkillValues weightsOfPrimarySkillsForHighLevels;
        SecondarySkillValues weightsOfSecondarySkills;
    };

    struct SecondarySkillValuesPerLevel
    {
        const char * id;
        ValuesPerLevel values;
    };
}

#endif
