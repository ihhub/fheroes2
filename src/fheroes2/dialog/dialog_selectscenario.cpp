/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "dialog_selectscenario.h"
#include "agg_image.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "difficulty.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "localevent.h"
#include "maps.h"
#include "settings.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

#include <cassert>

namespace
{
    enum
    {
        // MAP LIST
        SCENARIO_LIST_COUNT_PLAYERS_OFFSET_X = 48,
        SCENARIO_LIST_MAP_SIZE_OFFSET_X = 67,
        SCENARIO_LIST_MAP_TYPE_OFFSET_X = 86,
        SCENARIO_LIST_MAP_NAME_OFFSET_X = 104,
        SCENARIO_LIST_MAP_NAME_WIDTH = 160,
        SCENARIO_LIST_VICTORY_CONDITION_OFFSET_X = 266,
        SCENARIO_LIST_LOSS_CONDITION_OFFSET_X = 285,
        SCENARIO_LIST_ROW_OFFSET_Y = 57,
        SCENARIO_LIST_COLUMN_HEIGHT = 175,
        MAP_LIST_ROW_SPACING_Y = 4,
        // SELECTED
        SELECTED_SCENARIO_COUNT_PLAYERS_OFFSET_X = 45,
        SELECTED_SCENARIO_MAP_SIZE_OFFSET_X = 64,
        SELECTED_SCENARIO_MAP_TYPE_OFFSET_X = 83,
        SELECTED_SCENARIO_MAP_NAME_OFFSET_X = 107,
        SELECTED_SCENARIO_MAP_NAME_WIDTH = 160,
        SELECTED_SCENARIO_VICTORY_CONDITION_OFFSET_X = 276,
        SELECTED_SCENARIO_LOSS_CONDITION_OFFSET_X = 295,
        SELECTED_SCENARIO_DIFFICULTY_OFFSET_X = 220,
        SELECTED_SCENARIO_DIFFICULTY_OFFSET_Y = 293,
        SELECTED_SCENARIO_DIFFICULTY_WIDTH = 114,
        SELECTED_SCENARIO_DIFFICULTY_HEIGHT = 20,
        SELECTED_SCENARIO_DESCRIPTION_OFFSET_X = 42,
        SELECTED_SCENARIO_DESCRIPTION_OFFSET_Y = 318,
        SELECTED_SCENARIO_DESCRIPTION_WIDTH = 292,
        SELECTED_SCENARIO_DESCRIPTION_HEIGHT = 90,
        SELECTED_SCENARIO_GENERAL_OFFSET_Y = 265,
        // COMMON
        ICON_SIZE = 18,
        MAP_SIZE_BUTTON_OFFSET_Y = 23
    };

    void ShowToolTip( const std::string & header, const std::string & body )
    {
        fheroes2::showStandardTextMessage( header, body, Dialog::ZERO );
    }

    void mapInfo( const Maps::FileInfo & info )
    {
        // On some OSes like Windows, the path may contain '\' symbols. This symbol doesn't exist in the resources.
        // To avoid this we have to replace all '\' symbols by '/' symbols.
        std::string fullPath = info.file;
        StringReplace( fullPath, "\\", "/" );

        fheroes2::Text header( info.name, fheroes2::FontType::normalYellow() );

        fheroes2::MultiFontText body;
        body.add( { _( "Location: " ), fheroes2::FontType::normalYellow() } );
        body.add( { fullPath, fheroes2::FontType::normalWhite() } );
        body.add( { _( "\n\nMap Type:\n" ), fheroes2::FontType::normalYellow() } );
        switch ( info._version ) {
        case GameVersion::SUCCESSION_WARS:
            body.add( { _( "The Succession Wars" ), fheroes2::FontType::normalWhite() } );
            break;
        case GameVersion::PRICE_OF_LOYALTY:
            body.add( { _( "The Price of Loyalty" ), fheroes2::FontType::normalWhite() } );
            break;
        default:
            // Did you add a new map version? Add the logic above!
            assert( 0 );
            break;
        }

        fheroes2::showMessage( header, body, Dialog::ZERO );
    }

