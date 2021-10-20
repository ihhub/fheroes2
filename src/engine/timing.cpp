/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include "timing.h"

#include <thread>

#include <SDL.h>

namespace fheroes2
{
    struct TimerImp
    {
        SDL_TimerID id = 0;

        bool valid() const
        {
            return id != 0;
        }

        void run( uint32_t interval, uint32_t ( *fn )( uint32_t, void * ), void * param )
        {
            remove();

            id = SDL_AddTimer( interval, fn, param );
        }

        void remove()
        {
            if ( valid() ) {
                SDL_RemoveTimer( id );
                id = 0;
            }
        }
    };

    Time::Time()
        : _startTime( std::chrono::steady_clock::now() )
    {}

    void Time::reset()
    {
        _startTime = std::chrono::steady_clock::now();
    }

    double Time::get() const
    {
        const std::chrono::duration<double> time = std::chrono::steady_clock::now() - _startTime;
        return time.count();
    }

    uint64_t Time::getMs() const
    {
        return static_cast<uint64_t>( get() * 1000 + 0.5 );
    }

    TimeDelay::TimeDelay( const uint64_t delayMs )
        : _prevTime( std::chrono::steady_clock::now() )
        , _delayMs( delayMs )
    {}

    void TimeDelay::setDelay( const uint64_t delayMs )
    {
        _delayMs = delayMs;
    }

    bool TimeDelay::isPassed() const
    {
        return isPassed( _delayMs );
    }

    bool TimeDelay::isPassed( const uint64_t delayMs ) const
    {
        const std::chrono::duration<double> time = std::chrono::steady_clock::now() - _prevTime;
        const uint64_t passedMs = static_cast<uint64_t>( time.count() * 1000 + 0.5 );
        return passedMs >= delayMs;
    }

    void TimeDelay::reset()
    {
        _prevTime = std::chrono::steady_clock::now();
    }

    void TimeDelay::pass()
    {
        _prevTime = std::chrono::steady_clock::now() - std::chrono::milliseconds( 2 * _delayMs );
    }

    Timer::Timer()
        : _timer( new TimerImp )
    {
        // Do nothing.
    }

    Timer::~Timer()
    {
        delete _timer;
    }

    bool Timer::valid() const
    {
        return _timer->valid();
    }

    void Timer::run( uint32_t interval, uint32_t ( *fn )( uint32_t, void * ), void * param )
    {
        _timer->run( interval, fn, param );
    }

    void Timer::remove()
    {
        _timer->remove();
    }

    void delayforMs( const uint32_t delayMs )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( delayMs ) );
    }
}
