/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#include <cstdint>

#include "agg_image.h"
#include "dialog.h" // IWYU pragma: associated
#include "icn.h"
#include "image.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"

Dialog::FrameBorder::FrameBorder( int v )
    : restorer( fheroes2::Display::instance(), 0, 0, 0, 0 )
    , border( v )
{}

void Dialog::FrameBorder::SetPosition( int32_t posx, int32_t posy, int32_t encw, int32_t ench )
{
    restorer.restore();

    rect.x = posx;
    rect.y = posy;

    if ( encw > 0 && ench > 0 ) {
        rect.width = encw + 2 * border;
        rect.height = ench + 2 * border;

        restorer.update( rect.x, rect.y, rect.width, rect.height );

        area.width = encw;
        area.height = ench;
    }
    else {
        restorer.update( posx, posy, restorer.width(), restorer.height() );
    }

    area.x = posx + border;
    area.y = posy + border;

    top = fheroes2::Rect( rect.x, rect.y, rect.width, border );
}

void Dialog::FrameBorder::RenderRegular( const fheroes2::Rect & dstrt )
{
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ( Settings::Get().isEvilInterfaceEnabled() ? ICN::SURDRBKE : ICN::SURDRBKG ), 0 );
    const fheroes2::Image renderedImage = fheroes2::Stretch( sprite, fheroes2::shadowWidthPx, 0, sprite.width() - fheroes2::shadowWidthPx,
                                                             sprite.height() - fheroes2::shadowWidthPx, dstrt.width, dstrt.height );
    fheroes2::Blit( renderedImage, fheroes2::Display::instance(), dstrt.x, dstrt.y );
}

void Dialog::FrameBorder::RenderOther( const fheroes2::Image & srcsf, const fheroes2::Rect & dstrt )
{
    const fheroes2::Image renderedImage = fheroes2::Stretch( srcsf, 0, 0, srcsf.width(), srcsf.height(), dstrt.width, dstrt.height );
    fheroes2::Blit( renderedImage, fheroes2::Display::instance(), dstrt.x, dstrt.y );
}
