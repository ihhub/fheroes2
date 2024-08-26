/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2024                                             *
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

#include "audio_manager.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <deque>
#include <list>
#include <mutex>
#include <optional>
#include <ostream>
#include <utility>

#include "agg_file.h"
#include "audio.h"
#include "dir.h"
#include "logging.h"
#include "m82.h"
#include "mus.h"
#include "serialize.h"
#include "settings.h"
#include "system.h"
#include "thread.h"
#include "tools.h"
#include "xmi.h"

namespace
{
    struct MusicFileType
    {
        explicit MusicFileType( const MUS::ExternalMusicNamingScheme scheme )
            : namingScheme( scheme )
        {
            // Do nothing.
        }

        MUS::ExternalMusicNamingScheme namingScheme = MUS::ExternalMusicNamingScheme::WIN_VERSION;

        std::array<std::string, 3> extension{ ".ogg", ".mp3", ".flac" };
    };

    bool findMusicFile( const std::vector<std::string> & directories, const std::string & fileName, std::string & fullPath )
    {
        for ( const std::string & dir : directories ) {
            ListFiles musicFilePaths;

            musicFilePaths.ReadDir( dir, fileName, false );
            if ( musicFilePaths.empty() ) {
                continue;
            }

            const std::string correctFilePath = StringLower( System::concatPath( dir, fileName ) );

            for ( std::string & path : musicFilePaths ) {
                if ( StringLower( path ) != correctFilePath ) {
                    continue;
                }

                // Avoid string copy.
                std::swap( fullPath, path );

                return true;
            }
        }

        return false;
    }

    std::string getExternalMusicFile( const int musicTrackId )
    {
        static const std::vector<std::string> musicDirectories = []() {
            std::vector<std::string> result;

            for ( const std::string & dir : Settings::GetRootDirs() ) {
                std::string fullDirectoryPath = System::concatPath( dir, "music" );

                if ( System::IsDirectory( fullDirectoryPath ) ) {
                    result.emplace_back( std::move( fullDirectoryPath ) );
                }
            }

            return result;
        }();

        if ( musicDirectories.empty() ) {
            // Nothing to search.
            return {};
        }

        thread_local std::map<int, std::string> musicTrackIdToFilePath;

        const auto iter = musicTrackIdToFilePath.find( musicTrackId );
        if ( iter != musicTrackIdToFilePath.end() ) {
            return iter->second;
        }

        const auto tryMusicFileType = [musicTrackId]( MusicFileType & musicFileType ) -> std::string {
            std::string fullPath;

            std::string fileName = MUS::getFileName( musicTrackId, musicFileType.namingScheme, musicFileType.extension[0].c_str() );
            if ( findMusicFile( musicDirectories, fileName, fullPath ) ) {
                return fullPath;
            }

            fheroes2::replaceStringEnding( fileName, musicFileType.extension[0].c_str(), musicFileType.extension[1].c_str() );
            if ( findMusicFile( musicDirectories, fileName, fullPath ) ) {
                // Swap extensions to improve cache hit.
                std::swap( musicFileType.extension[0], musicFileType.extension[1] );
                return fullPath;
            }

            fheroes2::replaceStringEnding( fileName, musicFileType.extension[1].c_str(), musicFileType.extension[2].c_str() );
            if ( findMusicFile( musicDirectories, fileName, fullPath ) ) {
                // Swap extensions to improve cache hit.
                std::swap( musicFileType.extension[0], musicFileType.extension[2] );
                return fullPath;
            }

            // Looks like music file does not exist.
            return {};
        };

        // To avoid extra I/O calls to data storage it might be useful to remember the last successful type of music and try to search for it next time.
        thread_local std::array<MusicFileType, 3> musicFileTypes{ MusicFileType( MUS::ExternalMusicNamingScheme::DOS_VERSION ),
                                                                  MusicFileType( MUS::ExternalMusicNamingScheme::WIN_VERSION ),
                                                                  MusicFileType( MUS::ExternalMusicNamingScheme::MAPPED ) };

        for ( size_t i = 0; i < musicFileTypes.size(); ++i ) {
            std::string filePath = tryMusicFileType( musicFileTypes[i] );

            if ( filePath.empty() ) {
                continue;
            }

            if ( i > 0 ) {
                // Swap music types to improve cache hit.
                std::swap( musicFileTypes[0], musicFileTypes[i] );
            }

            // Place the path to the cache
            musicTrackIdToFilePath.try_emplace( musicTrackId, filePath );

            return filePath;
        }

        // Place the negative result to the cache
        musicTrackIdToFilePath.try_emplace( musicTrackId );

        return {};
    }

