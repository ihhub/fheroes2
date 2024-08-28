/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#ifndef H2MAGEGUILD_H
#define H2MAGEGUILD_H

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
    void educateHero( HeroBase & hero, int guildLevel, bool hasLibrary ) const;
    SpellStorage GetSpells( int guildLevel, bool hasLibrary, int spellLevel = -1 ) const;

private:
    friend OStreamBase & operator<<( OStreamBase & stream, const MageGuild & guild );
    friend IStreamBase & operator>>( IStreamBase & stream, MageGuild & guild );

    SpellStorage general;
    SpellStorage library;
};

#endif
