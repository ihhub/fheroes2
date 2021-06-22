/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include "view_world.h"
#include "agg_image.h"
#include "color.h"
#include "cursor.h"
#include "game.h"
#include "game_interface.h"
#include "icn.h"
#include "image.h"
#include "interface_border.h"
#include "maps.h"
#include "settings.h"
#include "tools.h"
#include "world.h"

#include <cassert>

//#define VIEWWORLD_DEBUG_ZOOM_LEVEL  // Activate this when you want to debug this window. It will provide an extra zoom level at 1:1 scale

namespace
{
    const int tileSizePerZoomLevel[4] = {4, 6, 12, 32};
    const int icnPerZoomLevel[4] = {ICN::MISC4, ICN::MISC6, ICN::MISC12, ICN::MISC12};
    const int icnLetterPerZoomLevel[4] = { ICN::LETTER4, ICN::LETTER6, ICN::LETTER12, ICN::LETTER12 };
    const int icnPerZoomLevelFlags[4] = {ICN::VWFLAG4, ICN::VWFLAG6, ICN::VWFLAG12, ICN::VWFLAG12};

    // Compute a rectangle that defines which world pixels we can see in the "view world" window,
    // based on given zoom level and initial center
    fheroes2::Rect computeROI( const fheroes2::Point & centerInPixel, const ViewWorld::ZoomLevel zoomLevel )
    {
        const fheroes2::Rect & sizeInPixels = Interface::Basic::Get().GetGameArea().GetROI();

        // how many pixels from "world map" we can see in "view world" window, given current zoom
        const int pixelsW = sizeInPixels.width * TILEWIDTH / tileSizePerZoomLevel[static_cast<int>( zoomLevel )];
        const int pixelsH = sizeInPixels.height * TILEWIDTH / tileSizePerZoomLevel[static_cast<int>( zoomLevel )];

        const int x = centerInPixel.x - pixelsW / 2;
        const int y = centerInPixel.y - pixelsH / 2;

        return fheroes2::Rect( x, y, pixelsW, pixelsH );
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

    int colorToOffsetICN( const int color )
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
        explicit CacheForMapWithResources( const bool revealAll )
        {
#ifdef VIEWWORLD_DEBUG_ZOOM_LEVEL
            cachedImages.resize( 4 );
#else
            cachedImages.resize( 3 );
#endif

            for ( size_t i = 0; i < cachedImages.size(); ++i ) {
                cachedImages[i].resize( world.w() * tileSizePerZoomLevel[i], world.h() * tileSizePerZoomLevel[i] );
                cachedImages[i]._disableTransformLayer();
            }

            const int32_t blockSizeX = TILEWIDTH * 18;
            const int32_t blockSizeY = TILEWIDTH * 18;

            const int32_t worldWidthPixels = world.w() * TILEWIDTH;
            const int32_t worldHeightPixels = world.h() * TILEWIDTH;

            // Assert will fail in case we add non-standard map sizes, otherwise standard map sizes are multiples of 18 tiles
            assert( worldWidthPixels % blockSizeX == 0 );
            assert( worldHeightPixels % blockSizeY == 0 );

            // Create temporary image where we will draw blocks of the main map on
            fheroes2::Image temporaryImg( blockSizeX, blockSizeY );
            temporaryImg._disableTransformLayer();

            Interface::GameArea gamearea = Interface::Basic::Get().GetGameArea();
            gamearea.SetAreaPosition( 0, 0, blockSizeX, blockSizeY );

            int drawingFlags = Interface::RedrawLevelType::LEVEL_ALL & ~Interface::RedrawLevelType::LEVEL_ROUTES;
            if ( revealAll ) {
                drawingFlags &= ~Interface::RedrawLevelType::LEVEL_FOG;
            }

            drawingFlags ^= Interface::RedrawLevelType::LEVEL_HEROES;

            // Draw sub-blocks of the main map, and resize them to draw them on lower-res cached versions:
            for ( int x = 0; x < worldWidthPixels; x += blockSizeX ) {
                for ( int y = 0; y < worldHeightPixels; y += blockSizeY ) {
                    gamearea.SetCenterInPixels( fheroes2::Point( x + blockSizeX / 2, y + blockSizeY / 2 ) );
                    gamearea.Redraw( temporaryImg, drawingFlags );

                    for ( size_t i = 0; i < cachedImages.size(); ++i ) {
                        const int blockSizeResizedX = blockSizeX * tileSizePerZoomLevel[i] / TILEWIDTH;
                        const int blockSizeResizedY = blockSizeY * tileSizePerZoomLevel[i] / TILEWIDTH;
                        fheroes2::Resize( temporaryImg, 0, 0, temporaryImg.width(), temporaryImg.height(), cachedImages[i], x * tileSizePerZoomLevel[i] / TILEWIDTH,
                                          y * tileSizePerZoomLevel[i] / TILEWIDTH, blockSizeResizedX, blockSizeResizedY );
                    }
                }
            }
        }
    };

