/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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

#include "editor_daily_events_window.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "editor_daily_event_spec_window.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "map_format_info.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    const int32_t elementOffset{ 9 };

    const fheroes2::Size eventsArea{ 500, 315 };

    const int32_t listAreaHeightDeduction{ 8 };

    class EventListBox final : public Interface::ListBox<Maps::Map_Format::DailyEvent>
    {
    public:
        using Interface::ListBox<Maps::Map_Format::DailyEvent>::ActionListDoubleClick;
        using Interface::ListBox<Maps::Map_Format::DailyEvent>::ActionListSingleClick;
        using Interface::ListBox<Maps::Map_Format::DailyEvent>::ActionListPressRight;

        EventListBox( const fheroes2::Point & pt, const fheroes2::SupportedLanguage language )
            : ListBox( pt )
            , _language( language )
        {
            // Do nothing.
        }

        void RedrawItem( const Maps::Map_Format::DailyEvent & event, int32_t posX, int32_t posY, bool current ) override
        {
            std::string message = _( "Day: %{day}" );
            StringReplace( message, "%{day}", std::to_string( event.firstOccurrenceDay ) );

            if ( !event.message.empty() ) {
                message += " \"";
            }

            fheroes2::Text dateText{ message, ( current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() ) };
            const int32_t dateLength = dateText.width();
            assert( dateLength < eventsArea.width - 10 );

            fheroes2::MultiFontText text;
            text.add( std::move( dateText ) );

            if ( !event.message.empty() ) {
                fheroes2::Text messageText{ event.message + '\"', ( current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() ), _language };
                messageText.fitToOneRow( eventsArea.width - 10 - dateLength );
                text.add( std::move( messageText ) );
            }

            text.draw( posX + 5, posY + 5, fheroes2::Display::instance() );
        }

        void RedrawBackground( const fheroes2::Point & /*unused*/ ) override
        {
            _listBackground->restore();
        }

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( Maps::Map_Format::DailyEvent & /*unused*/ ) override
        {
            _isDoubleClicked = true;
        }

        bool isDoubleClicked() const
        {
            return _isDoubleClicked;
        }

        void resetDoubleClickedState()
        {
            _isDoubleClicked = false;
        }

        void ActionListSingleClick( Maps::Map_Format::DailyEvent & /*unused*/ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( Maps::Map_Format::DailyEvent & /*unused*/ ) override
        {
            // Do nothing.
        }

        int getCurrentId() const
        {
            return _currentId;
        }

        void initListBackgroundRestorer( fheroes2::Rect roi )
        {
            _listBackground = std::make_unique<fheroes2::ImageRestorer>( fheroes2::Display::instance(), roi.x, roi.y, roi.width, roi.height );
        }

        void updateScrollBarImage()
        {
            const int32_t scrollBarWidth = _scrollbar.width();

            setScrollBarImage( fheroes2::generateScrollbarSlider( _scrollbar, false, _scrollbar.getArea().height, VisibleItemCount(), _size(),
                                                                  { 0, 0, scrollBarWidth, 8 }, { 0, 7, scrollBarWidth, 8 } ) );
            _scrollbar.moveToIndex( _topId );
        }

    private:
        std::unique_ptr<fheroes2::ImageRestorer> _listBackground;

        bool _isDoubleClicked{ false };

        const fheroes2::SupportedLanguage _language;
    };

    void sortEvents( std::vector<Maps::Map_Format::DailyEvent> & dailyEvents )
    {
        std::sort( dailyEvents.begin(), dailyEvents.end(),
                   []( const auto & first, const auto & second ) { return first.firstOccurrenceDay < second.firstOccurrenceDay; } );
    }
}

