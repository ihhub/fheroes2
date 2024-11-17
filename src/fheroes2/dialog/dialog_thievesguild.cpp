/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "castle.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h" // IWYU pragma: associated
#include "game.h"
#include "game_hotkeys.h"
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
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    struct ValueColors : std::pair<int, int>
    {
        ValueColors( int value, int color )
            : std::pair<int, int>( value, color )
        {}

        static bool SortValueGreat( const ValueColors & v1, const ValueColors & v2 )
        {
            return v1.first > v2.first;
        }
    };

    void UpdateValuesColors( std::vector<ValueColors> & v, int value, int color )
    {
        const auto it = std::find_if( v.begin(), v.end(), [value]( const ValueColors & vc ) { return vc.first == value; } );

        if ( it == v.end() ) {
            v.emplace_back( value, color );
        }
        else {
            ( *it ).second |= color;
        }
    }

    void getInfo( std::vector<ValueColors> & v, const Colors & colors, const std::function<int( const int )> & getValue )
    {
        // 'getValue' should contain a callable function.
        assert( getValue );

        v.clear();

        for ( const int color : colors ) {
            UpdateValuesColors( v, getValue( color ), color );
        }

        std::sort( v.begin(), v.end(), ValueColors::SortValueGreat );
    }

    int getWoodOreValue( const int color )
    {
        const Funds & funds = world.GetKingdom( color ).GetFunds();
        return funds.Get( Resource::WOOD ) + funds.Get( Resource::ORE );
    }

    int getGemsCrSlfMerValue( const int color )
    {
        const Funds & funds = world.GetKingdom( color ).GetFunds();
        return funds.Get( Resource::GEMS ) + funds.Get( Resource::CRYSTAL ) + funds.Get( Resource::SULFUR ) + funds.Get( Resource::MERCURY );
    }

    void drawFlags( const std::vector<ValueColors> & v, const fheroes2::Point & pos, const int32_t step, const size_t count, fheroes2::Image & output )
    {
        const size_t flagGroups = std::min( count, v.size() );

        if ( flagGroups == 0 ) {
            return;
        }

        const int32_t sptireWidth = fheroes2::AGG::GetICN( ICN::TOWNWIND, 22 ).width();
        const int32_t offsetY = pos.y - 4;

        for ( size_t i = 0; i < flagGroups; ++i ) {
            const Colors colors( v[i].second );

            int32_t offsetX = pos.x + static_cast<int32_t>( i ) * step - ( static_cast<int32_t>( colors.size() ) * sptireWidth ) / 2 + 3;

            for ( const int color : colors ) {
                const fheroes2::Sprite & flag = fheroes2::AGG::GetICN( ICN::TOWNWIND, 22 + Color::GetIndex( color ) );
                fheroes2::Blit( flag, output, offsetX, offsetY );
                offsetX += sptireWidth;
            }
        }
    }

    void drawHeroStats( const Heroes * hero, const int32_t offsetX, int32_t offsetY, fheroes2::Image & output )
    {
        fheroes2::Text text( _( "Att." ), fheroes2::FontType::smallWhite() );
        text.draw( offsetX, offsetY, output );
        text.set( std::to_string( hero->GetAttack() ), fheroes2::FontType::smallWhite() );
        text.draw( offsetX + 50 - text.width(), offsetY, output );

        offsetY += 11;
        text.set( _( "Def." ), fheroes2::FontType::smallWhite() );
        text.draw( offsetX, offsetY, output );
        text.set( std::to_string( hero->GetDefense() ), fheroes2::FontType::smallWhite() );
        text.draw( offsetX + 50 - text.width(), offsetY, output );

        offsetY += 11;
        text.set( _( "Power" ), fheroes2::FontType::smallWhite() );
        text.draw( offsetX, offsetY, output );
        text.set( std::to_string( hero->GetPower() ), fheroes2::FontType::smallWhite() );
        text.draw( offsetX + 50 - text.width(), offsetY, output );

        offsetY += 11;
        text.set( _( "Knowl" ), fheroes2::FontType::smallWhite() );
        text.draw( offsetX, offsetY, output );
        text.set( std::to_string( hero->GetKnowledge() ), fheroes2::FontType::smallWhite() );
        text.draw( offsetX + 50 - text.width(), offsetY, output );
    }

    void drawHeroIcons( const Colors & colors, const bool drawStats, const fheroes2::Point & pos, const int32_t step, const int frameIcnID, fheroes2::Image & output )
    {
        int32_t offsetX = pos.x + 1;

        for ( const int color : colors ) {
            const Heroes * hero = world.GetKingdom( color ).GetBestHero();
            if ( hero == nullptr ) {
                offsetX += step;
                continue;
            }

            const fheroes2::Sprite & window = fheroes2::AGG::GetICN( frameIcnID, 22 );
            fheroes2::Blit( window, output, offsetX - window.width() / 2, pos.y - 4 );

            const fheroes2::Sprite & icon = hero->GetPortrait( PORT_SMALL );
            fheroes2::Copy( icon, 0, 0, output, offsetX - icon.width() / 2, pos.y, icon.width(), icon.height() );

            if ( drawStats ) {
                drawHeroStats( hero, offsetX - 26, pos.y + 34, output );
            }

            offsetX += step;
        }
    }

    void drawPersonality( const Colors & colors, const fheroes2::Point & pos, const int32_t step, fheroes2::Image & output )
    {
        int32_t offsetX = pos.x;

        for ( const int color : colors ) {
            const Player * player = Players::Get( color );
            const fheroes2::Text text( player->isControlHuman() ? _( "Human" ) : player->GetPersonalityString(), fheroes2::FontType::smallWhite() );
            text.draw( offsetX - text.width() / 2, pos.y, output );

            offsetX += step;
        }
    }

    void drawBestMonsterIcons( const Colors & colors, const fheroes2::Point & pos, const int32_t step, fheroes2::Image & output )
    {
        int32_t offsetX = pos.x;

        for ( const int color : colors ) {
            const Monster monster = world.GetKingdom( color ).GetStrongestMonster();
            if ( monster.isValid() ) {
                const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MONS32, monster.GetSpriteIndex() );
                fheroes2::Blit( sprite, output, offsetX - sprite.width() / 2, pos.y - sprite.height() / 2 );
            }

            offsetX += step;
        }
    }

    const char * getPlayerOrderString( const size_t player )
    {
        switch ( player ) {
        case 0:
            return _( "1st" );
        case 1:
            return _( "2nd" );
        case 2:
            return _( "3rd" );
        case 3:
            return _( "4th" );
        case 4:
            return _( "5th" );
        case 5:
            return _( "6th" );
        default:
            // The engine supports up to 6 players. Check your logic!
            assert( 0 );
        }

        return {};
    }
}

