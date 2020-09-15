/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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
#include <vector>

#include "agg.h"
#include "cursor.h"
#include "game.h"
#include "game_interface.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "mus.h"
#include "puzzle.h"
#include "settings.h"
#include "ui_button.h"
#include "world.h"

const u8 zone1_index[] = {0, 1, 2, 3, 4, 5, 6, 11, 12, 17, 18, 23, 24, 29, 30, 35, 36, 41, 42, 43, 44, 45, 46, 47};
const u8 zone2_index[] = {7, 8, 9, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 38, 39, 40};
const u8 zone3_index[] = {14, 15, 32, 33};
const u8 zone4_index[] = {20, 21, 26, 27};

bool ClosedTilesExists( const Puzzle &, const u8 *, const u8 * );
void ZoneOpenFirstTiles( Puzzle &, u32 &, const u8 *, const u8 * );
void ShowStandardDialog( const Puzzle &, const fheroes2::Image & );
void ShowExtendedDialog( const Puzzle &, const fheroes2::Image & );
void PuzzlesDraw( const Puzzle &, const fheroes2::Image &, s32, s32 );

Puzzle::Puzzle()
{
    std::copy( zone1_index, zone1_index + ARRAY_COUNT( zone1_index ), zone1_order );
    std::copy( zone2_index, zone2_index + ARRAY_COUNT( zone2_index ), zone2_order );
    std::copy( zone3_index, zone3_index + ARRAY_COUNT( zone3_index ), zone3_order );
    std::copy( zone4_index, zone4_index + ARRAY_COUNT( zone4_index ), zone4_order );

    std::random_shuffle( zone1_order, zone1_order + ARRAY_COUNT( zone1_order ) );
    std::random_shuffle( zone2_order, zone2_order + ARRAY_COUNT( zone2_order ) );
    std::random_shuffle( zone3_order, zone3_order + ARRAY_COUNT( zone3_order ) );
    std::random_shuffle( zone4_order, zone4_order + ARRAY_COUNT( zone4_order ) );
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

void Puzzle::Update( u32 open_obelisk, u32 total_obelisk )
{
    u32 open_puzzle = open_obelisk * PUZZLETILES / total_obelisk;
    u32 need_puzzle = open_puzzle > count() ? open_puzzle - count() : 0;

    if ( need_puzzle && ClosedTilesExists( *this, zone1_order, ARRAY_COUNT_END( zone1_order ) ) )
        ZoneOpenFirstTiles( *this, need_puzzle, zone1_order, ARRAY_COUNT_END( zone1_order ) );

    if ( need_puzzle && ClosedTilesExists( *this, zone2_order, ARRAY_COUNT_END( zone2_order ) ) )
        ZoneOpenFirstTiles( *this, need_puzzle, zone2_order, ARRAY_COUNT_END( zone2_order ) );

    if ( need_puzzle && ClosedTilesExists( *this, zone3_order, ARRAY_COUNT_END( zone3_order ) ) )
        ZoneOpenFirstTiles( *this, need_puzzle, zone3_order, ARRAY_COUNT_END( zone3_order ) );

    if ( need_puzzle && ClosedTilesExists( *this, zone4_order, ARRAY_COUNT_END( zone4_order ) ) )
        ZoneOpenFirstTiles( *this, need_puzzle, zone4_order, ARRAY_COUNT_END( zone4_order ) );
}

void Puzzle::ShowMapsDialog( void ) const
{
    Cursor & cursor = Cursor::Get();
    fheroes2::Display & display = fheroes2::Display::instance();
    int old_cursor = cursor.Themes();

    if ( !Settings::Get().MusicMIDI() )
        AGG::PlayMusic( MUS::PUZZLE );

    const fheroes2::Image & sf = world.GetUltimateArtifact().GetPuzzleMapSurface();

    if ( !sf.empty() ) {
        cursor.Hide();

        AGG::PlayMusic( MUS::PUZZLE, false );

        if ( display.isDefaultSize() && !Settings::Get().ExtGameHideInterface() )
            ShowStandardDialog( *this, sf );
        else
            ShowExtendedDialog( *this, sf );

        cursor.SetThemes( old_cursor );
    }
}

bool ClosedTilesExists( const Puzzle & pzl, const u8 * it1, const u8 * it2 )
{
    while ( it1 < it2 )
        if ( !pzl.test( *it1++ ) )
            return true;
    return false;
}

void ZoneOpenFirstTiles( Puzzle & pzl, u32 & opens, const u8 * it1, const u8 * it2 )
{
    while ( opens ) {
        const u8 * it = it1;
        while ( it < it2 && pzl.test( *it ) )
            ++it;

        if ( it != it2 ) {
            pzl.set( *it );
            --opens;
        }
        else
            break;
    }
}

void ShowStandardDialog( const Puzzle & pzl, const fheroes2::Image & sf )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();

    Interface::Radar & radar = Interface::Basic::Get().GetRadar();
    const Rect & radarPos = radar.GetArea();
    const bool isEvilInterface = Settings::Get().ExtGameEvilInterface();

    fheroes2::ImageRestorer back( display, BORDERWIDTH, BORDERWIDTH, sf.width(), sf.height() );

    fheroes2::Blit( fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::EVIWPUZL : ICN::VIEWPUZL ), 0 ), display, radarPos.x, radarPos.y );
    fheroes2::Blit( sf, display, BORDERWIDTH, BORDERWIDTH );

    Point dst_pt( radarPos.x + 32, radarPos.y + radarPos.h - 37 );
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ( isEvilInterface ? ICN::LGNDXTRE : ICN::LGNDXTRA ), 4, 5 );

    buttonExit.draw();
    PuzzlesDraw( pzl, sf, BORDERWIDTH, BORDERWIDTH );

    cursor.SetThemes( Cursor::POINTER );
    cursor.Show();
    display.render();
    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
            break;
    }

    radar.SetRedraw();

    cursor.Hide();
}

