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

#include "spell_storage.h"

#include <algorithm>

#include "artifact.h"

SpellStorage::SpellStorage()
{
    reserve( 67 );
}

SpellStorage SpellStorage::GetSpells( int lvl ) const
{
    SpellStorage result;
    result.reserve( 20 );
    for ( const_iterator it = begin(); it != end(); ++it )
        if ( lvl == -1 || ( *it ).isLevel( lvl ) )
            result.push_back( *it );
    return result;
}

void SpellStorage::Append( const Spell & sp )
{
    if ( sp != Spell::NONE && end() == std::find( begin(), end(), sp ) )
        push_back( sp );
}

void SpellStorage::Append( const SpellStorage & st )
{
    for ( const Spell & sp : st ) {
        if ( std::find( begin(), end(), sp ) == end() ) {
            push_back( sp );
        }
    }
}

bool SpellStorage::isPresentSpell( const Spell & spell ) const
{
    return end() != std::find( begin(), end(), spell );
}

std::string SpellStorage::String() const
{
    std::string output;

    for ( const_iterator it = begin(); it != end(); ++it ) {
        output += it->GetName();
        output += ", ";
    }

    return output;
}

void SpellStorage::Append( const BagArtifacts & bag )
{
    for ( BagArtifacts::const_iterator it = bag.begin(); it != bag.end(); ++it )
        Append( *it );
}

void SpellStorage::Append( const Artifact & art )
{
    Append( Spell( art.getSpellId() ) );
}

bool SpellStorage::removeSpell( const Spell & spell )
{
    if ( spell == Spell::NONE ) {
        return false;
    }

    if ( auto foundSpell = std::find( begin(), end(), spell ); foundSpell != end() ) {
        erase( foundSpell );
        return true;
    }

    return false;
}
