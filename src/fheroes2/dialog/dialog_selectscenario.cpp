/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "dialog_selectscenario.h"
#include "agg.h"
#include "cursor.h"
#include "dialog.h"
#include "difficulty.h"
#include "game.h"
#include "maps.h"
#include "settings.h"
#include "text.h"
#include "tools.h"
#include "ui_button.h"

void LossConditionInfo( const Maps::FileInfo & );
void VictoryConditionInfo( const Maps::FileInfo & );

fheroes2::Image GetNonStandardSizeIcon( void )
{
    fheroes2::Image icon( 17, 17 );
    icon.reset();
    fheroes2::Fill( icon, 1, 1, 15, 15, fheroes2::GetColorId( 0x8D, 0x73, 0xFF ) );
    Text text( "N", Font::SMALL );
    text.Blit( ( 17 - text.w() ) / 2, ( 17 - text.h() ) / 2, icon );
    return icon;
}

void ScenarioListBox::RedrawItem( const Maps::FileInfo & info, s32 dstx, s32 dsty, bool current )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    Text text;
    int index = 19 + Color::Count( info.kingdom_colors );

    dstx = dstx - 10;
    dsty = dsty + 2;

    const fheroes2::Sprite & spriteCount = fheroes2::AGG::GetICN( ICN::REQUESTS, index );
    fheroes2::Blit( spriteCount, display, dstx, dsty );

    if ( info.size_w != info.size_h || info.size_w < Maps::SMALL || info.size_w > Maps::XLARGE ) {
        fheroes2::Blit( GetNonStandardSizeIcon(), display, dstx + spriteCount.width() + 2, dsty );
    }
    else {
        switch ( info.size_w ) {
        case Maps::SMALL:
            index = 26;
            break;
        case Maps::MEDIUM:
            index = 27;
            break;
        case Maps::LARGE:
            index = 28;
            break;
        case Maps::XLARGE:
            index = 29;
            break;
        default:
            break;
        }

        const fheroes2::Sprite & spriteSize = fheroes2::AGG::GetICN( ICN::REQUESTS, index );
        fheroes2::Blit( spriteSize, display, dstx + spriteCount.width() + 2, dsty );
    }

    text.Set( info.name, ( current ? Font::YELLOW_BIG : Font::BIG ) );
    text.Blit( dstx + 54, dsty + 2 );

    index = 30 + info.conditions_wins;
    const fheroes2::Sprite & spriteWins = fheroes2::AGG::GetICN( ICN::REQUESTS, index );
    fheroes2::Blit( spriteWins, display, dstx + 224, dsty );

    index = 36 + info.conditions_loss;
    const fheroes2::Sprite & spriteLoss = fheroes2::AGG::GetICN( ICN::REQUESTS, index );
    fheroes2::Blit( spriteLoss, display, dstx + 224 + spriteWins.width() + 2, dsty );
}

void ScenarioListBox::ActionListDoubleClick( Maps::FileInfo & )
{
    selectOk = true;
}

void ScenarioListBox::RedrawBackground( const Point & dst )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::REQSBKG, 0 ), display, dst.x, dst.y );

    if ( isSelected() ) {
        Text text;
        const Maps::FileInfo & info = GetCurrent();
        int index = 19 + Color::Count( info.kingdom_colors );

        const fheroes2::Sprite & spriteCount = fheroes2::AGG::GetICN( ICN::REQUESTS, index );
        fheroes2::Blit( spriteCount, display, dst.x + 65, dst.y + 265 );

        switch ( info.size_w ) {
        case Maps::SMALL:
            index = 26;
            break;
        case Maps::MEDIUM:
            index = 27;
            break;
        case Maps::LARGE:
            index = 28;
            break;
        case Maps::XLARGE:
            index = 29;
            break;
        default:
            index = 30;
            break;
        }

        const fheroes2::Sprite & spriteSize = fheroes2::AGG::GetICN( ICN::REQUESTS, index );
        fheroes2::Blit( spriteSize, display, dst.x + 65 + spriteCount.width() + 2, dst.y + 265 );

        text.Set( info.name, Font::BIG );
        text.Blit( dst.x + 190 - text.w() / 2, dst.y + 265 );

        index = 30 + info.conditions_wins;
        const fheroes2::Sprite & spriteWins = fheroes2::AGG::GetICN( ICN::REQUESTS, index );
        fheroes2::Blit( spriteWins, display, dst.x + 275, dst.y + 265 );

        index = 36 + info.conditions_loss;
        const fheroes2::Sprite & spriteLoss = fheroes2::AGG::GetICN( ICN::REQUESTS, index );
        fheroes2::Blit( spriteLoss, display, dst.x + 275 + spriteWins.width() + 2, dst.y + 265 );

        text.Set( _( "Map difficulty:" ), Font::BIG );
        text.Blit( dst.x + 210 - text.w(), dst.y + 290 );

        text.Set( Difficulty::String( info.difficulty ) );
        text.Blit( dst.x + 275 - text.w() / 2, dst.y + 290 );

        TextBox box( info.description, Font::BIG, 290 );
        box.Blit( dst.x + 45, dst.y + 320 );
    }
}

