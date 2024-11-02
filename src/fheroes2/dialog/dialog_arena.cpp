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
#include <string>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h" // IWYU pragma: associated
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace
{
    void InfoSkillClear( const fheroes2::Rect & rect1, const fheroes2::Rect & rect2, const fheroes2::Rect & rect3 )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::XPRIMARY, 0 ), display, rect1.x, rect1.y );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::XPRIMARY, 1 ), display, rect2.x, rect2.y );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::XPRIMARY, 2 ), display, rect3.x, rect3.y );
    }

    void InfoSkillSelect( int skill, const fheroes2::Rect & rect1, const fheroes2::Rect & rect2, const fheroes2::Rect & rect3 )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        switch ( skill ) {
        case Skill::Primary::ATTACK:
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::XPRIMARY, 4 ), display, rect1.x, rect1.y );
            break;
        case Skill::Primary::DEFENSE:
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::XPRIMARY, 5 ), display, rect2.x, rect2.y );
            break;
        case Skill::Primary::POWER:
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::XPRIMARY, 6 ), display, rect3.x, rect3.y );
            break;
        default:
            break;
        }
    }

    int InfoSkillNext( int skill )
    {
        switch ( skill ) {
        case Skill::Primary::ATTACK:
            return Skill::Primary::DEFENSE;
        case Skill::Primary::DEFENSE:
            return Skill::Primary::POWER;
        default:
            break;
        }

        return Skill::Primary::UNKNOWN;
    }

    int InfoSkillPrev( int skill )
    {
        switch ( skill ) {
        case Skill::Primary::POWER:
            return Skill::Primary::DEFENSE;
        case Skill::Primary::DEFENSE:
            return Skill::Primary::ATTACK;
        default:
            break;
        }

        return Skill::Primary::UNKNOWN;
    }
}

int Dialog::SelectSkillFromArena()
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const int system = Settings::Get().isEvilInterfaceEnabled() ? ICN::SYSTEME : ICN::SYSTEM;

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::Text title( _( "Arena" ), fheroes2::FontType::normalYellow() );

    fheroes2::Text textbox(
        _( "You enter the arena and face a pack of vicious lions. You handily defeat them, to the wild cheers of the crowd. Impressed by your skill, the aged trainer of gladiators agrees to train you in a skill of your choice." ),
        fheroes2::FontType::normalWhite() );
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::XPRIMARY, 0 );
    const int spacer = 10;

    const Dialog::FrameBox box( title.height( fheroes2::boxAreaWidthPx ) + textbox.height( fheroes2::boxAreaWidthPx ) + 2 * spacer + sprite.height() + 15, true );

    const fheroes2::Rect & box_rt = box.GetArea();
    fheroes2::Point dst_pt( box_rt.x, box_rt.y );

    title.draw( dst_pt.x, dst_pt.y + 2, fheroes2::boxAreaWidthPx, display );
    dst_pt.y += title.height( fheroes2::boxAreaWidthPx ) + spacer;

    textbox.draw( dst_pt.x, dst_pt.y + 2, fheroes2::boxAreaWidthPx, display );
    dst_pt.y += textbox.height( fheroes2::boxAreaWidthPx ) + spacer;

    int res = Skill::Primary::ATTACK;

    const int spacingX = ( box_rt.width - sprite.width() * 3 ) / 4;

    fheroes2::Rect rect1( dst_pt.x + spacingX, dst_pt.y, sprite.width(), sprite.height() );
    fheroes2::Rect rect2( rect1.x + sprite.width() + spacingX, dst_pt.y, sprite.width(), sprite.height() );
    fheroes2::Rect rect3( rect2.x + sprite.width() + spacingX, dst_pt.y, sprite.width(), sprite.height() );

    InfoSkillClear( rect1, rect2, rect3 );
    InfoSkillSelect( res, rect1, rect2, rect3 );

    // info texts
    const int32_t skillTextWidth = 60;

    fheroes2::Text text( Skill::Primary::String( Skill::Primary::ATTACK ), fheroes2::FontType::smallWhite() );
    dst_pt.x = rect1.x + ( rect1.width - skillTextWidth ) / 2;
    dst_pt.y = rect1.y + rect1.height + 5;
    text.draw( dst_pt.x, dst_pt.y + 2, skillTextWidth, display );

    text.set( Skill::Primary::String( Skill::Primary::DEFENSE ), fheroes2::FontType::smallWhite() );
    dst_pt.x = rect2.x + ( rect2.width - skillTextWidth ) / 2;
    dst_pt.y = rect2.y + rect2.height + 5;
    text.draw( dst_pt.x, dst_pt.y + 2, skillTextWidth, display );

    text.set( Skill::Primary::String( Skill::Primary::POWER ), fheroes2::FontType::smallWhite() );
    dst_pt.x = rect3.x + ( rect3.width - skillTextWidth ) / 2;
    dst_pt.y = rect3.y + rect3.height + 5;
    text.draw( dst_pt.x, dst_pt.y + 2, skillTextWidth, display );

    // buttons
    dst_pt.x = box_rt.x + ( box_rt.width - fheroes2::AGG::GetICN( system, 1 ).width() ) / 2;
    dst_pt.y = box_rt.y + box_rt.height - fheroes2::AGG::GetICN( system, 1 ).height();
    fheroes2::Button buttonOk( dst_pt.x, dst_pt.y, system, 1, 2 );

    LocalEvent & le = LocalEvent::Get();

    buttonOk.draw();
    display.render();

    // message loop
    while ( le.HandleEvents() ) {
        bool redraw = false;

        le.isMouseLeftButtonPressedInArea( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_LEFT ) && Skill::Primary::UNKNOWN != InfoSkillPrev( res ) ) {
            res = InfoSkillPrev( res );
            redraw = true;
        }
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_RIGHT ) && Skill::Primary::UNKNOWN != InfoSkillNext( res ) ) {
            res = InfoSkillNext( res );
            redraw = true;
        }
        else if ( le.MouseClickLeft( rect1 ) ) {
            res = Skill::Primary::ATTACK;
            redraw = true;
        }
        else if ( le.MouseClickLeft( rect2 ) ) {
            res = Skill::Primary::DEFENSE;
            redraw = true;
        }
        else if ( le.MouseClickLeft( rect3 ) ) {
            res = Skill::Primary::POWER;
            redraw = true;
        }
        else if ( le.isMouseRightButtonPressedInArea( rect1 ) ) {
            fheroes2::PrimarySkillDialogElement( Skill::Primary::ATTACK, "" ).showPopup( Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( rect2 ) ) {
            fheroes2::PrimarySkillDialogElement( Skill::Primary::DEFENSE, "" ).showPopup( Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( rect3 ) ) {
            fheroes2::PrimarySkillDialogElement( Skill::Primary::POWER, "" ).showPopup( Dialog::ZERO );
        }

        if ( redraw ) {
            InfoSkillClear( rect1, rect2, rect3 );
            InfoSkillSelect( res, rect1, rect2, rect3 );
            display.render();
        }

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( buttonOk.area() ) )
            break;
    }

    return res;
}
