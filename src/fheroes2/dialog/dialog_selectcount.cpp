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

#include "agg.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "pocketpc.h"
#include "settings.h"
#include "text.h"
#include "ui_button.h"

namespace
{
    void SwitchMaxMinButtons( fheroes2::ButtonBase & minButton, fheroes2::ButtonBase & maxButton, uint32_t currentValue, uint32_t maximumValue )
    {
        const bool isMaxValue = ( currentValue >= maximumValue );

        if ( isMaxValue ) {
            minButton.show();
            maxButton.hide();
        }
        else {
            minButton.hide();
            maxButton.show();
        }

        minButton.draw();
        maxButton.draw();
    }
}

class SelectValue : public Rect
{
public:
    SelectValue( u32 min, u32 max, u32 cur, u32 st )
        : vmin( min )
        , vmax( max )
        , vcur( cur )
        , step( st )
    {
        if ( vmin >= vmax )
            vmin = 0;
        if ( vcur > vmax || vcur < vmin )
            vcur = vmin;

        btnUp.setICNInfo( ICN::TOWNWIND, 5, 6 );
        btnDn.setICNInfo( ICN::TOWNWIND, 7, 8 );

        pos.width = 90;
        pos.height = 30;
    }

    u32 Min( void )
    {
        return vmin;
    }

    u32 Max( void )
    {
        return vmax;
    }

    void SetCur( u32 v )
    {
        vcur = v;
    }

    void SetPos( const fheroes2::Point & pt )
    {
        pos.x = pt.x;
        pos.y = pt.y;

        btnUp.setPosition( pt.x + 70, pt.y );
        btnDn.setPosition( pt.x + 70, pt.y + 16 );
    }

    u32 operator()( void ) const
    {
        return vcur;
    }

    void Redraw( void )
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Sprite & sprite_edit = fheroes2::AGG::GetICN( ICN::TOWNWIND, 4 );
        fheroes2::Blit( sprite_edit, display, pos.x, pos.y + 4 );

        Text text( GetString( vcur ), Font::BIG );
        text.Blit( pos.x + ( sprite_edit.width() - text.w() ) / 2, pos.y + 5 );

        btnUp.draw();
        btnDn.draw();
    }

    bool QueueEventProcessing( void )
    {
        LocalEvent & le = LocalEvent::Get();

        le.MousePressLeft( btnUp.area() ) ? btnUp.drawOnPress() : btnUp.drawOnRelease();
        le.MousePressLeft( btnDn.area() ) ? btnDn.drawOnPress() : btnDn.drawOnRelease();

        if ( ( le.MouseWheelUp( pos ) || le.MouseClickLeft( btnUp.area() ) ) && vcur < vmax ) {
            vcur += vcur + step <= vmax ? step : vmax - vcur;
            return true;
        }
        else
            // down
            if ( ( le.MouseWheelDn( pos ) || le.MouseClickLeft( btnDn.area() ) ) && vmin < vcur ) {
            vcur -= vmin + vcur >= step ? step : vcur;
            return true;
        }

        return false;
    }

protected:
    u32 vmin;
    u32 vmax;
    u32 vcur;
    u32 step;

    fheroes2::Rect pos;

    fheroes2::Button btnUp;
    fheroes2::Button btnDn;
};

