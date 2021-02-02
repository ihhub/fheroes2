/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov                               *
 *   afletdinov@mail.dc.baikal.ru                                          *
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

#include "agg.h"
#include "battle_board.h"
#include "cursor.h"
#include "game.h"
#include "localevent.h"
#include "pocketpc.h"
#include "text.h"

u32 PocketPC::GetCursorAttackDialog( const Point & dst, int allow )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    const Rect rt( dst.x - 32, dst.y - 32, 86, 86 );

    const fheroes2::Sprite & sp_info = fheroes2::AGG::GetICN( ICN::CMSECO, 5 );
    const fheroes2::Sprite & sp_bleft = fheroes2::AGG::GetICN( ICN::CMSECO, 10 );
    const fheroes2::Sprite & sp_left = fheroes2::AGG::GetICN( ICN::CMSECO, 11 );
    const fheroes2::Sprite & sp_tleft = fheroes2::AGG::GetICN( ICN::CMSECO, 12 );
    const fheroes2::Sprite & sp_tright = fheroes2::AGG::GetICN( ICN::CMSECO, 7 );
    const fheroes2::Sprite & sp_right = fheroes2::AGG::GetICN( ICN::CMSECO, 8 );
    const fheroes2::Sprite & sp_bright = fheroes2::AGG::GetICN( ICN::CMSECO, 9 );

    fheroes2::Image shadow( rt.w, rt.h );
    shadow.fill( 0 );

    fheroes2::ImageRestorer back( display, rt.x, rt.y, rt.w, rt.h );

    Cursor & cursor = Cursor::Get();
    cursor.SetThemes( Cursor::POINTER );

    // blit alpha
    fheroes2::AlphaBlit( shadow, display, rt.x, rt.y, 120 );

    const fheroes2::Rect rt_info( rt.x + ( rt.w - sp_info.width() ) / 2, rt.y + ( rt.h - sp_info.height() ) / 2, sp_info.width(), sp_info.height() );
    fheroes2::Blit( sp_info, display, rt_info.x, rt_info.y );

    const fheroes2::Rect rt_tright( rt.x + 1, rt.y + rt.h - 1 - sp_tright.height(), sp_tright.width(), sp_tright.height() );
    if ( allow & Battle::BOTTOM_LEFT )
        fheroes2::Blit( sp_tright, display, rt_tright.x, rt_tright.y );

    const fheroes2::Rect rt_right( rt.x + 1, rt.y + ( rt.h - sp_right.height() ) / 2, sp_right.width(), sp_right.height() );
    if ( allow & Battle::LEFT )
        fheroes2::Blit( sp_right, display, rt_right.x, rt_right.y );

    const fheroes2::Rect rt_bright( rt.x + 1, rt.y + 1, sp_bright.width(), sp_bright.height() );
    if ( allow & Battle::TOP_LEFT )
        fheroes2::Blit( sp_bright, display, rt_bright.x, rt_bright.y );

    const fheroes2::Rect rt_tleft( rt.x + rt.w - 1 - sp_tleft.width(), rt.y + rt.h - 1 - sp_tleft.height(), sp_tleft.width(), sp_tleft.height() );
    if ( allow & Battle::BOTTOM_RIGHT )
        fheroes2::Blit( sp_tleft, display, rt_tleft.x, rt_tleft.y );

    const fheroes2::Rect rt_left( rt.x + rt.w - 1 - sp_left.width(), rt.y + ( rt.h - sp_left.height() ) / 2, sp_left.width(), sp_left.height() );
    if ( allow & Battle::RIGHT )
        fheroes2::Blit( sp_left, display, rt_left.x, rt_left.y );

    const fheroes2::Rect rt_bleft( rt.x + rt.w - 1 - sp_bleft.width(), rt.y + 1, sp_bleft.width(), sp_bleft.height() );
    if ( allow & Battle::TOP_RIGHT )
        fheroes2::Blit( sp_bleft, display, rt_bleft.x, rt_bleft.y );

    display.render();

    while ( le.HandleEvents() && !le.MouseClickLeft() )
        ;

    const Point & mousePos = le.GetMouseCursor();
    const fheroes2::Point position( mousePos.x, mousePos.y );

    if ( ( allow & Battle::BOTTOM_LEFT ) && ( rt_tright & position ) )
        return Cursor::SWORD_TOPRIGHT;
    else if ( ( allow & Battle::LEFT ) && ( rt_right & position ) )
        return Cursor::SWORD_RIGHT;
    else if ( ( allow & Battle::TOP_LEFT ) && ( rt_bright & position ) )
        return Cursor::SWORD_BOTTOMRIGHT;
    else if ( ( allow & Battle::BOTTOM_RIGHT ) && ( rt_tleft & position ) )
        return Cursor::SWORD_TOPLEFT;
    else if ( ( allow & Battle::RIGHT ) && ( rt_left & position ) )
        return Cursor::SWORD_LEFT;
    else if ( ( allow & Battle::TOP_RIGHT ) && ( rt_bleft & position ) )
        return Cursor::SWORD_BOTTOMLEFT;

    return Cursor::WAR_INFO;
}