void ShowExtendedDialog( const Puzzle & pzl, const fheroes2::Image & sf )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    const Settings & conf = Settings::Get();
    const Rect & gameArea = Interface::Basic::Get().GetGameArea().GetROI();

    Dialog::FrameBorder frameborder( gameArea.x + ( gameArea.w - sf.width() - BORDERWIDTH * 2 ) / 2, gameArea.y + ( gameArea.h - sf.height() - BORDERWIDTH * 2 ) / 2,
                                     sf.width(), sf.height() );

    Rect blitArea = frameborder.GetArea();

    fheroes2::Image background( blitArea.w, blitArea.h );
    if ( conf.ExtGameEvilInterface() )
        background.fill( fheroes2::GetColorId( 80, 80, 80 ) );
    else
        background.fill( fheroes2::GetColorId( 128, 64, 32 ) );

    fheroes2::Blit( background, display, blitArea.x, blitArea.y );
    fheroes2::Blit( sf, display, blitArea.x, blitArea.y );

    Interface::Radar & radar = Interface::Basic::Get().GetRadar();
    const Rect & radarPos = radar.GetArea();
    const bool isEvilInterface = Settings::Get().ExtGameEvilInterface();

    fheroes2::Blit( fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::EVIWPUZL : ICN::VIEWPUZL ), 0 ), display, radarPos.x, radarPos.y );

    Point dst_pt( radarPos.x + 32, radarPos.y + radarPos.h - 37 );
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ( isEvilInterface ? ICN::LGNDXTRE : ICN::LGNDXTRA ), 4, 5 );

    buttonExit.draw();
    PuzzlesDraw( pzl, sf, blitArea.x, blitArea.y );

    cursor.SetThemes( Cursor::POINTER );
    cursor.Show();
    display.render();
    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
            break;
    }

    radar.SetRedraw();
}

void PuzzlesDraw( const Puzzle & pzl, const fheroes2::Image & sf, s32 dstx, s32 dsty )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();

    // show all for debug
    if ( IS_DEVEL() )
        return;

    int alpha = 250;
    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() && 0 < alpha ) {
        if ( Game::AnimateInfrequentDelay( Game::PUZZLE_FADE_DELAY ) ) {
            cursor.Hide();
            fheroes2::Blit( sf, display, dstx, dsty );
            for ( size_t ii = 0; ii < pzl.size(); ++ii ) {
                const fheroes2::Sprite & piece = fheroes2::AGG::GetICN( ICN::PUZZLE, ii );

                int pieceAlpha = 255;
                if ( pzl.test( ii ) )
                    pieceAlpha = alpha;

                fheroes2::AlphaBlit( piece, display, dstx + piece.x() - BORDERWIDTH, dsty + piece.y() - BORDERWIDTH, pieceAlpha );
            }
            cursor.Show();
            display.render();
            alpha -= 10;
        }
    }
    cursor.Hide();
}

StreamBase & operator<<( StreamBase & msg, const Puzzle & pzl )
{
    msg << pzl.to_string<char, std::char_traits<char>, std::allocator<char> >();

    // orders
    msg << static_cast<u8>( ARRAY_COUNT( pzl.zone1_order ) );
    for ( u32 ii = 0; ii < ARRAY_COUNT( pzl.zone1_order ); ++ii )
        msg << pzl.zone1_order[ii];

    msg << static_cast<u8>( ARRAY_COUNT( pzl.zone2_order ) );
    for ( u32 ii = 0; ii < ARRAY_COUNT( pzl.zone2_order ); ++ii )
        msg << pzl.zone2_order[ii];

    msg << static_cast<u8>( ARRAY_COUNT( pzl.zone3_order ) );
    for ( u32 ii = 0; ii < ARRAY_COUNT( pzl.zone3_order ); ++ii )
        msg << pzl.zone3_order[ii];

    msg << static_cast<u8>( ARRAY_COUNT( pzl.zone4_order ) );
    for ( u32 ii = 0; ii < ARRAY_COUNT( pzl.zone4_order ); ++ii )
        msg << pzl.zone4_order[ii];

    return msg;
}

StreamBase & operator>>( StreamBase & msg, Puzzle & pzl )
{
    std::string str;

    msg >> str;
    pzl = str.c_str();

    u8 size;

    msg >> size;
    for ( u32 ii = 0; ii < size; ++ii )
        msg >> pzl.zone1_order[ii];

    msg >> size;
    for ( u32 ii = 0; ii < size; ++ii )
        msg >> pzl.zone2_order[ii];

    msg >> size;
    for ( u32 ii = 0; ii < size; ++ii )
        msg >> pzl.zone3_order[ii];

    msg >> size;
    for ( u32 ii = 0; ii < size; ++ii )
        msg >> pzl.zone4_order[ii];

    return msg;
}