    struct ChannelAudioLoopEffectInfo : public AudioManager::AudioLoopEffectInfo
    {
        ChannelAudioLoopEffectInfo( const AudioLoopEffectInfo & info, const int chan )
            : AudioLoopEffectInfo( info )
            , channelId( chan )
        {
            // Do nothing.
        }

        bool operator==( const AudioManager::AudioLoopEffectInfo & other ) const
        {
            return AudioManager::AudioLoopEffectInfo::operator==( other );
        }

        int channelId{ -1 };
    };

    std::vector<uint8_t> getDataFromAggFile( const std::string & key, const bool ignoreExpansion );

    void LoadWAV( int m82, std::vector<uint8_t> & v )
    {
        DEBUG_LOG( DBG_GAME, DBG_TRACE, M82::GetString( m82 ) )
        const std::vector<uint8_t> & body = getDataFromAggFile( M82::GetString( m82 ), false );

        if ( !body.empty() ) {
            RWStreamBuf wavHeader( 44 );
            wavHeader.putLE32( 0x46464952 ); // RIFF marker ("RIFF")
            wavHeader.putLE32( static_cast<uint32_t>( body.size() ) + 0x24 ); // Total size minus the size of this and previous fields
            wavHeader.putLE32( 0x45564157 ); // File type header ("WAVE")
            wavHeader.putLE32( 0x20746D66 ); // Format sub-chunk marker ("fmt ")
            wavHeader.putLE32( 0x10 ); // Size of the format sub-chunk
            wavHeader.putLE16( 0x01 ); // Audio format (1 for PCM)
            wavHeader.putLE16( 0x01 ); // Number of channels
            wavHeader.putLE32( 22050 ); // Sample rate
            wavHeader.putLE32( 22050 ); // Byte rate (SampleRate * BitsPerSample * NumberOfChannels) / 8
            wavHeader.putLE16( 0x01 ); // Block align (BitsPerSample * NumberOfChannels) / 8
            wavHeader.putLE16( 0x08 ); // Bits per sample
            wavHeader.putLE32( 0x61746164 ); // Data sub-chunk marker ("data")
            wavHeader.putLE32( static_cast<uint32_t>( body.size() ) ); // Size of the data sub-chunk

            v.reserve( body.size() + 44 );
            v.assign( wavHeader.data(), wavHeader.data() + 44 );
            v.insert( v.begin() + 44, body.begin(), body.end() );
        }
    }

    void LoadMID( int xmi, std::vector<uint8_t> & v )
    {
        DEBUG_LOG( DBG_GAME, DBG_TRACE, XMI::GetString( xmi ) )
        const std::vector<uint8_t> & body = getDataFromAggFile( XMI::GetString( xmi ), xmi >= XMI::MIDI_ORIGINAL_KNIGHT );

        if ( !body.empty() ) {
            v = Music::Xmi2Mid( body );
        }
    }

    std::map<int, std::vector<uint8_t>> wavDataCache;
    std::map<int, std::vector<uint8_t>> MIDDataCache;

    const std::vector<uint8_t> & GetWAV( int m82 )
    {
        std::vector<uint8_t> & v = wavDataCache[m82];
        if ( v.empty() ) {
            LoadWAV( m82, v );
        }

        return v;
    }

