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
#include "heroes.h"
#include "kingdom.h"
#include "resource.h"
#include "settings.h"
#include "text.h"
#include "world.h"

void Castle::OpenTavern( void )
{
    const std::string & header = _( "A generous tip for the barkeep yields the following rumor:" );
    const int system = ( Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM );
    const int tavwin = ICN::TAVWIN;
    const std::string & tavern = GetStringBuilding( BUILD_TAVERN );
    const std::string & message = world.GetRumors();

    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    Text text( tavern, Font::YELLOW_BIG );
    const fheroes2::Sprite & s1 = fheroes2::AGG::GetICN( tavwin, 0 );
    TextBox box1( header, Font::BIG, BOXAREA_WIDTH );
    TextBox box2( message, Font::BIG, BOXAREA_WIDTH );

    Dialog::FrameBox box( text.h() + 10 + s1.height() + 13 + box1.h() + 20 + box2.h(), true );

    const fheroes2::Rect & pos = box.GetArea();
    fheroes2::Point dst_pt( pos.x, pos.y );

    text.Blit( pos.x + ( pos.width - text.w() ) / 2, dst_pt.y );

    dst_pt.x = pos.x + ( pos.width - s1.width() ) / 2;
    dst_pt.y += 10 + text.h();
    fheroes2::Blit( s1, display, dst_pt.x, dst_pt.y );

    dst_pt.x += 3;
    dst_pt.y += 3;

    const fheroes2::Sprite & tavernSprite = fheroes2::AGG::GetICN( tavwin, 1 );
    fheroes2::Blit( tavernSprite, display, dst_pt.x, dst_pt.y );

    if ( const u32 index = ICN::AnimationFrame( tavwin, 0, 0 ) ) {
        const fheroes2::Sprite & animation = fheroes2::AGG::GetICN( tavwin, index );
        fheroes2::Blit( animation, display, dst_pt.x + animation.x(), dst_pt.y + animation.y() );
    }

    box1.Blit( pos.x, dst_pt.y + s1.height() + 10 );
    box2.Blit( pos.x, dst_pt.y + s1.height() + 10 + box1.h() + 20 );

    // button yes
    const fheroes2::Sprite & s4 = fheroes2::AGG::GetICN( system, 5 );
    fheroes2::Button buttonYes( pos.x + ( pos.width - s4.width() ) / 2, pos.y + pos.height - s4.height(), system, 5, 6 );

    buttonYes.draw();

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();
    u32 frame = 0;

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonYes.area() ) ? buttonYes.drawOnPress() : buttonYes.drawOnRelease();
        if ( le.MouseClickLeft( buttonYes.area() ) || HotKeyCloseWindow )
            break;

        // animation
        if ( Game::AnimateInfrequentDelay( Game::CASTLE_TAVERN_DELAY ) ) {
            cursor.Hide();
            fheroes2::Blit( tavernSprite, display, dst_pt.x, dst_pt.y );

            if ( const u32 index = ICN::AnimationFrame( tavwin, 0, frame++ ) ) {
                const fheroes2::Sprite & s22 = fheroes2::AGG::GetICN( tavwin, index );
                fheroes2::Blit( s22, display, dst_pt.x + s22.x(), dst_pt.y + s22.y() );
            }

            cursor.Show();
            display.render();
        }
    }
}
