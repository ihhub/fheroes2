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
#include <map>
#include <numeric>
#include <set>

#include <SDL.h>
#include <SDL_mixer.h>

#include "audio.h"
#include "core.h"
#include "logging.h"
#include "system.h"
#include "thread.h"
#include "timing.h"

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
            channels = 2; // Support stereo audio.
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
                ERROR_LOG( "Failed to unregister all effects from channel " << channelId << ". The error: " << Mix_GetError() )
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
            ERROR_LOG( "Failed to create an audio chunk from memory. The error: " << Mix_GetError() )
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
            ERROR_LOG( "Failed to play an audio chunk for channel " << channelId << ". The error: " << Mix_GetError() )
        }

        return channelId;
    }

    void addSoundEffect( const int channelId, const int16_t angle, uint8_t volumePercentage )
    {
        if ( volumePercentage > 100 ) {
            volumePercentage = 100;
        }

        if ( Mix_SetPosition( channelId, angle, 255 * ( volumePercentage - 100 ) ) == 0 ) {
            ERROR_LOG( "Failed to apply sound effect for channel " << channelId << ". The error: " << Mix_GetError() )
        }
        else {
            audioEffectLedger.registerChannel( channelId );
        }
    }

    int PlayChunkFromDistance( Mix_Chunk * sample, const int channel, const bool loop, const int16_t angle, uint8_t volumePercentage )
    {
        assert( sample != nullptr );

        const int channelId = Mix_PlayChannel( channel, sample, loop ? -1 : 0 );

        if ( channelId < 0 ) {
            ERROR_LOG( "Failed to play a mix chunk for original channel " << channel << ". The error: " << Mix_GetError() )
            return channelId;
        }

        addSoundEffect( channelId, angle, volumePercentage );

        return channelId;
    }

    struct MusicInfo
    {
        Mix_Music * mix{ nullptr };

        double position{ 0 };
    };

    class MusicTrackManager
    {
    public:
        MusicInfo getMusicInfoByUID( const uint64_t musicUID ) const
        {
            auto iter = _musicCache.find( musicUID );
            if ( iter == _musicCache.end() ) {
                return {};
            }

            return iter->second;
        }

        void update( const uint64_t musicUID, Mix_Music * mix, const double position )
        {
            assert( mix != nullptr );

            auto iter = _musicCache.find( musicUID );
            if ( iter == _musicCache.end() ) {
                _musicCache.try_emplace( musicUID, MusicInfo{ mix, position } );
                return;
            }

            iter->second.mix = mix;
            iter->second.position = position;
        }

        void clear()
        {
            for ( auto & musicInfoPair : _musicCache ) {
                Mix_FreeMusic( musicInfoPair.second.mix );
            }

            _musicCache.clear();
        }

        double getCurrentTrackPosition() const
        {
            return _currentTrackTimer.get();
        }

        void resetTimer()
        {
            _currentTrackTimer.reset();
        }

    private:
        std::map<uint64_t, MusicInfo> _musicCache;

        fheroes2::Time _currentTrackTimer;
    };

    void PlayMusic( const uint64_t musicUID, bool loop );

    void replayCurrentMusic();

    class AsyncMusicManager : public MultiThreading::AsyncManager
    {
    public:
        void restartPlayback()
        {
            _createThreadIfNeeded();

            std::lock_guard<std::mutex> mutexLock( _mutex );

            notifyThread();
        }

    protected:
        void doStuff() override
        {
            replayCurrentMusic();

            _runFlag = 0;
            _mutex.unlock();
        }
    };

    struct MusicSettings
    {
        MusicInfo currentTrack;
        uint64_t currentTrackUID{ 0 };
        uint64_t asyncTrackUID{ 0 };

        int fadeInMs{ 0 };
        int fadeOutMs{ 0 };

        MusicTrackManager trackManager;

        AsyncMusicManager asyncManager;
    };

    MusicSettings musicSettings;

    bool isMusicResumeSupported( const Mix_Music * mix )
    {
        assert( mix != nullptr );

        const Mix_MusicType musicType = Mix_GetMusicType( mix );

        return ( musicType == Mix_MusicType::MUS_OGG ) || ( musicType == Mix_MusicType::MUS_MP3 );
    }

    void replayCurrentMusic()
    {
        const std::lock_guard<std::recursive_mutex> guard( mutex );

        if ( musicSettings.asyncTrackUID != musicSettings.currentTrackUID ) {
            // Looks like the track was changed.
            return;
        }

        musicSettings.trackManager.resetTimer();

        musicSettings.currentTrack.position = 0;
        musicSettings.trackManager.update( musicSettings.currentTrackUID, musicSettings.currentTrack.mix, 0 );

        PlayMusic( musicSettings.currentTrackUID, true );
    }

    void PlayMusic( const uint64_t musicUID, bool loop )
    {
        MusicInfo musicInfo = musicSettings.trackManager.getMusicInfoByUID( musicUID );
        if ( musicInfo.mix == nullptr ) {
            // How is it even possible! Check your logic!
            assert( 0 );
            return;
        }

        if ( musicUID != musicSettings.currentTrackUID ) {
            Music::Stop();
        }

        const bool isResumeSupported = loop && isMusicResumeSupported( musicInfo.mix );
        if ( isResumeSupported ) {
            loop = false;
            musicSettings.asyncTrackUID = musicUID;

            // Mix_HookMusicFinished() function does not allow any SDL calls to be done within the assigned function. In this case the only to trigger restart of the
            // current song it using multithreading.
            Mix_HookMusicFinished( []() { musicSettings.asyncManager.restartPlayback(); } );
        }
        else {
            musicSettings.asyncTrackUID = 0;
        }

        musicSettings.trackManager.resetTimer();

        const int loopCount = loop ? -1 : 0;
        const int returnCode
            = ( musicSettings.fadeInMs > 0 ) ? Mix_FadeInMusic( musicInfo.mix, loopCount, musicSettings.fadeInMs ) : Mix_PlayMusic( musicInfo.mix, loopCount );
        if ( returnCode != 0 ) {
            ERROR_LOG( "Failed to play music mix. The error: " << Mix_GetError() )
            return;
        }

        if ( isResumeSupported && musicInfo.position > 1 ) {
            // Set music position only when at least 1 second of the music has been played.
            Mix_SetMusicPosition( musicInfo.position );
        }

        musicSettings.currentTrack = musicInfo;
        musicSettings.currentTrackUID = musicUID;
    }

    int normalizeToSDLVolume( const int volumePercentage )
    {
        if ( volumePercentage < 0 ) {
            // Why are you passing a negative volume value?
            assert( 0 );
            return 0;
        }

        if ( volumePercentage >= 100 ) {
            return MIX_MAX_VOLUME;
        }

        return volumePercentage * MIX_MAX_VOLUME / 100;
    }

    int normalizeFromSDLVolume( const int volume )
    {
        return volume * 100 / MIX_MAX_VOLUME;
    }
}