    const std::vector<uint8_t> & GetMID( int xmi )
    {
        std::vector<uint8_t> & v = MIDDataCache[xmi];
        if ( v.empty() ) {
            LoadMID( xmi, v );
        }

        return v;
    }

    // Returns the ID of the channel occupied by the sound being played, or a negative value (-1) in case of failure.
    int PlaySoundImpl( const int m82 );
    void PlayMusicImpl( const int trackId, const MusicSource musicType, const Music::PlaybackMode playbackMode );
    void playLoopSoundsImpl( std::map<M82::SoundType, std::vector<AudioManager::AudioLoopEffectInfo>> soundEffects, const bool is3DAudioEnabled );

    // SDL MIDI player is a single threaded library which requires a lot of time to start playing some long midi compositions.
    // This leads to a situation of a short application freeze while a hero crosses terrains or ending a battle.
    // The only way to avoid this is to fire MIDI requests asynchronously and synchronize them if needed.
    class AsyncSoundManager final : public MultiThreading::AsyncManager
    {
    public:
        void pushMusic( const int musicId, const MusicSource musicType, const Music::PlaybackMode playbackMode )
        {
            createWorker();

            const std::scoped_lock<std::mutex> lock( _mutex );

            _musicTask.emplace( musicId, musicType, playbackMode );

            notifyWorker();
        }

        void pushSound( const int m82Sound )
        {
            createWorker();

            const std::scoped_lock<std::mutex> lock( _mutex );

            _soundTasks.emplace_back( m82Sound );

            notifyWorker();
        }

        void pushLoopSound( std::map<M82::SoundType, std::vector<AudioManager::AudioLoopEffectInfo>> effects, const bool is3DAudioEnabled )
        {
            createWorker();

            const std::scoped_lock<std::mutex> lock( _mutex );

            _loopSoundTask.emplace( std::move( effects ), is3DAudioEnabled );

            notifyWorker();
        }

        void removeMusicTask()
        {
            const std::scoped_lock<std::mutex> lock( _mutex );

            _musicTask.reset();

            if ( _taskToExecute == TaskType::PlayMusic ) {
                _taskToExecute = TaskType::None;
            }
        }

        void removeSoundTasks()
        {
            const std::scoped_lock<std::mutex> lock( _mutex );

            _soundTasks.clear();

            if ( _taskToExecute == TaskType::PlaySound ) {
                _taskToExecute = TaskType::None;
            }
        }

        void removeAllSoundTasks()
        {
            const std::scoped_lock<std::mutex> lock( _mutex );

            _soundTasks.clear();
            _loopSoundTask.reset();

            switch ( _taskToExecute ) {
            case TaskType::PlaySound:
            case TaskType::PlayLoopSound:
                _taskToExecute = TaskType::None;
                break;
            default:
                break;
            }
        }

        void removeAllTasks()
        {
            const std::scoped_lock<std::mutex> lock( _mutex );

            _musicTask.reset();
            _soundTasks.clear();
            _loopSoundTask.reset();

            _taskToExecute = TaskType::None;
        }

        // This mutex protects operations with AudioManager's resources, such as AGG files, data caches, etc
        std::recursive_mutex & resourceMutex()
        {
            return _resourceMutex;
        }

    private:
        enum class TaskType : int
        {
            None,
            PlayMusic,
            PlaySound,
            PlayLoopSound
        };

        struct MusicTask
        {
            MusicTask() = default;

            MusicTask( const int id, const MusicSource type, const Music::PlaybackMode mode )
                : musicId( id )
                , musicType( type )
                , playbackMode( mode )
            {
                // Do nothing.
            }

            int musicId{ 0 };
            MusicSource musicType{ MUSIC_MIDI_ORIGINAL };
            Music::PlaybackMode playbackMode{ Music::PlaybackMode::PLAY_ONCE };
        };

        struct SoundTask
        {
            SoundTask() = default;

            explicit SoundTask( const int m82 )
                : m82Sound( m82 )
            {
                // Do nothing.
            }

            int m82Sound{ 0 };
        };