    void DrawWorld( const ViewWorld::ZoomROIs & ROI, CacheForMapWithResources & cache )
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Image & image = cache.cachedImages[static_cast<int>( ROI._zoomLevel )];

        const fheroes2::Rect & roiScreen = Interface::Basic::Get().GetGameArea().GetROI();

        const int offsetPixelsX = tileSizePerZoomLevel[static_cast<int>( ROI._zoomLevel )] * ROI.GetROIinPixels().x / TILEWIDTH;
        const int offsetPixelsY = tileSizePerZoomLevel[static_cast<int>( ROI._zoomLevel )] * ROI.GetROIinPixels().y / TILEWIDTH;

        const fheroes2::Point inPos( offsetPixelsX < 0 ? 0 : offsetPixelsX, offsetPixelsY < 0 ? 0 : offsetPixelsY );
        const fheroes2::Point outPos( BORDERWIDTH + ( offsetPixelsX < 0 ? -offsetPixelsX : 0 ), BORDERWIDTH + ( offsetPixelsY < 0 ? -offsetPixelsY : 0 ) );
        const fheroes2::Size outSize( roiScreen.width + 2 * BORDERWIDTH + RADARWIDTH, roiScreen.height );

        fheroes2::Blit( image, inPos, display, outPos, outSize );

        // now, blit black pixels outside of the main view
        // left bar
        fheroes2::Fill( display, BORDERWIDTH, BORDERWIDTH, outPos.x - BORDERWIDTH, outSize.height, 0 );

        // right bar
        fheroes2::Fill( display, BORDERWIDTH - offsetPixelsX + image.width(), BORDERWIDTH, display.width() - BORDERWIDTH + offsetPixelsX - image.width(), outSize.height,
                        0 );

        // top bar
        fheroes2::Fill( display, BORDERWIDTH, BORDERWIDTH, display.width(), outPos.y - BORDERWIDTH, 0 );

