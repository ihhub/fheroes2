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

#include "image.h"

namespace Interface
{
    class Basic;

    class ControlPanel : protected fheroes2::Rect
    {
    public:
        ControlPanel( Basic & );

        void SetPos( int32_t, int32_t );
        void Redraw( void );
        void ResetTheme( void );
        int QueueEventProcessing( void );

        const fheroes2::Rect & GetArea( void );

    private:
        Basic & interface;

        fheroes2::Sprite btn_radr;
        fheroes2::Sprite btn_icon;
        fheroes2::Sprite btn_bttn;
        fheroes2::Sprite btn_stat;
        fheroes2::Sprite btn_quit;

        fheroes2::Rect rt_radr;
        fheroes2::Rect rt_icon;
        fheroes2::Rect rt_bttn;
        fheroes2::Rect rt_stat;
        fheroes2::Rect rt_quit;
    };
}

#endif
