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

#include "ui_window.h"
#include "agg_image.h"
#include "icn.h"
#include "settings.h"

namespace
{
    const int32_t borderSize = 16;
}

namespace fheroes2
{
    StandardWindow::StandardWindow( const int32_t width, const int32_t height, Image & output )
        : _output( output )
        , _activeArea( ( output.width() - width ) / 2, ( output.height() - height ) / 2, width, height )
        , _windowArea( _activeArea.x - borderSize, _activeArea.y - borderSize, _activeArea.width + 2 * borderSize, _activeArea.height + 2 * borderSize )
        , _restorer( output, _windowArea.x - borderSize, _windowArea.y, _windowArea.width + borderSize, _windowArea.height + borderSize )
    {
        render();
    }

    StandardWindow::StandardWindow( const int32_t x, const int32_t y, const int32_t width, const int32_t height, Image & output )
        : _output( output )
        , _activeArea( x, y, width, height )
        , _windowArea( _activeArea.x - borderSize, _activeArea.y - borderSize, _activeArea.width + 2 * borderSize, _activeArea.height + 2 * borderSize )
        , _restorer( output, _windowArea.x - borderSize, _windowArea.y, _windowArea.width + borderSize, _windowArea.height + borderSize )
    {
        render();
    }

    void StandardWindow::render()
    {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ( Settings::Get().ExtGameEvilInterface() ? ICN::SURDRBKE : ICN::SURDRBKG ), 0 );
        const fheroes2::Image renderedImage
            = fheroes2::Stretch( sprite, borderSize, 0, sprite.width() - borderSize, sprite.height() - borderSize, _windowArea.width, _windowArea.height );
        fheroes2::Blit( renderedImage, _output, _windowArea.x, _windowArea.y );

        fheroes2::ApplyTransform( _output, _windowArea.x - borderSize, _windowArea.y + borderSize, 1, _windowArea.height - borderSize, 5 );
        fheroes2::ApplyTransform( _output, _windowArea.x - borderSize + 1, _windowArea.y + borderSize, 1, _windowArea.height - borderSize, 4 );
        fheroes2::ApplyTransform( _output, _windowArea.x - borderSize + 2, _windowArea.y + borderSize, borderSize - 2, _windowArea.height - borderSize, 3 );
        fheroes2::ApplyTransform( _output, _windowArea.x - borderSize, _windowArea.y + _windowArea.height, _windowArea.width, borderSize - 2, 3 );
        fheroes2::ApplyTransform( _output, _windowArea.x - borderSize, _windowArea.y + _windowArea.height + borderSize - 2, _windowArea.width, 1, 4 );
        fheroes2::ApplyTransform( _output, _windowArea.x - borderSize, _windowArea.y + _windowArea.height + borderSize - 1, _windowArea.width, 1, 5 );
    }
}
