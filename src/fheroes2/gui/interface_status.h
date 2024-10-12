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

#ifndef H2INTERFACE_STATUS_H
#define H2INTERFACE_STATUS_H

#include <cstdint>

#include "interface_border.h"
#include "resource.h"
#include "timing.h"

namespace Interface
{
    class BaseInterface;

    enum class StatusType : int
    {
        STATUS_UNKNOWN,
        STATUS_DAY,
        STATUS_FUNDS,
        STATUS_ARMY,
        STATUS_RESOURCE,
        STATUS_AITURN
    };

    class StatusPanel final : public BorderWindow
    {
    public:
        explicit StatusPanel( BaseInterface & interface )
            : BorderWindow( { 0, 0, 144, 72 } )
            , _interface( interface )
        {
            // Do nothing.
        }

        StatusPanel( const StatusPanel & ) = delete;

        ~StatusPanel() override = default;

        StatusPanel & operator=( const StatusPanel & ) = delete;

        void SetPos( int32_t x, int32_t y ) override;
        void SavePosition() override;
        void setRedraw() const;

        void Reset()
        {
            _state = StatusType::STATUS_DAY;
            _lastResource = Resource::UNKNOWN;
            _lastResourceCount = 0;
        }

        void NextState();

        void SetState( const StatusType status );
        void SetResource( const int resource, const uint32_t count );
        void drawAITurnProgress( const uint32_t progressValue );

        void resetAITurnProgress()
        {
            // We set _aiTurnProgress equal to 10 to properly handle the first draw call for the '0' progress.
            _aiTurnProgress = 10;
        }

        void QueueEventProcessing();
        void TimerEventProcessing();

        // Do not call this method directly, use Interface::AdventureMap::redraw() instead to avoid issues in the "no interface" mode.
        // The name of this method starts from _ on purpose to do not mix with other public methods.
        void _redraw() const;

    private:
        void _drawKingdomInfo( const int32_t offsetY = 0 ) const;
        void _drawDayInfo( const int32_t offsetY = 0 ) const;
        void _drawArmyInfo( const int32_t offsetY = 0 ) const;
        void _drawResourceInfo( const int32_t offsetY = 0 ) const;
        void _drawBackground() const;
        void _drawAITurns() const;

        fheroes2::TimeDelay _showLastResourceDelay{ 2500 };

        BaseInterface & _interface;

        StatusType _state{ StatusType::STATUS_UNKNOWN };
        int _lastResource{ Resource::UNKNOWN };
        uint32_t _lastResourceCount{ 0 };
        uint32_t _aiTurnProgress{ 10 };
        uint32_t _grainsAnimationIndexOffset{ 0 };
    };
}

#endif
