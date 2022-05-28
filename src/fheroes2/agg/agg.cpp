/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <array>
#include <cassert>
#include <condition_variable>
#include <map>
#include <queue>
#include <thread>
#include <utility>

#include "agg.h"
#include "agg_file.h"
#include "audio.h"
#include "dir.h"
#include "game.h"
#include "localevent.h"
#include "logging.h"
#include "m82.h"
#include "mus.h"
#include "settings.h"
#include "system.h"
#include "tools.h"
#include "xmi.h"

namespace
{
    struct MusicFileType
    {
        explicit MusicFileType( const MUS::EXTERNAL_MUSIC_TYPE type_ )
            : type( type_ )
        {
            // Do nothing.
        }

        MUS::EXTERNAL_MUSIC_TYPE type = MUS::EXTERNAL_MUSIC_TYPE::WIN_VERSION;

        std::array<std::string, 3> extension{ ".ogg", ".mp3", ".flac" };
    };

    const std::string externalMusicDirectory( "music" );

    std::vector<std::string> getMusicDirectories()
    {
        std::vector<std::string> directories;
        for ( const std::string & dir : Settings::GetRootDirs() ) {
            std::string fullDirectoryPath = System::ConcatePath( dir, externalMusicDirectory );
            if ( System::IsDirectory( fullDirectoryPath ) ) {
                directories.emplace_back( std::move( fullDirectoryPath ) );
            }
        }

        return directories;
    }

    bool findMusicFile( const std::vector<std::string> & directories, const std::string & fileName, std::string & fullPath )
    {
        for ( const std::string & dir : directories ) {
            ListFiles musicFilePaths;
            musicFilePaths.ReadDir( dir, fileName, false );
            if ( musicFilePaths.empty() ) {
                continue;
            }

            std::string correctFilePath = System::ConcatePath( dir, fileName );
            correctFilePath = StringLower( correctFilePath );

            for ( std::string & path : musicFilePaths ) {
                const std::string temp = StringLower( path );
                if ( temp == correctFilePath ) {
                    // Avoid string copy.
                    std::swap( fullPath, path );
                    return true;
                }
            }
        }

        return false;
    }

    std::string getExternalMusicFile( const int musicTrackId, const std::vector<std::string> & directories, MusicFileType & musicType )
    {
        if ( directories.empty() ) {
            // Nothing to search.
            return {};
        }

        std::string fullPath;

        std::string fileName = MUS::getFileName( musicTrackId, musicType.type, musicType.extension[0].c_str() );
        if ( findMusicFile( directories, fileName, fullPath ) ) {
            return fullPath;
        }

        fheroes2::replaceStringEnding( fileName, musicType.extension[0].c_str(), musicType.extension[1].c_str() );
        if ( findMusicFile( directories, fileName, fullPath ) ) {
            // Swap extensions to improve cache hit.
            std::swap( musicType.extension[0], musicType.extension[1] );
            return fullPath;
        }

        fheroes2::replaceStringEnding( fileName, musicType.extension[1].c_str(), musicType.extension[2].c_str() );
        if ( findMusicFile( directories, fileName, fullPath ) ) {
            // Swap extensions to improve cache hit.
            std::swap( musicType.extension[0], musicType.extension[2] );
            return fullPath;
        }

        // Looks like music file does not exist.
        return {};
    }
}

namespace AGG
{
    struct ChannelAudioLoopEffectInfo : public AudioLoopEffectInfo
    {
        ChannelAudioLoopEffectInfo() = default;

        ChannelAudioLoopEffectInfo( const AudioLoopEffectInfo & info, const int channelId_ )
            : AudioLoopEffectInfo( info )
            , channelId( channelId_ )
        {
            // Do nothing.
        }

        int channelId{ -1 };
    };

    std::map<M82::SoundType, std::vector<ChannelAudioLoopEffectInfo>> currentAudioLoopEffects;

    fheroes2::AGGFile heroes2_agg;
    fheroes2::AGGFile heroes2x_agg;

    std::map<int, std::vector<uint8_t>> wav_cache;
    std::map<int, std::vector<uint8_t>> mid_cache;

    const std::vector<uint8_t> & GetWAV( int m82 );
    const std::vector<uint8_t> & GetMID( int xmi );