        // bottom bar
        fheroes2::Fill( display, BORDERWIDTH, BORDERWIDTH - offsetPixelsY + image.height(), outSize.width,
                        display.height() - BORDERWIDTH + offsetPixelsY - image.height(), 0 );
    }

    void DrawObjectsIcons( const int color, const ViewWorldMode mode, const ViewWorld::ZoomROIs & ROI )
    {
        const bool revealAll = mode == ViewWorldMode::ViewAll;
        const bool revealMines = revealAll || ( mode == ViewWorldMode::ViewMines );
        const bool revealHeroes = revealAll || ( mode == ViewWorldMode::ViewHeroes );
        const bool revealTowns = revealAll || ( mode == ViewWorldMode::ViewTowns );
        const bool revealArtifacts = revealAll || ( mode == ViewWorldMode::ViewArtifacts );
        const bool revealResources = revealAll || ( mode == ViewWorldMode::ViewResources );

        const int zoomLevelId = static_cast<int>( ROI._zoomLevel );
        const int tileSize = tileSizePerZoomLevel[zoomLevelId];
        const int icnBase = icnPerZoomLevel[zoomLevelId];
        const int icnLetterId = icnLetterPerZoomLevel[zoomLevelId];
        const int icnFlagsBase = icnPerZoomLevelFlags[zoomLevelId];

        fheroes2::Display & display = fheroes2::Display::instance();

        const int32_t worldWidth = world.w();
        const int32_t worldHeight = world.h();

        const fheroes2::Rect roiPixels = ROI.GetROIinPixels();

        const int offsetX = roiPixels.x * tileSize / TILEWIDTH;
        const int offsetY = roiPixels.y * tileSize / TILEWIDTH;

        const fheroes2::Rect roiTiles = ROI.GetROIinTiles();

        // add margin because we also draw under the radar zone
        const int32_t marginForRightSide = ( 2 * BORDERWIDTH + RADARWIDTH ) / tileSize + 1;

        // add a margin of 2 tiles because icons outside of view can still show on the view
        const int32_t minTileX = clamp( roiTiles.x - 2, 0, worldWidth );
        const int32_t maxTileX = clamp( roiTiles.x + roiTiles.width + marginForRightSide + 2, 0, worldWidth );
        const int32_t minTileY = clamp( roiTiles.y - 2, 0, worldHeight );
        const int32_t maxTileY = clamp( roiTiles.y + roiTiles.height + 2, 0, worldHeight );

        for ( int32_t posY = minTileY; posY < maxTileY; ++posY ) {
            const int dsty = posY * tileSize - offsetY + BORDERWIDTH;

            for ( int32_t posX = minTileX; posX < maxTileX; ++posX ) {
                const int dstx = posX * tileSize - offsetX + BORDERWIDTH;

                const Maps::Tiles & tile = world.GetTiles( posX, posY );
                int icn = icnBase;
                int index = -1;

                int letterIndex = -1;

                int spriteOffsetX = 0;
                int spriteOffsetY = 0;

                switch ( tile.GetObject() ) {
                case MP2::OBJ_HEROES: {
                    if ( revealHeroes || !tile.isFog( color ) ) {
                        const Heroes * hero = world.GetHeroes( tile.GetCenter() );
                        if ( hero ) {
                            const int colorOffset = colorToOffsetICN( hero->GetColor() );
                            index = colorOffset >= 0 ? 7 + colorOffset : -1;

                            // handle case of hero above town/mine :
                            switch ( tile.GetObject( false ) ) {
                            case MP2::OBJ_ALCHEMYLAB:
                            case MP2::OBJ_MINES:
                            case MP2::OBJ_SAWMILL:
                                if ( revealMines || !tile.isFog( color ) ) { // draw mine now, hero on top after the switch
                                    const int colorOffsetForMine = colorToOffsetICN( tile.QuantityColor() );
                                    const fheroes2::Sprite & mineSprite = fheroes2::AGG::GetICN( icnBase, colorOffsetForMine );
                                    fheroes2::Blit( mineSprite, display, dstx, dsty );
                                }
                                break;
                            case MP2::OBJ_CASTLE:
                                if ( revealTowns || !tile.isFog( color ) ) { // draw hero now, castle flag on top later
                                    const Castle * castle = world.GetCastle( tile.GetCenter() );
                                    if ( castle ) {
                                        const fheroes2::Sprite & heroIcon = fheroes2::AGG::GetICN( icnBase, index );
                                        fheroes2::Blit( heroIcon, display, dstx, dsty );

                                        icn = icnFlagsBase;
                                        index = colorToOffsetICN( castle->GetColor() );
                                    }
                                }
                                break;
                            }
                        }
                    }
                } break;

                case MP2::OBJ_CASTLE: {
                    if ( revealTowns || !tile.isFog( color ) ) {
                        const Castle * castle = world.GetCastle( tile.GetCenter() );
                        if ( castle ) {
                            icn = icnFlagsBase;
                            index = colorToOffsetICN( castle->GetColor() );
                        }
                    }
                } break;

                case MP2::OBJ_ALCHEMYLAB:
                case MP2::OBJ_MINES:
                case MP2::OBJ_SAWMILL:
                    if ( revealMines || !tile.isFog( color ) ) {
                        index = colorToOffsetICN( tile.QuantityColor() );
                        spriteOffsetX = -6; // TODO -4 , -3
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
                    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, index );
                    fheroes2::Blit( sprite, display, dstx + spriteOffsetX, dsty + spriteOffsetY );

                    if ( letterIndex >= 0 ) {
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
                        fheroes2::Blit( letter, display, dstx + spriteOffsetX + ( sprite.width() - letter.width() ) / 2,
                                        dsty + spriteOffsetY + ( sprite.height() - letter.height() ) / 2 );
                    }
                }
            }
        }
    }

    int GetSpriteResource( const ViewWorldMode mode, const bool evil )
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
}

ViewWorld::ZoomROIs::ZoomROIs( const ViewWorld::ZoomLevel zoomLevel, const fheroes2::Point & centerInPixels )
    : _zoomLevel( zoomLevel )
    , _center( centerInPixels )
{
    for ( int i = 0; i < 4; ++i ) {
        _roiForZoomLevels[i] = computeROI( _center, static_cast<ViewWorld::ZoomLevel>( i ) );
    }
}

bool ViewWorld::ZoomROIs::ChangeCenter( const fheroes2::Point & centerInPixels )
{
    const fheroes2::Point newCenter( clamp( centerInPixels.x, 0, world.w() * TILEWIDTH ), clamp( centerInPixels.y, 0, world.h() * TILEWIDTH ) );

    if ( newCenter == _center ) {
        return false;
    }
    _center = newCenter;
    for ( int i = 0; i < 4; ++i ) {
        _roiForZoomLevels[i] = computeROI( _center, static_cast<ViewWorld::ZoomLevel>( i ) );
    }
    return true;
}

