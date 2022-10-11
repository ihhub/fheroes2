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
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

#include <cassert>

namespace
{
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
        Dialog::Message( _( "Loss Condition" ), msg, Font::BIG );
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
        Dialog::Message( _( "Victory Condition" ), msg, Font::BIG );
    }

    fheroes2::Image GetNonStandardSizeIcon()
    {
        fheroes2::Image icon( 17, 17 );
        icon.reset();
        fheroes2::Fill( icon, 1, 1, 15, 15, fheroes2::GetColorId( 0x8D, 0x73, 0xFF ) );
        Text text( "N", Font::SMALL );
        text.Blit( ( 17 - text.w() ) / 2, ( 17 - text.h() ) / 2, icon );
        return icon;
    }

    fheroes2::Image undefinedMapSizeIcon;

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

    int32_t GetCenteredTextXCoordinate( const int32_t startCoordX, const int32_t textBoxWidth, const int32_t textWidth )
    {
        const int32_t centerTransform = textBoxWidth > textWidth ? ( textBoxWidth - textWidth ) / 2 : 0;
        return startCoordX + centerTransform;
    }
}

void ScenarioListBox::RedrawItem( const Maps::FileInfo & info, int32_t /*dstx*/, int32_t dsty, bool current )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    dsty = dsty + _offsetY;

    _renderMapListItem( info, display, dsty, current );
}

void ScenarioListBox::_renderMapListItem( const Maps::FileInfo & info, fheroes2::Display & display, int32_t & dsty, bool current )
{
    fheroes2::Blit( _getPlayersCountIcon( info.kingdom_colors ), display, _listItemCoords.playersCountCoordX, dsty );
    fheroes2::Blit( _getMapSizeIcon( info.size_w ), display, _listItemCoords.mapSizeCoordX, dsty );
    fheroes2::Blit( _getMapTypeIcon( info._version ), display, _listItemCoords.mapTypeCoordX, dsty );
    _renderMapName( info, current, dsty, display );
    fheroes2::Blit( _getWinConditionsIcon( info.conditions_wins ), display, _listItemCoords.winConditionsCoordX, dsty );
    fheroes2::Blit( _getLossConditionsIcon( info.conditions_loss ), display, _listItemCoords.lossConditionsCoordX, dsty );
}

void ScenarioListBox::RedrawBackground( const fheroes2::Point & dst )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::REQSBKG, 0 ), display, dst.x, dst.y );

    if ( isSelected() ) {
        _renderSelectedMapInfo( display, dst );
    }
}

void ScenarioListBox::_renderSelectedMapInfo( fheroes2::Display & display, const fheroes2::Point & dst )
{
    Text text;
    const Maps::FileInfo & info = GetCurrent();

    fheroes2::Blit( _getPlayersCountIcon( info.kingdom_colors ), display, _selectedCoords.playersCountCoordX, dst.y + 265 );
    fheroes2::Blit( _getMapSizeIcon( info.size_w ), display, _selectedCoords.mapSizeCoordX, dst.y + 265 );
    fheroes2::Blit( _getMapTypeIcon( info._version ), display, _selectedCoords.mapTypeCoordX, dst.y + 265 );

    text.Set( info.name, Font::BIG );
    text.Blit( GetCenteredTextXCoordinate( _selectedCoords.mapNameCoordX, _selectedCoords.mapNameWidth, text.w() ), dst.y + 266 );

    fheroes2::Blit( _getWinConditionsIcon( info.conditions_wins ), display, _selectedCoords.winConditionsCoordX, dst.y + 265 );
    fheroes2::Blit( _getLossConditionsIcon( info.conditions_loss ), display, _selectedCoords.lossConditionsCoordX, dst.y + 265 );

    text.Set( _( "Map difficulty:" ), Font::BIG );
    text.Blit( dst.x + 210 - text.w(), dst.y + 291 );

    text.Set( Difficulty::String( info.difficulty ) );
    text.Blit( GetCenteredTextXCoordinate( _selectedCoords.difficultyCoordX, _selectedCoords.difficultyWidth, text.w() ), dst.y + 291 );

    TextBox box( info.description, Font::BIG, 290 );
    box.Blit( _selectedCoords.descriptionCoordX, dst.y + 321 );
}

