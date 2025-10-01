/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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
#include <cassert>
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
#include "dialog.h" // IWYU pragma: associated
#include "dir.h"
#include "game_hotkeys.h"
#include "game_io.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "logging.h"
#include "maps_fileinfo.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_keyboard.h"
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

    void redrawTextInputField( const std::string & filename, const fheroes2::Rect & field, fheroes2::Image & output )
    {
        if ( filename.empty() ) {
            return;
        }

        fheroes2::Text textInput( filename, fheroes2::FontType::normalYellow() );

        // Do not ignore spaces at the end.
        textInput.keepLineTrailingSpaces();
        textInput.fitToOneRow( maxFileNameWidth );

        textInput.drawInRoi( field.x + 4 + ( maxFileNameWidth - textInput.width() ) / 2, field.y + 4, output, field );
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
            const fheroes2::Text header( System::GetStem( info.filename ), fheroes2::FontType::normalYellow() );

            fheroes2::MultiFontText body;

            body.add( { _( "Map: " ), fheroes2::FontType::normalYellow() } );
            body.add( { info.name, fheroes2::FontType::normalWhite(), info.getSupportedLanguage() } );

            if ( info.worldDay > 0 || info.worldWeek > 0 || info.worldMonth > 0 ) {
                body.add( { _( "\n\nMonth: " ), fheroes2::FontType::normalYellow() } );
                body.add( { std::to_string( info.worldMonth ), fheroes2::FontType::normalWhite() } );
                body.add( { _( ", Week: " ), fheroes2::FontType::normalYellow() } );
                body.add( { std::to_string( info.worldWeek ), fheroes2::FontType::normalWhite() } );
                body.add( { _( ", Day: " ), fheroes2::FontType::normalYellow() } );
                body.add( { std::to_string( info.worldDay ), fheroes2::FontType::normalWhite() } );
            }

            body.add( { _( "\n\nLocation: " ), fheroes2::FontType::smallYellow() } );
            body.add( { info.filename, fheroes2::FontType::smallWhite() } );

            fheroes2::showMessage( header, body, Dialog::ZERO );
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
        std::string savname( System::GetStem( info.filename ) );
        assert( !savname.empty() );

        const fheroes2::FontType font = current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite();
        fheroes2::Display & display = fheroes2::Display::instance();

        dsty += 2;

        fheroes2::Text text{ std::move( savname ), font };
        text.keepLineTrailingSpaces();
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

    void sortMapInfos( MapsFileInfoList & mapInfos )
    {
        const SaveFileSortingMethod sortType = Settings::Get().getSaveFileSortingMethod();
        if ( sortType == SaveFileSortingMethod::FILENAME ) {
            std::sort( mapInfos.begin(), mapInfos.end(), Maps::FileInfo::CompareByFileName{} );
        }
        else {
            assert( sortType == SaveFileSortingMethod::TIMESTAMP );
            std::sort( mapInfos.begin(), mapInfos.end(), Maps::FileInfo::CompareByTimestamp{} );
        }
    }

    MapsFileInfoList getSortedMapsFileInfoList()
    {
        ListFiles files;
        files.ReadDir( Game::GetSaveDir(), Game::GetSaveFileExtension() );

        MapsFileInfoList mapInfos;
        mapInfos.reserve( files.size() );

        for ( std::string & saveFile : files ) {
            Maps::FileInfo mapInfo;

            if ( Game::LoadSAV2FileInfo( std::move( saveFile ), mapInfo ) ) {
                mapInfos.emplace_back( std::move( mapInfo ) );
            }
        }

        sortMapInfos( mapInfos );

        return mapInfos;
    }

    MapsFileInfoList::const_iterator findInMapInfos( const MapsFileInfoList::const_iterator & begin, const MapsFileInfoList::const_iterator & end,
                                                     const std::string & lastChoice )
    {
        return std::find_if( begin, end, [&lastChoice]( const Maps::FileInfo & info ) { return info.filename == lastChoice; } );
    }

    MapsFileInfoList::const_iterator findInMapInfos( const MapsFileInfoList::const_iterator & begin, const MapsFileInfoList::const_iterator & end,
                                                     const std::string & lastChoice, const uint32_t lastChoiceTimestamp, const SaveFileSortingMethod sortingMethod )
    {
        switch ( sortingMethod ) {
        case SaveFileSortingMethod::FILENAME: {
#ifdef WITH_DEBUG
            assert( std::is_sorted( begin, end, Maps::FileInfo::CompareByFileName{} ) );
#endif

            // With case-insensitive sorting, there may be several "identical" file names on the case-sensitive file system
            const auto [beginSameFileName, endSameFileName] = std::equal_range( begin, end, lastChoice, Maps::FileInfo::CompareByFileName{} );

            if ( const MapsFileInfoList::const_iterator iter = findInMapInfos( beginSameFileName, endSameFileName, lastChoice ); iter != endSameFileName ) {
                return iter;
            }

            break;
        }
        case SaveFileSortingMethod::TIMESTAMP: {
#ifdef WITH_DEBUG
            assert( std::is_sorted( begin, end, Maps::FileInfo::CompareByTimestamp{} ) );
#endif

            const auto [beginSameTimestamp, endSameTimestamp] = std::equal_range( begin, end, lastChoiceTimestamp, Maps::FileInfo::CompareByTimestamp{} );

            if ( const MapsFileInfoList::const_iterator iter = findInMapInfos( beginSameTimestamp, endSameTimestamp, lastChoice ); iter != endSameTimestamp ) {
                return iter;
            }

            break;
        }
        default:
            assert( 0 );
            break;
        }

        return end;
    }

    MapsFileInfoList::const_iterator findInMapInfos( const MapsFileInfoList::const_iterator & begin, const MapsFileInfoList::const_iterator & end,
                                                     const std::string & lastChoice, const SaveFileSortingMethod sortingMethod )
    {
        switch ( sortingMethod ) {
        case SaveFileSortingMethod::FILENAME: {
#ifdef WITH_DEBUG
            assert( std::is_sorted( begin, end, Maps::FileInfo::CompareByFileName{} ) );
#endif

            // With case-insensitive sorting, there may be several "identical" file names on the case-sensitive file system
            const auto [beginSameFileName, endSameFileName] = std::equal_range( begin, end, lastChoice, Maps::FileInfo::CompareByFileName{} );

            if ( const MapsFileInfoList::const_iterator iter = findInMapInfos( beginSameFileName, endSameFileName, lastChoice ); iter != endSameFileName ) {
                return iter;
            }

            break;
        }
        case SaveFileSortingMethod::TIMESTAMP:
            return findInMapInfos( begin, end, lastChoice );
        default:
            assert( 0 );
            break;
        }

        return end;
    }

    std::string selectFileListSimple( const std::string & header, const std::string & lastfile, const bool isEditing )
    {
        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        MapsFileInfoList lists = getSortedMapsFileInfoList();

        const int32_t listHeightDeduction = 120;
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

        const fheroes2::Rect dialogArea( background.activeArea() );
        const fheroes2::Rect listRoi( dialogArea.x + 24, dialogArea.y + 57, dialogArea.width - 75, dialogArea.height - listHeightDeduction );
        const fheroes2::Rect nameHeaderRoi( listRoi.x, listRoi.y - 28, maxFileNameWidth + 8, 28 );
        const fheroes2::Rect textInputRoi( listRoi.x, listRoi.y + listRoi.height + 3, maxFileNameWidth + 8, 23 );
        const int32_t dateTimeOffsetX = textInputRoi.x + textInputRoi.width;
        const int32_t dateTimeWidth = listRoi.width - textInputRoi.width;
        const fheroes2::Rect dateHeaderRoi( dateTimeOffsetX, nameHeaderRoi.y, dateTimeWidth, nameHeaderRoi.height );

        // We divide the save-files list: file name and file date/time.
        background.applyTextBackgroundShading( { listRoi.x, listRoi.y, textInputRoi.width, listRoi.height } );
        background.applyTextBackgroundShading( { listRoi.x + textInputRoi.width, listRoi.y, dateTimeWidth, listRoi.height } );
        background.applyTextBackgroundShading( nameHeaderRoi );
        background.applyTextBackgroundShading( dateHeaderRoi );
        background.applyTextBackgroundShading( textInputRoi );
        // Make background for the selected file date and time.
        background.applyTextBackgroundShading( { dateTimeOffsetX, textInputRoi.y, dateTimeWidth, textInputRoi.height } );

        fheroes2::ImageRestorer textInputBackground( display, textInputRoi.x, textInputRoi.y, textInputRoi.width, textInputRoi.height );
        fheroes2::ImageRestorer dateBackground( display, dateTimeOffsetX, textInputRoi.y, dateTimeWidth, textInputRoi.height );
        const fheroes2::Rect textInputAndDateROI( textInputRoi.x, textInputRoi.y, listRoi.width, textInputRoi.height );

        // Prepare OKAY and CANCEL buttons and render their shadows.
        fheroes2::Button buttonOk;
        if ( !isEditing && lists.empty() ) {
            buttonOk.disable();
        }
        fheroes2::Button buttonCancel;

        background.renderOkayCancelButtons( buttonOk, buttonCancel );

        FileInfoListBox listbox( dialogArea.getPosition() );

        // Initialize list background restorer to use it in list method 'listbox.RedrawBackground()'.
        listbox.initListBackgroundRestorer( listRoi );

        listbox.SetAreaItems( { listRoi.x, listRoi.y + 3, listRoi.width - listAreaOffsetY, listRoi.height - listAreaHeightDeduction } );

        Settings & settings = Settings::Get();
        const bool isEvilInterface = settings.isEvilInterfaceEnabled();
        const SaveFileSortingMethod fileSortingMethod = settings.getSaveFileSortingMethod();

        int32_t scrollbarOffsetX = dialogArea.x + dialogArea.width - 35;
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
            filename = System::GetStem( lastfile );
            charInsertPos = filename.size();
            const MapsFileInfoList::const_iterator it = findInMapInfos( lists.cbegin(), lists.cend(), lastfile, fileSortingMethod );

            if ( it != lists.cend() ) {
                listbox.SetCurrent( std::distance( lists.cbegin(), it ) );
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
            filename = System::GetStem( listbox.GetCurrent().filename );
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

        // Virtual keyboard button, text input and blinking cursor are used only in save game mode (when 'isEditing' is true ).
        std::unique_ptr<fheroes2::Button> buttonVirtualKB;
        std::unique_ptr<fheroes2::TextInputField> textInput;

        const fheroes2::Text title( header, fheroes2::FontType::normalYellow() );
        title.drawInRoi( dialogArea.x + ( dialogArea.width - title.width() ) / 2, dialogArea.y + 9, display, dialogArea );

        const fheroes2::Text nameHeader( _( "saveLoadDialog|Name" ), fheroes2::FontType::normalYellow() );
        nameHeader.drawInRoi( nameHeaderRoi.x, nameHeaderRoi.y + ( nameHeaderRoi.height - nameHeader.height() ) / 2 + 3, nameHeaderRoi.width, display, nameHeaderRoi );

        const fheroes2::Text dateHeader( _( "saveLoadDialog|Date" ), fheroes2::FontType::normalYellow() );
        dateHeader.drawInRoi( dateHeaderRoi.x, dateHeaderRoi.y + ( dateHeaderRoi.height - dateHeader.height() ) / 2 + 3, dateHeaderRoi.width, display, dateHeaderRoi );

        // Draw radio buttons for toggling between sorting methods.
        const int sortMarkIcnID = isEvilInterface ? ICN::CELLWIN_EVIL : ICN::CELLWIN;
        const fheroes2::Sprite & markBackground = fheroes2::AGG::GetICN( sortMarkIcnID, 4 );

        const fheroes2::Rect markBackgroundNameRoi( nameHeaderRoi.x + 5, nameHeaderRoi.y + 6, markBackground.width(), markBackground.height() );
        const fheroes2::Rect markBackgroundDateRoi( dateTimeOffsetX + 5, nameHeaderRoi.y + 6, markBackground.width(), markBackground.height() );
        fheroes2::Blit( markBackground, display, markBackgroundNameRoi );
        fheroes2::Blit( markBackground, display, markBackgroundDateRoi );

        const fheroes2::Sprite & mark = fheroes2::AGG::GetICN( sortMarkIcnID, 5 );

        if ( fileSortingMethod == SaveFileSortingMethod::FILENAME ) {
            fheroes2::Blit( mark, display, { markBackgroundNameRoi.x + 3, markBackgroundNameRoi.y + 3, mark.width(), mark.height() } );
        }
        else {
            fheroes2::Blit( mark, display, { markBackgroundDateRoi.x + 3, markBackgroundDateRoi.y + 3, mark.width(), mark.height() } );
        }

        // Redraw sort radio buttons, sort file list in new method, and return whether to redraw the list.
        auto switchFileSorting
            = [&markBackgroundNameRoi, &markBackgroundDateRoi, &markBackground, &mark, &display, &listbox, &lists, &settings]( const bool doSortByDate ) {
                  const fheroes2::Rect roiMarkBackground = doSortByDate ? markBackgroundNameRoi : markBackgroundDateRoi;
                  const fheroes2::Rect roiMark = doSortByDate ? markBackgroundDateRoi : markBackgroundNameRoi;
                  fheroes2::Blit( markBackground, display, roiMarkBackground );
                  fheroes2::Blit( mark, display, { roiMark.x + 3, roiMark.y + 3, roiMark.width, roiMark.height } );

                  const int currentId = listbox.getCurrentId();
                  std::string lastChoice{};
                  uint32_t lastChoiceTimestamp{};
                  if ( currentId >= 0 && static_cast<size_t>( currentId ) < lists.size() ) {
                      lastChoice = lists[currentId].filename;
                      lastChoiceTimestamp = lists[currentId].timestamp;
                  }

                  settings.setSaveFileSortingMethod( doSortByDate ? SaveFileSortingMethod::TIMESTAMP : SaveFileSortingMethod::FILENAME );
                  sortMapInfos( lists );

                  bool redrawNeeded = false;

                  // Re-select the last selected file if any, unless we're typing in the list box
                  if ( !lastChoice.empty() ) {
                      const MapsFileInfoList::const_iterator it
                          = findInMapInfos( lists.cbegin(), lists.cend(), lastChoice, lastChoiceTimestamp, settings.getSaveFileSortingMethod() );

                      if ( it != lists.cend() ) {
                          const int newId = static_cast<int>( std::distance( lists.cbegin(), it ) );
                          if ( newId != currentId ) {
                              listbox.SetCurrent( newId );
                              redrawNeeded = true;
                          }
                      }
                  }
                  return redrawNeeded;
              };

        if ( isEditing ) {
            // Render a button to open the Virtual Keyboard window.
            buttonVirtualKB = std::make_unique<fheroes2::Button>();
            const int buttonVirtualKBIcnID = isEvilInterface ? ICN::BUTTON_VIRTUAL_KEYBOARD_EVIL : ICN::BUTTON_VIRTUAL_KEYBOARD_GOOD;
            background.renderButton( *buttonVirtualKB, buttonVirtualKBIcnID, 0, 1, { 0, 7 }, fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

            // Prepare the text input and set it to always fit the file name input field width.
            textInput = std::make_unique<fheroes2::TextInputField>( fheroes2::Rect{ textInputRoi.x + 4, textInputRoi.y + 2, maxFileNameWidth, textInputRoi.height - 2 },
                                                                    false, true, display );

            if ( !listbox.isSelected() ) {
                textInput->draw( filename, static_cast<int32_t>( charInsertPos ) );
                redrawDateTime( display, std::time( nullptr ), dateTimeOffsetX, textInputRoi.y + 4, fheroes2::FontType::normalWhite() );
            }
        }

        if ( listbox.isSelected() ) {
            // Render the saved file name, date and time.
            redrawTextInputField( filename, textInputRoi, display );
            redrawDateTime( display, listbox.GetCurrent().timestamp, dateTimeOffsetX, textInputRoi.y + 4, fheroes2::FontType::normalYellow() );
        }

        display.render( background.totalArea() );

        std::string result;
        std::string lastSelectedSaveFileName;

        const bool isInGameKeyboardRequired = System::isVirtualKeyboardSupported();

        const size_t lengthLimit{ 255 };

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() && result.empty() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonOk.area() ) );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );
            if ( isEditing ) {
                buttonVirtualKB->drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonVirtualKB->area() ) );
            }

            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                break;
            }

            const int listId = listbox.getCurrentId();

            const bool listboxEvent = listbox.QueueEventProcessing();

            bool isListboxSelected = listbox.isSelected();

            bool needRedraw = ( listId != listbox.getCurrentId() );
            bool listUpdated = false;

            if ( le.isKeyPressed( fheroes2::Key::KEY_DELETE ) && isListboxSelected ) {
                listbox.SetCurrent( listId );
                listbox.Redraw();

                std::string msg( _( "Are you sure you want to delete file:" ) );
                msg.append( "\n\n" );
                msg.append( System::GetFileName( listbox.GetCurrent().filename ) );

                if ( Dialog::YES == fheroes2::showStandardTextMessage( _( "Warning" ), msg, Dialog::YES | Dialog::NO ) ) {
                    if ( !System::Unlink( listbox.GetCurrent().filename ) ) {
                        ERROR_LOG( "Unable to delete file " << listbox.GetCurrent().filename )
                    }

                    listbox.RemoveSelected();

                    if ( lists.empty() ) {
                        listbox.Redraw();

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
                if ( isListboxSelected ) {
                    result = listbox.GetCurrent().filename;
                }
                else if ( !filename.empty() ) {
                    result = System::concatPath( Game::GetSaveDir(), filename + Game::GetSaveFileExtension() );
                }
            }
            else if ( le.MouseClickLeft( nameHeaderRoi ) && settings.getSaveFileSortingMethod() != SaveFileSortingMethod::FILENAME ) {
                listUpdated = true;
                needRedraw = switchFileSorting( false );
            }
            else if ( le.MouseClickLeft( dateHeaderRoi ) && settings.getSaveFileSortingMethod() != SaveFileSortingMethod::TIMESTAMP ) {
                listUpdated = true;
                needRedraw = switchFileSorting( true );
            }
            else if ( isEditing ) {
                assert( textInput != nullptr );

                bool prepareRedraw = false;

                if ( le.MouseClickLeft( buttonVirtualKB->area() ) || ( isInGameKeyboardRequired && le.MouseClickLeft( textInputRoi ) ) ) {
                    fheroes2::openVirtualKeyboard( filename, lengthLimit );

                    charInsertPos = filename.size();
                    prepareRedraw = true;

                    // Set the whole screen to redraw next time to properly restore image under the Virtual Keyboard dialog.
                    display.updateNextRenderRoi( { 0, 0, display.width(), display.height() } );
                }
                else if ( !filename.empty() && le.MouseClickLeft( textInputRoi ) ) {
                    if ( isListboxSelected ) {
                        // To proper position the text cursor update the textInput text and set cursor position to 0 to correspond
                        // the behavior of `fitToOneRow` of `fheroes2::Text` before calling `fheroes2::getTextInputCursorPosition()`.
                        textInput->set( filename, 0 );
                    }

                    const size_t newPos = textInput->getCursorInTextPosition( le.getMouseLeftButtonPressedPos() );

                    if ( newPos != charInsertPos || isListboxSelected ) {
                        charInsertPos = newPos;
                        prepareRedraw = true;
                    }
                }
                else if ( !listboxEvent && le.isAnyKeyPressed() ) {
                    const fheroes2::Key pressedKey = le.getPressedKeyValue();
                    if ( ( filename.size() < lengthLimit || pressedKey == fheroes2::Key::KEY_BACKSPACE || pressedKey == fheroes2::Key::KEY_DELETE )
                         && pressedKey != fheroes2::Key::KEY_UP && pressedKey != fheroes2::Key::KEY_DOWN ) {
                        charInsertPos = InsertKeySym( filename, charInsertPos, pressedKey, LocalEvent::getCurrentKeyModifiers() );
                        prepareRedraw = true;
                    }
                }

                if ( prepareRedraw ) {
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
            else if ( le.isMouseRightButtonPressedInArea( nameHeaderRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Sort by Name" ), _( "Click here if you wish to sort save files by their name." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( dateHeaderRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Sort by Date" ), _( "Click here if you you wish to sort save files by their last modified date." ), Dialog::ZERO );
            }

            // TODO: ListBox::SetCurrent() call should update needRedraw variable as it changes the internal UI view of the class.
            const bool needRedrawListbox = listUpdated || listbox.IsNeedRedraw();

            if ( isEditing && !needRedraw && !isListboxSelected && textInput->eventProcessing() ) {
                // Text input blinking cursor render is done in Save Game dialog when no file is selected
                // and when the render of the filename (with cursor) is not planned.
                display.render( textInput->getCursorArea() );
            }

            if ( !needRedraw && !needRedrawListbox ) {
                continue;
            }

            if ( needRedraw ) {
                if ( isListboxSelected ) {
                    const std::string selectedFileName = System::GetStem( listbox.GetCurrent().filename );
                    if ( lastSelectedSaveFileName != selectedFileName ) {
                        lastSelectedSaveFileName = selectedFileName;
                        filename = selectedFileName;
                        charInsertPos = filename.size();

                        buttonOkDisabler();
                    }

                    textInputBackground.restore();
                    dateBackground.restore();

                    redrawTextInputField( filename, textInputRoi, display );
                    redrawDateTime( display, listbox.GetCurrent().timestamp, dateTimeOffsetX, textInputRoi.y + 4, fheroes2::FontType::normalYellow() );
                }
                else if ( isEditing ) {
                    // Empty last selected save file name so that we can replace the input field's name if we select the same save file again.
                    // But when loading (i.e. isEditing == false), this doesn't matter since we cannot write to the input field
                    lastSelectedSaveFileName.clear();

                    dateBackground.restore();
                    textInput->draw( filename, static_cast<int32_t>( charInsertPos ) );
                    redrawDateTime( display, std::time( nullptr ), dateTimeOffsetX, textInputRoi.y + 4, fheroes2::FontType::normalWhite() );
                }
            }

            if ( needRedrawListbox ) {
                listbox.Redraw();
                display.render( dialogArea );
            }
            else {
                display.render( textInputAndDateROI );
            }
        }

        const SaveFileSortingMethod lastFileSortingMethod = settings.getSaveFileSortingMethod();

        if ( lastFileSortingMethod != fileSortingMethod ) {
            settings.Save( Settings::configFileName );
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
