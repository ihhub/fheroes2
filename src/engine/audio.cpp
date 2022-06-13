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
#include <mutex>
#include <numeric>

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
    bool isMuted = false;

    std::vector<int> savedMixerVolumes;
    int savedMusicVolume = 0;

    // This mutex protects all operations with audio. In order to avoid deadlocks, it shouldn't
    // be acquired in any callback functions that can be called by SDL_Mixer.
    std::recursive_mutex audioMutex;

    // This is the callback function set by Mix_ChannelFinished(). As a rule, it is called from
    // a SDL_Mixer internal thread.
    //
    // TODO: according to SDL_Mixer manual, calls of any SDL_Mixer functions are not allowed in
    // callbacks. The current code works, but it would be good to find a reliable way to perform
    // channel cleanup without calling these functions.
    void channelFinished( const int channelId )
    {
        // This callback function should never be called if audio is not initialized
        assert( isInitialized );

        Mix_Chunk * sample = Mix_GetChunk( channelId );
        assert( sample != nullptr );

        Mix_FreeChunk( sample );

        if ( Mix_UnregisterAllEffects( channelId ) == 0 ) {
            ERROR_LOG( "Failed to unregister all effects from channel " << channelId << ". The error: " << Mix_GetError() )
        }
    }

    int playSound( const uint8_t * ptr, const uint32_t size, const int channelId, const bool loop )
    {
        assert( ptr != nullptr && size != 0 );

        Mix_Chunk * sample = Mix_LoadWAV_RW( SDL_RWFromConstMem( ptr, size ), 1 );
        if ( sample == nullptr ) {
            ERROR_LOG( "Failed to create an audio chunk from memory. The error: " << Mix_GetError() )
            return -1;
        }

        const int channel = Mix_PlayChannel( channelId, sample, loop ? -1 : 0 );
        if ( channel < 0 ) {
            ERROR_LOG( "Failed to play an audio chunk for channel " << channel << ". The error: " << Mix_GetError() )
        }

        return channel;
    }

    void addSoundEffect( const int channelId, const int16_t angle, uint8_t volumePercentage )
    {
        if ( volumePercentage > 100 ) {
            volumePercentage = 100;
        }

        if ( Mix_SetPosition( channelId, angle, 255 * ( volumePercentage - 100 ) ) == 0 ) {
            ERROR_LOG( "Failed to apply a sound effect for channel " << channelId << ". The error: " << Mix_GetError() )
        }
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

        void update( const uint64_t musicUID, const MusicInfo & info )
        {
            assert( info.mix != nullptr );

            auto iter = _musicCache.find( musicUID );
            if ( iter == _musicCache.end() ) {
                _musicCache.try_emplace( musicUID, info );
                return;
            }

            iter->second = info;
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

    struct MusicSettings
    {
        void resetCurrentTrack()
        {
            currentTrack = {};

            currentTrackUID = 0;
            currentTrackPlaybackMode = Music::PlaybackMode::PLAY_ONCE;

            ++currentTrackChangeCounter;
        }

        void updateCurrentTrack( const MusicInfo & track, const uint64_t trackUID, const Music::PlaybackMode trackPlaybackMode )
        {
            currentTrack = track;

            currentTrackUID = trackUID;
            currentTrackPlaybackMode = trackPlaybackMode;

            ++currentTrackChangeCounter;
        }

        MusicInfo currentTrack;

        uint64_t currentTrackUID{ 0 };
        Music::PlaybackMode currentTrackPlaybackMode{ Music::PlaybackMode::PLAY_ONCE };
        // This counter should be incremented every time the currentTrackUID or
        // currentTrackPlaybackMode are changed
        std::atomic<uint64_t> currentTrackChangeCounter{ 0 };

        int fadeInMs{ 0 };

        MusicTrackManager trackManager;
    };

    MusicSettings musicSettings;

    void playMusic( const uint64_t musicUID, Music::PlaybackMode playbackMode );

    class MusicRestartManager final : public MultiThreading::AsyncManager
    {
    public:
        void restartCurrentMusicTrack()
        {
            std::scoped_lock<std::mutex> lock( _mutex );

            _trackChangeCounter = musicSettings.currentTrackChangeCounter;

            notifyWorker();
        }

    private:
        // This function is called by the worker thread and is protected by _mutex
        bool prepareTask() override
        {
            // Make a copy for the worker thread to ensure that this counter will
            // not be changed by another thread in the middle of executeTask()
            _taskTrackChangeCounter = _trackChangeCounter;

            return false;
        }

        // This function is called by the worker thread, but is not protected by _mutex
        void executeTask() override
        {
            const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

            if ( !isInitialized ) {
                return;
            }

            // The current track managed to change during the start of this task
            if ( _taskTrackChangeCounter != musicSettings.currentTrackChangeCounter ) {
                return;
            }

            // REWIND_AND_PLAY_INFINITE should be handled by the Mix_PlayMusic() itself
            assert( musicSettings.currentTrackPlaybackMode == Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

            musicSettings.currentTrack.position = 0;
            musicSettings.trackManager.update( musicSettings.currentTrackUID, musicSettings.currentTrack );

            playMusic( musicSettings.currentTrackUID, musicSettings.currentTrackPlaybackMode );
        }

        // This variable can be accessed by multiple threads and it is protected by _mutex
        uint64_t _trackChangeCounter{ 0 };
        // This variable can be accessed only by the worker thread
        uint64_t _taskTrackChangeCounter{ 0 };
    };

    MusicRestartManager musicRestartManager;

    // This is the callback function set by Mix_HookMusicFinished(). As a rule, it is called from
    // a SDL_Mixer internal thread. Calls of any SDL_Mixer functions are not allowed in callbacks.
    void musicFinished()
    {
        // This callback function should never be called if audio is not initialized
        assert( isInitialized );

        musicRestartManager.restartCurrentMusicTrack();
    }

    bool isMusicResumeSupported( const Mix_Music * mix )
    {
        assert( mix != nullptr );

        const Mix_MusicType musicType = Mix_GetMusicType( mix );

        return ( musicType == Mix_MusicType::MUS_OGG ) || ( musicType == Mix_MusicType::MUS_MP3 ) || ( musicType == Mix_MusicType::MUS_FLAC );
    }

    void playMusic( const uint64_t musicUID, Music::PlaybackMode playbackMode )
    {
        // Unregister this callback to ensure that it will not be called while we are modifying the
        // current track information
        Mix_HookMusicFinished( nullptr );

        const MusicInfo musicInfo = musicSettings.trackManager.getMusicInfoByUID( musicUID );
        if ( musicInfo.mix == nullptr ) {
            // How is it even possible! Check your logic!
            assert( 0 );
            return;
        }

        bool resumePlayback = false;
        bool autoLoop = false;

        if ( playbackMode == Music::PlaybackMode::RESUME_AND_PLAY_INFINITE ) {
            if ( isMusicResumeSupported( musicInfo.mix ) ) {
                resumePlayback = true;
            }
            else {
                // It is impossible to resume this track, let's reflect it by changing the playback mode
                playbackMode = Music::PlaybackMode::REWIND_AND_PLAY_INFINITE;
                autoLoop = true;
            }
        }
        else if ( playbackMode == Music::PlaybackMode::REWIND_AND_PLAY_INFINITE ) {
            autoLoop = true;
        }

        // Should be updated before the callback set by Mix_HookMusicFinished() will get a chance to be called
        musicSettings.updateCurrentTrack( musicInfo, musicUID, playbackMode );

        // Once we are done with the current track information, let's register this callback back. We need this
        // callback only when resuming the music track, because there is no other way to loop such a track.
        if ( playbackMode == Music::PlaybackMode::RESUME_AND_PLAY_INFINITE ) {
            Mix_HookMusicFinished( musicFinished );
        }

        const int loopCount = autoLoop ? -1 : 0;

        if ( musicSettings.fadeInMs > 0 ) {
            int returnCode = -1;

            // Resume the music only if at least 1 second of the track has been played.
            if ( resumePlayback && musicInfo.position > 1 ) {
                returnCode = Mix_FadeInMusicPos( musicInfo.mix, loopCount, musicSettings.fadeInMs, musicInfo.position );

                if ( returnCode != 0 ) {
                    ERROR_LOG( "Failed to resume a music mix. The error: " << Mix_GetError() )
                }
            }

            // Either there is no need to resume music playback, or the resumption failed. Let's start the playback from the beginning.
            if ( returnCode != 0 ) {
                returnCode = Mix_FadeInMusic( musicInfo.mix, loopCount, musicSettings.fadeInMs );

                if ( returnCode != 0 ) {
                    ERROR_LOG( "Failed to play a music mix. The error: " << Mix_GetError() )
                }
            }
        }
        else {
            if ( Mix_PlayMusic( musicInfo.mix, loopCount ) != 0 ) {
                ERROR_LOG( "Failed to play a music mix. The error: " << Mix_GetError() )
            }
            // Resume the music only if at least 1 second of the track has been played.
            else if ( resumePlayback && musicInfo.position > 1 && Mix_SetMusicPosition( musicInfo.position ) != 0 ) {
                ERROR_LOG( "Failed to set the position for a music mix. The error: " << Mix_GetError() )
            }
        }

        // For better accuracy reset the timer only when actual playback is already started
        musicSettings.trackManager.resetTimer();
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
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

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

    musicRestartManager.createWorker();

    Mix_ChannelFinished( channelFinished );

    isInitialized = true;
}

void Audio::Quit()
{
    {
        const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

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

        Mix_ChannelFinished( nullptr );

        musicSettings.trackManager.clear();

        Mix_CloseAudio();
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        Mix_Quit();
#endif

        isInitialized = false;
    }

    // We can't hold the audioMutex here because if MusicRestartManager's working
    // thread is already waiting on it, then there will be a deadlock while waiting
    // for it to join. The Mix_HookMusicFinished()'s callback can no longer be called
    // at the moment because it has been already unregistered by the Music::Stop().
    musicRestartManager.stopWorker();
}

void Audio::Mute()
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( isMuted || !isInitialized ) {
        return;
    }

    isMuted = true;

    const size_t channelsCount = static_cast<size_t>( Mix_AllocateChannels( -1 ) );

    savedMixerVolumes.resize( channelsCount );

    for ( size_t channel = 0; channel < channelsCount; ++channel ) {
        savedMixerVolumes[channel] = Mix_Volume( static_cast<int>( channel ), 0 );
    }

    savedMusicVolume = Mix_VolumeMusic( 0 );
}

void Audio::Unmute()
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isMuted || !isInitialized ) {
        return;
    }

    isMuted = false;

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

int Mixer::SetChannels( const int num )
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return 0;
    }

    const int channelsCount = Mix_AllocateChannels( num );
    if ( num != channelsCount ) {
        ERROR_LOG( "Failed to allocate the required amount of channels for sound. The required number of channels " << num << " but allocated only " << channelsCount )
    }

    if ( channelsCount > 0 ) {
        Mix_ReserveChannels( 1 );
    }

    if ( isMuted ) {
        savedMixerVolumes.resize( static_cast<size_t>( channelsCount ), 0 );

        Mix_Volume( -1, 0 );
    }

    // Just to verify that we are synced with SDL.
    assert( Mix_AllocateChannels( -1 ) == channelsCount );

    return channelsCount;
}

