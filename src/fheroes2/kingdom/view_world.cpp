/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2023                                             *
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
#include <memory>
#include <vector>

#include "agg_image.h"
#include "castle.h"
#include "color.h"
#include "cursor.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "gamedefs.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "interface_border.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "localevent.h"
#include "maps.h"
#include "maps_tiles.h"
#include "mp2.h"
#include "pairs.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "ui_button.h"
#include "view_world.h"
#include "world.h"

#include "logging.h"
// #define VIEWWORLD_DEBUG_ZOOM_LEVEL // Activate this when you want to debug this window. It will provide an extra zoom level at 1:1 scale

#if defined( VIEWWORLD_DEBUG_ZOOM_LEVEL )
#define SAVE_WORLD_MAP
#include "image_tool.h"

namespace
{
    const std::string saveFilePrefix = "_old";
}

#endif

namespace
{
    const std::vector<int32_t> tileSizePerZoomLevel{ 4, 6, 12, 32 };
    const std::vector<int32_t> icnPerZoomLevel{ ICN::MISC4, ICN::MISC6, ICN::MISC12, ICN::MISC12 };
    const std::vector<int32_t> icnLetterPerZoomLevel{ ICN::LETTER4, ICN::LETTER6, ICN::LETTER12, ICN::LETTER12 };
    const std::vector<int32_t> icnPerZoomLevelFlags{ ICN::VWFLAG4, ICN::VWFLAG6, ICN::VWFLAG12, ICN::VWFLAG12 };

#ifdef VIEWWORLD_DEBUG_ZOOM_LEVEL
    const int32_t zoomLevels = 4;
#else
    const int32_t zoomLevels = 3;
#endif

    // Compute a rectangle that defines which world pixels we can see in the "view world" window,
    // based on given zoom level and initial center
    fheroes2::Rect computeROI( const fheroes2::Point & centerInPixel, const ViewWorld::ZoomLevel zoomLevel )
    {
        const fheroes2::Rect sizeInPixels = Interface::Basic::Get().GetGameArea().GetROI();

        // how many pixels from "world map" we can see in "view world" window, given current zoom
        const int32_t pixelsW = sizeInPixels.width * TILEWIDTH / tileSizePerZoomLevel[zoomLevel];
        const int32_t pixelsH = sizeInPixels.height * TILEWIDTH / tileSizePerZoomLevel[zoomLevel];

        const int32_t x = centerInPixel.x - pixelsW / 2;
        const int32_t y = centerInPixel.y - pixelsH / 2;

        return { x, y, pixelsW, pixelsH };
    }

    ViewWorld::ZoomLevel GetNextZoomLevel( const ViewWorld::ZoomLevel level, const bool cycle )
    {
        switch ( level ) {
        case ViewWorld::ZoomLevel::ZoomLevel0:
            return ViewWorld::ZoomLevel::ZoomLevel1;
        case ViewWorld::ZoomLevel::ZoomLevel1:
            return ViewWorld::ZoomLevel::ZoomLevel2;
#ifdef VIEWWORLD_DEBUG_ZOOM_LEVEL
        case ViewWorld::ZoomLevel::ZoomLevel2:
            return ViewWorld::ZoomLevel::ZoomLevel3;
        default:
            return cycle ? ViewWorld::ZoomLevel::ZoomLevel0 : ViewWorld::ZoomLevel::ZoomLevel3;
#else
        default:
            return cycle ? ViewWorld::ZoomLevel::ZoomLevel0 : ViewWorld::ZoomLevel::ZoomLevel2;
#endif
        }
    }

    ViewWorld::ZoomLevel GetPreviousZoomLevel( const ViewWorld::ZoomLevel level, const bool cycle )
    {
        switch ( level ) {
#ifdef VIEWWORLD_DEBUG_ZOOM_LEVEL
        case ViewWorld::ZoomLevel::ZoomLevel0:
            return cycle ? ViewWorld::ZoomLevel::ZoomLevel3 : ViewWorld::ZoomLevel::ZoomLevel0;
#else
        case ViewWorld::ZoomLevel::ZoomLevel0:
            return cycle ? ViewWorld::ZoomLevel::ZoomLevel2 : ViewWorld::ZoomLevel::ZoomLevel0;
#endif
        case ViewWorld::ZoomLevel::ZoomLevel1:
            return ViewWorld::ZoomLevel::ZoomLevel0;
        case ViewWorld::ZoomLevel::ZoomLevel2:
            return ViewWorld::ZoomLevel::ZoomLevel1;
        default:
            return ViewWorld::ZoomLevel::ZoomLevel2;
        }
    }

