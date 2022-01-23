/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

    bool ReadDataDir( void );
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

        void pushStopMusic()
        {
            _createThreadIfNeeded();

            std::lock_guard<std::mutex> mutexLock( _mutex );

            while ( !_musicTasks.empty() ) {
                _musicTasks.pop();
            }

            _musicTasks.emplace( -1, MUSIC_MIDI_ORIGINAL, true );
            _runFlag = 1;
            _workerNotification.notify_all();
        }

        void pushMusic( const int musicId, const MusicSource musicType, const bool isLooped )
        {
            assert( musicId >= 0 );
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
                    if ( musicTask.musicId >= 0 ) {
                        PlayMusicInternally( musicTask.musicId, musicTask.musicType, musicTask.isLooped );
                    }
                    else {
                        Music::Reset();
                    }
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

/* read data directory */
bool AGG::ReadDataDir( void )
{
    Settings & conf = Settings::Get();

    ListFiles aggs = Settings::FindFiles( "data", ".agg", false );

    // not found agg, exit
    if ( aggs.empty() )
        return false;

    // attach agg files
    for ( ListFiles::const_iterator it = aggs.begin(); it != aggs.end(); ++it ) {
        std::string lower = StringLower( *it );
        if ( std::string::npos != lower.find( "heroes2.agg" ) && !heroes2_agg.isGood() ) {
            heroes2_agg.open( *it );
            g_midiHeroes2AGG.open( *it );
        }
        if ( std::string::npos != lower.find( "heroes2x.agg" ) && !heroes2x_agg.isGood() ) {
            heroes2x_agg.open( *it );
            g_midiHeroes2xAGG.open( *it );
        }
    }

    conf.EnablePriceOfLoyaltySupport( heroes2x_agg.isGood() );

    return heroes2_agg.isGood();
}

std::vector<uint8_t> AGG::ReadChunk( const std::string & key )
{
    if ( heroes2x_agg.isGood() ) {
        const std::vector<uint8_t> & buf = heroes2x_agg.read( key );
        if ( !buf.empty() )
            return buf;
    }

    return heroes2_agg.read( key );
}

std::vector<uint8_t> AGG::ReadMusicChunk( const std::string & key, const bool ignoreExpansion )
{
    if ( !ignoreExpansion && g_midiHeroes2xAGG.isGood() ) {
        const std::vector<uint8_t> & buf = g_midiHeroes2xAGG.read( key );
        if ( !buf.empty() )
            return buf;
    }

    return g_midiHeroes2AGG.read( key );
}

/* load 82M object to AGG::Cache in Audio::CVT */
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

/* load XMI object */
void AGG::LoadMID( int xmi, std::vector<u8> & v )
{
    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, XMI::GetString( xmi ) );
    const std::vector<uint8_t> & body = ReadMusicChunk( XMI::GetString( xmi ), xmi >= XMI::MIDI_ORIGINAL_KNIGHT );

    if ( !body.empty() ) {
        v = Music::Xmi2Mid( body );
    }
}

/* return CVT */
const std::vector<u8> & AGG::GetWAV( int m82 )
{
    std::vector<u8> & v = wav_cache[m82];
    if ( Audio::isValid() && v.empty() )
        LoadWAV( m82, v );
    return v;
}

/* return MID */
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

/* wrapper Audio::Play */
void AGG::PlaySound( int m82, bool asyncronizedCall )
{
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

/* wrapper Audio::Play */
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

    const std::string prefix_music( "music" );

    bool isSongFound = false;

    if ( musicType == MUSIC_EXTERNAL ) {
        std::string filename = Settings::GetLastFile( prefix_music, MUS::GetString( mus, MUS::OGG_MUSIC_TYPE::DOS_VERSION ) );

        if ( !System::IsFile( filename ) ) {
            filename = Settings::GetLastFile( prefix_music, MUS::GetString( mus, MUS::OGG_MUSIC_TYPE::WIN_VERSION ) );
            if ( !System::IsFile( filename ) ) {
                filename.clear();
            }
        }

        if ( filename.empty() ) {
            filename = Settings::GetLastFile( prefix_music, MUS::GetString( mus, MUS::OGG_MUSIC_TYPE::MAPPED ) );

            if ( !System::IsFile( filename ) ) {
                StringReplace( filename, ".ogg", ".mp3" );

                if ( !System::IsFile( filename ) ) {
                    DEBUG_LOG( DBG_ENGINE, DBG_WARN,
                               "error read file: " << Settings::GetLastFile( prefix_music, MUS::GetString( mus, MUS::OGG_MUSIC_TYPE::MAPPED ) ) << ", skipping..." );
                    filename.clear();
                }
            }
        }

        if ( !filename.empty() ) {
            Music::Play( filename, loop );
            isSongFound = true;

            Game::SetCurrentMusic( mus );
        }
        DEBUG_LOG( DBG_ENGINE, DBG_TRACE, MUS::GetString( mus, MUS::OGG_MUSIC_TYPE::MAPPED ) );
    }

    if ( !isSongFound ) {
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
}

// This exists to avoid exposing AGG::ReadChunk
std::vector<u8> AGG::LoadBINFRM( const char * frm_file )
{
    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, frm_file );
    return AGG::ReadChunk( frm_file );
}

void AGG::ResetMixer( bool asyncronizedCall /* = false */ )
{
    if ( asyncronizedCall ) {
        g_asyncSoundManager.pushStopMusic();
        Mixer::Stop();
    }
    else {
        g_asyncSoundManager.sync();

        std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

        Mixer::Reset();
    }
    loop_sounds.clear();
    loop_sounds.reserve( 7 );
}

AGG::AGGInitializer::AGGInitializer()
{
    if ( ReadDataDir() ) {
        return;
    }

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Image & image = CreateImageFromZlib( 290, 190, errorMessage, sizeof( errorMessage ), false );

    display.fill( 0 );
    fheroes2::Copy( image, 0, 0, display, ( display.width() - image.width() ) / 2, ( display.height() - image.height() ) / 2, image.width(), image.height() );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() && !le.KeyPress() && !le.MouseClickLeft() ) {
        // Do nothing.
    }

    DEBUG_LOG( DBG_ENGINE, DBG_WARN, "No data files found." );
    throw std::logic_error( "No data files found." );
}

AGG::AGGInitializer::~AGGInitializer()
{
    wav_cache.clear();
    mid_cache.clear();
    loop_sounds.clear();
}
