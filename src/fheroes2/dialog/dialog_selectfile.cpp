/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "dir.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_io.h"
#include "gamedefs.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "maps_fileinfo.h"
#include "math_base.h"
#include "screen.h"
#include "system.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_keyboard.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "world.h"

namespace
{
    std::string ResizeToShortName( const std::string & str )
    {
        std::string res = System::GetBasename( str );
        const size_t it = res.rfind( '.' );
        if ( std::string::npos != it ) {
            res.resize( it );
        }
        return res;
    }
}

std::string SelectFileListSimple( const std::string &, const std::string &, const bool );
bool RedrawExtraInfo( const fheroes2::Point &, const std::string &, const std::string &, const fheroes2::Rect & );

class FileInfoListBox : public Interface::ListBox<Maps::FileInfo>
{
public:
    using Interface::ListBox<Maps::FileInfo>::ActionListDoubleClick;
    using Interface::ListBox<Maps::FileInfo>::ActionListSingleClick;
    using Interface::ListBox<Maps::FileInfo>::ActionListPressRight;

    explicit FileInfoListBox( const fheroes2::Point & pt )
        : Interface::ListBox<Maps::FileInfo>( pt )
        , _isDoubleClicked( false )
    {}

    void RedrawItem( const Maps::FileInfo & info, int32_t dstx, int32_t dsty, bool current ) override;
    void RedrawBackground( const fheroes2::Point & ) override;

    void ActionCurrentUp() override;
    void ActionCurrentDn() override;
    void ActionListDoubleClick( Maps::FileInfo & ) override;
    void ActionListSingleClick( Maps::FileInfo & ) override;

    void ActionListPressRight( Maps::FileInfo & info ) override
    {
        // On some OSes like Windows, the path may contain '\' symbols. This symbol doesn't exist in the resources.
        // To avoid this we have to replace all '\' symbols by '/' symbols.
        std::string fullPath = info.file;
        StringReplace( fullPath, "\\", "/" );

        const fheroes2::Text header( ResizeToShortName( info.file ), fheroes2::FontType::normalYellow() );

        fheroes2::MultiFontText body;

        body.add( { _( "Map: " ), fheroes2::FontType::normalYellow() } );
        body.add( { info.name, fheroes2::FontType::normalWhite() } );

        if ( info.worldDay > 0 || info.worldWeek > 0 || info.worldMonth > 0 ) {
            body.add( { _( "\n\nMonth: " ), fheroes2::FontType::normalYellow() } );
            body.add( { std::to_string( info.worldMonth ), fheroes2::FontType::normalWhite() } );
            body.add( { _( ", Week: " ), fheroes2::FontType::normalYellow() } );
            body.add( { std::to_string( info.worldWeek ), fheroes2::FontType::normalWhite() } );
            body.add( { _( ", Day: " ), fheroes2::FontType::normalYellow() } );
            body.add( { std::to_string( info.worldDay ), fheroes2::FontType::normalWhite() } );
        }

        body.add( { _( "\n\nLocation: " ), fheroes2::FontType::smallYellow() } );
        body.add( { fullPath, fheroes2::FontType::smallWhite() } );

        fheroes2::showMessage( header, body, Dialog::ZERO );
    }

    bool isDoubleClicked() const
    {
        return _isDoubleClicked;
    }

private:
    bool _isDoubleClicked;
};

#define ARRAY_COUNT( A ) sizeof( A ) / sizeof( A[0] )