int Mixer::Play( const uint8_t * ptr, const uint32_t size, const int channelId, const bool loop )
{
    if ( ptr == nullptr || size == 0 ) {
        // You are trying to play an empty sound. Check your logic!
        assert( 0 );
        return -1;
    }

    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return -1;
    }

    return playSound( ptr, size, channelId, loop );
}

int Mixer::PlayFromDistance( const uint8_t * ptr, const uint32_t size, const int channelId, const bool loop, const int16_t angle, const uint8_t volumePercentage )
{
    if ( ptr == nullptr || size == 0 ) {
        // You are trying to play an empty sound. Check your logic!
        assert( 0 );
        return -1;
    }

    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return -1;
    }

    const int channel = playSound( ptr, size, channelId, loop );
    if ( channel < 0 ) {
        return channel;
    }

    addSoundEffect( channel, angle, volumePercentage );

    return channel;
}

int Mixer::applySoundEffect( const int channelId, const int16_t angle, const uint8_t volumePercentage )
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return -1;
    }

    addSoundEffect( channelId, angle, volumePercentage );

    return channelId;
}

int Mixer::setVolume( const int channelId, const int volumePercentage )
{
    const int volume = normalizeToSDLVolume( volumePercentage );

    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return 0;
    }

    if ( !isMuted ) {
        return normalizeFromSDLVolume( Mix_Volume( channelId, volume ) );
    }

    if ( channelId < 0 ) {
        if ( savedMixerVolumes.empty() ) {
            return 0;
        }

        // return the average volume
        const int prevVolume = std::accumulate( savedMixerVolumes.begin(), savedMixerVolumes.end(), 0 ) / static_cast<int>( savedMixerVolumes.size() );
        std::fill( savedMixerVolumes.begin(), savedMixerVolumes.end(), volume );

        return normalizeFromSDLVolume( prevVolume );
    }

    const size_t channelNum = static_cast<size_t>( channelId );

    if ( channelNum >= savedMixerVolumes.size() ) {
        return 0;
    }

    const int prevVolume = savedMixerVolumes[channelNum];
    savedMixerVolumes[channelNum] = volume;

    return normalizeFromSDLVolume( prevVolume );
}

