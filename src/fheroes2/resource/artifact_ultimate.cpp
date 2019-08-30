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

#include "maps.h"
#include "interface_gamearea.h"
#include "artifact_ultimate.h"

UltimateArtifact::UltimateArtifact() : index(-1), isfound(false)
{
}

void UltimateArtifact::Set(s32 pos, const Artifact & a)
{
    Artifact & art = *this;
    art = a.isValid() ? a : Artifact::Rand(Artifact::ART_ULTIMATE);
    index = pos;
    isfound = false;

    MakeSurface();
}

const Surface & UltimateArtifact::GetPuzzleMapSurface(void) const
{
    return puzzlemap;
}

const Artifact & UltimateArtifact::GetArtifact(void) const
{
    return *this;
}

bool UltimateArtifact::isFound(void) const
{
    return isfound;
}

void UltimateArtifact::SetFound(bool f)
{
    isfound = f;
}

bool UltimateArtifact::isPosition(s32 pos) const
{
    return 0 <= index && pos == index;
}

void UltimateArtifact::Reset(void)
{
    Artifact::Reset();
    puzzlemap.Reset();
    index = -1;
    isfound = false;
}

void UltimateArtifact::MakeSurface(void)
{
    if(Maps::isValidAbsIndex(index))
	puzzlemap = Interface::GameArea::GenerateUltimateArtifactAreaSurface(index);
    else
	puzzlemap.Reset();
}

StreamBase & operator<< (StreamBase & msg, const UltimateArtifact & ultimate)
{
    return msg << static_cast<Artifact>(ultimate) << ultimate.index << ultimate.isfound;
}

StreamBase & operator>> (StreamBase & msg, UltimateArtifact & ultimate)
{
    Artifact & artifact = ultimate;
    msg >> artifact >> ultimate.index >> ultimate.isfound;

    ultimate.MakeSurface();

    return msg;
}
