/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "battle_interface.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <ostream>
#include <set>

#include "agg_image.h"
#include "audio.h"
#include "audio_manager.h"
#include "battle.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "battle_bridge.h"
#include "battle_catapult.h"
#include "battle_cell.h"
#include "battle_command.h"
#include "battle_tower.h"
#include "battle_troop.h"
#include "bin_info.h"
#include "castle.h"
#include "color.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "gamedefs.h"
#include "ground.h"
#include "heroes_base.h"
#include "icn.h"
#include "interface_list.h"
#include "localevent.h"
#include "logging.h"
#include "m82.h"
#include "maps.h"
#include "maps_tiles.h"
#include "monster.h"
#include "monster_anim.h"
#include "monster_info.h"
#include "mp2.h"
#include "mus.h"
#include "pal.h"
#include "players.h"
#include "race.h"
#include "rand.h"
#include "screen.h"
#include "settings.h"
#include "spell_book.h"
#include "timing.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"
#include "world.h"

class Kingdom;

namespace
{
    const int32_t cellYOffset = -9;

    const int32_t armyOrderMonsterIconSize = 43; // in both directions.

    // The parameters of castle buildings destruction by a catapult:
    // Smoke cloud frame number, after which the building should be drawn as destroyed.
    const int32_t castleBuildingDestroyFrame = 5;
    // Bridge demolition second smoke cloud offset from the first one after the catapult attack.
    const fheroes2::Point bridgeDestroySmokeOffset( -45, 65 );
    // Smoke cloud frame number, after which the bridge should be drawn as destroyed.
    const int32_t bridgeDestroyFrame = 6;
    // The number of frames the second smoke cloud is delayed by.
    const int32_t bridgeDestroySmokeDelay = 2;

    struct LightningPoint
    {
        explicit LightningPoint( const fheroes2::Point & p = fheroes2::Point(), uint32_t thick = 1 )
            : point( p )
            , thickness( thick )
        {}

        fheroes2::Point point;
        uint32_t thickness;
    };

    double getDistance( const fheroes2::Point & p1, const fheroes2::Point & p2 )
    {
        const double diffX = p1.x - p2.x;
        const double diffY = p1.y - p2.y;

        return std::sqrt( diffX * diffX + diffY * diffY );
    }

    fheroes2::Point rotate( const fheroes2::Point & point, double angle )
    {
        const double sinValue = sin( angle );
        const double cosValue = cos( angle );

        return { static_cast<int32_t>( point.x * cosValue - point.y * sinValue ), static_cast<int32_t>( point.x * sinValue + point.y * cosValue ) };
    }

    double getAngle( const fheroes2::Point & start, const fheroes2::Point & end )
    {
        return std::atan2( end.y - start.y, end.x - start.x );
    }

    std::vector<std::pair<LightningPoint, LightningPoint>> GenerateLightning( const fheroes2::Point & src, const fheroes2::Point & dst )
    {
        const int distance = static_cast<int>( getDistance( src, dst ) );
        const double angle = getAngle( src, dst );

        int iterationCount = ( distance + 50 ) / 100;
        if ( iterationCount < 3 )
            iterationCount = 3;
        if ( iterationCount > 5 )
            iterationCount = 5;

        std::vector<std::pair<LightningPoint, LightningPoint>> lines;
        lines.emplace_back( LightningPoint( { 0, 0 }, 5 ), LightningPoint( { distance, 0 }, 3 ) );

        int maxOffset = distance;

        for ( int step = 0; step < iterationCount; ++step ) {
            std::vector<std::pair<LightningPoint, LightningPoint>> oldLines;
            std::swap( lines, oldLines );

            for ( size_t i = 0; i < oldLines.size(); ++i ) {
                fheroes2::Point middle( oldLines[i].first.point + oldLines[i].second.point );
                middle.x /= 2;
                middle.y /= 2;

                const bool isPositive = ( Rand::Get( 1, 2 ) == 1 );
                int offsetY = static_cast<int>( Rand::Get( 1, 10 ) ) * maxOffset / 100;
                if ( offsetY < 1 )
                    offsetY = 1;

                middle.y += isPositive ? offsetY : -offsetY;

                const uint32_t middleThickness = ( oldLines[i].first.thickness + oldLines[i].second.thickness ) / 2;

                const LightningPoint middlePoint( middle, middleThickness );

                lines.emplace_back( oldLines[i].first, middlePoint );
                lines.emplace_back( middlePoint, oldLines[i].second );

                if ( Rand::Get( 1, 4 ) == 1 ) { // 25%
                    offsetY = static_cast<int>( Rand::Get( 1, 10 ) ) * maxOffset / 100;
                    const int32_t x = static_cast<int32_t>( ( middle.x - oldLines[i].first.point.x ) * 0.7 ) + middle.x;
                    const int32_t y = static_cast<int32_t>( ( middle.y - oldLines[i].first.point.y ) * 0.7 ) + middle.y + ( isPositive ? offsetY : -offsetY );
                    lines.emplace_back( middlePoint, LightningPoint( { x, y }, 1 ) );
                }
            }

            maxOffset /= 2;
        }

        for ( size_t i = 0; i < lines.size(); ++i ) {
            lines[i].first.point = rotate( lines[i].first.point, angle ) + src;
            lines[i].second.point = rotate( lines[i].second.point, angle ) + src;
        }

        return lines;
    }

    void RedrawLightning( const std::vector<std::pair<LightningPoint, LightningPoint>> & lightning, uint8_t color, fheroes2::Image & surface,
                          const fheroes2::Rect & roi = fheroes2::Rect() )
    {
        for ( size_t i = 0; i < lightning.size(); ++i ) {
            const fheroes2::Point & first = lightning[i].first.point;
            const fheroes2::Point & second = lightning[i].second.point;
            const bool isHorizontal = std::abs( first.x - second.x ) >= std::abs( first.y - second.y );
            const int xOffset = isHorizontal ? 0 : 1;
            const int yOffset = isHorizontal ? 1 : 0;

            fheroes2::DrawLine( surface, first, second, color, roi );

            for ( uint32_t thickness = 1; thickness < lightning[i].second.thickness; ++thickness ) {
                const bool isUpper = ( ( thickness % 2 ) == 1 );
                const int offset = isUpper ? ( thickness + 1 ) / 2 : -static_cast<int>( ( thickness + 1 ) / 2 );
                const int x = xOffset * offset;
                const int y = yOffset * offset;

                fheroes2::DrawLine( surface, { first.x + x, first.y + y }, { second.x + x, second.y + y }, color, roi );
            }

            for ( uint32_t thickness = lightning[i].second.thickness; thickness < lightning[i].first.thickness; ++thickness ) {
                const bool isUpper = ( ( thickness % 2 ) == 1 );
                const int offset = isUpper ? ( thickness + 1 ) / 2 : -static_cast<int>( ( thickness + 1 ) / 2 );

                fheroes2::DrawLine( surface, { first.x + xOffset * offset, first.y + yOffset * offset }, second, color, roi );
            }
        }
    }

    void GetHalfArc( std::vector<int32_t> & arc, int32_t width, const int32_t height, const int32_t pow1, const int32_t pow2, const double pow2Ratio )
    {
        // For positive width the arc will start from y = 0, for negative it will end at y = 0.
        const int32_t x0 = ( width < 0 ) ? width : 0;

        // For positive height the arc top will be y = 0, for negative - arc will be flipped for 'y' coordinate.
        // TODO: consider height offset from previous arc part.
        const int32_t y0 = ( height < 0 ) ? height : 0;

        // Coefficients for multipliers of 'pow1' and 'pow2' degrees.
        const double k1 = ( height * ( 1 - pow2Ratio ) ) / std::pow( width, pow1 );
        const double k2 = ( height * pow2Ratio ) / std::pow( width, pow2 );

        width = std::abs( width );
        // Calculate 'y' coordinates for the arc: y = k1*(x-x0)^pow1+k2*(x-x0)^pow2 and push it to the 'arc' vector.
        for ( int32_t x = 0; x < width; ++x ) {
            arc.push_back( static_cast<int32_t>( k1 * std::pow( ( x + x0 ), pow1 ) + k2 * std::pow( ( x + x0 ), pow2 ) - y0 ) );
        }
    }

    fheroes2::Image DrawRainbow( const std::vector<int32_t> & rainbowArc, const int32_t rainbowThickness, const bool isVertical, const bool flipHorizontally )
    {
        // Rainbow image size should include the arc size plus the thickness of the rainbow.
        const int32_t rainbowWidth = static_cast<int32_t>( rainbowArc.size() );
        const int32_t rainbowHeight = *std::max_element( rainbowArc.begin(), rainbowArc.end() ) + rainbowThickness;

        // If the rainbow is vertical - swap width and height.
        const int32_t rainbowImgWidth = isVertical ? rainbowHeight : rainbowWidth;
        const int32_t rainbowImgHeight = isVertical ? rainbowWidth : rainbowHeight;
        fheroes2::Image rainbow( rainbowImgWidth, rainbowImgHeight );
        rainbow.reset();
        std::vector<int32_t>::const_iterator pnt = rainbowArc.begin();

        // Get the original good luck sprite, since it has a rainbow image which will be used to get line.
        const fheroes2::Sprite & luckSprite = fheroes2::AGG::GetICN( ICN::EXPMRL, 0 );

        // Get a single rainbow line from the center of the luckSprite.
        fheroes2::Image croppedRainbow( 1, rainbowThickness );
        fheroes2::Copy( luckSprite, luckSprite.width() / 2, 0, croppedRainbow, 0, 0, 1, rainbowThickness );
        fheroes2::Image rainbowLine;
        if ( isVertical ) {
            rainbowLine = fheroes2::Image( croppedRainbow.height(), croppedRainbow.width() );
            // For a vertical rainbow orientation the line needs to be transposed.
            fheroes2::Transpose( croppedRainbow, rainbowLine );
            if ( !flipHorizontally ) {
                rainbowLine = fheroes2::Flip( rainbowLine, true, false );
            }
        }
        else {
            rainbowLine = std::move( croppedRainbow );
        }

        // Draw a rainbow image for each 'x' coordinate and corresponding '*pnt' value.
        for ( int32_t x = 0; pnt != rainbowArc.end(); ++x, ++pnt ) {
            // Set the 'x' and 'y' coordinates of the current rainbow pixel in the resulting rainbow image according to the rainbow direction.
            const int32_t imgX = isVertical ? ( flipHorizontally ? *pnt : rainbowImgWidth - *pnt - rainbowThickness ) : ( flipHorizontally ? rainbowImgWidth - x : x );
            const int32_t imgY = isVertical ? x : *pnt;

            // Insert a rainbow line at the current arc position.
            fheroes2::Copy( rainbowLine, 0, 0, rainbow, imgX, imgY, rainbowLine.width(), rainbowLine.height() );
        }
        return rainbow;
    }

    fheroes2::Point CalculateSpellPosition( const Battle::Unit & target, int spellICN, const fheroes2::Sprite & spellSprite )
    {
        const fheroes2::Rect & pos = target.GetRectPosition();

        // Get the sprite for the first frame, so its center won't shift if the creature is animating (instead of target.GetFrame()).
        const fheroes2::Sprite & unitSprite = fheroes2::AGG::GetICN( target.GetMonsterSprite(), target.animation.firstFrame() );

        // Bottom-left corner (default) position with spell offset applied
        fheroes2::Point result( pos.x + spellSprite.x(), pos.y + pos.height + cellYOffset + spellSprite.y() );

        switch ( spellICN ) {
        case ICN::SHIELD:
            // in front of the unit
            result.x += target.isReflect() ? -pos.width / ( target.isWide() ? 2 : 1 ) : pos.width;
            result.y += unitSprite.y() / 2;
            break;
        case ICN::BLIND: {
            // unit's eyes
            const fheroes2::Point & offset = target.animation.getBlindOffset();

            // calculate OG Heroes2 unit position to apply offset to
            const int rearCenterX = ( target.isWide() && target.isReflect() ) ? pos.width * 3 / 4 : CELLW / 2;

            // Overwrite result with custom blind value
            result.x += rearCenterX + ( target.isReflect() ? -offset.x : offset.x );
            result.y += offset.y;
            break;
        }
        case ICN::STONSKIN:
        case ICN::STELSKIN:
            // bottom center point
            result.x += pos.width / 2;
            break;
        case ICN::REDDEATH:
            // Shift spell sprite position for a wide creature to its head.
            result.x += pos.width / 2 + ( target.isReflect() ? ( 1 - spellSprite.width() - 2 * spellSprite.x() - pos.width / 8 ) : ( pos.width / 8 ) );
            result.y -= pos.height - 4;
            break;
        case ICN::MAGIC08:
            // Position shifts for the Holy Shout spell to be closer to OG.
            result.x += pos.width / 2 + ( target.isReflect() ? 12 : 0 );
            result.y += unitSprite.y() / 2 - 1;
            break;
        default:
            // center point of the unit
            result.x += pos.width / 2;
            result.y += unitSprite.y() / 2;
            break;
        }

        if ( result.y < 0 ) {
            const int maximumY = fheroes2::AGG::GetAbsoluteICNHeight( spellICN );
            result.y = maximumY + spellSprite.y();
        }

        return result;
    }
}

namespace Battle
{
    int GetIndexIndicator( const Unit & );
    int GetSwordCursorDirection( int );
    int GetDirectionFromCursorSword( uint32_t sword );
    int GetCursorFromSpell( int );

    class StatusListBox : public ::Interface::ListBox<std::string>
    {
    public:
        using ::Interface::ListBox<std::string>::ActionListDoubleClick;
        using ::Interface::ListBox<std::string>::ActionListSingleClick;
        using ::Interface::ListBox<std::string>::ActionListPressRight;

        StatusListBox()
            : openlog( false )
        {}

        void SetPosition( uint32_t px, uint32_t py )
        {
            const int32_t mx = 6;
            const int32_t sw = fheroes2::Display::DEFAULT_WIDTH;
            const int32_t sh = mx * 20;
            border.SetPosition( px, py - sh - 2, sw - 32, sh - 30 );
            const fheroes2::Rect & area = border.GetArea();
            const int32_t ax = area.x + area.width - 20;

            SetTopLeft( area.getPosition() );
            SetAreaMaxItems( mx );

            SetScrollButtonUp( ICN::DROPLISL, 6, 7, fheroes2::Point( ax + 8, area.y - 10 ) );
            SetScrollButtonDn( ICN::DROPLISL, 8, 9, fheroes2::Point( ax + 8, area.y + area.height - 11 ) );

            const int32_t scrollbarSliderAreaLength = buttonPgDn.area().y - ( buttonPgUp.area().y + buttonPgUp.area().height ) - 7;

            setScrollBarArea( { ax + 5 + 8, buttonPgUp.area().y + buttonPgUp.area().height + 3, 12, scrollbarSliderAreaLength } );

            const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::DROPLISL, 13 );
            const fheroes2::Image scrollbarSlider
                = fheroes2::generateScrollbarSlider( originalSlider, false, scrollbarSliderAreaLength, VisibleItemCount(), static_cast<int32_t>( messages.size() ),
                                                     { 0, 0, originalSlider.width(), 4 }, { 0, 4, originalSlider.width(), 8 } );

            setScrollBarImage( scrollbarSlider );
            _scrollbar.hide();
            SetAreaItems( { area.x, area.y, area.width - 10, area.height } );
            SetListContent( messages );
        }

        const fheroes2::Rect & GetArea() const
        {
            return border.GetRect();
        }

        void AddMessage( std::string str )
        {
            messages.push_back( std::move( str ) );
            if ( !openlog ) {
                _scrollbar.hide();
            }

            SetListContent( messages );
            SetCurrent( messages.size() - 1 );

            const int32_t scrollbarSliderAreaLength = buttonPgDn.area().y - ( buttonPgUp.area().y + buttonPgUp.area().height ) - 7;
            const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::DROPLISL, 13 );
            const fheroes2::Image scrollbarSlider
                = fheroes2::generateScrollbarSlider( originalSlider, false, scrollbarSliderAreaLength, VisibleItemCount(), static_cast<int32_t>( messages.size() ),
                                                     { 0, 0, originalSlider.width(), 4 }, { 0, 4, originalSlider.width(), 8 } );
            setScrollBarImage( scrollbarSlider );
            SetCurrentVisible();
        }

        void RedrawItem( const std::string & str, int32_t px, int32_t py, bool ) override
        {
            const Text text( str, Font::BIG );
            text.Blit( px, py );
        }

        void RedrawBackground( const fheroes2::Point & pt ) override
        {
            (void)pt;

            fheroes2::Display & display = fheroes2::Display::instance();
            const fheroes2::Sprite & sp1 = fheroes2::AGG::GetICN( ICN::DROPLISL, 10 );
            const fheroes2::Sprite & sp2 = fheroes2::AGG::GetICN( ICN::DROPLISL, 12 );
            const fheroes2::Sprite & sp3 = fheroes2::AGG::GetICN( ICN::DROPLISL, 11 );
            const int32_t ax = buttonPgUp.area().x;
            const int32_t ah = buttonPgDn.area().y - ( buttonPgUp.area().y + buttonPgUp.area().height );

            const fheroes2::Rect & borderRect = border.GetRect();
            Dialog::FrameBorder::RenderOther( fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ), borderRect );

            for ( int32_t i = 0; i < ( ah / sp3.height() ); ++i )
                fheroes2::Blit( sp3, display, ax, buttonPgUp.area().y + buttonPgUp.area().height + ( sp3.height() * i ) );

            fheroes2::Blit( sp1, display, ax, buttonPgUp.area().y + buttonPgUp.area().height );
            fheroes2::Blit( sp2, display, ax, buttonPgDn.area().y - sp2.height() );
        }

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( std::string & ) override
        {
            // Do nothing.
        }

        void ActionListSingleClick( std::string & ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( std::string & ) override
        {
            // Do nothing.
        }

        void SetOpenLog( const bool f )
        {
            openlog = f;
        }

        bool isOpenLog() const
        {
            return openlog;
        }

    private:
        Dialog::FrameBorder border;
        std::vector<std::string> messages;
        bool openlog;
    };

    int matchHeroType( const HeroBase * base )
    {
        if ( base->isCaptain() )
            return CAPTAIN;

        switch ( base->GetRace() ) {
        case Race::BARB:
            return BARBARIAN;
        case Race::SORC:
            return SORCERESS;
        case Race::WRLK:
            return WARLOCK;
        case Race::WZRD:
            return WIZARD;
        case Race::NECR:
            return NECROMANCER;
        default:
            break;
        }
        return KNIGHT;
    }

    const std::vector<int> & getHeroAnimation( const HeroBase * hero, int animation )
    {
        static std::vector<int> staticAnim;
        if ( staticAnim.empty() ) {
            staticAnim.push_back( 1 );
        }

        if ( !hero || animation == OP_STATIC )
            return staticAnim;

        const int heroType = matchHeroType( hero );

        if ( animation == OP_SORROW ) {
            static const std::vector<int> sorrowAnim = { 2, 3, 4, 5, 4, 5, 4, 3, 2 };

            return ( heroType == CAPTAIN ) ? staticAnim : sorrowAnim;
        }

        static std::vector<int> heroTypeAnim[7][9];

        if ( heroTypeAnim[heroType][animation].empty() ) {
            const int sourceArray[7][9][9] = {
                //   JOY                CAST_MASS             CAST_UP               CAST_DOWN     IDLE
                { { 6, 7, 8, 9, 8, 9, 8, 7, 6 }, { 10, 11 }, { 10 }, { 6, 12, 13 }, { 12, 6 }, { 2, 14 }, { 2 }, { 15, 16, 17 }, { 18, 19 } }, // KNIGHT
                { { 6, 7, 8, 9, 9, 8, 7, 6 }, { 6, 10, 11 }, { 10, 6 }, { 6, 12, 13 }, { 12, 6 }, { 6, 14 }, { 6 }, { 15, 16, 17 }, { 18 } }, // BARBARIAN
                { { 6, 7, 8, 7, 6 }, { 6, 7, 9 }, { 7, 6 }, { 6, 10, 11 }, { 10, 6 }, { 6, 12 }, { 6 }, { 13, 14, 15 }, { 16 } }, // SORCERESS
                { { 6, 7, 8, 9, 10, 9, 8, 7, 6 }, { 6, 7, 11, 12 }, { 11, 6 }, { 6, 7, 13 }, { 6 }, { 6, 14 }, { 6 }, { 15, 16 }, { 6 } }, // WARLOCK
                { { 6, 7, 8, 9, 8, 7, 6 }, { 6, 10, 11, 12, 13 }, { 12, 11, 10, 6 }, { 6, 14 }, { 6 }, { 6, 15 }, { 6 }, { 16, 17 }, { 18 } }, // WIZARD
                { { 6, 7, 6, 7, 6, 7 },
                  { 7, 8, 9, 10, 11 },
                  { 10, 9, 7 },
                  { 7, 12, 13, 14, 15 },
                  { 7 },
                  { 7, 12, 13, 14, 16 },
                  { 7 },
                  { 17 },
                  { 18, 19 } }, // NECROMANCER
                { { 1 }, { 2, 3, 4 }, { 3, 2 }, { 5, 6 }, { 5 }, { 5, 7 }, { 5 }, { 8, 9 }, { 10 } } // CAPTAIN
            };

            for ( int frame = 0; frame < 9; ++frame ) {
                if ( sourceArray[heroType][animation][frame] != 0 ) {
                    heroTypeAnim[heroType][animation].push_back( sourceArray[heroType][animation][frame] );
                }
            }
        }

        return heroTypeAnim[heroType][animation];
    }
}

bool CursorAttack( uint32_t theme )
{
    switch ( theme ) {
    case Cursor::WAR_ARROW:
    case Cursor::WAR_BROKENARROW:
    case Cursor::SWORD_TOPRIGHT:
    case Cursor::SWORD_RIGHT:
    case Cursor::SWORD_BOTTOMRIGHT:
    case Cursor::SWORD_BOTTOMLEFT:
    case Cursor::SWORD_LEFT:
    case Cursor::SWORD_TOPLEFT:
    case Cursor::SWORD_TOP:
    case Cursor::SWORD_BOTTOM:
        return true;
    default:
        break;
    }

    return false;
}

fheroes2::Image DrawHexagon( const uint8_t colorId )
{
    const int r = 22;
    const int l = 10;
    const int w = CELLW;
    const int h = CELLH;

    fheroes2::Image sf( w + 1, h + 1 );
    sf.reset();

    fheroes2::DrawLine( sf, { r - 1, 1 }, { 0, l + 1 }, colorId );
    fheroes2::SetPixel( sf, r, 1, colorId );
    fheroes2::DrawLine( sf, { r + 1, 1 }, { w, l + 1 }, colorId );

    fheroes2::DrawLine( sf, { 0, l + 1 }, { 0, h - l }, colorId );
    fheroes2::DrawLine( sf, { w, l + 1 }, { w, h - l }, colorId );

    fheroes2::DrawLine( sf, { r - 1, h }, { 0, h - l }, colorId );
    fheroes2::SetPixel( sf, r, h, colorId );
    fheroes2::DrawLine( sf, { r + 1, h }, { w, h - l }, colorId );

    return sf;
}

fheroes2::Image DrawHexagonShadow( const uint8_t alphaValue, const int32_t horizSpace )
{
    const int l = 13;
    const int w = CELLW;
    const int h = CELLH;

    fheroes2::Image sf( w, h );
    sf.reset();
    fheroes2::Rect rt( horizSpace, l - 1, w + 1 - horizSpace * 2, 2 * l + 4 );
    for ( int i = 0; i < w / 2; i += 2 ) {
        for ( int x = 0; x < rt.width; ++x ) {
            for ( int y = 0; y < rt.height; ++y ) {
                fheroes2::SetTransformPixel( sf, rt.x + x, rt.y + y, alphaValue );
            }
        }
        --rt.y;
        rt.height += 2;
        rt.x += ( i == 0 ) ? 1 : 2;
        rt.width -= ( i == 0 ) ? 2 : 4;
    }

    return sf;
}

bool Battle::TargetInfo::isFinishAnimFrame( const TargetInfo & info )
{
    return info.defender && info.defender->isFinishAnimFrame();
}

int Battle::GetCursorFromSpell( int spell )
{
    switch ( spell ) {
    case Spell::SLOW:
        return Cursor::SP_SLOW;
    case Spell::CURSE:
        return Cursor::SP_CURSE;
    case Spell::CURE:
        return Cursor::SP_CURE;
    case Spell::BLESS:
        return Cursor::SP_BLESS;
    case Spell::FIREBALL:
        return Cursor::SP_FIREBALL;
    case Spell::FIREBLAST:
        return Cursor::SP_FIREBLAST;
    case Spell::TELEPORT:
        return Cursor::SP_TELEPORT;
    case Spell::RESURRECT:
        return Cursor::SP_RESURRECT;
    case Spell::HASTE:
        return Cursor::SP_HASTE;
    case Spell::SHIELD:
        return Cursor::SP_SHIELD;
    case Spell::ARMAGEDDON:
        return Cursor::SP_ARMAGEDDON;
    case Spell::ANTIMAGIC:
        return Cursor::SP_ANTIMAGIC;
    case Spell::BERSERKER:
        return Cursor::SP_BERSERKER;
    case Spell::PARALYZE:
        return Cursor::SP_PARALYZE;
    case Spell::BLIND:
        return Cursor::SP_BLIND;

    case Spell::LIGHTNINGBOLT:
        return Cursor::SP_LIGHTNINGBOLT;
    case Spell::CHAINLIGHTNING:
        return Cursor::SP_CHAINLIGHTNING;
    case Spell::ELEMENTALSTORM:
        return Cursor::SP_ELEMENTALSTORM;
    case Spell::RESURRECTTRUE:
        return Cursor::SP_RESURRECTTRUE;
    case Spell::DISPEL:
        return Cursor::SP_DISPEL;
    case Spell::HOLYWORD:
        return Cursor::SP_HOLYWORD;
    case Spell::HOLYSHOUT:
        return Cursor::SP_HOLYSHOUT;
    case Spell::METEORSHOWER:
        return Cursor::SP_METEORSHOWER;

    case Spell::ANIMATEDEAD:
        return Cursor::SP_ANIMATEDEAD;
    case Spell::MIRRORIMAGE:
        return Cursor::SP_MIRRORIMAGE;
    case Spell::BLOODLUST:
        return Cursor::SP_BLOODLUST;
    case Spell::DEATHRIPPLE:
        return Cursor::SP_DEATHRIPPLE;
    case Spell::DEATHWAVE:
        return Cursor::SP_DEATHWAVE;
    case Spell::STEELSKIN:
        return Cursor::SP_STEELSKIN;
    case Spell::STONESKIN:
        return Cursor::SP_STONESKIN;
    case Spell::DRAGONSLAYER:
        return Cursor::SP_DRAGONSLAYER;
    case Spell::EARTHQUAKE:
        return Cursor::SP_EARTHQUAKE;
    case Spell::DISRUPTINGRAY:
        return Cursor::SP_DISRUPTINGRAY;
    case Spell::COLDRING:
        return Cursor::SP_COLDRING;
    case Spell::COLDRAY:
        return Cursor::SP_COLDRAY;
    case Spell::HYPNOTIZE:
        return Cursor::SP_HYPNOTIZE;
    case Spell::ARROW:
        return Cursor::SP_ARROW;

    default:
        break;
    }
    return Cursor::WAR_NONE;
}

int Battle::GetSwordCursorDirection( int dir )
{
    switch ( dir ) {
    case BOTTOM_RIGHT:
        return Cursor::SWORD_TOPLEFT;
    case BOTTOM_LEFT:
        return Cursor::SWORD_TOPRIGHT;
    case RIGHT:
        return Cursor::SWORD_LEFT;
    case TOP_RIGHT:
        return Cursor::SWORD_BOTTOMLEFT;
    case TOP_LEFT:
        return Cursor::SWORD_BOTTOMRIGHT;
    case LEFT:
        return Cursor::SWORD_RIGHT;
    default:
        break;
    }
    return 0;
}

int Battle::GetDirectionFromCursorSword( uint32_t sword )
{
    switch ( sword ) {
    case Cursor::SWORD_TOPLEFT:
        return BOTTOM_RIGHT;
    case Cursor::SWORD_TOPRIGHT:
        return BOTTOM_LEFT;
    case Cursor::SWORD_LEFT:
        return RIGHT;
    case Cursor::SWORD_BOTTOMLEFT:
        return TOP_RIGHT;
    case Cursor::SWORD_BOTTOMRIGHT:
        return TOP_LEFT;
    case Cursor::SWORD_RIGHT:
        return LEFT;
    default:
        break;
    }

    return UNKNOWN;
}

