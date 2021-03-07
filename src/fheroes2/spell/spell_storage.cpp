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

#include <algorithm>
#include <sstream>

#include "artifact.h"
#include "settings.h"
#include "skill.h"
#include "spell_storage.h"

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

bool SpellStorage::hasAdventureSpell( const int lvl ) const
{
    for ( const_iterator it = begin(); it != end(); ++it ) {
        if ( ( *it ).Level() == lvl && ( *it ).isAdventure() )
            return true;
    }

    return false;
}

std::string SpellStorage::String( void ) const
{
    std::ostringstream os;

    for ( const_iterator it = begin(); it != end(); ++it )
        os << ( *it ).GetName() << ", ";

    return os.str();
}

void SpellStorage::Append( const BagArtifacts & bag )
{
    for ( BagArtifacts::const_iterator it = bag.begin(); it != bag.end(); ++it )
        Append( *it );
}

void SpellStorage::Append( const Artifact & art )
{
    switch ( art() ) {
    case Artifact::SPELL_SCROLL:
        Append( Spell( art.GetSpell() ) );
        break;

    case Artifact::CRYSTAL_BALL:
        if ( Settings::Get().ExtWorldArtifactCrystalBall() ) {
            Append( Spell( Spell::IDENTIFYHERO ) );
            Append( Spell( Spell::VISIONS ) );
        }
        break;

    case Artifact::BATTLE_GARB:
        Append( Spell( Spell::TOWNPORTAL ) );
        break;

    default:
        break;
    }
}
