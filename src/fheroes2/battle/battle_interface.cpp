/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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
#include <iterator>
#include <ostream>
#include <set>
#include <type_traits>

#include "agg_image.h"
#include "audio.h"
#include "audio_manager.h"
#include "battle.h" // IWYU pragma: associated
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
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "ground.h"
#include "heroes_base.h"
#include "interface_list.h"
#include "localevent.h"
#include "logging.h"
#include "m82.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_tools.h"
#include "monster.h"
#include "monster_anim.h"
#include "monster_info.h"
#include "mp2.h"
#include "mus.h"
#include "pal.h"
#include "players.h"
#include "race.h"
#include "rand.h"
#include "settings.h"
#include "spell_book.h"
#include "timing.h"
#include "tools.h"
#include "translations.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_scrollbar.h"
#include "ui_tool.h"
#include "ui_window.h"
#include "world.h"

class Kingdom;

namespace
{
    const int32_t cellYOffset = -9;

    const int32_t turnOrderMonsterIconSize = 43; // in both directions.

    // The parameters of castle buildings destruction by a catapult:
    // Smoke cloud frame number, after which the building should be drawn as destroyed.
    const int32_t castleBuildingDestroyFrame = 5;
    // Bridge demolition second smoke cloud offset from the first one after the catapult attack.
    const fheroes2::Point bridgeDestroySmokeOffset( -45, 65 );
    // Smoke cloud frame number, after which the bridge should be drawn as destroyed.
    const int32_t bridgeDestroyFrame = 6;
    // The number of frames the second smoke cloud is delayed by.
    const int32_t bridgeDestroySmokeDelay = 2;

    const int32_t offsetForTextBar{ 32 };

    const int32_t maxElementsInBattleLog{ 6 };

    // This value must be equal to the height of Normal font.
    const int32_t battleLogElementHeight{ 17 };

    const int32_t battleLogLastElementOffset{ 4 };

    const int32_t battleLogElementWidth{ fheroes2::Display::DEFAULT_WIDTH - 32 - 16 };

    struct LightningPoint
    {
        explicit LightningPoint( const fheroes2::Point & p = fheroes2::Point(), const int32_t thick = 1 )
            : point( p )
            , thickness( thick )
        {
            // Do nothing.
        }

        fheroes2::Point point;
        int32_t thickness;
    };

    double getDistance( const fheroes2::Point & p1, const fheroes2::Point & p2 )
    {
        return std::hypot( p1.x - p2.x, p1.y - p2.y );
    }

    fheroes2::Point rotate( const fheroes2::Point & point, double angle )
    {
        const double sinValue = sin( angle );
        const double cosValue = cos( angle );

        return { static_cast<int32_t>( std::round( point.x * cosValue - point.y * sinValue ) ),
                 static_cast<int32_t>( std::round( point.x * sinValue + point.y * cosValue ) ) };
    }

    double getAngle( const fheroes2::Point & start, const fheroes2::Point & end )
    {
        return std::atan2( end.y - start.y, end.x - start.x );
    }

    std::vector<std::pair<LightningPoint, LightningPoint>> GenerateLightning( const fheroes2::Point & src, const fheroes2::Point & dst )
    {
        int32_t distance = static_cast<int32_t>( std::round( getDistance( src, dst ) ) );
        const double angle = getAngle( src, dst );

        const int32_t iterationCount = std::clamp( ( distance + 50 ) / 100, 3, 5 );

        std::vector<std::pair<LightningPoint, LightningPoint>> lines;
        lines.emplace_back( LightningPoint( { 0, 0 }, 5 ), LightningPoint( { distance, 0 }, 3 ) );

        for ( int step = 0; step < iterationCount; ++step ) {
            std::vector<std::pair<LightningPoint, LightningPoint>> oldLines;
            std::swap( lines, oldLines );

            for ( const auto & [firstPoint, secondPoint] : oldLines ) {
                fheroes2::Point middle( firstPoint.point + secondPoint.point );
                middle.x /= 2;
                middle.y /= 2;

                const bool isPositive = ( Rand::Get( 1, 2 ) == 1 );
                int32_t offsetY = std::max( 1, static_cast<int32_t>( Rand::Get( 1, 10 ) ) * distance / 100 );

                middle.y += isPositive ? offsetY : -offsetY;

                const int32_t middleThickness = ( firstPoint.thickness + secondPoint.thickness ) / 2;

                const LightningPoint middlePoint( middle, middleThickness );

                lines.emplace_back( firstPoint, middlePoint );
                lines.emplace_back( middlePoint, secondPoint );

                if ( Rand::Get( 1, 4 ) == 1 ) { // 25%
                    offsetY = static_cast<int32_t>( Rand::Get( 1, 10 ) ) * distance / 100;
                    const int32_t x = static_cast<int32_t>( ( middle.x - firstPoint.point.x ) * 0.7 ) + middle.x;
                    const int32_t y = static_cast<int32_t>( ( middle.y - firstPoint.point.y ) * 0.7 ) + middle.y + ( isPositive ? offsetY : -offsetY );
                    lines.emplace_back( middlePoint, LightningPoint( { x, y }, 1 ) );
                }
            }

            distance /= 2;
        }

        for ( auto & [firstPoint, secondPoint] : lines ) {
            firstPoint.point = rotate( firstPoint.point, angle ) + src;
            secondPoint.point = rotate( secondPoint.point, angle ) + src;
        }

        return lines;
    }

    void RedrawLightning( const std::vector<std::pair<LightningPoint, LightningPoint>> & lightning, const uint8_t color, fheroes2::Image & surface,
                          const fheroes2::Rect & roi = fheroes2::Rect() )
    {
        for ( const auto & [firstPoint, secondPoint] : lightning ) {
            const fheroes2::Point & first = firstPoint.point;
            const fheroes2::Point & second = secondPoint.point;
            const bool isHorizontal = std::abs( first.x - second.x ) >= std::abs( first.y - second.y );

            fheroes2::DrawLine( surface, first, second, color, roi );

            for ( int32_t thickness = 1; thickness < secondPoint.thickness; ++thickness ) {
                const bool isUpper = ( ( thickness % 2 ) == 1 );
                const int32_t offset = isUpper ? ( thickness + 1 ) / 2 : -( thickness + 1 ) / 2;

                if ( isHorizontal ) {
                    fheroes2::DrawLine( surface, { first.x, first.y + offset }, { second.x, second.y + offset }, color, roi );
                }
                else {
                    fheroes2::DrawLine( surface, { first.x + offset, first.y }, { second.x + offset, second.y }, color, roi );
                }
            }

            for ( int32_t thickness = secondPoint.thickness; thickness < firstPoint.thickness; ++thickness ) {
                const bool isUpper = ( ( thickness % 2 ) == 1 );
                const int32_t offset = isUpper ? ( thickness + 1 ) / 2 : -( thickness + 1 ) / 2;

                if ( isHorizontal ) {
                    fheroes2::DrawLine( surface, { first.x, first.y + offset }, second, color, roi );
                }
                else {
                    fheroes2::DrawLine( surface, { first.x + offset, first.y }, second, color, roi );
                }
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
        std::vector<int32_t>::const_iterator pnt = rainbowArc.begin();
        const std::vector<int32_t>::const_iterator pntEnd = rainbowArc.end();
        const int32_t rainbowHeight = *std::max_element( pnt, pntEnd ) + rainbowThickness;

        // If the rainbow is vertical - swap width and height.
        const int32_t rainbowImgWidth = isVertical ? rainbowHeight : rainbowWidth;
        const int32_t rainbowImgHeight = isVertical ? rainbowWidth : rainbowHeight;
        fheroes2::Image rainbow( rainbowImgWidth, rainbowImgHeight );
        rainbow.reset();

        // Get the original good luck sprite, since it has a rainbow image which will be used to get line.
        const fheroes2::Sprite & luckSprite = fheroes2::AGG::GetICN( ICN::EXPMRL, 0 );

        // Get a single rainbow line from the center of the luckSprite.
        fheroes2::Image croppedRainbow( 1, rainbowThickness );
        croppedRainbow._disableTransformLayer();
        fheroes2::Copy( luckSprite, luckSprite.width() / 2, 0, croppedRainbow, 0, 0, 1, rainbowThickness );
        fheroes2::Image rainbowLine;

        if ( isVertical ) {
            rainbowLine = fheroes2::Image( croppedRainbow.height(), croppedRainbow.width() );
            rainbowLine._disableTransformLayer();

            // For a vertical rainbow orientation the line needs to be transposed.
            fheroes2::Transpose( croppedRainbow, rainbowLine );
            if ( !flipHorizontally ) {
                rainbowLine = fheroes2::Flip( rainbowLine, true, false );
            }
        }
        else {
            rainbowLine = std::move( croppedRainbow );
        }

        const int32_t rainbowLineWidth = rainbowLine.width();
        const int32_t rainbowLineHeight = rainbowLine.height();

        // Draw a rainbow image for each 'x' coordinate and corresponding '*pnt' value.
        for ( int32_t x = 0; pnt != pntEnd; ++x, ++pnt ) {
            // Set the 'x' and 'y' coordinates of the current rainbow pixel in the resulting rainbow image according to the rainbow direction.
            const int32_t imgX = isVertical ? ( flipHorizontally ? *pnt : rainbowImgWidth - *pnt - rainbowThickness ) : ( flipHorizontally ? rainbowImgWidth - x : x );
            const int32_t imgY = isVertical ? x : *pnt;

            // Insert a rainbow line at the current arc position.
            fheroes2::Copy( rainbowLine, 0, 0, rainbow, imgX, imgY, rainbowLineWidth, rainbowLineHeight );
        }
        return rainbow;
    }

    int32_t GetAbsoluteICNHeight( int icnId )
    {
        const uint32_t frameCount = fheroes2::AGG::GetICNCount( icnId );
        if ( frameCount == 0 ) {
            return 0;
        }

        int32_t height = 0;
        for ( uint32_t i = 0; i < frameCount; ++i ) {
            const int32_t offset = -fheroes2::AGG::GetICN( icnId, i ).y();
            if ( offset > height ) {
                height = offset;
            }
        }

        return height;
    }

    fheroes2::Point CalculateSpellPosition( const Battle::Unit & target, const int spellICN, const fheroes2::Sprite & spellSprite )
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
            const int32_t rearCenterX = ( target.isWide() && target.isReflect() ) ? pos.width * 3 / 4 : Battle::Cell::widthPx / 2;

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
            const int maximumY = GetAbsoluteICNHeight( spellICN );
            result.y = maximumY + spellSprite.y();
        }

        return result;
    }

    int matchHeroType( const HeroBase * base )
    {
        if ( base->isCaptain() ) {
            return Battle::CAPTAIN;
        }

        switch ( base->GetRace() ) {
        case Race::KNGT:
            return Battle::KNIGHT;
        case Race::BARB:
            return Battle::BARBARIAN;
        case Race::SORC:
            return Battle::SORCERESS;
        case Race::WRLK:
            return Battle::WARLOCK;
        case Race::WZRD:
            return Battle::WIZARD;
        case Race::NECR:
            return Battle::NECROMANCER;
        default:
            break;
        }

        // Have you added a new race? Update the logic above!
        assert( 0 );

        return Battle::KNIGHT;
    }

    const std::vector<int> & getHeroAnimation( const HeroBase * hero, int animation )
    {
        static const std::vector<int> staticAnim{ 1 };

        if ( !hero || animation == Battle::OP_STATIC ) {
            return staticAnim;
        }

        const int heroType = matchHeroType( hero );

        if ( animation == Battle::OP_SORROW ) {
            static const std::vector<int> sorrowAnim{ 2, 3, 4, 5, 4, 5, 4, 3, 2 };
            return ( heroType == Battle::CAPTAIN ) ? staticAnim : sorrowAnim;
        }

        static const std::vector<int> heroTypeAnim[7][9]{
            //   JOY                CAST_MASS             CAST_UP               CAST_DOWN     IDLE
            { { 6, 7, 8, 9, 8, 9, 8, 7, 6 }, { 10, 11 }, { 10 }, { 6, 12, 13 }, { 12, 6 }, { 2, 14 }, { 2 }, { 15, 16, 17 }, { 18, 19 } }, // KNIGHT
            { { 6, 7, 8, 9, 9, 8, 7, 6 }, { 6, 10, 11 }, { 10, 6 }, { 6, 12, 13 }, { 12, 6 }, { 6, 14 }, { 6 }, { 15, 16, 17 }, { 18 } }, // BARBARIAN
            { { 6, 7, 8, 7, 6 }, { 6, 7, 9 }, { 7, 6 }, { 6, 10, 11 }, { 10, 6 }, { 6, 12 }, { 6 }, { 13, 14, 15 }, { 16 } }, // SORCERESS
            { { 6, 7, 8, 9, 10, 9, 8, 7, 6 }, { 6, 7, 11, 12 }, { 11, 6 }, { 6, 7, 13 }, { 6 }, { 6, 14 }, { 6 }, { 15, 16 }, { 6 } }, // WARLOCK
            { { 6, 7, 8, 9, 8, 7, 6 }, { 6, 10, 11, 12, 13 }, { 12, 11, 10, 6 }, { 6, 14 }, { 6 }, { 6, 15 }, { 6 }, { 16, 17 }, { 18 } }, // WIZARD
            { { 6, 7, 6, 7, 6, 7 }, { 7, 8, 9, 10, 11 }, { 10, 9, 7 }, { 7, 12, 13, 14, 15 }, { 7 }, { 7, 12, 13, 14, 16 }, { 7 }, { 17 }, { 18, 19 } }, // NECROMANCER
            { { 1 }, { 2, 3, 4 }, { 3, 2 }, { 5, 6 }, { 5 }, { 5, 7 }, { 5 }, { 8, 9 }, { 10 } } // CAPTAIN
        };

        return heroTypeAnim[heroType][animation];
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
        const fheroes2::Sprite & originalGrid = fheroes2::AGG::GetICN( ICN::CMBTMISC, 0 );

        // Move hexagon by 1 pixel down to match the original game's grid.
        fheroes2::Image hexagonSprite( originalGrid.width(), originalGrid.height() + 1 );

        // Make the upper image line transparent. The other part will be filled with the copied image.
        fheroes2::FillTransform( hexagonSprite, 0, 0, originalGrid.width(), 1, 1 );
        fheroes2::Copy( originalGrid, 0, 0, hexagonSprite, 0, 1, originalGrid.width(), originalGrid.height() );

        fheroes2::ReplaceColorId( hexagonSprite, 0, colorId );

        return hexagonSprite;
    }

