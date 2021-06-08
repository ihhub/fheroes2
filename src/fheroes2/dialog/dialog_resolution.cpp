/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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
#include "agg_image.h"
#include "cursor.h"
#include "embedded_image.h"
#include "game.h"
#include "icn.h"
#include "interface_list.h"
#include "localevent.h"
#include "screen.h"
#include "text.h"
#include "ui_button.h"
#include "zzlib.h"

namespace
{
    const int editBoxLength = 266;

    std::string GetResolutionString( const fheroes2::Size & resolution )
    {
        return std::to_string( resolution.width ) + " x " + std::to_string( resolution.height );
    }

    class ResolutionList : public Interface::ListBox<fheroes2::Size>
    {
    public:
        explicit ResolutionList( const fheroes2::Point & offset )
            : Interface::ListBox<fheroes2::Size>( offset )
            , _isDoubleClicked( false )
        {}

        void RedrawItem( const fheroes2::Size & resolution, s32 offsetX, s32 offsetY, bool current ) override
        {
            const Text text( GetResolutionString( resolution ), ( current ? Font::YELLOW_BIG : Font::BIG ) );
            text.Blit( ( editBoxLength - text.w() ) / 2 + offsetX, offsetY, editBoxLength );
        }

        void RedrawBackground( const fheroes2::Point & dst ) override
        {
            const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
            fheroes2::Blit( panel, fheroes2::Display::instance(), dst.x, dst.y );
        }

        void ActionCurrentUp() override {}
        void ActionCurrentDn() override {}
        void ActionListSingleClick( fheroes2::Size & ) override {}
        void ActionListPressRight( fheroes2::Size & ) override {}

        void ActionListDoubleClick( fheroes2::Size & ) override
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

    void RedrawInfo( const fheroes2::Point & dst, const fheroes2::Size & resolution )
    {
        Text text( "Select Game Resolution:", Font::YELLOW_BIG );
        text.Blit( dst.x + ( 377 - text.w() ) / 2, dst.y + 30 );

        if ( resolution.width > 0 && resolution.height > 0 ) {
            text.Set( GetResolutionString( resolution ), Font::YELLOW_BIG );
            text.Blit( dst.x + ( editBoxLength - text.w() ) / 2 + 41, dst.y + 287 + ( 19 - text.h() + 2 ) / 2, editBoxLength );
        }
    }
}

namespace Dialog
{
    bool SelectResolution()
    {
        std::vector<fheroes2::Size> resolutions = fheroes2::engine().getAvailableResolutions();
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

        fheroes2::Button buttonOk( roi.x + 34, roi.y + 315, ICN::REQUEST, 1, 2 );
        fheroes2::Button buttonCancel( roi.x + 244, roi.y + 315, ICN::REQUEST, 3, 4 );

        ResolutionList resList( roi.getPosition() );

        resList.RedrawBackground( roi.getPosition() );
        resList.SetScrollButtonUp( ICN::REQUESTS, 5, 6, fheroes2::Point( roi.x + 327, roi.y + 55 ) );
        resList.SetScrollButtonDn( ICN::REQUESTS, 7, 8, fheroes2::Point( roi.x + 327, roi.y + 257 ) );
        resList.SetScrollBar( fheroes2::AGG::GetICN( ICN::ESCROLL, 3 ), fheroes2::Rect( roi.x + 328, roi.y + 73, 12, 180 ) );
        resList.SetAreaMaxItems( 11 );
        resList.SetAreaItems( fheroes2::Rect( roi.x + 41, roi.y + 55 + 3, editBoxLength, 215 ) );

        resList.SetListContent( resolutions );

        const fheroes2::Size currentResolution( display.width(), display.height() );

        fheroes2::Size selectedResolution;
        for ( size_t i = 0; i < resolutions.size(); ++i ) {
            if ( resolutions[i].width == currentResolution.width && resolutions[i].height == currentResolution.height ) {
                resList.SetCurrent( i );
                selectedResolution = resList.GetCurrent();
                break;
            }
        }

        resList.Redraw();

        buttonOk.draw();
        buttonCancel.draw();

        RedrawInfo( roi.getPosition(), selectedResolution );

        display.render();

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonOk.area() ) && buttonOk.isEnabled() ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
            le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

            resList.QueueEventProcessing();

            if ( ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) || resList.isDoubleClicked() ) {
                if ( resList.isSelected() ) {
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
                selectedResolution = fheroes2::Size( 0, 0 );
                break;
            }
            else if ( resList.isSelected() ) {
                selectedResolution = resList.GetCurrent();
            }

            if ( !resList.IsNeedRedraw() ) {
                continue;
            }

            resList.Redraw();
            buttonOk.draw();
            buttonCancel.draw();
            RedrawInfo( roi.getPosition(), selectedResolution );
            display.render();
        }

        if ( selectedResolution.width > 0 && selectedResolution.height > 0
             && ( selectedResolution.width != currentResolution.width || selectedResolution.height != currentResolution.height ) ) {
            display.resize( selectedResolution.width, selectedResolution.height );

#ifdef WITH_ZLIB
            const fheroes2::Image & appIcon = CreateImageFromZlib( 32, 32, iconImage, sizeof( iconImage ), true );
            fheroes2::engine().setIcon( appIcon );
#endif

            return true;
        }

        return false;
    }
}
