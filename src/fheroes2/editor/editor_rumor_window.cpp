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

#include "editor_rumor_window.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

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
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    const int32_t elementOffset{ 9 };

    const fheroes2::Size rumorArea{ 550, 300 };

    const int32_t listAreaHeightDeduction{ 8 };

    const size_t longestRumor{ 200 };

    class RumorListBox final : public Interface::ListBox<std::string>
    {
    public:
        using Interface::ListBox<std::string>::ActionListDoubleClick;
        using Interface::ListBox<std::string>::ActionListSingleClick;
        using Interface::ListBox<std::string>::ActionListPressRight;

        RumorListBox( const fheroes2::Point & pt, const fheroes2::SupportedLanguage language )
            : ListBox( pt )
            , _language( language )
        {
            // Do nothing.
        }

        void RedrawItem( const std::string & rumor, int32_t posX, int32_t posY, bool current ) override
        {
            fheroes2::Text text{ rumor, ( current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() ), _language };
            text.fitToOneRow( rumorArea.width - 10 );
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

        void ActionListDoubleClick( std::string & /*unused*/ ) override
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

        void ActionListSingleClick( std::string & /*unused*/ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( std::string & /*unused*/ ) override
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
}

namespace Editor
{
    bool openRumorWindow( std::vector<std::string> & rumors, const fheroes2::SupportedLanguage language )
    {
        // Remove all empty rumors.
        assert( std::all_of( rumors.begin(), rumors.end(), []( const auto & rumor ) { return !rumor.empty(); } ) );

        rumors.erase( std::remove_if( rumors.begin(), rumors.end(), []( const auto & rumor ) { return rumor.empty(); } ), rumors.end() );

        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::StandardWindow background{ rumorArea.width + 50, rumorArea.height + 100, true, display };

        const fheroes2::Rect windowArea{ background.activeArea() };

        int32_t offsetY = windowArea.y + elementOffset;

        const fheroes2::Text title( _( "Rumors" ), fheroes2::FontType::normalYellow() );
        title.draw( windowArea.x + ( windowArea.width - title.width() ) / 2, offsetY, display );

        offsetY += title.height() + elementOffset;

        const fheroes2::Rect rumorsRoi{ windowArea.x + elementOffset, offsetY, rumorArea.width, rumorArea.height };
        background.applyTextBackgroundShading( rumorsRoi );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        RumorListBox rumorList( rumorsRoi.getPosition(), language );
        rumorList.initListBackgroundRestorer( rumorsRoi );

        rumorList.SetAreaItems( { rumorsRoi.x, rumorsRoi.y, rumorsRoi.width, rumorsRoi.height - listAreaHeightDeduction } );

        int32_t scrollbarOffsetX = rumorsRoi.x + rumorsRoi.width + 5;
        background.renderScrollbarBackground( { scrollbarOffsetX, rumorsRoi.y, rumorsRoi.width, rumorsRoi.height }, isEvilInterface );

        const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;
        const int32_t topPartHeight = 19;
        ++scrollbarOffsetX;

        rumorList.SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX, rumorsRoi.y + 1 } );
        rumorList.SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX, rumorsRoi.y + rumorsRoi.height - 15 } );
        rumorList.setScrollBarArea( { scrollbarOffsetX + 2, rumorsRoi.y + topPartHeight, 10, rumorsRoi.height - 2 * topPartHeight } );
        rumorList.setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );
        rumorList.SetAreaMaxItems( 10 );
        rumorList.SetListContent( rumors );
        rumorList.updateScrollBarImage();

        rumorList.Redraw();

        const int minibuttonIcnId = isEvilInterface ? ICN::CELLWIN_EVIL : ICN::CELLWIN;

        const fheroes2::Sprite & buttonImage = fheroes2::AGG::GetICN( minibuttonIcnId, 13 );
        const int32_t buttonWidth = buttonImage.width();
        const int32_t buttonOffset = ( rumorArea.width - 3 * buttonWidth ) / 2 + buttonWidth;

        fheroes2::Button buttonAdd( rumorsRoi.x, rumorsRoi.y + rumorsRoi.height + 5, minibuttonIcnId, 13, 14 );
        buttonAdd.draw();

        fheroes2::Button buttonEdit( rumorsRoi.x + buttonOffset, rumorsRoi.y + rumorsRoi.height + 5, minibuttonIcnId, 15, 16 );
        buttonEdit.draw();

        fheroes2::Button buttonDelete( rumorsRoi.x + rumorArea.width - buttonWidth, rumorsRoi.y + rumorsRoi.height + 5, minibuttonIcnId, 17, 18 );
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

            rumorList.QueueEventProcessing();

            if ( rumorList.IsNeedRedraw() ) {
                rumorList.Redraw();
                isRedrawNeeded = true;
            }

            if ( le.MouseClickLeft( buttonAdd.area() ) ) {
                std::string newRumor;

                const fheroes2::Text body{ _( "Rumor:" ), fheroes2::FontType::normalWhite() };
                if ( Dialog::inputString( fheroes2::Text{}, body, newRumor, longestRumor, true, language ) ) {
                    if ( std::any_of( rumors.begin(), rumors.end(), [&newRumor]( const auto & rumor ) { return rumor == newRumor; } ) ) {
                        fheroes2::showStandardTextMessage( _( "Rumor" ), _( "This rumor already exists in the list." ), Dialog::OK );
                        continue;
                    }

                    rumors.emplace_back( std::move( newRumor ) );

                    rumorList.updateScrollBarImage();
                    rumorList.Redraw();
                    isRedrawNeeded = true;
                }
            }
            else if ( rumorList.isDoubleClicked() || le.MouseClickLeft( buttonEdit.area() ) ) {
                if ( rumorList.getCurrentId() < 0 ) {
                    continue;
                }

                rumorList.resetDoubleClickedState();

                std::string temp = rumorList.GetCurrent();

                const fheroes2::Text body{ _( "Rumor:" ), fheroes2::FontType::normalWhite() };
                if ( Dialog::inputString( fheroes2::Text{}, body, temp, longestRumor, true, language ) ) {
                    const auto count = std::count_if( rumors.begin(), rumors.end(), [&temp]( const auto & rumor ) { return rumor == temp; } );
                    if ( rumorList.GetCurrent() != temp && count > 0 ) {
                        fheroes2::showStandardTextMessage( _( "Rumor" ), _( "This rumor already exists in the list." ), Dialog::OK );
                        continue;
                    }

                    rumorList.GetCurrent() = std::move( temp );

                    rumorList.Redraw();
                    isRedrawNeeded = true;
                }
            }
            else if ( le.MouseClickLeft( buttonDelete.area() ) ) {
                if ( rumorList.getCurrentId() < 0 ) {
                    continue;
                }

                rumorList.RemoveSelected();
                rumorList.updateScrollBarImage();
                rumorList.Redraw();
                isRedrawNeeded = true;
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to save the rumors." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonAdd.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Add Rumor" ), _( "Add an additional rumor." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonEdit.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Edit Rumor" ), _( "Edit an existing rumor." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonDelete.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Delete Rumor" ), _( "Delete an existing rumor." ), Dialog::ZERO );
            }

            if ( isRedrawNeeded ) {
                isRedrawNeeded = false;

                display.render( windowArea );
            }
        }

        return false;
    }
}