    void LossConditionInfo( const Maps::FileInfo & info )
    {
        std::string msg;

        switch ( info.conditions_loss ) {
        case Maps::FileInfo::LOSS_EVERYTHING:
            msg = _( "Lose all your heroes and towns." );
            break;
        case Maps::FileInfo::LOSS_TOWN:
            msg = _( "Lose a specific town." );
            break;
        case Maps::FileInfo::LOSS_HERO:
            msg = _( "Lose a specific hero." );
            break;
        case Maps::FileInfo::LOSS_OUT_OF_TIME:
            msg = _( "Run out of time. Fail to win by a certain point." );
            break;
        default:
            // This is an unknown condition. Add the logic for it above!
            assert( 0 );
            return;
        }

        ShowToolTip( _( "Loss Condition" ), msg );
    }

    void VictoryConditionInfo( const Maps::FileInfo & info )
    {
        std::string msg;

        switch ( info.conditions_wins ) {
        case Maps::FileInfo::VICTORY_DEFEAT_EVERYONE:
            msg = _( "Defeat all enemy heroes and towns." );
            break;
        case Maps::FileInfo::VICTORY_CAPTURE_TOWN:
            msg = _( "Capture a specific town." );
            break;
        case Maps::FileInfo::VICTORY_KILL_HERO:
            msg = _( "Defeat a specific hero." );
            break;
        case Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT:
            msg = _( "Find a specific artifact." );
            break;
        case Maps::FileInfo::VICTORY_DEFEAT_OTHER_SIDE:
            msg = _( "Your side defeats the opposing side." );
            break;
        case Maps::FileInfo::VICTORY_COLLECT_ENOUGH_GOLD:
            msg = _( "Accumulate a large amount of gold." );
            break;
        default:
            // This is an unknown condition. Add the logic for it above!
            assert( 0 );
            return;
        }
        ShowToolTip( _( "Victory Condition" ), msg );
    }

    size_t GetSelectedMapId( const MapsFileInfoList & lists )
    {
        const Settings & conf = Settings::Get();

        const std::string & mapName = conf.CurrentFileInfo().name;
        const std::string & mapFileName = System::GetBasename( conf.CurrentFileInfo().file );
        size_t mapId = 0;
        for ( MapsFileInfoList::const_iterator mapIter = lists.begin(); mapIter != lists.end(); ++mapIter, ++mapId ) {
            if ( ( mapIter->name == mapName ) && ( System::GetBasename( mapIter->file ) == mapFileName ) ) {
                return mapId;
            }
        }

        return 0;
    }

    int32_t GetCenteredTextXCoordinate( const int32_t startCoordX, const int32_t areaWidth, const int32_t textWidth )
    {
        const int32_t centerTransform = areaWidth > textWidth ? ( areaWidth - textWidth ) / 2 : 0;
        return startCoordX + centerTransform;
    }
}

void ScenarioListBox::RedrawItem( const Maps::FileInfo & info, int32_t /*dstx*/, int32_t dsty, bool current )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    dsty = dsty + MAP_LIST_ROW_SPACING_Y;

    _renderScenarioListItem( info, display, dsty, current );
}

void ScenarioListBox::RedrawBackground( const fheroes2::Point & dst )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::REQSBKG, 0 ), display, dst.x, dst.y );

    if ( isSelected() ) {
        _renderSelectedScenarioInfo( display, dst );
    }
}

void ScenarioListBox::_renderScenarioListItem( const Maps::FileInfo & info, fheroes2::Display & display, int32_t & dsty, bool current )
{
    fheroes2::Blit( _getPlayersCountIcon( info.kingdom_colors ), display, _offsetX + SCENARIO_LIST_COUNT_PLAYERS_OFFSET_X, dsty );
    _renderMapIcon( info.size_w, display, _offsetX + SCENARIO_LIST_MAP_SIZE_OFFSET_X, dsty );
    fheroes2::Blit( _getMapTypeIcon( info._version ), display, _offsetX + SCENARIO_LIST_MAP_TYPE_OFFSET_X, dsty );
    _renderMapName( info, current, dsty, display );
    fheroes2::Blit( _getWinConditionsIcon( info.conditions_wins ), display, _offsetX + SCENARIO_LIST_VICTORY_CONDITION_OFFSET_X, dsty );
    fheroes2::Blit( _getLossConditionsIcon( info.conditions_loss ), display, _offsetX + SCENARIO_LIST_LOSS_CONDITION_OFFSET_X, dsty );
}

