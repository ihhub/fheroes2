/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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

#include "skill_bar.h"

#include <cassert>
#include <utility>

#include "agg_image.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "heroes.h"
#include "icn.h"
#include "screen.h"
#include "skill.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace
{
    fheroes2::Image GetBarBackgroundSprite()
    {
        fheroes2::Image icon( 34, 34 );
        icon.reset();
        fheroes2::DrawBorder( icon, fheroes2::GetColorId( 0xD0, 0xC0, 0x48 ) );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HSICONS, 0 ), 26, 21, icon, 1, 1, 32, 32 );
        return icon;
    }
}

PrimarySkillsBar::PrimarySkillsBar( const Heroes * hero, bool mini )
    : _hero( hero )
    , useSmallSize( mini )
    , content{ Skill::Primary::ATTACK, Skill::Primary::DEFENSE, Skill::Primary::POWER, Skill::Primary::KNOWLEDGE }
    , toff( 0, 0 )
{
    if ( useSmallSize ) {
        backsf = GetBarBackgroundSprite();
        setSingleItemSize( { backsf.width(), backsf.height() } );
    }
    else {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::PRIMSKIL, 0 );
        setSingleItemSize( { sprite.width(), sprite.height() } );
    }

    SetContent( content );
}

void PrimarySkillsBar::SetTextOff( int32_t ox, int32_t oy )
{
    toff = fheroes2::Point( ox, oy );
}

void PrimarySkillsBar::RedrawBackground( const fheroes2::Rect & pos, fheroes2::Image & dstsf )
{
    if ( useSmallSize )
        fheroes2::Blit( backsf, dstsf, pos.x, pos.y );
}

void PrimarySkillsBar::RedrawItem( int & skill, const fheroes2::Rect & pos, fheroes2::Image & dstsf )
{
    if ( skill == Skill::Primary::UNKNOWN ) {
        // Why is the unknown skill here?
        assert( 0 );
        return;
    }

    if ( useSmallSize ) {
        const fheroes2::Sprite & backSprite = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );
        const int ww = 32;
        const fheroes2::Point dstpt( pos.x + ( pos.width - ww ) / 2, pos.y + ( pos.height - ww ) / 2 );

        std::string skillText;

        switch ( skill ) {
        case Skill::Primary::ATTACK:
            fheroes2::Blit( backSprite, 217, 52, dstsf, dstpt.x, dstpt.y, ww, ww );
            if ( _hero ) {
                skillText = std::to_string( _hero->GetAttack() );
            }
            break;
        case Skill::Primary::DEFENSE:
            fheroes2::Blit( backSprite, 217, 85, dstsf, dstpt.x, dstpt.y, ww, ww );
            if ( _hero ) {
                skillText = std::to_string( _hero->GetDefense() );
            }
            break;
        case Skill::Primary::POWER:
            fheroes2::Blit( backSprite, 217, 118, dstsf, dstpt.x, dstpt.y, ww, ww );
            if ( _hero ) {
                skillText = std::to_string( _hero->GetPower() );
            }
            break;
        case Skill::Primary::KNOWLEDGE:
            fheroes2::Blit( backSprite, 217, 151, dstsf, dstpt.x, dstpt.y, ww, ww );
            if ( _hero ) {
                skillText = std::to_string( _hero->GetKnowledge() );
            }
            break;
        default:
            // Your primary skill is different. Make sure that the logic is correct!
            assert( 0 );
            break;
        }

        if ( _hero != nullptr && !skillText.empty() ) {
            // Render text only if there is something to render.
            const fheroes2::Text text{ std::move( skillText ), fheroes2::FontType::smallWhite() };
            text.draw( pos.x + ( pos.width + toff.x - text.width() ) / 2, pos.y + pos.height + toff.y + 2, dstsf );
        }
    }
    else {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::PRIMSKIL, skill - 1 );
        fheroes2::Blit( sprite, dstsf, pos.x + ( pos.width - sprite.width() ) / 2, pos.y + ( pos.height - sprite.height() ) / 2 );

        fheroes2::Text text{ Skill::Primary::String( skill ), fheroes2::FontType::smallWhite() };
        text.draw( pos.x + ( pos.width - text.width() ) / 2, pos.y + 6, dstsf );

        if ( _hero == nullptr ) {
            // Nothing more to render.
            return;
        }

        switch ( skill ) {
        case Skill::Primary::ATTACK:
            text.set( std::to_string( _hero->GetAttack() ), fheroes2::FontType::normalWhite() );
            break;
        case Skill::Primary::DEFENSE:
            text.set( std::to_string( _hero->GetDefense() ), fheroes2::FontType::normalWhite() );
            break;
        case Skill::Primary::POWER:
            text.set( std::to_string( _hero->GetPower() ), fheroes2::FontType::normalWhite() );
            break;
        case Skill::Primary::KNOWLEDGE:
            text.set( std::to_string( _hero->GetKnowledge() ), fheroes2::FontType::normalWhite() );
            break;
        default:
            // Your primary skill is different. Make sure that the logic is correct!
            assert( 0 );
            return;
        }

        text.draw( pos.x + ( pos.width - text.width() ) / 2, pos.y + pos.height - text.height(), dstsf );
    }
}

bool PrimarySkillsBar::ActionBarLeftMouseSingleClick( int & skill )
{
    if ( Skill::Primary::UNKNOWN != skill ) {
        fheroes2::showStandardTextMessage( Skill::Primary::String( skill ), Skill::Primary::StringDescription( skill, _hero ), Dialog::OK );
        return true;
    }

    return false;
}