void Mixer::Pause( const int channelId /* = -1 */ )
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( isInitialized ) {
        Mix_Pause( channelId );
    }
}

void Mixer::Resume( const int channelId /* = -1 */ )
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( isInitialized ) {
        Mix_Resume( channelId );
    }
}

void Mixer::Stop( const int channelId /* = -1 */ )
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( isInitialized ) {
        Mix_HaltChannel( channelId );
    }
}

bool Mixer::isPlaying( const int channelId )
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    return isInitialized && Mix_Playing( channelId ) > 0;
}

bool Music::Play( const uint64_t musicUID, const PlaybackMode playbackMode )
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return false;
    }

    const MusicInfo musicInfo = musicSettings.trackManager.getMusicInfoByUID( musicUID );
    if ( musicInfo.mix != nullptr ) {
        Stop();

        playMusic( musicUID, playbackMode );

        return true;
    }

    return false;
}

void Music::Play( const uint64_t musicUID, const std::vector<uint8_t> & v, const PlaybackMode playbackMode )
{
    if ( v.empty() ) {
        return;
    }

    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return;
    }

    const MusicInfo musicInfo = musicSettings.trackManager.getMusicInfoByUID( musicUID );
    if ( musicInfo.mix != nullptr ) {
        Stop();

        playMusic( musicUID, playbackMode );

        return;
    }

    SDL_RWops * rwops = SDL_RWFromConstMem( &v[0], static_cast<int>( v.size() ) );
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    Mix_Music * mix = Mix_LoadMUS_RW( rwops, 0 );
#else
    Mix_Music * mix = Mix_LoadMUS_RW( rwops );
