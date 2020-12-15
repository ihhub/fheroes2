/***************************************************************************
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2ARTIFACT_ULTIMATE_H
#define H2ARTIFACT_ULTIMATE_H

#include "artifact.h"
#include "gamedefs.h"

class UltimateArtifact : public Artifact
{
public:
    UltimateArtifact();

    bool isPosition( s32 ) const;
    bool isFound( void ) const;
    void SetFound( bool );
    void Set( s32, const Artifact & );
    void Reset( void );

    const fheroes2::Image & GetPuzzleMapSurface( void ) const;
    const Artifact & GetArtifact( void ) const;

private:
    friend StreamBase & operator<<( StreamBase &, const UltimateArtifact & );
    friend StreamBase & operator>>( StreamBase &, UltimateArtifact & );

    void MakeSurface( void );

    fheroes2::Image puzzlemap;
    s32 index;
    bool isfound;
};

StreamBase & operator<<( StreamBase &, const UltimateArtifact & );
StreamBase & operator>>( StreamBase &, UltimateArtifact & );

#endif