bool PrimarySkillsBar::ActionBarRightMouseHold( int & skill )
{
    if ( Skill::Primary::UNKNOWN != skill ) {
        fheroes2::showStandardTextMessage( Skill::Primary::String( skill ), Skill::Primary::StringDescription( skill, _hero ), Dialog::ZERO );
        return true;
    }

    return false;
}

bool PrimarySkillsBar::ActionBarCursor( int & skill )
{
    if ( Skill::Primary::UNKNOWN != skill ) {
        msg = _( "View %{skill} Info" );
        StringReplace( msg, "%{skill}", Skill::Primary::String( skill ) );
    }

    return false;
}

bool PrimarySkillsBar::QueueEventProcessing( std::string * str )
{
    msg.clear();
    bool res = Interface::ItemsBar<int>::QueueEventProcessing();
    if ( str )
        *str = msg;
    return res;
}

SecondarySkillsBar::SecondarySkillsBar( const Heroes & hero, bool mini /* true */, bool change /* false */ )
    : use_mini_sprite( mini )
    , can_change( change )
    , _hero( hero )
{
    if ( use_mini_sprite ) {
        backsf = GetBarBackgroundSprite();
        setSingleItemSize( { backsf.width(), backsf.height() } );
    }
    else {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::SECSKILL, 0 );
        setSingleItemSize( { sprite.width(), sprite.height() } );
    }
}

void SecondarySkillsBar::RedrawBackground( const fheroes2::Rect & pos, fheroes2::Image & dstsf )
{
    if ( use_mini_sprite )
        fheroes2::Blit( backsf, dstsf, pos.x, pos.y );
    else
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::SECSKILL, 0 ), dstsf, pos.x, pos.y );
}

void SecondarySkillsBar::RedrawItem( Skill::Secondary & skill, const fheroes2::Rect & pos, fheroes2::Image & dstsf )
{
    if ( !skill.isValid() ) {
        // If the skill is invalid nothing we need to do.
        return;
    }

    const fheroes2::Sprite & sprite
        = use_mini_sprite ? fheroes2::AGG::GetICN( ICN::MINISS, skill.GetIndexSprite2() ) : fheroes2::AGG::GetICN( ICN::SECSKILL, skill.GetIndexSprite1() );
    fheroes2::Blit( sprite, dstsf, pos.x + ( pos.width - sprite.width() ) / 2, pos.y + ( pos.height - sprite.height() ) / 2 );

    if ( use_mini_sprite ) {
        const fheroes2::Text text{ std::to_string( skill.Level() ), fheroes2::FontType::smallWhite() };
        text.draw( pos.x + ( pos.width - text.width() ) - 3, pos.y + pos.height - 10, dstsf );
    }
    else {
        fheroes2::Text text{ Skill::Secondary::String( skill.Skill() ), fheroes2::FontType::smallWhite() };
        text.draw( pos.x + ( pos.width - text.width() ) / 2, pos.y + 5, dstsf );

        text.set( Skill::Level::StringWithBonus( _hero, skill ), fheroes2::FontType::smallWhite() );
        text.draw( pos.x + ( pos.width - text.width() ) / 2, pos.y + 53, dstsf );
    }
}

bool SecondarySkillsBar::ActionBarLeftMouseSingleClick( Skill::Secondary & skill )
{
    if ( skill.isValid() ) {
        fheroes2::SecondarySkillDialogElement( skill, _hero ).showPopup( Dialog::OK );
        return true;
    }
    else if ( can_change ) {
        Skill::Secondary alt = Dialog::selectSecondarySkill( _hero );

        if ( alt.isValid() ) {
            skill = alt;
            return true;
        }
    }

    return false;
}

bool SecondarySkillsBar::ActionBarRightMouseHold( Skill::Secondary & skill )
{
    if ( skill.isValid() ) {
        if ( can_change ) {
            skill.Reset();
        }
        else {
            fheroes2::SecondarySkillDialogElement( skill, _hero ).showPopup( Dialog::ZERO );
        }
        return true;
    }

    return false;
}

bool SecondarySkillsBar::ActionBarCursor( Skill::Secondary & skill )
{
    if ( skill.isValid() ) {
        msg = _( "View %{skill} Info" );
        StringReplace( msg, "%{skill}", skill.GetName() );
    }

    return false;
}

bool SecondarySkillsBar::QueueEventProcessing( std::string * str )
{
    msg.clear();
    bool res = Interface::ItemsBar<Skill::Secondary>::QueueEventProcessing();
    if ( str )
        *str = msg;
    return res;
}

namespace fheroes2
{
    void RedrawPrimarySkillInfo( const fheroes2::Point & pos, PrimarySkillsBar * bar1, PrimarySkillsBar * bar2 )
    {
        Display & display = Display::instance();

        // attack skill
        Text text( Skill::Primary::String( Skill::Primary::ATTACK ), FontType::smallWhite() );
        text.draw( pos.x + 320 - text.width() / 2, pos.y + 62, display );

        // defense skill
        text.set( Skill::Primary::String( Skill::Primary::DEFENSE ), FontType::smallWhite() );
        text.draw( pos.x + 320 - text.width() / 2, pos.y + 95, display );

        // spell power
        text.set( Skill::Primary::String( Skill::Primary::POWER ), FontType::smallWhite() );
        text.draw( pos.x + 320 - text.width() / 2, pos.y + 128, display );

        // knowledge
        text.set( Skill::Primary::String( Skill::Primary::KNOWLEDGE ), FontType::smallWhite() );
        text.draw( pos.x + 320 - text.width() / 2, pos.y + 161, display );

        if ( bar1 ) {
            bar1->Redraw( display );
        }
        if ( bar2 ) {
            bar2->Redraw( display );
        }
    }
}
