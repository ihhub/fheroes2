/***************************************************************************
 *   Copyright (C) 2008 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2AUDIO_MIXER_H
#define H2AUDIO_MIXER_H

#include "types.h"

namespace Mixer
{
    void SetChannels( int num );

    int Play( const char *, int = -1, bool = false );
    int Play( const u8 *, u32, int = -1, bool = false );

    int MaxVolume();
    int Volume( int channel, int vol = -1 );

    void Pause( int channel = -1 );
    void Resume( int channel = -1 );
    void Stop( int channel = -1 );
    void Reset();

    bool isPlaying( int channel );
    bool isPaused( int channel );
    bool isValid();

    void Reduce();
    void Enhance();

    void Mute();
    void Unmute();
}

#endif
