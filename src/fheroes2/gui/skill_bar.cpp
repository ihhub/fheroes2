/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#include <algorithm>
#include <cassert>
#include <utility>

#include "agg_image.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "heroes.h"
#include "icn.h"
#include "pal.h"
#include "screen.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace
{
    fheroes2::Image GetBarBackgroundSprite()
    {
        fheroes2::Image icon( 34, 34 );
        // Sprite ( ICN::HSICONS, 0 ) has no transparency so we can say that 'icon' will also have no transparency.
        icon._disableTransformLayer();
        icon.reset();
        fheroes2::DrawBorder( icon, fheroes2::GetColorId( 0xD0, 0xC0, 0x48 ) );
        fheroes2::Copy( fheroes2::AGG::GetICN( ICN::HSICONS, 0 ), 26, 21, icon, 1, 1, 32, 32 );
        return icon;
    }
}

PrimarySkillsBar::PrimarySkillsBar( Heroes * hero, const bool useSmallSize, const bool isEditMode, const bool allowSkillReset )
    : _hero( hero )
    , _useSmallSize( useSmallSize )
    , _isEditMode( isEditMode )
    , _allowSkillReset( allowSkillReset )
{
    if ( useSmallSize ) {
        backsf = GetBarBackgroundSprite();
        setSingleItemSize( { backsf.width(), backsf.height() } );
    }
    else {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::PRIMSKIL, 0 );
        setSingleItemSize( { sprite.width(), sprite.height() } );
    }

    SetContent( _content );
}

void PrimarySkillsBar::SetTextOff( int32_t ox, int32_t oy )
{
    toff = fheroes2::Point( ox, oy );
}

void PrimarySkillsBar::RedrawBackground( const fheroes2::Rect & pos, fheroes2::Image & dstsf )
{
    if ( _useSmallSize ) {
        fheroes2::Copy( backsf, 0, 0, dstsf, pos );
    }
}

void PrimarySkillsBar::RedrawItem( int & skill, const fheroes2::Rect & pos, fheroes2::Image & dstsf )
{
    if ( skill == Skill::Primary::UNKNOWN ) {
        // Why is the unknown skill here?
        assert( 0 );
        return;
    }

    if ( _useSmallSize ) {
        const fheroes2::Sprite & backSprite = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );
        const int ww = 32;
        const fheroes2::Point dstpt( pos.x + ( pos.width - ww ) / 2, pos.y + ( pos.height - ww ) / 2 );

        std::string skillText;

        switch ( skill ) {
        case Skill::Primary::ATTACK:
            fheroes2::Copy( backSprite, 217, 52, dstsf, dstpt.x, dstpt.y, ww, ww );
            if ( _hero ) {
                skillText = std::to_string( _hero->GetAttack() );
            }
            break;
        case Skill::Primary::DEFENSE:
            fheroes2::Copy( backSprite, 217, 85, dstsf, dstpt.x, dstpt.y, ww, ww );
            if ( _hero ) {
                skillText = std::to_string( _hero->GetDefense() );
            }
            break;
        case Skill::Primary::POWER:
            fheroes2::Copy( backSprite, 217, 118, dstsf, dstpt.x, dstpt.y, ww, ww );
            if ( _hero ) {
                skillText = std::to_string( _hero->GetPower() );
            }
            break;
        case Skill::Primary::KNOWLEDGE:
            fheroes2::Copy( backSprite, 217, 151, dstsf, dstpt.x, dstpt.y, ww, ww );
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
        fheroes2::Copy( sprite, 0, 0, dstsf, pos.x + ( pos.width - sprite.width() ) / 2, pos.y + ( pos.height - sprite.height() ) / 2, pos.width, pos.height );

        fheroes2::Text text{ Skill::Primary::String( skill ), fheroes2::FontType::smallWhite() };

        if ( _hero == nullptr ) {
            text.drawInRoi( pos.x + ( pos.width - text.width() ) / 2, pos.y + 6, dstsf, pos );
            return;
        }

        std::string skillValueText;

        auto prepareEditModeText = [this, &dstsf, &pos, &skill]( int baseValue, const int modificatorValue ) {
            if ( _isDefault ) {
                // In editor the negative value means that this skill does not use custom value.
                baseValue = Heroes::getHeroDefaultSkillValue( skill, _hero->GetRace() );

                // Make the background darker to make text more readable.
                fheroes2::ApplyPalette( dstsf, pos.x, pos.y, dstsf, pos.x, pos.y, pos.width, pos.height, PAL::GetPalette( PAL::PaletteType::DARKENING ) );

                const fheroes2::Text defaultText( _( "Default\nvalue" ), fheroes2::FontType::normalWhite() );
                defaultText.drawInRoi( pos.x, pos.y + ( pos.height - defaultText.height() * defaultText.rows( pos.width ) ) / 2 + 2, pos.width, dstsf, pos );
            }

            std::string skillText = "%{withModificators} (%{base})";
            StringReplace( skillText, "%{base}", baseValue );
            StringReplace( skillText, "%{withModificators}", baseValue + modificatorValue );

            return skillText;
        };

        switch ( skill ) {
        case Skill::Primary::ATTACK:
            skillValueText = _isEditMode ? prepareEditModeText( _hero->getAttackBaseValue(), _hero->GetAttackModificator() ) : std::to_string( _hero->GetAttack() );
            break;
        case Skill::Primary::DEFENSE:
            skillValueText = _isEditMode ? prepareEditModeText( _hero->getDefenseBaseValue(), _hero->GetDefenseModificator() ) : std::to_string( _hero->GetDefense() );
            break;
        case Skill::Primary::POWER:
            skillValueText = _isEditMode ? prepareEditModeText( _hero->getPowerBaseValue(), _hero->GetPowerModificator() ) : std::to_string( _hero->GetPower() );
            break;
        case Skill::Primary::KNOWLEDGE:
            skillValueText
                = _isEditMode ? prepareEditModeText( _hero->getKnowledgeBaseValue(), _hero->GetKnowledgeModificator() ) : std::to_string( _hero->GetKnowledge() );
            break;
        default:
            // Your primary skill is different. Make sure that the logic is correct!
            assert( 0 );
            return;
        }

        // In editor the background may be darkened so we render texts here.
        text.drawInRoi( pos.x + ( pos.width - text.width() ) / 2, pos.y + 6, dstsf, pos );
        text.set( std::move( skillValueText ), fheroes2::FontType::normalWhite() );
        text.drawInRoi( pos.x, pos.y + pos.height - text.height(), pos.width, dstsf, pos );
    }
}

