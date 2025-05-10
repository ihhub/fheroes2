/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2025                                             *
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

#pragma once

#include <chrono>
#include <cstdint>

namespace fheroes2
{
    // IMPORTANT!!! According to https://en.cppreference.com/w/cpp/chrono/high_resolution_clock we should never use high_resolution_clock for time internal measurements
    // because for high_resolution_clock the time may go backwards.

    class Time
    {
    public:
        Time()
        {
            reset();
        }

        void reset()
        {
            _startTime = std::chrono::steady_clock::now();
        }

        // Returns time in seconds.
        double getS() const
        {
            const std::chrono::duration<double> time = std::chrono::steady_clock::now() - _startTime;
            return time.count();
        }

        // Returns rounded time in milliseconds.
        uint64_t getMs() const
        {
            const auto time = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - _startTime );
            return time.count();
        }

    private:
        std::chrono::time_point<std::chrono::steady_clock> _startTime;
    };

    class TimeDelay
    {
    public:
        TimeDelay() = delete;

        explicit TimeDelay( const uint64_t delayMs )
            : _delayMs( delayMs )
        {
            reset();
        }

        void setDelay( const uint64_t delayMs )
        {
            _delayMs = delayMs;
        }

        uint64_t getDelay() const
        {
            return _delayMs;
        }

        bool isPassed() const
        {
            return isPassed( _delayMs );
        }

        bool isPassed( const uint64_t delayMs ) const
        {
            const auto time = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - _prevTime );
            const uint64_t passedMs = time.count();
            return passedMs >= delayMs;
        }

        // Reset delay by starting the count from the current time.
        void reset()
        {
            _prevTime = std::chrono::steady_clock::now();
        }

        // Explicitly set delay to passed state. Can be used in cases when first call of isPassed() must return true.
        void pass()
        {
            _prevTime = std::chrono::steady_clock::now() - std::chrono::milliseconds( 2 * _delayMs );
        }

    private:
        std::chrono::time_point<std::chrono::steady_clock> _prevTime;
        uint64_t _delayMs;
    };

    void delayforMs( const uint32_t delayMs );
}