void ScenarioListBox::_renderMapName( const Maps::FileInfo & info, bool selected, const int32_t & baseYOffset, fheroes2::Display & display ) const
{
    fheroes2::Text mapName( info.name, { fheroes2::FontSize::NORMAL, ( selected ? fheroes2::FontColor::YELLOW : fheroes2::FontColor::WHITE ) } );
    const int32_t xCoordinate = GetCenteredTextXCoordinate( _listItemCoords.mapNameCoordX, _listItemCoords.mapNameWidth, mapName.width() );
    const int32_t yCoordinate = baseYOffset + _offsetY - 1;

    mapName.draw( xCoordinate, yCoordinate, display );
}

const fheroes2::Sprite & ScenarioListBox::_getPlayersCountIcon( uint8_t colors )
{
    const int iconIndex = 19 + Color::Count( colors );
    return fheroes2::AGG::GetICN( ICN::REQUESTS, iconIndex );
}

const fheroes2::Sprite & ScenarioListBox::_getMapTypeIcon( GameVersion version )
{
    return fheroes2::AGG::GetICN( ICN::MAP_TYPE_ICON, version == GameVersion::PRICE_OF_LOYALTY ? 1 : 0 );
}

const fheroes2::Sprite & ScenarioListBox::_getWinConditionsIcon( uint8_t condition )
{
    int iconIndex = 30 + condition;
    return fheroes2::AGG::GetICN( ICN::REQUESTS, iconIndex );
}

const fheroes2::Sprite & ScenarioListBox::_getLossConditionsIcon( uint8_t condition )
{
    int iconIndex = 36 + condition;
    return fheroes2::AGG::GetICN( ICN::REQUESTS, iconIndex );
}

const fheroes2::Image & ScenarioListBox::_getMapSizeIcon( uint16_t size )
{
    short mapIconIndex;

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
        return undefinedMapSizeIcon;
    }

    return fheroes2::AGG::GetICN( ICN::REQUESTS, mapIconIndex );
}

void ScenarioListBox::ActionListDoubleClick( Maps::FileInfo & )
{
    selectOk = true;
}

