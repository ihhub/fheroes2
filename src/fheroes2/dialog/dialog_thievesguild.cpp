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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "castle.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_hotkeys.h"
#include "gamedefs.h"
#include "heroes.h"
#include "heroes_base.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "localevent.h"
#include "math_base.h"
#include "monster.h"
#include "mp2.h"
#include "players.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_text.h"
#include "world.h"

struct ValueColors : std::pair<int, int>
{
    ValueColors( int v, int c )
        : std::pair<int, int>( v, c )
    {}

    bool IsValue( int v ) const
    {
        return v == first;
    }

    static bool SortValueGreat( const ValueColors & v1, const ValueColors & v2 )
    {
        return v1.first > v2.first;
    }
};

void UpdateValuesColors( std::vector<ValueColors> & v, int value, int color )
{
    std::vector<ValueColors>::iterator it = std::find_if( v.begin(), v.end(), [value]( const ValueColors & vc ) { return vc.IsValue( value ); } );

    if ( it == v.end() )
        v.emplace_back( value, color );
    else
        ( *it ).second |= color;
}

void GetTownsInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        const uint32_t townCount = world.GetKingdom( *color ).GetCountTown();
        UpdateValuesColors( v, static_cast<int32_t>( townCount ), *color );
    }

    std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
}

void GetCastlesInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        const uint32_t castleCount = world.GetKingdom( *color ).GetCountCastle();
        UpdateValuesColors( v, static_cast<int32_t>( castleCount ), *color );
    }

    std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
}

void GetHeroesInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        const size_t heroCount = world.GetKingdom( *color ).GetHeroes().size();
        UpdateValuesColors( v, static_cast<int>( heroCount ), *color );
    }

    std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
}

void GetGoldsInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        const int value = world.GetKingdom( *color ).GetFunds().Get( Resource::GOLD );
        UpdateValuesColors( v, value, *color );
    }

    std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
}

void GetWoodOreInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        const Funds & funds = world.GetKingdom( *color ).GetFunds();
        const int value = funds.Get( Resource::WOOD ) + funds.Get( Resource::ORE );
        UpdateValuesColors( v, value, *color );
    }

    std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
}

void GetGemsCrSlfMerInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        const Funds & funds = world.GetKingdom( *color ).GetFunds();
        const int value = funds.Get( Resource::GEMS ) + funds.Get( Resource::CRYSTAL ) + funds.Get( Resource::SULFUR ) + funds.Get( Resource::MERCURY );
        UpdateValuesColors( v, value, *color );
    }

    std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
}

void GetObelisksInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        const int value = world.GetKingdom( *color ).CountVisitedObjects( MP2::OBJ_OBELISK );
        UpdateValuesColors( v, value, *color );
    }

    std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
}

void GetArtifactsInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        const int value = world.GetKingdom( *color ).GetCountArtifacts();
        UpdateValuesColors( v, value, *color );
    }

    std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
}

void GetArmyInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        const int value = static_cast<int>( world.GetKingdom( *color ).GetArmiesStrength() );
        UpdateValuesColors( v, value, *color );
    }

    std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
}

void GetIncomesInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        const int value = world.GetKingdom( *color ).GetIncome().gold;
        UpdateValuesColors( v, value, *color );
    }

    std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
}

void GetBestHeroArmyInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        const Heroes * hero = world.GetKingdom( *color ).GetBestHero();
        v.emplace_back( ( hero ? hero->GetID() : Heroes::UNKNOWN ), *color );
    }
}

void DrawFlags( const std::vector<ValueColors> & v, const fheroes2::Point & pos, int step, size_t count )
{
    for ( int32_t i = 0; i < static_cast<int32_t>( count ); ++i ) {
        if ( i < static_cast<int32_t>( v.size() ) ) {
            const Colors colors( v[i].second );
            const int32_t sw = fheroes2::AGG::GetICN( ICN::TOWNWIND, 22 ).width();
            int32_t px = pos.x + i * step - ( static_cast<int32_t>( colors.size() ) * sw ) / 2 + 3;

            for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
                const fheroes2::Sprite & flag = fheroes2::AGG::GetICN( ICN::TOWNWIND, 22 + Color::GetIndex( *color ) );
                fheroes2::Blit( flag, fheroes2::Display::instance(), px, pos.y - 2 );
                px = px + sw;
            }
        }
    }
}

void DrawHeroIcons( const std::vector<ValueColors> & v, const fheroes2::Point & pos, int step, const int frameIcnID )
{
    if ( !v.empty() ) {
        fheroes2::Display & display = fheroes2::Display::instance();

        for ( uint32_t ii = 0; ii < v.size(); ++ii ) {
            const Heroes * hero = world.GetHeroes( v[ii].first );
            if ( hero ) {
                int32_t px = pos.x + ii * step;
                const fheroes2::Sprite & window = fheroes2::AGG::GetICN( frameIcnID, 22 );
                fheroes2::Blit( window, display, px - window.width() / 2, pos.y - 4 );

                const fheroes2::Sprite & icon = hero->GetPortrait( PORT_SMALL );
                if ( !icon.empty() )
                    fheroes2::Blit( icon, display, px - icon.width() / 2, pos.y );
            }
        }
    }
}

