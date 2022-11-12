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

#include <cstdint>
#include <ostream>
#include <string>

#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "localevent.h"
#include "logging.h"
#include "math_base.h"
#include "screen.h"
#include "text.h"
#include "ui_button.h"
#include "translations.h"

namespace
{
    void outputInTextSupportMode( const std::string & header, const std::string & message, const int buttonTypes )
    {
        START_TEXT_SUPPORT_MODE

        if ( !header.empty() ) {
            COUT( header )
            COUT( '\n' )
        }
        COUT( message )

        if ( buttonTypes & Dialog::YES ) {
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_OKAY ) << " to choose YES." )
        }
        if ( buttonTypes & Dialog::NO ) {
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_CANCEL ) << " to choose NO." )
        }
        if ( buttonTypes & Dialog::OK ) {
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_OKAY ) << " to choose OK." )
        }
        if ( buttonTypes & Dialog::CANCEL ) {
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_CANCEL ) << " to choose CANCEL." )
        }
    }
}

int Dialog::Message( const std::string & header, const std::string & message, int ft, int buttons )
{
    outputInTextSupportMode( header, message, buttons );

    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( buttons != 0, Cursor::POINTER );

    TextBox textbox1( header, Font::YELLOW_BIG, BOXAREA_WIDTH );
    TextBox textbox2( _(message), ft, BOXAREA_WIDTH );

    const int32_t headerHeight = !header.empty() ? textbox1.h() + 10 : 0;
    FrameBox box( 10 + headerHeight + textbox2.h(), buttons != 0 );
    const fheroes2::Rect & pos = box.GetArea();

    if ( !header.empty() )
        textbox1.Blit( pos.x, pos.y + 10 );
    if ( !message.empty() )
        textbox2.Blit( pos.x, pos.y + 10 + headerHeight );

    LocalEvent & le = LocalEvent::Get();

    fheroes2::ButtonGroup group( pos, buttons );
    group.draw();

    display.render();

    // message loop
    int result = Dialog::ZERO;

    while ( result == Dialog::ZERO && le.HandleEvents() ) {
        if ( !buttons && !le.MousePressRight() )
            break;
        result = group.processEvents();
    }

    return result;
}