void FileInfoListBox::RedrawItem( const Maps::FileInfo & info, int32_t dstx, int32_t dsty, bool current )
{
    char shortDate[20];
    char shortHours[20];
    char shortTime[20];

    const tm tmi = System::GetTM( info.timestamp );

    std::fill( shortDate, std::end( shortDate ), static_cast<char>( 0 ) );
    std::fill( shortHours, std::end( shortHours ), static_cast<char>( 0 ) );
    std::fill( shortTime, std::end( shortTime ), static_cast<char>( 0 ) );
    std::strftime( shortDate, ARRAY_COUNT( shortDate ) - 1, "%b %d,", &tmi );
    std::strftime( shortHours, ARRAY_COUNT( shortHours ) - 1, "%H", &tmi );
    std::strftime( shortTime, ARRAY_COUNT( shortTime ) - 1, ":%M", &tmi );
    std::string savname( System::GetBasename( info.file ) );

    if ( !savname.empty() ) {
        Text text;

        const std::string saveExtension = Game::GetSaveFileExtension();
        const size_t dotPos = savname.size() - saveExtension.size();

        if ( StringLower( savname.substr( dotPos ) ) == saveExtension )
            savname.erase( dotPos );

        text.Set( savname, ( current ? Font::YELLOW_BIG : Font::BIG ) );
        text.Blit( dstx + 5, dsty, 150 );

        text.Set( shortDate, ( current ? Font::YELLOW_BIG : Font::BIG ) );
        text.Blit( dstx + 225 - text.w(), dsty );

        text.Set( shortHours, ( current ? Font::YELLOW_BIG : Font::BIG ) );
        text.Blit( dstx + 245 - text.w(), dsty );

        text.Set( shortTime, ( current ? Font::YELLOW_BIG : Font::BIG ) );
        text.Blit( dstx + 245, dsty );
    }
}

void FileInfoListBox::RedrawBackground( const fheroes2::Point & dst )
{
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::REQBKG, 0 ), fheroes2::Display::instance(), dst.x, dst.y );
}

void FileInfoListBox::ActionCurrentUp()
{
    // Do nothing.
}

void FileInfoListBox::ActionCurrentDn()
{
    // Do nothing.
}

void FileInfoListBox::ActionListDoubleClick( Maps::FileInfo & )
{
    _isDoubleClicked = true;
}

void FileInfoListBox::ActionListSingleClick( Maps::FileInfo & /*unused*/ )
{
    // Do nothing.
}

MapsFileInfoList GetSortedMapsFileInfoList()
{
    ListFiles list1;
    list1.ReadDir( Game::GetSaveDir(), Game::GetSaveFileExtension(), false );

    MapsFileInfoList list2( list1.size() );
    int32_t saveFileCount = 0;
    for ( const std::string & saveFile : list1 ) {
        if ( list2[saveFileCount].ReadSAV( saveFile ) ) {
            ++saveFileCount;
        }
    }
    if ( static_cast<size_t>( saveFileCount ) != list2.size() ) {
        list2.resize( saveFileCount );
    }
    std::sort( list2.begin(), list2.end(), Maps::FileInfo::FileSorting );

    return list2;
}

std::string Dialog::SelectFileSave()
{
    std::ostringstream os;

    os << System::concatPath( Game::GetSaveDir(), Game::GetSaveFileBaseName() ) << '_' << std::setw( 4 ) << std::setfill( '0' ) << world.CountDay()
       << Game::GetSaveFileExtension();

    return SelectFileListSimple( _( "File to Save:" ), os.str(), true );
}

std::string Dialog::SelectFileLoad()
{
    const std::string & lastfile = Game::GetLastSaveName();
    return SelectFileListSimple( _( "File to Load:" ), ( !lastfile.empty() ? lastfile : "" ), false );
}