        struct LoopSoundTask
        {
            LoopSoundTask() = default;

            LoopSoundTask( std::map<M82::SoundType, std::vector<AudioManager::AudioLoopEffectInfo>> effects, const bool is3DAudioOn )
                : soundEffects( std::move( effects ) )
                , is3DAudioEnabled( is3DAudioOn )
            {
                // Do nothing.
            }

            std::map<M82::SoundType, std::vector<AudioManager::AudioLoopEffectInfo>> soundEffects;
            bool is3DAudioEnabled{ false };
        };

        std::optional<MusicTask> _musicTask;
        std::deque<SoundTask> _soundTasks;
        std::optional<LoopSoundTask> _loopSoundTask;

        MusicTask _currentMusicTask;
        SoundTask _currentSoundTask;
        LoopSoundTask _currentLoopSoundTask;

        std::atomic<TaskType> _taskToExecute{ TaskType::None };

        std::recursive_mutex _resourceMutex;

        // This method is called by the worker thread and is protected by _mutex
        bool prepareTask() override
        {
            if ( _musicTask ) {
                std::swap( _currentMusicTask, *_musicTask );
                _musicTask.reset();

                _taskToExecute = TaskType::PlayMusic;

                return true;
            }

            if ( !_soundTasks.empty() ) {
                std::swap( _currentSoundTask, _soundTasks.front() );
                _soundTasks.pop_front();

                _taskToExecute = TaskType::PlaySound;

                return true;
            }

            if ( _loopSoundTask ) {
                std::swap( _currentLoopSoundTask, *_loopSoundTask );
                _loopSoundTask.reset();

                _taskToExecute = TaskType::PlayLoopSound;

                return true;
            }

            _taskToExecute = TaskType::None;

            return false;
        }

        // This method is called by the worker thread, but is not protected by _mutex
        void executeTask() override
        {
            // Do not allow the main thread to acquire this mutex in the interval between the
            // _taskToExecute was checked and the task was started executing. Release it only
            // when the task is fully completed.
            const std::scoped_lock<std::recursive_mutex> lock( _resourceMutex );

            switch ( _taskToExecute ) {
            case TaskType::None:
                // Nothing to do.
                return;
            case TaskType::PlayMusic:
                PlayMusicImpl( _currentMusicTask.musicId, _currentMusicTask.musicType, _currentMusicTask.playbackMode );
                return;
            case TaskType::PlaySound:
                PlaySoundImpl( _currentSoundTask.m82Sound );
                return;
            case TaskType::PlayLoopSound:
                playLoopSoundsImpl( std::move( _currentLoopSoundTask.soundEffects ), _currentLoopSoundTask.is3DAudioEnabled );
                return;
            default:
                // How is it even possible? Did you add a new task?
                assert( 0 );
                break;
            }
        }
    };

    std::map<M82::SoundType, std::vector<ChannelAudioLoopEffectInfo>> currentAudioLoopEffects;
    bool is3DAudioLoopEffectsEnabled{ false };

    // The music track last requested to be played
    int lastRequestedMusicTrackId{ MUS::UNKNOWN };
    // The music track that is currently being played
    int currentMusicTrackId{ MUS::UNKNOWN };

    fheroes2::AGGFile g_midiHeroes2AGG;
    fheroes2::AGGFile g_midiHeroes2xAGG;

    std::vector<uint8_t> getDataFromAggFile( const std::string & key, const bool ignoreExpansion )
    {
        if ( !ignoreExpansion && g_midiHeroes2xAGG.isGood() ) {
            // Make sure that the below container is not const and not a reference
            // so returning it from the function will invoke a move constructor instead of copy constructor.
            std::vector<uint8_t> buf = g_midiHeroes2xAGG.read( key );
            if ( !buf.empty() )
                return buf;
        }

        return g_midiHeroes2AGG.read( key );
    }

    AsyncSoundManager g_asyncSoundManager;