Battle::OpponentSprite::OpponentSprite( const fheroes2::Rect & area, const HeroBase * b, bool r )
    : base( b )
    , _currentAnim( getHeroAnimation( b, OP_STATIC ) )
    , _animationType( OP_STATIC )
    , _idleTimer( 8000 )
    , _heroIcnId( ICN::UNKNOWN )
    , reflect( r )
    , _offset( area.x, area.y )
{
    const bool isCaptain = b->isCaptain();
    switch ( b->GetRace() ) {
    case Race::KNGT:
        _heroIcnId = isCaptain ? ICN::CMBTCAPK : ICN::CMBTHROK;
        break;
    case Race::BARB:
        _heroIcnId = isCaptain ? ICN::CMBTCAPB : ICN::CMBTHROB;
        break;
    case Race::SORC:
        _heroIcnId = isCaptain ? ICN::CMBTCAPS : ICN::CMBTHROS;
        break;
    case Race::WRLK:
        _heroIcnId = isCaptain ? ICN::CMBTCAPW : ICN::CMBTHROW;
        break;
    case Race::WZRD:
        _heroIcnId = isCaptain ? ICN::CMBTCAPZ : ICN::CMBTHROZ;
        break;
    case Race::NECR:
        _heroIcnId = isCaptain ? ICN::CMBTCAPN : ICN::CMBTHRON;
        break;
    default:
        // Did you add a new faction? Add the logic here.
        assert( 0 );
        break;
    }

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( _heroIcnId, _currentAnim.getFrame() );

    if ( reflect ) {
        pos.x = _offset.x + fheroes2::Display::DEFAULT_WIDTH - RIGHT_HERO_X_OFFSET - ( sprite.x() + sprite.width() );
        pos.y = _offset.y + RIGHT_HERO_Y_OFFSET + sprite.y();
    }
    else {
        pos.x = _offset.x + LEFT_HERO_X_OFFSET + sprite.x();
        pos.y = _offset.y + LEFT_HERO_Y_OFFSET + sprite.y();
    }

    if ( isCaptain ) {
        if ( reflect )
            pos.x += CAPTAIN_X_OFFSET;
        else
            pos.x -= CAPTAIN_X_OFFSET;
        pos.y += CAPTAIN_Y_OFFSET;
    }

    pos.width = sprite.width();
    pos.height = sprite.height();
}

void Battle::OpponentSprite::IncreaseAnimFrame( bool loop )
{
    _currentAnim.playAnimation( loop );
}

void Battle::OpponentSprite::SetAnimation( int rule )
{
    _animationType = rule;
    _currentAnim = getHeroAnimation( base, rule );
}

fheroes2::Point Battle::OpponentSprite::GetCastPosition() const
{
    const bool isCaptain = base->isCaptain();
    fheroes2::Point offset;
    switch ( base->GetRace() ) {
    case Race::KNGT:
        offset.x = isCaptain ? 0 : 13;
        offset.y = isCaptain ? 3 : -7;
        break;
    case Race::BARB:
        offset.x = isCaptain ? 0 : 16;
        offset.y = isCaptain ? 3 : -15;
        break;
    case Race::SORC:
        offset.x = isCaptain ? 0 : 11;
        offset.y = isCaptain ? 3 : -8;
        break;
    case Race::WRLK:
        offset.x = isCaptain ? 2 : 9;
        offset.y = isCaptain ? 5 : -11;
        break;
    case Race::WZRD:
        offset.x = isCaptain ? 5 : 1;
        offset.y = isCaptain ? 8 : -9;
        break;
    case Race::NECR:
        offset.x = isCaptain ? 5 : 13;
        offset.y = isCaptain ? 6 : -7;
        break;
    default:
        break;
    }

    return { pos.x + ( reflect ? offset.x : pos.width - offset.x ), pos.y + pos.height / 2 + offset.y };
}

void Battle::OpponentSprite::Redraw( fheroes2::Image & dst ) const
{
    const fheroes2::Sprite & hero = fheroes2::AGG::GetICN( _heroIcnId, _currentAnim.getFrame() );

    fheroes2::Point offset( _offset );
    if ( base->isCaptain() ) {
        if ( reflect )
            offset.x += CAPTAIN_X_OFFSET;
        else
            offset.x -= CAPTAIN_X_OFFSET;
        offset.y += CAPTAIN_Y_OFFSET;
    }

    if ( reflect )
        fheroes2::Blit( hero, dst, offset.x + fheroes2::Display::DEFAULT_WIDTH - RIGHT_HERO_X_OFFSET - ( hero.x() + hero.width() ),
                        offset.y + RIGHT_HERO_Y_OFFSET + hero.y(), reflect );
    else
        fheroes2::Blit( hero, dst, offset.x + LEFT_HERO_X_OFFSET + hero.x(), offset.y + LEFT_HERO_Y_OFFSET + hero.y() );
}

bool Battle::OpponentSprite::updateAnimationState()
{
    if ( _currentAnim.isLastFrame() ) {
        if ( _animationType != OP_STATIC ) {
            if ( _animationType != OP_CAST_MASS && _animationType != OP_CAST_UP && _animationType != OP_CAST_DOWN ) {
                SetAnimation( OP_STATIC );
                return true;
            }
        }
        else if ( _idleTimer.checkDelay() ) {
            SetAnimation( ( Rand::Get( 1, 3 ) < 2 ) ? OP_IDLE2 : OP_IDLE );
            return true;
        }

        return false;
    }

    IncreaseAnimFrame();
    return true;
}

Battle::Status::Status()
    : back1( fheroes2::AGG::GetICN( ICN::TEXTBAR, 8 ) )
    , back2( fheroes2::AGG::GetICN( ICN::TEXTBAR, 9 ) )
    , listlog( nullptr )
{
    width = back1.width();
    height = back1.height() + back2.height();

    bar1.Set( Font::BIG );
    bar2.Set( Font::BIG );
}

void Battle::Status::SetPosition( int32_t cx, int32_t cy )
{
    fheroes2::Rect::x = cx;
    fheroes2::Rect::y = cy;
}

void Battle::Status::SetMessage( const std::string & str, bool top )
{
    if ( top ) {
        bar1.Set( str );
        if ( listlog )
            listlog->AddMessage( str );
    }
    else if ( str != message ) {
        bar2.Set( str );
        message = str;
    }
}

void Battle::Status::Redraw( fheroes2::Image & output ) const
{
    fheroes2::Blit( back1, output, x, y );
    fheroes2::Blit( back2, output, x, y + back1.height() );

    if ( bar1.Size() )
        bar1.Blit( x + ( back1.width() - bar1.w() ) / 2, y + 2 );
    if ( bar2.Size() )
        bar2.Blit( x + ( back2.width() - bar2.w() ) / 2, y + back1.height() - 2 );
}

void Battle::Status::clear()
{
    bar1.Clear();
    bar2.Clear();
}

Battle::ArmiesOrder::ArmiesOrder()
    : _army2Color( 0 )
{
    // Do nothing.
}

void Battle::ArmiesOrder::Set( const fheroes2::Rect & rt, const std::shared_ptr<const Units> & units, const int army2Color )
{
    _area = rt;
    _orders = units;
    _army2Color = army2Color;

    if ( units ) {
        _rects.reserve( units->size() );
    }
}

void Battle::ArmiesOrder::QueueEventProcessing( std::string & msg, const fheroes2::Point & offset ) const
{
    LocalEvent & le = LocalEvent::Get();

    for ( const UnitPos & unitPos : _rects ) {
        assert( unitPos.first != nullptr );

        const fheroes2::Rect unitRoi = unitPos.second + offset;
        if ( le.MouseCursor( unitRoi ) ) {
            msg = _( "View %{monster} info" );
            StringReplaceWithLowercase( msg, "%{monster}", unitPos.first->GetName() );
        }

        const Unit & unit = *( unitPos.first );

        if ( le.MouseClickLeft( unitRoi ) ) {
            Dialog::ArmyInfo( unit, Dialog::BUTTONS, unit.isReflect() );
        }
        else if ( le.MousePressRight( unitRoi ) ) {
            Dialog::ArmyInfo( unit, Dialog::ZERO, unit.isReflect() );
        }
    }
}

void Battle::ArmiesOrder::RedrawUnit( const fheroes2::Rect & pos, const Battle::Unit & unit, const bool revert, const bool isCurrentUnit, const uint8_t currentUnitColor,
                                      fheroes2::Image & output ) const
{
    // Render background.
    const fheroes2::Sprite & backgroundOriginal = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );
    fheroes2::Copy( backgroundOriginal, 37, 268, output, pos.x + 1, pos.y + 1, armyOrderMonsterIconSize - 2, armyOrderMonsterIconSize - 2 );

    // Draw a monster's sprite.
    const fheroes2::Sprite & mons32 = fheroes2::AGG::GetICN( ICN::MONS32, unit.GetSpriteIndex() );
    fheroes2::Blit( mons32, output, pos.x + ( pos.width - mons32.width() ) / 2, pos.y + pos.height - mons32.height() - ( mons32.height() + 3 < pos.height ? 3 : 0 ),
                    revert );

    Text number( fheroes2::abbreviateNumber( unit.GetCount() ), Font::SMALL );
    number.Blit( pos.x + 2, pos.y + 2, output );

    if ( isCurrentUnit ) {
        fheroes2::DrawRect( output, { pos.x, pos.y, armyOrderMonsterIconSize, armyOrderMonsterIconSize }, currentUnitColor );
    }
    else {
        uint8_t color = 0;

        switch ( unit.GetCurrentColor() ) {
        case -1: // Berserkers
            color = ARMY_COLOR_BLACK;
            break;
        case Color::BLUE:
            color = ARMY_COLOR_BLUE;
            break;
        case Color::GREEN:
            color = ARMY_COLOR_GREEN;
            break;
        case Color::RED:
            color = ARMY_COLOR_RED;
            break;
        case Color::YELLOW:
            color = ARMY_COLOR_YELLOW;
            break;
        case Color::ORANGE:
            color = ARMY_COLOR_ORANGE;
            break;
        case Color::PURPLE:
            color = ARMY_COLOR_PURPLE;
            break;
        case Color::NONE:
            color = ARMY_COLOR_GRAY;
            break;
        default:
            assert( 0 ); // Did you add another player color?
            break;
        }

        fheroes2::DrawRect( output, { pos.x, pos.y, armyOrderMonsterIconSize, armyOrderMonsterIconSize }, color );

        if ( unit.Modes( Battle::TR_MOVED ) ) {
            fheroes2::ApplyPalette( output, pos.x, pos.y, output, pos.x, pos.y, armyOrderMonsterIconSize, armyOrderMonsterIconSize,
                                    PAL::GetPalette( PAL::PaletteType::GRAY ) );
            fheroes2::ApplyPalette( output, pos.x, pos.y, output, pos.x, pos.y, armyOrderMonsterIconSize, armyOrderMonsterIconSize, 3 );
        }
    }
}

void Battle::ArmiesOrder::Redraw( const Unit * current, const uint8_t currentUnitColor, fheroes2::Image & output )
{
    if ( _orders.expired() ) {
        // Nothing to show.
        return;
    }

    const std::shared_ptr<const Units> orders = _orders.lock();

    const int32_t validUnitCount = static_cast<int32_t>( std::count_if( orders->begin(), orders->end(), []( const Unit * unit ) {
        assert( unit != nullptr );
        return unit->isValid();
    } ) );

    const int32_t maximumUnitsToDraw = _area.width / armyOrderMonsterIconSize;

    int32_t offsetX = _area.x;

    if ( validUnitCount > maximumUnitsToDraw ) {
        offsetX += ( _area.width - armyOrderMonsterIconSize * maximumUnitsToDraw ) / 2;
    }
    else {
        offsetX += ( _area.width - armyOrderMonsterIconSize * validUnitCount ) / 2;
    }

    fheroes2::Rect::x = offsetX;
    fheroes2::Rect::y = _area.y;
    fheroes2::Rect::height = armyOrderMonsterIconSize;

    _rects.clear();

    int32_t unitsDrawn = 0;
    int32_t unitsProcessed = 0;

    for ( const Unit * unit : *orders ) {
        if ( unitsDrawn == maximumUnitsToDraw ) {
            break;
        }

        assert( unit != nullptr );
        if ( !unit->isValid() ) {
            continue;
        }

        if ( unit->Modes( Battle::TR_MOVED ) && ( validUnitCount - unitsProcessed > maximumUnitsToDraw ) ) {
            ++unitsProcessed;
            continue;
        }

        _rects.emplace_back( unit, fheroes2::Rect( offsetX, _area.y, armyOrderMonsterIconSize, armyOrderMonsterIconSize ) );
        RedrawUnit( _rects.back().second, *unit, unit->GetColor() == _army2Color, current == unit, currentUnitColor, output );
        offsetX += armyOrderMonsterIconSize;
        fheroes2::Rect::width += armyOrderMonsterIconSize;

        ++unitsDrawn;
        ++unitsProcessed;
    }
}

Battle::Interface::Interface( Arena & battleArena, const int32_t tileIndex )
    : arena( battleArena )
    , _surfaceInnerArea( 0, 0, fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT )
    , _mainSurface( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT )
    , icn_cbkg( ICN::UNKNOWN )
    , icn_frng( ICN::UNKNOWN )
    , humanturn_spell( Spell::NONE )
    , humanturn_exit( true )
    , humanturn_redraw( true )
    , animation_flags_frame( 0 )
    , catapult_frame( 0 )
    , _interruptAutoBattleForColor( 0 )
    , _contourColor( 110 )
    , _brightLandType( false )
    , _contourCycle( 0 )
    , _currentUnit( nullptr )
    , _movingUnit( nullptr )
    , _flyingUnit( nullptr )
    , b_current_sprite( nullptr )
    , index_pos( -1 )
    , teleport_src( -1 )
    , listlog( nullptr )
    , _cursorRestorer( true, Cursor::WAR_POINTER )
    , _bridgeAnimation( { false, BridgeMovementAnimation::UP_POSITION } )
{
    const Settings & conf = Settings::Get();

    // border
    const fheroes2::Display & display = fheroes2::Display::instance();

    _interfacePosition = { ( display.width() - fheroes2::Display::DEFAULT_WIDTH ) / 2, ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2,
                           _surfaceInnerArea.width, _surfaceInnerArea.height };
    border.SetPosition( _interfacePosition.x - BORDERWIDTH, _interfacePosition.y - BORDERWIDTH, fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );

    // damage info popup
    popup.setBattleUIRect( _interfacePosition );

    // cover
    const bool trees = !Maps::ScanAroundObject( tileIndex, MP2::OBJ_TREES ).empty();
    const Maps::Tiles & tile = world.GetTiles( tileIndex );

    const int groundType = tile.GetGround();
    _brightLandType
        = ( groundType == Maps::Ground::SNOW || groundType == Maps::Ground::DESERT || groundType == Maps::Ground::WASTELAND || groundType == Maps::Ground::BEACH );
    if ( _brightLandType ) {
        _contourColor = 108;
    }

    switch ( groundType ) {
    case Maps::Ground::DESERT:
        icn_cbkg = ICN::CBKGDSRT;
        icn_frng = ICN::FRNG0004;
        break;
    case Maps::Ground::SNOW:
        icn_cbkg = trees ? ICN::CBKGSNTR : ICN::CBKGSNMT;
        icn_frng = trees ? ICN::FRNG0006 : ICN::FRNG0007;
        break;
    case Maps::Ground::SWAMP:
        icn_cbkg = ICN::CBKGSWMP;
        icn_frng = ICN::FRNG0008;
        break;
    case Maps::Ground::WASTELAND:
        icn_cbkg = ICN::CBKGCRCK;
        icn_frng = ICN::FRNG0003;
        break;
    case Maps::Ground::BEACH:
        icn_cbkg = ICN::CBKGBEAC;
        icn_frng = ICN::FRNG0002;
        break;
    case Maps::Ground::LAVA:
        icn_cbkg = ICN::CBKGLAVA;
        icn_frng = ICN::FRNG0005;
        break;
    case Maps::Ground::DIRT:
        icn_cbkg = trees ? ICN::CBKGDITR : ICN::CBKGDIMT;
        icn_frng = trees ? ICN::FRNG0010 : ICN::FRNG0009;
        break;
    case Maps::Ground::GRASS:
        icn_cbkg = trees ? ICN::CBKGGRTR : ICN::CBKGGRMT;
        icn_frng = trees ? ICN::FRNG0011 : ICN::FRNG0012;
        break;
    case Maps::Ground::WATER:
        icn_cbkg = ICN::CBKGWATR;
        icn_frng = ICN::FRNG0013;
        break;
    default:
        break;
    }

    // hexagon
    _hexagonGrid = DrawHexagon( fheroes2::GetColorId( 0x68, 0x8C, 0x04 ) );
    // Shadow under the cursor: the first parameter is the shadow strength (smaller is stronger), the second is the distance between the hexagonal shadows.
    _hexagonCursorShadow = DrawHexagonShadow( 2, 1 );
    // Hexagon shadow for the case when grid is disabled.
    _hexagonShadow = DrawHexagonShadow( 4, 2 );
    // Shadow that fits the hexagon grid.
    _hexagonGridShadow = DrawHexagonShadow( 4, 1 );

    btn_auto.setICNInfo( ICN::TEXTBAR, 4, 5 );
    btn_settings.setICNInfo( ICN::TEXTBAR, 6, 7 );

    // opponents
    opponent1 = arena.GetCommander1() ? new OpponentSprite( _surfaceInnerArea, arena.GetCommander1(), false ) : nullptr;
    opponent2 = arena.GetCommander2() ? new OpponentSprite( _surfaceInnerArea, arena.GetCommander2(), true ) : nullptr;

    if ( Arena::GetCastle() )
        main_tower = { 570, 145, 70, 160 };

    const fheroes2::Rect & area = border.GetArea();

    const fheroes2::Rect autoRect = btn_auto.area();
    const fheroes2::Rect settingsRect = btn_settings.area();
    btn_auto.setPosition( area.x, area.y + area.height - settingsRect.height - autoRect.height );
    btn_settings.setPosition( area.x, area.y + area.height - settingsRect.height );

    btn_skip.setICNInfo( ICN::TEXTBAR, 0, 1 );
    btn_skip.setPosition( area.x + area.width - btn_skip.area().width, area.y + area.height - btn_skip.area().height );

    status.SetPosition( area.x + settingsRect.width, btn_auto.area().y );

    listlog = std::make_unique<StatusListBox>();

    if ( listlog )
        listlog->SetPosition( area.x, area.y + area.height - status.height );
    status.SetLogs( listlog.get() );

    AudioManager::ResetAudio();

    // Don't waste time playing the pre-battle sound if the game sounds are turned off
    if ( conf.SoundVolume() > 0 ) {
        AudioManager::PlaySound( M82::PREBATTL );
    }
}

Battle::Interface::~Interface()
{
    AudioManager::ResetAudio();

    if ( opponent1 )
        delete opponent1;
    if ( opponent2 )
        delete opponent2;
}

void Battle::Interface::SetOrderOfUnits( const std::shared_ptr<const Units> & units )
{
    armies_order.Set( GetArea(), units, arena.GetArmy2Color() );
}

fheroes2::Point Battle::Interface::GetMouseCursor() const
{
    return LocalEvent::Get().GetMouseCursor() - _interfacePosition.getPosition();
}

void Battle::Interface::SetStatus( const std::string & msg, bool top )
{
    if ( top ) {
        status.SetMessage( msg, true );
        status.SetMessage( "", false );
    }
    else {
        status.SetMessage( msg );
    }
    humanturn_redraw = true;
}

void Battle::Interface::UpdateContourColor()
{
    ++_contourCycle;

    if ( _brightLandType ) {
        static const std::array<const uint8_t, 6> contourColorTable = { 108, 115, 122, 129, 122, 115 };
        _contourColor = contourColorTable[_contourCycle % sizeof( contourColorTable )];
    }
    else {
        static const std::array<const uint8_t, 8> contourColorTable = { 110, 114, 118, 122, 126, 122, 118, 114 };
        _contourColor = contourColorTable[_contourCycle % sizeof( contourColorTable )];
    }
}

void Battle::Interface::fullRedraw()
{
    if ( !_background ) {
        _background = std::make_unique<fheroes2::StandardWindow>( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT, false );
    }

    Redraw();
}

void Battle::Interface::Redraw()
{
    RedrawPartialStart();
    RedrawPartialFinish();
}

void Battle::Interface::RedrawPartialStart()
{
    RedrawCover();
    RedrawArmies();
}

void Battle::Interface::RedrawPartialFinish()
{
    fheroes2::Display & display = fheroes2::Display::instance();

    if ( Settings::Get().BattleShowArmyOrder() ) {
        armies_order.Redraw( _currentUnit, _contourColor, _mainSurface );
    }

#ifdef WITH_DEBUG
    if ( IS_DEVEL() ) {
        const Board * board = Arena::GetBoard();
        assert( board != nullptr );

        for ( const Cell & cell : *board ) {
            Text text( std::to_string( cell.GetIndex() ), Font::SMALL );
            text.Blit( cell.GetPos().x + 20, cell.GetPos().y + 22, _mainSurface );
        }
    }
#endif

    fheroes2::Blit( _mainSurface, display, _interfacePosition.x, _interfacePosition.y );
    RedrawInterface();

    display.render();
}

void Battle::Interface::RedrawInterface()
{
    status.Redraw( fheroes2::Display::instance() );

    btn_auto.draw();
    btn_settings.draw();
    btn_skip.draw();

    popup.Redraw();

    if ( listlog && listlog->isOpenLog() ) {
        listlog->Redraw();
    }
}

void Battle::Interface::RedrawArmies()
{
    const Castle * castle = Arena::GetCastle();

    const int32_t wallCellIds[ARENAH]
        = { Arena::CASTLE_FIRST_TOP_WALL_POS, Arena::CASTLE_TOP_ARCHER_TOWER_POS,  Arena::CASTLE_SECOND_TOP_WALL_POS, Arena::CASTLE_TOP_GATE_TOWER_POS,
            Arena::CASTLE_GATE_POS,           Arena::CASTLE_BOTTOM_GATE_TOWER_POS, Arena::CASTLE_THIRD_TOP_WALL_POS,  Arena::CASTLE_BOTTOM_ARCHER_TOWER_POS,
            Arena::CASTLE_FOURTH_TOP_WALL_POS };

    if ( castle == nullptr ) {
        RedrawKilled();
    }

    for ( int32_t cellRowId = 0; cellRowId < ARENAH; ++cellRowId ) {
        // Redraw objects.
        for ( int32_t cellColumnId = 0; cellColumnId < ARENAW; ++cellColumnId ) {
            const int32_t cellId = cellRowId * ARENAW + cellColumnId;
            RedrawHighObjects( cellId );
        }

        if ( castle != nullptr ) {
            // Redraw main tower.
            if ( cellRowId == 5 ) {
                RedrawCastleMainTower( *castle );
            }
            else if ( cellRowId == 7 ) { // Redraw catapult.
                RedrawCastle( *castle, Arena::CATAPULT_POS );
            }

            std::vector<const Unit *> deadTroopBeforeWall;
            std::vector<const Unit *> deadTroopAfterWall;

            std::vector<const Unit *> troopCounterBeforeWall;
            std::vector<const Unit *> troopCounterAfterWall;

            std::vector<const Unit *> troopBeforeWall;
            std::vector<const Unit *> troopAfterWall;

            std::vector<const Unit *> movingTroopBeforeWall;
            std::vector<const Unit *> movingTroopAfterWall;

            // Overlay sprites for troops (i.e. spell effect animation) should be rendered after rendering all troops
            // for current row so the next troop will not be rendered over the overlay sprite.
            std::vector<const UnitSpellEffectInfo *> troopOverlaySpriteBeforeWall;
            std::vector<const UnitSpellEffectInfo *> troopOverlaySpriteAfterWall;

            const int32_t wallCellId = wallCellIds[cellRowId];

            for ( int32_t cellColumnId = 0; cellColumnId < ARENAW; ++cellColumnId ) {
                const int32_t cellId = cellRowId * ARENAW + cellColumnId;

                bool isCellBefore = true;
                if ( cellRowId < 5 ) {
                    isCellBefore = cellId < wallCellId;
                }
                else {
                    isCellBefore = cellId > wallCellId;
                    if ( ( wallCellId == Arena::CASTLE_THIRD_TOP_WALL_POS || wallCellId == Arena::CASTLE_FOURTH_TOP_WALL_POS )
                         && Board::GetCell( wallCellId )->GetObject() == 0 ) {
                        isCellBefore = false;
                    }
                }

                const std::vector<const Unit *> & deadUnits = arena.GetGraveyardTroops( cellId );
                for ( size_t i = 0; i < deadUnits.size(); ++i ) {
                    if ( deadUnits[i] && cellId != deadUnits[i]->GetTailIndex() ) {
                        if ( isCellBefore ) {
                            deadTroopBeforeWall.emplace_back( deadUnits[i] );

                            // Check for overlay sprites of dead units (i.e. Resurrect spell).
                            for ( const Battle::UnitSpellEffectInfo & overlaySprite : _unitSpellEffectInfos ) {
                                if ( overlaySprite.unitId == deadUnits[i]->GetUID() ) {
                                    troopOverlaySpriteBeforeWall.emplace_back( &overlaySprite );
                                }
                            }
                        }
                        else {
                            deadTroopAfterWall.emplace_back( deadUnits[i] );

                            // Check for overlay sprites of dead units (i.e. Resurrect spell).
                            for ( const Battle::UnitSpellEffectInfo & overlaySprite : _unitSpellEffectInfos ) {
                                if ( overlaySprite.unitId == deadUnits[i]->GetUID() ) {
                                    troopOverlaySpriteAfterWall.emplace_back( &overlaySprite );
                                }
                            }
                        }
                    }
                }

                const Unit * unitOnCell = Board::GetCell( cellId )->GetUnit();
                if ( unitOnCell == nullptr || _flyingUnit == unitOnCell || cellId == unitOnCell->GetTailIndex() ) {
                    continue;
                }

                if ( _movingUnit != unitOnCell && unitOnCell->isValid() ) {
                    const int unitAnimState = unitOnCell->GetAnimationState();
                    const bool isStaticUnit = unitAnimState == Monster_Info::STATIC || unitAnimState == Monster_Info::IDLE;

                    if ( isCellBefore ) {
                        if ( isStaticUnit ) {
                            troopCounterBeforeWall.emplace_back( unitOnCell );
                        }
                        troopBeforeWall.emplace_back( unitOnCell );
                    }
                    else {
                        if ( isStaticUnit ) {
                            troopCounterAfterWall.emplace_back( unitOnCell );
                        }
                        troopAfterWall.emplace_back( unitOnCell );
                    }
                }
                else {
                    if ( isCellBefore ) {
                        movingTroopBeforeWall.emplace_back( unitOnCell );
                    }
                    else {
                        movingTroopAfterWall.emplace_back( unitOnCell );
                    }
                }

                // Check for overlay sprites for 'unitOnCell'.
                for ( const Battle::UnitSpellEffectInfo & overlaySprite : _unitSpellEffectInfos ) {
                    if ( overlaySprite.unitId == unitOnCell->GetUID() ) {
                        if ( isCellBefore ) {
                            troopOverlaySpriteBeforeWall.emplace_back( &overlaySprite );
                        }
                        else {
                            troopOverlaySpriteAfterWall.emplace_back( &overlaySprite );
                        }
                    }
                }
            }

            for ( size_t i = 0; i < deadTroopBeforeWall.size(); ++i ) {
                RedrawTroopSprite( *deadTroopBeforeWall[i] );
            }

            for ( size_t i = 0; i < troopBeforeWall.size(); ++i ) {
                RedrawTroopSprite( *troopBeforeWall[i] );
            }

            for ( size_t i = 0; i < troopCounterBeforeWall.size(); ++i ) {
                RedrawTroopCount( *troopCounterBeforeWall[i] );
            }

            for ( size_t i = 0; i < movingTroopBeforeWall.size(); ++i ) {
                RedrawTroopSprite( *movingTroopBeforeWall[i] );
            }

            // Render the overlay sprite for units in current cell row above all units in this and upper rows.
            for ( const Battle::UnitSpellEffectInfo * overlaySprite : troopOverlaySpriteBeforeWall ) {
                assert( overlaySprite->icnId != ICN::UNKNOWN );
                const fheroes2::Sprite & spellSprite = fheroes2::AGG::GetICN( overlaySprite->icnId, overlaySprite->icnIndex );
                fheroes2::Blit( spellSprite, _mainSurface, overlaySprite->position.x, overlaySprite->position.y, overlaySprite->isReflectedImage );
            }

            RedrawCastle( *castle, wallCellId );

            for ( size_t i = 0; i < deadTroopAfterWall.size(); ++i ) {
                RedrawTroopSprite( *deadTroopAfterWall[i] );
            }

            for ( size_t i = 0; i < troopAfterWall.size(); ++i ) {
                RedrawTroopSprite( *troopAfterWall[i] );
            }

            for ( size_t i = 0; i < troopCounterAfterWall.size(); ++i ) {
                RedrawTroopCount( *troopCounterAfterWall[i] );
            }

            for ( size_t i = 0; i < movingTroopAfterWall.size(); ++i ) {
                RedrawTroopSprite( *movingTroopAfterWall[i] );
            }

            // Render the overlay sprite for units in current cell row above all units in this and upper rows.
            for ( const Battle::UnitSpellEffectInfo * overlaySprite : troopOverlaySpriteAfterWall ) {
                assert( overlaySprite->icnId != ICN::UNKNOWN );
                const fheroes2::Sprite & spellSprite = fheroes2::AGG::GetICN( overlaySprite->icnId, overlaySprite->icnIndex );
                fheroes2::Blit( spellSprite, _mainSurface, overlaySprite->position.x, overlaySprite->position.y, overlaySprite->isReflectedImage );
            }
        }
        else {
            std::vector<const Unit *> troopCounter;
            std::vector<const Unit *> troop;
            std::vector<const Unit *> movingTroop;
            std::vector<const UnitSpellEffectInfo *> troopOverlaySprite;

            // Redraw monsters.
            for ( int32_t cellColumnId = 0; cellColumnId < ARENAW; ++cellColumnId ) {
                const int32_t cellId = cellRowId * ARENAW + cellColumnId;

                // Check for overlay sprites of dead units (i.e. Resurrect spell).
                for ( const Unit * deadUnit : arena.GetGraveyardTroops( cellId ) ) {
                    for ( const Battle::UnitSpellEffectInfo & overlaySprite : _unitSpellEffectInfos ) {
                        if ( overlaySprite.unitId == deadUnit->GetUID() ) {
                            troopOverlaySprite.emplace_back( &overlaySprite );
                        }
                    }
                }

                const Unit * unitOnCell = Board::GetCell( cellId )->GetUnit();
                if ( unitOnCell == nullptr || _flyingUnit == unitOnCell || cellId == unitOnCell->GetTailIndex() ) {
                    continue;
                }

                if ( _movingUnit != unitOnCell && unitOnCell->isValid() ) {
                    const int unitAnimState = unitOnCell->GetAnimationState();
                    const bool isStaticUnit = unitAnimState == Monster_Info::STATIC || unitAnimState == Monster_Info::IDLE;
                    if ( isStaticUnit ) {
                        troopCounter.emplace_back( unitOnCell );
                    }

                    troop.emplace_back( unitOnCell );
                }
                else {
                    movingTroop.emplace_back( unitOnCell );
                }

                // Check for overlay sprites for 'unitOnCell'.
                for ( const Battle::UnitSpellEffectInfo & overlaySprite : _unitSpellEffectInfos ) {
                    if ( overlaySprite.unitId == unitOnCell->GetUID() ) {
                        troopOverlaySprite.emplace_back( &overlaySprite );
                    }
                }
            }

            // Redraw monster counters.
            for ( size_t i = 0; i < troop.size(); ++i ) {
                RedrawTroopSprite( *troop[i] );
            }

            for ( size_t i = 0; i < troopCounter.size(); ++i ) {
                RedrawTroopCount( *troopCounter[i] );
            }

            for ( size_t i = 0; i < movingTroop.size(); ++i ) {
                RedrawTroopSprite( *movingTroop[i] );
            }

            // Render the overlay sprite for units in current cell row above all units in this and upper rows.
            for ( const Battle::UnitSpellEffectInfo * overlaySprite : troopOverlaySprite ) {
                assert( overlaySprite->icnId != ICN::UNKNOWN );
                const fheroes2::Sprite & spellSprite = fheroes2::AGG::GetICN( overlaySprite->icnId, overlaySprite->icnIndex );
                fheroes2::Blit( spellSprite, _mainSurface, overlaySprite->position.x, overlaySprite->position.y, overlaySprite->isReflectedImage );
            }
        }

        // Redraw heroes.
        if ( cellRowId == 2 ) {
            RedrawOpponents();
        }
    }

    if ( _flyingUnit ) {
        RedrawTroopSprite( *_flyingUnit );
    }

    // Continue the idle animation for all troops on the battlefield.
    IdleTroopsAnimation();
}

