/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

#include "agg_image.h"
#include "game.h"
#include "icn.h"
#include "interface_gamearea.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "monster_anim.h"
#include "til.h"
#include "ui_object_rendering.h"
#include "world.h"

namespace
{
    bool contains( const int base, const int value )
    {
        return ( base & value ) == value;
    }

    bool isDirectRenderingRestricted( const int icnId )
    {
        switch ( icnId ) {
        case ICN::UNKNOWN:
        case ICN::MONS32:
        case ICN::BOAT32:
        case ICN::MINIHERO:
            // Either it is an invalid sprite or a sprite which needs to be divided into tiles in order to properly render it.
            return true;
        default:
            break;
        }

        return false;
    }

    void renderAddonObject( fheroes2::Image & output, const Interface::GameArea & area, const fheroes2::Point & offset, const Maps::TilesAddon & addon )
    {
        assert( addon._objectIcnType != MP2::OBJ_ICN_TYPE_UNKNOWN && addon._imageIndex != 255 );

        const int icn = MP2::getIcnIdFromObjectIcnType( addon._objectIcnType );
        if ( isDirectRenderingRestricted( icn ) ) {
            return;
        }

        const uint8_t alphaValue = area.getObjectAlphaValue( addon._uid );

        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, addon._imageIndex );

        // Ideally we need to check that the image is within a tile area. However, flags are among those for which this rule doesn't apply.
        if ( icn == ICN::FLAG32 ) {
            assert( sprite.width() <= TILEWIDTH && sprite.height() <= TILEWIDTH );
        }
        else {
            assert( sprite.x() >= 0 && sprite.width() + sprite.x() <= TILEWIDTH && sprite.y() >= 0 && sprite.height() + sprite.y() <= TILEWIDTH );
        }

        area.BlitOnTile( output, sprite, sprite.x(), sprite.y(), offset, false, alphaValue );

        const uint32_t animationIndex = ICN::AnimationFrame( icn, addon._imageIndex, Game::getAdventureMapAnimationIndex() );
        if ( animationIndex > 0 ) {
            const fheroes2::Sprite & animationSprite = fheroes2::AGG::GetICN( icn, animationIndex );

            // If this assertion blows up we are trying to render an image bigger than a tile. Render this object properly as heroes or monsters!
            assert( animationSprite.x() >= 0 && animationSprite.width() + animationSprite.x() <= TILEWIDTH && animationSprite.y() >= 0
                    && animationSprite.height() + animationSprite.y() <= TILEWIDTH );

            area.BlitOnTile( output, animationSprite, animationSprite.x(), animationSprite.y(), offset, false, alphaValue );
        }
    }

    void renderMainObject( fheroes2::Image & output, const Interface::GameArea & area, const fheroes2::Point & offset, const Maps::Tiles & tile )
    {
        assert( tile.getObjectIcnType() != MP2::OBJ_ICN_TYPE_UNKNOWN && tile.GetObjectSpriteIndex() != 255 );

        const int mainObjectIcn = MP2::getIcnIdFromObjectIcnType( tile.getObjectIcnType() );
        if ( isDirectRenderingRestricted( mainObjectIcn ) ) {
            return;
        }

        const uint8_t mainObjectAlphaValue = area.getObjectAlphaValue( tile.GetObjectUID() );

        const fheroes2::Sprite & mainObjectSprite = fheroes2::AGG::GetICN( mainObjectIcn, tile.GetObjectSpriteIndex() );

        // If this assertion blows up we are trying to render an image bigger than a tile. Render this object properly as heroes or monsters!
        assert( mainObjectSprite.x() >= 0 && mainObjectSprite.width() + mainObjectSprite.x() <= TILEWIDTH && mainObjectSprite.y() >= 0
                && mainObjectSprite.height() + mainObjectSprite.y() <= TILEWIDTH );

        area.BlitOnTile( output, mainObjectSprite, mainObjectSprite.x(), mainObjectSprite.y(), offset, false, mainObjectAlphaValue );

        // Render possible animation image.
        // TODO: quantity2 is used in absolutely incorrect way! Fix all the logic for it. As of now (quantity2 != 0) expression is used only for Magic Garden.
        const uint32_t mainObjectAnimationIndex
            = ICN::AnimationFrame( mainObjectIcn, tile.GetObjectSpriteIndex(), Game::getAdventureMapAnimationIndex(), tile.metadata()[1] != 0 );
        if ( mainObjectAnimationIndex > 0 ) {
            const fheroes2::Sprite & animationSprite = fheroes2::AGG::GetICN( mainObjectIcn, mainObjectAnimationIndex );

            // If this assertion blows up we are trying to render an image bigger than a tile. Render this object properly as heroes or monsters!
            assert( animationSprite.x() >= 0 && animationSprite.width() + animationSprite.x() <= TILEWIDTH && animationSprite.y() >= 0
                    && animationSprite.height() + animationSprite.y() <= TILEWIDTH );

            area.BlitOnTile( output, animationSprite, animationSprite.x(), animationSprite.y(), offset, false, mainObjectAlphaValue );
        }
    }

