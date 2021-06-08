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
#include <numeric>
#include <vector>

#include "audio.h"
#include "audio_cdrom.h"
#include "audio_mixer.h"
#include "audio_music.h"
#include "engine.h"
#include "logging.h"

#include <SDL_mixer.h>

namespace Mixer
{
    void Init( void );
    void Quit( void );
}

namespace
{
    bool valid = false;

    bool muted = false;
    std::vector<int> savedVolumes;

    void FreeChannel( int channel )
    {
        Mix_Chunk * sample = Mix_GetChunk( channel );
        if ( sample )
            Mix_FreeChunk( sample );
    }

    Mix_Chunk * LoadWAV( const char * file )
    {
        Mix_Chunk * sample = Mix_LoadWAV( file );
        if ( !sample )
            ERROR_LOG( SDL_GetError() );
        return sample;
    }

    Mix_Chunk * LoadWAV( const u8 * ptr, u32 size )
    {
        Mix_Chunk * sample = Mix_LoadWAV_RW( SDL_RWFromConstMem( ptr, size ), 1 );
        if ( !sample )
            ERROR_LOG( SDL_GetError() );
        return sample;
    }

    int PlayChunk( Mix_Chunk * sample, int channel, bool loop )
    {
        int res = Mix_PlayChannel( channel, sample, loop ? -1 : 0 );
        if ( res == -1 )
            ERROR_LOG( SDL_GetError() );
        return res;
    }
}

void Mixer::Init( void )
{
    if ( SDL::SubSystem( SDL_INIT_AUDIO ) ) {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        Mix_Init( MIX_INIT_OGG | MIX_INIT_MP3 | MIX_INIT_MOD );
#endif
        Audio::Spec & hardware = Audio::GetHardwareSpec();
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
        ERROR_LOG( "audio subsystem not initialize" );
        valid = false;
    }
}

void Mixer::Quit( void )
{
    if ( SDL::SubSystem( SDL_INIT_AUDIO ) && valid ) {
        Music::Reset();
        Mixer::Reset();
        valid = false;
        Mix_CloseAudio();
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        Mix_Quit();
#endif
    }
}

void Mixer::SetChannels( int num )
{
    if ( !valid ) {
        return;
    }

    const size_t channelsCount = static_cast<size_t>( Mix_AllocateChannels( num ) );

    if ( channelsCount > 0 ) {
        Mix_ReserveChannels( 1 );
    }

    if ( muted ) {
        savedVolumes.resize( channelsCount, MIX_MAX_VOLUME );

        Mix_Volume( -1, 0 );
    }
}

int Mixer::Play( const char * file, int channel, bool loop )
{
    if ( valid ) {
        Mix_Chunk * sample = LoadWAV( file );
        if ( sample ) {
            Mix_ChannelFinished( FreeChannel );
            return PlayChunk( sample, channel, loop );
        }
    }
    return -1;
}

int Mixer::Play( const u8 * ptr, u32 size, int channel, bool loop )
{
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

int Mixer::Volume( int channel, int vol /* = -1 */ )
{
    if ( !valid ) {
        return 0;
    }

    if ( vol > MIX_MAX_VOLUME ) {
        vol = MIX_MAX_VOLUME;
    }

    if ( muted ) {
        if ( channel < 0 ) {
            if ( savedVolumes.empty() ) {
                return 0;
            }

            // return the average volume
            const int prevVolume = std::accumulate( savedVolumes.begin(), savedVolumes.end(), 0 ) / static_cast<int>( savedVolumes.size() );

            if ( vol >= 0 ) {
                std::fill( savedVolumes.begin(), savedVolumes.end(), vol );
            }

            return prevVolume;
        }

        const size_t channelNum = static_cast<size_t>( channel );

        if ( channelNum >= savedVolumes.size() ) {
            return 0;
        }

        const int prevVolume = savedVolumes[channelNum];

        if ( vol >= 0 ) {
            savedVolumes[channelNum] = vol;
        }

        return prevVolume;
    }

    return Mix_Volume( channel, vol );
}

void Mixer::Pause( int channel /* = -1 */ )
{
    Mix_Pause( channel );
}

void Mixer::Resume( int channel /* = -1 */ )
{
    Mix_Resume( channel );
}

void Mixer::Stop( int channel /* = -1 */ )
{
    Mix_HaltChannel( channel );
}

void Mixer::Reset()
{
    Music::Reset();
#ifdef WITH_AUDIOCD
    if ( Cdrom::isValid() )
        Cdrom::Pause();
#endif
    Mix_HaltChannel( -1 );
}

bool Mixer::isPlaying( int channel )
{
    return Mix_Playing( channel ) > 0;
}

bool Mixer::isPaused( int channel )
{
    return Mix_Paused( channel ) > 0;
}

bool Mixer::isValid()
{
    return valid;
}

void Mixer::Reduce() {}

void Mixer::Enhance() {}

void Mixer::Mute()
{
    if ( muted || !valid ) {
        return;
    }

    muted = true;

    const size_t channelsCount = static_cast<size_t>( Mix_AllocateChannels( -1 ) );

    savedVolumes.resize( channelsCount );

    for ( size_t channel = 0; channel < channelsCount; ++channel ) {
        savedVolumes[channel] = Mix_Volume( static_cast<int>( channel ), 0 );
    }
}

void Mixer::Unmute()
{
    if ( !muted || !valid ) {
        return;
    }

    muted = false;

    const size_t channelsCount = std::min( static_cast<size_t>( Mix_AllocateChannels( -1 ) ), savedVolumes.size() );

    for ( size_t channel = 0; channel < channelsCount; ++channel ) {
        Mix_Volume( static_cast<int>( channel ), savedVolumes[channel] );
    }
}