void Battle::Interface::RedrawOpponents()
{
    if ( opponent1 )
        opponent1->Redraw( _mainSurface );
    if ( opponent2 )
        opponent2->Redraw( _mainSurface );

    RedrawOpponentsFlags();
}

void Battle::Interface::RedrawOpponentsFlags()
{
    if ( opponent1 ) {
        int icn = ICN::UNKNOWN;

        switch ( arena.GetArmy1Color() ) {
        case Color::BLUE:
            icn = ICN::HEROFL00;
            break;
        case Color::GREEN:
            icn = ICN::HEROFL01;
            break;
        case Color::RED:
            icn = ICN::HEROFL02;
            break;
        case Color::YELLOW:
            icn = ICN::HEROFL03;
            break;
        case Color::ORANGE:
            icn = ICN::HEROFL04;
            break;
        case Color::PURPLE:
            icn = ICN::HEROFL05;
            break;
        default:
            icn = ICN::HEROFL06;
            break;
        }

        const fheroes2::Sprite & flag = fheroes2::AGG::GetICN( icn, ICN::AnimationFrame( icn, 0, animation_flags_frame ) );
        fheroes2::Blit( flag, _mainSurface, opponent1->Offset().x + OpponentSprite::LEFT_HERO_X_OFFSET + flag.x(),
                        opponent1->Offset().y + OpponentSprite::LEFT_HERO_Y_OFFSET + flag.y() );
    }

    if ( opponent2 ) {
        int icn = ICN::UNKNOWN;

        switch ( arena.GetForce2().GetColor() ) {
        case Color::BLUE:
            icn = ICN::HEROFL00;
            break;
        case Color::GREEN:
            icn = ICN::HEROFL01;
            break;
        case Color::RED:
            icn = ICN::HEROFL02;
            break;
        case Color::YELLOW:
            icn = ICN::HEROFL03;
            break;
        case Color::ORANGE:
            icn = ICN::HEROFL04;
            break;
        case Color::PURPLE:
            icn = ICN::HEROFL05;
            break;
        default:
            icn = ICN::HEROFL06;
            break;
        }

        const fheroes2::Sprite & flag = fheroes2::AGG::GetICN( icn, ICN::AnimationFrame( icn, 0, animation_flags_frame ) );
        const fheroes2::Point offset = opponent2->Offset();
        fheroes2::Blit( flag, _mainSurface, offset.x + fheroes2::Display::DEFAULT_WIDTH - OpponentSprite::RIGHT_HERO_X_OFFSET - ( flag.x() + flag.width() ),
                        offset.y + OpponentSprite::RIGHT_HERO_Y_OFFSET + flag.y(), true );
    }
}

fheroes2::Point GetTroopPosition( const Battle::Unit & unit, const fheroes2::Sprite & sprite )
{
    const fheroes2::Rect & rt = unit.GetRectPosition();

    int32_t offsetX = 0;
    if ( unit.isReflect() ) {
        if ( unit.isWide() ) {
            offsetX = rt.x + ( rt.width / 2 + rt.width / 4 ) - sprite.width() - sprite.x() + 1;
        }
        else {
            offsetX = rt.x + ( rt.width / 2 ) - sprite.width() - sprite.x() + 1;
        }
    }
    else {
        if ( unit.isWide() ) {
            offsetX = rt.x + ( rt.width / 4 ) + sprite.x();
        }
        else {
            offsetX = rt.x + ( rt.width / 2 ) + sprite.x();
        }
    }

    const int32_t offsetY = rt.y + rt.height + sprite.y() + cellYOffset;

    return { offsetX, offsetY };
}

void Battle::Interface::RedrawTroopSprite( const Unit & unit )
{
    if ( b_current_sprite && _currentUnit == &unit ) {
        drawTroopSprite( unit, *b_current_sprite );
    }
    else if ( unit.Modes( SP_STONE ) ) {
        // Current monster can't be active if it's under Stunning effect.
        const int monsterIcnId = unit.GetMonsterSprite();
        fheroes2::Sprite monsterSprite = fheroes2::AGG::GetICN( monsterIcnId, unit.GetFrame() );
        fheroes2::ApplyPalette( monsterSprite, PAL::GetPalette( PAL::PaletteType::GRAY ) );
        drawTroopSprite( unit, monsterSprite );
    }
    else if ( unit.Modes( CAP_MIRRORIMAGE ) ) {
        fheroes2::Sprite monsterSprite;

        if ( _currentUnit == &unit && b_current_sprite != nullptr ) {
            monsterSprite = *b_current_sprite;
        }
        else {
            const int monsterIcnId = unit.GetMonsterSprite();
            monsterSprite = fheroes2::AGG::GetICN( monsterIcnId, unit.GetFrame() );
        }

        fheroes2::ApplyPalette( monsterSprite, PAL::GetPalette( PAL::PaletteType::MIRROR_IMAGE ) );

        const fheroes2::Point drawnPosition = drawTroopSprite( unit, monsterSprite );

        if ( _currentUnit == &unit && b_current_sprite == nullptr ) {
            // Current unit's turn which is idling.
            const fheroes2::Sprite & monsterContour = fheroes2::CreateContour( monsterSprite, _contourColor );
            fheroes2::Blit( monsterContour, _mainSurface, drawnPosition.x, drawnPosition.y, unit.isReflect() );
        }
    }
    else {
        const int monsterIcnId = unit.GetMonsterSprite();
        const bool isCurrentMonsterAction = ( _currentUnit == &unit && b_current_sprite != nullptr );

        const fheroes2::Sprite & monsterSprite = isCurrentMonsterAction ? *b_current_sprite : fheroes2::AGG::GetICN( monsterIcnId, unit.GetFrame() );

        const fheroes2::Point drawnPosition = drawTroopSprite( unit, monsterSprite );

        if ( _currentUnit == &unit && b_current_sprite == nullptr ) {
            // Current unit's turn which is idling.
            const fheroes2::Sprite & monsterContour = fheroes2::CreateContour( monsterSprite, _contourColor );
            fheroes2::Blit( monsterContour, _mainSurface, drawnPosition.x, drawnPosition.y, unit.isReflect() );
        }
    }
}

fheroes2::Point Battle::Interface::drawTroopSprite( const Unit & unit, const fheroes2::Sprite & troopSprite )
{
    const fheroes2::Rect & unitPosition = unit.GetRectPosition();
    // Get the sprite rendering offset.
    fheroes2::Point offset = GetTroopPosition( unit, troopSprite );

    if ( _movingUnit == &unit ) {
        // Monster is moving.
        // Unit coordinates relevant to its position are set in Sprite coordinates 'x'  and 'y' (some values may be set incorrectly).
        // The offset data from BIN file is used in Monster info dialog and to correct horizontal movement range when moving diagonally.
        // IMPORTANT: The 'x' offset from BIN file cannot be used in horizontal movement animation as
        // it does not take into account the uneven movement during the step. Use sprite 'x' coordinate for this purpose.

        if ( _movingUnit->animation.animationLength() ) {
            // Get the horizontal and vertical movement projections.
            const int32_t moveX = _movingPos.x - unitPosition.x;
            const int32_t moveY = _movingPos.y - unitPosition.y;

            // If it is a slowed flying creature, then it should smoothly move horizontally.
            if ( _movingUnit->isAbilityPresent( fheroes2::MonsterAbilityType::FLYING ) ) {
                const double movementProgress = _movingUnit->animation.movementProgress();
                offset.x += static_cast<int32_t>( movementProgress * moveX );
                offset.y += static_cast<int32_t>( movementProgress * moveY );
            }
            // If the creature has to move diagonally.
            else if ( moveY != 0 ) {
                offset.x -= Sign( moveX ) * ( _movingUnit->animation.getCurrentFrameXOffset() ) / 2;
                offset.y += static_cast<int32_t>( _movingUnit->animation.movementProgress() * moveY );
            }
        }
    }
    else if ( _flyingUnit == &unit ) {
        // Monster is flying.
        // Get the horizontal and vertical movement projections.
        const int32_t moveX = _flyingPos.x - unitPosition.x;
        const int32_t moveY = _flyingPos.y - unitPosition.y;

        const double movementProgress = _flyingUnit->animation.movementProgress();

        offset.x += moveX + static_cast<int32_t>( ( _movingPos.x - _flyingPos.x ) * movementProgress );
        offset.y += moveY + static_cast<int32_t>( ( _movingPos.y - _flyingPos.y ) * movementProgress );
    }

    fheroes2::AlphaBlit( troopSprite, _mainSurface, offset.x, offset.y, unit.GetCustomAlpha(), unit.isReflect() );

    return offset;
}

void Battle::Interface::RedrawTroopCount( const Unit & unit )
{
    const fheroes2::Rect & rt = unit.GetRectPosition();
    const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( ICN::TEXTBAR, GetIndexIndicator( unit ) );
    const bool isReflected = unit.isReflect();

    const int32_t monsterIndex = unit.GetHeadIndex();
    const int tileInFront = Board::GetIndexDirection( monsterIndex, isReflected ? Battle::LEFT : Battle::RIGHT );
    const bool isValidFrontMonster = ( monsterIndex / ARENAW ) == ( tileInFront == ARENAW );

    int32_t sx = rt.x + ( isReflected ? -7 : rt.width - 13 );
    const int32_t sy = rt.y + rt.height - bar.height() - ( isReflected ? 21 : 9 );

    int xOffset = unit.animation.getTroopCountOffset( isReflected );
    // check if has unit standing in front
    if ( xOffset > 0 && isValidFrontMonster && Board::isValidIndex( tileInFront ) && Board::GetCell( tileInFront )->GetUnit() != nullptr )
        xOffset = 0;

    sx += isReflected ? -xOffset : xOffset;

    fheroes2::Blit( bar, _mainSurface, sx, sy );

    Text text( fheroes2::abbreviateNumber( unit.GetCount() ), Font::SMALL );
    text.Blit( sx + ( bar.width() - text.w() ) / 2, sy, _mainSurface );
}

void Battle::Interface::RedrawCover()
{
    const Settings & conf = Settings::Get();
    const Board & board = *Arena::GetBoard();

    RedrawCoverStatic( conf, board );

    const Bridge * bridge = Arena::GetBridge();
    if ( bridge && ( bridge->isDown() || _bridgeAnimation.animationIsRequired ) ) {
        uint32_t spriteIndex = bridge->isDestroyed() ? BridgeMovementAnimation::DESTROYED : BridgeMovementAnimation::DOWN_POSITION;

        if ( _bridgeAnimation.animationIsRequired ) {
            spriteIndex = _bridgeAnimation.currentFrameId;
        }

        const fheroes2::Sprite & bridgeImage = fheroes2::AGG::GetICN( ICN::Get4Castle( Arena::GetCastle()->GetRace() ), spriteIndex );
        fheroes2::Blit( bridgeImage, _mainSurface, bridgeImage.x(), bridgeImage.y() );
    }

    // cursor
    const Cell * cell = Board::GetCell( index_pos );
    const int cursorType = Cursor::Get().Themes();

    if ( cell && _currentUnit && conf.BattleShowMouseShadow() ) {
        std::set<const Cell *> highlightCells;

        if ( humanturn_spell.isValid() ) {
            switch ( humanturn_spell.GetID() ) {
            case Spell::COLDRING: {
                const Indexes around = Board::GetAroundIndexes( index_pos );
                for ( size_t i = 0; i < around.size(); ++i ) {
                    const Cell * nearbyCell = Board::GetCell( around[i] );
                    if ( nearbyCell != nullptr ) {
                        highlightCells.emplace( nearbyCell );
                    }
                }
                break;
            }
            case Spell::FIREBALL:
            case Spell::METEORSHOWER: {
                highlightCells.emplace( cell );
                const Indexes around = Board::GetAroundIndexes( index_pos );
                for ( size_t i = 0; i < around.size(); ++i ) {
                    const Cell * nearbyCell = Board::GetCell( around[i] );
                    if ( nearbyCell != nullptr ) {
                        highlightCells.emplace( nearbyCell );
                    }
                }
                break;
            }
            case Spell::FIREBLAST: {
                highlightCells.emplace( cell );
                const Indexes around = Board::GetDistanceIndexes( index_pos, 2 );
                for ( size_t i = 0; i < around.size(); ++i ) {
                    const Cell * nearbyCell = Board::GetCell( around[i] );
                    if ( nearbyCell != nullptr ) {
                        highlightCells.emplace( nearbyCell );
                    }
                }
                break;
            }
            default:
                highlightCells.emplace( cell );
            }
        }
        else if ( _currentUnit->isAbilityPresent( fheroes2::MonsterAbilityType::AREA_SHOT )
                  && ( cursorType == Cursor::WAR_ARROW || cursorType == Cursor::WAR_BROKENARROW ) ) {
            highlightCells.emplace( cell );
            const Indexes around = Board::GetAroundIndexes( index_pos );
            for ( size_t i = 0; i < around.size(); ++i ) {
                const Cell * nearbyCell = Board::GetCell( around[i] );
                if ( nearbyCell != nullptr ) {
                    highlightCells.emplace( nearbyCell );
                }
            }
        }
        else if ( _currentUnit->isWide() && ( cursorType == Cursor::WAR_MOVE || cursorType == Cursor::WAR_FLY ) ) {
            const Position pos = Position::GetReachable( *_currentUnit, index_pos );

            assert( pos.GetHead() != nullptr );
            assert( pos.GetTail() != nullptr );

            highlightCells.emplace( pos.GetHead() );
            highlightCells.emplace( pos.GetTail() );
        }
        else if ( cursorType == Cursor::SWORD_TOPLEFT || cursorType == Cursor::SWORD_TOPRIGHT || cursorType == Cursor::SWORD_BOTTOMLEFT
                  || cursorType == Cursor::SWORD_BOTTOMRIGHT || cursorType == Cursor::SWORD_LEFT || cursorType == Cursor::SWORD_RIGHT ) {
            highlightCells.emplace( cell );

            int direction = 0;
            if ( cursorType == Cursor::SWORD_TOPLEFT ) {
                direction = BOTTOM_RIGHT;
            }
            else if ( cursorType == Cursor::SWORD_TOPRIGHT ) {
                direction = BOTTOM_LEFT;
            }
            else if ( cursorType == Cursor::SWORD_BOTTOMLEFT ) {
                direction = TOP_RIGHT;
            }
            else if ( cursorType == Cursor::SWORD_BOTTOMRIGHT ) {
                direction = TOP_LEFT;
            }
            else if ( cursorType == Cursor::SWORD_LEFT ) {
                direction = RIGHT;
            }
            else if ( cursorType == Cursor::SWORD_RIGHT ) {
                direction = LEFT;
            }
            else {
                assert( 0 );
            }

            const Position pos = Position::GetReachable( *_currentUnit, Board::GetIndexDirection( index_pos, direction ) );

            assert( pos.GetHead() != nullptr );

            highlightCells.emplace( pos.GetHead() );

            if ( _currentUnit->isWide() ) {
                assert( pos.GetTail() != nullptr );

                highlightCells.emplace( pos.GetTail() );
            }

            if ( _currentUnit->isDoubleCellAttack() ) {
                const Cell * secondAttackedCell = Board::GetCell( index_pos, Board::GetReflectDirection( direction ) );

                if ( secondAttackedCell ) {
                    highlightCells.emplace( secondAttackedCell );
                }
            }
            else if ( _currentUnit->isAllAdjacentCellsAttack() ) {
                for ( const int32_t nearbyIdx : Board::GetAroundIndexes( pos ) ) {
                    // Should already be highlighted
                    if ( nearbyIdx == index_pos ) {
                        continue;
                    }

                    const Cell * nearbyCell = Board::GetCell( nearbyIdx );
                    assert( nearbyCell != nullptr );

                    const Unit * nearbyUnit = nearbyCell->GetUnit();

                    if ( nearbyUnit && nearbyUnit->GetColor() != _currentUnit->GetCurrentColor() ) {
                        highlightCells.emplace( nearbyCell );
                    }
                }
            }
        }
        else {
            highlightCells.emplace( cell );
        }

        assert( !highlightCells.empty() );

        const HeroBase * currentCommander = arena.GetCurrentCommander();
        const int spellPower = ( currentCommander == nullptr ) ? 0 : currentCommander->GetPower();

        for ( const Cell * highlightCell : highlightCells ) {
            bool isApplicable = highlightCell->isPassable( false );

            if ( isApplicable ) {
                const Unit * highlightedUnit = highlightCell->GetUnit();

                isApplicable
                    = highlightedUnit == nullptr || !humanturn_spell.isValid() || !highlightedUnit->isMagicResist( humanturn_spell, spellPower, currentCommander );
            }

            if ( isApplicable ) {
                fheroes2::Blit( _hexagonCursorShadow, _mainSurface, highlightCell->GetPos().x, highlightCell->GetPos().y );
            }
        }
    }
}

void Battle::Interface::RedrawCoverStatic( const Settings & conf, const Board & board )
{
    if ( icn_cbkg != ICN::UNKNOWN ) {
        const fheroes2::Sprite & cbkg = fheroes2::AGG::GetICN( icn_cbkg, 0 );
        fheroes2::Copy( cbkg, _mainSurface );
    }

    if ( icn_frng != ICN::UNKNOWN ) {
        const fheroes2::Sprite & frng = fheroes2::AGG::GetICN( icn_frng, 0 );
        fheroes2::Blit( frng, _mainSurface, frng.x(), frng.y() );
    }

    if ( arena.GetICNCovr() != ICN::UNKNOWN ) {
        const fheroes2::Sprite & cover = fheroes2::AGG::GetICN( arena.GetICNCovr(), 0 );
        fheroes2::Blit( cover, _mainSurface, cover.x(), cover.y() );
    }

    const Castle * castle = Arena::GetCastle();
    int castleBackgroundIcnId = ICN::UNKNOWN;

    if ( castle != nullptr ) {
        switch ( castle->GetRace() ) {
        case Race::BARB:
            castleBackgroundIcnId = ICN::CASTBKGB;
            break;
        case Race::KNGT:
            castleBackgroundIcnId = ICN::CASTBKGK;
            break;
        case Race::NECR:
            castleBackgroundIcnId = ICN::CASTBKGN;
            break;
        case Race::SORC:
            castleBackgroundIcnId = ICN::CASTBKGS;
            break;
        case Race::WRLK:
            castleBackgroundIcnId = ICN::CASTBKGW;
            break;
        case Race::WZRD:
            castleBackgroundIcnId = ICN::CASTBKGZ;
            break;
        default:
            // DId you add a new race? Add the appropriate logic for it.
            assert( 0 );
            break;
        }

        const fheroes2::Sprite & castleBackground = fheroes2::AGG::GetICN( castleBackgroundIcnId, 1 );
        fheroes2::Blit( castleBackground, _mainSurface, castleBackground.x(), castleBackground.y() );

        // moat
        if ( castle->isBuild( BUILD_MOAT ) ) {
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MOATWHOL, 0 );
            fheroes2::Blit( sprite, _mainSurface, sprite.x(), sprite.y() );
        }
    }

    const bool isGridEnabled = conf.BattleShowGrid();

    // grid
    if ( isGridEnabled ) {
        for ( const Cell & cell : board ) {
            fheroes2::Blit( _hexagonGrid, _mainSurface, cell.GetPos().x, cell.GetPos().y );
        }
    }

    // ground obstacles
    for ( int32_t cellId = 0; cellId < ARENASIZE; ++cellId ) {
        RedrawLowObjects( cellId );
    }

    if ( castle != nullptr ) {
        // top wall
        const fheroes2::Sprite & sprite2 = fheroes2::AGG::GetICN( castleBackgroundIcnId, castle->isFortificationBuild() ? 4 : 3 );
        fheroes2::Blit( sprite2, _mainSurface, sprite2.x(), sprite2.y() );
    }

    // shadow
    if ( !_movingUnit && conf.BattleShowMoveShadow() && _currentUnit && !( _currentUnit->GetCurrentControl() & CONTROL_AI ) ) {
        const fheroes2::Image & shadowImage = isGridEnabled ? _hexagonGridShadow : _hexagonShadow;

        for ( const Cell & cell : board ) {
            const Position pos = Position::GetReachable( *_currentUnit, cell.GetIndex() );

            if ( pos.GetHead() != nullptr ) {
                assert( !_currentUnit->isWide() || pos.GetTail() != nullptr );

                fheroes2::Blit( shadowImage, _mainSurface, cell.GetPos().x, cell.GetPos().y );
            }
        }
    }
}

void Battle::Interface::RedrawCastle( const Castle & castle, int32_t cellId )
{
    const int castleIcnId = ICN::Get4Castle( castle.GetRace() );

    if ( Arena::CATAPULT_POS == cellId ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::CATAPULT, catapult_frame );
        fheroes2::Blit( sprite, _mainSurface, 22 + sprite.x(), 390 + sprite.y() );
    }
    else if ( Arena::CASTLE_GATE_POS == cellId ) {
        const Bridge * bridge = Arena::GetBridge();
        assert( bridge != nullptr );
        if ( bridge != nullptr && !bridge->isDestroyed() ) {
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( castleIcnId, 4 );
            fheroes2::Blit( sprite, _mainSurface, sprite.x(), sprite.y() );
        }
    }
    else if ( Arena::CASTLE_FIRST_TOP_WALL_POS == cellId || Arena::CASTLE_SECOND_TOP_WALL_POS == cellId || Arena::CASTLE_THIRD_TOP_WALL_POS == cellId
              || Arena::CASTLE_FOURTH_TOP_WALL_POS == cellId ) {
        uint32_t index = 0;

        switch ( cellId ) {
        case Arena::CASTLE_FIRST_TOP_WALL_POS:
            index = 5;
            break;
        case Arena::CASTLE_SECOND_TOP_WALL_POS:
            index = 6;
            break;
        case Arena::CASTLE_THIRD_TOP_WALL_POS:
            index = 7;
            break;
        case Arena::CASTLE_FOURTH_TOP_WALL_POS:
            index = 8;
            break;
        default:
            break;
        }

        if ( castle.isFortificationBuild() ) {
            switch ( Board::GetCell( cellId )->GetObject() ) {
            case 0:
                index += 31;
                break;
            case 1:
                index += 35;
                break;
            case 2:
                index += 27;
                break;
            case 3:
                index += 23;
                break;
            default:
                break;
            }
        }
        else {
            switch ( Board::GetCell( cellId )->GetObject() ) {
            case 0:
                index += 8;
                break;
            case 1:
                index += 4;
                break;
            case 2:
                index += 0;
                break;
            default:
                break;
            }
        }

        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( castleIcnId, index );
        fheroes2::Blit( sprite, _mainSurface, sprite.x(), sprite.y() );
    }
    else if ( Arena::CASTLE_TOP_ARCHER_TOWER_POS == cellId ) {
        const Tower * ltower = Arena::GetTower( TowerType::TWR_LEFT );
        uint32_t index = 17;

        if ( castle.isBuild( BUILD_LEFTTURRET ) && ltower )
            index = ltower->isValid() ? 18 : 19;

        const fheroes2::Sprite & towerSprite = fheroes2::AGG::GetICN( castleIcnId, index );
        fheroes2::Blit( towerSprite, _mainSurface, 443 + towerSprite.x(), 153 + towerSprite.y() );
    }
    else if ( Arena::CASTLE_BOTTOM_ARCHER_TOWER_POS == cellId ) {
        const Tower * rtower = Arena::GetTower( TowerType::TWR_RIGHT );
        uint32_t index = 17;

        if ( castle.isBuild( BUILD_RIGHTTURRET ) && rtower )
            index = rtower->isValid() ? 18 : 19;

        const fheroes2::Sprite & towerSprite = fheroes2::AGG::GetICN( castleIcnId, index );
        fheroes2::Blit( towerSprite, _mainSurface, 443 + towerSprite.x(), 405 + towerSprite.y() );
    }
    else if ( Arena::CASTLE_TOP_GATE_TOWER_POS == cellId ) {
        const fheroes2::Sprite & towerSprite = fheroes2::AGG::GetICN( castleIcnId, 17 );
        fheroes2::Blit( towerSprite, _mainSurface, 399 + towerSprite.x(), 237 + towerSprite.y() );
    }
    else if ( Arena::CASTLE_BOTTOM_GATE_TOWER_POS == cellId ) {
        const fheroes2::Sprite & towerSprite = fheroes2::AGG::GetICN( castleIcnId, 17 );
        fheroes2::Blit( towerSprite, _mainSurface, 399 + towerSprite.x(), 321 + towerSprite.y() );
    }
}

void Battle::Interface::RedrawCastleMainTower( const Castle & castle )
{
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::Get4Castle( castle.GetRace() ), ( Arena::GetTower( TowerType::TWR_CENTER )->isValid() ? 20 : 26 ) );

    fheroes2::Blit( sprite, _mainSurface, sprite.x(), sprite.y() );
}

void Battle::Interface::RedrawLowObjects( int32_t cell_index )
{
    const Cell * cell = Board::GetCell( cell_index );
    if ( cell == nullptr )
        return;

    const int cellObjectId = cell->GetObject();
    if ( cellObjectId == 0 ) {
        // No object exists.
        return;
    }

    int objectIcnId = 0;

    switch ( cellObjectId ) {
    case 0x84:
        objectIcnId = ICN::COBJ0004;
        break;
    case 0x87:
        objectIcnId = ICN::COBJ0007;
        break;
    case 0x90:
        objectIcnId = ICN::COBJ0016;
        break;
    case 0x9E:
        objectIcnId = ICN::COBJ0030;
        break;
    case 0x9F:
        objectIcnId = ICN::COBJ0031;
        break;
    default:
        return;
    }

    const fheroes2::Sprite & objectSprite = fheroes2::AGG::GetICN( objectIcnId, 0 );
    const fheroes2::Rect & pt = cell->GetPos();
    fheroes2::Blit( objectSprite, _mainSurface, pt.x + pt.width / 2 + objectSprite.x(), pt.y + pt.height + objectSprite.y() + cellYOffset );
}

