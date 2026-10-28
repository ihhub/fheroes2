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
#include <type_traits>
#include <utility>
#include <vector>

#include "cursor.h"
#include "dialog.h"
#include "game_assets.h"
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
    constexpr int32_t minKeyDescriptionWidth = 260;
    constexpr int32_t minHotKeyWidth = 120;
    constexpr int32_t minCategoryWidth = 170;
    constexpr int32_t dialogExtraWidth = 90;

    struct CategorySelection final
    {
        std::optional<Game::HotKeyCategory> category;
        fheroes2::Rect roi;
    };

    std::string getCategoryName( const std::optional<Game::HotKeyCategory> category )
    {
        return ( category == std::nullopt ) ? _( "hotkeys|All Categories" ) : _( Game::getHotKeyCategoryName( *category ) );
    }

    void drawHotKeyCategories( const fheroes2::Rect & categoryRoi, const std::optional<Game::HotKeyCategory> selectedCategory, const int32_t categoryWidth,
                               std::vector<CategorySelection> & categorySelections )
    {
        const bool needInitCategories = categorySelections.empty();

        int32_t offsetY = categoryRoi.y + 4;

        fheroes2::Display & display = fheroes2::Display::instance();

        const auto renderCategotyText = [needInitCategories, selectedCategory, categoryWidth, &categoryRoi, &categorySelections, &display,
                                         &offsetY]( const std::optional<Game::HotKeyCategory> category ) {
            const bool isSelected = ( category == selectedCategory );

            const fheroes2::FontType fontType = isSelected ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite();

            fheroes2::Text categoryText( getCategoryName( category ), fontType );
            categoryText.fitToOneRow( categoryWidth );
            categoryText.drawInRoi( categoryRoi.x + 4, offsetY, display, categoryRoi );

            const int32_t stepY = categoryText.height() + 4;

            if ( needInitCategories ) {
                categorySelections.push_back( { category, { categoryRoi.x, offsetY, categoryRoi.width, stepY } } );
            }

            offsetY += stepY;
        };

        renderCategotyText( std::nullopt );

        for ( uint8_t i = static_cast<uint8_t>( Game::HotKeyCategory::DEFAULT ); i <= static_cast<uint8_t>( Game::HotKeyCategory::EDITOR ); ++i ) {
            const Game::HotKeyCategory category = static_cast<Game::HotKeyCategory>( i );
            // If Editor is not supported by the given assets then do not show "Editor" category.
            if ( ( category == Game::HotKeyCategory::EDITOR ) && !Settings::Get().isPriceOfLoyaltySupported() ) {
                continue;
            }

            renderCategotyText( static_cast<Game::HotKeyCategory>( i ) );
        }
    }

    class HotKeyElement final : public fheroes2::DialogElement
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

        // Never call this method as a custom image has nothing to popup.
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

    class HotKeyList final : public Interface::ListBox<std::pair<Game::HotKeyEvent, Game::HotKeyCategory>>
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
            name.fitToOneRow( _keyDescriptionWidth );
            name.draw( offsetX + 4, offsetY, display );

            fheroes2::Text hotkey( Game::getHotKeyNameByEventId( hotKeyEvent.first ), fontType );
            hotkey.fitToOneRow( _hotKeyWidth );
            hotkey.draw( offsetX + _keyDescriptionWidth + 9, offsetY, _hotKeyWidth, display );
        }

        void RedrawBackground( const fheroes2::Point & /* unused */ ) override
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

        void ActionListSingleClick( std::pair<Game::HotKeyEvent, Game::HotKeyCategory> & /*unused*/ ) override
        {
            // Do nothing.
        }

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

            // Okay and Cancel events are special cases as they are used in dialogs. By default we need to disable these events to allow to be set any key for an event.
            // Global events (that work on all screens) must be disabled as well.
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

            // To avoid UI issues we need to reset restorer manually.
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

        void setKeyDescriptionWidth( const int32_t keyDescriptionWidth )
        {
            _keyDescriptionWidth = keyDescriptionWidth;
        }

        void setHotKeyWidth( const int32_t hotKeyWidth )
        {
            _hotKeyWidth = hotKeyWidth;
        }

    private:
        std::unique_ptr<fheroes2::ImageRestorer> _listBackground;
        int32_t _keyDescriptionWidth{ 0 };
        int32_t _hotKeyWidth{ 0 };
    };
}