    int PlaySoundImpl( const int m82 )
    {
        const std::scoped_lock<std::recursive_mutex> lock( g_asyncSoundManager.resourceMutex() );

        DEBUG_LOG( DBG_GAME, DBG_TRACE, "Try to play sound " << M82::GetString( m82 ) )

        const std::vector<uint8_t> & v = GetWAV( m82 );
        if ( v.empty() ) {
            return -1;
        }

        return Mixer::Play( v.data(), static_cast<uint32_t>( v.size() ), false );
    }

    uint64_t getMusicUID( const int trackId, const MusicSource musicType )
    {
        static_assert( MUS::UNUSED == 0, "Why are you changing this value?" );
        assert( trackId != MUS::UNUSED && trackId != MUS::UNKNOWN );

        return ( static_cast<uint64_t>( musicType ) << 32 ) + static_cast<uint64_t>( trackId );
    }

    void PlayMusicImpl( const int trackId, const MusicSource musicType, const Music::PlaybackMode playbackMode )
    {
        // Make sure that the music track is valid.
        assert( trackId != MUS::UNUSED && trackId != MUS::UNKNOWN );

        const uint64_t musicUID = getMusicUID( trackId, musicType );

        const std::scoped_lock<std::recursive_mutex> lock( g_asyncSoundManager.resourceMutex() );

        if ( currentMusicTrackId == trackId && Music::isPlaying() ) {
            return;
        }

        // Check if the music track is already available in the music database.
        if ( Music::Play( musicUID, playbackMode ) ) {
            DEBUG_LOG( DBG_GAME, DBG_TRACE, "Play music track " << trackId )

            currentMusicTrackId = trackId;

            return;
        }

        if ( musicType == MUSIC_EXTERNAL ) {
            const std::string filePath = getExternalMusicFile( trackId );

            if ( filePath.empty() ) {
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Cannot find a file for " << trackId << " track." )
            }
            else {
                Music::Play( musicUID, filePath, playbackMode );

                currentMusicTrackId = trackId;

                DEBUG_LOG( DBG_GAME, DBG_TRACE, "Play music track " << MUS::getFileName( trackId, MUS::ExternalMusicNamingScheme::MAPPED, " " ) )

                return;
            }
        }

        int xmi = XMI::UNKNOWN;

        // Check if music needs to be pulled from HEROES2X
        if ( musicType == MUSIC_MIDI_EXPANSION ) {
            xmi = XMI::FromMUS( trackId, g_midiHeroes2xAGG.isGood() );
        }

        if ( XMI::UNKNOWN == xmi ) {
            xmi = XMI::FromMUS( trackId, false );
        }

        if ( XMI::UNKNOWN != xmi ) {
            const std::vector<uint8_t> & v = GetMID( xmi );
            if ( !v.empty() ) {
                Music::Play( musicUID, v, playbackMode );

                currentMusicTrackId = trackId;
            }
        }

        DEBUG_LOG( DBG_GAME, DBG_TRACE, "Play MIDI music track " << XMI::GetString( xmi ) )
    }

    void getClosestSoundIdPairByAngle( const std::vector<AudioManager::AudioLoopEffectInfo> & soundToAdd, const std::vector<ChannelAudioLoopEffectInfo> & soundToReplace,
                                       size_t & bestSoundToAddId, size_t & bestSoundToReplaceId )
    {
        assert( !soundToAdd.empty() && !soundToReplace.empty() );

        bestSoundToAddId = 0;
        bestSoundToReplaceId = 0;
        int bestAngleDiff = INT_MAX;
        int bestDistanceDiff = INT_MAX;

        for ( size_t soundToAddId = 0; soundToAddId < soundToAdd.size(); ++soundToAddId ) {
            for ( size_t soundToReplaceId = 0; soundToReplaceId < soundToReplace.size(); ++soundToReplaceId ) {
                const int angleDiff = std::abs( soundToAdd[soundToAddId].angle - soundToReplace[soundToReplaceId].angle );
                const int distanceDiff
                    = std::abs( static_cast<int>( soundToAdd[soundToAddId].distance ) - static_cast<int>( soundToReplace[soundToReplaceId].distance ) );

                if ( bestAngleDiff < angleDiff ) {
                    continue;
                }

                if ( bestAngleDiff == angleDiff && bestDistanceDiff < distanceDiff ) {
                    continue;
                }

                bestAngleDiff = angleDiff;
                bestDistanceDiff = distanceDiff;

                bestSoundToAddId = soundToAddId;
                bestSoundToReplaceId = soundToReplaceId;
            }
        }
    }