    void LoadWAV( int m82, std::vector<uint8_t> & v );
    void LoadMID( int xmi, std::vector<uint8_t> & v );

    std::vector<uint8_t> ReadMusicChunk( const std::string & key, const bool ignoreExpansion = false );

    void PlayMusicInternally( const int mus, const MusicSource musicType, const bool loop );
    void PlaySoundInternally( const int m82, const int soundVolume );
    void LoadLOOPXXSoundsInternally( std::map<M82::SoundType, std::vector<AudioLoopEffectInfo>> soundEffects, const int soundVolume );

    fheroes2::AGGFile g_midiHeroes2AGG;
    fheroes2::AGGFile g_midiHeroes2xAGG;

    // SDL MIDI player is single threaded library which requires a lot of time for some long midi compositions.
    // This leads to a situation of short application freeze while a hero crosses terrains or ending a battle.
    // The only way to avoid this is to fire MIDI requests asynchronously and synchronize them if needed.
    class AsyncSoundManager
    {
    public:
        AsyncSoundManager()
            : _exitFlag( 0 )
            , _runFlag( 1 )
        {}

        AsyncSoundManager( const AsyncSoundManager & ) = delete;

        ~AsyncSoundManager()
        {
            if ( _worker ) {
                {
                    std::lock_guard<std::mutex> guard( _mutex );

                    _exitFlag = 1;
                    _runFlag = 1;
                    _workerNotification.notify_all();
                }

                _worker->join();
                _worker.reset();
            }
        }

        AsyncSoundManager & operator=( const AsyncSoundManager & ) = delete;

        void pushMusic( const int musicId, const MusicSource musicType, const bool isLooped )
        {
            _createThreadIfNeeded();

            std::lock_guard<std::mutex> mutexLock( _mutex );

            while ( !_musicTasks.empty() ) {
                _musicTasks.pop();
            }

            _musicTasks.emplace( musicId, musicType, isLooped );
            _runFlag = 1;
            _workerNotification.notify_all();
        }

        void pushSound( const int m82Sound, const int soundVolume )
        {
            _createThreadIfNeeded();

            std::lock_guard<std::mutex> mutexLock( _mutex );

            _soundTasks.emplace( m82Sound, soundVolume );
            _runFlag = 1;
            _workerNotification.notify_all();
        }

        void pushLoopSound( std::map<M82::SoundType, std::vector<AudioLoopEffectInfo>> vols, const int soundVolume )
        {
            _createThreadIfNeeded();

            std::lock_guard<std::mutex> mutexLock( _mutex );

            _loopSoundTasks.emplace( std::move( vols ), soundVolume );
            _runFlag = 1;
            _workerNotification.notify_all();
        }

        void sync()
        {
            std::lock_guard<std::mutex> mutexLock( _mutex );

            while ( !_musicTasks.empty() ) {
                _musicTasks.pop();
            }

            while ( !_soundTasks.empty() ) {
                _soundTasks.pop();
            }

            while ( !_loopSoundTasks.empty() ) {
                _loopSoundTasks.pop();
            }
        }

        // This mutex is used to avoid access to global objects and classes related to SDL Mixer.
        std::mutex & resourceMutex()
        {
            return _resourceMutex;
        }

    private:
        struct MusicTask
        {
            MusicTask( const int musicId_, const MusicSource musicType_, const bool isLooped_ )
                : musicId( musicId_ )
                , musicType( musicType_ )
                , isLooped( isLooped_ )
            {}

            int musicId;
            MusicSource musicType;
            bool isLooped;
        };

        struct SoundTask
        {
            SoundTask( const int m82Sound_, const int soundVolume_ )
                : m82Sound( m82Sound_ )
                , soundVolume( soundVolume_ )
            {}

            int m82Sound;
            int soundVolume;
        };

        struct LoopSoundTask
        {
            LoopSoundTask( std::map<M82::SoundType, std::vector<AudioLoopEffectInfo>> effects, const int soundVolume_ )
                : soundEffects( std::move( effects ) )
                , soundVolume( soundVolume_ )
            {}

            std::map<M82::SoundType, std::vector<AudioLoopEffectInfo>> soundEffects;
            int soundVolume;
        };

        std::unique_ptr<std::thread> _worker;
        std::mutex _mutex;