bool Dialog::SelectCount( const std::string & header, u32 min, u32 max, u32 & cur, int step )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    Text text( header, Font::BIG );
    const int spacer = 10;

    FrameBox box( text.h() + spacer + 30, true );
    SelectValue sel( min, max, cur, step );

    const fheroes2::Rect & pos = box.GetArea();

    text.Blit( pos.x + ( pos.width - text.w() ) / 2, pos.y );

    sel.SetPos( fheroes2::Point( pos.x + 80, pos.y + 30 ) );
    sel.Redraw();

    fheroes2::ButtonGroup btnGroups( box.GetArea(), Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    text.Set( "MAX", Font::SMALL );
    const fheroes2::Rect rectMax( pos.x + 173, pos.y + 38, text.w(), text.h() );
    text.Blit( rectMax.x, rectMax.y );

    LocalEvent & le = LocalEvent::Get();

    bool redraw_count = false;
    cursor.Show();
    display.render();

    // message loop
    int result = Dialog::ZERO;
    while ( result == Dialog::ZERO && le.HandleEvents() ) {
        if ( PressIntKey( max, cur ) ) {
            sel.SetCur( cur );
            redraw_count = true;
        }

        // max
        if ( le.MouseClickLeft( rectMax ) ) {
            sel.SetCur( max );
            redraw_count = true;
        }
        if ( sel.QueueEventProcessing() )
            redraw_count = true;

        if ( redraw_count ) {
            cursor.Hide();
            sel.Redraw();
            cursor.Show();
            display.render();

            redraw_count = false;
        }

        result = btnGroups.processEvents();
    }

    cur = result == Dialog::OK ? sel() : 0;

    return result == Dialog::OK;
}

bool Dialog::InputString( const std::string & header, std::string & res )
{
    const int system = Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM;

    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    int oldcursor = cursor.Themes();
    cursor.SetThemes( cursor.POINTER );

    if ( res.size() )
        res.clear();
    res.reserve( 48 );
    size_t charInsertPos = 0;

    TextBox textbox( header, Font::BIG, BOXAREA_WIDTH );
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ( Settings::Get().ExtGameEvilInterface() ? ICN::BUYBUILD : ICN::BUYBUILE ), 3 );

    FrameBox box( 10 + textbox.h() + 10 + sprite.height(), true );
    const fheroes2::Rect & box_rt = box.GetArea();

    // text
    fheroes2::Point dst_pt;
    dst_pt.x = box_rt.x + ( box_rt.width - textbox.w() ) / 2;
    dst_pt.y = box_rt.y + 10;
    textbox.Blit( dst_pt.x, dst_pt.y );

    dst_pt.y = box_rt.y + 10 + textbox.h() + 10;
    dst_pt.x = box_rt.x + ( box_rt.width - sprite.width() ) / 2;
    fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
    const fheroes2::Rect text_rt( dst_pt.x, dst_pt.y, sprite.width(), sprite.height() );

    Text text( "_", Font::BIG );
    fheroes2::Blit( sprite, display, text_rt.x, text_rt.y );
    text.Blit( dst_pt.x + ( sprite.width() - text.w() ) / 2, dst_pt.y - 1 );

    dst_pt.x = box_rt.x;
    dst_pt.y = box_rt.y + box_rt.height - fheroes2::AGG::GetICN( system, 1 ).height();
    fheroes2::Button buttonOk( dst_pt.x, dst_pt.y, system, 1, 2 );

    dst_pt.x = box_rt.x + box_rt.width - fheroes2::AGG::GetICN( system, 3 ).width();
    dst_pt.y = box_rt.y + box_rt.height - fheroes2::AGG::GetICN( system, 3 ).height();
    fheroes2::Button buttonCancel( dst_pt.x, dst_pt.y, system, 3, 4 );

    if ( res.empty() )
        buttonOk.disable();
    else
        buttonOk.enable();

    buttonOk.draw();
    buttonCancel.draw();

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();
    bool redraw = true;

    // message loop
    while ( le.HandleEvents() ) {
        buttonOk.isEnabled() && le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        if ( Settings::Get().PocketPC() && le.MousePressLeft( text_rt ) ) {
            PocketPC::KeyboardDialog( res );
            redraw = true;
        }

        if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) || ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) )
            break;
        else if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancel.area() ) ) {
            res.clear();
            break;
        }
        else if ( le.KeyPress() ) {
            charInsertPos = InsertKeySym( res, charInsertPos, le.KeyValue(), le.KeyMod() );
            redraw = true;
        }

        if ( redraw ) {
            if ( res.empty() )
                buttonOk.disable();
            else
                buttonOk.enable();
            buttonOk.draw();

            text.Set( InsertString( res, charInsertPos, "_" ) );

            if ( text.w() < sprite.width() - 24 ) {
                cursor.Hide();
                fheroes2::Blit( sprite, display, text_rt.x, text_rt.y );
                text.Blit( text_rt.x + ( text_rt.width - text.w() ) / 2, text_rt.y + 1 );
                cursor.Show();
                display.render();
            }
            redraw = false;
        }
    }

    cursor.SetThemes( oldcursor );
    cursor.Hide();

    return !res.empty();
}

