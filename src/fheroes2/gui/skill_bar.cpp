/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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
#include "agg.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "text.h"

fheroes2::Image GetBarBackgroundSprite( void )
{
    fheroes2::Image icon( 34, 34 );
    icon.reset();
    fheroes2::DrawBorder( icon, fheroes2::GetColorId( 0xD0, 0xC0, 0x48 ) );
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HSICONS, 0 ), 26, 21, icon, 1, 1, 32, 32 );
    return icon;
}

PrimarySkillsBar::PrimarySkillsBar( const Heroes * hr, bool mini )
    : hero( hr )
    , use_mini_sprite( mini )
    , toff( 0, 0 )
{
    content.push_back( Skill::Primary::ATTACK );
    content.push_back( Skill::Primary::DEFENSE );
    content.push_back( Skill::Primary::POWER );
    content.push_back( Skill::Primary::KNOWLEDGE );

    if ( use_mini_sprite ) {
        backsf = GetBarBackgroundSprite();
        SetItemSize( backsf.width(), backsf.height() );
    }
    else {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::PRIMSKIL, 0 );
        SetItemSize( sprite.width(), sprite.height() );
    }

    SetContent( content );
}

void PrimarySkillsBar::SetTextOff( s32 ox, s32 oy )
{
    toff = fheroes2::Point( ox, oy );
}

void PrimarySkillsBar::RedrawBackground( const Rect & pos, fheroes2::Image & dstsf )
{
    if ( use_mini_sprite )
        fheroes2::Blit( backsf, dstsf, pos.x, pos.y );
}

void PrimarySkillsBar::RedrawItem( int & skill, const Rect & pos, fheroes2::Image & dstsf )
{
    if ( Skill::Primary::UNKNOWN != skill ) {
        if ( use_mini_sprite ) {
            const fheroes2::Sprite & backSprite = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );
            const int ww = 32;
            Text text( "", Font::SMALL );
            const Point dstpt( pos.x + ( pos.w - ww ) / 2, pos.y + ( pos.h - ww ) / 2 );

            switch ( skill ) {
            case Skill::Primary::ATTACK:
                fheroes2::Blit( backSprite, 217, 52, dstsf, dstpt.x, dstpt.y, ww, ww );
                if ( hero )
                    text.Set( GetString( hero->GetAttack() ) );
                break;

            case Skill::Primary::DEFENSE:
                fheroes2::Blit( backSprite, 217, 85, dstsf, dstpt.x, dstpt.y, ww, ww );
                if ( hero )
                    text.Set( GetString( hero->GetDefense() ) );
                break;

            case Skill::Primary::POWER:
                fheroes2::Blit( backSprite, 217, 118, dstsf, dstpt.x, dstpt.y, ww, ww );
                if ( hero )
                    text.Set( GetString( hero->GetPower() ) );
                break;

            case Skill::Primary::KNOWLEDGE:
                fheroes2::Blit( backSprite, 217, 151, dstsf, dstpt.x, dstpt.y, ww, ww );
                if ( hero )
                    text.Set( GetString( hero->GetKnowledge() ) );
                break;

            default:
                break;
            }

            if ( hero )
                text.Blit( pos.x + ( pos.w + toff.x - text.w() ) / 2, pos.y + pos.h + toff.y, dstsf );
        }
        else {
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::PRIMSKIL, skill - 1 );
            fheroes2::Blit( sprite, dstsf, pos.x + ( pos.w - sprite.width() ) / 2, pos.y + ( pos.h - sprite.height() ) / 2 );

            Text text( Skill::Primary::String( skill ), Font::SMALL );
            text.Blit( pos.x + ( pos.w - text.w() ) / 2, pos.y + 4, dstsf );

            if ( hero ) {
                switch ( skill ) {
                case Skill::Primary::ATTACK:
                    text.Set( GetString( hero->GetAttack() ), Font::BIG );
                    break;

                case Skill::Primary::DEFENSE:
                    text.Set( GetString( hero->GetDefense() ), Font::BIG );
                    break;

                case Skill::Primary::POWER:
                    text.Set( GetString( hero->GetPower() ), Font::BIG );
                    break;

                case Skill::Primary::KNOWLEDGE:
                    text.Set( GetString( hero->GetKnowledge() ), Font::BIG );
                    break;

                default:
                    break;
                }

                text.Blit( pos.x + ( pos.w - text.w() ) / 2, pos.y + pos.h - text.h() - 2, dstsf );
            }
        }
    }
}

bool PrimarySkillsBar::ActionBarLeftMouseSingleClick( int & skill )
{
    if ( Skill::Primary::UNKNOWN != skill ) {
        Dialog::Message( Skill::Primary::String( skill ), Skill::Primary::StringDescription( skill, hero ), Font::BIG, Dialog::OK );
        return true;
    }

    return false;
}

bool PrimarySkillsBar::ActionBarRightMouseHold( int & skill )
{
    if ( Skill::Primary::UNKNOWN != skill ) {
        Dialog::Message( Skill::Primary::String( skill ), Skill::Primary::StringDescription( skill, hero ), Font::BIG );
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

SecondarySkillsBar::SecondarySkillsBar( bool mini /* true */, bool change /* false */ )
    : use_mini_sprite( mini )
    , can_change( change )
{
    if ( use_mini_sprite ) {
        backsf = GetBarBackgroundSprite();
        SetItemSize( backsf.width(), backsf.height() );
    }
    else {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::SECSKILL, 0 );
        SetItemSize( sprite.width(), sprite.height() );
    }
}

void SecondarySkillsBar::RedrawBackground( const Rect & pos, fheroes2::Image & dstsf )
{
    if ( use_mini_sprite )
        fheroes2::Blit( backsf, dstsf, pos.x, pos.y );
    else
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::SECSKILL, 0 ), dstsf, pos.x, pos.y );
}

void SecondarySkillsBar::RedrawItem( Skill::Secondary & skill, const Rect & pos, fheroes2::Image & dstsf )
{
    if ( skill.isValid() ) {
        const fheroes2::Sprite & sprite
            = use_mini_sprite ? fheroes2::AGG::GetICN( ICN::MINISS, skill.GetIndexSprite2() ) : fheroes2::AGG::GetICN( ICN::SECSKILL, skill.GetIndexSprite1() );
        fheroes2::Blit( sprite, dstsf, pos.x + ( pos.w - sprite.width() ) / 2, pos.y + ( pos.h - sprite.height() ) / 2 );

        if ( use_mini_sprite ) {
            Text text( GetString( skill.Level() ), Font::SMALL );
            text.Blit( pos.x + ( pos.w - text.w() ) - 3, pos.y + pos.h - 12, dstsf );
        }
        else {
            Text text( Skill::Secondary::String( skill.Skill() ), Font::SMALL );
            text.Blit( pos.x + ( pos.w - text.w() ) / 2, pos.y + 3, dstsf );

            text.Set( Skill::Level::String( skill.Level() ) );
            text.Blit( pos.x + ( pos.w - text.w() ) / 2, pos.y + 51, dstsf );
        }
    }
}

bool SecondarySkillsBar::ActionBarLeftMouseSingleClick( Skill::Secondary & skill )
{
    if ( skill.isValid() ) {
        Dialog::SecondarySkillInfo( skill, true );
        return true;
    }
    else if ( can_change ) {
        Skill::Secondary alt = Dialog::SelectSecondarySkill();

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
        if ( can_change )
            skill.Reset();
        else
            Dialog::SecondarySkillInfo( skill, false );
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
