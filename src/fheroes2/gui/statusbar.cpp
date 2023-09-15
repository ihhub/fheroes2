/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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
#include "screen.h"
#include "tools.h"

StatusBar::StatusBar()
    : MovableText( fheroes2::Display::instance() )
{
    // Do nothing.
}

void StatusBar::ShowMessage( std::string msg )
{
    if ( msg == _prevMessage ) {
        // No updates.
        return;
    }

    _prevMessage = std::move( msg );

    hide();

    auto text = std::make_unique<fheroes2::Text>( _prevMessage, fheroes2::FontType::normalWhite() );
    if ( text->width() > _roi.width ) {
        text->fitToOneRow( _roi.width );
    }

    fheroes2::Rect messageRoi{ _roi.x + ( _roi.width - text->width() ) / 2, _roi.y, text->width(), text->height() };

    update( std::move( text ) );

    draw( messageRoi.x, messageRoi.y );

    fheroes2::Display::instance().render( fheroes2::getBoundaryRect( _prevMessageRoi, messageRoi ) );
    _prevMessageRoi = std::move( messageRoi );
}
