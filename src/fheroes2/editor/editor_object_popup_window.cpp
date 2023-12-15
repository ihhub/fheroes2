/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

#include "editor_object_popup_window.h"

#include "agg_image.h"
#include "cursor.h"
#include "editor_interface.h"
#include "icn.h"
#include "image.h"
#include "maps_tiles.h"
#include "screen.h"
#include "translations.h"
#include "world.h"

namespace
{
    fheroes2::Rect makeRectQuickInfo( const LocalEvent & le, const fheroes2::Sprite & imageBox )
    {
        // place box next to mouse cursor
        const fheroes2::Point & mp = le.GetMouseCursor();

        const int32_t mx = ( ( mp.x - BORDERWIDTH ) / TILEWIDTH ) * TILEWIDTH;
        const int32_t my = ( ( mp.y - BORDERWIDTH ) / TILEWIDTH ) * TILEWIDTH;

        const Interface::GameArea & gamearea = Interface::EditorInterface::Get().getGameArea();
        const fheroes2::Rect & ar = gamearea.GetROI();

        int32_t xpos = mx + TILEWIDTH - ( imageBox.width() / 2 );
        int32_t ypos = my + TILEWIDTH - ( imageBox.height() / 2 );

        // clamp box to edges of adventure screen game area
        assert( ar.width >= imageBox.width() && ar.height >= imageBox.height() );
        xpos = std::clamp( xpos, BORDERWIDTH, ( ar.width - imageBox.width() ) + BORDERWIDTH );
        ypos = std::clamp( ypos, BORDERWIDTH, ( ar.height - imageBox.height() ) + BORDERWIDTH );

        return { xpos, ypos, imageBox.width(), imageBox.height() };
    }

    std::string getObjectInfoText( const Maps::Tiles & tile )
    {
        const MP2::MapObjectType type = tile.GetObject();
        switch ( type ) {
        case MP2::OBJ_NONE:
            if ( tile.isRoad() ) {
                return _( "Road" );
            }

            return Maps::Ground::String( tile.GetGround() );
        case MP2::OBJ_RESOURCE: {
            const Funds funds = getFundsFromTile( tile );
            assert( funds.GetValidItemsCount() == 1 );

            return Resource::String( funds.getFirstValidResource().first );
        }
        default:
            break;
        }

        return MP2::StringObject( type );
    }
}

namespace Editor
{
    void showPopupWindow( const Maps::Tiles & tile )
    {
        const CursorRestorer cursorRestorer( false, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        // image box
        const fheroes2::Sprite & box = fheroes2::AGG::GetICN( ICN::QWIKINFO, 0 );

        LocalEvent & le = LocalEvent::Get();
        const fheroes2::Rect pos = makeRectQuickInfo( le, box );

        fheroes2::ImageRestorer restorer( display, pos.x, pos.y, pos.width, pos.height );
        fheroes2::Blit( box, display, pos.x, pos.y );

        std::string infoString;
        const int32_t mainTileIndex = Maps::Tiles::getIndexOfMainTile( tile );

        if ( mainTileIndex != -1 ) {
            infoString = getObjectInfoText( world.GetTiles( mainTileIndex ) );
        }
        else {
            infoString = getObjectInfoText( tile );
        }

        const int32_t objectTextBorderedWidth = pos.width - 2 * BORDERWIDTH;
        const fheroes2::Text text( infoString, fheroes2::FontType::smallWhite() );
        text.draw( pos.x + 22, pos.y - 6 + ( ( pos.height - text.height( objectTextBorderedWidth ) ) / 2 ), objectTextBorderedWidth, display );

        display.render( restorer.rect() );

        // quick info loop
        while ( le.HandleEvents() && le.MousePressRight() )
            ;

        // restore background
        restorer.restore();
        display.render( restorer.rect() );
    }
}
