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

#include <cstdlib>
#include <iostream>

#include "audio_mixer.h"
#include "audio_music.h"
#include "system.h"
#include "tools.h"

#ifdef WITH_MIXER
#include <SDL_mixer.h>

namespace Music
{
    void Play( Mix_Music * mix, /*u32 id,*/ bool loop );

    Mix_Music * music = NULL;
    int fadein = 0;
    int fadeout = 0;
}

void Music::Play( Mix_Music * mix, /*u32 id,*/ bool loop )
{
    Reset();

    int res = fadein ? Mix_FadeInMusic( mix, loop ? -1 : 0, fadein ) : Mix_PlayMusic( mix, loop ? -1 : 0 );

    if ( res < 0 ) {
        ERROR( Mix_GetError() );
    }
    else
        music = mix;
}

void Music::Play( const std::vector<u8> & v, bool loop )
{
    if ( Mixer::isValid() && v.size() ) {
        // u32 id = CheckSum( v );
        SDL_RWops * rwops = SDL_RWFromConstMem( &v[0], v.size() );
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        Mix_Music * mix = Mix_LoadMUS_RW( rwops, 0 );
#else
        Mix_Music * mix = Mix_LoadMUS_RW( rwops );
#endif
        SDL_FreeRW( rwops );
        Music::Play( mix, /*id,*/ loop );
    }
}

void Music::Play( const std::string & file, bool loop )
{
    if ( Mixer::isValid() ) {
        // u32 id = CheckSum( file );
        Mix_Music * mix = Mix_LoadMUS( file.c_str() );

        if ( !mix ) {
            ERROR( Mix_GetError() );
        }
        else
            Music::Play( mix, /*id,*/ loop );
    }
}

void Music::SetFadeIn( int f )
{
    fadein = f;
}

void Music::SetFadeOut( int f )
{
    fadeout = f;
}

u16 Music::Volume( int16_t vol )
{
    return Mixer::isValid() ? ( Mix_VolumeMusic( vol > MIX_MAX_VOLUME ? MIX_MAX_VOLUME : vol ) ) : 0;
}

void Music::Pause( void )
{
    if ( music )
        Mix_PauseMusic();
}

void Music::Resume( void )
{
    if ( music )
        Mix_ResumeMusic();
}

void Music::Reset( void )
{
    if ( music ) {
        if ( fadeout )
            while ( !Mix_FadeOutMusic( fadeout ) && Mix_PlayingMusic() )
                SDL_Delay( 50 );
        else
            Mix_HaltMusic();

        Mix_FreeMusic( music );
        music = NULL;
    }
}

bool Music::isPlaying( void )
{
    return music && Mix_PlayingMusic();
}

bool Music::isPaused( void )
{
    return music && Mix_PausedMusic();
}

void Music::SetExtCommand( const std::string & ) {}

#else
#include "thread.h"
#include <algorithm>
#include <list>

namespace Music
{
    enum
    {
        UNUSED = 0,
        PLAY = 0x01,
        PAUSE = 0x02,
        LOOP = 0x04
    };
    std::string command;
}

struct info_t
{
    info_t()
        : status( 0 )
    {}

    std::string file;
    int status;
};

int callbackPlayMusic( void * ptr )
{
    if ( ptr && std::system( NULL ) ) {
        info_t * info = reinterpret_cast<info_t *>( ptr );
        std::ostringstream os;
        os << Music::command << " " << info->file;

        info->status |= Music::PLAY;

        do {
            std::system( os.str().c_str() );
            DELAY( 100 );
        } while ( info->status & Music::LOOP );

        info->status &= ~Music::PLAY;

        return 0;
    }

    return -1;
}

struct play_t : std::pair<SDL::Thread, info_t>
{
    play_t() {}

    bool operator==( const std::string & f ) const
    {
        return f == second.file;
    }

    void Run( const info_t & info )
    {
        second = info;
        first.Create( callbackPlayMusic, &second );
    }
    void Run( void )
    {
        first.Create( callbackPlayMusic, &second );
    }
    void Stop( void )
    {
        if ( System::GetEnvironment( "MUSIC_WRAPPER" ) )
            RunMusicWrapper( "stop" );
        first.Kill();
        second.status = Music::UNUSED;
    }

    void RunMusicWrapper( const char * action )
    {
        std::ostringstream os;
        os << System::GetEnvironment( "MUSIC_WRAPPER" ) << " " << action << " " << second.file;
        std::system( os.str().c_str() );
    }

    void Pause( void )
    {
        RunMusicWrapper( "pause" );
        second.status |= Music::PAUSE;
    }
    void Continue( void )
    {
        RunMusicWrapper( "continue" );
        second.status &= ~Music::PAUSE;
    }

    bool isPlay( void ) const
    {
        return second.status & Music::PLAY;
    }
    bool isPaused( void ) const
    {
        return second.status & Music::PAUSE;
    }

    static bool isPlaying( const play_t & p )
    {
        return p.isPlay() && !p.isPaused();
    }
    static bool isRunning( const play_t & p )
    {
        return p.first.IsRun();
    }
    static bool isFree( const play_t & p )
    {
        return p.second.status == Music::UNUSED;
    }
};

namespace Music
{
    std::list<play_t> musics;
    std::list<play_t>::iterator current = musics.end();
}

void Music::SetExtCommand( const std::string & cmd )
{
    command = cmd;
}

void Music::Play( const std::vector<u8> & v, bool loop ) {}

void Music::Play( const std::string & f, bool loop )
{
    std::list<play_t>::iterator it = std::find( musics.begin(), musics.end(), f );

    // skip repeat
    if ( it != musics.end() ) {
        if ( ( *it ).isPaused() ) {
            Pause();
            current = it;
            Resume();
            DELAY( 100 );
        }

        if ( ( *it ).isPlay() )
            return;
    }

    // stop run
    Pause();

    info_t info;
    info.file = f;
    info.status = loop ? Music::LOOP : 0;

    it = std::find_if( musics.begin(), musics.end(), play_t::isFree );

    if ( it == musics.end() ) {
        musics.push_back( play_t() );
        it = --musics.end();
    }

    ( *it ).Run( info );
    current = it;
}

void Music::SetFadeIn( int f ) {}

void Music::SetFadeOut( int f ) {}

u16 Music::Volume( int16_t vol )
{
    return 0;
}

void Music::Pause( void )
{
    if ( !System::GetEnvironment( "MUSIC_WRAPPER" ) )
        Reset();
    else if ( current != musics.end() && ( *current ).isPlay() && !( *current ).isPaused() )
        ( *current ).Pause();
}

void Music::Resume( void )
{
    if ( current != musics.end() ) {
        if ( !System::GetEnvironment( "MUSIC_WRAPPER" ) )
            ( *current ).Run();
        else if ( ( *current ).isPlay() && ( *current ).isPaused() )
            ( *current ).Continue();
    }
}

bool Music::isPlaying( void )
{
    std::list<play_t>::iterator it = std::find_if( musics.begin(), musics.end(), play_t::isPlaying );
    return it != musics.end();
}

bool Music::isPaused( void )
{
    return false;
}

void Music::Reset( void )
{
    std::list<play_t>::iterator it = std::find_if( musics.begin(), musics.end(), play_t::isRunning );

    if ( it != musics.end() )
        ( *it ).Stop();
}

#endif
