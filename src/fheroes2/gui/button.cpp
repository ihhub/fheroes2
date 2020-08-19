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

#include "button.h"
#include "agg.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "settings.h"

namespace
{
    enum
    {
        BTN_PRESSED = 0x0080,
        BTN_DISABLE = 0x0008,
        BTN_VISIBLE = 0x0800
    };

    Surface GetDisabledButtonSurface( int icnId, int index )
    {
        // TODO: we still struggle with shadows so not all buttons look correct
        // That's why we do such magic trick for now
        if ( icnId == ICN::HSBTNS || icnId == ICN::ADVBTNS || icnId == ICN::ADVEBTNS || icnId == ICN::SMALLBAR || icnId == ICN::VIEWGEN ) {
            Surface surface( AGG::GetICN( icnId, index ).GetSurface() );
            if ( AGG::ReplaceColors( surface, PAL::GetPalette( PAL::DARKENING ), icnId, index, false ) ) {
                return surface;
            }
        }

        return Surface();
    }
}

Button::Button()
    : flags( BTN_VISIBLE )
{}

Button::Button( s32 ox, s32 oy, int icn, u32 index1, u32 index2 )
    : flags( BTN_VISIBLE )
{
    SetPos( ox, oy );

    sf1 = AGG::GetICN( icn, index1 );
    sf2 = AGG::GetICN( icn, index2 );
    _disabledSurface = GetDisabledButtonSurface( icn, index1 );

    SetSize( sf1.w(), sf1.h() );
}

bool Button::isEnable( void ) const
{
    return !isDisable();
}

bool Button::isDisable( void ) const
{
    return flags & BTN_DISABLE;
}

bool Button::isVisible() const
{
    return flags & BTN_VISIBLE;
}

bool Button::isPressed( void ) const
{
    return flags & BTN_PRESSED;
}

bool Button::isReleased( void ) const
{
    return !isPressed();
}

void Button::SetPos( s32 ox, s32 oy )
{
    x = ox;
    y = oy;
}

void Button::SetSize( u32 ow, u32 oh )
{
    w = ow;
    h = oh;
}

void Button::SetPos( const Point & pos )
{
    SetPos( pos.x, pos.y );
}

void Button::SetSprite( int icn, u32 index1, u32 index2 )
{
    sf1 = AGG::GetICN( icn, index1 );
    sf2 = AGG::GetICN( icn, index2 );
    _disabledSurface = GetDisabledButtonSurface( icn, index1 );

    SetSize( sf1.w(), sf1.h() );
}

void Button::SetSprite( const Surface & s1, const Surface & s2 )
{
    sf1 = s1;
    sf2 = s2;

    SetSize( sf1.w(), sf1.h() );
}

void Button::SetDisable( bool f )
{
    if ( f )
        flags |= ( BTN_DISABLE | BTN_PRESSED );
    else
        flags &= ~( BTN_DISABLE | BTN_PRESSED );
}

void Button::SetVisible( bool isVisible )
{
    if ( isVisible )
        flags |= BTN_VISIBLE;
    else
        flags &= ~BTN_VISIBLE;
}

void Button::Press( void )
{
    if ( isEnable() && isReleased() )
        flags |= BTN_PRESSED;
}

void Button::Release( void )
{
    if ( isEnable() && isPressed() )
        flags &= ~BTN_PRESSED;
}

void Button::PressDraw( void )
{
    if ( isEnable() && isReleased() ) {
        Press();
        Draw();
        Display::Get().Flip();
    }
}

void Button::ReleaseDraw( void )
{
    if ( isEnable() && isPressed() ) {
        Release();
        Draw();
        Display::Get().Flip();
    }
}

void Button::Draw( void )
{
    if ( !this->isVisible() )
        return;
    bool localcursor = false;
    Cursor & cursor = Cursor::Get();

    if ( ( *this & cursor.GetArea() ) && cursor.isVisible() ) {
        cursor.Hide();
        localcursor = true;
    }

    if ( isDisable() && _disabledSurface.isValid() ) {
        _disabledSurface.Blit( x, y, Display::Get() );
    }
    else if ( isPressed() )
        sf2.Blit( x, y, Display::Get() );
    else
        sf1.Blit( x, y, Display::Get() );

    if ( localcursor )
        cursor.Show();
}

LabeledButton::LabeledButton()
    : Button()
{}

LabeledButton::LabeledButton( s32 ox, s32 oy, int icn, u32 index1, u32 index2 )
    : Button( ox, oy, icn, index1, index2 )
{}

void LabeledButton::SetTopText( const std::string & text )
{
    ResetText( topText, topBack, text, 0, -2 );
}

void LabeledButton::SetMidleText( const std::string & text )
{
    ResetText( middleText, middleBack, text, 0, 0 );
}

void LabeledButton::SetBottomText( const std::string & text )
{
    ResetText( buttomText, buttomBack, text, 0, sf1.h() - 7 );
}

void LabeledButton::ResetText( TextBox & button, SpriteBack & back, const std::string & text, int xIndent, int yIndent )
{
    back.Restore();
    button.Set( text, Font::SMALL, sf1.w() );
    const int yOffSet = yIndent < 0 ? yIndent - button.h() : yIndent + button.h();
    back.Save( Rect( x + ( sf1.w() - button.w() ) / 2 + xIndent, y + yOffSet, button.w(), button.h() ) );
    button.Blit( x + ( sf1.w() - button.w() ) / 2 + xIndent, y + yOffSet );
}