void DrawHeroStats( const std::vector<ValueColors> & v, const fheroes2::Point & pos, int step )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    for ( size_t i = 0; i < v.size(); ++i ) {
        const Heroes * hero = world.GetHeroes( v[i].first );
        if ( hero == nullptr ) {
            continue;
        }
        const int32_t px = pos.x - 25 + static_cast<int32_t>( i ) * step;

        fheroes2::Text text( _( "Att." ), fheroes2::FontType::smallWhite() );
        text.draw( px, pos.y + 2, display );
        text.set( std::to_string( hero->GetAttack() ), fheroes2::FontType::smallWhite() );
        text.draw( px + 50 - text.width(), pos.y + 2, display );

        text.set( _( "Def." ), fheroes2::FontType::smallWhite() );
        text.draw( px, pos.y + 13, display );
        text.set( std::to_string( hero->GetDefense() ), fheroes2::FontType::smallWhite() );
        text.draw( px + 50 - text.width(), pos.y + 13, display );

        text.set( _( "Power" ), fheroes2::FontType::smallWhite() );
        text.draw( px, pos.y + 24, display );
        text.set( std::to_string( hero->GetPower() ), fheroes2::FontType::smallWhite() );
        text.draw( px + 50 - text.width(), pos.y + 24, display );

        text.set( _( "Knowl" ), fheroes2::FontType::smallWhite() );
        text.draw( px, pos.y + 35, display );
        text.set( std::to_string( hero->GetKnowledge() ), fheroes2::FontType::smallWhite() );
        text.draw( px + 50 - text.width(), pos.y + 35, display );
    }
}

void DrawPersonality( const Colors & colors, const fheroes2::Point & pos, int step )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    for ( size_t i = 0; i < colors.size(); ++i ) {
        const Player * player = Players::Get( colors[i] );
        const fheroes2::Text text( player->isControlHuman() ? _( "Human" ) : player->GetPersonalityString(), fheroes2::FontType::smallWhite() );
        text.draw( pos.x - text.width() / 2 + step * static_cast<int32_t>( i ), pos.y + 2, display );
    }
}

void DrawBestMonsterIcons( const Colors & colors, const fheroes2::Point & pos, int step )
{
    for ( size_t i = 0; i < colors.size(); ++i ) {
        const Monster monster = world.GetKingdom( colors[i] ).GetStrongestMonster();
        if ( monster.isValid() ) {
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MONS32, monster.GetSpriteIndex() );
            if ( !sprite.empty() )
                fheroes2::Blit( sprite, fheroes2::Display::instance(), pos.x + static_cast<int32_t>( i ) * step - sprite.width() / 2, pos.y );
        }
    }
}