    fheroes2::Image DrawHexagonShadow( const uint8_t alphaValue, const int32_t horizSpace )
    {
        const int32_t l = 13;
        const int32_t w = Battle::Cell::widthPx;
        const int32_t h = Battle::Cell::heightPx;

        fheroes2::Image sf( w, h );
        sf.reset();
        fheroes2::Rect rt( horizSpace, l - 1, w + 1 - horizSpace * 2, 2 * l + 4 );
        for ( int32_t i = 0; i < w / 2; i += 2 ) {
            for ( int32_t x = 0; x < rt.width; ++x ) {
                for ( int32_t y = 0; y < rt.height; ++y ) {
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

    int GetIndexIndicator( const Battle::Unit & unit )
    {
        if ( unit.Modes( Battle::IS_GOOD_MAGIC | Battle::SP_ANTIMAGIC ) ) {
            if ( unit.Modes( Battle::IS_BAD_MAGIC ) ) {
                // ICN::TEXTBAR index for yellow indicator background color.
                return 13;
            }

            // ICN::TEXTBAR index for green indicator background color.
            return 12;
        }

        if ( unit.Modes( Battle::IS_BAD_MAGIC ) ) {
            // ICN::TEXTBAR index for red indicator background color.
            return 14;
        }

        // ICN::TEXTBAR index for purple indicator background color.
        return 10;
    }

    int getCursorForSpell( const int spell )
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

    int getSwordCursorForAttackDirection( const Battle::AttackDirection direction )
    {
        switch ( direction ) {
        case Battle::AttackDirection::BOTTOM_RIGHT:
            return Cursor::SWORD_TOPLEFT;
        case Battle::AttackDirection::BOTTOM_LEFT:
            return Cursor::SWORD_TOPRIGHT;
        case Battle::AttackDirection::RIGHT:
            return Cursor::SWORD_LEFT;
        case Battle::AttackDirection::TOP_RIGHT:
            return Cursor::SWORD_BOTTOMLEFT;
        case Battle::AttackDirection::TOP_LEFT:
            return Cursor::SWORD_BOTTOMRIGHT;
        case Battle::AttackDirection::LEFT:
            return Cursor::SWORD_RIGHT;
        case Battle::AttackDirection::BOTTOM:
            return Cursor::SWORD_TOP;
        case Battle::AttackDirection::TOP:
            return Cursor::SWORD_BOTTOM;
        default:
            break;
        }

        return 0;
    }

    Battle::AttackDirection getAttackDirectionForSwordCursor( const uint32_t sword )
    {
        switch ( sword ) {
        case Cursor::SWORD_TOPLEFT:
            return Battle::AttackDirection::BOTTOM_RIGHT;
        case Cursor::SWORD_TOPRIGHT:
            return Battle::AttackDirection::BOTTOM_LEFT;
        case Cursor::SWORD_LEFT:
            return Battle::AttackDirection::RIGHT;
        case Cursor::SWORD_BOTTOMLEFT:
            return Battle::AttackDirection::TOP_RIGHT;
        case Cursor::SWORD_BOTTOMRIGHT:
            return Battle::AttackDirection::TOP_LEFT;
        case Cursor::SWORD_RIGHT:
            return Battle::AttackDirection::LEFT;
        case Cursor::SWORD_TOP:
            return Battle::AttackDirection::BOTTOM;
        case Cursor::SWORD_BOTTOM:
            return Battle::AttackDirection::TOP;
        default:
            break;
        }

        return Battle::AttackDirection::UNKNOWN;
    }

    Battle::CellDirection getAttackingCellDirectionForAttackDirection( const Battle::Unit & attacker, const Battle::AttackDirection dir )
    {
        switch ( dir ) {
        // This attack is performed from the head cell of the wide attacking unit in the direction of "back and down".
        case Battle::AttackDirection::TOP:
            return attacker.isReflect() ? Battle::CellDirection::TOP_LEFT : Battle::CellDirection::TOP_RIGHT;
        // This attack is performed from the head cell of the wide attacking unit in the direction of "back and up".
        case Battle::AttackDirection::BOTTOM:
            return attacker.isReflect() ? Battle::CellDirection::BOTTOM_LEFT : Battle::CellDirection::BOTTOM_RIGHT;
        case Battle::AttackDirection::UNKNOWN:
            return Battle::CellDirection::UNKNOWN;
        case Battle::AttackDirection::TOP_LEFT:
            return Battle::CellDirection::TOP_LEFT;
        case Battle::AttackDirection::TOP_RIGHT:
            return Battle::CellDirection::TOP_RIGHT;
        case Battle::AttackDirection::RIGHT:
            return Battle::CellDirection::RIGHT;
        case Battle::AttackDirection::BOTTOM_RIGHT:
            return Battle::CellDirection::BOTTOM_RIGHT;
        case Battle::AttackDirection::BOTTOM_LEFT:
            return Battle::CellDirection::BOTTOM_LEFT;
        case Battle::AttackDirection::LEFT:
            return Battle::CellDirection::LEFT;
        case Battle::AttackDirection::CENTER:
            return Battle::CellDirection::CENTER;
        default:
            assert( 0 );
            break;
        }

        return Battle::CellDirection::UNKNOWN;
    }

    // Mapping a color bit flag to a color code
    uint8_t GetArmyColorFromPlayerColor( const PlayerColor playerColor )
    {
        switch ( playerColor ) {
        // Units under Berserker spell.
        case PlayerColor::UNUSED:
            return Battle::ArmyColor::ARMY_COLOR_BLACK;
        case PlayerColor::BLUE:
            return Battle::ArmyColor::ARMY_COLOR_BLUE;
        case PlayerColor::GREEN:
            return Battle::ArmyColor::ARMY_COLOR_GREEN;
        case PlayerColor::RED:
            return Battle::ArmyColor::ARMY_COLOR_RED;
        case PlayerColor::YELLOW:
            return Battle::ArmyColor::ARMY_COLOR_YELLOW;
        case PlayerColor::ORANGE:
            return Battle::ArmyColor::ARMY_COLOR_ORANGE;
        case PlayerColor::PURPLE:
            return Battle::ArmyColor::ARMY_COLOR_PURPLE;
        case PlayerColor::NONE:
            return Battle::ArmyColor::ARMY_COLOR_GRAY;
        default:
            // Did you add another player color?
            assert( 0 );
            return Battle::ArmyColor::ARMY_COLOR_GRAY;
        }
    }

    // The cast down is applied below the 2nd battlefield row (count is started from 0)
    // and for the (rowNumber - 2) columns starting from the side of the hero.
    bool isHeroCastDown( const int32_t cellindex, const bool isLeftOpponent )
    {
        return isLeftOpponent ? ( ( cellindex % 11 ) < cellindex / 11 - 2 ) : ( ( 10 - ( cellindex % 11 ) ) < cellindex / 11 - 2 );
    }

    Battle::Position calculateAttackPosition( const Battle::Unit & unit, const int32_t attackedCellIndex, const Battle::AttackDirection attackDir )
    {
        const Battle::CellDirection attackingCellDir = getAttackingCellDirectionForAttackDirection( unit, attackDir );
        assert( Battle::Board::isValidDirection( attackedCellIndex, attackingCellDir ) );

        const int32_t attackingCellIndex = Battle::Board::GetIndexDirection( attackedCellIndex, attackingCellDir );
        assert( Battle::Board::isValidIndex( attackingCellIndex ) );

        const Battle::Position pos = Battle::Position::GetReachable( unit, attackingCellIndex );
        assert( pos.isValidForUnit( unit ) );

        if ( !unit.isWide() ) {
            return pos;
        }

        if ( unit.isReflect() ? ( attackDir != Battle::AttackDirection::TOP_LEFT && attackDir != Battle::AttackDirection::BOTTOM_LEFT )
                              : ( attackDir != Battle::AttackDirection::TOP_RIGHT && attackDir != Battle::AttackDirection::BOTTOM_RIGHT ) ) {
            // There is no tail attack, so there is no need for further adjustments
            return pos;
        }

        if ( pos.GetTail()->GetIndex() == attackingCellIndex ) {
            // The attacking unit will already be in the correct position, there is no need for further adjustments
            return pos;
        }

        const Battle::CellDirection adjustmentDirection = unit.isReflect() ? Battle::CellDirection::LEFT : Battle::CellDirection::RIGHT;

        if ( !Battle::Board::isValidDirection( attackingCellIndex, adjustmentDirection ) ) {
            return pos;
        }

        if ( const Battle::Position adjustedPos = Battle::Position::GetReachable( unit, Battle::Board::GetIndexDirection( attackingCellIndex, adjustmentDirection ) );
             // Shifting a unit's position to the left or right can sometimes change its actual position by more than 1 cell. For instance, in a situation where a unit
             // can occupy either position [1, 2] or [3, 4], but not the position [2, 3] (due to limitations caused by the terrain or the location of other units), its
             // shift to the right from position [1, 2] will mean taking position [3, 4], and not [2, 3], as one might expect.
             adjustedPos.isValidForUnit( unit ) && adjustedPos.GetTail()->GetIndex() == attackingCellIndex ) {
            return adjustedPos;
        }

        return pos;
    }
}

namespace Battle
{
    class StatusListBox final : public ::Interface::ListBox<std::string>
    {
    public:
        using ::Interface::ListBox<std::string>::ActionListDoubleClick;
        using ::Interface::ListBox<std::string>::ActionListSingleClick;
        using ::Interface::ListBox<std::string>::ActionListPressRight;

        void SetPosition( int32_t px, int32_t py )
        {
            assert( px >= 0 && py >= 0 );

            const int32_t totalElementHeight = maxElementsInBattleLog * battleLogElementHeight - battleLogLastElementOffset;
            _border.SetPosition( px, py - totalElementHeight - 32, fheroes2::Display::DEFAULT_WIDTH - 32, totalElementHeight );
            const fheroes2::Rect & area = _border.GetArea();

            SetTopLeft( area.getPosition() );
            SetAreaMaxItems( maxElementsInBattleLog );

            const int32_t ax = area.x + area.width - 12;
            SetScrollButtonUp( ICN::DROPLISL, 6, 7, fheroes2::Point( ax, area.y - 10 ) );
            SetScrollButtonDn( ICN::DROPLISL, 8, 9, fheroes2::Point( ax, area.y + area.height - 11 ) );

            const fheroes2::Rect & buttonPgUpArea = _buttonPgUp.area();

            _scrollbarSliderAreaLength = _buttonPgDn.area().y - ( buttonPgUpArea.y + buttonPgUpArea.height ) - 7;

            setScrollBarArea( { ax + 5, buttonPgUpArea.y + buttonPgUpArea.height + 3, 12, _scrollbarSliderAreaLength } );

            const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::DROPLISL, 13 );
            const fheroes2::Image scrollbarSlider
                = fheroes2::generateScrollbarSlider( originalSlider, false, _scrollbarSliderAreaLength, VisibleItemCount(), static_cast<int32_t>( _messages.size() ),
                                                     { 0, 0, originalSlider.width(), 4 }, { 0, 4, originalSlider.width(), 8 } );

            setScrollBarImage( scrollbarSlider );
            _scrollbar.hide();
            SetAreaItems( { area.x, area.y, area.width - 16, area.height + battleLogLastElementOffset } );
            SetListContent( _messages );
        }

        const fheroes2::Rect & GetArea() const
        {
            return _border.GetRect();
        }

        void AddMessage( std::string str )
        {
            // Check if message string fits the log width.
            fheroes2::Text text( str, fheroes2::FontType::normalWhite() );
            size_t pos = std::string::npos;

            while ( text.width() > battleLogElementWidth ) {
                // Find the maximum allowed phrase to fit the line. We assume that all words are separated by a space.
                pos = str.rfind( ' ', pos - 1 );

                // Theoretically there could be such a large word that did not fit the log width.
                if ( pos == std::string::npos ) {
                    // Find the next word in this phrase.
                    pos = str.find( ' ' );

                    // Cut this long word with the trailing '...'.
                    text.fitToOneRow( battleLogElementWidth );

                    if ( pos == std::string::npos ) {
                        // This is the only word in the phrase.
                        str = text.text();
                    }
                    else {
                        _messages.push_back( text.text() );
                        // Put the rest of the phrase to the next line.
                        str = str.substr( pos + 1 );
                        // There is no need to split this phrase any more.
                        pos = std::string::npos;
                    }

                    break;
                }

                text.set( str.substr( 0, pos ), fheroes2::FontType::normalWhite() );
            }

            if ( pos == std::string::npos ) {
                _messages.push_back( std::move( str ) );
            }
            else {
                // If a phrase does not fit the log width we split it into two lines.
                _messages.push_back( str.substr( 0, pos ) );
                _messages.push_back( str.substr( pos + 1 ) );
            }

            if ( !_isLogOpened ) {
                _scrollbar.hide();
            }

            SetListContent( _messages );

            // Update the scrollbar image.
            const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::DROPLISL, 13 );
            const fheroes2::Image scrollbarSlider
                = fheroes2::generateScrollbarSlider( originalSlider, false, _scrollbarSliderAreaLength, VisibleItemCount(), static_cast<int32_t>( _messages.size() ),
                                                     { 0, 0, originalSlider.width(), 4 }, { 0, 4, originalSlider.width(), 8 } );
            setScrollBarImage( scrollbarSlider );
            SetCurrent( _messages.size() - 1 );
        }

        void RedrawItem( const std::string & str, int32_t px, int32_t py, bool /* unused */ ) override
        {
            const fheroes2::Text text( str, fheroes2::FontType::normalWhite() );
            text.draw( px, py, fheroes2::Display::instance() );
        }

        void RedrawBackground( const fheroes2::Point & /* unused*/ ) override
        {
            fheroes2::Display & display = fheroes2::Display::instance();

            const fheroes2::Rect & buttonPgUpArea = _buttonPgUp.area();
            const int32_t buttonPgUpBottom = buttonPgUpArea.y + buttonPgUpArea.height;
            const int32_t buttonPgDnAreaY = _buttonPgDn.area().y;

            const int32_t ax = buttonPgUpArea.x;
            const int32_t ah = buttonPgDnAreaY - buttonPgUpBottom;

            const fheroes2::Rect & borderRect = _border.GetRect();
            Dialog::FrameBorder::RenderOther( fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ), borderRect );

            const fheroes2::Sprite & sp3 = fheroes2::AGG::GetICN( ICN::DROPLISL, 11 );
            for ( int32_t i = 0; i < ( ah / sp3.height() ); ++i ) {
                fheroes2::Copy( sp3, 0, 0, display, ax, buttonPgUpBottom + ( sp3.height() * i ), sp3.width(), sp3.height() );
            }

            const fheroes2::Sprite & sp1 = fheroes2::AGG::GetICN( ICN::DROPLISL, 10 );
            const fheroes2::Sprite & sp2 = fheroes2::AGG::GetICN( ICN::DROPLISL, 12 );

            fheroes2::Copy( sp1, 0, 0, display, ax, buttonPgUpBottom, sp1.width(), sp1.height() );
            fheroes2::Copy( sp2, 0, 0, display, ax, buttonPgDnAreaY - sp2.height(), sp2.width(), sp2.height() );
        }

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( std::string & /* unused */ ) override
        {
            // Do nothing.
        }

        void ActionListSingleClick( std::string & /* unused */ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( std::string & /* unused */ ) override
        {
            // Do nothing.
        }

        void SetOpenLog( const bool opened )
        {
            _isLogOpened = opened;
        }

        bool isOpenLog() const
        {
            return _isLogOpened;
        }

    private:
        Dialog::FrameBorder _border;
        std::vector<std::string> _messages;
        int32_t _scrollbarSliderAreaLength{ 0 };
        bool _isLogOpened{ false };
    };
}

bool Battle::TargetInfo::isFinishAnimFrame( const TargetInfo & info )
{
    return info.defender && info.defender->isFinishAnimFrame();
}

Battle::OpponentSprite::OpponentSprite( const fheroes2::Rect & area, HeroBase * hero, const bool isReflect )
    : _heroBase( hero )
    , _currentAnim( getHeroAnimation( hero, OP_STATIC ) )
    , _isFlippedHorizontally( isReflect )
    , _offset( area.x, area.y )
{
    const bool isCaptain = hero->isCaptain();
    switch ( hero->GetRace() ) {
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

    _area.width = sprite.width();
    _area.height = sprite.height();

    if ( _isFlippedHorizontally ) {
        _area.x = _offset.x + fheroes2::Display::DEFAULT_WIDTH - RIGHT_HERO_X_OFFSET - ( sprite.x() + _area.width );
        _area.y = _offset.y + RIGHT_HERO_Y_OFFSET + sprite.y();
    }
    else {
        _area.x = _offset.x + LEFT_HERO_X_OFFSET + sprite.x();
        _area.y = _offset.y + LEFT_HERO_Y_OFFSET + sprite.y();
    }

    if ( isCaptain ) {
        if ( _isFlippedHorizontally ) {
            _area.x += CAPTAIN_X_OFFSET;
        }
        else {
            _area.x -= CAPTAIN_X_OFFSET;
        }

        _area.y += CAPTAIN_Y_OFFSET;
    }
}

void Battle::OpponentSprite::IncreaseAnimFrame()
{
    _currentAnim.playAnimation( false );
}

void Battle::OpponentSprite::SetAnimation( const int rule )
{
    _animationType = rule;
    _currentAnim = getHeroAnimation( _heroBase, rule );
}

fheroes2::Point Battle::OpponentSprite::GetCastPosition() const
{
    const bool isCaptain = _heroBase->isCaptain();
    fheroes2::Point offset;
    switch ( _heroBase->GetRace() ) {
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

    return { _area.x + ( _isFlippedHorizontally ? offset.x : _area.width - offset.x ), _area.y + _area.height / 2 + offset.y };
}

void Battle::OpponentSprite::Redraw( fheroes2::Image & dst ) const
{
    const fheroes2::Sprite & hero = fheroes2::AGG::GetICN( _heroIcnId, _currentAnim.getFrame() );

    fheroes2::Point offset( _offset );
    if ( _heroBase->isCaptain() ) {
        if ( _isFlippedHorizontally ) {
            offset.x += CAPTAIN_X_OFFSET;
        }
        else {
            offset.x -= CAPTAIN_X_OFFSET;
        }
        offset.y += CAPTAIN_Y_OFFSET;
    }

    if ( _isFlippedHorizontally ) {
        fheroes2::Blit( hero, dst, offset.x + fheroes2::Display::DEFAULT_WIDTH - RIGHT_HERO_X_OFFSET - ( hero.x() + hero.width() ),
                        offset.y + RIGHT_HERO_Y_OFFSET + hero.y(), _isFlippedHorizontally );
    }
    else {
        fheroes2::Blit( hero, dst, offset.x + LEFT_HERO_X_OFFSET + hero.x(), offset.y + LEFT_HERO_Y_OFFSET + hero.y() );
    }
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

    _currentAnim.playAnimation( false );
    return true;
}

Battle::Status::Status()
    : _upperBackground( fheroes2::AGG::GetICN( ICN::TEXTBAR, 8 ) )
    , _lowerBackground( fheroes2::AGG::GetICN( ICN::TEXTBAR, 9 ) )
{
    width = _upperBackground.width();
    height = _upperBackground.height() + _lowerBackground.height();
}

void Battle::Status::setMessage( std::string messageString, const bool top )
{
    if ( top ) {
        _upperText.set( messageString, fheroes2::FontType::normalWhite() );
        // The text cannot go beyond the text area so it is important to truncate it when necessary.
        _upperText.fitToOneRow( width - offsetForTextBar * 2 );

        if ( _battleStatusLog ) {
            _battleStatusLog->AddMessage( std::move( messageString ) );
        }
    }
    else if ( messageString != _lastMessage ) {
        _lowerText.set( messageString, fheroes2::FontType::normalWhite() );
        // The text cannot go beyond the text area so it is important to truncate it when necessary.
        _lowerText.fitToOneRow( width - offsetForTextBar * 2 );

        _lastMessage = std::move( messageString );
    }
}

void Battle::Status::redraw( fheroes2::Image & output ) const
{
    fheroes2::Copy( _upperBackground, 0, 0, output, x, y, width, _upperBackground.height() );
    fheroes2::Copy( _lowerBackground, 0, 0, output, x, y + _upperBackground.height(), width, _lowerBackground.height() );

    if ( !_upperText.empty() ) {
        _upperText.draw( x + ( width - _upperText.width() ) / 2, y + 4, output );
    }

    if ( !_lowerText.empty() ) {
        _lowerText.draw( x + ( width - _lowerText.width() ) / 2, y + _upperBackground.height(), output );
    }
}

void Battle::Status::clear()
{
    _upperText.set( "", fheroes2::FontType::normalWhite() );
    _lowerText.set( "", fheroes2::FontType::normalWhite() );
    _lastMessage = "";
}

bool Battle::TurnOrder::queueEventProcessing( Interface & interface, std::string & msg, const fheroes2::Point & offset ) const
{
    LocalEvent & le = LocalEvent::Get();

    for ( const auto & [unit, unitPos] : _rects ) {
        assert( unit != nullptr );

        const fheroes2::Rect unitRoi = unitPos + offset;
        if ( le.isMouseCursorPosInArea( unitRoi ) ) {
            msg = _( "View %{monster} info" );
            StringReplaceWithLowercase( msg, "%{monster}", unit->GetName() );

            interface.setUnitTobeHighlighted( unit );

            // Process mouse buttons events.
            if ( le.MouseClickLeft( unitRoi ) ) {
                Dialog::ArmyInfo( *unit, Dialog::BUTTONS, unit->isReflect() );
                return true;
            }
            if ( le.isMouseRightButtonPressed() ) {
                Dialog::ArmyInfo( *unit, Dialog::ZERO, unit->isReflect() );
                return true;
            }
            // There is no need to process other items of `_rects`.
            return false;
        }
    }

    return false;
}

void Battle::TurnOrder::_redrawUnit( const fheroes2::Rect & pos, const Battle::Unit & unit, const bool revert, const uint8_t unitColor, fheroes2::Image & output )
{
    // Render background.
    const fheroes2::Sprite & backgroundOriginal = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );
    fheroes2::Copy( backgroundOriginal, 37, 268, output, pos.x + 1, pos.y + 1, pos.width - 2, pos.height - 2 );

    // Draw a monster's sprite.
    const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( ICN::MONS32, unit.GetSpriteIndex() );
    const int32_t monsterSpriteHeight = monsterSprite.height();
    if ( unit.Modes( Battle::CAP_MIRRORIMAGE ) ) {
        fheroes2::Sprite mirroredMonster = monsterSprite;

        fheroes2::ApplyPalette( mirroredMonster, PAL::GetPalette( PAL::PaletteType::MIRROR_IMAGE ) );
        fheroes2::Blit( mirroredMonster, output, pos.x + ( pos.width - monsterSprite.width() ) / 2,
                        pos.y + pos.height - monsterSpriteHeight - ( monsterSpriteHeight + 3 < pos.height ? 3 : 0 ), revert );
    }
    else {
        fheroes2::Blit( monsterSprite, output, pos.x + ( pos.width - monsterSprite.width() ) / 2,
                        pos.y + pos.height - monsterSpriteHeight - ( monsterSpriteHeight + 3 < pos.height ? 3 : 0 ), revert );
    }

    const fheroes2::Text number( fheroes2::abbreviateNumber( static_cast<int32_t>( unit.GetCount() ) ), fheroes2::FontType::smallWhite() );
    number.drawInRoi( pos.x + 2, pos.y + 4, output, pos );

    fheroes2::DrawRect( output, pos, unitColor );

    if ( unit.Modes( Battle::TR_MOVED ) ) {
        fheroes2::ApplyPalette( output, pos.x, pos.y, output, pos.x, pos.y, turnOrderMonsterIconSize, turnOrderMonsterIconSize,
                                PAL::GetPalette( PAL::PaletteType::GRAY ) );
        fheroes2::ApplyPalette( output, pos.x, pos.y, output, pos.x, pos.y, turnOrderMonsterIconSize, turnOrderMonsterIconSize, 3 );
    }
}

void Battle::TurnOrder::redraw( const Unit * current, const uint8_t currentUnitColor, const Unit * underCursor, fheroes2::Image & output )
{
    if ( _orderOfUnits.expired() ) {
        // Nothing to show.
        return;
    }

    const std::shared_ptr<const Units> unitsOrder = _orderOfUnits.lock();

    const int32_t validUnitCount = static_cast<int32_t>( std::count_if( unitsOrder->begin(), unitsOrder->end(), []( const Unit * unit ) {
        assert( unit != nullptr );
        return unit->isValid();
    } ) );

    const int32_t unitsToDraw = std::min( _area.width / turnOrderMonsterIconSize, validUnitCount );

    if ( _rects.size() != static_cast<size_t>( unitsToDraw ) ) {
        // Update units icons positions.

        width = turnOrderMonsterIconSize * unitsToDraw;

        int32_t offsetX = ( _area.width - width ) / 2;

        x = _area.x + offsetX;
        y = _area.y;
        height = turnOrderMonsterIconSize;

        _rects.clear();
        _rects.reserve( unitsToDraw );

        for ( int32_t index = 0; index < unitsToDraw; ++index ) {
            // Just for now place 'nullptr' instead of a pointer to unit.
            // These pointers will be updated in the next loop.
            _rects.emplace_back( nullptr, fheroes2::Rect( offsetX, 0, turnOrderMonsterIconSize, turnOrderMonsterIconSize ) );

            offsetX += turnOrderMonsterIconSize;
        }
    }

    int32_t unitRectIndex = 0;
    int32_t unitsProcessed = 0;

    for ( const Unit * unit : *unitsOrder ) {
        if ( unitRectIndex == unitsToDraw ) {
            break;
        }

        assert( unit != nullptr );
        if ( !unit->isValid() ) {
            continue;
        }

        if ( unit->Modes( Battle::TR_MOVED ) && ( validUnitCount - unitsProcessed > unitsToDraw ) ) {
            ++unitsProcessed;
            continue;
        }

        _rects[unitRectIndex].first = unit;

        uint8_t unitColor = GetArmyColorFromPlayerColor( unit->GetCurrentColor() );
        if ( current == unit || underCursor == unit ) {
            unitColor = currentUnitColor;
        }

        _redrawUnit( _rects[unitRectIndex].second, *unit, unit->GetColor() == _opponentColor, unitColor, output );

        ++unitRectIndex;
        ++unitsProcessed;
    }
}

Battle::Interface::Interface( Arena & battleArena, const int32_t tileIndex )
    : arena( battleArena )
{
    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    // border
    const fheroes2::Display & display = fheroes2::Display::instance();

    _interfacePosition = { ( display.width() - fheroes2::Display::DEFAULT_WIDTH ) / 2, ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2,
                           _surfaceInnerArea.width, _surfaceInnerArea.height };
    border.SetPosition( _interfacePosition.x - fheroes2::borderWidthPx, _interfacePosition.y - fheroes2::borderWidthPx, fheroes2::Display::DEFAULT_WIDTH,
                        fheroes2::Display::DEFAULT_HEIGHT );

    // damage info popup
    popup.setBattleUIRect( _interfacePosition );

    // cover
    const bool trees = !Maps::ScanAroundObject( tileIndex, MP2::OBJ_TREES ).empty();
    const Maps::Tile & tile = world.getTile( tileIndex );

    const int groundType = tile.GetGround();
    _brightLandType
        = ( groundType == Maps::Ground::SNOW || groundType == Maps::Ground::DESERT || groundType == Maps::Ground::WASTELAND || groundType == Maps::Ground::BEACH );
    if ( _brightLandType ) {
        _contourColor = 108;
    }

    switch ( groundType ) {
    case Maps::Ground::DESERT:
        _battleGroundIcn = ICN::CBKGDSRT;
        _borderObjectsIcn = ICN::FRNG0004;
        break;
    case Maps::Ground::SNOW:
        _battleGroundIcn = trees ? ICN::CBKGSNTR : ICN::CBKGSNMT;
        _borderObjectsIcn = trees ? ICN::FRNG0006 : ICN::FRNG0007;
        break;
    case Maps::Ground::SWAMP:
        _battleGroundIcn = ICN::CBKGSWMP;
        _borderObjectsIcn = ICN::FRNG0008;
        break;
    case Maps::Ground::WASTELAND:
        _battleGroundIcn = ICN::CBKGCRCK;
        _borderObjectsIcn = ICN::FRNG0003;
        break;
    case Maps::Ground::BEACH:
        _battleGroundIcn = ICN::CBKGBEAC;
        _borderObjectsIcn = ICN::FRNG0002;
        break;
    case Maps::Ground::LAVA:
        _battleGroundIcn = ICN::CBKGLAVA;
        _borderObjectsIcn = ICN::FRNG0005;
        break;
    case Maps::Ground::DIRT:
        _battleGroundIcn = trees ? ICN::CBKGDITR : ICN::CBKGDIMT;
        _borderObjectsIcn = trees ? ICN::FRNG0010 : ICN::FRNG0009;
        break;
    case Maps::Ground::GRASS:
        _battleGroundIcn = trees ? ICN::CBKGGRTR : ICN::CBKGGRMT;
        _borderObjectsIcn = trees ? ICN::FRNG0011 : ICN::FRNG0012;
        break;
    case Maps::Ground::WATER:
        _battleGroundIcn = ICN::CBKGWATR;
        _borderObjectsIcn = ICN::FRNG0013;
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

    _buttonAuto.setICNInfo( ICN::TEXTBAR, 4, 5 );
    _buttonSettings.setICNInfo( ICN::TEXTBAR, 6, 7 );

    // opponents
    if ( HeroBase * opponent = arena.getAttackingArmyCommander(); opponent != nullptr ) {
        _attackingOpponent = std::make_unique<OpponentSprite>( _surfaceInnerArea, opponent, false );
    }
    if ( HeroBase * opponent = arena.getDefendingArmyCommander(); opponent != nullptr ) {
        _defendingOpponent = std::make_unique<OpponentSprite>( _surfaceInnerArea, opponent, true );
    }

    if ( Arena::GetCastle() ) {
        _ballistaTowerRect = fheroes2::Rect{ _interfacePosition.x + 570, _interfacePosition.y + 145, 70, 160 };
    }

    const fheroes2::Rect & area = border.GetArea();

    const fheroes2::Rect & settingsRect = _buttonSettings.area();
    const int32_t satusOffsetY = area.y + area.height - settingsRect.height - _buttonAuto.area().height;
    _buttonAuto.setPosition( area.x, satusOffsetY );
    _buttonSettings.setPosition( area.x, area.y + area.height - settingsRect.height );

    _buttonSkip.setICNInfo( ICN::TEXTBAR, 0, 1 );
    _buttonSkip.setPosition( area.x + area.width - _buttonSkip.area().width, area.y + area.height - _buttonSkip.area().height );

    status.setPosition( area.x + settingsRect.width, satusOffsetY );

    listlog = std::make_unique<StatusListBox>();

    const int32_t battlefieldHeight = area.height - status.height;

    if ( listlog ) {
        listlog->SetPosition( area.x, area.y + battlefieldHeight );
    }
    status.setLogs( listlog.get() );

    // As `_battleGround` and '_mainSurface' are used to prepare battlefield screen to render on display they do not need to have a transform layer.
    _battleGround._disableTransformLayer();
    _mainSurface._disableTransformLayer();

    // Battlefield area excludes the lower part where the status log is located.
    _mainSurface.resize( area.width, battlefieldHeight );
    _battleGround.resize( area.width, battlefieldHeight );

    AudioManager::ResetAudio();
}

Battle::Interface::~Interface()
{
    AudioManager::ResetAudio();

    // Fade-out battlefield.
    const bool isDefaultScreenSize = fheroes2::Display::instance().isDefaultSize();

    fheroes2::fadeOutDisplay( _background->activeArea(), !isDefaultScreenSize );

    // For 640x480 resolution we do screen fade-in.
    if ( isDefaultScreenSize ) {
        // Reset the battlefield dialog window to restore the previous display image from screen restorer.
        // We have multiple return places after the battle: the adventure map, Main Menu (from Battle only),
        // the battle results screen (if the battle was quick ended).
        _background.reset();

        Game::setDisplayFadeIn();
    }
}

void Battle::Interface::SetOrderOfUnits( const std::shared_ptr<const Units> & units )
{
    _turnOrder.set( _interfacePosition, units, arena.getDefendingArmyColor() );
}

fheroes2::Point Battle::Interface::getRelativeMouseCursorPos() const
{
    return LocalEvent::Get().getMouseCursorPos() - _interfacePosition.getPosition();
}

void Battle::Interface::setStatus( const std::string & message, const bool top )
{
    status.setMessage( message, top );

    if ( top ) {
        status.setMessage( "", false );
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

    fheroes2::Display & display = fheroes2::Display::instance();

    // Fade-out game screen only for 640x480 resolution.
    const bool isDefaultScreenSize = display.isDefaultSize();
    if ( isDefaultScreenSize ) {
        fheroes2::fadeOutDisplay();
    }

    // Don't waste time playing the pre-battle sound if the game sounds are turned off
    if ( Settings::Get().SoundVolume() > 0 ) {
        _preBattleSoundChannelId = AudioManager::PlaySound( M82::PREBATTL );
    }

    // Prepare the Battlefield ground.
    _redrawBattleGround();

    RedrawPartialStart();
    // We do not render battlefield display image to properly fade-in it.
    redrawPreRender();

    // Fade-in battlefield.
    if ( !isDefaultScreenSize ) {
        // We need to expand the ROI for the next render to properly render window borders and shadow.
        display.updateNextRenderRoi( _background->totalArea() );
    }

    fheroes2::fadeInDisplay( _background->activeArea(), !isDefaultScreenSize );
}

void Battle::Interface::Redraw()
{
    // Check that the pre-battle sound is over to start playing the battle music.
    // IMPORTANT: This implementation may suffer from the race condition as the pre-battle sound channel may be reused
    // by new sounds if they are played (one way or another) after the end of the pre-battle sound, but before calling
    // this method. Although in this case here it will not lead to serious problems as in the worst case, the launch of
    // battle music will be postponed for some time.
    if ( _preBattleSoundChannelId && ( _preBattleSoundChannelId < 0 || !Mixer::isPlaying( _preBattleSoundChannelId.value() ) ) && !Music::isPlaying() ) {
        // Reset the value of _preBattleSoundChannelId to skip isPlaying() checks in future as these checks could freeze
        // the game for some time in certain cases (e.g. slow MIDI backend).
        _preBattleSoundChannelId.reset();

        AudioManager::PlayMusicAsync( MUS::GetBattleRandom(), Music::PlaybackMode::REWIND_AND_PLAY_INFINITE );
    }

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
    redrawPreRender();

    fheroes2::Display::instance().render( _interfacePosition );
}

void Battle::Interface::redrawPreRender()
{
    if ( Settings::Get().BattleShowTurnOrder() ) {
        const Unit * unit = nullptr;
        const Cell * cell = Board::GetCell( _currentCellIndex );
        if ( cell != nullptr ) {
            unit = cell->GetUnit();
        }
        _turnOrder.redraw( _currentUnit, _contourColor, unit, _mainSurface );
    }

#ifdef WITH_DEBUG
    if ( IS_DEVEL() ) {
        const Board * board = Arena::GetBoard();
        assert( board != nullptr );

        for ( const Cell & cell : *board ) {
            const fheroes2::Text text( std::to_string( cell.GetIndex() ), fheroes2::FontType::smallWhite() );
            text.draw( cell.GetPos().x + 20, cell.GetPos().y + 24, _mainSurface );
        }
    }
#endif

    fheroes2::Copy( _mainSurface, 0, 0, fheroes2::Display::instance(), _interfacePosition.x, _interfacePosition.y, _mainSurface.width(), _mainSurface.height() );
    RedrawInterface();
}

void Battle::Interface::RedrawInterface()
{
    status.redraw( fheroes2::Display::instance() );

    _buttonAuto.draw();
    _buttonSettings.draw();
    _buttonSkip.draw();

    popup.redraw();

    if ( listlog && listlog->isOpenLog() ) {
        listlog->Redraw();
    }
}

void Battle::Interface::RedrawArmies()
{
    // Continue the idle animation for all troops on the battlefield: update idle animation frames before rendering the troops.
    IdleTroopsAnimation();

    const Castle * castle = Arena::GetCastle();

    std::array<int32_t, Board::heightInCells>
        wallCellIds{ Arena::CASTLE_FIRST_TOP_WALL_POS, Arena::CASTLE_TOP_ARCHER_TOWER_POS,  Arena::CASTLE_SECOND_TOP_WALL_POS, Arena::CASTLE_TOP_GATE_TOWER_POS,
                     Arena::CASTLE_GATE_POS,           Arena::CASTLE_BOTTOM_GATE_TOWER_POS, Arena::CASTLE_THIRD_TOP_WALL_POS,  Arena::CASTLE_BOTTOM_ARCHER_TOWER_POS,
                     Arena::CASTLE_FOURTH_TOP_WALL_POS };

    if ( castle == nullptr ) {
        RedrawKilled();
    }

    for ( int32_t cellRowId = 0; cellRowId < Board::heightInCells; ++cellRowId ) {
        // Redraw objects.
        for ( int32_t cellColumnId = 0; cellColumnId < Board::widthInCells; ++cellColumnId ) {
            const int32_t cellId = cellRowId * Board::widthInCells + cellColumnId;
            _redrawHighObjects( cellId );
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

            std::vector<const Unit *> downwardMovingTroopBeforeWall;
            std::vector<const Unit *> downwardMovingTroopAfterWall;
            std::vector<const Unit *> movingTroopAfterWall;

            // Overlay sprites for troops (i.e. spell effect animation) should be rendered after rendering all troops
            // for current row so the next troop will not be rendered over the overlay sprite.
            std::vector<const UnitSpellEffectInfo *> troopOverlaySpriteBeforeWall;
            std::vector<const UnitSpellEffectInfo *> troopOverlaySpriteAfterWall;

            const int32_t wallCellId = wallCellIds[cellRowId];

            for ( int32_t cellColumnId = 0; cellColumnId < Board::widthInCells; ++cellColumnId ) {
                const int32_t cellId = cellRowId * Board::widthInCells + cellColumnId;

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

                for ( const Unit * deadUnit : arena.getGraveyardUnits( cellId ) ) {
                    if ( deadUnit && cellId != deadUnit->GetTailIndex() ) {
                        if ( isCellBefore ) {
                            deadTroopBeforeWall.emplace_back( deadUnit );

                            // Check for overlay sprites of dead units (i.e. Resurrect spell).
                            for ( const Battle::UnitSpellEffectInfo & overlaySprite : _unitSpellEffectInfos ) {
                                if ( overlaySprite.unitId == deadUnit->GetUID() ) {
                                    troopOverlaySpriteBeforeWall.emplace_back( &overlaySprite );
                                }
                            }
                        }
                        else {
                            deadTroopAfterWall.emplace_back( deadUnit );

                            // Check for overlay sprites of dead units (i.e. Resurrect spell).
                            for ( const Battle::UnitSpellEffectInfo & overlaySprite : _unitSpellEffectInfos ) {
                                if ( overlaySprite.unitId == deadUnit->GetUID() ) {
                                    troopOverlaySpriteAfterWall.emplace_back( &overlaySprite );
                                }
                            }
                        }
                    }
                }

                const Cell * currentCell = Board::GetCell( cellId );
                const Unit * unitOnCell = currentCell->GetUnit();
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
                        if ( _movingPos.y <= currentCell->GetPos().y ) {
                            // The troop is moving horizontally or upwards. We should render it prior to this row units.
                            movingTroopBeforeWall.emplace_back( unitOnCell );
                        }
                        else {
                            downwardMovingTroopBeforeWall.emplace_back( unitOnCell );
                        }
                    }
                    else {
                        if ( _movingPos.y <= currentCell->GetPos().y ) {
                            // The troop is moving horizontally or upwards. We should render it prior to this row units.
                            movingTroopAfterWall.emplace_back( unitOnCell );
                        }
                        else {
                            downwardMovingTroopAfterWall.emplace_back( unitOnCell );
                        }
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

            for ( const Unit * unit : deadTroopBeforeWall ) {
                RedrawTroopSprite( *unit );
            }

            for ( const Unit * unit : movingTroopBeforeWall ) {
                RedrawTroopSprite( *unit );
            }

            for ( const Unit * unit : troopBeforeWall ) {
                RedrawTroopSprite( *unit );
            }

            for ( const Unit * unit : troopCounterBeforeWall ) {
                RedrawTroopCount( *unit );
            }

            for ( const Unit * unit : downwardMovingTroopBeforeWall ) {
                RedrawTroopSprite( *unit );
            }

            // Render the overlay sprite for units in current cell row above all units in this and upper rows.
            for ( const Battle::UnitSpellEffectInfo * overlaySprite : troopOverlaySpriteBeforeWall ) {
                assert( overlaySprite->icnId != ICN::UNKNOWN );
                const fheroes2::Sprite & spellSprite = fheroes2::AGG::GetICN( overlaySprite->icnId, overlaySprite->icnIndex );
                fheroes2::Blit( spellSprite, _mainSurface, overlaySprite->position.x, overlaySprite->position.y, overlaySprite->isReflectedImage );
            }

            RedrawCastle( *castle, wallCellId );

            for ( const Unit * unit : deadTroopAfterWall ) {
                RedrawTroopSprite( *unit );
            }

            for ( const Unit * unit : movingTroopAfterWall ) {
                RedrawTroopSprite( *unit );
            }

            for ( const Unit * unit : troopAfterWall ) {
                RedrawTroopSprite( *unit );
            }

            for ( const Unit * unit : troopCounterAfterWall ) {
                RedrawTroopCount( *unit );
            }

            for ( const Unit * unit : downwardMovingTroopAfterWall ) {
                RedrawTroopSprite( *unit );
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
            std::vector<const Unit *> downwardMovingTroop;
            std::vector<const UnitSpellEffectInfo *> troopOverlaySprite;

            for ( int32_t cellColumnId = 0; cellColumnId < Board::widthInCells; ++cellColumnId ) {
                const int32_t cellId = cellRowId * Board::widthInCells + cellColumnId;

                // Check for overlay sprites of dead units (i.e. Resurrect spell).
                for ( const Unit * deadUnit : arena.getGraveyardUnits( cellId ) ) {
                    for ( const Battle::UnitSpellEffectInfo & overlaySprite : _unitSpellEffectInfos ) {
                        if ( overlaySprite.unitId == deadUnit->GetUID() ) {
                            troopOverlaySprite.emplace_back( &overlaySprite );
                        }
                    }
                }

                const Cell * currentCell = Board::GetCell( cellId );
                const Unit * unitOnCell = currentCell->GetUnit();
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
                else if ( _movingPos.y <= currentCell->GetPos().y ) {
                    // The troop is moving horizontally or upwards. Render it prior to this row units.
                    RedrawTroopSprite( *unitOnCell );
                }
                else {
                    downwardMovingTroop.emplace_back( unitOnCell );
                }

                // Check for overlay sprites for 'unitOnCell'.
                for ( const Battle::UnitSpellEffectInfo & overlaySprite : _unitSpellEffectInfos ) {
                    if ( overlaySprite.unitId == unitOnCell->GetUID() ) {
                        troopOverlaySprite.emplace_back( &overlaySprite );
                    }
                }
            }

            // Redraw monsters.
            for ( const Unit * unit : troop ) {
                RedrawTroopSprite( *unit );
            }

            // Redraw monster counters.
            for ( const Unit * unit : troopCounter ) {
                RedrawTroopCount( *unit );
            }

            for ( const Unit * unit : downwardMovingTroop ) {
                RedrawTroopSprite( *unit );
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
}

void Battle::Interface::RedrawOpponents()
{
    if ( _attackingOpponent )
        _attackingOpponent->Redraw( _mainSurface );
    if ( _defendingOpponent )
        _defendingOpponent->Redraw( _mainSurface );

    RedrawOpponentsFlags();
}

void Battle::Interface::RedrawOpponentsFlags()
{
    auto getFlagIcn = []( const PlayerColor color ) {
        switch ( color ) {
        case PlayerColor::BLUE:
            return ICN::HEROFL00;
        case PlayerColor::GREEN:
            return ICN::HEROFL01;
        case PlayerColor::RED:
            return ICN::HEROFL02;
        case PlayerColor::YELLOW:
            return ICN::HEROFL03;
        case PlayerColor::ORANGE:
            return ICN::HEROFL04;
        case PlayerColor::PURPLE:
            return ICN::HEROFL05;
        default:
            break;
        }
        return ICN::HEROFL06;
    };

    if ( _attackingOpponent ) {
        const int icn = getFlagIcn( arena.getAttackingForce().GetColor() );
        const fheroes2::Sprite & flag = fheroes2::AGG::GetICN( icn, ICN::getAnimatedIcnIndex( icn, 0, animation_flags_frame ) );
        fheroes2::Blit( flag, _mainSurface, _attackingOpponent->Offset().x + OpponentSprite::LEFT_HERO_X_OFFSET + flag.x(),
                        _attackingOpponent->Offset().y + OpponentSprite::LEFT_HERO_Y_OFFSET + flag.y() );
    }

    if ( _defendingOpponent ) {
        const int icn = getFlagIcn( arena.getDefendingForce().GetColor() );
        const fheroes2::Sprite & flag = fheroes2::AGG::GetICN( icn, ICN::getAnimatedIcnIndex( icn, 0, animation_flags_frame ) );
        fheroes2::Blit( flag, _mainSurface,
                        _defendingOpponent->Offset().x + fheroes2::Display::DEFAULT_WIDTH - OpponentSprite::RIGHT_HERO_X_OFFSET - ( flag.x() + flag.width() ),
                        _defendingOpponent->Offset().y + OpponentSprite::RIGHT_HERO_Y_OFFSET + flag.y(), true );
    }
}

void Battle::Interface::RedrawTroopSprite( const Unit & unit )
{
    const bool isCurrentMonsterAction = ( _currentUnit == &unit && _spriteInsteadCurrentUnit != nullptr );

    const fheroes2::Sprite & monsterSprite = isCurrentMonsterAction ? *_spriteInsteadCurrentUnit : fheroes2::AGG::GetICN( unit.GetMonsterSprite(), unit.GetFrame() );

    fheroes2::Point drawnPosition;

    if ( unit.Modes( SP_STONE | CAP_MIRRORIMAGE ) ) {
        // Apply Stone or Mirror image visual effect.
        fheroes2::Sprite modifiedMonsterSprite( monsterSprite );
        const PAL::PaletteType paletteType = unit.Modes( SP_STONE ) ? PAL::PaletteType::GRAY : PAL::PaletteType::MIRROR_IMAGE;

        fheroes2::ApplyPalette( modifiedMonsterSprite, PAL::GetPalette( paletteType ) );

        drawnPosition = _drawTroopSprite( unit, modifiedMonsterSprite );
    }
    else {
        drawnPosition = _drawTroopSprite( unit, monsterSprite );
    }

    if ( _unitToHighlight == &unit ) {
        // Additional unit to pay attention to. Highlight this unit's contour.
        const fheroes2::Sprite & monsterContour = fheroes2::CreateContour( monsterSprite, GetArmyColorFromPlayerColor( unit.GetArmyColor() ) );
        fheroes2::Blit( monsterContour, _mainSurface, drawnPosition.x, drawnPosition.y, unit.isReflect() );
        setUnitTobeHighlighted( nullptr );
    }
    else if ( _currentUnit == &unit && _spriteInsteadCurrentUnit == nullptr ) {
        // Current unit's turn which is idling. Highlight this unit's contour.
        const fheroes2::Sprite & monsterContour = fheroes2::CreateContour( monsterSprite, _contourColor );
        fheroes2::Blit( monsterContour, _mainSurface, drawnPosition.x, drawnPosition.y, unit.isReflect() );
    }
}

fheroes2::Point Battle::Interface::_drawTroopSprite( const Unit & unit, const fheroes2::Sprite & troopSprite )
{
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
            const fheroes2::Rect & unitPosition = unit.GetRectPosition();
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
        const fheroes2::Rect & unitPosition = unit.GetRectPosition();
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
    const int tileInFront = Board::GetIndexDirection( monsterIndex, isReflected ? Battle::CellDirection::LEFT : Battle::CellDirection::RIGHT );
    const bool isValidFrontMonster = ( monsterIndex / Board::widthInCells ) == ( tileInFront == Board::widthInCells );

    int32_t sx = rt.x + ( isReflected ? -7 : rt.width - 13 );
    const int32_t sy = rt.y + rt.height - bar.height() - ( isReflected ? 21 : 9 );

    int xOffset = unit.animation.getTroopCountOffset( isReflected );
    // check if has unit standing in front
    if ( xOffset > 0 && isValidFrontMonster && Board::isValidIndex( tileInFront ) && Board::GetCell( tileInFront )->GetUnit() != nullptr )
        xOffset = 0;

    sx += isReflected ? -xOffset : xOffset;

    fheroes2::Copy( bar, 0, 0, _mainSurface, sx, sy, bar.width(), bar.height() );

    const fheroes2::Text text( fheroes2::abbreviateNumber( static_cast<int32_t>( unit.GetCount() ) ), fheroes2::FontType::smallWhite() );
    text.draw( sx + ( bar.width() - text.width() ) / 2, sy + 2, _mainSurface );
}

void Battle::Interface::RedrawCover()
{
    _redrawCoverStatic();

    const Bridge * bridge = Arena::GetBridge();
    if ( bridge && ( bridge->isDown() || _bridgeAnimation.animationIsRequired ) ) {
        uint32_t spriteIndex = bridge->isDestroyed() ? BridgeMovementAnimation::DESTROYED : BridgeMovementAnimation::DOWN_POSITION;

        if ( _bridgeAnimation.animationIsRequired ) {
            spriteIndex = _bridgeAnimation.currentFrameId;
        }

        const fheroes2::Sprite & bridgeImage = fheroes2::AGG::GetICN( ICN::getCastleIcnId( Arena::GetCastle()->GetRace() ), spriteIndex );
        fheroes2::Blit( bridgeImage, _mainSurface, bridgeImage.x(), bridgeImage.y() );
    }

    const Cell * cell = Board::GetCell( _currentCellIndex );
    const int cursorType = Cursor::Get().Themes();

    if ( cell && _currentUnit && Settings::Get().BattleShowMouseShadow() ) {
        std::set<const Cell *> highlightedCells;

        if ( humanturn_spell.isValid() ) {
            switch ( humanturn_spell.GetID() ) {
            case Spell::COLDRING: {
                for ( const int32_t & around : Board::GetAroundIndexes( _currentCellIndex ) ) {
                    const Cell * nearbyCell = Board::GetCell( around );
                    if ( nearbyCell != nullptr ) {
                        highlightedCells.emplace( nearbyCell );
                    }
                }
                break;
            }
            case Spell::FIREBALL:
            case Spell::METEORSHOWER: {
                highlightedCells.emplace( cell );
                for ( const int32_t & around : Board::GetAroundIndexes( _currentCellIndex ) ) {
                    const Cell * nearbyCell = Board::GetCell( around );
                    if ( nearbyCell != nullptr ) {
                        highlightedCells.emplace( nearbyCell );
                    }
                }
                break;
            }
            case Spell::FIREBLAST: {
                highlightedCells.emplace( cell );
                for ( const int32_t & around : Board::GetDistanceIndexes( _currentCellIndex, 2 ) ) {
                    const Cell * nearbyCell = Board::GetCell( around );
                    if ( nearbyCell != nullptr ) {
                        highlightedCells.emplace( nearbyCell );
                    }
                }
                break;
            }
            case Spell::TELEPORT: {
                switch ( cursorType ) {
                case Cursor::WAR_NONE:
                    highlightedCells.emplace( cell );
                    break;
                case Cursor::SP_TELEPORT:
                    if ( Board::isValidIndex( _teleportSpellSrcIdx ) ) {
                        const Unit * unitToTeleport = arena.GetTroopBoard( _teleportSpellSrcIdx );
                        assert( unitToTeleport != nullptr );

                        const Position pos = Position::GetPosition( *unitToTeleport, _currentCellIndex );
                        assert( pos.GetHead() != nullptr );

                        highlightedCells.emplace( pos.GetHead() );

                        if ( unitToTeleport->isWide() ) {
                            assert( pos.GetTail() != nullptr );

                            highlightedCells.emplace( pos.GetTail() );
                        }
                    }
                    else {
                        highlightedCells.emplace( cell );
                    }
                    break;
                default:
                    // This should never happen
                    assert( 0 );
                    break;
                }
                break;
            }
            default:
                highlightedCells.emplace( cell );
                break;
            }
        }
        else if ( _currentUnit->isAbilityPresent( fheroes2::MonsterAbilityType::AREA_SHOT )
                  && ( cursorType == Cursor::WAR_ARROW || cursorType == Cursor::WAR_BROKENARROW ) ) {
            highlightedCells.emplace( cell );
            for ( const int32_t & around : Board::GetAroundIndexes( _currentCellIndex ) ) {
                const Cell * nearbyCell = Board::GetCell( around );
                if ( nearbyCell != nullptr ) {
                    highlightedCells.emplace( nearbyCell );
                }
            }
        }
        else if ( _currentUnit->isWide() && ( cursorType == Cursor::WAR_MOVE || cursorType == Cursor::WAR_FLY ) ) {
            const Position pos = Position::GetReachable( *_currentUnit, _currentCellIndex );

            assert( pos.GetHead() != nullptr );
            assert( pos.GetTail() != nullptr );

            highlightedCells.emplace( pos.GetHead() );
            highlightedCells.emplace( pos.GetTail() );
        }
        else if ( cursorType == Cursor::SWORD_TOPLEFT || cursorType == Cursor::SWORD_TOPRIGHT || cursorType == Cursor::SWORD_BOTTOMLEFT
                  || cursorType == Cursor::SWORD_BOTTOMRIGHT || cursorType == Cursor::SWORD_LEFT || cursorType == Cursor::SWORD_RIGHT || cursorType == Cursor::SWORD_TOP
                  || cursorType == Cursor::SWORD_BOTTOM ) {
            highlightedCells.emplace( cell );

            AttackDirection direction = AttackDirection::UNKNOWN;
            if ( cursorType == Cursor::SWORD_TOPLEFT ) {
                direction = AttackDirection::BOTTOM_RIGHT;
            }
            else if ( cursorType == Cursor::SWORD_TOPRIGHT ) {
                direction = AttackDirection::BOTTOM_LEFT;
            }
            else if ( cursorType == Cursor::SWORD_BOTTOMLEFT ) {
                direction = AttackDirection::TOP_RIGHT;
            }
            else if ( cursorType == Cursor::SWORD_BOTTOMRIGHT ) {
                direction = AttackDirection::TOP_LEFT;
            }
            else if ( cursorType == Cursor::SWORD_LEFT ) {
                direction = AttackDirection::RIGHT;
            }
            else if ( cursorType == Cursor::SWORD_RIGHT ) {
                direction = AttackDirection::LEFT;
            }
            else if ( cursorType == Cursor::SWORD_TOP ) {
                direction = AttackDirection::BOTTOM;
            }
            else if ( cursorType == Cursor::SWORD_BOTTOM ) {
                direction = AttackDirection::TOP;
            }
            else {
                assert( 0 );
            }

            // Wide creatures can attack from top/bottom, we need to get the actual attack direction
            const CellDirection attackingCellDirection = getAttackingCellDirectionForAttackDirection( *_currentUnit, direction );
            const Position pos = calculateAttackPosition( *_currentUnit, _currentCellIndex, direction );
            assert( pos.GetHead() != nullptr );

            highlightedCells.emplace( pos.GetHead() );

            if ( _currentUnit->isWide() ) {
                assert( pos.GetTail() != nullptr );

                highlightedCells.emplace( pos.GetTail() );
            }

            if ( _currentUnit->isDoubleCellAttack() ) {
                const Cell * secondAttackedCell = Board::GetCell( _currentCellIndex, Board::GetReflectDirection( attackingCellDirection ) );

                if ( secondAttackedCell ) {
                    highlightedCells.emplace( secondAttackedCell );
                }
            }
            else if ( _currentUnit->isAllAdjacentCellsAttack() ) {
                for ( const int32_t nearbyIdx : Board::GetAroundIndexes( pos ) ) {
                    // Should already be highlighted
                    if ( nearbyIdx == _currentCellIndex ) {
                        continue;
                    }

                    const Cell * nearbyCell = Board::GetCell( nearbyIdx );
                    assert( nearbyCell != nullptr );

                    const Unit * nearbyUnit = nearbyCell->GetUnit();

                    if ( nearbyUnit && nearbyUnit->GetColor() != _currentUnit->GetCurrentColor() ) {
                        highlightedCells.emplace( nearbyCell );
                    }
                }
            }
        }
        else {
            highlightedCells.emplace( cell );
        }

        assert( !highlightedCells.empty() );

        const HeroBase * currentCommander = arena.GetCurrentCommander();

        for ( const Cell * highlightedCell : highlightedCells ) {
            assert( highlightedCell != nullptr );

            bool isApplicable = highlightedCell->isPassable( false );

            if ( isApplicable ) {
                const Unit * highlightedUnit = highlightedCell->GetUnit();

                isApplicable = highlightedUnit == nullptr || !humanturn_spell.isValid() || highlightedUnit->AllowApplySpell( humanturn_spell, currentCommander );
            }

            if ( isApplicable ) {
                fheroes2::Blit( _hexagonCursorShadow, _mainSurface, highlightedCell->GetPos().x, highlightedCell->GetPos().y );
            }
        }
    }
}

void Battle::Interface::_redrawBattleGround()
{
    // Battlefield background image.
    if ( _battleGroundIcn != ICN::UNKNOWN ) {
        const fheroes2::Sprite & cbkg = fheroes2::AGG::GetICN( _battleGroundIcn, 0 );
        fheroes2::Copy( cbkg, _battleGround );
    }

    // Objects near the left and right borders of the Battlefield.
    if ( _borderObjectsIcn != ICN::UNKNOWN ) {
        const fheroes2::Sprite & frng = fheroes2::AGG::GetICN( _borderObjectsIcn, 0 );
        fheroes2::Blit( frng, _battleGround, frng.x(), frng.y() );
    }

    // Big obstacles in the center of the Battlefield.
    if ( arena.GetICNCovr() != ICN::UNKNOWN ) {
        const fheroes2::Sprite & cover = fheroes2::AGG::GetICN( arena.GetICNCovr(), 0 );
        fheroes2::Blit( cover, _battleGround, cover.x(), cover.y() );
    }

    const Castle * castle = Arena::GetCastle();
    int castleBackgroundIcnId = ICN::UNKNOWN;

    if ( castle != nullptr ) {
        // Castle ground.
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
            // Did you add a new race? Add the appropriate logic for it.
            assert( 0 );
            break;
        }

        const fheroes2::Sprite & castleBackground = fheroes2::AGG::GetICN( castleBackgroundIcnId, 1 );
        fheroes2::Blit( castleBackground, _battleGround, castleBackground.x(), castleBackground.y() );

        // Moat.
        if ( castle->isBuild( BUILD_MOAT ) ) {
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MOATWHOL, 0 );
            fheroes2::Blit( sprite, _battleGround, sprite.x(), sprite.y() );
        }
    }

    // Battlefield grid.
    if ( Settings::Get().BattleShowGrid() ) {
        const Board & board = *Arena::GetBoard();

        for ( const Cell & cell : board ) {
            fheroes2::Blit( _hexagonGrid, _battleGround, cell.GetPos().x, cell.GetPos().y );
        }
    }

    // Ground obstacles.
    for ( int32_t cellId = 0; cellId < Board::sizeInCells; ++cellId ) {
        _redrawGroundObjects( cellId );
    }

    // Castle top wall.
    if ( castle != nullptr ) {
        const fheroes2::Sprite & sprite2 = fheroes2::AGG::GetICN( castleBackgroundIcnId, castle->isFortificationBuilt() ? 4 : 3 );
        fheroes2::Blit( sprite2, _battleGround, sprite2.x(), sprite2.y() );
    }
}

void Battle::Interface::_redrawCoverStatic()
{
    fheroes2::Copy( _battleGround, _mainSurface );

    const Settings & conf = Settings::Get();

    // Movement shadow.
    if ( !_movingUnit && conf.BattleShowMoveShadow() && _currentUnit && !( _currentUnit->GetCurrentControl() & CONTROL_AI ) ) {
        const fheroes2::Image & shadowImage = conf.BattleShowGrid() ? _hexagonGridShadow : _hexagonShadow;
        const Board & board = *Arena::GetBoard();

        for ( const Cell & cell : board ) {
            const Position pos = Position::GetReachable( *_currentUnit, cell.GetIndex() );
            if ( pos.GetHead() != nullptr ) {
                assert( pos.isValidForUnit( _currentUnit ) );

                fheroes2::Blit( shadowImage, _mainSurface, cell.GetPos().x, cell.GetPos().y );
            }
        }
    }
}

void Battle::Interface::RedrawCastle( const Castle & castle, const int32_t cellId )
{
    const int castleIcnId = ICN::getCastleIcnId( castle.GetRace() );

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

        if ( castle.isFortificationBuilt() ) {
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

        if ( castle.isBuild( BUILD_LEFTTURRET ) && ltower ) {
            index = ltower->isValid() ? 18 : 19;
        }
        else if ( Board::GetCell( cellId )->GetObject() == 1 ) {
            // Tower without built turret can be damaged by the Earthquake spell.
            index = 19;
        }

        const fheroes2::Sprite & towerSprite = fheroes2::AGG::GetICN( castleIcnId, index );
        fheroes2::Blit( towerSprite, _mainSurface, 443 + towerSprite.x(), 153 + towerSprite.y() );
    }
    else if ( Arena::CASTLE_BOTTOM_ARCHER_TOWER_POS == cellId ) {
        const Tower * rtower = Arena::GetTower( TowerType::TWR_RIGHT );
        uint32_t index = 17;

        if ( castle.isBuild( BUILD_RIGHTTURRET ) && rtower ) {
            index = rtower->isValid() ? 18 : 19;
        }
        else if ( Board::GetCell( cellId )->GetObject() == 1 ) {
            // Tower without built turret can be damaged by the Earthquake spell.
            index = 19;
        }

        const fheroes2::Sprite & towerSprite = fheroes2::AGG::GetICN( castleIcnId, index );
        fheroes2::Blit( towerSprite, _mainSurface, 443 + towerSprite.x(), 405 + towerSprite.y() );
    }
    else if ( Arena::CASTLE_TOP_GATE_TOWER_POS == cellId ) {
        const int index = ( Board::GetCell( cellId )->GetObject() == 1 ) ? 19 : 17;
        const fheroes2::Sprite & towerSprite = fheroes2::AGG::GetICN( castleIcnId, index );
        fheroes2::Blit( towerSprite, _mainSurface, 399 + towerSprite.x(), 237 + towerSprite.y() );
    }
    else if ( Arena::CASTLE_BOTTOM_GATE_TOWER_POS == cellId ) {
        const int index = ( Board::GetCell( cellId )->GetObject() == 1 ) ? 19 : 17;
        const fheroes2::Sprite & towerSprite = fheroes2::AGG::GetICN( castleIcnId, index );
        fheroes2::Blit( towerSprite, _mainSurface, 399 + towerSprite.x(), 321 + towerSprite.y() );
    }
}

void Battle::Interface::RedrawCastleMainTower( const Castle & castle )
{
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::getCastleIcnId( castle.GetRace() ), ( Arena::GetTower( TowerType::TWR_CENTER )->isValid() ? 20 : 26 ) );

    fheroes2::Blit( sprite, _mainSurface, sprite.x(), sprite.y() );
}

void Battle::Interface::_redrawGroundObjects( const int32_t cellId )
{
    const Cell * cell = Board::GetCell( cellId );
    if ( cell == nullptr )
        return;

    const int cellObjectId = cell->GetObject();
    if ( cellObjectId == 0 ) {
        // No object exists.
        return;
    }

    int objectIcnId = 0;

    switch ( cellObjectId ) {
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
    fheroes2::Blit( objectSprite, _battleGround, pt.x + pt.width / 2 + objectSprite.x(), pt.y + pt.height + objectSprite.y() + cellYOffset );
}

void Battle::Interface::_redrawHighObjects( const int32_t cellId )
{
    const Cell * cell = Board::GetCell( cellId );
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
    case 0x84:
        objectIcnId = ICN::COBJ0004;
        break;
    case 0x85:
        objectIcnId = ICN::COBJ0005;
        break;
    case 0x86:
        objectIcnId = ICN::COBJ0006;
        break;
    case 0x87:
        objectIcnId = ICN::COBJ0007;
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
    // Redraw killed troops.
    for ( const int32_t & cell : arena.getCellsOccupiedByGraveyard() ) {
        for ( const Unit * unit : arena.getGraveyardUnits( cell ) ) {
            if ( unit && cell != unit->GetTailIndex() ) {
                RedrawTroopSprite( *unit );
            }
        }
    }
}

int Battle::Interface::GetBattleCursor( std::string & statusMsg ) const
{
    statusMsg.clear();

    const Cell * cell = Board::GetCell( _currentCellIndex );

    if ( cell && _currentUnit ) {
        const auto formatViewInfoMsg = []( const Unit * unit ) {
            assert( unit != nullptr );

            std::string msg = _( "View %{monster} info" );
            StringReplaceWithLowercase( msg, "%{monster}", unit->GetMultiName() );

            return msg;
        };

        const Unit * unit = cell->GetUnit();

        if ( unit == nullptr || _currentUnit == unit ) {
            const Position pos = Position::GetReachable( *_currentUnit, _currentCellIndex );
            if ( pos.GetHead() != nullptr ) {
                assert( pos.isValidForUnit( _currentUnit ) );

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
            std::set<AttackDirection> availableAttackDirections;

            for ( const CellDirection direction : { CellDirection::BOTTOM_RIGHT, CellDirection::BOTTOM_LEFT, CellDirection::RIGHT, CellDirection::TOP_RIGHT,
                                                    CellDirection::TOP_LEFT, CellDirection::LEFT } ) {
                if ( Board::isValidDirection( _currentCellIndex, direction )
                     && Board::CanAttackFromCell( *_currentUnit, Board::GetIndexDirection( _currentCellIndex, direction ) ) ) {
                    availableAttackDirections.emplace( asAttackDirection( direction ) );
                }
            }

            // Wide monsters can also attack from the top and bottom.
            if ( _currentUnit->isWide() ) {
                const auto checkTopBottom = [this]( const CellDirection topOrBottomDirection, const CellDirection leftDirection, const CellDirection rightDirection ) {
                    const int32_t dst = Board::GetIndexDirection( _currentCellIndex, topOrBottomDirection );
                    if ( !Board::isValidDirection( _currentCellIndex, leftDirection ) || !Board::isValidDirection( _currentCellIndex, rightDirection ) ) {
                        return false;
                    }
                    const Position position = Position::GetReachable( *_currentUnit, dst );
                    const Cell * head = position.GetHead();
                    // Position must be both reachable and not "fixed" to be a tail attack. Tail attacks are covered above.
                    return head != nullptr && head->GetIndex() == dst;
                };

                // The attack from the top is performed from the head cell of the wide attacking unit in the direction of "back and down".
                const CellDirection topDirection = _currentUnit->isReflect() ? CellDirection::TOP_LEFT : CellDirection::TOP_RIGHT;
                if ( checkTopBottom( topDirection, CellDirection::TOP_LEFT, CellDirection::TOP_RIGHT ) ) {
                    availableAttackDirections.emplace( AttackDirection::TOP );
                }

                // The attack from the bottom is performed from the head cell of the wide attacking unit in the direction of "back and up".
                const CellDirection bottomDirection = _currentUnit->isReflect() ? CellDirection::BOTTOM_LEFT : CellDirection::BOTTOM_RIGHT;
                if ( checkTopBottom( bottomDirection, CellDirection::BOTTOM_LEFT, CellDirection::BOTTOM_RIGHT ) ) {
                    availableAttackDirections.emplace( AttackDirection::BOTTOM );
                }
            }

            if ( !availableAttackDirections.empty() ) {
                AttackDirection currentDirection = cell->GetTriangleDirection( getRelativeMouseCursorPos() );
                if ( currentDirection == AttackDirection::UNKNOWN ) {
                    // This could happen when another window has popped up and the user moved the mouse.
                    currentDirection = AttackDirection::CENTER;
                }

                if ( availableAttackDirections.count( currentDirection ) == 0 ) {
                    // This direction is not valid. Find the nearest one.
                    if ( availableAttackDirections.size() == 1 ) {
                        currentDirection = *availableAttackDirections.begin();
                    }
                    else {
                        // First search clockwise.
                        AttackDirection clockWiseDirection = currentDirection;
                        AttackDirection antiClockWiseDirection = currentDirection;

                        while ( true ) {
                            ++clockWiseDirection;
                            if ( availableAttackDirections.count( clockWiseDirection ) > 0 ) {
                                currentDirection = clockWiseDirection;
                                break;
                            }

                            --antiClockWiseDirection;
                            if ( availableAttackDirections.count( antiClockWiseDirection ) > 0 ) {
                                currentDirection = antiClockWiseDirection;
                                break;
                            }
                        }
                    }
                }

                const int cursor = getSwordCursorForAttackDirection( currentDirection );

                statusMsg = _( "Attack %{monster}" );
                StringReplaceWithLowercase( statusMsg, "%{monster}", unit->GetName() );

                return cursor;
            }
        }
    }

    statusMsg = _( "Turn %{turn}" );
    StringReplace( statusMsg, "%{turn}", arena.GetTurnNumber() );

    return Cursor::WAR_NONE;
}

int Battle::Interface::GetBattleSpellCursor( std::string & statusMsg ) const
{
    statusMsg.clear();

    const Cell * cell = Board::GetCell( _currentCellIndex );
    const Spell & spell = humanturn_spell;

    if ( cell && _currentUnit && spell.isValid() ) {
        const Unit * unitOnCell = cell->GetUnit();

        // Cursor is over some dead unit that we can resurrect
        if ( unitOnCell == nullptr && arena.isAbleToResurrectFromGraveyard( _currentCellIndex, spell ) ) {
            unitOnCell = arena.getLastResurrectableUnitFromGraveyard( _currentCellIndex, spell );
            assert( unitOnCell != nullptr && !unitOnCell->isValid() );
        }

        // Check the Teleport spell first
        if ( Board::isValidIndex( _teleportSpellSrcIdx ) ) {
            const Unit * unitToTeleport = arena.GetTroopBoard( _teleportSpellSrcIdx );
            assert( unitToTeleport != nullptr );

            if ( unitOnCell == nullptr && cell->isPassableForUnit( *unitToTeleport ) ) {
                statusMsg = _( "Teleport here" );

                return Cursor::SP_TELEPORT;
            }

            statusMsg = _( "Invalid teleport destination" );

            return Cursor::WAR_NONE;
        }

        if ( unitOnCell && unitOnCell->AllowApplySpell( spell, _currentUnit->GetCurrentOrArmyCommander() ) ) {
            statusMsg = _( "Cast %{spell} on %{monster}" );
            StringReplace( statusMsg, "%{spell}", spell.GetName() );
            StringReplaceWithLowercase( statusMsg, "%{monster}", unitOnCell->GetName() );

            return getCursorForSpell( spell.GetID() );
        }

        if ( !spell.isApplyToFriends() && !spell.isApplyToEnemies() && !spell.isApplyToAnyTroops() ) {
            statusMsg = _( "Cast %{spell}" );
            StringReplace( statusMsg, "%{spell}", spell.GetName() );

            return getCursorForSpell( spell.GetID() );
        }
    }

    statusMsg = _( "Select spell target" );

    return Cursor::WAR_NONE;
}

void Battle::Interface::getPendingActions( Actions & actions )
{
    if ( _interruptAutoCombatForColor != PlayerColor::NONE ) {
        actions.emplace_back( Command::TOGGLE_AUTO_COMBAT, static_cast<std::underlying_type_t<PlayerColor>>( _interruptAutoCombatForColor ) );

        _interruptAutoCombatForColor = PlayerColor::NONE;
    }
}

void Battle::Interface::HumanTurn( const Unit & unit, Actions & actions )
{
    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    // Reset the cursor position to avoid forcing the cursor shadow to be drawn at the last position of the previous turn.
    _currentCellIndex = -1;

    _currentUnit = &unit;
    humanturn_redraw = false;
    humanturn_exit = false;
    catapult_frame = 0;

    // in case we moved the window
    _interfacePosition = border.GetArea();

    popup.reset();

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
        if ( le.isMouseCursorPosInArea( { _interfacePosition.x, _interfacePosition.y, _interfacePosition.width, _interfacePosition.height - status.height } ) ) {
            indexNew = board->GetIndexAbsPosition( getRelativeMouseCursorPos() );
        }
        if ( _currentCellIndex != indexNew ) {
            _currentCellIndex = indexNew;
            humanturn_redraw = true;
        }

        if ( humanturn_spell.isValid() ) {
            HumanCastSpellTurn( unit, actions, msg );
        }
        else {
            HumanBattleTurn( unit, actions, msg );
        }

        // update status
        if ( msg != status.getMessage() ) {
            status.setMessage( msg, false );
            humanturn_redraw = true;
        }

        // animation troops
        if ( IdleTroopsAnimation() ) {
            humanturn_redraw = true;
        }

        CheckGlobalEvents( le );

        // redraw arena
        if ( humanturn_redraw ) {
            Redraw();
            humanturn_redraw = false;
        }
        else if ( listlog && listlog->IsNeedRedraw() ) {
            listlog->Redraw();
            fheroes2::Display::instance().render( listlog->GetArea() );
        }
    }

    popup.reset();

    _currentUnit = nullptr;
}

void Battle::Interface::HumanBattleTurn( const Unit & unit, Actions & actions, std::string & msg )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    Settings & conf = Settings::Get();

    BoardActionIntentUpdater boardActionIntentUpdater( _boardActionIntent, le.isMouseEventFromTouchpad() );

    _buttonAuto.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( _buttonAuto.area() ) );
    _buttonSettings.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( _buttonSettings.area() ) );
    _buttonSkip.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( _buttonSkip.area() ) );

    if ( le.isAnyKeyPressed() ) {
        // Skip the turn
        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_SKIP ) ) {
            actions.emplace_back( Command::SKIP, unit.GetUID() );
            humanturn_exit = true;
        }
        // Battle options
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_OPTIONS ) ) {
            EventShowOptions();
        }
        // Toggle the display of the battle turn order
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_TOGGLE_TURN_ORDER_DISPLAY ) ) {
            conf.setBattleShowTurnOrder( !conf.BattleShowTurnOrder() );

            humanturn_redraw = true;
        }
        // Switch the auto combat mode on
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_TOGGLE_AUTO_COMBAT ) ) {
            if ( fheroes2::showStandardTextMessage( {}, _( "Are you sure you want to enable the auto combat mode?" ), Dialog::YES | Dialog::NO ) == Dialog::YES ) {
                _startAutoCombat( unit, actions );
            }
        }
        // Resolve the combat in quick combat mode
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_QUICK_COMBAT ) ) {
            if ( fheroes2::showStandardTextMessage( {}, _( "Are you sure you want to resolve the battle in the quick combat mode?" ), Dialog::YES | Dialog::NO )
                 == Dialog::YES ) {
                _quickCombat( actions );
            }
        }
        // Cast the spell
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_CAST_SPELL ) ) {
            ProcessingHeroDialogResult( 1, actions );
        }
        // Retreat
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_RETREAT ) ) {
            ProcessingHeroDialogResult( 2, actions );
        }
        // Surrender
        else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_SURRENDER ) ) {
            ProcessingHeroDialogResult( 3, actions );
        }
    }

    // Add offsets to inner objects
    fheroes2::Rect battleFieldRect{ _interfacePosition.x, _interfacePosition.y, _interfacePosition.width, _interfacePosition.height - status.height };

    // Swipe attack motion finished, but the destination was outside the arena. We need to clear the swipe attack state.
    if ( !le.isDragInProgress() && Board::isValidIndex( _swipeAttack.srcCellIndex ) && !Board::isValidIndex( _currentCellIndex ) ) {
        _swipeAttack = {};
    }

    bool doListlogProcessing = listlog && listlog->isOpenLog();

    if ( doListlogProcessing ) {
        const fheroes2::Rect & lislogRect = listlog->GetArea();
        battleFieldRect.height -= lislogRect.height;

        // Do battle log event processing only if mouse pointer is over it.
        doListlogProcessing = le.isMouseCursorPosInArea( lislogRect ) || le.isMouseLeftButtonPressedInArea( lislogRect );
    }

    if ( doListlogProcessing ) {
        cursor.SetThemes( Cursor::WAR_POINTER );

        // Hide the possibly shown Damage Info pop-up dialog.
        popup.reset();

        listlog->QueueEventProcessing();
    }
    else if ( Arena::GetTower( TowerType::TWR_CENTER ) && le.isMouseCursorPosInArea( _ballistaTowerRect ) ) {
        cursor.SetThemes( Cursor::WAR_INFO );
        msg = _( "View Ballista info" );

        if ( le.MouseClickLeft( _ballistaTowerRect ) || le.isMouseRightButtonPressedInArea( _ballistaTowerRect ) ) {
            const Castle * cstl = Arena::GetCastle();
            std::string ballistaMessage = Tower::GetInfo( *cstl );

            if ( cstl->isBuild( BUILD_MOAT ) ) {
                ballistaMessage.append( "\n\n" );
                ballistaMessage.append( Battle::Board::GetMoatInfo() );
            }

            fheroes2::showStandardTextMessage( _( "Ballista" ), ballistaMessage, le.isMouseRightButtonPressed() ? Dialog::ZERO : Dialog::OK );
        }
    }
    else if ( conf.BattleShowTurnOrder() && le.isMouseCursorPosInArea( _turnOrder ) ) {
        cursor.SetThemes( Cursor::POINTER );
        if ( _turnOrder.queueEventProcessing( *this, msg, _interfacePosition.getPosition() ) ) {
            humanturn_redraw = true;
        }
    }
    else if ( le.isMouseCursorPosInArea( _buttonAuto.area() ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );

        msg = _( "Automatic combat modes" );

        if ( le.MouseClickLeft( _buttonAuto.area() ) ) {
            OpenAutoModeDialog( unit, actions );
        }
        else if ( le.isMouseRightButtonPressed() ) {
            fheroes2::showStandardTextMessage( _( "Automatic Combat Modes" ), _( "Choose between proceeding the combat in auto combat mode or in quick combat mode." ),
                                               Dialog::ZERO );
        }
    }
    else if ( le.isMouseCursorPosInArea( _buttonSettings.area() ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );

        msg = _( "Customize system options" );

        if ( le.MouseClickLeft( _buttonSettings.area() ) ) {
            _openBattleSettingsDialog();

            humanturn_redraw = true;
        }
        else if ( le.isMouseRightButtonPressed() ) {
            fheroes2::showStandardTextMessage( _( "System Options" ), _( "Allows you to customize the combat screen." ), Dialog::ZERO );
        }
    }
    else if ( le.isMouseCursorPosInArea( _buttonSkip.area() ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );

        msg = _( "Skip this unit" );

        if ( le.MouseClickLeft( _buttonSkip.area() ) ) {
            assert( _currentUnit != nullptr );

            actions.emplace_back( Command::SKIP, _currentUnit->GetUID() );

            humanturn_exit = true;
        }
        else if ( le.isMouseRightButtonPressed() ) {
            fheroes2::showStandardTextMessage( _( "Skip" ),
                                               _( "Skips the current creature. The current creature ends its turn and does not get to go again until the next round." ),
                                               Dialog::ZERO );
        }
    }
    else if ( _attackingOpponent && le.isMouseCursorPosInArea( _attackingOpponent->GetArea() + _interfacePosition.getPosition() ) ) {
        const fheroes2::Rect attackingOpponentArea = _attackingOpponent->GetArea() + _interfacePosition.getPosition();
        if ( arena.GetCurrentColor() == arena.getAttackingArmyColor() ) {
            if ( _attackingOpponent->GetHero()->isCaptain() ) {
                msg = _( "View Captain's options" );
            }
            else {
                msg = _( "View Hero's options" );
            }
            cursor.SetThemes( Cursor::WAR_HERO );

            if ( le.MouseClickLeft( attackingOpponentArea ) ) {
                ProcessingHeroDialogResult( arena.DialogBattleHero( *_attackingOpponent->GetHero(), true, status ), actions );
                humanturn_redraw = true;
            }
        }
        else {
            if ( _attackingOpponent->GetHero()->isCaptain() ) {
                msg = _( "View opposing Captain" );
            }
            else {
                msg = _( "View opposing Hero" );
            }
            cursor.SetThemes( Cursor::WAR_INFO );

            if ( le.MouseClickLeft( attackingOpponentArea ) ) {
                arena.DialogBattleHero( *_attackingOpponent->GetHero(), true, status );
                humanturn_redraw = true;
            }
        }

        if ( le.isMouseRightButtonPressedInArea( attackingOpponentArea ) ) {
            arena.DialogBattleHero( *_attackingOpponent->GetHero(), false, status );
            humanturn_redraw = true;
        }
    }
    else if ( _defendingOpponent && le.isMouseCursorPosInArea( _defendingOpponent->GetArea() + _interfacePosition.getPosition() ) ) {
        const fheroes2::Rect defendingOpponentArea = _defendingOpponent->GetArea() + _interfacePosition.getPosition();
        if ( arena.GetCurrentColor() == arena.getDefendingForce().GetColor() ) {
            if ( _defendingOpponent->GetHero()->isCaptain() ) {
                msg = _( "View Captain's options" );
            }
            else {
                msg = _( "View Hero's options" );
            }

            cursor.SetThemes( Cursor::WAR_HERO );

            if ( le.MouseClickLeft( defendingOpponentArea ) ) {
                ProcessingHeroDialogResult( arena.DialogBattleHero( *_defendingOpponent->GetHero(), true, status ), actions );
                humanturn_redraw = true;
            }
        }
        else {
            if ( _defendingOpponent->GetHero()->isCaptain() ) {
                msg = _( "View opposing Captain" );
            }
            else {
                msg = _( "View opposing Hero" );
            }

            cursor.SetThemes( Cursor::WAR_INFO );

            if ( le.MouseClickLeft( defendingOpponentArea ) ) {
                arena.DialogBattleHero( *_defendingOpponent->GetHero(), true, status );
                humanturn_redraw = true;
            }
        }

        if ( le.isMouseRightButtonPressedInArea( defendingOpponentArea ) ) {
            arena.DialogBattleHero( *_defendingOpponent->GetHero(), false, status );
            humanturn_redraw = true;
        }
    }
    else if ( le.isMouseCursorPosInArea( battleFieldRect ) ) {
        int themes = GetBattleCursor( msg );

        if ( _swipeAttack.isValid() ) {
            // The swipe attack motion is either in progress or has finished.
            if ( _currentCellIndex == _swipeAttack.dstCellIndex ) {
                // The cursor is above the stored destination, we should display the stored attack theme.
                themes = _swipeAttack.dstTheme;
            }
            else {
                // The cursor has left the destination. Abort the swipe attack.
                _swipeAttack = {};
                _boardActionIntent = {};
            }
        }
        else if ( _swipeAttack.isValidDestination( themes, _currentCellIndex ) ) {
            // Valid swipe attack target cell. Calculate the attack angle based on destination and source cells.
            themes = getSwordCursorForAttackDirection( asAttackDirection( Board::GetDirection( _currentCellIndex, _swipeAttack.srcCellIndex ) ) );

            // Remember the swipe destination cell and theme.
            _swipeAttack.setDst( themes, _currentCellIndex );

            // Clear any pending intents. We don't want to confirm previous actions by performing swipe attack motion.
            _boardActionIntent = {};
        }

        cursor.SetThemes( themes );

        const Cell * cell = Board::GetCell( _currentCellIndex );
        if ( cell ) {
            if ( CursorAttack( themes ) ) {
                popup.setAttackInfo( _currentUnit, cell->GetUnit() );
            }
            else {
                popup.reset();
            }

            boardActionIntentUpdater.setIntent( { themes, _currentCellIndex } );

            if ( le.MouseClickLeft( battleFieldRect ) ) {
                const bool isConfirmed = boardActionIntentUpdater.isConfirmed();

                if ( isConfirmed ) {
                    // Intent is confirmed, it is safe to clear the swipe state (regardless of the intent and the input method).
                    _swipeAttack = {};
                }

                MouseLeftClickBoardAction( themes, *cell, isConfirmed, actions );
            }
            else if ( le.isMouseRightButtonPressed() && MousePressRightBoardAction( *cell ) ) {
                humanturn_redraw = true;
            }
            else if ( le.isMouseLeftButtonPressedInArea( battleFieldRect ) ) {
                if ( !le.isDragInProgress() && !_swipeAttack.isValid() ) {
                    le.registerDrag();

                    // Remember the swipe source cell and theme.
                    _swipeAttack = {};
                    _swipeAttack.setSrc( themes, _currentCellIndex, _currentUnit );
                }
            }
        }
        else {
            le.MouseClickLeft();
            le.MouseClickRight();

            // Reset the shown Damage Info pop-up when the cursor is moved out of the cells area.
            popup.reset();
        }
    }
    else if ( le.isMouseCursorPosInArea( status ) ) {
        // Hide the possibly shown Damage Info pop-up dialog.
        popup.reset();

        if ( listlog ) {
            msg = ( listlog->isOpenLog() ? _( "Hide logs" ) : _( "Show logs" ) );

            if ( le.MouseClickLeft( status ) ) {
                listlog->SetOpenLog( !listlog->isOpenLog() );
            }
            else if ( le.isMouseRightButtonPressedInArea( status ) ) {
                fheroes2::showStandardTextMessage( _( "Message Bar" ), _( "Shows the results of individual monster's actions." ), Dialog::ZERO );
            }
        }

        cursor.SetThemes( Cursor::WAR_POINTER );
    }
    else {
        cursor.SetThemes( Cursor::WAR_NONE );

        le.MouseClickLeft();
        le.MouseClickRight();
    }
}

