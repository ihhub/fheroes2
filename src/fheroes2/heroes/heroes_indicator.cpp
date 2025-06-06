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

#include "heroes_indicator.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <utility>

#include "agg_image.h"
#include "army.h"
#include "dialog.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "pal.h"
#include "screen.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace fheroes2
{
    std::string MoraleString( const int morale )
    {
        if ( morale == Morale::BLOOD ) {
            return _( "Blood Morale" );
        }

        std::string str = _( "%{morale} Morale" );

        StringReplace( str, "%{morale}", Morale::String( morale ) );

        return str;
    }

    std::string LuckString( const int luck )
    {
        std::string str = _( "%{luck} Luck" );

        StringReplace( str, "%{luck}", Luck::String( luck ) );

        return str;
    }
}

HeroesIndicator::HeroesIndicator( const Heroes * hero )
    : _hero( hero )
    , _back( fheroes2::Display::instance() )
{
    assert( _hero != nullptr );
}

void HeroesIndicator::SetPos( const fheroes2::Point & pt )
{
    _area.x = pt.x;
    _area.y = pt.y;

    _back.update( _area.x, _area.y, _area.width, _area.height );
}

void LuckIndicator::Redraw()
{
    std::string modificators;
    _luck = _hero->getLuckWithModifiers( &modificators );

    _description.clear();
    _description.append( Luck::Description( _luck ) );
    _description.append( "\n\n" );
    _description.append( _( "Current Luck Modifiers:" ) );
    _description.append( "\n\n" );

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::HSICONS, ( 0 > _luck ? 3 : ( 0 < _luck ? 2 : 6 ) ) );
    const int32_t inter = 6;
    int32_t count = ( 0 == _luck ? 1 : std::abs( _luck ) );
    int32_t cx = _area.x + ( _area.width - ( sprite.width() + inter * ( count - 1 ) ) ) / 2;
    const int32_t cy = _area.y + ( _area.height - sprite.height() ) / 2;

    if ( modificators.empty() ) {
        _description.append( _( "None" ) );
    }
    else {
        _description.append( modificators );
    }

    _back.restore();

    fheroes2::Display & display = fheroes2::Display::instance();

    while ( count-- ) {
        fheroes2::Blit( sprite, display, cx, cy );
        cx += inter;
    }
}

void LuckIndicator::QueueEventProcessing( const LuckIndicator & indicator )
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.MouseClickLeft( indicator._area ) ) {
        fheroes2::showStandardTextMessage( fheroes2::LuckString( indicator._luck ), indicator._description, Dialog::OK );
    }
    else if ( le.isMouseRightButtonPressedInArea( indicator._area ) ) {
        fheroes2::showStandardTextMessage( fheroes2::LuckString( indicator._luck ), indicator._description, Dialog::ZERO );
    }
}

void MoraleIndicator::Redraw()
{
    std::string modificators;
    _morale = _hero->getMoraleWithModifiers( &modificators );

    _description.clear();
    _description.append( Morale::Description( _morale ) );
    _description.append( "\n\n" );
    _description.append( _( "Current Morale Modifiers:" ) );
    _description.append( "\n\n" );

    if ( modificators.empty() ) {
        _description.append( _( "None" ) );
    }
    else {
        _description.append( modificators );
    }

    uint32_t spriteInx = 7;
    if ( _morale < Morale::NORMAL ) {
        spriteInx = 5;
    }
    else if ( _morale > Morale::NORMAL ) {
        spriteInx = 4;
    }

    fheroes2::Sprite sprite = fheroes2::AGG::GetICN( ICN::HSICONS, spriteInx );
    if ( _hero->GetArmy().AllTroopsAreUndead() ) {
        _description.append( "\n\n" );
        _description.append( _( "Entire army is undead, so morale does not apply." ) );
        fheroes2::ApplyPalette( sprite, PAL::GetPalette( PAL::PaletteType::GRAY ) );
        fheroes2::ApplyPalette( sprite, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
    }

    const int32_t inter = 6;
    int32_t count = ( 0 == _morale ? 1 : std::abs( _morale ) );
    int32_t cx = _area.x + ( _area.width - ( sprite.width() + inter * ( count - 1 ) ) ) / 2;
    const int32_t cy = _area.y + ( _area.height - sprite.height() ) / 2;

    _back.restore();

    fheroes2::Display & display = fheroes2::Display::instance();

    while ( count-- ) {
        fheroes2::Blit( sprite, display, cx, cy );
        cx += inter;
    }
}

void MoraleIndicator::QueueEventProcessing( const MoraleIndicator & indicator )
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.MouseClickLeft( indicator._area ) ) {
        fheroes2::showStandardTextMessage( fheroes2::MoraleString( indicator._morale ), indicator._description, Dialog::OK );
    }
    else if ( le.isMouseRightButtonPressedInArea( indicator._area ) ) {
        fheroes2::showStandardTextMessage( fheroes2::MoraleString( indicator._morale ), indicator._description, Dialog::ZERO );
    }
}

ExperienceIndicator::ExperienceIndicator( const Heroes * hero )
    : HeroesIndicator( hero )
{
    assert( hero != nullptr );

    _area.width = 35;
    _area.height = 36;

    _description = _( "Current experience %{exp1}.\n Next level %{exp2}." );

    const uint32_t experience = _hero->GetExperience();
    StringReplace( _description, "%{exp1}", experience );
    StringReplace( _description, "%{exp2}", Heroes::GetExperienceFromLevel( Heroes::GetLevelFromExperience( experience ) ) );
}

