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

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "dir.h"
#include "game.h"
#include "icn.h"
#include "interface_list.h"
#include "maps_fileinfo.h"
#include "settings.h"
#include "system.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "world.h"

namespace
{
    size_t GetInsertPosition( const std::string & text, const int32_t cursorPosition, const int32_t startXPosition )
    {
        if ( text.empty() ) {
            // The text is empty, return start position.
            return 0;
        }

        if ( cursorPosition <= startXPosition ) {
            return 0;
        }

        int32_t positionOffset = 0;
        for ( size_t i = 0; i < text.size(); ++i ) {
            positionOffset += Text::getCharacterWidth( static_cast<uint8_t>( text[i] ), Font::BIG );
            if ( positionOffset + startXPosition > cursorPosition ) {
                return i;
            }
        }

        return text.size();
    }

    std::string ResizeToShortName( const std::string & str )
    {
        std::string res = System::GetBasename( str );
        size_t it = res.rfind( '.' );
        if ( std::string::npos != it )
            res.resize( it );
        return res;
    }
}

std::string SelectFileListSimple( const std::string &, const std::string &, const bool );
bool RedrawExtraInfo( const fheroes2::Point &, const std::string &, const std::string &, const fheroes2::Rect & );

class FileInfoListBox : public Interface::ListBox<Maps::FileInfo>
{
public:
    explicit FileInfoListBox( const fheroes2::Point & pt )
        : Interface::ListBox<Maps::FileInfo>( pt )
        , _isDoubleClicked( false )
    {}

    void RedrawItem( const Maps::FileInfo &, s32, s32, bool ) override;
    void RedrawBackground( const fheroes2::Point & ) override;

    void ActionCurrentUp() override;
    void ActionCurrentDn() override;
    void ActionListDoubleClick( Maps::FileInfo & ) override;
    void ActionListSingleClick( Maps::FileInfo & ) override;