void Battle::Interface::HumanCastSpellTurn( const Unit & /* unused */, Actions & actions, std::string & msg )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    BoardActionIntentUpdater boardActionIntentUpdater( _boardActionIntent, le.isMouseEventFromTouchpad() );

    // Cancel the spellcast
    if ( le.isMouseRightButtonPressed() || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
        popup.reset();

        humanturn_spell = Spell::NONE;

        _teleportSpellSrcIdx = -1;
    }
    else if ( le.isMouseCursorPosInArea( _interfacePosition ) && humanturn_spell.isValid() ) {
        const int themes = GetBattleSpellCursor( msg );
        cursor.SetThemes( themes );

        if ( const Cell * cell = Board::GetCell( _currentCellIndex ); cell && _currentUnit && cell->GetUnit() ) {
            popup.setSpellAttackInfo( _currentUnit->GetCurrentOrArmyCommander(), cell->GetUnit(), humanturn_spell );
        }
        else {
            popup.reset();
        }

        boardActionIntentUpdater.setIntent( { themes, _currentCellIndex } );

        if ( le.MouseClickLeft() && Cursor::WAR_NONE != cursor.Themes() && boardActionIntentUpdater.isConfirmed() ) {
            if ( !Board::isValidIndex( _currentCellIndex ) ) {
                DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Spell destination is out of range: " << _currentCellIndex )
                return;
            }

            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, humanturn_spell.GetName() << ", dst: " << _currentCellIndex )

            if ( Cursor::SP_TELEPORT == cursor.Themes() ) {
                if ( _teleportSpellSrcIdx < 0 ) {
                    _teleportSpellSrcIdx = _currentCellIndex;
                }
                else {
                    actions.emplace_back( Command::SPELLCAST, Spell::TELEPORT, _teleportSpellSrcIdx, _currentCellIndex );

                    humanturn_spell = Spell::NONE;
                    humanturn_exit = true;

                    _teleportSpellSrcIdx = -1;
                }
            }
            else if ( Cursor::SP_MIRRORIMAGE == cursor.Themes() ) {
                actions.emplace_back( Command::SPELLCAST, Spell::MIRRORIMAGE, _currentCellIndex );

                humanturn_spell = Spell::NONE;
                humanturn_exit = true;
            }
            else {
                actions.emplace_back( Command::SPELLCAST, humanturn_spell.GetID(), _currentCellIndex );

                humanturn_spell = Spell::NONE;
                humanturn_exit = true;
            }
        }
    }
    else {
        cursor.SetThemes( Cursor::WAR_NONE );
    }
}

