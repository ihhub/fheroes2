/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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

#include "maps_tiles_render.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <list>
#include <map>
#include <ostream>
#include <string>
#include <utility>

#include "agg_image.h"
#include "color.h"
#include "direction.h"
#include "game.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "interface_gamearea.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "monster.h"
#include "monster_anim.h"
#include "mp2.h"
#include "race.h"
#include "spell.h"
#include "til.h"
#include "ui_constants.h"
#include "ui_object_rendering.h"
#include "world.h"

namespace
{
    const fheroes2::Point monsterImageOffset{ 16, 30 };

    bool contains( const int base, const int value )
    {
        return ( base & value ) == value;
    }

    bool isTileDirectRenderingRestricted( const int icnId, const MP2::MapObjectType objectType )
    {
        switch ( icnId ) {
        case ICN::UNKNOWN:
        case ICN::BOAT32:
        case ICN::MINIHERO:
            // Either it is an invalid sprite or a sprite which needs to be divided into tiles in order to properly render it.
            return true;
        case ICN::MONS32:
            // Random monsters must be displayed for the Editor.
            return objectType != MP2::OBJ_RANDOM_MONSTER && objectType != MP2::OBJ_RANDOM_MONSTER_WEAK && objectType != MP2::OBJ_RANDOM_MONSTER_MEDIUM
                   && objectType != MP2::OBJ_RANDOM_MONSTER_STRONG && objectType != MP2::OBJ_RANDOM_MONSTER_VERY_STRONG;
        default:
            break;
        }

        return false;
    }

    bool isObjectPartDirectRenderingRestricted( const int icnId )
    {
        switch ( icnId ) {
        case ICN::UNKNOWN:
        case ICN::BOAT32:
        case ICN::MINIHERO:
        case ICN::MONS32:
            // Either it is an invalid sprite or a sprite which needs to be divided into tiles in order to properly render it.
            return true;
        default:
            break;
        }

        return false;
    }

    void renderObjectPart( fheroes2::Image & output, const Interface::GameArea & area, const fheroes2::Point & offset, const Maps::ObjectPart & part )
    {
        assert( part.icnType != MP2::OBJ_ICN_TYPE_UNKNOWN && part.icnIndex != 255 );

        const int icn = MP2::getIcnIdFromObjectIcnType( part.icnType );
        if ( isObjectPartDirectRenderingRestricted( icn ) ) {
            return;
        }

        const uint8_t alphaValue = area.getObjectAlphaValue( part._uid );

        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, part.icnIndex );

        // Ideally we need to check that the image is within a tile area. However, flags are among those for which this rule doesn't apply.
        if ( icn == ICN::FLAG32 ) {
            assert( sprite.width() <= fheroes2::tileWidthPx && sprite.height() <= fheroes2::tileWidthPx );
        }
        else {
            assert( sprite.x() >= 0 && sprite.width() + sprite.x() <= fheroes2::tileWidthPx && sprite.y() >= 0 && sprite.height() + sprite.y() <= fheroes2::tileWidthPx );
        }

        area.BlitOnTile( output, sprite, sprite.x(), sprite.y(), offset, false, alphaValue );

        const uint32_t animationIndex = ICN::getAnimatedIcnIndex( icn, part.icnIndex, Game::getAdventureMapAnimationIndex() );
        if ( animationIndex > 0 ) {
            const fheroes2::Sprite & animationSprite = fheroes2::AGG::GetICN( icn, animationIndex );

            // If this assertion blows up we are trying to render an image bigger than a tile. Render this object properly as heroes or monsters!
            assert( animationSprite.x() >= 0 && animationSprite.width() + animationSprite.x() <= fheroes2::tileWidthPx && animationSprite.y() >= 0
                    && animationSprite.height() + animationSprite.y() <= fheroes2::tileWidthPx );

            area.BlitOnTile( output, animationSprite, animationSprite.x(), animationSprite.y(), offset, false, alphaValue );
        }
    }

    void renderMainObject( fheroes2::Image & output, const Interface::GameArea & area, const fheroes2::Point & offset, const Maps::Tile & tile )
    {
        assert( tile.getMainObjectPart().icnType != MP2::OBJ_ICN_TYPE_UNKNOWN && tile.getMainObjectPart().icnIndex != 255 );

        const int mainObjectIcn = MP2::getIcnIdFromObjectIcnType( tile.getMainObjectPart().icnType );
        if ( isTileDirectRenderingRestricted( mainObjectIcn, tile.getMainObjectType() ) ) {
            return;
        }

        const uint8_t mainObjectAlphaValue = area.getObjectAlphaValue( tile.getMainObjectPart()._uid );

        const fheroes2::Sprite & mainObjectSprite = fheroes2::AGG::GetICN( mainObjectIcn, tile.getMainObjectPart().icnIndex );

        // If this assertion blows up we are trying to render an image bigger than a tile. Render this object properly as heroes or monsters!
        assert( mainObjectSprite.x() >= 0 && mainObjectSprite.width() + mainObjectSprite.x() <= fheroes2::tileWidthPx && mainObjectSprite.y() >= 0
                && mainObjectSprite.height() + mainObjectSprite.y() <= fheroes2::tileWidthPx );

        area.BlitOnTile( output, mainObjectSprite, mainObjectSprite.x(), mainObjectSprite.y(), offset, false, mainObjectAlphaValue );

        // Render possible animation image.
        // TODO: quantity2 is used in absolutely incorrect way! Fix all the logic for it. As of now (quantity2 != 0) expression is used only for Magic Garden.
        const uint32_t mainObjectAnimationIndex
            = ICN::getAnimatedIcnIndex( mainObjectIcn, tile.getMainObjectPart().icnIndex, Game::getAdventureMapAnimationIndex(), tile.metadata()[1] != 0 );
        if ( mainObjectAnimationIndex > 0 ) {
            const fheroes2::Sprite & animationSprite = fheroes2::AGG::GetICN( mainObjectIcn, mainObjectAnimationIndex );

            // If this assertion blows up we are trying to render an image bigger than a tile. Render this object properly as heroes or monsters!
            assert( animationSprite.x() >= 0 && animationSprite.width() + animationSprite.x() <= fheroes2::tileWidthPx && animationSprite.y() >= 0
                    && animationSprite.height() + animationSprite.y() <= fheroes2::tileWidthPx );

            area.BlitOnTile( output, animationSprite, animationSprite.x(), animationSprite.y(), offset, false, mainObjectAlphaValue );
        }
    }

    const fheroes2::Image & PassableViewSurface( const int passable, const bool isActionObject )
    {
        static std::map<std::pair<int, bool>, fheroes2::Image> imageMap;

        auto key = std::make_pair( passable, isActionObject );

        auto iter = imageMap.find( key );
        if ( iter != imageMap.end() ) {
            return iter->second;
        }

        const int32_t size = 31;
        const uint8_t red = 0xBA;
        const uint8_t green = isActionObject ? 115 : 90;

        fheroes2::Image sf( size, size );
        sf.reset();

        if ( 0 == passable || Direction::CENTER == passable ) {
            fheroes2::DrawBorder( sf, red );
        }
        else if ( DIRECTION_ALL == passable ) {
            fheroes2::DrawBorder( sf, green );
        }
        else {
            const uint8_t topLeftColor = ( ( passable & Direction::TOP_LEFT ) != 0 ) ? green : red;
            const uint8_t bottomRightColor = ( ( passable & Direction::BOTTOM_RIGHT ) != 0 ) ? green : red;
            const uint8_t topRightColor = ( ( passable & Direction::TOP_RIGHT ) != 0 ) ? green : red;
            const uint8_t bottomLeftColor = ( ( passable & Direction::BOTTOM_LEFT ) != 0 ) ? green : red;
            const uint8_t topColor = ( ( passable & Direction::TOP ) != 0 ) ? green : red;
            const uint8_t bottomColor = ( ( passable & Direction::BOTTOM ) != 0 ) ? green : red;
            const uint8_t leftColor = ( ( passable & Direction::LEFT ) != 0 ) ? green : red;
            const uint8_t rightColor = ( ( passable & Direction::RIGHT ) != 0 ) ? green : red;

            uint8_t * image = sf.image();
            uint8_t * transform = sf.transform();

            // Horizontal
            for ( int32_t i = 0; i < 10; ++i ) {
                *( image + i ) = topLeftColor;
                *( transform + i ) = 0;

                *( image + i + ( size - 1 ) * size ) = bottomLeftColor;
                *( transform + i + ( size - 1 ) * size ) = 0;
            }

            for ( int32_t i = 10; i < 21; ++i ) {
                *( image + i ) = topColor;
                *( transform + i ) = 0;

                *( image + i + ( size - 1 ) * size ) = bottomColor;
                *( transform + i + ( size - 1 ) * size ) = 0;
            }

            for ( int32_t i = 21; i < size; ++i ) {
                *( image + i ) = topRightColor;
                *( transform + i ) = 0;

                *( image + i + ( size - 1 ) * size ) = bottomRightColor;
                *( transform + i + ( size - 1 ) * size ) = 0;
            }

            // Vertical
            for ( int32_t i = 0; i < 10; ++i ) {
                *( image + i * size ) = topLeftColor;
                *( transform + i * size ) = 0;

                *( image + size - 1 + i * size ) = topRightColor;
                *( transform + size - 1 + i * size ) = 0;
            }

            for ( int32_t i = 10; i < 21; ++i ) {
                *( image + i * size ) = leftColor;
                *( transform + i * size ) = 0;

                *( image + size - 1 + i * size ) = rightColor;
                *( transform + size - 1 + i * size ) = 0;
            }

            for ( int32_t i = 21; i < size; ++i ) {
                *( image + i * size ) = bottomLeftColor;
                *( transform + i * size ) = 0;

                *( image + size - 1 + i * size ) = bottomRightColor;
                *( transform + size - 1 + i * size ) = 0;
            }
        }

        return imageMap.try_emplace( std::move( key ), std::move( sf ) ).first->second;
    }

