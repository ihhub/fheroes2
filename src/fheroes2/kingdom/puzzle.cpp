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

#include "puzzle.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "agg_image.h"
#include "artifact_ultimate.h"
#include "audio.h"
#include "audio_manager.h"
#include "cursor.h"
#include "dialog.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "icn.h"
#include "image.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "localevent.h"
#include "logging.h"
#include "math_base.h"
#include "mus.h"
#include "rand.h"
#include "screen.h"
#include "serialize.h"
#include "settings.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_tool.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    bool ClosedTilesExists( const Puzzle & pzl, const std::vector<uint8_t> & zone )
    {
        return std::any_of( zone.begin(), zone.end(), [&pzl]( const uint8_t tile ) { return !pzl.test( tile ); } );
    }

    void ZoneOpenFirstTiles( Puzzle & pzl, size_t & opens, const std::vector<uint8_t> & zone )
    {
        while ( opens ) {
            std::vector<uint8_t>::const_iterator it = zone.begin();
            while ( it != zone.end() && pzl.test( *it ) )
                ++it;

            if ( it != zone.end() ) {
                pzl.set( *it );
                --opens;
            }
            else
                break;
        }
    }

    void drawPuzzle( const Puzzle & pzl, const fheroes2::Image & sf, int32_t dstx, int32_t dsty )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        // Immediately reveal the entire puzzle in developer mode
        if ( IS_DEVEL() ) {
            assert( sf.singleLayer() );

            fheroes2::Copy( sf, 0, 0, display, dstx, dsty, sf.width(), sf.height() );

            return;
        }

        for ( size_t i = 0; i < pzl.size(); ++i ) {
            const fheroes2::Sprite & piece = fheroes2::AGG::GetICN( ICN::PUZZLE, static_cast<uint32_t>( i ) );

            fheroes2::Blit( piece, display, dstx + piece.x() - fheroes2::borderWidthPx, dsty + piece.y() - fheroes2::borderWidthPx );
        }
    }

    bool revealPuzzle( const Puzzle & pzl, const fheroes2::Image & sf, int32_t dstx, int32_t dsty, fheroes2::Button & buttonExit,
                       const std::function<fheroes2::Rect()> * drawControlPanel = nullptr )
    {
        // In developer mode, the entire puzzle should already be revealed
        if ( IS_DEVEL() ) {
            return false;
        }

        // The game area puzzle image should be single-layer.
        assert( sf.singleLayer() );

        fheroes2::Display & display = fheroes2::Display::instance();
        LocalEvent & le = LocalEvent::Get();

        const std::vector<Game::DelayType> delayTypes = { Game::PUZZLE_FADE_DELAY };
        Game::passAnimationDelay( Game::PUZZLE_FADE_DELAY );

        int alpha = 250;

        while ( alpha >= 0 && le.HandleEvents( Game::isDelayNeeded( delayTypes ) ) ) {
            le.isMouseLeftButtonPressedInArea( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
            // If exit button was pressed before reveal animation is finished, return true to indicate early exit.
            if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
                return true;
            }

            if ( Game::validateAnimationDelay( Game::PUZZLE_FADE_DELAY ) ) {
                fheroes2::Copy( sf, 0, 0, display, dstx, dsty, sf.width(), sf.height() );

                for ( size_t i = 0; i < pzl.size(); ++i ) {
                    const fheroes2::Sprite & piece = fheroes2::AGG::GetICN( ICN::PUZZLE, static_cast<uint32_t>( i ) );

                    uint8_t pieceAlpha = 255;
                    if ( pzl.test( i ) )
                        pieceAlpha = static_cast<uint8_t>( alpha );

                    fheroes2::AlphaBlit( piece, display, dstx + piece.x() - fheroes2::borderWidthPx, dsty + piece.y() - fheroes2::borderWidthPx, pieceAlpha );
                }

                if ( drawControlPanel ) {
                    display.render( ( *drawControlPanel )() );
                }

                display.render( { dstx, dsty, sf.width(), sf.height() } );

                if ( alpha <= 0 ) {
                    break;
                }

                alpha -= 10;
                assert( alpha >= 0 );
            }
        }

        return false;
    }

    void ShowStandardDialog( const Puzzle & pzl, const fheroes2::Image & sf )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        // Puzzle map is called only for the Adventure Map, not Editor.
        Interface::AdventureMap & adventureMapInterface = Interface::AdventureMap::Get();
        const fheroes2::Rect & radarArea = adventureMapInterface.getRadar().GetArea();

        fheroes2::ImageRestorer back( display, fheroes2::borderWidthPx, fheroes2::borderWidthPx, sf.width(), sf.height() );
        fheroes2::ImageRestorer radarRestorer( display, radarArea.x, radarArea.y, radarArea.width, radarArea.height );

        fheroes2::fadeOutDisplay( back.rect(), false );

        fheroes2::Copy( fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::EVIWPUZL : ICN::VIEWPUZL ), 0 ), 0, 0, display, radarArea );
        display.updateNextRenderRoi( radarArea );

        fheroes2::Button buttonExit( radarArea.x + 32, radarArea.y + radarArea.height - 37,
                                     ( isEvilInterface ? ICN::BUTTON_EXIT_PUZZLE_DIM_DOOR_EVIL : ICN::BUTTON_EXIT_PUZZLE_DIM_DOOR_GOOD ), 0, 1 );
        buttonExit.draw();

        drawPuzzle( pzl, sf, fheroes2::borderWidthPx, fheroes2::borderWidthPx );

        display.updateNextRenderRoi( radarArea );

        fheroes2::fadeInDisplay( back.rect(), false );

        const bool earlyExit = revealPuzzle( pzl, sf, fheroes2::borderWidthPx, fheroes2::borderWidthPx, buttonExit );

        LocalEvent & le = LocalEvent::Get();

        while ( !earlyExit && le.HandleEvents() ) {
            le.isMouseLeftButtonPressedInArea( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
            if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
                break;
            }
        }

        // Fade from puzzle map to adventure map.
        fheroes2::fadeOutDisplay( back.rect(), false );
        back.restore();
        radarRestorer.restore();
        display.updateNextRenderRoi( radarArea );
        fheroes2::fadeInDisplay( back.rect(), false );

        adventureMapInterface.getGameArea().SetUpdateCursor();
    }

    void ShowExtendedDialog( const Puzzle & pzl, const fheroes2::Image & sf )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        // Puzzle map is called only for the Adventure Map, not Editor.
        Interface::AdventureMap & adventureMapInterface = Interface::AdventureMap::Get();
        Interface::GameArea & gameArea = adventureMapInterface.getGameArea();
        const fheroes2::Rect & gameAreaRoi = gameArea.GetROI();
        const Interface::Radar & radar = adventureMapInterface.getRadar();
        const fheroes2::Rect & radarArea = radar.GetArea();

        const fheroes2::ImageRestorer radarRestorer( display, radarArea.x, radarArea.y, radarArea.width, radarArea.height );

        const fheroes2::StandardWindow border( gameAreaRoi.x + ( gameAreaRoi.width - sf.width() ) / 2, gameAreaRoi.y + ( gameAreaRoi.height - sf.height() ) / 2,
                                               sf.width(), sf.height(), false );

        const fheroes2::Rect & puzzleArea = border.activeArea();

        fheroes2::Image background( puzzleArea.width, puzzleArea.height );

        const Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();
        const bool isHideInterface = conf.isHideInterfaceEnabled();

        if ( isEvilInterface ) {
            background.fill( fheroes2::GetColorId( 80, 80, 80 ) );
        }
        else {
            background.fill( fheroes2::GetColorId( 128, 64, 32 ) );
        }

        fheroes2::Copy( background, 0, 0, display, puzzleArea );

        fheroes2::Button buttonExit( radarArea.x + 32, radarArea.y + radarArea.height - 37,
                                     ( isEvilInterface ? ICN::BUTTON_EXIT_PUZZLE_DIM_DOOR_EVIL : ICN::BUTTON_EXIT_PUZZLE_DIM_DOOR_GOOD ), 0, 1 );

        const fheroes2::Rect & radarRect = radar.GetRect();

        const std::function<fheroes2::Rect()> drawControlPanel = [&display, isEvilInterface, isHideInterface, &radarRect, &radarArea, &buttonExit]() {
            if ( isHideInterface ) {
                Dialog::FrameBorder::RenderRegular( radarRect );
            }

            fheroes2::Copy( fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::EVIWPUZL : ICN::VIEWPUZL ), 0 ), 0, 0, display, radarArea );
            display.updateNextRenderRoi( radarArea );

            buttonExit.draw();

            return radarRect;
        };

        drawPuzzle( pzl, sf, puzzleArea.x, puzzleArea.y );
        drawControlPanel();

        display.updateNextRenderRoi( border.totalArea() );
        fheroes2::fadeInDisplay( border.activeArea(), true );

        const bool earlyExit = revealPuzzle( pzl, sf, puzzleArea.x, puzzleArea.y, buttonExit, isHideInterface ? &drawControlPanel : nullptr );

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() && !earlyExit ) {
            le.isMouseLeftButtonPressedInArea( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
            if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
                break;
            }
        }

        fheroes2::fadeOutDisplay( border.activeArea(), true );

        gameArea.SetUpdateCursor();
    }
}