void Battle::Interface::FadeArena( const bool clearMessageLog )
{
    AudioManager::ResetAudio();

    fheroes2::Display & display = fheroes2::Display::instance();

    if ( clearMessageLog ) {
        status.clear();
        status.redraw( display );
    }

    Redraw();

    const fheroes2::Rect srt = border.GetArea();
    fheroes2::Image top;
    top._disableTransformLayer();
    top.resize( srt.width, srt.height );

    fheroes2::Copy( display, srt.x, srt.y, top, 0, 0, srt.width, srt.height );
    fheroes2::FadeDisplayWithPalette( top, srt.getPosition(), 5, 300, 5 );

    display.render();
}

void Battle::Interface::_openBattleSettingsDialog()
{
    const Settings & conf = Settings::Get();
    const bool showGrid = conf.BattleShowGrid();

    DialogBattleSettings();

    if ( showGrid != conf.BattleShowGrid() ) {
        // The grid setting has changed. Update for the Battlefield ground.
        _redrawBattleGround();
    }
}

void Battle::Interface::EventShowOptions()
{
    _buttonSettings.drawOnPress();
    _openBattleSettingsDialog();
    _buttonSettings.drawOnRelease();
    humanturn_redraw = true;
}

void Battle::Interface::_startAutoCombat( const Unit & unit, Actions & actions )
{
    // TODO: remove this temporary assertion
    assert( arena.CanToggleAutoCombat() && !arena.AutoCombatInProgress() );

    actions.emplace_back( Command::TOGGLE_AUTO_COMBAT, static_cast<std::underlying_type_t<PlayerColor>>( unit.GetCurrentOrArmyColor() ) );

    humanturn_redraw = true;
    humanturn_exit = true;
}

