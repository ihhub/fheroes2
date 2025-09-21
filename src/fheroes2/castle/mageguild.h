/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "spell_storage.h"

class IStreamBase;
class OStreamBase;

class HeroBase;

class MageGuild
{
public:
    MageGuild() = default;

    // Initializes the Mage Guild according to the rules described in
    // https://handbookhmm.ru/kakim-obrazom-zaklinaniya-popadayut-v-magicheskuyu-gildiyu.html
    // except for the part related to the hidden AI-only bonuses.
    void initialize( const int race, const bool hasLibrary );
    void initialize( const int race, const bool hasLibrary, const std::map<uint8_t, int32_t> & mustHaveSpells, const std::vector<int32_t> & bannedSpells );
    void trainHero( HeroBase & hero, const int guildLevel, const bool hasLibrary ) const;
    SpellStorage GetSpells( const int guildLevel, const bool hasLibrary, const int spellLevel = -1 ) const;

    static int32_t getMaxSpellsCount( const int spellLevel, const bool hasLibrary );

private:
    friend OStreamBase & operator<<( OStreamBase & stream, const MageGuild & guild );
    friend IStreamBase & operator>>( IStreamBase & stream, MageGuild & guild );

    SpellStorage _general;
    SpellStorage _library;
};
