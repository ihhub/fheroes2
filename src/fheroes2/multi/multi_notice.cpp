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

#include "multi_notice.h"

#include <algorithm>
#include <chrono>
#include <cstdint>

#include "cursor.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "interface_base.h"
#include "multi_game.h"
#include "multi_simultaneous.h"
#include "screen.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    struct MapNote
    {
        std::string message;
        std::chrono::steady_clock::time_point end;
        // Stays until the end of the wait.
        bool isHeld{ false };
    };

    MapNote & mapNote()
    {
        static MapNote note;
        return note;
    }

    // Kept to detect clicks on the battle of an ally.
    fheroes2::Rect & bannerArea()
    {
        static fheroes2::Rect area;
        return area;
    }

    std::chrono::seconds getReadingTime( const std::string & message )
    {
        return std::max( std::chrono::seconds( 4 ), std::chrono::seconds( message.size() / 15 ) );
    }

    void redrawMap()
    {
        Interface::AdventureMap::Get().redraw( Interface::REDRAW_GAMEAREA );
        fheroes2::Display::instance().render();
    }
}

void Multi::showMapNote( const std::string & message )
{
    MapNote & note = mapNote();
    note.message = message;
    note.end = std::chrono::steady_clock::now() + getReadingTime( message );
    note.isHeld = false;

    Interface::AdventureMap::Get().setRedraw( Interface::REDRAW_GAMEAREA );
}

Multi::MapWait::MapWait( const std::string & message )
{
    if ( isTestMode() ) {
        return;
    }

    _cursorRestorer = std::make_unique<CursorRestorer>( true, Cursor::WAIT );

    MapNote & note = mapNote();
    note.message = message;
    note.isHeld = true;

    redrawMap();
}

Multi::MapWait::~MapWait()
{
    if ( !_cursorRestorer ) {
        return;
    }

    // A note from showMapNote() during the wait keeps its own time.
    MapNote & note = mapNote();
    if ( note.isHeld ) {
        note.message.clear();
        note.isHeld = false;
    }

    redrawMap();
}

void Multi::drawMapNote( const fheroes2::Rect & gameArea )
{
    // While waiting, the battle of an ally goes first and the note below it.
    bannerArea() = {};
    int32_t offset = 0;

    if ( isObservingOtherPlayers() ) {
        const std::string bannerText = getObserverBannerText();
        if ( !bannerText.empty() ) {
            bannerArea() = drawMapText( bannerText, gameArea, 0 );
            offset = bannerArea().height + 4;
        }
    }

    MapNote & note = mapNote();
    if ( note.message.empty() ) {
        return;
    }

    // Removed at the first redraw after its time.
    if ( !note.isHeld && std::chrono::steady_clock::now() > note.end ) {
        note.message.clear();
        return;
    }

    drawMapText( note.message, gameArea, offset );
}

fheroes2::Rect Multi::getMapBannerArea()
{
    return bannerArea();
}

fheroes2::Rect Multi::drawMapText( const std::string & message, const fheroes2::Rect & gameArea, const int32_t offset )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Text text( message, fheroes2::FontType::normalYellow() );
    const int32_t width = std::min( text.width() + 20, gameArea.width );
    const fheroes2::Rect area( gameArea.x + ( gameArea.width - width ) / 2, gameArea.y + 4 + offset, width, text.height( width - 20 ) + 8 );

    fheroes2::StandardWindow::applyTextBackgroundShading( display, area );
    text.draw( area.x + 10, area.y + 5, width - 20, display );

    return area;
}

void Multi::showMapNoteForTest( const std::string & message )
{
    MapNote & note = mapNote();
    note.message = message;
    note.isHeld = true;

    Interface::AdventureMap::Get().observeOtherPlayers( []() { return !Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ); } );

    note.message.clear();
    note.isHeld = false;
}
