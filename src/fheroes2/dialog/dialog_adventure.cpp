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
#include "settings.h"
#include "text.h"
#include "ui_button.h"

int Dialog::AdventureOptions( bool enabledig )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // preload
    const int apanbkg = Settings::Get().ExtGameEvilInterface() ? ICN::APANBKGE : ICN::APANBKG;
    const int apanel = Settings::Get().ExtGameEvilInterface() ? ICN::APANELE : ICN::APANEL;

    // cursor
    Cursor & cursor = Cursor::Get();
    const int oldcursor = cursor.Themes();

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    // image box
    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( apanbkg, 0 );

    Point rb( ( display.width() - box.width() ) / 2, ( display.height() - box.height() ) / 2 );
    fheroes2::ImageRestorer back( display, rb.x, rb.y, box.width(), box.height() );
    fheroes2::Blit( box, display, rb.x, rb.y );

    LocalEvent & le = LocalEvent::Get();

    fheroes2::Button buttonWorld( rb.x + 62, rb.y + 30, apanel, 0, 1 );
    fheroes2::Button buttonPuzzle( rb.x + 195, rb.y + 30, apanel, 2, 3 );
    fheroes2::Button buttonInfo( rb.x + 62, rb.y + 107, apanel, 4, 5 );
    fheroes2::Button buttonDig( rb.x + 195, rb.y + 107, apanel, 6, 7 );
    fheroes2::Button buttonCancel( rb.x + 128, rb.y + 184, apanel, 8, 9 );

    if ( !enabledig )
        buttonDig.disable();

    buttonWorld.draw();
    buttonPuzzle.draw();
    buttonInfo.draw();
    buttonDig.draw();
    buttonCancel.draw();

    cursor.Show();
    display.render();

    int result = Dialog::ZERO;

    // dialog menu loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonWorld.area() ) ? buttonWorld.drawOnPress() : buttonWorld.drawOnRelease();
        le.MousePressLeft( buttonPuzzle.area() ) ? buttonPuzzle.drawOnPress() : buttonPuzzle.drawOnRelease();
        le.MousePressLeft( buttonInfo.area() ) ? buttonInfo.drawOnPress() : buttonInfo.drawOnRelease();
        le.MousePressLeft( buttonDig.area() ) ? buttonDig.drawOnPress() : buttonDig.drawOnRelease();
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        if ( le.MouseClickLeft( buttonWorld.area() ) ) {
            result = Dialog::WORLD;
            break;
        }
        if ( le.MouseClickLeft( buttonPuzzle.area() ) ) {
            result = Dialog::PUZZLE;
            break;
        }
        if ( le.MouseClickLeft( buttonInfo.area() ) ) {
            result = Dialog::INFO;
            break;
        }
        if ( le.MouseClickLeft( buttonDig.area() ) && buttonDig.isEnabled() ) {
            result = Dialog::DIG;
            break;
        }
        if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
            result = Dialog::CANCEL;
            break;
        }

        // right info
        if ( le.MousePressRight( buttonWorld.area() ) )
            Dialog::Message( "", _( "View the entire world." ), Font::BIG );
        if ( le.MousePressRight( buttonPuzzle.area() ) )
            Dialog::Message( "", _( "View the obelisk puzzle." ), Font::BIG );
        if ( le.MousePressRight( buttonInfo.area() ) )
            Dialog::Message( "", _( "View information on the scenario you are currently playing." ), Font::BIG );
        if ( le.MousePressRight( buttonDig.area() ) )
            Dialog::Message( "", _( "Dig for the Ultimate Artifact." ), Font::BIG );
        if ( le.MousePressRight( buttonCancel.area() ) )
            Dialog::Message( "", _( "Exit this menu without doing anything." ), Font::BIG );
    }

    // restore background
    cursor.Hide();
    back.restore();
    cursor.SetThemes( oldcursor );
    cursor.Show();
    display.render();

    return result;
}
