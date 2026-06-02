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

#include <cassert>
#include <vector>

#include "color.h"

namespace fheroes2
{
    bool openMapAutoPlayTest();

    class AutoGameplay final
    {
    public:
        static AutoGameplay & instance();

        void setMaxDaysInGameplay( const uint32_t days )
        {
            _maxDaysInGameplay = days;
        }

        uint32_t getMaxDaysInGameplay() const
        {
            return _maxDaysInGameplay;
        }

        void setMovementSpeed( const int32_t speed )
        {
            _movementSpeed = std::clamp( speed, 0, 10 );
        }

        int32_t getMovementSpeed() const
        {
            return _movementSpeed;
        }

        struct PlayerInfo final
        {
            enum class State : uint8_t
            {
                WINNER,
                LOSER,
                TIME_LIMIT,
                INTERRUPTED,
            };

            PlayerColor color{ PlayerColor::NONE };
            State state{ State::WINNER };
            uint32_t dayOfState{ 0 };
        };

        void reset( const PlayerColorsSet colors )
        {
            _roundId = 0;
            _roundResults.clear();

            auto & infos =_roundResults.emplace_back();
            for ( const auto color : PlayerColorsVector( colors ) ) {
                auto & info = infos.emplace_back();
                info.color = color;
            }
        }

        void nextRound()
        {
            assert( !_roundResults.empty() );

            ++_roundId;

            std::vector<PlayerInfo> lastResult = _roundResults.back();
            for ( auto & state : lastResult ) {
                state.dayOfState = 0;
                state.state = PlayerInfo::State::WINNER;
            }

            _roundResults.emplace_back( std::move( lastResult ) );
        }

        void setDefeatedPlayer( const PlayerColor color, const uint32_t day )
        {
            assert( !_roundResults.empty() );

            for ( auto & info : _roundResults.back() ) {
                if ( info.color == color ) {
                    info.dayOfState = day;
                    info.state = PlayerInfo::State::LOSER;
                }
            }
        }

        void markTimeLimit()
        {
            assert( !_roundResults.empty() );

            for ( auto & info : _roundResults.back() ) {
                if ( info.state != PlayerInfo::State::LOSER ) {
                    info.dayOfState = _maxDaysInGameplay;
                    info.state = PlayerInfo::State::TIME_LIMIT;
                }
            }
        }

        void interrupt( const uint32_t day )
        {
            assert( !_roundResults.empty() );

            for ( auto & info : _roundResults.back() ) {
                if ( info.state != PlayerInfo::State::LOSER ) {
                    info.dayOfState = day;
                    info.state = PlayerInfo::State::INTERRUPTED;
                }
            }
        }

        const std::vector<std::vector<PlayerInfo>> & getResults() const
        {
            return _roundResults;
        }

    private:
        AutoGameplay() = default;
        ~AutoGameplay() = default;

        int32_t _roundId{ 0 };

        uint32_t _maxDaysInGameplay{ 365 };

        int32_t _movementSpeed{ 10 };

        std::vector<std::vector<PlayerInfo>> _roundResults;
    };
}