Puzzle::Puzzle()
{
    Rand::Shuffle( zone1_order );
    Rand::Shuffle( zone2_order );
    Rand::Shuffle( zone3_order );
    Rand::Shuffle( zone4_order );
}

Puzzle & Puzzle::operator=( const char * str )
{
    while ( str && *str ) {
        *this <<= 1;
        if ( *str == 0x31 )
            set( 0 );
        ++str;
    }

    return *this;
}

void Puzzle::Update( uint32_t open_obelisk, uint32_t total_obelisk )
{
    const uint32_t open_puzzle = open_obelisk * numOfPuzzleTiles / total_obelisk;
    size_t need_puzzle = open_puzzle > count() ? open_puzzle - count() : 0;

    if ( need_puzzle && ClosedTilesExists( *this, zone1_order ) )
        ZoneOpenFirstTiles( *this, need_puzzle, zone1_order );

    if ( need_puzzle && ClosedTilesExists( *this, zone2_order ) )
        ZoneOpenFirstTiles( *this, need_puzzle, zone2_order );

    if ( need_puzzle && ClosedTilesExists( *this, zone3_order ) )
        ZoneOpenFirstTiles( *this, need_puzzle, zone3_order );

    if ( need_puzzle && ClosedTilesExists( *this, zone4_order ) )
        ZoneOpenFirstTiles( *this, need_puzzle, zone4_order );
}