    int32_t colorToOffsetICN( const int32_t color )
    {
        switch ( color ) {
        case Color::BLUE:
            return 0;
        case Color::GREEN:
            return 1;
        case Color::RED:
            return 2;
        case Color::YELLOW:
            return 3;
        case Color::ORANGE:
            return 4;
        case Color::PURPLE:
            return 5;
        case Color::NONE:
        case Color::UNUSED:
            return 6;
        default:
            return -1;
        }
    }

    struct CacheForMapWithResources
    {
        std::vector<fheroes2::Image> cachedImages; // One image per zoom Level

        CacheForMapWithResources() = delete;

        // Compute complete world map, and save it for all zoom levels
        explicit CacheForMapWithResources( const ViewWorldMode viewMode )
        {
            fheroes2::Time counter;
            counter.reset();

            cachedImages.resize( zoomLevels );

            for ( size_t i = 0; i < cachedImages.size(); ++i ) {
                cachedImages[i].resize( world.w() * tileSizePerZoomLevel[i], world.h() * tileSizePerZoomLevel[i] );
                cachedImages[i]._disableTransformLayer();
            }

            const int32_t blockSizeX = 18;
            const int32_t blockSizeY = 18;

            const int32_t worldWidth = world.w();
            const int32_t worldHeight = world.h();

            // Assert will fail in case we add non-standard map sizes, otherwise standard map sizes are multiples of 18 tiles
            assert( worldWidth % blockSizeX == 0 );
            assert( worldHeight % blockSizeY == 0 );

            const int32_t redrawAreaWidth = blockSizeX * TILEWIDTH;
            const int32_t redrawAreaHeight = blockSizeY * TILEWIDTH;
            const int32_t redrawAreaCenterX = blockSizeX * TILEWIDTH / 2;
            const int32_t redrawAreaCenterY = blockSizeY * TILEWIDTH / 2;

            std::vector<int32_t> blockSizeResizedX( zoomLevels );
            std::vector<int32_t> blockSizeResizedY( zoomLevels );
            for ( size_t i = 0; i < zoomLevels; ++i ) {
                blockSizeResizedX[i] = blockSizeX * tileSizePerZoomLevel[i];
                blockSizeResizedY[i] = blockSizeY * tileSizePerZoomLevel[i];
            }

            // Create temporary image where we will draw blocks of the main map on
            fheroes2::Image temporaryImg( redrawAreaWidth, redrawAreaHeight );
            temporaryImg._disableTransformLayer();

            Interface::GameArea gamearea = Interface::Basic::Get().GetGameArea();
            gamearea.SetAreaPosition( 0, 0, redrawAreaWidth, redrawAreaHeight );

            int32_t drawingFlags = Interface::RedrawLevelType::LEVEL_ALL & ~Interface::RedrawLevelType::LEVEL_ROUTES;
            if ( viewMode == ViewWorldMode::ViewAll ) {
                drawingFlags &= ~Interface::RedrawLevelType::LEVEL_FOG;
            }
            else if ( viewMode == ViewWorldMode::ViewTowns ) {
                drawingFlags |= Interface::RedrawLevelType::LEVEL_TOWNS;
            }

#if !defined( SAVE_WORLD_MAP )
            drawingFlags ^= Interface::RedrawLevelType::LEVEL_HEROES;
#endif

            double tmp = counter.getS() * 1000;
            VERBOSE_LOG( "Pre draw time: " << tmp << " ms." )
            counter.reset();

            // Draw sub-blocks of the main map, and resize them to draw them on lower-res cached versions:
            for ( int32_t x = 0; x < worldWidth; x += blockSizeX ) {
                for ( int32_t y = 0; y < worldHeight; y += blockSizeY ) {
                    gamearea.SetCenterInPixels( { x * TILEWIDTH + redrawAreaCenterX, y * TILEWIDTH + redrawAreaCenterY } );
                    gamearea.Redraw( temporaryImg, drawingFlags );

                    for ( size_t i = 0; i < zoomLevels; ++i ) {
                        fheroes2::Resize( temporaryImg, 0, 0, temporaryImg.width(), temporaryImg.height(), cachedImages[i], x * tileSizePerZoomLevel[i],
                                          y * tileSizePerZoomLevel[i], blockSizeResizedX[i], blockSizeResizedY[i] );
                    }
                }
            }

#if defined( SAVE_WORLD_MAP )
            fheroes2::Save( cachedImages[3], Settings::Get().MapsName() + saveFilePrefix + ".bmp" );
#endif
            tmp = counter.getS() * 1000;
            VERBOSE_LOG( "Draw time: " << tmp << " ms." )
        }
    };

