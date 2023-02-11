/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2023                                             *
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

#include "ui_window.h"

#include <utility>

#include "agg_image.h"
#include "icn.h"
#include "settings.h"

namespace
{
    const int32_t borderSize = BORDERWIDTH;
}

namespace fheroes2
{
    StandardWindow::StandardWindow( const int32_t width, const int32_t height, const bool renderBackground, Image & output )
        : _output( output )
        , _activeArea( ( output.width() - width ) / 2, ( output.height() - height ) / 2, width, height )
        , _windowArea( _activeArea.x - borderSize, _activeArea.y - borderSize, _activeArea.width + 2 * borderSize, _activeArea.height + 2 * borderSize )
        , _restorer( output, _windowArea.x - borderSize, _windowArea.y, _windowArea.width + borderSize, _windowArea.height + borderSize )
        , _hasBackground{ renderBackground }
    {
        render();
    }

    StandardWindow::StandardWindow( const int32_t x, const int32_t y, const int32_t width, const int32_t height, const bool renderBackground, Image & output )
        : _output( output )
        , _activeArea( x, y, width, height )
        , _windowArea( _activeArea.x - borderSize, _activeArea.y - borderSize, _activeArea.width + 2 * borderSize, _activeArea.height + 2 * borderSize )
        , _restorer( output, _windowArea.x - borderSize, _windowArea.y, _windowArea.width + borderSize, _windowArea.height + borderSize )
        , _hasBackground{ renderBackground }
    {
        render();
    }