    void ActionListPressRight( Maps::FileInfo & info ) override
    {
        // On some OS like Windows path contains '\' symbol. This symbol doesn't exist in the resources.
        // To avoid this we have to replace all '\' symbols by '/' symbols.
        std::string fullPath = info.file;
        StringReplace( fullPath, "\\", "/" );

        fheroes2::Text header( ResizeToShortName( info.file ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } );

        fheroes2::MultiFontText body;
        body.add( { _( "Map: " ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } } );
        body.add( { info.name, { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } } );
        body.add( { _( "\n\nLocation: " ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } } );
        body.add( { fullPath, { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } } );

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

void FileInfoListBox::RedrawItem( const Maps::FileInfo & info, s32 dstx, s32 dsty, bool current )
{
    char shortDate[20];
    char shortHours[20];
    char shortTime[20];
    time_t timeval = info.localtime;

    std::fill( shortDate, std::end( shortDate ), 0 );
    std::fill( shortHours, std::end( shortHours ), 0 );
    std::fill( shortTime, std::end( shortTime ), 0 );
    std::strftime( shortDate, ARRAY_COUNT( shortDate ) - 1, "%b %d,", std::localtime( &timeval ) );
    std::strftime( shortHours, ARRAY_COUNT( shortHours ) - 1, "%H", std::localtime( &timeval ) );
    std::strftime( shortTime, ARRAY_COUNT( shortTime ) - 1, ":%M", std::localtime( &timeval ) );
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

MapsFileInfoList GetSortedMapsFileInfoList( void )
{
    ListFiles list1;
    list1.ReadDir( Game::GetSaveDir(), Game::GetSaveFileExtension(), false );

    MapsFileInfoList list2( list1.size() );
    int ii = 0;
    for ( ListFiles::const_iterator itd = list1.begin(); itd != list1.end(); ++itd, ++ii )
        if ( !list2[ii].ReadSAV( *itd ) )
            --ii;
    if ( static_cast<size_t>( ii ) != list2.size() )
        list2.resize( ii );
    std::sort( list2.begin(), list2.end(), Maps::FileInfo::FileSorting );

    return list2;
}

std::string Dialog::SelectFileSave( void )
{
    const Settings & conf = Settings::Get();
    const std::string & name = conf.CurrentFileInfo().name;

    std::string base = !name.empty() ? name : "newgame";
    base.erase( std::find_if( base.begin(), base.end(), ::ispunct ), base.end() );
    std::replace_if( base.begin(), base.end(), ::isspace, '_' );
    std::ostringstream os;

    os << System::ConcatePath( Game::GetSaveDir(), base ) <<
        // add postfix:
        '_' << std::setw( 4 ) << std::setfill( '0' ) << world.CountDay() << Game::GetSaveFileExtension();
    std::string lastfile = os.str();
    return SelectFileListSimple( _( "File to Save:" ), lastfile, true );
}

std::string Dialog::SelectFileLoad( void )
{
    const std::string & lastfile = Game::GetLastSavename();
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

    fheroes2::ImageRestorer restorer( display, shadowOffset.x, shadowOffset.y, sprite.width() + BORDERWIDTH, sprite.height() + BORDERWIDTH );
    const fheroes2::Rect rt( dialogOffset.x, dialogOffset.y, sprite.width(), sprite.height() );

    fheroes2::Blit( spriteShadow, display, rt.x - BORDERWIDTH, rt.y + BORDERWIDTH );

    const fheroes2::Rect enter_field( rt.x + 42, rt.y + 286, 260, 16 );

    fheroes2::Button buttonOk( rt.x + 34, rt.y + 315, ICN::REQUEST, 1, 2 );
    fheroes2::Button buttonCancel( rt.x + 244, rt.y + 315, ICN::REQUEST, 3, 4 );

    MapsFileInfoList lists = GetSortedMapsFileInfoList();
    FileInfoListBox listbox( rt.getPosition() );

    listbox.RedrawBackground( rt.getPosition() );
    listbox.SetScrollButtonUp( ICN::REQUESTS, 5, 6, fheroes2::Point( rt.x + 327, rt.y + 55 ) );
    listbox.SetScrollButtonDn( ICN::REQUESTS, 7, 8, fheroes2::Point( rt.x + 327, rt.y + 257 ) );
    listbox.SetScrollBar( fheroes2::AGG::GetICN( ICN::ESCROLL, 3 ), fheroes2::Rect( rt.x + 328, rt.y + 73, 12, 180 ) );
    listbox.SetAreaMaxItems( 11 );
    listbox.SetAreaItems( fheroes2::Rect( rt.x + 40, rt.y + 55, 265, 215 ) );
    listbox.SetListContent( lists );

    std::string filename;
    size_t charInsertPos = 0;

    if ( !lastfile.empty() ) {
        filename = ResizeToShortName( lastfile );
        charInsertPos = filename.size();

        MapsFileInfoList::iterator it = lists.begin();
        for ( ; it != lists.end(); ++it )
            if ( ( *it ).file == lastfile )
                break;

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

    if ( !isEditing && lists.empty() )
        buttonOk.disable();

    if ( filename.empty() && listbox.isSelected() ) {
        filename = ResizeToShortName( listbox.GetCurrent().file );
        charInsertPos = filename.size();
    }

    listbox.Redraw();
    RedrawExtraInfo( rt.getPosition(), header, filename, enter_field );

    buttonOk.draw();
    buttonCancel.draw();

    display.render();
    le.OpenVirtualKeyboard();

    std::string result;
    bool is_limit = false;
    std::string lastSelectedSaveFileName;

    while ( le.HandleEvents() && result.empty() ) {
        le.MousePressLeft( buttonOk.area() ) && buttonOk.isEnabled() ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        listbox.QueueEventProcessing();

        bool needRedraw = false;
        bool isListboxSelected = listbox.isSelected();

        if ( ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) || listbox.isDoubleClicked() ) {
            if ( !filename.empty() )
                result = System::ConcatePath( Game::GetSaveDir(), filename + Game::GetSaveFileExtension() );
            else if ( isListboxSelected )
                result = listbox.GetCurrent().file;
        }
        else if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
            break;
        }
        else if ( le.MouseClickLeft( enter_field ) && isEditing ) {
            charInsertPos = GetInsertPosition( filename, le.GetMouseCursor().x, enter_field.x );
            if ( filename.empty() )
                buttonOk.disable();

            needRedraw = true;
        }
        else if ( isEditing && le.KeyPress() && ( !is_limit || KEY_BACKSPACE == le.KeyValue() || KEY_DELETE == le.KeyValue() ) ) {
            charInsertPos = InsertKeySym( filename, charInsertPos, le.KeyValue(), le.KeyMod() );
            if ( filename.empty() )
                buttonOk.disable();
            else
                buttonOk.enable();

            needRedraw = true;
            listbox.Unselect();
            isListboxSelected = false;
        }

        if ( le.MousePressRight( buttonCancel.area() ) ) {
            Dialog::Message( _( "Cancel" ), _( "Exit this menu without doing anything." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonOk.area() ) ) {
            if ( isEditing ) {
                Dialog::Message( _( "OK" ), _( "Click to save the current game." ), Font::BIG );
            }
            else {
                Dialog::Message( _( "OK" ), _( "Click to load a previously saved game." ), Font::BIG );
            }
        }

        if ( !isEditing && le.KeyPress( KEY_DELETE ) && isListboxSelected ) {
            std::string msg( _( "Are you sure you want to delete file:" ) );
            msg.append( "\n \n" );
            msg.append( System::GetBasename( listbox.GetCurrent().file ) );
            if ( Dialog::YES == Dialog::Message( _( "Warning!" ), msg, Font::BIG, Dialog::YES | Dialog::NO ) ) {
                System::Unlink( listbox.GetCurrent().file );
                listbox.RemoveSelected();
                if ( lists.empty() || filename.empty() )
                    buttonOk.disable();
                listbox.SetListContent( lists );
            }

            needRedraw = true;
        }

        if ( !needRedraw && !listbox.IsNeedRedraw() ) {
            continue;
        }

        listbox.Redraw();

        std::string selectedFileName = isListboxSelected ? ResizeToShortName( listbox.GetCurrent().file ) : "";
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

        is_limit = isEditing ? RedrawExtraInfo( rt.getPosition(), header, InsertString( filename, charInsertPos, "_" ), enter_field )
                             : RedrawExtraInfo( rt.getPosition(), header, filename, enter_field );

        buttonOk.draw();
        buttonCancel.draw();
        display.render();
    }

    le.CloseVirtualKeyboard();

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