const Maps::FileInfo * Dialog::SelectScenario( const MapsFileInfoList & all )
{
    if ( all.empty() )
        return nullptr;

    undefinedMapSizeIcon = GetNonStandardSizeIcon();
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

    const fheroes2::Rect countPlayers( rt.x + 48, rt.y + 57, 18, 175 );
    const fheroes2::Rect sizeMaps( rt.x + 67, rt.y + 57, 18, 175 );
    const fheroes2::Rect mapTypes( rt.x + 86, rt.y + 57, 18, 175 );
    const fheroes2::Rect mapNames( rt.x + 104, rt.y + 57, 160, 175 );
    const fheroes2::Rect victoryConds( rt.x + 266, rt.y + 57, 18, 175 );
    const fheroes2::Rect lossConds( rt.x + 285, rt.y + 57, 18, 175 );
    ScenarioListItemCoordinates listCoords( countPlayers.x, sizeMaps.x, mapTypes.x, mapNames.width, mapNames.x, victoryConds.x, lossConds.x );

    const fheroes2::Rect curCountPlayer( rt.x + 45, rt.y + 264, 18, 18 );
    const fheroes2::Rect curMapSize( rt.x + 64, rt.y + 264, 18, 18 );
    const fheroes2::Rect curMapType( rt.x + 83, rt.y + 264, 18, 18 );
    const fheroes2::Rect curMapName( rt.x + 107, rt.y + 264, 160, 18 );
    const fheroes2::Rect curVictoryCond( rt.x + 276, rt.y + 264, 18, 18 );
    const fheroes2::Rect curLossCond( rt.x + 294, rt.y + 264, 18, 18 );
    const fheroes2::Rect curDifficulty( rt.x + 220, rt.y + 292, 114, 20 );
    const fheroes2::Rect curDescription( rt.x + 42, rt.y + 316, 292, 90 );
    SelectedScenarioCoordinates selectedCoords( curCountPlayer.x, curMapSize.x, curMapType.x, curMapName.width, curMapName.x, curVictoryCond.x, curLossCond.x,
                                                curDifficulty.x, curDescription.x, curDifficulty.width );

    fheroes2::Button buttonOk( rt.x + 140, rt.y + 410, ICN::REQUESTS, 1, 2 );

    fheroes2::Button buttonSelectSmall( rt.x + 37, rt.y + 22, ICN::REQUESTS, 9, 10 );
    fheroes2::Button buttonSelectMedium( rt.x + 99, rt.y + 22, ICN::REQUESTS, 11, 12 );
    fheroes2::Button buttonSelectLarge( rt.x + 161, rt.y + 22, ICN::REQUESTS, 13, 14 );
    fheroes2::Button buttonSelectXLarge( rt.x + 223, rt.y + 22, ICN::REQUESTS, 15, 16 );
    fheroes2::Button buttonSelectAll( rt.x + 285, rt.y + 22, ICN::REQUESTS, 17, 18 );

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

    ScenarioListBox listbox( rt.getPosition(), listCoords, selectedCoords );

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
                Dialog::Message( "", _( "No maps exist at that size" ), Font::BIG, Dialog::OK );
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
                Dialog::Message( "", _( "No maps exist at that size" ), Font::BIG, Dialog::OK );
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
                Dialog::Message( "", _( "No maps exist at that size" ), Font::BIG, Dialog::OK );
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
                Dialog::Message( "", _( "No maps exist at that size" ), Font::BIG, Dialog::OK );
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
            Dialog::Message( _( "Small Maps" ), _( "View only maps of size small (36 x 36)." ), Font::BIG );
        else if ( le.MousePressRight( buttonSelectMedium.area() ) )
            Dialog::Message( _( "Medium Maps" ), _( "View only maps of size medium (72 x 72)." ), Font::BIG );
        else if ( le.MousePressRight( buttonSelectLarge.area() ) )
            Dialog::Message( _( "Large Maps" ), _( "View only maps of size large (108 x 108)." ), Font::BIG );
        else if ( le.MousePressRight( buttonSelectXLarge.area() ) )
            Dialog::Message( _( "Extra Large Maps" ), _( "View only maps of size extra large (144 x 144)." ), Font::BIG );
        else if ( le.MousePressRight( buttonSelectAll.area() ) )
            Dialog::Message( _( "All Maps" ), _( "View all maps, regardless of size." ), Font::BIG );
        else if ( le.MousePressRight( countPlayers ) || le.MousePressRight( curCountPlayer ) )
            Dialog::Message( _( "Players Icon" ),
                             _( "Indicates how many players total are in the scenario. Any positions not occupied by humans will be occupied by computer players." ),
                             Font::BIG );
        else if ( le.MousePressRight( sizeMaps ) || le.MousePressRight( curMapSize ) )
            Dialog::Message( _( "Size Icon" ), _( "Indicates whether the map\nis small (36 x 36), medium\n(72 x 72), large (108 x 108),\nor extra large (144 x 144)." ),
                             Font::BIG );
        else if ( le.MousePressRight( mapTypes ) || le.MousePressRight( curMapType ) )
            Dialog::Message( _( "Map Type" ), _( "Indicates whether the map is made for \"The Succession Wars\" or \"The Price of Loyalty\" version of the game." ),
                             Font::BIG );
        else if ( le.MousePressRight( mapNames ) ) {
            const Maps::FileInfo * item = listbox.GetFromPosition( le.GetMouseCursor() );
            if ( item )
                mapInfo( *item );
        }
        else if ( le.MousePressRight( curMapName ) )
            Dialog::Message( _( "Selected Name" ), _( "The name of the currently selected map." ), Font::BIG );
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
            Dialog::Message(
                _( "Selected Map Difficulty" ),
                _( "The map difficulty of the currently selected map.  The map difficulty is determined by the scenario designer. More difficult maps might include more or stronger enemies, fewer resources, or other special conditions making things tougher for the human player." ),
                Font::BIG );
        else if ( le.MousePressRight( curDescription ) )
            Dialog::Message( _( "Selected Description" ), _( "The description of the currently selected map." ), Font::BIG );
        else if ( le.MousePressRight( buttonOk.area() ) )
            Dialog::Message( _( "Okay" ), _( "Accept the choice made." ), Font::BIG );

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
