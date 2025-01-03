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

#include "audio.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <utility>
#include <variant>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif

#include <SDL_audio.h>
#include <SDL_error.h>
#include <SDL_mixer.h>
#include <SDL_rwops.h>
#include <SDL_stdinc.h>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif

#include "core.h"
#include "dir.h"
#include "logging.h"
#include "system.h"
#include "thread.h"
#include "timing.h"

namespace
{
    struct AudioSpec
    {
#if defined( TARGET_PS_VITA )
        // Notice: The PS Vita sound resampler is CPU intensive if the value is not 22050.
        int frequency = 22050;
#else
        // Notice: Value 22050 causes music distortion on Windows.
        int frequency = 44100;
#endif
        // Signed 16-bit samples with platform-dependent byte order
        uint16_t format = AUDIO_S16SYS;
        // Stereo audio support
        int channels = 2;
#if defined( ANDROID )
        // Value greater than 1024 causes audio distortion on Android
        int chunkSize = 1024;
#else
        int chunkSize = 2048;
#endif
    };

    std::atomic<bool> isInitialized{ false };

    std::atomic<int> mixerChannelCount{ 0 };

    bool isMuted{ false };
    int savedMixerVolume{ 0 };
    int savedMusicVolume{ 0 };

    int musicFadeInMs{ 0 };

    // This mutex protects all operations with audio. In order to avoid deadlocks, it shouldn't
    // be acquired in any callback functions that can be called by SDL_Mixer.
    std::recursive_mutex audioMutex;

    class SoundSampleManager
    {
    public:
        SoundSampleManager() = default;
        SoundSampleManager( const SoundSampleManager & ) = delete;

        ~SoundSampleManager()
        {
            // Make sure that all sound samples have been eventually freed
            assert( std::all_of( _channelSamples.begin(), _channelSamples.end(), []( const auto & item ) {
                static const decltype( item.second ) nullQueue{ nullptr, nullptr };

                return item.second == nullQueue;
            } ) );
        }

        SoundSampleManager & operator=( const SoundSampleManager & ) = delete;

        void channelStarted( const int channelId, Mix_Chunk * sample )
        {
            assert( channelId >= 0 && sample != nullptr );

            const auto iter = _channelSamples.find( channelId );

            if ( iter != _channelSamples.end() ) {
                auto & sampleQueue = iter->second;

                if ( sampleQueue.first == nullptr ) {
                    sampleQueue.first = sample;
                }
                else if ( sampleQueue.second == nullptr ) {
                    sampleQueue.second = sample;
                }
                else {
                    // The sample queue is already full, this shouldn't happen
                    assert( 0 );
                }

                return;
            }

            const auto res = _channelSamples.try_emplace( channelId, std::make_pair( sample, nullptr ) );
            if ( !res.second ) {
                assert( 0 );
            }
        }

        // This method can be called from the SDL_Mixer callback (without acquiring the audioMutex)
        void channelFinished( const int channelId )
        {
            assert( channelId >= 0 );

            const std::scoped_lock<std::mutex> lock( _channelsToCleanupMutex );

            _channelsToCleanup.push_back( channelId );
        }

        void clearFinishedSamples()
        {
            std::vector<int> channelsToCleanup;

            {
                const std::scoped_lock<std::mutex> lock( _channelsToCleanupMutex );

                std::swap( channelsToCleanup, _channelsToCleanup );
            }

            for ( const int channel : channelsToCleanup ) {
                const auto iter = _channelSamples.find( channel );
                assert( iter != _channelSamples.end() );

                auto & sampleQueue = iter->second;
                assert( sampleQueue.first != nullptr );

                Mix_FreeChunk( sampleQueue.first );

                // Shift the sample queue
                sampleQueue.first = sampleQueue.second;
                sampleQueue.second = nullptr;
            }
        }

    private:
        std::map<int, std::pair<Mix_Chunk *, Mix_Chunk *>> _channelSamples;

        std::vector<int> _channelsToCleanup;
        // This mutex protects operations with _channelsToCleanup
        std::mutex _channelsToCleanupMutex;
    };

    SoundSampleManager soundSampleManager;

