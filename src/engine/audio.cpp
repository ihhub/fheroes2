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

#include <algorithm>
#include <atomic>
#include <cassert>
#include <mutex>
#include <numeric>
#include <set>

#include <SDL.h>
#include <SDL_mixer.h>

#include "audio.h"
#include "core.h"
#include "logging.h"
#include "system.h"

namespace
{
    // TODO: This structure is not used anywhere else except Audio::Init(). Consider this structure to be removed or utilize it properly.
    struct Spec : public SDL_AudioSpec
    {
        Spec()
            : SDL_AudioSpec()
        {
            freq = 22050;
            format = AUDIO_S16;
            channels = 2; // Support tereo audio.
            silence = 0;
            samples = 2048;
            size = 0;
            // TODO: research if we need to utilize these 2 paremeters in the future.
            callback = nullptr;
            userdata = nullptr;
        }
    };

    Spec audioSpecs;

    std::atomic<bool> isInitialized{ false };
    bool muted = false;

    std::vector<int> savedMixerVolumes;
    int savedMusicVolume = 0;

    int musicFadeIn = 0;
    int musicFadeOut = 0;

    Mix_Music * music = nullptr;

    struct AudioEffectState
    {
        void registerChannel( const int channelId )
        {
            channels.emplace( channelId );
        }

        void unregisterChannel( const int channelId )
        {
            if ( channels.count( channelId ) == 0 ) {
                // The channel does not have any effects.
                return;
            }

            if ( Mix_UnregisterAllEffects( channelId ) == 0 ) {
                ERROR_LOG( "Unable to unregister all effects from channel " << channelId )
            }

            channels.erase( channelId );
        }

        std::set<int> channels;
    };

    AudioEffectState audioEffectLedger;

    std::recursive_mutex mutex;

    void FreeChannel( const int channel )
    {
        Mix_Chunk * sample = Mix_GetChunk( channel );

        if ( sample ) {
            Mix_FreeChunk( sample );
        }

        audioEffectLedger.unregisterChannel( channel );
    }

    Mix_Chunk * LoadWAV( const uint8_t * ptr, const uint32_t size )
    {
        Mix_Chunk * sample = Mix_LoadWAV_RW( SDL_RWFromConstMem( ptr, size ), 1 );
        if ( !sample ) {
            ERROR_LOG( "Unable to create audio chunk from memory. The error: " << Mix_GetError() )
        }

        return sample;
    }

    int PlayChunk( Mix_Chunk * sample, const int channel, const bool loop )
    {
        assert( sample != nullptr );

        if ( channel >= 0 ) {
            audioEffectLedger.unregisterChannel( channel );
        }

        const int channelId = Mix_PlayChannel( channel, sample, loop ? -1 : 0 );
        if ( channelId < 0 ) {
            ERROR_LOG( Mix_GetError() )
        }

        return channelId;
    }

    void addSoundEffect( const int channelId, const int16_t angle, uint8_t volumePercentage )
    {
        if ( volumePercentage > 100 ) {
            volumePercentage = 100;
        }

        if ( !Mix_SetPosition( channelId, angle, 255 * ( volumePercentage - 100 ) ) ) {
            ERROR_LOG( Mix_GetError() )
        }
        else {
            audioEffectLedger.registerChannel( channelId );
        }
    }

    int PlayChunkFromDistance( Mix_Chunk * sample, const int channel, const bool loop, const int16_t angle, uint8_t volumePercentage )
    {
        assert( sample != nullptr );

        const int channelId = Mix_PlayChannel( channel, sample, loop ? -1 : 0 );

        if ( channelId == -1 ) {
            ERROR_LOG( Mix_GetError() )
            return channelId;
        }

        addSoundEffect( channelId, angle, volumePercentage );

        return channelId;
    }

