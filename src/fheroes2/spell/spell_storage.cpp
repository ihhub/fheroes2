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
    _spells.reserve( 67 );
}

SpellStorage::const_iterator SpellStorage::begin() const
{
    return _spells.begin();
}

SpellStorage::const_iterator SpellStorage::end() const
{
    return _spells.end();
}

bool SpellStorage::hasSpell( const Spell & spell ) const
{
    return std::find( _spells.begin(), _spells.end(), spell ) != _spells.end();
}

bool SpellStorage::hasAdventureSpellAtLevel( const int spellLevel ) const
{
    for ( const Spell & spell : _spells ) {
        if ( spell.isLevel( spellLevel ) && spell.isAdventure() )
            return true;
    }

    return false;
}

std::vector<Spell> SpellStorage::getSpells( const int spellLevel ) const
{
    if ( spellLevel == -1 ) {
        return _spells;
    }

    std::vector<Spell> result;
    result.reserve( 20 );
    for ( const Spell & spell : _spells )
        if ( spell.isLevel( spellLevel ) ) {
            result.push_back( spell );
        }

    return result;
}

void SpellStorage::append( const SpellStorage & spellStorage )
{
    for ( const Spell & spell : spellStorage._spells ) {
        if ( std::find( _spells.begin(), _spells.end(), spell ) == _spells.end() ) {
            _spells.push_back( spell );
        }
    }
}

void SpellStorage::append( const Spell & spell )
{
    if ( spell != Spell::NONE && _spells.end() == std::find( _spells.begin(), _spells.end(), spell ) )
        _spells.push_back( spell );
}

void SpellStorage::append( const BagArtifacts & bag )
{
    for ( BagArtifacts::const_iterator it = bag.begin(); it != bag.end(); ++it )
        append( *it );
}

void SpellStorage::append( const Artifact & artifact )
{
    switch ( artifact.GetID() ) {
    case Artifact::SPELL_SCROLL:
        append( Spell( artifact.GetSpell() ) );
        break;

    case Artifact::CRYSTAL_BALL:
        if ( Settings::Get().ExtWorldArtifactCrystalBall() ) {
            append( Spell( Spell::IDENTIFYHERO ) );
            append( Spell( Spell::VISIONS ) );
        }
        break;

    case Artifact::BATTLE_GARB:
        append( Spell( Spell::TOWNPORTAL ) );
        break;

    default:
        break;
    }
}

std::string SpellStorage::string() const
{
    std::string output;

    for ( const Spell & spell : _spells ) {
        if ( !output.empty() ) {
            output += ", ";
        }
        output += spell.GetName();
    }

    return output;
}

StreamBase & operator<<( StreamBase & msg, const SpellStorage & spellStorage )
{
    return msg << spellStorage._spells;
}

StreamBase & operator>>( StreamBase & msg, SpellStorage & spellStorage )
{
    return msg >> spellStorage._spells;
}
