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
#include <iostream>
#include <map>
#include <queue>
#include <thread>

#include "agg.h"
#include "agg_file.h"
#include "audio.h"
#include "audio_cdrom.h"
#include "audio_mixer.h"
#include "audio_music.h"
#include "dir.h"
#include "font.h"
#include "game.h"
#include "gamedefs.h"
#include "localevent.h"
#include "logging.h"
#include "m82.h"
#include "mus.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "text.h"
#include "xmi.h"

#ifdef WITH_ZLIB
#include "embedded_image.h"
#include "zzlib.h"
#endif

namespace AGG
{
    // struct fnt_cache_t
    // {
    //     Surface sfs[4]; /* small_white, small_yellow, medium_white, medium_yellow */
    // };

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

    std::map<int, std::vector<u8> > wav_cache;
    std::map<int, std::vector<u8> > mid_cache;
    std::vector<loop_sound_t> loop_sounds;
    // std::map<u32, fnt_cache_t> fnt_cache;

#ifdef WITH_TTF
    FontTTF * fonts; /* small, medium */

    // void LoadTTFChar( u32 );
#endif

    const std::vector<u8> & GetWAV( int m82 );
    const std::vector<u8> & GetMID( int xmi );

    void LoadWAV( int m82, std::vector<u8> & );
    void LoadMID( int xmi, std::vector<u8> & );

    void LoadFNT( void );

    bool ReadDataDir( void );
    std::vector<uint8_t> ReadMusicChunk( const std::string & key, const bool ignoreExpansion = false );

    void PlayMusicInternally( const int mus, const bool loop );
    void PlaySoundInternally( const int m82 );
    void LoadLOOPXXSoundsInternally( const std::vector<int> & vols );

    /* return letter sprite */
    // Surface GetUnicodeLetter( u32 ch, u32 ft )
    // {
    //     bool ttf_valid = fonts[0].isValid() && fonts[1].isValid();
    //
    //     if ( !ttf_valid )
    //         return Surface();
    //
    //     if ( !fnt_cache[ch].sfs[0].isValid() )
    //         LoadTTFChar( ch );
    //
    //     switch ( ft ) {
    //     case Font::YELLOW_SMALL:
    //         return fnt_cache[ch].sfs[1];
    //     case Font::BIG:
    //         return fnt_cache[ch].sfs[2];
    //     case Font::YELLOW_BIG:
    //         return fnt_cache[ch].sfs[3];
    //     default:
    //         break;
    //     }
    //
    //     return fnt_cache[ch].sfs[0];
    // }

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

        ~AsyncSoundManager()
        {
            if ( _worker ) {
                _mutex.lock();

                _exitFlag = 1;
                _runFlag = 1;
                _workerNotification.notify_all();

                _mutex.unlock();

                _worker->join();
                _worker.reset();
            }
        }

        void pushMusic( const int musicId, const bool isLooped )
        {
            _createThreadIfNeeded();

            std::lock_guard<std::mutex> mutexLock( _mutex );

            while ( !_musicTasks.empty() ) {
                _musicTasks.pop();
            }

            _musicTasks.emplace( musicId, isLooped );
            _runFlag = 1;
            _workerNotification.notify_all();
        }

        void pushSound( const int m82Sound )
        {
            _createThreadIfNeeded();

            std::lock_guard<std::mutex> mutexLock( _mutex );

            _soundTasks.emplace( m82Sound );
            _runFlag = 1;
            _workerNotification.notify_all();
        }