void ScenarioListBox::_renderSelectedScenarioInfo( fheroes2::Display & display, const fheroes2::Point & dst )
{
    const Maps::FileInfo & info = GetCurrent();

    fheroes2::Blit( _getPlayersCountIcon( info.kingdom_colors ), display, dst.x + SELECTED_SCENARIO_COUNT_PLAYERS_OFFSET_X, dst.y + SELECTED_SCENARIO_GENERAL_OFFSET_Y );
    _renderMapIcon( info.size_w, display, dst.x + SELECTED_SCENARIO_MAP_SIZE_OFFSET_X, dst.y + SELECTED_SCENARIO_GENERAL_OFFSET_Y );
    fheroes2::Blit( _getMapTypeIcon( info._version ), display, dst.x + SELECTED_SCENARIO_MAP_TYPE_OFFSET_X, dst.y + SELECTED_SCENARIO_GENERAL_OFFSET_Y );

    fheroes2::Text mapNameText( info.name, fheroes2::FontType::normalWhite() );
    mapNameText.draw( GetCenteredTextXCoordinate( dst.x + SELECTED_SCENARIO_MAP_NAME_OFFSET_X, SELECTED_SCENARIO_MAP_NAME_WIDTH, mapNameText.width() ),
                      dst.y + SELECTED_SCENARIO_GENERAL_OFFSET_Y + 3, display );

    fheroes2::Blit( _getWinConditionsIcon( info.conditions_wins ), display, dst.x + SELECTED_SCENARIO_VICTORY_CONDITION_OFFSET_X,
                    dst.y + SELECTED_SCENARIO_GENERAL_OFFSET_Y );
    fheroes2::Blit( _getLossConditionsIcon( info.conditions_loss ), display, dst.x + SELECTED_SCENARIO_LOSS_CONDITION_OFFSET_X,
                    dst.y + SELECTED_SCENARIO_GENERAL_OFFSET_Y );

    const int32_t difficultyOffsetY = SELECTED_SCENARIO_DIFFICULTY_OFFSET_Y;

    fheroes2::Text difficultyLabelText( _( "Map difficulty:" ), fheroes2::FontType::normalWhite() );
    difficultyLabelText.draw( dst.x + 210 - difficultyLabelText.width(), dst.y + difficultyOffsetY, display );

    fheroes2::Text difficultyText( Difficulty::String( info.difficulty ), fheroes2::FontType::normalWhite() );
    difficultyText.draw( GetCenteredTextXCoordinate( dst.x + SELECTED_SCENARIO_DIFFICULTY_OFFSET_X, SELECTED_SCENARIO_DIFFICULTY_WIDTH, difficultyText.width() ),
                         dst.y + difficultyOffsetY, display );

    fheroes2::Text descriptionText( info.description, fheroes2::FontType::normalWhite() );
    descriptionText.draw( dst.x + SELECTED_SCENARIO_DESCRIPTION_OFFSET_X, dst.y + SELECTED_SCENARIO_DESCRIPTION_OFFSET_Y + 5, SELECTED_SCENARIO_DESCRIPTION_WIDTH - 2,
                          display );
}

void ScenarioListBox::_renderMapName( const Maps::FileInfo & info, bool selected, const int32_t & baseYOffset, fheroes2::Display & display ) const
{
    fheroes2::Text mapName( info.name, { fheroes2::FontSize::NORMAL, ( selected ? fheroes2::FontColor::YELLOW : fheroes2::FontColor::WHITE ) } );
    const int32_t xCoordinate = GetCenteredTextXCoordinate( _offsetX + SCENARIO_LIST_MAP_NAME_OFFSET_X, SCENARIO_LIST_MAP_NAME_WIDTH, mapName.width() );
    const int32_t yCoordinate = baseYOffset + MAP_LIST_ROW_SPACING_Y - 1;

    mapName.draw( xCoordinate, yCoordinate, display );
}

void ScenarioListBox::_renderMapIcon( const uint16_t size, fheroes2::Display & display, const int32_t coordX, const int32_t coordY )
{
    int16_t mapIconIndex = -1;

    switch ( size ) {
    case Maps::SMALL:
        mapIconIndex = 26;
        break;
    case Maps::MEDIUM:
        mapIconIndex = 27;
        break;
    case Maps::LARGE:
        mapIconIndex = 28;
        break;
    case Maps::XLARGE:
        mapIconIndex = 29;
        break;
    default:
        break;
    }

    if ( mapIconIndex == -1 ) {
        fheroes2::Image icon( 17, 17 );
        icon.reset();
        fheroes2::Fill( icon, 1, 1, 15, 15, fheroes2::GetColorId( 0x8D, 0x73, 0xFF ) );
        fheroes2::Text text( _( "N" ), fheroes2::FontType::smallWhite() );
        text.draw( ( 17 - text.width() ) / 2, ( 17 - text.height() ) / 2, icon );

        fheroes2::Blit( icon, display, coordX, coordY );
    }
    else {
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::REQUESTS, mapIconIndex ), display, coordX, coordY );
    }
}

