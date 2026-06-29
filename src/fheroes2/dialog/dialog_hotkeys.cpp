/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2026                                             *
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

#include "dialog_hotkeys.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    const int32_t keyDescriptionLength = 200;
    const int32_t hotKeyLength = 120;
    const int32_t categoryLength = 100;

    struct CategorySelection
    {
        std::optional<Game::HotKeyCategory> category;
        fheroes2::Rect roi;
    };

    const char * getHotKeyCategoryDisplayName( const Game::HotKeyCategory category )
    {
        switch ( category ) {
        case Game::HotKeyCategory::DEFAULT:
            return "Default";
        case Game::HotKeyCategory::GLOBAL:
            return "Global";
        case Game::HotKeyCategory::BATTLE:
            return "Battle";
        case Game::HotKeyCategory::TOWN:
            return "Town";
        case Game::HotKeyCategory::ARMY:
            return "Army";
        default:
            return Game::getHotKeyCategoryName( category );
        }
    }

    void drawHotKeyCategories( const fheroes2::Rect & categoryRoi, std::vector<CategorySelection> & categorySelections,
                               const std::optional<Game::HotKeyCategory> selectedCategory )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        categorySelections.clear();

        int32_t offsetY = categoryRoi.y + 4;

        const fheroes2::FontType allFontType = selectedCategory ? fheroes2::FontType::normalWhite() : fheroes2::FontType::normalYellow();
        const fheroes2::Text allText( _( "All" ), allFontType );
        allText.draw( categoryRoi.x + 4, offsetY, display );

        categorySelections.push_back( { std::nullopt, { categoryRoi.x, offsetY, categoryRoi.width, allText.height() + 4 } } );

        offsetY += allText.height() + 4;

        for ( uint8_t i = static_cast<uint8_t>( Game::HotKeyCategory::DEFAULT ); i <= static_cast<uint8_t>( Game::HotKeyCategory::EDITOR ); ++i ) {
            const auto category = static_cast<Game::HotKeyCategory>( i );

            const bool isSelected = selectedCategory && category == *selectedCategory;
            const fheroes2::FontType fontType = isSelected ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite();

            const fheroes2::Text categoryText( _( getHotKeyCategoryDisplayName( category ) ), fontType );
            categoryText.draw( categoryRoi.x + 4, offsetY, display );

            categorySelections.push_back( { category, { categoryRoi.x, offsetY, categoryRoi.width, categoryText.height() + 4 } } );

            offsetY += categoryText.height() + 4;
        }
    }

    class HotKeyElement : public fheroes2::DialogElement
    {
    public:
        HotKeyElement( const fheroes2::Key key, fheroes2::Image & output )
            : _restorer( output, 0, 0, 0, 0 )
            , _key( key )
        {
            // Text always occupies the whole width of the dialog.
            _area = { fheroes2::boxAreaWidthPx, fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) };
        }

        ~HotKeyElement() override = default;

        void draw( fheroes2::Image & output, const fheroes2::Point & offset ) const override
        {
            _restorer.restore();

            fheroes2::MultiFontText text;
            text.add( fheroes2::Text{ _( "Hotkey: " ), fheroes2::FontType::normalYellow() } );
            text.add( fheroes2::Text{ StringUpper( KeySymGetName( _key ) ), fheroes2::FontType::normalWhite() } );

            _restorer.update( offset.x, offset.y, fheroes2::boxAreaWidthPx, text.height( fheroes2::boxAreaWidthPx ) );

            text.draw( offset.x, offset.y, fheroes2::boxAreaWidthPx, output );
        }

        void processEvents( const fheroes2::Point & /*offset*/ ) const override
        {
            const LocalEvent & le = LocalEvent::Get();
            if ( le.isAnyKeyPressed() ) {
                _key = le.getPressedKeyValue();
                _keyChanged = true;
            }
        }

        void showPopup( const int /*buttons*/ ) const override
        {
            assert( 0 );
        }

        bool update( fheroes2::Image & output, const fheroes2::Point & offset ) const override
        {
            if ( _keyChanged ) {
                _keyChanged = false;
                draw( output, offset );
                return true;
            }

            return false;
        }

        void reset()
        {
            _restorer.reset();
        }

        fheroes2::Key getKey() const
        {
            return _key;
        }

    private:
        mutable fheroes2::ImageRestorer _restorer;
        mutable fheroes2::Key _key;
        mutable bool _keyChanged{ false };
    };

    class HotKeyList : public Interface::ListBox<std::pair<Game::HotKeyEvent, Game::HotKeyCategory>>
    {
    public:
        using Interface::ListBox<std::pair<Game::HotKeyEvent, Game::HotKeyCategory>>::ListBox;

        using Interface::ListBox<std::pair<Game::HotKeyEvent, Game::HotKeyCategory>>::ActionListSingleClick;
        using Interface::ListBox<std::pair<Game::HotKeyEvent, Game::HotKeyCategory>>::ActionListPressRight;
        using Interface::ListBox<std::pair<Game::HotKeyEvent, Game::HotKeyCategory>>::ActionListDoubleClick;

        void RedrawItem( const std::pair<Game::HotKeyEvent, Game::HotKeyCategory> & hotKeyEvent, int32_t offsetX, int32_t offsetY, bool current ) override
        {
            fheroes2::Display & display = fheroes2::Display::instance();

            const fheroes2::FontType fontType = current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite();

            offsetY += 2;

            fheroes2::Text name( _( Game::getHotKeyEventNameByEventId( hotKeyEvent.first ) ), fontType );
            name.fitToOneRow( keyDescriptionLength );
            name.draw( offsetX + 4, offsetY, display );

            fheroes2::Text hotkey( Game::getHotKeyNameByEventId( hotKeyEvent.first ), fontType );
            hotkey.fitToOneRow( hotKeyLength );
            hotkey.draw( offsetX + keyDescriptionLength + 9, offsetY, hotKeyLength, display );
        }

        void RedrawBackground( const fheroes2::Point & /* unused */ ) override
        {
            _listBackground->restore();
        }

        void ActionCurrentUp() override {}
        void ActionCurrentDn() override {}

        void ActionListSingleClick( std::pair<Game::HotKeyEvent, Game::HotKeyCategory> & /*unused*/ ) override {}

        void ActionListPressRight( std::pair<Game::HotKeyEvent, Game::HotKeyCategory> & hotKeyEvent ) override
        {
            fheroes2::MultiFontText title;

            title.add( fheroes2::Text{ _( "Category: " ), fheroes2::FontType::normalYellow() } );
            title.add( fheroes2::Text{ _( Game::getHotKeyCategoryName( hotKeyEvent.second ) ), fheroes2::FontType::normalWhite() } );
            title.add( fheroes2::Text{ "\n\n", fheroes2::FontType::normalWhite() } );
            title.add( fheroes2::Text{ _( "Event: " ), fheroes2::FontType::normalYellow() } );
            title.add( fheroes2::Text{ _( Game::getHotKeyEventNameByEventId( hotKeyEvent.first ) ), fheroes2::FontType::normalWhite() } );
            title.add( fheroes2::Text{ "\n\n", fheroes2::FontType::normalWhite() } );
            title.add( fheroes2::Text{ _( "Hotkey: " ), fheroes2::FontType::normalYellow() } );
            title.add( fheroes2::Text{ Game::getHotKeyNameByEventId( hotKeyEvent.first ), fheroes2::FontType::normalWhite() } );

            fheroes2::showMessage( fheroes2::Text{}, title, Dialog::ZERO );
        }

        void ActionListDoubleClick( std::pair<Game::HotKeyEvent, Game::HotKeyCategory> & hotKeyEvent ) override
        {
            HotKeyElement hotKeyUI( Game::getHotKeyForEvent( hotKeyEvent.first ), fheroes2::Display::instance() );

            const fheroes2::Key okayEventKey = Game::getHotKeyForEvent( Game::HotKeyEvent::DEFAULT_OKAY );
            const fheroes2::Key cancelEventKey = Game::getHotKeyForEvent( Game::HotKeyEvent::DEFAULT_CANCEL );
            const fheroes2::Key fullscreenEventKey = Game::getHotKeyForEvent( Game::HotKeyEvent::GLOBAL_TOGGLE_FULLSCREEN );
            const fheroes2::Key textSupportModeEventKey = Game::getHotKeyForEvent( Game::HotKeyEvent::GLOBAL_TOGGLE_TEXT_SUPPORT_MODE );

            Game::setHotKeyForEvent( Game::HotKeyEvent::DEFAULT_OKAY, fheroes2::Key::NONE );
            Game::setHotKeyForEvent( Game::HotKeyEvent::DEFAULT_CANCEL, fheroes2::Key::NONE );
            Game::setHotKeyForEvent( Game::HotKeyEvent::GLOBAL_TOGGLE_FULLSCREEN, fheroes2::Key::NONE );
            Game::setHotKeyForEvent( Game::HotKeyEvent::GLOBAL_TOGGLE_TEXT_SUPPORT_MODE, fheroes2::Key::NONE );

            fheroes2::MultiFontText title;

            title.add( fheroes2::Text{ _( "Category: " ), fheroes2::FontType::normalYellow() } );
            title.add( fheroes2::Text{ _( Game::getHotKeyCategoryName( hotKeyEvent.second ) ), fheroes2::FontType::normalWhite() } );
            title.add( fheroes2::Text{ "\n\n", fheroes2::FontType::normalWhite() } );
            title.add( fheroes2::Text{ _( "Event: " ), fheroes2::FontType::normalYellow() } );
            title.add( fheroes2::Text{ _( Game::getHotKeyEventNameByEventId( hotKeyEvent.first ) ), fheroes2::FontType::normalWhite() } );

            const int returnValue = fheroes2::showMessage( fheroes2::Text{}, title, Dialog::OK | Dialog::CANCEL, { &hotKeyUI } );

            Game::setHotKeyForEvent( Game::HotKeyEvent::DEFAULT_OKAY, okayEventKey );
            Game::setHotKeyForEvent( Game::HotKeyEvent::DEFAULT_CANCEL, cancelEventKey );
            Game::setHotKeyForEvent( Game::HotKeyEvent::GLOBAL_TOGGLE_FULLSCREEN, fullscreenEventKey );
            Game::setHotKeyForEvent( Game::HotKeyEvent::GLOBAL_TOGGLE_TEXT_SUPPORT_MODE, textSupportModeEventKey );

            hotKeyUI.reset();

            if ( returnValue == Dialog::CANCEL ) {
                return;
            }

            Game::setHotKeyForEvent( hotKeyEvent.first, hotKeyUI.getKey() );
            Game::HotKeySave();
        }

        void initListBackgroundRestorer( fheroes2::Rect roi )
        {
            _listBackground = std::make_unique<fheroes2::ImageRestorer>( fheroes2::Display::instance(), roi.x, roi.y, roi.width, roi.height );
        }

    private:
        std::unique_ptr<fheroes2::ImageRestorer> _listBackground;
    };
}

