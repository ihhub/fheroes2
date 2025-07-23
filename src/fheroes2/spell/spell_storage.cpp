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

#include "spell_storage.h"

#include <algorithm>

#include "artifact.h"

SpellStorage SpellStorage::GetSpells( const int level /* = -1 */ ) const
{
    if ( level == -1 ) {
        return *this;
    }

    SpellStorage result;

    for ( const Spell & spell : *this ) {
        if ( spell.isLevel( level ) ) {
            result.push_back( spell );
        }
    }

    return result;
}

void SpellStorage::Append( const Spell & spell )
{
    if ( spell == Spell::NONE ) {
        return;
    }

    if ( isPresentSpell( spell ) ) {
        return;
    }

    push_back( spell );
}

void SpellStorage::Append( const SpellStorage & storage )
{
    for ( const Spell & spell : storage ) {
        if ( std::find( cbegin(), cend(), spell ) == cend() ) {
            push_back( spell );
        }
    }
}

std::string SpellStorage::String() const
{
    std::string output;

    for ( const Spell & spell : *this ) {
        output += spell.GetName();
        output += ", ";
    }

    return output;
}

void SpellStorage::Append( const BagArtifacts & bag )
{
    for ( const Artifact & artifact : bag ) {
        Append( Spell( artifact.getSpellId() ) );
    }
}

bool SpellStorage::removeSpell( const Spell & spell )
{
    if ( spell == Spell::NONE ) {
        return false;
    }

    auto foundSpell = std::find( cbegin(), cend(), spell );

    if ( foundSpell == cend() ) {
        return false;
    }

    erase( foundSpell );

    return true;
}

bool SpellStorage::isPresentSpell( const Spell & spell ) const
{
    return std::find( cbegin(), cend(), spell ) != cend();
}
