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
#include "spell_storage.h"

SpellStorage::SpellStorage()
{
    spells.reserve( 67 );
}

SpellStorage::const_iterator SpellStorage::begin() const
{
    return spells.begin();
}

SpellStorage::const_iterator SpellStorage::end() const
{
    return spells.end();
}

int SpellStorage::Size() const
{
    return static_cast<int>( spells.size() );
}

bool SpellStorage::HasSpell( const Spell & spell ) const
{
    return std::find( spells.begin(), spells.end(), spell ) != spells.end();
}

bool SpellStorage::HasAdventureSpellAtLevel( const int spellLevel ) const
{
    for ( auto it : spells ) {
        if ( it.isLevel( spellLevel ) && it.isAdventure() )
            return true;
    }

    return false;
}

std::vector<Spell> SpellStorage::GetSpells( const int spellLevel ) const
{
    if ( spellLevel == -1 ) {
        return spells;
    }

    std::vector<Spell> result;
    result.reserve( 20 );
    for ( auto it : spells )
        if ( it.isLevel( spellLevel ) )
            result.push_back( it );
    return result;
}

void SpellStorage::Append( const SpellStorage & spellStorage )
{
    for ( const Spell & spell : spellStorage.spells ) {
        if ( std::find( spells.begin(), spells.end(), spell ) == spells.end() ) {
            spells.push_back( spell );
        }
    }
}

void SpellStorage::Append( const Spell & spell )
{
    if ( spell != Spell::NONE && spells.end() == std::find( spells.begin(), spells.end(), spell ) )
        spells.push_back( spell );
}

void SpellStorage::Append( const BagArtifacts & bag )
{
    for ( BagArtifacts::const_iterator it = bag.begin(); it != bag.end(); ++it )
        Append( *it );
}

void SpellStorage::Append( const Artifact & artifact )
{
    switch ( artifact.GetID() ) {
    case Artifact::SPELL_SCROLL:
        Append( Spell( artifact.GetSpell() ) );
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

std::string SpellStorage::String( void ) const
{
    std::ostringstream os;

    for ( auto it : spells )
        os << it.GetName() << ", ";

    return os.str();
}

StreamBase & operator<<( StreamBase & msg, const SpellStorage & spellStorage )
{
    return msg << spellStorage.spells;
}

StreamBase & operator>>( StreamBase & msg, SpellStorage & spellStorage )
{
    return msg >> spellStorage.spells;
}
