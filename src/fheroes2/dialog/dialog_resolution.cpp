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
#include "game.h"
#include "interface_list.h"

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
            const Sprite & panel = AGG::GetICN( ICN::REQBKG, 0 );
            panel.Blit( dst );
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
        std::vector<std::pair<int, int> > resolutions = Display::GetAvailableResolutions();
        if ( resolutions.empty() )
            return false;

        Display & display = Display::Get();
        Cursor & cursor = Cursor::Get();

        cursor.Hide();
        cursor.SetThemes( cursor.POINTER );

        const Sprite & sprite = AGG::GetICN( ICN::REQBKG, 0 );
        const Sprite & spriteShadow = AGG::GetICN( ICN::REQBKG, 1 );
        Size panel( sprite.w(), sprite.h() );

        const Point dialogOffset( ( display.w() - sprite.w() ) / 2, ( display.h() - sprite.h() ) / 2 );
        const Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

        SpriteBack back( Rect( shadowOffset.x, shadowOffset.y, sprite.w() + BORDERWIDTH, sprite.h() + BORDERWIDTH ) );
        const Rect roi( dialogOffset.x, dialogOffset.y, sprite.w(), sprite.h() );

        spriteShadow.Blit( roi.x - BORDERWIDTH, roi.y + BORDERWIDTH );

        Button buttonOk( roi.x + 34, roi.y + 315, ICN::REQUEST, 1, 2 );
        Button buttonCancel( roi.x + 244, roi.y + 315, ICN::REQUEST, 3, 4 );

        ResolutionList resList( roi );

        resList.RedrawBackground( roi );
        resList.SetScrollButtonUp( ICN::REQUESTS, 5, 6, Point( roi.x + 327, roi.y + 55 ) );
        resList.SetScrollButtonDn( ICN::REQUESTS, 7, 8, Point( roi.x + 327, roi.y + 257 ) );
        resList.SetScrollSplitter( AGG::GetICN( ICN::ESCROLL, 3 ), Rect( roi.x + 328, roi.y + 73, 12, 180 ) );
        resList.SetAreaMaxItems( 11 );
        resList.SetAreaItems( Rect( roi.x + 40, roi.y + 55, 265, 215 ) );

        resList.SetListContent( resolutions );

        const Size currentResolution = display.GetSize();

        std::pair<int, int> selectedResolution;
        for ( size_t i = 0; i < resolutions.size(); ++i ) {
            if ( resolutions[i].first == currentResolution.w && resolutions[i].second == currentResolution.h ) {
                resList.SetCurrent( i );
                selectedResolution = resList.GetCurrent();
                break;
            }
        }

        resList.Redraw();

        buttonOk.Draw();
        buttonCancel.Draw();

        RedrawInfo( roi, selectedResolution );

        cursor.Show();
        display.Flip();

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonOk ) && buttonOk.isEnable() ? buttonOk.PressDraw() : buttonOk.ReleaseDraw();
            le.MousePressLeft( buttonCancel ) ? buttonCancel.PressDraw() : buttonCancel.ReleaseDraw();

            resList.QueueEventProcessing();

            if ( ( buttonOk.isEnable() && le.MouseClickLeft( buttonOk ) ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) || resList.isDoubleClicked() ) {
                if ( resList.isSelected() ) {
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonCancel ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
                selectedResolution = std::make_pair( 0, 0 );
                break;
            }
            else if ( resList.isSelected() ) {
                selectedResolution = resList.GetCurrent();
            }

            if ( !cursor.isVisible() ) {
                resList.Redraw();
                buttonOk.Draw();
                buttonCancel.Draw();
                RedrawInfo( roi, selectedResolution );
                cursor.Show();
                display.Flip();
            }
        }

        cursor.Hide();
        back.Restore();

        if ( selectedResolution.first > 0 && selectedResolution.second > 0
             && ( selectedResolution.first != currentResolution.w || selectedResolution.second != currentResolution.h ) ) {
            Settings & conf = Settings::Get();
            display.SetVideoMode( selectedResolution.first, selectedResolution.second, display.IsFullScreen(), conf.KeepAspectRatio(),
                                  conf.ChangeFullscreenResolution() );
            return true;
        }

        return false;
    }
}
