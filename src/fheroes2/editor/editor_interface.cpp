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

#if defined( WITH_DEBUG )

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
#include "icn.h"
#include "image.h"
#include "interface_border.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "interface_status.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_tool.h"
#include "view_world.h"

namespace Interface
{
    Interface::Editor::Editor()
        : _editorPanel( *this )
    {
        redraw = 0;
        Reset();
    }

    void Editor::Reset()
    {
        const fheroes2::Display & display = fheroes2::Display::instance();

        const int32_t xOffset = display.width() - BORDERWIDTH - RADARWIDTH;
        radar.SetPos( xOffset, BORDERWIDTH );

        if ( display.height() > display.DEFAULT_HEIGHT + BORDERWIDTH ) {
            _editorPanel.setPos( xOffset, radar.GetArea().y + radar.GetArea().height + BORDERWIDTH );
            statusWindow.SetPos( xOffset, _editorPanel.getRect().y + _editorPanel.getRect().height );
        }
        else {
            _editorPanel.setPos( xOffset, radar.GetArea().y + radar.GetArea().height );
        }

        const fheroes2::Point prevCenter = gameArea.getCurrentCenterInPixels();
        const fheroes2::Rect prevRoi = gameArea.GetROI();

        gameArea.SetAreaPosition( BORDERWIDTH, BORDERWIDTH, display.width() - RADARWIDTH - 3 * BORDERWIDTH, display.height() - 2 * BORDERWIDTH );

        const fheroes2::Rect newRoi = gameArea.GetROI();

        gameArea.SetCenterInPixels( prevCenter + fheroes2::Point( newRoi.x + newRoi.width / 2, newRoi.y + newRoi.height / 2 )
                                    - fheroes2::Point( prevRoi.x + prevRoi.width / 2, prevRoi.y + prevRoi.height / 2 ) );
    }

    void Editor::Redraw( const uint32_t force /* = 0 */ )
    {
        const fheroes2::Display & display = fheroes2::Display::instance();

        const uint32_t combinedRedraw = redraw | force;

        if ( combinedRedraw & REDRAW_GAMEAREA ) {
            // Render all except the fog.
            gameArea.Redraw( fheroes2::Display::instance(), LEVEL_OBJECTS | LEVEL_HEROES | LEVEL_ROUTES );

            // TODO:: Render horizontal and vertical map tiles scale.
        }

        if ( combinedRedraw & ( REDRAW_RADAR_CURSOR | REDRAW_RADAR ) ) {
            radar.redrawForEditor( combinedRedraw & REDRAW_RADAR );
        }

        if ( combinedRedraw & REDRAW_BORDER ) {
            // Game border for the View World are the same as for the Map Editor.
            GameBorderRedraw( true );
        }

        if ( combinedRedraw & REDRAW_BUTTONS ) {
            _editorPanel._redraw();
        }

        if ( ( combinedRedraw & REDRAW_STATUS ) && ( display.height() > display.DEFAULT_HEIGHT + BORDERWIDTH ) ) {
            statusWindow.Redraw();
        }

        redraw = 0;
    }

    Interface::Editor & Interface::Editor::Get()
    {
        static Editor editorInterface;
        return editorInterface;
    }