        std::condition_variable _workerNotification;
        std::condition_variable _masterNotification;

        std::queue<MusicTask> _musicTasks;
        std::queue<SoundTask> _soundTasks;
        std::queue<LoopSoundTask> _loopSoundTasks;

        uint8_t _exitFlag;
        uint8_t _runFlag;

        std::mutex _resourceMutex;

        void _createThreadIfNeeded()
        {
            if ( !_worker ) {
                _runFlag = 1;
                _worker.reset( new std::thread( AsyncSoundManager::_workerThread, this ) );

                std::unique_lock<std::mutex> mutexLock( _mutex );
                _masterNotification.wait( mutexLock, [this] { return _runFlag == 0; } );
            }
        }

        static void _workerThread( AsyncSoundManager * manager )
        {
            assert( manager != nullptr );

            {
                std::lock_guard<std::mutex> guard( manager->_mutex );
                manager->_runFlag = 0;
                manager->_masterNotification.notify_one();
            }

            while ( manager->_exitFlag == 0 ) {
                std::unique_lock<std::mutex> mutexLock( manager->_mutex );
                manager->_workerNotification.wait( mutexLock, [manager] { return manager->_runFlag == 1; } );
                mutexLock.unlock();

                if ( manager->_exitFlag )
                    break;

                manager->_mutex.lock();

                if ( !manager->_soundTasks.empty() ) {
                    const SoundTask soundTask = manager->_soundTasks.back();
                    manager->_soundTasks.pop();

                    manager->_mutex.unlock();

                    PlaySoundInternally( soundTask.m82Sound, soundTask.soundVolume );
                }
                else if ( !manager->_loopSoundTasks.empty() ) {
                    LoopSoundTask loopSoundTask = manager->_loopSoundTasks.back();
                    manager->_loopSoundTasks.pop();

                    manager->_mutex.unlock();

                    LoadLOOPXXSoundsInternally( std::move( loopSoundTask.soundEffects ), loopSoundTask.soundVolume );
                }
                else if ( !manager->_musicTasks.empty() ) {
                    const MusicTask musicTask = manager->_musicTasks.back();

                    while ( !manager->_musicTasks.empty() ) {
                        manager->_musicTasks.pop();
                    }

                    manager->_mutex.unlock();

                    PlayMusicInternally( musicTask.musicId, musicTask.musicType, musicTask.isLooped );
                }
                else {
                    manager->_runFlag = 0;

                    manager->_mutex.unlock();
                }
            }
        }
    };

    AsyncSoundManager g_asyncSoundManager;
}

std::vector<uint8_t> AGG::ReadChunk( const std::string & key )
{
    if ( heroes2x_agg.isGood() ) {
        // Make sure that the below container is not const and not a reference
        // so returning it from the function will invoke a move constructor instead of copy constructor.
        std::vector<uint8_t> buf = heroes2x_agg.read( key );
        if ( !buf.empty() )
            return buf;
    }

    return heroes2_agg.read( key );
}

std::vector<uint8_t> AGG::ReadMusicChunk( const std::string & key, const bool ignoreExpansion )
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

void AGG::LoadWAV( int m82, std::vector<uint8_t> & v )
{
    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, M82::GetString( m82 ) )
    const std::vector<uint8_t> & body = ReadMusicChunk( M82::GetString( m82 ) );

    if ( !body.empty() ) {
        // create WAV format
        StreamBuf wavHeader( 44 );
        wavHeader.putLE32( 0x46464952 ); // RIFF
        wavHeader.putLE32( static_cast<uint32_t>( body.size() ) + 0x24 ); // size
        wavHeader.putLE32( 0x45564157 ); // WAVE
        wavHeader.putLE32( 0x20746D66 ); // FMT
        wavHeader.putLE32( 0x10 ); // size_t
        wavHeader.putLE16( 0x01 ); // format
        wavHeader.putLE16( 0x01 ); // channels
        wavHeader.putLE32( 22050 ); // samples
        wavHeader.putLE32( 22050 ); // byteper
        wavHeader.putLE16( 0x01 ); // align
        wavHeader.putLE16( 0x08 ); // bitsper
        wavHeader.putLE32( 0x61746164 ); // DATA
        wavHeader.putLE32( static_cast<uint32_t>( body.size() ) ); // size

        v.reserve( body.size() + 44 );
        v.assign( wavHeader.data(), wavHeader.data() + 44 );
        v.insert( v.begin() + 44, body.begin(), body.end() );
    }
}

