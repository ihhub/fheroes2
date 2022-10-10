/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "audio.h"
#include "dir.h"

namespace M82
{
    enum SoundType : int;
}

namespace AudioManager
{
    class AudioInitializer
    {
    public:
        AudioInitializer() = delete;

        AudioInitializer( const std::string & originalAGGFilePath, const std::string & expansionAGGFilePath, const ListFiles & midiSoundFonts );
        AudioInitializer( const AudioInitializer & ) = delete;
        AudioInitializer & operator=( const AudioInitializer & ) = delete;

        ~AudioInitializer();
    };

    // Useful for restoring background music after playing short-term music effects.
    // Please note that this class will attempt to restore playback of the music track
    // that was last requested to play at the time of initialization of an instance of
    // this class, which is not necessarily the same music track that was actually
    // playing at the time.
    class MusicRestorer
    {
    public:
        MusicRestorer();
        MusicRestorer( const MusicRestorer & ) = delete;

        ~MusicRestorer();

        MusicRestorer & operator=( const MusicRestorer & ) = delete;

    private:
        const int _music;
    };

    struct AudioLoopEffectInfo
    {
        AudioLoopEffectInfo() = default;

        AudioLoopEffectInfo( const int16_t angle_, const uint8_t volumePercentage_ )
            : angle( angle_ )
            , volumePercentage( volumePercentage_ )
        {
            // Do nothing.
        }

        bool operator==( const AudioLoopEffectInfo & other ) const
        {
            return other.angle == angle && other.volumePercentage == volumePercentage;
        }

        int16_t angle{ 0 };
        uint8_t volumePercentage{ 0 };
    };

    void playLoopSoundsAsync( std::map<M82::SoundType, std::vector<AudioLoopEffectInfo>> soundEffects );

    void PlaySound( const int m82 );
    void PlaySoundAsync( const int m82 );

    void PlayMusic( const int trackId, const Music::PlaybackMode playbackMode );
    void PlayMusicAsync( const int trackId, const Music::PlaybackMode playbackMode );

    // Assumes that the current music track is looped and should be resumed.
    //
    // TODO: Is subject to a (minor) race condition when called while the playback
    // TODO: of a new music track is being started in the AsyncSoundManager's worker
    // TODO: thread. In this case, the wrong music track (the one that is actually
    // TODO: being played at the moment, and not the one that is being prepared by
    // TODO: the worker thread for playback) may be played.
    void PlayCurrentMusic();

    void stopSounds();
    void ResetAudio();
}
