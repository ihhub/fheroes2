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

#include "editor_interface.h"

#include <cassert>
#include <vector>

#include "agg_image.h"
#include "audio_manager.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "gamedefs.h"
#include "ground.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "interface_base.h"
#include "interface_border.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "interface_status.h"
#include "localevent.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_tool.h"
#include "view_world.h"
#include "world.h"

class Castle;

namespace Interface
{
    Interface::Editor::Editor()
        : _editorPanel( *this )
    {
        Editor::reset();
    }

    void Editor::reset()
    {
        const fheroes2::Display & display = fheroes2::Display::instance();

        const int32_t xOffset = display.width() - BORDERWIDTH - RADARWIDTH;
        _radar.SetPos( xOffset, BORDERWIDTH );

        if ( display.height() > display.DEFAULT_HEIGHT + BORDERWIDTH ) {
            _editorPanel.setPos( xOffset, _radar.GetArea().y + _radar.GetArea().height + BORDERWIDTH );
            _statusWindow.SetPos( xOffset, _editorPanel.getRect().y + _editorPanel.getRect().height );
        }
        else {
            _editorPanel.setPos( xOffset, _radar.GetArea().y + _radar.GetArea().height );
        }

        const fheroes2::Point prevCenter = _gameArea.getCurrentCenterInPixels();
        const fheroes2::Rect prevRoi = _gameArea.GetROI();

        _gameArea.SetAreaPosition( BORDERWIDTH, BORDERWIDTH, display.width() - RADARWIDTH - 3 * BORDERWIDTH, display.height() - 2 * BORDERWIDTH );

        const fheroes2::Rect newRoi = _gameArea.GetROI();

        _gameArea.SetCenterInPixels( prevCenter + fheroes2::Point( newRoi.x + newRoi.width / 2, newRoi.y + newRoi.height / 2 )
                                     - fheroes2::Point( prevRoi.x + prevRoi.width / 2, prevRoi.y + prevRoi.height / 2 ) );
    }

    void Editor::redraw( const uint32_t force /* = 0 */ )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const uint32_t combinedRedraw = _redraw | force;

        if ( combinedRedraw & REDRAW_GAMEAREA ) {
            // Render all except the fog.
            _gameArea.Redraw( display, LEVEL_OBJECTS | LEVEL_HEROES | LEVEL_ROUTES );

            // TODO:: Render horizontal and vertical map tiles scale and highlight with yellow text cursor position.

            if ( _editorPanel.isTerrainEdit() ) {
                LocalEvent & le = LocalEvent::Get();
                const fheroes2::Point & mousePosition = le.GetMouseCursor();
                const int32_t index = _gameArea.GetValidTileIdFromPoint( mousePosition );

                if ( index > -1 ) {
                    const int32_t brushSize = _editorPanel.getBrushSize();

                    if ( brushSize > 0 ) {
                        const int32_t worldWidth = world.w();
                        const int32_t cursorSizeX = std::min( brushSize, worldWidth - index % worldWidth ) - 1;
                        const int32_t cursorSizeY = std::min( brushSize, world.h() - index / worldWidth ) - 1;
                        const int32_t endIndex = index + cursorSizeX + worldWidth * cursorSizeY;
                        _gameArea.renderTileCursor( display, index, endIndex );
                    }
                    else if ( brushSize == -1 ) {
                        _gameArea.renderTileCursor( display, _selectedTile, index );
                    }
                }
            }
        }

        if ( combinedRedraw & ( REDRAW_RADAR_CURSOR | REDRAW_RADAR ) ) {
            // Render the mini-map without fog.
            _radar.redrawForEditor( combinedRedraw & REDRAW_RADAR );
        }

        if ( combinedRedraw & REDRAW_BORDER ) {
            // Game border for the View World are the same as for the Map Editor.
            GameBorderRedraw( true );
        }

        if ( combinedRedraw & REDRAW_PANEL ) {
            _editorPanel._redraw();
        }