void AGG::LoadMID( int xmi, std::vector<uint8_t> & v )
{
    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, XMI::GetString( xmi ) )
    const std::vector<uint8_t> & body = ReadMusicChunk( XMI::GetString( xmi ), xmi >= XMI::MIDI_ORIGINAL_KNIGHT );

    if ( !body.empty() ) {
        v = Music::Xmi2Mid( body );
    }
}

const std::vector<uint8_t> & AGG::GetWAV( int m82 )
{
    std::vector<uint8_t> & v = wav_cache[m82];
    if ( Audio::isValid() && v.empty() )
        LoadWAV( m82, v );
    return v;
}

const std::vector<uint8_t> & AGG::GetMID( int xmi )
{
    std::vector<uint8_t> & v = mid_cache[xmi];
    if ( Audio::isValid() && v.empty() )
        LoadMID( xmi, v );
    return v;
}

void AGG::LoadLOOPXXSounds( std::map<M82::SoundType, std::vector<AudioLoopEffectInfo>> soundEffects, bool asyncronizedCall )
{
    if ( asyncronizedCall ) {
        g_asyncSoundManager.pushLoopSound( std::move( soundEffects ), Settings::Get().SoundVolume() );
    }
    else {
        g_asyncSoundManager.sync();
        LoadLOOPXXSoundsInternally( std::move( soundEffects ), Settings::Get().SoundVolume() );
    }
}

void AGG::LoadLOOPXXSoundsInternally( std::map<M82::SoundType, std::vector<AudioLoopEffectInfo>> soundEffects, const int soundVolume )
{
    if ( !Audio::isValid() ) {
        return;
    }

    if ( soundVolume == 0 ) {
        // The volume is 0. Remove all sound effects.
        for ( auto iter = currentAudioLoopEffects.begin(); iter != currentAudioLoopEffects.end(); ++iter ) {
            std::vector<ChannelAudioLoopEffectInfo> & existingEffects = iter->second;

            for ( const ChannelAudioLoopEffectInfo & info : existingEffects ) {
                if ( Mixer::isPlaying( info.channelId ) ) {
                    Mixer::Pause( info.channelId );
                    Mixer::Volume( info.channelId, Mixer::MaxVolume() * soundVolume / 10 );
                    Mixer::Stop( info.channelId );
                }
            }
        }

        return;
    }

    std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

    std::map<M82::SoundType, std::vector<ChannelAudioLoopEffectInfo>> temp;
    std::swap( temp, currentAudioLoopEffects );

    // First find channels with existing sounds and just update them.
    for ( auto iter = soundEffects.begin(); iter != soundEffects.end(); ) {
        const M82::SoundType soundType = iter->first;
        std::vector<AudioLoopEffectInfo> & effects = iter->second;
        assert( !effects.empty() );

        auto foundSoundTypeIter = temp.find( soundType );
        if ( foundSoundTypeIter == temp.end() ) {
            // This sound type does not exist.
            ++iter;
            continue;
        }

        std::vector<ChannelAudioLoopEffectInfo> & existingEffects = foundSoundTypeIter->second;

        size_t elementSize = std::min( effects.size(), existingEffects.size() );
        while ( elementSize > 0 ) {
            --elementSize;

            currentAudioLoopEffects[soundType].emplace_back( std::move( existingEffects.back() ) );
            existingEffects.pop_back();

            ChannelAudioLoopEffectInfo & currenInfo = currentAudioLoopEffects[soundType].back();
            currenInfo = { effects.back(), currenInfo.channelId };
            effects.pop_back();

            if ( Mixer::isPlaying( currenInfo.channelId ) ) {
                Mixer::Pause( currenInfo.channelId );
                Mixer::applySoundEffect( currenInfo.channelId, currenInfo.angle, currenInfo.volumePercentage );
                Mixer::Volume( currenInfo.channelId, Mixer::MaxVolume() * soundVolume / 10 );
                Mixer::Resume( currenInfo.channelId );
            }
        }

        if ( existingEffects.empty() ) {
            temp.erase( foundSoundTypeIter );
        }

        auto currentIter = iter;
        ++iter;

        if ( effects.empty() ) {
            soundEffects.erase( currentIter );
        }
    }

    // Stop all running sound effects.
    for ( auto iter = temp.begin(); iter != temp.end(); ++iter ) {
        std::vector<ChannelAudioLoopEffectInfo> & existingEffects = iter->second;

        for ( const ChannelAudioLoopEffectInfo & info : existingEffects ) {
            if ( Mixer::isPlaying( info.channelId ) ) {
                Mixer::Pause( info.channelId );
                Mixer::Volume( info.channelId, Mixer::MaxVolume() * soundVolume / 10 );
                Mixer::Stop( info.channelId );
            }
        }
    }

    temp.clear();

    // Add new sound effects.
    for ( auto iter = soundEffects.begin(); iter != soundEffects.end(); ++iter ) {
        const M82::SoundType soundType = iter->first;
        std::vector<AudioLoopEffectInfo> & effects = iter->second;
        assert( !effects.empty() );

        for ( const AudioLoopEffectInfo & info : effects ) {
            // It is a new sound effect. Register and play it.
            const std::vector<uint8_t> & audioData = GetWAV( soundType );
            if ( audioData.empty() ) {
                // Looks like nothing to play. Ignore it.
                continue;
            }

            const int channelId = Mixer::PlayFromDistance( &audioData[0], static_cast<uint32_t>( audioData.size() ), -1, true, info.angle, info.volumePercentage );
            if ( channelId < 0 ) {
                // Unable to play this audio. It is probably an invalid audio sample.
                continue;
            }

            // Adjust channel based on given parameters.
            Mixer::Pause( channelId );
            Mixer::Volume( channelId, Mixer::MaxVolume() * soundVolume / 10 );
            Mixer::Resume( channelId );

            currentAudioLoopEffects[soundType].emplace_back( ChannelAudioLoopEffectInfo( info, channelId ) );

            DEBUG_LOG( DBG_ENGINE, DBG_TRACE, "Playing sound " << M82::GetString( soundType ) )
        }
    }
}

