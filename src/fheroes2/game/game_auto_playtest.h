/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

#include "color.h"

namespace fheroes2
{
    class AutoPlaytest final
    {
    public:
        enum class PlayerState : uint8_t
        {
            WINNER,
            LOSER,
            TIME_LIMIT,
            INTERRUPTED,
        };

        struct PlayerInfo final
        {
            PlayerColor color{ PlayerColor::NONE };
            PlayerState state{ PlayerState::WINNER };
            uint32_t dayOfState{ 1 };
        };

        static constexpr int32_t playthroughLimit{ 100 };
        static constexpr int32_t dayLimit{ 1000 };
        static constexpr int32_t animationLimit{ 10 };

        static AutoPlaytest & instance();

        void setMaxDaysInPlaythrough( const int32_t days )
        {
            _maxDaysInPlaythrough = std::clamp<int32_t>( days, 1, dayLimit );
        }

        int32_t getMaxDaysInPlaythrough() const
        {
            return _maxDaysInPlaythrough;
        }

        void setAnimationSpeed( const int32_t speed )
        {
            _animationSpeed = std::clamp<int32_t>( speed, 1, animationLimit );
        }

        int32_t getAnimationSpeed() const
        {
            return _animationSpeed;
        }

        void setMaxPlaythroughs( const int32_t playthroughCount )
        {
            _maxPlaythroughs = std::clamp<int32_t>( playthroughCount, 1, playthroughLimit );
        }

        int32_t getMaxPlaythroughs() const
        {
            return _maxPlaythroughs;
        }

        void enableAnimation( const bool enable )
        {
            _isAnimationEnabled = enable;
        }

        bool isAnimationEnabled() const
        {
            return _isAnimationEnabled;
        }

        void enableSounds( const bool enable )
        {
            _playEnvironmentSounds = enable;
        }

        bool areEnvironmentSoundsEnabled() const
        {
            return _playEnvironmentSounds;
        }

        void reset( const PlayerColorsSet colors )
        {
            _playthroughResults.clear();

            auto & infos = _playthroughResults.emplace_back();
            for ( const auto color : PlayerColorsVector( colors ) ) {
                auto & info = infos.emplace_back();
                info.color = color;
            }
        }

        void nextPlaythrough()
        {
            assert( !_playthroughResults.empty() );

            std::vector<PlayerInfo> lastResult = _playthroughResults.back();
            for ( auto & state : lastResult ) {
                state.dayOfState = 0;
                state.state = PlayerState::WINNER;
            }

            _playthroughResults.emplace_back( std::move( lastResult ) );
        }

        void setDefeatedPlayer( const PlayerColor color, const uint32_t day )
        {
            assert( !_playthroughResults.empty() );

            for ( auto & info : _playthroughResults.back() ) {
                if ( info.color == color ) {
                    info.dayOfState = day;
                    info.state = PlayerState::LOSER;
                }
            }
        }

        void markTimeLimit()
        {
            assert( !_playthroughResults.empty() );

            for ( auto & info : _playthroughResults.back() ) {
                if ( info.state != PlayerState::LOSER ) {
                    info.dayOfState = _maxDaysInPlaythrough;
                    info.state = PlayerState::TIME_LIMIT;
                }
            }
        }

        void interrupt( const uint32_t day )
        {
            assert( !_playthroughResults.empty() );

            for ( auto & info : _playthroughResults.back() ) {
                if ( info.state != PlayerState::LOSER ) {
                    info.dayOfState = day;
                    info.state = PlayerState::INTERRUPTED;
                }
            }
        }

        bool isInterrupted() const
        {
            assert( !_playthroughResults.empty() );

            for ( const auto & info : _playthroughResults.back() ) {
                if ( info.state == PlayerState::INTERRUPTED ) {
                    return true;
                }
            }

            return false;
        }

        const std::vector<std::vector<PlayerInfo>> & getResults() const
        {
            return _playthroughResults;
        }

        void popLastResults()
        {
            _playthroughResults.pop_back();
        }

    private:
        AutoPlaytest() = default;
        ~AutoPlaytest() = default;

        std::vector<std::vector<PlayerInfo>> _playthroughResults;

        int32_t _maxPlaythroughs{ 1 };
        int32_t _maxDaysInPlaythrough{ 365 };
        int32_t _animationSpeed{ animationLimit };
        bool _isAnimationEnabled{ true };
        bool _playEnvironmentSounds{ true };
    };

    bool openMapAutoPlayTest();

    void interruptAutoPlaytest();
}