        if ( ( combinedRedraw & REDRAW_STATUS ) && ( display.height() > display.DEFAULT_HEIGHT + BORDERWIDTH ) ) {
            // Currently the Adventure Map status is rendered to fill the space under the Editor buttons on high resolutions.
            // TODO: Make special status for Editor to display some map info, e.g. object properties under the cursor (castle garrison, amount of resources, etc.)
            // TODO: Decide where to output the status for low resolutions (reduce the number of displayed buttons - put some into sub-menu).
            _statusWindow._redraw();
        }

        _redraw = 0;
    }

    Interface::Editor & Interface::Editor::Get()
    {
        static Editor editorInterface;
        return editorInterface;
    }

    fheroes2::GameMode Interface::Editor::startEdit()
    {
        reset();

        // Stop all sounds and music.
        AudioManager::ResetAudio();

        const Settings & conf = Settings::Get();

        _radar.Build();
        _radar.SetHide( false );

        fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

        _gameArea.SetUpdateCursor();

        setRedraw( REDRAW_GAMEAREA | REDRAW_RADAR | REDRAW_PANEL | REDRAW_STATUS | REDRAW_BORDER );

        int32_t fastScrollRepeatCount = 0;
        const int32_t fastScrollStartThreshold = 2;

        bool isCursorOverGamearea = false;

        const std::vector<Game::DelayType> delayTypes = { Game::MAPS_DELAY };

        LocalEvent & le = LocalEvent::Get();
        Cursor & cursor = Cursor::Get();

        while ( res == fheroes2::GameMode::CANCEL ) {
            if ( !le.HandleEvents( Game::isDelayNeeded( delayTypes ), true ) ) {
                if ( EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                    res = fheroes2::GameMode::QUIT_GAME;
                    break;
                }
                continue;
            }

            // Process hot-keys.
            if ( le.KeyPress() ) {
                // adventure map control
                if ( HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_QUIT ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                    res = EventExit();
                }
                // TODO: remove this 'if defined' when Editor is ready for release.
#if defined( WITH_DEBUG )
                else if ( HotKeyPressEvent( Game::HotKeyEvent::EDITOR_NEW_MAP_MENU ) ) {
                    res = eventNewMap();
                }
#endif
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SAVE_GAME ) ) {
                    fheroes2::showStandardTextMessage( _( "Warning!" ), "The Map Editor is still in development. Save function is not implemented yet.", Dialog::OK );
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME ) ) {
                    res = eventLoadMap();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_FILE_OPTIONS ) ) {
                    res = eventFileDialog();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCENARIO_INFORMATION ) ) {
                    // TODO: Make the scenario info editor.
                    Dialog::GameInfo();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_VIEW_WORLD ) ) {
                    eventViewWorld();
                }
                // map scrolling control
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_LEFT ) ) {
                    _gameArea.SetScroll( SCROLL_LEFT );
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_RIGHT ) ) {
                    _gameArea.SetScroll( SCROLL_RIGHT );
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_UP ) ) {
                    _gameArea.SetScroll( SCROLL_TOP );
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_DOWN ) ) {
                    _gameArea.SetScroll( SCROLL_BOTTOM );
                }
                // default action
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_DEFAULT_ACTION ) ) {
                    fheroes2::showStandardTextMessage( _( "Warning!" ), "The Map Editor is still in development. No actions are available yet.", Dialog::OK );
                }
                // open focus
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_OPEN_FOCUS ) ) {
                    fheroes2::showStandardTextMessage( _( "Warning!" ), "The Map Editor is still in development. Open focused object dialog is not implemented yet.",
                                                       Dialog::OK );
                }
            }

            if ( res != fheroes2::GameMode::CANCEL ) {
                break;
            }

            if ( fheroes2::cursor().isFocusActive() && !_gameArea.isDragScroll() && !_radar.isDragRadar() && ( conf.ScrollSpeed() != SCROLL_SPEED_NONE ) ) {
                int scrollPosition = SCROLL_NONE;

                if ( isScrollLeft( le.GetMouseCursor() ) )
                    scrollPosition |= SCROLL_LEFT;
                else if ( isScrollRight( le.GetMouseCursor() ) )
                    scrollPosition |= SCROLL_RIGHT;
                if ( isScrollTop( le.GetMouseCursor() ) )
                    scrollPosition |= SCROLL_TOP;
                else if ( isScrollBottom( le.GetMouseCursor() ) )
                    scrollPosition |= SCROLL_BOTTOM;

                if ( scrollPosition != SCROLL_NONE ) {
                    if ( Game::validateAnimationDelay( Game::SCROLL_START_DELAY ) && ( fastScrollRepeatCount < fastScrollStartThreshold ) ) {
                        ++fastScrollRepeatCount;
                    }

                    if ( fastScrollRepeatCount >= fastScrollStartThreshold ) {
                        _gameArea.SetScroll( scrollPosition );
                    }
                }
                else {
                    fastScrollRepeatCount = 0;
                }
            }
            else {
                fastScrollRepeatCount = 0;
            }

            isCursorOverGamearea = false;

            // cursor is over the radar
            if ( le.MouseCursor( _radar.GetRect() ) ) {
                if ( Cursor::POINTER != cursor.Themes() ) {
                    cursor.SetThemes( Cursor::POINTER );
                }
                if ( !_gameArea.isDragScroll() ) {
                    _radar.QueueEventProcessing();
                }
            }
            // cursor is over the game area
            else if ( le.MouseCursor( _gameArea.GetROI() ) && !_gameArea.NeedScroll() ) {
                isCursorOverGamearea = true;
            }
            // cursor is over the buttons area
            else if ( le.MouseCursor( _editorPanel.getRect() ) ) {
                if ( Cursor::POINTER != cursor.Themes() ) {
                    cursor.SetThemes( Cursor::POINTER );
                }
                if ( !_gameArea.NeedScroll() ) {
                    res = _editorPanel.queueEventProcessing();
                }
            }
            // cursor is somewhere else
            else if ( !_gameArea.NeedScroll() ) {
                if ( Cursor::POINTER != cursor.Themes() ) {
                    cursor.SetThemes( Cursor::POINTER );
                }
                _gameArea.ResetCursorPosition();
            }

            // gamearea
            if ( !_gameArea.NeedScroll() ) {
                if ( !_radar.isDragRadar() ) {
                    _gameArea.QueueEventProcessing( isCursorOverGamearea );
                }
                else if ( !le.MousePressLeft() ) {
                    _radar.QueueEventProcessing();
                }
            }

            if ( le.MouseReleaseLeft() ) {
                _selectedTile = -1;
            }

            // fast scroll
            if ( Game::validateAnimationDelay( Game::SCROLL_DELAY ) && ( _gameArea.NeedScroll() || _gameArea.needDragScrollRedraw() ) ) {
                if ( ( isScrollLeft( le.GetMouseCursor() ) || isScrollRight( le.GetMouseCursor() ) || isScrollTop( le.GetMouseCursor() )
                       || isScrollBottom( le.GetMouseCursor() ) )
                     && !_gameArea.isDragScroll() ) {
                    cursor.SetThemes( _gameArea.GetScrollCursor() );
                }

                _gameArea.Scroll();

                _redraw |= REDRAW_GAMEAREA | REDRAW_RADAR_CURSOR;
            }

            if ( res == fheroes2::GameMode::CANCEL ) {
                // map objects animation
                if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
                    Game::updateAdventureMapAnimationIndex();
                    _redraw |= REDRAW_GAMEAREA;
                }

                if ( needRedraw() ) {
                    redraw();

                    // If this assertion blows up it means that we are holding a RedrawLocker lock for rendering which should not happen.
                    assert( getRedrawMask() == 0 );

                    validateFadeInAndRender();
                }
            }
        }

        Game::setDisplayFadeIn();

        fheroes2::fadeOutDisplay();

        return res;
    }

    fheroes2::GameMode Editor::eventLoadMap()
    {
        return Dialog::YES
                       == fheroes2::showStandardTextMessage( "", _( "Are you sure you want to load a new map? (Any unsaved changes to the current map will be lost.)" ),
                                                             Dialog::YES | Dialog::NO )
                   ? fheroes2::GameMode::EDITOR_LOAD_MAP
                   : fheroes2::GameMode::CANCEL;
    }

    fheroes2::GameMode Editor::eventNewMap()
    {
        return Dialog::YES
                       == fheroes2::showStandardTextMessage( "", _( "Are you sure you want to create a new map? (Any unsaved changes to the current map will be lost.)" ),
                                                             Dialog::YES | Dialog::NO )
                   ? fheroes2::GameMode::EDITOR_NEW_MAP
                   : fheroes2::GameMode::CANCEL;
    }

    fheroes2::GameMode Editor::eventFileDialog()
    {
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        const int cpanbkg = isEvilInterface ? ICN::CPANBKGE : ICN::CPANBKG;
        const fheroes2::Sprite & background = fheroes2::AGG::GetICN( cpanbkg, 0 );

        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        // Since the original image contains shadow it is important to remove it from calculation of window's position.
        const fheroes2::Point rb( ( display.width() - background.width() - BORDERWIDTH ) / 2, ( display.height() - background.height() + BORDERWIDTH ) / 2 );
        fheroes2::ImageRestorer back( display, rb.x, rb.y, background.width(), background.height() );
        fheroes2::Blit( background, display, rb.x, rb.y );

        // TODO: Make Evil interface for New/Load/Save Map buttons.
        fheroes2::Button buttonNew( rb.x + 62, rb.y + 31, ICN::ECPANEL, 0, 1 );
        fheroes2::Button buttonLoad( rb.x + 195, rb.y + 31, ICN::ECPANEL, 2, 3 );
        fheroes2::Button buttonSave( rb.x + 62, rb.y + 107, ICN::ECPANEL, 4, 5 );
        fheroes2::Button buttonQuit( rb.x + 195, rb.y + 107, isEvilInterface ? ICN::BUTTON_QUIT_EVIL : ICN::BUTTON_QUIT_GOOD, 0, 1 );
        fheroes2::Button buttonCancel( rb.x + 128, rb.y + 184, isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1 );

        buttonNew.draw();
        buttonLoad.draw();
        buttonSave.draw();
        buttonQuit.draw();
        buttonCancel.draw();

        display.render( back.rect() );

        fheroes2::GameMode result = fheroes2::GameMode::QUIT_GAME;

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonNew.area() ) ? buttonNew.drawOnPress() : buttonNew.drawOnRelease();
            le.MousePressLeft( buttonLoad.area() ) ? buttonLoad.drawOnPress() : buttonLoad.drawOnRelease();
            le.MousePressLeft( buttonSave.area() ) ? buttonSave.drawOnPress() : buttonSave.drawOnRelease();
            le.MousePressLeft( buttonQuit.area() ) ? buttonQuit.drawOnPress() : buttonQuit.drawOnRelease();
            le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

            // TODO: remove this 'if defined' when Editor is ready for release.
