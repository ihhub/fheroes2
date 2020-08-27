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
#include "difficulty.h"
#include "game.h"
#include "game_over.h"
#include "icn.h"
#include "maps.h"
#include "player_info.h"
#include "settings.h"
#include "text.h"
#include "ui_button.h"

void Dialog::GameInfo( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    Settings & conf = Settings::Get();

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( ICN::SCENIBKG, 0 );

    Point pt( ( display.width() - box.width() ) / 2, ( display.height() - box.height() ) / 2 );
    fheroes2::ImageRestorer back( display, pt.x, pt.y, box.width(), box.height() );
    fheroes2::Blit( box, display, pt.x, pt.y );

    TextBox text;

    text.Set( conf.MapsName(), Font::BIG, 350 );
    text.Blit( pt.x + 52, pt.y + 30 );

    text.Set( _( "Map\nDifficulty" ), Font::SMALL, 80 );
    text.Blit( pt.x + 50, pt.y + 54 );

    text.Set( _( "Game\nDifficulty" ), Font::SMALL, 80 );
    text.Blit( pt.x + 140, pt.y + 54 );

    text.Set( _( "Rating" ), Font::SMALL, 80 );
    text.Blit( pt.x + 230, pt.y + 61 );

    text.Set( _( "Map Size" ), Font::SMALL, 80 );
    text.Blit( pt.x + 322, pt.y + 61 );

    text.Set( Difficulty::String( conf.MapsDifficulty() ), Font::SMALL, 80 );
    text.Blit( pt.x + 50, pt.y + 80 );

    text.Set( Difficulty::String( conf.GameDifficulty() ), Font::SMALL, 80 );
    text.Blit( pt.x + 140, pt.y + 80 );

    text.Set( GetString( Game::GetRating() ) + " %", Font::SMALL, 80 );
    text.Blit( pt.x + 230, pt.y + 80 );

    text.Set( Maps::SizeString( conf.MapsSize().w ), Font::SMALL, 80 );
    text.Blit( pt.x + 322, pt.y + 80 );

    text.Set( conf.MapsDescription(), Font::SMALL, 350 );
    text.Blit( pt.x + 52, pt.y + 105 );

    text.Set( _( "Opponents" ), Font::SMALL, 350 );
    text.Blit( pt.x + 52, pt.y + 150 );

    text.Set( _( "Class" ), Font::SMALL, 350 );
    text.Blit( pt.x + 52, pt.y + 225 );

    Interface::PlayersInfo playersInfo( true, true, false );

    playersInfo.UpdateInfo( conf.GetPlayers(), Point( pt.x + 40, pt.y + 165 ), Point( pt.x + 40, pt.y + 240 ) );
    playersInfo.RedrawInfo( true );

    text.Set( _( "Victory\nConditions" ), Font::SMALL, 80 );
    text.Blit( pt.x + 40, pt.y + 345 );

    text.Set( GameOver::GetActualDescription( conf.ConditionWins() ), Font::SMALL, 272 );
    text.Blit( pt.x + 130, pt.y + 348 );

    text.Set( _( "Loss\nConditions" ), Font::SMALL, 80 );
    text.Blit( pt.x + 40, pt.y + 390 );

    text.Set( GameOver::GetActualDescription( conf.ConditionLoss() ), Font::SMALL, 272 );
    text.Blit( pt.x + 130, pt.y + 396 );

    text.Set( "score: " + GetString( Game::GetGameOverScores() ), Font::YELLOW_SMALL, 80 );
    text.Blit( pt.x + 415 - text.w(), pt.y + 434 );

    fheroes2::Button buttonOk( pt.x + 180, pt.y + 425, ICN::SYSTEM, 1, 2 );
    fheroes2::Button buttonCfg( pt.x + 50, pt.y + 425, ICN::BTNCONFIG, 0, 1 );

    buttonOk.draw();
    buttonCfg.draw();

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.MousePressLeft( buttonCfg.area() ) ? buttonCfg.drawOnPress() : buttonCfg.drawOnRelease();

        if ( le.MouseClickLeft( buttonCfg.area() ) ) {
            Dialog::ExtSettings( true );
            Cursor::Get().Show();
            display.render();
        }

        if ( le.MouseClickLeft( buttonOk.area() ) || HotKeyCloseWindow )
            break;
    }

    cursor.Hide();
}
