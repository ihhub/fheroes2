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

#ifndef H2SPELLSTORAGE_H
#define H2SPELLSTORAGE_H

#include <vector>

#include "gamedefs.h"
#include "spell.h"

class Artifact;
class BagArtifacts;

class SpellStorage
{
public:
    SpellStorage();

    // used to allow iteration over this object
    using const_iterator = std::vector<Spell>::const_iterator;
    const_iterator begin() const;
    const_iterator end() const;

    bool hasSpell( const Spell & spell ) const;
    bool hasAdventureSpellAtLevel( const int spellLevel ) const;
    std::vector<Spell> getSpells( const int spellLevel = -1 ) const;

    void append( const SpellStorage & spellStorage );
    void append( const Spell & spell );
    void append( const BagArtifacts & bag );
    void append( const Artifact & artifact );

    std::string string() const;

protected:
    std::vector<Spell> _spells;

private:
    friend StreamBase & operator<<( StreamBase &, const SpellStorage & );
    friend StreamBase & operator>>( StreamBase &, SpellStorage & );
};

StreamBase & operator<<( StreamBase &, const SpellStorage & );
StreamBase & operator>>( StreamBase &, SpellStorage & );

#endif