std::string SelectFileListSimple( const std::string & header, const std::string & lastfile, const bool isEditing )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
    const fheroes2::Sprite & spriteShadow = fheroes2::AGG::GetICN( ICN::REQBKG, 1 );

    const fheroes2::Point dialogOffset( ( display.width() - sprite.width() ) / 2, ( display.height() - sprite.height() ) / 2 );
    const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

    const fheroes2::ImageRestorer restorer( display, shadowOffset.x, shadowOffset.y, sprite.width() + BORDERWIDTH, sprite.height() + BORDERWIDTH );
    const fheroes2::Rect rt( dialogOffset.x, dialogOffset.y, sprite.width(), sprite.height() );

    fheroes2::Blit( spriteShadow, display, rt.x - BORDERWIDTH, rt.y + BORDERWIDTH );

    const fheroes2::Rect enter_field( rt.x + 42, rt.y + 286, 260, 16 );

    fheroes2::Button buttonOk( rt.x + 34, rt.y + 315, ICN::BUTTON_SMALL_OKAY_GOOD, 0, 1 );
    fheroes2::Button buttonCancel( rt.x + 244, rt.y + 315, ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1 );

    fheroes2::ButtonSprite buttonVirtualKB;

    MapsFileInfoList lists = GetSortedMapsFileInfoList();
    FileInfoListBox listbox( rt.getPosition() );

    listbox.RedrawBackground( rt.getPosition() );
    listbox.SetScrollButtonUp( ICN::REQUESTS, 5, 6, { rt.x + 327, rt.y + 55 } );
    listbox.SetScrollButtonDn( ICN::REQUESTS, 7, 8, { rt.x + 327, rt.y + 257 } );

    const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::ESCROLL, 3 );
    const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, 180, 11, static_cast<int32_t>( lists.size() ),
                                                                               { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );

    listbox.setScrollBarArea( { rt.x + 328, rt.y + 73, 12, 180 } );
    listbox.setScrollBarImage( scrollbarSlider );
    listbox.SetAreaMaxItems( 11 );
    listbox.SetAreaItems( { rt.x + 40, rt.y + 55, 265, 215 } );
    listbox.SetListContent( lists );

    std::string filename;
    size_t charInsertPos = 0;

    if ( !lastfile.empty() ) {
        filename = ResizeToShortName( lastfile );
        charInsertPos = filename.size();

        MapsFileInfoList::iterator it = lists.begin();
        for ( ; it != lists.end(); ++it ) {
            if ( ( *it ).file == lastfile ) {
                break;
            }
        }

        if ( it != lists.end() ) {
            listbox.SetCurrent( std::distance( lists.begin(), it ) );
        }
        else {
            if ( !isEditing ) {
                filename.clear();
                charInsertPos = 0;
            }
            listbox.Unselect();
        }
    }

    if ( !isEditing && lists.empty() ) {
        buttonOk.disable();
    }

    if ( filename.empty() && listbox.isSelected() ) {
        filename = ResizeToShortName( listbox.GetCurrent().file );
        charInsertPos = filename.size();
    }

    listbox.Redraw();
    RedrawExtraInfo( rt.getPosition(), header, filename, enter_field );

    buttonOk.draw();
    buttonCancel.draw();

    if ( isEditing ) {
        // Generate and render a button to open the Virtual Keyboard window.
        fheroes2::Sprite released;
        fheroes2::Sprite pressed;

        makeButtonSprites( released, pressed, "...", 15, false, true );
        buttonVirtualKB = makeButtonWithShadow( rt.x + 315, rt.y + 283, released, pressed, display, { -4, 4 } );

        buttonVirtualKB.draw();

        Game::passAnimationDelay( Game::DelayType::CURSOR_BLINK_DELAY );
    }

    display.render();

    std::string result;
    bool is_limit = false;
    std::string lastSelectedSaveFileName;

    const bool isInGameKeyboardRequired = System::isVirtualKeyboardSupported();

    bool isCursorVisible = true;

    while ( le.HandleEvents( !isEditing || Game::isDelayNeeded( { Game::DelayType::CURSOR_BLINK_DELAY } ) ) && result.empty() ) {
        le.MousePressLeft( buttonOk.area() ) && buttonOk.isEnabled() ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();
        if ( isEditing ) {
            le.MousePressLeft( buttonVirtualKB.area() ) ? buttonVirtualKB.drawOnPress() : buttonVirtualKB.drawOnRelease();
        }

        const bool listboxEvent = listbox.QueueEventProcessing();

        bool isListboxSelected = listbox.isSelected();

        bool needRedraw = false;

        if ( ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY )
             || listbox.isDoubleClicked() ) {
            if ( !filename.empty() ) {
                result = System::concatPath( Game::GetSaveDir(), filename + Game::GetSaveFileExtension() );
            }
            else if ( isListboxSelected ) {
                result = listbox.GetCurrent().file;
            }
        }
        else if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            break;
        }
        else if ( isEditing ) {
            if ( le.MouseClickLeft( buttonVirtualKB.area() ) || ( isInGameKeyboardRequired && le.MouseClickLeft( enter_field ) ) ) {
                fheroes2::openVirtualKeyboard( filename );
                charInsertPos = filename.size();
                listbox.Unselect();
                isListboxSelected = false;
                needRedraw = true;
            }
            else if ( le.MouseClickLeft( enter_field ) ) {
                charInsertPos = fheroes2::getTextInputCursorPosition( filename, fheroes2::FontType::normalWhite(), charInsertPos, le.GetMouseCursor().x, enter_field.x );
                if ( filename.empty() ) {
                    buttonOk.disable();
                }

                needRedraw = true;
            }
            else if ( !listboxEvent && le.KeyPress() && ( !is_limit || fheroes2::Key::KEY_BACKSPACE == le.KeyValue() || fheroes2::Key::KEY_DELETE == le.KeyValue() ) ) {
                charInsertPos = InsertKeySym( filename, charInsertPos, le.KeyValue(), LocalEvent::getCurrentKeyModifiers() );
                if ( filename.empty() ) {
                    buttonOk.disable();
                }
                else {
                    buttonOk.enable();
                }

                needRedraw = true;
                listbox.Unselect();
                isListboxSelected = false;
            }
        }

        if ( le.MousePressRight( buttonCancel.area() ) ) {
            Dialog::Message( _( "Cancel" ), _( "Exit this menu without doing anything." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonOk.area() ) ) {
            if ( isEditing ) {
                Dialog::Message( _( "Okay" ), _( "Click to save the current game." ), Font::BIG );
            }
            else {
                Dialog::Message( _( "Okay" ), _( "Click to load a previously saved game." ), Font::BIG );
            }
        }
        else if ( isEditing && le.MousePressRight( buttonVirtualKB.area() ) ) {
            Dialog::Message( _( "Open Virtual Keyboard" ), _( "Click to open the Virtual Keyboard dialog." ), Font::BIG );
        }

        if ( !isEditing && le.KeyPress( fheroes2::Key::KEY_DELETE ) && isListboxSelected ) {
            std::string msg( _( "Are you sure you want to delete file:" ) );
            msg.append( "\n \n" );
            msg.append( System::GetBasename( listbox.GetCurrent().file ) );
            if ( Dialog::YES == Dialog::Message( _( "Warning!" ), msg, Font::BIG, Dialog::YES | Dialog::NO ) ) {
                System::Unlink( listbox.GetCurrent().file );
                listbox.RemoveSelected();
                if ( lists.empty() || filename.empty() ) {
                    buttonOk.disable();
                    isListboxSelected = false;
                    filename.clear();
                }

                const fheroes2::Image updatedScrollbarSlider
                    = fheroes2::generateScrollbarSlider( originalSlider, false, 180, 11, static_cast<int32_t>( lists.size() ), { 0, 0, originalSlider.width(), 8 },
                                                         { 0, 7, originalSlider.width(), 8 } );

                listbox.setScrollBarImage( updatedScrollbarSlider );

                listbox.SetListContent( lists );
            }

            needRedraw = true;
        }

        // Text input cursor blink.
        if ( isEditing && Game::validateAnimationDelay( Game::DelayType::CURSOR_BLINK_DELAY ) ) {
            isCursorVisible = !isCursorVisible;
            needRedraw = true;
        }

        if ( !needRedraw && !listbox.IsNeedRedraw() ) {
            continue;
        }

        listbox.Redraw();

        const std::string selectedFileName = isListboxSelected ? ResizeToShortName( listbox.GetCurrent().file ) : "";
        if ( isListboxSelected && lastSelectedSaveFileName != selectedFileName ) {
            lastSelectedSaveFileName = selectedFileName;
            filename = selectedFileName;
            charInsertPos = filename.size();
        }
        else if ( isEditing ) {
            // Empty last selected save file name so that we can replace the input field's name if we select the same save file again
            // but when loading (isEditing == false), this doesn't matter since we cannot write to the input field
            lastSelectedSaveFileName = "";
        }

        is_limit = isEditing ? RedrawExtraInfo( rt.getPosition(), header, insertCharToString( filename, charInsertPos, isCursorVisible ? '_' : '\x7F' ), enter_field )
                             : RedrawExtraInfo( rt.getPosition(), header, filename, enter_field );

        buttonOk.draw();
        buttonCancel.draw();

        if ( isEditing ) {
            buttonVirtualKB.draw();
        }

        display.render();
    }

    return result;
}

bool RedrawExtraInfo( const fheroes2::Point & dst, const std::string & header, const std::string & filename, const fheroes2::Rect & field )
{
    Text text( header, Font::BIG );
    text.Blit( dst.x + 175 - text.w() / 2, dst.y + 30 );

    if ( !filename.empty() ) {
        text.Set( filename, Font::BIG );
        text.Blit( field.x, field.y + 1, field.width );
    }

    return text.w() + 10 > field.width;
}
