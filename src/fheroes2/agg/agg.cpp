/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
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
#include "embedded_image.h"
#include "game.h"
#include "localevent.h"
#include "logging.h"
#include "m82.h"
#include "mus.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "tools.h"
#include "xmi.h"
#include "zzlib.h"

namespace
{
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

    std::string getExternalMusicFile( const int musicTrackId, const MUS::EXTERNAL_MUSIC_TYPE musicType, const std::vector<std::string> & directories )
    {
        if ( directories.empty() ) {
            // Nothing to search.
            return {};
        }

        // Instead of relying some generic functions we want to have maximum performance as I/O operations are the slowest.
        std::vector<std::string> possibleFilenames;
        possibleFilenames.reserve( directories.size() );

        for ( const std::string & dir : directories ) {
            possibleFilenames.emplace_back( System::ConcatePath( dir, MUS::getFileName( musicTrackId, musicType, ".flac" ) ) );
        }

        // Search for FLAC files as they have the best audio quality.
        for ( const std::string & filename : possibleFilenames ) {
            if ( System::IsFile( filename ) ) {
                return filename;
            }
        }

        // None of FLAC files found. Try OGG files.
        for ( std::string & filename : possibleFilenames ) {
            fheroes2::replaceStringEnding( filename, ".flac", ".ogg" );

            if ( System::IsFile( filename ) ) {
                return filename;
            }
        }

        // No luck with even OGG. Try with MP3.
        for ( std::string & filename : possibleFilenames ) {
            fheroes2::replaceStringEnding( filename, ".ogg", ".mp3" );

            if ( System::IsFile( filename ) ) {
                return filename;
            }
        }

        // Looks like music file does not exist.
        return {};
    }
}

namespace AGG
{
    struct loop_sound_t
    {
        loop_sound_t( int w, int c )
            : sound( w )
            , channel( c )
        {}

        bool operator==( int m82 ) const
        {
            return m82 == sound;
        }

        int sound;
        int channel;
    };

    fheroes2::AGGFile heroes2_agg;
    fheroes2::AGGFile heroes2x_agg;

    std::map<int, std::vector<u8>> wav_cache;
    std::map<int, std::vector<u8>> mid_cache;
    std::vector<loop_sound_t> loop_sounds;

    const std::vector<u8> & GetWAV( int m82 );
    const std::vector<u8> & GetMID( int xmi );

    void LoadWAV( int m82, std::vector<u8> & );
    void LoadMID( int xmi, std::vector<u8> & );

    std::vector<uint8_t> ReadMusicChunk( const std::string & key, const bool ignoreExpansion = false );

    void PlayMusicInternally( const int mus, const MusicSource musicType, const bool loop );
    void PlaySoundInternally( const int m82, const int soundVolume );
    void LoadLOOPXXSoundsInternally( const std::vector<int> & vols, const int soundVolume );

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

