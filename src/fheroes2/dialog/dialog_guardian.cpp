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

#include "agg_image.h"
#include "army_bar.h"
#include "army_troop.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "heroes.h"
#include "heroes_indicator.h"
#include "icn.h"
#include "text.h"
#include "ui_button.h"
#include "ui_window.h"
#include "world.h"

class ArmyCell : public fheroes2::Rect
{
public:
    ArmyCell( const Troop & t, const fheroes2::Point & pt, const bool ro )
        : fheroes2::Rect( pt.x, pt.y, 43, 53 )
        , troop( t )
        , select( false )
        , readonly( ro )
    {
        const fheroes2::Sprite & backSprite = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );
        back.resize( width, height );
        fheroes2::Copy( backSprite, 36, 267, back, 0, 0, width, height );

        curs.resize( width, height - 10 );
        curs.reset();
        fheroes2::DrawBorder( curs, fheroes2::GetColorId( 0xc0, 0x2c, 0 ) );
    }

    void Redraw() const
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::Blit( back, display, x, y );
        if ( troop.isValid() ) {
            const fheroes2::Sprite & mons32 = fheroes2::AGG::GetICN( ICN::MONS32, troop.GetSpriteIndex() );
            fheroes2::Blit( mons32, display, x + ( back.width() - mons32.width() ) / 2, y + back.height() - mons32.height() - 11 );

            if ( readonly )
                fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LOCATORS, 24 ), display, x + 33, y + 5 );

            Text text( std::to_string( troop.GetCount() ), Font::SMALL );
            text.Blit( x + ( back.width() - text.w() ) / 2, y + back.height() - 11 );
        }

        if ( select )
            fheroes2::Blit( curs, display, x, y );
    }

    const Troop & troop;
    bool select;
    fheroes2::Image back;
    fheroes2::Image curs;
    bool readonly;
};

class ArmySplit
{
public:
    ArmySplit( const fheroes2::Point & pt, CapturedObject & co )
        : cobj( co )
        , rt1( pt.x + 140, pt.y + 19, 20, 10 )
        , rt2( pt.x + 140, pt.y + 33, 20, 10 )
        , rt3( pt.x + 140, pt.y + 47, 20, 10 )
    {}

    void Redraw( const Troop & troop )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        Text txt1( "/1", Font::SMALL );
        Text txt2( "/3", Font::SMALL );
        Text txt3( "/5", Font::SMALL );

        const fheroes2::Sprite & sp = fheroes2::AGG::GetICN( ICN::CAMPXTRG, 8 );
        fheroes2::Blit( sp, display, rt1.x, rt1.y );
        fheroes2::Blit( sp, display, rt2.x, rt2.y );
        fheroes2::Blit( sp, display, rt3.x, rt3.y );

        if ( troop.isValid() ) {
            const fheroes2::Sprite & cr = fheroes2::AGG::GetICN( ICN::CELLWIN, 5 );

            switch ( cobj.GetSplit() ) {
            case 3:
                fheroes2::Blit( cr, display, rt2.x + 1, rt2.y + 1 );
                break;
            case 5:
                fheroes2::Blit( cr, display, rt3.x + 1, rt3.y + 1 );
                break;
            default:
                fheroes2::Blit( cr, display, rt1.x + 1, rt1.y + 1 );
                break;
            }
        }
        else if ( 1 != cobj.GetSplit() )
            cobj.SetSplit( 1 );

        txt1.Blit( rt1.x + 14, rt1.y + 1 );
        txt2.Blit( rt2.x + 14, rt2.y + 1 );
        txt3.Blit( rt3.x + 14, rt3.y + 1 );
    }

    bool QueueProcessing( LocalEvent & le, const Troop & troop )
    {
        if ( le.MouseClickLeft( rt1 ) && 1 != cobj.GetSplit() ) {
            cobj.SetSplit( 1 );
            return true;
        }
        else if ( le.MouseClickLeft( rt2 ) && 3 != cobj.GetSplit() && troop.GetCount() >= 3 ) {
            cobj.SetSplit( 3 );
            return true;
        }
        else if ( le.MouseClickLeft( rt3 ) && 5 != cobj.GetSplit() && troop.GetCount() >= 5 ) {
            cobj.SetSplit( 5 );
            return true;
        }
        return false;
    }

    CapturedObject & cobj;

    const fheroes2::Rect rt1;
    const fheroes2::Rect rt2;
    const fheroes2::Rect rt3;
};

