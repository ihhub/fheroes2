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

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h" // IWYU pragma: associated
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "payment.h"
#include "resource.h"
#include "screen.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_text.h"

int Dialog::BuyBoat( bool enable )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    Resource::BoxSprite rbs( PaymentConditions::BuyBoat(), fheroes2::boxAreaWidthPx );

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::BOATWIND, 0 );
    fheroes2::Text text{ _( "Build a new ship:" ), fheroes2::FontType::normalWhite() };
    const int spacer = 10;

    Dialog::FrameBox box( text.height() + spacer + sprite.height() + spacer + text.height() + spacer + rbs.GetArea().height - 20, true );

    const fheroes2::Rect & box_rt = box.GetArea();
    fheroes2::Point dst_pt( box_rt.x + ( box_rt.width - text.width() ) / 2, box_rt.y );
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = box_rt.x + ( box_rt.width - sprite.width() ) / 2;
    dst_pt.y = box_rt.y + text.height() + spacer;
    fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );

    text.set( _( "Resource cost:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = box_rt.x + ( box_rt.width - text.width() ) / 2;
    dst_pt.y = dst_pt.y + sprite.height() + spacer;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    rbs.SetPos( box_rt.x, dst_pt.y + spacer );
    rbs.Redraw();

    // buttons
    fheroes2::ButtonGroup buttonGroup( box_rt, Dialog::OK | Dialog::CANCEL );
    fheroes2::ButtonBase & buttonOkay = buttonGroup.button( 0 );

    if ( !enable ) {
        buttonOkay.press();
        buttonOkay.disable();
    }

    buttonGroup.draw();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        const int result = buttonGroup.processEvents();
        if ( result != Dialog::ZERO ) {
            return result;
        }
    }

    return Dialog::ZERO;
}