void Battle::Interface::_quickCombat( Actions & actions )
{
    actions.emplace_back( Command::QUICK_COMBAT );

    humanturn_redraw = true;
    humanturn_exit = true;
}

void Battle::Interface::OpenAutoModeDialog( const Unit & unit, Actions & actions )
{
    Cursor::Get().SetThemes( Cursor::POINTER );

    const fheroes2::FontType buttonFontType = fheroes2::FontType::buttonReleasedWhite();
    fheroes2::ButtonGroup autoButtons(
        { fheroes2::getSupportedText( gettext_noop( "AUTO\nCOMBAT" ), buttonFontType ), fheroes2::getSupportedText( gettext_noop( "QUICK\nCOMBAT" ), buttonFontType ) } );

    const int32_t titleYOffset = 16;

    const fheroes2::Text title( _( "Automatic Combat Modes" ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } );

    fheroes2::ButtonBase & autoCombatButton = autoButtons.button( 0 );
    fheroes2::ButtonBase & quickCombatButton = autoButtons.button( 1 );

    fheroes2::StandardWindow background( autoButtons, false, titleYOffset + title.height() );

    background.renderSymmetricButtons( autoButtons, titleYOffset + title.height(), false );

    fheroes2::Button buttonCancel;

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    background.renderButton( buttonCancel, isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1, { 0, 11 },
                             fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

    const fheroes2::Rect roiArea = background.activeArea();

    fheroes2::Display & display = fheroes2::Display::instance();

    title.draw( roiArea.x, roiArea.y + titleYOffset, roiArea.width, display );

    display.render( background.totalArea() );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        autoCombatButton.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( autoCombatButton.area() ) );
        quickCombatButton.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( quickCombatButton.area() ) );
        buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );

        if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            return;
        }
        if ( le.MouseClickLeft( autoCombatButton.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_TOGGLE_AUTO_COMBAT ) ) {
            _startAutoCombat( unit, actions );
            return;
        }
        if ( le.MouseClickLeft( quickCombatButton.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_QUICK_COMBAT ) ) {
            _quickCombat( actions );
            return;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( autoCombatButton.area() ) ) {
            std::string msg = _( "The computer continues the combat for you." );
            msg += "\n\n";
            msg += _(
                "autoCombat|This can be interrupted at any moment by pressing the Auto Combat hotkey or the default Cancel key, or by performing a left or right click anywhere on the game screen." );
            fheroes2::showStandardTextMessage( _( "Auto Combat" ), msg, Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( quickCombatButton.area() ) ) {
            std::string msg = _( "The combat is resolved from the current state." );
            msg += "\n\n";
            msg += _( "quickCombat|This cannot be undone." );
            fheroes2::showStandardTextMessage( _( "Quick Combat" ), msg, Dialog::ZERO );
        }
    }
}

bool Battle::Interface::MousePressRightBoardAction( const Cell & cell ) const
{
    const auto getUnit = [this, &cell]() -> const Unit * {
        if ( const Unit * unit = cell.GetUnit(); unit != nullptr ) {
            return unit;
        }

        if ( const Unit * unit = arena.getLastResurrectableUnitFromGraveyard( cell.GetIndex() ); unit != nullptr ) {
            return unit;
        }

        if ( const Unit * unit = arena.getLastUnitFromGraveyard( cell.GetIndex() ); unit != nullptr ) {
            return unit;
        }

        return nullptr;
    };

    if ( const Unit * unit = getUnit(); unit != nullptr ) {
        Dialog::ArmyInfo( *unit, Dialog::ZERO, unit->isReflect() );
        return true;
    }

    return false;
}

void Battle::Interface::MouseLeftClickBoardAction( const int themes, const Cell & cell, const bool isConfirmed, Actions & actions )
{
    const int32_t index = cell.GetIndex();

    if ( _currentUnit ) {
        switch ( themes ) {
        case Cursor::WAR_FLY:
        case Cursor::WAR_MOVE: {
            if ( !isConfirmed ) {
                break;
            }

            const int32_t targetCellIdx = [this, index]() {
                if ( !_currentUnit->isWide() ) {
                    return index;
                }

                const Position pos = Position::GetReachable( *_currentUnit, index );
                assert( pos.GetHead() != nullptr && pos.GetTail() != nullptr );

                return pos.GetHead()->GetIndex();
            }();

            actions.emplace_back( Command::MOVE, _currentUnit->GetUID(), targetCellIdx );

            humanturn_exit = true;

            break;
        }

        case Cursor::SWORD_TOPLEFT:
        case Cursor::SWORD_TOP:
        case Cursor::SWORD_TOPRIGHT:
        case Cursor::SWORD_RIGHT:
        case Cursor::SWORD_BOTTOMRIGHT:
        case Cursor::SWORD_BOTTOM:
        case Cursor::SWORD_BOTTOMLEFT:
        case Cursor::SWORD_LEFT: {
            if ( !isConfirmed ) {
                break;
            }

            const Unit * unitOnCell = cell.GetUnit();
            if ( !unitOnCell ) {
                break;
            }

            const AttackDirection dir = getAttackDirectionForSwordCursor( themes );

            // Wide creatures can attack from top/bottom, we need to get the actual attack direction
            const CellDirection attackingCellDir = getAttackingCellDirectionForAttackDirection( *_currentUnit, dir );
            if ( !Board::isValidDirection( index, attackingCellDir ) ) {
                break;
            }

            if ( !_currentUnit->isWide() ) {
                const int32_t move = Board::GetIndexDirection( index, attackingCellDir );

                actions.emplace_back( Command::ATTACK, _currentUnit->GetUID(), unitOnCell->GetUID(), ( _currentUnit->GetHeadIndex() == move ? -1 : move ), index,
                                      static_cast<int>( Board::GetReflectDirection( attackingCellDir ) ) );

                humanturn_exit = true;

                break;
            }

            const Position pos = calculateAttackPosition( *_currentUnit, index, dir );
            assert( pos.isValidForUnit( _currentUnit ) );

            actions.emplace_back( Command::ATTACK, _currentUnit->GetUID(), unitOnCell->GetUID(),
                                  ( _currentUnit->GetHeadIndex() == pos.GetHead()->GetIndex() ? -1 : pos.GetHead()->GetIndex() ), index,
                                  static_cast<int>( Board::GetReflectDirection( attackingCellDir ) ) );

            humanturn_exit = true;

            break;
        }

        case Cursor::WAR_BROKENARROW:
        case Cursor::WAR_ARROW: {
            if ( !isConfirmed ) {
                break;
            }

            const Unit * unitOnCell = cell.GetUnit();
            if ( !unitOnCell ) {
                break;
            }

            actions.emplace_back( Command::ATTACK, _currentUnit->GetUID(), unitOnCell->GetUID(), -1, index, 0 );

            humanturn_exit = true;

            break;
        }

        case Cursor::WAR_INFO: {
            const Unit * unitOnCell = cell.GetUnit();
            if ( !unitOnCell ) {
                break;
            }

            Dialog::ArmyInfo( *unitOnCell, Dialog::BUTTONS, unitOnCell->isReflect() );

            humanturn_redraw = true;

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

void Battle::Interface::AnimateUnitWithDelay( Unit & unit, const bool skipLastFrameRender /* = false */ )
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

void Battle::Interface::_animateOpponents( OpponentSprite * hero )
{
    if ( hero == nullptr ) {
        return;
    }

    // Render the first animation frame. We do it here not to skip small animations with duration for 1 frame.
    // For such (one frame) animations we are already ad the end of animation and `isFinishFrame()` always will return true.
    Redraw();

    LocalEvent & le = LocalEvent::Get();

    // We need to wait this delay before rendering the first frame of hero animation.
    Game::AnimateResetDelay( Game::DelayType::BATTLE_OPPONENTS_DELAY );

    // 'BATTLE_OPPONENTS_DELAY' is different than 'BATTLE_IDLE_DELAY', so we handle the idle animation separately in this loop.
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_OPPONENTS_DELAY, Game::BATTLE_IDLE_DELAY } ) ) ) {
        // Animate the idling units.
        if ( IdleTroopsAnimation() ) {
            Redraw();
        }

        if ( Game::validateAnimationDelay( Game::BATTLE_OPPONENTS_DELAY ) ) {
            if ( hero->isFinishFrame() ) {
                // We have reached the end of animation.
                break;
            }

            hero->IncreaseAnimFrame();

            // Render the next frame and then wait a delay before checking if it is the last frame in the animation.
            Redraw();
        }
    }
}