namespace Editor
{
    bool openDailyEventsWindow( std::vector<Maps::Map_Format::DailyEvent> & dailyEvents, const uint8_t humanPlayerColors, const uint8_t computerPlayerColors,
                                const fheroes2::SupportedLanguage language )
    {
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::StandardWindow background{ eventsArea.width + 50, eventsArea.height + 100, true, display };

        const fheroes2::Rect windowArea{ background.activeArea() };

        int32_t offsetY = windowArea.y + elementOffset;

        const fheroes2::Text title( _( "Daily Events" ), fheroes2::FontType::normalYellow() );
        title.draw( windowArea.x + ( windowArea.width - title.width() ) / 2, offsetY, display );

        offsetY += title.height() + elementOffset;

        const fheroes2::Rect eventsRoi{ windowArea.x + elementOffset, offsetY, eventsArea.width, eventsArea.height };
        background.applyTextBackgroundShading( eventsRoi );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        EventListBox eventList( eventsRoi.getPosition(), language );
        eventList.initListBackgroundRestorer( eventsRoi );

        eventList.SetAreaItems( { eventsRoi.x, eventsRoi.y, eventsRoi.width, eventsRoi.height - listAreaHeightDeduction } );

        int32_t scrollbarOffsetX = eventsRoi.x + eventsRoi.width + 5;
        background.renderScrollbarBackground( { scrollbarOffsetX, eventsRoi.y, eventsRoi.width, eventsRoi.height }, isEvilInterface );

        const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;
        const int32_t topPartHeight = 19;
        ++scrollbarOffsetX;

        eventList.SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX, eventsRoi.y + 1 } );
        eventList.SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX, eventsRoi.y + eventsRoi.height - 15 } );
        eventList.setScrollBarArea( { scrollbarOffsetX + 2, eventsRoi.y + topPartHeight, 10, eventsRoi.height - 2 * topPartHeight } );
        eventList.setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );
        eventList.SetAreaMaxItems( 10 );
        eventList.SetListContent( dailyEvents );
        eventList.updateScrollBarImage();

        eventList.Redraw();

        const int minibuttonIcnId = isEvilInterface ? ICN::CELLWIN_EVIL : ICN::CELLWIN;

        const fheroes2::Sprite & buttonImage = fheroes2::AGG::GetICN( minibuttonIcnId, 13 );
        const int32_t buttonWidth = buttonImage.width();
        const int32_t buttonOffset = ( eventsArea.width - 3 * buttonWidth ) / 2 + buttonWidth;

        fheroes2::Button buttonAdd( eventsRoi.x, eventsRoi.y + eventsRoi.height + 5, minibuttonIcnId, 13, 14 );
        buttonAdd.draw();

        fheroes2::Button buttonEdit( eventsRoi.x + buttonOffset, eventsRoi.y + eventsRoi.height + 5, minibuttonIcnId, 15, 16 );
        buttonEdit.draw();

        fheroes2::Button buttonDelete( eventsRoi.x + eventsArea.width - buttonWidth, eventsRoi.y + eventsRoi.height + 5, minibuttonIcnId, 17, 18 );
        buttonDelete.draw();

        // Prepare OKAY and CANCEL buttons and render their shadows.
        fheroes2::Button buttonOk;
        fheroes2::Button buttonCancel;

        background.renderOkayCancelButtons( buttonOk, buttonCancel, isEvilInterface );

        display.render( background.totalArea() );

        bool isRedrawNeeded = false;

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedInArea( buttonOk.area() ) );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) );
            buttonAdd.drawOnState( le.isMouseLeftButtonPressedInArea( buttonAdd.area() ) );
            buttonEdit.drawOnState( le.isMouseLeftButtonPressedInArea( buttonEdit.area() ) );
            buttonDelete.drawOnState( le.isMouseLeftButtonPressedInArea( buttonDelete.area() ) );

            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                break;
            }

            if ( buttonOk.isEnabled() && ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) ) {
                return true;
            }

            eventList.QueueEventProcessing();

            if ( eventList.IsNeedRedraw() ) {
                eventList.Redraw();
                isRedrawNeeded = true;
            }

            if ( le.MouseClickLeft( buttonAdd.area() ) ) {
                Maps::Map_Format::DailyEvent temp;
                if ( editDailyEvent( temp, humanPlayerColors, computerPlayerColors, language ) ) {
                    dailyEvents.emplace_back( std::move( temp ) );

                    sortEvents( dailyEvents );

                    eventList.updateScrollBarImage();
                    eventList.Redraw();
                }

                isRedrawNeeded = true;
            }
            else if ( eventList.isDoubleClicked() || le.MouseClickLeft( buttonEdit.area() ) ) {
                if ( eventList.getCurrentId() < 0 ) {
                    continue;
                }

                eventList.resetDoubleClickedState();

                Maps::Map_Format::DailyEvent temp = eventList.GetCurrent();
                if ( editDailyEvent( temp, humanPlayerColors, computerPlayerColors, language ) ) {
                    eventList.GetCurrent() = std::move( temp );

                    sortEvents( dailyEvents );
                    eventList.Redraw();
                }

                isRedrawNeeded = true;
            }
            else if ( le.MouseClickLeft( buttonDelete.area() ) ) {
                if ( eventList.getCurrentId() < 0 ) {
                    continue;
                }

                eventList.RemoveSelected();
                sortEvents( dailyEvents );

                eventList.updateScrollBarImage();
                eventList.Redraw();
                isRedrawNeeded = true;
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to save the events." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonAdd.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Add Event" ), _( "Add an additional event." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonEdit.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Edit Event" ), _( "Edit an existing event." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonDelete.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Delete Event" ), _( "Delete an existing event." ), Dialog::ZERO );
            }

            if ( isRedrawNeeded ) {
                isRedrawNeeded = false;

                display.render( windowArea );
            }
        }

        return false;
    }
}
