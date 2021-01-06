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
#include <iterator>
#include <sstream>
#include <string>

#include "agg.h"
#include "cursor.h"
#include "dialog.h"
#include "dir.h"
#include "game.h"
#include "interface_list.h"
#include "maps_fileinfo.h"
#include "pocketpc.h"
#include "settings.h"
#include "system.h"
#include "text.h"
#include "ui_button.h"
#include "world.h"

std::string SelectFileListSimple( const std::string &, const std::string &, const bool );
bool RedrawExtraInfo( const fheroes2::Point &, const std::string &, const std::string &, const fheroes2::Rect & );

class FileInfoListBox : public Interface::ListBox<Maps::FileInfo>
{
public:
    FileInfoListBox( const Point & pt, bool & edit )
        : Interface::ListBox<Maps::FileInfo>( pt )
        , edit_mode( edit )
        , _isDoubleClicked( false ){};

    void RedrawItem( const Maps::FileInfo &, s32, s32, bool );
    void RedrawBackground( const Point & );

    void ActionCurrentUp( void );
    void ActionCurrentDn( void );
    void ActionListDoubleClick( Maps::FileInfo & );
    void ActionListSingleClick( Maps::FileInfo & );
    void ActionListPressRight( Maps::FileInfo & ){};

    bool & edit_mode;

    bool isDoubleClicked() const
    {
        return _isDoubleClicked;
    }

private:
    bool _isDoubleClicked;
};

void FileInfoListBox::RedrawItem( const Maps::FileInfo & info, s32 dstx, s32 dsty, bool current )
{
    char short_date[20];
    time_t timeval = info.localtime;

    std::fill( short_date, ARRAY_COUNT_END( short_date ), 0 );
    std::strftime( short_date, ARRAY_COUNT( short_date ) - 1, "%b %d, %H:%M", std::localtime( &timeval ) );
    std::string savname( System::GetBasename( info.file ) );

    if ( savname.size() ) {
        Text text;

        const std::string saveExtension = Game::GetSaveFileExtension();
        const size_t dotPos = savname.size() - saveExtension.size();

        if ( StringLower( savname.substr( dotPos ) ) == saveExtension )
            savname.erase( dotPos );

        text.Set( savname, ( current ? Font::YELLOW_BIG : Font::BIG ) );
        text.Blit( dstx + 5, dsty, 155 );

        text.Set( short_date, ( current ? Font::YELLOW_BIG : Font::BIG ) );
        text.Blit( dstx + 265 - text.w(), dsty );
    }
}

void FileInfoListBox::RedrawBackground( const Point & dst )
{
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::REQBKG, 0 ), fheroes2::Display::instance(), dst.x, dst.y );
}

void FileInfoListBox::ActionCurrentUp( void )
{
    edit_mode = false;
}

void FileInfoListBox::ActionCurrentDn( void )
{
    edit_mode = false;
}

void FileInfoListBox::ActionListDoubleClick( Maps::FileInfo & )
{
    edit_mode = false;
    _isDoubleClicked = true;
}

void FileInfoListBox::ActionListSingleClick( Maps::FileInfo & )
{
    edit_mode = false;
}

std::string ResizeToShortName( const std::string & str )
{
    std::string res = System::GetBasename( str );
    size_t it = res.find( '.' );
    if ( std::string::npos != it )
        res.resize( it );
    return res;
}

size_t GetInsertPosition( const std::string & name, s32 cx, s32 posx )
{
    if ( name.size() ) {
        s32 tw = Text::width( name, Font::SMALL );
        if ( cx <= posx )
            return 0;
        else if ( cx >= posx + tw )
            return name.size();
        else {
            float cw = tw / name.size();
            return static_cast<size_t>( ( cx - posx ) / cw );
        }
    }
    return 0;
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

    std::string base = name.size() ? name : "newgame";
    base.resize( std::distance( base.begin(), std::find_if( base.begin(), base.end(), ::ispunct ) ) );
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
    return SelectFileListSimple( _( "File to Load:" ), ( lastfile.size() ? lastfile : "" ), false );
}