void Battle::Interface::RedrawHighObjects( int32_t cell_index )
{
    const Cell * cell = Board::GetCell( cell_index );
    if ( cell == nullptr )
        return;

    const int cellObjectId = cell->GetObject();
    if ( cellObjectId == 0 ) {
        // No object exists.
        return;
    }

    int objectIcnId = 0;

    switch ( cellObjectId ) {
    case 0x80:
        objectIcnId = ICN::COBJ0000;
        break;
    case 0x81:
        objectIcnId = ICN::COBJ0001;
        break;
    case 0x82:
        objectIcnId = ICN::COBJ0002;
        break;
    case 0x83:
        objectIcnId = ICN::COBJ0003;
        break;
    case 0x85:
        objectIcnId = ICN::COBJ0005;
        break;
    case 0x86:
        objectIcnId = ICN::COBJ0006;
        break;
    case 0x88:
        objectIcnId = ICN::COBJ0008;
        break;
    case 0x89:
        objectIcnId = ICN::COBJ0009;
        break;
    case 0x8A:
        objectIcnId = ICN::COBJ0010;
        break;
    case 0x8B:
        objectIcnId = ICN::COBJ0011;
        break;
    case 0x8C:
        objectIcnId = ICN::COBJ0012;
        break;
    case 0x8D:
        objectIcnId = ICN::COBJ0013;
        break;
    case 0x8E:
        objectIcnId = ICN::COBJ0014;
        break;
    case 0x8F:
        objectIcnId = ICN::COBJ0015;
        break;
    case 0x91:
        objectIcnId = ICN::COBJ0017;
        break;
    case 0x92:
        objectIcnId = ICN::COBJ0018;
        break;
    case 0x93:
        objectIcnId = ICN::COBJ0019;
        break;
    case 0x94:
        objectIcnId = ICN::COBJ0020;
        break;
    case 0x95:
        objectIcnId = ICN::COBJ0021;
        break;
    case 0x96:
        objectIcnId = ICN::COBJ0022;
        break;
    case 0x97:
        objectIcnId = ICN::COBJ0023;
        break;
    case 0x98:
        objectIcnId = ICN::COBJ0024;
        break;
    case 0x99:
        objectIcnId = ICN::COBJ0025;
        break;
    case 0x9A:
        objectIcnId = ICN::COBJ0026;
        break;
    case 0x9B:
        objectIcnId = ICN::COBJ0027;
        break;
    case 0x9C:
        objectIcnId = ICN::COBJ0028;
        break;
    case 0x9D:
        objectIcnId = ICN::COBJ0029;
        break;
    default:
        return;
    }

    const fheroes2::Sprite & objectSprite = fheroes2::AGG::GetICN( objectIcnId, 0 );
    const fheroes2::Rect & pt = cell->GetPos();
    fheroes2::Blit( objectSprite, _mainSurface, pt.x + pt.width / 2 + objectSprite.x(), pt.y + pt.height + objectSprite.y() + cellYOffset );
}

void Battle::Interface::RedrawKilled()
{
    // redraw killed troop
    const Indexes cells = arena.GraveyardOccupiedCells();

    for ( Indexes::const_iterator it = cells.begin(); it != cells.end(); ++it ) {
        const std::vector<const Unit *> & units = arena.GetGraveyardTroops( *it );
        for ( size_t i = 0; i < units.size(); ++i ) {
            if ( units[i] && *it != units[i]->GetTailIndex() ) {
                RedrawTroopSprite( *units[i] );
            }
        }
    }
}

int Battle::Interface::GetBattleCursor( std::string & statusMsg ) const
{
    statusMsg.clear();

    const Cell * cell = Board::GetCell( index_pos );

    if ( cell && _currentUnit ) {
        auto formatViewInfoMsg = []( const Unit * unit ) {
            assert( unit != nullptr );

            std::string msg = _( "View %{monster} info" );
            StringReplaceWithLowercase( msg, "%{monster}", unit->GetMultiName() );

            return msg;
        };

        const Unit * unit = cell->GetUnit();

        if ( unit == nullptr || _currentUnit == unit ) {
            const Position pos = Position::GetReachable( *_currentUnit, index_pos );

            if ( pos.GetHead() != nullptr ) {
                assert( !_currentUnit->isWide() || pos.GetTail() != nullptr );

                if ( pos.GetHead()->GetIndex() == _currentUnit->GetHeadIndex() ) {
                    assert( !_currentUnit->isWide() || pos.GetTail()->GetIndex() == _currentUnit->GetTailIndex() );

                    statusMsg = formatViewInfoMsg( _currentUnit );

                    return Cursor::WAR_INFO;
                }

                statusMsg = _currentUnit->isFlying() ? _( "Fly %{monster} here" ) : _( "Move %{monster} here" );
                StringReplaceWithLowercase( statusMsg, "%{monster}", _currentUnit->GetName() );

                return _currentUnit->isFlying() ? Cursor::WAR_FLY : Cursor::WAR_MOVE;
            }
        }
        else {
            if ( _currentUnit->GetCurrentColor() == unit->GetColor() ) {
                statusMsg = formatViewInfoMsg( unit );

                return Cursor::WAR_INFO;
            }

            if ( _currentUnit->isArchers() && !_currentUnit->isHandFighting() ) {
                statusMsg = _( "Shoot %{monster}" );
                statusMsg.append( " " );
                statusMsg.append( _n( "(1 shot left)", "(%{count} shots left)", _currentUnit->GetShots() ) );
                StringReplaceWithLowercase( statusMsg, "%{monster}", unit->GetMultiName() );
                StringReplace( statusMsg, "%{count}", _currentUnit->GetShots() );

                return arena.IsShootingPenalty( *_currentUnit, *unit ) ? Cursor::WAR_BROKENARROW : Cursor::WAR_ARROW;
            }

            // Find all possible directions where the current monster can attack.
            std::set<int> availableAttackDirection;

            for ( const int direction : { BOTTOM_RIGHT, BOTTOM_LEFT, RIGHT, TOP_RIGHT, TOP_LEFT, LEFT } ) {
                if ( Board::isValidDirection( index_pos, direction ) && Board::CanAttackFromCell( *_currentUnit, Board::GetIndexDirection( index_pos, direction ) ) ) {
                    availableAttackDirection.emplace( direction );
                }
            }

            if ( !availableAttackDirection.empty() ) {
                int currentDirection = cell->GetTriangleDirection( GetMouseCursor() );
                if ( currentDirection == UNKNOWN ) {
                    // This could happen when another window has popped up and the user moved the mouse.
                    currentDirection = CENTER;
                }

                if ( availableAttackDirection.count( currentDirection ) == 0 ) {
                    // This direction is not valid. Find the nearest one.
                    if ( availableAttackDirection.size() == 1 ) {
                        currentDirection = *availableAttackDirection.begin();
                    }
                    else {
                        // First search clockwise.
                        direction_t clockWiseDirection = static_cast<direction_t>( currentDirection );
                        direction_t antiClockWiseDirection = static_cast<direction_t>( currentDirection );

                        while ( true ) {
                            ++clockWiseDirection;
                            if ( availableAttackDirection.count( clockWiseDirection ) > 0 ) {
                                currentDirection = clockWiseDirection;
                                break;
                            }

                            --antiClockWiseDirection;
                            if ( availableAttackDirection.count( antiClockWiseDirection ) > 0 ) {
                                currentDirection = antiClockWiseDirection;
                                break;
                            }
                        }
                    }
                }

                const int cursor = GetSwordCursorDirection( currentDirection );

                statusMsg = _( "Attack %{monster}" );
                StringReplaceWithLowercase( statusMsg, "%{monster}", unit->GetName() );

                return cursor;
            }
        }
    }

    statusMsg = _( "Turn %{turn}" );
    StringReplace( statusMsg, "%{turn}", arena.GetCurrentTurn() );

    return Cursor::WAR_NONE;
}

int Battle::Interface::GetBattleSpellCursor( std::string & statusMsg ) const
{
    statusMsg.clear();

    const Cell * cell = Board::GetCell( index_pos );
    const Spell & spell = humanturn_spell;

    if ( cell && _currentUnit && spell.isValid() ) {
        const Unit * b_stats = cell->GetUnit();

        // over graveyard
        if ( !b_stats && arena.GraveyardAllowResurrect( index_pos, spell ) ) {
            b_stats = arena.GraveyardLastTroop( index_pos );
            if ( b_stats->isWide() ) { // we need to check tail and head positions
                const Cell * tailCell = Board::GetCell( b_stats->GetTailIndex() );
                const Cell * headCell = Board::GetCell( b_stats->GetHeadIndex() );
                if ( !tailCell || tailCell->GetUnit() || !headCell || headCell->GetUnit() )
                    b_stats = nullptr;
            }
        }

        // teleport check first
        if ( Board::isValidIndex( teleport_src ) ) {
            const Unit * unitToTeleport = arena.GetTroopBoard( teleport_src );

            assert( unitToTeleport != nullptr );

            if ( !b_stats && cell->isPassableForUnit( *unitToTeleport ) ) {
                statusMsg = _( "Teleport here" );
                return Cursor::SP_TELEPORT;
            }

            statusMsg = _( "Invalid teleport destination" );
            return Cursor::WAR_NONE;
        }
        else if ( b_stats && b_stats->AllowApplySpell( spell, _currentUnit->GetCurrentOrArmyCommander() ) ) {
            statusMsg = _( "Cast %{spell} on %{monster}" );
            StringReplace( statusMsg, "%{spell}", spell.GetName() );
            StringReplaceWithLowercase( statusMsg, "%{monster}", b_stats->GetName() );
            return GetCursorFromSpell( spell.GetID() );
        }
        else if ( !spell.isApplyToFriends() && !spell.isApplyToEnemies() && !spell.isApplyToAnyTroops() ) {
            statusMsg = _( "Cast %{spell}" );
            StringReplace( statusMsg, "%{spell}", spell.GetName() );
            return GetCursorFromSpell( spell.GetID() );
        }
    }

    statusMsg = _( "Select spell target" );

    return Cursor::WAR_NONE;
}

void Battle::Interface::getPendingActions( Actions & actions )
{
    if ( _interruptAutoBattleForColor ) {
        actions.emplace_back( CommandType::MSG_BATTLE_AUTO_SWITCH, _interruptAutoBattleForColor );

        _interruptAutoBattleForColor = 0;
    }
}

void Battle::Interface::HumanTurn( const Unit & b, Actions & a )
{
    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    // Reset the cursor position to avoid forcing the cursor shadow to be drawn at the last position of the previous turn.
    index_pos = -1;

    _currentUnit = &b;
    humanturn_redraw = false;
    humanturn_exit = false;
    catapult_frame = 0;

    // in case we moved the window
    _interfacePosition = border.GetArea();

    popup.Reset();

    // Wait for previously set and not passed delays before rendering a new frame.
    WaitForAllActionDelays();

    ResetIdleTroopAnimation();
    Redraw();

    std::string msg;
    animation_flags_frame = 0;

    // TODO: update delay types within the loop to avoid rendering slowdown.
    const std::vector<Game::DelayType> delayTypes{ Game::BATTLE_FLAGS_DELAY };

    const Board * board = Arena::GetBoard();
    LocalEvent & le = LocalEvent::Get();

    while ( !humanturn_exit && le.HandleEvents( Game::isDelayNeeded( delayTypes ) ) ) {
        // move cursor
        int32_t indexNew = -1;
        if ( le.MouseCursor( { _interfacePosition.x, _interfacePosition.y, _interfacePosition.width, _interfacePosition.height - status.height } ) ) {
            indexNew = board->GetIndexAbsPosition( GetMouseCursor() );
        }
        if ( index_pos != indexNew ) {
            index_pos = indexNew;
            humanturn_redraw = true;
        }

        if ( humanturn_spell.isValid() )
            HumanCastSpellTurn( b, a, msg );
        else
            HumanBattleTurn( b, a, msg );

        // update status
        if ( msg != status.GetMessage() ) {
            status.SetMessage( msg );
            humanturn_redraw = true;
        }

        // animation troops
        if ( IdleTroopsAnimation() )
            humanturn_redraw = true;

        CheckGlobalEvents( le );

        // redraw arena
        if ( humanturn_redraw ) {
            Redraw();
            humanturn_redraw = false;
        }
    }

    popup.Reset();
    _currentUnit = nullptr;
}

void Battle::Interface::HumanBattleTurn( const Unit & b, Actions & a, std::string & msg )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    const Settings & conf = Settings::Get();

    if ( le.KeyPress() ) {
        // Skip the turn
        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_SKIP ) ) {
            a.emplace_back( CommandType::MSG_BATTLE_SKIP, b.GetUID() );
            humanturn_exit = true;
        }
        // Battle options
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_OPTIONS ) ) {
            EventShowOptions();
        }
        // Switch the auto battle mode on/off
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_AUTO_SWITCH ) ) {
            EventAutoSwitch( b, a );
        }
        // Finish the battle in auto mode
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_AUTO_FINISH ) ) {
            EventAutoFinish( a );
        }
        // Cast the spell
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_CAST_SPELL ) ) {
            ProcessingHeroDialogResult( 1, a );
        }
        // Retreat
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_RETREAT ) ) {
            ProcessingHeroDialogResult( 2, a );
        }
        // Surrender
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_SURRENDER ) ) {
            ProcessingHeroDialogResult( 3, a );
        }

#ifdef WITH_DEBUG
        if ( IS_DEVEL() )
            switch ( le.KeyValue() ) {
            case fheroes2::Key::KEY_W:
                // The attacker wins the battle instantly
                arena.GetResult().army1 = RESULT_WINS;
                humanturn_exit = true;
                a.emplace_back( CommandType::MSG_BATTLE_END_TURN, b.GetUID() );
                break;

            case fheroes2::Key::KEY_L:
                // The attacker loses the battle instantly
                arena.GetResult().army1 = RESULT_LOSS;
                humanturn_exit = true;
                a.emplace_back( CommandType::MSG_BATTLE_END_TURN, b.GetUID() );
                break;

            default:
                break;
            }
#endif
    }

    // Add offsets to inner objects
    const fheroes2::Rect mainTowerRect = main_tower + _interfacePosition.getPosition();
    const fheroes2::Rect armiesOrderRect = armies_order + _interfacePosition.getPosition();
    if ( Arena::GetTower( TowerType::TWR_CENTER ) && le.MouseCursor( mainTowerRect ) ) {
        cursor.SetThemes( Cursor::WAR_INFO );
        msg = _( "View Ballista info" );

        if ( le.MouseClickLeft( mainTowerRect ) || le.MousePressRight( mainTowerRect ) ) {
            const Castle * cstl = Arena::GetCastle();
            std::string ballistaMessage = Tower::GetInfo( *cstl );

            if ( cstl->isBuild( BUILD_MOAT ) ) {
                ballistaMessage.append( "\n \n" );
                ballistaMessage.append( Battle::Board::GetMoatInfo() );
            }

            Dialog::Message( _( "Ballista" ), ballistaMessage, Font::BIG, le.MousePressRight() ? 0 : Dialog::OK );
        }
    }
    else if ( conf.BattleShowArmyOrder() && le.MouseCursor( armiesOrderRect ) ) {
        cursor.SetThemes( Cursor::POINTER );
        armies_order.QueueEventProcessing( msg, _interfacePosition.getPosition() );
    }
    else if ( le.MouseCursor( btn_auto.area() ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );
        msg = _( "Enable auto combat" );
        ButtonAutoAction( b, a );

        if ( le.MousePressRight() ) {
            Dialog::Message( _( "Auto Combat" ), _( "Allows the computer to fight out the battle for you." ), Font::BIG );
        }
    }
    else if ( le.MouseCursor( btn_settings.area() ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );
        msg = _( "Customize system options" );
        ButtonSettingsAction();

        if ( le.MousePressRight() ) {
            Dialog::Message( _( "System Options" ), _( "Allows you to customize the combat screen." ), Font::BIG );
        }
    }
    else if ( le.MouseCursor( btn_skip.area() ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );
        msg = _( "Skip this unit" );
        ButtonSkipAction( a );

        if ( le.MousePressRight() ) {
            Dialog::Message( _( "Skip" ), _( "Skips the current creature. The current creature ends its turn and does not get to go again until the next round." ),
                             Font::BIG );
        }
    }
    else if ( opponent1 && le.MouseCursor( opponent1->GetArea() + _interfacePosition.getPosition() ) ) {
        const fheroes2::Rect opponent1Area = opponent1->GetArea() + _interfacePosition.getPosition();
        if ( arena.GetCurrentColor() == arena.GetArmy1Color() ) {
            if ( opponent1->GetHero()->isCaptain() ) {
                msg = _( "View Captain's options" );
            }
            else {
                msg = _( "View Hero's options" );
            }
            cursor.SetThemes( Cursor::WAR_HERO );

            if ( le.MouseClickLeft( opponent1Area ) ) {
                ProcessingHeroDialogResult( arena.DialogBattleHero( *opponent1->GetHero(), true, status ), a );
                humanturn_redraw = true;
            }
        }
        else {
            if ( opponent1->GetHero()->isCaptain() ) {
                msg = _( "View opposing Captain" );
            }
            else {
                msg = _( "View opposing Hero" );
            }
            cursor.SetThemes( Cursor::WAR_INFO );

            if ( le.MouseClickLeft( opponent1Area ) ) {
                arena.DialogBattleHero( *opponent1->GetHero(), true, status );
                humanturn_redraw = true;
            }
        }

        if ( le.MousePressRight( opponent1Area ) ) {
            arena.DialogBattleHero( *opponent1->GetHero(), false, status );
            humanturn_redraw = true;
        }
    }
    else if ( opponent2 && le.MouseCursor( opponent2->GetArea() + _interfacePosition.getPosition() ) ) {
        const fheroes2::Rect opponent2Area = opponent2->GetArea() + _interfacePosition.getPosition();
        if ( arena.GetCurrentColor() == arena.GetForce2().GetColor() ) {
            if ( opponent2->GetHero()->isCaptain() ) {
                msg = _( "View Captain's options" );
            }
            else {
                msg = _( "View Hero's options" );
            }

            cursor.SetThemes( Cursor::WAR_HERO );

            if ( le.MouseClickLeft( opponent2Area ) ) {
                ProcessingHeroDialogResult( arena.DialogBattleHero( *opponent2->GetHero(), true, status ), a );
                humanturn_redraw = true;
            }
        }
        else {
            if ( opponent2->GetHero()->isCaptain() ) {
                msg = _( "View opposing Captain" );
            }
            else {
                msg = _( "View opposing Hero" );
            }

            cursor.SetThemes( Cursor::WAR_INFO );

            if ( le.MouseClickLeft( opponent2Area ) ) {
                arena.DialogBattleHero( *opponent2->GetHero(), true, status );
                humanturn_redraw = true;
            }
        }

        if ( le.MousePressRight( opponent2Area ) ) {
            arena.DialogBattleHero( *opponent2->GetHero(), false, status );
            humanturn_redraw = true;
        }
    }
    else if ( listlog && listlog->isOpenLog() && le.MouseCursor( listlog->GetArea() ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );

        listlog->QueueEventProcessing();
    }
    else if ( le.MouseCursor( { _interfacePosition.x, _interfacePosition.y, _interfacePosition.width, _interfacePosition.height - status.height } ) ) {
        const int themes = GetBattleCursor( msg );

        if ( cursor.Themes() != themes )
            cursor.SetThemes( themes );

        const Cell * cell = Board::GetCell( index_pos );

        if ( cell ) {
            if ( CursorAttack( themes ) ) {
                const Unit * b_enemy = cell->GetUnit();
                popup.SetAttackInfo( cell, _currentUnit, b_enemy );
            }
            else
                popup.Reset();

            if ( le.MouseClickLeft() )
                MouseLeftClickBoardAction( themes, *cell, a );
            else if ( le.MousePressRight() )
                MousePressRightBoardAction( *cell );
        }
        else {
            le.MouseClickLeft();
            le.MousePressRight();
        }
    }
    else if ( le.MouseCursor( status ) ) {
        if ( listlog ) {
            msg = ( listlog->isOpenLog() ? _( "Hide logs" ) : _( "Show logs" ) );
            if ( le.MouseClickLeft( status ) ) {
                listlog->SetOpenLog( !listlog->isOpenLog() );
            }
            else if ( le.MousePressRight( status ) ) {
                fheroes2::showMessage( fheroes2::Text( _( "Message Bar" ), fheroes2::FontType::normalYellow() ),
                                       fheroes2::Text( _( "Shows the results of individual monster's actions." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
            }
        }
        cursor.SetThemes( Cursor::WAR_POINTER );
    }
    else {
        cursor.SetThemes( Cursor::WAR_NONE );
        le.MouseClickLeft();
        le.MousePressRight();
    }
}

void Battle::Interface::HumanCastSpellTurn( const Unit & /*b*/, Actions & a, std::string & msg )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    // reset cast
    if ( le.MousePressRight() || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
        humanturn_spell = Spell::NONE;
        teleport_src = -1;
    }
    else if ( le.MouseCursor( _interfacePosition ) && humanturn_spell.isValid() ) {
        const int themes = GetBattleSpellCursor( msg );

        if ( cursor.Themes() != themes )
            cursor.SetThemes( themes );

        const Cell * cell = Board::GetCell( index_pos );
        if ( cell && _currentUnit && cell->GetUnit() ) {
            popup.SetSpellAttackInfo( cell, _currentUnit->GetCurrentOrArmyCommander(), cell->GetUnit(), humanturn_spell );
        }
        else {
            popup.Reset();
        }

        if ( le.MouseClickLeft() && Cursor::WAR_NONE != cursor.Themes() ) {
            if ( !Board::isValidIndex( index_pos ) ) {
                DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                           "dst: "
                               << "out of range" )
                return;
            }

            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, humanturn_spell.GetName() << ", dst: " << index_pos )

            if ( Cursor::SP_TELEPORT == cursor.Themes() ) {
                if ( 0 > teleport_src )
                    teleport_src = index_pos;
                else {
                    a.emplace_back( CommandType::MSG_BATTLE_CAST, Spell::TELEPORT, teleport_src, index_pos );
                    humanturn_spell = Spell::NONE;
                    humanturn_exit = true;
                    teleport_src = -1;
                }
            }
            else if ( Cursor::SP_MIRRORIMAGE == cursor.Themes() ) {
                a.emplace_back( CommandType::MSG_BATTLE_CAST, Spell::MIRRORIMAGE, index_pos );
                humanturn_spell = Spell::NONE;
                humanturn_exit = true;
            }
            else {
                a.emplace_back( CommandType::MSG_BATTLE_CAST, humanturn_spell.GetID(), index_pos );
                humanturn_spell = Spell::NONE;
                humanturn_exit = true;
            }
        }
    }
    else {
        cursor.SetThemes( Cursor::WAR_NONE );
    }
}

void Battle::Interface::FadeArena( bool clearMessageLog )
{
    AudioManager::ResetAudio();

    fheroes2::Display & display = fheroes2::Display::instance();

    if ( clearMessageLog ) {
        status.clear();
        status.Redraw( display );
    }

    Redraw();

    const fheroes2::Rect srt = border.GetArea();
    fheroes2::Image top( srt.width, srt.height );

    fheroes2::Copy( display, srt.x, srt.y, top, 0, 0, srt.width, srt.height );
    fheroes2::FadeDisplayWithPalette( top, srt.getPosition(), 5, 300, 5 );

    display.render();
}

int Battle::GetIndexIndicator( const Unit & b )
{
    // yellow
    if ( b.Modes( IS_GREEN_STATUS ) && b.Modes( IS_RED_STATUS ) ) {
        return 13;
    }
    // green
    if ( b.Modes( IS_GREEN_STATUS ) ) {
        return 12;
    }
    // red
    if ( b.Modes( IS_RED_STATUS ) ) {
        return 14;
    }

    return 10;
}

void Battle::Interface::EventShowOptions()
{
    btn_settings.drawOnPress();
    DialogBattleSettings();
    btn_settings.drawOnRelease();
    humanturn_redraw = true;
}

void Battle::Interface::EventAutoSwitch( const Unit & b, Actions & a )
{
    if ( !arena.CanToggleAutoBattle() ) {
        return;
    }

    a.emplace_back( CommandType::MSG_BATTLE_AUTO_SWITCH, b.GetCurrentOrArmyColor() );

    humanturn_redraw = true;
    humanturn_exit = true;
}

void Battle::Interface::EventAutoFinish( Actions & a )
{
    if ( fheroes2::showMessage( fheroes2::Text( "", {} ),
                                fheroes2::Text( _( "Are you sure you want to finish the battle in auto mode?" ), fheroes2::FontType::normalWhite() ),
                                Dialog::YES | Dialog::NO )
         != Dialog::YES ) {
        return;
    }

    a.emplace_back( CommandType::MSG_BATTLE_AUTO_FINISH );

    humanturn_redraw = true;
    humanturn_exit = true;
}

void Battle::Interface::ButtonAutoAction( const Unit & b, Actions & a )
{
    LocalEvent & le = LocalEvent::Get();

    le.MousePressLeft( btn_auto.area() ) ? btn_auto.drawOnPress() : btn_auto.drawOnRelease();

    if ( le.MouseClickLeft( btn_auto.area() ) )
        EventAutoSwitch( b, a );
}

void Battle::Interface::ButtonSettingsAction()
{
    LocalEvent & le = LocalEvent::Get();

    le.MousePressLeft( btn_settings.area() ) ? btn_settings.drawOnPress() : btn_settings.drawOnRelease();

    if ( le.MouseClickLeft( btn_settings.area() ) ) {
        DialogBattleSettings();
        humanturn_redraw = true;
    }
}

void Battle::Interface::ButtonSkipAction( Actions & a )
{
    LocalEvent & le = LocalEvent::Get();

    le.MousePressLeft( btn_skip.area() ) ? btn_skip.drawOnPress() : btn_skip.drawOnRelease();

    if ( le.MouseClickLeft( btn_skip.area() ) && _currentUnit ) {
        a.emplace_back( CommandType::MSG_BATTLE_SKIP, _currentUnit->GetUID() );
        humanturn_exit = true;
    }
}

void Battle::Interface::MousePressRightBoardAction( const Cell & cell ) const
{
    const Unit * unitOnCell = cell.GetUnit();

    if ( unitOnCell != nullptr ) {
        Dialog::ArmyInfo( *unitOnCell, Dialog::ZERO, unitOnCell->isReflect() );
    }
    else {
        unitOnCell = arena.GraveyardLastTroop( cell.GetIndex() );
        if ( unitOnCell != nullptr ) {
            Dialog::ArmyInfo( *unitOnCell, Dialog::ZERO, unitOnCell->isReflect() );
        }
    }
}

void Battle::Interface::MouseLeftClickBoardAction( int themes, const Cell & cell, Actions & a )
{
    const int32_t index = cell.GetIndex();
    const Unit * b = cell.GetUnit();

    auto fixupDestinationCell = []( const Unit & unit, const int32_t dst ) {
        // Only wide units may need this fixup
        if ( !unit.isWide() ) {
            return dst;
        }

        const Position pos = Position::GetReachable( unit, dst );

        assert( pos.GetHead() != nullptr && pos.GetTail() != nullptr );

        return pos.GetHead()->GetIndex();
    };

    if ( _currentUnit ) {
        switch ( themes ) {
        case Cursor::WAR_FLY:
        case Cursor::WAR_MOVE:
            a.emplace_back( CommandType::MSG_BATTLE_MOVE, _currentUnit->GetUID(), fixupDestinationCell( *_currentUnit, index ) );
            a.emplace_back( CommandType::MSG_BATTLE_END_TURN, _currentUnit->GetUID() );
            humanturn_exit = true;
            break;

        case Cursor::SWORD_TOPLEFT:
        case Cursor::SWORD_TOPRIGHT:
        case Cursor::SWORD_RIGHT:
        case Cursor::SWORD_BOTTOMRIGHT:
        case Cursor::SWORD_BOTTOMLEFT:
        case Cursor::SWORD_LEFT: {
            const Unit * enemy = b;
            const int dir = GetDirectionFromCursorSword( themes );

            if ( enemy && Board::isValidDirection( index, dir ) ) {
                const int32_t move = fixupDestinationCell( *_currentUnit, Board::GetIndexDirection( index, dir ) );

                if ( _currentUnit->GetHeadIndex() != move ) {
                    a.emplace_back( CommandType::MSG_BATTLE_MOVE, _currentUnit->GetUID(), move );
                }
                a.emplace_back( CommandType::MSG_BATTLE_ATTACK, _currentUnit->GetUID(), enemy->GetUID(), index, Board::GetReflectDirection( dir ) );
                a.emplace_back( CommandType::MSG_BATTLE_END_TURN, _currentUnit->GetUID() );
                humanturn_exit = true;
            }
            break;
        }

        case Cursor::WAR_BROKENARROW:
        case Cursor::WAR_ARROW: {
            const Unit * enemy = b;

            if ( enemy ) {
                a.emplace_back( CommandType::MSG_BATTLE_ATTACK, _currentUnit->GetUID(), enemy->GetUID(), index, 0 );
                a.emplace_back( CommandType::MSG_BATTLE_END_TURN, _currentUnit->GetUID() );
                humanturn_exit = true;
            }
            break;
        }

        case Cursor::WAR_INFO: {
            if ( b ) {
                Dialog::ArmyInfo( *b, Dialog::BUTTONS, b->isReflect() );
                humanturn_redraw = true;
            }
            break;
        }

        default:
            break;
        }
    }
}

void Battle::Interface::WaitForAllActionDelays()
{
    LocalEvent & le = LocalEvent::Get();

    // The array of possible delays of previous battlefield actions.
    const std::vector<Game::DelayType> unitDelays{ Game::DelayType::BATTLE_FRAME_DELAY,
                                                   Game::DelayType::BATTLE_MISSILE_DELAY,
                                                   Game::DelayType::BATTLE_SPELL_DELAY,
                                                   Game::DelayType::BATTLE_DISRUPTING_DELAY,
                                                   Game::DelayType::BATTLE_CATAPULT_CLOUD_DELAY,
                                                   Game::DelayType::BATTLE_BRIDGE_DELAY,
                                                   Game::DelayType::CUSTOM_BATTLE_UNIT_MOVEMENT_DELAY };

    // Wait for the delay after previous render and only after it render a new frame and proceed to the rest of this function.
    while ( le.HandleEvents( Game::isDelayNeeded( unitDelays ) ) ) {
        CheckGlobalEvents( le );

        if ( Game::hasEveryDelayPassed( unitDelays ) ) {
            break;
        }
    }
}

