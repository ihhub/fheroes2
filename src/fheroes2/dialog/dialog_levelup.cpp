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

#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h" // IWYU pragma: associated
#include "game_hotkeys.h"
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
#include "ui_language.h"
#include "ui_text.h"

namespace
{
    void DialogPrimaryOnly( const std::string & name, const int primarySkillType )
    {
        std::string message = _( "%{name} has gained a level." );
        message.append( "\n\n" );
        message.append( _( "%{skill} +1" ) );
        StringReplace( message, "%{name}", name );
        StringReplace( message, "%{skill}", Skill::Primary::String( primarySkillType ) );

        const fheroes2::PrimarySkillDialogElement primarySkillUI( primarySkillType, "+1" );

        fheroes2::showStandardTextMessage( {}, std::move( message ), Dialog::OK, { &primarySkillUI } );
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

        fheroes2::showStandardTextMessage( {}, std::move( message ), Dialog::OK, { &secondarySkillUI } );

        return sec.Skill();
    }

    int DialogSelectSecondary( const std::string & name, const int primarySkillType, const Skill::Secondary & sec1, const Skill::Secondary & sec2, Heroes & hero )
    {
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        std::string header = _( "%{name} has gained a level.\n\n%{skill} +1" );
        StringReplace( header, "%{name}", name );
        StringReplace( header, "%{skill}", Skill::Primary::String( primarySkillType ) );

        std::string body = _( "You may learn either:\n%{skill1}\nor\n%{skill2}" );
        StringReplace( body, "%{skill1}", sec1.GetName() );
        StringReplace( body, "%{skill2}", sec2.GetName() );

        const fheroes2::Text headerText{ std::move( header ), fheroes2::FontType::normalWhite() };
        const fheroes2::Text bodyText{ std::move( body ), fheroes2::FontType::normalWhite() };
        const int spacer = 10;

        const fheroes2::SecondarySkillDialogElement skillLeft{ sec1, hero };
        const fheroes2::SecondarySkillDialogElement skillRight{ sec2, hero };

        const Dialog::FrameBox dialogFrame( headerText.height( fheroes2::boxAreaWidthPx ) + spacer + bodyText.height( fheroes2::boxAreaWidthPx ) + 10
                                                + skillLeft.area().height,
                                            true );

        const Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();
        const int buttonLearnIcnID = isEvilInterface ? ICN::BUTTON_SMALL_LEARN_EVIL : ICN::BUTTON_SMALL_LEARN_GOOD;

        const fheroes2::Rect & dialogRoi = dialogFrame.GetArea();
        const fheroes2::Sprite & buttonLearnImage = fheroes2::AGG::GetICN( buttonLearnIcnID, 0 );

        fheroes2::Point offset;
        offset.x = dialogRoi.x + dialogRoi.width / 2 - buttonLearnImage.width() - 20;
        offset.y = dialogRoi.y + dialogRoi.height - buttonLearnImage.height();
        fheroes2::Button buttonLearnLeft( offset.x, offset.y, buttonLearnIcnID, 0, 1 );

        offset.x = dialogRoi.x + dialogRoi.width / 2 + 20;
        offset.y = dialogRoi.y + dialogRoi.height - buttonLearnImage.height();
        fheroes2::Button buttonLearnRight( offset.x, offset.y, buttonLearnIcnID, 0, 1 );

        offset = { dialogRoi.x, dialogRoi.y };

        fheroes2::Display & display = fheroes2::Display::instance();
        headerText.draw( offset.x, offset.y + 2, fheroes2::boxAreaWidthPx, display );
        offset.y += headerText.height( fheroes2::boxAreaWidthPx ) + spacer;

        bodyText.draw( offset.x, offset.y + 2, fheroes2::boxAreaWidthPx, display );
        offset.y += bodyText.height( fheroes2::boxAreaWidthPx ) + spacer;

        offset.x = dialogRoi.x + dialogRoi.width / 2 - skillLeft.area().width - 20;
        offset.x += 3;
        skillLeft.draw( display, offset );
        const fheroes2::Rect skillLeftRoi{ offset.x, offset.y, skillLeft.area().width, skillLeft.area().height };

        offset.x = dialogRoi.x + dialogRoi.width / 2 + 20;
        offset.x += 3;
        skillRight.draw( display, offset );
        const fheroes2::Rect skillRightRoi{ offset.x, offset.y, skillRight.area().width, skillRight.area().height };

        // hero button
        offset.x = dialogRoi.x + dialogRoi.width / 2 - 18;
        offset.y = dialogRoi.y + dialogRoi.height - 35;

        const int icnHeroes = isEvilInterface ? ICN::EVIL_ARMY_BUTTON : ICN::GOOD_ARMY_BUTTON;
        fheroes2::ButtonSprite buttonHero
            = fheroes2::makeButtonWithBackground( offset.x, offset.y, fheroes2::AGG::GetICN( icnHeroes, 0 ), fheroes2::AGG::GetICN( icnHeroes, 1 ), display );

        const fheroes2::Text text{ std::to_string( hero.GetSecondarySkills().Count() ) + "/" + std::to_string( Heroes::maxNumOfSecSkills ),
                                   fheroes2::FontType::normalWhite() };
        text.draw( dialogRoi.x + ( dialogRoi.width - text.width() ) / 2, offset.y - 15, display );

        buttonLearnLeft.draw();
        buttonLearnRight.draw();
        buttonHero.draw();

        display.render();

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            buttonLearnLeft.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonLearnLeft.area() ) );
            buttonLearnRight.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonLearnRight.area() ) );
            buttonHero.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonHero.area() ) );

            if ( le.MouseClickLeft( buttonLearnLeft.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_LEFT ) ) {
                return sec1.Skill();
            }

            if ( le.MouseClickLeft( buttonLearnRight.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_RIGHT ) ) {
                return sec2.Skill();
            }

            if ( le.MouseClickLeft( buttonHero.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
                le.reset();
                hero.OpenDialog( false, true, true, true, true, false, fheroes2::getLanguageFromAbbreviation( conf.getGameLanguage() ) );
                display.render();
            }

            if ( le.MouseClickLeft( skillLeftRoi ) ) {
                skillLeft.showPopup( Dialog::OK );
            }
            else if ( le.MouseClickLeft( skillRightRoi ) ) {
                skillRight.showPopup( Dialog::OK );
            }

            if ( le.isMouseRightButtonPressedInArea( skillLeftRoi ) ) {
                skillLeft.showPopup( Dialog::ZERO );
                display.render();
            }
            else if ( le.isMouseRightButtonPressedInArea( skillRightRoi ) ) {
                skillRight.showPopup( Dialog::ZERO );
                display.render();
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonHero.area() ) ) {
                fheroes2::showStandardTextMessage( "", _( "View Hero" ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonLearnLeft.area() ) ) {
                std::string message = _( "Learn %{secondary-skill}" );
                StringReplace( message, "%{secondary-skill}", sec1.GetName() );

                fheroes2::showStandardTextMessage( "", std::move( message ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonLearnRight.area() ) ) {
                std::string message = _( "Learn %{secondary-skill}" );
                StringReplace( message, "%{secondary-skill}", sec2.GetName() );

                fheroes2::showStandardTextMessage( "", std::move( message ), Dialog::ZERO );
            }
        }

        return Skill::Secondary::UNKNOWN;
    }
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
