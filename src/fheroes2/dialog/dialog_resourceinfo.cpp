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

#include "agg.h"
#include "cursor.h"
#include "dialog.h"
#include "resource.h"
#include "text.h"
#include "ui_button.h"

int Dialog::ResourceInfo( const std::string & header, const std::string & message, const Funds & rs, int buttons )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // cursor
    Cursor & cursor = Cursor::Get();

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    TextBox box1( header, Font::YELLOW_BIG, BOXAREA_WIDTH );
    TextBox box2( message, Font::BIG, BOXAREA_WIDTH );
    Resource::BoxSprite rbs( rs, BOXAREA_WIDTH );

    const int spacer = 10;

    FrameBox box( box1.h() + spacer + box2.h() + spacer + rbs.GetArea().h, buttons != 0 );
    Point pos = box.GetArea();

    if ( header.size() )
        box1.Blit( pos );
    pos.y += box1.h() + spacer;

    if ( message.size() )
        box2.Blit( pos );
    pos.y += box2.h() + spacer;

    rbs.SetPos( pos.x, pos.y );
    rbs.Redraw();

    LocalEvent & le = LocalEvent::Get();

    fheroes2::ButtonGroup btnGroups( fheroes2::Rect( box.GetArea().x, box.GetArea().y, box.GetArea().w, box.GetArea().h ), buttons );
    btnGroups.draw();

    cursor.Show();
    display.render();

    int result = Dialog::ZERO;

    while ( result == Dialog::ZERO && le.HandleEvents() ) {
        if ( !buttons && !le.MousePressRight() )
            break;
        result = btnGroups.processEvents();
    }

    return result;
}
