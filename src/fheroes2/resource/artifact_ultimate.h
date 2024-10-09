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

#ifndef H2ARTIFACT_ULTIMATE_H
#define H2ARTIFACT_ULTIMATE_H

#include <cstdint>

#include "artifact.h"
#include "image.h"
#include "math_base.h"

class IStreamBase;
class OStreamBase;

class UltimateArtifact : public Artifact
{
public:
    UltimateArtifact();

    bool isPosition( const int32_t position ) const;

    int32_t getPosition() const
    {
        return _index;
    }

    bool isFound() const
    {
        return _isFound;
    }

    void markAsFound()
    {
        _isFound = true;
    }

    void Set( const int32_t position, const Artifact & );
    void Reset();

    fheroes2::Image GetPuzzleMapSurface() const;
    const Artifact & GetArtifact() const;

private:
    friend OStreamBase & operator<<( OStreamBase & stream, const UltimateArtifact & ultimate );
    friend IStreamBase & operator>>( IStreamBase & stream, UltimateArtifact & ultimate );

    fheroes2::Point _offset;
    int32_t _index;
    bool _isFound;
};

#endif