namespace fheroes2
{
    void openHotkeysDialog()
    {
        const CursorRestorer cursorRestorer( true, ::Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::StandardWindow background( categoryLength + keyDescriptionLength + hotKeyLength + 8 + 75, std::min<int32_t>( display.height() - 100, 520 ), true,
                                             display );

        const fheroes2::Rect roi( background.activeArea() );
        const fheroes2::Rect listRoi( roi.x + 24, roi.y + 50, categoryLength + keyDescriptionLength + hotKeyLength + 8, roi.height - 88 );
        const fheroes2::Rect categoryRoi( listRoi.x, listRoi.y, categoryLength, listRoi.height );
        const fheroes2::Rect hotKeyRoi( listRoi.x + categoryLength, listRoi.y, keyDescriptionLength + hotKeyLength + 8, listRoi.height );

        const fheroes2::Text title( _( "Hot Keys:" ), fheroes2::FontType::normalYellow() );
        title.draw( roi.x + ( roi.width - title.width() ) / 2, roi.y + 16, display );

        const fheroes2::Text categoryHeader( _( "Categories:" ), fheroes2::FontType::normalWhite() );
        categoryHeader.draw( categoryRoi.x + ( categoryRoi.width - categoryHeader.width() ) / 2, roi.y + 29, display );

        background.applyTextBackgroundShading( categoryRoi );
        background.applyTextBackgroundShading( { hotKeyRoi.x, hotKeyRoi.y, keyDescriptionLength + 8, hotKeyRoi.height } );
        background.applyTextBackgroundShading( { hotKeyRoi.x + keyDescriptionLength + 8, hotKeyRoi.y, hotKeyLength, hotKeyRoi.height } );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        fheroes2::Button buttonOk;
        const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        background.renderButton( buttonOk, buttonOkIcn, 0, 1, { 0, 7 }, StandardWindow::Padding::BOTTOM_CENTER );

        HotKeyList listbox( roi.getPosition() );
        listbox.initListBackgroundRestorer( hotKeyRoi );
        listbox.SetAreaItems( { hotKeyRoi.x, hotKeyRoi.y + 3, hotKeyRoi.width - 3, hotKeyRoi.height - 4 } );

        int32_t scrollbarOffsetX = roi.x + roi.width - 35;
        background.renderScrollbarBackground( { scrollbarOffsetX, hotKeyRoi.y, hotKeyRoi.width, hotKeyRoi.height }, isEvilInterface );

        const int32_t topPartHeight = 19;
        const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;
        ++scrollbarOffsetX;

        listbox.SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX, hotKeyRoi.y + 1 } );
        listbox.SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX, hotKeyRoi.y + hotKeyRoi.height - 15 } );
        listbox.setScrollBarArea( { scrollbarOffsetX + 2, hotKeyRoi.y + topPartHeight, 10, hotKeyRoi.height - 2 * topPartHeight } );
        listbox.setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );
        listbox.SetAreaMaxItems( ( hotKeyRoi.height - 7 ) / fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) );

        std::vector<std::pair<Game::HotKeyEvent, Game::HotKeyCategory>> hotKeyEvents = Game::getAllHotKeyEvents();
        std::vector<std::pair<Game::HotKeyEvent, Game::HotKeyCategory>> displayedHotKeyEvents = hotKeyEvents;

        std::optional<Game::HotKeyCategory> selectedCategory;

        std::vector<CategorySelection> categorySelections;
        drawHotKeyCategories( categoryRoi, categorySelections, selectedCategory );

        listbox.SetListContent( displayedHotKeyEvents );
        listbox.updateScrollBarImage();
        listbox.Redraw();

        display.render( background.totalArea() );

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonOk.area() ) );

            listbox.QueueEventProcessing();

            std::optional<Game::HotKeyCategory> clickedCategory;
            bool isCategoryClicked = false;

            for ( const CategorySelection & selection : categorySelections ) {
                if ( le.MouseClickLeft( selection.roi ) ) {
                    isCategoryClicked = true;
                    clickedCategory = selection.category;
                    break;
                }
            }

            if ( isCategoryClicked && clickedCategory != selectedCategory ) {
                selectedCategory = clickedCategory;

                displayedHotKeyEvents.clear();

                if ( !selectedCategory ) {
                    displayedHotKeyEvents = hotKeyEvents;
                }
                else {
                    for ( const auto & hotKeyEvent : hotKeyEvents ) {
                        if ( hotKeyEvent.second == *selectedCategory ) {
                            displayedHotKeyEvents.push_back( hotKeyEvent );
                        }
                    }
                }

                listbox.SetListContent( displayedHotKeyEvents );

                drawHotKeyCategories( categoryRoi, categorySelections, selectedCategory );

                listbox.updateScrollBarImage();
                listbox.Redraw();
                display.render( roi );

                continue;
            }

            if ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyCloseWindow() ) {
                return;
            }

            if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Exit this menu." ), Dialog::ZERO );

                continue;
            }

            if ( !listbox.IsNeedRedraw() ) {
                continue;
            }

            listbox.Redraw();
            display.render( roi );
        }
    }
}
