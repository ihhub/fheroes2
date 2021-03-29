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

#include "statusbar.h"
#include "cursor.h"
#include "screen.h"

StatusBar::StatusBar() {}

void StatusBar::SetCenter( s32 cx, s32 cy )
{
    center.x = cx;
    center.y = cy;
}

void StatusBar::ShowMessage( const std::string & msg )
{
    if ( msg != prev ) {
        Cursor::Get().Hide();
        SetText( msg );
        SetPos( center.x - w() / 2, center.y - h() / 2 );
        Show();
        Cursor::Get().Show();
        fheroes2::Display::instance().render();
        prev = msg;
    }
}

void StatusBar::Redraw( void )
{
    Hide();
    Show();
}
