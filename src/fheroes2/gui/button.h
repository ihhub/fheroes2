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
#ifndef H2BUTTON_H
#define H2BUTTON_H

#include "gamedefs.h"
#include "text.h"

class Surface;

class Button : public Rect
{
public:
    Button();
    Button( int, int, int, uint32_t indxDisabled, uint32_t indexPressed );

    bool isEnable( void ) const;
    bool isDisable( void ) const;
    bool isVisible() const;
    bool isPressed( void ) const;
    bool isReleased( void ) const;

    void Press( void );
    void Release( void );

    void SetPos( int, int );
    void SetPos( const Point & );
    void SetSize( uint32_t, uint32_t );
    void SetSprite( int icn, uint32_t, uint32_t );
    void SetSprite( const Surface &, const Surface & );
    void SetDisable( bool );
    void SetVisible( bool isVisible );

    void Draw( void );
    void PressDraw( void );
    void ReleaseDraw( void );

protected:
    Surface sf1;
    Surface sf2;
    Surface _disabledSurface;

    uint32_t flags;
};

class LabeledButton : public Button
{
public:
    LabeledButton();
    LabeledButton( int ox, int oy, int icn, uint32_t indxDisabled, uint32_t indexPressed );
    void SetTopText( const std::string & text );
    void SetMidleText( const std::string & text );
    void SetBottomText( const std::string & text );

protected:
    TextBox topText;
    TextBox middleText;
    TextBox buttomText;
    SpriteBack topBack;
    SpriteBack middleBack;
    SpriteBack buttomBack;

    void ResetText( TextBox & buttonText, SpriteBack & back, const std::string & newText, int xIndent, int yIndent );
};

class ButtonSprite : public Button
{
public:
    ButtonSprite() {}

protected:
    Surface sf;
};

class ButtonGroups
{
public:
    ButtonGroups( const Rect &, uint32_t );
    ~ButtonGroups();

    void Draw( void );
    int QueueEventProcessing( void );

    void DisableButton1( bool );
    void DisableButton2( bool );

protected:
    Button * button1;
    Button * button2;
    int result1;
    int result2;
    int buttons;
};

#endif
