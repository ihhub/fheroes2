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
#include "game.h"
#include "profit.h"
#include "settings.h"
#include "skill.h"
#include "text.h"
#include "ui_button.h"

void Dialog::SecondarySkillInfo( const Skill::Secondary & skill, const bool ok_button )
{
    SecondarySkillInfo( skill.GetName(), skill.GetDescription(), skill, ok_button );
}

void Dialog::SecondarySkillInfo( const std::string & header, const std::string & message, const Skill::Secondary & skill, const bool ok_button )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const int system = Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM;

    // cursor
    Cursor & cursor = Cursor::Get();

    cursor.SetThemes( cursor.POINTER );

    TextBox box1( header, Font::YELLOW_BIG, BOXAREA_WIDTH );
    TextBox box2( message, Font::BIG, BOXAREA_WIDTH );
    const fheroes2::Sprite & border = fheroes2::AGG::GetICN( ICN::SECSKILL, 15 );
    const int spacer = 10;

    FrameBox box( box1.h() + spacer + box2.h() + spacer + border.height(), ok_button );
    fheroes2::Rect pos = box.GetArea();

    if ( header.size() )
        box1.Blit( pos.x, pos.y );
    pos.y += box1.h() + spacer;

    if ( message.size() )
        box2.Blit( pos.x, pos.y );
    pos.y += box2.h() + spacer;

    // blit sprite
    pos.x = box.GetArea().x + ( pos.width - border.width() ) / 2;
    fheroes2::Blit( border, display, pos.x, pos.y );

    const fheroes2::Rect skillInfoArea( pos.x, pos.y, border.width(), border.height() );

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::SECSKILL, skill.GetIndexSprite1() );
    pos.x = box.GetArea().x + ( pos.width - sprite.width() ) / 2;
    fheroes2::Blit( sprite, display, pos.x, pos.y + 3 );

    Text text;

    // small text
    text.Set( Skill::Secondary::String( skill.Skill() ), Font::SMALL );
    pos.x = box.GetArea().x + ( pos.width - text.w() ) / 2;
    text.Blit( pos.x, pos.y + 3 );

    text.Set( Skill::Level::String( skill.Level() ) );
    pos.x = box.GetArea().x + ( pos.width - text.w() ) / 2;
    text.Blit( pos.x, pos.y + 55 );

    LocalEvent & le = LocalEvent::Get();

    fheroes2::Button * button = NULL;

    if ( ok_button ) {
        const fheroes2::Point pt( box.GetArea().x + ( box.GetArea().width - fheroes2::AGG::GetICN( system, 1 ).width() ) / 2,
                                  box.GetArea().y + box.GetArea().height - fheroes2::AGG::GetICN( system, 1 ).height() );
        button = new fheroes2::Button( pt.x, pt.y, system, 1, 2 );
    }

    if ( button )
        button->draw();

    display.render();

    // message loop
    while ( le.HandleEvents() ) {
        if ( !ok_button && !le.MousePressRight() )
            break;

        if ( button )
            le.MousePressLeft( button->area() ) ? button->drawOnPress() : button->drawOnRelease();

        if ( button && le.MouseClickLeft( button->area() ) ) {
            break;
        }

        if ( button && le.MousePressRight( skillInfoArea ) ) {
            SecondarySkillInfo( skill, false );
        }

        if ( HotKeyCloseWindow ) {
            break;
        }
    }

    if ( button )
        delete button;
}

void Dialog::PrimarySkillInfo( const std::string & header, const std::string & message, int skill )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const int system = Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM;

    // cursor
    Cursor & cursor = Cursor::Get();

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    int index = 0;
    std::string skill_name;

    switch ( skill ) {
    case Skill::Primary::ATTACK:
        index = 0;
        skill_name = _( "Attack Skill" );
        break;

    case Skill::Primary::DEFENSE:
        index = 1;
        skill_name = _( "Defense Skill" );
        break;

    case Skill::Primary::POWER:
        index = 2;
        skill_name = _( "Spell Power" );
        break;

    case Skill::Primary::KNOWLEDGE:
        index = 3;
        skill_name = _( "Knowledge" );
        break;

    default:
        break;
    }

    TextBox box1( header, Font::YELLOW_BIG, BOXAREA_WIDTH );
    TextBox box2( message, Font::BIG, BOXAREA_WIDTH );
    const fheroes2::Sprite & border = fheroes2::AGG::GetICN( ICN::PRIMSKIL, 4 );
    const int spacer = 10;

    FrameBox box( box1.h() + spacer + box2.h() + spacer + border.height(), true );
    fheroes2::Rect pos = box.GetArea();

    if ( header.size() )
        box1.Blit( pos.x, pos.y );
    pos.y += box1.h() + spacer;

    if ( message.size() )
        box2.Blit( pos.x, pos.y );
    pos.y += box2.h() + spacer;

    // blit sprite
    pos.x = box.GetArea().x + ( pos.width - border.width() ) / 2;
    fheroes2::Blit( border, display, pos.x, pos.y );
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::PRIMSKIL, index );
    pos.x = box.GetArea().x + ( pos.width - sprite.width() ) / 2;
    fheroes2::Blit( sprite, display, pos.x, pos.y + 6 );

    Text text;

    text.Set( skill_name, Font::SMALL );
    pos.x = box.GetArea().x + ( pos.width - text.w() ) / 2;
    text.Blit( pos.x, pos.y + 8 );

    text.Set( "+1", Font::BIG );
    pos.x = box.GetArea().x + ( pos.width - text.w() ) / 2;
    text.Blit( pos.x, pos.y + 80 );

    LocalEvent & le = LocalEvent::Get();

    const fheroes2::Point pt( box.GetArea().x + ( box.GetArea().width - fheroes2::AGG::GetICN( system, 1 ).width() ) / 2,
                              box.GetArea().y + box.GetArea().height - fheroes2::AGG::GetICN( system, 1 ).height() );
    fheroes2::Button button( pt.x, pt.y, system, 1, 2 );

    button.draw();

    cursor.Show();
    display.render();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( button.area() ) ? button.drawOnPress() : button.drawOnRelease();

        if ( le.MouseClickLeft( button.area() ) ) {
            break;
        }

        if ( HotKeyCloseWindow ) {
            break;
        }
    }

    cursor.Hide();
}
