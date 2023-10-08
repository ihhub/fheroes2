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

#include "heroes_indicator.h"

#include <cstdint>
#include <cstdlib>

#include "agg_image.h"
#include "army.h"
#include "dialog.h"
#include "heroes.h"
#include "icn.h"
#include "localevent.h"
#include "luck.h"
#include "morale.h"
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

HeroesIndicator::HeroesIndicator( const Heroes * h )
    : hero( h )
    , back( fheroes2::Display::instance() )
{
    descriptions.reserve( 256 );
}

const fheroes2::Rect & HeroesIndicator::GetArea() const
{
    return area;
}

void HeroesIndicator::SetHero( const Heroes * h )
{
    hero = h;
}

void HeroesIndicator::SetPos( const fheroes2::Point & pt )
{
    area.x = pt.x;
    area.y = pt.y;
    back.update( area.x, area.y, area.width, area.height );
}

LuckIndicator::LuckIndicator( const Heroes * h )
    : HeroesIndicator( h )
    , luck( Luck::NORMAL )
{
    area.width = 35;
    area.height = 26;
}

void LuckIndicator::Redraw()
{
    if ( !hero )
        return;

    std::string modificators;
    modificators.reserve( 256 );
    luck = hero->GetLuckWithModificators( &modificators );

    descriptions.clear();
    descriptions.append( Luck::Description( luck ) );
    descriptions.append( "\n\n" );
    descriptions.append( _( "Current Luck Modifiers:" ) );
    descriptions.append( "\n\n" );

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::HSICONS, ( 0 > luck ? 3 : ( 0 < luck ? 2 : 6 ) ) );
    const int inter = 6;
    int count = ( 0 == luck ? 1 : std::abs( luck ) );
    int32_t cx = area.x + ( area.width - ( sprite.width() + inter * ( count - 1 ) ) ) / 2;
    int32_t cy = area.y + ( area.height - sprite.height() ) / 2;

    if ( !modificators.empty() )
        descriptions.append( modificators );
    else
        descriptions.append( _( "None" ) );

    back.restore();
    while ( count-- ) {
        fheroes2::Blit( sprite, fheroes2::Display::instance(), cx, cy );
        cx += inter;
    }
}

void LuckIndicator::QueueEventProcessing( const LuckIndicator & indicator )
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.MouseClickLeft( indicator.area ) )
        fheroes2::showStandardTextMessage( fheroes2::LuckString( indicator.luck ), indicator.descriptions, Dialog::OK );
    else if ( le.MousePressRight( indicator.area ) )
        fheroes2::showStandardTextMessage( fheroes2::LuckString( indicator.luck ), indicator.descriptions, Dialog::ZERO );
}

MoraleIndicator::MoraleIndicator( const Heroes * h )
    : HeroesIndicator( h )
    , morale( Morale::NORMAL )
{
    area.width = 35;
    area.height = 26;
}

void MoraleIndicator::Redraw()
{
    if ( !hero )
        return;

    std::string modificators;
    modificators.reserve( 256 );
    morale = hero->GetMoraleWithModificators( &modificators );

    descriptions.clear();
    descriptions.append( Morale::Description( morale ) );
    descriptions.append( "\n\n" );
    descriptions.append( _( "Current Morale Modifiers:" ) );
    descriptions.append( "\n\n" );

    if ( modificators.empty() )
        descriptions.append( _( "None" ) );
    else
        descriptions.append( modificators );

    descriptions.append( "\n\n" );
    if ( hero->GetArmy().AllTroopsAreUndead() ) {
        descriptions.append( _( "Entire army is undead, so morale does not apply." ) );
    }

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::HSICONS, ( 0 > morale ? 5 : ( 0 < morale ? 4 : 7 ) ) );
    const int inter = 6;
    int count = ( 0 == morale ? 1 : std::abs( morale ) );
    int32_t cx = area.x + ( area.width - ( sprite.width() + inter * ( count - 1 ) ) ) / 2;
    int32_t cy = area.y + ( area.height - sprite.height() ) / 2;

    back.restore();
    while ( count-- ) {
        fheroes2::Blit( sprite, fheroes2::Display::instance(), cx, cy );
        cx += inter;
    }
}

void MoraleIndicator::QueueEventProcessing( const MoraleIndicator & indicator )
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.MouseClickLeft( indicator.area ) )
        fheroes2::showStandardTextMessage( fheroes2::MoraleString( indicator.morale ), indicator.descriptions, Dialog::OK );
    else if ( le.MousePressRight( indicator.area ) )
        fheroes2::showStandardTextMessage( fheroes2::MoraleString( indicator.morale ), indicator.descriptions, Dialog::ZERO );
}

ExperienceIndicator::ExperienceIndicator( const Heroes * h )
    : HeroesIndicator( h )
{
    area.width = 35;
    area.height = 36;

    descriptions = _( "Current experience %{exp1}.\n Next level %{exp2}." );
    if ( hero ) {
        const uint32_t experience = hero->GetExperience();
        StringReplace( descriptions, "%{exp1}", experience );
        StringReplace( descriptions, "%{exp2}", Heroes::GetExperienceFromLevel( Heroes::GetLevelFromExperience( experience ) ) );
    }
}

void ExperienceIndicator::Redraw() const
{
    if ( !hero )
        return;

    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Sprite & sprite3 = fheroes2::AGG::GetICN( ICN::HSICONS, 1 );
    fheroes2::Blit( sprite3, display, area.x, area.y );

    const fheroes2::Text text( std::to_string( hero->GetExperience() ), fheroes2::FontType::smallWhite() );
    text.draw( area.x + 17 - text.width() / 2, area.y + 25, display );
}

void ExperienceIndicator::QueueEventProcessing() const
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.MouseClickLeft( area ) || le.MousePressRight( area ) ) {
        std::string message = _( "Level %{level}" );
        StringReplace( message, "%{level}", hero->GetLevel() );
        fheroes2::showStandardTextMessage( message, descriptions, ( le.MousePressRight() ? Dialog::ZERO : Dialog::OK ) );
    }
}

SpellPointsIndicator::SpellPointsIndicator( const Heroes * h )
    : HeroesIndicator( h )
{
    area.width = 35;
    area.height = 36;

    descriptions = _(
        "%{name} currently has %{point} spell points out of a maximum of %{max}. The maximum number of spell points is 10 times your knowledge. It is occasionally possible to have more than your maximum spell points via special events." );
    if ( hero ) {
        StringReplace( descriptions, "%{name}", hero->GetName() );
        StringReplace( descriptions, "%{point}", hero->GetSpellPoints() );
        StringReplace( descriptions, "%{max}", hero->GetMaxSpellPoints() );
    }
}

void SpellPointsIndicator::Redraw() const
{
    if ( !hero )
        return;

    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Sprite & sprite3 = fheroes2::AGG::GetICN( ICN::HSICONS, 8 );
    fheroes2::Blit( sprite3, display, area.x, area.y );

    const fheroes2::Text text( std::to_string( hero->GetSpellPoints() ) + "/" + std::to_string( hero->GetMaxSpellPoints() ), fheroes2::FontType::smallWhite() );
    text.draw( area.x + sprite3.width() / 2 - text.width() / 2, area.y + 23, display );
}

void SpellPointsIndicator::QueueEventProcessing() const
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.MouseClickLeft( area ) || le.MousePressRight( area ) ) {
        fheroes2::showStandardTextMessage( _( "Spell Points" ), descriptions, ( le.MousePressRight() ? Dialog::ZERO : Dialog::OK ) );
    }
}
