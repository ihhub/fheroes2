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
#include "logging.h"
#include "tools.h"

#include <SDL.h>

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
        ERROR_LOG( Mix_GetError() );
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
            ERROR_LOG( Mix_GetError() );
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