    void StandardWindow::render()
    {
        // Notice: ICN::SURDRBKE and ICN::SURDRBKG has 16 (equals to BORDERWIDTH) pixels shadow from the left and the bottom sides.
        const fheroes2::Sprite & horizontalSprite = fheroes2::AGG::GetICN( ( Settings::Get().isEvilInterfaceEnabled() ? ICN::SURDRBKE : ICN::SURDRBKG ), 0 );
        const fheroes2::Sprite & verticalSprite = fheroes2::AGG::GetICN( ( Settings::Get().isEvilInterfaceEnabled() ? ICN::WINLOSEE : ICN::WINLOSE ), 0 );

        // Offset from border egdes (size of evil interface corners is 43 pixels) - this egdes (corners) will not be copied to fill the border.
        const int32_t borderEdgeOffset{ 43 };

        // Offset from window edges to background copy area and also the size of corners to render.
        const int32_t cornerSize{ _hasBackground ? 22 : borderSize };

        // Size in pixels of dithered transition from one image to another.
        const int32_t transitionSize{ 10 };

        const int32_t horizontalSpriteWidth{ horizontalSprite.width() - BORDERWIDTH };
        const int32_t horizontalSpriteHeight{ horizontalSprite.height() - BORDERWIDTH };
        const int32_t verticalSpriteHeight{ verticalSprite.height() };
        const int32_t verticalSpriteWidth{ verticalSprite.width() };

        // Render window corners. The corners are the same in used original images, so we use only 'verticalSprite'.
        const int32_t rightCornerOffsetX = _windowArea.x + _windowArea.width - cornerSize;
        const int32_t bottomCornerOffsetY = _windowArea.y + _windowArea.height - cornerSize;
        const int32_t rightCornerSpriteOffsetX = verticalSpriteWidth - cornerSize;
        const int32_t bottomCornerSpriteOffsetY = verticalSpriteHeight - cornerSize;
        fheroes2::Blit( verticalSprite, 0, 0, _output, _windowArea.x, _windowArea.y, cornerSize, cornerSize );
        fheroes2::Blit( verticalSprite, rightCornerSpriteOffsetX, 0, _output, rightCornerOffsetX, _windowArea.y, cornerSize, cornerSize );
        fheroes2::Blit( verticalSprite, 0, bottomCornerSpriteOffsetY, _output, _windowArea.x, bottomCornerOffsetY, cornerSize, cornerSize );
        fheroes2::Blit( verticalSprite, rightCornerSpriteOffsetX, bottomCornerSpriteOffsetY, _output, rightCornerOffsetX, bottomCornerOffsetY, cornerSize, cornerSize );

        // Render additional part of border corners. This part will not be repeated to fill the border length.
        const int32_t extraCornerSize = borderEdgeOffset - cornerSize;

        fheroes2::Blit( verticalSprite, cornerSize, 0, _output, _windowArea.x + cornerSize, _windowArea.y, extraCornerSize, cornerSize );
        fheroes2::Blit( verticalSprite, 0, cornerSize, _output, _windowArea.x, _windowArea.y + cornerSize, cornerSize, extraCornerSize );

        fheroes2::Blit( verticalSprite, verticalSpriteWidth - borderEdgeOffset, 0, _output, rightCornerOffsetX - extraCornerSize, _windowArea.y, extraCornerSize,
                        cornerSize );
        fheroes2::Blit( verticalSprite, rightCornerSpriteOffsetX, cornerSize, _output, rightCornerOffsetX, _windowArea.y + cornerSize, cornerSize, extraCornerSize );

        fheroes2::Blit( verticalSprite, cornerSize, bottomCornerSpriteOffsetY, _output, _windowArea.x + cornerSize, bottomCornerOffsetY, extraCornerSize, cornerSize );
        fheroes2::Blit( verticalSprite, 0, verticalSpriteHeight - borderEdgeOffset, _output, _windowArea.x, bottomCornerOffsetY - extraCornerSize, cornerSize,
                        extraCornerSize );

        fheroes2::Blit( verticalSprite, verticalSpriteWidth - borderEdgeOffset, bottomCornerSpriteOffsetY, _output, rightCornerOffsetX - extraCornerSize,
                        bottomCornerOffsetY, extraCornerSize, cornerSize );
        fheroes2::Blit( verticalSprite, rightCornerSpriteOffsetX, verticalSpriteHeight - borderEdgeOffset, _output, rightCornerOffsetX,
                        bottomCornerOffsetY - extraCornerSize, cornerSize, extraCornerSize );

        // Render the background.
        if ( _hasBackground ) {
            const fheroes2::Sprite & backgroundSprite = fheroes2::AGG::GetICN( ( Settings::Get().isEvilInterfaceEnabled() ? ICN::STONEBAK_EVIL : ICN::STONEBAK ), 0 );
            const int32_t backgroundSpriteWidth{ backgroundSprite.width() };
            const int32_t backgroundSpriteHeight{ backgroundSprite.height() };

            const int32_t backgroundWidth = _windowArea.width - cornerSize * 2;
            const int32_t backgroundHeight = _windowArea.height - cornerSize * 2;
            const int32_t backgroundHorizontalCopies = ( backgroundWidth - 1 - transitionSize ) / ( backgroundSpriteWidth - transitionSize );
            const int32_t backgroundVerticalCopies = ( backgroundHeight - 1 - transitionSize ) / ( backgroundSpriteHeight - transitionSize );

            const int32_t backgroundCopyWidth = std::min( backgroundSpriteWidth, backgroundWidth );
            const int32_t backgroundCopyHeight = std::min( backgroundSpriteHeight, backgroundHeight );
            const int32_t backgroundOffsetX = _windowArea.x + cornerSize;
            const int32_t backgroundOffsetY = _windowArea.y + cornerSize;

            // We do a copy as the background image does not have transparent pixels.
            fheroes2::Copy( backgroundSprite, 0, 0, _output, backgroundOffsetX, backgroundOffsetY, backgroundCopyWidth, backgroundCopyHeight );

            // If we need more copies to fill background horizontally we make a transition and copy existing image.
            if ( backgroundHorizontalCopies > 0 ) {
                int32_t toOffsetX = cornerSize + backgroundSpriteWidth;
                fheroes2::DitheringTransition( backgroundSprite, 0, 0, _output, _windowArea.x + toOffsetX - transitionSize, backgroundOffsetY, transitionSize,
                                               backgroundCopyHeight, true, false );

                const int32_t stepX = backgroundSpriteWidth - transitionSize;
                const int32_t fromOffsetX = cornerSize + transitionSize;

                for ( int32_t i = 0; i < backgroundHorizontalCopies; ++i ) {
                    fheroes2::Copy( _output, _windowArea.x + fromOffsetX, backgroundOffsetY, _output, _windowArea.x + toOffsetX, backgroundOffsetY,
                                    std::min( backgroundSpriteWidth, _windowArea.width - cornerSize - toOffsetX ), backgroundCopyHeight );
                    toOffsetX += stepX;
                }
            }

            // If we need more copies to fill background vertically we make a transition and copy existing image in full background width.
            if ( backgroundVerticalCopies > 0 ) {
                int32_t toOffsetY = cornerSize + backgroundSpriteHeight;
                fheroes2::DitheringTransition( _output, backgroundOffsetX, backgroundOffsetY, _output, backgroundOffsetX, _windowArea.y + toOffsetY - transitionSize,
                                               backgroundWidth, transitionSize, false, false );

                const int32_t stepY = backgroundSpriteHeight - transitionSize;
                const int32_t fromOffsetY = cornerSize + transitionSize;

                for ( int32_t i = 0; i < backgroundVerticalCopies; ++i ) {
                    fheroes2::Copy( _output, backgroundOffsetX, _windowArea.y + fromOffsetY, _output, backgroundOffsetX, _windowArea.y + toOffsetY, backgroundWidth,
                                    std::min( backgroundSpriteHeight, _windowArea.height - cornerSize - toOffsetY ) );
                    toOffsetY += stepY;
                }
            }

            // Make a transition from borders to the background in the corners.
            fheroes2::DitheringTransition( verticalSprite, cornerSize, cornerSize, _output, _windowArea.x + cornerSize, _windowArea.y + cornerSize, extraCornerSize,
                                           transitionSize, false, true );
            fheroes2::DitheringTransition( verticalSprite, cornerSize, cornerSize, _output, _windowArea.x + cornerSize, _windowArea.y + cornerSize, transitionSize,
                                           extraCornerSize, true, true );

            fheroes2::DitheringTransition( verticalSprite, verticalSpriteWidth - borderEdgeOffset, cornerSize, _output, rightCornerOffsetX - extraCornerSize,
                                           _windowArea.y + cornerSize, extraCornerSize, transitionSize, false, true );
            fheroes2::DitheringTransition( verticalSprite, rightCornerSpriteOffsetX - transitionSize, cornerSize, _output, rightCornerOffsetX - transitionSize,
                                           _windowArea.y + cornerSize, transitionSize, extraCornerSize, true, false );

            fheroes2::DitheringTransition( verticalSprite, cornerSize, bottomCornerSpriteOffsetY - transitionSize, _output, _windowArea.x + cornerSize,
                                           bottomCornerOffsetY - transitionSize, extraCornerSize, transitionSize, false, false );
            fheroes2::DitheringTransition( verticalSprite, cornerSize, verticalSpriteHeight - borderEdgeOffset, _output, _windowArea.x + cornerSize,
                                           bottomCornerOffsetY - extraCornerSize, transitionSize, extraCornerSize, true, true );

            fheroes2::DitheringTransition( verticalSprite, verticalSpriteWidth - borderEdgeOffset, bottomCornerSpriteOffsetY - transitionSize, _output,
                                           rightCornerOffsetX - extraCornerSize, bottomCornerOffsetY - transitionSize, extraCornerSize, transitionSize, false, false );
            fheroes2::DitheringTransition( verticalSprite, rightCornerSpriteOffsetX - transitionSize, verticalSpriteHeight - borderEdgeOffset, _output,
                                           rightCornerOffsetX - transitionSize, bottomCornerOffsetY - extraCornerSize, transitionSize, extraCornerSize, true, false );
        }

        // Render vertical borders.
        const int32_t verticalSpriteCopyHeight = std::min( _windowArea.height, verticalSpriteHeight ) - borderEdgeOffset * 2;
        const int32_t verticalSpriteCopies
            = ( _windowArea.height - borderEdgeOffset * 2 - 1 - transitionSize ) / ( verticalSpriteHeight - borderEdgeOffset * 2 - transitionSize );
        const int32_t rightBorderOffsetX = _windowArea.x + _windowArea.width - cornerSize;
        const int32_t rightBorderSpriteOffsetX = verticalSpriteWidth - cornerSize;

        fheroes2::Blit( verticalSprite, 0, borderEdgeOffset, _output, _windowArea.x, _windowArea.y + borderEdgeOffset, cornerSize, verticalSpriteCopyHeight );
        fheroes2::Blit( verticalSprite, rightBorderSpriteOffsetX, borderEdgeOffset, _output, rightBorderOffsetX, _windowArea.y + borderEdgeOffset, cornerSize,
                        verticalSpriteCopyHeight );

        // Render a transition to the background.
        if ( _hasBackground ) {
            fheroes2::DitheringTransition( verticalSprite, cornerSize, borderEdgeOffset, _output, _windowArea.x + cornerSize, _windowArea.y + borderEdgeOffset,
                                           transitionSize, verticalSpriteCopyHeight, true, true );
            fheroes2::DitheringTransition( verticalSprite, rightBorderSpriteOffsetX - transitionSize, borderEdgeOffset, _output, rightBorderOffsetX - transitionSize,
                                           _windowArea.y + borderEdgeOffset, transitionSize, verticalSpriteCopyHeight, true, false );
        }

        // If we need more copies to fill vertical borders we make a transition and copy the central part of the border.
        if ( verticalSpriteCopies > 0 ) {
            int32_t toOffsetY = borderEdgeOffset + verticalSpriteCopyHeight;
            const int32_t outputY = _windowArea.y + toOffsetY - transitionSize;
            fheroes2::DitheringTransition( verticalSprite, 0, borderEdgeOffset, _output, _windowArea.x, outputY, cornerSize, transitionSize, false, false );
            fheroes2::DitheringTransition( verticalSprite, rightBorderSpriteOffsetX, borderEdgeOffset, _output, rightBorderOffsetX, outputY, cornerSize, transitionSize,
                                           false, false );

            const int32_t stepY = verticalSpriteCopyHeight - transitionSize;
            const int32_t fromOffsetY = borderEdgeOffset + transitionSize;

            for ( int32_t i = 0; i < verticalSpriteCopies; ++i ) {
                const int32_t copyHeight = std::min( verticalSpriteCopyHeight, _windowArea.height - borderEdgeOffset - toOffsetY );
                const int32_t toY = _windowArea.y + toOffsetY;

                fheroes2::Blit( verticalSprite, 0, fromOffsetY, _output, _windowArea.x, toY, cornerSize, copyHeight );
                fheroes2::Blit( verticalSprite, rightBorderSpriteOffsetX, fromOffsetY, _output, rightBorderOffsetX, toY, cornerSize, copyHeight );

                // Render a transition to the background.
                if ( _hasBackground ) {
                    fheroes2::DitheringTransition( verticalSprite, cornerSize, fromOffsetY, _output, _windowArea.x + cornerSize, toY, transitionSize, copyHeight, true,
                                                   true );
                    fheroes2::DitheringTransition( verticalSprite, rightBorderSpriteOffsetX - transitionSize, fromOffsetY, _output, rightBorderOffsetX - transitionSize,
                                                   toY, transitionSize, copyHeight, true, false );
                }

                toOffsetY += stepY;
            }
        }

        // Make a transition to the bottom corners.
        const int32_t verticalSpriteBottomCornerEdgeY = verticalSpriteHeight - borderEdgeOffset - transitionSize;
        const int32_t optputBottomCornerEdgeY = _windowArea.y + _windowArea.height - borderEdgeOffset - transitionSize;
        fheroes2::DitheringTransition( verticalSprite, 0, verticalSpriteBottomCornerEdgeY, _output, _windowArea.x, optputBottomCornerEdgeY, cornerSize, transitionSize,
                                       false, false );
        fheroes2::DitheringTransition( verticalSprite, rightBorderSpriteOffsetX, verticalSpriteBottomCornerEdgeY, _output, rightBorderOffsetX, optputBottomCornerEdgeY,
                                       cornerSize, transitionSize, false, false );

        // Render horizontal borders. We have to remember that 'verticalSprite' has 16 (equals to BORDERWIDTH) pixels of shadow at the left and bottom sides.
        const int32_t horizontalSpriteCopyWidth = std::min( _windowArea.width, horizontalSpriteWidth ) - borderEdgeOffset * 2;
        const int32_t horizontalSpriteCopies
            = ( _windowArea.width - borderEdgeOffset * 2 - 1 - transitionSize ) / ( horizontalSpriteWidth - borderEdgeOffset * 2 - transitionSize );
        const int32_t bottomBorderOffsetY = _windowArea.y + _windowArea.height - cornerSize;
        const int32_t bottomBorderSpriteOffsetY = horizontalSpriteHeight - cornerSize;
        const int32_t horizontalSpriteCopyStartX = borderEdgeOffset + BORDERWIDTH;

        fheroes2::Blit( horizontalSprite, horizontalSpriteCopyStartX, 0, _output, _windowArea.x + borderEdgeOffset, _windowArea.y, horizontalSpriteCopyWidth,
                        cornerSize );
        fheroes2::Blit( horizontalSprite, horizontalSpriteCopyStartX, bottomBorderSpriteOffsetY, _output, _windowArea.x + borderEdgeOffset, bottomBorderOffsetY,
                        horizontalSpriteCopyWidth, cornerSize );

        // Render a transition to the background.
        if ( _hasBackground ) {
            fheroes2::DitheringTransition( horizontalSprite, horizontalSpriteCopyStartX, cornerSize, _output, _windowArea.x + borderEdgeOffset,
                                           _windowArea.y + cornerSize, horizontalSpriteCopyWidth, transitionSize, false, true );
            fheroes2::DitheringTransition( horizontalSprite, horizontalSpriteCopyStartX, bottomBorderSpriteOffsetY - transitionSize, _output,
                                           _windowArea.x + borderEdgeOffset, bottomBorderOffsetY - transitionSize, horizontalSpriteCopyWidth, transitionSize, false,
                                           false );
        }

        // If we need more copies to fill horizontal borders we make a transition and copy the central part of the border.
        if ( horizontalSpriteCopies > 0 ) {
            int32_t toOffsetX = borderEdgeOffset + horizontalSpriteCopyWidth;
            const int32_t outputX = _windowArea.x + toOffsetX - transitionSize;
            fheroes2::DitheringTransition( horizontalSprite, horizontalSpriteCopyStartX, 0, _output, outputX, _windowArea.y, transitionSize, cornerSize, true, false );
            fheroes2::DitheringTransition( horizontalSprite, horizontalSpriteCopyStartX, bottomBorderSpriteOffsetY, _output, outputX, bottomBorderOffsetY, transitionSize,
                                           cornerSize, true, false );

            const int32_t stepX = horizontalSpriteCopyWidth - transitionSize;
            const int32_t fromOffsetX = horizontalSpriteCopyStartX + transitionSize;

            for ( int32_t i = 0; i < horizontalSpriteCopies; ++i ) {
                const int32_t copyWidth = std::min( horizontalSpriteCopyWidth, _windowArea.width - borderEdgeOffset - toOffsetX );
                const int32_t toX = _windowArea.x + toOffsetX;

                fheroes2::Blit( horizontalSprite, fromOffsetX, 0, _output, toX, _windowArea.y, copyWidth, cornerSize );
                fheroes2::Blit( horizontalSprite, fromOffsetX, bottomBorderSpriteOffsetY, _output, toX, bottomBorderOffsetY, copyWidth, cornerSize );

                // Render a transition to the background.
                if ( _hasBackground ) {
                    fheroes2::DitheringTransition( horizontalSprite, fromOffsetX, cornerSize, _output, toX, _windowArea.y + cornerSize, copyWidth, transitionSize, false,
                                                   true );
                    fheroes2::DitheringTransition( horizontalSprite, fromOffsetX, bottomBorderSpriteOffsetY - transitionSize, _output, toX,
                                                   bottomBorderOffsetY - transitionSize, copyWidth, transitionSize, false, false );
                }

                toOffsetX += stepX;
            }
        }

        // Make a transition to the right corners.
        const int32_t horizontalSpriteRightCornerEdgeX = horizontalSprite.width() - borderEdgeOffset - transitionSize;
        const int32_t optputRightCornerEdgeX = _windowArea.x + _windowArea.width - borderEdgeOffset - transitionSize;
        fheroes2::DitheringTransition( horizontalSprite, horizontalSpriteRightCornerEdgeX, 0, _output, optputRightCornerEdgeX, _windowArea.y, transitionSize, cornerSize,
                                       true, false );
        fheroes2::DitheringTransition( horizontalSprite, horizontalSpriteRightCornerEdgeX, bottomBorderSpriteOffsetY, _output, optputRightCornerEdgeX,
                                       bottomBorderOffsetY, transitionSize, cornerSize, true, false );

        // Render shadow.
        fheroes2::ApplyTransform( _output, _windowArea.x - borderSize, _windowArea.y + borderSize, 1, _windowArea.height - borderSize, 5 );
        fheroes2::ApplyTransform( _output, _windowArea.x - borderSize + 1, _windowArea.y + borderSize, 1, _windowArea.height - borderSize, 4 );
        fheroes2::ApplyTransform( _output, _windowArea.x - borderSize + 2, _windowArea.y + borderSize, borderSize - 2, _windowArea.height - borderSize, 3 );
        fheroes2::ApplyTransform( _output, _windowArea.x - borderSize, _windowArea.y + _windowArea.height, _windowArea.width, borderSize - 2, 3 );
        fheroes2::ApplyTransform( _output, _windowArea.x - borderSize, _windowArea.y + _windowArea.height + borderSize - 2, _windowArea.width, 1, 4 );
        fheroes2::ApplyTransform( _output, _windowArea.x - borderSize, _windowArea.y + _windowArea.height + borderSize - 1, _windowArea.width, 1, 5 );
    }
}