    // This is the callback function set by Mix_ChannelFinished(). As a rule, it is called from
    // a SDL_Mixer internal thread. Calls of any SDL_Mixer functions are not allowed in callbacks.
    void SDLCALL channelFinished( const int channelId )
    {
        // This callback function should never be called if audio is not initialized
        assert( isInitialized );

        soundSampleManager.channelFinished( channelId );
    }

    class MusicInfo
    {
    public:
        explicit MusicInfo( std::vector<uint8_t> v )
            : _source( std::move( v ) )
        {
            // Do nothing
        }

        explicit MusicInfo( std::string file )
            : _source( std::move( file ) )
        {
            // Do nothing
        }

        MusicInfo( const MusicInfo & ) = delete;

        ~MusicInfo() = default;

        MusicInfo & operator=( const MusicInfo & ) = delete;

        // Mix_Music objects should never be cached because they store the state of the music decoder's backend,
        // and should be passed to functions like Mix_PlayMusic() only once, otherwise weird things can happen.
        std::unique_ptr<Mix_Music, void ( * )( Mix_Music * )> createMusic() const
        {
            if ( std::holds_alternative<std::vector<uint8_t>>( _source ) ) {
                const std::vector<uint8_t> & v = std::get<std::vector<uint8_t>>( _source );

                const std::unique_ptr<SDL_RWops, void ( * )( SDL_RWops * )> rwops( SDL_RWFromConstMem( v.data(), static_cast<int>( v.size() ) ), SDL_FreeRW );
                if ( !rwops ) {
                    ERROR_LOG( "Failed to create a music track from memory. The error: " << SDL_GetError() )

                    return { nullptr, Mix_FreeMusic };
                }

                std::unique_ptr<Mix_Music, void ( * )( Mix_Music * )> result( Mix_LoadMUS_RW( rwops.get(), 0 ), Mix_FreeMusic );
                if ( !result ) {
                    ERROR_LOG( "Failed to create a music track from memory. The error: " << Mix_GetError() )
                }

                return result;
            }

            if ( std::holds_alternative<std::string>( _source ) ) {
                const std::string & file = std::get<std::string>( _source );

                std::unique_ptr<Mix_Music, void ( * )( Mix_Music * )> result( Mix_LoadMUS( System::encLocalToUTF8( file ).c_str() ), Mix_FreeMusic );
                if ( !result ) {
                    ERROR_LOG( "Failed to create a music track from file " << file << ". The error: " << Mix_GetError() )
                }

                return result;
            }

            assert( 0 );

            return { nullptr, Mix_FreeMusic };
        }

        double getPosition() const
        {
            return _position;
        }

        void setPosition( const double pos )
        {
            _position = pos;
        }

    private:
        const std::variant<std::vector<uint8_t>, std::string> _source;
        double _position{ 0 };
    };

    class MusicTrackManager
    {
    public:
        MusicTrackManager() = default;
        MusicTrackManager( const MusicTrackManager & ) = delete;

        ~MusicTrackManager()
        {
            // Make sure that all music tracks have been eventually freed
            assert( _musicQueue == nullptr );
        }

        MusicTrackManager & operator=( const MusicTrackManager & ) = delete;

        bool isTrackInMusicDB( const uint64_t musicUID ) const
        {
            return ( _musicDB.find( musicUID ) != _musicDB.end() );
        }

        std::shared_ptr<MusicInfo> getTrackFromMusicDB( const uint64_t musicUID ) const
        {
            const auto iter = _musicDB.find( musicUID );
            assert( iter != _musicDB.end() );

            return iter->second;
        }

        void addTrackToMusicDB( const uint64_t musicUID, const std::shared_ptr<MusicInfo> & track )
        {
            const auto res = _musicDB.try_emplace( musicUID, track );
            if ( !res.second ) {
                assert( 0 );
            }
        }

        void clearMusicDB()
        {
            _musicDB.clear();
        }

        std::weak_ptr<MusicInfo> getCurrentTrack() const
        {
            return _currentTrack;
        }

        uint64_t getCurrentTrackUID() const
        {
            return _currentTrackUID;
        }

        Music::PlaybackMode getCurrentTrackPlaybackMode() const
        {
            return _currentTrackPlaybackMode;
        }