void Battle::Interface::RedrawTroopDefaultDelay( Unit & unit )
{
    if ( unit.isFinishAnimFrame() ) {
        // Nothing to animate.
        return;
    }

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

void Battle::Interface::RedrawActionSkipStatus( const Unit & unit )
{
    std::string msg = _n( "The %{name} skips their turn.", "The %{name} skip their turn.", unit.GetCount() );
    StringReplaceWithLowercase( msg, "%{name}", unit.GetName() );

    setStatus( msg, true );
}

void Battle::Interface::RedrawMissileAnimation( const fheroes2::Point & startPos, const fheroes2::Point & endPos, const double angle, const uint32_t monsterID )
{
    LocalEvent & le = LocalEvent::Get();

    const bool reverse = startPos.x > endPos.x;
    const bool isMage = ( monsterID == Monster::MAGE || monsterID == Monster::ARCHMAGE );

    fheroes2::Sprite missile;
    fheroes2::Point endPosShift{ 0, 0 };

    // Mage is channeling the bolt; doesn't have missile sprite
    if ( isMage ) {
        fheroes2::delayforMs( Game::ApplyBattleSpeed( 115 ) );
    }
    else {
        missile = fheroes2::AGG::GetICN( static_cast<int>( Monster::GetMissileICN( monsterID ) ),
                                         static_cast<uint32_t>( Bin_Info::GetMonsterInfo( monsterID ).getProjectileID( angle ) ) );

        // The projectile has to hit the target but not go through it so its end position is shifted in the direction to the shooter.
        endPosShift.x = reverse ? ( missile.width() / 2 ) : -( missile.width() / 2 );
        endPosShift.y = ( startPos.y > endPos.y ) ? ( missile.height() / 2 ) : -( missile.height() / 2 );
    }

    // Lich/Power lich has projectile speed of 25
    const std::vector<fheroes2::Point> points = getLinePoints( startPos, endPos + endPosShift, isMage ? 50 : std::max( missile.width(), 25 ) );
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
    if ( listlog == nullptr ) {
        return;
    }

    std::string msg = _( "Turn %{turn}" );
    StringReplace( msg, "%{turn}", arena.GetTurnNumber() );

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

    const bool archer = attacker.isArchers() && !Unit::isHandFighting( attacker, defender );
    const bool isDoubleCell = attacker.isDoubleCellAttack() && 2 == targets.size();

    // redraw luck animation
    if ( attacker.Modes( LUCK_GOOD | LUCK_BAD ) ) {
        RedrawActionLuck( attacker );
    }

    AudioManager::PlaySound( attacker.M82Attk( defender ) );

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
        const int rearCenterX = ( attacker.isWide() && attacker.isReflect() ) ? pos1.width * 3 / 4 : Cell::widthPx / 2;
        const fheroes2::Point shooterPos( pos1.x + rearCenterX, attackerPos.y - attackerSprite.y() );

        // Use the front one to calculate the angle, then overwrite
        fheroes2::Point offset = attacker.GetStartMissileOffset( Monster_Info::FRONT );

        const fheroes2::Point targetPos = defender.GetCenterPoint();

        double angle = GetAngle( fheroes2::Point( shooterPos.x + offset.x, shooterPos.y + offset.y ), targetPos );
        // This check is made only for situations when a target stands on the same row as shooter.
        if ( attacker.GetHeadIndex() / Board::widthInCells == defender.GetHeadIndex() / Board::widthInCells ) {
            angle = 0;
        }

        // Angles are used in Heroes2 as 90 (TOP) -> 0 (FRONT) -> -90 (BOT) degrees
        const int direction = angle >= 25.0 ? Monster_Info::TOP : ( angle <= -25.0 ) ? Monster_Info::BOTTOM : Monster_Info::FRONT;

        if ( direction != Monster_Info::FRONT ) {
            offset = attacker.GetStartMissileOffset( direction );
        }

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
    _redrawActionWincesKills( targets, &attacker, &defender );
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

            for ( const TargetInfo & target : targets ) {
                if ( !target.defender->isModes( CAP_MIRRORIMAGE ) ) {
                    killed += target.killed;
                    damage += target.damage;
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

        setStatus( msg, true );

        if ( resurrects != 0 ) {
            const auto updateStatusBar = [this]( std::string & localMsg, const uint32_t localRes, const char * localUnit ) {
                StringReplace( localMsg, "%{count}", localRes );
                StringReplaceWithLowercase( localMsg, "%{unit}", localUnit );

                setStatus( localMsg, true );
            };

            if ( attacker.isAbilityPresent( fheroes2::MonsterAbilityType::SOUL_EATER ) ) {
                msg = _n( "1 soul is incorporated.", "%{count} souls are incorporated.", resurrects );
                updateStatusBar( msg, resurrects, attacker.GetPluralName( resurrects ) );
            }
            else if ( attacker.isAbilityPresent( fheroes2::MonsterAbilityType::HP_DRAIN ) ) {
                msg = _n( "1 %{unit} is revived.", "%{count} %{unit} are revived.", resurrects );
                updateStatusBar( msg, resurrects, attacker.GetPluralName( resurrects ) );
            }
        }
    }

    _movingUnit = nullptr;
}

void Battle::Interface::_redrawActionWincesKills( const TargetsInfo & targets, Unit * attacker /* = nullptr */, const Unit * defender /* = nullptr */ )
{
    // Reset the delay to wait till the next frame.
    if ( !Game::isDelayNeeded( { Game::DelayType::BATTLE_FRAME_DELAY } ) ) {
        Game::AnimateResetDelay( Game::DelayType::BATTLE_FRAME_DELAY );
    }

    LocalEvent & le = LocalEvent::Get();

    // Number of targets to be animated with a wince or kill animation.
    ptrdiff_t animatingTargets = 0;
    PlayerColor deathColor = PlayerColor::UNUSED;

    std::vector<Unit *> mirrorImages;
    std::set<Unit *> resistantTarget;
    std::set<int> unitSounds;

    // If this was a Lich attack, we should render an explosion cloud over the target unit immediately after the projectile hits the target,
    // along with the unit kill/wince animation.
    const bool drawLichCloud = ( attacker != nullptr ) && ( defender != nullptr ) && attacker->isArchers() && !Unit::isHandFighting( *attacker, *defender )
                               && attacker->isAbilityPresent( fheroes2::MonsterAbilityType::AREA_SHOT );

    // Play sound only if it is not already playing.
    const auto playSoundIfNotPlaying = [&unitSounds]( const int unitSound ) {
        const auto [dummy, isUnique] = unitSounds.insert( unitSound );
        if ( isUnique ) {
            AudioManager::PlaySound( unitSound );
        }
    };

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
            playSoundIfNotPlaying( unit->M82Kill() );
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
                playSoundIfNotPlaying( unit->M82Wnce() );
            }
            ++animatingTargets;
        }
        else {
            // have immunity
            resistantTarget.insert( target.defender );
            playSoundIfNotPlaying( M82::RSBRYFZL );
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

        // Make a check if all animation sequences are over (after rendering the last frame) to break this render loop.
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

        // Progress all units animations.
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

                    playSoundIfNotPlaying( target.defender->M82Wnce() );
                }
                else if ( drawLichCloud && lichCloudFrame == wnceDownStartFrame && ( target.defender->GetAnimationState() == Monster_Info::WNCE_UP ) ) {
                    target.defender->SwitchAnimation( Monster_Info::WNCE_DOWN );
                }
                else {
                    target.defender->IncreaseAnimFrame();
                }
            }
        }
    }

    // Fade away animation for destroyed mirror images
    if ( !mirrorImages.empty() ) {
        RedrawActionRemoveMirrorImage( mirrorImages );
    }
}

void Battle::Interface::SetHeroAnimationReactionToTroopDeath( const PlayerColor deathColor ) const
{
    if ( deathColor == PlayerColor::UNUSED ) {
        return;
    }

    const bool attackersTurn = ( deathColor == arena.getDefendingArmyColor() );
    OpponentSprite * attackingHero = attackersTurn ? _attackingOpponent.get() : _defendingOpponent.get();
    OpponentSprite * defendingHero = attackersTurn ? _defendingOpponent.get() : _attackingOpponent.get();
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

    if ( unit.Modes( SP_HASTE ) && frameDelay > 1 ) {
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
    StringReplace( msg, "%{src}",
                   std::to_string( ( unit.GetHeadIndex() / Board::widthInCells ) + 1 ) + ", " + std::to_string( ( unit.GetHeadIndex() % Board::widthInCells ) + 1 ) );

    assert( _movingUnit == nullptr && _flyingUnit == nullptr );

    _currentUnit = nullptr;
    _movingUnit = &unit;

    // If it is a flying creature that acts like walking one when it is under the Slow spell.
    const bool canFly = unit.isAbilityPresent( fheroes2::MonsterAbilityType::FLYING );
    // If it is a wide creature (cache this boolean to use in the loop).
    const bool isWide = unit.isWide();
    const Indexes::const_iterator pathEnd = path.end();
    const Indexes::const_iterator finalStep = pathEnd - 1;

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
            assert( Board::GetDirection( path[1], path[2] ) == ( unit.isReflect() ? CellDirection::LEFT : CellDirection::RIGHT ) );

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

    // For Battle speed 9 and 10 we play one sound at a time without any simultaneous playbacks
    // because on these speeds the unit will end its movement before the sound is finished.
    const bool playSoundBySound = Settings::Get().BattleSpeed() > 8;
    int soundStatus = -1;
    const int walkSoundId = unit.M82Move();

    while ( dst != pathEnd ) {
        // Check if a wide unit changes its horizontal direction.
        if ( isWide && unit.GetTailIndex() == *dst ) {
            // We must not reflect the flyers at the and of the path (just before the landing).
            if ( !canFly || ( dst != finalStep ) ) {
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
            if ( dst == finalStep ) {
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
            _movingPos.x -= Cell::widthPx;
        }

        // TODO: adjust sounds calls and synchronize them with frames. Take into account that some sounds (like for Cavalry) consists of a sequence of steps.
        if ( playSoundBySound ) {
            // Start unit move sound only if it is not already playing.
            if ( soundStatus < 0 || !Mixer::isPlaying( soundStatus ) ) {
                soundStatus = AudioManager::PlaySound( walkSoundId );
            }
        }
        else {
            AudioManager::PlaySound( walkSoundId );
        }

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
                if ( dst == finalStep ) {
                    // There is only one cell left to move after standing.
                    unit.SwitchAnimation( Monster_Info::MOVE_QUICK );
                }
                else {
                    // If the path has more than one step after the bridge action then begin the movement.
                    unit.SwitchAnimation( Monster_Info::MOVE_START );
                }
            }
            else if ( dst == finalStep ) {
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

    StringReplace( msg, "%{dst}",
                   std::to_string( ( unit.GetHeadIndex() / Board::widthInCells ) + 1 ) + ", " + std::to_string( ( unit.GetHeadIndex() % Board::widthInCells ) + 1 ) );

    status.setMessage( msg, true );

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
        targetPos.x -= Cell::widthPx; // this is needed to avoid extra cell shifting upon landing when we move to right side
    }

    std::string msg = _( "Moved %{monster}: from [%{src}] to [%{dst}]." );
    StringReplaceWithLowercase( msg, "%{monster}", unit.GetName() );
    StringReplace( msg, "%{src}",
                   std::to_string( ( unit.GetHeadIndex() / Board::widthInCells ) + 1 ) + ", " + std::to_string( ( unit.GetHeadIndex() % Board::widthInCells ) + 1 ) );

    const uint32_t step = unit.animation.getFlightSpeed();
    uint32_t frameDelay = Game::ApplyBattleSpeed( unit.animation.getMoveSpeed() );

    if ( unit.Modes( SP_HASTE ) && frameDelay > 1 ) {
        frameDelay = frameDelay * 8 / 10; // 20% faster
    }
    else if ( unit.Modes( SP_SLOW ) ) {
        frameDelay = frameDelay * 12 / 10; // 20% slower
    }

    // Set the delay between movement animation frames. This delay will be used for all types of movement animations.
    unit.SwitchAnimation( Monster_Info::MOVING );
    Game::setCustomUnitMovementDelay( frameDelay / unit.animation.animationLength() );

    const std::vector<fheroes2::Point> points = getLinePoints( destPos, targetPos, step );
    std::vector<fheroes2::Point>::const_iterator currentPoint = points.begin();

    Bridge * bridge = Arena::GetBridge();

    // Lower the bridge if the unit needs to land on it
    if ( bridge && ( bridge->NeedDown( unit, destIndex ) || ( unit.isWide() && bridge->NeedDown( unit, destTailIndex ) ) ) ) {
        bridge->ActionDown();
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

    // For Battle speeds 9 and 10 we play one sound at a time without any simultaneous playbacks
    // because on these speeds the unit will end its movement before the sound is finished.
    const bool playSoundBySound = Settings::Get().BattleSpeed() > 8;
    int soundStatus = -1;
    const int flySoundId = unit.M82Move();

    unit.SwitchAnimation( Monster_Info::MOVING );
    while ( currentPoint != points.end() ) {
        _movingPos = *currentPoint;

        if ( playSoundBySound ) {
            // Start unit move sound only if it is not already playing.
            if ( soundStatus < 0 || !Mixer::isPlaying( soundStatus ) ) {
                soundStatus = AudioManager::PlaySound( flySoundId );
            }
        }
        else {
            AudioManager::PlaySound( flySoundId );
        }

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

    StringReplace( msg, "%{dst}",
                   std::to_string( ( unit.GetHeadIndex() / Board::widthInCells ) + 1 ) + ", " + std::to_string( ( unit.GetHeadIndex() % Board::widthInCells ) + 1 ) );

    status.setMessage( msg, true );

    assert( _currentUnit == nullptr && _movingUnit == nullptr && _flyingUnit == nullptr );
}

void Battle::Interface::RedrawActionResistSpell( const Unit & target, const bool playSound )
{
    if ( playSound ) {
        AudioManager::PlaySound( M82::RSBRYFZL );
    }
    std::string str( _n( "The %{name} resists the spell!", "The %{name} resist the spell!", target.GetCount() ) );
    StringReplaceWithLowercase( str, "%{name}", target.GetName() );
    setStatus( str, true );
}

void Battle::Interface::redrawActionSpellCastStatus( const Spell & spell, int32_t dst, const std::string & name, const TargetsInfo & targets )
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

        setStatus( msg, true );
    }
}

void Battle::Interface::redrawActionSpellCastPart1( const Spell & spell, int32_t dst, const HeroBase * caster, const TargetsInfo & targets )
{
    // Reset the idle animation delay timer to prevent the target unit from starting the idle animation.
    for ( const TargetInfo & spellTarget : targets ) {
        spellTarget.defender->checkIdleDelay();
    }

    const bool isMassSpell = spell.isApplyWithoutFocusObject();
    bool isCastDown = false;
    OpponentSprite * opponent = nullptr;

    // set spell cast animation
    if ( caster ) {
        const bool isLeftOpponent = ( caster->GetColor() == arena.getAttackingArmyColor() );
        opponent = isLeftOpponent ? _attackingOpponent.get() : _defendingOpponent.get();
        if ( opponent != nullptr ) {
            if ( isMassSpell ) {
                opponent->SetAnimation( OP_CAST_MASS );
            }
            else {
                isCastDown = isHeroCastDown( dst, isLeftOpponent );

                opponent->SetAnimation( isCastDown ? OP_CAST_DOWN : OP_CAST_UP );
            }

            _animateOpponents( opponent );
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
        _redrawActionColdRingSpell( dst, targets );
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
        _redrawActionDeathWaveSpell( 7 );
        break;
    case Spell::DEATHWAVE:
        _redrawActionDeathWaveSpell( 14 );
        break;

    case Spell::HOLYWORD:
        _redrawActionHolyShoutSpell( 16 );
        break;
    case Spell::HOLYSHOUT:
        _redrawActionHolyShoutSpell( 24 );
        break;

    case Spell::ELEMENTALSTORM:
        _redrawActionElementalStormSpell( targets );
        break;
    case Spell::ARMAGEDDON:
        _redrawActionArmageddonSpell(); // hit everything
        break;

    default:
        break;
    }

    // with object
    if ( !targets.empty() ) {
        Unit * target = targets.front().defender;

        if ( spell.isResurrect() )
            _redrawActionResurrectSpell( *target, spell );
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
            case Spell::ARROW:
                _redrawActionArrowSpell( *target );
                break;
            case Spell::BLOODLUST:
                _redrawActionBloodLustSpell( *target );
                break;
            case Spell::CHAINLIGHTNING:
                _redrawActionChainLightningSpell( targets );
                break;
            case Spell::COLDRAY:
                _redrawActionColdRaySpell( *target );
                break;
            case Spell::DISRUPTINGRAY:
                _redrawActionDisruptingRaySpell( *target );
                break;
            case Spell::LIGHTNINGBOLT:
                _redrawActionLightningBoltSpell( *target );
                break;
            case Spell::PETRIFY:
                _redrawActionStoneSpell( *target );
                break;
            case Spell::SUMMONEELEMENT:
            case Spell::SUMMONAELEMENT:
            case Spell::SUMMONFELEMENT:
            case Spell::SUMMONWELEMENT:
                _redrawActionSummonElementalSpell( *target );
                break;
            case Spell::TELEPORT:
                _redrawActionTeleportSpell( *target, dst );
                break;

            default:
                break;
            }
    }

    if ( opponent != nullptr ) {
        if ( isMassSpell ) {
            opponent->SetAnimation( OP_CAST_MASS_RETURN );
        }
        else {
            opponent->SetAnimation( isCastDown ? OP_CAST_DOWN_RETURN : OP_CAST_UP_RETURN );
        }
        _animateOpponents( opponent );

        // Return to the static animation of hero.
        opponent->SetAnimation( OP_STATIC );
    }
}

void Battle::Interface::redrawActionSpellCastPart2( const Spell & spell, const TargetsInfo & targets )
{
    if ( spell.isDamage() ) {
        uint32_t killed = 0;
        uint32_t totalDamage = 0;
        uint32_t maximumDamage = 0;
        uint32_t damagedMonsters = 0;

        for ( const TargetInfo & target : targets ) {
            if ( !target.defender->isModes( CAP_MIRRORIMAGE ) ) {
                killed += target.killed;

                ++damagedMonsters;
                totalDamage += target.damage;
                if ( maximumDamage < target.damage ) {
                    maximumDamage = target.damage;
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
            _redrawActionWincesKills( targets );
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
                    status.setMessage( msg, true );

                    if ( killed > 0 ) {
                        msg = _n( "1 creature perishes.", "%{count} creatures perish.", killed );
                        StringReplace( msg, "%{count}", killed );
                        status.setMessage( msg, true );
                    }
                }
                else {
                    msg = _( "The %{spell} does %{damage} damage to all undead creatures." );
                    StringReplace( msg, "%{spell}", spell.GetName() );
                    StringReplace( msg, "%{damage}", maximumDamage );
                    status.setMessage( msg, true );

                    if ( killed > 0 ) {
                        msg = _( "The %{spell} does %{damage} damage, %{count} creatures perish." );
                        StringReplace( msg, "%{count}", killed );
                    }
                    else {
                        msg = _( "The %{spell} does %{damage} damage." );
                    }

                    StringReplace( msg, "%{spell}", spell.GetName() );
                    StringReplace( msg, "%{damage}", totalDamage );
                    status.setMessage( msg, true );
                }
            }
            else if ( spell.isAliveOnly() ) {
                if ( damagedMonsters == 1 ) {
                    msg = _( "The %{spell} does %{damage} damage to one living creature." );
                    StringReplace( msg, "%{spell}", spell.GetName() );
                    StringReplace( msg, "%{damage}", totalDamage );
                    status.setMessage( msg, true );

                    if ( killed > 0 ) {
                        msg = _n( "1 creature perishes.", "%{count} creatures perish.", killed );
                        StringReplace( msg, "%{count}", killed );
                        status.setMessage( msg, true );
                    }
                }
                else {
                    msg = _( "The %{spell} does %{damage} damage to all living creatures." );
                    StringReplace( msg, "%{spell}", spell.GetName() );
                    StringReplace( msg, "%{damage}", maximumDamage );
                    status.setMessage( msg, true );

                    if ( killed > 0 ) {
                        msg = _( "The %{spell} does %{damage} damage, %{count} creatures perish." );
                        StringReplace( msg, "%{count}", killed );
                    }
                    else {
                        msg = _( "The %{spell} does %{damage} damage." );
                    }

                    StringReplace( msg, "%{spell}", spell.GetName() );
                    StringReplace( msg, "%{damage}", totalDamage );
                    status.setMessage( msg, true );
                }
            }
            else {
                msg = _( "The %{spell} does %{damage} damage." );
                StringReplace( msg, "%{spell}", spell.GetName() );
                StringReplace( msg, "%{damage}", totalDamage );
                status.setMessage( msg, true );

                if ( killed > 0 ) {
                    msg = _n( "1 creature perishes.", "%{count} creatures perish.", killed );
                    StringReplace( msg, "%{count}", killed );
                    status.setMessage( msg, true );
                }
            }
        }
    }

    status.setMessage( " ", false );

    // TODO: remove this temporary assertion
    assert( _movingUnit == nullptr );
}

void Battle::Interface::RedrawActionMonsterSpellCastStatus( const Spell & spell, const Unit & attacker, const TargetInfo & target )
{
    std::string msg;
    const uint32_t attackerCount = attacker.GetCount();

    switch ( spell.GetID() ) {
    case Spell::BLIND:
        msg = _n( "The %{attacker}'s attack blinds the %{target}!", "The %{attacker}' attack blinds the %{target}!", attackerCount );
        break;
    case Spell::PETRIFY:
        msg = _n( "The %{attacker}'s gaze turns the %{target} to stone!", "The %{attacker}' gaze turns the %{target} to stone!", attackerCount );
        break;
    case Spell::CURSE:
        msg = _n( "The %{attacker}'s curse falls upon the %{target}!", "The %{attacker}' curse falls upon the %{target}!", attackerCount );
        break;
    case Spell::PARALYZE:
        msg = _n( "The %{target} is paralyzed by the %{attacker}!", "The %{target} are paralyzed by the %{attacker}!", target.defender->GetCount() );
        break;
    case Spell::DISPEL:
        msg = _n( "The %{attacker} dispels all good spells on your %{target}!", "The %{attacker} dispel all good spells on your %{target}!", attackerCount );
        break;
    default:
        // Did you add a new monster spell casting ability? Add the logic above!
        assert( 0 );
        msg = _n( "The %{attacker} casts %{spell} on the %{target}!", "The %{attacker} cast %{spell} on the %{target}!", attackerCount );
        StringReplace( msg, "%{spell}", spell.GetName() );
        break;
    }

    StringReplaceWithLowercase( msg, "%{attacker}", attacker.GetName() );
    StringReplaceWithLowercase( msg, "%{target}", target.defender->GetName() );

    setStatus( msg, true );
}

void Battle::Interface::RedrawActionLuck( const Unit & unit )
{
    LocalEvent & le = LocalEvent::Get();

    const bool isGoodLuck = unit.Modes( LUCK_GOOD );
    const fheroes2::Rect & pos = unit.GetRectPosition();

    std::string msg = isGoodLuck ? _( "Good luck shines on the %{attacker}." ) : _( "Bad luck descends on the %{attacker}." );
    StringReplaceWithLowercase( msg, "%{attacker}", unit.GetName() );
    setStatus( msg, true );

    // Don't waste time waiting for Luck sound if the game sounds are turned off.
    const bool soundOn = Settings::Get().SoundVolume() > 0;

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );
    if ( isGoodLuck ) {
        // Reset the delay to wait till the next frame if it's not already waiting.
        if ( !Game::isDelayNeeded( { Game::DelayType::BATTLE_MISSILE_DELAY } ) ) {
            Game::AnimateResetDelay( Game::DelayType::BATTLE_MISSILE_DELAY );
        }

        const fheroes2::Rect & battleArea = border.GetArea();
        const fheroes2::Point rainbowDescendPoint( pos.x + pos.width / 2, pos.y - pos.height / 2 );

        // If the creature is low on the battleboard - the rainbow will be from the top (in the original game the threshold is about 140 pixels).
        const bool isVerticalRainbow = ( rainbowDescendPoint.y > 140 );

        // Set the rainbow animation direction to match the army side.
        // Also, if the creature is under effect of the Berserker spell (or similar), then check its original color.
        bool isRainbowFromRight = ( unit.GetCurrentColor() == PlayerColor::UNUSED ) ? ( unit.GetColor() == arena.getDefendingArmyColor() )
                                                                                    : ( unit.GetCurrentColor() == arena.getDefendingArmyColor() );

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

        const int32_t rainbowDrawSteps = 30;
        // Rainbow animation draw step (in original game it is random and about 7-11 pixels).
        // We set the constant animation time for all rainbows: rainbowLength/30 fits the rainbow sound duration on '1' speed.
        const double drawStep = static_cast<double>( rainbowLength ) / rainbowDrawSteps;

        if ( soundOn ) {
            AudioManager::PlaySound( M82::GOODLUCK );
        }

        // If sound is turned off we still wait for the GOODLUCK sound to over but no more the twice the rainbow animation time.
        // So we start counting steps from '-rainbowDrawSteps' to 'rainbowDrawSteps'.
        int32_t step = -rainbowDrawSteps;
        double x = 0;
        while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_MISSILE_DELAY } ) )
                && ( ( ( soundOn || step < rainbowDrawSteps ) && Mixer::isPlaying( -1 ) ) || x < rainbowLength ) ) {
            CheckGlobalEvents( le );

            if ( Game::validateAnimationDelay( Game::BATTLE_MISSILE_DELAY ) ) {
                // Reset all units idle animation delay during rainbow animation not to start new idle animations.
                // This does not affect Zombies, Genies, Medusas and Ghosts as they have permanent idle animations.
                ResetIdleTroopAnimation();

                RedrawPartialStart();

                if ( x < rainbowLength ) {
                    x += drawStep;
                }

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

                ++step;
            }
        }
    }
    else {
        const int32_t offsetX = pos.x + pos.width / 2;
        // Don't let the Bad Luck sprite clip outside the top of the battlefield.
        const int32_t offsetY = std::max( GetAbsoluteICNHeight( ICN::CLOUDLUK ), pos.y + pos.height + cellYOffset );

        const uint32_t frameLimit = 8;
        uint32_t frameId = 0;

        if ( soundOn ) {
            // This sound lasts for about 2200 milliseconds.
            AudioManager::PlaySound( M82::BADLUCK );
        }

        // Make the animation duration consistent with the played sound on the normal battle speed.
        const uint64_t animationDelay = Game::ApplyBattleSpeed( 2200 / frameLimit );
        // Immediately indicate that the delay has passed to render first frame immediately.
        Game::passCustomAnimationDelay( animationDelay );
        // Make sure that the first run is passed immediately.
        assert( !Game::isCustomDelayNeeded( animationDelay ) );

        while ( le.HandleEvents( Game::isCustomDelayNeeded( animationDelay ) ) && ( frameId < frameLimit || ( soundOn && Mixer::isPlaying( -1 ) ) ) ) {
            CheckGlobalEvents( le );

            if ( Game::validateCustomAnimationDelay( animationDelay ) ) {
                RedrawPartialStart();

                if ( frameId < frameLimit ) {
                    const fheroes2::Sprite & luckSprite = fheroes2::AGG::GetICN( ICN::CLOUDLUK, frameId );
                    fheroes2::Blit( luckSprite, _mainSurface, offsetX + luckSprite.x(), offsetY + luckSprite.y() );

                    ++frameId;
                }

                RedrawPartialFinish();
            }
        }
    }
}

