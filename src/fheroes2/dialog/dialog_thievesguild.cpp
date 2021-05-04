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

#include <algorithm>
#include <string>

#include "agg_image.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "icn.h"
#include "kingdom.h"
#include "monster.h"
#include "settings.h"
#include "text.h"
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
    // bool IsColor(int c) const { return (c & second); };

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
        int value = world.GetKingdom( *color ).GetCountTown();
        UpdateValuesColors( v, value, *color );
    }

    std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
}

void GetCastlesInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        int value = world.GetKingdom( *color ).GetCountCastle();
        UpdateValuesColors( v, value, *color );
    }

    std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
}

void GetHeroesInfo( std::vector<ValueColors> & v, const Colors & colors )
{
    v.clear();

    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        const int value = world.GetKingdom( *color ).GetHeroes().size();
        UpdateValuesColors( v, value, *color );
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
    for ( int32_t ii = 0; ii < static_cast<int32_t>( count ); ++ii ) {
        if ( ii < static_cast<int32_t>( v.size() ) ) {
            const Colors colors( v[ii].second );
            const int32_t sw = fheroes2::AGG::GetICN( ICN::FLAG32, 1 ).width();
            int32_t px = pos.x + ii * step - ( colors.size() * sw - ( colors.size() - 1 ) ) / 2 + 3;

            for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
                const fheroes2::Sprite & flag = fheroes2::AGG::GetICN( ICN::FLAG32, Color::GetIndex( *color ) * 2 + 1 );
                fheroes2::Blit( flag, fheroes2::Display::instance(), px, pos.y );
                px = px + sw - 1;
            }
        }
    }
}

