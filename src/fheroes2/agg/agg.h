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

#ifndef H2AGG_H
#define H2AGG_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace M82
{
    enum SoundType : int;
}

namespace AGG
{
    class AGGInitializer
    {
    public:
        AGGInitializer();
        AGGInitializer( const AGGInitializer & ) = delete;
        AGGInitializer & operator=( const AGGInitializer & ) = delete;

        ~AGGInitializer();

    private:
        static bool init();
    };

    struct AudioLoopEffectInfo
    {
        AudioLoopEffectInfo() = default;

        AudioLoopEffectInfo( const int16_t angle_, const uint8_t volumePercentage_ )
            : angle( angle_ )
            , volumePercentage( volumePercentage_ )
        {
            // Do nothing.
        }

        int16_t angle{ 0 };
        uint8_t volumePercentage{ 0 };
    };

    void playLoopSounds( std::map<M82::SoundType, std::vector<AudioLoopEffectInfo>> soundEffects, bool asyncronizedCall );
    void PlaySound( int m82, bool asyncronizedCall = false );
    void PlayMusic( int mus, bool loop = true, bool asyncronizedCall = false );
    void ResetAudio();

    std::vector<uint8_t> ReadChunk( const std::string & key );
}

#endif
