/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "artifact_ultimate.h"

#include "interface_gamearea.h"
#include "rand.h"
#include "serialize.h"

UltimateArtifact::UltimateArtifact()
    : _index( -1 )
    , _isFound( false )
{}

void UltimateArtifact::Set( const int32_t position, const Artifact & a )
{
    Artifact & art = *this;
    art = a.isValid() ? a : Artifact::Rand( Artifact::ART_ULTIMATE );
    _index = position;
    _isFound = false;

    // The Ultimate Artifact should not always be shown right in the center of the puzzle to make it
    // harder to find, but it should be shown behind one of the last four central pieces of the puzzle.
    _offset.x = Rand::Get( 0, 4 ) - 2;
    _offset.y = Rand::Get( 0, 2 ) - 1;
}

fheroes2::Image UltimateArtifact::GetPuzzleMapSurface() const
{
    return Interface::GameArea::GenerateUltimateArtifactAreaSurface( _index, _offset );
}

const Artifact & UltimateArtifact::GetArtifact() const
{
    return *this;
}

bool UltimateArtifact::isPosition( const int32_t position ) const
{
    return 0 <= _index && position == _index;
}

void UltimateArtifact::Reset()
{
    Artifact::Reset();

    _offset = fheroes2::Point();
    _index = -1;
    _isFound = false;
}

OStreamBase & operator<<( OStreamBase & stream, const UltimateArtifact & ultimate )
{
    return stream << static_cast<const Artifact &>( ultimate ) << ultimate._index << ultimate._isFound << ultimate._offset;
}

IStreamBase & operator>>( IStreamBase & stream, UltimateArtifact & ultimate )
{
    Artifact & artifact = ultimate;
    return stream >> artifact >> ultimate._index >> ultimate._isFound >> ultimate._offset;
}
