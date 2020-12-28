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
#include "dialog_resolution.h"
#include "game.h"
#include "game_interface.h"
#include "gamedefs.h"
#include "image.h"
#include "mus.h"
#include "settings.h"
#include "text.h"
#include "ui_button.h"

#define NEWGAME_DEFAULT 1
#define LOADGAME_DEFAULT 5
#define HIGHSCORES_DEFAULT 9
#define CREDITS_DEFAULT 13
#define QUIT_DEFAULT 17

int Game::MainMenu( bool isFirstGameRun )
{
    Mixer::Pause();
    AGG::PlayMusic( MUS::MAINMENU );

    Settings & conf = Settings::Get();

    conf.SetGameType( TYPE_MENU );

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    // image background
    fheroes2::Copy( fheroes2::AGG::GetICN( ICN::HEROES, 0 ), display );
    if ( isFirstGameRun ) {
        bool isResolutionChanged = Dialog::SelectResolution();
        conf.Save( "fheroes2.cfg" );
        if ( isResolutionChanged ) {
            fheroes2::Copy( fheroes2::AGG::GetICN( ICN::HEROES, 0 ), display );
        }

        Dialog::Message( "Please remember",
                         "You can always change game resolution by clicking on the door on the left side of main menu. To switch between windowed "
                         "and full screen modes press 'F4' key on the keyboard. Enjoy the game!",
                         Font::BIG, Dialog::OK );
    }

    LocalEvent & le = LocalEvent::Get();

    fheroes2::Button buttonNewGame( 0, 0, ICN::BTNSHNGL, NEWGAME_DEFAULT, NEWGAME_DEFAULT + 2 );
    fheroes2::Button buttonLoadGame( 0, 0, ICN::BTNSHNGL, LOADGAME_DEFAULT, LOADGAME_DEFAULT + 2 );
    fheroes2::Button buttonHighScores( 0, 0, ICN::BTNSHNGL, HIGHSCORES_DEFAULT, HIGHSCORES_DEFAULT + 2 );
    fheroes2::Button buttonCredits( 0, 0, ICN::BTNSHNGL, CREDITS_DEFAULT, CREDITS_DEFAULT + 2 );
    fheroes2::Button buttonQuit( 0, 0, ICN::BTNSHNGL, QUIT_DEFAULT, QUIT_DEFAULT + 2 );

    const Point lt_pt( 0, 0 );

    const fheroes2::Sprite & lantern10 = fheroes2::AGG::GetICN( ICN::SHNGANIM, 0 );
    fheroes2::Blit( lantern10, display, lantern10.x(), lantern10.y() );

    const fheroes2::Sprite & lantern11 = fheroes2::AGG::GetICN( ICN::SHNGANIM, ICN::AnimationFrame( ICN::SHNGANIM, 0, 0 ) );
    fheroes2::Blit( lantern11, display, lantern11.x(), lantern11.y() );

    buttonNewGame.draw();
    buttonLoadGame.draw();
    buttonHighScores.draw();
    buttonCredits.draw();
    buttonQuit.draw();

    cursor.Show();
    display.render();

    const double scaleX = static_cast<double>( display.width() ) / fheroes2::Display::DEFAULT_WIDTH;
    const double scaleY = static_cast<double>( display.height() ) / fheroes2::Display::DEFAULT_HEIGHT;
    const Rect resolutionArea( 63 * scaleX, 202 * scaleY, 90 * scaleX, 160 * scaleY );

    u32 lantern_frame = 0;

    struct ButtonInfo
    {
        u32 frame;
        fheroes2::Button & button;
        bool isOver;
        bool wasOver;
    } buttons[] = {{NEWGAME_DEFAULT, buttonNewGame, false, false},
                   {LOADGAME_DEFAULT, buttonLoadGame, false, false},
                   {HIGHSCORES_DEFAULT, buttonHighScores, false, false},
                   {CREDITS_DEFAULT, buttonCredits, false, false},
                   {QUIT_DEFAULT, buttonQuit, false, false}};

    for ( u32 i = 0; le.MouseMotion() && i < ARRAY_COUNT( buttons ); i++ ) {
        cursor.Hide();
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::BTNSHNGL, buttons[i].frame );
        fheroes2::Blit( sprite, display, sprite.x(), sprite.y() );
        cursor.Show();
    }

    // mainmenu loop
    while ( 1 ) {
        if ( !le.HandleEvents( true, true ) ) {
            if ( Interface::Basic::EventExit() == QUITGAME ) {
                // if ( conf.ExtGameUseFade() )
                //    display.Fade();
                break;
            }
        }

        bool redrawScreen = false;

        for ( u32 i = 0; i < ARRAY_COUNT( buttons ); i++ ) {
            buttons[i].wasOver = buttons[i].isOver;

            if ( le.MousePressLeft( buttons[i].button.area() ) ) {
                buttons[i].button.drawOnPress();
            }
            else {
                buttons[i].button.drawOnRelease();
            }

            buttons[i].isOver = le.MouseCursor( buttons[i].button.area() );

            if ( buttons[i].isOver != buttons[i].wasOver ) {
                u32 frame = buttons[i].frame;

                if ( buttons[i].isOver && !buttons[i].wasOver )
                    frame++;

                if ( !redrawScreen ) {
                    cursor.Hide();
                    redrawScreen = true;
                }
                const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::BTNSHNGL, frame );
                fheroes2::Blit( sprite, display, sprite.x(), sprite.y() );
            }
        }

        if ( redrawScreen ) {
            cursor.Show();
            display.render();
        }

        if ( HotKeyPressEvent( EVENT_BUTTON_NEWGAME ) || le.MouseClickLeft( buttonNewGame.area() ) )
            return NEWGAME;
        else if ( HotKeyPressEvent( EVENT_BUTTON_LOADGAME ) || le.MouseClickLeft( buttonLoadGame.area() ) )
            return LOADGAME;
        else if ( HotKeyPressEvent( EVENT_BUTTON_HIGHSCORES ) || le.MouseClickLeft( buttonHighScores.area() ) )
            return HIGHSCORES;
        else if ( HotKeyPressEvent( EVENT_BUTTON_CREDITS ) || le.MouseClickLeft( buttonCredits.area() ) )
            return CREDITS;
        else if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonQuit.area() ) ) {
            if ( Interface::Basic::EventExit() == QUITGAME ) {
                // if ( conf.ExtGameUseFade() )
                //     display.Fade();
                return QUITGAME;
            }
        }
        else if ( le.MouseClickLeft( resolutionArea ) ) {
            if ( Dialog::SelectResolution() ) {
                conf.Save( "fheroes2.cfg" );
                // force interface to reset area and positions
                Interface::Basic::Get().Reset();
                return MAINMENU;
            }
        }

        // right info
        if ( le.MousePressRight( buttonQuit.area() ) )
            Dialog::Message( _( "Quit" ), _( "Quit Heroes of Might and Magic and return to the operating system." ), Font::BIG );
        else if ( le.MousePressRight( buttonLoadGame.area() ) )
            Dialog::Message( _( "Load Game" ), _( "Load a previously saved game." ), Font::BIG );
        else if ( le.MousePressRight( buttonCredits.area() ) )
            Dialog::Message( _( "Credits" ), _( "View the credits screen." ), Font::BIG );
        else if ( le.MousePressRight( buttonHighScores.area() ) )
            Dialog::Message( _( "High Scores" ), _( "View the high score screen." ), Font::BIG );
        else if ( le.MousePressRight( buttonNewGame.area() ) )
            Dialog::Message( _( "New Game" ), _( "Start a single or multi-player game." ), Font::BIG );
        else if ( le.MousePressRight( resolutionArea ) )
            Dialog::Message( _( "Select Game Resolution" ), _( "Change resolution of the game." ), Font::BIG );

        if ( AnimateInfrequentDelay( MAIN_MENU_DELAY ) ) {
            cursor.Hide();
            const fheroes2::Sprite & lantern12 = fheroes2::AGG::GetICN( ICN::SHNGANIM, ICN::AnimationFrame( ICN::SHNGANIM, 0, lantern_frame++ ) );
            fheroes2::Blit( lantern12, display, lantern12.x(), lantern12.y() );
            cursor.Show();
            display.render();
        }
    }

    return QUITGAME;
}
