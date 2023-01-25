/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog_resolution.h"
#include "embedded_image.h"
#include "game_hotkeys.h"
#include "gamedefs.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "zzlib.h"

namespace
{
    const int32_t editBoxLength = 266;
    const int32_t resolutionItemHeight = 19;
    // TODO: this is a hack over partially incorrect text height caclulation. Fix it later together with the Text classes.
    const int32_t textOffsetYCorrection = 6;
    const std::string middleText = " x ";

    std::pair<std::string, std::string> getResolutionStrings( const fheroes2::ResolutionInfo & resolution )
    {
        if ( resolution.scale > 1 ) {
            return std::make_pair( std::to_string( resolution.width ), std::to_string( resolution.height ) + " (x" + std::to_string( resolution.scale ) + ")" );
        }

        return std::make_pair( std::to_string( resolution.width ), std::to_string( resolution.height ) );
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

            const int32_t textOffsetX = offsetX + editBoxLength / 2 - leftTextSize - middleTextSize / 2;
            const int32_t textOffsetY = offsetY + ( resolutionItemHeight - resolutionText.height() + textOffsetYCorrection ) / 2;

            resolutionText.draw( textOffsetX, textOffsetY, fheroes2::Display::instance() );
        }

        void RedrawBackground( const fheroes2::Point & dst ) override
        {
            const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
            fheroes2::Blit( panel, fheroes2::Display::instance(), dst.x, dst.y );
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

        void ActionListPressRight( fheroes2::ResolutionInfo & /*unused*/ ) override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( fheroes2::ResolutionInfo & /*unused*/ ) override
        {
            _isDoubleClicked = true;
        }

        bool isDoubleClicked() const
        {
            return _isDoubleClicked;
        }

    private:
        bool _isDoubleClicked;
    };

    void RedrawInfo( const fheroes2::Point & dst, const fheroes2::ResolutionInfo & resolution, fheroes2::Image & output )
    {
        fheroes2::Text text( _( "Select Game Resolution:" ), fheroes2::FontType::normalYellow() );
        text.draw( dst.x + ( 377 - text.width() ) / 2, dst.y + 32, output );

        if ( resolution.width > 0 && resolution.height > 0 ) {
            const fheroes2::FontType fontType = fheroes2::FontType::normalYellow();

            const auto [leftText, rightText] = getResolutionStrings( resolution );
            const int32_t middleTextSize = fheroes2::Text( middleText, fontType ).width();
            const int32_t leftTextSize = fheroes2::Text( leftText, fontType ).width();

            const int32_t textOffsetX = dst.x + 41 + editBoxLength / 2 - leftTextSize - middleTextSize / 2;

            const fheroes2::Text resolutionText( leftText + middleText + rightText, fontType );
            resolutionText.draw( textOffsetX, dst.y + 287 + ( resolutionItemHeight - text.height() + textOffsetYCorrection ) / 2, output );
        }
    }
}

namespace Dialog
{
    bool SelectResolution()
    {
        std::vector<fheroes2::ResolutionInfo> resolutions = fheroes2::engine().getAvailableResolutions();
        if ( resolutions.empty() )
            return false;

        fheroes2::Display & display = fheroes2::Display::instance();

        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
        const fheroes2::Sprite & spriteShadow = fheroes2::AGG::GetICN( ICN::REQBKG, 1 );

        const fheroes2::Point dialogOffset( ( display.width() - sprite.width() ) / 2, ( display.height() - sprite.height() ) / 2 );
        const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

        fheroes2::ImageRestorer restorer( display, shadowOffset.x, shadowOffset.y, sprite.width() + BORDERWIDTH, sprite.height() + BORDERWIDTH );
        const fheroes2::Rect roi( dialogOffset.x, dialogOffset.y, sprite.width(), sprite.height() );

        fheroes2::Blit( spriteShadow, display, roi.x - BORDERWIDTH, roi.y + BORDERWIDTH );

        fheroes2::Button buttonOk( roi.x + 34, roi.y + 315, ICN::BUTTON_SMALL_OKAY_GOOD, 0, 1 );
        fheroes2::Button buttonCancel( roi.x + 244, roi.y + 315, ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1 );

        ResolutionList resList( roi.getPosition() );

        resList.RedrawBackground( roi.getPosition() );
        resList.SetScrollButtonUp( ICN::REQUESTS, 5, 6, { roi.x + 327, roi.y + 55 } );
        resList.SetScrollButtonDn( ICN::REQUESTS, 7, 8, { roi.x + 327, roi.y + 257 } );

        const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::ESCROLL, 3 );
        const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, 180, 11, static_cast<int32_t>( resolutions.size() ),
                                                                                   { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );
        resList.setScrollBarArea( { roi.x + 328, roi.y + 73, 12, 180 } );
        resList.setScrollBarImage( scrollbarSlider );
        const int32_t maximumItems = 11;
        resList.SetAreaMaxItems( maximumItems );
        resList.SetAreaItems( { roi.x + 41, roi.y + 55 + 4, editBoxLength, resolutionItemHeight * maximumItems } );

        resList.SetListContent( resolutions );

        const fheroes2::ResolutionInfo currentResolution{ display.width(), display.height(), display.scale() };

        fheroes2::ResolutionInfo selectedResolution;
        for ( size_t i = 0; i < resolutions.size(); ++i ) {
            if ( resolutions[i] == currentResolution ) {
                resList.SetCurrent( i );
                selectedResolution = resList.GetCurrent();
                break;
            }
        }

        resList.Redraw();

        buttonOk.draw();
        buttonCancel.draw();

        RedrawInfo( roi.getPosition(), selectedResolution, display );

        display.render();

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonOk.area() ) && buttonOk.isEnabled() ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
            le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

            resList.QueueEventProcessing();

            if ( ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY )
                 || resList.isDoubleClicked() ) {
                if ( resList.isSelected() ) {
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                selectedResolution = {};
                break;
            }
            else if ( le.MousePressRight( buttonCancel.area() ) ) {
                fheroes2::Text header( _( "Cancel" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Exit this menu without doing anything." ), fheroes2::FontType::normalWhite() );
                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( buttonOk.area() ) ) {
                fheroes2::Text header( _( "Okay" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Click to apply the selected resolution." ), fheroes2::FontType::normalWhite() );
                fheroes2::showMessage( header, body, 0 );
            }

            if ( resList.isSelected() ) {
                selectedResolution = resList.GetCurrent();
            }

            if ( !resList.IsNeedRedraw() ) {
                continue;
            }

            resList.Redraw();
            buttonOk.draw();
            buttonCancel.draw();
            RedrawInfo( roi.getPosition(), selectedResolution, display );
            display.render();
        }

        if ( selectedResolution.width > 0 && selectedResolution.height > 0 && selectedResolution.scale > 0 && selectedResolution != currentResolution ) {
            display.setResolution( selectedResolution );

#if !defined( MACOS_APP_BUNDLE )
            const fheroes2::Image & appIcon = CreateImageFromZlib( 32, 32, iconImage, sizeof( iconImage ), true );
            fheroes2::engine().setIcon( appIcon );
#endif

            return true;
        }

        return false;
    }
}