namespace fheroes2
{
    void openHotkeysDialog()
    {
        // Setup cursor.
        const CursorRestorer cursorRestorer( true, ::Cursor::POINTER );

        std::vector<std::pair<Game::HotKeyEvent, Game::HotKeyCategory>> allHotKeyEvents = Game::getAllHotKeyEvents();
        std::vector<std::pair<Game::HotKeyEvent, Game::HotKeyCategory>> currentCategoryHotKeyEvents;

        // Determine the maximum needed widths of the lists in the dialog by their content.
        int32_t maxCategoryWidth = minCategoryWidth;
        using categoryUnderlyingType = std::underlying_type_t<Game::HotKeyCategory>;
        for ( categoryUnderlyingType i = static_cast<categoryUnderlyingType>( Game::HotKeyCategory::DEFAULT );
              i <= static_cast<categoryUnderlyingType>( Game::HotKeyCategory::EDITOR ); ++i ) {
            const Game::HotKeyCategory category = static_cast<Game::HotKeyCategory>( i );
            // If Editor is not supported by the given assets then do not show "Editor" category.
            if ( ( category == Game::HotKeyCategory::EDITOR ) && !Settings::Get().isPriceOfLoyaltySupported() ) {
                continue;
            }

            fheroes2::Text name( getCategoryName( category ), fheroes2::FontType::normalWhite() );
            maxCategoryWidth = std::max( maxCategoryWidth, name.width() );
        }

        int32_t maxKeyDescriptionWidth = minKeyDescriptionWidth;
        for ( const auto & [event, category] : allHotKeyEvents ) {
            fheroes2::Text name( _( Game::getHotKeyEventNameByEventId( event ) ), fheroes2::FontType::normalWhite() );
            maxKeyDescriptionWidth = std::max( maxKeyDescriptionWidth, name.width() );
        }

        int32_t maxHotKeyWidth = minHotKeyWidth;
        using keyUnderlyingType = std::underlying_type_t<fheroes2::Key>;
        for ( keyUnderlyingType k = 0; k < static_cast<keyUnderlyingType>( fheroes2::Key::LAST_KEY ); ++k ) {
            std::string text = fheroes2::KeySymGetName( static_cast<fheroes2::Key>( k ) );
            if ( !text.empty() ) {
                fheroes2::Text name( std::move( text ), fheroes2::FontType::normalWhite() );
                maxHotKeyWidth = std::max( maxHotKeyWidth, name.width() );
            }
        }

        fheroes2::Display & display = fheroes2::Display::instance();

        // Dialog width is capped with the current screen height minus 150 pixels.
        const int32_t maxDialogWidth = std::max<int32_t>( display.width() - 150, fheroes2::Display::DEFAULT_WIDTH );

        // We calculate how much the dialog can be widened and then proportionally resize each list up to the maximum needed width to fit all texts.
        const int32_t listsAllowedWidening = maxDialogWidth - fheroes2::Display::DEFAULT_WIDTH;
        const int32_t categoryMaxWidening = maxCategoryWidth - minCategoryWidth;
        const int32_t keyDescriptionMaxWidening = maxKeyDescriptionWidth - minKeyDescriptionWidth;
        const int32_t hotKeyMaxWidening = maxHotKeyWidth - minHotKeyWidth;
        const int32_t totalListsMaxWidening = categoryMaxWidening + keyDescriptionMaxWidening + hotKeyMaxWidening;
        const float wideningCoefficient = std::min( 1.0F, static_cast<float>( listsAllowedWidening ) / static_cast<float>( totalListsMaxWidening ) );
        const int32_t categoryWidth = minCategoryWidth + static_cast<int32_t>( static_cast<float>( categoryMaxWidening ) * wideningCoefficient );
        const int32_t keyDescriptionWidth = minKeyDescriptionWidth + static_cast<int32_t>( static_cast<float>( keyDescriptionMaxWidening ) * wideningCoefficient );
        const int32_t hotKeyWidth = minHotKeyWidth + static_cast<int32_t>( static_cast<float>( hotKeyMaxWidening ) * wideningCoefficient );

        // Dialog height is capped with the current screen height minus 100 pixels.
        fheroes2::StandardWindow background( categoryWidth + keyDescriptionWidth + hotKeyWidth + dialogExtraWidth,
                                             std::max<int32_t>( display.height() - 100, fheroes2::Display::DEFAULT_HEIGHT ), true, display );

        const fheroes2::Rect roi( background.activeArea() );
        const fheroes2::Rect listRoi( roi.x + 24, roi.y + 37, categoryWidth + keyDescriptionWidth + hotKeyWidth + 16, roi.height - 75 );
        const fheroes2::Rect categoryRoi( listRoi.x, listRoi.y, categoryWidth + 8, listRoi.height );
        const fheroes2::Rect hotKeyRoi( listRoi.x + categoryWidth + 8, listRoi.y, keyDescriptionWidth + hotKeyWidth + 8, listRoi.height );

        const fheroes2::Text title( _( "Hot Keys:" ), fheroes2::FontType::normalYellow() );
        title.draw( roi.x + ( roi.width - title.width() ) / 2, roi.y + 16, display );

        background.applyTextBackgroundShading( categoryRoi );
        background.applyTextBackgroundShading( { hotKeyRoi.x, hotKeyRoi.y, keyDescriptionWidth + 8, hotKeyRoi.height } );
        background.applyTextBackgroundShading( { hotKeyRoi.x + keyDescriptionWidth + 8, hotKeyRoi.y, hotKeyWidth, hotKeyRoi.height } );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        // Prepare OKAY button and render its shadow.
        fheroes2::Button buttonOk;
        const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        background.renderButton( buttonOk, buttonOkIcn, 0, 1, { 0, 7 }, StandardWindow::Padding::BOTTOM_CENTER );

        HotKeyList listbox( roi.getPosition() );
        listbox.setKeyDescriptionWidth( keyDescriptionWidth );
        listbox.setHotKeyWidth( hotKeyWidth );
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
        listbox.setScrollBarImage( Assets::getImage( listIcnId, 4 ) );
        listbox.SetAreaMaxItems( ( hotKeyRoi.height - 7 ) / fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) );

