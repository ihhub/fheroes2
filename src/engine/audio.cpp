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

#include <algorithm>
#include <atomic>
#include <mutex>
#include <numeric>

#include <SDL.h>
#include <SDL_mixer.h>

#include "audio.h"
#include "core.h"
#include "logging.h"

namespace
{
    struct Spec : public SDL_AudioSpec
    {
        Spec()
            : SDL_AudioSpec()
        {
            freq = 0;
            format = 0;
            channels = 0;
            silence = 0;
            samples = 0;
            size = 0;
            callback = nullptr;
            userdata = nullptr;
        }
    };

    Spec hardware;

    std::atomic<bool> valid{ false };
    bool muted = false;

    std::vector<int> savedMixerVolumes;
    int savedMusicVolume = 0;

    int musicFadeIn = 0;
    int musicFadeOut = 0;

    Mix_Music * music = nullptr;

    std::recursive_mutex mutex;

    void FreeChannel( const int channel )
    {
        Mix_Chunk * sample = Mix_GetChunk( channel );

        if ( sample ) {
            Mix_FreeChunk( sample );
        }
    }

    Mix_Chunk * LoadWAV( const char * file )
    {
        Mix_Chunk * sample = Mix_LoadWAV( file );

        if ( !sample ) {
            ERROR_LOG( SDL_GetError() );
        }

        return sample;
    }

    Mix_Chunk * LoadWAV( const uint8_t * ptr, const uint32_t size )
    {
        Mix_Chunk * sample = Mix_LoadWAV_RW( SDL_RWFromConstMem( ptr, size ), 1 );

        if ( !sample ) {
            ERROR_LOG( SDL_GetError() );
        }

        return sample;
    }

    int PlayChunk( Mix_Chunk * sample, const int channel, const bool loop )
    {
        int res = Mix_PlayChannel( channel, sample, loop ? -1 : 0 );

        if ( res == -1 ) {
            ERROR_LOG( SDL_GetError() );
        }

        return res;
    }

    void PlayMusic( Mix_Music * mix, const bool loop )
    {
        Music::Reset();

        int res = musicFadeIn ? Mix_FadeInMusic( mix, loop ? -1 : 0, musicFadeIn ) : Mix_PlayMusic( mix, loop ? -1 : 0 );

        if ( res < 0 ) {
            ERROR_LOG( Mix_GetError() );
        }
        else {
            music = mix;
        }
    }
}

void Audio::Init()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( fheroes2::isComponentInitialized( fheroes2::SystemInitializationComponent::Audio ) ) {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        Mix_Init( MIX_INIT_OGG | MIX_INIT_MP3 | MIX_INIT_MOD );
#endif
        hardware.freq = 22050;
        hardware.format = AUDIO_S16;
        hardware.channels = 2;
        hardware.samples = 2048;

        if ( 0 != Mix_OpenAudio( hardware.freq, hardware.format, hardware.channels, hardware.samples ) ) {
            ERROR_LOG( SDL_GetError() );

            valid = false;
        }
        else {
            int channels = 0;

            Mix_QuerySpec( &hardware.freq, &hardware.format, &channels );
            hardware.channels = channels;

            valid = true;
        }
    }
    else {
        ERROR_LOG( "The audio subsystem was not initialized." );

        valid = false;
    }
}

void Audio::Quit()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( valid && fheroes2::isComponentInitialized( fheroes2::SystemInitializationComponent::Audio ) ) {
        Music::Reset();
        Mixer::Reset();

        valid = false;

        Mix_CloseAudio();
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        Mix_Quit();
#endif
    }
}

void Audio::Mute()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( muted || !valid ) {
        return;
    }

    muted = true;

    const size_t channelsCount = static_cast<size_t>( Mix_AllocateChannels( -1 ) );

    savedMixerVolumes.resize( channelsCount );

    for ( size_t channel = 0; channel < channelsCount; ++channel ) {
        savedMixerVolumes[channel] = Mix_Volume( static_cast<int>( channel ), 0 );
    }

    savedMusicVolume = Mix_VolumeMusic( 0 );
}

void Audio::Unmute()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !muted || !valid ) {
        return;
    }

    muted = false;

    const size_t channelsCount = std::min( static_cast<size_t>( Mix_AllocateChannels( -1 ) ), savedMixerVolumes.size() );

    for ( size_t channel = 0; channel < channelsCount; ++channel ) {
        Mix_Volume( static_cast<int>( channel ), savedMixerVolumes[channel] );
    }

    Mix_VolumeMusic( savedMusicVolume );
}

bool Audio::isValid()
{
    return valid;
}

void Mixer::SetChannels( const int num )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !valid ) {
        return;
    }

    const size_t channelsCount = static_cast<size_t>( Mix_AllocateChannels( num ) );

    if ( channelsCount > 0 ) {
        Mix_ReserveChannels( 1 );
    }

    if ( muted ) {
        savedMixerVolumes.resize( channelsCount, MIX_MAX_VOLUME );

        Mix_Volume( -1, 0 );
    }
}