    void DrawWorld( const ViewWorld::ZoomROIs & ROI, CacheForMapWithResources & cache )
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Image & image = cache.cachedImages[ROI._zoomLevel];

        const fheroes2::Rect roiScreen = Interface::Basic::Get().GetGameArea().GetROI();

        const int32_t offsetPixelsX = tileSizePerZoomLevel[ROI._zoomLevel] * ROI.GetROIinPixels().x / TILEWIDTH;
        const int32_t offsetPixelsY = tileSizePerZoomLevel[ROI._zoomLevel] * ROI.GetROIinPixels().y / TILEWIDTH;

        const fheroes2::Point inPos( offsetPixelsX < 0 ? 0 : offsetPixelsX, offsetPixelsY < 0 ? 0 : offsetPixelsY );
        const fheroes2::Point outPos( BORDERWIDTH + ( offsetPixelsX < 0 ? -offsetPixelsX : 0 ), BORDERWIDTH + ( offsetPixelsY < 0 ? -offsetPixelsY : 0 ) );
        const fheroes2::Size outSize( roiScreen.width, roiScreen.height );

        fheroes2::Copy( image, inPos.x, inPos.y, display, outPos.x, outPos.y, outSize.width, outSize.height );

        // Fill black pixels outside of the main view.
        auto fillBlack = [&display]( const int32_t x, const int32_t y, const int32_t width, const int32_t height ) {
            const int32_t displayWidth = display.width();

            assert( ( width > 0 ) && ( height > 0 ) && ( x >= 0 ) && ( y >= 0 ) && ( ( x + width ) < displayWidth ) && ( ( y + height ) < display.height() ) );

            uint8_t * imageY = display.image() + static_cast<ptrdiff_t>( y ) * displayWidth + x;
            const uint8_t * imageYEnd = imageY + static_cast<ptrdiff_t>( height ) * displayWidth;

            for ( ; imageY != imageYEnd; imageY += displayWidth ) {
                std::fill( imageY, imageY + width, static_cast<uint8_t>( 0 ) );
            }
        };

        if ( image.width() < roiScreen.width ) {
            // Left black bar.
            fillBlack( BORDERWIDTH, BORDERWIDTH, outPos.x - BORDERWIDTH, outSize.height );
            // Right black bar.
            fillBlack( BORDERWIDTH - offsetPixelsX + image.width(), BORDERWIDTH, display.width() - 3 * BORDERWIDTH - RADARWIDTH + offsetPixelsX - image.width(),
                       outSize.height );
        }