        void pushLoopSound( const std::vector<int> & vols )
        {
            _createThreadIfNeeded();

            std::lock_guard<std::mutex> mutexLock( _mutex );

            _loopSoundTasks.emplace( vols );
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
        std::unique_ptr<std::thread> _worker;
        std::mutex _mutex;

        std::condition_variable _workerNotification;
        std::condition_variable _masterNotification;

        std::queue<std::pair<int, bool> > _musicTasks;
        std::queue<int> _soundTasks;
        std::queue<std::vector<int> > _loopSoundTasks;

        uint8_t _exitFlag;
        uint8_t _runFlag;

        std::mutex _resourceMutex;

        void _createThreadIfNeeded()
        {
            if ( !_worker ) {
                _runFlag = 1;
                _worker.reset( new std::thread( AsyncSoundManager::_workerThread, this ) );

                std::unique_lock<std::mutex> mutexLock( _mutex );
                _masterNotification.wait( mutexLock, [&] { return _runFlag == 0; } );
            }
        }

        static void _workerThread( AsyncSoundManager * manager )
        {
            assert( manager != nullptr );

            manager->_mutex.lock();
            manager->_runFlag = 0;
            manager->_masterNotification.notify_one();
            manager->_mutex.unlock();

            while ( manager->_exitFlag == 0 ) {
                std::unique_lock<std::mutex> mutexLock( manager->_mutex );
                manager->_workerNotification.wait( mutexLock, [&] { return manager->_runFlag == 1; } );
                mutexLock.unlock();

                if ( manager->_exitFlag )
                    break;

                manager->_mutex.lock();

                if ( !manager->_soundTasks.empty() ) {
                    const int m82Sound = manager->_soundTasks.back();
                    manager->_soundTasks.pop();

                    manager->_mutex.unlock();

                    PlaySoundInternally( m82Sound );
                }
                else if ( !manager->_loopSoundTasks.empty() ) {
                    const std::vector<int> vols = manager->_loopSoundTasks.back();
                    manager->_loopSoundTasks.pop();

                    manager->_mutex.unlock();

                    LoadLOOPXXSoundsInternally( vols );
                }
                else if ( !manager->_musicTasks.empty() ) {
                    const std::pair<int, bool> musicInfo = manager->_musicTasks.back();

                    while ( !manager->_musicTasks.empty() ) {
                        manager->_musicTasks.pop();
                    }

                    manager->_mutex.unlock();

                    PlayMusicInternally( musicInfo.first, musicInfo.second );
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

    ListFiles aggs = Settings::GetListFiles( "data", ".agg" );
    const std::string & other_data = conf.GetDataParams();

    if ( other_data.size() && other_data != "data" )
        aggs.Append( Settings::GetListFiles( other_data, ".agg" ) );

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

    conf.SetPriceLoyaltyVersion( heroes2x_agg.isGood() );

    return heroes2_agg.isGood();
}

std::vector<uint8_t> AGG::ReadChunk( const std::string & key, bool ignoreExpansion )
{
    if ( !ignoreExpansion && heroes2x_agg.isGood() ) {
        const std::vector<u8> & buf = heroes2x_agg.read( key );
        if ( buf.size() )
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
#ifdef WITH_MIXER
    const Settings & conf = Settings::Get();

    if ( conf.UseAltResource() ) {
        std::string name = StringLower( M82::GetString( m82 ) );
        std::string prefix_sounds = System::ConcatePath( "files", "sounds" );

        // ogg
        StringReplace( name, ".82m", ".ogg" );
        std::string sound = Settings::GetLastFile( prefix_sounds, name );
        v = LoadFileToMem( sound );

        if ( v.empty() ) {
            // find mp3
            StringReplace( name, ".82m", ".mp3" );
            sound = Settings::GetLastFile( prefix_sounds, name );

            v = LoadFileToMem( sound );
        }

        if ( v.size() ) {
            DEBUG_LOG( DBG_ENGINE, DBG_INFO, sound );
            return;
        }
    }
#endif

    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, M82::GetString( m82 ) );
    const std::vector<u8> & body = ReadMusicChunk( M82::GetString( m82 ) );

    if ( body.size() ) {
#ifdef WITH_MIXER
        // create WAV format
        StreamBuf wavHeader( 44 );
        wavHeader.putLE32( 0x46464952 ); // RIFF
        wavHeader.putLE32( body.size() + 0x24 ); // size
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
#else
        Audio::Spec wav_spec;
        wav_spec.format = AUDIO_U8;
        wav_spec.channels = 1;
        wav_spec.freq = 22050;

        const Audio::Spec & hardware = Audio::GetHardwareSpec();

        Audio::CVT cvt;

        if ( cvt.Build( wav_spec, hardware ) ) {
            const u32 size = cvt.len_mult * body.size();

            cvt.buf = new u8[size];
            cvt.len = body.size();

            memcpy( cvt.buf, &body[0], body.size() );

            cvt.Convert();

            v.assign( cvt.buf, cvt.buf + size - 1 );

            delete[] cvt.buf;
            cvt.buf = NULL;
        }
#endif
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
    if ( Mixer::isValid() && v.empty() )
        LoadWAV( m82, v );
    return v;
}

/* return MID */
const std::vector<u8> & AGG::GetMID( int xmi )
{
    std::vector<u8> & v = mid_cache[xmi];
    if ( Mixer::isValid() && v.empty() )
        LoadMID( xmi, v );
    return v;
}

void AGG::LoadLOOPXXSounds( const std::vector<int> & vols, bool asyncronizedCall )
{
    if ( vols.empty() ) {
        return;
    }

    if ( asyncronizedCall ) {
        g_asyncSoundManager.pushLoopSound( vols );
    }
    else {
        g_asyncSoundManager.sync();
        LoadLOOPXXSoundsInternally( vols );
    }
}

void AGG::LoadLOOPXXSoundsInternally( const std::vector<int> & vols )
{
    const Settings & conf = Settings::Get();
    if ( !conf.Sound() ) {
        return;
    }

    std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

    // set volume loop sounds
    for ( std::vector<int>::const_iterator itv = vols.begin(); itv != vols.end(); ++itv ) {
        int vol = *itv;
        int m82 = M82::GetLOOP00XX( std::distance( vols.begin(), itv ) );
        if ( M82::UNKNOWN == m82 )
            continue;

        // find loops
        std::vector<loop_sound_t>::iterator itl = std::find( loop_sounds.begin(), loop_sounds.end(), m82 );

        if ( itl != loop_sounds.end() ) {
            // unused and free
            if ( 0 == vol || conf.SoundVolume() == 0 ) {
                if ( Mixer::isPlaying( ( *itl ).channel ) ) {
                    Mixer::Pause( ( *itl ).channel );
                    Mixer::Volume( ( *itl ).channel, Mixer::MaxVolume() * conf.SoundVolume() / 10 );
                    Mixer::Stop( ( *itl ).channel );
                }
                ( *itl ).sound = M82::UNKNOWN;
            }
            // used and set vols
            else if ( Mixer::isPlaying( ( *itl ).channel ) ) {
                Mixer::Pause( ( *itl ).channel );
                Mixer::Volume( ( *itl ).channel, vol * conf.SoundVolume() / 10 );
                Mixer::Resume( ( *itl ).channel );
            }
        }
        else
            // new sound
            if ( 0 != vol ) {
            const std::vector<u8> & v = GetWAV( m82 );
            const int ch = Mixer::Play( &v[0], v.size(), -1, true );

            if ( 0 <= ch ) {
                Mixer::Pause( ch );
                Mixer::Volume( ch, vol * conf.SoundVolume() / 10 );
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
        g_asyncSoundManager.pushSound( m82 );
    }
    else {
        g_asyncSoundManager.sync();
        PlaySoundInternally( m82 );
    }
}

void AGG::PlaySoundInternally( const int m82 )
{
    const Settings & conf = Settings::Get();
    if ( !conf.Sound() ) {
        return;
    }

    std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, M82::GetString( m82 ) );
    const std::vector<u8> & v = AGG::GetWAV( m82 );
    const int ch = Mixer::Play( &v[0], v.size(), -1, false );
    Mixer::Pause( ch );
    Mixer::Volume( ch, Mixer::MaxVolume() * conf.SoundVolume() / 10 );
    Mixer::Resume( ch );
}

/* wrapper Audio::Play */
void AGG::PlayMusic( int mus, bool loop, bool asyncronizedCall )
{
    if ( MUS::UNUSED == mus || MUS::UNKNOWN == mus ) {
        return;
    }

    if ( asyncronizedCall ) {
        g_asyncSoundManager.pushMusic( mus, loop );
    }
    else {
        g_asyncSoundManager.sync();
        PlayMusicInternally( mus, loop );
    }
}

void AGG::PlayMusicInternally( const int mus, const bool loop )
{
    const Settings & conf = Settings::Get();
    if ( !conf.Music() ) {
        return;
    }

    std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

    if ( Game::CurrentMusic() == mus && Music::isPlaying() ) {
        return;
    }

    Game::SetCurrentMusic( mus );
    const std::string prefix_music( "music" );
    const MusicSource type = conf.MusicType();

    bool isSongFound = false;

    if ( type == MUSIC_EXTERNAL ) {
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

        if ( filename.size() ) {
            Music::Play( filename, loop );
            isSongFound = true;
        }
        DEBUG_LOG( DBG_ENGINE, DBG_TRACE, MUS::GetString( mus, MUS::OGG_MUSIC_TYPE::MAPPED ) );
    }
#ifdef WITH_AUDIOCD
    else if ( type == MUSIC_CDROM && Cdrom::isValid() ) {
        Cdrom::Play( mus, loop );
        isSongFound = true;
        DEBUG_LOG( DBG_ENGINE, DBG_INFO, "cd track " << static_cast<int>( mus ) );
    }
#endif

    if ( !isSongFound ) {
        // Check if music needs to be pulled from HEROES2X
        int xmi = XMI::UNKNOWN;
        if ( type == MUSIC_MIDI_EXPANSION ) {
            xmi = XMI::FromMUS( mus, g_midiHeroes2xAGG.isGood() );
        }

        if ( XMI::UNKNOWN == xmi ) {
            xmi = XMI::FromMUS( mus, false );
        }

        if ( XMI::UNKNOWN != xmi ) {
#ifdef WITH_MIXER
            const std::vector<u8> & v = GetMID( xmi );
            if ( v.size() )
                Music::Play( v, loop );
#else
            std::string mid = XMI::GetString( xmi );
            StringReplace( mid, ".XMI", ".MID" );
            const std::string file = System::ConcatePath( Settings::GetWriteableDir( "music" ), mid );

            if ( !System::IsFile( file ) )
                SaveMemToFile( GetMID( xmi ), file );

            Music::Play( file, loop );
#endif
        }
        DEBUG_LOG( DBG_ENGINE, DBG_TRACE, XMI::GetString( xmi ) );
    }
}

#ifdef WITH_TTF
// void AGG::LoadTTFChar( u32 ch )
// {
//     const Settings & conf = Settings::Get();
//      const RGBA white( 0xFF, 0xFF, 0xFF );
//      const RGBA yellow( 0xFF, 0xFF, 0x00 );
//
//      small
//      fnt_cache[ch].sfs[0] = fonts[0].RenderUnicodeChar( ch, white, !conf.FontSmallRenderBlended() );
//      fnt_cache[ch].sfs[1] = fonts[0].RenderUnicodeChar( ch, yellow, !conf.FontSmallRenderBlended() );
//
//      medium
//      fnt_cache[ch].sfs[2] = fonts[1].RenderUnicodeChar( ch, white, !conf.FontNormalRenderBlended() );
//      fnt_cache[ch].sfs[3] = fonts[1].RenderUnicodeChar( ch, yellow, !conf.FontNormalRenderBlended() );
//
//     DEBUG_LOG( DBG_ENGINE, DBG_TRACE, "0x" << std::hex << ch );
// }

void AGG::LoadFNT( void )
{
    // const Settings & conf = Settings::Get();
    //
    // if ( !conf.Unicode() ) {
    //     DEBUG_LOG( DBG_ENGINE, DBG_INFO, "use bitmap fonts" );
    // }
    // else if ( fnt_cache.empty() ) {
    //     const std::string letters = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    //     std::vector<u16> unicode = StringUTF8_to_UNICODE( letters );
    //
    //     for ( std::vector<u16>::const_iterator it = unicode.begin(); it != unicode.end(); ++it )
    //         LoadTTFChar( *it );
    //
    //     if ( fnt_cache.empty() ) {
    //         DEBUG_LOG( DBG_ENGINE, DBG_INFO, "use bitmap fonts" );
    //     }
    //     else {
    //         DEBUG_LOG( DBG_ENGINE, DBG_INFO, "normal fonts " << conf.FontsNormal() );
    //         DEBUG_LOG( DBG_ENGINE, DBG_INFO, "small fonts " << conf.FontsSmall() );
    //         DEBUG_LOG( DBG_ENGINE, DBG_INFO, "preload english charsets" );
    //     }
    // }
}

u32 AGG::GetFontHeight( bool small )
{
    return small ? fonts[0].Height() : fonts[1].Height();
}

#else
void AGG::LoadFNT( void )
{
    DEBUG_LOG( DBG_ENGINE, DBG_INFO, "use bitmap fonts" );
}
#endif

// This exists to avoid exposing AGG::ReadChunk
std::vector<u8> AGG::LoadBINFRM( const char * frm_file )
{
    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, frm_file );
    return AGG::ReadChunk( frm_file );
}

void AGG::ResetMixer()
{
    g_asyncSoundManager.sync();

    std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

    Mixer::Reset();
    loop_sounds.clear();
    loop_sounds.reserve( 7 );
}

bool AGG::Init( void )
{
    // read data dir
    if ( !ReadDataDir() ) {
        DEBUG_LOG( DBG_ENGINE, DBG_WARN, "data files not found" );

#ifdef WITH_ZLIB
        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Image & image = CreateImageFromZlib( 290, 190, errorMessage, sizeof( errorMessage ) );

        display.fill( 0 );
        fheroes2::Copy( image, 0, 0, display, ( display.width() - image.width() ) / 2, ( display.height() - image.height() ) / 2, image.width(), image.height() );

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() && !le.KeyPress() && !le.MouseClickLeft() )
            ;
#endif

        return false;
    }

#ifdef WITH_TTF
    Settings & conf = Settings::Get();
    const std::string prefix_fonts = System::ConcatePath( "files", "fonts" );
    const std::string font1 = Settings::GetLastFile( prefix_fonts, conf.FontsNormal() );
    const std::string font2 = Settings::GetLastFile( prefix_fonts, conf.FontsSmall() );

    fonts = new FontTTF[2];

    if ( conf.Unicode() ) {
        DEBUG_LOG( DBG_ENGINE, DBG_INFO, "fonts: " << font1 << ", " << font2 );
        if ( !fonts[1].Open( font1, conf.FontsNormalSize() ) || !fonts[0].Open( font2, conf.FontsSmallSize() ) )
            conf.SetUnicode( false );
    }
#endif

    // load font
    LoadFNT();

    return true;
}

void AGG::Quit( void )
{
    wav_cache.clear();
    mid_cache.clear();
    loop_sounds.clear();
    // fnt_cache.clear();

#ifdef WITH_TTF
    delete[] fonts;
#endif
}
