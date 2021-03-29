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

#ifndef H2AGG_H
#define H2AGG_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace AGG
{
    bool Init( void );
    void Quit( void );

    std::vector<uint8_t> LoadBINFRM( const char * frm_file );
#ifdef WITH_TTF
    uint32_t GetFontHeight( bool small );
#endif
    void LoadLOOPXXSounds( const std::vector<int> & vols, bool asyncronizedCall = false );
    void PlaySound( int m82, bool asyncronizedCall = false );
    void PlayMusic( int mus, bool loop = true, bool asyncronizedCall = false );
    void ResetMixer();

    std::vector<uint8_t> ReadChunk( const std::string & key, bool ignoreExpansion = false );
}

#endif