fheroes2::Image CreateTouchButton( void )
{
    fheroes2::Image sf( 24, 24 );

    const int32_t ww = sf.width() / 2;
    const int32_t hh = sf.height() / 2;

    const fheroes2::Sprite & sp0 = fheroes2::AGG::GetICN( ICN::LOCATORS, 22 );

    fheroes2::Blit( sp0, 0, 0, sf, 0, 0, ww, hh );
    fheroes2::Blit( sp0, sp0.width() - ww, 0, sf, ww, 0, ww, hh );
    fheroes2::Blit( sp0, 0, sp0.height() - hh, sf, 0, hh, ww, hh );
    fheroes2::Blit( sp0, sp0.width() - ww, sp0.height() - hh, sf, ww, hh, ww, hh );

    return sf;
}

void RedrawTouchButton( const fheroes2::Image & sf, const fheroes2::Rect & rt, const char * lb )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    if ( sf.width() != rt.width ) {
        const u32 ww = 4;
        fheroes2::Blit( sf, 0, 0, display, rt.x, rt.y, ww, sf.height() );

        if ( rt.width > 8 ) {
            const u32 count = ( rt.width - ww ) / ww;
            for ( u32 ii = 0; ii < count; ++ii )
                fheroes2::Blit( sf, ww, 0, display, rt.x + ww * ( ii + 1 ), rt.y, ww, sf.height() );
        }

        fheroes2::Blit( sf, sf.width() - ww, 0, display, rt.x + rt.width - ww, rt.y, ww, sf.height() );
    }
    else {
        fheroes2::Blit( sf, display, rt.x, rt.y );
    }

    if ( lb ) {
        Text tx( lb, Font::BIG );
        tx.Blit( rt.x + ( rt.width - tx.w() ) / 2, rt.y + ( rt.height - tx.h() ) / 2 );
    }
}

