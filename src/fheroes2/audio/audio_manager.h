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

        AudioInitializer( const std::string & originalAGGFilePath, const std::string & expansionAGGFilePath );
        AudioInitializer( const AudioInitializer & ) = delete;
        AudioInitializer & operator=( const AudioInitializer & ) = delete;

        ~AudioInitializer();
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

    void playLoopSounds( std::map<M82::SoundType, std::vector<AudioLoopEffectInfo>> soundEffects, bool asyncronizedCall );
    void PlaySound( int m82, bool asyncronizedCall = false );

    enum class MusicPlaybackMode : uint8_t
    {
        PLAY_ONCE,
        CONTINUE_TO_PLAY_INFINITE,
        REWIND_AND_PLAY_INFINITE
    };

    void PlayMusic( const int trackId, const MusicPlaybackMode playbackMode );
    void PlayMusicAsync( const int trackId, const MusicPlaybackMode playbackMode );

    void ResetAudio();
}
