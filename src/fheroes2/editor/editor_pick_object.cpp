/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include "editor_pick_object.h"
#include "agg_image.h"
#include "game.h"
#include "game_interface.h"
#include "localevent.h"
#include "screen.h"

namespace
{
    const int32_t tileSize = 32;

    fheroes2::Image renderBaseObject( const fheroes2::MapObjectInfo & info, const int icnId )
    {
        fheroes2::Point minOffset;
        fheroes2::Point maxOffset;
        for ( const fheroes2::MapObjectPartInfo & part : info.parts ) {
            if ( minOffset.x > part.offset.x ) {
                minOffset.x = part.offset.x;
            }
            if ( minOffset.y > part.offset.y ) {
                minOffset.y = part.offset.y;
            }

            if ( maxOffset.x < part.offset.x ) {
                maxOffset.x = part.offset.x;
            }
            if ( maxOffset.y < part.offset.y ) {
                maxOffset.y = part.offset.y;
            }
        }

        const fheroes2::Size size( ( maxOffset.x - minOffset.x + 1 ), ( maxOffset.y - minOffset.y + 1 ) );
        fheroes2::Image image( size.width * tileSize, size.height * tileSize );
        image.reset();

        // Draw lines
        fheroes2::DrawRect( image, fheroes2::Rect( 0, 0, image.width(), image.height() ), 0 );
        for ( int32_t x = 0; x < size.width; ++x ) {
            fheroes2::DrawLine( image, { x * tileSize, 0 }, { x * tileSize, image.height() }, 0 );
        }
        for ( int32_t y = 0; y < size.height; ++y ) {
            fheroes2::DrawLine( image, { 0, y * tileSize }, { image.width(), y * tileSize }, 0 );
        }

        for ( const fheroes2::MapObjectPartInfo & part : info.parts ) {
            const fheroes2::Sprite & imagePart = fheroes2::AGG::GetICN( icnId, part.index );
            fheroes2::Blit( imagePart, 0, 0, image, imagePart.x() + ( part.offset.x - minOffset.x ) * tileSize,
                            imagePart.y() + ( part.offset.y - minOffset.y ) * tileSize, imagePart.width(), imagePart.height() );
        }

        return image;
    }

    void drawImage( const std::vector<fheroes2::MapObjectInfo> & objects, const int icnId, const size_t imageId )
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        display.fill( 13 );

        const fheroes2::Image image = renderBaseObject( objects[imageId], icnId );

        Blit( image, 0, 0, display, ( display.width() - image.width() ) / 2, ( display.height() - image.height() ) / 2, image.width(), image.height() );

        fheroes2::cursor().update( image, -image.width() / 2, -image.height() / 2 );

        const fheroes2::Point & currentPos = LocalEvent::Get().GetMouseCursor();
        fheroes2::cursor().setPosition( currentPos.x, currentPos.y );

        display.render();
    }
}

namespace fheroes2
{
    void showObjects( const std::vector<fheroes2::MapObjectInfo> & objects, const int icnId )
    {
        fheroes2::cursor().show( true );

        size_t currentIndex = 0;

        drawImage( objects, icnId, currentIndex );

        LocalEvent & le = LocalEvent::Get();

        while ( 1 ) {
            if ( !le.HandleEvents( true, true ) ) {
                break;
            }

            if ( le.KeyPress( KeySym::KEY_LEFT ) ) {
                if ( currentIndex > 0 ) {
                    --currentIndex;
                }
                else {
                    currentIndex = objects.size() - 1;
                }

                drawImage( objects, icnId, currentIndex );
            }
            else if ( le.KeyPress( KeySym::KEY_RIGHT ) ) {
                ++currentIndex;
                if ( currentIndex >= objects.size() ) {
                    currentIndex = 0;
                }

                drawImage( objects, icnId, currentIndex );
            }
            else if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
                break;
            }
        }

        fheroes2::cursor().show( false );
    }
}
