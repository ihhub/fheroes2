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

#include <string>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h" // IWYU pragma: associated
#include "difficulty.h"
#include "game.h"
#include "game_hotkeys.h"
#include "game_over.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "math_base.h"
#include "player_info.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace
{
    enum GameInfoCoordinates
    {
        SCENARIO_INFO_VALUES_BOX_WIDTH = 80,
        DIALOG_CONTENT_WIDTH = 420,
        SCENARIO_INFO_BOX_INNER_MARGIN = 11,
        SCENARIO_INFO_ROW_OUTER_MARGIN = 16,
        DESCRIPTION_INNER_MARGIN = 6,
        SCENARIO_DESCRIPTION_OUTER_MARGIN = 11,
        // This is the shadow offset from the original ICN::SCENIBKG image.
        DIALOG_SHADOW_OFFSET_X = 16,
        DIALOG_SHADOW_OFFSET_Y = 4 + 12, // This ICN has been modified with a 12 px larger shadow.
        DIALOG_BORDER_WIDTH = 18,
        // The following values are calculated using the previous values.
        SCENARIO_INFO_ROW_OFFSET = DIALOG_BORDER_WIDTH + SCENARIO_INFO_ROW_OUTER_MARGIN,
        SCENARIO_MAP_DIFFICULTY_OFFSET = SCENARIO_INFO_ROW_OFFSET,
        SCENARIO_GAME_DIFFICULTY_OFFSET = SCENARIO_INFO_ROW_OFFSET + SCENARIO_INFO_VALUES_BOX_WIDTH + SCENARIO_INFO_BOX_INNER_MARGIN,
        SCENARIO_RATING_OFFSET = SCENARIO_INFO_ROW_OFFSET + SCENARIO_INFO_VALUES_BOX_WIDTH * 2 + SCENARIO_INFO_BOX_INNER_MARGIN * 2,
        SCENARIO_MAP_SIZE_OFFSET = SCENARIO_INFO_ROW_OFFSET + SCENARIO_INFO_VALUES_BOX_WIDTH * 3 + SCENARIO_INFO_BOX_INNER_MARGIN * 3,
        SCENARIO_DESCRIPTION_OFFSET = DIALOG_BORDER_WIDTH + SCENARIO_DESCRIPTION_OUTER_MARGIN + DESCRIPTION_INNER_MARGIN,
        SCENARIO_DESCRIPTION_WIDTH = 350,
        PLAYER_INFO_ROW_OFFSET = DIALOG_BORDER_WIDTH + 6,
        CONDITION_LABEL_OFFSET = DIALOG_BORDER_WIDTH,
        CONDITION_LABEL_WIDTH = 93,
        CONDITION_DESCRIPTION_OFFSET = CONDITION_LABEL_OFFSET + CONDITION_LABEL_WIDTH + DESCRIPTION_INNER_MARGIN,
        CONDITION_DESCRIPTION_WIDTH = 266,
        OK_BUTTON_OFFSET = DIALOG_CONTENT_WIDTH / 2,
    };
}