#if defined( WITH_DEBUG )
            if ( le.MouseClickLeft( buttonNew.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_NEW_MAP_MENU ) ) {
#else
            if ( le.MouseClickLeft( buttonNew.area() ) ) {
#endif
                if ( eventNewMap() == fheroes2::GameMode::EDITOR_NEW_MAP ) {
                    result = fheroes2::GameMode::EDITOR_NEW_MAP;
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonLoad.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME ) ) {
                if ( eventLoadMap() == fheroes2::GameMode::EDITOR_LOAD_MAP ) {
                    result = fheroes2::GameMode::EDITOR_LOAD_MAP;
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonSave.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_SAVE_GAME ) ) {
                back.restore();

                fheroes2::showStandardTextMessage( _( "Warning!" ), "The Map Editor is still in development. Save function is not implemented yet.", Dialog::OK );

                break;
            }
            else if ( le.MouseClickLeft( buttonQuit.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_QUIT ) ) {
                if ( EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                    result = fheroes2::GameMode::QUIT_GAME;
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyCloseWindow() ) {
                result = fheroes2::GameMode::CANCEL;
                break;
            }
            else if ( le.MousePressRight( buttonNew.area() ) ) {
                fheroes2::showStandardTextMessage( _( "New Map" ), _( "Create a new map, either from scratch or using the random map generator." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( buttonLoad.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Load Map" ), _( "Load an existing map." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( buttonSave.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Save Map" ), _( "Save the current map." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( buttonQuit.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Quit" ), _( "Quit out of the map editor." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
        }

        // restore background
        back.restore();
        display.render( back.rect() );

        return result;
    }

    void Editor::eventViewWorld()
    {
        // TODO: Make proper borders restoration for low height resolutions, like for hide interface mode.
        ViewWorld::ViewWorldWindow( 0, ViewWorldMode::ViewAll, *this );
    }

    void Editor::mouseCursorAreaClickLeft( const int32_t tileIndex )
    {
        Maps::Tiles & tile = world.GetTiles( tileIndex );

        Heroes * otherHero = tile.GetHeroes();
        Castle * otherCastle = world.getCastle( tile.GetCenter() );

        if ( otherHero ) {
            // TODO: Make hero edit dialog: like Battle only dialog, but only for one hero.
            Game::OpenHeroesDialog( *otherHero, true, true );
        }
        else if ( otherCastle ) {
            // TODO: Make Castle edit dialog: like original build dialog.
            Game::OpenCastleDialog( *otherCastle );
        }
        else if ( _editorPanel.isTerrainEdit() ) {
            const int32_t brushSize = _editorPanel.getBrushSize();
            if ( brushSize > 0 ) {
                const int groundId = _editorPanel.selectedGroundType();
                Maps::Tiles * brushTile = &tile;

                for ( int32_t tileOffsetY = 0; tileOffsetY < brushSize; ++tileOffsetY ) {
                    for ( int32_t tileOffsetX = 0; tileOffsetX < brushSize; ++tileOffsetX ) {
                        Maps::Tiles * currentTile = brushTile + tileOffsetX;

                        currentTile->setTerrainImage( Maps::Ground::getRandomTerrainImageIndex( groundId ), false, false );

                        if ( !Maps::isValidDirection( currentTile->GetIndex(), Direction::RIGHT ) ) {
                            break;
                        }
                    }

                    if ( !Maps::isValidDirection( brushTile->GetIndex(), Direction::BOTTOM ) ) {
                        break;
                    }

                    brushTile += world.w();
                }
            }
            else if ( brushSize == -1 ) {
                // TODO: Add ability to select area (without dragging a map).
                _selectedTile = -1;
            }
            _redraw |= REDRAW_GAMEAREA | REDRAW_RADAR;
        }
    }

    void Editor::mouseCursorAreaPressLeft( const int32_t tileIndex )
    {
        _selectedTile = tileIndex;
    }

    void Editor::mouseCursorAreaPressRight( const int32_t tileIndex ) const
    {
        const Maps::Tiles & tile = world.GetTiles( tileIndex );

        switch ( tile.GetObject() ) {
        case MP2::OBJ_NON_ACTION_CASTLE:
        case MP2::OBJ_CASTLE: {
            const Castle * castle = world.getCastle( tile.GetCenter() );

            if ( castle ) {
                Dialog::QuickInfo( *castle );
            }
            else {
                Dialog::QuickInfo( tile );
            }

            break;
        }
        case MP2::OBJ_HEROES: {
            const Heroes * heroes = tile.GetHeroes();

            if ( heroes ) {
                Dialog::QuickInfo( *heroes );
            }

            break;
        }
        default:
            Dialog::QuickInfo( tile );
            break;
        }
    }
}
