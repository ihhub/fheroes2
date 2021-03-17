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

#include "interface_cpanel.h"
#include "agg_image.h"
#include "game.h"
#include "game_interface.h"
#include "icn.h"
#include "localevent.h"
#include "settings.h"

Interface::ControlPanel::ControlPanel( Basic & basic )
    : interface( basic )
{
    width = 180;
    height = 36;

    rt_radr.width = 36;
    rt_radr.height = 36;
    rt_icon.width = 36;
    rt_icon.height = 36;
    rt_bttn.width = 36;
    rt_bttn.height = 36;
    rt_stat.width = 36;
    rt_stat.height = 36;
    rt_quit.width = 36;
    rt_quit.height = 36;

    ResetTheme();
}

void Interface::ControlPanel::ResetTheme( void )
{
    const int icn = Settings::Get().ExtGameEvilInterface() ? ICN::ADVEBTNS : ICN::ADVBTNS;

    // Make a copy of Surface
    btn_radr = fheroes2::AGG::GetICN( icn, 4 );
    btn_icon = fheroes2::AGG::GetICN( icn, 0 );
    btn_bttn = fheroes2::AGG::GetICN( icn, 12 );
    btn_stat = fheroes2::AGG::GetICN( icn, 10 );
    btn_quit = fheroes2::AGG::GetICN( icn, 8 );
}

const fheroes2::Rect & Interface::ControlPanel::GetArea( void )
{
    return *this;
}

void Interface::ControlPanel::SetPos( int32_t ox, int32_t oy )
{
    x = ox;
    y = oy;

    rt_radr.x = x;
    rt_radr.y = y;
    rt_icon.x = x + 36;
    rt_icon.y = y;
    rt_bttn.x = x + 72;
    rt_bttn.y = y;
    rt_stat.x = x + 108;
    rt_stat.y = y;
    rt_quit.x = x + 144;
    rt_quit.y = y;
}

void Interface::ControlPanel::Redraw( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const uint8_t alpha = 128;

    fheroes2::AlphaBlit( btn_radr, display, x, y, alpha );
    fheroes2::AlphaBlit( btn_icon, display, x + 36, y, alpha );
    fheroes2::AlphaBlit( btn_bttn, display, x + 72, y, alpha );
    fheroes2::AlphaBlit( btn_stat, display, x + 108, y, alpha );
    fheroes2::AlphaBlit( btn_quit, display, x + 144, y, alpha );
}

int Interface::ControlPanel::QueueEventProcessing( void )
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.MouseClickLeft( rt_radr ) )
        interface.EventSwitchShowRadar();
    else if ( le.MouseClickLeft( rt_icon ) )
        interface.EventSwitchShowIcons();
    else if ( le.MouseClickLeft( rt_bttn ) )
        interface.EventSwitchShowButtons();
    else if ( le.MouseClickLeft( rt_stat ) )
        interface.EventSwitchShowStatus();
    else if ( le.MouseClickLeft( rt_quit ) )
        return interface.EventEndTurn();

    return Game::CANCEL;
}
