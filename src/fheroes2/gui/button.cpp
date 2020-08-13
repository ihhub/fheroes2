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

ButtonGroups::ButtonGroups( const Rect & pos, u32 btns )
    : button1( NULL )
    , button2( NULL )
    , result1( Dialog::ZERO )
    , result2( Dialog::ZERO )
    , buttons( btns )
{
    Point pt;
    const int system = Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM;

    switch ( buttons ) {
    case Dialog::YES | Dialog::NO:
        pt.x = pos.x;
        pt.y = pos.y + pos.h - AGG::GetICN( system, 5 ).h();
        button1 = new Button( pt.x, pt.y, system, 5, 6 );
        result1 = Dialog::YES;
        pt.x = pos.x + pos.w - AGG::GetICN( system, 7 ).w();
        pt.y = pos.y + pos.h - AGG::GetICN( system, 7 ).h();
        button2 = new Button( pt.x, pt.y, system, 7, 8 );
        result2 = Dialog::NO;
        break;

    case Dialog::OK | Dialog::CANCEL:
        pt.x = pos.x;
        pt.y = pos.y + pos.h - AGG::GetICN( system, 1 ).h();
        button1 = new Button( pt.x, pt.y, system, 1, 2 );
        result1 = Dialog::OK;
        pt.x = pos.x + pos.w - AGG::GetICN( system, 3 ).w();
        pt.y = pos.y + pos.h - AGG::GetICN( system, 3 ).h();
        button2 = new Button( pt.x, pt.y, system, 3, 4 );
        result2 = Dialog::CANCEL;
        break;

    case Dialog::OK:
        pt.x = pos.x + ( pos.w - AGG::GetICN( system, 1 ).w() ) / 2;
        pt.y = pos.y + pos.h - AGG::GetICN( system, 1 ).h();
        button1 = new Button( pt.x, pt.y, system, 1, 2 );
        result1 = Dialog::OK;
        break;

    case Dialog::CANCEL:
        pt.x = pos.x + ( pos.w - AGG::GetICN( system, 3 ).w() ) / 2;
        pt.y = pos.y + pos.h - AGG::GetICN( system, 3 ).h();
        button2 = new Button( pt.x, pt.y, system, 3, 4 );
        result2 = Dialog::CANCEL;
        break;

    default:
        break;
    }
}

ButtonGroups::~ButtonGroups()
{
    if ( button1 )
        delete button1;
    if ( button2 )
        delete button2;
}

void ButtonGroups::Draw( void )
{
    if ( button1 )
        ( *button1 ).Draw();
    if ( button2 )
        ( *button2 ).Draw();
}

int ButtonGroups::QueueEventProcessing( void )
{
    LocalEvent & le = LocalEvent::Get();

    if ( button1 && button1->isEnable() )
        le.MousePressLeft( *button1 ) ? button1->PressDraw() : button1->ReleaseDraw();
    if ( button2 && button2->isEnable() )
        le.MousePressLeft( *button2 ) ? button2->PressDraw() : button2->ReleaseDraw();

    if ( button1 && button1->isEnable() && le.MouseClickLeft( *button1 ) )
        return result1;
    if ( button2 && button2->isEnable() && le.MouseClickLeft( *button2 ) )
        return result2;

    if ( button1 && button2 ) {
        if ( buttons == ( Dialog::YES | Dialog::NO ) || buttons == ( Dialog::OK | Dialog::CANCEL ) ) {
            if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) && button1->isEnable() )
                return result1;
            if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) && button2->isEnable() )
                return result2;
        }

        if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_LEFT ) && button1->isEnable() )
            return result1;
        else if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_RIGHT ) && button2->isEnable() )
            return result2;
    }
    else
    // one button
    {
        if ( HotKeyCloseWindow )
            return buttons;
    }

    return Dialog::ZERO;
}

void ButtonGroups::DisableButton1( bool f )
{
    if ( button1 ) {
        if ( f ) {
            button1->Press();
            button1->SetDisable( true );
        }
        else {
            button1->Release();
            button1->SetDisable( false );
        }
    }
}

void ButtonGroups::DisableButton2( bool f )
{
    if ( button2 ) {
        if ( f ) {
            button2->Press();
            button2->SetDisable( true );
        }
        else {
            button2->Release();
            button2->SetDisable( false );
        }
    }
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