void Battle::Interface::RedrawActionMorale( Unit & unit, const bool isGoodMorale )
{
    std::string msg;
    // Reset the idle animation delay timer to prevent the unit from starting the idle animation.
    unit.checkIdleDelay();

    if ( isGoodMorale ) {
        msg = _( "High morale enables the %{monster} to attack again." );
        StringReplaceWithLowercase( msg, "%{monster}", unit.GetName() );
        setStatus( msg, true );
        RedrawTroopWithFrameAnimation( unit, ICN::MORALEG, M82::GOODMRLE, NONE );
    }
    else {
        msg = _( "Low morale causes the %{monster} to freeze in panic." );
        StringReplaceWithLowercase( msg, "%{monster}", unit.GetName() );
        setStatus( msg, true );
        RedrawTroopWithFrameAnimation( unit, ICN::MORALEB, M82::BADMRLE, WINCE );
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
    _redrawActionWincesKills( targets );

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
        setStatus( msg, true );
    }

    // TODO: remove this temporary assertion
    assert( _movingUnit == nullptr );
}

void Battle::Interface::RedrawActionCatapultPart1( const CastleDefenseStructure catapultTarget, const bool isHit )
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
    fheroes2::Point pt2 = Catapult::GetTargetPosition( catapultTarget, isHit );
    const int32_t boulderArcStep = ( pt2.x - pt1.x ) / 30;

    // set the projectile arc height for each castle target and a formula for an unknown target
    int32_t boulderArcHeight;
    switch ( catapultTarget ) {
    case CastleDefenseStructure::WALL1:
        boulderArcHeight = 220;
        break;
    case CastleDefenseStructure::WALL2:
    case CastleDefenseStructure::BRIDGE:
        boulderArcHeight = 216;
        break;
    case CastleDefenseStructure::WALL3:
        boulderArcHeight = 204;
        break;
    case CastleDefenseStructure::WALL4:
        boulderArcHeight = 208;
        break;
    case CastleDefenseStructure::TOWER1:
    case CastleDefenseStructure::TOWER2:
        boulderArcHeight = 206;
        break;
    case CastleDefenseStructure::CENTRAL_TOWER:
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
    const bool isBridgeDestroyed = isHit && ( catapultTarget == CastleDefenseStructure::BRIDGE );
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

void Battle::Interface::RedrawActionCatapultPart2( const CastleDefenseStructure catapultTarget )
{
    // Finish the smoke cloud animation after the building's state has changed after the hit and it is drawn as demolished.

    const fheroes2::Point pt1 = Catapult::GetTargetPosition( catapultTarget, true ) + GetArea().getPosition();
    fheroes2::Point pt2;

    // Continue the smoke cloud animation from the 6th frame.
    const int32_t icnId = ICN::LICHCLOD;
    uint32_t frame = castleBuildingDestroyFrame;
    const uint32_t maxFrame = fheroes2::AGG::GetICNCount( icnId );
    uint32_t maxAnimationFrame = maxFrame;
    const bool isBridgeDestroyed = ( catapultTarget == CastleDefenseStructure::BRIDGE );
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

void Battle::Interface::_redrawActionArrowSpell( const Unit & target )
{
    const HeroBase * caster = arena.GetCurrentCommander();

    if ( caster ) {
        const fheroes2::Point missileStart = caster == _attackingOpponent->GetHero() ? _attackingOpponent->GetCastPosition() : _defendingOpponent->GetCastPosition();

        const fheroes2::Point targetPos = target.GetCenterPoint();
        const double angle = GetAngle( missileStart, targetPos );

        Cursor::Get().SetThemes( Cursor::WAR_POINTER );
        AudioManager::PlaySound( M82::MAGCAROW );

        // Magic arrow == Archer missile
        RedrawMissileAnimation( missileStart, targetPos, angle, Monster::ARCHER );
    }
}

void Battle::Interface::_redrawActionTeleportSpell( Unit & target, const int32_t dst )
{
    LocalEvent & le = LocalEvent::Get();

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    // Hide the counter.
    target.SwitchAnimation( Monster_Info::STAND_STILL );

    const fheroes2::Sprite & unitSprite = fheroes2::AGG::GetICN( target.GetMonsterSprite(), target.GetFrame() );
    fheroes2::Sprite rippleSprite;
    _spriteInsteadCurrentUnit = &rippleSprite;
    const int32_t spriteHeight = unitSprite.height();

    const Unit * oldCurrent = _currentUnit;
    _currentUnit = &target;

    // Don't highlight the cursor position.
    _currentCellIndex = -1;

    // Don't highlight movement area cells while animating teleportation.
    Settings & conf = Settings::Get();
    const bool showMoveShadowState = conf.BattleShowMoveShadow();
    if ( showMoveShadowState ) {
        conf.SetBattleMovementShaded( false );
    }

    const bool soundOn = conf.SoundVolume() > 0;

    if ( soundOn ) {
        AudioManager::PlaySound( M82::TELPTOUT );
    }

    int32_t frame = 1;
    const int32_t frameLimit = 25;
    const int32_t maxAmplitude = 3;
    const int32_t imageCutFrames = 7;
    const double phaseCoeff = 0.6;
    const int32_t wavePeriod = 60;

    Game::passAnimationDelay( Game::BATTLE_DISRUPTING_DELAY );

    // Animate first teleportation phase: disappearing.
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_DISRUPTING_DELAY } ) ) && ( frame <= frameLimit || ( soundOn && Mixer::isPlaying( -1 ) ) ) ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
            if ( frame >= frameLimit ) {
                // Render battlefield without this unit.
                rippleSprite.clear();
            }
            else {
                const int32_t amplitude = std::min( frame / 2, maxAmplitude );
                rippleSprite = fheroes2::createRippleEffect( unitSprite, amplitude, -phaseCoeff * frame, wavePeriod );

                if ( frame > frameLimit - imageCutFrames ) {
                    // Animate disappearing by making sprite transparent starting from the top.
                    fheroes2::FillTransform( rippleSprite, 0, 0, rippleSprite.width(), spriteHeight * ( frame - frameLimit + imageCutFrames ) / imageCutFrames, 1 );
                }
            }

            ++frame;

            Redraw();
        }
    }

    target.SetPosition( dst );
    if ( soundOn ) {
        AudioManager::PlaySound( M82::TELPTIN );
    }

    frame = 1;
    // Animate second teleportation phase: appearing.
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_DISRUPTING_DELAY } ) ) && ( frame < frameLimit || ( soundOn && Mixer::isPlaying( -1 ) ) ) ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
            if ( frame == frameLimit ) {
                // Render battlefield with this unit.
                _spriteInsteadCurrentUnit = &unitSprite;
            }
            else {
                const int32_t amplitude = std::min( ( frameLimit - frame ) / 2, maxAmplitude );
                rippleSprite = fheroes2::createRippleEffect( unitSprite, amplitude, phaseCoeff * ( frame + frameLimit ), wavePeriod );

                if ( frame < imageCutFrames ) {
                    // Animate appearing from bottom to top by making the top part transparent.
                    fheroes2::FillTransform( rippleSprite, 0, 0, rippleSprite.width(), spriteHeight * ( imageCutFrames - frame ) / imageCutFrames, 1 );
                }

                ++frame;
            }

            Redraw();
        }
    }

    // Restore the initial state.
    target.SwitchAnimation( Monster_Info::STATIC );
    _currentUnit = oldCurrent;
    _spriteInsteadCurrentUnit = nullptr;
    if ( showMoveShadowState ) {
        conf.SetBattleMovementShaded( true );
    }
}

void Battle::Interface::_redrawActionSummonElementalSpell( Unit & target )
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

void Battle::Interface::redrawActionMirrorImageSpell( const HeroBase & caster, const int32_t targetCell, const Unit & originalUnit, Unit & mirrorUnit )
{
    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    // Temporary set mirror image as the current unit to animate the spell effect.
    const Unit * oldCurrent = _currentUnit;
    mirrorUnit.SetCustomAlpha( 0 );
    // Hide the counter.
    mirrorUnit.SwitchAnimation( Monster_Info::STAND_STILL );
    _currentUnit = &mirrorUnit;

    // Set custom sprite for mirrored unit to hide its contour and to have ability to change sprite position.
    fheroes2::Sprite mirrorUnitSprite = fheroes2::AGG::GetICN( originalUnit.GetMonsterSprite(), originalUnit.GetFrame() );
    _spriteInsteadCurrentUnit = &mirrorUnitSprite;

    // Don't highlight the cursor position.
    _currentCellIndex = -1;

    // Don't highlight movement area cells while animating spell.
    Settings & conf = Settings::Get();
    const bool showMoveShadowState = conf.BattleShowMoveShadow();
    if ( showMoveShadowState ) {
        conf.SetBattleMovementShaded( false );
    }

    // Animate casting.
    const bool isLeftOpponent = ( caster.GetColor() == arena.getAttackingArmyColor() );
    const bool isCastDown = isHeroCastDown( targetCell, isLeftOpponent );

    OpponentSprite * opponent = isLeftOpponent ? _attackingOpponent.get() : _defendingOpponent.get();
    assert( opponent != nullptr );

    opponent->SetAnimation( isCastDown ? OP_CAST_DOWN : OP_CAST_UP );
    _animateOpponents( opponent );

    const fheroes2::Point & unitPos = originalUnit.GetRectPosition().getPosition();
    const fheroes2::Point & mirrorPos = mirrorUnit.GetRectPosition().getPosition();
    const fheroes2::Point spriteOffet( mirrorUnitSprite.x(), mirrorUnitSprite.y() );

    // Get spell animation points.
    const std::vector<fheroes2::Point> points = getLinePoints( unitPos - mirrorPos + spriteOffet, spriteOffet, 5 );
    std::vector<fheroes2::Point>::const_iterator pnt = points.cbegin();

    // Make the mirror unit visible.
    mirrorUnit.SetCustomAlpha( 255 );

    AudioManager::PlaySound( M82::MIRRORIM );

    LocalEvent & le = LocalEvent::Get();

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && pnt != points.cend() ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            mirrorUnitSprite.setPosition( pnt->x, pnt->y );
            RedrawPartialStart();
            RedrawPartialFinish();

            ++pnt;
        }
    }

    // Run caster's "cast return" animation.
    opponent->SetAnimation( isCastDown ? OP_CAST_DOWN_RETURN : OP_CAST_UP_RETURN );
    _animateOpponents( opponent );
    opponent->SetAnimation( OP_STATIC );

    // Restore the initial state.
    mirrorUnit.SwitchAnimation( Monster_Info::STATIC );
    _currentUnit = oldCurrent;
    _spriteInsteadCurrentUnit = nullptr;
    if ( showMoveShadowState ) {
        conf.SetBattleMovementShaded( true );
    }

    setStatus( _( "The mirror image is created." ), true );
}

void Battle::Interface::_redrawLightningOnTargets( const std::vector<fheroes2::Point> & points, const fheroes2::Rect & drawRoi )
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
            const int32_t spriteWidth = sprite.width();

            for ( size_t i = 1; i < points.size(); ++i ) {
                const fheroes2::Point pt = points[i] - fheroes2::Point( spriteWidth / 2, 0 ) + roiOffset;
                fheroes2::Blit( sprite, _mainSurface, pt.x, pt.y );
            }
            RedrawPartialFinish();

            ++frame;
        }
    }
}

void Battle::Interface::_redrawActionLightningBoltSpell( const Unit & target )
{
    _currentUnit = nullptr;

    const fheroes2::Point startingPos
        = arena.GetCurrentCommander() == _attackingOpponent->GetHero() ? _attackingOpponent->GetCastPosition() : _defendingOpponent->GetCastPosition();
    const fheroes2::Rect & pos = target.GetRectPosition();
    const fheroes2::Point endPos( pos.x + pos.width / 2, pos.y );

    const std::vector<fheroes2::Point> points{ startingPos, endPos };

    _redrawLightningOnTargets( points, _surfaceInnerArea );
}

void Battle::Interface::_redrawActionChainLightningSpell( const TargetsInfo & targets )
{
    const fheroes2::Point startingPos
        = arena.GetCurrentCommander() == _attackingOpponent->GetHero() ? _attackingOpponent->GetCastPosition() : _defendingOpponent->GetCastPosition();
    std::vector<fheroes2::Point> points;
    points.reserve( size( targets ) + 1 );
    points.push_back( startingPos );

    for ( const TargetInfo & target : targets ) {
        const fheroes2::Rect & pos = target.defender->GetRectPosition();
        points.emplace_back( pos.x + pos.width / 2, pos.y );
    }

    _redrawLightningOnTargets( points, _surfaceInnerArea );
}

void Battle::Interface::_redrawActionBloodLustSpell( const Unit & target )
{
    LocalEvent & le = LocalEvent::Get();

    const fheroes2::Sprite & unitSprite = fheroes2::AGG::GetICN( target.GetMonsterSprite(), target.GetFrame() );

    fheroes2::Sprite bloodlustEffect( unitSprite );
    fheroes2::ApplyPalette( bloodlustEffect, PAL::GetPalette( PAL::PaletteType::RED ) );

    fheroes2::Sprite mixSprite;

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    _currentUnit = &target;
    _spriteInsteadCurrentUnit = &mixSprite;

    const uint32_t bloodlustDelay = 1800 / 20;
    // duration is 1900ms
    AudioManager::PlaySound( M82::BLOODLUS );

    uint8_t alpha = 0;
    uint32_t frame = 0;

    // Immediately indicate that the delay has passed to render first frame immediately.
    Game::passCustomAnimationDelay( bloodlustDelay );
    // Make sure that the first run is passed immediately.
    assert( !Game::isCustomDelayNeeded( bloodlustDelay ) );

    while ( le.HandleEvents( Game::isCustomDelayNeeded( bloodlustDelay ) ) && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( frame < 20 && Game::validateCustomAnimationDelay( bloodlustDelay ) ) {
            mixSprite = unitSprite;
            fheroes2::AlphaBlit( bloodlustEffect, mixSprite, alpha );
            Redraw();

            alpha += ( frame < 10 ) ? 20 : -20;
            ++frame;
        }
    }

    _currentUnit = nullptr;
    _spriteInsteadCurrentUnit = nullptr;
}

void Battle::Interface::_redrawActionStoneSpell( const Unit & target )
{
    LocalEvent & le = LocalEvent::Get();

    const fheroes2::Sprite & unitSprite = fheroes2::AGG::GetICN( target.GetMonsterSprite(), target.GetFrame() );

    fheroes2::Sprite stoneEffect( unitSprite );
    fheroes2::ApplyPalette( stoneEffect, PAL::GetPalette( PAL::PaletteType::GRAY ) );

    fheroes2::Sprite mixSprite;

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    // Don't highlight the cursor position.
    _currentCellIndex = -1;

    // Don't highlight movement area cells while animating teleportation.
    Settings & conf = Settings::Get();
    const bool showMoveShadowState = conf.BattleShowMoveShadow();
    if ( showMoveShadowState ) {
        conf.SetBattleMovementShaded( false );
    }

    _currentUnit = &target;
    _spriteInsteadCurrentUnit = &mixSprite;

    AudioManager::PlaySound( M82::PARALIZE );

    uint8_t alpha = 0;
    uint32_t frame = 0;
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( frame < 25 && Game::validateCustomAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            mixSprite = unitSprite;
            fheroes2::AlphaBlit( stoneEffect, mixSprite, alpha );
            Redraw();

            alpha += 10;
            ++frame;
        }
    }

    _currentUnit = nullptr;
    _spriteInsteadCurrentUnit = nullptr;
    if ( showMoveShadowState ) {
        conf.SetBattleMovementShaded( true );
    }
}

void Battle::Interface::_redrawActionResurrectSpell( Unit & target, const Spell & spell )
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

void Battle::Interface::_redrawActionColdRaySpell( Unit & target )
{
    _redrawRaySpell( target, ICN::COLDRAY, M82::COLDRAY, 18 );
    RedrawTroopWithFrameAnimation( target, ICN::ICECLOUD, M82::UNKNOWN, NONE );
}

void Battle::Interface::_redrawRaySpell( const Unit & target, const int spellICN, const int spellSound, const int32_t size )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    // Casting hero position
    const fheroes2::Point startingPos
        = arena.GetCurrentCommander() == _attackingOpponent->GetHero() ? _attackingOpponent->GetCastPosition() : _defendingOpponent->GetCastPosition();
    const fheroes2::Point targetPos = target.GetCenterPoint();

    const std::vector<fheroes2::Point> path = getLinePoints( startingPos, targetPos, size );
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

void Battle::Interface::_redrawActionDisruptingRaySpell( Unit & target )
{
    LocalEvent & le = LocalEvent::Get();

    _redrawRaySpell( target, ICN::DISRRAY, M82::DISRUPTR, 24 );

    // Hide the counter.
    target.SwitchAnimation( Monster_Info::STAND_STILL );

    // Part 2 - ripple effect
    const fheroes2::Sprite & unitSprite = fheroes2::AGG::GetICN( target.GetMonsterSprite(), target.GetFrame() );
    fheroes2::Sprite rippleSprite;
    _spriteInsteadCurrentUnit = &rippleSprite;

    const Unit * oldCurrent = _currentUnit;
    _currentUnit = &target;

    // Don't highlight the cursor position.
    _currentCellIndex = -1;

    // Don't highlight movement area cells while animating teleportation.
    Settings & conf = Settings::Get();
    const bool showMoveShadowState = conf.BattleShowMoveShadow();
    if ( showMoveShadowState ) {
        conf.SetBattleMovementShaded( false );
    }

    int32_t frame = 1;
    const int32_t frameLimit = 60;
    const int32_t maxAmplitude = 3;

    Game::passAnimationDelay( Game::BATTLE_DISRUPTING_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_DISRUPTING_DELAY } ) ) && frame < frameLimit ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
            const int32_t amplitude = std::min( std::min( frame, ( frameLimit - frame ) ) / 3, maxAmplitude );
            rippleSprite = fheroes2::createRippleEffect( unitSprite, amplitude, -0.4 * frame, 60 );

            Redraw();

            ++frame;
        }
    }

    // Restore the initial state.
    target.SwitchAnimation( Monster_Info::STATIC );
    _currentUnit = oldCurrent;
    _spriteInsteadCurrentUnit = nullptr;
    if ( showMoveShadowState ) {
        conf.SetBattleMovementShaded( true );
    }
}

void Battle::Interface::_redrawActionDeathWaveSpell( const int32_t strength )
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

    fheroes2::Image battleFieldCopy;
    battleFieldCopy._disableTransformLayer();
    battleFieldCopy.resize( area.width, area.height );
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

    // Prepare the blank image for the Death Wave spell effect.
    fheroes2::Image spellEffect;
    spellEffect._disableTransformLayer();
    spellEffect.resize( waveLength, area.height );

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
            fheroes2::Copy( battleFieldCopy, restorePositionX, area.y, display, renderArea.x, renderArea.y, restoreWidth, renderArea.height );

            // Place the Death Wave effect to its new position.
            fheroes2::CreateDeathWaveEffect( spellEffect, battleFieldCopy, position, deathWaveCurve );
            fheroes2::Copy( spellEffect, 0, 0, display, renderArea.x + restoreWidth, renderArea.y, waveWidth, area.height );

            // Render only the changed screen area.
            display.render( renderArea );

            position += waveStep;
        }
    }

    SwitchAllUnitsAnimation( Monster_Info::STATIC );
}

void Battle::Interface::_redrawActionColdRingSpell( const int32_t dst, const TargetsInfo & targets )
{
    LocalEvent & le = LocalEvent::Get();

    const int icn = ICN::COLDRING;
    const int m82 = M82::FromSpell( Spell::COLDRING );
    uint32_t frame = 0;
    const fheroes2::Rect & center = Board::GetCell( dst )->GetPos();

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    // set WNCE
    _currentUnit = nullptr;
    for ( const TargetInfo & target : targets ) {
        if ( target.defender && target.damage ) {
            target.defender->SwitchAnimation( Monster_Info::WNCE );
        }
    }

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

            for ( const TargetInfo & target : targets ) {
                if ( target.defender && target.damage ) {
                    target.defender->IncreaseAnimFrame( false );
                }
            }
            ++frame;
        }
    }

    for ( const TargetInfo & target : targets ) {
        if ( target.defender ) {
            target.defender->SwitchAnimation( Monster_Info::STATIC );
            _currentUnit = nullptr;
        }
    }
}

void Battle::Interface::_redrawActionHolyShoutSpell( const uint8_t strength )
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

    fheroes2::Image battleFieldCopy;
    battleFieldCopy._disableTransformLayer();
    battleFieldCopy.resize( area.width, area.height );
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

    const uint32_t spellcastDelay = Game::ApplyBattleSpeed( 3000 / maxFrame );
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

