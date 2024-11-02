/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#include "dialog_resolution.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "embedded_image.h"
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
#include "ui_dialog.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_window.h"
#include "zzlib.h"

namespace
{
    const int32_t textAreaWidth = 270;
    const int32_t scrollBarAreaWidth = 48;
    const int32_t paddingLeftSide = 24;
    const int32_t resolutionItemHeight = 19;
    // TODO: this is a hack over partially incorrect text height calculation. Fix it later together with the Text classes.
    const int32_t textOffsetYCorrection = 6;
    const std::string middleText = " x ";

    std::pair<std::string, std::string> getResolutionStrings( const fheroes2::ResolutionInfo & resolution )
    {
        if ( resolution.screenWidth != resolution.gameWidth && resolution.screenHeight != resolution.gameHeight ) {
            const int32_t integer = resolution.screenWidth / resolution.gameWidth;
            const int32_t fraction = resolution.screenWidth * 10 / resolution.gameWidth - 10 * integer;

            return std::make_pair( std::to_string( resolution.gameWidth ),
                                   std::to_string( resolution.gameHeight ) + " (x" + std::to_string( integer ) + "." + std::to_string( fraction ) + ')' );
        }

        return std::make_pair( std::to_string( resolution.gameWidth ), std::to_string( resolution.gameHeight ) );
    }

    class ResolutionList : public Interface::ListBox<fheroes2::ResolutionInfo>
    {
    public:
        using Interface::ListBox<fheroes2::ResolutionInfo>::ActionListSingleClick;
        using Interface::ListBox<fheroes2::ResolutionInfo>::ActionListPressRight;
        using Interface::ListBox<fheroes2::ResolutionInfo>::ActionListDoubleClick;

        explicit ResolutionList( const fheroes2::Point & offset )
            : Interface::ListBox<fheroes2::ResolutionInfo>( offset )
            , _isDoubleClicked( false )
        {
            // Do nothing.
        }