        // This method can be called from the SDL_Mixer callback (without acquiring the audioMutex)
        uint64_t getCurrentTrackChangeCounter() const
        {
            return _currentTrackChangeCounter;
        }

        double getCurrentTrackPosition() const
        {
            return _currentTrackTimer.getS();
        }

        void updateCurrentTrack( const uint64_t musicUID, const Music::PlaybackMode trackPlaybackMode )
        {
            _currentTrack = getTrackFromMusicDB( musicUID );

            _currentTrackUID = musicUID;
            _currentTrackPlaybackMode = trackPlaybackMode;

            ++_currentTrackChangeCounter;
        }

        void resetCurrentTrack()
        {
            _currentTrack = {};

            _currentTrackUID = 0;
            _currentTrackPlaybackMode = Music::PlaybackMode::PLAY_ONCE;

            ++_currentTrackChangeCounter;
        }

        void resetTimer()
        {
            _currentTrackTimer.reset();
        }

        void musicStarted( Mix_Music * mus )
        {
            // If the _musicQueue is not nullptr, then the music queue (consisting of only one music
            // track) is already full, this shouldn't happen
            assert( _musicQueue == nullptr );

            _musicQueue = mus;
        }

        void clearFinishedMusic()
        {
            // This queue consists of only one music track
            if ( _musicQueue == nullptr ) {
                return;
            }

            Mix_FreeMusic( _musicQueue );

            _musicQueue = nullptr;
        }

    private:
        std::map<uint64_t, std::shared_ptr<MusicInfo>> _musicDB;

        std::weak_ptr<MusicInfo> _currentTrack;

        uint64_t _currentTrackUID{ 0 };
        Music::PlaybackMode _currentTrackPlaybackMode{ Music::PlaybackMode::PLAY_ONCE };

        // This counter should be incremented every time the current track or its playback mode changes
        std::atomic<uint64_t> _currentTrackChangeCounter{ 0 };

        fheroes2::Time _currentTrackTimer;

        Mix_Music * _musicQueue{ nullptr };
    };

    MusicTrackManager musicTrackManager;

    void playMusic( const uint64_t musicUID, Music::PlaybackMode playbackMode );

    class MusicRestartManager final : public MultiThreading::AsyncManager
    {
    public:
        void restartCurrentMusicTrack()
        {
            const std::scoped_lock<std::mutex> lock( _mutex );

            _trackChangeCounter = musicTrackManager.getCurrentTrackChangeCounter();

            notifyWorker();
        }

    private:
        // This method is called by the worker thread and is protected by _mutex
        bool prepareTask() override
        {
            // Make a copy for the worker thread to ensure that this counter will
            // not be changed by another thread in the middle of executeTask()
            _taskTrackChangeCounter = _trackChangeCounter;

            return false;
        }

        // This method is called by the worker thread, but is not protected by _mutex
        void executeTask() override
        {
            const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

            if ( !isInitialized ) {
                return;
            }

            // The current track managed to change during the start of this task
            if ( _taskTrackChangeCounter != musicTrackManager.getCurrentTrackChangeCounter() ) {
                return;
            }

            // REWIND_AND_PLAY_INFINITE should be handled by the SDL_Mixer itself
            if ( musicTrackManager.getCurrentTrackPlaybackMode() != Music::PlaybackMode::RESUME_AND_PLAY_INFINITE ) {
                return;
            }

            const std::shared_ptr<MusicInfo> currentTrack = musicTrackManager.getCurrentTrack().lock();
            assert( currentTrack );

            currentTrack->setPosition( 0 );

            playMusic( musicTrackManager.getCurrentTrackUID(), musicTrackManager.getCurrentTrackPlaybackMode() );
        }

        // This variable can be accessed by multiple threads and it is protected by _mutex
        uint64_t _trackChangeCounter{ 0 };
        // This variable can be accessed only by the worker thread
        uint64_t _taskTrackChangeCounter{ 0 };
    };

    MusicRestartManager musicRestartManager;

    // This is the callback function set by Mix_HookMusicFinished(). As a rule, it is called from
    // a SDL_Mixer internal thread. Calls of any SDL_Mixer functions are not allowed in callbacks.
    void SDLCALL musicFinished()
    {
        // This callback function should never be called if audio is not initialized
        assert( isInitialized );

        musicRestartManager.restartCurrentMusicTrack();
    }

