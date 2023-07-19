/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cstdint>
#include <string>

#include "image.h"
#include "interface_itemsbar.h"
#include "math_base.h"
#include "ui_tool.h"

class Army;
class ArmyTroop;

class ArmyBar : public Interface::ItemsActionBar<ArmyTroop>
{
public:
    using Interface::ItemsActionBar<ArmyTroop>::RedrawItem;
    using Interface::ItemsActionBar<ArmyTroop>::ActionBarRightMouseHold;

    ArmyBar( Army *, bool mini, bool ro, bool change = false );

    void RedrawBackground( const fheroes2::Rect &, fheroes2::Image & ) override;
    void RedrawItem( ArmyTroop &, const fheroes2::Rect &, bool, fheroes2::Image & ) override;

    void SetBackground( const fheroes2::Size & sz, const uint8_t fillColor );
    void SetArmy( Army * );

    void setTroopWindowOffsetY( const int32_t offsetY )
    {
        _troopWindowOffsetY = offsetY;
    }

    bool isValid() const;

    void ResetSelected();
    void Redraw( fheroes2::Image & dstsf );

    bool ActionBarLeftMouseSingleClick( ArmyTroop & troop ) override;
    bool ActionBarLeftMouseSingleClick( ArmyTroop & destTroop, ArmyTroop & selectedTroop ) override;
    bool ActionBarLeftMouseDoubleClick( ArmyTroop & troop ) override;
    bool ActionBarLeftMouseRelease( ArmyTroop & troop ) override;
    bool ActionBarLeftMouseRelease( ArmyTroop & destTroop, ArmyTroop & troop ) override;
    bool ActionBarRightMouseHold( ArmyTroop & troop ) override;
    bool ActionBarRightMouseSingleClick( ArmyTroop & troop ) override;
    bool ActionBarRightMouseSingleClick( ArmyTroop & destTroop, ArmyTroop & selectedTroop ) override;

    bool ActionBarCursor( ArmyTroop & ) override;
    bool ActionBarCursor( ArmyTroop &, ArmyTroop & ) override;

    bool QueueEventProcessing( std::string * = nullptr );
    bool QueueEventProcessing( ArmyBar &, std::string * = nullptr );

protected:
    fheroes2::MovableSprite spcursor;

private:
    bool AbleToRedistributeArmyOnRightMouseSingleClick( const ArmyTroop & troop );

    Army * _army;
    fheroes2::Image backsf;
    bool use_mini_sprite;
    bool read_only;
    bool can_change;
    std::string msg;
    int32_t _troopWindowOffsetY;
};

#endif
