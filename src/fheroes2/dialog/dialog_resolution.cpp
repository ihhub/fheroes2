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
#include "agg.h"
#include "display.h"
#include "embedded_image.h"
#include "game.h"
#include "interface_list.h"
#include "screen.h"
#include "settings.h"
#include "text.h"
#include "ui_button.h"
#include "zzlib.h"

namespace
{
    const int editBoxLength = 265;

    std::string GetResolutionString( const std::pair<int, int> & resolution )
    {
        std::string msg = _( "%{width} x %{height}" );
        StringReplace( msg, "%{width}", resolution.first );
        StringReplace( msg, "%{height}", resolution.second );
        return msg;
    }

    class ResolutionList : public Interface::ListBox<std::pair<int, int> >
    {
    public:
        ResolutionList( const Point & offset )
            : Interface::ListBox<std::pair<int, int> >( offset )
            , _isDoubleClicked( false )
        {}

        void RedrawItem( const std::pair<int, int> & resolution, s32 offsetX, s32 offsetY, bool current )
        {
            Text text( GetResolutionString( resolution ), ( current ? Font::YELLOW_BIG : Font::BIG ) );
            text.Blit( ( editBoxLength - text.w() ) / 2 + offsetX, offsetY, editBoxLength );
        }

        void RedrawBackground( const Point & dst )
        {
            const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
            fheroes2::Blit( panel, fheroes2::Display::instance(), dst.x, dst.y );
        }

        void ActionCurrentUp( void ) {}
        void ActionCurrentDn( void ) {}
        void ActionListSingleClick( std::pair<int, int> & ) {}
        void ActionListPressRight( std::pair<int, int> & ) {}

        void ActionListDoubleClick( std::pair<int, int> & )
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

    void RedrawInfo( const Point & dst, const std::pair<int, int> & resolution )
    {
        Text text( "Select game resolution:", Font::YELLOW_BIG );
        text.Blit( dst.x + 175 - text.w() / 2, dst.y + 30 );

        if ( resolution.first > 0 && resolution.second > 0 ) {
            Text text( GetResolutionString( resolution ), Font::YELLOW_BIG );
            text.Blit( dst.x + ( editBoxLength - text.w() ) / 2 + 40, dst.y + 288, editBoxLength );
        }
    }
}

namespace Dialog
{
    bool SelectResolution()
    {
        std::vector<std::pair<int, int> > resolutions = fheroes2::engine().getAvailableResolutions();
        if ( resolutions.empty() )
            return false;

        fheroes2::Display & display = fheroes2::Display::instance();
        Cursor & cursor = Cursor::Get();

        cursor.Hide();
        cursor.SetThemes( cursor.POINTER );

        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
        const fheroes2::Sprite & spriteShadow = fheroes2::AGG::GetICN( ICN::REQBKG, 1 );
        Size panel( sprite.width(), sprite.height() );

        const Point dialogOffset( ( display.width() - sprite.width() ) / 2, ( display.height() - sprite.height() ) / 2 );
        const Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

        fheroes2::ImageRestorer restorer( display, shadowOffset.x, shadowOffset.y, sprite.width() + BORDERWIDTH, sprite.height() + BORDERWIDTH );
        const Rect roi( dialogOffset.x, dialogOffset.y, sprite.width(), sprite.height() );

        fheroes2::Blit( spriteShadow, display, roi.x - BORDERWIDTH, roi.y + BORDERWIDTH );

        fheroes2::Button buttonOk( roi.x + 34, roi.y + 315, ICN::REQUEST, 1, 2 );
        fheroes2::Button buttonCancel( roi.x + 244, roi.y + 315, ICN::REQUEST, 3, 4 );

        ResolutionList resList( roi );

        resList.RedrawBackground( roi );
        resList.SetScrollButtonUp( ICN::REQUESTS, 5, 6, fheroes2::Point( roi.x + 327, roi.y + 55 ) );
        resList.SetScrollButtonDn( ICN::REQUESTS, 7, 8, fheroes2::Point( roi.x + 327, roi.y + 257 ) );
        resList.SetScrollSplitter( fheroes2::AGG::GetICN( ICN::ESCROLL, 3 ), Rect( roi.x + 328, roi.y + 73, 12, 180 ) );
        resList.SetAreaMaxItems( 11 );
        resList.SetAreaItems( Rect( roi.x + 40, roi.y + 55, 265, 215 ) );

        resList.SetListContent( resolutions );

        const Size currentResolution( display.width(), display.height() );

        std::pair<int, int> selectedResolution;
        for ( size_t i = 0; i < resolutions.size(); ++i ) {
            if ( resolutions[i].first == currentResolution.w && resolutions[i].second == currentResolution.h ) {
                resList.SetCurrent( i );
                selectedResolution = resList.GetCurrent();
                break;
            }
        }

        resList.Redraw();

        buttonOk.draw();
        buttonCancel.draw();

        RedrawInfo( roi, selectedResolution );

        cursor.Show();
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
                selectedResolution = std::make_pair( 0, 0 );
                break;
            }
            else if ( resList.isSelected() ) {
                selectedResolution = resList.GetCurrent();
            }

            if ( !cursor.isVisible() ) {
                resList.Redraw();
                buttonOk.draw();
                buttonCancel.draw();
                RedrawInfo( roi, selectedResolution );
                cursor.Show();
                display.render();
            }
        }

        cursor.Hide();

        if ( selectedResolution.first > 0 && selectedResolution.second > 0
             && ( selectedResolution.first != currentResolution.w || selectedResolution.second != currentResolution.h ) ) {
            Settings & conf = Settings::Get();
            display.resize( selectedResolution.first, selectedResolution.second );

#ifdef WITH_ZLIB
            const fheroes2::Image & appIcon = CreateImageFromZlib( 32, 32, iconImageLayer, sizeof( iconImageLayer ), iconTransformLayer, sizeof( iconTransformLayer ) );
            fheroes2::engine().setIcon( appIcon );
#endif

            return true;
        }

        return false;
    }
}