    bool isMusicResumeSupported( const Mix_Music * mus )
    {
        assert( mus != nullptr );

        const Mix_MusicType musicType = Mix_GetMusicType( mus );

        return ( musicType == Mix_MusicType::MUS_OGG ) || ( musicType == Mix_MusicType::MUS_MP3 ) || ( musicType == Mix_MusicType::MUS_FLAC );
    }

    void playMusic( const uint64_t musicUID, Music::PlaybackMode playbackMode )
    {
        // This function should never be called if a music track is currently playing.
        // Thus we have a guarantee that the Mix_HookMusicFinished()'s callback will
        // not be called while we are modifying the current track information.
        assert( !Music::isPlaying() );

        musicTrackManager.clearFinishedMusic();

        const std::shared_ptr<MusicInfo> track = musicTrackManager.getTrackFromMusicDB( musicUID );
        assert( track );

        std::unique_ptr<Mix_Music, void ( * )( Mix_Music * )> mus = track->createMusic();
        if ( !mus ) {
            musicTrackManager.resetCurrentTrack();

            return;
        }

        bool resumePlayback = false;
        bool autoLoop = false;

        if ( playbackMode == Music::PlaybackMode::RESUME_AND_PLAY_INFINITE ) {
            if ( isMusicResumeSupported( mus.get() ) ) {
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

        // Update the current track information while the music playback is not yet started, so the
        // Mix_HookMusicFinished()'s callback cannot be called
        musicTrackManager.updateCurrentTrack( musicUID, playbackMode );

        const int loopCount = autoLoop ? -1 : 0;

        int returnCode = -1;

        // Resume the music only if at least 1 second of the track has been played.
        if ( resumePlayback && track->getPosition() > 1 ) {
            returnCode = Mix_FadeInMusicPos( mus.get(), loopCount, musicFadeInMs, track->getPosition() );

            if ( returnCode != 0 ) {
                ERROR_LOG( "Failed to resume the music track. The error: " << Mix_GetError() )
            }
        }

        // Either there is no need to resume music playback, or the resumption failed. Let's try to
        // start the playback from the beginning.
        if ( returnCode != 0 ) {
            returnCode = Mix_FadeInMusic( mus.get(), loopCount, musicFadeInMs );

            if ( returnCode != 0 ) {
                ERROR_LOG( "Failed to play the music track. The error: " << Mix_GetError() )
            }
        }

        if ( returnCode != 0 ) {
            // Since the music playback failed, the Mix_HookMusicFinished()'s callback cannot be called
            // here, so we can safely reset the current track information
            musicTrackManager.resetCurrentTrack();

            return;
        }

        // For better accuracy reset the timer right after the actual playback starts
        musicTrackManager.resetTimer();

        // There can be no more than one element in the music queue - the current track, the previous
        // one should already be freed
        musicTrackManager.musicStarted( mus.release() );
    }

    // By the Weber-Fechner law, humans subjective sound sensation is proportional logarithm of sound intensity.
    // So for linear changing sound intensity we have to change the volume exponential.
    // There is a good explanation at https://www.dr-lex.be/info-stuff/volumecontrols.html.
    // This function maps sound volumes in percents to SDL units with values [0..MIX_MAX_VOLUME] by exponential law.
    int normalizeToSDLVolume( const int volumePercentage )
    {
        if ( volumePercentage < 0 ) {
            // Why are you passing a negative volume value?
            assert( 0 );
            return 0;
        }

        if ( volumePercentage >= 100 ) {
            // Reserve an extra 0.5 dB for possible sound overloads in SDL_mixer, multiplying max volume by 50/53.
            return MIX_MAX_VOLUME * 50 / 53;
        }

        // MIX_MAX_VOLUME is divided by 10.6, not 10 to reserve an extra 0.5 dB for possible sound overloads in SDL_mixer.
        return static_cast<int>( ( std::exp( std::log( 10 + 1 ) * volumePercentage / 100 ) - 1 ) / 10.6 * MIX_MAX_VOLUME );
    }

    // Synchronizes the volume settings of all active mixer channels
    void syncChannelsVolume()
    {
        if ( Mix_AllocateChannels( -1 ) < 2 ) {
            return;
        }

        Mix_Volume( -1, Mix_Volume( 0, -1 ) );
    }

#ifndef NDEBUG
    // Checks whether the volume settings of all active mixer channels are synchronized
    bool checkChannelsVolumeSync()
    {
        const int channelsCount = Mix_AllocateChannels( -1 );
        if ( channelsCount < 2 ) {
            return true;
        }

        const int vol = Mix_Volume( 0, -1 );

        for ( int i = 1; i < channelsCount; ++i ) {
            if ( Mix_Volume( i, -1 ) != vol ) {
                return false;
            }
        }

        return true;
    }
#endif
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

    const int initializationFlags = MIX_INIT_FLAC | MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_MID;
    const int initializedFlags = Mix_Init( initializationFlags );
    if ( ( initializedFlags & initializationFlags ) != initializationFlags ) {
        DEBUG_LOG( DBG_ENGINE, DBG_WARN,
                   "Expected music initialization flags as " << initializationFlags << " but received " << ( initializedFlags & initializationFlags ) )

        if ( ( initializedFlags & MIX_INIT_FLAC ) == 0 ) {
            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "FLAC module failed to be initialized" )
        }

        if ( ( initializedFlags & MIX_INIT_MP3 ) == 0 ) {
            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "MP3 module failed to be initialized" )
        }

        if ( ( initializedFlags & MIX_INIT_OGG ) == 0 ) {
            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "OGG module failed to be initialized" )
        }

