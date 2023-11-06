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

#include <string>
#include <utility>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "gamedefs.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

void DialogPrimaryOnly( const std::string & name, const int primarySkillType )
{
    std::string message = _( "%{name} has gained a level." );
    message.append( "\n\n" );
    message.append( _( "%{skill} +1" ) );
    StringReplace( message, "%{name}", name );
    StringReplace( message, "%{skill}", Skill::Primary::String( primarySkillType ) );

    const fheroes2::PrimarySkillDialogElement primarySkillUI( primarySkillType, "+1" );

    fheroes2::showMessage( fheroes2::Text( "", {} ), fheroes2::Text( message, fheroes2::FontType::normalWhite() ), Dialog::OK, { &primarySkillUI } );
}

int DialogOneSecondary( const Heroes & hero, const std::string & name, const int primarySkillType, const Skill::Secondary & sec )
{
    std::string message = _( "%{name} has gained a level." );
    message.append( "\n\n" );
    message.append( _( "%{skill} +1" ) );
    StringReplace( message, "%{name}", name );
    StringReplace( message, "%{skill}", Skill::Primary::String( primarySkillType ) );

    message.append( "\n\n" );
    message.append( _( "You have learned %{skill}." ) );
    StringReplace( message, "%{skill}", sec.GetName() );

    const fheroes2::SecondarySkillDialogElement secondarySkillUI( sec, hero );

    fheroes2::showMessage( fheroes2::Text( "", {} ), fheroes2::Text( message, fheroes2::FontType::normalWhite() ), Dialog::OK, { &secondarySkillUI } );

    return sec.Skill();
}

