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

#include <vector>

#include "types.h"

#include <SDL_mixer.h>

namespace Mixer
{
    typedef Mix_Chunk chunk_t;

    void FreeChunk( chunk_t * );
    chunk_t * LoadWAV( const char * );
    chunk_t * LoadWAV( const u8 *, u32 );

    int Play( chunk_t *, int, bool );
    int Play( const char *, int = -1, bool = false );
    int Play( const u8 *, u32, int = -1, bool = false );

    void SetChannels( int num );
    int MaxVolume();
    int Volume( int channel, int vol = -1 );

    void Pause( int channel = -1 );
    void Resume( int channel = -1 );
    void Stop( int channel = -1 );
    void Reset( void );

    bool isPlaying( int channel );
    bool isPaused( int channel );
    bool isValid( void );

    void Reduce( void );
    void Enhance( void );

    void Mute();
    void Unmute();
}

#endif