        if ( ( initializedFlags & MIX_INIT_MID ) == 0 ) {
            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "MID module failed to be initialized" )
        }
    }

    const AudioSpec audioSpec;

    if ( Mix_OpenAudio( audioSpec.frequency, audioSpec.format, audioSpec.channels, audioSpec.chunkSize ) != 0 ) {
        ERROR_LOG( "Failed to initialize an audio device. The error: " << Mix_GetError() )
        return;
    }

    int frequency = 0;
    uint16_t format = 0;
    int channels = 0;

    const int deviceInitCount = Mix_QuerySpec( &frequency, &format, &channels );
    if ( deviceInitCount == 0 ) {
        ERROR_LOG( "Failed to query an audio device specs. The error: " << Mix_GetError() )
    }

    if ( deviceInitCount != 1 ) {
        // The device must be opened only once.
        assert( 0 );
        ERROR_LOG( "Trying to initialize an audio system that has been already initialized." )
    }

    if ( audioSpec.frequency != frequency ) {
        // At least on Windows the standard frequency is 48000 Hz. Sounds in the game are 22500 Hz frequency.
        // However, resampling is done inside SDL so this is not exactly an error.
        DEBUG_LOG( DBG_ENGINE, DBG_WARN, "Audio frequency is initialized as " << frequency << " instead of " << audioSpec.frequency )
    }

    if ( audioSpec.format != format ) {
        ERROR_LOG( "Audio format is initialized as " << format << " instead of " << audioSpec.format )
    }

    if ( audioSpec.channels != channels ) {
        ERROR_LOG( "Number of audio channels is initialized as " << channels << " instead of " << audioSpec.channels )
    }

    // Make sure that all mixer channels have the same volume settings.
    syncChannelsVolume();

    // By default this value should be MIX_CHANNELS.
    mixerChannelCount = Mix_AllocateChannels( -1 );

    isMuted = false;
    savedMixerVolume = 0;
    savedMusicVolume = 0;

    musicRestartManager.createWorker();

    Mix_ChannelFinished( channelFinished );
    Mix_HookMusicFinished( musicFinished );

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
        Mix_HookMusicFinished( nullptr );

        soundSampleManager.clearFinishedSamples();

        musicTrackManager.clearFinishedMusic();
        musicTrackManager.clearMusicDB();

        Mix_CloseAudio();
        Mix_Quit();

        mixerChannelCount = 0;

        isInitialized = false;
    }

    // We can't hold the audioMutex here because if MusicRestartManager's working
    // thread is already waiting on it, then there will be a deadlock while waiting
    // for it to join. The Mix_HookMusicFinished()'s callback can no longer be called
    // at the moment because it has been already unregistered.
    musicRestartManager.stopWorker();
}