#ifdef WITH_DEBUG
    const fheroes2::Image & getDebugFogImage()
    {
        static const fheroes2::Image fog = []() {
            fheroes2::Image temp( 32, 32 );
            fheroes2::FillTransform( temp, 0, 0, temp.width(), temp.height(), 2 );
            return temp;
        }();

        return fog;
    }
#endif

    std::pair<uint32_t, uint32_t> GetMonsterSpriteIndices( const Maps::Tile & tile, uint32_t monsterIndex, const bool isEditorMode )
    {
        const int tileIndex = tile.GetIndex();
        int attackerIndex = -1;

        // scan for a hero around
        if ( !isEditorMode ) {
            for ( const int32_t idx : Maps::ScanAroundObject( tileIndex, MP2::OBJ_HERO, false ) ) {
                const Heroes * hero = world.getTile( idx ).getHero();
                assert( hero != nullptr );

                // hero is going to attack monsters on this tile
                if ( hero->GetAttackedMonsterTileIndex() == tileIndex ) {
                    attackerIndex = idx;
                    break;
                }
            }
        }

        std::pair<uint32_t, uint32_t> spriteIndices( monsterIndex * 9, 0 );

        // draw an attacking sprite if there is an attacking hero nearby
        if ( attackerIndex != -1 ) {
            spriteIndices.first += 7;

            switch ( Maps::GetDirection( tileIndex, attackerIndex ) ) {
            case Direction::TOP_LEFT:
            case Direction::LEFT:
            case Direction::BOTTOM_LEFT:
                spriteIndices.first += 1;
                break;
            default:
                break;
            }
        }
        else {
            const fheroes2::Point & mp = Maps::GetPoint( tileIndex );
            const std::array<uint8_t, 15> & monsterAnimationSequence = fheroes2::getMonsterAnimationSequence();
            spriteIndices.second
                = monsterIndex * 9 + 1 + monsterAnimationSequence[( Game::getAdventureMapAnimationIndex() + mp.x * mp.y ) % monsterAnimationSequence.size()];
        }
        return spriteIndices;
    }

    bool doesHeroImageNeedToBeReflected( const int directionFrom )
    {
        switch ( directionFrom ) {
        case Direction::TOP_LEFT:
        case Direction::LEFT:
        case Direction::BOTTOM_LEFT:
            return true;

        default:
            break;
        }

        return false;
    }

    void getHeroSpriteInfo( const Heroes & hero, const int heroMovementIndex, const bool isHeroChangingDirection, int & icnId, uint32_t & icnIndex )
    {
        icnId = ICN::UNKNOWN;

        if ( hero.isShipMaster() ) {
            icnId = ICN::BOAT32;
        }
        else {
            const int raceId = hero.GetRace();
            switch ( raceId ) {
            case Race::KNGT:
                icnId = ICN::KNGT32;
                break;
            case Race::BARB:
                icnId = ICN::BARB32;
                break;
            case Race::SORC:
                icnId = ICN::SORC32;
                break;
            case Race::WRLK:
                icnId = ICN::WRLK32;
                break;
            case Race::WZRD:
                icnId = ICN::WZRD32;
                break;
            case Race::NECR:
                icnId = ICN::NECR32;
                break;

            default:
                // Did you add a new race? Add logic above!
                assert( 0 );
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero race " << raceId )
                break;
            }
        }

        icnIndex = 0;

        if ( isHeroChangingDirection ) {
            icnIndex = 45;
        }
        else {
            const int heroDirection = hero.GetDirection();
            switch ( heroDirection ) {
            case Direction::TOP:
                icnIndex = 0;
                break;
            case Direction::TOP_RIGHT:
                icnIndex = 9;
                break;
            case Direction::RIGHT:
                icnIndex = 18;
                break;
            case Direction::BOTTOM_RIGHT:
                icnIndex = 27;
                break;
            case Direction::BOTTOM:
                icnIndex = 36;
                break;
            case Direction::BOTTOM_LEFT:
                icnIndex = 27;
                break;
            case Direction::LEFT:
                icnIndex = 18;
                break;
            case Direction::TOP_LEFT:
                icnIndex = 9;
                break;

            default:
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero direction" << heroDirection )
                break;
            }
        }

        icnIndex = icnIndex + ( heroMovementIndex % Heroes::heroFrameCountPerTile );
    }

    void getFlagSpriteInfo( const Heroes & hero, const int heroMovementIndex, const bool isHeroChangingDirection, fheroes2::Point & flagOffset, int & icnId,
                            uint32_t & icnIndex )
    {
        icnId = ICN::UNKNOWN;

        const int heroColor = hero.GetColor();
        switch ( heroColor ) {
        case Color::BLUE:
            icnId = hero.isShipMaster() ? ICN::B_BFLG32 : ICN::B_FLAG32;
            break;
        case Color::GREEN:
            icnId = hero.isShipMaster() ? ICN::G_BFLG32 : ICN::G_FLAG32;
            break;
        case Color::RED:
            icnId = hero.isShipMaster() ? ICN::R_BFLG32 : ICN::R_FLAG32;
            break;
        case Color::YELLOW:
            icnId = hero.isShipMaster() ? ICN::Y_BFLG32 : ICN::Y_FLAG32;
            break;
        case Color::ORANGE:
            icnId = hero.isShipMaster() ? ICN::O_BFLG32 : ICN::O_FLAG32;
            break;
        case Color::PURPLE:
            icnId = hero.isShipMaster() ? ICN::P_BFLG32 : ICN::P_FLAG32;
            break;

        default:
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero color " << heroColor )
            break;
        }

        icnIndex = 0;

        if ( isHeroChangingDirection ) {
            icnIndex = 45;
        }
        else {
            const int heroDirection = hero.GetDirection();
            switch ( heroDirection ) {
            case Direction::TOP:
                icnIndex = 0;
                break;
            case Direction::TOP_RIGHT:
                icnIndex = 9;
                break;
            case Direction::RIGHT:
                icnIndex = 18;
                break;
            case Direction::BOTTOM_RIGHT:
                icnIndex = 27;
                break;
            case Direction::BOTTOM:
                icnIndex = 36;
                break;
            case Direction::BOTTOM_LEFT:
                icnIndex = 27;
                break;
            case Direction::LEFT:
                icnIndex = 18;
                break;
            case Direction::TOP_LEFT:
                icnIndex = 9;
                break;

            default:
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero direction " << heroDirection )
                break;
            }
        }

        const int frameId = heroMovementIndex % Heroes::heroFrameCountPerTile;
        icnIndex = icnIndex + frameId;

        if ( !hero.isMoveEnabled() ) {
            static const fheroes2::Point offsetTop[Heroes::heroFrameCountPerTile]
                = { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 0, 2 }, { 0, 0 }, { 0, 1 }, { 0, 3 }, { 0, 2 }, { 0, 1 } };
            static const fheroes2::Point offsetBottom[Heroes::heroFrameCountPerTile]
                = { { 0, 0 }, { 0, -1 }, { 0, -2 }, { 0, 0 }, { 0, -1 }, { 0, -2 }, { 0, -3 }, { 0, 0 }, { 0, -1 } };
            static const fheroes2::Point offsetSideways[Heroes::heroFrameCountPerTile]
                = { { 0, 0 }, { -1, 0 }, { 0, 0 }, { 1, 0 }, { 1, -1 }, { 2, -1 }, { 1, 0 }, { 0, 0 }, { 1, 0 } };
            static const fheroes2::Point offsetTopSideways[Heroes::heroFrameCountPerTile]
                = { { 0, 0 }, { -1, 0 }, { 0, 0 }, { -1, -1 }, { -2, -1 }, { -2, 0 }, { -1, 0 }, { 0, 0 }, { 1, 0 } };
            static const fheroes2::Point offsetBottomSideways[Heroes::heroFrameCountPerTile]
                = { { 0, 0 }, { -1, 0 }, { 0, -1 }, { 2, -2 }, { 0, -2 }, { -1, -3 }, { -1, -2 }, { -1, -1 }, { 1, 0 } };

            static const fheroes2::Point offsetShipTopBottom[Heroes::heroFrameCountPerTile]
                = { { 0, -1 }, { 0, 0 }, { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 0 }, { 0, 1 }, { 0, 1 }, { 0, 1 } };
            static const fheroes2::Point offsetShipSideways[Heroes::heroFrameCountPerTile]
                = { { 0, -2 }, { 0, -1 }, { 0, 0 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { 0, 0 }, { 0, -1 }, { 0, 1 } };
            static const fheroes2::Point offsetShipTopSideways[Heroes::heroFrameCountPerTile]
                = { { 0, 0 }, { 0, -1 }, { 0, 0 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { 0, 0 }, { 0, -1 }, { 0, 1 } };
            static const fheroes2::Point offsetShipBottomSideways[Heroes::heroFrameCountPerTile]
                = { { 0, -2 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };

            const int heroDirection = hero.GetDirection();
            switch ( heroDirection ) {
            case Direction::TOP:
                flagOffset = hero.isShipMaster() ? offsetShipTopBottom[frameId] : offsetTop[frameId];
                break;
            case Direction::BOTTOM:
                flagOffset = hero.isShipMaster() ? offsetShipTopBottom[frameId] : offsetBottom[frameId];
                break;
            case Direction::RIGHT:
            case Direction::LEFT:
                flagOffset = hero.isShipMaster() ? offsetShipSideways[frameId] : offsetSideways[frameId];
                break;
            case Direction::TOP_RIGHT:
            case Direction::TOP_LEFT:
                flagOffset = hero.isShipMaster() ? offsetShipTopSideways[frameId] : offsetTopSideways[frameId];
                break;
            case Direction::BOTTOM_RIGHT:
            case Direction::BOTTOM_LEFT:
                flagOffset = hero.isShipMaster() ? offsetShipBottomSideways[frameId] : offsetBottomSideways[frameId];
                break;

            default:
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero direction " << heroDirection )
                break;
            }
        }
    }

    void getShadowSpriteInfo( const Heroes & hero, const int heroMovementIndex, int & icnId, uint32_t & icnIndex )
    {
        const int32_t heroDirection = hero.GetDirection();
        const bool isOnBoat = hero.isShipMaster();

        switch ( heroDirection ) {
        case Direction::TOP:
            icnIndex = 0;
            break;
        case Direction::TOP_RIGHT:
            icnIndex = 9;
            break;
        case Direction::RIGHT:
            icnIndex = 18;
            break;
        case Direction::BOTTOM_RIGHT:
            icnIndex = 27;
            break;
        case Direction::BOTTOM:
            icnIndex = 36;
            break;
        case Direction::BOTTOM_LEFT:
            icnIndex = isOnBoat ? 45 : 77;
            break;
        case Direction::LEFT:
            icnIndex = isOnBoat ? 54 : 68;
            break;
        case Direction::TOP_LEFT:
            icnIndex = isOnBoat ? 63 : 59;
            break;
        default:
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero direction " << heroDirection )
            break;
        }

        icnId = isOnBoat ? ICN::BOATSHAD : ICN::SHADOW32;

        icnIndex += ( heroMovementIndex % Heroes::heroFrameCountPerTile );
    }

    void getFrothSpriteInfo( const Heroes & hero, const int heroMovementIndex, int & icnId, uint32_t & icnIndex )
    {
        icnIndex = 0;

        const int heroDirection = hero.GetDirection();
        switch ( heroDirection ) {
        case Direction::TOP:
            icnIndex = 0;
            break;
        case Direction::TOP_RIGHT:
            icnIndex = 9;
            break;
        case Direction::RIGHT:
            icnIndex = 18;
            break;
        case Direction::BOTTOM_RIGHT:
            icnIndex = 27;
            break;
        case Direction::BOTTOM:
            icnIndex = 36;
            break;
        case Direction::BOTTOM_LEFT:
            icnIndex = 27;
            break;
        case Direction::LEFT:
            icnIndex = 18;
            break;
        case Direction::TOP_LEFT:
            icnIndex = 9;
            break;

        default:
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero direction " << heroDirection )
            break;
        }

        icnId = ICN::FROTH;
        icnIndex = icnIndex + ( heroMovementIndex % Heroes::heroFrameCountPerTile );
    }
}

namespace Maps
{
    void redrawEmptyTile( fheroes2::Image & dst, const fheroes2::Point & mp, const Interface::GameArea & area )
    {
        if ( mp.y == -1 && mp.x >= 0 && mp.x < world.w() ) { // top first row
            area.DrawTile( dst, fheroes2::AGG::GetTIL( TIL::STON, 20 + ( mp.x % 4 ), 0 ), mp );
        }
        else if ( mp.x == world.w() && mp.y >= 0 && mp.y < world.h() ) { // right first row
            area.DrawTile( dst, fheroes2::AGG::GetTIL( TIL::STON, 24 + ( mp.y % 4 ), 0 ), mp );
        }
        else if ( mp.y == world.h() && mp.x >= 0 && mp.x < world.w() ) { // bottom first row
            area.DrawTile( dst, fheroes2::AGG::GetTIL( TIL::STON, 28 + ( mp.x % 4 ), 0 ), mp );
        }
        else if ( mp.x == -1 && mp.y >= 0 && mp.y < world.h() ) { // left first row
            area.DrawTile( dst, fheroes2::AGG::GetTIL( TIL::STON, 32 + ( mp.y % 4 ), 0 ), mp );
        }
        else {
            area.DrawTile( dst, fheroes2::AGG::GetTIL( TIL::STON, ( std::abs( mp.y ) % 4 ) * 4 + std::abs( mp.x ) % 4, 0 ), mp );
        }
    }

    void redrawTopLayerExtraObjects( const Tile & tile, fheroes2::Image & dst, const bool isPuzzleDraw, const Interface::GameArea & area )
    {
        if ( isPuzzleDraw ) {
            // Extra objects should not be shown on Puzzle Map as they are temporary objects appearing under specific conditions like flags.
            return;
        }

        // Ghost animation is unique and can be rendered in multiple cases.
        bool renderFlyingGhosts = false;

        const MP2::MapObjectType objectType = tile.getMainObjectType( false );
        if ( objectType == MP2::OBJ_ABANDONED_MINE ) {
            renderFlyingGhosts = true;
        }
        else if ( objectType == MP2::OBJ_MINE ) {
            const int32_t spellID = Maps::getMineSpellIdFromTile( tile );

            switch ( spellID ) {
            case Spell::NONE:
                // No spell exists. Nothing we need to render.
            case Spell::SETEGUARDIAN:
            case Spell::SETAGUARDIAN:
            case Spell::SETFGUARDIAN:
            case Spell::SETWGUARDIAN:
                // The logic for these spells is done while rendering the bottom layer. Nothing should be done here.
                break;
            case Spell::HAUNT:
                renderFlyingGhosts = true;
                break;
            default:
                // Did you add a new spell for mines? Add the rendering for it above!
                assert( 0 );
                break;
            }
        }

        if ( renderFlyingGhosts ) {
            // This sprite is bigger than tileWidthPx but rendering is correct for heroes and boats.
            // TODO: consider adding this sprite as a part of an object part.
            const fheroes2::Sprite & image = fheroes2::AGG::GetICN( ICN::OBJNHAUN, Game::getAdventureMapAnimationIndex() % 15 );

            const uint8_t alphaValue = area.getObjectAlphaValue( tile.getMainObjectPart()._uid );

            area.BlitOnTile( dst, image, image.x(), image.y(), Maps::GetPoint( tile.GetIndex() ), false, alphaValue );
        }
    }

    void redrawTopLayerObject( const Tile & tile, fheroes2::Image & dst, const bool isPuzzleDraw, const Interface::GameArea & area, const ObjectPart & part )
    {
        if ( isPuzzleDraw && MP2::isHiddenForPuzzle( tile.GetGround(), part.icnType, part.icnIndex ) ) {
            return;
        }

        renderObjectPart( dst, area, Maps::GetPoint( tile.GetIndex() ), part );
    }

    void drawFog( const Tile & tile, fheroes2::Image & dst, const Interface::GameArea & area )
    {
        const uint16_t fogDirection = tile.getFogDirection();
        // This method should not be called for a tile without fog.
        assert( fogDirection & Direction::CENTER );

        const fheroes2::Point & mp = Maps::GetPoint( tile.GetIndex() );

        // TODO: Cache all directions into a map: have an array which represents all conditions within this method. The index can be 'fogDirection', the value is index.
        // And another array to store revert flag.

        if ( DIRECTION_ALL == fogDirection ) {
            const fheroes2::Image & sf = fheroes2::AGG::GetTIL( TIL::CLOF32, ( mp.x + mp.y ) % 4, 0 );
            area.DrawTile( dst, sf, mp );
        }
        else {
            uint32_t index = 0;
            bool revert = false;

            if ( !( fogDirection & ( Direction::TOP | Direction::BOTTOM | Direction::LEFT | Direction::RIGHT ) ) ) {
                index = 10;
            }
            else if ( ( contains( fogDirection, Direction::TOP ) ) && !( fogDirection & ( Direction::BOTTOM | Direction::LEFT | Direction::RIGHT ) ) ) {
                index = 6;
            }
            else if ( ( contains( fogDirection, Direction::RIGHT ) ) && !( fogDirection & ( Direction::TOP | Direction::BOTTOM | Direction::LEFT ) ) ) {
                index = 7;
            }
            else if ( ( contains( fogDirection, Direction::LEFT ) ) && !( fogDirection & ( Direction::TOP | Direction::BOTTOM | Direction::RIGHT ) ) ) {
                index = 7;
                revert = true;
            }
            else if ( ( contains( fogDirection, Direction::BOTTOM ) ) && !( fogDirection & ( Direction::TOP | Direction::LEFT | Direction::RIGHT ) ) ) {
                index = 8;
            }
            else if ( ( contains( fogDirection, DIRECTION_CENTER_COL ) ) && !( fogDirection & ( Direction::LEFT | Direction::RIGHT ) ) ) {
                index = 9;
            }
            else if ( ( contains( fogDirection, DIRECTION_CENTER_ROW ) ) && !( fogDirection & ( Direction::TOP | Direction::BOTTOM ) ) ) {
                index = 29;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~Direction::TOP_RIGHT ) ) ) {
                index = 15;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~Direction::TOP_LEFT ) ) ) {
                index = 15;
                revert = true;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~Direction::BOTTOM_RIGHT ) ) ) {
                index = 22;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~Direction::BOTTOM_LEFT ) ) ) {
                index = 22;
                revert = true;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~( Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT ) ) ) ) {
                index = 16;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~( Direction::TOP_LEFT | Direction::BOTTOM_LEFT ) ) ) ) {
                index = 16;
                revert = true;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~( Direction::TOP_RIGHT | Direction::BOTTOM_LEFT ) ) ) ) {
                index = 17;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~( Direction::TOP_LEFT | Direction::BOTTOM_RIGHT ) ) ) ) {
                index = 17;
                revert = true;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~( Direction::TOP_LEFT | Direction::TOP_RIGHT ) ) ) ) {
                index = 18;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~( Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT ) ) ) ) {
                index = 23;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~DIRECTION_TOP_RIGHT_CORNER ) ) ) {
                index = 13;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~DIRECTION_TOP_LEFT_CORNER ) ) ) {
                index = 13;
                revert = true;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~DIRECTION_BOTTOM_RIGHT_CORNER ) ) ) {
                index = 14;
            }
            else if ( fogDirection == ( DIRECTION_ALL & ( ~DIRECTION_BOTTOM_LEFT_CORNER ) ) ) {
                index = 14;
                revert = true;
            }
            else if ( contains( fogDirection, Direction::LEFT | Direction::BOTTOM_LEFT | Direction::BOTTOM )
                      && !( fogDirection & ( Direction::TOP | Direction::RIGHT ) ) ) {
                index = 11;
            }
            else if ( contains( fogDirection, Direction::RIGHT | Direction::BOTTOM_RIGHT | Direction::BOTTOM )
                      && !( fogDirection & ( Direction::TOP | Direction::LEFT ) ) ) {
                index = 11;
                revert = true;
            }
            else if ( contains( fogDirection, Direction::LEFT | Direction::TOP_LEFT | Direction::TOP ) && !( fogDirection & ( Direction::BOTTOM | Direction::RIGHT ) ) ) {
                index = 12;
            }
            else if ( contains( fogDirection, Direction::RIGHT | Direction::TOP_RIGHT | Direction::TOP )
                      && !( fogDirection & ( Direction::BOTTOM | Direction::LEFT ) ) ) {
                index = 12;
                revert = true;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP | Direction::TOP_LEFT )
                      && !( fogDirection & ( Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT | Direction::TOP_RIGHT ) ) ) {
                index = 19;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP | Direction::TOP_RIGHT )
                      && !( fogDirection & ( Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT | Direction::TOP_LEFT ) ) ) {
                index = 19;
                revert = true;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP | Direction::BOTTOM_LEFT )
                      && !( fogDirection & ( Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT | Direction::TOP_LEFT ) ) ) {
                index = 20;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP | Direction::BOTTOM_RIGHT )
                      && !( fogDirection & ( Direction::TOP_RIGHT | Direction::BOTTOM_LEFT | Direction::TOP_LEFT ) ) ) {
                index = 20;
                revert = true;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP )
                      && !( fogDirection & ( Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT | Direction::BOTTOM_LEFT | Direction::TOP_LEFT ) ) ) {
                index = 21;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::BOTTOM_LEFT )
                      && !( fogDirection & ( Direction::TOP | Direction::BOTTOM_RIGHT ) ) ) {
                index = 24;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::BOTTOM_RIGHT )
                      && !( fogDirection & ( Direction::TOP | Direction::BOTTOM_LEFT ) ) ) {
                index = 24;
                revert = true;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_COL | Direction::LEFT | Direction::TOP_LEFT )
                      && !( fogDirection & ( Direction::RIGHT | Direction::BOTTOM_LEFT ) ) ) {
                index = 25;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_COL | Direction::RIGHT | Direction::TOP_RIGHT )
                      && !( fogDirection & ( Direction::LEFT | Direction::BOTTOM_RIGHT ) ) ) {
                index = 25;
                revert = true;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_COL | Direction::BOTTOM_LEFT | Direction::LEFT )
                      && !( fogDirection & ( Direction::RIGHT | Direction::TOP_LEFT ) ) ) {
                index = 26;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_COL | Direction::BOTTOM_RIGHT | Direction::RIGHT )
                      && !( fogDirection & ( Direction::LEFT | Direction::TOP_RIGHT ) ) ) {
                index = 26;
                revert = true;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | Direction::TOP_LEFT | Direction::TOP )
                      && !( fogDirection & ( Direction::BOTTOM | Direction::TOP_RIGHT ) ) ) {
                index = 30;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | Direction::TOP_RIGHT | Direction::TOP )
                      && !( fogDirection & ( Direction::BOTTOM | Direction::TOP_LEFT ) ) ) {
                index = 30;
                revert = true;
            }
            else if ( contains( fogDirection, Direction::BOTTOM | Direction::LEFT )
                      && !( fogDirection & ( Direction::TOP | Direction::RIGHT | Direction::BOTTOM_LEFT ) ) ) {
                index = 27;
            }
            else if ( contains( fogDirection, Direction::BOTTOM | Direction::RIGHT )
                      && !( fogDirection & ( Direction::TOP | Direction::LEFT | Direction::BOTTOM_RIGHT ) ) ) {
                index = 27;
                revert = true;
            }
            else if ( contains( fogDirection, Direction::LEFT | Direction::TOP ) && !( fogDirection & ( Direction::TOP_LEFT | Direction::RIGHT | Direction::BOTTOM ) ) ) {
                index = 28;
            }
            else if ( contains( fogDirection, Direction::RIGHT | Direction::TOP )
                      && !( fogDirection & ( Direction::TOP_RIGHT | Direction::LEFT | Direction::BOTTOM ) ) ) {
                index = 28;
                revert = true;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | Direction::TOP )
                      && !( fogDirection & ( Direction::BOTTOM | Direction::TOP_LEFT | Direction::TOP_RIGHT ) ) ) {
                index = 31;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_COL | Direction::RIGHT )
                      && !( fogDirection & ( Direction::LEFT | Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT ) ) ) {
                index = 32;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_COL | Direction::LEFT )
                      && !( fogDirection & ( Direction::RIGHT | Direction::TOP_LEFT | Direction::BOTTOM_LEFT ) ) ) {
                index = 32;
                revert = true;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | Direction::BOTTOM )
                      && !( fogDirection & ( Direction::TOP | Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT ) ) ) {
                index = 33;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW ) && !( fogDirection & Direction::TOP ) ) {
                index = ( tile.GetIndex() % 2 ) ? 0 : 1;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_ROW | DIRECTION_TOP_ROW ) && !( fogDirection & Direction::BOTTOM ) ) {
                index = ( tile.GetIndex() % 2 ) ? 4 : 5;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_COL | DIRECTION_LEFT_COL ) && !( fogDirection & Direction::RIGHT ) ) {
                index = ( tile.GetIndex() % 2 ) ? 2 : 3;
            }
            else if ( contains( fogDirection, DIRECTION_CENTER_COL | DIRECTION_RIGHT_COL ) && !( fogDirection & Direction::LEFT ) ) {
                index = ( tile.GetIndex() % 2 ) ? 2 : 3;
                revert = true;
            }
            else {
                // unknown
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid direction for fog: " << Direction::String( fogDirection ) << ". Tile index: " << tile.GetIndex() )
                const fheroes2::Image & sf = fheroes2::AGG::GetTIL( TIL::CLOF32, ( mp.x + mp.y ) % 4, 0 );
                area.DrawTile( dst, sf, mp );
                return;
            }

            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::CLOP32, index );
            area.BlitOnTile( dst, sprite, ( revert ? fheroes2::tileWidthPx - sprite.x() - sprite.width() : sprite.x() ), sprite.y(), mp, revert, 255 );
        }
    }

    void redrawPassable( const Tile & tile, fheroes2::Image & dst, const int friendColors, const Interface::GameArea & area, const bool isEditor )
    {
#ifdef WITH_DEBUG
        if ( friendColors != 0 && tile.isFog( friendColors ) ) {
            area.BlitOnTile( dst, getDebugFogImage(), 0, 0, Maps::GetPoint( tile.GetIndex() ), false, 255 );
        }
#else
        (void)friendColors;
#endif

        const bool isActionObject = isEditor ? MP2::isOffGameActionObject( tile.getMainObjectType() ) : MP2::isInGameActionObject( tile.getMainObjectType() );
        if ( isActionObject || tile.GetPassable() != DIRECTION_ALL ) {
            area.BlitOnTile( dst, PassableViewSurface( tile.GetPassable(), isActionObject ), 0, 0, Maps::GetPoint( tile.GetIndex() ), false, 255 );
        }
    }

    void redrawBottomLayerObjects( const Tile & tile, fheroes2::Image & dst, bool isPuzzleDraw, const Interface::GameArea & area, const uint8_t level )
    {
        assert( level <= 0x03 );

        const fheroes2::Point & mp = Maps::GetPoint( tile.GetIndex() );

        // Since the original game stores information about objects in a very weird way and this is how it is implemented for us we need to do the following procedure:
        // - run through all ground object parts first
        // - check the main object which is on the tile

        // Some object parts must be rendered after the main object on the tile. This applies for flags.
        // Since this method is called intensively during rendering we have to avoid memory allocation on heap.
        const size_t maxPostRenderPart = 16;
        std::array<const ObjectPart *, maxPostRenderPart> postRenderingPart{};
        size_t postRenderObjectCount = 0;

        for ( const auto & part : tile.getGroundObjectParts() ) {
            if ( part.layerType != level ) {
                continue;
            }

            if ( isPuzzleDraw && MP2::isHiddenForPuzzle( tile.GetGround(), part.icnType, part.icnIndex ) ) {
                continue;
            }

            if ( part.icnType == MP2::OBJ_ICN_TYPE_FLAG32 ) {
                // Based on logically thinking it is impossible to have more than 16 flags on a single tile.
                assert( postRenderObjectCount < maxPostRenderPart );

                postRenderingPart[postRenderObjectCount] = &part;
                ++postRenderObjectCount;
                continue;
            }

            renderObjectPart( dst, area, mp, part );
        }

        if ( tile.getMainObjectPart().icnType != MP2::OBJ_ICN_TYPE_UNKNOWN && tile.getMainObjectPart().layerType == level
             && ( !isPuzzleDraw || !MP2::isHiddenForPuzzle( tile.GetGround(), tile.getMainObjectPart().icnType, tile.getMainObjectPart().icnIndex ) ) ) {
            renderMainObject( dst, area, mp, tile );
        }

        for ( size_t i = 0; i < postRenderObjectCount; ++i ) {
            assert( postRenderingPart[i] != nullptr );

            renderObjectPart( dst, area, mp, *postRenderingPart[i] );
        }
    }

    void drawByObjectIcnType( const Tile & tile, fheroes2::Image & output, const Interface::GameArea & area, const MP2::ObjectIcnType objectIcnType )
    {
        const fheroes2::Point & tileOffset = Maps::GetPoint( tile.GetIndex() );

        for ( const auto & part : tile.getGroundObjectParts() ) {
            if ( part.icnType == objectIcnType ) {
                renderObjectPart( output, area, tileOffset, part );
            }
        }

        if ( tile.getMainObjectPart().icnType == objectIcnType ) {
            renderMainObject( output, area, tileOffset, tile );
        }

        for ( const auto & part : tile.getTopObjectParts() ) {
            if ( part.icnType == objectIcnType ) {
                renderObjectPart( output, area, tileOffset, part );
            }
        }
    }

    std::vector<fheroes2::ObjectRenderingInfo> getMonsterSpritesPerTile( const Tile & tile, const bool isEditorMode )
    {
        assert( tile.getMainObjectType() == MP2::OBJ_MONSTER );

        const Monster monster = getMonsterFromTile( tile );
        const std::pair<uint32_t, uint32_t> spriteIndices = GetMonsterSpriteIndices( tile, monster.GetSpriteIndex(), isEditorMode );

        const int icnId{ ICN::MINI_MONSTER_IMAGE };
        const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( icnId, spriteIndices.first );
        const fheroes2::Point monsterSpriteOffset( monsterSprite.x() + monsterImageOffset.x, monsterSprite.y() + monsterImageOffset.y );

        std::vector<fheroes2::Point> outputSquareInfo;
        std::vector<std::pair<fheroes2::Point, fheroes2::Rect>> outputImageInfo;
        fheroes2::DivideImageBySquares( monsterSpriteOffset, monsterSprite, fheroes2::tileWidthPx, outputSquareInfo, outputImageInfo );

        assert( outputSquareInfo.size() == outputImageInfo.size() );

        std::vector<fheroes2::ObjectRenderingInfo> objectInfo;
        for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
            objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, spriteIndices.first, false,
                                     static_cast<uint8_t>( 255 ) );
        }

        outputSquareInfo.clear();
        outputImageInfo.clear();

        if ( spriteIndices.second > 0 ) {
            const fheroes2::Sprite & secondaryMonsterSprite = fheroes2::AGG::GetICN( icnId, spriteIndices.second );
            const fheroes2::Point secondaryMonsterSpriteOffset( secondaryMonsterSprite.x() + monsterImageOffset.x, secondaryMonsterSprite.y() + monsterImageOffset.y );

            fheroes2::DivideImageBySquares( secondaryMonsterSpriteOffset, secondaryMonsterSprite, fheroes2::tileWidthPx, outputSquareInfo, outputImageInfo );

            assert( outputSquareInfo.size() == outputImageInfo.size() );

            for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
                objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, spriteIndices.second, false,
                                         static_cast<uint8_t>( 255 ) );
            }
        }

        return objectInfo;
    }

    std::vector<fheroes2::ObjectRenderingInfo> getMonsterShadowSpritesPerTile( const Tile & tile, const bool isEditorMode )
    {
        assert( tile.getMainObjectType() == MP2::OBJ_MONSTER );

        const Monster monster = getMonsterFromTile( tile );
        const std::pair<uint32_t, uint32_t> spriteIndices = GetMonsterSpriteIndices( tile, monster.GetSpriteIndex(), isEditorMode );

        const int icnId{ ICN::MINI_MONSTER_SHADOW };
        const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( icnId, spriteIndices.first );
        const fheroes2::Point monsterSpriteOffset( monsterSprite.x() + monsterImageOffset.x, monsterSprite.y() + monsterImageOffset.y );

        std::vector<fheroes2::Point> outputSquareInfo;
        std::vector<std::pair<fheroes2::Point, fheroes2::Rect>> outputImageInfo;
        fheroes2::DivideImageBySquares( monsterSpriteOffset, monsterSprite, fheroes2::tileWidthPx, outputSquareInfo, outputImageInfo );

        assert( outputSquareInfo.size() == outputImageInfo.size() );

        std::vector<fheroes2::ObjectRenderingInfo> objectInfo;
        for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
            objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, spriteIndices.first, false,
                                     static_cast<uint8_t>( 255 ) );
        }

        outputSquareInfo.clear();
        outputImageInfo.clear();

        if ( spriteIndices.second > 0 ) {
            const fheroes2::Sprite & secondaryMonsterSprite = fheroes2::AGG::GetICN( icnId, spriteIndices.second );
            const fheroes2::Point secondaryMonsterSpriteOffset( secondaryMonsterSprite.x() + monsterImageOffset.x, secondaryMonsterSprite.y() + monsterImageOffset.y );

            fheroes2::DivideImageBySquares( secondaryMonsterSpriteOffset, secondaryMonsterSprite, fheroes2::tileWidthPx, outputSquareInfo, outputImageInfo );

            assert( outputSquareInfo.size() == outputImageInfo.size() );

            for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
                objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, spriteIndices.second, false,
                                         static_cast<uint8_t>( 255 ) );
            }
        }

        return objectInfo;
    }

    std::vector<fheroes2::ObjectRenderingInfo> getBoatSpritesPerTile( const Tile & tile )
    {
        // TODO: combine both boat image generation for heroes and empty boats.
        assert( tile.getMainObjectType() == MP2::OBJ_BOAT );

        const uint32_t spriteIndex = ( tile.getMainObjectPart().icnIndex == 255 ) ? 18 : tile.getMainObjectPart().icnIndex;

        const bool isReflected = ( spriteIndex > 128 );

        const int icnId{ ICN::BOAT32 };
        const uint32_t icnIndex = spriteIndex % 128;
        const fheroes2::Sprite & boatSprite = fheroes2::AGG::GetICN( icnId, icnIndex );

        const fheroes2::Point boatSpriteOffset( ( isReflected ? ( fheroes2::tileWidthPx + 1 - boatSprite.x() - boatSprite.width() ) : boatSprite.x() ),
                                                boatSprite.y() + fheroes2::tileWidthPx - 11 );

        std::vector<fheroes2::Point> outputSquareInfo;
        std::vector<std::pair<fheroes2::Point, fheroes2::Rect>> outputImageInfo;
        fheroes2::DivideImageBySquares( boatSpriteOffset, boatSprite, fheroes2::tileWidthPx, outputSquareInfo, outputImageInfo );

        assert( outputSquareInfo.size() == outputImageInfo.size() );

        std::vector<fheroes2::ObjectRenderingInfo> objectInfo;
        for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
            objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, icnIndex, isReflected,
                                     static_cast<uint8_t>( 255 ) );
        }

        return objectInfo;
    }

    std::vector<fheroes2::ObjectRenderingInfo> getBoatShadowSpritesPerTile( const Tile & tile )
    {
        assert( tile.getMainObjectType() == MP2::OBJ_BOAT );

        // TODO: boat shadow logic is more complex than this and it is not directly depend on spriteIndex. Find the proper logic and fix it!
        const uint32_t spriteIndex = ( tile.getMainObjectPart().icnIndex == 255 ) ? 18 : tile.getMainObjectPart().icnIndex;

        const int icnId{ ICN::BOATSHAD };
        const uint32_t icnIndex = spriteIndex % 128;
        const fheroes2::Sprite & boatShadowSprite = fheroes2::AGG::GetICN( icnId, icnIndex );
        const fheroes2::Point boatShadowSpriteOffset( boatShadowSprite.x(), fheroes2::tileWidthPx + boatShadowSprite.y() - 11 );

        // Shadows cannot be flipped so flip flag is always false.
        std::vector<fheroes2::Point> outputSquareInfo;
        std::vector<std::pair<fheroes2::Point, fheroes2::Rect>> outputImageInfo;
        fheroes2::DivideImageBySquares( boatShadowSpriteOffset, boatShadowSprite, fheroes2::tileWidthPx, outputSquareInfo, outputImageInfo );

        assert( outputSquareInfo.size() == outputImageInfo.size() );

        std::vector<fheroes2::ObjectRenderingInfo> objectInfo;
        for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
            objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, icnIndex, false, static_cast<uint8_t>( 255 ) );
        }

        return objectInfo;
    }

    std::vector<fheroes2::ObjectRenderingInfo> getMineGuardianSpritesPerTile( const Tile & tile )
    {
        assert( tile.getMainObjectType( false ) == MP2::OBJ_MINE );

        std::vector<fheroes2::ObjectRenderingInfo> objectInfo;

        const int32_t spellID = Maps::getMineSpellIdFromTile( tile );
        switch ( spellID ) {
        case Spell::SETEGUARDIAN:
        case Spell::SETAGUARDIAN:
        case Spell::SETFGUARDIAN:
        case Spell::SETWGUARDIAN: {
            static_assert( Spell::SETAGUARDIAN - Spell::SETEGUARDIAN == 1 && Spell::SETFGUARDIAN - Spell::SETEGUARDIAN == 2
                               && Spell::SETWGUARDIAN - Spell::SETEGUARDIAN == 3,
                           "Why are you changing the order of spells?! Be extremely careful of what you are doing" );

            const int icnId{ ICN::OBJNXTRA };
            const uint32_t icnIndex = spellID - Spell::SETEGUARDIAN;
            const fheroes2::Sprite & image = fheroes2::AGG::GetICN( icnId, icnIndex );

            std::vector<fheroes2::Point> outputSquareInfo;
            std::vector<std::pair<fheroes2::Point, fheroes2::Rect>> outputImageInfo;
            fheroes2::DivideImageBySquares( { image.x(), image.y() }, image, fheroes2::tileWidthPx, outputSquareInfo, outputImageInfo );

            assert( outputSquareInfo.size() == outputImageInfo.size() );

            for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
                objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, icnIndex, false, static_cast<uint8_t>( 255 ) );
            }
            break;
        }
        default:
            break;
        }

        return objectInfo;
    }

    std::vector<fheroes2::ObjectRenderingInfo> getHeroSpritesPerTile( const Heroes & hero )
    {
        // Reflected hero sprite should be shifted by 1 pixel to right.
        const bool reflect = doesHeroImageNeedToBeReflected( hero.GetDirection() );

        int flagFrameID = hero.GetSpriteIndex();
        if ( !hero.isMoveEnabled() ) {
            flagFrameID = hero.isShipMaster() ? 0 : static_cast<int>( Game::getAdventureMapAnimationIndex() );
        }

        fheroes2::Point offset;
        // Boat sprite has to be shifted so it matches other boats.
        if ( hero.isShipMaster() ) {
            offset.y -= 11;
        }
        else {
            offset.y -= 1;
        }

        // Apply hero offset when he moves from one tile to another.
        offset += hero.getCurrentPixelOffset();

        int icnId{ 0 };
        uint32_t icnIndex{ 0 };
        getHeroSpriteInfo( hero, hero.GetSpriteIndex(), false, icnId, icnIndex );

        const fheroes2::Sprite & spriteHero = fheroes2::AGG::GetICN( icnId, icnIndex );
        const fheroes2::Point heroSpriteOffset( offset.x + ( reflect ? ( fheroes2::tileWidthPx + 1 - spriteHero.x() - spriteHero.width() ) : spriteHero.x() ),
                                                offset.y + spriteHero.y() + fheroes2::tileWidthPx );

        std::vector<fheroes2::Point> outputSquareInfo;
        std::vector<std::pair<fheroes2::Point, fheroes2::Rect>> outputImageInfo;
        fheroes2::DivideImageBySquares( heroSpriteOffset, spriteHero, fheroes2::tileWidthPx, outputSquareInfo, outputImageInfo );

        assert( outputSquareInfo.size() == outputImageInfo.size() );

        std::vector<fheroes2::ObjectRenderingInfo> objectInfo;
        for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
            objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, icnIndex, reflect, static_cast<uint8_t>( 255 ) );
        }

        outputSquareInfo.clear();
        outputImageInfo.clear();

        fheroes2::Point flagOffset;
        getFlagSpriteInfo( hero, flagFrameID, false, flagOffset, icnId, icnIndex );

        const fheroes2::Sprite & spriteFlag = fheroes2::AGG::GetICN( icnId, icnIndex );
        const fheroes2::Point flagSpriteOffset( offset.x
                                                    + ( reflect ? ( fheroes2::tileWidthPx - spriteFlag.x() - flagOffset.x - spriteFlag.width() )
                                                                : spriteFlag.x() + flagOffset.x ),
                                                offset.y + spriteFlag.y() + flagOffset.y + fheroes2::tileWidthPx );

        fheroes2::DivideImageBySquares( flagSpriteOffset, spriteFlag, fheroes2::tileWidthPx, outputSquareInfo, outputImageInfo );

        assert( outputSquareInfo.size() == outputImageInfo.size() );

        for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
            objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, icnIndex, reflect, static_cast<uint8_t>( 255 ) );
        }

        outputSquareInfo.clear();
        outputImageInfo.clear();

        if ( hero.isShipMaster() && hero.isMoveEnabled() && hero.isInDeepOcean() ) {
            // TODO: draw froth for all boats in deep water, not only for a moving boat.
            getFrothSpriteInfo( hero, hero.GetSpriteIndex(), icnId, icnIndex );
            const fheroes2::Sprite & spriteFroth = fheroes2::AGG::GetICN( icnId, icnIndex );
            const fheroes2::Point frothSpriteOffset( offset.x + ( reflect ? fheroes2::tileWidthPx - spriteFroth.x() - spriteFroth.width() : spriteFroth.x() ),
                                                     offset.y + spriteFroth.y() + fheroes2::tileWidthPx );

            fheroes2::DivideImageBySquares( frothSpriteOffset, spriteFroth, fheroes2::tileWidthPx, outputSquareInfo, outputImageInfo );

            for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
                objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, icnIndex, reflect,
                                         static_cast<uint8_t>( 255 ) );
            }
        }

        return objectInfo;
    }

    std::vector<fheroes2::ObjectRenderingInfo> getHeroShadowSpritesPerTile( const Heroes & hero )
    {
        fheroes2::Point offset;
        // Boat sprite has to be shifted so it matches other boats.
        if ( hero.isShipMaster() ) {
            offset.y -= 11;
        }

        // Apply hero offset when he moves from one tile to another.
        offset += hero.getCurrentPixelOffset();

        int icnId{ 0 };
        uint32_t icnIndex{ 0 };
        getShadowSpriteInfo( hero, hero.GetSpriteIndex(), icnId, icnIndex );

        const fheroes2::Sprite & spriteShadow = fheroes2::AGG::GetICN( icnId, icnIndex );
        const fheroes2::Point shadowSpriteOffset( offset.x + spriteShadow.x(), offset.y + spriteShadow.y() + fheroes2::tileWidthPx );

        std::vector<fheroes2::Point> outputSquareInfo;
        std::vector<std::pair<fheroes2::Point, fheroes2::Rect>> outputImageInfo;
        fheroes2::DivideImageBySquares( shadowSpriteOffset, spriteShadow, fheroes2::tileWidthPx, outputSquareInfo, outputImageInfo );

        assert( outputSquareInfo.size() == outputImageInfo.size() );

        std::vector<fheroes2::ObjectRenderingInfo> objectInfo;
        for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
            objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, icnIndex, false, static_cast<uint8_t>( 255 ) );
        }

        return objectInfo;
    }

    std::vector<fheroes2::ObjectRenderingInfo> getEditorHeroSpritesPerTile( const Tile & tile )
    {
        assert( tile.getMainObjectType() == MP2::OBJ_HERO );

        const uint32_t icnIndex = tile.getMainObjectPart().icnIndex;
        const int icnId{ ICN::MINIHERO };

        const fheroes2::Sprite & boatSprite = fheroes2::AGG::GetICN( icnId, icnIndex );

        const fheroes2::Point boatSpriteOffset{ 0, 32 - 50 };

        std::vector<fheroes2::Point> outputSquareInfo;
        std::vector<std::pair<fheroes2::Point, fheroes2::Rect>> outputImageInfo;
        fheroes2::DivideImageBySquares( boatSpriteOffset, boatSprite, fheroes2::tileWidthPx, outputSquareInfo, outputImageInfo );

        assert( outputSquareInfo.size() == outputImageInfo.size() );

        std::vector<fheroes2::ObjectRenderingInfo> objectInfo;
        for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
            objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, icnIndex, false, static_cast<uint8_t>( 255 ) );
        }

        return objectInfo;
    }

    const fheroes2::Image & getTileSurface( const Tile & tile )
    {
        return fheroes2::AGG::GetTIL( TIL::GROUND32, tile.getTerrainImageIndex(), ( tile.getTerrainFlags() & 0x3 ) );
    }
}
