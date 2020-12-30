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

#include "agg.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_interface.h"
#include "settings.h"
#include "text.h"

int Dialog::FileOptions( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    // preload
    const int cpanbkg = Settings::Get().ExtGameEvilInterface() ? ICN::CPANBKGE : ICN::CPANBKG;
    const int cpanel = Settings::Get().ExtGameEvilInterface() ? ICN::CPANELE : ICN::CPANEL;

    // cursor
    Cursor & cursor = Cursor::Get();
    const int oldcursor = cursor.Themes();
    cursor.Hide();
    cursor.SetThemes( Cursor::POINTER );

    // image box
    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( cpanbkg, 0 );

    const fheroes2::Point rb( ( display.width() - box.width() - BORDERWIDTH ) / 2, ( display.height() - box.height() ) / 2 );
    fheroes2::ImageRestorer back( display, rb.x, rb.y, box.width(), box.height() );
    fheroes2::Blit( box, display, rb.x, rb.y );

    LocalEvent & le = LocalEvent::Get();

    fheroes2::Button buttonNew( rb.x + 62, rb.y + 31, cpanel, 0, 1 );
    fheroes2::Button buttonLoad( rb.x + 195, rb.y + 31, cpanel, 2, 3 );
    fheroes2::Button buttonSave( rb.x + 62, rb.y + 107, cpanel, 4, 5 );
    fheroes2::Button buttonQuit( rb.x + 195, rb.y + 107, cpanel, 6, 7 );
    fheroes2::Button buttonCancel( rb.x + 128, rb.y + 184, cpanel, 8, 9 );

    buttonNew.draw();
    buttonLoad.draw();
    buttonSave.draw();
    buttonQuit.draw();
    buttonCancel.draw();

    cursor.Show();
    display.render();

    int result = Game::QUITGAME;

    // dialog menu loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonNew.area() ) ? buttonNew.drawOnPress() : buttonNew.drawOnRelease();
        le.MousePressLeft( buttonLoad.area() ) ? buttonLoad.drawOnPress() : buttonLoad.drawOnRelease();
        le.MousePressLeft( buttonSave.area() ) ? buttonSave.drawOnPress() : buttonSave.drawOnRelease();
        le.MousePressLeft( buttonQuit.area() ) ? buttonQuit.drawOnPress() : buttonQuit.drawOnRelease();
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        if ( le.MouseClickLeft( buttonNew.area() ) ) {
            if ( Interface::Basic::Get().EventNewGame() == Game::NEWGAME ) {
                result = Game::NEWGAME;
                break;
            }
        }
        else if ( le.MouseClickLeft( buttonLoad.area() ) ) {
            if ( ListFiles::IsEmpty( Game::GetSaveDir(), Game::GetSaveFileExtension(), false ) ) {
                Dialog::Message( _( "Load Game" ), _( "No save files to load." ), Font::BIG, Dialog::OK );
            }
            else {
                if ( Interface::Basic::Get().EventLoadGame() == Game::LOADGAME ) {
                    const int gameType = Settings::Get().GameType();

                    if ( gameType & Game::TYPE_STANDARD )
                        result = Game::LOADSTANDARD;
                    else if ( gameType & Game::TYPE_CAMPAIGN )
                        result = Game::LOADCAMPAIN;
                    else
                        result = Game::LOADGAME;
                    break;
                }
            }
        }
        else if ( le.MouseClickLeft( buttonSave.area() ) ) {
            result = Interface::Basic::Get().EventSaveGame();
            break;
        }
        else if ( le.MouseClickLeft( buttonQuit.area() ) ) {
            if ( Interface::Basic::Get().EventExit() == Game::QUITGAME ) {
                result = Game::QUITGAME;
                break;
            }
        }
        else if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
            result = Game::CANCEL;
            break;
        }
        else if ( le.MousePressRight( buttonNew.area() ) ) {
            Dialog::Message( _( "New Game" ), _( "Start a single or multi-player game." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonLoad.area() ) ) {
            Dialog::Message( _( "Load Game" ), _( "Load a previously saved game." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonSave.area() ) ) {
            Dialog::Message( _( "Save Game" ), _( "Save the current game." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonQuit.area() ) ) {
            Dialog::Message( _( "Quit" ), _( "Quit out of Free Heroes of Might and Magic II." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonCancel.area() ) ) {
            Dialog::Message( _( "Cancel" ), _( "Exit this menu without doing anything." ), Font::BIG );
        }
    }

    // restore background
    cursor.Hide();
    back.restore();
    cursor.SetThemes( oldcursor );
    cursor.Show();
    display.render();

    return result;
}
