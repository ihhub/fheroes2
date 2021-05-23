/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include "game_mode.h"
#include "image.h"

#include <memory>

namespace Interface
{
    class Basic;

    class ControlPanel : protected fheroes2::Rect
    {
    public:
        explicit ControlPanel( Basic & );

        void SetPos( int32_t, int32_t );
        void Redraw( void ) const;
        void ResetTheme( void );
        fheroes2::GameMode QueueEventProcessing();

        const fheroes2::Rect & GetArea( void ) const;

    private:
        Basic & interface;

        // We do not want to make a copy of images but to store just references to them.
        struct Buttons
        {
            Buttons( const fheroes2::Sprite & radar_, const fheroes2::Sprite & icon_, const fheroes2::Sprite & button_, const fheroes2::Sprite & stats_,
                     const fheroes2::Sprite & quit_ )
                : radar( radar_ )
                , icon( icon_ )
                , button( button_ )
                , stats( stats_ )
                , quit( quit_ )
            {}

            const fheroes2::Sprite & radar;
            const fheroes2::Sprite & icon;
            const fheroes2::Sprite & button;
            const fheroes2::Sprite & stats;
            const fheroes2::Sprite & quit;
        };

        std::unique_ptr<Buttons> _buttons;

        fheroes2::Rect rt_radr;
        fheroes2::Rect rt_icon;
        fheroes2::Rect rt_bttn;
        fheroes2::Rect rt_stat;
        fheroes2::Rect rt_quit;
    };
}

#endif