int DialogSelectSecondary( const std::string & name, const int primarySkillType, const Skill::Secondary & sec1, const Skill::Secondary & sec2, Heroes & hero )
{
    std::string header = _( "%{name} has gained a level.\n\n%{skill} +1" );
    StringReplace( header, "%{name}", name );
    StringReplace( header, "%{skill}", Skill::Primary::String( primarySkillType ) );

    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::Sprite & sprite_frame = fheroes2::AGG::GetICN( ICN::SECSKILL, 15 );
    const fheroes2::Sprite & sprite_skill1 = fheroes2::AGG::GetICN( ICN::SECSKILL, sec1.GetIndexSprite1() );
    const fheroes2::Sprite & sprite_skill2 = fheroes2::AGG::GetICN( ICN::SECSKILL, sec2.GetIndexSprite1() );

    std::string message = _( "You may learn either:\n%{skill1}\nor\n%{skill2}" );
    StringReplace( message, "%{skill1}", sec1.GetName() );
    StringReplace( message, "%{skill2}", sec2.GetName() );

    fheroes2::Text box1( std::move( header ), fheroes2::FontType::normalWhite() );
    fheroes2::Text box2( std::move( message ), fheroes2::FontType::normalWhite() );
    const int spacer = 10;

    Dialog::FrameBox box( box1.height( BOXAREA_WIDTH ) + spacer + box2.height( BOXAREA_WIDTH ) + 10 + sprite_frame.height(), true );

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const int buttonLearnIcnID = isEvilInterface ? ICN::BUTTON_SMALL_LEARN_EVIL : ICN::BUTTON_SMALL_LEARN_GOOD;

    fheroes2::Point pt;
    pt.x = box.GetArea().x + box.GetArea().width / 2 - fheroes2::AGG::GetICN( buttonLearnIcnID, 0 ).width() - 20;
    pt.y = box.GetArea().y + box.GetArea().height - fheroes2::AGG::GetICN( buttonLearnIcnID, 0 ).height();
    fheroes2::Button button_learn1( pt.x, pt.y, buttonLearnIcnID, 0, 1 );

    pt.x = box.GetArea().x + box.GetArea().width / 2 + 20;
    pt.y = box.GetArea().y + box.GetArea().height - fheroes2::AGG::GetICN( buttonLearnIcnID, 0 ).height();
    fheroes2::Button button_learn2( pt.x, pt.y, buttonLearnIcnID, 0, 1 );

    const fheroes2::Rect & boxArea = box.GetArea();
    fheroes2::Point pos( boxArea.x, boxArea.y );

    box1.draw( pos.x, pos.y + 2, BOXAREA_WIDTH, display );
    pos.y += box1.height( BOXAREA_WIDTH ) + spacer;

    box2.draw( pos.x, pos.y + 2, BOXAREA_WIDTH, display );
    pos.y += box2.height( BOXAREA_WIDTH ) + spacer;

    // sprite1
    pos.x = box.GetArea().x + box.GetArea().width / 2 - sprite_frame.width() - 20;
    fheroes2::Blit( sprite_frame, display, pos.x, pos.y );
    pos.x += 3;
    fheroes2::Rect rect_image1( pos.x, pos.y, sprite_skill1.width(), sprite_skill1.height() );
    fheroes2::Blit( sprite_skill1, display, pos.x, pos.y + 3 );

    fheroes2::Text text{ Skill::Secondary::String( sec1.Skill() ), fheroes2::FontType::smallWhite() };
    text.draw( pos.x + ( sprite_skill1.width() - text.width() ) / 2, pos.y + 7, display );
    text.set( Skill::Level::String( sec1.Level() ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + ( sprite_skill1.width() - text.width() ) / 2, pos.y + sprite_skill1.height() - 10, display );

    // sprite2
    pos.x = box.GetArea().x + box.GetArea().width / 2 + 20;
    fheroes2::Blit( sprite_frame, display, pos.x, pos.y );
    pos.x += 3;

    fheroes2::Rect rect_image2( pos.x, pos.y, sprite_skill2.width(), sprite_skill2.height() );
    fheroes2::Blit( sprite_skill2, display, pos.x, pos.y + 3 );
    // text
    fheroes2::Text name_skill2( Skill::Secondary::String( sec2.Skill() ), fheroes2::FontType::smallWhite() );
    name_skill2.draw( pos.x + ( sprite_skill2.width() - name_skill2.width() ) / 2, pos.y + 7, display );
    fheroes2::Text name_level2( Skill::Level::String( sec2.Level() ), fheroes2::FontType::smallWhite() );
    name_level2.draw( pos.x + ( sprite_skill2.width() - name_level2.width() ) / 2, pos.y + sprite_skill2.height() - 10, display );

    // hero button
    pt.x = box.GetArea().x + box.GetArea().width / 2 - 18;
    pt.y = box.GetArea().y + box.GetArea().height - 35;

    const int icnHeroes = isEvilInterface ? ICN::EVIL_ARMY_BUTTON : ICN::GOOD_ARMY_BUTTON;
    fheroes2::ButtonSprite button_hero
        = fheroes2::makeButtonWithBackground( pt.x, pt.y, fheroes2::AGG::GetICN( icnHeroes, 0 ), fheroes2::AGG::GetICN( icnHeroes, 1 ), display );

    text.set( std::to_string( hero.GetSecondarySkills().Count() ) + "/" + std::to_string( HEROESMAXSKILL ), fheroes2::FontType::normalWhite() );
    text.draw( box.GetArea().x + ( box.GetArea().width - text.width() ) / 2, pt.y - 13, display );

    button_learn1.draw();
    button_learn2.draw();
    button_hero.draw();

    display.render();
    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( button_learn1.area() ) ? button_learn1.drawOnPress() : button_learn1.drawOnRelease();
        le.MousePressLeft( button_learn2.area() ) ? button_learn2.drawOnPress() : button_learn2.drawOnRelease();
        le.MousePressLeft( button_hero.area() ) ? button_hero.drawOnPress() : button_hero.drawOnRelease();

        if ( le.MouseClickLeft( button_learn1.area() ) ) {
            return sec1.Skill();
        }

        if ( le.MouseClickLeft( button_learn2.area() ) ) {
            return sec2.Skill();
        }

        if ( le.MouseClickLeft( button_hero.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
            LocalEvent::GetClean();
            hero.OpenDialog( false, true, true, true, true );
            display.render();
        }

        if ( le.MouseClickLeft( rect_image1 ) ) {
            fheroes2::SecondarySkillDialogElement( sec1, hero ).showPopup( Dialog::OK );
        }
        else if ( le.MouseClickLeft( rect_image2 ) ) {
            fheroes2::SecondarySkillDialogElement( sec2, hero ).showPopup( Dialog::OK );
        }

        if ( le.MousePressRight( rect_image1 ) ) {
            fheroes2::SecondarySkillDialogElement( sec1, hero ).showPopup( Dialog::ZERO );
            display.render();
        }
        else if ( le.MousePressRight( rect_image2 ) ) {
            fheroes2::SecondarySkillDialogElement( sec2, hero ).showPopup( Dialog::ZERO );
            display.render();
        }
        else if ( le.MousePressRight( button_hero.area() ) ) {
            fheroes2::showStandardTextMessage( "", _( "View Hero" ), Dialog::ZERO );
        }
    }

    return Skill::Secondary::UNKNOWN;
}

int Dialog::LevelUpSelectSkill( const std::string & name, const int primarySkillType, const Skill::Secondary & sec1, const Skill::Secondary & sec2, Heroes & hero )
{
    if ( Skill::Secondary::UNKNOWN == sec1.Skill() && Skill::Secondary::UNKNOWN == sec2.Skill() ) {
        DialogPrimaryOnly( name, primarySkillType );
        return Skill::Secondary::UNKNOWN;
    }

    if ( Skill::Secondary::UNKNOWN == sec1.Skill() || Skill::Secondary::UNKNOWN == sec2.Skill() ) {
        return DialogOneSecondary( hero, name, primarySkillType, ( Skill::Secondary::UNKNOWN == sec2.Skill() ? sec1 : sec2 ) );
    }

    return DialogSelectSecondary( name, primarySkillType, sec1, sec2, hero );
}