bool PrimarySkillsBar::ActionBarLeftMouseSingleClick( int & skill )
{
    if ( skill == Skill::Primary::UNKNOWN ) {
        return false;
    }

    if ( !_isEditMode ) {
        fheroes2::showStandardTextMessage( Skill::Primary::String( skill ), Skill::Primary::StringDescription( skill, _hero ), Dialog::OK );

        return false;
    }

    // The case when we are in Editor mode.
    auto primarySkillEditDialog = [&skill]( int32_t & skillValue ) {
        std::string header = _( "Set %{skill} Skill" );
        StringReplace( header, "%{skill}", Skill::Primary::String( skill ) );

        return Dialog::SelectCount( header, skill == Skill::Primary::POWER ? 1 : 0, 99, skillValue );
    };

    int32_t value;

    switch ( skill ) {
    case Skill::Primary::ATTACK:
        value = _hero->getAttackBaseValue();
        if ( primarySkillEditDialog( value ) ) {
            _hero->setAttackBaseValue( value );
            _isDefault = false;
            return true;
        }
        break;
    case Skill::Primary::DEFENSE:
        value = _hero->getDefenseBaseValue();
        if ( primarySkillEditDialog( value ) ) {
            _hero->setDefenseBaseValue( value );
            _isDefault = false;
            return true;
        }
        break;
    case Skill::Primary::POWER:
        value = _hero->getPowerBaseValue();
        if ( primarySkillEditDialog( value ) ) {
            _hero->setPowerBaseValue( value );
            _isDefault = false;
            return true;
        }
        break;
    case Skill::Primary::KNOWLEDGE:
        value = _hero->getKnowledgeBaseValue();
        if ( primarySkillEditDialog( value ) ) {
            _hero->setKnowledgeBaseValue( value );
            _isDefault = false;
            return true;
        }
        break;
    default:
        // Are you sure that you are passing the correct skill type?
        assert( 0 );
        break;
    }

    return false;
}

bool PrimarySkillsBar::ActionBarRightMouseHold( int & skill )
{
    if ( skill == Skill::Primary::UNKNOWN ) {
        return false;
    }

    if ( !_isEditMode || !_allowSkillReset || _isDefault ) {
        fheroes2::showStandardTextMessage( Skill::Primary::String( skill ), Skill::Primary::StringDescription( skill, _hero ), Dialog::ZERO );

        return false;
    }

    // In Editor the Right-click on any primary skill resets all skills to their default state.
    _isDefault = true;

    const int heroRace = _hero->GetRace();
    _hero->setAttackBaseValue( Heroes::getHeroDefaultSkillValue( Skill::Primary::ATTACK, heroRace ) );
    _hero->setDefenseBaseValue( Heroes::getHeroDefaultSkillValue( Skill::Primary::DEFENSE, heroRace ) );
    _hero->setPowerBaseValue( Heroes::getHeroDefaultSkillValue( Skill::Primary::POWER, heroRace ) );
    _hero->setKnowledgeBaseValue( Heroes::getHeroDefaultSkillValue( Skill::Primary::KNOWLEDGE, heroRace ) );

    return true;
}

