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

#include <cstdint>
#include <string>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "localevent.h"
#include "math_base.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_text.h"
#include "world.h"

bool Dialog::SelectGoldOrExp( const std::string & header, const std::string & message, uint32_t gold, uint32_t expr, const Heroes & hero )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::Sprite & sprite_gold = fheroes2::AGG::GetICN( ICN::RESOURCE, 6 );
    const fheroes2::Sprite & sprite_expr = fheroes2::AGG::GetICN( ICN::EXPMRL, 4 );

    fheroes2::Text headerText( header, fheroes2::FontType::normalYellow() );
    fheroes2::Text messageText( message, fheroes2::FontType::normalWhite() );

    fheroes2::Text text{ std::to_string( gold ) + " (" + _( "Total: " ) + std::to_string( world.GetKingdom( hero.GetColor() ).GetFunds().Get( Resource::GOLD ) ) + ")",
                         fheroes2::FontType::smallWhite() };

    const int spacer = 10;
    FrameBox box( headerText.height( BOXAREA_WIDTH ) + spacer + messageText.height( BOXAREA_WIDTH ) + spacer + sprite_expr.height() + 2 + text.height(), true );

    fheroes2::Point pt;

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const int buttonYesIcnID = isEvilInterface ? ICN::BUTTON_SMALL_YES_EVIL : ICN::BUTTON_SMALL_YES_GOOD;
    const int buttonNoIcnID = isEvilInterface ? ICN::BUTTON_SMALL_NO_EVIL : ICN::BUTTON_SMALL_NO_GOOD;

    pt.x = box.GetArea().x + box.GetArea().width / 2 - fheroes2::AGG::GetICN( buttonYesIcnID, 0 ).width() - 20;
    pt.y = box.GetArea().y + box.GetArea().height - fheroes2::AGG::GetICN( buttonYesIcnID, 0 ).height();
    fheroes2::Button button_yes( pt.x, pt.y, buttonYesIcnID, 0, 1 );

    pt.x = box.GetArea().x + box.GetArea().width / 2 + 20;
    pt.y = box.GetArea().y + box.GetArea().height - fheroes2::AGG::GetICN( buttonNoIcnID, 0 ).height();
    fheroes2::Button button_no( pt.x, pt.y, buttonNoIcnID, 0, 1 );

    fheroes2::Rect pos = box.GetArea();

    if ( !header.empty() ) {
        headerText.draw( pos.x, pos.y + 2, BOXAREA_WIDTH, display );
    }

    pos.y += headerText.height( BOXAREA_WIDTH ) + spacer;

    if ( !message.empty() ) {
        messageText.draw( pos.x, pos.y + 2, BOXAREA_WIDTH, display );
    }

    pos.y += messageText.height( BOXAREA_WIDTH ) + spacer;

    pos.y += sprite_expr.height();
    // sprite1
    pos.x = box.GetArea().x + box.GetArea().width / 2 - sprite_gold.width() - 30;
    fheroes2::Blit( sprite_gold, display, pos.x, pos.y - sprite_gold.height() );
    // text
    text.draw( pos.x + sprite_gold.width() / 2 - text.width() / 2, pos.y + 4, display );

    // sprite2
    pos.x = box.GetArea().x + box.GetArea().width / 2 + 30;
    fheroes2::Blit( sprite_expr, display, pos.x, pos.y - sprite_expr.height() );
    // text
    text.set( std::to_string( expr ) + " (" + _( "Need: " ) + std::to_string( Heroes::GetExperienceFromLevel( hero.GetLevel() ) - hero.GetExperience() ) + ")",
              fheroes2::FontType::smallWhite() );
    text.draw( pos.x + sprite_expr.width() / 2 - text.width() / 2, pos.y + 4, display );

    button_yes.draw();
    button_no.draw();

    display.render();
    LocalEvent & le = LocalEvent::Get();
    bool result = false;

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( button_yes.area() ) ? button_yes.drawOnPress() : button_yes.drawOnRelease();
        le.MousePressLeft( button_no.area() ) ? button_no.drawOnPress() : button_no.drawOnRelease();

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( button_yes.area() ) ) {
            result = true;
            break;
        }
        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( button_no.area() ) ) {
            result = false;
            break;
        }
    }

    return result;
}
