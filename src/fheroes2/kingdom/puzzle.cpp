/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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
#include "gamedefs.h"
#include "icn.h"
#include "image.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "localevent.h"
#include "logging.h"
#include "math_base.h"
#include "mus.h"
#include "puzzle.h"
#include "rand.h"
#include "screen.h"
#include "serialize.h"
#include "settings.h"
#include "ui_button.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    bool ClosedTilesExists( const Puzzle & pzl, const std::vector<uint8_t> & zone )
    {
        for ( const uint8_t tile : zone ) {
            if ( !pzl.test( tile ) )
                return true;
        }
        return false;
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

    void PuzzlesDraw( const Puzzle & pzl, const fheroes2::Image & sf, int32_t dstx, int32_t dsty, const std::function<fheroes2::Rect()> * drawControlPanel = nullptr )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        // show all for debug
        if ( IS_DEVEL() )
            return;

        int alpha = 250;
        LocalEvent & le = LocalEvent::Get();

        const std::vector<Game::DelayType> delayTypes = { Game::PUZZLE_FADE_DELAY };
        Game::passAnimationDelay( Game::PUZZLE_FADE_DELAY );

        while ( alpha >= 0 && le.HandleEvents( Game::isDelayNeeded( delayTypes ) ) ) {
            if ( Game::validateAnimationDelay( Game::PUZZLE_FADE_DELAY ) ) {
                fheroes2::Blit( sf, display, dstx, dsty );

                for ( size_t i = 0; i < pzl.size(); ++i ) {
                    const fheroes2::Sprite & piece = fheroes2::AGG::GetICN( ICN::PUZZLE, static_cast<uint32_t>( i ) );

                    uint8_t pieceAlpha = 255;
                    if ( pzl.test( i ) )
                        pieceAlpha = static_cast<uint8_t>( alpha );

                    fheroes2::AlphaBlit( piece, display, dstx + piece.x() - BORDERWIDTH, dsty + piece.y() - BORDERWIDTH, pieceAlpha );
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
    }

    void ShowStandardDialog( const Puzzle & pzl, const fheroes2::Image & sf )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        const Interface::Radar & radar = Interface::Basic::Get().GetRadar();
        const fheroes2::Rect & radarArea = radar.GetArea();

        fheroes2::ImageRestorer back( display, BORDERWIDTH, BORDERWIDTH, sf.width(), sf.height() );

        fheroes2::Blit( fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::EVIWPUZL : ICN::VIEWPUZL ), 0 ), display, radarArea.x, radarArea.y );
        fheroes2::Blit( sf, display, BORDERWIDTH, BORDERWIDTH );

        fheroes2::Button buttonExit( radarArea.x + 32, radarArea.y + radarArea.height - 37, ( isEvilInterface ? ICN::LGNDXTRE : ICN::LGNDXTRA ), 4, 5 );
        buttonExit.draw();

        PuzzlesDraw( pzl, sf, BORDERWIDTH, BORDERWIDTH );

        display.render();

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
            if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() )
                break;
        }

        radar.SetRedraw();
    }

    void ShowExtendedDialog( const Puzzle & pzl, const fheroes2::Image & sf )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::Rect & gameArea = Interface::Basic::Get().GetGameArea().GetROI();

        const fheroes2::StandardWindow border( gameArea.x + ( gameArea.width - sf.width() - BORDERWIDTH * 2 ) / 2,
                                               gameArea.y + ( gameArea.height - sf.height() - BORDERWIDTH * 2 ) / 2, sf.width(), sf.height(), false );

        fheroes2::Rect blitArea = border.activeArea();

        fheroes2::Image background( blitArea.width, blitArea.height );

        const Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();
        const bool isHideInterface = conf.isHideInterfaceEnabled();

        if ( isEvilInterface ) {
            background.fill( fheroes2::GetColorId( 80, 80, 80 ) );
        }
        else {
            background.fill( fheroes2::GetColorId( 128, 64, 32 ) );
        }

        fheroes2::Blit( background, display, blitArea.x, blitArea.y );
        fheroes2::Blit( sf, display, blitArea.x, blitArea.y );

        const Interface::Radar & radar = Interface::Basic::Get().GetRadar();
        const fheroes2::Rect & radarRect = radar.GetRect();
        const fheroes2::Rect & radarArea = radar.GetArea();

        fheroes2::Button buttonExit( radarArea.x + 32, radarArea.y + radarArea.height - 37, ( isEvilInterface ? ICN::LGNDXTRE : ICN::LGNDXTRA ), 4, 5 );

        std::function<fheroes2::Rect()> drawControlPanel = [&display, isEvilInterface, isHideInterface, &radarRect, &radarArea, &buttonExit]() {
            if ( isHideInterface ) {
                Dialog::FrameBorder::RenderRegular( radarRect );
            }

            fheroes2::Blit( fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::EVIWPUZL : ICN::VIEWPUZL ), 0 ), display, radarArea.x, radarArea.y );

            buttonExit.draw();

            return radarRect;
        };

        drawControlPanel();

        PuzzlesDraw( pzl, sf, blitArea.x, blitArea.y, isHideInterface ? &drawControlPanel : nullptr );

        display.render();

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
            if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() )
                break;
        }

        radar.SetRedraw();
    }
}