    void clearAllAudioLoopEffects()
    {
        for ( const auto & audioEffectPair : currentAudioLoopEffects ) {
            const std::vector<ChannelAudioLoopEffectInfo> & existingEffects = audioEffectPair.second;

            for ( const ChannelAudioLoopEffectInfo & info : existingEffects ) {
                if ( Mixer::isPlaying( info.channelId ) ) {
                    Mixer::Stop( info.channelId );
                }
            }
        }

        currentAudioLoopEffects.clear();
    }

    void playLoopSoundsImpl( std::map<M82::SoundType, std::vector<AudioManager::AudioLoopEffectInfo>> soundEffects, const bool is3DAudioEnabled )
    {
        const std::scoped_lock<std::recursive_mutex> lock( g_asyncSoundManager.resourceMutex() );

        if ( is3DAudioLoopEffectsEnabled != is3DAudioEnabled ) {
            is3DAudioLoopEffectsEnabled = is3DAudioEnabled;
            clearAllAudioLoopEffects();
        }

        // TODO: use another container for sound effects to support more efficient sort and find operations based on the code below.

        std::map<M82::SoundType, std::vector<ChannelAudioLoopEffectInfo>> tempAudioLoopEffects;
        std::swap( tempAudioLoopEffects, currentAudioLoopEffects );

        // Remove all sounds which aren't currently played anymore. This might be the case when Audio::Stop() function is called.
        for ( auto iter = tempAudioLoopEffects.begin(); iter != tempAudioLoopEffects.end(); ) {
            std::vector<ChannelAudioLoopEffectInfo> & existingEffects = iter->second;

            for ( auto effectIter = existingEffects.begin(); effectIter != existingEffects.end(); ) {
                if ( !Mixer::isPlaying( effectIter->channelId ) ) {
                    effectIter = existingEffects.erase( effectIter );
                }
                else {
                    ++effectIter;
                }
            }

            if ( existingEffects.empty() ) {
                iter = tempAudioLoopEffects.erase( iter );
            }
            else {
                ++iter;
            }
        }

        // First find channels with existing sounds and just update them.
        for ( auto iter = soundEffects.begin(); iter != soundEffects.end(); ) {
            const M82::SoundType soundType = iter->first;
            std::vector<AudioManager::AudioLoopEffectInfo> & effectsToAdd = iter->second;
            assert( !effectsToAdd.empty() );

            auto foundSoundTypeIter = tempAudioLoopEffects.find( soundType );
            if ( foundSoundTypeIter == tempAudioLoopEffects.end() ) {
                // This sound type does not exist.
                ++iter;
                continue;
            }

            std::vector<ChannelAudioLoopEffectInfo> & effectsToReplace = foundSoundTypeIter->second;

            // Search for an existing sound which has the exact distance and angle.
            for ( auto soundToAddIter = effectsToAdd.begin(); soundToAddIter != effectsToAdd.end(); ) {
                auto exactSoundEffect = std::find( effectsToReplace.begin(), effectsToReplace.end(), *soundToAddIter );
                if ( exactSoundEffect != effectsToReplace.end() ) {
                    currentAudioLoopEffects[soundType].emplace_back( *exactSoundEffect );
                    effectsToReplace.erase( exactSoundEffect );

                    soundToAddIter = effectsToAdd.erase( soundToAddIter );
                    continue;
                }

                ++soundToAddIter;
            }

            size_t effectsToReplaceCount = std::min( effectsToAdd.size(), effectsToReplace.size() );

            while ( effectsToReplaceCount > 0 ) {
                --effectsToReplaceCount;

                // Find the closest angles to those which are going to be added.
                size_t soundToAddId = 0;
                size_t soundToReplaceId = 0;
                getClosestSoundIdPairByAngle( effectsToAdd, effectsToReplace, soundToAddId, soundToReplaceId );

                currentAudioLoopEffects[soundType].emplace_back( effectsToReplace[soundToReplaceId] );
                effectsToReplace.erase( effectsToReplace.begin() + static_cast<ptrdiff_t>( soundToReplaceId ) );

                ChannelAudioLoopEffectInfo & currentInfo = currentAudioLoopEffects[soundType].back();
                currentInfo = { effectsToAdd[soundToAddId], currentInfo.channelId };
                effectsToAdd.erase( effectsToAdd.begin() + static_cast<ptrdiff_t>( soundToAddId ) );

                assert( is3DAudioEnabled || currentInfo.angle == 0 );

                Mixer::setPosition( currentInfo.channelId, currentInfo.angle, currentInfo.distance );
            }

            if ( effectsToReplace.empty() ) {
                tempAudioLoopEffects.erase( foundSoundTypeIter );
            }

            if ( effectsToAdd.empty() ) {
                iter = soundEffects.erase( iter );
            }
            else {
                ++iter;
            }
        }

        for ( const auto & audioEffectPair : tempAudioLoopEffects ) {
            const std::vector<ChannelAudioLoopEffectInfo> & existingEffects = audioEffectPair.second;

            for ( const ChannelAudioLoopEffectInfo & info : existingEffects ) {
                Mixer::Stop( info.channelId );
            }
        }

        tempAudioLoopEffects.clear();

        // Add new sound effects.
        for ( const auto & audioEffectPair : soundEffects ) {
            const M82::SoundType soundType = audioEffectPair.first;
            const std::vector<AudioManager::AudioLoopEffectInfo> & effects = audioEffectPair.second;
            assert( !effects.empty() );

            for ( const AudioManager::AudioLoopEffectInfo & info : effects ) {
                // It is a new sound effect. Register and play it.
                const std::vector<uint8_t> & audioData = GetWAV( soundType );
                if ( audioData.empty() ) {
                    // Looks like nothing to play. Ignore it.
                    continue;
                }

                assert( is3DAudioEnabled || info.angle == 0 );

                const int channelId = Mixer::Play( audioData.data(), static_cast<uint32_t>( audioData.size() ), true, std::pair{ info.angle, info.distance } );
                if ( channelId < 0 ) {
                    // Unable to play this sound.
                    continue;
                }

                currentAudioLoopEffects[soundType].emplace_back( info, channelId );

                DEBUG_LOG( DBG_GAME, DBG_TRACE, "Playing sound " << M82::GetString( soundType ) )
            }
        }
    }
}