void Battle::Interface::_redrawActionElementalStormSpell( const TargetsInfo & targets )
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
    for ( const TargetInfo & target : targets ) {
        if ( target.defender && target.damage ) {
            target.defender->SwitchAnimation( Monster_Info::WNCE );
        }
    }

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

            for ( const TargetInfo & target : targets ) {
                if ( target.defender && target.damage ) {
                    target.defender->IncreaseAnimFrame( false );
                }
            }
            ++frame;
        }
    }

    for ( const TargetInfo & target : targets ) {
        if ( target.defender ) {
            target.defender->SwitchAnimation( Monster_Info::STATIC );
            _currentUnit = nullptr;
        }
    }
}

void Battle::Interface::_redrawActionArmageddonSpell()
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    fheroes2::Rect area = GetArea();

    area.height -= status.height;

    // Hide Turn Order for the Armageddon animation original image not to shake it together with the land.
    Settings & settings = Settings::Get();
    const bool isTurnOrderShown = settings.BattleShowTurnOrder();
    if ( isTurnOrderShown ) {
        settings.setBattleShowTurnOrder( false );
    }

    // Set all non-dead troops animation to static and redraw the '_mainSurface'.
    SwitchAllUnitsAnimation( Monster_Info::STATIC );
    RedrawPartialStart();

    // Restore the Turn Order rendering if it was enabled.
    if ( isTurnOrderShown ) {
        settings.setBattleShowTurnOrder( true );
    }

    fheroes2::Image spriteWhitening;
    spriteWhitening._disableTransformLayer();
    spriteWhitening.resize( area.width, area.height );

    fheroes2::Copy( _mainSurface, area.x, area.y, spriteWhitening, 0, 0, area.width, area.height );
    fheroes2::Image spriteReddish = spriteWhitening;
    fheroes2::ApplyPalette( spriteReddish, PAL::GetPalette( PAL::PaletteType::RED ) );

    cursor.SetThemes( Cursor::WAR_POINTER );

    _currentUnit = nullptr;
    AudioManager::PlaySound( M82::ARMGEDN );
    uint32_t alpha = 10;

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && alpha < 100 ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            fheroes2::ApplyPalette( spriteWhitening, 9 );
            fheroes2::Copy( spriteWhitening, 0, 0, _mainSurface, area.x, area.y, area.width, area.height );
            RedrawPartialFinish();

            alpha += 10;
        }
    }

    fheroes2::Copy( spriteReddish, 0, 0, _mainSurface, area.x, area.y, area.width, area.height );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            const int32_t offsetX = static_cast<int32_t>( Rand::Get( 0, 28 ) ) - 14;
            const int32_t offsetY = static_cast<int32_t>( Rand::Get( 0, 22 ) ) - 11;

            fheroes2::Copy( spriteReddish, std::max( 0, -offsetX ), std::max( 0, -offsetY ), _mainSurface, std::max( 0, offsetX ), std::max( 0, offsetY ),
                            area.width - std::abs( offsetX ), area.height - std::abs( offsetY ) );

            RedrawPartialFinish();
        }
    }
}

void Battle::Interface::redrawActionEarthquakeSpellPart1( const HeroBase & caster, const std::vector<CastleDefenseStructure> & targets )
{
    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    // Animate casting.
    const bool isLeftOpponent = ( caster.GetColor() == arena.getAttackingArmyColor() );
    OpponentSprite * opponent = isLeftOpponent ? _attackingOpponent.get() : _defendingOpponent.get();
    assert( opponent != nullptr );

    opponent->SetAnimation( OP_CAST_MASS );
    _animateOpponents( opponent );

    fheroes2::Rect area = GetArea();
    area.height -= status.height;

    // Hide Turn Order for the Earthquake animation original image not to shake it together with the land.
    Settings & settings = Settings::Get();
    const bool isTurnOrderShown = settings.BattleShowTurnOrder();
    if ( isTurnOrderShown ) {
        settings.setBattleShowTurnOrder( false );
    }

    // Set all non-dead troops animation to static and redraw the '_mainSurface'.
    SwitchAllUnitsAnimation( Monster_Info::STATIC );
    RedrawPartialStart();

    // Restore the Turn Order rendering if it was enabled.
    if ( isTurnOrderShown ) {
        settings.setBattleShowTurnOrder( true );
    }

    fheroes2::Image battlefieldImage;
    battlefieldImage._disableTransformLayer();
    battlefieldImage.resize( area.width, area.height );
    fheroes2::Copy( _mainSurface, area.x, area.y, battlefieldImage, area );

    _currentUnit = nullptr;
    AudioManager::PlaySound( M82::ERTHQUAK );

    LocalEvent & le = LocalEvent::Get();
    uint32_t frame = 0;

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    // Draw earth quake animation.
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY } ) ) && frame < 18 ) {
        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY ) ) {
            const int32_t offsetX = static_cast<int32_t>( Rand::Get( 0, 28 ) ) - 14;
            const int32_t offsetY = static_cast<int32_t>( Rand::Get( 0, 22 ) ) - 11;

            fheroes2::Copy( battlefieldImage, std::max( 0, -offsetX ), std::max( 0, -offsetY ), _mainSurface, std::max( 0, offsetX ), std::max( 0, offsetY ),
                            area.width - std::abs( offsetX ), area.height - std::abs( offsetY ) );

            RedrawPartialFinish();

            ++frame;
        }
    }

    // Set end of casting animation. The animation will be done by `CheckGlobalEvents()`.
    opponent->SetAnimation( OP_CAST_MASS_RETURN );

    // Draw buildings demolition clouds.
    const int icn = ICN::LICHCLOD;
    const uint32_t maxFrame = fheroes2::AGG::GetICNCount( icn ) / 2;
    frame = 0;

    AudioManager::PlaySound( M82::CATSND02 );

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    // We need to wait this delay before rendering the first frame of hero animation.
    Game::AnimateResetDelay( Game::DelayType::BATTLE_OPPONENTS_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY, Game::BATTLE_OPPONENTS_DELAY } ) ) && frame < maxFrame ) {
        CheckGlobalEvents( le );

        const bool isNeedSpellAnimationUpdate = Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY );

        if ( isNeedSpellAnimationUpdate || Game::validateAnimationDelay( Game::BATTLE_OPPONENTS_DELAY ) ) {
            RedrawPartialStart();

            for ( const CastleDefenseStructure target : targets ) {
                const fheroes2::Point pt2 = Catapult::GetTargetPosition( target, true );

                if ( target == CastleDefenseStructure::BRIDGE && frame >= bridgeDestroySmokeDelay ) {
                    // When a bridge is demolished, there is one additional smoke explosion at the point where the bridge falls.
                    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, frame - bridgeDestroySmokeDelay );
                    fheroes2::Blit( sprite, _mainSurface, pt2.x + bridgeDestroySmokeOffset.x + sprite.x(), pt2.y + bridgeDestroySmokeOffset.y + sprite.y() );
                }

                const fheroes2::Sprite & spriteCloud = fheroes2::AGG::GetICN( icn, frame );
                fheroes2::Blit( spriteCloud, _mainSurface, pt2.x + spriteCloud.x(), pt2.y + spriteCloud.y() );
            }

            RedrawPartialFinish();

            if ( isNeedSpellAnimationUpdate ) {
                ++frame;
            }
        }
    }
}

void Battle::Interface::redrawActionEarthquakeSpellPart2( const std::vector<CastleDefenseStructure> & targets )
{
    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    LocalEvent & le = LocalEvent::Get();
    const int icn = ICN::LICHCLOD;
    uint32_t maxFrame = fheroes2::AGG::GetICNCount( icn );
    uint32_t frame = maxFrame / 2;

    const bool isBridgeDestroyed
        = std::any_of( targets.begin(), targets.end(), []( const CastleDefenseStructure target ) { return target == CastleDefenseStructure::BRIDGE; } );
    if ( isBridgeDestroyed ) {
        maxFrame += bridgeDestroySmokeDelay - 1;
    }

    Game::passAnimationDelay( Game::BATTLE_SPELL_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::BATTLE_SPELL_DELAY, Game::BATTLE_OPPONENTS_DELAY } ) ) && frame < maxFrame ) {
        CheckGlobalEvents( le );

        const bool isNeedSpellAnimationUpdate = Game::validateAnimationDelay( Game::BATTLE_SPELL_DELAY );

        if ( isNeedSpellAnimationUpdate || Game::validateAnimationDelay( Game::BATTLE_OPPONENTS_DELAY ) ) {
            RedrawPartialStart();

            for ( const CastleDefenseStructure target : targets ) {
                const fheroes2::Point pt2 = Catapult::GetTargetPosition( target, true );

                if ( target == CastleDefenseStructure::BRIDGE ) {
                    // When a bridge is demolished, there is one additional smoke explosion at the point where the bridge falls.
                    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, frame - bridgeDestroySmokeDelay + 1 );
                    fheroes2::Blit( sprite, _mainSurface, pt2.x + bridgeDestroySmokeOffset.x + sprite.x(), pt2.y + bridgeDestroySmokeOffset.y + sprite.y() );
                }

                const fheroes2::Sprite & spriteCloud = fheroes2::AGG::GetICN( icn, frame );
                fheroes2::Blit( spriteCloud, _mainSurface, pt2.x + spriteCloud.x(), pt2.y + spriteCloud.y() );
            }

            RedrawPartialFinish();

            if ( isNeedSpellAnimationUpdate ) {
                ++frame;
            }
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
    setStatus( _( "The mirror image is destroyed!" ), true );
}

void Battle::Interface::RedrawTargetsWithFrameAnimation( const int32_t dst, const TargetsInfo & targets, const int icn, const int m82, int repeatCount /* = 0 */ )
{
    LocalEvent & le = LocalEvent::Get();

    uint32_t frame = 0;
    const fheroes2::Rect & center = Board::GetCell( dst )->GetPos();

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    _currentUnit = nullptr;
    for ( const TargetInfo & target : targets ) {
        if ( target.defender && target.damage ) {
            target.defender->SwitchAnimation( Monster_Info::WNCE );
        }
    }

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

            for ( const TargetInfo & target : targets ) {
                if ( target.defender && target.damage ) {
                    target.defender->IncreaseAnimFrame( false );
                }
            }
            ++frame;

            if ( frame == frameCount && repeatCount > 0 ) {
                --repeatCount;
                frame = 0;
            }
        }
    }

    for ( const TargetInfo & target : targets ) {
        if ( target.defender ) {
            target.defender->SwitchAnimation( Monster_Info::STATIC );
            _currentUnit = nullptr;
        }
    }
}

void Battle::Interface::RedrawTargetsWithFrameAnimation( const TargetsInfo & targets, const int icn, const int m82, const bool wnce )
{
    LocalEvent & le = LocalEvent::Get();

    // A vector for creatures made by Mirror Image spell which will be destroyed by current spell.
    std::vector<Unit *> mirrorImages;

    Cursor::Get().SetThemes( Cursor::WAR_POINTER );

    _currentUnit = nullptr;

    if ( wnce ) {
        PlayerColor deathColor = PlayerColor::UNUSED;

        std::set<int> unitSounds;

        const auto playSoundIfNotPlaying = [&unitSounds]( const int unitSound ) {
            const auto [dummy, isUnique] = unitSounds.insert( unitSound );
            if ( isUnique ) {
                AudioManager::PlaySound( unitSound );
            }
        };

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
                playSoundIfNotPlaying( defender->M82Kill() );

                // Set the color of the dead creature to tell heroes about it.
                deathColor = defender->GetArmyColor();
            }
            // If the creature is damaged but is still alive set its wince animation.
            else if ( target.damage ) {
                defender->SwitchAnimation( Monster_Info::WNCE_UP );
                playSoundIfNotPlaying( defender->M82Wnce() );
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

void Battle::Interface::RedrawTroopWithFrameAnimation( Unit & unit, const int icn, const int m82, const CreatureSpellAnimation animation )
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
            if ( _bridgeAnimation.currentFrameId < BridgeMovementAnimation::DOWN_POSITION ) {
                break;
            }
        }
        else {
            if ( _bridgeAnimation.currentFrameId > BridgeMovementAnimation::UP_POSITION ) {
                break;
            }
        }

        CheckGlobalEvents( le );

        if ( Game::validateAnimationDelay( Game::BATTLE_BRIDGE_DELAY ) ) {
            Redraw();

            if ( bridgeDownAnimation ) {
                --_bridgeAnimation.currentFrameId;
            }
            else {
                ++_bridgeAnimation.currentFrameId;
            }
        }
    }

    _bridgeAnimation.animationIsRequired = false;

    if ( !bridgeDownAnimation ) {
        AudioManager::PlaySound( M82::DRAWBRG );
    }
}

bool Battle::Interface::IdleTroopsAnimation() const
{
    if ( Game::validateAnimationDelay( Game::BATTLE_IDLE_DELAY ) ) {
        const bool redrawNeeded = arena.getAttackingForce().animateIdleUnits();
        return arena.getDefendingForce().animateIdleUnits() || redrawNeeded;
    }

    return false;
}

void Battle::Interface::ResetIdleTroopAnimation() const
{
    arena.getAttackingForce().resetIdleAnimation();
    arena.getDefendingForce().resetIdleAnimation();
}

void Battle::Interface::SwitchAllUnitsAnimation( const int32_t animationState ) const
{
    for ( Battle::Unit * unit : arena.getAttackingForce() ) {
        if ( unit->isValid() ) {
            unit->SwitchAnimation( animationState );
        }
    }
    for ( Battle::Unit * unit : arena.getDefendingForce() ) {
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

    // Animation of flags and heroes idle.
    if ( Game::validateAnimationDelay( Game::BATTLE_FLAGS_DELAY ) ) {
        ++animation_flags_frame;
        humanturn_redraw = true;

        // Perform heroes idle animation only if heroes are not performing any other animation (e.g. spell casting).
        if ( Game::hasEveryDelayPassed( { Game::BATTLE_OPPONENTS_DELAY } ) ) {
            if ( _attackingOpponent && _attackingOpponent->updateAnimationState() ) {
                humanturn_redraw = true;
            }

            if ( _defendingOpponent && _defendingOpponent->updateAnimationState() ) {
                humanturn_redraw = true;
            }
        }
    }

    // Check if auto combat interruption was requested.
    InterruptAutoCombatIfRequested( le );
}

void Battle::Interface::InterruptAutoCombatIfRequested( LocalEvent & le )
{
    // Interrupt only if automation is currently on.
    if ( !arena.AutoCombatInProgress() && !arena.EnemyOfAIHasAutoCombatInProgress() ) {
        return;
    }

    if ( !le.MouseClickLeft() && !le.MouseClickRight() && !Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_TOGGLE_AUTO_COMBAT )
         && !Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
        return;
    }

    // Identify which color requested the auto combat interruption.
    PlayerColor color = arena.GetCurrentColor();
    if ( arena.GetCurrentForce().GetControl() & CONTROL_AI ) {
        color = arena.GetOppositeColor( color );
    }

    // The battle interruption is already scheduled, no need for the dialog.
    if ( color == _interruptAutoCombatForColor ) {
        return;
    }

    // Right now there should be no pending auto combat interruptions.
    assert( _interruptAutoCombatForColor == PlayerColor::NONE );

    const int interrupt = fheroes2::showStandardTextMessage( {}, _( "Are you sure you want to interrupt the auto combat mode?" ), Dialog::YES | Dialog::NO );
    if ( interrupt == Dialog::YES ) {
        _interruptAutoCombatForColor = color;
    }
}

void Battle::Interface::ProcessingHeroDialogResult( const int result, Actions & actions )
{
    switch ( result ) {
    // cast
    case 1: {
        const HeroBase * hero = _currentUnit->GetCurrentOrArmyCommander();

        if ( hero ) {
            if ( hero->HaveSpellBook() ) {
                std::string msg;

                if ( arena.isDisableCastSpell( Spell::NONE, &msg ) ) {
                    fheroes2::showStandardTextMessage( "", msg, Dialog::OK );
                }
                else {
                    const std::function<void( const std::string & )> statusCallback = [this]( const std::string & statusStr ) {
                        setStatus( statusStr, false );
                        status.redraw( fheroes2::Display::instance() );
                    };

                    const Spell spell = hero->OpenSpellBook( SpellBook::Filter::CMBT, true, true, statusCallback );

                    // Reset battlefield grid cursor position after closing the spell book.
                    _currentCellIndex = -1;

                    if ( spell.isValid() ) {
                        assert( spell.isCombat() );

                        if ( arena.isDisableCastSpell( spell, &msg ) ) {
                            fheroes2::showStandardTextMessage( spell.GetName(), msg, Dialog::OK );
                        }
                        else {
                            std::string error;

                            if ( hero->CanCastSpell( spell, &error ) ) {
                                if ( spell.isApplyWithoutFocusObject() ) {
                                    actions.emplace_back( Command::SPELLCAST, spell.GetID(), -1 );

                                    humanturn_redraw = true;
                                    humanturn_exit = true;
                                }
                                else {
                                    humanturn_spell = spell;
                                }
                            }
                            else {
                                fheroes2::showStandardTextMessage( _( "Error" ), error, Dialog::OK );
                            }
                        }
                    }
                }
            }
            else
                fheroes2::showStandardTextMessage( "", _( "No spells to cast." ), Dialog::OK );
        }
        break;
    }

    // retreat
    case 2: {
        if ( arena.CanRetreatOpponent( _currentUnit->GetCurrentOrArmyColor() ) ) {
            if ( Dialog::YES == fheroes2::showStandardTextMessage( "", _( "Are you sure you want to retreat?" ), Dialog::YES | Dialog::NO ) ) {
                actions.emplace_back( Command::RETREAT );

                humanturn_exit = true;
            }
        }
        else {
            fheroes2::showStandardTextMessage( "", _( "Retreat disabled" ), Dialog::OK );
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
                    actions.emplace_back( Command::SURRENDER );

                    humanturn_exit = true;
                }
            }
        }
        else {
            fheroes2::showStandardTextMessage( "", _( "Surrender disabled" ), Dialog::OK );
        }
        break;
    }

    default:
        break;
    }
}

bool Battle::PopupDamageInfo::_setDamageInfoBase( const Unit * defender )
{
    if ( defender == nullptr || ( defender == _defender ) ) {
        return false;
    }

    if ( !Settings::Get().isBattleShowDamageInfoEnabled() ) {
        return false;
    }

    // When pop-up is hidden: _cell is nullptr. Before showing pop-up
    if ( _needDelay ) {
        Game::AnimateResetDelay( Game::BATTLE_POPUP_DELAY );
        _needDelay = false;

        return false;
    }

    if ( !Game::hasEveryDelayPassed( { Game::BATTLE_POPUP_DELAY } ) ) {
        return false;
    }

    _defender = defender;

    return true;
}

void Battle::PopupDamageInfo::setAttackInfo( const Unit * attacker, const Unit * defender )
{
    if ( attacker == nullptr || !_setDamageInfoBase( defender ) ) {
        return;
    }

    if ( attacker->Modes( SP_BLESS ) ) {
        _maxDamage = attacker->CalculateMaxDamage( *defender );
        _minDamage = _maxDamage;
    }
    else if ( attacker->Modes( SP_CURSE ) ) {
        _minDamage = attacker->CalculateMinDamage( *defender );
        _maxDamage = _minDamage;
    }
    else {
        _minDamage = attacker->CalculateMinDamage( *defender );
        _maxDamage = attacker->CalculateMaxDamage( *defender );
    }

    _makeDamageImage();

    _redraw = true;
}

void Battle::PopupDamageInfo::setSpellAttackInfo( const HeroBase * hero, const Unit * defender, const Spell & spell )
{
    assert( hero != nullptr );

    // TODO: Currently, this functionality only supports a simple single-target spell case
    // We should refactor this to apply to all cases
    if ( !spell.isSingleTarget() || !spell.isDamage() ) {
        return;
    }

    // If defender unit immune to magic, do not show the tooltip
    if ( !defender->AllowApplySpell( spell, hero ) ) {
        return;
    }

    if ( !_setDamageInfoBase( defender ) ) {
        return;
    }

    const int spellPoints = hero ? hero->GetPower() : fheroes2::spellPowerForBuiltinMonsterSpells;
    _minDamage = defender->CalculateSpellDamage( spell, spellPoints, hero, 0, true );
    _maxDamage = _minDamage;

    _makeDamageImage();

    _redraw = true;
}

void Battle::PopupDamageInfo::reset()
{
    if ( !_redraw ) {
        return;
    }

    _redraw = false;
    _needDelay = true;
    _defender = nullptr;
    _damageImage.clear();
}

void Battle::PopupDamageInfo::_makeDamageImage()
{
    assert( _defender != nullptr );

    std::string str = _minDamage == _maxDamage ? _( "Damage: %{max}" ) : _( "Damage: %{min} - %{max}" );

    StringReplace( str, "%{min}", std::to_string( _minDamage ) );
    StringReplace( str, "%{max}", std::to_string( _maxDamage ) );

    const fheroes2::Text damageText( str, fheroes2::FontType::smallWhite() );

    const uint32_t minNumKilled = _defender->HowManyWillBeKilled( _minDamage );
    const uint32_t maxNumKilled = _defender->HowManyWillBeKilled( _maxDamage );

    assert( minNumKilled <= _defender->GetCount() && maxNumKilled <= _defender->GetCount() );

    str = minNumKilled == maxNumKilled ? _( "Perish: %{max}" ) : _( "Perish: %{min} - %{max}" );

    StringReplace( str, "%{min}", std::to_string( minNumKilled ) );
    StringReplace( str, "%{max}", std::to_string( maxNumKilled ) );

    const fheroes2::Text killedText( str, fheroes2::FontType::smallWhite() );

    const fheroes2::Rect & unitRect = _defender->GetRectPosition();

    // Get the border width and set the popup parameters.
    const int32_t borderWidth = BorderWidth();
    const int32_t x = _battleUIRect.x + unitRect.x + unitRect.width;
    const int32_t y = _battleUIRect.y + unitRect.y;
    const int32_t w = std::max( damageText.width(), killedText.width() ) + 2 * borderWidth;
    const int32_t h = damageText.height() + killedText.height() + 2 * borderWidth;

    // If the damage info popup doesn't fit the battlefield draw surface, then try to place it on the left side of the cell
    const bool isLeftSidePopup = ( unitRect.x + unitRect.width + w ) > _battleUIRect.width;
    const fheroes2::Rect borderRect( isLeftSidePopup ? ( x - w - unitRect.width - borderWidth ) : x, y, w, h );

    const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( ICN::CELLWIN, 1 );

    _damageImage = fheroes2::Stretch( backgroundImage, 0, 0, backgroundImage.width(), backgroundImage.height(), borderRect.width, borderRect.height );
    _damageImage.setPosition( borderRect.x, borderRect.y );

    damageText.draw( borderWidth, borderWidth + 2, _damageImage );
    killedText.draw( borderWidth, borderRect.height / 2 + 2, _damageImage );
}

void Battle::PopupDamageInfo::redraw() const
{
    if ( !_redraw ) {
        return;
    }

    assert( !_damageImage.empty() );

    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::Copy( _damageImage, 0, 0, display, _damageImage.x(), _damageImage.y(), _damageImage.width(), _damageImage.height() );
    fheroes2::addGradientShadowForArea( display, { _damageImage.x(), _damageImage.y() }, _damageImage.width(), _damageImage.height(), 5 );
}
