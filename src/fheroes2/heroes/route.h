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
#ifndef H2HEROPATH_H
#define H2HEROPATH_H

#include <cstdint>
#include <list>
#include <string>

#include "direction.h"

class Heroes;
class StreamBase;

namespace Route
{
    class Step
    {
    public:
        Step() = default;
        Step( int index, int32_t fromIndex, int dir, uint32_t cost )
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
        friend StreamBase & operator<<( StreamBase &, const Step & );
        friend StreamBase & operator>>( StreamBase &, Step & );

        int currentIndex = -1;
        int32_t from = -1;
        int direction = Direction::CENTER;
        uint32_t penalty = 0;
    };

    class Path : public std::list<Step>
    {
    public:
        explicit Path( const Heroes & );
        Path( const Path & ) = delete;

        Path & operator=( const Path & ) = delete;

        // Returns the destination index of the path, or, if the returnLastStep is true, returns the index of the last
        // step of the path. Usually it's the same thing if PlayerWorldPathfinder was used to calculate the path, but
        // this may not be the case if AIWorldPathfinder was used - due to the peculiarities of laying the path through
        // heroes, neutral armies, teleports or water.
        int32_t GetDestinationIndex( const bool returnLastStep = false ) const;
        int GetFrontDirection() const;
        uint32_t GetFrontPenalty() const;
        void setPath( const std::list<Step> & path, int32_t destIndex );

        void Show()
        {
            hide = false;
        }

        void Hide()
        {
            hide = true;
        }

        void Reset();
        void PopFront();

        bool isValid() const;

        bool isShow() const
        {
            return !hide;
        }

        bool hasAllowedSteps() const;

        std::string String() const;

        int GetAllowedSteps() const;
        static int GetIndexSprite( int from, int to, int mod );

    private:
        friend StreamBase & operator<<( StreamBase &, const Path & );
        friend StreamBase & operator>>( StreamBase &, Path & );

        const Heroes * hero;
        int32_t dst;
        bool hide;
    };

    StreamBase & operator<<( StreamBase &, const Step & );
    StreamBase & operator<<( StreamBase &, const Path & );
    StreamBase & operator>>( StreamBase &, Step & );
    StreamBase & operator>>( StreamBase &, Path & );
}

#endif