Puzzle::Puzzle()
{
    zone1_order = { 0, 1, 2, 3, 4, 5, 6, 11, 12, 17, 18, 23, 24, 29, 30, 35, 36, 41, 42, 43, 44, 45, 46, 47 };
    zone2_order = { 7, 8, 9, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 38, 39, 40 };
    zone3_order = { 14, 15, 32, 33 };
    zone4_order = { 20, 21, 26, 27 };

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
    const uint32_t open_puzzle = open_obelisk * PUZZLETILES / total_obelisk;
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

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    // restore the original music on exit
    const AudioManager::MusicRestorer musicRestorer;

    AudioManager::PlayMusic( MUS::PUZZLE, Music::PlaybackMode::PLAY_ONCE );

    if ( display.isDefaultSize() && !Settings::Get().isHideInterfaceEnabled() )
        ShowStandardDialog( *this, sf );
    else
        ShowExtendedDialog( *this, sf );
}

StreamBase & operator<<( StreamBase & msg, const Puzzle & pzl )
{
    msg << pzl.to_string<char, std::char_traits<char>, std::allocator<char>>();

    // orders
    msg << static_cast<uint8_t>( pzl.zone1_order.size() );
    for ( const uint8_t tile : pzl.zone1_order )
        msg << tile;

    msg << static_cast<uint8_t>( pzl.zone2_order.size() );
    for ( const uint8_t tile : pzl.zone2_order )
        msg << tile;

    msg << static_cast<uint8_t>( pzl.zone3_order.size() );
    for ( const uint8_t tile : pzl.zone3_order )
        msg << tile;

    msg << static_cast<uint8_t>( pzl.zone4_order.size() );
    for ( const uint8_t tile : pzl.zone4_order )
        msg << tile;

    return msg;
}

StreamBase & operator>>( StreamBase & msg, Puzzle & pzl )
{
    std::string str;

    msg >> str;
    pzl = str.c_str();

    uint8_t size;

    msg >> size;
    pzl.zone1_order.resize( size );
    for ( uint8_t ii = 0; ii < size; ++ii )
        msg >> pzl.zone1_order[ii];

    msg >> size;
    pzl.zone2_order.resize( size );
    for ( uint8_t ii = 0; ii < size; ++ii )
        msg >> pzl.zone2_order[ii];

    msg >> size;
    pzl.zone3_order.resize( size );
    for ( uint8_t ii = 0; ii < size; ++ii )
        msg >> pzl.zone3_order[ii];

    msg >> size;
    pzl.zone4_order.resize( size );
    for ( uint8_t ii = 0; ii < size; ++ii )
        msg >> pzl.zone4_order[ii];

    return msg;
}