void PocketPC::KeyboardDialog( std::string & str )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    const u32 width = 337;
    const u32 height = 118;

    fheroes2::ImageRestorer back( display, ( display.width() - width ) / 2, 0, width, height );
    const fheroes2::Rect top( back.x(), back.y(), back.width(), back.height() );
    fheroes2::Fill( display, back.x(), back.y(), back.width(), back.height(), 0 );

    const fheroes2::Image sp = CreateTouchButton();

    // 1 row
    const fheroes2::Rect rt_1( top.x + 2, top.y + 2, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_1, "1" );

    const fheroes2::Rect rt_2( rt_1.x + rt_1.width + 1, top.y + 2, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_2, "2" );

    const fheroes2::Rect rt_3( rt_2.x + rt_2.width + 1, top.y + 2, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_3, "3" );

    const fheroes2::Rect rt_4( rt_3.x + rt_3.width + 1, top.y + 2, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_4, "4" );

    const fheroes2::Rect rt_5( rt_4.x + rt_4.width + 1, top.y + 2, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_5, "5" );

    const fheroes2::Rect rt_6( rt_5.x + rt_5.width + 1, top.y + 2, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_6, "6" );

    const fheroes2::Rect rt_7( rt_6.x + rt_6.width + 1, top.y + 2, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_7, "7" );

    const fheroes2::Rect rt_8( rt_7.x + rt_7.width + 1, top.y + 2, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_8, "8" );

    const fheroes2::Rect rt_9( rt_8.x + rt_8.width + 1, top.y + 2, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_9, "9" );

    const fheroes2::Rect rt_0( rt_9.x + rt_9.width + 1, top.y + 2, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_0, "0" );

    const fheroes2::Rect rt_MINUS( rt_0.x + rt_0.width + 1, top.y + 2, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_MINUS, "-" );

    const fheroes2::Rect rt_BACKSPACE( rt_MINUS.x + rt_MINUS.width + 1, top.y + 2, 58, sp.height() );
    RedrawTouchButton( sp, rt_BACKSPACE, "back" );

    // 2 row
    const fheroes2::Rect rt_EMPTY1( top.x + 2, top.y + 27, 8, sp.height() );
    RedrawTouchButton( sp, rt_EMPTY1, NULL );

    const fheroes2::Rect rt_Q( rt_EMPTY1.x + rt_EMPTY1.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_Q, "q" );

    const fheroes2::Rect rt_W( rt_Q.x + rt_Q.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_W, "w" );

    const fheroes2::Rect rt_E( rt_W.x + rt_W.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_E, "e" );

    const fheroes2::Rect rt_R( rt_E.x + rt_E.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_R, "r" );

    const fheroes2::Rect rt_T( rt_R.x + rt_R.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_T, "t" );

    const fheroes2::Rect rt_Y( rt_T.x + rt_T.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_Y, "y" );

    const fheroes2::Rect rt_U( rt_Y.x + rt_Y.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_U, "u" );

    const fheroes2::Rect rt_I( rt_U.x + rt_U.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_I, "i" );

    const fheroes2::Rect rt_O( rt_I.x + rt_I.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_O, "o" );

    const fheroes2::Rect rt_P( rt_O.x + rt_O.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_P, "p" );

    const fheroes2::Rect rt_LB( rt_P.x + rt_P.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_LB, "[" );

    const fheroes2::Rect rt_RB( rt_LB.x + rt_LB.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_RB, "]" );

    const fheroes2::Rect rt_EQUAL( rt_RB.x + rt_RB.width + 1, top.y + 27, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_EQUAL, "=" );

    // 3 row
    const fheroes2::Rect rt_EMPTY3( top.x + 2, top.y + 52, 15, sp.height() );
    RedrawTouchButton( sp, rt_EMPTY3, NULL );

    const fheroes2::Rect rt_A( rt_EMPTY3.x + rt_EMPTY3.width + 1, top.y + 52, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_A, "a" );

    const fheroes2::Rect rt_S( rt_A.x + rt_A.width + 1, top.y + 52, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_S, "s" );

    const fheroes2::Rect rt_D( rt_S.x + rt_S.width + 1, top.y + 52, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_D, "d" );

    const fheroes2::Rect rt_F( rt_D.x + rt_D.width + 1, top.y + 52, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_F, "f" );

    const fheroes2::Rect rt_G( rt_F.x + rt_F.width + 1, top.y + 52, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_G, "g" );

    const fheroes2::Rect rt_H( rt_G.x + rt_G.width + 1, top.y + 52, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_H, "h" );

    const fheroes2::Rect rt_J( rt_H.x + rt_H.width + 1, top.y + 52, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_J, "j" );

    const fheroes2::Rect rt_K( rt_J.x + rt_J.width + 1, top.y + 52, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_K, "k" );

    const fheroes2::Rect rt_L( rt_K.x + rt_K.width + 1, top.y + 52, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_L, "l" );

    const fheroes2::Rect rt_SP( rt_L.x + rt_L.width + 1, top.y + 52, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_SP, ";" );

    const fheroes2::Rect rt_CM( rt_SP.x + rt_SP.width + 1, top.y + 52, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_CM, "'" );

    const fheroes2::Rect rt_RETURN( rt_CM.x + rt_CM.width + 1, top.y + 52, 42, sp.height() );
    RedrawTouchButton( sp, rt_RETURN, "rtrn" );

    // 4 row
    const fheroes2::Rect rt_EMPTY5( top.x + 2, top.y + 77, 26, sp.height() );
    RedrawTouchButton( sp, rt_EMPTY5, NULL );

    const fheroes2::Rect rt_Z( rt_EMPTY5.x + rt_EMPTY5.width + 1, top.y + 77, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_Z, "z" );

    const fheroes2::Rect rt_X( rt_Z.x + rt_Z.width + 1, top.y + 77, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_X, "x" );

    const fheroes2::Rect rt_C( rt_X.x + rt_X.width + 1, top.y + 77, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_C, "c" );

    const fheroes2::Rect rt_V( rt_C.x + rt_C.width + 1, top.y + 77, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_V, "v" );

    const fheroes2::Rect rt_B( rt_V.x + rt_V.width + 1, top.y + 77, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_B, "b" );

    const fheroes2::Rect rt_N( rt_B.x + rt_B.width + 1, top.y + 77, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_N, "n" );

    const fheroes2::Rect rt_M( rt_N.x + rt_N.width + 1, top.y + 77, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_M, "m" );

    const fheroes2::Rect rt_CS( rt_M.x + rt_M.width + 1, top.y + 77, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_CS, "," );

    const fheroes2::Rect rt_DT( rt_CS.x + rt_CS.width + 1, top.y + 77, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_DT, "." );

    const fheroes2::Rect rt_SL( rt_DT.x + rt_DT.width + 1, top.y + 77, sp.width(), sp.height() );
    RedrawTouchButton( sp, rt_SL, "/" );

    const fheroes2::Rect rt_SPACE( rt_SL.x + rt_SL.width + 1, top.y + 77, 56, sp.height() );
    RedrawTouchButton( sp, rt_SPACE, "space" );

    display.render();

    bool redraw = true;

    // mainmenu loop
    while ( le.HandleEvents() ) {
        if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
            str.clear();
            break;
        }

        char ch = 0;

        if ( le.MouseClickLeft( rt_0 ) )
            ch = '0';
        else if ( le.MouseClickLeft( rt_1 ) )
            ch = '1';
        else if ( le.MouseClickLeft( rt_2 ) )
            ch = '2';
        else if ( le.MouseClickLeft( rt_3 ) )
            ch = '3';
        else if ( le.MouseClickLeft( rt_4 ) )
            ch = '4';
        else if ( le.MouseClickLeft( rt_5 ) )
            ch = '5';
        else if ( le.MouseClickLeft( rt_6 ) )
            ch = '6';
        else if ( le.MouseClickLeft( rt_7 ) )
            ch = '7';
        else if ( le.MouseClickLeft( rt_8 ) )
            ch = '8';
        else if ( le.MouseClickLeft( rt_9 ) )
            ch = '9';
        else if ( le.MouseClickLeft( rt_A ) )
            ch = 'a';
        else if ( le.MouseClickLeft( rt_B ) )
            ch = 'b';
        else if ( le.MouseClickLeft( rt_C ) )
            ch = 'c';
        else if ( le.MouseClickLeft( rt_D ) )
            ch = 'd';
        else if ( le.MouseClickLeft( rt_E ) )
            ch = 'e';
        else if ( le.MouseClickLeft( rt_F ) )
            ch = 'f';
        else if ( le.MouseClickLeft( rt_G ) )
            ch = 'g';
        else if ( le.MouseClickLeft( rt_H ) )
            ch = 'h';
        else if ( le.MouseClickLeft( rt_I ) )
            ch = 'i';
        else if ( le.MouseClickLeft( rt_J ) )
            ch = 'j';
        else if ( le.MouseClickLeft( rt_K ) )
            ch = 'k';
        else if ( le.MouseClickLeft( rt_L ) )
            ch = 'l';
        else if ( le.MouseClickLeft( rt_M ) )
            ch = 'm';
        else if ( le.MouseClickLeft( rt_N ) )
            ch = 'n';
        else if ( le.MouseClickLeft( rt_O ) )
            ch = 'o';
        else if ( le.MouseClickLeft( rt_P ) )
            ch = 'p';
        else if ( le.MouseClickLeft( rt_Q ) )
            ch = 'q';
        else if ( le.MouseClickLeft( rt_R ) )
            ch = 'r';
        else if ( le.MouseClickLeft( rt_S ) )
            ch = 's';
        else if ( le.MouseClickLeft( rt_T ) )
            ch = 't';
        else if ( le.MouseClickLeft( rt_U ) )
            ch = 'u';
        else if ( le.MouseClickLeft( rt_V ) )
            ch = 'v';
        else if ( le.MouseClickLeft( rt_W ) )
            ch = 'w';
        else if ( le.MouseClickLeft( rt_X ) )
            ch = 'x';
        else if ( le.MouseClickLeft( rt_Y ) )
            ch = 'y';
        else if ( le.MouseClickLeft( rt_Z ) )
            ch = 'z';
        else if ( le.MouseClickLeft( rt_EQUAL ) )
            ch = '=';
        else if ( le.MouseClickLeft( rt_MINUS ) )
            ch = '-';
        else if ( le.MouseClickLeft( rt_LB ) )
            ch = '[';
        else if ( le.MouseClickLeft( rt_RB ) )
            ch = ']';
        else if ( le.MouseClickLeft( rt_SP ) )
            ch = ';';
        else if ( le.MouseClickLeft( rt_CM ) )
            ch = '\'';
        else if ( le.MouseClickLeft( rt_CS ) )
            ch = ',';
        else if ( le.MouseClickLeft( rt_DT ) )
            ch = '.';
        else if ( le.MouseClickLeft( rt_SL ) )
            ch = '/';
        else if ( le.MouseClickLeft( rt_SPACE ) )
            ch = 0x20;

        if ( le.MouseClickLeft( rt_BACKSPACE ) && str.size() ) {
            str.resize( str.size() - 1 );
            redraw = true;
        }
        else if ( le.MouseClickLeft( rt_RETURN ) )
            break;
        else if ( ch ) {
            str += ch;
            redraw = true;
        }

        if ( redraw ) {
            Text tx( str, Font::SMALL );
            if ( tx.w() < top.width ) {
                fheroes2::Fill( display, top.x, top.y + top.height - 16, top.width, 16, 0 );
                tx.Blit( top.x + ( top.width - tx.w() ) / 2, top.y + top.height - 16 + 2 );
                display.render();
            }
            redraw = false;
        }
    }

    back.restore();
    display.render();
}
