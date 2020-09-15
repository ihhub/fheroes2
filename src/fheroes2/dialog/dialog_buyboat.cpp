/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include <string>

#include "agg.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "kingdom.h"
#include "payment.h"
#include "settings.h"
#include "text.h"
#include "ui_button.h"
#include "world.h"

int Dialog::BuyBoat( bool enable )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const int system = Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM;

    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    Resource::BoxSprite rbs( PaymentConditions::BuyBoat(), BOXAREA_WIDTH );

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::BOATWIND, 0 );
    Text text( _( "Build a new ship:" ), Font::BIG );
    const int spacer = 10;

    Dialog::FrameBox box( text.h() + spacer + sprite.height() + spacer + text.h() + spacer + rbs.GetArea().h - 20, true );

    const Rect & box_rt = box.GetArea();
    Point dst_pt;
    dst_pt.x = box_rt.x + ( box_rt.w - text.w() ) / 2;
    dst_pt.y = box_rt.y;
    text.Blit( dst_pt );

    dst_pt.x = box_rt.x + ( box_rt.w - sprite.width() ) / 2;
    dst_pt.y = box_rt.y + text.h() + spacer;
    fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );

    text.Set( _( "Resource cost:" ), Font::BIG );
    dst_pt.x = box_rt.x + ( box_rt.w - text.w() ) / 2;
    dst_pt.y = dst_pt.y + sprite.height() + spacer;
    text.Blit( dst_pt );

    rbs.SetPos( box_rt.x, dst_pt.y + spacer );
    rbs.Redraw();

    // buttons
    dst_pt.x = box_rt.x;
    dst_pt.y = box_rt.y + box_rt.h - fheroes2::AGG::GetICN( system, 1 ).height();
    fheroes2::Button button1( dst_pt.x, dst_pt.y, system, 1, 2 );

    dst_pt.x = box_rt.x + box_rt.w - fheroes2::AGG::GetICN( system, 3 ).width();
    dst_pt.y = box_rt.y + box_rt.h - fheroes2::AGG::GetICN( system, 3 ).height();
    fheroes2::Button button2( dst_pt.x, dst_pt.y, system, 3, 4 );

    if ( !enable ) {
        button1.press();
        button1.disable();
    }

    button1.draw();
    button2.draw();

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        if ( button1.isEnabled() )
            le.MousePressLeft( button1.area() ) ? button1.drawOnPress() : button1.drawOnRelease();
        le.MousePressLeft( button2.area() ) ? button2.drawOnPress() : button2.drawOnRelease();

        if ( button1.isEnabled() && ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) || le.MouseClickLeft( button1.area() ) ) )
            return Dialog::OK;

        if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( button2.area() ) )
            return Dialog::CANCEL;
    }

    return Dialog::ZERO;
}
