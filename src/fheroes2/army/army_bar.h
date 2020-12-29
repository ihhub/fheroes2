/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2ARMYBAR_H
#define H2ARMYBAR_H

#include "army_troop.h"
#include "gamedefs.h"
#include "interface_itemsbar.h"
#include "ui_tool.h"

class Army;

class ArmyBar : public Interface::ItemsActionBar<ArmyTroop>
{
public:
    ArmyBar( Army *, bool mini, bool ro, bool change = false );

    virtual void RedrawBackground( const Rect &, fheroes2::Image & ) override;
    virtual void RedrawItem( ArmyTroop &, const Rect &, bool, fheroes2::Image & ) override;

    void SetBackground( const Size & sz, const uint8_t fillColor );
    void SetArmy( Army * );

    bool isValid( void ) const;

    void ResetSelected( void );
    void Redraw( fheroes2::Image & dstsf = fheroes2::Display::instance() );

    virtual bool ActionBarLeftMouseSingleClick( ArmyTroop & troop ) override;
    virtual bool ActionBarLeftMouseSingleClick( ArmyTroop & destTroop, ArmyTroop & selectedTroop ) override;
    virtual bool ActionBarLeftMouseDoubleClick( ArmyTroop & troop ) override;
    virtual bool ActionBarLeftMouseRelease( ArmyTroop & troop ) override;
    virtual bool ActionBarLeftMouseRelease( ArmyTroop & destTroop, ArmyTroop & troop ) override;
    virtual bool ActionBarRightMouseHold( ArmyTroop & troop ) override;
    virtual bool ActionBarRightMouseSingleClick( ArmyTroop & troop ) override;
    virtual bool ActionBarRightMouseSingleClick( ArmyTroop & destTroop, ArmyTroop & selectedTroop ) override;
    virtual bool ActionBarRightMouseRelease( ArmyTroop & troop ) override;
    virtual bool ActionBarRightMouseRelease( ArmyTroop & destTroop, ArmyTroop & selectedTroop ) override;

    virtual bool ActionBarCursor( ArmyTroop & ) override;
    virtual bool ActionBarCursor( ArmyTroop &, ArmyTroop & ) override;

    bool QueueEventProcessing( std::string * = NULL );
    bool QueueEventProcessing( ArmyBar &, std::string * = NULL );

protected:
    Army * army;
    fheroes2::Image backsf;
    fheroes2::MovableSprite spcursor;
    bool use_mini_sprite;
    bool read_only;
    bool can_change;
    bool _isTroopInfoVisible;
    std::string msg;
};

#endif