void Dialog::ThievesGuild( bool oracle )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    Dialog::FrameBorder frameborder( { fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT } );
    const fheroes2::Point cur_pt( frameborder.GetArea().x, frameborder.GetArea().y );

    const bool isEvilInterfaceTown = !oracle && Settings::Get().isEvilInterfaceEnabled();

    const int backgroundIcnID = isEvilInterfaceTown ? ICN::STONEBAK_EVIL : ICN::STONEBAK;
    fheroes2::Blit( fheroes2::AGG::GetICN( backgroundIcnID, 0 ), display, cur_pt.x, cur_pt.y );

    fheroes2::Point dst_pt( cur_pt.x, cur_pt.y );

    const uint32_t count = oracle ? 0xFF : world.GetKingdom( Settings::Get().CurrentColor() ).GetCountBuilding( BUILD_THIEVESGUILD );

    std::vector<ValueColors> v;
    v.reserve( KINGDOMMAX );
    const Colors colors( Game::GetActualKingdomColors() );
    const int textx = 207;
    const int startx = 264;
    const int stepx = 68;
    fheroes2::Text text;

    // head 1
    int32_t ii = 0;
    for ( ii = 0; ii < static_cast<int32_t>( colors.size() ); ++ii ) {
        switch ( ii + 1 ) {
        case 1:
            text.set( _( "1st" ), fheroes2::FontType::normalWhite() );
            break;
        case 2:
            text.set( _( "2nd" ), fheroes2::FontType::normalWhite() );
            break;
        case 3:
            text.set( _( "3rd" ), fheroes2::FontType::normalWhite() );
            break;
        case 4:
            text.set( _( "4th" ), fheroes2::FontType::normalWhite() );
            break;
        case 5:
            text.set( _( "5th" ), fheroes2::FontType::normalWhite() );
            break;
        case 6:
            text.set( _( "6th" ), fheroes2::FontType::normalWhite() );
            break;
        default:
            break;
        }

        dst_pt.x = cur_pt.x + startx + stepx * ii - text.width() / 2;
        dst_pt.y = cur_pt.y + 3;
        text.draw( dst_pt.x, dst_pt.y, display );
    }

    // status bar
    const int32_t exitWidth = fheroes2::AGG::GetICN( ICN::BUTTON_GUILDWELL_EXIT, 0 ).width();
    const int32_t bottomBarOffsetY = 461;

    dst_pt.x = cur_pt.x;
    dst_pt.y = cur_pt.y + bottomBarOffsetY;

    const fheroes2::Sprite & bottomBar = fheroes2::AGG::GetICN( ICN::SMALLBAR, 0 );
    const int32_t barHeight = bottomBar.height();
    // ICN::SMALLBAR image's first column contains all black pixels. This should not be drawn.
    fheroes2::Copy( bottomBar, 1, 0, display, dst_pt.x, dst_pt.y, fheroes2::Display::DEFAULT_WIDTH / 2, barHeight );
    fheroes2::Copy( bottomBar, bottomBar.width() - fheroes2::Display::DEFAULT_WIDTH / 2 + exitWidth - 1, 0, display, dst_pt.x + fheroes2::Display::DEFAULT_WIDTH / 2,
                    dst_pt.y, fheroes2::Display::DEFAULT_WIDTH / 2 - exitWidth + 1, barHeight );

    // text bar
    text.set( oracle ? _( "Oracle: Player Rankings" ) : _( "Thieves' Guild: Player Rankings" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + 290 - text.width() / 2;
    dst_pt.y = cur_pt.y + 465;
    text.draw( dst_pt.x, dst_pt.y, display );

    // button exit
    dst_pt.x = cur_pt.x + fheroes2::Display::DEFAULT_WIDTH - exitWidth;
    dst_pt.y = cur_pt.y + 461;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::BUTTON_GUILDWELL_EXIT, 0, 1 );

    text.set( _( "Number of Towns:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 29;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    GetTownsInfo( v, colors );
    DrawFlags( v, dst_pt, stepx, colors.size() );

    text.set( _( "Number of Castles:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 53;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    GetCastlesInfo( v, colors );
    DrawFlags( v, dst_pt, stepx, colors.size() );

    text.set( _( "Number of Heroes:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 77;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    GetHeroesInfo( v, colors );
    DrawFlags( v, dst_pt, stepx, colors.size() );

    text.set( _( "Gold in Treasury:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 101;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    GetGoldsInfo( v, colors );
    if ( 1 < count )
        DrawFlags( v, dst_pt, stepx, colors.size() );

    text.set( _( "Wood & Ore:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 125;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    GetWoodOreInfo( v, colors );
    if ( 1 < count )
        DrawFlags( v, dst_pt, stepx, colors.size() );

    text.set( _( "Gems, Cr, Slf & Mer:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 149;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    GetGemsCrSlfMerInfo( v, colors );
    if ( 2 < count )
        DrawFlags( v, dst_pt, stepx, colors.size() );

    text.set( _( "Obelisks Found:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 173;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    GetObelisksInfo( v, colors );
    if ( 2 < count )
        DrawFlags( v, dst_pt, stepx, colors.size() );

    text.set( _( "Artifacts:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 197;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    GetArtifactsInfo( v, colors );
    if ( 3 < count ) {
        DrawFlags( v, dst_pt, stepx, colors.size() );
    }

    text.set( _( "Total Army Strength:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 221;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    GetArmyInfo( v, colors );
    if ( 3 < count )
        DrawFlags( v, dst_pt, stepx, colors.size() );

    text.set( _( "Income:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 245;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    GetIncomesInfo( v, colors );
    if ( 4 < count )
        DrawFlags( v, dst_pt, stepx, colors.size() );

    // head 2
    ii = 0;
    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        text.set( Color::String( *color ), fheroes2::FontType::normalWhite() );
        dst_pt.x = cur_pt.x + startx + ii * stepx - text.width() / 2;
        dst_pt.y = cur_pt.y + 278;
        text.draw( dst_pt.x, dst_pt.y + 2, display );
        ++ii;
    }

    text.set( _( "Best Hero:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 307;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx + 1;
    dst_pt.y -= 2;
    GetBestHeroArmyInfo( v, colors );
    const int frameIcnID = isEvilInterfaceTown ? ICN::LOCATORE : ICN::LOCATORS;
    DrawHeroIcons( v, dst_pt, stepx, frameIcnID );

    text.set( _( "Best Hero Stats:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 350;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    dst_pt.y -= 13;
    if ( 1 < count )
        DrawHeroStats( v, dst_pt, stepx );

    text.set( _( "Personality:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 392;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    dst_pt.y += 3;
    if ( 2 < count )
        DrawPersonality( colors, dst_pt, stepx );

    text.set( _( "Best Monster:" ), fheroes2::FontType::normalWhite() );
    dst_pt.x = cur_pt.x + textx - text.width();
    dst_pt.y = cur_pt.y + 427;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    dst_pt.x = cur_pt.x + startx;
    dst_pt.y -= 9;
    if ( 3 < count )
        DrawBestMonsterIcons( colors, dst_pt, stepx );

    buttonExit.draw();

    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() )
            break;
    }
}