#endif
    SDL_FreeRW( rwops );

    if ( mix == nullptr ) {
        ERROR_LOG( "Failed to create a music mix from memory. The error: " << Mix_GetError() )
        return;
    }

    Stop();

    musicSettings.trackManager.update( musicUID, { mix, 0 } );

    playMusic( musicUID, playbackMode );
}

void Music::Play( const uint64_t musicUID, const std::string & file, const PlaybackMode playbackMode )
{
    if ( file.empty() ) {
        // Nothing to play. It is an empty file.
        return;
    }

    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return;
    }

    const MusicInfo musicInfo = musicSettings.trackManager.getMusicInfoByUID( musicUID );
    if ( musicInfo.mix != nullptr ) {
        Stop();

        playMusic( musicUID, playbackMode );

        return;
    }

    const std::string filePath = System::FileNameToUTF8( file );

    Mix_Music * mix = Mix_LoadMUS( filePath.c_str() );
    if ( mix == nullptr ) {
        ERROR_LOG( "Failed to create a music mix from path " << filePath << ". The error: " << Mix_GetError() )
        return;
    }

    Stop();

    musicSettings.trackManager.update( musicUID, { mix, 0 } );

    playMusic( musicUID, playbackMode );
}

void Music::SetFadeInMs( const int timeMs )
{
    if ( timeMs < 0 ) {
        // Why are you even setting a negative value?
        assert( 0 );
        return;
    }

    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    musicSettings.fadeInMs = timeMs;
}

int Music::setVolume( const int volumePercentage )
{
    const int volume = normalizeToSDLVolume( volumePercentage );

    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return 0;
    }

    if ( isMuted ) {
        const int prevVolume = savedMusicVolume;

        savedMusicVolume = volume;

        return normalizeFromSDLVolume( prevVolume );
    }

    return normalizeFromSDLVolume( Mix_VolumeMusic( volume ) );
}

void Music::Stop()
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return;
    }

    if ( musicSettings.currentTrack.mix == nullptr ) {
        // Nothing to do.
        return;
    }

    // Unregister this callback - there is no need to repeat this track anymore
    Mix_HookMusicFinished( nullptr );

    // Always returns 0
    Mix_HaltMusic();

    // We can resume this track, so let's remember the current position
    if ( musicSettings.currentTrackPlaybackMode == PlaybackMode::RESUME_AND_PLAY_INFINITE ) {
        musicSettings.currentTrack.position += musicSettings.trackManager.getCurrentTrackPosition();
    }
    // We cannot resume this track, but in future we should be able to play it in the
    // RESUME_AND_PLAY_INFINITE mode if necessary, so reset its position to zero
    else {
        musicSettings.currentTrack.position = 0;
    }

    musicSettings.trackManager.update( musicSettings.currentTrackUID, musicSettings.currentTrack );
    musicSettings.resetCurrentTrack();
}

bool Music::isPlaying()
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    return ( musicSettings.currentTrack.mix != nullptr ) && Mix_PlayingMusic();
}
