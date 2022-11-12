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

#ifndef H2INTERFACE_CPANEL_H
#define H2INTERFACE_CPANEL_H

#include <cstdint>
#include <memory>

#include "game_mode.h"
#include "math_base.h"

namespace fheroes2
{
    class Sprite;
}

namespace Interface
{
    class Basic;

    class ControlPanel final : protected fheroes2::Rect
    {
    public:
        explicit ControlPanel( Basic & );
        ControlPanel( const ControlPanel & ) = delete;

        ~ControlPanel() = default;

        ControlPanel & operator=( const ControlPanel & ) = delete;

        void SetPos( int32_t, int32_t );
        void ResetTheme();
        fheroes2::GameMode QueueEventProcessing() const;

        const fheroes2::Rect & GetArea() const;

    private:
        friend Basic;

        // Do not call this method directly, use Interface::Basic::Redraw() instead
        // to avoid issues in the "no interface" mode
        void Redraw() const;

        Basic & interface;

        // We do not want to make a copy of images but to store just references to them.
        struct Buttons
        {
            Buttons( const fheroes2::Sprite & radar_, const fheroes2::Sprite & icons_, const fheroes2::Sprite & buttons_, const fheroes2::Sprite & status_,
                     const fheroes2::Sprite & end_ )
                : radar( radar_ )
                , icons( icons_ )
                , buttons( buttons_ )
                , status( status_ )
                , end( end_ )
            {}

            const fheroes2::Sprite & radar;
            const fheroes2::Sprite & icons;
            const fheroes2::Sprite & buttons;
            const fheroes2::Sprite & status;
            const fheroes2::Sprite & end;
        };

        std::unique_ptr<Buttons> _buttons;

        fheroes2::Rect rt_radar;
        fheroes2::Rect rt_icons;
        fheroes2::Rect rt_buttons;
        fheroes2::Rect rt_status;
        fheroes2::Rect rt_end;
    };
}

#endif