    void PlayMusic( Mix_Music * mix, const bool loop )
    {
        // TODO: According to SDL documentation (https://www.libsdl.org/projects/SDL_mixer/docs/SDL_mixer.html) we could have a playback of a song from a particular
        //       position:
        //       - Mix_HookMusicFinished() is a hook function which is called at the end of a song. Please note that this function will not be called for looped music.
        //       - Mix_SetMusicPosition() accepts position in seconds for MP3 and OGG formats
        //       - Mix_GetMusicType() returns the type of music
        //       How a possible implementation should work: detect the type of music. If it is MP3 or OGG add a hook function and play the music track once. After the
        //       completion of the track the function will be called. Within the function inform a separate thread about this which will restart the music track again.

        Music::Stop();

        const int returnCode = musicFadeIn ? Mix_FadeInMusic( mix, loop ? -1 : 0, musicFadeIn ) : Mix_PlayMusic( mix, loop ? -1 : 0 );
        if ( returnCode != 0 ) {
            ERROR_LOG( Mix_GetError() )
            return;
        }

        music = mix;
    }
}

void Audio::Init()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( isInitialized ) {
        // If this assertion blows up you are trying to initialize already initialized system.
        assert( 0 );
        return;
    }

    if ( !fheroes2::isComponentInitialized( fheroes2::SystemInitializationComponent::Audio ) ) {
        ERROR_LOG( "The audio subsystem was not initialized." )
        return;
    }

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    const int initializationFlags = MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG;
    const int initializedFlags = Mix_Init( initializationFlags );
    if ( ( initializedFlags & initializationFlags ) != initializationFlags ) {
        DEBUG_LOG( DBG_ENGINE, DBG_WARN,
                   "Expected music initialization flags as " << initializationFlags << " but received " << ( initializedFlags & initializationFlags ) )

        if ( ( initializedFlags & MIX_INIT_FLAC ) == 0 ) {
            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "Flac module failed to be initialized" )
        }

        if ( ( initializedFlags & MIX_INIT_MOD ) == 0 ) {
            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "Mod module failed to be initialized" )
        }

        if ( ( initializedFlags & MIX_INIT_MP3 ) == 0 ) {
            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "Mp3 module failed to be initialized" )
        }

        if ( ( initializedFlags & MIX_INIT_OGG ) == 0 ) {
            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "Ogg module failed to be initialized" )
        }
    }
#endif

    if ( Mix_OpenAudio( audioSpecs.freq, audioSpecs.format, audioSpecs.channels, audioSpecs.samples ) != 0 ) {
        ERROR_LOG( Mix_GetError() )
        return;
    }

    int channels = 0;
    int frequency = 0;
    uint16_t format = 0;
    const int deviceInitCount = Mix_QuerySpec( &frequency, &format, &channels );
    if ( deviceInitCount == 0 ) {
        ERROR_LOG( "Failed to initialize an audio system. The error: " << Mix_GetError() )
    }

    if ( deviceInitCount != 1 ) {
        // The device must be opened only once.
        assert( 0 );
        ERROR_LOG( "Trying to initialize an audio system that has been already initialized." )
    }

    if ( audioSpecs.freq != frequency ) {
        ERROR_LOG( "Audio frequency is initialized as " << frequency << " instead of " << audioSpecs.freq )
    }

    if ( audioSpecs.format != format ) {
        ERROR_LOG( "Audio format is initialized as " << format << " instead of " << audioSpecs.format )
    }

    // If this assertion blows up it means that SDL doesn't work properly.
    assert( channels >= 0 && channels < 256 );

    audioSpecs.freq = frequency;
    audioSpecs.format = format;
    audioSpecs.channels = static_cast<uint8_t>( channels );

    isInitialized = true;
}

void Audio::Quit()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );
    if ( !isInitialized ) {
        // Nothing to do.
        return;
    }

    if ( !fheroes2::isComponentInitialized( fheroes2::SystemInitializationComponent::Audio ) ) {
        // Something wrong with the logic! The component must be initialized.
        assert( 0 );
        return;
    }

    Music::Stop();
    Mixer::Stop();

    Mix_CloseAudio();
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    Mix_Quit();
#endif

    isInitialized = false;
}

