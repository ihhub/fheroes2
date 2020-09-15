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
#include "settings.h"
#include "skill.h"
#include "text.h"
#include "ui_button.h"

void InfoSkillClear( const Rect &, const Rect &, const Rect &, const Rect & );
void InfoSkillSelect( int, const Rect &, const Rect &, const Rect &, const Rect & );
int InfoSkillNext( int );
int InfoSkillPrev( int );

int Dialog::SelectSkillFromArena( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const int system = Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM;
    const bool allSkills = Settings::Get().ExtHeroArenaCanChoiseAnySkills();

    // cursor
    Cursor & cursor = Cursor::Get();
    int oldthemes = cursor.Themes();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    TextBox textbox(
        _( "You enter the arena and face a pack of vicious lions. You handily defeat them, to the wild cheers of the crowd.  Impressed by your skill, the aged trainer of gladiators agrees to train you in a skill of your choice." ),
        Font::BIG, BOXAREA_WIDTH );
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::XPRIMARY, 0 );
    const int spacer = Settings::Get().QVGA() ? 5 : 10;

    Dialog::FrameBox box( textbox.h() + spacer + sprite.height() + 15, true );

    const Rect & box_rt = box.GetArea();
    Point dst_pt = box_rt;

    textbox.Blit( dst_pt );
    dst_pt.y += textbox.h() + spacer;

    int res = Skill::Primary::ATTACK;
    Rect rect1, rect2, rect3, rect4;

    const int spacingX = allSkills ? ( static_cast<int>( box_rt.w ) - sprite.width() * 4 ) / 5 : ( static_cast<int>( box_rt.w ) - sprite.width() * 3 ) / 4;

    rect1 = Rect( dst_pt.x + spacingX, dst_pt.y, sprite.width(), sprite.height() );
    rect2 = Rect( rect1.x + sprite.width() + spacingX, dst_pt.y, sprite.width(), sprite.height() );
    rect3 = Rect( rect2.x + sprite.width() + spacingX, dst_pt.y, sprite.width(), sprite.height() );
    rect4 = Rect( rect3.x + sprite.width() + spacingX, dst_pt.y, sprite.width(), sprite.height() );

    InfoSkillClear( rect1, rect2, rect3, rect4 );
    InfoSkillSelect( res, rect1, rect2, rect3, rect4 );

    // info texts
    TextBox text( Skill::Primary::String( Skill::Primary::ATTACK ), Font::SMALL, 60 );
    dst_pt.x = rect1.x + ( rect1.w - text.w() ) / 2;
    dst_pt.y = rect1.y + rect1.h + 5;
    text.Blit( dst_pt );

    text.Set( Skill::Primary::String( Skill::Primary::DEFENSE ), Font::SMALL, 60 );
    dst_pt.x = rect2.x + ( rect2.w - text.w() ) / 2;
    dst_pt.y = rect2.y + rect2.h + 5;
    text.Blit( dst_pt );

    text.Set( Skill::Primary::String( Skill::Primary::POWER ), Font::SMALL, 60 );
    dst_pt.x = rect3.x + ( rect3.w - text.w() ) / 2;
    dst_pt.y = rect3.y + rect3.h + 5;
    text.Blit( dst_pt );

    if ( allSkills ) {
        text.Set( Skill::Primary::String( Skill::Primary::KNOWLEDGE ), Font::SMALL, 65 );
        dst_pt.x = rect4.x + ( rect4.w - text.w() ) / 2;
        dst_pt.y = rect4.y + rect4.h + 5;
        text.Blit( dst_pt );
    }

    // buttons
    dst_pt.x = box_rt.x + ( box_rt.w - fheroes2::AGG::GetICN( system, 1 ).width() ) / 2;
    dst_pt.y = box_rt.y + box_rt.h - fheroes2::AGG::GetICN( system, 1 ).height();
    fheroes2::Button buttonOk( dst_pt.x, dst_pt.y, system, 1, 2 );

    LocalEvent & le = LocalEvent::Get();
    bool redraw = false;

    buttonOk.draw();
    cursor.Show();
    display.render();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();

        if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_LEFT ) && Skill::Primary::UNKNOWN != InfoSkillPrev( res ) ) {
            res = InfoSkillPrev( res );
            redraw = true;
        }
        else if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_RIGHT ) && Skill::Primary::UNKNOWN != InfoSkillNext( res ) ) {
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
        else if ( allSkills && le.MouseClickLeft( rect4 ) ) {
            res = Skill::Primary::KNOWLEDGE;
            redraw = true;
        }

        if ( redraw ) {
            cursor.Hide();
            InfoSkillClear( rect1, rect2, rect3, rect4 );
            InfoSkillSelect( res, rect1, rect2, rect3, rect4 );
            cursor.Show();
            display.render();
            redraw = false;
        }

        if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) || le.MouseClickLeft( buttonOk.area() ) )
            break;
    }

    cursor.Hide();
    cursor.SetThemes( oldthemes );
    cursor.Show();

    return res;
}

void InfoSkillClear( const Rect & rect1, const Rect & rect2, const Rect & rect3, const Rect & rect4 )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::XPRIMARY, 0 ), display, rect1.x, rect1.y );
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::XPRIMARY, 1 ), display, rect2.x, rect2.y );
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::XPRIMARY, 2 ), display, rect3.x, rect3.y );

    if ( Settings::Get().ExtHeroArenaCanChoiseAnySkills() )
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::XPRIMARY, 3 ), display, rect4.x, rect4.y );
}

void InfoSkillSelect( int skill, const Rect & rect1, const Rect & rect2, const Rect & rect3, const Rect & rect4 )
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
    case Skill::Primary::KNOWLEDGE:
        if ( Settings::Get().ExtHeroArenaCanChoiseAnySkills() )
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::XPRIMARY, 7 ), display, rect4.x, rect4.y );
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
    case Skill::Primary::POWER:
        if ( Settings::Get().ExtHeroArenaCanChoiseAnySkills() )
            return Skill::Primary::KNOWLEDGE;
        break;
    default:
        break;
    }

    return Skill::Primary::UNKNOWN;
}

int InfoSkillPrev( int skill )
{
    switch ( skill ) {
    case Skill::Primary::DEFENSE:
        return Skill::Primary::ATTACK;
    case Skill::Primary::POWER:
        return Skill::Primary::DEFENSE;
    case Skill::Primary::KNOWLEDGE:
        return Skill::Primary::POWER;
    default:
        break;
    }

    return Skill::Primary::UNKNOWN;
}