void AGG::PlaySound( int m82, bool asyncronizedCall )
{
    if ( m82 == M82::UNKNOWN ) {
        return;
    }

    if ( asyncronizedCall ) {
        g_asyncSoundManager.pushSound( m82, Settings::Get().SoundVolume() );
    }
    else {
        g_asyncSoundManager.sync();
        PlaySoundInternally( m82, Settings::Get().SoundVolume() );
    }
}

void AGG::PlaySoundInternally( const int m82, const int soundVolume )
{
    if ( !Audio::isValid() ) {
        return;
    }

    std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, M82::GetString( m82 ) )

    const std::vector<uint8_t> & v = AGG::GetWAV( m82 );
    const int ch = Mixer::Play( &v[0], static_cast<uint32_t>( v.size() ), -1, false );

    if ( ch >= 0 ) {
        Mixer::Pause( ch );
        Mixer::Volume( ch, Mixer::MaxVolume() * soundVolume / 10 );
        Mixer::Resume( ch );
    }
}

void AGG::PlayMusic( int mus, bool loop, bool asyncronizedCall )
{
    if ( MUS::UNUSED == mus || MUS::UNKNOWN == mus ) {
        return;
    }

    if ( asyncronizedCall ) {
        g_asyncSoundManager.pushMusic( mus, Settings::Get().MusicType(), loop );
    }
    else {
        g_asyncSoundManager.sync();
        PlayMusicInternally( mus, Settings::Get().MusicType(), loop );
    }
}

