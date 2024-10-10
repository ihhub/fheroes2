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

#include "interface_cpanel.h"

#include <cassert>

#include "agg_image.h"
#include "game_interface.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "screen.h"
#include "settings.h"

Interface::ControlPanel::ControlPanel( AdventureMap & basic )
    : interface( basic )
{
    width = 180;
    height = 36;

    rt_radar.width = 36;
    rt_radar.height = 36;
    rt_icons.width = 36;
    rt_icons.height = 36;
    rt_buttons.width = 36;
    rt_buttons.height = 36;
    rt_status.width = 36;
    rt_status.height = 36;
    rt_end.width = 36;
    rt_end.height = 36;

    ResetTheme();
}

void Interface::ControlPanel::ResetTheme()
{
    const int icn = Settings::Get().isEvilInterfaceEnabled() ? ICN::ADVEBTNS : ICN::ADVBTNS;

    _buttons.reset( new Buttons( fheroes2::AGG::GetICN( icn, 4 ), fheroes2::AGG::GetICN( icn, 0 ), fheroes2::AGG::GetICN( icn, 12 ), fheroes2::AGG::GetICN( icn, 10 ),
                                 fheroes2::AGG::GetICN( icn, 8 ) ) );
}

const fheroes2::Rect & Interface::ControlPanel::GetArea() const
{
    return *this;
}

void Interface::ControlPanel::SetPos( int32_t ox, int32_t oy )
{
    x = ox;
    y = oy;

    rt_radar.x = x;
    rt_radar.y = y;
    rt_icons.x = x + 36;
    rt_icons.y = y;
    rt_buttons.x = x + 72;
    rt_buttons.y = y;
    rt_status.x = x + 108;
    rt_status.y = y;
    rt_end.x = x + 144;
    rt_end.y = y;
}

void Interface::ControlPanel::_redraw() const
{
    assert( _buttons );

    fheroes2::Display & display = fheroes2::Display::instance();

    const uint8_t alpha = 128;

    fheroes2::AlphaBlit( _buttons->radar, display, rt_radar.x, rt_radar.y, alpha );
    fheroes2::AlphaBlit( _buttons->icons, display, rt_icons.x, rt_icons.y, alpha );
    fheroes2::AlphaBlit( _buttons->buttons, display, rt_buttons.x, rt_buttons.y, alpha );
    fheroes2::AlphaBlit( _buttons->status, display, rt_status.x, rt_status.y, alpha );
    fheroes2::AlphaBlit( _buttons->end, display, rt_end.x, rt_end.y, alpha );
}

fheroes2::GameMode Interface::ControlPanel::QueueEventProcessing() const
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.MouseClickLeft( rt_radar ) ) {
        interface.EventSwitchShowRadar();
    }
    else if ( le.MouseClickLeft( rt_icons ) ) {
        interface.EventSwitchShowIcons();
    }
    else if ( le.MouseClickLeft( rt_buttons ) ) {
        interface.EventSwitchShowButtons();
    }
    else if ( le.MouseClickLeft( rt_status ) ) {
        interface.EventSwitchShowStatus();
    }
    else if ( le.MouseClickLeft( rt_end ) ) {
        return interface.EventEndTurn();
    }

    return fheroes2::GameMode::CANCEL;
}