void Battle::Interface::AnimateUnitWithDelay( Unit & unit, const bool skipLastFrameRender )
{
    if ( unit.isFinishAnimFrame() && unit.animation.animationLength() != 1 ) {
        // If it is the last frame in the animation sequence with more than one frame or if we have no frames.
        return;
    }

    LocalEvent & le = LocalEvent::Get();

    // In the loop below we wait for the delay and then display the next frame.
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::DelayType::CUSTOM_BATTLE_UNIT_MOVEMENT_DELAY } ) ) ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::DelayType::CUSTOM_BATTLE_UNIT_MOVEMENT_DELAY ) ) {
            Redraw();

            if ( unit.isFinishAnimFrame() ) {
                // We have reached the end of animation and rendered the last frame.
                break;
            }

            unit.IncreaseAnimFrame();

            if ( skipLastFrameRender && unit.isFinishAnimFrame() ) {
                // We have reached the last animation frame and do not render it.
                break;
            }
        }
    }
}

void Battle::Interface::AnimateOpponents( OpponentSprite * target )
{
    if ( target == nullptr || target->isFinishFrame() ) // nothing to animate
        return;

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_OPPONENTS_DELAY } ) ) ) {
        if ( Game::validateAnimationDelay( Game::BATTLE_OPPONENTS_DELAY ) ) {
            // Render the first frame before waiting any delay.
            Redraw();

            if ( target->isFinishFrame() ) {
                // We have reached the end of animation.
                break;
            }

            target->IncreaseAnimFrame();
        }
    }
}

void Battle::Interface::RedrawTroopDefaultDelay( Unit & unit )
{
    if ( unit.isFinishAnimFrame() ) // nothing to animate
        return;

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_FRAME_DELAY } ) ) ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_FRAME_DELAY ) ) {
            Redraw();

            if ( unit.isFinishAnimFrame() ) {
                // We have reached the end of animation.
                break;
            }

            unit.IncreaseAnimFrame();
        }
    }
}

void Battle::Interface::RedrawActionSkipStatus( const Unit & attacker )
{
    std::string msg = _( "%{name} skip their turn." );
    StringReplaceWithLowercase( msg, "%{name}", attacker.GetName() );

    status.SetMessage( msg, true );
}

void Battle::Interface::RedrawMissileAnimation( const fheroes2::Point & startPos, const fheroes2::Point & endPos, double angle, uint32_t monsterID )
{
    LocalEvent & le = LocalEvent::Get();
    fheroes2::Sprite missile;

    const bool reverse = startPos.x > endPos.x;
    const bool isMage = ( monsterID == Monster::MAGE || monsterID == Monster::ARCHMAGE );

    // Mage is channeling the bolt; doesn't have missile sprite
    if ( isMage ) {
        fheroes2::delayforMs( Game::ApplyBattleSpeed( 115 ) );
    }
    else {
        missile = fheroes2::AGG::GetICN( Monster::GetMissileICN( monsterID ), static_cast<uint32_t>( Bin_Info::GetMonsterInfo( monsterID ).getProjectileID( angle ) ) );
    }

    // Lich/Power lich has projectile speed of 25
    const std::vector<fheroes2::Point> points = GetEuclideanLine( startPos, endPos, isMage ? 50 : std::max( missile.width(), 25 ) );
    std::vector<fheroes2::Point>::const_iterator pnt = points.begin();

    // For most shooting creatures we do not render the first missile position to better imitate start position change depending on shooting angle.
    if ( !isMage && ( monsterID != Monster::TROLL ) && ( monsterID != Monster::WAR_TROLL ) ) {
        ++pnt;
    }

    // Shooter projectile rendering offset uses 'x' and 'y' from sprite data.
    const fheroes2::Point missileOffset( reverse ? ( -missile.width() - missile.x() ) : missile.x(), ( angle > 0 ) ? ( -missile.height() - missile.y() ) : missile.y() );

    // Wait for previously set and not passed delays before rendering a new frame.
    WaitForAllActionDelays();

    // convert the following code into a function/event service
    while ( le.HandleEvents( false ) && pnt != points.end() ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_MISSILE_DELAY ) ) {
            RedrawPartialStart();
            if ( isMage ) {
                fheroes2::DrawLine( _mainSurface, { startPos.x, startPos.y - 2 }, { pnt->x, pnt->y - 2 }, 0x77 );
                fheroes2::DrawLine( _mainSurface, { startPos.x, startPos.y - 1 }, { pnt->x, pnt->y - 1 }, 0xB5 );
                fheroes2::DrawLine( _mainSurface, startPos, *pnt, 0xBC );
                fheroes2::DrawLine( _mainSurface, { startPos.x, startPos.y + 1 }, { pnt->x, pnt->y + 1 }, 0xB5 );
                fheroes2::DrawLine( _mainSurface, { startPos.x, startPos.y + 2 }, { pnt->x, pnt->y + 2 }, 0x77 );
            }
            else {
                // Coordinates in 'pnt' corresponds to the front side of the projectile (arrowhead).
                fheroes2::Blit( missile, _mainSurface, pnt->x + missileOffset.x, pnt->y + missileOffset.y, reverse );
            }
            RedrawPartialFinish();
            ++pnt;
        }
    }
}

void Battle::Interface::RedrawActionNewTurn() const
{
    if ( !Music::isPlaying() ) {
        AudioManager::PlayMusicAsync( MUS::GetBattleRandom(), Music::PlaybackMode::REWIND_AND_PLAY_INFINITE );
    }

    if ( listlog == nullptr ) {
        return;
    }

    std::string msg = _( "Turn %{turn}" );
    StringReplace( msg, "%{turn}", arena.GetCurrentTurn() );

    listlog->AddMessage( std::move( msg ) );
}

void Battle::Interface::RedrawActionAttackPart1( Unit & attacker, const Unit & defender, const TargetsInfo & targets )
{
    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    _currentUnit = nullptr;
    _movingUnit = &attacker;
    _movingPos = attacker.GetRectPosition().getPosition();

    // Unit 'Position' is position of the tile he's standing at
    const fheroes2::Rect & pos1 = attacker.GetRectPosition();
    const fheroes2::Rect & pos2 = defender.GetRectPosition();

    const bool archer = attacker.isArchers() && !attacker.isHandFighting();
    const bool isDoubleCell = attacker.isDoubleCellAttack() && 2 == targets.size();

    // redraw luck animation
    if ( attacker.Modes( LUCK_GOOD | LUCK_BAD ) )
        RedrawActionLuck( attacker );

    AudioManager::PlaySound( attacker.M82Attk() );

    // long distance attack animation
    if ( archer ) {
        // Reset the delay to wait till the next frame if is not already waiting.
        if ( !Game::isDelayNeeded( { Game::DelayType::CUSTOM_BATTLE_UNIT_MOVEMENT_DELAY } ) ) {
            Game::AnimateResetDelay( Game::DelayType::CUSTOM_BATTLE_UNIT_MOVEMENT_DELAY );
        }

        const fheroes2::Sprite & attackerSprite = fheroes2::AGG::GetICN( attacker.GetMonsterSprite(), attacker.GetFrame() );
        const fheroes2::Point attackerPos = GetTroopPosition( attacker, attackerSprite );

        // For shooter position we need bottom center position of rear tile
        // Use cell coordinates for X because sprite width is very inconsistent (e.g. Halfling)
        const int rearCenterX = ( attacker.isWide() && attacker.isReflect() ) ? pos1.width * 3 / 4 : CELLW / 2;
        const fheroes2::Point shooterPos( pos1.x + rearCenterX, attackerPos.y - attackerSprite.y() );

        // Use the front one to calculate the angle, then overwrite
        fheroes2::Point offset = attacker.GetStartMissileOffset( Monster_Info::FRONT );

        const fheroes2::Point targetPos = defender.GetCenterPoint();

        double angle = GetAngle( fheroes2::Point( shooterPos.x + offset.x, shooterPos.y + offset.y ), targetPos );
        // This check is made only for situations when a target stands on the same row as shooter.
        if ( attacker.GetHeadIndex() / ARENAW == defender.GetHeadIndex() / ARENAW ) {
            angle = 0;
        }

        // Angles are used in Heroes2 as 90 (TOP) -> 0 (FRONT) -> -90 (BOT) degrees
        const int direction = angle >= 25.0 ? Monster_Info::TOP : ( angle <= -25.0 ) ? Monster_Info::BOTTOM : Monster_Info::FRONT;

        if ( direction != Monster_Info::FRONT )
            offset = attacker.GetStartMissileOffset( direction );

        // redraw archer attack animation
        if ( attacker.SwitchAnimation( Monster_Info::RANG_TOP + direction * 2 ) ) {
            // Set the delay between shooting animation frames.
            Game::setCustomUnitMovementDelay( Game::ApplyBattleSpeed( attacker.animation.getShootingSpeed() ) / attacker.animation.animationLength() );

            // We do not render the last frame of shooting animation as all frames besides this contains the projectile.
            // The last frame will be rendered in RedrawMissileAnimation() function with the render of projectile.
            AnimateUnitWithDelay( attacker, true );
        }

        const fheroes2::Point missileStart( shooterPos.x + ( attacker.isReflect() ? -offset.x : offset.x ), shooterPos.y + offset.y );

        // draw missile animation
        RedrawMissileAnimation( missileStart, targetPos, angle, attacker.GetID() );
    }
    else {
        int attackAnim = isDoubleCell ? Monster_Info::RANG_FRONT : Monster_Info::MELEE_FRONT;
        if ( pos2.y < pos1.y ) {
            attackAnim -= 2;
        }
        else if ( pos2.y > pos1.y ) {
            attackAnim += 2;
        }

        // redraw melee attack animation
        if ( attacker.SwitchAnimation( attackAnim ) ) {
            // Reset the delay to wait till the next frame.
            Game::AnimateResetDelay( Game::DelayType::BATTLE_FRAME_DELAY );

            RedrawTroopDefaultDelay( attacker );
        }
    }
}

void Battle::Interface::RedrawActionAttackPart2( Unit & attacker, const Unit & defender, const TargetsInfo & targets, const uint32_t resurrects )
{
    // Reset the delay to wait till the next frame.
    if ( !Game::isDelayNeeded( { Game::DelayType::BATTLE_FRAME_DELAY } ) ) {
        Game::AnimateResetDelay( Game::DelayType::BATTLE_FRAME_DELAY );
    }

    // post attack animation
    int attackStart = attacker.animation.getCurrentState();
    if ( attackStart >= Monster_Info::MELEE_TOP && attackStart <= Monster_Info::RANG_BOT ) {
        ++attackStart;
        attacker.SwitchAnimation( attackStart );
    }

    // targets damage animation
    RedrawActionWincesKills( targets, &attacker, &defender );
    RedrawTroopDefaultDelay( attacker );

    attacker.SwitchAnimation( Monster_Info::STATIC );

    const bool isMirror = targets.size() == 1 && targets.front().defender->isModes( CAP_MIRRORIMAGE );
    // draw status for first defender
    if ( !isMirror && !targets.empty() ) {
        std::string msg( _n( "%{attacker} does %{damage} damage.", "%{attacker} do %{damage} damage.", attacker.GetCount() ) );
        StringReplaceWithLowercase( msg, "%{attacker}", attacker.GetName() );

        if ( 1 < targets.size() ) {
            uint32_t killed = 0;
            uint32_t damage = 0;

            for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it ) {
                if ( !it->defender->isModes( CAP_MIRRORIMAGE ) ) {
                    killed += ( *it ).killed;
                    damage += ( *it ).damage;
                }
            }

            StringReplace( msg, "%{damage}", damage );

            if ( killed ) {
                msg.append( " " );
                msg.append( _n( "1 creature perishes.", "%{count} creatures perish.", killed ) );
                StringReplace( msg, "%{count}", killed );
            }
        }
        else {
            const TargetInfo & target = targets.front();
            StringReplace( msg, "%{damage}", target.damage );

            if ( target.killed ) {
                msg.append( " " );
                msg.append( _n( "1 %{defender} perishes.", "%{count} %{defender} perish.", target.killed ) );
                StringReplace( msg, "%{count}", target.killed );
                StringReplaceWithLowercase( msg, "%{defender}", target.defender->GetPluralName( target.killed ) );
            }
        }

        status.SetMessage( msg, true );
        status.SetMessage( "", false );

        if ( resurrects != 0 ) {
            auto updateStatusBar = []( Battle::Status & statusBar, std::string & localMsg, const uint32_t localRes, const char * localUnit ) {
                StringReplace( localMsg, "%{count}", static_cast<int32_t>( localRes ) );
                StringReplaceWithLowercase( localMsg, "%{unit}", localUnit );

                statusBar.SetMessage( localMsg, true );
                statusBar.SetMessage( "", false );
            };

            if ( attacker.isAbilityPresent( fheroes2::MonsterAbilityType::SOUL_EATER ) ) {
                msg = _n( "1 soul is absorbed.", "%{count} souls are absorbed.", resurrects );
                updateStatusBar( status, msg, resurrects, attacker.GetPluralName( resurrects ) );
            }
            else if ( attacker.isAbilityPresent( fheroes2::MonsterAbilityType::HP_DRAIN ) ) {
                msg = _n( "1 %{unit} is revived.", "%{count} %{unit} are revived.", resurrects );
                updateStatusBar( status, msg, resurrects, attacker.GetPluralName( resurrects ) );
            }
        }
    }

    _movingUnit = nullptr;
}

void Battle::Interface::RedrawActionWincesKills( const TargetsInfo & targets, Unit * attacker /* = nullptr */, const Unit * defender /* = nullptr */ )
{
    // Reset the delay to wait till the next frame.
    if ( !Game::isDelayNeeded( { Game::DelayType::BATTLE_FRAME_DELAY } ) ) {
        Game::AnimateResetDelay( Game::DelayType::BATTLE_FRAME_DELAY );
    }

    LocalEvent & le = LocalEvent::Get();

    // Number of targets to be animated with a wince or kill animation.
    ptrdiff_t animatingTargets = 0;
    int32_t deathColor = Color::UNUSED;

    std::vector<Unit *> mirrorImages;
    std::set<Unit *> resistantTarget;

    // If this was a Lich attack, we should render an explosion cloud over the target unit immediately after the projectile hits the target,
    // along with the unit kill/wince animation.
    const bool drawLichCloud = ( attacker != nullptr ) && ( defender != nullptr ) && attacker->isArchers() && !attacker->isHandFighting()
                               && attacker->isAbilityPresent( fheroes2::MonsterAbilityType::AREA_SHOT );

    for ( const Battle::TargetInfo & target : targets ) {
        Unit * unit = target.defender;
        if ( unit == nullptr ) {
            continue;
        }

        if ( unit->isModes( CAP_MIRRORIMAGE ) ) {
            mirrorImages.push_back( unit );
        }

        // kill animation
        if ( !unit->isValid() ) {
            // destroy linked mirror
            if ( unit->isModes( CAP_MIRROROWNER ) ) {
                mirrorImages.push_back( unit->GetMirror() );
            }

            unit->SwitchAnimation( Monster_Info::KILL );
            AudioManager::PlaySound( unit->M82Kill() );
            ++animatingTargets;

            deathColor = unit->GetArmyColor();
        }
        else if ( target.damage ) {
            // wince animation
            if ( drawLichCloud ) {
                // The Lich cloud causes units to freeze for some time in the maximum wince state.
                // So we will divide the wince animation. First part: the creature stands for a couple of frames before wincing.
                unit->SwitchAnimation( Monster_Info::STAND_STILL );
            }
            else {
                unit->SwitchAnimation( Monster_Info::WNCE );
                AudioManager::PlaySound( unit->M82Wnce() );
            }
            ++animatingTargets;
        }
        else {
            // have immunity
            resistantTarget.insert( target.defender );
            AudioManager::PlaySound( M82::RSBRYFZL );
        }
    }

    SetHeroAnimationReactionToTroopDeath( deathColor );

    uint32_t lichCloudFrame = 0;
    const uint32_t lichCloudMaxFrame = fheroes2::AGG::GetICNCount( ICN::LICHCLOD );
    // Wince animation under the Lich cloud, second part: the frame number after which the target animation will be switched to 'WNCE_UP'.
    const uint32_t wnceUpStartFrame = 1;
    // Wince animation under the Lich cloud, third part: the frame number after which the target animation will be switched to 'WNCE_DOWN'.
    const uint32_t wnceDownStartFrame = lichCloudMaxFrame - 3;

    if ( drawLichCloud ) {
        // Lich cloud sound.
        AudioManager::PlaySound( attacker->M82Expl() );
    }

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_FRAME_DELAY } ) ) ) {
        CheckGlobalEvents( le );

        if ( !Game::validateAnimationDelay( Game::BATTLE_FRAME_DELAY ) ) {
            continue;
        }

        RedrawPartialStart();

        // Render a Lich cloud above the target unit if it is a Lich attack and if the cloud animation is not already finished.
        if ( drawLichCloud && lichCloudFrame < lichCloudMaxFrame ) {
            const fheroes2::Sprite & spellSprite = fheroes2::AGG::GetICN( ICN::LICHCLOD, lichCloudFrame );
            const fheroes2::Point & pos = CalculateSpellPosition( *defender, ICN::LICHCLOD, spellSprite );
            fheroes2::Blit( spellSprite, _mainSurface, pos.x, pos.y, false );
            ++lichCloudFrame;
        }

        RedrawPartialFinish();

        if ( attacker != nullptr ) {
            if ( attacker->isFinishAnimFrame() ) {
                attacker->SwitchAnimation( Monster_Info::STATIC );
            }
            else {
                attacker->IncreaseAnimFrame();
            }
        }

        for ( const Battle::TargetInfo & target : targets ) {
            if ( target.defender ) {
                if ( target.defender->isFinishAnimFrame()
                     && ( target.defender->GetAnimationState() == Monster_Info::WNCE || target.defender->GetAnimationState() == Monster_Info::WNCE_DOWN ) ) {
                    target.defender->SwitchAnimation( Monster_Info::STATIC );
                }
                else if ( drawLichCloud && lichCloudFrame == wnceUpStartFrame && ( target.defender->GetAnimationState() == Monster_Info::STAND_STILL ) ) {
                    target.defender->SwitchAnimation( Monster_Info::WNCE_UP );
                    AudioManager::PlaySound( target.defender->M82Wnce() );
                }
                else if ( drawLichCloud && lichCloudFrame == wnceDownStartFrame && ( target.defender->GetAnimationState() == Monster_Info::WNCE_UP ) ) {
                    target.defender->SwitchAnimation( Monster_Info::WNCE_DOWN );
                }
                else {
                    target.defender->IncreaseAnimFrame();
                }
            }
        }

        const ptrdiff_t finishedAnimationCount = std::count_if( targets.begin(), targets.end(), [&resistantTarget]( const TargetInfo & info ) {
            if ( info.defender == nullptr ) {
                return false;
            }

            if ( resistantTarget.count( info.defender ) > 0 ) {
                return false;
            }

            const int animationState = info.defender->GetAnimationState();
            if ( animationState == Monster_Info::WNCE || animationState == Monster_Info::WNCE_UP || animationState == Monster_Info::WNCE_DOWN
                 || animationState == Monster_Info::STAND_STILL ) {
                return false;
            }

            if ( animationState != Monster_Info::KILL ) {
                return true;
            }

            return TargetInfo::isFinishAnimFrame( info );
        } );

        // There should not be more finished animations than when we started.
        assert( finishedAnimationCount <= animatingTargets );

        // There should not be more Lich cloud animation frames than in the corresponding ICN.
        assert( lichCloudFrame <= lichCloudMaxFrame );

        // IMPORTANT: The game engine can change STATIC animation to IDLE, especially for Ghosts and Zombies,
        // so we need also to check for IDLE where we check for STATIC.
        if ( ( animatingTargets == finishedAnimationCount ) && ( !drawLichCloud || ( lichCloudFrame == lichCloudMaxFrame ) )
             && ( ( attacker == nullptr ) || ( attacker->animation.getCurrentState() == Monster_Info::STATIC )
                  || ( attacker->animation.getCurrentState() == Monster_Info::IDLE ) ) ) {
            // All unit animation frames are rendered and if it was a Lich attack then also its cloud frames are rendered too.
            break;
        }
    }

    // Fade away animation for destroyed mirror images
    if ( !mirrorImages.empty() ) {
        RedrawActionRemoveMirrorImage( mirrorImages );
    }
}

void Battle::Interface::SetHeroAnimationReactionToTroopDeath( const int32_t deathColor )
{
    if ( deathColor == Color::UNUSED ) {
        return;
    }

    const bool attackersTurn = ( deathColor == arena.GetArmy2Color() );
    OpponentSprite * attackingHero = attackersTurn ? opponent1 : opponent2;
    OpponentSprite * defendingHero = attackersTurn ? opponent2 : opponent1;
    // 60% of joyful animation
    if ( attackingHero && Rand::Get( 1, 5 ) < 4 ) {
        attackingHero->SetAnimation( OP_JOY );
    }
    // 80% of sorrow animation otherwise
    else if ( defendingHero && Rand::Get( 1, 5 ) < 5 ) {
        defendingHero->SetAnimation( OP_SORROW );
    }
}

void Battle::Interface::RedrawActionMove( Unit & unit, const Indexes & path )
{
    // If the path is empty there is no movement and so nothing to render.
    if ( path.empty() ) {
        return;
    }

    // Reset the delay to wait till the next frame if is not already waiting.
    if ( !Game::isDelayNeeded( { Game::DelayType::CUSTOM_BATTLE_UNIT_MOVEMENT_DELAY } ) ) {
        Game::AnimateResetDelay( Game::DelayType::CUSTOM_BATTLE_UNIT_MOVEMENT_DELAY );
    }

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    Indexes::const_iterator dst = path.begin();
    Bridge * bridge = Arena::GetBridge();

    // Get the time to animate movement for one cell.
    uint32_t frameDelay = Game::ApplyBattleSpeed( unit.animation.getMoveSpeed() );
    if ( unit.Modes( SP_HASTE ) ) {
        frameDelay = frameDelay * 65 / 100; // by 35% faster
    }
    else if ( unit.Modes( SP_SLOW ) ) {
        frameDelay = frameDelay * 150 / 100; // by 50% slower
    }

    // Set the delay between movement animation frames. This delay will be used for all types of movement animations.
    unit.SwitchAnimation( Monster_Info::MOVING );
    Game::setCustomUnitMovementDelay( frameDelay / unit.animation.animationLength() );

    std::string msg = _( "Moved %{monster}: from [%{src}] to [%{dst}]." );
    StringReplaceWithLowercase( msg, "%{monster}", unit.GetName() );
    StringReplace( msg, "%{src}", std::to_string( ( unit.GetHeadIndex() / ARENAW ) + 1 ) + ", " + std::to_string( ( unit.GetHeadIndex() % ARENAW ) + 1 ) );

    assert( _movingUnit == nullptr && _flyingUnit == nullptr );

    _currentUnit = nullptr;
    _movingUnit = &unit;

    // If it is a flying creature that acts like walking one when it is under the Slow spell.
    const bool canFly = unit.isAbilityPresent( fheroes2::MonsterAbilityType::FLYING );
    // If it is a wide creature (cache this boolean to use in the loop).
    const bool isWide = unit.isWide();
    const Indexes::const_iterator pathEnd = path.end();

    const bool isOneStepPath = [&unit, &path]() {
        if ( path.size() == 1 ) {
            return true;
        }

        if ( !unit.isWide() ) {
            return false;
        }

        // If wide unit performs 3 movements and the first movement is a turn back, then it's
        // path consists of only one "real" movement, because...
        if ( path.size() == 3 && path[0] == unit.GetTailIndex() ) {
            // ... its last movement should be a return to the normal position.
            assert( Board::GetDirection( path[1], path[2] ) == ( unit.isReflect() ? LEFT : RIGHT ) );

            return true;
        }

        return false;
    }();

    // TODO: make an analytic check with checks for wide creatures back step: is one step left before/after the bridge action.

    // Slowed flying creature has to fly off.
    if ( canFly ) {
        // If a flying creature has to cross the bridge during its path we have to open it before the creature flies up.
        // Otherwise it will freeze during the movement, waiting for the bridge to open. So we have to go the whole path
        // to analyze if the bridge needs to open for this creature.
        if ( bridge ) {
            const Position startPosition = unit.GetPosition();
            while ( dst != pathEnd ) {
                if ( bridge->NeedDown( unit, *dst ) ) {
                    // Restore the initial creature position before rendering the whole battlefield with the bridge animation.
                    unit.SetPosition( startPosition );
                    bridge->ActionDown();
                    break;
                }

                // Fix for wide flyers - go the whole path with reflections to check the bridge.
                if ( isWide && ( unit.GetTailIndex() == *dst ) ) {
                    unit.SetReflection( !unit.isReflect() );
                }
                unit.SetPosition( *dst );

                ++dst;
            }

            // If there was no bridge on the path then restore the initial creature position.
            if ( dst == pathEnd ) {
                unit.SetPosition( startPosition );
            }

            // Restore the initial path pointer state.
            dst = path.begin();
        }

        // The destination of fly off is same as the current creature position.
        _movingPos = unit.GetRectPosition().getPosition();
        const bool isFromRightArmy = unit.isReflect();
        const bool isFlyToRight = ( _movingPos.x < Board::GetCell( *dst )->GetPos().x );

        // Reflect the creature if it has to fly back.
        unit.SetReflection( !isFlyToRight );
        // For creatures, that has no 'FLY_UP' animation, like Ghosts, we check if the animation was correctly set.
        if ( unit.SwitchAnimation( Monster_Info::FLY_UP ) ) {
            AudioManager::PlaySound( unit.M82Tkof() );
            AnimateUnitWithDelay( unit );
        }
        // If a wide flyer returns back it should skip one path position (its head becomes its tail - it is already one move).
        if ( isWide && ( isFlyToRight == isFromRightArmy ) ) {
            ++dst;
        }

        // Switch animation to MOVING before going through the path.
        unit.SwitchAnimation( Monster_Info::MOVING );
    }
    else {
        // Every ground unit should start its movement from the special 'MOVE_START' animation
        // or if it moves only for one cell its animation must be 'MOVE_QUICK'. So a check for 1 cell path is made.
        if ( isOneStepPath ) {
            unit.SwitchAnimation( Monster_Info::MOVE_QUICK );
        }
        else {
            unit.SwitchAnimation( Monster_Info::MOVE_START );
        }
    }

    while ( dst != pathEnd ) {
        // Check if a wide unit changes its horizontal direction.
        if ( isWide && unit.GetTailIndex() == *dst ) {
            // We must not reflect the flyers at the and of the path (just before the landing).
            if ( !canFly || ( dst != ( pathEnd - 1 ) ) ) {
                unit.SetReflection( !unit.isReflect() );
            }
            // After changing the direction go to the next step in the path.
            ++dst;
            continue;
        }

        const Cell * cell = Board::GetCell( *dst );
        _movingPos = cell->GetPos().getPosition();

        if ( !isWide ) {
            // Check for change the horizontal direction. Only for non-wide units, the wide units use their own algorithm.
            unit.UpdateDirection( cell->GetPos() );
        }

        if ( bridge && bridge->NeedDown( unit, *dst ) ) {
            _movingUnit = nullptr;
            unit.SwitchAnimation( Monster_Info::STAND_STILL );
            bridge->ActionDown();
            _movingUnit = &unit;
            if ( dst == ( pathEnd - 1 ) ) {
                // There is only one cell left to move after standing.
                unit.SwitchAnimation( Monster_Info::MOVE_QUICK );
            }
            else {
                // If the path has more than one step after the bridge action then begin the movement.
                unit.SwitchAnimation( Monster_Info::MOVE_START );
            }
        }

        // If a wide flyer is flying to the right its visual horizontal destination should be shifted to the left by one cell.
        if ( canFly && isWide && !unit.isReflect() ) {
            _movingPos.x -= CELLW;
        }

        // Render the unit movement with the movement sound.
        // TODO: adjust sounds calls and synchronize them with frames. Take into account that some sounds (like for Cavalry) consists of a sequence of steps.
        AudioManager::PlaySound( unit.M82Move() );
        AnimateUnitWithDelay( unit );
        unit.SetPosition( *dst );

        ++dst;

        // Do a post-move check for the bridge action and set the animation the movement to the next cell in the path.
        if ( canFly ) {
            // The animation for the next step in the path of slowed flying creatures is always "MOVING".
            unit.SwitchAnimation( Monster_Info::MOVING );
        }
        else {
            // Check for possible bridge close action, after walking unit's end of movement to the next cell.
            // This check should exclude the flying creature because it can't 'hang' here to wait
            // for bridge to close. For this creature, the bridge should close after it lands.
            if ( bridge && bridge->AllowUp() ) {
                _movingUnit = nullptr;
                unit.SwitchAnimation( Monster_Info::STAND_STILL );
                bridge->ActionUp();
                _movingUnit = &unit;
                if ( dst == ( pathEnd - 1 ) ) {
                    // There is only one cell left to move after standing.
                    unit.SwitchAnimation( Monster_Info::MOVE_QUICK );
                }
                else {
                    // If the path has more than one step after the bridge action then begin the movement.
                    unit.SwitchAnimation( Monster_Info::MOVE_START );
                }
            }
            else if ( dst == ( pathEnd - 1 ) ) {
                // There is only one cell left to move.
                unit.SwitchAnimation( Monster_Info::MOVE_END );
            }
            else {
                unit.SwitchAnimation( Monster_Info::MOVING );
            }
        }
    }

    // Slowed flying creature has to land.
    if ( canFly ) {
        // IMPORTANT: do not combine into vector animations with the STATIC at the end: the game could randomly switch it to IDLE this way.
        unit.SwitchAnimation( { Monster_Info::FLY_LAND, Monster_Info::STAND_STILL } );
        AudioManager::PlaySound( unit.M82Land() );
        AnimateUnitWithDelay( unit );

        // Close the bridge only after the creature lands.
        if ( bridge && bridge->AllowUp() ) {
            bridge->ActionUp();
        }
    }

    _movingUnit = nullptr;

    unit.SwitchAnimation( Monster_Info::STATIC );

    StringReplace( msg, "%{dst}", std::to_string( ( unit.GetHeadIndex() / ARENAW ) + 1 ) + ", " + std::to_string( ( unit.GetHeadIndex() % ARENAW ) + 1 ) );

    status.SetMessage( msg, true );

    assert( _currentUnit == nullptr && _movingUnit == nullptr && _flyingUnit == nullptr );
}