        void pushLoopSound( const std::vector<int> & vols, const int soundVolume )
        {
            _createThreadIfNeeded();

            std::lock_guard<std::mutex> mutexLock( _mutex );

            _loopSoundTasks.emplace( vols, soundVolume );
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
            LoopSoundTask( std::vector<int> vols_, const int soundVolume_ )
                : vols( std::move( vols_ ) )
                , soundVolume( soundVolume_ )
            {}

            std::vector<int> vols;
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
                    const LoopSoundTask loopSoundTask = manager->_loopSoundTasks.back();
                    manager->_loopSoundTasks.pop();

                    manager->_mutex.unlock();

                    LoadLOOPXXSoundsInternally( loopSoundTask.vols, loopSoundTask.soundVolume );
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

void AGG::LoadWAV( int m82, std::vector<u8> & v )
{
    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, M82::GetString( m82 ) );
    const std::vector<u8> & body = ReadMusicChunk( M82::GetString( m82 ) );

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

void AGG::LoadMID( int xmi, std::vector<u8> & v )
{
    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, XMI::GetString( xmi ) );
    const std::vector<uint8_t> & body = ReadMusicChunk( XMI::GetString( xmi ), xmi >= XMI::MIDI_ORIGINAL_KNIGHT );

    if ( !body.empty() ) {
        v = Music::Xmi2Mid( body );
    }
}

const std::vector<u8> & AGG::GetWAV( int m82 )
{
    std::vector<u8> & v = wav_cache[m82];
    if ( Audio::isValid() && v.empty() )
        LoadWAV( m82, v );
    return v;
}

const std::vector<u8> & AGG::GetMID( int xmi )
{
    std::vector<u8> & v = mid_cache[xmi];
    if ( Audio::isValid() && v.empty() )
        LoadMID( xmi, v );
    return v;
}

void AGG::LoadLOOPXXSounds( const std::vector<int> & vols, bool asyncronizedCall )
{
    if ( vols.empty() ) {
        return;
    }

    if ( asyncronizedCall ) {
        g_asyncSoundManager.pushLoopSound( vols, Settings::Get().SoundVolume() );
    }
    else {
        g_asyncSoundManager.sync();
        LoadLOOPXXSoundsInternally( vols, Settings::Get().SoundVolume() );
    }
}

void AGG::LoadLOOPXXSoundsInternally( const std::vector<int> & vols, const int soundVolume )
{
    if ( !Audio::isValid() ) {
        return;
    }

    std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

    // set volume loop sounds
    for ( size_t i = 0; i < vols.size(); ++i ) {
        int vol = vols[i];
        int m82 = M82::GetLOOP00XX( i );
        if ( M82::UNKNOWN == m82 )
            continue;

        // find loops
        std::vector<loop_sound_t>::iterator itl = std::find( loop_sounds.begin(), loop_sounds.end(), m82 );

        if ( itl != loop_sounds.end() ) {
            // unused, stop
            if ( 0 == vol || soundVolume == 0 ) {
                if ( Mixer::isPlaying( ( *itl ).channel ) ) {
                    Mixer::Pause( ( *itl ).channel );
                    Mixer::Volume( ( *itl ).channel, Mixer::MaxVolume() * soundVolume / 10 );
                    Mixer::Stop( ( *itl ).channel );
                }
                ( *itl ).sound = M82::UNKNOWN;
            }
            // used, update volume
            else if ( Mixer::isPlaying( ( *itl ).channel ) ) {
                Mixer::Pause( ( *itl ).channel );
                Mixer::Volume( ( *itl ).channel, vol * soundVolume / 10 );
                Mixer::Resume( ( *itl ).channel );
            }
        }
        else
            // new sound
            if ( 0 != vol ) {
            const std::vector<u8> & v = GetWAV( m82 );
            const int ch = Mixer::Play( &v[0], static_cast<uint32_t>( v.size() ), -1, true );

            if ( 0 <= ch ) {
                Mixer::Pause( ch );
                Mixer::Volume( ch, vol * soundVolume / 10 );
                Mixer::Resume( ch );

                // find unused
                itl = std::find( loop_sounds.begin(), loop_sounds.end(), static_cast<int>( M82::UNKNOWN ) );

                if ( itl != loop_sounds.end() ) {
                    ( *itl ).sound = m82;
                    ( *itl ).channel = ch;
                }
                else
                    loop_sounds.emplace_back( m82, ch );

                DEBUG_LOG( DBG_ENGINE, DBG_TRACE, M82::GetString( m82 ) );
            }
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

    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, M82::GetString( m82 ) );

    const std::vector<u8> & v = AGG::GetWAV( m82 );
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

        std::string filename = getExternalMusicFile( mus, MUS::EXTERNAL_MUSIC_TYPE::DOS_VERSION, musicDirectories );
        if ( filename.empty() ) {
            filename = getExternalMusicFile( mus, MUS::EXTERNAL_MUSIC_TYPE::WIN_VERSION, musicDirectories );
        }

        if ( filename.empty() ) {
            filename = getExternalMusicFile( mus, MUS::EXTERNAL_MUSIC_TYPE::MAPPED, musicDirectories );
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
        const std::vector<u8> & v = GetMID( xmi );
        if ( !v.empty() ) {
            Music::Play( v, loop );

            Game::SetCurrentMusic( mus );
        }
    }
    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, XMI::GetString( xmi ) );
}

void AGG::ResetAudio()
{
    g_asyncSoundManager.sync();

    std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

    Music::Stop();
    Mixer::Stop();

    loop_sounds.clear();
    loop_sounds.reserve( 7 );
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
    loop_sounds.clear();
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
        // The main game resource file is not found.
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
        if ( path.size() < aggLowerCaseFilePath.size() ) {
            continue;
        }

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
