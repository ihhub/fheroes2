/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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

#include "ai_personality.h"

#include "rand.h"
#include "serialize.h"
#include "translations.h"

AI::Personality AI::getRandomPersonality()
{
    return Rand::Get( Personality::WARRIOR, Personality::EXPLORER );
}

std::string AI::getPersonalityString( const Personality personality )
{
    switch ( personality ) {
    case Personality::WARRIOR:
        return _( "Warrior" );
    case Personality::BUILDER:
        return _( "Builder" );
    case Personality::EXPLORER:
        return _( "Explorer" );
    default:
        break;
    }

    return _( "None" );
}

OStreamBase & AI::operator<<( OStreamBase & stream, const Personality personality )
{
    using PersonalityUnderlyingType = std::underlying_type_t<decltype( personality )>;

    return stream << static_cast<PersonalityUnderlyingType>( personality );
}

IStreamBase & AI::operator>>( IStreamBase & stream, Personality & personality )
{
    using PersonalityUnderlyingType = std::underlying_type_t<std::remove_reference_t<decltype( personality )>>;

    PersonalityUnderlyingType temp = static_cast<PersonalityUnderlyingType>( Personality::NONE );
    stream >> temp;

    personality = static_cast<Personality>( temp );

    return stream;
}