void Battle::Interface::RedrawActionFly( Unit & unit, const Position & pos )
{
    const int32_t destIndex = pos.GetHead()->GetIndex();
    const int32_t destTailIndex = unit.isWide() ? pos.GetTail()->GetIndex() : -1;

    // check if we're already there
    if ( unit.GetPosition().contains( destIndex ) ) {
        return;
    }

    // Reset the delay to wait till the next frame if is not already waiting.
    if ( !Game::isDelayNeeded( { Game::DelayType::CUSTOM_BATTLE_UNIT_MOVEMENT_DELAY } ) ) {
        Game::AnimateResetDelay( Game::DelayType::CUSTOM_BATTLE_UNIT_MOVEMENT_DELAY );
    }

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    const fheroes2::Point destPos = unit.GetRectPosition().getPosition();
    fheroes2::Point targetPos = Board::GetCell( destIndex )->GetPos().getPosition();

    if ( unit.isWide() && targetPos.x > destPos.x ) {
        targetPos.x -= CELLW; // this is needed to avoid extra cell shifting upon landing when we move to right side
    }

    std::string msg = _( "Moved %{monster}: from [%{src}] to [%{dst}]." );
    StringReplaceWithLowercase( msg, "%{monster}", unit.GetName() );
    StringReplace( msg, "%{src}", std::to_string( ( unit.GetHeadIndex() / ARENAW ) + 1 ) + ", " + std::to_string( ( unit.GetHeadIndex() % ARENAW ) + 1 ) );

    const uint32_t step = unit.animation.getFlightSpeed();
    uint32_t frameDelay = Game::ApplyBattleSpeed( unit.animation.getMoveSpeed() );
    if ( unit.Modes( SP_HASTE ) ) {
        frameDelay = frameDelay * 8 / 10; // 20% faster
    }
    else if ( unit.Modes( SP_SLOW ) ) {
        frameDelay = frameDelay * 12 / 10; // 20% slower
    }

    // Set the delay between movement animation frames. This delay will be used for all types of movement animations.
    unit.SwitchAnimation( Monster_Info::MOVING );
    Game::setCustomUnitMovementDelay( frameDelay / unit.animation.animationLength() );

    const std::vector<fheroes2::Point> points = GetEuclideanLine( destPos, targetPos, step );
    std::vector<fheroes2::Point>::const_iterator currentPoint = points.begin();

    Bridge * bridge = Arena::GetBridge();

    // Lower the bridge if the unit needs to land on it
    if ( bridge ) {
        if ( bridge->NeedDown( unit, destIndex ) || ( unit.isWide() && bridge->NeedDown( unit, destTailIndex ) ) ) {
            bridge->ActionDown();
        }
    }

    assert( _movingUnit == nullptr && _flyingUnit == nullptr );

    // Jump up
    _currentUnit = nullptr;
    _movingUnit = &unit;
    _movingPos = currentPoint != points.end() ? *currentPoint : destPos;

    // For creatures, that has no 'FLY_UP' animation, like Ghosts, we check if the animation was correctly set.
    if ( unit.SwitchAnimation( Monster_Info::FLY_UP ) ) {
        AudioManager::PlaySound( unit.M82Tkof() );
        AnimateUnitWithDelay( unit );
    }

    _movingUnit = nullptr;
    _flyingUnit = &unit;
    _flyingPos = _movingPos;

    if ( currentPoint != points.end() ) {
        ++currentPoint;
    }

    unit.SwitchAnimation( Monster_Info::MOVING );
    while ( currentPoint != points.end() ) {
        _movingPos = *currentPoint;

        AudioManager::PlaySound( unit.M82Move() );
        unit.animation.restartAnimation();
        AnimateUnitWithDelay( unit );

        _flyingPos = _movingPos;
        ++currentPoint;
    }

    unit.SetPosition( destIndex );

    // Landing
    _flyingUnit = nullptr;
    _movingUnit = &unit;
    _movingPos = targetPos;

    // IMPORTANT: do not combine into vector animations with the STATIC at the end: the game could randomly switch it to IDLE this way.
    unit.SwitchAnimation( { Monster_Info::FLY_LAND, Monster_Info::STAND_STILL } );
    AudioManager::PlaySound( unit.M82Land() );
    AnimateUnitWithDelay( unit );
    unit.SwitchAnimation( Monster_Info::STATIC );

    _movingUnit = nullptr;

    // Raise the bridge if possible after the unit has completed its movement
    if ( bridge && bridge->AllowUp() ) {
        bridge->ActionUp();
    }

    StringReplace( msg, "%{dst}", std::to_string( ( unit.GetHeadIndex() / ARENAW ) + 1 ) + ", " + std::to_string( ( unit.GetHeadIndex() % ARENAW ) + 1 ) );

    status.SetMessage( msg, true );

    assert( _currentUnit == nullptr && _movingUnit == nullptr && _flyingUnit == nullptr );
}

void Battle::Interface::RedrawActionResistSpell( const Unit & target, bool playSound )
{
    if ( playSound ) {
        AudioManager::PlaySound( M82::RSBRYFZL );
    }
    std::string str( _( "The %{name} resist the spell!" ) );
    StringReplaceWithLowercase( str, "%{name}", target.GetName() );
    status.SetMessage( str, true );
    status.SetMessage( "", false );
}

void Battle::Interface::RedrawActionSpellCastStatus( const Spell & spell, int32_t dst, const std::string & name, const TargetsInfo & targets )
{
    const Unit * target = !targets.empty() ? targets.front().defender : nullptr;

    std::string msg;

    if ( target && ( target->GetHeadIndex() == dst || ( target->isWide() && target->GetTailIndex() == dst ) ) ) {
        msg = _( "%{name} casts %{spell} on the %{troop}." );
        StringReplaceWithLowercase( msg, "%{troop}", target->GetName() );
    }
    else {
        msg = _( "%{name} casts %{spell}." );
    }

    if ( !msg.empty() ) {
        StringReplace( msg, "%{name}", name );
        StringReplace( msg, "%{spell}", spell.GetName() );

        status.SetMessage( msg, true );
        status.SetMessage( "", false );
    }
}

void Battle::Interface::RedrawActionSpellCastPart1( const Spell & spell, int32_t dst, const HeroBase * caster, const TargetsInfo & targets )
{
    Unit * target = !targets.empty() ? targets.front().defender : nullptr;

    // set spell cast animation
    if ( caster ) {
        OpponentSprite * opponent = caster->GetColor() == arena.GetArmy1Color() ? opponent1 : opponent2;
        if ( opponent ) {
            // Reset the delay to wait till the next frame.
            Game::AnimateResetDelay( Game::DelayType::BATTLE_OPPONENTS_DELAY );

            opponent->SetAnimation( spell.isApplyWithoutFocusObject() ? OP_CAST_MASS : OP_CAST_UP );
            AnimateOpponents( opponent );
        }
    }

    // without object
    switch ( spell.GetID() ) {
    case Spell::FIREBALL:
        RedrawTargetsWithFrameAnimation( dst, targets, ICN::FIREBALL, M82::FromSpell( spell.GetID() ) );
        break;
    case Spell::FIREBLAST:
        RedrawTargetsWithFrameAnimation( dst, targets, ICN::FIREBAL2, M82::FromSpell( spell.GetID() ) );
        break;
    case Spell::METEORSHOWER:
        RedrawTargetsWithFrameAnimation( dst, targets, ICN::METEOR, M82::FromSpell( spell.GetID() ), 1 );
        break;
    case Spell::COLDRING:
        RedrawActionColdRingSpell( dst, targets );
        break;

    case Spell::MASSSHIELD:
        RedrawTargetsWithFrameAnimation( targets, ICN::SHIELD, M82::FromSpell( spell.GetID() ), false );
        break;
    case Spell::MASSCURE:
        RedrawTargetsWithFrameAnimation( targets, ICN::MAGIC01, M82::FromSpell( spell.GetID() ), false );
        break;
    case Spell::MASSHASTE:
        RedrawTargetsWithFrameAnimation( targets, ICN::HASTE, M82::FromSpell( spell.GetID() ), false );
        break;
    case Spell::MASSSLOW:
        RedrawTargetsWithFrameAnimation( targets, ICN::MAGIC02, M82::FromSpell( spell.GetID() ), false );
        break;
    case Spell::MASSBLESS:
        RedrawTargetsWithFrameAnimation( targets, ICN::BLESS, M82::FromSpell( spell.GetID() ), false );
        break;
    case Spell::MASSCURSE:
        RedrawTargetsWithFrameAnimation( targets, ICN::CURSE, M82::FromSpell( spell.GetID() ), false );
        break;
    case Spell::MASSDISPEL:
        RedrawTargetsWithFrameAnimation( targets, ICN::MAGIC07, M82::FromSpell( spell.GetID() ), false );
        break;

    case Spell::DEATHRIPPLE:
        RedrawActionDeathWaveSpell( 7 );
        break;
    case Spell::DEATHWAVE:
        RedrawActionDeathWaveSpell( 14 );
        break;

    case Spell::HOLYWORD:
        RedrawActionHolyShoutSpell( 16 );
        break;
    case Spell::HOLYSHOUT:
        RedrawActionHolyShoutSpell( 24 );
        break;

    case Spell::ELEMENTALSTORM:
        RedrawActionElementalStormSpell( targets );
        break;
    case Spell::ARMAGEDDON:
        RedrawActionArmageddonSpell(); // hit everything
        break;

    default:
        break;
    }

    // with object
    if ( target ) {
        if ( spell.isResurrect() )
            RedrawActionResurrectSpell( *target, spell );
        else
            switch ( spell.GetID() ) {
            // simple spell animation
            case Spell::BLESS:
                RedrawTroopWithFrameAnimation( *target, ICN::BLESS, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::BLIND:
                RedrawTroopWithFrameAnimation( *target, ICN::BLIND, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::CURE:
                RedrawTroopWithFrameAnimation( *target, ICN::MAGIC01, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::SLOW:
                RedrawTroopWithFrameAnimation( *target, ICN::MAGIC02, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::SHIELD:
                RedrawTroopWithFrameAnimation( *target, ICN::SHIELD, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::HASTE:
                RedrawTroopWithFrameAnimation( *target, ICN::HASTE, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::CURSE:
                RedrawTroopWithFrameAnimation( *target, ICN::CURSE, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::ANTIMAGIC:
                RedrawTroopWithFrameAnimation( *target, ICN::MAGIC06, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::DISPEL:
                RedrawTroopWithFrameAnimation( *target, ICN::MAGIC07, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::STONESKIN:
                RedrawTroopWithFrameAnimation( *target, ICN::STONSKIN, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::STEELSKIN:
                RedrawTroopWithFrameAnimation( *target, ICN::STELSKIN, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::PARALYZE:
                RedrawTroopWithFrameAnimation( *target, ICN::PARALYZE, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::HYPNOTIZE:
                RedrawTroopWithFrameAnimation( *target, ICN::HYPNOTIZ, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::DRAGONSLAYER:
                RedrawTroopWithFrameAnimation( *target, ICN::DRAGSLAY, M82::FromSpell( spell.GetID() ), NONE );
                break;
            case Spell::BERSERKER:
                RedrawTroopWithFrameAnimation( *target, ICN::BERZERK, M82::FromSpell( spell.GetID() ), NONE );
                break;

            // uniq spell animation
            case Spell::LIGHTNINGBOLT:
                RedrawActionLightningBoltSpell( *target );
                break;
            case Spell::CHAINLIGHTNING:
                RedrawActionChainLightningSpell( targets );
                break;
            case Spell::ARROW:
                RedrawActionArrowSpell( *target );
                break;
            case Spell::COLDRAY:
                RedrawActionColdRaySpell( *target );
                break;
            case Spell::DISRUPTINGRAY:
                RedrawActionDisruptingRaySpell( *target );
                break;
            case Spell::BLOODLUST:
                RedrawActionBloodLustSpell( *target );
                break;
            case Spell::PETRIFY:
                RedrawActionStoneSpell( *target );
                break;
            default:
                break;
            }
    }

    if ( caster ) {
        OpponentSprite * opponent = caster->GetColor() == arena.GetArmy1Color() ? opponent1 : opponent2;
        if ( opponent ) {
            // Reset the delay to wait till the next frame.
            Game::AnimateResetDelay( Game::DelayType::BATTLE_OPPONENTS_DELAY );

            opponent->SetAnimation( ( target != nullptr ) ? OP_CAST_UP_RETURN : OP_CAST_MASS_RETURN );
            AnimateOpponents( opponent );
        }
    }
}

void Battle::Interface::RedrawActionSpellCastPart2( const Spell & spell, const TargetsInfo & targets )
{
    if ( spell.isDamage() ) {
        uint32_t killed = 0;
        uint32_t totalDamage = 0;
        uint32_t maximumDamage = 0;
        uint32_t damagedMonsters = 0;

        for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it ) {
            if ( !it->defender->isModes( CAP_MIRRORIMAGE ) ) {
                killed += ( *it ).killed;

                ++damagedMonsters;
                totalDamage += it->damage;
                if ( maximumDamage < it->damage ) {
                    maximumDamage = it->damage;
                }
            }
        }

        // targets damage animation
        switch ( spell.GetID() ) {
            // For some spells the damage animation is done during the flame animation after the spell animation.
        case Spell::DEATHRIPPLE:
        case Spell::DEATHWAVE:
            RedrawTargetsWithFrameAnimation( targets, ICN::REDDEATH, M82::UNKNOWN, true );
            break;
        case Spell::HOLYWORD:
        case Spell::HOLYSHOUT:
            RedrawTargetsWithFrameAnimation( targets, ICN::MAGIC08, M82::UNKNOWN, true );
            break;
        default:
            RedrawActionWincesKills( targets );
            break;
        }

        if ( totalDamage > 0 ) {
            assert( damagedMonsters > 0 );
            std::string msg;
            if ( spell.isUndeadOnly() ) {
                if ( damagedMonsters == 1 ) {
                    msg = _( "The %{spell} does %{damage} damage to one undead creature." );
                    StringReplace( msg, "%{spell}", spell.GetName() );
                    StringReplace( msg, "%{damage}", totalDamage );
                    status.SetMessage( msg, true );

                    if ( killed > 0 ) {
                        msg = _n( "1 creature perishes.", "%{count} creatures perish.", killed );
                        StringReplace( msg, "%{count}", killed );
                        status.SetMessage( msg, true );
                    }
                }
                else {
                    msg = _( "The %{spell} does %{damage} damage to all undead creatures." );
                    StringReplace( msg, "%{spell}", spell.GetName() );
                    StringReplace( msg, "%{damage}", maximumDamage );
                    status.SetMessage( msg, true );

                    if ( killed > 0 ) {
                        msg = _( "The %{spell} does %{damage} damage, %{count} creatures perish." );
                        StringReplace( msg, "%{count}", killed );
                    }
                    else {
                        msg = _( "The %{spell} does %{damage} damage." );
                    }

                    StringReplace( msg, "%{spell}", spell.GetName() );
                    StringReplace( msg, "%{damage}", totalDamage );
                    status.SetMessage( msg, true );
                }
            }
            else if ( spell.isAliveOnly() ) {
                if ( damagedMonsters == 1 ) {
                    msg = _( "The %{spell} does %{damage} damage to one living creature." );
                    StringReplace( msg, "%{spell}", spell.GetName() );
                    StringReplace( msg, "%{damage}", totalDamage );
                    status.SetMessage( msg, true );

                    if ( killed > 0 ) {
                        msg = _n( "1 creature perishes.", "%{count} creatures perish.", killed );
                        StringReplace( msg, "%{count}", killed );
                        status.SetMessage( msg, true );
                    }
                }
                else {
                    msg = _( "The %{spell} does %{damage} damage to all living creatures." );
                    StringReplace( msg, "%{spell}", spell.GetName() );
                    StringReplace( msg, "%{damage}", maximumDamage );
                    status.SetMessage( msg, true );

                    if ( killed > 0 ) {
                        msg = _( "The %{spell} does %{damage} damage, %{count} creatures perish." );
                        StringReplace( msg, "%{count}", killed );
                    }
                    else {
                        msg = _( "The %{spell} does %{damage} damage." );
                    }

                    StringReplace( msg, "%{spell}", spell.GetName() );
                    StringReplace( msg, "%{damage}", totalDamage );
                    status.SetMessage( msg, true );
                }
            }
            else {
                msg = _( "The %{spell} does %{damage} damage." );
                StringReplace( msg, "%{spell}", spell.GetName() );
                StringReplace( msg, "%{damage}", totalDamage );
                status.SetMessage( msg, true );

                if ( killed > 0 ) {
                    msg = _n( "1 creature perishes.", "%{count} creatures perish.", killed );
                    StringReplace( msg, "%{count}", killed );
                    status.SetMessage( msg, true );
                }
            }
        }
    }

    status.SetMessage( " ", false );

    // TODO: remove this temporary assertion
    assert( _movingUnit == nullptr );
}

void Battle::Interface::RedrawActionMonsterSpellCastStatus( const Spell & spell, const Unit & attacker, const TargetInfo & target )
{
    std::string msg;

    switch ( spell.GetID() ) {
    case Spell::BLIND:
        msg = _( "The %{attacker}' attack blinds the %{target}!" );
        break;
    case Spell::PETRIFY:
        msg = _( "The %{attacker}' gaze turns the %{target} to stone!" );
        break;
    case Spell::CURSE:
        msg = _( "The %{attacker}' curse falls upon the %{target}!" );
        break;
    case Spell::PARALYZE:
        msg = _( "The %{target} are paralyzed by the %{attacker}!" );
        break;
    case Spell::DISPEL:
        msg = _( "The %{attacker} dispel all good spells on your %{target}!" );
        break;
    default:
        // Did you add a new monster spell casting ability? Add the logic above!
        assert( 0 );
        msg = _( "The %{attacker} cast %{spell} on %{target}!" );
        StringReplace( msg, "%{spell}", spell.GetName() );
        break;
    }

    StringReplaceWithLowercase( msg, "%{attacker}", attacker.GetMultiName() );
    StringReplaceWithLowercase( msg, "%{target}", target.defender->GetName() );

    status.SetMessage( msg, true );
    status.SetMessage( "", false );
}

void Battle::Interface::RedrawActionLuck( const Unit & unit )
{
    // Reset the delay to wait till the next frame if is not already waiting.
    if ( !Game::isDelayNeeded( { Game::DelayType::BATTLE_MISSILE_DELAY } ) ) {
        Game::AnimateResetDelay( Game::DelayType::BATTLE_MISSILE_DELAY );
    }

    LocalEvent & le = LocalEvent::Get();

    const bool isGoodLuck = unit.Modes( LUCK_GOOD );
    const fheroes2::Rect & pos = unit.GetRectPosition();

    std::string msg = isGoodLuck ? _( "Good luck shines on the %{attacker}." ) : _( "Bad luck descends on the %{attacker}." );
    StringReplaceWithLowercase( msg, "%{attacker}", unit.GetName() );
    status.SetMessage( msg, true );

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );
    if ( isGoodLuck ) {
        const fheroes2::Rect & battleArea = border.GetArea();
        const fheroes2::Point rainbowDescendPoint( pos.x + pos.width / 2, pos.y - pos.height / 2 );

        // If the creature is low on the battleboard - the rainbow will be from the top (in the original game the threshold is about 140 pixels).
        const bool isVerticalRainbow = ( rainbowDescendPoint.y > 140 );

        // Set the rainbow animation direction to match the army side.
        // Also, if the creature is under effect of the Berserker spell (or similar), then check its original color.
        bool isRainbowFromRight = ( unit.GetCurrentColor() < 0 ) ? ( unit.GetColor() == arena.GetArmy2Color() ) : ( unit.GetCurrentColor() == arena.GetArmy2Color() );

        // The distance from the right or left battlefield border to the 'lucky' creature in the direction from the beginning of the animation to its end.
        const int32_t borderDistance = isRainbowFromRight ? battleArea.width - rainbowDescendPoint.x : rainbowDescendPoint.x;

        // The rainbow thickness in pixels: it must be 15 pixels to match the rainbow thickness from original sprite 'ICN::EXPMRL'.
        const int32_t rainbowThickness = 15;

        // Declare rainbow generation parameters and set default values:
        // Rainbow arc parameters for: y = (1-pow2ratio)*k1*(x-x0)^pow1+pow2ratio*k2*(x-x0)^pow2.
        // Parameters pow3, pow4, pow4ratio are the same as pow1, pow2, pow2ratio, but for the second part of the arc.
        const int32_t pow1{ 2 };
        int32_t pow2{ 10 };
        double pow2ratio{ 0.16 };
        const int32_t pow3{ 2 };
        int32_t pow4{ 2 };
        double pow4ratio{ 0.77 };
        // The distance from the start to the end of the rainbow in direction of animation (in pixels).
        int32_t rainbowLength{ rainbowDescendPoint.y };
        // The distance from the start to the end (the 'lucky' creature) of the rainbow orthogonal to the direction of animation (in pixels).
        int32_t rainbowAscend{ 75 };
        // The distance from the top to the end (the 'lucky' creature) of the rainbow orthogonal to the direction of animation (in pixels).
        int32_t rainbowDescend{ 10 };
        // The coordinate where the rainbow arc changes its direction.
        int32_t rainbowTop{ 50 };
        // Rainbow image offset from battlefield zero coordinates to "fall" onto the 'lucky' creature.
        int32_t drawOffset{ 10 };

        // Set rainbow generation parameters.
        if ( isVerticalRainbow ) {
            rainbowAscend = static_cast<int32_t>( 0.4845 * rainbowLength + 156.2 );
            // If the rainbow doesn't fit on the screen, then change its horizontal direction.
            if ( ( borderDistance + rainbowThickness / 2 ) < rainbowAscend ) {
                isRainbowFromRight = !isRainbowFromRight;
            }
            rainbowDescend = std::max( 1, static_cast<int32_t>( 0.0342 * rainbowLength - 4.868 ) );
            rainbowTop = static_cast<int32_t>( 0.8524 * rainbowLength + 17.7 );
            drawOffset
                = isRainbowFromRight ? ( rainbowDescendPoint.x - rainbowDescend - rainbowThickness / 2 ) : ( rainbowDescendPoint.x - rainbowDescend - rainbowAscend );
        }
        else {
            pow2 = 0;
            pow4 = 5;
            pow2ratio = 0.0;
            pow4ratio = 0.5;
            rainbowLength = borderDistance;
            rainbowDescend = std::max( 1, static_cast<int32_t>( 0.1233 * rainbowLength + 0.7555 ) );
            rainbowTop = static_cast<int32_t>( 0.6498 * rainbowLength + 11.167 );
            drawOffset = std::max( 10, rainbowDescendPoint.y - rainbowDescend );
        }

        const fheroes2::Size rainbowArcBegin( rainbowTop, rainbowDescend + rainbowAscend );
        const fheroes2::Size rainbowArcEnd( rainbowLength - rainbowTop, rainbowDescend );

        std::vector<int32_t> rainbowArc;

        GetHalfArc( rainbowArc, -rainbowArcBegin.width, rainbowArcBegin.height, pow1, pow2, pow2ratio );
        GetHalfArc( rainbowArc, rainbowArcEnd.width, rainbowArcEnd.height, pow3, pow4, pow4ratio );

        const fheroes2::Image luckSprite = DrawRainbow( rainbowArc, rainbowThickness, isVerticalRainbow, isRainbowFromRight );

        // Rainbow animation draw step (in original game it is random and about 7-11 pixels).
        // We set the constant animation time for all rainbows: rainbowLength/30 fits the rainbow sound duration on '1' speed.
        const double drawStep = rainbowLength / 30.0;

        AudioManager::PlaySound( M82::GOODLUCK );

        double x = 0;
        while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_MISSILE_DELAY } ) ) && ( Mixer::isPlaying( -1 ) || x < rainbowLength ) ) {
            CheckGlobalEvents( le );

            if ( x < rainbowLength && Game::validateAnimationDelay( Game::BATTLE_MISSILE_DELAY ) ) {
                RedrawPartialStart();

                x += drawStep;
                const int32_t drawWidth = x > rainbowLength ? rainbowLength : static_cast<int32_t>( x );

                // For different rainbow types use appropriate animation direction.
                if ( isVerticalRainbow ) {
                    fheroes2::Blit( luckSprite, 0, 0, _mainSurface, drawOffset, 0, luckSprite.width(), drawWidth );
                }
                else {
                    if ( isRainbowFromRight ) {
                        fheroes2::Blit( luckSprite, rainbowLength - drawWidth, 0, _mainSurface, battleArea.width - drawWidth, drawOffset, drawWidth,
                                        luckSprite.height() );
                    }
                    else {
                        fheroes2::Blit( luckSprite, 0, 0, _mainSurface, 0, drawOffset, drawWidth, luckSprite.height() );
                    }
                }

                RedrawPartialFinish();
            }
        }
    }
    else {
        const int maxHeight = fheroes2::AGG::GetAbsoluteICNHeight( ICN::CLOUDLUK );
        int y = pos.y + pos.height + cellYOffset;

        // move drawing position if it will clip outside of the battle window
        if ( y - maxHeight < 0 )
            y = maxHeight;

        AudioManager::PlaySound( M82::BADLUCK );

        int frameId = 0;
        while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_MISSILE_DELAY } ) ) && Mixer::isPlaying( -1 ) ) {
            CheckGlobalEvents( le );

            if ( frameId < 8 && Game::validateAnimationDelay( Game::BATTLE_MISSILE_DELAY ) ) {
                RedrawPartialStart();

                const fheroes2::Sprite & luckSprite = fheroes2::AGG::GetICN( ICN::CLOUDLUK, frameId );
                fheroes2::Blit( luckSprite, _mainSurface, pos.x + pos.width / 2 + luckSprite.x(), y + luckSprite.y() );

                RedrawPartialFinish();

                ++frameId;
            }
        }
    }
}

void Battle::Interface::RedrawActionMorale( Unit & b, bool good )
{
    std::string msg;

    if ( good ) {
        msg = _( "High morale enables the %{monster} to attack again." );
        StringReplaceWithLowercase( msg, "%{monster}", b.GetName() );
        status.SetMessage( msg, true );
        RedrawTroopWithFrameAnimation( b, ICN::MORALEG, M82::GOODMRLE, NONE );
    }
    else {
        msg = _( "Low morale causes the %{monster} to freeze in panic." );
        StringReplaceWithLowercase( msg, "%{monster}", b.GetName() );
        status.SetMessage( msg, true );
        RedrawTroopWithFrameAnimation( b, ICN::MORALEB, M82::BADMRLE, WINCE );
    }
}

void Battle::Interface::RedrawActionTowerPart1( const Tower & tower, const Unit & defender )
{
    Cursor::Get().SetThemes( Cursor::WAR_POINTER );
    _currentUnit = nullptr;

    const fheroes2::Point missileStart = tower.GetPortPosition();
    const fheroes2::Point targetPos = defender.GetCenterPoint();
    const double angle = GetAngle( missileStart, targetPos );

    AudioManager::PlaySound( M82::KEEPSHOT );

    // Keep missile == Orc missile
    RedrawMissileAnimation( missileStart, targetPos, angle, Monster::ORC );
}

void Battle::Interface::RedrawActionTowerPart2( const Tower & tower, const TargetInfo & target )
{
    TargetsInfo targets;
    targets.push_back( target );
    const bool isMirror = target.defender->isModes( CAP_MIRRORIMAGE );

    // targets damage animation
    RedrawActionWincesKills( targets );

    // draw status for first defender
    std::string msg = _( "%{tower} does %{damage} damage." );
    StringReplace( msg, "%{tower}", tower.GetName() );
    StringReplace( msg, "%{damage}", target.damage );
    if ( target.killed ) {
        msg += ' ';
        msg.append( _n( "1 %{defender} perishes.", "%{count} %{defender} perish.", target.killed ) );
        StringReplace( msg, "%{count}", target.killed );
        StringReplaceWithLowercase( msg, "%{defender}", target.defender->GetPluralName( target.killed ) );
    }

    if ( !isMirror ) {
        status.SetMessage( msg, true );
        status.SetMessage( "", false );
    }

    // TODO: remove this temporary assertion
    assert( _movingUnit == nullptr );
}

void Battle::Interface::RedrawActionCatapultPart1( const int catapultTargetId, const bool isHit )
{
    // Reset the delay before rendering the first frame of catapult animation.
    Game::AnimateResetDelay( Game::DelayType::BATTLE_CATAPULT_DELAY );

    LocalEvent & le = LocalEvent::Get();

    const fheroes2::Rect & area = GetArea();

    AudioManager::PlaySound( M82::CATSND00 );

    // catapult animation
    while ( le.HandleEvents( false ) && catapult_frame < 5 ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_CATAPULT_DELAY ) ) {
            Redraw();
            ++catapult_frame;
        }
    }

    // Reset the delay before rendering the first frame of catapult boulder animation.
    Game::AnimateResetDelay( Game::DelayType::BATTLE_CATAPULT_BOULDER_DELAY );

    // boulder animation
    fheroes2::Point pt1( 30, 290 );
    fheroes2::Point pt2 = Catapult::GetTargetPosition( catapultTargetId, isHit );
    const int32_t boulderArcStep = ( pt2.x - pt1.x ) / 30;

    // set the projectile arc height for each castle target and a formula for an unknown target
    int32_t boulderArcHeight;
    switch ( catapultTargetId ) {
    case Battle::CAT_WALL1:
        boulderArcHeight = 220;
        break;
    case Battle::CAT_WALL2:
    case Battle::CAT_BRIDGE:
        boulderArcHeight = 216;
        break;
    case Battle::CAT_WALL3:
        boulderArcHeight = 204;
        break;
    case Battle::CAT_WALL4:
        boulderArcHeight = 208;
        break;
    case Battle::CAT_TOWER1:
    case Battle::CAT_TOWER2:
        boulderArcHeight = 206;
        break;
    case Battle::CAT_CENTRAL_TOWER:
        boulderArcHeight = 290;
        break;
    default:
        boulderArcHeight = static_cast<int32_t>( std::lround( 0.55 * getDistance( pt1, pt2 ) ) );
    }

    pt1.x += area.x;
    pt2.x += area.x;
    pt1.y += area.y;
    pt2.y += area.y;

    const std::vector<fheroes2::Point> points = GetArcPoints( pt1, pt2, boulderArcHeight, boulderArcStep );
    std::vector<fheroes2::Point>::const_iterator pnt = points.begin();

    uint32_t boulderFrameId = 0;
    const uint32_t boulderFramesCount = fheroes2::AGG::GetICNCount( ICN::BOULDER );

    while ( le.HandleEvents( false ) && pnt != points.end() ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_CATAPULT_BOULDER_DELAY ) ) {
            if ( catapult_frame < 9 )
                ++catapult_frame;

            RedrawPartialStart();
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::BOULDER, boulderFrameId ), _mainSurface, pnt->x, pnt->y );
            RedrawPartialFinish();
            ++pnt;
            ++boulderFrameId;
            if ( boulderFrameId >= boulderFramesCount ) {
                boulderFrameId = 0;
            }
        }
    }

    // Reset the delay before rendering the catapult cloud.
    Game::AnimateResetDelay( Game::DelayType::BATTLE_CATAPULT_CLOUD_DELAY );

    // draw cloud
    const int32_t icn = isHit ? ICN::LICHCLOD : ICN::SMALCLOD;
    uint32_t frame = 0;
    // If the building is hit, end the animation on the 5th frame to change the building state (when the smoke cloud is largest).
    uint32_t maxFrame = isHit ? castleBuildingDestroyFrame : fheroes2::AGG::GetICNCount( icn );
    const bool isBridgeDestroyed = isHit && ( catapultTargetId == Battle::CAT_BRIDGE );
    // If the bridge is destroyed - prepare parameters for the second smoke cloud.
    if ( isBridgeDestroyed ) {
        pt1 = pt2 + bridgeDestroySmokeOffset;
        // Increase maxFrame to get bigger second smoke cloud.
        maxFrame = bridgeDestroyFrame;
    }

    AudioManager::PlaySound( M82::CATSND02 );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_CATAPULT_CLOUD_DELAY } ) ) && frame < maxFrame ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_CATAPULT_CLOUD_DELAY ) ) {
            RedrawPartialStart();
            // Start animation of the second smoke cloud only for the bridge and after 2 frames of the first smoke cloud.
            if ( isBridgeDestroyed && frame >= bridgeDestroySmokeDelay ) {
                const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, frame - bridgeDestroySmokeDelay );
                fheroes2::Blit( sprite, _mainSurface, pt1.x + sprite.x(), pt1.y + sprite.y() );
            }
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, frame );
            fheroes2::Blit( sprite, _mainSurface, pt2.x + sprite.x(), pt2.y + sprite.y() );
            RedrawPartialFinish();

            ++frame;
        }
    }

    if ( !isHit ) {
        catapult_frame = 0;
    }
}

