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
#include <utility>
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
#include "settings.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_keyboard.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"
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

    bool redrawExtraInfo( const std::string & filename, const fheroes2::Rect & field )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        if ( filename.empty() ) {
            return false;
        }

        fheroes2::Text currentFilename( filename, fheroes2::FontType::normalWhite() );
        currentFilename.fitToOneRow( field.width );
        currentFilename.draw( field.x + 2, field.y + 4, display );

        return currentFilename.width() + 10 > field.width;
    }

    class FileInfoListBox : public Interface::ListBox<Maps::FileInfo>
    {
    public:
        using Interface::ListBox<Maps::FileInfo>::ActionListDoubleClick;
        using Interface::ListBox<Maps::FileInfo>::ActionListSingleClick;
        using Interface::ListBox<Maps::FileInfo>::ActionListPressRight;

        explicit FileInfoListBox( const fheroes2::Point & pt )
            : Interface::ListBox<Maps::FileInfo>( pt )
        {}

        void RedrawItem( const Maps::FileInfo & info, int32_t dstx, int32_t dsty, bool current ) override;
        void RedrawBackground( const fheroes2::Point & dst ) override;

        void ActionCurrentUp() override;
        void ActionCurrentDn() override;
        void ActionListDoubleClick( Maps::FileInfo & info ) override;
        void ActionListSingleClick( Maps::FileInfo & info ) override;

        void ActionListPressRight( Maps::FileInfo & info ) override
        {
            // On some OSes like Windows, the path may contain '\' symbols. This symbol doesn't exist in the resources.
            // To avoid this we have to replace all '\' symbols by '/' symbols.
            std::string fullPath = info.file;

            // TODO: Make '\' symbol in small font to properly show file path in OS familliar style.
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

        void updateScrollBarImage()
        {
            const int32_t scrollBarWidth = _scrollbar.width();

            setScrollBarImage( fheroes2::generateScrollbarSlider( _scrollbar, false, _scrollbar.getArea().height, VisibleItemCount(), _size(),
                                                                  { 0, 0, scrollBarWidth, 8 }, { 0, 7, scrollBarWidth, 8 } ) );
            _scrollbar.moveToIndex( _topId );
        }

        void initListBackgroundRestorer( fheroes2::Rect roi )
        {
            _listBackground = std::make_unique<fheroes2::ImageRestorer>( fheroes2::Display::instance(), roi.x, roi.y, roi.width, roi.height );
        }

        bool isDoubleClicked() const
        {
            return _isDoubleClicked;
        }

    private:
        bool _isDoubleClicked{ false };
        std::unique_ptr<fheroes2::ImageRestorer> _listBackground;
    };

    void FileInfoListBox::RedrawItem( const Maps::FileInfo & info, int32_t dstx, int32_t dsty, bool current )
    {
        const size_t arraySize = 10;
        const size_t arrayWriteSize = arraySize - 1;

        char shortDate[arraySize]{ 0 };
        char shortHours[arraySize]{ 0 };
        char shortMinutes[arraySize]{ 0 };

        const tm tmi = System::GetTM( info.timestamp );

        std::strftime( shortDate, arrayWriteSize, "%b %d,", &tmi );
        std::strftime( shortHours, arrayWriteSize, "%H", &tmi );
        std::strftime( shortMinutes, arrayWriteSize, ":%M", &tmi );
        std::string savname( System::GetBasename( info.file ) );

        if ( !savname.empty() ) {
            const std::string saveExtension = Game::GetSaveFileExtension();
            const size_t dotPos = savname.size() - saveExtension.size();

            if ( StringLower( savname.substr( dotPos ) ) == saveExtension ) {
                savname.erase( dotPos );
            }

            const fheroes2::FontType font = current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite();
            fheroes2::Display & display = fheroes2::Display::instance();

            dsty += 2;

            fheroes2::Text text{ std::move( savname ), font };
            text.fitToOneRow( _listBackground->width() - 120 );
            text.draw( dstx + 5, dsty, display );

            dstx += _listBackground->width();

            text.set( shortDate, font );
            text.draw( dstx - 45 - text.width(), dsty, display );

            text.set( shortHours, font );
            text.draw( dstx - 35 - text.width() / 2, dsty, display );

            text.set( shortMinutes, font );
            text.draw( dstx - 25, dsty, display );
        }
    }

    void FileInfoListBox::RedrawBackground( const fheroes2::Point & /* unused */ )
    {
        _listBackground->restore();
    }

    void FileInfoListBox::ActionCurrentUp()
    {
        // Do nothing.
    }

    void FileInfoListBox::ActionCurrentDn()
    {
        // Do nothing.
    }

    void FileInfoListBox::ActionListDoubleClick( Maps::FileInfo & /*unused*/ )
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

    std::string SelectFileListSimple( const std::string & header, const std::string & lastfile, const bool isEditing )
    {
        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::StandardWindow background( 455, display.height() - 100, true, display );

        const fheroes2::Rect area = background.activeArea();
        const fheroes2::Rect listRoi( area.x + 24, area.y + 37, area.width - 75, area.height - 112 );
        const fheroes2::Rect textInputRoi( area.x + 24, listRoi.y + listRoi.height + 12, 270, 20 );
        background.applyTextBackgroundShading( listRoi );
        background.applyTextBackgroundShading( textInputRoi );

        fheroes2::ImageRestorer textInputBackground( fheroes2::Display::instance(), textInputRoi.x, textInputRoi.y, textInputRoi.width, textInputRoi.height );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        fheroes2::Button buttonOk( area.x + 18, area.y + area.height - 32, buttonOkIcn, 0, 1 );
        fheroes2::addGradientShadow( fheroes2::AGG::GetICN( buttonOkIcn, 0 ), display, buttonOk.area().getPosition(), { -5, 5 } );

        const int buttonCancelIcn = isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD;
        const fheroes2::Sprite & buttonCancelSprite = fheroes2::AGG::GetICN( buttonCancelIcn, 0 );
        fheroes2::Button buttonCancel( area.x + area.width - buttonCancelSprite.width() - 20, area.y + area.height - 32, buttonCancelIcn, 0, 1 );
        fheroes2::addGradientShadow( fheroes2::AGG::GetICN( buttonCancelIcn, 0 ), display, buttonCancel.area().getPosition(), { -5, 5 } );

        std::unique_ptr<fheroes2::ButtonSprite> buttonVirtualKB;

        MapsFileInfoList lists = GetSortedMapsFileInfoList();
        FileInfoListBox listbox( area.getPosition() );

        listbox.initListBackgroundRestorer( listRoi );

        listbox.SetAreaItems( { listRoi.x, listRoi.y + 5, listRoi.width - 10, listRoi.height - 5 } );

        const fheroes2::Sprite & scrollBar = fheroes2::AGG::GetICN( isEvilInterface ? ICN::ADVBORDE : ICN::ADVBORD, 0 );

        int32_t scrollbarOffsetX = area.x + area.width - 35;

        // Top part of scrollbar background.
        const int32_t topPartHeight = 19;
        const int32_t scrollBarWidth = 16;
        fheroes2::Copy( scrollBar, 536, 176, display, scrollbarOffsetX, listRoi.y, scrollBarWidth, topPartHeight );

        // Middle part of scrollbar background.
        int32_t offsetY = topPartHeight;
        const int32_t middlePartHeight = 88;
        const int32_t middlePartCount = ( listRoi.height - 2 * topPartHeight + middlePartHeight - 1 ) / middlePartHeight;

        for ( int32_t i = 0; i < middlePartCount; ++i ) {
            fheroes2::Copy( scrollBar, 536, 196, display, scrollbarOffsetX, listRoi.y + offsetY, scrollBarWidth,
                            std::min( middlePartHeight, listRoi.height - offsetY - topPartHeight ) );
            offsetY += middlePartHeight;
        }

        // Bottom part of scrollbar background.
        fheroes2::Copy( scrollBar, 536, 285, display, scrollbarOffsetX, listRoi.y + listRoi.height - topPartHeight, scrollBarWidth, topPartHeight );

        const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;

        ++scrollbarOffsetX;

        listbox.SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX, listRoi.y + 1 } );
        listbox.SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX, listRoi.y + listRoi.height - 15 } );

        listbox.setScrollBarArea( { scrollbarOffsetX + 2, listRoi.y + topPartHeight, 10, listRoi.height - 2 * topPartHeight } );

        listbox.setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );

        // Make scrollbar shadow.
        for ( uint8_t i = 0; i < 4; ++i ) {
            const uint8_t transformId = i + 2;
            const int32_t sizeCorrection = i + 1;
            fheroes2::ApplyTransform( display, scrollbarOffsetX - transformId, listRoi.y + sizeCorrection, 1, listRoi.height - sizeCorrection, transformId );
            fheroes2::ApplyTransform( display, scrollbarOffsetX - transformId, listRoi.y + listRoi.height + i, scrollBarWidth, 1, transformId );
        }

        listbox.SetAreaMaxItems( ( listRoi.height - 10 ) / fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) );
        listbox.SetListContent( lists );
        listbox.updateScrollBarImage();

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
        redrawExtraInfo( filename, textInputRoi );

        const fheroes2::Text title( header, fheroes2::FontType::normalYellow() );
        title.draw( area.x + ( area.width - title.width() ) / 2, area.y + 16, display );

        buttonOk.draw();
        buttonCancel.draw();

        if ( isEditing ) {
            // Generate and render a button to open the Virtual Keyboard window.
            fheroes2::Sprite released;
            fheroes2::Sprite pressed;

            makeButtonSprites( released, pressed, "...", 15, isEvilInterface, false );
            buttonVirtualKB = std::make_unique<fheroes2::ButtonSprite>( textInputRoi.x + textInputRoi.width + 5, textInputRoi.y, released, pressed );

            fheroes2::addGradientShadow( released, display, buttonVirtualKB->area().getPosition(), { -5, 5 } );
            buttonVirtualKB->draw();

            Game::passAnimationDelay( Game::DelayType::CURSOR_BLINK_DELAY );
        }

        display.render( background.totalArea() );

        std::string result;
        bool isTextLimit = false;
        std::string lastSelectedSaveFileName;

        const bool isInGameKeyboardRequired = System::isVirtualKeyboardSupported();

        bool isCursorVisible = true;

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents( !isEditing || Game::isDelayNeeded( { Game::DelayType::CURSOR_BLINK_DELAY } ) ) && result.empty() ) {
            le.MousePressLeft( buttonOk.area() ) && buttonOk.isEnabled() ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
            le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();
            if ( isEditing ) {
                le.MousePressLeft( buttonVirtualKB->area() ) ? buttonVirtualKB->drawOnPress() : buttonVirtualKB->drawOnRelease();
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
                if ( le.MouseClickLeft( buttonVirtualKB->area() ) || ( isInGameKeyboardRequired && le.MouseClickLeft( textInputRoi ) ) ) {
                    fheroes2::openVirtualKeyboard( filename );
                    charInsertPos = filename.size();
                    listbox.Unselect();
                    isListboxSelected = false;
                    needRedraw = true;
                }
                else if ( le.MouseClickLeft( textInputRoi ) ) {
                    charInsertPos
                        = fheroes2::getTextInputCursorPosition( filename, fheroes2::FontType::normalWhite(), charInsertPos, le.GetMouseCursor().x, textInputRoi.x );
                    if ( filename.empty() ) {
                        buttonOk.disable();
                    }

                    needRedraw = true;
                }
                else if ( !listboxEvent && le.KeyPress()
                          && ( !isTextLimit || fheroes2::Key::KEY_BACKSPACE == le.KeyValue() || fheroes2::Key::KEY_DELETE == le.KeyValue() ) ) {
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
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( buttonOk.area() ) ) {
                if ( isEditing ) {
                    fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to save the current game." ), Dialog::ZERO );
                }
                else {
                    fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to load a previously saved game." ), Dialog::ZERO );
                }
            }
            else if ( isEditing && le.MousePressRight( buttonVirtualKB->area() ) ) {
                fheroes2::showStandardTextMessage( _( "Open Virtual Keyboard" ), _( "Click to open the Virtual Keyboard dialog." ), Dialog::ZERO );
            }

            if ( !isEditing && le.KeyPress( fheroes2::Key::KEY_DELETE ) && isListboxSelected ) {
                std::string msg( _( "Are you sure you want to delete file:" ) );
                msg.append( "\n \n" );
                msg.append( System::GetBasename( listbox.GetCurrent().file ) );
                if ( Dialog::YES == fheroes2::showStandardTextMessage( _( "Warning!" ), msg, Dialog::YES | Dialog::NO ) ) {
                    System::Unlink( listbox.GetCurrent().file );
                    listbox.RemoveSelected();
                    if ( lists.empty() || filename.empty() ) {
                        buttonOk.disable();
                        isListboxSelected = false;
                        filename.clear();
                    }

                    listbox.updateScrollBarImage();

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

            textInputBackground.restore();
            isTextLimit = isEditing ? redrawExtraInfo( insertCharToString( filename, charInsertPos, isCursorVisible ? '_' : '\x7F' ), textInputRoi )
                                    : redrawExtraInfo( filename, textInputRoi );

            display.render( background.activeArea() );
        }

        return result;
    }
}

namespace Dialog
{
    std::string SelectFileSave()
    {
        std::ostringstream os;

        os << System::concatPath( Game::GetSaveDir(), Game::GetSaveFileBaseName() ) << '_' << std::setw( 4 ) << std::setfill( '0' ) << world.CountDay()
           << Game::GetSaveFileExtension();

        return SelectFileListSimple( _( "File to Save:" ), os.str(), true );
    }

    std::string SelectFileLoad()
    {
        const std::string & lastfile = Game::GetLastSaveName();
        return SelectFileListSimple( _( "File to Load:" ), ( !lastfile.empty() ? lastfile : "" ), false );
    }
}
