/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct ListFiles;

namespace Audio
{
    // Audio initialization and deinitialization functions are not designed to be called
    // concurrently from different threads. They should be called from the main thread only.
    void Init();
    void Quit();

    void Mute();
    void Unmute();

    bool isValid();
}

namespace Mixer
{
    void SetChannels( const int num );

    int getChannelCount();

    // Starts playback of the given sound with the ability of looping it, as well as (optionally)
    // the ability to specify the position of the sound source relative to the listener (the angle
    // of direction to the sound source in degrees and the distance to the sound source).
    int Play( const uint8_t * ptr, const uint32_t size, const bool loop, const std::optional<std::pair<int16_t, uint8_t>> position = {} );

    void setVolume( const int volumePercentage );

    // Sets the position of the sound source relative to the listener (the angle of direction to
    // the sound source in degrees and the distance to the sound source) for the given channel.
    void setPosition( const int channelId, const int16_t angle, const uint8_t distance );

    void Stop( const int channelId = -1 );

    bool isPlaying( const int channelId );
}

namespace Music
{
    enum class PlaybackMode : uint8_t
    {
        // Play a music track once from the beginning, do not remember the position at which
        // playback of this track was stopped (rewind it to the beginning when playback stops)
        PLAY_ONCE,
        // Play a music track in an endless loop, starting from its previously remembered position
        // (or from the beginning if this track is being played for the first time) and remember
        // the position at which playback of this track was stopped
        RESUME_AND_PLAY_INFINITE,
        // Play a music track in an endless loop, starting from the beginning, do not remember
        // the position at which playback of this track was stopped (rewind it to the beginning
        // when playback stops)
        REWIND_AND_PLAY_INFINITE
    };

    // Music UID is used to store metadata of music tracks in the music database. It is caller's
    // responsibility to generate them. This function searches for a music track with the specified
    // UID in the database and starts playback if it is found. Returns true if the music track was
    // found in the database, otherwise returns false.
    bool Play( const uint64_t musicUID, const PlaybackMode playbackMode );

    // Adds a music track from the memory buffer to the music database and starts playback. A music
    // track with the specified UID should not already be present in the database.
    void Play( const uint64_t musicUID, const std::vector<uint8_t> & v, const PlaybackMode playbackMode );

    // Adds the music track available in the specified file to the music database and starts playback.
    // A music track with the specified UID should not already be present in the database.
    void Play( const uint64_t musicUID, const std::string & file, const PlaybackMode playbackMode );

    void setVolume( const int volumePercentage );

    void SetFadeInMs( const int timeMs );

    void Stop();

    bool isPlaying();

    void SetMidiSoundFonts( const ListFiles & files );

    std::vector<uint8_t> Xmi2Mid( const std::vector<uint8_t> & buf );
}

#endif