        void RedrawItem( const fheroes2::ResolutionInfo & resolution, int32_t offsetX, int32_t offsetY, bool current ) override
        {
            const fheroes2::FontType fontType = current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite();

            const auto [leftText, rightText] = getResolutionStrings( resolution );
            const int32_t middleTextSize = fheroes2::Text( middleText, fontType ).width();
            const int32_t leftTextSize = fheroes2::Text( leftText, fontType ).width();

            const fheroes2::Text resolutionText( leftText + middleText + rightText, fontType );

            const int32_t textOffsetX = offsetX + textAreaWidth / 2 - leftTextSize - middleTextSize / 2;
            const int32_t textOffsetY = offsetY + ( resolutionItemHeight - resolutionText.height() + textOffsetYCorrection ) / 2;

            resolutionText.draw( textOffsetX, textOffsetY, fheroes2::Display::instance() );
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

        void ActionListSingleClick( fheroes2::ResolutionInfo & /*unused*/ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( fheroes2::ResolutionInfo & resolution ) override
        {
            const auto [leftText, rightText] = getResolutionStrings( resolution );

            if ( resolution.gameHeight == resolution.screenHeight && resolution.gameWidth == resolution.screenWidth ) {
                fheroes2::showStandardTextMessage( leftText + middleText + rightText, _( "Select this game resolution." ), Dialog::ZERO );
            }
            else {
                const int32_t integer = resolution.screenWidth / resolution.gameWidth;
                const int32_t fraction = resolution.screenWidth * 10 / resolution.gameWidth - 10 * integer;

                std::string scaledResolutionText = _(
                    "Selecting this resolution will set a resolution that is scaled from the original resolution (%{resolution}) by multiplying it with the number "
                    "in the brackets (%{scale}).\n\nA resolution with a clean integer number (2.0x, 3.0x etc.) will usually look better because the pixels are upscaled "
                    "evenly in both horizontal and vertical directions." );

                StringReplace( scaledResolutionText, "%{resolution}", std::to_string( resolution.gameWidth ) + middleText + std::to_string( resolution.gameHeight ) );
                StringReplace( scaledResolutionText, "%{scale}", std::to_string( integer ) + "." + std::to_string( fraction ) );

                fheroes2::showStandardTextMessage( leftText + middleText + rightText, scaledResolutionText, Dialog::ZERO );
            }
        }

        void ActionListDoubleClick( fheroes2::ResolutionInfo & /*unused*/ ) override
        {
            _isDoubleClicked = true;
        }

        bool isDoubleClicked() const
        {
            return _isDoubleClicked;
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
        bool _isDoubleClicked;
        std::unique_ptr<fheroes2::ImageRestorer> _listBackground;
    };

    void RedrawInfo( const fheroes2::Point & dst, const fheroes2::ResolutionInfo & resolution, fheroes2::Image & output )
    {
        if ( resolution.gameWidth > 0 && resolution.gameHeight > 0 ) {
            const fheroes2::FontType fontType = fheroes2::FontType::normalYellow();

            const auto [leftText, rightText] = getResolutionStrings( resolution );
            const int32_t middleTextSize = fheroes2::Text( middleText, fontType ).width();
            const int32_t leftTextSize = fheroes2::Text( leftText, fontType ).width();

            const int32_t textOffsetX = dst.x + textAreaWidth / 2 - leftTextSize - middleTextSize / 2;

            const fheroes2::Text resolutionText( leftText + middleText + rightText, fontType );
            resolutionText.draw( textOffsetX, dst.y + ( resolutionItemHeight - resolutionText.height() + textOffsetYCorrection ) / 2, output );
        }
    }

    fheroes2::ResolutionInfo getNewResolution()
    {
        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        const int32_t listHeightDeduction = 112;
        const int32_t listAreaOffsetY = 3;
        const int32_t listAreaHeightDeduction = 4;

        std::vector<fheroes2::ResolutionInfo> resolutions = fheroes2::engine().getAvailableResolutions();

        // If we don't have many resolutions, we reduce the maximum dialog height,
        // but not less than enough for 11 elements.
        // We also limit the maximum list height to 22 lines.
        const int32_t maxDialogHeight = fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) * std::clamp( static_cast<int32_t>( resolutions.size() ), 11, 22 )
                                        + listAreaOffsetY + listAreaHeightDeduction + listHeightDeduction;

        fheroes2::Display & display = fheroes2::Display::instance();

        // Dialog height is also capped with the current screen height.
        fheroes2::StandardWindow background( paddingLeftSide + textAreaWidth + scrollBarAreaWidth + 3, std::min( display.height() - 100, maxDialogHeight ), true,
                                             display );

        const fheroes2::Rect roi( background.activeArea() );
        const fheroes2::Rect listRoi( roi.x + paddingLeftSide, roi.y + 37, textAreaWidth, roi.height - listHeightDeduction );

        // We divide the list: resolution list and selected resolution.
        const fheroes2::Rect selectedResRoi( listRoi.x, listRoi.y + listRoi.height + 12, listRoi.width, 21 );
        background.applyTextBackgroundShading( selectedResRoi );
        background.applyTextBackgroundShading( { listRoi.x, listRoi.y, listRoi.width, listRoi.height } );

        fheroes2::ImageRestorer selectedResBackground( fheroes2::Display::instance(), selectedResRoi.x, selectedResRoi.y, listRoi.width, selectedResRoi.height );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        // Prepare OKAY and CANCEL buttons and render their shadows.
        fheroes2::Button buttonOk;
        fheroes2::Button buttonCancel;
        background.renderOkayCancelButtons( buttonOk, buttonCancel, isEvilInterface );

        ResolutionList listBox( roi.getPosition() );

        listBox.initListBackgroundRestorer( listRoi );

        listBox.SetAreaItems( { listRoi.x, listRoi.y + 3, listRoi.width - 3, listRoi.height - 4 } );

        int32_t scrollbarOffsetX = roi.x + roi.width - 35;
        background.renderScrollbarBackground( { scrollbarOffsetX, listRoi.y, listRoi.width, listRoi.height }, isEvilInterface );

        const int32_t topPartHeight = 19;
        ++scrollbarOffsetX;

        const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;
        listBox.SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX, listRoi.y + 1 } );
        listBox.SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX, listRoi.y + listRoi.height - 15 } );
        listBox.setScrollBarArea( { scrollbarOffsetX + 2, listRoi.y + topPartHeight, 10, listRoi.height - 2 * topPartHeight } );
        listBox.setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );
        listBox.SetAreaMaxItems( ( listRoi.height - 7 ) / fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) );
        listBox.SetListContent( resolutions );
        listBox.updateScrollBarImage();

        const fheroes2::ResolutionInfo currentResolution{ display.width(), display.height(), display.screenSize().width, display.screenSize().height };

        fheroes2::ResolutionInfo selectedResolution;
        for ( size_t i = 0; i < resolutions.size(); ++i ) {
            if ( resolutions[i] == currentResolution ) {
                listBox.SetCurrent( i );
                selectedResolution = listBox.GetCurrent();
                break;
            }
        }

        listBox.Redraw();

        RedrawInfo( selectedResRoi.getPosition(), selectedResolution, display );

        const fheroes2::Text title( _( "Select Game Resolution:" ), fheroes2::FontType::normalYellow() );
        title.draw( roi.x + ( roi.width - title.width() ) / 2, roi.y + 16, display );

        display.render( background.totalArea() );

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            le.isMouseLeftButtonPressedInArea( buttonOk.area() ) && buttonOk.isEnabled() ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
            le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

            const int listId = listBox.getCurrentId();
            listBox.QueueEventProcessing();
            const bool needRedraw = listId != listBox.getCurrentId();

            if ( ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY )
                 || listBox.isDoubleClicked() ) {
                if ( listBox.isSelected() ) {
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                selectedResolution = {};
                break;
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to apply the selected resolution." ), Dialog::ZERO );
            }

            if ( !listBox.IsNeedRedraw() ) {
                continue;
            }

            if ( needRedraw ) {
                selectedResolution = listBox.GetCurrent();
                selectedResBackground.restore();
                RedrawInfo( selectedResRoi.getPosition(), selectedResolution, display );
            }

            listBox.Redraw();
            display.render( roi );
        }

        return selectedResolution;
    }
}

namespace Dialog
{
    bool SelectResolution()
    {
        const fheroes2::ResolutionInfo selectedResolution = getNewResolution();
        if ( selectedResolution == fheroes2::ResolutionInfo{} ) {
            return false;
        }

        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::ResolutionInfo currentResolution{ display.width(), display.height(), display.screenSize().width, display.screenSize().height };

        if ( selectedResolution.gameWidth > 0 && selectedResolution.gameHeight > 0 && selectedResolution.screenWidth >= selectedResolution.gameWidth
             && selectedResolution.screenHeight >= selectedResolution.gameHeight && selectedResolution != currentResolution ) {
            display.setResolution( selectedResolution );

#if !defined( MACOS_APP_BUNDLE )
            const fheroes2::Image & appIcon = Compression::CreateImageFromZlib( 32, 32, iconImage, sizeof( iconImage ), true );
            fheroes2::engine().setIcon( appIcon );
#endif

            return true;
        }

        return false;
    }
}