bool ViewWorld::ZoomROIs::ChangeZoom( const bool zoomIn, const bool cycle )
{
    ViewWorld::ZoomLevel newLevel = zoomIn ? GetNextZoomLevel( _zoomLevel, cycle ) : GetPreviousZoomLevel( _zoomLevel, cycle );
    if ( newLevel == _zoomLevel ) {
        return false;
    }
    _zoomLevel = newLevel;
    return true;
}

const fheroes2::Rect & ViewWorld::ZoomROIs::GetROIinPixels() const
{
    return _roiForZoomLevels[static_cast<int>( _zoomLevel )];
}

fheroes2::Rect ViewWorld::ZoomROIs::GetROIinTiles() const
{
    fheroes2::Rect result = _roiForZoomLevels[static_cast<int>( _zoomLevel )];
    result.x = result.x / TILEWIDTH;
    result.y = result.y / TILEWIDTH;
    result.width = result.width / TILEWIDTH;
    result.height = result.height / TILEWIDTH;
    return result;
}

void ViewWorld::ViewWorldWindow( const int color, const ViewWorldMode mode, Interface::Basic & interface )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::ImageRestorer restorer( display );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    LocalEvent & le = LocalEvent::Get();
    le.PauseCycling();

    // Creates fixed radar on top-right, even if hidden interface
    Interface::Radar radar = Interface::Radar::MakeRadarViewWorld( interface.GetRadar() );

    const fheroes2::Rect worldMapROI = interface.GetGameArea().GetVisibleTileROI();
    const fheroes2::Rect & visibleScreenInPixels = interface.GetGameArea().GetROI();

    // Initial view is centered on where the player is centered
    fheroes2::Point viewCenterInPixels( worldMapROI.x * TILEWIDTH + visibleScreenInPixels.width / 2, worldMapROI.y * TILEWIDTH + visibleScreenInPixels.height / 2 );

    // Special case: full map picture can be contained within the window -> center view on center of the map
    if ( world.w() * tileSizePerZoomLevel[static_cast<int>( ZoomLevel::ZoomLevel2 )] <= visibleScreenInPixels.width
         && world.h() * tileSizePerZoomLevel[static_cast<int>( ZoomLevel::ZoomLevel2 )] <= visibleScreenInPixels.height ) {
        viewCenterInPixels.x = world.w() * TILEWIDTH / 2;
        viewCenterInPixels.y = world.h() * TILEWIDTH / 2;
    }

    ZoomROIs currentROI( ZoomLevel::ZoomLevel2, viewCenterInPixels );

    CacheForMapWithResources cache( mode == ViewWorldMode::ViewAll );

    DrawWorld( currentROI, cache );
    DrawObjectsIcons( color, mode, currentROI );
    Interface::GameBorderRedraw( true );

    // Draw radar
    radar.RedrawForViewWorld( currentROI, mode );

    // "View world" sprite
    const bool isEvilInterface = Settings::Get().ExtGameEvilInterface();
    const fheroes2::Sprite & viewWorldSprite = fheroes2::AGG::GetICN( GetSpriteResource( mode, isEvilInterface ), 0 );
    fheroes2::Blit( viewWorldSprite, display, display.width() - viewWorldSprite.width() - BORDERWIDTH, 2 * BORDERWIDTH + RADARWIDTH );

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
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
        le.MousePressLeft( buttonZoom.area() ) ? buttonZoom.drawOnPress() : buttonZoom.drawOnRelease();

        bool changed = false;

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
            break;
        }
        else if ( le.MouseClickLeft( buttonZoom.area() ) ) {
            changed = currentROI.ChangeZoom( false, true );
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
                changed = currentROI.ChangeCenter( newRoiCenter );
            }
            else {
                isDrag = true;
                initMousePos = le.GetMouseCursor();
                initRoiCenter = currentROI._center;
            }
        }
        else if ( le.MouseWheelUp() ) {
            changed = currentROI.ChangeZoom( true );
        }
        else if ( le.MouseWheelDn() ) {
            changed = currentROI.ChangeZoom( false );
        }

        if ( !le.MousePressLeft( visibleScreenInPixels ) || !le.MouseCursor( visibleScreenInPixels ) ) {
            isDrag = false;
        }

        if ( changed ) {
            DrawWorld( currentROI, cache );
            DrawObjectsIcons( color, mode, currentROI );
            Interface::GameBorderRedraw( true );
            radar.RedrawForViewWorld( currentROI, mode );
            fheroes2::Blit( viewWorldSprite, display, display.width() - viewWorldSprite.width() - BORDERWIDTH, 2 * BORDERWIDTH + RADARWIDTH );
            display.render();
        }
    }

    le.ResumeCycling();
}