const fheroes2::Sprite & ScenarioListBox::_getPlayersCountIcon( const uint8_t colors )
{
    const uint32_t iconIndex = 19 + Color::Count( colors );
    return fheroes2::AGG::GetICN( ICN::REQUESTS, iconIndex );
}

const fheroes2::Sprite & ScenarioListBox::_getMapTypeIcon( const GameVersion version )
{
    return fheroes2::AGG::GetICN( ICN::MAP_TYPE_ICON, version == GameVersion::PRICE_OF_LOYALTY ? 1 : 0 );
}

const fheroes2::Sprite & ScenarioListBox::_getWinConditionsIcon( const uint8_t condition )
{
    uint32_t iconIndex = 30 + condition;
    return fheroes2::AGG::GetICN( ICN::REQUESTS, iconIndex );
}

const fheroes2::Sprite & ScenarioListBox::_getLossConditionsIcon( const uint8_t condition )
{
    uint32_t iconIndex = 36 + condition;
    return fheroes2::AGG::GetICN( ICN::REQUESTS, iconIndex );
}

void ScenarioListBox::ActionListDoubleClick( Maps::FileInfo & )
{
    selectOk = true;
}

const Maps::FileInfo * Dialog::SelectScenario( const MapsFileInfoList & all )
{
    if ( all.empty() )
        return nullptr;

    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    MapsFileInfoList small;
    MapsFileInfoList medium;
    MapsFileInfoList large;
    MapsFileInfoList xlarge;

    small.reserve( all.size() );
    medium.reserve( all.size() );
    large.reserve( all.size() );
    xlarge.reserve( all.size() );

    for ( const Maps::FileInfo & info : all ) {
        switch ( info.size_w ) {
        case Maps::SMALL:
            small.push_back( info );
            break;
        case Maps::MEDIUM:
            medium.push_back( info );
            break;
        case Maps::LARGE:
            large.push_back( info );
            break;
        case Maps::XLARGE:
            xlarge.push_back( info );
            break;
        default:
            // Did you add a new map size? Add the corresponding logic!
            assert( 0 );
            continue;
        }
    }

    const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::REQSBKG, 0 );
    const fheroes2::Rect rt( ( display.width() - panel.width() ) / 2, ( display.height() - panel.height() ) / 2, panel.width(), panel.height() );

    fheroes2::ImageRestorer background( display, rt.x - SHADOWWIDTH, rt.y, rt.width + SHADOWWIDTH, rt.height + SHADOWWIDTH );

    const fheroes2::Sprite & shadow = fheroes2::AGG::GetICN( ICN::REQSBKG, 1 );
    fheroes2::Blit( shadow, display, rt.x - SHADOWWIDTH, rt.y + SHADOWWIDTH );

    const fheroes2::Rect countPlayers( rt.x + SCENARIO_LIST_COUNT_PLAYERS_OFFSET_X, rt.y + SCENARIO_LIST_ROW_OFFSET_Y, ICON_SIZE, SCENARIO_LIST_COLUMN_HEIGHT );
    const fheroes2::Rect sizeMaps( rt.x + SCENARIO_LIST_MAP_SIZE_OFFSET_X, rt.y + SCENARIO_LIST_ROW_OFFSET_Y, ICON_SIZE, SCENARIO_LIST_COLUMN_HEIGHT );
    const fheroes2::Rect mapTypes( rt.x + SCENARIO_LIST_MAP_TYPE_OFFSET_X, rt.y + SCENARIO_LIST_ROW_OFFSET_Y, ICON_SIZE, SCENARIO_LIST_COLUMN_HEIGHT );
    const fheroes2::Rect mapNames( rt.x + SCENARIO_LIST_MAP_NAME_OFFSET_X, rt.y + SCENARIO_LIST_ROW_OFFSET_Y, SCENARIO_LIST_MAP_NAME_WIDTH, SCENARIO_LIST_COLUMN_HEIGHT );
    const fheroes2::Rect victoryConds( rt.x + SCENARIO_LIST_VICTORY_CONDITION_OFFSET_X, rt.y + SCENARIO_LIST_ROW_OFFSET_Y, ICON_SIZE, SCENARIO_LIST_COLUMN_HEIGHT );
    const fheroes2::Rect lossConds( rt.x + SCENARIO_LIST_LOSS_CONDITION_OFFSET_X, rt.y + SCENARIO_LIST_ROW_OFFSET_Y, ICON_SIZE, SCENARIO_LIST_COLUMN_HEIGHT );

    const fheroes2::Rect curCountPlayer( rt.x + SELECTED_SCENARIO_COUNT_PLAYERS_OFFSET_X, rt.y + SELECTED_SCENARIO_GENERAL_OFFSET_Y, ICON_SIZE, ICON_SIZE );
    const fheroes2::Rect curMapSize( rt.x + SELECTED_SCENARIO_MAP_SIZE_OFFSET_X, rt.y + SELECTED_SCENARIO_GENERAL_OFFSET_Y, ICON_SIZE, ICON_SIZE );
    const fheroes2::Rect curMapType( rt.x + SELECTED_SCENARIO_MAP_TYPE_OFFSET_X, rt.y + SELECTED_SCENARIO_GENERAL_OFFSET_Y, ICON_SIZE, ICON_SIZE );
    const fheroes2::Rect curMapName( rt.x + SELECTED_SCENARIO_MAP_NAME_OFFSET_X, rt.y + SELECTED_SCENARIO_GENERAL_OFFSET_Y, SELECTED_SCENARIO_MAP_NAME_WIDTH, ICON_SIZE );
    const fheroes2::Rect curVictoryCond( rt.x + SELECTED_SCENARIO_VICTORY_CONDITION_OFFSET_X, rt.y + SELECTED_SCENARIO_GENERAL_OFFSET_Y, ICON_SIZE, ICON_SIZE );
    const fheroes2::Rect curLossCond( rt.x + SELECTED_SCENARIO_LOSS_CONDITION_OFFSET_X, rt.y + SELECTED_SCENARIO_GENERAL_OFFSET_Y, ICON_SIZE, ICON_SIZE );
    const fheroes2::Rect curDifficulty( rt.x + SELECTED_SCENARIO_DIFFICULTY_OFFSET_X, rt.y + SELECTED_SCENARIO_DIFFICULTY_OFFSET_Y, SELECTED_SCENARIO_DIFFICULTY_WIDTH,
                                        SELECTED_SCENARIO_DIFFICULTY_HEIGHT );
    const fheroes2::Rect curDescription( rt.x + SELECTED_SCENARIO_DESCRIPTION_OFFSET_X, rt.y + SELECTED_SCENARIO_DESCRIPTION_OFFSET_Y,
                                         SELECTED_SCENARIO_DESCRIPTION_WIDTH, SELECTED_SCENARIO_DESCRIPTION_HEIGHT );

    fheroes2::Button buttonOk( rt.x + 140, rt.y + 410, ICN::REQUESTS, 1, 2 );

    fheroes2::Button buttonSelectSmall( rt.x + 36, rt.y + MAP_SIZE_BUTTON_OFFSET_Y, ICN::REQUESTS, 9, 10 );
    fheroes2::Button buttonSelectMedium( rt.x + 98, rt.y + MAP_SIZE_BUTTON_OFFSET_Y, ICN::REQUESTS, 11, 12 );
    fheroes2::Button buttonSelectLarge( rt.x + 160, rt.y + MAP_SIZE_BUTTON_OFFSET_Y, ICN::REQUESTS, 13, 14 );
    fheroes2::Button buttonSelectXLarge( rt.x + 222, rt.y + MAP_SIZE_BUTTON_OFFSET_Y, ICN::REQUESTS, 15, 16 );
    fheroes2::Button buttonSelectAll( rt.x + 284, rt.y + MAP_SIZE_BUTTON_OFFSET_Y, ICN::REQUESTS, 17, 18 );

    fheroes2::ButtonBase * currentPressedButton = nullptr;

    // This variable is used to remember the selection of map size through the game.
    static int selectedMapSize = Maps::mapsize_t::ZERO;

    switch ( selectedMapSize ) {
    case Maps::SMALL:
        if ( !small.empty() ) {
            buttonSelectSmall.press();
            currentPressedButton = &buttonSelectSmall;
        }
        break;
    case Maps::MEDIUM:
        if ( !medium.empty() ) {
            buttonSelectMedium.press();
            currentPressedButton = &buttonSelectMedium;
        }
        break;
    case Maps::LARGE:
        if ( !large.empty() ) {
            buttonSelectLarge.press();
            currentPressedButton = &buttonSelectLarge;
        }
        break;
    case Maps::XLARGE:
        if ( !xlarge.empty() ) {
            buttonSelectXLarge.press();
            currentPressedButton = &buttonSelectXLarge;
        }
        break;
    default:
        break;
    }

    if ( currentPressedButton == nullptr ) {
        buttonSelectAll.press();
        currentPressedButton = &buttonSelectAll;
        selectedMapSize = Maps::mapsize_t::ZERO;
    }

    assert( currentPressedButton != nullptr );

    fheroes2::OptionButtonGroup buttonGroup;
    buttonGroup.addButton( &buttonSelectSmall );
    buttonGroup.addButton( &buttonSelectMedium );
    buttonGroup.addButton( &buttonSelectLarge );
    buttonGroup.addButton( &buttonSelectXLarge );
    buttonGroup.addButton( &buttonSelectAll );

    ScenarioListBox listbox( rt.getPosition() );

    listbox.RedrawBackground( rt.getPosition() );
    listbox.SetScrollButtonUp( ICN::REQUESTS, 5, 6, { rt.x + 327, rt.y + 55 } );
    listbox.SetScrollButtonDn( ICN::REQUESTS, 7, 8, { rt.x + 327, rt.y + 217 } );

    const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::ESCROLL, 3 );
    const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, 140, 9, static_cast<int32_t>( all.size() ),
                                                                               { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );

    listbox.setScrollBarArea( { rt.x + 328, rt.y + 73, 12, 140 } );

    listbox.setScrollBarImage( scrollbarSlider );
    listbox.SetAreaMaxItems( 9 );
    listbox.SetAreaItems( { rt.x + 55, rt.y + 55, 270, 171 } );

    switch ( selectedMapSize ) {
    case Maps::SMALL:
        listbox.SetListContent( small );
        listbox.SetCurrent( GetSelectedMapId( small ) );
        break;
    case Maps::MEDIUM:
        listbox.SetListContent( medium );
        listbox.SetCurrent( GetSelectedMapId( medium ) );
        break;
    case Maps::LARGE:
        listbox.SetListContent( large );
        listbox.SetCurrent( GetSelectedMapId( large ) );
        break;
    case Maps::XLARGE:
        listbox.SetListContent( xlarge );
        listbox.SetCurrent( GetSelectedMapId( xlarge ) );
        break;
    default:
        listbox.SetListContent( const_cast<MapsFileInfoList &>( all ) );
        listbox.SetCurrent( GetSelectedMapId( all ) );
        break;
    }

    listbox.Redraw();

    buttonOk.draw();
    buttonSelectSmall.draw();
    buttonSelectMedium.draw();
    buttonSelectLarge.draw();
    buttonSelectXLarge.draw();
    buttonSelectAll.draw();

    display.render();

    while ( le.HandleEvents() ) {
        if ( buttonOk.isEnabled() )
            le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();

        if ( le.MousePressLeft( buttonSelectSmall.area() ) )
            buttonSelectSmall.drawOnPress();
        if ( le.MousePressLeft( buttonSelectMedium.area() ) )
            buttonSelectMedium.drawOnPress();
        if ( le.MousePressLeft( buttonSelectLarge.area() ) )
            buttonSelectLarge.drawOnPress();
        if ( le.MousePressLeft( buttonSelectXLarge.area() ) )
            buttonSelectXLarge.drawOnPress();
        if ( le.MousePressLeft( buttonSelectAll.area() ) )
            buttonSelectAll.drawOnPress();

        listbox.QueueEventProcessing();

        bool needRedraw = false;

        if ( ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || listbox.selectOk ) {
            MapsFileInfoList::const_iterator it = std::find( all.begin(), all.end(), listbox.GetCurrent() );
            return ( it != all.end() ) ? &( *it ) : nullptr;
        }

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            return nullptr;
        }

        if ( le.MouseClickLeft( buttonSelectSmall.area() ) || HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_SMALL ) /*&& buttonSelectSmall.isEnabled()*/ ) {
            if ( small.empty() ) {
                fheroes2::showStandardTextMessage( "", _( "No maps exist at that size" ), Dialog::OK );
                currentPressedButton->drawOnPress();
            }
            else {
                const fheroes2::Image updatedScrollbarSlider
                    = fheroes2::generateScrollbarSlider( originalSlider, false, 140, 9, static_cast<int32_t>( small.size() ), { 0, 0, originalSlider.width(), 8 },
                                                         { 0, 7, originalSlider.width(), 8 } );
                listbox.setScrollBarImage( updatedScrollbarSlider );

                listbox.SetListContent( small );
                listbox.SetCurrent( GetSelectedMapId( small ) );

                currentPressedButton = &buttonSelectSmall;
                currentPressedButton->press();
                selectedMapSize = Maps::mapsize_t::SMALL;
            }

            needRedraw = true;
        }
        else if ( le.MouseClickLeft( buttonSelectMedium.area() )
                  || HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_MEDIUM ) /*&& buttonSelectMedium.isEnabled()*/ ) {
            if ( medium.empty() ) {
                fheroes2::showStandardTextMessage( "", _( "No maps exist at that size" ), Dialog::OK );
                currentPressedButton->drawOnPress();
            }
            else {
                const fheroes2::Image updatedScrollbarSlider
                    = fheroes2::generateScrollbarSlider( originalSlider, false, 140, 9, static_cast<int32_t>( medium.size() ), { 0, 0, originalSlider.width(), 8 },
                                                         { 0, 7, originalSlider.width(), 8 } );
                listbox.setScrollBarImage( updatedScrollbarSlider );

                listbox.SetListContent( medium );
                listbox.SetCurrent( GetSelectedMapId( medium ) );

                currentPressedButton = &buttonSelectMedium;
                currentPressedButton->press();
                selectedMapSize = Maps::mapsize_t::MEDIUM;
            }

            needRedraw = true;
        }
        else if ( le.MouseClickLeft( buttonSelectLarge.area() )
                  || HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_LARGE ) /*&& buttonSelectLarge.isEnabled()*/ ) {
            if ( large.empty() ) {
                fheroes2::showStandardTextMessage( "", _( "No maps exist at that size" ), Dialog::OK );
                currentPressedButton->drawOnPress();
            }
            else {
                const fheroes2::Image updatedScrollbarSlider
                    = fheroes2::generateScrollbarSlider( originalSlider, false, 140, 9, static_cast<int32_t>( large.size() ), { 0, 0, originalSlider.width(), 8 },
                                                         { 0, 7, originalSlider.width(), 8 } );
                listbox.setScrollBarImage( updatedScrollbarSlider );

                listbox.SetListContent( large );
                listbox.SetCurrent( GetSelectedMapId( large ) );

                currentPressedButton = &buttonSelectLarge;
                currentPressedButton->press();
                selectedMapSize = Maps::mapsize_t::LARGE;
            }

            needRedraw = true;
        }
        else if ( le.MouseClickLeft( buttonSelectXLarge.area() )
                  || HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_EXTRA_LARGE ) /*&& buttonSelectXLarge.isEnabled()*/ ) {
            if ( xlarge.empty() ) {
                fheroes2::showStandardTextMessage( "", _( "No maps exist at that size" ), Dialog::OK );
                currentPressedButton->drawOnPress();
            }
            else {
                const fheroes2::Image updatedScrollbarSlider
                    = fheroes2::generateScrollbarSlider( originalSlider, false, 140, 9, static_cast<int32_t>( xlarge.size() ), { 0, 0, originalSlider.width(), 8 },
                                                         { 0, 7, originalSlider.width(), 8 } );
                listbox.setScrollBarImage( updatedScrollbarSlider );

                listbox.SetListContent( xlarge );
                listbox.SetCurrent( GetSelectedMapId( xlarge ) );

                currentPressedButton = &buttonSelectXLarge;
                currentPressedButton->press();
                selectedMapSize = Maps::mapsize_t::XLARGE;
            }

            needRedraw = true;
        }
        else if ( le.MouseClickLeft( buttonSelectAll.area() ) || HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_ALL ) ) {
            const fheroes2::Image updatedScrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, 140, 9, static_cast<int32_t>( all.size() ),
                                                                                              { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );
            listbox.setScrollBarImage( updatedScrollbarSlider );

            listbox.SetListContent( const_cast<MapsFileInfoList &>( all ) );
            listbox.SetCurrent( GetSelectedMapId( all ) );

            currentPressedButton = &buttonSelectAll;
            currentPressedButton->press();
            selectedMapSize = Maps::mapsize_t::ZERO;

            needRedraw = true;
        }

        // right info
        if ( le.MousePressRight( buttonSelectSmall.area() ) )
            ShowToolTip( _( "Small Maps" ), _( "View only maps of size small (36 x 36)." ) );
        else if ( le.MousePressRight( buttonSelectMedium.area() ) )
            ShowToolTip( _( "Medium Maps" ), _( "View only maps of size medium (72 x 72)." ) );
        else if ( le.MousePressRight( buttonSelectLarge.area() ) )
            ShowToolTip( _( "Large Maps" ), _( "View only maps of size large (108 x 108)." ) );
        else if ( le.MousePressRight( buttonSelectXLarge.area() ) )
            ShowToolTip( _( "Extra Large Maps" ), _( "View only maps of size extra large (144 x 144)." ) );
        else if ( le.MousePressRight( buttonSelectAll.area() ) )
            ShowToolTip( _( "All Maps" ), _( "View all maps, regardless of size." ) );
        else if ( le.MousePressRight( countPlayers ) || le.MousePressRight( curCountPlayer ) )
            ShowToolTip( _( "Players Icon" ),
                         _( "Indicates how many players total are in the scenario. Any positions not occupied by humans will be occupied by computer players." ) );
        else if ( le.MousePressRight( sizeMaps ) || le.MousePressRight( curMapSize ) )
            ShowToolTip( _( "Size Icon" ), _( "Indicates whether the map\nis small (36 x 36), medium\n(72 x 72), large (108 x 108),\nor extra large (144 x 144)." ) );
        else if ( le.MousePressRight( mapTypes ) || le.MousePressRight( curMapType ) )
            ShowToolTip( _( "Map Type" ), _( "Indicates whether the map is made for \"The Succession Wars\" or \"The Price of Loyalty\" version of the game." ) );
        else if ( le.MousePressRight( mapNames ) ) {
            const Maps::FileInfo * item = listbox.GetFromPosition( le.GetMouseCursor() );
            if ( item )
                mapInfo( *item );
        }
        else if ( le.MousePressRight( curMapName ) )
            ShowToolTip( _( "Selected Name" ), _( "The name of the currently selected map." ) );
        else if ( le.MousePressRight( victoryConds ) ) {
            const Maps::FileInfo * item = listbox.GetFromPosition( le.GetMouseCursor() );
            if ( item )
                VictoryConditionInfo( *item );
        }
        else if ( le.MousePressRight( lossConds ) ) {
            const Maps::FileInfo * item = listbox.GetFromPosition( le.GetMouseCursor() );
            if ( item )
                LossConditionInfo( *item );
        }
        else if ( le.MousePressRight( curVictoryCond ) )
            VictoryConditionInfo( listbox.GetCurrent() );
        else if ( le.MousePressRight( curLossCond ) )
            LossConditionInfo( listbox.GetCurrent() );
        else if ( le.MousePressRight( curDifficulty ) )
            ShowToolTip(
                _( "Selected Map Difficulty" ),
                _( "The map difficulty of the currently selected map.  The map difficulty is determined by the scenario designer. More difficult maps might include more or stronger enemies, fewer resources, or other special conditions making things tougher for the human player." ) );
        else if ( le.MousePressRight( curDescription ) )
            ShowToolTip( _( "Selected Description" ), _( "The description of the currently selected map." ) );
        else if ( le.MousePressRight( buttonOk.area() ) )
            ShowToolTip( _( "Okay" ), _( "Accept the choice made." ) );

        if ( !needRedraw && !listbox.IsNeedRedraw() ) {
            continue;
        }

        listbox.Redraw();
        buttonOk.draw();
        buttonSelectSmall.draw();
        buttonSelectMedium.draw();
        buttonSelectLarge.draw();
        buttonSelectXLarge.draw();
        buttonSelectAll.draw();
        display.render();
    }

    return nullptr;
}