        if ( image.height() < roiScreen.height ) {
            // Top black bar.
            fillBlack( BORDERWIDTH, BORDERWIDTH, display.width() - 3 * BORDERWIDTH - RADARWIDTH, outPos.y - BORDERWIDTH );
            // Bottom black bar.
            fillBlack( BORDERWIDTH, BORDERWIDTH - offsetPixelsY + image.height(), outSize.width, display.height() - 2 * BORDERWIDTH + offsetPixelsY - image.height() );
        }
    }

    void DrawObjectsIcons( const int32_t color, const ViewWorldMode mode, const ViewWorld::ZoomROIs & ROI )
    {
        fheroes2::Time counter;
        counter.reset();

        const bool revealAll = mode == ViewWorldMode::ViewAll;
        const bool revealMines = revealAll || ( mode == ViewWorldMode::ViewMines );
        const bool revealHeroes = revealAll || ( mode == ViewWorldMode::ViewHeroes );
        const bool revealTowns = revealAll || ( mode == ViewWorldMode::ViewTowns );
        const bool revealArtifacts = revealAll || ( mode == ViewWorldMode::ViewArtifacts );
        const bool revealResources = revealAll || ( mode == ViewWorldMode::ViewResources );

        const int32_t zoomLevelId = ROI._zoomLevel;
        const int32_t tileSize = tileSizePerZoomLevel[zoomLevelId];
        const int32_t icnBase = icnPerZoomLevel[zoomLevelId];
        const int32_t icnLetterId = icnLetterPerZoomLevel[zoomLevelId];
        const int32_t icnFlagsBase = icnPerZoomLevelFlags[zoomLevelId];

        fheroes2::Display & display = fheroes2::Display::instance();

        const int32_t worldWidth = world.w();
        const int32_t worldHeight = world.h();

        const fheroes2::Rect & roiPixels = ROI.GetROIinPixels();

        const int32_t offsetX = roiPixels.x * tileSize / TILEWIDTH - BORDERWIDTH;
        const int32_t offsetY = roiPixels.y * tileSize / TILEWIDTH - BORDERWIDTH;

        const fheroes2::Rect roiTiles = ROI.GetROIinTiles();

        // add a margin of 2 tiles because icons outside of view can still show on the view
        assert( worldWidth >= 0 && worldHeight >= 0 );
        const int32_t minTileX = std::clamp( roiTiles.x - 2, 0, worldWidth );
        const int32_t maxTileX = std::clamp( roiTiles.x + roiTiles.width + 2, 0, worldWidth );
        const int32_t minTileY = std::clamp( roiTiles.y - 2, 0, worldHeight );
        const int32_t maxTileY = std::clamp( roiTiles.y + roiTiles.height + 2, 0, worldHeight );

        // Initialize variables before the loops to avoid many memory allocations inside the loop.
        int32_t index{ -1 };
        uint32_t letterIndex{ 0 };
        // There could be maximum 2 objects.
        std::vector<MP2::MapObjectType> objectTypes( 2 );
        uint32_t objects{ 0 };
        bool isCastle{ false };

        for ( int32_t posY = minTileY; posY < maxTileY; ++posY ) {
            for ( int32_t posX = minTileX; posX < maxTileX; ++posX ) {
                const Maps::Tiles & tile = world.GetTiles( posX, posY );

                objectTypes[0] = tile.GetObject( false );

                if ( objectTypes[0] == MP2::OBJ_NONE ) {
                    continue;
                }

                if ( objectTypes[0] != tile.GetObject( true ) ) {
                    objectTypes[1] = tile.GetObject( true );
                    objects = 2;
                }
                else {
                    objects = 1;
                }

                for ( uint32_t i = 0; i < objects; ++i ) {
                    switch ( objectTypes[i] ) {
                    case MP2::OBJ_HEROES: {
                        if ( revealHeroes || !tile.isFog( color ) ) {
                            const Heroes * hero = world.GetHeroes( tile.GetCenter() );
                            if ( hero ) {
                                const int32_t colorOffset = colorToOffsetICN( hero->GetColor() );
                                index = colorOffset >= 0 ? 7 + colorOffset : -1;
                            }
                        }
                        break;
                    }

                    case MP2::OBJ_CASTLE: {
                        if ( revealTowns || !tile.isFog( color ) ) {
                            const Castle * castle = world.getCastleEntrance( tile.GetCenter() );
                            if ( castle ) {
                                isCastle = true;
                                index = colorToOffsetICN( castle->GetColor() );
                            }
                        }
                        break;
                    }

                    case MP2::OBJ_ALCHEMIST_LAB:
                    case MP2::OBJ_MINES:
                    case MP2::OBJ_SAWMILL:
                        if ( revealMines || !tile.isFog( color ) ) {
                            index = colorToOffsetICN( tile.QuantityColor() );
                            letterIndex = tile.QuantityResourceCount().first;
                        }
                        break;

                    case MP2::OBJ_ARTIFACT:
                        if ( revealArtifacts || !tile.isFog( color ) ) {
                            index = 14;
                        }
                        break;

                    case MP2::OBJ_RESOURCE:
                        if ( revealResources || !tile.isFog( color ) ) {
                            index = 13;
                            letterIndex = tile.GetQuantity1();
                        }
                        break;

                    default:
                        continue;
                    }

                    if ( index >= 0 ) {
                        // Castle flag needs an extra horozontal shift and a second flipped flag to render.
                        if ( isCastle ) {
                            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icnFlagsBase, index );
                            const int32_t dstx = posX * tileSize - offsetX + ( tileSize - sprite.width() ) / 2;
                            const int32_t dsty = posY * tileSize - offsetY + ( tileSize - sprite.height() ) / 2 + 1;

                            fheroes2::Blit( sprite, display, dstx + tileSize, dsty, false );
                            // We place a second flag, flipped horizontally.
                            fheroes2::Blit( sprite, display, dstx - tileSize, dsty, true );

                            isCastle = false;
                        }
                        else {
                            const int32_t dstx = posX * tileSize + tileSize / 2 - offsetX;
                            const int32_t dsty = posY * tileSize + tileSize / 2 - offsetY;

                            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icnBase, index );
                            fheroes2::Blit( sprite, display, dstx - sprite.width() / 2, dsty - sprite.height() / 2 );

                            if ( letterIndex > 0 ) {
                                switch ( letterIndex ) {
                                case Resource::WOOD:
                                    letterIndex = 0;
                                    break;
                                case Resource::MERCURY:
                                    letterIndex = 1;
                                    break;
                                case Resource::ORE:
                                    letterIndex = 2;
                                    break;
                                case Resource::SULFUR:
                                    letterIndex = 3;
                                    break;
                                case Resource::CRYSTAL:
                                    letterIndex = 4;
                                    break;
                                case Resource::GEMS:
                                    letterIndex = 5;
                                    break;
                                case Resource::GOLD:
                                    letterIndex = 6;
                                    break;
                                default:
                                    break;
                                }

                                const fheroes2::Sprite & letter = fheroes2::AGG::GetICN( icnLetterId, letterIndex );
                                fheroes2::Blit( letter, display, dstx - letter.width() / 2, dsty - letter.height() / 2 );

                                // Restore 'letterIndex' to its initial state.
                                letterIndex = 0;
                            }
                        }
                        // Restore 'index' to its initial state.
                        index = -1;
                    }
                }
            }
        }

        const double tmp = counter.getS() * 1000;
        VERBOSE_LOG( "Time: " << tmp << " ms." )
    }

    int32_t GetSpriteResource( const ViewWorldMode mode, const bool evil )
    {
        switch ( mode ) {
        case ViewWorldMode::ViewAll:
            return evil ? ICN::EVIW_ALL : ICN::VIEW_ALL;
        case ViewWorldMode::ViewArtifacts:
            return evil ? ICN::EVIWRTFX : ICN::VIEWRTFX;
        case ViewWorldMode::ViewMines:
            return evil ? ICN::EVIWMINE : ICN::VIEWMINE;
        case ViewWorldMode::ViewResources:
            return evil ? ICN::EVIWRSRC : ICN::VIEWRSRC;
        case ViewWorldMode::ViewHeroes:
            return evil ? ICN::EVIWHROS : ICN::VIEWHROS;
        case ViewWorldMode::ViewTowns:
            return evil ? ICN::EVIWTWNS : ICN::VIEWTWNS;
        default: // "View World"
            return evil ? ICN::EVIWWRLD : ICN::VIEWWRLD;
        }
    }

    void drawViewWorldSprite( const fheroes2::Sprite & viewWorldSprite, fheroes2::Display & display, const bool isEvilInterface )
    {
        const int32_t dstX = display.width() - viewWorldSprite.width() - BORDERWIDTH;
        int32_t dstY = 2 * BORDERWIDTH + RADARWIDTH;
        const int32_t cutHeight = 275;
        fheroes2::Copy( viewWorldSprite, 0, 0, display, dstX, dstY, viewWorldSprite.width(), cutHeight );
        dstY += cutHeight;

        if ( display.height() > fheroes2::Display::DEFAULT_HEIGHT ) {
            const fheroes2::Sprite & icnston = fheroes2::AGG::GetICN( isEvilInterface ? ICN::STONBAKE : ICN::STONBACK, 0 );
            const int32_t startY = 11;
            const int32_t copyHeight = 46;
            const int32_t repeatHeight = display.height() - BORDERWIDTH - dstY - ( viewWorldSprite.height() - cutHeight );
            const int32_t repeatCount = repeatHeight / copyHeight;
            for ( int32_t i = 0; i < repeatCount; ++i ) {
                fheroes2::Copy( icnston, 0, startY, display, dstX, dstY, icnston.width(), copyHeight );
                dstY += copyHeight;
            }
            fheroes2::Copy( icnston, 0, startY, display, dstX, dstY, icnston.width(), repeatHeight % copyHeight );
            dstY += repeatHeight % copyHeight;
        }

        fheroes2::Copy( viewWorldSprite, 0, cutHeight, display, dstX, dstY, viewWorldSprite.width(), viewWorldSprite.height() - cutHeight );
    }
}