bool PrimarySkillsBar::ActionBarCursor( int & skill )
{
    if ( Skill::Primary::UNKNOWN != skill ) {
        msg = _isEditMode ? _( "Set %{skill} base value. Right-click to reset all skills to default." ) : _( "View %{skill} Info" );
        StringReplace( msg, "%{skill}", Skill::Primary::String( skill ) );
    }

    return false;
}

bool PrimarySkillsBar::QueueEventProcessing( std::string * str )
{
    msg.clear();
    const bool res = Interface::ItemsBar<int>::QueueEventProcessing();
    if ( str ) {
        *str = msg;
    }
    return res;
}

SecondarySkillsBar::SecondarySkillsBar( const Heroes & hero, const bool mini /* true */, const bool change /* false */, const bool showDefaultSkillsMessage /* false */ )
    : use_mini_sprite( mini )
    , can_change( change )
    , _showDefaultSkillsMessage( showDefaultSkillsMessage )
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
    if ( use_mini_sprite ) {
        fheroes2::Copy( backsf, 0, 0, dstsf, pos );
    }
    else {
        if ( _showDefaultSkillsMessage && std::none_of( items.begin(), items.end(), []( Skill::Secondary const * skill ) { return skill->isValid(); } ) ) {
            // In editor when no skill are set it means that default skills will be applied on the game start.
            fheroes2::ApplyPalette( fheroes2::AGG::GetICN( ICN::SECSKILL, 0 ), 0, 0, dstsf, pos.x, pos.y, pos.width, pos.height,
                                    PAL::GetPalette( PAL::PaletteType::DARKENING ) );

            const fheroes2::Text text( _( "Default\nskill" ), fheroes2::FontType::normalWhite() );
            text.drawInRoi( pos.x, pos.y + ( pos.height - text.height() * text.rows( pos.width ) ) / 2 + 2, pos.width, dstsf, pos );
        }
        else {
            fheroes2::Copy( fheroes2::AGG::GetICN( ICN::SECSKILL, 0 ), 0, 0, dstsf, pos );
        }
    }
}

void SecondarySkillsBar::RedrawItem( Skill::Secondary & skill, const fheroes2::Rect & pos, fheroes2::Image & dstsf )
{
    if ( !skill.isValid() ) {
        // If the skill is invalid nothing we need to do.
        return;
    }

    const fheroes2::Sprite & sprite
        = use_mini_sprite ? fheroes2::AGG::GetICN( ICN::MINISS, skill.GetIndexSprite2() ) : fheroes2::AGG::GetICN( ICN::SECSKILL, skill.GetIndexSprite1() );
    fheroes2::Copy( sprite, 0, 0, dstsf, pos.x + ( pos.width - sprite.width() ) / 2, pos.y + ( pos.height - sprite.height() ) / 2, pos.width, pos.height );

    if ( use_mini_sprite ) {
        const fheroes2::Text text{ std::to_string( skill.Level() ), fheroes2::FontType::smallWhite() };
        text.draw( pos.x + ( pos.width - text.width() ) - 3, pos.y + pos.height - 10, dstsf );
    }
    else {
        fheroes2::Text text{ Skill::Secondary::String( skill.Skill() ), fheroes2::FontType::smallWhite() };
        if ( text.width() > sprite.width() + 1 ) {
            text.fitToOneRow( pos.width );
        }
        const int skillNamePaddingX = ( pos.width - text.width() ) / 2 - 1;
        text.drawInRoi( pos.x + skillNamePaddingX, pos.y + 5, dstsf, pos );

        text.set( Skill::Level::StringWithBonus( _hero, skill ), fheroes2::FontType::smallWhite() );
        if ( text.width() > sprite.width() + 1 ) {
            text.fitToOneRow( pos.width );
        }
        const int skillLevelPaddingX = ( pos.width - text.width() ) / 2 - 1;
        text.drawInRoi( pos.x + skillLevelPaddingX, pos.y + 53, dstsf, pos );
    }
}

bool SecondarySkillsBar::ActionBarLeftMouseSingleClick( Skill::Secondary & skill )
{
    if ( skill.isValid() ) {
        fheroes2::SecondarySkillDialogElement( skill, _hero ).showPopup( Dialog::OK );
        return true;
    }

    if ( can_change ) {
        const Skill::Secondary alt = Dialog::selectSecondarySkill( _hero );

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
    const bool res = Interface::ItemsBar<Skill::Secondary>::QueueEventProcessing();
    if ( str ) {
        *str = msg;
    }
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