namespace AudioManager
{
    AudioInitializer::AudioInitializer( const std::string & originalAGGFilePath, const std::string & expansionAGGFilePath, const ListFiles & midiSoundFonts )
    {
        if ( Audio::isValid() ) {
            Mixer::SetChannels( 32 );

            // Some platforms (e.g. Linux) may have their own predefined soundfonts, don't overwrite them if we don't have our own
            if ( !midiSoundFonts.empty() ) {
                Music::SetMidiSoundFonts( midiSoundFonts );
            }

            Mixer::setVolume( 100 * Settings::Get().SoundVolume() / 10 );

            Music::setVolume( 100 * Settings::Get().MusicVolume() / 10 );
            Music::SetFadeInMs( 900 );
        }

        assert( !originalAGGFilePath.empty() );
        if ( !g_midiHeroes2AGG.open( originalAGGFilePath ) ) {
            VERBOSE_LOG( "Failed to open HEROES2.AGG file for audio playback." )
        }

        if ( !expansionAGGFilePath.empty() && !g_midiHeroes2xAGG.open( expansionAGGFilePath ) ) {
            VERBOSE_LOG( "Failed to open HEROES2X.AGG file for audio playback." )
        }
    }

    AudioInitializer::~AudioInitializer()
    {
        g_asyncSoundManager.removeAllTasks();
        g_asyncSoundManager.stopWorker();

        wavDataCache.clear();
        MIDDataCache.clear();
        currentAudioLoopEffects.clear();
    }