std::string SelectFileListSimple( const std::string & header, const std::string & lastfile, const bool editor )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

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

    bool edit_mode = false;

    MapsFileInfoList lists = GetSortedMapsFileInfoList();
    FileInfoListBox listbox( Point( rt.x, rt.y ), edit_mode );

    listbox.RedrawBackground( Point( rt.x, rt.y ) );
    listbox.SetScrollButtonUp( ICN::REQUESTS, 5, 6, fheroes2::Point( rt.x + 327, rt.y + 55 ) );
    listbox.SetScrollButtonDn( ICN::REQUESTS, 7, 8, fheroes2::Point( rt.x + 327, rt.y + 257 ) );
    listbox.SetScrollBar( fheroes2::AGG::GetICN( ICN::ESCROLL, 3 ), fheroes2::Rect( rt.x + 328, rt.y + 73, 12, 180 ) );
    listbox.SetAreaMaxItems( 11 );
    listbox.SetAreaItems( fheroes2::Rect( rt.x + 40, rt.y + 55, 265, 215 ) );
    listbox.SetListContent( lists );

    std::string filename;
    size_t charInsertPos = 0;

    if ( lastfile.size() ) {
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
            if ( !editor ) {
                filename.clear();
                charInsertPos = 0;
            }
            listbox.Unselect();
        }
    }

    if ( !editor && lists.empty() )
        buttonOk.disable();

    if ( filename.empty() && listbox.isSelected() ) {
        filename = ResizeToShortName( listbox.GetCurrent().file );
        charInsertPos = filename.size();
    }

    listbox.Redraw();
    RedrawExtraInfo( fheroes2::Point( rt.x, rt.y ), header, filename, enter_field );

    buttonOk.draw();
    buttonCancel.draw();

    cursor.Show();
    display.render();

    std::string result;
    bool is_limit = false;

    while ( le.HandleEvents() && result.empty() ) {
        le.MousePressLeft( buttonOk.area() ) && buttonOk.isEnabled() ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        listbox.QueueEventProcessing();

        if ( ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) || listbox.isDoubleClicked() ) {
            if ( filename.size() )
                result = System::ConcatePath( Game::GetSaveDir(), filename + Game::GetSaveFileExtension() );
            else if ( listbox.isSelected() )
                result = listbox.GetCurrent().file;
        }
        else if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
            break;
        }
        else if ( le.MouseClickLeft( enter_field ) && editor ) {
            edit_mode = true;
            charInsertPos = GetInsertPosition( filename, le.GetMouseCursor().x, enter_field.x );
            if ( Settings::Get().PocketPC() )
                PocketPC::KeyboardDialog( filename );
            if ( filename.empty() )
                buttonOk.disable();
            cursor.Hide();
        }
        else if ( edit_mode && le.KeyPress() && ( !is_limit || KEY_BACKSPACE == le.KeyValue() || KEY_DELETE == le.KeyValue() ) ) {
            charInsertPos = InsertKeySym( filename, charInsertPos, le.KeyValue(), le.KeyMod() );
            if ( filename.empty() )
                buttonOk.disable();
            else
                buttonOk.enable();
            cursor.Hide();
        }
        if ( !edit_mode && le.KeyPress( KEY_DELETE ) && listbox.isSelected() ) {
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
            cursor.Hide();
        }

        if ( !cursor.isVisible() ) {
            listbox.Redraw();

            if ( edit_mode && editor )
                is_limit = RedrawExtraInfo( fheroes2::Point( rt.x, rt.y ), header, InsertString( filename, charInsertPos, "_" ), enter_field );
            else if ( listbox.isSelected() ) {
                filename = ResizeToShortName( listbox.GetCurrent().file );
                charInsertPos = filename.size();
                is_limit = RedrawExtraInfo( fheroes2::Point( rt.x, rt.y ), header, filename, enter_field );
            }
            else
                is_limit = RedrawExtraInfo( fheroes2::Point( rt.x, rt.y ), header, filename, enter_field );

            buttonOk.draw();
            buttonCancel.draw();
            cursor.Show();
            display.render();
        }
    }

    cursor.Hide();

    return result;
}

bool RedrawExtraInfo( const fheroes2::Point & dst, const std::string & header, const std::string & filename, const fheroes2::Rect & field )
{
    Text text( header, Font::BIG );
    text.Blit( dst.x + 175 - text.w() / 2, dst.y + 30 );

    if ( filename.size() ) {
        text.Set( filename, Font::BIG );
        text.Blit( field.x, field.y + 1, field.width );
    }

    return text.w() + 10 > field.width;
}