void Dialog::GameInfo()
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Settings & conf = Settings::Get();
    const Maps::FileInfo & mapInfo = conf.getCurrentMapInfo();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const bool isEvilInterface = conf.isEvilInterfaceEnabled();
    const fheroes2::Sprite & window = fheroes2::AGG::GetICN( isEvilInterface ? ICN::SCENIBKG_EVIL : ICN::SCENIBKG, 0 );

    const fheroes2::Point dialogOffset( ( display.width() - window.width() - DIALOG_SHADOW_OFFSET_X ) / 2, ( display.height() - window.height() ) / 2 );
    const fheroes2::Point shadowOffset( dialogOffset.x + DIALOG_SHADOW_OFFSET_X, dialogOffset.y + DIALOG_SHADOW_OFFSET_Y / 2 );

    fheroes2::ImageRestorer restorer( display, dialogOffset.x, shadowOffset.y, window.width(), window.height() );

    fheroes2::Blit( window, display, dialogOffset.x, shadowOffset.y );

    fheroes2::Text text( mapInfo.name, fheroes2::FontType::normalWhite(), mapInfo.getSupportedLanguage() );
    text.draw( shadowOffset.x, shadowOffset.y + 32, DIALOG_CONTENT_WIDTH, display );

    text.set( _( "Map\nDifficulty" ), fheroes2::FontType::smallWhite() );
    text.draw( shadowOffset.x + SCENARIO_MAP_DIFFICULTY_OFFSET, shadowOffset.y + 56, SCENARIO_INFO_VALUES_BOX_WIDTH, display );

    text.set( _( "Game\nDifficulty" ), fheroes2::FontType::smallWhite() );
    text.draw( shadowOffset.x + SCENARIO_GAME_DIFFICULTY_OFFSET, shadowOffset.y + 56, SCENARIO_INFO_VALUES_BOX_WIDTH, display );

    text.set( _( "Rating" ), fheroes2::FontType::smallWhite() );
    text.draw( shadowOffset.x + SCENARIO_RATING_OFFSET, shadowOffset.y + 78 - text.height( SCENARIO_INFO_VALUES_BOX_WIDTH ), SCENARIO_INFO_VALUES_BOX_WIDTH, display );

    text.set( _( "Map Size" ), fheroes2::FontType::smallWhite() );
    text.draw( shadowOffset.x + SCENARIO_MAP_SIZE_OFFSET, shadowOffset.y + 78 - text.height( SCENARIO_INFO_VALUES_BOX_WIDTH ), SCENARIO_INFO_VALUES_BOX_WIDTH, display );

    text.set( Difficulty::String( mapInfo.difficulty ), fheroes2::FontType::smallWhite() );
    text.draw( shadowOffset.x + SCENARIO_MAP_DIFFICULTY_OFFSET, shadowOffset.y + 84, SCENARIO_INFO_VALUES_BOX_WIDTH, display );

    text.set( Difficulty::String( Game::getDifficulty() ), fheroes2::FontType::smallWhite() );
    text.draw( shadowOffset.x + SCENARIO_GAME_DIFFICULTY_OFFSET, shadowOffset.y + 84, SCENARIO_INFO_VALUES_BOX_WIDTH, display );

    text.set( std::to_string( Game::GetRating() ) + " %", fheroes2::FontType::smallWhite() );
    text.draw( shadowOffset.x + SCENARIO_RATING_OFFSET, shadowOffset.y + 84, SCENARIO_INFO_VALUES_BOX_WIDTH, display );

    text.set( Maps::SizeString( mapInfo.width ), fheroes2::FontType::smallWhite() );
    text.draw( shadowOffset.x + SCENARIO_MAP_SIZE_OFFSET, shadowOffset.y + 84, SCENARIO_INFO_VALUES_BOX_WIDTH, display );

    text.set( mapInfo.description, fheroes2::FontType::smallWhite(), mapInfo.getSupportedLanguage() );
    text.draw( shadowOffset.x + SCENARIO_DESCRIPTION_OFFSET, shadowOffset.y + 107, SCENARIO_DESCRIPTION_WIDTH, display );

    text.set( _( "Opponents" ), fheroes2::FontType::smallWhite() );
    text.draw( shadowOffset.x, shadowOffset.y + 152, DIALOG_CONTENT_WIDTH, display );

    text.set( _( "Class" ), fheroes2::FontType::smallWhite() );
    text.draw( shadowOffset.x, shadowOffset.y + 229, DIALOG_CONTENT_WIDTH, display );

    Interface::PlayersInfo playersInfo;

    playersInfo.UpdateInfo( conf.GetPlayers(), fheroes2::Point( shadowOffset.x + PLAYER_INFO_ROW_OFFSET, shadowOffset.y + 165 ),
                            fheroes2::Point( shadowOffset.x + PLAYER_INFO_ROW_OFFSET, shadowOffset.y + 240 ) );
    playersInfo.RedrawInfo( true );

    text.set( _( "Victory\nConditions" ), fheroes2::FontType::smallWhite() );
    text.draw( shadowOffset.x + CONDITION_LABEL_OFFSET, shadowOffset.y + 347, CONDITION_LABEL_WIDTH, display );

    text.set( GameOver::GetActualDescription( mapInfo.ConditionWins() ), fheroes2::FontType::smallWhite() );
    text.setUniformVerticalAlignment( false );
    text.draw( shadowOffset.x + CONDITION_DESCRIPTION_OFFSET, shadowOffset.y + 350, CONDITION_DESCRIPTION_WIDTH, display );

    text.set( _( "Loss\nConditions" ), fheroes2::FontType::smallWhite() );
    text.setUniformVerticalAlignment( true );
    text.draw( shadowOffset.x + CONDITION_LABEL_OFFSET, shadowOffset.y + 392, CONDITION_LABEL_WIDTH, display );

    text.set( GameOver::GetActualDescription( mapInfo.ConditionLoss() ), fheroes2::FontType::smallWhite() );
    text.setUniformVerticalAlignment( false );
    text.draw( shadowOffset.x + CONDITION_DESCRIPTION_OFFSET, shadowOffset.y + 398, CONDITION_DESCRIPTION_WIDTH, display );

    const int buttonOkIcnId = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
    fheroes2::Button buttonOk( shadowOffset.x + OK_BUTTON_OFFSET - fheroes2::AGG::GetICN( buttonOkIcnId, 0 ).width() / 2, shadowOffset.y + 426, buttonOkIcnId, 0, 1 );
    buttonOk.draw();

    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();

        if ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyCloseWindow() )
            break;

        if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Okay" ), _( "Exit this menu." ), 0 );
        }
        else {
            playersInfo.readOnlyEventProcessing();
        }
    }
}