    MusicRestorer::MusicRestorer()
        : _music( lastRequestedMusicTrackId )
    {
        // Do nothing.
    }

    MusicRestorer::~MusicRestorer()
    {
        // It is assumed that the previous track was looped and should be resumed
        PlayMusicAsync( _music, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );
    }

    void playLoopSoundsAsync( std::map<M82::SoundType, std::vector<AudioLoopEffectInfo>> soundEffects )
    {
        if ( !Audio::isValid() ) {
            return;
        }

        g_asyncSoundManager.pushLoopSound( std::move( soundEffects ), Settings::Get().is3DAudioEnabled() );
    }

    int PlaySound( const int m82 )
    {
        if ( m82 == M82::UNKNOWN ) {
            return -1;
        }

        if ( !Audio::isValid() ) {
            return -1;
        }

        g_asyncSoundManager.removeSoundTasks();

        return PlaySoundImpl( m82 );
    }

    void PlaySoundAsync( const int m82 )
    {
        if ( m82 == M82::UNKNOWN ) {
            return;
        }

        if ( !Audio::isValid() ) {
            return;
        }

        g_asyncSoundManager.pushSound( m82 );
    }

    bool isExternalMusicFileAvailable( const int trackId )
    {
        return !getExternalMusicFile( trackId ).empty();
    }

    void PlayMusic( const int trackId, const Music::PlaybackMode playbackMode )
    {
        if ( !Audio::isValid() ) {
            return;
        }

        lastRequestedMusicTrackId = trackId;

        if ( trackId == MUS::UNUSED || trackId == MUS::UNKNOWN ) {
            return;
        }

        g_asyncSoundManager.removeMusicTask();

        PlayMusicImpl( trackId, Settings::Get().MusicType(), playbackMode );
    }

    void PlayMusicAsync( const int trackId, const Music::PlaybackMode playbackMode )
    {
        if ( !Audio::isValid() ) {
            return;
        }

        lastRequestedMusicTrackId = trackId;

        if ( trackId == MUS::UNUSED || trackId == MUS::UNKNOWN ) {
            return;
        }

        g_asyncSoundManager.pushMusic( trackId, Settings::Get().MusicType(), playbackMode );
    }

    void PlayCurrentMusic()
    {
        if ( !Audio::isValid() ) {
            return;
        }

        g_asyncSoundManager.removeMusicTask();

        const std::scoped_lock<std::recursive_mutex> lock( g_asyncSoundManager.resourceMutex() );

        if ( currentMusicTrackId == MUS::UNUSED || currentMusicTrackId == MUS::UNKNOWN ) {
            return;
        }

        const int trackId = std::exchange( currentMusicTrackId, MUS::UNKNOWN );

        PlayMusicImpl( trackId, Settings::Get().MusicType(), Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );
    }

    void stopSounds()
    {
        if ( !Audio::isValid() ) {
            return;
        }

        g_asyncSoundManager.removeAllSoundTasks();

        const std::scoped_lock<std::recursive_mutex> lock( g_asyncSoundManager.resourceMutex() );

        clearAllAudioLoopEffects();

        Mixer::Stop();
    }

    void ResetAudio()
    {
        if ( !Audio::isValid() ) {
            return;
        }

        g_asyncSoundManager.removeAllTasks();

        const std::scoped_lock<std::recursive_mutex> lock( g_asyncSoundManager.resourceMutex() );

        clearAllAudioLoopEffects();

        Music::Stop();
        Mixer::Stop();

        lastRequestedMusicTrackId = MUS::UNKNOWN;
        currentMusicTrackId = MUS::UNKNOWN;
    }
}