void DrawHeroIcons( const std::vector<ValueColors> & v, const fheroes2::Point & pos, int step )
{
    if ( v.size() ) {
        fheroes2::Display & display = fheroes2::Display::instance();

        for ( u32 ii = 0; ii < v.size(); ++ii ) {
            const Heroes * hero = world.GetHeroes( v[ii].first );
            if ( hero ) {
                int32_t px = pos.x + ii * step;
                const fheroes2::Sprite & window = fheroes2::AGG::GetICN( ICN::LOCATORS, 22 );
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
    for ( size_t i = 0; i < v.size(); ++i ) {
        const Heroes * hero = world.GetHeroes( v[i].first );
        if ( hero == nullptr ) {
            continue;
        }
        const int32_t px = pos.x - 25 + i * step;

        Text text( _( "Att." ), Font::SMALL );
        text.Blit( px, pos.y );
        text.Set( std::to_string( hero->GetAttack() ) );
        text.Blit( px + 50 - text.w(), pos.y );
        text.Set( _( "Def." ) );
        text.Blit( px, pos.y + 10 );
        text.Set( std::to_string( hero->GetDefense() ) );
        text.Blit( px + 50 - text.w(), pos.y + 10 );
        text.Set( _( "Power" ), Font::SMALL );
        text.Blit( px, pos.y + 20 );
        text.Set( std::to_string( hero->GetPower() ) );
        text.Blit( px + 50 - text.w(), pos.y + 20 );
        text.Set( _( "Knowl" ), Font::SMALL );
        text.Blit( px, pos.y + 30 );
        text.Set( std::to_string( hero->GetKnowledge() ) );
        text.Blit( px + 50 - text.w(), pos.y + 30 );
    }
}

void DrawPersonality( const Colors & colors, fheroes2::Point & pos, int step )
{
    for ( size_t i = 0; i < colors.size(); ++i ) {
        const Player * player = Players::Get( colors[i] );
        const Text text( player->isControlHuman() ? _( "Human" ) : player->GetPersonalityString(), Font::SMALL );
        text.Blit( pos.x - text.w() / 2 + step * i, pos.y );
    }
}

void DrawBestMonsterIcons( const Colors & colors, fheroes2::Point & pos, int step )
{
    for ( size_t i = 0; i < colors.size(); ++i ) {
        const Monster monster = world.GetKingdom( colors[i] ).GetStrongestMonster();
        if ( monster.isValid() ) {
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MONS32, monster.GetSpriteIndex() );
            if ( !sprite.empty() )
                fheroes2::Blit( sprite, fheroes2::Display::instance(), pos.x + i * step - sprite.width() / 2, pos.y );
        }
    }
}

void Dialog::ThievesGuild( bool oracle )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // cursor
    Cursor & cursor = Cursor::Get();

    cursor.Hide();
    cursor.SetThemes( Cursor::POINTER );

    Dialog::FrameBorder frameborder( fheroes2::Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT ) );
    const fheroes2::Point cur_pt( frameborder.GetArea().x, frameborder.GetArea().y );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STONEBAK, 0 ), display, cur_pt.x, cur_pt.y );

    fheroes2::Point dst_pt( cur_pt.x, cur_pt.y );

    const uint32_t count = oracle ? 0xFF : world.GetKingdom( Settings::Get().CurrentColor() ).GetCountBuilding( BUILD_THIEVESGUILD );

    std::vector<ValueColors> v;
    v.reserve( KINGDOMMAX );
    const Colors colors( Game::GetActualKingdomColors() );
    const int textx = 185;
    const int startx = 264;
    const int stepx = 68;
    Text text;

    // head 1
    int32_t ii = 0;
    for ( ii = 0; ii < static_cast<int32_t>( colors.size() ); ++ii ) {
        switch ( ii + 1 ) {
        case 1:
            text.Set( _( "1st" ) );
            break;
        case 2:
            text.Set( _( "2nd" ) );
            break;
        case 3:
            text.Set( _( "3rd" ) );
            break;
        case 4:
            text.Set( _( "4th" ) );
            break;
        case 5:
            text.Set( _( "5th" ) );
            break;
        case 6:
            text.Set( _( "6th" ) );
            break;
        default:
            break;
        }

        dst_pt.x = cur_pt.x + startx + stepx * ii - text.w() / 2;
        dst_pt.y = cur_pt.y + 5;
        text.Blit( dst_pt.x, dst_pt.y );
    }

    // bar
    dst_pt.x = cur_pt.x;
    dst_pt.y = cur_pt.y + 461;
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::WELLXTRA, 2 ), display, dst_pt.x, dst_pt.y );

    // text bar
    text.Set( oracle ? _( "Oracle: Player Rankings" ) : _( "Thieves' Guild: Player Rankings" ), Font::BIG );
    dst_pt.x = cur_pt.x + 290 - text.w() / 2;
    dst_pt.y = cur_pt.y + 463;
    text.Blit( dst_pt.x, dst_pt.y );

    // button exit
    dst_pt.x = cur_pt.x + 578;
    dst_pt.y = cur_pt.y + 461;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::WELLXTRA, 0, 1 );

    text.Set( _( "Number of Towns:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 28;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    GetTownsInfo( v, colors );
    DrawFlags( v, dst_pt, stepx, colors.size() );

    text.Set( _( "Number of Castles:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 52;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    GetCastlesInfo( v, colors );
    DrawFlags( v, dst_pt, stepx, colors.size() );

    text.Set( _( "Number of Heroes:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 76;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    GetHeroesInfo( v, colors );
    DrawFlags( v, dst_pt, stepx, colors.size() );

    text.Set( _( "Gold in Treasury:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 100;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    GetGoldsInfo( v, colors );
    if ( 1 < count )
        DrawFlags( v, dst_pt, stepx, colors.size() );

    text.Set( _( "Wood & Ore:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 124;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    GetWoodOreInfo( v, colors );
    if ( 1 < count )
        DrawFlags( v, dst_pt, stepx, colors.size() );

    text.Set( _( "Gems, Cr, Slf & Mer:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 148;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    GetGemsCrSlfMerInfo( v, colors );
    if ( 2 < count )
        DrawFlags( v, dst_pt, stepx, colors.size() );

    text.Set( _( "Obelisks Found:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 172;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    GetObelisksInfo( v, colors );
    if ( 2 < count )
        DrawFlags( v, dst_pt, stepx, colors.size() );

    text.Set( _( "Artifacts:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 196;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    GetArtifactsInfo( v, colors );
    if ( 3 < count ) {
        DrawFlags( v, dst_pt, stepx, colors.size() );
    }

    text.Set( _( "Total Army Strength:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 220;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    GetArmyInfo( v, colors );
    if ( 3 < count )
        DrawFlags( v, dst_pt, stepx, colors.size() );

    text.Set( _( "Income:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 248;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    GetIncomesInfo( v, colors );
    if ( 4 < count )
        DrawFlags( v, dst_pt, stepx, colors.size() );

    // head 2
    ii = 0;
    for ( Colors::const_iterator color = colors.begin(); color != colors.end(); ++color ) {
        text.Set( Color::String( *color ) );
        dst_pt.x = cur_pt.x + startx + ii * stepx - text.w() / 2;
        dst_pt.y = cur_pt.y + 276;
        text.Blit( dst_pt.x, dst_pt.y );
        ++ii;
    }

    text.Set( _( "Best Hero:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 312;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    GetBestHeroArmyInfo( v, colors );
    DrawHeroIcons( v, dst_pt, stepx );

    text.Set( _( "Best Hero Stats:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 353;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    dst_pt.y -= 9;
    if ( 1 < count )
        DrawHeroStats( v, dst_pt, stepx );

    text.Set( _( "Personality:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 394;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    dst_pt.y += 3;
    if ( 2 < count )
        DrawPersonality( colors, dst_pt, stepx );

    text.Set( _( "Best Monster:" ) );
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 435;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = cur_pt.x + startx;
    dst_pt.y -= 13;
    if ( 3 < count )
        DrawBestMonsterIcons( colors, dst_pt, stepx );

    buttonExit.draw();

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
            break;
    }
}