void Audio::Init()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( isInitialized ) {
        // If this assertion blows up you are trying to initialize an already initialized system.
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
        ERROR_LOG( "Failed to initialize an audio device. The error: " << Mix_GetError() )
        return;
    }

    int channels = 0;
    int frequency = 0;
    uint16_t format = 0;
    const int deviceInitCount = Mix_QuerySpec( &frequency, &format, &channels );
    if ( deviceInitCount == 0 ) {
        ERROR_LOG( "Failed to query an audio device specs. The error: " << Mix_GetError() )
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

    musicSettings.trackManager.clear();

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

    const int channelsCount = Mix_AllocateChannels( num );
    if ( num != channelsCount ) {
        ERROR_LOG( "Failed to allocate the required amount of channels for sound. The required number of channels " << num << " but allocated only " << channelsCount )
    }

    if ( channelsCount > 0 ) {
        Mix_ReserveChannels( 1 );
    }

    if ( muted ) {
        savedMixerVolumes.resize( static_cast<size_t>( channelsCount ), 0 );

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
    if ( ptr == nullptr || size == 0 ) {
        // You are trying to play an empty sound. Check your logic!
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
    if ( ptr == nullptr || size == 0 ) {
        // You are trying to play an empty sound. Check your logic!
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

int Mixer::setVolume( const int channel, const int volumePercentage )
{
    const int volume = normalizeToSDLVolume( volumePercentage );

    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !isInitialized ) {
        return 0;
    }

    if ( !muted ) {
        return normalizeFromSDLVolume( Mix_Volume( channel, volume ) );
    }

    if ( channel < 0 ) {
        if ( savedMixerVolumes.empty() ) {
            return 0;
        }

        // return the average volume
        const int prevVolume = std::accumulate( savedMixerVolumes.begin(), savedMixerVolumes.end(), 0 ) / static_cast<int>( savedMixerVolumes.size() );
        std::fill( savedMixerVolumes.begin(), savedMixerVolumes.end(), volume );

        return normalizeFromSDLVolume( prevVolume );
    }

    const size_t channelNum = static_cast<size_t>( channel );

    if ( channelNum >= savedMixerVolumes.size() ) {
        return 0;
    }

    const int prevVolume = savedMixerVolumes[channelNum];
    savedMixerVolumes[channelNum] = volume;

    return normalizeFromSDLVolume( prevVolume );
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

void Music::Play( const uint64_t musicUID, const std::vector<uint8_t> & v, const bool loop )
{
    if ( v.empty() ) {
        return;
    }

    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !isInitialized ) {
        return;
    }

    if ( musicSettings.currentTrackUID == musicUID ) {
        // We are playing the same track. No need to do anything.
        return;
    }

    const MusicInfo musicInfo = musicSettings.trackManager.getMusicInfoByUID( musicUID );
    if ( musicInfo.mix != nullptr ) {
        PlayMusic( musicUID, loop );
        return;
    }

    SDL_RWops * rwops = SDL_RWFromConstMem( &v[0], static_cast<int>( v.size() ) );
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    Mix_Music * mix = Mix_LoadMUS_RW( rwops, 0 );
#else
    Mix_Music * mix = Mix_LoadMUS_RW( rwops );
#endif
    SDL_FreeRW( rwops );

    if ( !mix ) {
        ERROR_LOG( "Failed to create a music mix from memory. The error: " << Mix_GetError() )
        return;
    }

    musicSettings.trackManager.update( musicUID, mix, 0 );
    PlayMusic( musicUID, loop );
}

void Music::Play( const uint64_t musicUID, const std::string & file, const bool loop )
{
    if ( file.empty() ) {
        // Nothing to play. It is an empty file.
        return;
    }

    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !isInitialized ) {
        return;
    }

    if ( musicSettings.currentTrackUID == musicUID ) {
        // We are playing the same track. No need to do anything.
        return;
    }

    const MusicInfo musicInfo = musicSettings.trackManager.getMusicInfoByUID( musicUID );
    if ( musicInfo.mix != nullptr ) {
        PlayMusic( musicUID, loop );
        return;
    }

    const std::string filePath = System::FileNameToUTF8( file );

    Mix_Music * mix = Mix_LoadMUS( filePath.c_str() );
    if ( !mix ) {
        ERROR_LOG( "Failed to create a music mix from path " << filePath << ". The error: " << Mix_GetError() )
        return;
    }

    musicSettings.trackManager.update( musicUID, mix, 0 );
    PlayMusic( musicUID, loop );
}

void Music::SetFadeInMs( const int timeInMs )
{
    if ( timeInMs < 0 ) {
        // Why are you even setting a negative value?
        assert( 0 );
        return;
    }

    const std::lock_guard<std::recursive_mutex> guard( mutex );

    musicSettings.fadeInMs = timeInMs;
}

int Music::setVolume( const int volumePercentage )
{
    const int volume = normalizeToSDLVolume( volumePercentage );

    const std::lock_guard<std::recursive_mutex> guard( mutex );

    if ( !isInitialized ) {
        return 0;
    }

    if ( muted ) {
        const int prevVolume = savedMusicVolume;

        savedMusicVolume = volume;

        return normalizeFromSDLVolume( prevVolume );
    }

    return normalizeFromSDLVolume( Mix_VolumeMusic( volume ) );
}

void Music::Stop()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );
    if ( musicSettings.currentTrack.mix == nullptr ) {
        // Nothing to do.
        return;
    }

    if ( musicSettings.fadeOutMs > 0 ) {
        while ( !Mix_FadeOutMusic( musicSettings.fadeOutMs ) && Mix_PlayingMusic() ) {
            SDL_Delay( 50 );
        }
    }
    else {
        // According to the documentation (https://www.libsdl.org/projects/SDL_mixer/docs/SDL_mixer.html#SEC67) this function always returns 0.
        Mix_HaltMusic();
    }

    if ( musicSettings.currentTrackUID == musicSettings.asyncTrackUID ) {
        const double position = musicSettings.trackManager.getCurrentTrackPosition();
        musicSettings.currentTrack.position += position;

        musicSettings.trackManager.update( musicSettings.currentTrackUID, musicSettings.currentTrack.mix, musicSettings.currentTrack.position );
    }

    musicSettings.currentTrack = {};
}

bool Music::isPlaying()
{
    const std::lock_guard<std::recursive_mutex> guard( mutex );

    return ( musicSettings.currentTrack.mix != nullptr ) && Mix_PlayingMusic();
}
