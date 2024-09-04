/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#ifndef H2ROUTE_H
#define H2ROUTE_H

#include <cstdint>
#include <list>
#include <string>

#include "direction.h"

class IStreamBase;
class OStreamBase;

class Heroes;

namespace Route
{
    class Step
    {
    public:
        Step() = default;
        Step( int32_t index, int32_t fromIndex, int dir, uint32_t cost )
            : currentIndex( index )
            , from( fromIndex )
            , direction( dir )
            , penalty( cost )
        {}

        int32_t GetIndex() const
        {
            return currentIndex;
        }

        uint32_t GetPenalty() const
        {
            return penalty;
        }

        int32_t GetFrom() const
        {
            return from;
        }

        int GetDirection() const
        {
            return direction;
        }

    protected:
        friend OStreamBase & operator<<( OStreamBase & stream, const Step & step );
        friend IStreamBase & operator>>( IStreamBase & stream, Step & step );

        int32_t currentIndex = -1;
        int32_t from = -1;
        int direction = Direction::CENTER;
        uint32_t penalty = 0;
    };

    class Path : public std::list<Step>
    {
    public:
        explicit Path( const Heroes & hero );
        Path( const Path & ) = delete;

        Path & operator=( const Path & ) = delete;

        // Returns the index of the last step of the path. If the path is empty, then returns -1.
        int32_t GetDestinationIndex() const
        {
            return empty() ? -1 : back().GetIndex();
        }

        // Returns the target index of the current step (the first in the queue). If the queue is empty, then returns -1.
        int32_t GetFrontIndex() const
        {
            return empty() ? -1 : front().GetIndex();
        }

        // Returns the source index of the current step (the first in the queue). If the queue is empty, then returns -1.
        int32_t GetFrontFrom() const
        {
            return empty() ? -1 : front().GetFrom();
        }

        // Returns the direction of the current step (the first in the queue). If the queue is empty, then returns
        // 'Direction::UNKNOWN'.
        int GetFrontDirection() const
        {
            return empty() ? Direction::UNKNOWN : front().GetDirection();
        }

        // Returns the penalty of the current step (the first in the queue). If the queue is empty, then returns 0.
        uint32_t GetFrontPenalty() const
        {
            return empty() ? 0 : front().GetPenalty();
        }

        // Returns the direction of the next step (the second in the queue). If the size of the queue is less than 2 elements,
        // then returns 'Direction::UNKNOWN'.
        int GetNextStepDirection() const
        {
            if ( size() < 2 ) {
                return Direction::UNKNOWN;
            }

            auto iter = cbegin();

            return ( ++iter )->GetDirection();
        }

        void setPath( const std::list<Step> & path )
        {
            assign( path.begin(), path.end() );
        }

        void Show()
        {
            _hide = false;
        }

        void Hide()
        {
            _hide = true;
        }

        void Reset()
        {
            clear();
        }

        // Truncates the path after the current step (the first in the queue). If the queue is empty, then does nothing.
        void Truncate()
        {
            if ( empty() ) {
                return;
            }

            auto iter = cbegin();

            erase( ++iter, cend() );
        }

        void PopFront()
        {
            if ( empty() ) {
                return;
            }

            pop_front();
        }

        // Returns true if this path is valid for normal movement on the map (the current step is performed to the tile
        // adjacent to the hero), otherwise returns false
        bool isValidForMovement() const;

        // Returns true if this path is valid to move through teleportation (the current step is performed NOT to the tile
        // adjacent to the hero, but to some distant valid tile), otherwise returns false
        bool isValidForTeleportation() const;

        bool isShow() const
        {
            return !_hide;
        }

        bool hasAllowedSteps() const;
        int GetAllowedSteps() const;

        std::string String() const;

        static int GetIndexSprite( const int from, const int to, const int cost );

    private:
        friend OStreamBase & operator<<( OStreamBase & stream, const Path & path );
        friend IStreamBase & operator>>( IStreamBase & stream, Path & path );

        const Heroes * _hero;
        bool _hide;
    };

    OStreamBase & operator<<( OStreamBase & stream, const Step & step );
    IStreamBase & operator>>( IStreamBase & stream, Step & step );

    OStreamBase & operator<<( OStreamBase & stream, const Path & path );
    IStreamBase & operator>>( IStreamBase & stream, Path & path );

    uint32_t calculatePathPenalty( const std::list<Step> & path );
}

#endif