    fheroes2::GameMode Interface::Editor::startEdit()
    {
        Reset();

        // Stop all sounds and music.
        AudioManager::ResetAudio();

        const Settings & conf = Settings::Get();

        radar.Build();
        radar.SetHide( false );

        fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

        gameArea.SetUpdateCursor();

        SetRedraw( REDRAW_GAMEAREA | REDRAW_RADAR | REDRAW_BUTTONS | REDRAW_STATUS | REDRAW_BORDER );

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
                    EventSaveGame();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME ) ) {
                    res = eventLoadMap();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_FILE_OPTIONS ) ) {
                    res = eventFileDialog();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCENARIO_INFORMATION ) ) {
                    res = EventScenarioInformation();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_VIEW_WORLD ) ) {
                    EventViewWorld();
                }
                // map scrolling control
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_LEFT ) ) {
                    gameArea.SetScroll( SCROLL_LEFT );
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_RIGHT ) ) {
                    gameArea.SetScroll( SCROLL_RIGHT );
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_UP ) ) {
                    gameArea.SetScroll( SCROLL_TOP );
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_DOWN ) ) {
                    gameArea.SetScroll( SCROLL_BOTTOM );
                }
                // default action
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_DEFAULT_ACTION ) ) {
                    res = EventDefaultAction( res );
                }
                // open focus
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_OPEN_FOCUS ) ) {
                    EventOpenFocus();
                }
            }

            if ( res != fheroes2::GameMode::CANCEL ) {
                break;
            }

            if ( fheroes2::cursor().isFocusActive() && !gameArea.isDragScroll() && !radar.isDragRadar() && ( conf.ScrollSpeed() != SCROLL_SPEED_NONE ) ) {
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
                    if ( Game::validateAnimationDelay( Game::SCROLL_START_DELAY ) ) {
                        if ( fastScrollRepeatCount < fastScrollStartThreshold ) {
                            ++fastScrollRepeatCount;
                        }
                    }

                    if ( fastScrollRepeatCount >= fastScrollStartThreshold ) {
                        gameArea.SetScroll( scrollPosition );
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
            if ( le.MouseCursor( radar.GetRect() ) ) {
                if ( Cursor::POINTER != cursor.Themes() ) {
                    cursor.SetThemes( Cursor::POINTER );
                }
                if ( !gameArea.isDragScroll() ) {
                    radar.QueueEventProcessing();
                }
            }
            // cursor is over the game area
            else if ( le.MouseCursor( gameArea.GetROI() ) && !gameArea.NeedScroll() ) {
                isCursorOverGamearea = true;
            }
            // cursor is over the buttons area
            else if ( le.MouseCursor( _editorPanel.getRect() ) ) {
                if ( Cursor::POINTER != cursor.Themes() ) {
                    cursor.SetThemes( Cursor::POINTER );
                }
                if ( !gameArea.NeedScroll() ) {
                    res = _editorPanel.queueEventProcessing();
                }
            }
            // cursor is somewhere else
            else if ( !gameArea.NeedScroll() ) {
                if ( Cursor::POINTER != cursor.Themes() ) {
                    cursor.SetThemes( Cursor::POINTER );
                }
                gameArea.ResetCursorPosition();
            }

            // gamearea
            if ( !gameArea.NeedScroll() ) {
                if ( !radar.isDragRadar() ) {
                    gameArea.QueueEventProcessing( isCursorOverGamearea );
                }
                else if ( !le.MousePressLeft() ) {
                    radar.QueueEventProcessing();
                }
            }

            // fast scroll
            if ( gameArea.NeedScroll() || gameArea.needDragScrollRedraw() ) {
                if ( Game::validateAnimationDelay( Game::SCROLL_DELAY ) ) {
                    if ( ( isScrollLeft( le.GetMouseCursor() ) || isScrollRight( le.GetMouseCursor() ) || isScrollTop( le.GetMouseCursor() )
                           || isScrollBottom( le.GetMouseCursor() ) )
                         && !gameArea.isDragScroll() ) {
                        cursor.SetThemes( gameArea.GetScrollCursor() );
                    }

                    gameArea.Scroll();

                    redraw |= REDRAW_GAMEAREA | REDRAW_RADAR_CURSOR;
                }
            }

            // Render map only if the turn is not over.
            if ( res == fheroes2::GameMode::CANCEL ) {
                // map objects animation
                if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
                    Game::updateAdventureMapAnimationIndex();
                    redraw |= REDRAW_GAMEAREA;
                }

                if ( NeedRedraw() ) {
                    Redraw();

                    // If this assertion blows up it means that we are holding a RedrawLocker lock for rendering which should not happen.
                    assert( GetRedrawMask() == 0 );

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

    fheroes2::GameMode Editor::eventFileDialog() const
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

                return EventSaveGame();
            }
            else if ( le.MouseClickLeft( buttonQuit.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_QUIT ) ) {
                if ( Interface::Basic::EventExit() == fheroes2::GameMode::QUIT_GAME ) {
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

    void Editor::EventViewWorld()
    {
        ViewWorld::ViewWorldWindow( 0, ViewWorldMode::ViewAll, *this );
    }
}
#endif
