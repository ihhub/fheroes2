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
#include "artifact.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "settings.h"
#include "text.h"
#include "ui_button.h"

int Dialog::ArtifactInfo( const std::string & hdr, const std::string & msg, const Artifact & art, int buttons )
{
    const fheroes2::Sprite & border = fheroes2::AGG::GetICN( ICN::RESOURCE, 7 );
    const fheroes2::Sprite & artifact = fheroes2::AGG::GetICN( ICN::ARTIFACT, art.IndexSprite64() );

    fheroes2::Image image = border;
    fheroes2::Blit( artifact, image, 5, 5 );

    std::string ext = msg;
    ext.append( "\n" );
    ext.append( " " );
    ext.append( "\n" );
    ext.append( art.GetDescription() );

    return Dialog::SpriteInfo( hdr, ext, image, buttons );
}

int Dialog::SpriteInfo( const std::string & header, const std::string & message, const fheroes2::Image & sprite, int buttons )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // cursor
    Cursor & cursor = Cursor::Get();

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    TextBox box1( header, Font::YELLOW_BIG, BOXAREA_WIDTH );
    TextBox box2( message, Font::BIG, BOXAREA_WIDTH );
    const int spacer = Settings::Get().QVGA() ? 5 : 10;

    FrameBox box( box1.h() + spacer + box2.h() + spacer + sprite.height(), buttons );
    Rect pos = box.GetArea();

    if ( header.size() )
        box1.Blit( pos );
    pos.y += box1.h() + spacer;

    if ( message.size() )
        box2.Blit( pos );
    pos.y += box2.h() + spacer;

    // blit sprite
    pos.x = box.GetArea().x + ( pos.w - sprite.width() ) / 2;
    fheroes2::Blit( sprite, display, pos.x, pos.y );

    LocalEvent & le = LocalEvent::Get();

    fheroes2::ButtonGroup btnGroups( fheroes2::Rect( box.GetArea().x, box.GetArea().y, box.GetArea().w, box.GetArea().h ), buttons );
    btnGroups.draw();

    cursor.Show();
    display.render();

    // message loop
    int result = Dialog::ZERO;

    while ( result == Dialog::ZERO && le.HandleEvents() ) {
        if ( !buttons && !le.MousePressRight() )
            break;
        result = btnGroups.processEvents();
    }

    cursor.Hide();
    return result;
}
