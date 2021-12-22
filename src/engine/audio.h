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

#ifndef H2AUDIO_H
#define H2AUDIO_H

#include <cstdint>
#include <string>
#include <vector>

namespace Audio
{
    void Init();
    void Quit();

    void Mute();
    void Unmute();

    bool isValid();
}

namespace Mixer
{
    void SetChannels( const int num );

    size_t getChannelCount();

    int Play( const std::string & file, const int channel = -1, const bool loop = false );
    int Play( const uint8_t * ptr, const uint32_t size, const int channel = -1, const bool loop = false );

    int MaxVolume();
    int Volume( const int channel, int vol );

    void Pause( const int channel = -1 );
    void Resume( const int channel = -1 );
    void Stop( const int channel = -1 );
    void Reset();

    bool isPlaying( const int channel );
}

namespace Music
{
    void Play( const std::vector<uint8_t> & v, const bool loop );
    void Play( const std::string & file, const bool loop );

    int Volume( int vol );

    void SetFadeIn( const int f );

    void Pause();
    void Reset();

    bool isPlaying();

    std::vector<uint8_t> Xmi2Mid( const std::vector<uint8_t> & buf );
}

#endif
