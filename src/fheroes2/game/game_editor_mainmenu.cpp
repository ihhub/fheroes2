/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

#include "agg_image.h"
#include "audio_manager.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_hotkeys.h"
#include "game_mainmenu_ui.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "mus.h"
#include "screen.h"
#include "text.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_tool.h"

namespace
{
    const int32_t buttonYStep = 66;
}

fheroes2::GameMode Game::editorMainMenu()
{
    // Stop all sounds, but not the music
    AudioManager::stopSounds();

    AudioManager::PlayMusicAsync( MUS::MAINMENU, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::drawEditorMainMenuScreen();

    const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

    std::vector<fheroes2::Button> buttons( 3 );
    const size_t buttonCount = buttons.size();

    buttons[0].setICNInfo( ICN::BTNEMAIN, 0, 1 );
    buttons[1].setICNInfo( ICN::BTNEMAIN, 2, 3 );
    buttons[2].setICNInfo( ICN::BUTTON_LARGE_CANCEL, 0, 1 );

    for ( size_t i = 0; i < buttonCount - 1; ++i ) {
        buttons[i].setPosition( buttonPos.x, buttonPos.y + buttonYStep * static_cast<int32_t>( i ) );
        buttons[i].draw();
    }

    // following the cancel button in newgame
    buttons.back().setPosition( buttonPos.x, buttonPos.y + buttonYStep * 5 );
    buttons.back().draw();

    fheroes2::validateFadeInAndRender();

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        for ( size_t i = 0; i < buttonCount; ++i ) {
            le.MousePressLeft( buttons[i].area() ) ? buttons[i].drawOnPress() : buttons[i].drawOnRelease();
        }

        if ( le.MouseClickLeft( buttons[2].area() ) || HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) ) {
            return fheroes2::GameMode::MAIN_MENU;
        }

        if ( le.MousePressRight( buttons[0].area() ) ) {
            Dialog::Message( _( "New Map" ), _( "Create a new map either from scratch or using the random map generator." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttons[1].area() ) ) {
            Dialog::Message( _( "Load Map" ), _( "Load an existing map." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttons[2].area() ) ) {
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );
        }
    }

    return fheroes2::GameMode::MAIN_MENU;
}