bool Dialog::SetGuardian( Heroes & hero, Troop & troop, CapturedObject & co, bool readonly )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::StandardWindow frameborder( 230, 160 );
    const fheroes2::Rect area( frameborder.activeArea() );

    // portrait
    const fheroes2::Sprite & window = fheroes2::AGG::GetICN( ICN::BRCREST, 6 );

    fheroes2::Point dst_pt( area.x + 3, area.y + 5 );
    fheroes2::Blit( window, display, dst_pt.x, dst_pt.y );

    const fheroes2::Sprite & port = hero.GetPortrait( PORT_MEDIUM );
    if ( !port.empty() )
        fheroes2::Blit( port, display, dst_pt.x + 4, dst_pt.y + 4 );

    // indicators
    dst_pt.x = area.x + 185;
    dst_pt.y = area.y + 5;
    MoraleIndicator moraleIndicator( &hero );
    moraleIndicator.SetPos( dst_pt );
    moraleIndicator.Redraw();

    dst_pt.x = area.x + 185;
    dst_pt.y = area.y + 35;
    LuckIndicator luckIndicator( &hero );
    luckIndicator.SetPos( dst_pt );
    luckIndicator.Redraw();

    // army bar
    dst_pt.x = area.x + 3;
    dst_pt.y = area.y + 73;

    ArmyBar selectArmy( &hero.GetArmy(), true, false );
    selectArmy.SetColRows( 5, 1 );
    selectArmy.SetPos( dst_pt.x, dst_pt.y );
    selectArmy.SetHSpace( 2 );
    selectArmy.Redraw();

    // guardian
    dst_pt.x = area.x + 93;
    dst_pt.y = area.y + 17;

    ArmyCell guardian( troop, dst_pt, readonly );
    guardian.Redraw();

    // label
    Text text( _( "Set Guardian" ), Font::SMALL );
    text.Blit( area.x + ( area.width - text.w() ) / 2, area.y + 3 );

    ArmySplit armySplit( area.getPosition(), co );
    armySplit.Redraw( troop );

    fheroes2::ButtonGroup btnGroups( area, Dialog::OK );
    btnGroups.draw();

    const Troop shadow( troop );

    display.render();

    // message loop
    int buttons = Dialog::ZERO;
    while ( buttons == Dialog::ZERO && le.HandleEvents() ) {
        buttons = btnGroups.processEvents();

        bool needRedraw = false;

        if ( le.MouseCursor( selectArmy.GetArea() ) ) {
            if ( guardian.select && le.MouseClickLeft( selectArmy.GetArea() ) ) {
                Troop * troop1 = selectArmy.GetItem( le.GetMouseCursor() );

                if ( troop1 ) {
                    // combine
                    if ( troop() == troop1->GetID() ) {
                        troop1->SetCount( troop.GetCount() + troop1->GetCount() );
                        troop.Reset();
                    }
                    else if ( troop1->GetCount() >= MAXU16 )
                        Dialog::Message( "", _( "Your army too big!" ), Font::BIG, Dialog::OK );
                    // swap
                    else {
                        Army::SwapTroops( *troop1, troop );
                    }
                }

                guardian.select = false;

                needRedraw = true;
            }
            else if ( selectArmy.QueueEventProcessing() ) {
                guardian.select = false;
                selectArmy.Redraw();

                needRedraw = true;
            }
        }
        else if ( le.MouseCursor( moraleIndicator.GetArea() ) )
            MoraleIndicator::QueueEventProcessing( moraleIndicator );
        else if ( le.MouseCursor( luckIndicator.GetArea() ) )
            LuckIndicator::QueueEventProcessing( luckIndicator );
        else if ( le.MouseClickLeft( guardian ) ) {
            if ( guardian.select ) {
                Dialog::ArmyInfo( troop, Dialog::READONLY | Dialog::BUTTONS );

                needRedraw = true;
            }
            else if ( selectArmy.isSelected() && !readonly && !hero.GetArmy().SaveLastTroop() ) {
                Troop * troop1 = selectArmy.GetSelectedItem();

                if ( troop1 ) {
                    // combine
                    if ( troop() == troop1->GetID() ) {
                        if ( troop1->GetCount() + troop.GetCount() < MAXU16 ) {
                            troop.SetCount( troop1->GetCount() + troop.GetCount() );
                            troop1->Reset();
                        }
                        else {
                            troop1->SetCount( troop1->GetCount() + troop.GetCount() - MAXU16 );
                            troop.SetCount( MAXU16 );
                        }
                    }
                    else if ( troop1->GetCount() >= MAXU16 )
                        Dialog::Message( "", _( "Your army too big!" ), Font::BIG, Dialog::OK );
                    // swap
                    else {
                        Army::SwapTroops( *troop1, troop );
                    }
                }

                selectArmy.ResetSelected();

                needRedraw = true;
            }
            else
                // select
                if ( troop.isValid() && !readonly ) {
                selectArmy.ResetSelected();
                guardian.select = true;

                needRedraw = true;
            }
        }
        else if ( le.MousePressRight( guardian ) && troop.isValid() ) {
            selectArmy.ResetSelected();
            Dialog::ArmyInfo( troop, 0 );

            needRedraw = true;
        }
        else if ( armySplit.QueueProcessing( le, troop ) )
            needRedraw = true;

        if ( !needRedraw ) {
            continue;
        }

        guardian.Redraw();
        moraleIndicator.Redraw();
        luckIndicator.Redraw();
        selectArmy.Redraw();
        armySplit.Redraw( troop );
        display.render();
    }

    return shadow() != troop() || shadow.GetCount() != troop.GetCount();
}
