/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include "multi_waiting_screen.h"

#include <cstdint>
#include <utility>

#include "dialog.h"
#include "game.h"
#include "game_exit.h"
#include "game_hotkeys.h"
#include "game_mainmenu_ui.h"
#include "localevent.h"
#include "multi_protocol.h"
#include "screen.h"
#include "system.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_window.h"

Multi::WaitingScreen::WaitingScreen( std::string message )
    : _message( std::move( message ) )
{
    _draw();
}

Multi::WaitingScreen::~WaitingScreen() = default;

bool Multi::WaitingScreen::keepWaiting()
{
    LocalEvent & le = LocalEvent::Get();

    if ( !le.HandleEvents( true, true ) ) {
        if ( Game::processExitEvent() == fheroes2::GameMode::QUIT_GAME ) {
            _isQuitRequested = true;
            return false;
        }

        _draw();
    }

    if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || ( System::isHandheldDevice() && le.MouseClickLeft() ) ) {
        if ( Dialog::YES & fheroes2::showStandardTextMessage( _( "Network game" ), _( "Are you sure you want to leave the network game?" ), Dialog::YES | Dialog::NO ) ) {
            _isCancelled = true;
            return false;
        }

        _draw();
    }

    return true;
}

Multi::KeepWaiting Multi::WaitingScreen::callback()
{
    return [this]() { return keepWaiting(); };
}

void Multi::WaitingScreen::setMessage( std::string message )
{
    if ( message != _message ) {
        _message = std::move( message );
        _draw();
    }
}

void Multi::WaitingScreen::_draw()
{
    fheroes2::Display & display = fheroes2::Display::instance();

    _window.reset();

    fheroes2::drawMainMenuScreen();

    const fheroes2::Text title( _( "Network game" ), fheroes2::FontType::normalYellow() );
    const fheroes2::Text text( _message + "\n\n" + ( System::isHandheldDevice() ? _( "Touch the screen to leave." ) : _( "Press Esc to leave." ) ),
                               fheroes2::FontType::normalWhite() );

    const int32_t textWidth = 260;

    // The window erases itself when destroyed.
    _window = std::make_unique<fheroes2::StandardWindow>( textWidth + 40, title.height() + text.height( textWidth ) + 60, true, display );
    const fheroes2::Rect & area = _window->activeArea();

    title.draw( area.x + ( area.width - title.width() ) / 2, area.y + 20, display );
    text.draw( area.x + 20, area.y + 30 + title.height(), textWidth, display );

    display.render();
}

void Multi::showNetworkError( const std::string & header, const std::string & reason )
{
    fheroes2::engine().requestAttention();
    fheroes2::showStandardTextMessage( _( "Network game" ), header + "\n\n" + translateError( reason ), Dialog::OK );
}