void Audio::Mute()
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( isMuted || !isInitialized ) {
        return;
    }

    assert( Mix_AllocateChannels( -1 ) > 0 && checkChannelsVolumeSync() );

    isMuted = true;

    // Mix_Volume( -1, X ) returns the average of all channels' volumes, but since they all have to have the same volume (and we just checked that their volume settings
    // are in sync) it is safe to use its result here.
    savedMixerVolume = Mix_Volume( -1, 0 );
    savedMusicVolume = Mix_VolumeMusic( 0 );
}

void Audio::Unmute()
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isMuted || !isInitialized ) {
        return;
    }

    assert( Mix_AllocateChannels( -1 ) > 0 && savedMixerVolume >= 0 && savedMusicVolume >= 0 );

    isMuted = false;

    Mix_Volume( -1, savedMixerVolume );
    Mix_VolumeMusic( savedMusicVolume );
}

bool Audio::isValid()
{
    return isInitialized;
}

void Mixer::SetChannels( const int num )
{
    if ( num <= 0 ) {
        // When trying to allocate zero channels, an attempt to call Mix_Volume( -1, X ) results in division by zero when calculating the average volume
        assert( 0 );
        return;
    }

    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return;
    }

    assert( checkChannelsVolumeSync() );

    mixerChannelCount = Mix_AllocateChannels( num );
    if ( num != mixerChannelCount ) {
        ERROR_LOG( "Failed to allocate the requested number of audio channels. The requested number of channels is " << num << ", the actual allocated number is "
                                                                                                                     << mixerChannelCount )
    }

    assert( mixerChannelCount > 0 );

    if ( isMuted ) {
        // Make sure that all mixer channels (including the ones that have just been allocated) are muted.
        Mix_Volume( -1, 0 );
    }
    else {
        // Make sure that all mixer channels (including the ones that have just been allocated) have the same volume settings.
        syncChannelsVolume();
    }
}

int Mixer::getChannelCount()
{
    return mixerChannelCount;
}

int Mixer::Play( const uint8_t * ptr, const uint32_t size, const bool loop, const std::optional<std::pair<int16_t, uint8_t>> position /* = {} */ )
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

    soundSampleManager.clearFinishedSamples();

    const std::unique_ptr<SDL_RWops, void ( * )( SDL_RWops * )> rwops( SDL_RWFromConstMem( ptr, static_cast<int>( size ) ), SDL_FreeRW );
    if ( !rwops ) {
        ERROR_LOG( "Failed to create an audio chunk from memory. The error: " << SDL_GetError() )
        return -1;
    }

    std::unique_ptr<Mix_Chunk, void ( * )( Mix_Chunk * )> sample( Mix_LoadWAV_RW( rwops.get(), 0 ), Mix_FreeChunk );
    if ( !sample ) {
        ERROR_LOG( "Failed to create an audio chunk from memory. The error: " << Mix_GetError() )
        return -1;
    }

    // SDL itself maintains all internal channel bookkeeping, so when using the "first free channel"
    // for playback, it is not known in advance which channel will be used. If additional channel
    // setup is needed, then, to avoid arbitrary volume fluctuations, we will temporarily mute the
    // audio chunk itself until we can properly adjust the channel parameters.
    const int chunkVolume = position ? Mix_VolumeChunk( sample.get(), 0 ) : 0;
    if ( chunkVolume < 0 ) {
        ERROR_LOG( "Failed to mute the audio chunk. The error: " << Mix_GetError() )
        return -1;
    }

    const int channel = Mix_PlayChannel( -1, sample.get(), loop ? -1 : 0 );
    if ( channel < 0 ) {
        ERROR_LOG( "Failed to play the audio chunk. The error: " << Mix_GetError() )
        return channel;
    }

    if ( position ) {
        // Immediately pause the channel so as not to continue playing while it is being set up
        Mix_Pause( channel );

        setPosition( channel, position->first, position->second );

        // When restoring the volume of an audio chunk, the only correct result of the call is zero,
        // because this is exactly what the volume of the muted chunk should be
        if ( Mix_VolumeChunk( sample.get(), chunkVolume ) != 0 ) {
            ERROR_LOG( "Failed to restore the volume of the audio chunk for channel " << channel << ". The error: " << Mix_GetError() )
        }

        // Resume the channel as soon as all its parameters are settled
        Mix_Resume( channel );
    }

    // There can be a maximum of two items in the sample queue for a channel:
    // the previous sample (if it hasn't been freed yet) and the current one
    soundSampleManager.channelStarted( channel, sample.release() );

    return channel;
}

