/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "statusbar.h"

#include <cstdint>
#include <memory>
#include <utility>

#include "math_tools.h"
#include "screen.h"
#include "ui_text.h"

StatusBar::StatusBar()
    : MovableText( fheroes2::Display::instance() )
{
    // Do nothing.
}

fheroes2::Rect StatusBar::updateMessage( std::string msg )
{
    if ( msg == _prevMessage ) {
        // No updates.
        return {};
    }

    _prevMessage = msg;

    // Create text with normal white font (with shadow)
    auto text = std::make_unique<fheroes2::Text>( std::move( msg ), fheroes2::FontType::normalWhite() );

    // Cut off the end of the text if it exceeds the given width
    text->fitToOneRow( _roi.width );

    const int32_t textWidth = text->width();
    const fheroes2::Rect messageRoi{ _roi.x + ( _roi.width - textWidth ) / 2, _roi.y + 1, textWidth, text->height() };

    update( std::move( text ) );

    // Draw text aligned and cutoff with the ROI, accounting for the +3 ROI offset
    drawInRoi( messageRoi.x, messageRoi.y + 2, messageRoi );

    auto area = fheroes2::getBoundaryRect( _prevMessageRoi, messageRoi );

    _prevMessageRoi = messageRoi;

    return area;
}

void StatusBar::ShowMessage( std::string msg )
{
    const auto area = updateMessage( std::move( msg ) );
    if ( area == fheroes2::Rect{} ) {
        return;
    }

    fheroes2::Display::instance().render( area );
}