void AGG::PlayMusicInternally( const int mus, const MusicSource musicType, const bool loop )
{
    if ( !Audio::isValid() ) {
        return;
    }

    std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

    if ( Game::CurrentMusic() == mus && Music::isPlaying() ) {
        return;
    }

    if ( musicType == MUSIC_EXTERNAL ) {
        const std::vector<std::string> & musicDirectories = getMusicDirectories();

        // To avoid extra I/O calls to data storage it might be useful to remember the last successful type of music and try to search for it next time.
        static std::array<MusicFileType, 3> musicFileTypes{ MusicFileType( MUS::EXTERNAL_MUSIC_TYPE::DOS_VERSION ),
                                                            MusicFileType( MUS::EXTERNAL_MUSIC_TYPE::WIN_VERSION ), MusicFileType( MUS::EXTERNAL_MUSIC_TYPE::MAPPED ) };

        std::string filename;

        for ( size_t i = 0; i < musicFileTypes.size(); ++i ) {
            filename = getExternalMusicFile( mus, musicDirectories, musicFileTypes[i] );
            if ( !filename.empty() ) {
                if ( i > 0 ) {
                    // Swap music types to improve cache hit.
                    std::swap( musicFileTypes[0], musicFileTypes[i] );
                }
                break;
            }
        }

        if ( filename.empty() ) {
            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "Cannot find a file for " << mus << " track." )
        }
        else {
            Music::Play( filename, loop );

            Game::SetCurrentMusic( mus );

            DEBUG_LOG( DBG_ENGINE, DBG_TRACE, MUS::getFileName( mus, MUS::EXTERNAL_MUSIC_TYPE::MAPPED, " " ) )

            return;
        }
    }

    // Check if music needs to be pulled from HEROES2X
    int xmi = XMI::UNKNOWN;
    if ( musicType == MUSIC_MIDI_EXPANSION ) {
        xmi = XMI::FromMUS( mus, g_midiHeroes2xAGG.isGood() );
    }

    if ( XMI::UNKNOWN == xmi ) {
        xmi = XMI::FromMUS( mus, false );
    }

    if ( XMI::UNKNOWN != xmi ) {
        const std::vector<uint8_t> & v = GetMID( xmi );
        if ( !v.empty() ) {
            Music::Play( v, loop );

            Game::SetCurrentMusic( mus );
        }
    }
    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, XMI::GetString( xmi ) )
}

void AGG::ResetAudio()
{
    g_asyncSoundManager.sync();

    std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

    Music::Stop();
    Mixer::Stop();

    currentAudioLoopEffects.clear();
}

AGG::AGGInitializer::AGGInitializer()
{
    if ( init() ) {
        return;
    }

    throw std::logic_error( "No AGG data files found." );
}

AGG::AGGInitializer::~AGGInitializer()
{
    wav_cache.clear();
    mid_cache.clear();
    currentAudioLoopEffects.clear();
}

bool AGG::AGGInitializer::init()
{
    const ListFiles aggFileNames = Settings::FindFiles( "data", ".agg", false );
    if ( aggFileNames.empty() ) {
        return false;
    }

    const std::string heroes2AggFileName( "heroes2.agg" );
    std::string heroes2AggFilePath;
    std::string aggLowerCaseFilePath;

    for ( const std::string & path : aggFileNames ) {
        if ( path.size() < heroes2AggFileName.size() ) {
            // Obviously this is not a correct file.
            continue;
        }

        std::string tempPath = StringLower( path );

        if ( tempPath.compare( tempPath.size() - heroes2AggFileName.size(), heroes2AggFileName.size(), heroes2AggFileName ) == 0 ) {
            heroes2AggFilePath = path;
            aggLowerCaseFilePath = std::move( tempPath );
            break;
        }
    }

    if ( heroes2AggFilePath.empty() ) {
        // The main game resource file was not found.
        return false;
    }

    if ( !heroes2_agg.open( heroes2AggFilePath ) ) {
        return false;
    }

    if ( !g_midiHeroes2AGG.open( heroes2AggFilePath ) ) {
        // How is it even possible that for the second time the file is not readable?
        assert( 0 );
        return false;
    }

    // Find "heroes2x.agg" file.
    std::string heroes2XAggFilePath;
    fheroes2::replaceStringEnding( aggLowerCaseFilePath, ".agg", "x.agg" );

    for ( const std::string & path : aggFileNames ) {
        const std::string tempPath = StringLower( path );
        if ( tempPath == aggLowerCaseFilePath ) {
            heroes2XAggFilePath = path;
            break;
        }
    }

    if ( !heroes2XAggFilePath.empty() ) {
        heroes2x_agg.open( heroes2XAggFilePath );
        g_midiHeroes2xAGG.open( heroes2XAggFilePath );
    }

    Settings::Get().EnablePriceOfLoyaltySupport( heroes2x_agg.isGood() );

    return true;
}
