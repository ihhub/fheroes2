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

#ifndef H2INTERFACE_STATUS_H
#define H2INTERFACE_STATUS_H

#include <cstdint>

#include "interface_border.h"
#include "timing.h"

namespace Interface
{
    class Basic;

    enum class StatusType : int
    {
        STATUS_UNKNOWN,
        STATUS_DAY,
        STATUS_FUNDS,
        STATUS_ARMY,
        STATUS_RESOURCE,
        STATUS_AITURN
    };

    class StatusWindow final : public BorderWindow
    {
    public:
        explicit StatusWindow( Basic & basic );
        StatusWindow( const StatusWindow & ) = delete;

        ~StatusWindow() override = default;

        StatusWindow & operator=( const StatusWindow & ) = delete;

        void SetPos( int32_t ox, int32_t oy ) override;
        void SavePosition() override;
        void SetRedraw() const;

        void Reset();

        void NextState();

        void SetState( const StatusType status );
        void SetResource( int, uint32_t );
        void SetToRedrawTurnProgress( const uint32_t progressValue );
        void QueueEventProcessing();
        void TimerEventProcessing();

    private:
        friend Basic;

        // Do not call this method directly, use Interface::Basic::Redraw() instead
        // to avoid issues in the "no interface" mode
        void Redraw() const;
        void DrawKingdomInfo( int oh = 0 ) const;
        void DrawDayInfo( int oh = 0 ) const;
        void DrawArmyInfo( int oh = 0 ) const;
        void DrawResourceInfo( int oh = 0 ) const;
        void DrawBackground() const;
        void DrawAITurns() const;

        Basic & interface;

        StatusType _state;
        int lastResource;
        uint32_t countLastResource;
        uint32_t turn_progress;

        fheroes2::TimeDelay showLastResourceDelay;
    };
}

#endif