void Mixer::setPosition( const int channelId, const int16_t angle, const uint8_t distance )
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return;
    }

    if ( Mix_SetPosition( channelId, angle, distance ) == 0 ) {
        ERROR_LOG( "Failed to set the position of channel " << channelId << ". The error: " << Mix_GetError() )
    }
}

void Mixer::setVolume( const int volumePercentage )
{
    const int volume = normalizeToSDLVolume( volumePercentage );

    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return;
    }

    assert( Mix_AllocateChannels( -1 ) > 0 && checkChannelsVolumeSync() );

    if ( isMuted ) {
        savedMixerVolume = volume;
        return;
    }

    Mix_Volume( -1, volume );
}

void Mixer::Stop( const int channelId /* = -1 */ )
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return;
    }

    Mix_HaltChannel( channelId );
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

    if ( musicTrackManager.isTrackInMusicDB( musicUID ) ) {
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

    musicTrackManager.addTrackToMusicDB( musicUID, std::make_shared<MusicInfo>( v ) );

    Stop();

    playMusic( musicUID, playbackMode );
}

void Music::Play( const uint64_t musicUID, const std::string & file, const PlaybackMode playbackMode )
{
    if ( file.empty() ) {
        // Nothing to play, the file name is empty.
        return;
    }

    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return;
    }

    musicTrackManager.addTrackToMusicDB( musicUID, std::make_shared<MusicInfo>( file ) );

    Stop();

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

    musicFadeInMs = timeMs;
}

void Music::setVolume( const int volumePercentage )
{
    const int volume = normalizeToSDLVolume( volumePercentage );

    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return;
    }

    if ( isMuted ) {
        savedMusicVolume = volume;
        return;
    }

    Mix_VolumeMusic( volume );
}

void Music::Stop()
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return;
    }

    if ( musicTrackManager.getCurrentTrack().expired() ) {
        // Nothing to do.
        return;
    }

    // Always returns 0. After this call we have a guarantee that the Mix_HookMusicFinished()'s
    // callback will not be called while we are modifying the current track information.
    Mix_HaltMusic();

    const std::shared_ptr<MusicInfo> currentTrack = musicTrackManager.getCurrentTrack().lock();
    assert( currentTrack );

    // We can and should reliably calculate the current playback position, let's remember it
    if ( musicTrackManager.getCurrentTrackPlaybackMode() == PlaybackMode::RESUME_AND_PLAY_INFINITE ) {
        currentTrack->setPosition( currentTrack->getPosition() + musicTrackManager.getCurrentTrackPosition() );
    }
    // We either shouldn't (PLAY_ONCE) or can't (REWIND_AND_PLAY_INFINITE) reliably calculate the
    // current playback position, let's reset it to zero
    else {
        currentTrack->setPosition( 0 );
    }

    musicTrackManager.resetCurrentTrack();
}

bool Music::isPlaying()
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    return !musicTrackManager.getCurrentTrack().expired() && Mix_PlayingMusic();
}

void Music::SetMidiSoundFonts( const ListFiles & files )
{
    const std::scoped_lock<std::recursive_mutex> lock( audioMutex );

    if ( !isInitialized ) {
        return;
    }

    std::string filePaths;

    for ( const std::string & file : files ) {
        filePaths.append( file );
        filePaths.push_back( ';' );
    }

    // Remove the last semicolon
    if ( !filePaths.empty() ) {
        assert( filePaths.back() == ';' );

        filePaths.pop_back();
    }

    if ( Mix_SetSoundFonts( System::encLocalToUTF8( filePaths ).c_str() ) == 0 ) {
        ERROR_LOG( "Failed to set MIDI SoundFonts using paths " << filePaths << ". The error: " << Mix_GetError() )
    }
}