void Battle::Interface::RedrawActionCatapultPart2( const int catapultTargetId )
{
    // Finish the smoke cloud animation after the building's state has changed after the hit and it is drawn as demolished.

    const fheroes2::Point pt1 = Catapult::GetTargetPosition( catapultTargetId, true ) + GetArea().getPosition();
    fheroes2::Point pt2;

    // Continue the smoke cloud animation from the 6th frame.
    const int32_t icnId = ICN::LICHCLOD;
    uint32_t frame = castleBuildingDestroyFrame;
    const uint32_t maxFrame = fheroes2::AGG::GetICNCount( icnId );
    uint32_t maxAnimationFrame = maxFrame;
    const bool isBridgeDestroyed = ( catapultTargetId == Battle::CAT_BRIDGE );
    // If the bridge is destroyed - prepare parameters for the second smoke cloud.
    if ( isBridgeDestroyed ) {
        pt2 = pt1 + bridgeDestroySmokeOffset;
        // Increase maxAnimationFrame to finish the second smoke animation.
        maxAnimationFrame += bridgeDestroySmokeDelay;
        // Bridge smoke animation should continue from the 7th frame.
        frame = bridgeDestroyFrame;
    }

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_CATAPULT_CLOUD_DELAY } ) ) && frame < maxAnimationFrame ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_CATAPULT_CLOUD_DELAY ) ) {
            RedrawPartialStart();
            // Draw animation of the second smoke cloud only for the bridge.
            if ( isBridgeDestroyed ) {
                const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icnId, frame - bridgeDestroySmokeDelay );
                fheroes2::Blit( sprite, _mainSurface, pt2.x + sprite.x(), pt2.y + sprite.y() );
            }
            // Don't draw the smoke cloud after its animation has ended (in case the second smoke cloud is still animating).
            if ( frame <= maxFrame ) {
                const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icnId, frame );
                fheroes2::Blit( sprite, _mainSurface, pt1.x + sprite.x(), pt1.y + sprite.y() );
            }
            RedrawPartialFinish();

            ++frame;
        }
    }
    catapult_frame = 0;
}

void Battle::Interface::RedrawActionArrowSpell( const Unit & target )
{
    const HeroBase * caster = arena.GetCurrentCommander();

    if ( caster ) {
        const fheroes2::Point missileStart = caster == opponent1->GetHero() ? opponent1->GetCastPosition() : opponent2->GetCastPosition();

        const fheroes2::Point targetPos = target.GetCenterPoint();
        const double angle = GetAngle( missileStart, targetPos );

        Cursor::Get().SetThemes( Cursor::WAR_POINTER );
        AudioManager::PlaySound( M82::MAGCAROW );

        // Magic arrow == Archer missile
        RedrawMissileAnimation( missileStart, targetPos, angle, Monster::ARCHER );
    }
}

void Battle::Interface::RedrawActionTeleportSpell( Unit & target, int32_t dst )
{
    LocalEvent & le = LocalEvent::Get();

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    uint8_t currentAlpha = target.GetCustomAlpha();
    const uint8_t alphaStep = 15;

    AudioManager::PlaySound( M82::TELPTOUT );

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( currentAlpha >= alphaStep && Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            currentAlpha -= alphaStep;
            target.SetCustomAlpha( currentAlpha );
            Redraw();
        }
    }

    currentAlpha = 0;
    Redraw();

    target.SetPosition( dst );
    AudioManager::PlaySound( M82::TELPTIN );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( currentAlpha <= ( 255 - alphaStep ) && Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            currentAlpha += alphaStep;
            target.SetCustomAlpha( currentAlpha );
            Redraw();
        }
    }

    target.SetCustomAlpha( 255 );
}

void Battle::Interface::RedrawActionSummonElementalSpell( Unit & target )
{
    LocalEvent & le = LocalEvent::Get();

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    uint8_t currentAlpha = 0;
    const uint8_t alphaStep = 20;

    AudioManager::PlaySound( M82::SUMNELM );

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && currentAlpha <= ( 255 - alphaStep ) ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            currentAlpha += alphaStep;
            target.SetCustomAlpha( currentAlpha );
            Redraw();
        }
    }

    target.SetCustomAlpha( 255 );
}

void Battle::Interface::RedrawActionMirrorImageSpell( const Unit & target, const Position & pos )
{
    LocalEvent & le = LocalEvent::Get();

    fheroes2::Sprite sprite = fheroes2::AGG::GetICN( target.GetMonsterSprite(), target.GetFrame() );
    fheroes2::ApplyPalette( sprite, PAL::GetPalette( PAL::PaletteType::MIRROR_IMAGE ) );

    const fheroes2::Rect & rt1 = target.GetRectPosition();
    const fheroes2::Rect & rt2 = pos.GetRect();

    const std::vector<fheroes2::Point> points = GetLinePoints( rt1.getPosition(), rt2.getPosition(), 5 );
    std::vector<fheroes2::Point>::const_iterator pnt = points.begin();

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );
    AudioManager::PlaySound( M82::MIRRORIM );

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && pnt != points.end() ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            const fheroes2::Point & sp = GetTroopPosition( target, sprite );

            RedrawPartialStart();
            fheroes2::Blit( sprite, _mainSurface, sp.x - rt1.x + ( *pnt ).x, sp.y - rt1.y + ( *pnt ).y, target.isReflect() );
            RedrawPartialFinish();

            ++pnt;
        }
    }

    status.SetMessage( _( "The mirror image is created." ), true );
}

void Battle::Interface::RedrawLightningOnTargets( const std::vector<fheroes2::Point> & points, const fheroes2::Rect & drawRoi )
{
    if ( points.size() < 2 )
        return;

    const fheroes2::Point roiOffset( drawRoi.x, drawRoi.y );

    LocalEvent & le = LocalEvent::Get();
    Cursor & cursor = Cursor::Get();
    cursor.SetThemes( Cursor::WAR_POINTER );

    AudioManager::PlaySound( points.size() > 2 ? M82::CHAINLTE : M82::LIGHTBLT );

    for ( size_t i = 1; i < points.size(); ++i ) {
        const fheroes2::Point & startingPos = points[i - 1];
        const fheroes2::Point & endPos = points[i];

        const std::vector<std::pair<LightningPoint, LightningPoint>> & lightningBolt = GenerateLightning( startingPos + roiOffset, endPos + roiOffset );
        fheroes2::Rect roi;
        const bool isHorizontalBolt = std::abs( startingPos.x - endPos.x ) > std::abs( startingPos.y - endPos.y );
        const bool isForwardDirection = isHorizontalBolt ? ( endPos.x > startingPos.x ) : ( endPos.y > startingPos.y );
        const int animationStep = 100;

        if ( isHorizontalBolt ) {
            roi.height = drawRoi.height;
            if ( isForwardDirection ) {
                roi.x = 0;
                roi.width = startingPos.x;
            }
            else {
                roi.x = startingPos.x;
                roi.width = drawRoi.width - startingPos.x;
            }
        }
        else {
            roi.width = drawRoi.width;
            if ( isForwardDirection ) {
                roi.y = 0;
                roi.height = startingPos.y;
            }
            else {
                roi.y = startingPos.y;
                roi.height = drawRoi.height - startingPos.y;
            }
        }

        while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_DISRUPTING_DELAY } ) )
                && ( ( isHorizontalBolt && roi.width < drawRoi.width ) || ( !isHorizontalBolt && roi.height < drawRoi.height ) ) ) {
            if ( Game::validateAnimationDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
                if ( isHorizontalBolt ) {
                    if ( isForwardDirection ) {
                        roi.width += animationStep;
                    }
                    else {
                        roi.width += animationStep;
                        roi.x -= animationStep;
                    }

                    if ( roi.x < 0 )
                        roi.x = 0;
                    if ( roi.width > drawRoi.width )
                        roi.width = drawRoi.width;
                }
                else {
                    if ( isForwardDirection ) {
                        roi.height += animationStep;
                    }
                    else {
                        roi.height += animationStep;
                        roi.y -= animationStep;
                    }

                    if ( roi.y < 0 )
                        roi.y = 0;
                    if ( roi.height > drawRoi.height )
                        roi.height = drawRoi.height;
                }

                RedrawPartialStart();

                RedrawLightning( lightningBolt, fheroes2::GetColorId( 0xff, 0xff, 0 ), _mainSurface,
                                 { roi.x + roiOffset.x, roi.y + roiOffset.y, roi.width, roi.height } );
                fheroes2::ApplyPalette( _mainSurface, 7 );

                RedrawPartialFinish();
            }
        }
    }

    // small delay to display fully drawn lightning
    fheroes2::delayforMs( 100 );

    uint32_t frame = 0;
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_DISRUPTING_DELAY } ) ) && frame < fheroes2::AGG::GetICNCount( ICN::SPARKS ) ) {
        CheckGlobalEvents( le );

        if ( ( frame == 0 ) || Game::validateAnimationDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
            RedrawPartialStart();

            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::SPARKS, frame );

            for ( size_t i = 1; i < points.size(); ++i ) {
                fheroes2::Point pt = points[i] - fheroes2::Point( sprite.width() / 2, 0 ) + roiOffset;
                fheroes2::Blit( sprite, _mainSurface, pt.x, pt.y );
            }
            RedrawPartialFinish();

            ++frame;
        }
    }
}

void Battle::Interface::RedrawActionLightningBoltSpell( const Unit & target )
{
    _currentUnit = nullptr;

    const fheroes2::Point startingPos = arena.GetCurrentCommander() == opponent1->GetHero() ? opponent1->GetCastPosition() : opponent2->GetCastPosition();
    const fheroes2::Rect & pos = target.GetRectPosition();
    const fheroes2::Point endPos( pos.x + pos.width / 2, pos.y );

    std::vector<fheroes2::Point> points;
    points.push_back( startingPos );
    points.push_back( endPos );

    RedrawLightningOnTargets( points, _surfaceInnerArea );
}

void Battle::Interface::RedrawActionChainLightningSpell( const TargetsInfo & targets )
{
    const fheroes2::Point startingPos = arena.GetCurrentCommander() == opponent1->GetHero() ? opponent1->GetCastPosition() : opponent2->GetCastPosition();
    std::vector<fheroes2::Point> points;
    points.push_back( startingPos );

    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it ) {
        const fheroes2::Rect & pos = it->defender->GetRectPosition();
        points.emplace_back( pos.x + pos.width / 2, pos.y );
    }

    RedrawLightningOnTargets( points, _surfaceInnerArea );
}

void Battle::Interface::RedrawActionBloodLustSpell( const Unit & target )
{
    LocalEvent & le = LocalEvent::Get();

    fheroes2::Sprite unitSprite = fheroes2::AGG::GetICN( target.GetMonsterSprite(), target.GetFrame() );

    std::vector<std::vector<uint8_t>> originalPalette;
    if ( target.Modes( SP_STONE ) ) {
        originalPalette.push_back( PAL::GetPalette( PAL::PaletteType::GRAY ) );
    }
    else if ( target.Modes( CAP_MIRRORIMAGE ) ) {
        originalPalette.push_back( PAL::GetPalette( PAL::PaletteType::MIRROR_IMAGE ) );
    }

    if ( !originalPalette.empty() ) {
        for ( size_t i = 1; i < originalPalette.size(); ++i ) {
            originalPalette[0] = PAL::CombinePalettes( originalPalette[0], originalPalette[i] );
        }
        fheroes2::ApplyPalette( unitSprite, originalPalette[0] );
    }

    std::vector<uint8_t> convert = PAL::GetPalette( PAL::PaletteType::RED );
    if ( !originalPalette.empty() ) {
        convert = PAL::CombinePalettes( PAL::GetPalette( PAL::PaletteType::GRAY ), convert );
    }

    fheroes2::Sprite bloodlustEffect( unitSprite );
    fheroes2::ApplyPalette( bloodlustEffect, convert );

    fheroes2::Sprite mixSprite( unitSprite );

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    _currentUnit = &target;
    b_current_sprite = &mixSprite;

    const uint32_t bloodlustDelay = 1800 / 20;
    // duration is 1900ms
    AudioManager::PlaySound( M82::BLOODLUS );

    uint32_t alpha = 0;
    uint32_t frame = 0;

    // Immediately indicate that the delay has passed to render first frame immediately.
    Game::passCustomAnimationDelay( bloodlustDelay );
    // Make sure that the first run is passed immediately.
    assert( !Game::isCustomDelayNeeded( bloodlustDelay ) );

    while ( le.HandleEvents( Game::isCustomDelayNeeded( bloodlustDelay ) ) && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( frame < 20 && Game::validateCustomAnimationDelay( bloodlustDelay ) ) {
            mixSprite = unitSprite;
            fheroes2::AlphaBlit( bloodlustEffect, mixSprite, static_cast<uint8_t>( alpha ) );
            Redraw();

            alpha += ( frame < 10 ) ? 20 : -20;
            ++frame;
        }
    }

    _currentUnit = nullptr;
    b_current_sprite = nullptr;
}

void Battle::Interface::RedrawActionStoneSpell( const Unit & target )
{
    LocalEvent & le = LocalEvent::Get();

    const fheroes2::Sprite & unitSprite = fheroes2::AGG::GetICN( target.GetMonsterSprite(), target.GetFrame() );

    fheroes2::Sprite stoneEffect( unitSprite );
    fheroes2::ApplyPalette( stoneEffect, PAL::GetPalette( PAL::PaletteType::GRAY ) );

    fheroes2::Sprite mixSprite( unitSprite );

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    _currentUnit = &target;
    b_current_sprite = &mixSprite;

    AudioManager::PlaySound( M82::PARALIZE );

    uint32_t alpha = 0;
    uint32_t frame = 0;
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( frame < 25 && Game::validateCustomAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            mixSprite = fheroes2::Sprite( unitSprite );
            fheroes2::AlphaBlit( stoneEffect, mixSprite, static_cast<uint8_t>( alpha ) );
            Redraw();

            alpha += 10;
            ++frame;
        }
    }

    _currentUnit = nullptr;
    b_current_sprite = nullptr;
}

void Battle::Interface::RedrawActionResurrectSpell( Unit & target, const Spell & spell )
{
    if ( !target.isValid() ) {
        // Restore direction of the creature, since it could be killed when it was reflected.
        target.UpdateDirection();

        Redraw();
        target.IncreaseAnimFrame();

        Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) ) {
            CheckGlobalEvents( le );

            if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
                if ( target.isFinishAnimFrame() ) {
                    // We have reached the end of animation.
                    break;
                }

                Redraw();
                target.IncreaseAnimFrame();
            }
        }
    }

    AudioManager::PlaySound( M82::FromSpell( spell.GetID() ) );

    RedrawTroopWithFrameAnimation( target, ICN::YINYANG, M82::UNKNOWN, target.GetHitPoints() == 0 ? RESURRECT : NONE );
}

void Battle::Interface::RedrawActionColdRaySpell( Unit & target )
{
    RedrawRaySpell( target, ICN::COLDRAY, M82::COLDRAY, 18 );
    RedrawTroopWithFrameAnimation( target, ICN::ICECLOUD, M82::UNKNOWN, NONE );
}

void Battle::Interface::RedrawRaySpell( const Unit & target, int spellICN, int spellSound, int32_t size )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    // Casting hero position
    const fheroes2::Point startingPos = arena.GetCurrentCommander() == opponent1->GetHero() ? opponent1->GetCastPosition() : opponent2->GetCastPosition();
    const fheroes2::Point targetPos = target.GetCenterPoint();

    const std::vector<fheroes2::Point> path = GetEuclideanLine( startingPos, targetPos, size );
    const uint32_t spriteCount = fheroes2::AGG::GetICNCount( spellICN );

    cursor.SetThemes( Cursor::WAR_POINTER );
    AudioManager::PlaySound( spellSound );

    size_t i = 0;
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_DISRUPTING_DELAY } ) ) && i < path.size() ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
            const uint32_t frame = static_cast<uint32_t>( i * spriteCount / path.size() ); // it's safe to do such as i <= path.size()
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( spellICN, frame );
            fheroes2::Blit( sprite, _mainSurface, path[i].x - sprite.width() / 2, path[i].y - sprite.height() / 2 );
            RedrawPartialFinish();
            ++i;
        }
    }
}

void Battle::Interface::RedrawActionDisruptingRaySpell( const Unit & target )
{
    LocalEvent & le = LocalEvent::Get();

    RedrawRaySpell( target, ICN::DISRRAY, M82::DISRUPTR, 24 );

    // Part 2 - ripple effect
    const fheroes2::Sprite & unitSprite = fheroes2::AGG::GetICN( target.GetMonsterSprite(), target.GetFrame() );
    fheroes2::Sprite rippleSprite;

    const Unit * old_current = _currentUnit;
    _currentUnit = &target;
    _movingPos = { 0, 0 };

    uint32_t frame = 0;
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_DISRUPTING_DELAY } ) ) && frame < 60 ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
            rippleSprite = fheroes2::CreateRippleEffect( unitSprite, frame );
            rippleSprite.setPosition( unitSprite.x(), unitSprite.y() );

            b_current_sprite = &rippleSprite;
            Redraw();

            frame += 2;
        }
    }

    _currentUnit = old_current;
    b_current_sprite = nullptr;
}

void Battle::Interface::RedrawActionDeathWaveSpell( const int32_t strength )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    _currentUnit = nullptr;
    cursor.SetThemes( Cursor::WAR_POINTER );

    // Set all non-dead troops animation to static and redraw the '_mainSurface'.
    SwitchAllUnitsAnimation( Monster_Info::STATIC );
    Redraw();

    fheroes2::Rect area = GetArea();
    // Cut out the battle log image so we don't use it in the death wave effect.
    area.height -= status.height;
    // And if listlog is open, then cut off it too.
    if ( listlog && listlog->isOpenLog() ) {
        area.height -= listlog->GetArea().height;
    }

    fheroes2::Image battleFieldCopy( area.width, area.height );
    fheroes2::Copy( _mainSurface, 0, 0, battleFieldCopy, 0, 0, area.width, area.height );

    // The death wave horizontal length in pixels.
    const int32_t waveLength = 38;
    const int32_t waveStep = 5;
    // A death wave parameter that limits the curve to one cosine period.
    const double waveLimit = waveLength / M_PI / 2;
    std::vector<int32_t> deathWaveCurve;
    deathWaveCurve.reserve( waveLength );

    // Calculate the "Death Wave" curve as one period of cosine, which starts from 0 with an amplitude of 1/2 and shifted down by 0.5.
    // So we get a smooth hill, which is the multiplied with 'strength' and shifted after that by -1px.
    // (The "Death Wave" curve has to shift the image and cosine starts from 0 so we add extra 1px).
    for ( int32_t posX = 0; posX < waveLength; ++posX ) {
        deathWaveCurve.push_back( static_cast<int32_t>( std::round( strength * ( cos( posX / waveLimit ) / 2 - 0.5 ) ) ) - 1 );
    }

    // Take into account that the Death Wave starts outside the battle screen.
    area.x -= waveLength;
    // The first frame of spell effect must have a part of the spell, so we start from its first position.
    int32_t position = waveStep;
    fheroes2::Display & display = fheroes2::Display::instance();

    // Prepare the blank image for the Death Wave spell effect with the transform layer equal to "0"
    fheroes2::Image spellEffect( waveLength, area.height );
    std::fill( spellEffect.transform(), spellEffect.transform() + static_cast<size_t>( waveLength * area.height ), static_cast<uint8_t>( 0 ) );

    AudioManager::PlaySound( M82::MNRDEATH );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_DISRUPTING_DELAY } ) ) && position < area.width + waveLength ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
            const int32_t wavePositionX = ( area.x + position < 0 ) ? 0 : ( area.x + position );
            const int32_t waveWidth = position > waveLength ? ( position > area.width ? ( waveLength - position + area.width ) : waveLength ) : position;
            const int32_t restorePositionX = ( wavePositionX < waveStep ) ? 0 : ( wavePositionX - waveStep );
            const int32_t restoreWidth = wavePositionX < waveStep ? wavePositionX : waveStep;

            const fheroes2::Rect renderArea( _interfacePosition.x + restorePositionX, _interfacePosition.y + area.y, waveWidth + restoreWidth, area.height );

            // Place a copy of the original image where the Death Wave effect was on the previous frame.
            fheroes2::Blit( battleFieldCopy, restorePositionX, area.y, display, renderArea.x, renderArea.y, restoreWidth, renderArea.height );

            // Place the Death Wave effect to its new position.
            fheroes2::CreateDeathWaveEffect( spellEffect, battleFieldCopy, position, deathWaveCurve );
            fheroes2::Blit( spellEffect, 0, 0, display, renderArea.x + restoreWidth, renderArea.y, waveWidth, area.height );

            // Render only the changed screen area.
            display.render( renderArea );

            position += waveStep;
        }
    }

    SwitchAllUnitsAnimation( Monster_Info::STATIC );
}

void Battle::Interface::RedrawActionColdRingSpell( int32_t dst, const TargetsInfo & targets )
{
    LocalEvent & le = LocalEvent::Get();

    const int icn = ICN::COLDRING;
    const int m82 = M82::FromSpell( Spell::COLDRING );
    uint32_t frame = 0;
    const fheroes2::Rect & center = Board::GetCell( dst )->GetPos();

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    // set WNCE
    _currentUnit = nullptr;
    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
        if ( ( *it ).defender && ( *it ).damage )
            ( *it ).defender->SwitchAnimation( Monster_Info::WNCE );

    AudioManager::PlaySound( m82 );

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && frame < fheroes2::AGG::GetICNCount( icn ) ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            RedrawPartialStart();

            const fheroes2::Sprite & sprite1 = fheroes2::AGG::GetICN( icn, frame );
            fheroes2::Blit( sprite1, _mainSurface, center.x + center.width / 2 + sprite1.x(), center.y + center.height / 2 + sprite1.y() );
            const fheroes2::Sprite & sprite2 = fheroes2::AGG::GetICN( icn, frame );
            fheroes2::Blit( sprite2, _mainSurface, center.x + center.width / 2 - sprite2.width() - sprite2.x(), center.y + center.height / 2 + sprite2.y(), true );
            RedrawPartialFinish();

            for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
                if ( ( *it ).defender && ( *it ).damage )
                    ( *it ).defender->IncreaseAnimFrame( false );
            ++frame;
        }
    }

    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
        if ( ( *it ).defender ) {
            ( *it ).defender->SwitchAnimation( Monster_Info::STATIC );
            _currentUnit = nullptr;
        }
}

void Battle::Interface::RedrawActionHolyShoutSpell( const uint8_t strength )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.SetThemes( Cursor::WAR_POINTER );

    // Set all non-dead troops animation to static and redraw the '_mainSurface'.
    SwitchAllUnitsAnimation( Monster_Info::STATIC );
    Redraw();

    fheroes2::Rect area = GetArea();
    // Cut out the battle log image so we don't use it in the death wave effect.
    area.height -= status.height;
    // And if listlog is open, then cut off it too.
    if ( listlog && listlog->isOpenLog() ) {
        area.height -= listlog->GetArea().height;
    }

    fheroes2::Image battleFieldCopy( area.width, area.height );
    fheroes2::Copy( _mainSurface, 0, 0, battleFieldCopy, 0, 0, area.width, area.height );

    _currentUnit = nullptr;

    const uint32_t maxFrame = 20;
    const uint32_t halfMaxFrame = maxFrame / 2;

    // A vector of frames to animate the increase of the spell effect. The decrease will be shown in reverse frames order.
    // Initialize a vector with copies of battle field to use them in making the spell effect increase animation.
    std::vector<fheroes2::Image> spellEffect;
    static_assert( halfMaxFrame > 1 );
    spellEffect.reserve( halfMaxFrame );

    const uint32_t spellEffectLastFrame = halfMaxFrame - 1;

    // The similar frames number is smaller than size by 1 as the last frame will be different.
    while ( spellEffect.size() < spellEffectLastFrame ) {
        spellEffect.push_back( battleFieldCopy );
    }

    // The last frame is the full power of spell effect. It will be used to produce other frames.
    spellEffect.emplace_back( fheroes2::CreateHolyShoutEffect( battleFieldCopy, 4, strength ) );

    const uint32_t spellcastDelay = Game::ApplyBattleSpeed( 3000 ) / maxFrame;
    uint32_t frame = 0;
    uint8_t alpha = 30;
    const uint8_t alphaStep = 25;

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Rect renderArea( _interfacePosition.x + area.x, _interfacePosition.y + area.y, area.width, area.height );

    // Immediately indicate that the delay has passed to render first frame immediately.
    Game::passCustomAnimationDelay( spellcastDelay );
    // Make sure that the first run is passed immediately.
    assert( !Game::isCustomDelayNeeded( spellcastDelay ) );

    AudioManager::PlaySound( M82::MASSCURS );

    while ( le.HandleEvents( Game::isCustomDelayNeeded( spellcastDelay ) ) && frame < maxFrame ) {
        CheckGlobalEvents( le );

        if ( Game::validateCustomAnimationDelay( spellcastDelay ) ) {
            // Display the maximum spell effect for 1 more 'spellcastDelay' without rendering a frame.
            if ( frame != halfMaxFrame ) {
                // If the spell effect is increasing we generate the frame for it in the vector to use it later in decreasing animation.
                if ( frame < spellEffectLastFrame ) {
                    fheroes2::AlphaBlit( spellEffect[spellEffectLastFrame], spellEffect[frame], alpha );
                    alpha += alphaStep;
                }

                const uint32_t spellEffectFrame = ( frame < halfMaxFrame ) ? frame : ( maxFrame - frame - 1 );
                fheroes2::Copy( spellEffect[spellEffectFrame], area.x, area.y, display, renderArea.x, renderArea.y, renderArea.width, renderArea.height );

                display.render( renderArea );
            }

            ++frame;
        }
    }
}

