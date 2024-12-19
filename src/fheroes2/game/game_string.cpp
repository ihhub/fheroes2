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

#include "game_string.h"

#include "serialize.h"

OStreamBase & operator<<( OStreamBase & stream, const fheroes2::LocalizedString & string )
{
    stream << string.text;

    stream << string.language.has_value();

    if ( string.language.has_value() ) {
        stream << string.language.value();
    }

    return stream;
}

IStreamBase & operator>>( IStreamBase & stream, fheroes2::LocalizedString & string )
{
    stream >> string.text;

    bool hasValue = false;
    stream >> hasValue;

    if ( hasValue ) {
        fheroes2::SupportedLanguage language;

        stream >> language;
        string.language = language;
    }
    else {
        string.language.reset();
    }

    return stream;
}