ViewWorld::ZoomROIs::ZoomROIs( const ViewWorld::ZoomLevel zoomLevel, const fheroes2::Point & centerInPixels )
    : _zoomLevel( zoomLevel )
    , _center( centerInPixels )
{
    _updateZoomLevels();
    _updateCenter();
}

void ViewWorld::ZoomROIs::_updateZoomLevels()
{
    for ( int32_t i = 0; i < zoomLevels; ++i ) {
        _roiForZoomLevels[i] = computeROI( _center, static_cast<ViewWorld::ZoomLevel>( i ) );
    }
}

bool ViewWorld::ZoomROIs::_updateCenter()
{
    return changeCenter( _center );
}

bool ViewWorld::ZoomROIs::changeCenter( const fheroes2::Point & centerInPixels )
{
    const fheroes2::Rect & currentRect = GetROIinPixels();
    const fheroes2::Size worldSize( world.w() * TILEWIDTH, world.h() * TILEWIDTH );
    fheroes2::Point newCenter;

    if ( worldSize.width <= currentRect.width ) {
        newCenter.x = worldSize.width / 2;
    }
    else {
        newCenter.x = std::clamp( centerInPixels.x, currentRect.width / 2, worldSize.width - currentRect.width / 2 );
    }

    if ( worldSize.height <= currentRect.height ) {
        newCenter.y = worldSize.height / 2;
    }
    else {
        newCenter.y = std::clamp( centerInPixels.y, currentRect.height / 2, worldSize.height - currentRect.height / 2 );
    }

    if ( newCenter == _center ) {
        return false;
    }
    _center = newCenter;
    _updateZoomLevels();
    return true;
}