size_t Mixer::getChannelCount()
{
    if ( !valid ) {
        return 0;
    }

    return static_cast<size_t>( Mix_AllocateChannels( -1 ) );
}

int Mixer::Play( const char * file, const int channel /* = -1 */, const bool loop /* = false */ )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( valid ) {
        Mix_Chunk * sample = LoadWAV( file );
        if ( sample ) {
            Mix_ChannelFinished( FreeChannel );
            return PlayChunk( sample, channel, loop );
        }
    }

    return -1;
}

int Mixer::Play( const uint8_t * ptr, const uint32_t size, const int channel /* = -1 */, const bool loop /* = false */ )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( valid && ptr ) {
        Mix_Chunk * sample = LoadWAV( ptr, size );
        if ( sample ) {
            Mix_ChannelFinished( FreeChannel );
            return PlayChunk( sample, channel, loop );
        }
    }

    return -1;
}

int Mixer::MaxVolume()
{
    return MIX_MAX_VOLUME;
}

int Mixer::Volume( const int channel, int vol )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !valid ) {
        return 0;
    }

    if ( vol > MIX_MAX_VOLUME ) {
        vol = MIX_MAX_VOLUME;
    }

    if ( muted ) {
        if ( channel < 0 ) {
            if ( savedMixerVolumes.empty() ) {
                return 0;
            }

            // return the average volume
            const int prevVolume = std::accumulate( savedMixerVolumes.begin(), savedMixerVolumes.end(), 0 ) / static_cast<int>( savedMixerVolumes.size() );

            if ( vol >= 0 ) {
                std::fill( savedMixerVolumes.begin(), savedMixerVolumes.end(), vol );
            }

            return prevVolume;
        }

        const size_t channelNum = static_cast<size_t>( channel );

        if ( channelNum >= savedMixerVolumes.size() ) {
            return 0;
        }

        const int prevVolume = savedMixerVolumes[channelNum];

        if ( vol >= 0 ) {
            savedMixerVolumes[channelNum] = vol;
        }

        return prevVolume;
    }

    return Mix_Volume( channel, vol );
}

void Mixer::Pause( const int channel /* = -1 */ )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( valid ) {
        Mix_Pause( channel );
    }
}

void Mixer::Resume( const int channel /* = -1 */ )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( valid ) {
        Mix_Resume( channel );
    }
}

void Mixer::Stop( const int channel /* = -1 */ )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( valid ) {
        Mix_HaltChannel( channel );
    }
}

void Mixer::Reset()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    Music::Reset();

    if ( valid ) {
        Mix_HaltChannel( -1 );
    }
}

bool Mixer::isPlaying( const int channel )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    return valid && Mix_Playing( channel ) > 0;
}

void Music::Play( const std::vector<uint8_t> & v, const bool loop )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( valid && !v.empty() ) {
        SDL_RWops * rwops = SDL_RWFromConstMem( &v[0], static_cast<int>( v.size() ) );
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        Mix_Music * mix = Mix_LoadMUS_RW( rwops, 0 );
#else
        Mix_Music * mix = Mix_LoadMUS_RW( rwops );
#endif
        SDL_FreeRW( rwops );

        if ( !mix ) {
            ERROR_LOG( Mix_GetError() );
        }
        else {
            PlayMusic( mix, loop );
        }
    }
}

void Music::Play( const std::string & file, const bool loop )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( valid ) {
        Mix_Music * mix = Mix_LoadMUS( file.c_str() );

        if ( !mix ) {
            ERROR_LOG( Mix_GetError() );
        }
        else {
            PlayMusic( mix, loop );
        }
    }
}

void Music::SetFadeIn( const int f )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    musicFadeIn = f;
}

int Music::Volume( int vol )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !valid ) {
        return 0;
    }

    if ( vol > MIX_MAX_VOLUME ) {
        vol = MIX_MAX_VOLUME;
    }

    if ( muted ) {
        const int prevVolume = savedMusicVolume;

        if ( vol >= 0 ) {
            savedMusicVolume = vol;
        }

        return prevVolume;
    }

    return Mix_VolumeMusic( vol );
}

void Music::Pause()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( music ) {
        Mix_PauseMusic();
    }
}

void Music::Reset()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( music ) {
        if ( musicFadeOut ) {
            while ( !Mix_FadeOutMusic( musicFadeOut ) && Mix_PlayingMusic() ) {
                SDL_Delay( 50 );
            }
        }
        else {
            Mix_HaltMusic();
        }

        Mix_FreeMusic( music );
        music = nullptr;
    }
}

bool Music::isPlaying()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    return music && Mix_PlayingMusic();
}
