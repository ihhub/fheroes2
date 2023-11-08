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

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "agg_image.h"
#include "artifact.h"
#include "audio_manager.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "gamedefs.h"
#include "heroes.h"
#include "history_manager.h"
#include "icn.h"
#include "image.h"
#include "interface_base.h"
#include "interface_border.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "interface_status.h"
#include "localevent.h"
#include "logging.h"
#include "map_object_info.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "mp2.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_tool.h"
#include "view_world.h"
#include "world.h"

class Castle;

namespace
{
    const uint32_t mapUpdateFlags = Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR;

    int32_t getBrushAreaEndIndex( const int32_t brushSize, const int32_t startIndex )
    {
        const int32_t worldWidth = world.w();
        const int32_t cursorSizeX = std::min( brushSize, worldWidth - startIndex % worldWidth ) - 1;
        const int32_t cursorSizeY = std::min( brushSize, world.h() - startIndex / worldWidth ) - 1;
        return startIndex + cursorSizeX + worldWidth * cursorSizeY;
    }
}

namespace Interface
{
    void EditorInterface::reset()
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

        _historyManager.reset();
    }

    void EditorInterface::redraw( const uint32_t force )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const uint32_t combinedRedraw = _redraw | force;

        if ( combinedRedraw & REDRAW_GAMEAREA ) {
            // Render all except the fog.
            _gameArea.Redraw( display, LEVEL_OBJECTS | LEVEL_HEROES | LEVEL_ROUTES );

            // TODO:: Render horizontal and vertical map tiles scale and highlight with yellow text cursor position.

            if ( _editorPanel.showAreaSelectRect() && ( _tileUnderCursor > -1 ) ) {
                const int32_t brushSize = _editorPanel.getBrushSize();

                if ( brushSize > 0 ) {
                    _gameArea.renderTileAreaSelect( display, _tileUnderCursor, getBrushAreaEndIndex( brushSize, _tileUnderCursor ) );
                }
                else if ( brushSize == 0 ) {
                    // Render area selection from the tile where the left mouse button was pressed till the tile under the cursor.
                    _gameArea.renderTileAreaSelect( display, _selectedTile, _tileUnderCursor );
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

    Interface::EditorInterface & Interface::EditorInterface::Get()
    {
        static EditorInterface editorInterface;
        return editorInterface;
    }

    fheroes2::GameMode Interface::EditorInterface::startEdit()
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
                else if ( HotKeyPressEvent( Game::HotKeyEvent::EDITOR_NEW_MAP_MENU ) ) {
                    res = eventNewMap();
                }
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
                else if ( HotKeyPressEvent( Game::HotKeyEvent::EDITOR_UNDO_LAST_ACTION ) ) {
                    undoAction();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::EDITOR_REDO_LAST_ACTION ) ) {
                    redoAction();
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
                cursor.SetThemes( Cursor::POINTER );

                // TODO: Add checks for object placing/moving, and other Editor functions that uses mouse dragging.
                if ( !_gameArea.isDragScroll() && ( _editorPanel.getBrushSize() > 0 || _selectedTile == -1 ) ) {
                    _radar.QueueEventProcessing();
                }
            }
            // cursor is over the game area
            else if ( le.MouseCursor( _gameArea.GetROI() ) && !_gameArea.NeedScroll() ) {
                isCursorOverGamearea = true;
            }
            // cursor is over the buttons area
            else if ( le.MouseCursor( _editorPanel.getRect() ) ) {
                cursor.SetThemes( Cursor::POINTER );

                if ( !_gameArea.NeedScroll() ) {
                    res = _editorPanel.queueEventProcessing();
                }
            }
            // cursor is somewhere else
            else if ( !_gameArea.NeedScroll() ) {
                cursor.SetThemes( Cursor::POINTER );

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

            if ( isCursorOverGamearea ) {
                // Get tile index under the cursor.
                const int32_t tileIndex = _gameArea.GetValidTileIdFromPoint( le.GetMouseCursor() );
                const int32_t brushSize = _editorPanel.getBrushSize();

                if ( _tileUnderCursor != tileIndex ) {
                    _tileUnderCursor = tileIndex;

                    // Force redraw if cursor position was changed as area rectangle is also changed.
                    if ( _editorPanel.showAreaSelectRect() && ( brushSize > 0 || _selectedTile != -1 ) ) {
                        _redraw |= REDRAW_GAMEAREA;
                    }
                }

                if ( _selectedTile == -1 && tileIndex != -1 && brushSize == 0 && le.MousePressLeft() ) {
                    _selectedTile = tileIndex;
                    _redraw |= REDRAW_GAMEAREA;
                }
            }
            else if ( _tileUnderCursor != -1 ) {
                _tileUnderCursor = -1;
                _redraw |= REDRAW_GAMEAREA;
            }

            if ( _selectedTile > -1 && le.MouseReleaseLeft() ) {
                if ( isCursorOverGamearea && _editorPanel.getBrushSize() == 0 ) {
                    if ( _editorPanel.isTerrainEdit() ) {
                        // Fill the selected area in terrain edit mode.
                        const fheroes2::ActionCreator action( _historyManager );

                        const int groundId = _editorPanel.selectedGroundType();
                        Maps::setTerrainOnTiles( _selectedTile, _tileUnderCursor, groundId );
                    }
                    else if ( _editorPanel.isEraseMode() ) {
                        // Erase objects in the selected area.
                        const fheroes2::ActionCreator action( _historyManager );

                        Maps::eraseObjectsOnTiles( _selectedTile, _tileUnderCursor, _editorPanel.getEraseTypes() );
                    }
                }

                // Reset the area start tile.
                _selectedTile = -1;

                _redraw |= mapUpdateFlags;
            }

            // fast scroll
            if ( ( Game::validateAnimationDelay( Game::SCROLL_DELAY ) && _gameArea.NeedScroll() ) || _gameArea.needDragScrollRedraw() ) {
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
                    redraw( 0 );

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

    fheroes2::GameMode EditorInterface::eventLoadMap()
    {
        return Dialog::YES
                       == fheroes2::showStandardTextMessage( "", _( "Are you sure you want to load a new map? (Any unsaved changes to the current map will be lost.)" ),
                                                             Dialog::YES | Dialog::NO )
                   ? fheroes2::GameMode::EDITOR_LOAD_MAP
                   : fheroes2::GameMode::CANCEL;
    }

    fheroes2::GameMode EditorInterface::eventNewMap()
    {
        return Dialog::YES
                       == fheroes2::showStandardTextMessage( "", _( "Are you sure you want to create a new map? (Any unsaved changes to the current map will be lost.)" ),
                                                             Dialog::YES | Dialog::NO )
                   ? fheroes2::GameMode::EDITOR_NEW_MAP
                   : fheroes2::GameMode::CANCEL;
    }

    fheroes2::GameMode EditorInterface::eventFileDialog()
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

        fheroes2::GameMode result = fheroes2::GameMode::CANCEL;

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonNew.area() ) ? buttonNew.drawOnPress() : buttonNew.drawOnRelease();
            le.MousePressLeft( buttonLoad.area() ) ? buttonLoad.drawOnPress() : buttonLoad.drawOnRelease();
            le.MousePressLeft( buttonSave.area() ) ? buttonSave.drawOnPress() : buttonSave.drawOnRelease();
            le.MousePressLeft( buttonQuit.area() ) ? buttonQuit.drawOnPress() : buttonQuit.drawOnRelease();
            le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

            if ( le.MouseClickLeft( buttonNew.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_NEW_MAP_MENU ) ) {
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

    void EditorInterface::eventViewWorld()
    {
        // TODO: Make proper borders restoration for low height resolutions, like for hide interface mode.
        ViewWorld::ViewWorldWindow( 0, ViewWorldMode::ViewAll, *this );
    }

    void EditorInterface::mouseCursorAreaClickLeft( const int32_t tileIndex )
    {
        Maps::Tiles & tile = world.GetTiles( tileIndex );

        Heroes * otherHero = tile.getHero();
        Castle * otherCastle = world.getCastle( tile.GetCenter() );

        if ( otherHero ) {
            // TODO: Make hero edit dialog: e.g. with functions like in Battle only dialog, but only for one hero.
            Game::OpenHeroesDialog( *otherHero, true, true );
        }
        else if ( otherCastle ) {
            // TODO: Make Castle edit dialog: e.g. like original build dialog.
            Game::OpenCastleDialog( *otherCastle );
        }
        else if ( _editorPanel.isTerrainEdit() ) {
            const int32_t brushSize = _editorPanel.getBrushSize();
            const int groundId = _editorPanel.selectedGroundType();

            const fheroes2::ActionCreator action( _historyManager );

            if ( brushSize > 0 ) {
                Maps::setTerrainOnTiles( tileIndex, getBrushAreaEndIndex( brushSize, tileIndex ), groundId );
            }
            else if ( brushSize == 0 ) {
                // This is a case when area was not selected but a single tile was clicked.
                Maps::setTerrainOnTiles( tileIndex, tileIndex, groundId );

                _selectedTile = -1;
            }

            _redraw |= mapUpdateFlags;
        }
        else if ( _editorPanel.isRoadDraw() ) {
            const fheroes2::ActionCreator action( _historyManager );

            if ( Maps::updateRoadOnTile( tile, true ) ) {
                _redraw |= mapUpdateFlags;
            }
        }
        else if ( _editorPanel.isStreamDraw() ) {
            const fheroes2::ActionCreator action( _historyManager );

            if ( Maps::updateStreamOnTile( tile, true ) ) {
                _redraw |= mapUpdateFlags;
            }
        }
        else if ( _editorPanel.isEraseMode() ) {
            const int32_t brushSize = _editorPanel.getBrushSize();

            const fheroes2::ActionCreator action( _historyManager );

            if ( brushSize > 1 ) {
                if ( Maps::eraseObjectsOnTiles( tileIndex, getBrushAreaEndIndex( brushSize, tileIndex ), _editorPanel.getEraseTypes() ) ) {
                    _redraw |= mapUpdateFlags;
                }
            }
            else {
                if ( Maps::eraseOjects( tile, _editorPanel.getEraseTypes() ) ) {
                    _redraw |= mapUpdateFlags;
                }

                if ( brushSize == 0 ) {
                    // This is a case when area was not selected but a single tile was clicked.
                    _selectedTile = -1;
                }
            }
        }
        else if ( _editorPanel.isMonsterSettingMode() ) {
            if ( tile.isWater() ) {
                fheroes2::showStandardTextMessage( _( "Monster" ), _( "Monsters cannot be placed on water." ), Dialog::OK );
            }
            else if ( !Maps::isClearGround( tile ) ) {
                fheroes2::showStandardTextMessage( _( "Monster" ), _( "Choose a tile which does not contain any objects." ), Dialog::OK );
            }
            else {
                setObjectOnTile( tile, Maps::ObjectGroup::Monster, _editorPanel.getMonsterType() );
            }
        }
        else if ( _editorPanel.isTreasureSettingMode() ) {
            if ( tile.isWater() ) {
                fheroes2::showStandardTextMessage( _( "Treasure" ), _( "Treasures cannot be placed on water." ), Dialog::OK );
            }
            else if ( !Maps::isClearGround( tile ) ) {
                fheroes2::showStandardTextMessage( _( "Treasure" ), _( "Choose a tile which does not contain any objects." ), Dialog::OK );
            }
            else {
                setObjectOnTile( tile, Maps::ObjectGroup::Treasure, _editorPanel.getTreasureType() );
            }
        }
        else if ( _editorPanel.isHeroSettingMode() ) {
            if ( tile.isWater() ) {
                fheroes2::showStandardTextMessage( _( "Heroes" ), _( "Heroes cannot be placed on water." ), Dialog::OK );
            }
            else if ( !Maps::isClearGround( tile ) ) {
                fheroes2::showStandardTextMessage( _( "Heroes" ), _( "Choose a tile which does not contain any objects." ), Dialog::OK );
            }
            else {
                setObjectOnTile( tile, Maps::ObjectGroup::Hero, _editorPanel.getHeroType() );
            }
        }
        else if ( _editorPanel.isArtifactSettingMode() ) {
            if ( tile.isWater() ) {
                fheroes2::showStandardTextMessage( _( "Artifacts" ), _( "Artifacts cannot be placed on water." ), Dialog::OK );
            }
            else if ( !Maps::isClearGround( tile ) ) {
                fheroes2::showStandardTextMessage( _( "Artifacts" ), _( "Choose a tile which does not contain any objects." ), Dialog::OK );
            }
            else {
                const int32_t artifactType = _editorPanel.getArtifactType();

                const auto & artifactInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::Artifact );
                assert( artifactType >= 0 && artifactType < static_cast<int32_t>( artifactInfo.size() ) );

                // For each Spell Scroll artifact we select a spell.
                if ( artifactInfo[artifactType].objectType == MP2::OBJ_ARTIFACT && artifactInfo[artifactType].metadata[0] == Artifact::SPELL_SCROLL ) {
                    const int spellId = Dialog::selectSpell( Spell::RANDOM, true ).GetID();

                    if ( spellId == Spell::NONE ) {
                        // We do not place the Spell Scroll artifact if the spell for it was not selected.
                        return;
                    }

                    setObjectOnTile( tile, Maps::ObjectGroup::Artifact, _editorPanel.getArtifactType() );
                    Maps::setSpellOnTile( tile, spellId );
                }
                else {
                    setObjectOnTile( tile, Maps::ObjectGroup::Artifact, _editorPanel.getArtifactType() );
                }
            }
        }
    }

    void EditorInterface::mouseCursorAreaPressRight( const int32_t tileIndex ) const
    {
        const Maps::Tiles & tile = world.GetTiles( tileIndex );

        DEBUG_LOG( DBG_DEVEL, DBG_INFO, std::endl << tile.String() )

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
            const Heroes * heroes = tile.getHero();

            if ( heroes ) {
                Dialog::QuickInfo( *heroes );
            }
            else if ( tile.getObjectIcnType() == MP2::OBJ_ICN_TYPE_MINIHERO ) {
                fheroes2::showStandardTextMessage( _( "Heroes" ), "", Dialog::ZERO );
            }

            break;
        }
        default:
            Dialog::QuickInfo( tile );
            break;
        }
    }

    void EditorInterface::updateCursor( const int32_t tileIndex )
    {
        if ( _cursorUpdater && tileIndex >= 0 ) {
            _cursorUpdater( tileIndex );
        }
        else {
            Cursor::Get().SetThemes( Cursor::POINTER );
        }
    }

    void EditorInterface::setObjectOnTile( Maps::Tiles & tile, const Maps::ObjectGroup group, const int32_t objectType )
    {
        const auto & objectInfo = Maps::getObjectsByGroup( group );
        if ( objectType < 0 || objectType >= static_cast<int32_t>( objectInfo.size() ) ) {
            return;
        }

        const fheroes2::ActionCreator action( _historyManager );

        Maps::setObjectOnTile( tile, objectInfo[objectType] );

        _redraw |= mapUpdateFlags;
    }
}