void Puzzle::ShowMapsDialog() const
{
    const fheroes2::Image & sf = world.GetUltimateArtifact().GetPuzzleMapSurface();
    if ( sf.empty() )
        return;

    const fheroes2::Display & display = fheroes2::Display::instance();

    // Set the cursor image. After this dialog the Game Area will be shown, so it does not require a cursor restorer.
    Cursor::Get().SetThemes( Cursor::POINTER );

    // restore the original music on exit
    const AudioManager::MusicRestorer musicRestorer;

    AudioManager::PlayMusic( MUS::PUZZLE, Music::PlaybackMode::PLAY_ONCE );

    if ( display.isDefaultSize() && !Settings::Get().isHideInterfaceEnabled() ) {
        ShowStandardDialog( *this, sf );
    }
    else {
        ShowExtendedDialog( *this, sf );
    }
}

OStreamBase & operator<<( OStreamBase & stream, const Puzzle & pzl )
{
    stream << pzl.to_string<char, std::char_traits<char>, std::allocator<char>>();

    stream << static_cast<uint8_t>( pzl.zone1_order.size() );
    for ( const uint8_t tile : pzl.zone1_order ) {
        stream << tile;
    }

    stream << static_cast<uint8_t>( pzl.zone2_order.size() );
    for ( const uint8_t tile : pzl.zone2_order ) {
        stream << tile;
    }

    stream << static_cast<uint8_t>( pzl.zone3_order.size() );
    for ( const uint8_t tile : pzl.zone3_order ) {
        stream << tile;
    }

    stream << static_cast<uint8_t>( pzl.zone4_order.size() );
    for ( const uint8_t tile : pzl.zone4_order ) {
        stream << tile;
    }

    return stream;
}

IStreamBase & operator>>( IStreamBase & stream, Puzzle & pzl )
{
    std::string str;

    stream >> str;
    pzl = str.c_str();

    uint8_t size{ 0 };

    stream >> size;
    pzl.zone1_order.resize( size );
    for ( uint8_t i = 0; i < size; ++i ) {
        stream >> pzl.zone1_order[i];
    }

    stream >> size;
    pzl.zone2_order.resize( size );
    for ( uint8_t i = 0; i < size; ++i ) {
        stream >> pzl.zone2_order[i];
    }

    stream >> size;
    pzl.zone3_order.resize( size );
    for ( uint8_t i = 0; i < size; ++i ) {
        stream >> pzl.zone3_order[i];
    }

    stream >> size;
    pzl.zone4_order.resize( size );
    for ( uint8_t i = 0; i < size; ++i ) {
        stream >> pzl.zone4_order[i];
    }

    return stream;
}