const Maps::FileInfo * Dialog::SelectScenario( const MapsFileInfoList & all, size_t selectedId )
{
    if ( all.empty() )
        return NULL;

    if ( selectedId >= all.size() )
        selectedId = 0;

    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    const Maps::FileInfo * result = NULL;
    MapsFileInfoList small;
    MapsFileInfoList medium;
    MapsFileInfoList large;
    MapsFileInfoList xlarge;

    small.reserve( all.size() );
    medium.reserve( all.size() );
    large.reserve( all.size() );
    xlarge.reserve( all.size() );

    for ( MapsFileInfoList::const_iterator cur = all.begin(); cur != all.end(); ++cur ) {
        switch ( ( *cur ).size_w ) {
        case Maps::SMALL:
            small.push_back( *cur );
            break;
        case Maps::MEDIUM:
            medium.push_back( *cur );
            break;
        case Maps::LARGE:
            large.push_back( *cur );
            break;
        case Maps::XLARGE:
            xlarge.push_back( *cur );
            break;
        default:
            continue;
        }
    }

    const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::REQSBKG, 0 );
    Rect rt( ( display.width() - panel.width() ) / 2, ( display.height() - panel.height() ) / 2, panel.width(), panel.height() );

    fheroes2::ImageRestorer background( display, rt.x - SHADOWWIDTH, rt.y, rt.w + SHADOWWIDTH, rt.h + SHADOWWIDTH );

    const fheroes2::Sprite & shadow = fheroes2::AGG::GetICN( ICN::REQSBKG, 1 );
    fheroes2::Blit( shadow, display, rt.x - SHADOWWIDTH, rt.y + SHADOWWIDTH );

    const Rect countPlayers( rt.x + 45, rt.y + 55, 20, 175 );
    const Rect sizeMaps( rt.x + 62, rt.y + 55, 20, 175 );
    const Rect victoryConds( rt.x + 267, rt.y + 55, 20, 175 );
    const Rect lossConds( rt.x + 287, rt.y + 55, 20, 175 );

    const Rect curCountPlayer( rt.x + 66, rt.y + 264, 18, 18 );
    const Rect curMapSize( rt.x + 85, rt.y + 264, 18, 18 );
    const Rect curMapName( rt.x + 107, rt.y + 264, 166, 18 );
    const Rect curVictoryCond( rt.x + 277, rt.y + 264, 18, 18 );
    const Rect curLossCond( rt.x + 295, rt.y + 264, 18, 18 );
    const Rect curDifficulty( rt.x + 220, rt.y + 292, 114, 20 );
    const Rect curDescription( rt.x + 42, rt.y + 316, 292, 90 );

    fheroes2::Button buttonOk( rt.x + 140, rt.y + 410, ICN::REQUESTS, 1, 2 );

    fheroes2::Button buttonSelectSmall( rt.x + 37, rt.y + 22, ICN::REQUESTS, 9, 10 );
    fheroes2::Button buttonSelectMedium( rt.x + 99, rt.y + 22, ICN::REQUESTS, 11, 12 );
    fheroes2::Button buttonSelectLarge( rt.x + 161, rt.y + 22, ICN::REQUESTS, 13, 14 );
    fheroes2::Button buttonSelectXLarge( rt.x + 223, rt.y + 22, ICN::REQUESTS, 15, 16 );
    fheroes2::Button buttonSelectAll( rt.x + 285, rt.y + 22, ICN::REQUESTS, 17, 18 );

    buttonSelectAll.press();
    fheroes2::ButtonBase * currentPressedButton = &buttonSelectAll;

    fheroes2::OptionButtonGroup buttonGroup;
    buttonGroup.addButton( &buttonSelectSmall );
    buttonGroup.addButton( &buttonSelectMedium );
    buttonGroup.addButton( &buttonSelectLarge );
    buttonGroup.addButton( &buttonSelectXLarge );
    buttonGroup.addButton( &buttonSelectAll );

    ScenarioListBox listbox( rt );

    listbox.RedrawBackground( rt );
    listbox.SetScrollButtonUp( ICN::REQUESTS, 5, 6, fheroes2::Point( rt.x + 327, rt.y + 55 ) );
    listbox.SetScrollButtonDn( ICN::REQUESTS, 7, 8, fheroes2::Point( rt.x + 327, rt.y + 217 ) );

    listbox.SetScrollBar( fheroes2::AGG::GetICN( ICN::ESCROLL, 3 ), fheroes2::Rect( rt.x + 328, rt.y + 73, 12, 141 ) );
    listbox.SetAreaMaxItems( 9 );
    listbox.SetAreaItems( fheroes2::Rect( rt.x + 55, rt.y + 55, 270, 175 ) );
    listbox.SetListContent( const_cast<MapsFileInfoList &>( all ) );
    listbox.SetCurrent( selectedId );
    listbox.Redraw();

    buttonOk.draw();
    buttonSelectSmall.draw();
    buttonSelectMedium.draw();
    buttonSelectLarge.draw();
    buttonSelectXLarge.draw();
    buttonSelectAll.draw();

    cursor.Show();
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

        if ( ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) || listbox.selectOk ) {
            MapsFileInfoList::const_iterator it = std::find( all.begin(), all.end(), listbox.GetCurrent() );
            result = it != all.end() ? &( *it ) : NULL;
            break;
        }
        else if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
            result = NULL;
            break;
        }
        else if ( ( le.MouseClickLeft( buttonSelectSmall.area() ) || le.KeyPress( KEY_s ) ) /*&& buttonSelectSmall.isEnabled()*/ ) {
            if ( small.empty() ) {
                Dialog::Message( "", _( "No maps exist at that size" ), Font::BIG, Dialog::OK );
                currentPressedButton->drawOnPress();
            }
            else {
                listbox.SetListContent( small );
                currentPressedButton = &buttonSelectSmall;
            }
            cursor.Hide();
        }
        else if ( ( le.MouseClickLeft( buttonSelectMedium.area() ) || le.KeyPress( KEY_m ) ) /*&& buttonSelectMedium.isEnabled()*/ ) {
            if ( medium.empty() ) {
                Dialog::Message( "", _( "No maps exist at that size" ), Font::BIG, Dialog::OK );
                currentPressedButton->drawOnPress();
            }
            else {
                listbox.SetListContent( medium );
                currentPressedButton = &buttonSelectMedium;
            }
            cursor.Hide();
        }
        else if ( ( le.MouseClickLeft( buttonSelectLarge.area() ) || le.KeyPress( KEY_l ) ) /*&& buttonSelectLarge.isEnabled()*/ ) {
            if ( large.empty() ) {
                Dialog::Message( "", _( "No maps exist at that size" ), Font::BIG, Dialog::OK );
                currentPressedButton->drawOnPress();
            }
            else {
                listbox.SetListContent( large );
                currentPressedButton = &buttonSelectLarge;
            }
            cursor.Hide();
        }
        else if ( ( le.MouseClickLeft( buttonSelectXLarge.area() ) || le.KeyPress( KEY_x ) ) /*&& buttonSelectXLarge.isEnabled()*/ ) {
            if ( xlarge.empty() ) {
                Dialog::Message( "", _( "No maps exist at that size" ), Font::BIG, Dialog::OK );
                currentPressedButton->drawOnPress();
            }
            else {
                listbox.SetListContent( xlarge );
                currentPressedButton = &buttonSelectXLarge;
            }
            cursor.Hide();
        }
        else if ( le.MouseClickLeft( buttonSelectAll.area() ) || le.KeyPress( KEY_a ) ) {
            listbox.SetListContent( const_cast<MapsFileInfoList &>( all ) );
            currentPressedButton = &buttonSelectAll;
            cursor.Hide();
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
            Dialog::Message( _( "Size Icon" ), _( "Indicates whether the maps is small (36 x 36), medium (72 x 72), large (108 x 108), or extra large (144 x 144)." ),
                             Font::BIG );
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

        if ( !cursor.isVisible() ) {
            listbox.Redraw();
            buttonOk.draw();
            buttonSelectSmall.draw();
            buttonSelectMedium.draw();
            buttonSelectLarge.draw();
            buttonSelectXLarge.draw();
            buttonSelectAll.draw();
            cursor.Show();
            display.render();
        }
    }

    cursor.Hide();

    return result;
}

void LossConditionInfo( const Maps::FileInfo & info )
{
    std::string msg;

    switch ( info.conditions_loss ) {
    case 0:
        msg = _( "Lose all your heroes and towns." );
        break;
    case 1:
        msg = _( "Lose a specific town." );
        break;
    case 2:
        msg = _( "Lose a specific hero." );
        break;
    case 3:
        msg = _( "Run out of time. Fail to win by a certain point." );
        break;
    default:
        return;
    }
    Dialog::Message( _( "Loss Condition" ), msg, Font::BIG );
}

void VictoryConditionInfo( const Maps::FileInfo & info )
{
    std::string msg;

    switch ( info.conditions_wins ) {
    case 0:
        msg = _( "Defeat all enemy heroes and towns." );
        break;
    case 1:
        msg = _( "Capture a specific town." );
        break;
    case 2:
        msg = _( "Defeat a specific hero." );
        break;
    case 3:
        msg = _( "Find a specific artifact." );
        break;
    case 4:
        msg = _( "Your side defeats the opposing side." );
        break;
    case 5:
        msg = _( "Accumulate a large amount of gold." );
        break;
    default:
        return;
    }
    Dialog::Message( _( "Victory Condition" ), msg, Font::BIG );
}
