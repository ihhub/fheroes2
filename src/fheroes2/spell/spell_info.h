/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#ifndef H2_SPELL_INFO_H
#define H2_SPELL_INFO_H

#include <cstdint>
#include <string>

class Castle;
class HeroBase;
class Heroes;
class Spell;

namespace fheroes2
{
    uint32_t getSpellDamage( const Spell & spell, const uint32_t spellPower, const HeroBase * hero );

    uint32_t getSummonMonsterCount( const Spell & spell, const uint32_t spellPower, const HeroBase * hero );

    uint32_t getHPRestorePoints( const Spell & spell, const uint32_t spellPower, const HeroBase * hero );

    uint32_t getResurrectPoints( const Spell & spell, const uint32_t spellPower, const HeroBase * hero );

    uint32_t getGuardianMonsterCount( const Spell & spell, const uint32_t spellPower, const HeroBase * hero );

    uint32_t getHypnotizeMonsterHPPoints( const Spell & spell, const uint32_t spellPower, const HeroBase * hero );

    const Castle * getNearestCastleTownGate( const Heroes & hero );

    std::string getSpellDescription( const Spell & spell, const HeroBase * hero );

    // Returns the index of the tile that the boat would have been summoned to when using the Summon Boat spell by the given hero, or -1 if there is no suitable tile
    // nearby.
    int32_t getPossibleBoatPosition( const Heroes & hero );

    int32_t getSummonableBoat( const Heroes & hero );

    bool isHeroNearWater( const Heroes & hero );
}

#endif