void Battle::Interface::RedrawActionElementalStormSpell( const TargetsInfo & targets )
{
    LocalEvent & le = LocalEvent::Get();

    const int icn = ICN::STORM;
    const int m82 = M82::FromSpell( Spell::ELEMENTALSTORM );
    const int spriteSize = 54;
    const uint32_t icnCount = fheroes2::AGG::GetICNCount( icn );

    std::vector<fheroes2::Sprite> spriteCache;
    spriteCache.reserve( icnCount );
    for ( uint32_t i = 0; i < icnCount; ++i ) {
        spriteCache.push_back( fheroes2::AGG::GetICN( icn, i ) );
    }

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    _currentUnit = nullptr;
    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
        if ( ( *it ).defender && ( *it ).damage )
            ( *it ).defender->SwitchAnimation( Monster_Info::WNCE );

    AudioManager::PlaySound( m82 );

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    uint32_t frame = 0;
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && frame < 60 ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            RedrawPartialStart();

            if ( icnCount > 0 ) {
                for ( int x = 0; x * spriteSize < _surfaceInnerArea.width; ++x ) {
                    const int idX = frame + x * 3;
                    const int offsetX = x * spriteSize;
                    for ( int y = 0; y * spriteSize < _surfaceInnerArea.height; ++y ) {
                        const fheroes2::Sprite & sprite = spriteCache[( idX + y ) % icnCount];
                        fheroes2::Blit( sprite, _mainSurface, offsetX + sprite.x(), y * spriteSize + sprite.y() );
                    }
                }
            }

            RedrawPartialFinish();

            for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
                if ( ( *it ).defender && ( *it ).damage )
                    ( *it ).defender->IncreaseAnimFrame( false );
            ++frame;
        }
    }

    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
        if ( ( *it ).defender ) {
            ( *it ).defender->SwitchAnimation( Monster_Info::STATIC );
            _currentUnit = nullptr;
        }
}

void Battle::Interface::RedrawActionArmageddonSpell()
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    fheroes2::Rect area = GetArea();

    area.height -= 37;

    fheroes2::Image spriteWhitening( area.width, area.height );
    fheroes2::Image spriteReddish( area.width, area.height );
    fheroes2::Copy( _mainSurface, area.x, area.y, spriteWhitening, 0, 0, area.width, area.height );
    fheroes2::Copy( _mainSurface, area.x, area.y, spriteReddish, 0, 0, area.width, area.height );

    cursor.SetThemes( Cursor::WAR_POINTER );

    _currentUnit = nullptr;
    AudioManager::PlaySound( M82::ARMGEDN );
    uint32_t alpha = 10;

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && alpha < 100 ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            fheroes2::ApplyPalette( spriteWhitening, 9 );
            fheroes2::Blit( spriteWhitening, _mainSurface, area.x, area.y );
            RedrawPartialFinish();

            alpha += 10;
        }
    }

    fheroes2::ApplyPalette( spriteReddish, PAL::GetPalette( PAL::PaletteType::RED ) );
    fheroes2::Copy( spriteReddish, 0, 0, _mainSurface, area.x, area.y, area.width, area.height );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            const int32_t offsetX = static_cast<int32_t>( Rand::Get( 0, 14 ) ) - 7;
            const int32_t offsetY = static_cast<int32_t>( Rand::Get( 0, 14 ) ) - 7;
            const fheroes2::Rect initialArea( area );
            fheroes2::Rect original = initialArea ^ fheroes2::Rect( area.x + offsetX, area.y + offsetY, area.width, area.height );

            fheroes2::Rect shifted( initialArea.x - original.x, initialArea.y - original.y, original.width, original.height );
            if ( shifted.x < 0 ) {
                const int32_t offset = -shifted.x;
                shifted.x = 0;
                original.x += offset;
                shifted.width -= offset;
                shifted.x = 0;
            }
            if ( shifted.y < 0 ) {
                const int32_t offset = -shifted.y;
                shifted.y = 0;
                original.y += offset;
                shifted.height -= offset;
                shifted.y = 0;
            }
            fheroes2::Blit( spriteReddish, shifted.x, shifted.y, _mainSurface, original.x, original.y, shifted.width, shifted.height );

            RedrawPartialFinish();
        }
    }
}

void Battle::Interface::RedrawActionEarthQuakeSpell( const std::vector<int> & targets )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    fheroes2::Rect area = GetArea();

    uint32_t frame = 0;
    area.height -= 38;

    cursor.SetThemes( Cursor::WAR_POINTER );

    fheroes2::Image sprite( area.width, area.height );
    fheroes2::Copy( _mainSurface, area.x, area.y, sprite, 0, 0, area.width, area.height );

    _currentUnit = nullptr;
    AudioManager::PlaySound( M82::ERTHQUAK );

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    // draw earth quake
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && frame < 18 ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            const int32_t offsetX = static_cast<int32_t>( Rand::Get( 0, 14 ) ) - 7;
            const int32_t offsetY = static_cast<int32_t>( Rand::Get( 0, 14 ) ) - 7;
            const fheroes2::Rect initialArea( area );
            fheroes2::Rect original = initialArea ^ fheroes2::Rect( area.x + offsetX, area.y + offsetY, area.width, area.height );

            fheroes2::Rect shifted( initialArea.x - original.x, initialArea.y - original.y, original.width, original.height );
            if ( shifted.x < 0 ) {
                const int32_t offset = -shifted.x;
                shifted.x = 0;
                original.x += offset;
                shifted.width -= offset;
                shifted.x = 0;
            }
            if ( shifted.y < 0 ) {
                const int32_t offset = -shifted.y;
                shifted.y = 0;
                original.y += offset;
                shifted.height -= offset;
                shifted.y = 0;
            }

            fheroes2::Blit( sprite, shifted.x, shifted.y, _mainSurface, original.x, original.y, shifted.width, shifted.height );

            RedrawPartialFinish();
            ++frame;
        }
    }

    // draw cloud
    const int icn = ICN::LICHCLOD;
    frame = 0;

    AudioManager::PlaySound( M82::CATSND02 );

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && frame < fheroes2::AGG::GetICNCount( icn ) ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            RedrawPartialStart();

            for ( std::vector<int>::const_iterator it = targets.begin(); it != targets.end(); ++it ) {
                fheroes2::Point pt2 = Catapult::GetTargetPosition( *it, true );

                pt2.x += area.x;
                pt2.y += area.y;

                const fheroes2::Sprite & spriteCloud = fheroes2::AGG::GetICN( icn, frame );
                fheroes2::Blit( spriteCloud, _mainSurface, pt2.x + spriteCloud.x(), pt2.y + spriteCloud.y() );
            }

            RedrawPartialFinish();

            ++frame;
        }
    }
}

void Battle::Interface::RedrawActionRemoveMirrorImage( const std::vector<Unit *> & mirrorImages )
{
    if ( mirrorImages.empty() ) // nothing to animate
        return;

    // Reset the delay to wait till the next frame if is not already waiting.
    if ( !Game::isDelayNeeded( { Game::DelayType::BATTLE_FRAME_DELAY } ) ) {
        Game::AnimateResetDelay( Game::DelayType::BATTLE_FRAME_DELAY );
    }

    LocalEvent & le = LocalEvent::Get();

    uint8_t frameId = 10;
    const uint8_t alphaStep = 25;

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_FRAME_DELAY } ) ) && frameId > 0 ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_FRAME_DELAY ) ) {
            const uint8_t alpha = frameId * alphaStep;
            for ( Unit * unit : mirrorImages ) {
                if ( unit != nullptr ) {
                    unit->SetCustomAlpha( alpha );
                }
            }

            Redraw();

            --frameId;
        }
    }
    status.SetMessage( _( "The mirror image is destroyed!" ), true );
}

void Battle::Interface::RedrawTargetsWithFrameAnimation( int32_t dst, const TargetsInfo & targets, int icn, int m82, int repeatCount )
{
    LocalEvent & le = LocalEvent::Get();

    uint32_t frame = 0;
    const fheroes2::Rect & center = Board::GetCell( dst )->GetPos();

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    _currentUnit = nullptr;
    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
        if ( ( *it ).defender && ( *it ).damage )
            ( *it ).defender->SwitchAnimation( Monster_Info::WNCE );

    AudioManager::PlaySound( m82 );

    uint32_t frameCount = fheroes2::AGG::GetICNCount( icn );

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && frame < frameCount ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            RedrawPartialStart();

            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, frame );
            fheroes2::Blit( sprite, _mainSurface, center.x + center.width / 2 + sprite.x(), center.y + center.height / 2 + sprite.y() );
            RedrawPartialFinish();

            for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
                if ( ( *it ).defender && ( *it ).damage )
                    ( *it ).defender->IncreaseAnimFrame( false );
            ++frame;

            if ( frame == frameCount && repeatCount > 0 ) {
                --repeatCount;
                frame = 0;
            }
        }
    }

    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
        if ( ( *it ).defender ) {
            ( *it ).defender->SwitchAnimation( Monster_Info::STATIC );
            _currentUnit = nullptr;
        }
}

void Battle::Interface::RedrawTargetsWithFrameAnimation( const TargetsInfo & targets, int icn, int m82, bool wnce )
{
    LocalEvent & le = LocalEvent::Get();

    // A vector for creatures made by Mirror Image spell which will be destroyed by current spell.
    std::vector<Unit *> mirrorImages;

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    _currentUnit = nullptr;

    if ( wnce ) {
        int32_t deathColor = Color::UNUSED;

        for ( const TargetInfo & target : targets ) {
            Unit * defender = target.defender;
            if ( defender == nullptr ) {
                continue;
            }

            if ( defender->isModes( CAP_MIRRORIMAGE ) ) {
                mirrorImages.push_back( defender );
            }

            // If the creature was killed set its death animation.
            if ( !defender->isValid() ) {
                // Destroy linked mirror to a dead creature.
                if ( defender->isModes( CAP_MIRROROWNER ) ) {
                    mirrorImages.push_back( defender->GetMirror() );
                }

                defender->SwitchAnimation( Monster_Info::KILL );
                AudioManager::PlaySound( defender->M82Kill() );

                // Set the color of the dead creature to tell heroes about it.
                deathColor = defender->GetArmyColor();
            }
            // If the creature is damaged but is still alive set its wince animation.
            else if ( target.damage ) {
                defender->SwitchAnimation( Monster_Info::WNCE_UP );
                AudioManager::PlaySound( defender->M82Wnce() );
            }

            SetHeroAnimationReactionToTroopDeath( deathColor );
        }
    }

    // For certain spells reflect the spell sprite if the creature is reflected.
    const bool isReflectICN = ( icn == ICN::SHIELD || icn == ICN::REDDEATH || icn == ICN::MAGIC08 );

    size_t overlaySpriteCount = _unitSpellEffectInfos.size();
    _unitSpellEffectInfos.reserve( overlaySpriteCount + targets.size() );

    for ( const Battle::TargetInfo & target : targets ) {
        if ( target.defender ) {
            _unitSpellEffectInfos.emplace_back( target.defender->GetUID(), icn, ( isReflectICN && target.defender->isReflect() ) );
        }
    }

    overlaySpriteCount = _unitSpellEffectInfos.size() - overlaySpriteCount;
    const std::vector<Battle::UnitSpellEffectInfo>::iterator overlaySpriteEnd = _unitSpellEffectInfos.end();
    const std::vector<Battle::UnitSpellEffectInfo>::iterator overlaySpriteBegin = overlaySpriteEnd - static_cast<ptrdiff_t>( overlaySpriteCount );

    // Set the defender wince animation state.
    bool isDefenderAnimating = wnce;
    const uint32_t maxFrame = fheroes2::AGG::GetICNCount( icn );
    uint32_t frame = 0;

    // Wait for previously set and not passed delays before rendering a new frame.
    WaitForAllActionDelays();

    AudioManager::PlaySound( m82 );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && ( frame < maxFrame || isDefenderAnimating ) ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            if ( frame < maxFrame ) {
                std::vector<Battle::UnitSpellEffectInfo>::iterator overlaySpriteIter = overlaySpriteBegin;
                const fheroes2::Sprite & spellSprite = fheroes2::AGG::GetICN( icn, frame );

                for ( const Battle::TargetInfo & target : targets ) {
                    if ( target.defender ) {
                        overlaySpriteIter->position = CalculateSpellPosition( *target.defender, icn, spellSprite );
                        overlaySpriteIter->icnIndex = frame;

                        ++overlaySpriteIter;
                    }
                }
            }

            Redraw();

            // Reset the defender wince animation state.
            isDefenderAnimating = false;

            if ( wnce ) {
                for ( const TargetInfo & target : targets ) {
                    if ( target.defender == nullptr ) {
                        continue;
                    }

                    // Fully animate creature death.
                    if ( !target.defender->isValid() ) {
                        target.defender->IncreaseAnimFrame( false );

                        // If the death animation is still in process then set isDefenderAnimating to false.
                        isDefenderAnimating |= !target.defender->isFinishAnimFrame();
                    }
                    else if ( target.damage ) {
                        // If the target has taken damage and is not killed.
                        // Check if the current animation is not finished.
                        if ( !target.defender->isFinishAnimFrame() ) {
                            target.defender->IncreaseAnimFrame( false );
                        }
                        else if ( frame >= ( maxFrame - 1 ) && target.defender->GetAnimationState() == Monster_Info::WNCE_UP ) {
                            // If the main spell sprite animation and WNCE_UP are finished then switch unit animation to WNCE_DOWN.
                            target.defender->SwitchAnimation( Monster_Info::WNCE_DOWN );
                        }
                        else if ( target.defender->GetAnimationState() == Monster_Info::WNCE_DOWN ) {
                            // If the WNCE_DOWN animation is finished, then set STATIC animation to the target.
                            target.defender->SwitchAnimation( Monster_Info::STATIC );
                        }

                        // If not all damaged (and not killed) units are set to STATIC animation then set isDefenderAnimating to false.
                        // IMPORTANT: The game engine can change STATIC animation to IDLE, especially for Ghosts and Zombies,
                        // so we need to check IDLE where we check for STATIC.
                        const int unitAnimState = target.defender->GetAnimationState();
                        isDefenderAnimating |= ( unitAnimState != Monster_Info::STATIC ) && ( unitAnimState != Monster_Info::IDLE );
                    }
                }
            }

            ++frame;

            // Remove all overlay sprites when the animation is finished.
            if ( frame == maxFrame ) {
                _unitSpellEffectInfos.erase( overlaySpriteBegin, overlaySpriteEnd );
            }
        }
    }

    // TODO: When there is a need to display some permanent effects with '_unitSpellEffectInfos' - remove this assertion.
    assert( _unitSpellEffectInfos.empty() );

    if ( !mirrorImages.empty() ) {
        // Fade away animation for destroyed mirror images.
        RedrawActionRemoveMirrorImage( mirrorImages );
    }
}

void Battle::Interface::RedrawTroopWithFrameAnimation( Unit & unit, int icn, int m82, CreatureSpellAnimation animation )
{
    LocalEvent & le = LocalEvent::Get();

    uint32_t frame = 0;
    const bool reflect = ( icn == ICN::SHIELD && unit.isReflect() );

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    if ( animation == WINCE ) {
        _currentUnit = nullptr;
        unit.SwitchAnimation( Monster_Info::WNCE_UP );
    }
    else if ( animation == RESURRECT ) {
        _currentUnit = nullptr;
        unit.SwitchAnimation( Monster_Info::KILL, true );
    }

    _unitSpellEffectInfos.emplace_back( unit.GetUID(), icn, reflect );

    // Wait for previously set and not passed delays before rendering a new frame.
    WaitForAllActionDelays();

    const uint32_t maxICNFrame = fheroes2::AGG::GetICNCount( icn );

    AudioManager::PlaySound( m82 );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && ( frame < maxICNFrame || unit.GetAnimationState() == Monster_Info::WNCE_DOWN ) ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            if ( frame < maxICNFrame ) {
                _unitSpellEffectInfos.back().position = CalculateSpellPosition( unit, icn, fheroes2::AGG::GetICN( icn, frame ) );
                _unitSpellEffectInfos.back().icnIndex = frame;
            }
            Redraw();

            if ( animation != NONE ) {
                if ( ( animation == RESURRECT || unit.GetAnimationState() == Monster_Info::WNCE_DOWN ) && unit.isFinishAnimFrame() ) {
                    unit.SwitchAnimation( Monster_Info::STAND_STILL );
                }
                unit.IncreaseAnimFrame( false );
                if ( frame == maxICNFrame - 1 && animation == WINCE ) {
                    unit.SwitchAnimation( Monster_Info::WNCE_DOWN );
                }
            }
            ++frame;
            if ( frame == maxICNFrame ) {
                // Spell animation is finished, so delete the overlay sprite from unit.
                _unitSpellEffectInfos.pop_back();
            }
        }
    }

    // TODO: When there is a need to display some permanent effects with '_unitSpellEffectInfos' - remove this assertion.
    assert( _unitSpellEffectInfos.empty() );

    if ( animation != NONE ) {
        unit.SwitchAnimation( Monster_Info::STATIC );
        _currentUnit = nullptr;
    }
}

void Battle::Interface::RedrawBridgeAnimation( const bool bridgeDownAnimation )
{
    // Wait for previously set and not passed delays before rendering a new frame.
    WaitForAllActionDelays();

    LocalEvent & le = LocalEvent::Get();

    _bridgeAnimation.animationIsRequired = true;

    _bridgeAnimation.currentFrameId = bridgeDownAnimation ? BridgeMovementAnimation::UP_POSITION : BridgeMovementAnimation::DOWN_POSITION;

    if ( bridgeDownAnimation )
        AudioManager::PlaySound( M82::DRAWBRG );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_BRIDGE_DELAY } ) ) ) {
        if ( bridgeDownAnimation ) {
            if ( _bridgeAnimation.currentFrameId < BridgeMovementAnimation::DOWN_POSITION )
                break;
        }
        else {
            if ( _bridgeAnimation.currentFrameId > BridgeMovementAnimation::UP_POSITION )
                break;
        }

        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_BRIDGE_DELAY ) ) {
            Redraw();

            if ( bridgeDownAnimation )
                --_bridgeAnimation.currentFrameId;
            else
                ++_bridgeAnimation.currentFrameId;
        }
    }

    _bridgeAnimation.animationIsRequired = false;

    if ( !bridgeDownAnimation )
        AudioManager::PlaySound( M82::DRAWBRG );
}

bool Battle::Interface::IdleTroopsAnimation() const
{
    if ( Game::validateAnimationDelay( Game::BATTLE_IDLE_DELAY ) ) {
        const bool redrawNeeded = arena.GetForce1().animateIdleUnits();
        return arena.GetForce2().animateIdleUnits() || redrawNeeded;
    }

    return false;
}

void Battle::Interface::ResetIdleTroopAnimation() const
{
    arena.GetForce1().resetIdleAnimation();
    arena.GetForce2().resetIdleAnimation();
}

void Battle::Interface::SwitchAllUnitsAnimation( const int32_t animationState ) const
{
    for ( Battle::Unit * unit : arena.GetForce1() ) {
        if ( unit->isValid() ) {
            unit->SwitchAnimation( animationState );
        }
    }
    for ( Battle::Unit * unit : arena.GetForce2() ) {
        if ( unit->isValid() ) {
            unit->SwitchAnimation( animationState );
        }
    }
}

void Battle::Interface::CheckGlobalEvents( LocalEvent & le )
{
    // Animation of the currently active unit's contour
    if ( Game::validateAnimationDelay( Game::BATTLE_SELECTED_UNIT_DELAY ) ) {
        UpdateContourColor();
    }

    // Animation of heroes
    if ( Game::validateAnimationDelay( Game::BATTLE_OPPONENTS_DELAY ) ) {
        if ( opponent1 && opponent1->updateAnimationState() ) {
            humanturn_redraw = true;
        }

        if ( opponent2 && opponent2->updateAnimationState() ) {
            humanturn_redraw = true;
        }
    }

    // Animation of flags
    if ( Game::validateAnimationDelay( Game::BATTLE_FLAGS_DELAY ) ) {
        ++animation_flags_frame;
        humanturn_redraw = true;
    }

    // Interrupting auto battle
    if ( arena.AutoBattleInProgress() && arena.CanToggleAutoBattle()
         && ( le.MouseClickLeft( btn_auto.area() )
              || ( le.KeyPress()
                   && ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_AUTO_SWITCH )
                        || ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL )
                             && fheroes2::showMessage( fheroes2::Text( "", {} ),
                                                       fheroes2::Text( _( "Are you sure you want to interrupt the auto battle?" ), fheroes2::FontType::normalWhite() ),
                                                       Dialog::YES | Dialog::NO )
                                    == Dialog::YES ) ) ) ) ) {
        _interruptAutoBattleForColor = arena.GetCurrentColor();
    }
}

void Battle::Interface::ProcessingHeroDialogResult( int res, Actions & a )
{
    switch ( res ) {
    // cast
    case 1: {
        const HeroBase * hero = _currentUnit->GetCurrentOrArmyCommander();

        if ( hero ) {
            if ( hero->HaveSpellBook() ) {
                std::string msg;

                if ( arena.isDisableCastSpell( Spell::NONE, &msg ) ) {
                    Dialog::Message( "", msg, Font::BIG, Dialog::OK );
                }
                else {
                    std::function<void( const std::string & )> statusCallback = [this]( const std::string & statusStr ) {
                        status.SetMessage( statusStr );
                        status.Redraw( fheroes2::Display::instance() );
                    };

                    const Spell spell = hero->OpenSpellBook( SpellBook::Filter::CMBT, true, true, &statusCallback );
                    if ( spell.isValid() ) {
                        assert( spell.isCombat() );

                        if ( arena.isDisableCastSpell( spell, &msg ) ) {
                            Dialog::Message( "", msg, Font::BIG, Dialog::OK );
                        }
                        else {
                            std::string error;

                            if ( hero->CanCastSpell( spell, &error ) ) {
                                if ( spell.isApplyWithoutFocusObject() ) {
                                    a.emplace_back( CommandType::MSG_BATTLE_CAST, spell.GetID(), -1 );

                                    humanturn_redraw = true;
                                    humanturn_exit = true;
                                }
                                else {
                                    humanturn_spell = spell;
                                }
                            }
                            else {
                                Dialog::Message( _( "Error" ), error, Font::BIG, Dialog::OK );
                            }
                        }
                    }
                }
            }
            else
                Dialog::Message( "", _( "No spells to cast." ), Font::BIG, Dialog::OK );
        }
        break;
    }

    // retreat
    case 2: {
        if ( arena.CanRetreatOpponent( _currentUnit->GetCurrentOrArmyColor() ) ) {
            if ( Dialog::YES == Dialog::Message( "", _( "Are you sure you want to retreat?" ), Font::BIG, Dialog::YES | Dialog::NO ) ) {
                a.emplace_back( CommandType::MSG_BATTLE_RETREAT );
                a.emplace_back( CommandType::MSG_BATTLE_END_TURN, _currentUnit->GetUID() );
                humanturn_exit = true;
            }
        }
        else {
            Dialog::Message( "", _( "Retreat disabled" ), Font::BIG, Dialog::OK );
        }
        break;
    }

    // surrender
    case 3: {
        if ( arena.CanSurrenderOpponent( _currentUnit->GetCurrentOrArmyColor() ) ) {
            const HeroBase * enemy = arena.getEnemyCommander( arena.GetCurrentColor() );

            if ( enemy ) {
                const int32_t cost = arena.GetCurrentForce().GetSurrenderCost();
                Kingdom & kingdom = world.GetKingdom( arena.GetCurrentColor() );

                if ( DialogBattleSurrender( *enemy, cost, kingdom ) ) {
                    a.emplace_back( CommandType::MSG_BATTLE_SURRENDER );
                    a.emplace_back( CommandType::MSG_BATTLE_END_TURN, _currentUnit->GetUID() );
                    humanturn_exit = true;
                }
            }
        }
        else {
            Dialog::Message( "", _( "Surrender disabled" ), Font::BIG, Dialog::OK );
        }
        break;
    }

    default:
        break;
    }
}

Battle::PopupDamageInfo::PopupDamageInfo()
    : Dialog::FrameBorder( 5 )
    , _cell( nullptr )
    , _defender( nullptr )
    , _minDamage( 0 )
    , _maxDamage( 0 )
    , _redraw( false )
{}

void Battle::PopupDamageInfo::setBattleUIRect( const fheroes2::Rect & battleUIRect )
{
    _battleUIRect = battleUIRect;
}

bool Battle::PopupDamageInfo::SetDamageInfoBase( const Cell * cell, const Unit * defender )
{
    if ( cell == nullptr || defender == nullptr ) {
        return false;
    }

    if ( !Settings::Get().isBattleShowDamageInfoEnabled() || !Game::validateAnimationDelay( Game::BATTLE_POPUP_DELAY ) ) {
        return false;
    }

    _cell = cell;
    _defender = defender;

    return true;
}

void Battle::PopupDamageInfo::SetAttackInfo( const Cell * cell, const Unit * attacker, const Unit * defender )
{
    if ( attacker == nullptr || !SetDamageInfoBase( cell, defender ) ) {
        return;
    }

    _redraw = true;
    _minDamage = attacker->CalculateMinDamage( *defender );
    _maxDamage = attacker->CalculateMaxDamage( *defender );

    if ( attacker->Modes( SP_BLESS ) ) {
        _minDamage = _maxDamage;
    }
    else if ( attacker->Modes( SP_CURSE ) ) {
        _maxDamage = _minDamage;
    }
}

void Battle::PopupDamageInfo::SetSpellAttackInfo( const Cell * cell, const HeroBase * hero, const Unit * defender, const Spell spell )
{
    assert( hero != nullptr );

    // TODO: Currently, this functionality only supports a simple single-target spell case
    // We should refactor this to apply to all cases
    if ( !spell.isSingleTarget() || !spell.isDamage() ) {
        return;
    }

    if ( !SetDamageInfoBase( cell, defender ) ) {
        return;
    }

    // If defender unit immune to magic, do not show the tooltip
    if ( !defender->AllowApplySpell( spell, hero ) ) {
        return;
    }

    const int spellPoints = hero ? hero->GetPower() : DEFAULT_SPELL_DURATION;
    const uint32_t spellDamage = defender->CalculateSpellDamage( spell, spellPoints, hero, 0 /* targetInfo damage */, true /* ignore defending hero */ );

    _redraw = true;
    _minDamage = spellDamage;
    _maxDamage = spellDamage;
}

void Battle::PopupDamageInfo::Reset()
{
    if ( _redraw ) {
        _redraw = false;
        _cell = nullptr;
        _defender = nullptr;
        _minDamage = 0;
        _maxDamage = 0;
    }

    Game::AnimateResetDelay( Game::BATTLE_POPUP_DELAY );
}

void Battle::PopupDamageInfo::Redraw() const
{
    if ( !_redraw ) {
        return;
    }

    assert( _cell != nullptr && _defender != nullptr );

    std::string str = _minDamage == _maxDamage ? _( "Damage: %{max}" ) : _( "Damage: %{min} - %{max}" );

    StringReplace( str, "%{min}", std::to_string( _minDamage ) );
    StringReplace( str, "%{max}", std::to_string( _maxDamage ) );

    Text damageText( str, Font::SMALL );

    const uint32_t minNumKilled = _defender->HowManyWillKilled( _minDamage );
    const uint32_t maxNumKilled = _defender->HowManyWillKilled( _maxDamage );

    assert( minNumKilled <= _defender->GetCount() && maxNumKilled <= _defender->GetCount() );

    str = minNumKilled == maxNumKilled ? _( "Perish: %{max}" ) : _( "Perish: %{min} - %{max}" );

    StringReplace( str, "%{min}", std::to_string( minNumKilled ) );
    StringReplace( str, "%{max}", std::to_string( maxNumKilled ) );

    Text killedText( str, Font::SMALL );

    const fheroes2::Rect & cellRect = _cell->GetPos();

    // Get the border width and set the popup parameters.
    const int borderWidth = BorderWidth();
    const int x = _battleUIRect.x + cellRect.x + cellRect.width;
    const int y = _battleUIRect.y + cellRect.y;
    const int w = std::max( damageText.w(), killedText.w() ) + 2 * borderWidth;
    const int h = damageText.h() + killedText.h() + 2 * borderWidth;

    // If the damage info popup doesn't fit the battlefield draw surface, then try to place it on the left side of the cell
    const bool isLeftSidePopup = ( cellRect.x + cellRect.width + w ) > _battleUIRect.width;
    const fheroes2::Rect borderRect( isLeftSidePopup ? ( x - w - cellRect.width - borderWidth ) : x, y, w, h );

    Dialog::FrameBorder::RenderOther( fheroes2::AGG::GetICN( ICN::CELLWIN, 1 ), borderRect );

    const int leftTextBorder = borderRect.x + borderWidth;
    damageText.Blit( leftTextBorder, borderRect.y + borderWidth );
    killedText.Blit( leftTextBorder, borderRect.y + borderRect.height / 2 );
}