void Audio::Mute()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( muted || !isInitialized ) {
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

    if ( !muted || !isInitialized ) {
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
    return isInitialized;
}

void Mixer::SetChannels( const int num )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !isInitialized ) {
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
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !isInitialized ) {
        return 0;
    }

    return static_cast<size_t>( Mix_AllocateChannels( -1 ) );
}

int Mixer::Play( const uint8_t * ptr, const uint32_t size, const int channelId, const bool loop )
{
    if ( ptr == nullptr ) {
        // You are trying to play an empty file. Check your logic!
        assert( 0 );
        return -1;
    }

    const std::lock_guard<std::recursive_mutex> guard( mutex );
    if ( !isInitialized ) {
        return -1;
    }

    Mix_Chunk * sample = LoadWAV( ptr, size );
    if ( sample == nullptr ) {
        return -1;
    }

    Mix_ChannelFinished( FreeChannel );
    return PlayChunk( sample, channelId, loop );
}

int Mixer::PlayFromDistance( const uint8_t * ptr, const uint32_t size, const int channelId, const bool loop, const int16_t angle, uint8_t volumePercentage )
{
    if ( ptr == nullptr ) {
        // You are trying to play an empty file. Check your logic!
        assert( 0 );
        return -1;
    }

    const std::lock_guard<std::recursive_mutex> guard( mutex );
    if ( !isInitialized ) {
        return -1;
    }

    Mix_Chunk * sample = LoadWAV( ptr, size );
    if ( sample == nullptr ) {
        return -1;
    }

    Mix_ChannelFinished( FreeChannel );
    return PlayChunkFromDistance( sample, channelId, loop, angle, volumePercentage );
}

int Mixer::applySoundEffect( const int channelId, const int16_t angle, uint8_t volumePercentage )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );
    if ( !isInitialized ) {
        return -1;
    }

    addSoundEffect( channelId, angle, volumePercentage );

    return channelId;
}

int Mixer::MaxVolume()
{
    return MIX_MAX_VOLUME;
}

int Mixer::Volume( const int channel, int vol )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !isInitialized ) {
        return 0;
    }

    if ( vol > MIX_MAX_VOLUME ) {
        vol = MIX_MAX_VOLUME;
    }

    if ( !muted ) {
        return Mix_Volume( channel, vol );
    }

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

void Mixer::Pause( const int channel /* = -1 */ )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( isInitialized ) {
        Mix_Pause( channel );
    }
}

void Mixer::Resume( const int channel /* = -1 */ )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( isInitialized ) {
        Mix_Resume( channel );
    }
}

void Mixer::Stop( const int channel /* = -1 */ )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( isInitialized ) {
        Mix_HaltChannel( channel );
    }
}

bool Mixer::isPlaying( const int channel )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    return isInitialized && Mix_Playing( channel ) > 0;
}

void Music::Play( const std::vector<uint8_t> & v, const bool loop )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( isInitialized && !v.empty() ) {
        SDL_RWops * rwops = SDL_RWFromConstMem( &v[0], static_cast<int>( v.size() ) );
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        Mix_Music * mix = Mix_LoadMUS_RW( rwops, 0 );
#else
        Mix_Music * mix = Mix_LoadMUS_RW( rwops );
#endif
        SDL_FreeRW( rwops );

        if ( !mix ) {
            ERROR_LOG( Mix_GetError() )
        }
        else {
            PlayMusic( mix, loop );
        }
    }
}

void Music::Play( const std::string & file, const bool loop )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !isInitialized ) {
        return;
    }

    Mix_Music * mix = Mix_LoadMUS( System::FileNameToUTF8( file ).c_str() );
    if ( !mix ) {
        ERROR_LOG( Mix_GetError() )
        return;
    }

    PlayMusic( mix, loop );
}

void Music::SetFadeIn( const int f )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    musicFadeIn = f;
}

int Music::Volume( int vol )
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !isInitialized ) {
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

void Music::Stop()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );
    if ( music == nullptr ) {
        // Nothing to do.
        return;
    }

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

bool Music::isPlaying()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    return music && Mix_PlayingMusic();
}