        std::optional<Game::HotKeyCategory> selectedCategory;

        std::vector<CategorySelection> categorySelections;
        drawHotKeyCategories( categoryRoi, selectedCategory, categoryWidth, categorySelections );

        listbox.SetListContent( allHotKeyEvents );
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

                if ( le.isMouseRightButtonPressedInArea( selection.roi ) ) {
                    fheroes2::showStandardTextMessage( _( "Select hotkeys category" ), _( "Click here to select:\n" ) + getCategoryName( selection.category ),
                                                       Dialog::ZERO );

                    break;
                }
            }

            if ( isCategoryClicked && clickedCategory != selectedCategory ) {
                selectedCategory = clickedCategory;

                if ( selectedCategory == std::nullopt ) {
                    listbox.SetListContent( allHotKeyEvents );
                }
                else {
                    currentCategoryHotKeyEvents.clear();

                    for ( const auto & hotKeyEvent : allHotKeyEvents ) {
                        if ( hotKeyEvent.second == *selectedCategory ) {
                            currentCategoryHotKeyEvents.push_back( hotKeyEvent );
                        }
                    }

                    listbox.SetListContent( currentCategoryHotKeyEvents );
                }

                drawHotKeyCategories( categoryRoi, selectedCategory, categoryWidth, categorySelections );

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
