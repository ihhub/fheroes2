/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2008 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

    // To play the audio in a new channel set its value to -1. Returns channel ID. A negative value (-1) in case of failure.
    int Play( const uint8_t * ptr, const uint32_t size, const int channelId, const bool loop );
    int PlayFromDistance( const uint8_t * ptr, const uint32_t size, const int channelId, const bool loop, const int16_t angle, uint8_t volumePercentage );

    int applySoundEffect( const int channelId, const int16_t angle, uint8_t volumePercentage );

    // Returns the previous volume percentage value.
    int setVolume( const int channel, const int volumePercentage );

    void Pause( const int channel = -1 );
    void Resume( const int channel = -1 );
    void Stop( const int channel = -1 );

    bool isPlaying( const int channel );
}

namespace Music
{
    enum class PlaybackMode : uint8_t
    {
        PLAY_ONCE,
        CONTINUE_TO_PLAY_INFINITE,
        REWIND_AND_PLAY_INFINITE
    };

    // Music UID is used to cache existing songs. It is caller's responsibility to generate them.
    // This function return true in case of music track for corresponding Music UID is cached.
    bool Play( const uint64_t musicUID, const PlaybackMode playbackMode );

    // Load a music track from memory and play it.
    void Play( const uint64_t musicUID, const std::vector<uint8_t> & v, const PlaybackMode playbackMode );

    // Load a music track from a file system location and play it.
    void Play( const uint64_t musicUID, const std::string & file, const PlaybackMode playbackMode );

    // Returns the previous volume percentage value.
    int setVolume( const int volumePercentage );

    void SetFadeInMs( const int timeInMs );

    void Stop();

    bool isPlaying();

    std::vector<uint8_t> Xmi2Mid( const std::vector<uint8_t> & buf );
}

#endif