#ifdef WITH_DEBUG
    const fheroes2::Image & PassableViewSurface( const int passable )
    {
        static std::map<int, fheroes2::Image> imageMap;

        auto iter = imageMap.find( passable );
        if ( iter != imageMap.end() ) {
            return iter->second;
        }

        const int32_t size = 31;
        const uint8_t red = 0xBA;
        const uint8_t green = 0x5A;

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

        return imageMap.try_emplace( passable, std::move( sf ) ).first->second;
    }

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

    std::pair<uint32_t, uint32_t> GetMonsterSpriteIndices( const Maps::Tiles & tile, uint32_t monsterIndex )
    {
        const int tileIndex = tile.GetIndex();
        int attackerIndex = -1;

        // scan for a hero around
        for ( const int32_t idx : Maps::ScanAroundObject( tileIndex, MP2::OBJ_HEROES, false ) ) {
            const Heroes * hero = world.GetTiles( idx ).GetHeroes();
            assert( hero != nullptr );

            // hero is going to attack monsters on this tile
            if ( hero->GetAttackedMonsterTileIndex() == tileIndex ) {
                attackerIndex = idx;
                break;
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

    void redrawTopLayerExtraObjects( const Tiles & tile, fheroes2::Image & dst, const bool isPuzzleDraw, const Interface::GameArea & area )
    {
        if ( isPuzzleDraw ) {
            // Extra objects should not be shown on Puzzle Map as they are temporary objects appearing under specific conditions like flags.
            return;
        }

        // Ghost animation is unique and can be rendered in multiple cases.
        bool renderFlyingGhosts = false;

        const MP2::MapObjectType objectType = tile.GetObject( false );
        if ( objectType == MP2::OBJ_ABANDONED_MINE ) {
            renderFlyingGhosts = true;
        }
        else if ( objectType == MP2::OBJ_MINES ) {
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
            // This sprite is bigger than TILEWIDTH but rendering is correct for heroes and boats.
            // TODO: consider adding this sprite as a part of an addon.
            const fheroes2::Sprite & image = fheroes2::AGG::GetICN( ICN::OBJNHAUN, Game::getAdventureMapAnimationIndex() % 15 );

            const uint8_t alphaValue = area.getObjectAlphaValue( tile.GetObjectUID() );

            area.BlitOnTile( dst, image, image.x(), image.y(), Maps::GetPoint( tile.GetIndex() ), false, alphaValue );
        }
    }

    void redrawTopLayerObject( const Tiles & tile, fheroes2::Image & dst, const bool isPuzzleDraw, const Interface::GameArea & area, const TilesAddon & addon )
    {
        if ( isPuzzleDraw && MP2::isHiddenForPuzzle( tile.GetGround(), addon._objectIcnType, addon._imageIndex ) ) {
            return;
        }

        renderAddonObject( dst, area, Maps::GetPoint( tile.GetIndex() ), addon );
    }

    void drawFog( const Tiles & tile, fheroes2::Image & dst, const Interface::GameArea & area )
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
                index = 22;
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
                      && !( fogDirection & ( Direction::TOP | Direction::TOP_LEFT | Direction::LEFT | Direction::BOTTOM_RIGHT ) ) ) {
                index = 27;
                revert = true;
            }
            else if ( contains( fogDirection, Direction::LEFT | Direction::TOP )
                      && !( fogDirection & ( Direction::TOP_LEFT | Direction::RIGHT | Direction::BOTTOM | Direction::BOTTOM_RIGHT ) ) ) {
                index = 28;
            }
            else if ( contains( fogDirection, Direction::RIGHT | Direction::TOP )
                      && !( fogDirection & ( Direction::TOP_RIGHT | Direction::LEFT | Direction::BOTTOM | Direction::BOTTOM_LEFT ) ) ) {
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
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid direction for fog: " << fogDirection << ". Tile index: " << tile.GetIndex() )
                const fheroes2::Image & sf = fheroes2::AGG::GetTIL( TIL::CLOF32, ( mp.x + mp.y ) % 4, 0 );
                area.DrawTile( dst, sf, mp );
                return;
            }

            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::CLOP32, index );
            area.BlitOnTile( dst, sprite, ( revert ? TILEWIDTH - sprite.x() - sprite.width() : sprite.x() ), sprite.y(), mp, revert, 255 );
        }
    }

    void redrawPassable( const Tiles & tile, fheroes2::Image & dst, const int friendColors, const Interface::GameArea & area )
    {
#ifdef WITH_DEBUG
        if ( tile.isFog( friendColors ) ) {
            area.BlitOnTile( dst, getDebugFogImage(), 0, 0, Maps::GetPoint( tile.GetIndex() ), false, 255 );
        }
        if ( 0 == tile.GetPassable() || DIRECTION_ALL != tile.GetPassable() ) {
            area.BlitOnTile( dst, PassableViewSurface( tile.GetPassable() ), 0, 0, Maps::GetPoint( tile.GetIndex() ), false, 255 );
        }
#else
        (void)dst;
        (void)area;
        (void)friendColors;
#endif
    }

    void redrawBottomLayerObjects( const Tiles & tile, fheroes2::Image & dst, bool isPuzzleDraw, const Interface::GameArea & area, const uint8_t level )
    {
        assert( level <= 0x03 );

        const fheroes2::Point & mp = Maps::GetPoint( tile.GetIndex() );

        // Since the original game stores information about objects in a very weird way and this is how it is implemented for us we need to do the following procedure:
        // - run through all bottom objects first which are stored in the addon stack
        // - check the main object which is on the tile

        // Some addons must be rendered after the main object on the tile. This applies for flags.
        // Since this method is called intensively during rendering we have to avoid memory allocation on heap.
        const size_t maxPostRenderAddons = 16;
        std::array<const TilesAddon *, maxPostRenderAddons> postRenderingAddon{};
        size_t postRenderAddonCount = 0;

        for ( const TilesAddon & addon : tile.getLevel1Addons() ) {
            if ( ( addon._layerType & 0x03 ) != level ) {
                continue;
            }

            if ( isPuzzleDraw && MP2::isHiddenForPuzzle( tile.GetGround(), addon._objectIcnType, addon._imageIndex ) ) {
                continue;
            }

            if ( addon._objectIcnType == MP2::OBJ_ICN_TYPE_FLAG32 ) {
                // Based on logically thinking it is impossible to have more than 16 flags on a single tile.
                assert( postRenderAddonCount < maxPostRenderAddons );

                postRenderingAddon[postRenderAddonCount] = &addon;
                ++postRenderAddonCount;
                continue;
            }

            renderAddonObject( dst, area, mp, addon );
        }

        if ( tile.getObjectIcnType() != MP2::OBJ_ICN_TYPE_UNKNOWN && ( tile.getLayerType() & 0x03 ) == level
             && ( !isPuzzleDraw || !MP2::isHiddenForPuzzle( tile.GetGround(), tile.getObjectIcnType(), tile.GetObjectSpriteIndex() ) ) ) {
            renderMainObject( dst, area, mp, tile );
        }

        for ( size_t i = 0; i < postRenderAddonCount; ++i ) {
            assert( postRenderingAddon[i] != nullptr );

            renderAddonObject( dst, area, mp, *postRenderingAddon[i] );
        }
    }

    void drawByObjectIcnType( const Tiles & tile, fheroes2::Image & output, const Interface::GameArea & area, const MP2::ObjectIcnType objectIcnType )
    {
        const fheroes2::Point & tileOffset = Maps::GetPoint( tile.GetIndex() );

        for ( const TilesAddon & addon : tile.getLevel1Addons() ) {
            if ( addon._objectIcnType == objectIcnType ) {
                renderAddonObject( output, area, tileOffset, addon );
            }
        }

        if ( tile.getObjectIcnType() == objectIcnType ) {
            renderMainObject( output, area, tileOffset, tile );
        }

        for ( const TilesAddon & addon : tile.getLevel2Addons() ) {
            if ( addon._objectIcnType == objectIcnType ) {
                renderAddonObject( output, area, tileOffset, addon );
            }
        }
    }

    std::vector<fheroes2::ObjectRenderingInfo> getMonsterSpritesPerTile( const Tiles & tile )
    {
        assert( tile.GetObject() == MP2::OBJ_MONSTER );

        const Monster & monster = getMonsterFromTile( tile );
        const std::pair<uint32_t, uint32_t> spriteIndices = GetMonsterSpriteIndices( tile, monster.GetSpriteIndex() );

        const int icnId{ ICN::MINI_MONSTER_IMAGE };
        const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( icnId, spriteIndices.first );
        const fheroes2::Point monsterSpriteOffset( monsterSprite.x() + 16, monsterSprite.y() + 30 );

        std::vector<fheroes2::Point> outputSquareInfo;
        std::vector<std::pair<fheroes2::Point, fheroes2::Rect>> outputImageInfo;
        fheroes2::DivideImageBySquares( monsterSpriteOffset, monsterSprite, TILEWIDTH, outputSquareInfo, outputImageInfo );

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
            const fheroes2::Point secondaryMonsterSpriteOffset( secondaryMonsterSprite.x() + 16, secondaryMonsterSprite.y() + 30 );

            fheroes2::DivideImageBySquares( secondaryMonsterSpriteOffset, secondaryMonsterSprite, TILEWIDTH, outputSquareInfo, outputImageInfo );

            assert( outputSquareInfo.size() == outputImageInfo.size() );

            for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
                objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, spriteIndices.second, false,
                                         static_cast<uint8_t>( 255 ) );
            }
        }

        return objectInfo;
    }

    std::vector<fheroes2::ObjectRenderingInfo> getMonsterShadowSpritesPerTile( const Tiles & tile )
    {
        assert( tile.GetObject() == MP2::OBJ_MONSTER );

        const Monster & monster = getMonsterFromTile( tile );
        const std::pair<uint32_t, uint32_t> spriteIndices = GetMonsterSpriteIndices( tile, monster.GetSpriteIndex() );

        const int icnId{ ICN::MINI_MONSTER_SHADOW };
        const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( icnId, spriteIndices.first );
        const fheroes2::Point monsterSpriteOffset( monsterSprite.x() + 16, monsterSprite.y() + 30 );

        std::vector<fheroes2::Point> outputSquareInfo;
        std::vector<std::pair<fheroes2::Point, fheroes2::Rect>> outputImageInfo;
        fheroes2::DivideImageBySquares( monsterSpriteOffset, monsterSprite, TILEWIDTH, outputSquareInfo, outputImageInfo );

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
            const fheroes2::Point secondaryMonsterSpriteOffset( secondaryMonsterSprite.x() + 16, secondaryMonsterSprite.y() + 30 );

            fheroes2::DivideImageBySquares( secondaryMonsterSpriteOffset, secondaryMonsterSprite, TILEWIDTH, outputSquareInfo, outputImageInfo );

            assert( outputSquareInfo.size() == outputImageInfo.size() );

            for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
                objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, spriteIndices.second, false,
                                         static_cast<uint8_t>( 255 ) );
            }
        }

        return objectInfo;
    }

    std::vector<fheroes2::ObjectRenderingInfo> getBoatSpritesPerTile( const Tiles & tile )
    {
        // TODO: combine both boat image generation for heroes and empty boats.
        assert( tile.GetObject() == MP2::OBJ_BOAT );

        const uint32_t spriteIndex = ( tile.GetObjectSpriteIndex() == 255 ) ? 18 : tile.GetObjectSpriteIndex();

        const bool isReflected = ( spriteIndex > 128 );

        const int icnId{ ICN::BOAT32 };
        const uint32_t icnIndex = spriteIndex % 128;
        const fheroes2::Sprite & boatSprite = fheroes2::AGG::GetICN( icnId, icnIndex );

        const fheroes2::Point boatSpriteOffset( ( isReflected ? ( TILEWIDTH + 1 - boatSprite.x() - boatSprite.width() ) : boatSprite.x() ),
                                                boatSprite.y() + TILEWIDTH - 11 );

        std::vector<fheroes2::Point> outputSquareInfo;
        std::vector<std::pair<fheroes2::Point, fheroes2::Rect>> outputImageInfo;
        fheroes2::DivideImageBySquares( boatSpriteOffset, boatSprite, TILEWIDTH, outputSquareInfo, outputImageInfo );

        assert( outputSquareInfo.size() == outputImageInfo.size() );

        std::vector<fheroes2::ObjectRenderingInfo> objectInfo;
        for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
            objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, icnIndex, isReflected,
                                     static_cast<uint8_t>( 255 ) );
        }

        return objectInfo;
    }

    std::vector<fheroes2::ObjectRenderingInfo> getBoatShadowSpritesPerTile( const Tiles & tile )
    {
        assert( tile.GetObject() == MP2::OBJ_BOAT );

        // TODO: boat shadow logic is more complex than this and it is not directly depend on spriteIndex. Find the proper logic and fix it!
        const uint32_t spriteIndex = ( tile.GetObjectSpriteIndex() == 255 ) ? 18 : tile.GetObjectSpriteIndex();

        const int icnId{ ICN::BOATSHAD };
        const uint32_t icnIndex = spriteIndex % 128;
        const fheroes2::Sprite & boatShadowSprite = fheroes2::AGG::GetICN( icnId, icnIndex );
        const fheroes2::Point boatShadowSpriteOffset( boatShadowSprite.x(), TILEWIDTH + boatShadowSprite.y() - 11 );

        // Shadows cannot be flipped so flip flag is always false.
        std::vector<fheroes2::Point> outputSquareInfo;
        std::vector<std::pair<fheroes2::Point, fheroes2::Rect>> outputImageInfo;
        fheroes2::DivideImageBySquares( boatShadowSpriteOffset, boatShadowSprite, TILEWIDTH, outputSquareInfo, outputImageInfo );

        assert( outputSquareInfo.size() == outputImageInfo.size() );

        std::vector<fheroes2::ObjectRenderingInfo> objectInfo;
        for ( size_t i = 0; i < outputSquareInfo.size(); ++i ) {
            objectInfo.emplace_back( outputSquareInfo[i], outputImageInfo[i].first, outputImageInfo[i].second, icnId, icnIndex, false, static_cast<uint8_t>( 255 ) );
        }

        return objectInfo;
    }

    std::vector<fheroes2::ObjectRenderingInfo> getMineGuardianSpritesPerTile( const Tiles & tile )
    {
        assert( tile.GetObject( false ) == MP2::OBJ_MINES );

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
            fheroes2::DivideImageBySquares( { image.x(), image.y() }, image, TILEWIDTH, outputSquareInfo, outputImageInfo );

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

    const fheroes2::Image & getTileSurface( const Tiles & tile )
    {
        return fheroes2::AGG::GetTIL( TIL::GROUND32, tile.getTerrainImageIndex(), ( tile.getTerrainFlags() & 0x3 ) );
    }
}
