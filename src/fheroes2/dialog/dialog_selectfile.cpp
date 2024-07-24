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
    // This constant sets the maximum displayed file name width. This value affects the dialog horizontal size.
    const int32_t maxFileNameWidth = 260;

    void redrawDateTime( fheroes2::Image & output, const time_t timestamp, const int32_t dstx, const int32_t dsty, const fheroes2::FontType font )
    {
        const size_t arraySize = 5;

        char shortMonth[arraySize];
        char shortDate[arraySize];
        char shortHours[arraySize];
        char shortMinutes[arraySize];

        const tm tmi = System::GetTM( timestamp );

        // TODO: Use system locale date format (e.g. 9 Mar instead of Mar 09).
        std::strftime( shortMonth, arraySize, "%b", &tmi );
        std::strftime( shortDate, arraySize, "%d", &tmi );
        std::strftime( shortHours, arraySize, "%H", &tmi );
        std::strftime( shortMinutes, arraySize, "%M", &tmi );

        fheroes2::Text text( shortMonth, font );
        text.draw( dstx + 4, dsty, output );

        text.set( shortDate, font );
        text.draw( dstx + 56 - text.width() / 2, dsty, output );

        text.set( ",", font );
        text.draw( dstx + 66, dsty, output );

        text.set( shortHours, font );
        text.draw( dstx + 82 - text.width() / 2, dsty, output );

        text.set( ":", font );
        text.draw( dstx + 92, dsty, output );

        text.set( shortMinutes, font );
        text.draw( dstx + 105 - text.width() / 2, dsty, output );
    }

    bool redrawTextInputField( const std::string & filename, const fheroes2::Rect & field, const bool isEditing )
    {
        if ( filename.empty() ) {
            return false;
        }

        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::Text currentFilename( filename, isEditing ? fheroes2::FontType::normalWhite() : fheroes2::FontType::normalYellow() );
        const int32_t initialTextWidth = currentFilename.width();
        currentFilename.fitToOneRow( maxFileNameWidth );
        currentFilename.draw( field.x + 4 + ( maxFileNameWidth - currentFilename.width() ) / 2, field.y + 4, display );

        return ( initialTextWidth + 10 ) > maxFileNameWidth;
    }

    class FileInfoListBox : public Interface::ListBox<Maps::FileInfo>
    {
    public:
        using Interface::ListBox<Maps::FileInfo>::ActionListDoubleClick;
        using Interface::ListBox<Maps::FileInfo>::ActionListSingleClick;
        using Interface::ListBox<Maps::FileInfo>::ActionListPressRight;

        using ListBox::ListBox;

        void RedrawItem( const Maps::FileInfo & info, int32_t dstx, int32_t dsty, bool current ) override;
        void RedrawBackground( const fheroes2::Point & dst ) override;

        void ActionCurrentUp() override;
        void ActionCurrentDn() override;
        void ActionListDoubleClick( Maps::FileInfo & info ) override;
        void ActionListSingleClick( Maps::FileInfo & info ) override;

        void ActionListPressRight( Maps::FileInfo & info ) override
        {
            // On some OSes like Windows, the path may contain '\' symbols. This symbol doesn't exist in the resources.
            // To avoid this we have to replace all '\' symbols with '/' symbols.
            std::string fullPath = info.filename;

            // TODO: Make '\' symbol in small font to properly show file path in OS familiar style.
            StringReplace( fullPath, "\\", "/" );

            const fheroes2::Text header( System::truncateFileExtensionAndPath( info.filename ), fheroes2::FontType::normalYellow() );

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

        int getCurrentId() const
        {
            return _currentId;
        }

        void initListBackgroundRestorer( fheroes2::Rect roi )
        {
            _listBackground = std::make_unique<fheroes2::ImageRestorer>( fheroes2::Display::instance(), roi.x, roi.y, roi.width, roi.height );
        }

        bool isDoubleClicked() const
        {
            return _isDoubleClicked;
        }

        void updateScrollBarImage()
        {
            const int32_t scrollBarWidth = _scrollbar.width();

            setScrollBarImage( fheroes2::generateScrollbarSlider( _scrollbar, false, _scrollbar.getArea().height, VisibleItemCount(), _size(),
                                                                  { 0, 0, scrollBarWidth, 8 }, { 0, 7, scrollBarWidth, 8 } ) );
            _scrollbar.moveToIndex( _topId );
        }

    private:
        bool _isDoubleClicked{ false };
        std::unique_ptr<fheroes2::ImageRestorer> _listBackground;
    };

    void FileInfoListBox::RedrawItem( const Maps::FileInfo & info, int32_t dstx, int32_t dsty, bool current )
    {
        std::string savname( System::GetBasename( info.filename ) );

        if ( savname.empty() ) {
            return;
        }

        const std::string saveExtension = Game::GetSaveFileExtension();
        const size_t dotPos = savname.size() - saveExtension.size();

        if ( StringLower( savname.substr( dotPos ) ) == saveExtension ) {
            savname.erase( dotPos );
        }

        const fheroes2::FontType font = current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite();
        fheroes2::Display & display = fheroes2::Display::instance();

        dsty += 2;

        fheroes2::Text text{ std::move( savname ), font };
        text.fitToOneRow( maxFileNameWidth );
        text.draw( dstx + 4 + ( maxFileNameWidth - text.width() ) / 2, dsty, display );

        redrawDateTime( display, info.timestamp, dstx + maxFileNameWidth + 9, dsty, font );
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

    MapsFileInfoList getSortedMapsFileInfoList()
    {
        ListFiles files;
        files.ReadDir( Game::GetSaveDir(), Game::GetSaveFileExtension(), false );

        MapsFileInfoList mapInfos;
        mapInfos.reserve( files.size() );

        for ( std::string & saveFile : files ) {
            Maps::FileInfo mapInfo;

            if ( Game::LoadSAV2FileInfo( std::move( saveFile ), mapInfo ) ) {
                mapInfos.emplace_back( std::move( mapInfo ) );
            }
        }

        std::sort( mapInfos.begin(), mapInfos.end(), Maps::FileInfo::sortByFileName );

        return mapInfos;
    }

    std::string selectFileListSimple( const std::string & header, const std::string & lastfile, const bool isEditing )
    {
        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        MapsFileInfoList lists = getSortedMapsFileInfoList();

        const int32_t listHeightDeduction = 112;
        const int32_t listAreaOffsetY = 3;
        const int32_t listAreaHeightDeduction = 4;

        // If we don't have many save files, we reduce the maximum dialog height,
        // but not less than enough for 11 elements.
        // We also limit the maximum list height to 22 lines.
        const int32_t maxDialogHeight = fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) * std::clamp( static_cast<int32_t>( lists.size() ), 11, 22 )
                                        + listAreaOffsetY + listAreaHeightDeduction + listHeightDeduction;

        fheroes2::Display & display = fheroes2::Display::instance();

        // Dialog height is also capped with the current screen height.
        fheroes2::StandardWindow background( maxFileNameWidth + 204, std::min( display.height() - 100, maxDialogHeight ), true, display );

        const fheroes2::Rect area( background.activeArea() );
        const fheroes2::Rect listRoi( area.x + 24, area.y + 37, area.width - 75, area.height - listHeightDeduction );
        const fheroes2::Rect textInputRoi( listRoi.x, listRoi.y + listRoi.height + 12, maxFileNameWidth + 8, 21 );
        const int32_t dateTimeoffsetX = textInputRoi.x + textInputRoi.width;
        const int32_t dateTimeWidth = listRoi.width - textInputRoi.width;

        // We divide the save-files list: file name and file date/time.
        background.applyTextBackgroundShading( { listRoi.x, listRoi.y, textInputRoi.width, listRoi.height } );
        background.applyTextBackgroundShading( { listRoi.x + textInputRoi.width, listRoi.y, dateTimeWidth, listRoi.height } );
        background.applyTextBackgroundShading( textInputRoi );
        // Make background for the selected file date and time.
        background.applyTextBackgroundShading( { dateTimeoffsetX, textInputRoi.y, dateTimeWidth, textInputRoi.height } );

        fheroes2::ImageRestorer textInputAndDateBackground( fheroes2::Display::instance(), textInputRoi.x, textInputRoi.y, listRoi.width, textInputRoi.height );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        // Prepare OKAY and CANCEL buttons and render their shadows.
        fheroes2::Button buttonOk;
        if ( !isEditing && lists.empty() ) {
            buttonOk.disable();
        }
        fheroes2::Button buttonCancel;

        background.renderOkayCancelButtons( buttonOk, buttonCancel, isEvilInterface );

        // Virtual keyboard button is used only in save game mode (when 'isEditing' is true ).
        std::unique_ptr<fheroes2::ButtonSprite> buttonVirtualKB;

        FileInfoListBox listbox( area.getPosition() );

        // Initialize list background restorer to use it in list method 'listbox.RedrawBackground()'.
        listbox.initListBackgroundRestorer( listRoi );

        listbox.SetAreaItems( { listRoi.x, listRoi.y + 3, listRoi.width - listAreaOffsetY, listRoi.height - listAreaHeightDeduction } );

        int32_t scrollbarOffsetX = area.x + area.width - 35;
        background.renderScrollbarBackground( { scrollbarOffsetX, listRoi.y, listRoi.width, listRoi.height }, isEvilInterface );

        const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;
        const int32_t topPartHeight = 19;
        ++scrollbarOffsetX;

        listbox.SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX, listRoi.y + 1 } );
        listbox.SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX, listRoi.y + listRoi.height - 15 } );
        listbox.setScrollBarArea( { scrollbarOffsetX + 2, listRoi.y + topPartHeight, 10, listRoi.height - 2 * topPartHeight } );
        listbox.setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );
        listbox.SetAreaMaxItems( ( listRoi.height - 7 ) / fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) );
        listbox.SetListContent( lists );
        listbox.updateScrollBarImage();

        std::string filename;
        size_t charInsertPos = 0;

        if ( !lastfile.empty() ) {
            filename = System::truncateFileExtensionAndPath( lastfile );
            charInsertPos = filename.size();

            MapsFileInfoList::iterator it = lists.begin();
            for ( ; it != lists.end(); ++it ) {
                if ( ( *it ).filename == lastfile ) {
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

        if ( filename.empty() && listbox.isSelected() ) {
            filename = System::truncateFileExtensionAndPath( listbox.GetCurrent().filename );
            charInsertPos = filename.size();
        }

        auto buttonOkDisabler = [&buttonOk, &filename]() {
            if ( filename.empty() && buttonOk.isEnabled() ) {
                buttonOk.disable();
                buttonOk.draw();
            }
            else if ( !filename.empty() && buttonOk.isDisabled() ) {
                buttonOk.enable();
                buttonOk.draw();
            }
        };

        listbox.Redraw();
        redrawTextInputField( filename, textInputRoi, isEditing );

        const fheroes2::Text title( header, fheroes2::FontType::normalYellow() );
        title.draw( area.x + ( area.width - title.width() ) / 2, area.y + 16, display );

        if ( isEditing ) {
            // Render a button to open the Virtual Keyboard window.
            buttonVirtualKB = std::make_unique<fheroes2::ButtonSprite>();
            background.renderButtonSprite( *buttonVirtualKB, "...", 48, { 0, 7 }, isEvilInterface, fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

            Game::passAnimationDelay( Game::DelayType::CURSOR_BLINK_DELAY );
        }
        else if ( listbox.isSelected() ) {
            // Render the saved file date and time.
            redrawDateTime( display, listbox.GetCurrent().timestamp, dateTimeoffsetX, textInputRoi.y + 4, fheroes2::FontType::normalYellow() );
        }

        display.render( background.totalArea() );

        std::string result;
        bool isTextLimit = false;
        std::string lastSelectedSaveFileName;

        const bool isInGameKeyboardRequired = System::isVirtualKeyboardSupported();

        bool isCursorVisible = true;

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents( !isEditing || Game::isDelayNeeded( { Game::DelayType::CURSOR_BLINK_DELAY } ) ) && result.empty() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedInArea( buttonOk.area() ) );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) );
            if ( isEditing ) {
                buttonVirtualKB->drawOnState( le.isMouseLeftButtonPressedInArea( buttonVirtualKB->area() ) );
            }

            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return {};
            }

            const int listId = listbox.getCurrentId();

            const bool listboxEvent = listbox.QueueEventProcessing();

            bool isListboxSelected = listbox.isSelected();

            bool needRedraw = listId != listbox.getCurrentId();

            if ( le.isKeyPressed( fheroes2::Key::KEY_DELETE ) && isListboxSelected ) {
                listbox.SetCurrent( listId );
                listbox.Redraw();

                std::string msg( _( "Are you sure you want to delete file:" ) );
                msg.append( "\n\n" );
                msg.append( System::GetBasename( listbox.GetCurrent().filename ) );
                if ( Dialog::YES == fheroes2::showStandardTextMessage( _( "Warning!" ), msg, Dialog::YES | Dialog::NO ) ) {
                    System::Unlink( listbox.GetCurrent().filename );
                    listbox.RemoveSelected();
                    if ( lists.empty() ) {
                        listbox.Redraw();

                        if ( !isEditing ) {
                            textInputAndDateBackground.restore();
                            fheroes2::showStandardTextMessage( _( "Load Game" ), _( "No save files to load." ), Dialog::OK );
                            return {};
                        }

                        isListboxSelected = false;
                        charInsertPos = 0;
                        filename.clear();

                        buttonOk.disable();
                        buttonOk.draw();
                    }

                    listbox.updateScrollBarImage();
                    listbox.SetCurrent( std::max( listId - 1, 0 ) );
                }

                needRedraw = true;
            }
            else if ( ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY )
                      || listbox.isDoubleClicked() ) {
                if ( !filename.empty() ) {
                    result = System::concatPath( Game::GetSaveDir(), filename + Game::GetSaveFileExtension() );
                }
                else if ( isListboxSelected ) {
                    result = listbox.GetCurrent().filename;
                }
            }
            else if ( isEditing ) {
                if ( le.MouseClickLeft( buttonVirtualKB->area() ) || ( isInGameKeyboardRequired && le.MouseClickLeft( textInputRoi ) ) ) {
                    fheroes2::openVirtualKeyboard( filename );

                    charInsertPos = filename.size();
                    listbox.Unselect();
                    isListboxSelected = false;
                    needRedraw = true;

                    buttonOkDisabler();

                    // Set the whole screen to redraw next time to properly restore image under the Virtual Keyboard dialog.
                    display.updateNextRenderRoi( { 0, 0, display.width(), display.height() } );
                }
                else if ( !filename.empty() && le.MouseClickLeft( textInputRoi ) ) {
                    const fheroes2::Text text( filename, fheroes2::FontType::normalWhite() );
                    const int32_t textStartOffsetX = isTextLimit ? 8 : ( textInputRoi.width - text.width() ) / 2;
                    charInsertPos = fheroes2::getTextInputCursorPosition( filename, fheroes2::FontType::normalWhite(), charInsertPos, le.getMouseCursorPos().x,
                                                                          textInputRoi.x + textStartOffsetX );

                    listbox.Unselect();
                    isListboxSelected = false;
                    needRedraw = true;
                }
                else if ( !listboxEvent && le.isAnyKeyPressed()
                          && ( !isTextLimit || fheroes2::Key::KEY_BACKSPACE == le.getPressedKeyValue() || fheroes2::Key::KEY_DELETE == le.getPressedKeyValue() )
                          && le.getPressedKeyValue() != fheroes2::Key::KEY_UP && le.getPressedKeyValue() != fheroes2::Key::KEY_DOWN ) {
                    charInsertPos = InsertKeySym( filename, charInsertPos, le.getPressedKeyValue(), LocalEvent::getCurrentKeyModifiers() );

                    buttonOkDisabler();

                    needRedraw = true;
                    listbox.Unselect();
                    isListboxSelected = false;
                }
            }

            if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                if ( isEditing ) {
                    fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to save the current game." ), Dialog::ZERO );
                }
                else {
                    fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to load a previously saved game." ), Dialog::ZERO );
                }
            }
            else if ( isEditing && le.isMouseRightButtonPressedInArea( buttonVirtualKB->area() ) ) {
                fheroes2::showStandardTextMessage( _( "Open Virtual Keyboard" ), _( "Click to open the Virtual Keyboard dialog." ), Dialog::ZERO );
            }

            // Text input cursor blink.
            if ( isEditing && Game::validateAnimationDelay( Game::DelayType::CURSOR_BLINK_DELAY ) ) {
                isCursorVisible = !isCursorVisible;
                needRedraw = true;
            }

            if ( !needRedraw && !listbox.IsNeedRedraw() ) {
                continue;
            }

            if ( needRedraw ) {
                const std::string selectedFileName = isListboxSelected ? System::truncateFileExtensionAndPath( listbox.GetCurrent().filename ) : "";
                if ( isListboxSelected && lastSelectedSaveFileName != selectedFileName ) {
                    lastSelectedSaveFileName = selectedFileName;
                    filename = selectedFileName;
                    charInsertPos = filename.size();

                    buttonOkDisabler();
                }
                else if ( isEditing ) {
                    // Empty last selected save file name so that we can replace the input field's name if we select the same save file again.
                    // But when loading (i.e. isEditing == false), this doesn't matter since we cannot write to the input field
                    lastSelectedSaveFileName = "";
                }

                textInputAndDateBackground.restore();

                if ( isEditing ) {
                    isTextLimit = redrawTextInputField( insertCharToString( filename, charInsertPos, isCursorVisible ? '_' : '\x7F' ), textInputRoi, true );
                    redrawDateTime( display, std::time( nullptr ), dateTimeoffsetX, textInputRoi.y + 4, fheroes2::FontType::normalWhite() );
                }
                else {
                    redrawTextInputField( filename, textInputRoi, false );
                    redrawDateTime( display, listbox.GetCurrent().timestamp, dateTimeoffsetX, textInputRoi.y + 4, fheroes2::FontType::normalYellow() );
                }
            }

            if ( listbox.IsNeedRedraw() ) {
                listbox.Redraw();
                display.render( area );
            }
            else {
                display.render( textInputAndDateBackground.rect() );
            }
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

        return selectFileListSimple( _( "File to Save:" ), os.str(), true );
    }

    std::string SelectFileLoad()
    {
        const std::string & lastfile = Game::GetLastSaveName();
        return selectFileListSimple( _( "File to Load:" ), ( !lastfile.empty() ? lastfile : "" ), false );
    }
}