void Dialog::ThievesGuild( const bool oracle )
{
    // Set the cursor image.This dialog does not require a cursor restorer. It is called from other dialogs that have the same cursor
    // or from the Game Area that will set the appropriate cursor after this dialog is closed.
    Cursor::Get().SetThemes( Cursor::POINTER );

    fheroes2::Rect dialogRoi;
    fheroes2::Rect dialogWithShadowRoi;
    std::unique_ptr<fheroes2::StandardWindow> background;
    std::unique_ptr<fheroes2::ImageRestorer> restorer;

    fheroes2::Display & display = fheroes2::Display::instance();

    if ( oracle ) {
        // We open a new dialog window for the Oracle.
        background = std::make_unique<fheroes2::StandardWindow>( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT, false );
        dialogRoi = background->activeArea();
        dialogWithShadowRoi = background->totalArea();
    }
    else {
        // The Thieves Guild dialog is opened from the castle dialog. We do not need to make a new dialog window.
        dialogRoi = { ( display.width() - fheroes2::Display::DEFAULT_WIDTH ) / 2, ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2,
                      fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT };
        restorer = std::make_unique<fheroes2::ImageRestorer>( display, dialogRoi.x, dialogRoi.y, dialogRoi.width, dialogRoi.height );
    }

    // Fade-out game screen only for 640x480 resolution and if 'renderBackgroundDialog' is false (we are replacing image in already opened dialog).
    const bool isDefaultScreenSize = display.isDefaultSize();
    if ( isDefaultScreenSize || !oracle ) {
        fheroes2::fadeOutDisplay( dialogRoi, !isDefaultScreenSize );
    }

    const bool isEvilInterfaceTown = Settings::Get().isEvilInterfaceEnabled();

    const fheroes2::Sprite & backgroundSprite = fheroes2::AGG::GetICN( isEvilInterfaceTown ? ICN::STONEBAK_EVIL : ICN::STONEBAK, 0 );
    fheroes2::Copy( backgroundSprite, 0, 0, display, dialogRoi.x, dialogRoi.y, backgroundSprite.width(), backgroundSprite.height() );

    const uint32_t thievesGuildCount = oracle ? 0xFF : world.GetKingdom( Settings::Get().CurrentColor() ).GetCountBuilding( BUILD_THIEVESGUILD );

    std::vector<ValueColors> valuesForPlayerColors;
    const Colors colors( Game::GetActualKingdomColors() );
    const size_t playersCount = colors.size();
    valuesForPlayerColors.reserve( playersCount );
    const int32_t textOffsetX = dialogRoi.x + 207;
    const int32_t startOffsetX = dialogRoi.x + 264;
    const int32_t stepX = 68;
    fheroes2::Text text;
    fheroes2::Point offset( startOffsetX, dialogRoi.y + 3 );

    for ( size_t player = 0; player < playersCount; ++player ) {
        text.set( getPlayerOrderString( player ), fheroes2::FontType::normalWhite() );
        text.draw( offset.x - text.width() / 2, offset.y, display );
        offset.x += stepX;
    }

    // Status bar.
    const int32_t exitWidth = fheroes2::AGG::GetICN( ICN::BUTTON_GUILDWELL_EXIT, 0 ).width();
    const int32_t bottomBarOffsetY = 461;

    const fheroes2::Sprite & bottomBar = fheroes2::AGG::GetICN( ICN::SMALLBAR, 0 );
    const int32_t barHeight = bottomBar.height();
    offset.y = dialogRoi.y + bottomBarOffsetY;

    // ICN::SMALLBAR image's first column contains all black pixels. This should not be drawn.
    fheroes2::Copy( bottomBar, 1, 0, display, dialogRoi.x, offset.y, fheroes2::Display::DEFAULT_WIDTH / 2, barHeight );
    fheroes2::Copy( bottomBar, bottomBar.width() - fheroes2::Display::DEFAULT_WIDTH / 2 + exitWidth - 1, 0, display, dialogRoi.x + fheroes2::Display::DEFAULT_WIDTH / 2,
                    offset.y, fheroes2::Display::DEFAULT_WIDTH / 2 - exitWidth + 1, barHeight );

    // Exit button.
    offset.x = dialogRoi.x + fheroes2::Display::DEFAULT_WIDTH - exitWidth;
    fheroes2::Button buttonExit( offset.x, offset.y, ICN::BUTTON_GUILDWELL_EXIT, 0, 1 );

    // Text on status bar.
    text.set( oracle ? _( "Oracle: Player Rankings" ) : _( "Thieves' Guild: Player Rankings" ), fheroes2::FontType::normalWhite() );
    offset.x = dialogRoi.x + ( fheroes2::Display::DEFAULT_WIDTH - exitWidth - text.width() ) / 2;
    text.draw( offset.x, dialogRoi.y + 464, display );

    text.set( _( "Number of Towns:" ), fheroes2::FontType::normalWhite() );
    offset.y = dialogRoi.y + 31;
    text.draw( textOffsetX - text.width(), offset.y, display );

    offset.x = startOffsetX;
    getInfo( valuesForPlayerColors, colors, []( const int color ) { return static_cast<int>( world.GetKingdom( color ).GetCountTown() ); } );
    drawFlags( valuesForPlayerColors, offset, stepX, playersCount, display );

    text.set( _( "Number of Castles:" ), fheroes2::FontType::normalWhite() );
    offset.y += 24;
    text.draw( textOffsetX - text.width(), offset.y, display );

    getInfo( valuesForPlayerColors, colors, []( const int color ) { return static_cast<int>( world.GetKingdom( color ).GetCountCastle() ); } );
    drawFlags( valuesForPlayerColors, offset, stepX, playersCount, display );

    text.set( _( "Number of Heroes:" ), fheroes2::FontType::normalWhite() );
    offset.y += 24;
    text.draw( textOffsetX - text.width(), offset.y, display );

    getInfo( valuesForPlayerColors, colors, []( const int color ) { return static_cast<int>( world.GetKingdom( color ).GetHeroes().size() ); } );
    drawFlags( valuesForPlayerColors, offset, stepX, playersCount, display );

    text.set( _( "Gold in Treasury:" ), fheroes2::FontType::normalWhite() );
    offset.y += 24;
    text.draw( textOffsetX - text.width(), offset.y, display );

    if ( thievesGuildCount > 1 ) {
        getInfo( valuesForPlayerColors, colors, []( const int color ) { return world.GetKingdom( color ).GetFunds().Get( Resource::GOLD ); } );
        drawFlags( valuesForPlayerColors, offset, stepX, playersCount, display );
    }

    text.set( _( "Wood & Ore:" ), fheroes2::FontType::normalWhite() );
    offset.y += 24;
    text.draw( textOffsetX - text.width(), offset.y, display );

    if ( thievesGuildCount > 1 ) {
        getInfo( valuesForPlayerColors, colors, getWoodOreValue );
        drawFlags( valuesForPlayerColors, offset, stepX, playersCount, display );
    }

    text.set( _( "Gems, Cr, Slf & Mer:" ), fheroes2::FontType::normalWhite() );
    offset.y += 24;
    text.draw( textOffsetX - text.width(), offset.y, display );

    if ( thievesGuildCount > 2 ) {
        getInfo( valuesForPlayerColors, colors, getGemsCrSlfMerValue );
        drawFlags( valuesForPlayerColors, offset, stepX, playersCount, display );
    }

    text.set( _( "Obelisks Found:" ), fheroes2::FontType::normalWhite() );
    offset.y += 24;
    text.draw( textOffsetX - text.width(), offset.y, display );

    if ( thievesGuildCount > 2 ) {
        getInfo( valuesForPlayerColors, colors, []( const int color ) { return static_cast<int>( world.GetKingdom( color ).CountVisitedObjects( MP2::OBJ_OBELISK ) ); } );
        drawFlags( valuesForPlayerColors, offset, stepX, playersCount, display );
    }

    text.set( _( "Artifacts:" ), fheroes2::FontType::normalWhite() );
    offset.y += 24;
    text.draw( textOffsetX - text.width(), offset.y, display );

    if ( thievesGuildCount > 3 ) {
        getInfo( valuesForPlayerColors, colors, []( const int color ) { return static_cast<int>( world.GetKingdom( color ).GetCountArtifacts() ); } );
        drawFlags( valuesForPlayerColors, offset, stepX, playersCount, display );
    }

    text.set( _( "Total Army Strength:" ), fheroes2::FontType::normalWhite() );
    offset.y += 24;
    text.draw( textOffsetX - text.width(), offset.y, display );

    if ( thievesGuildCount > 3 ) {
        getInfo( valuesForPlayerColors, colors, []( const int color ) { return static_cast<int>( world.GetKingdom( color ).GetArmiesStrength() ); } );
        drawFlags( valuesForPlayerColors, offset, stepX, playersCount, display );
    }

    text.set( _( "Income:" ), fheroes2::FontType::normalWhite() );
    offset.y += 24;
    text.draw( textOffsetX - text.width(), offset.y, display );

    if ( thievesGuildCount > 4 ) {
        getInfo( valuesForPlayerColors, colors, []( const int color ) { return world.GetKingdom( color ).GetIncome().gold; } );
        drawFlags( valuesForPlayerColors, offset, stepX, playersCount, display );
    }

    // Render color's names for each player.
    offset.y += 24;
    bool shiftVertically = false;
    for ( const int color : colors ) {
        text.set( Color::String( color ), fheroes2::FontType::normalWhite() );
        text.draw( offset.x - text.width() / 2, shiftVertically ? ( offset.y + 15 ) : offset.y, display );
        offset.x += stepX;
        shiftVertically = !shiftVertically;
    }

    text.set( _( "Best Hero:" ), fheroes2::FontType::normalWhite() );
    offset.y += 38;
    text.draw( textOffsetX - text.width(), offset.y, display );

    offset.x = startOffsetX;
    offset.y -= 4;
    const int frameIcnID = isEvilInterfaceTown ? ICN::LOCATORE : ICN::LOCATORS;
    drawHeroIcons( colors, thievesGuildCount > 1, offset, stepX, frameIcnID, display );

    text.set( _( "Best Hero Stats:" ), fheroes2::FontType::normalWhite() );
    offset.y += 47;
    text.draw( textOffsetX - text.width(), offset.y, display );

    text.set( _( "Personality:" ), fheroes2::FontType::normalWhite() );
    offset.y += 42;
    text.draw( textOffsetX - text.width(), offset.y, display );

    offset.y += 3;
    if ( thievesGuildCount > 2 ) {
        drawPersonality( colors, offset, stepX, display );
    }

    text.set( _( "Best Monster:" ), fheroes2::FontType::normalWhite() );
    offset.y += 32;
    text.draw( textOffsetX - text.width(), offset.y, display );

    offset.y += text.height() / 2;
    if ( thievesGuildCount > 3 ) {
        drawBestMonsterIcons( colors, offset, stepX, display );
    }

    buttonExit.draw();

    // Fade-in thieves guild dialog.
    if ( oracle && !isDefaultScreenSize ) {
        // We need to expand the ROI for the next render to properly render window borders and shadow.
        display.updateNextRenderRoi( dialogWithShadowRoi );
    }

    // Use half fade if game resolution is not 640x480.
    fheroes2::fadeInDisplay( dialogRoi, !isDefaultScreenSize );

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        buttonExit.drawOnState( le.isMouseLeftButtonPressedInArea( buttonExit.area() ) );

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
            break;
        }
        if ( le.isMouseRightButtonPressedInArea( buttonExit.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Exit" ), _( "Exit this menu." ), Dialog::ZERO );
        }
    }

    // Fade-out dialog.
    fheroes2::fadeOutDisplay( dialogRoi, !isDefaultScreenSize );

    if ( oracle ) {
        // Fade-in game screen only for 640x480 resolution.
        if ( isDefaultScreenSize ) {
            Game::setDisplayFadeIn();
        }
        else {
            display.updateNextRenderRoi( dialogWithShadowRoi );
        }
    }
    else {
        restorer->restore();
        fheroes2::fadeInDisplay( dialogRoi, !isDefaultScreenSize );
    }
}