bool ViewWorld::ZoomROIs::_changeZoom( const ZoomLevel newLevel )
{
    const bool changed = ( newLevel != _zoomLevel );
    _zoomLevel = newLevel;
    _updateCenter();
    return changed;
}

bool ViewWorld::ZoomROIs::zoomIn( const bool cycle )
{
    const ZoomLevel newLevel = GetNextZoomLevel( _zoomLevel, cycle );
    return _changeZoom( newLevel );
}

bool ViewWorld::ZoomROIs::zoomOut( const bool cycle )
{
    const ZoomLevel newLevel = GetPreviousZoomLevel( _zoomLevel, cycle );
    return _changeZoom( newLevel );
}

const fheroes2::Rect & ViewWorld::ZoomROIs::GetROIinPixels() const
{
    return _roiForZoomLevels[_zoomLevel];
}

fheroes2::Rect ViewWorld::ZoomROIs::GetROIinTiles() const
{
    fheroes2::Rect result = _roiForZoomLevels[_zoomLevel];
    result.x = result.x / TILEWIDTH;
    result.y = result.y / TILEWIDTH;
    result.width = ( result.width + TILEWIDTH - 1 ) / TILEWIDTH;
    result.height = ( result.height + TILEWIDTH - 1 ) / TILEWIDTH;
    return result;
}