int Dialog::ArmySplitTroop( int free_slots, u32 max, u32 & cur, bool savelast )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    const u32 min = 1;
    const int spacer = 10;

    const int defaultYPosition = 160;
    const int boxHeight = free_slots > 2 ? 90 + spacer : 45;
    const int boxYPosition = defaultYPosition + ( ( display.height() - display.DEFAULT_HEIGHT ) / 2 ) - boxHeight;

    NonFixedFrameBox box( boxHeight, boxYPosition, true );
    SelectValue sel( min, max, cur, 1 );
    Text text;

    const fheroes2::Rect & pos = box.GetArea();
    const int center = pos.x + pos.width / 2;

    text.Set( _( "How many troops to move?" ), Font::BIG );
    text.Blit( center - text.w() / 2, pos.y );

    sel.SetPos( fheroes2::Point( pos.x + 70, pos.y + 30 ) );
    sel.Redraw();

    fheroes2::MovableSprite ssp;
    fheroes2::Image sp3;
    fheroes2::Image sp4;
    fheroes2::Image sp5;

    std::vector<fheroes2::Rect> vrts( 3 );

    fheroes2::Rect & rt3 = vrts[0];
    fheroes2::Rect & rt4 = vrts[1];
    fheroes2::Rect & rt5 = vrts[2];

    switch ( free_slots ) {
    case 0:
        break;

    case 3:
        sp3 = fheroes2::AGG::GetICN( ICN::REQUESTS, 22 );
        rt3 = fheroes2::Rect( center - sp3.width() / 2, pos.y + 95, sp3.width(), sp3.height() );
        break;

    case 4:
        sp3 = fheroes2::AGG::GetICN( ICN::REQUESTS, 22 );
        sp4 = fheroes2::AGG::GetICN( ICN::REQUESTS, 23 );
        rt3 = fheroes2::Rect( center - 5 - sp3.width(), pos.y + 95, sp3.width(), sp3.height() );
        rt4 = fheroes2::Rect( center + 5, pos.y + 95, sp4.width(), sp4.height() );
        break;

    case 5:
        sp3 = fheroes2::AGG::GetICN( ICN::REQUESTS, 22 );
        sp4 = fheroes2::AGG::GetICN( ICN::REQUESTS, 23 );
        sp5 = fheroes2::AGG::GetICN( ICN::REQUESTS, 24 );
        rt3 = fheroes2::Rect( center - sp3.width() / 2 - 10 - sp3.width(), pos.y + 95, sp3.width(), sp3.height() );
        rt4 = fheroes2::Rect( center - sp4.width() / 2, pos.y + 95, sp4.width(), sp4.height() );
        rt5 = fheroes2::Rect( center + sp5.width() / 2 + 10, pos.y + 95, sp5.width(), sp5.height() );
        break;
    }

    if ( !sp3.empty() ) {
        text.Set( _( "Fast separation into slots:" ), Font::BIG );
        text.Blit( center - text.w() / 2, pos.y + 65 );

        fheroes2::Blit( sp3, display, rt3.x, rt3.y );

        if ( !sp4.empty() )
            fheroes2::Blit( sp4, display, rt4.x, rt4.y );
        if ( !sp5.empty() )
            fheroes2::Blit( sp5, display, rt5.x, rt5.y );

        ssp.hide();
        ssp.resize( sp3.width(), sp3.height() );
        ssp.reset();
        fheroes2::DrawBorder( ssp, 214 );
    }

    fheroes2::ButtonGroup btnGroups( box.GetArea(), Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    const uint32_t maximumAcceptedValue = savelast ? max : max - 1;

    const fheroes2::Point minMaxButtonOffset( pos.x + 165, pos.y + 30 );
    fheroes2::ButtonSprite buttonMax( minMaxButtonOffset.x, minMaxButtonOffset.y );
    fheroes2::ButtonSprite buttonMin( minMaxButtonOffset.x, minMaxButtonOffset.y );

    const Rect buttonArea( 5, 0, 61, 25 );
    buttonMax.setSprite( fheroes2::Crop( fheroes2::AGG::GetICN( ICN::RECRUIT, 4 ), buttonArea.x, buttonArea.y, buttonArea.w, buttonArea.h ),
                         fheroes2::Crop( fheroes2::AGG::GetICN( ICN::RECRUIT, 5 ), buttonArea.x, buttonArea.y, buttonArea.w, buttonArea.h ) );
    buttonMin.setSprite( fheroes2::Crop( fheroes2::AGG::GetICN( ICN::BTNMIN, 0 ), buttonArea.x, buttonArea.y, buttonArea.w, buttonArea.h ),
                         fheroes2::Crop( fheroes2::AGG::GetICN( ICN::BTNMIN, 1 ), buttonArea.x, buttonArea.y, buttonArea.w, buttonArea.h ) );

    SwitchMaxMinButtons( buttonMin, buttonMax, cur, maximumAcceptedValue );

    LocalEvent & le = LocalEvent::Get();

    bool redraw_count = false;
    cursor.Show();
    display.render();

    // message loop
    int bres = Dialog::ZERO;
    while ( bres == Dialog::ZERO && le.HandleEvents() ) {
        if ( buttonMax.isVisible() )
            le.MousePressLeft( buttonMax.area() ) ? buttonMax.drawOnPress() : buttonMax.drawOnRelease();
        if ( buttonMin.isVisible() )
            le.MousePressLeft( buttonMin.area() ) ? buttonMin.drawOnPress() : buttonMin.drawOnRelease();

        if ( PressIntKey( max, cur ) ) {
            sel.SetCur( cur );
            redraw_count = true;
        }
        else if ( buttonMax.isVisible() && le.MouseClickLeft( buttonMax.area() ) ) {
            le.MousePressLeft( buttonMax.area() ) ? buttonMax.drawOnPress() : buttonMax.drawOnRelease();
            cur = maximumAcceptedValue;
            sel.SetCur( maximumAcceptedValue );
            redraw_count = true;
        }
        else if ( buttonMin.isVisible() && le.MouseClickLeft( buttonMin.area() ) ) {
            le.MousePressLeft( buttonMin.area() ) ? buttonMin.drawOnPress() : buttonMin.drawOnRelease();
            cur = min;
            sel.SetCur( min );
            redraw_count = true;
        }
        else if ( sel.QueueEventProcessing() )
            redraw_count = true;

        if ( !ssp.empty() )
            for ( std::vector<fheroes2::Rect>::const_iterator it = vrts.begin(); it != vrts.end(); ++it ) {
                if ( le.MouseClickLeft( *it ) ) {
                    cursor.Hide();
                    ssp.setPosition( it->x, it->y );
                    ssp.show();
                    cursor.Show();
                    display.render();
                }
            }

        if ( redraw_count ) {
            SwitchMaxMinButtons( buttonMin, buttonMax, cur, maximumAcceptedValue );
            cursor.Hide();
            if ( !ssp.empty() )
                ssp.hide();
            sel.Redraw();

            if ( buttonMax.isVisible() )
                buttonMax.draw();
            if ( buttonMin.isVisible() )
                buttonMin.draw();

            cursor.Show();
            display.render();

            redraw_count = false;
        }

        bres = btnGroups.processEvents();
    }

    int result = 0;

    if ( bres == Dialog::OK ) {
        cur = sel();

        if ( !ssp.isHidden() ) {
            const fheroes2::Rect rt( ssp.x(), ssp.y(), ssp.width(), ssp.height() );

            if ( rt == rt3 )
                result = 3;
            else if ( rt == rt4 )
                result = 4;
            else if ( rt == rt5 )
                result = 5;
        }
        else
            result = 2;
    }

    return result;
}