void ExperienceIndicator::Redraw() const
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Sprite & experienceImage = fheroes2::AGG::GetICN( ICN::HSICONS, 1 );
    fheroes2::Blit( experienceImage, display, _area.x, _area.y );

    const fheroes2::Rect renderRoi{ _area.x + 1, _area.y + 24, 33, 9 };
    const int32_t widthReduction = experienceImage.width() - renderRoi.width - 1;

    if ( _isDefault ) {
        // For the default range of experience see Heroes::GetStartingXp() method.
        const fheroes2::Text text{ "40-90", fheroes2::FontType::smallWhite() };
        assert( text.width() <= renderRoi.width );
        text.drawInRoi( renderRoi.x + ( experienceImage.width() - text.width() ) / 2 - widthReduction, _area.y + 25, display, renderRoi );
    }
    else {
        // Experience can be longer than the width of the rendering area.
        // This is why it is important to either take into account letter shadows or change the experience value.
        const uint32_t experienceValue = _hero->GetExperience();
        std::string experienceString = std::to_string( _hero->GetExperience() );

        fheroes2::Text text{ std::move( experienceString ), fheroes2::FontType::smallWhite() };
        if ( text.width() > renderRoi.width + 1 ) {
            // The experience string is much longer than the rendering area. We want to avoid too long strings.
            constexpr uint32_t millionValue = 1000000;
            const uint32_t millions = experienceValue / millionValue;

            if ( experienceValue < 10 * millionValue ) {
                // Show two digits after the point ("x.xxM").
                experienceString
                    = std::to_string( millions ) + "." + std::to_string( ( experienceValue - millions * millionValue ) / ( millionValue / 100 ) ) + _( "million|M" );
            }
            else {
                // Show one digit after the point ("xx.xM").
                experienceString
                    = std::to_string( millions ) + "." + std::to_string( ( experienceValue - millions * millionValue ) / ( millionValue / 10 ) ) + _( "million|M" );
            }

            text.set( std::move( experienceString ), fheroes2::FontType::smallWhite() );
            text.drawInRoi( renderRoi.x + ( experienceImage.width() - text.width() ) / 2 - widthReduction, _area.y + 25, display, renderRoi );
        }

        text.drawInRoi( renderRoi.x + ( experienceImage.width() - text.width() ) / 2 - widthReduction, _area.y + 25, display, renderRoi );
    }
}

void ExperienceIndicator::QueueEventProcessing() const
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.MouseClickLeft( _area ) || le.isMouseRightButtonPressedInArea( _area ) ) {
        std::string message = _( "Level %{level}" );

        StringReplace( message, "%{level}", _hero->GetLevel() );

        fheroes2::showStandardTextMessage( std::move( message ), _description, ( le.isMouseRightButtonPressed() ? Dialog::ZERO : Dialog::OK ) );
    }
}

SpellPointsIndicator::SpellPointsIndicator( const Heroes * hero )
    : HeroesIndicator( hero )
{
    _area.width = 35;
    _area.height = 36;

    _description = _(
        "%{name} currently has %{point} spell points out of a maximum of %{max}. The maximum number of spell points is 10 times the hero's knowledge. It is occasionally possible for the hero to have more than their maximum spell points via special events." );

    StringReplace( _description, "%{name}", _hero->GetName() );
    StringReplace( _description, "%{point}", _hero->GetSpellPoints() );
    StringReplace( _description, "%{max}", _hero->GetMaxSpellPoints() );
}

void SpellPointsIndicator::Redraw()
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::HSICONS, 8 );

    const fheroes2::Rect renderRoi{ _area.x + 1, _area.y + 22, 33, 9 };

    fheroes2::Text text;
    if ( _isDefault ) {
        text.set( std::to_string( _hero->GetMaxSpellPoints() ), fheroes2::FontType::smallWhite() );
    }
    else {
        text.set( std::to_string( _hero->GetSpellPoints() ) + "/" + std::to_string( _hero->GetMaxSpellPoints() ), fheroes2::FontType::smallWhite() );
    }

    const bool drawOneLinedText = _isDefault || ( text.width() <= renderRoi.width );
    if ( drawOneLinedText && _needBackgroundRestore ) {
        _needBackgroundRestore = false;
        redrawOnlyBackground();
    }

    // Draw the SpellPoints indicator sprite.
    fheroes2::Blit( sprite, display, _area.x, _area.y );

    if ( drawOneLinedText ) {
        text.drawInRoi( _area.x + sprite.width() / 2 - text.width() / 2, _area.y + 23, display, renderRoi );
    }
    else {
        // The spell points do not fit the render area. Expand the area to the top.
        fheroes2::Copy( sprite, 0, 21, display, renderRoi.x - 1, renderRoi.y - renderRoi.height - 1, renderRoi.width + 2, renderRoi.height + 1 );

        // Render the current Spell Points value at the top line.
        text.set( std::to_string( _hero->GetSpellPoints() ) + "/", fheroes2::FontType::smallWhite() );
        text.drawInRoi( renderRoi.x, renderRoi.y - renderRoi.height + 1, display, { renderRoi.x, renderRoi.y - renderRoi.height, renderRoi.width, renderRoi.height } );

        // Render the maximum Spell Points value in the bottom line.
        text.set( std::to_string( _hero->GetMaxSpellPoints() ), fheroes2::FontType::smallWhite() );
        text.drawInRoi( renderRoi.x + renderRoi.width - text.width() - 1, renderRoi.y + 1, display, renderRoi );

        // The background of the two-lined text should be restored if the one-lined variant is going to be rendered.
        _needBackgroundRestore = true;
    }
}

void SpellPointsIndicator::QueueEventProcessing() const
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.MouseClickLeft( _area ) || le.isMouseRightButtonPressedInArea( _area ) ) {
        fheroes2::showStandardTextMessage( _( "Spell Points" ), _description, ( le.isMouseRightButtonPressed() ? Dialog::ZERO : Dialog::OK ) );
    }
}