void ViewWorld::ViewWorldWindow( const int32_t color, const ViewWorldMode mode, Interface::Basic & interface )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::ImageRestorer restorer( display );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    LocalEvent::PauseCycling();

    Settings & conf = Settings::Get();
    const bool isEvilInterface = conf.isEvilInterfaceEnabled();
    const bool isHideInterface = conf.isHideInterfaceEnabled();

    // If the interface is currently hidden, we have to temporarily bring it back, because
    // the map generation in the World View mode heavily depends on the existing game area
    if ( isHideInterface ) {
        conf.setHideInterface( false );
        interface.Reset();
    }

    // Creates fixed radar on top-right, suitable for the View World window
    Interface::Radar radar( interface.GetRadar(), fheroes2::Display::instance() );

    const Interface::GameArea gameArea = interface.GetGameArea();
    const fheroes2::Rect worldMapROI = gameArea.GetVisibleTileROI();
    const fheroes2::Rect visibleScreenInPixels = gameArea.GetROI();

    // Initial view is centered on where the player is centered
    fheroes2::Point viewCenterInPixels( worldMapROI.x * TILEWIDTH + visibleScreenInPixels.width / 2, worldMapROI.y * TILEWIDTH + visibleScreenInPixels.height / 2 );

    // Special case: full map picture can be contained within the window -> center view on center of the map
    if ( world.w() * tileSizePerZoomLevel[ZoomLevel::ZoomLevel2] <= visibleScreenInPixels.width
         && world.h() * tileSizePerZoomLevel[ZoomLevel::ZoomLevel2] <= visibleScreenInPixels.height ) {
        viewCenterInPixels.x = world.w() * TILEWIDTH / 2;
        viewCenterInPixels.y = world.h() * TILEWIDTH / 2;
    }

    ZoomROIs currentROI( ZoomLevel::ZoomLevel2, viewCenterInPixels );

    CacheForMapWithResources cache( mode );

    DrawWorld( currentROI, cache );
    DrawObjectsIcons( color, mode, currentROI );
    Interface::GameBorderRedraw( true );

    // Draw radar
    radar.RedrawForViewWorld( currentROI, mode );

    // "View world" sprite
    const fheroes2::Sprite & viewWorldSprite = fheroes2::AGG::GetICN( GetSpriteResource( mode, isEvilInterface ), 0 );
    drawViewWorldSprite( viewWorldSprite, display, isEvilInterface );

    // Zoom button
    const fheroes2::Point buttonZoomPosition( display.width() - RADARWIDTH + 16, 2 * BORDERWIDTH + RADARWIDTH + 128 );
    fheroes2::Button buttonZoom( buttonZoomPosition.x, buttonZoomPosition.y, ( isEvilInterface ? ICN::LGNDXTRE : ICN::LGNDXTRA ), 0, 1 );
    buttonZoom.draw();

    // Exit button
    const fheroes2::Point buttonExitPosition( display.width() - RADARWIDTH + 16, 2 * BORDERWIDTH + RADARWIDTH + 236 );
    fheroes2::Button buttonExit( buttonExitPosition.x, buttonExitPosition.y, ( isEvilInterface ? ICN::LGNDXTRE : ICN::LGNDXTRA ), 2, 3 );
    buttonExit.draw();

    display.render();

    // Use for dragging the map from main window
    bool isDrag = false;
    fheroes2::Point initMousePos;
    fheroes2::Point initRoiCenter;

    // message loop
    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
        le.MousePressLeft( buttonZoom.area() ) ? buttonZoom.drawOnPress() : buttonZoom.drawOnRelease();

        bool changed = false;

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
            break;
        }

        if ( le.MouseClickLeft( buttonZoom.area() ) ) {
            changed = currentROI.zoomOut( true );
        }
        else if ( le.MouseCursor( radar.GetRect() ) ) {
            changed = radar.QueueEventProcessingForWorldView( currentROI );
        }
        else if ( le.MousePressLeft( visibleScreenInPixels ) ) {
            if ( isDrag ) {
                const fheroes2::Point & newMousePos = le.GetMouseCursor();
                const fheroes2::Point
                    newRoiCenter( initRoiCenter.x - ( newMousePos.x - initMousePos.x ) * TILEWIDTH / tileSizePerZoomLevel[static_cast<int>( currentROI._zoomLevel )],
                                  initRoiCenter.y - ( newMousePos.y - initMousePos.y ) * TILEWIDTH / tileSizePerZoomLevel[static_cast<int>( currentROI._zoomLevel )] );
                changed = currentROI.changeCenter( newRoiCenter );
            }
            else {
                isDrag = true;
                initMousePos = le.GetMouseCursor();
                initRoiCenter = currentROI._center;
            }
        }
        else if ( le.MouseWheelUp() ) {
            changed = currentROI.zoomIn( false );
        }
        else if ( le.MouseWheelDn() ) {
            changed = currentROI.zoomOut( false );
        }

        if ( !le.MousePressLeft( visibleScreenInPixels ) || !le.MouseCursor( visibleScreenInPixels ) ) {
            isDrag = false;
        }

        if ( changed ) {
            DrawWorld( currentROI, cache );
            DrawObjectsIcons( color, mode, currentROI );
            // Interface::GameBorderRedraw( true );
            radar.RedrawForViewWorld( currentROI, mode );
            // drawViewWorldSprite( viewWorldSprite, display, isEvilInterface );
            display.render();
        }
    }

    // Don't forget to reset the interface settings back if necessary
    if ( isHideInterface ) {
        conf.setHideInterface( true );
        interface.Reset();
    }

    LocalEvent::ResumeCycling();
}
