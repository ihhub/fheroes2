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
#include "interface_itemsbar.h"
#include "gamedefs.h"

class Army;

class ArmyBar : public Interface::ItemsActionBar<ArmyTroop>
{
public:
    ArmyBar(Army*, bool mini, bool ro, bool change = false);

    void        RedrawBackground(const Rect &, Surface &);
    void        RedrawItem(ArmyTroop &, const Rect &, bool, Surface &);

    void	SetBackground(const Size &, const RGBA &);
    void	SetArmy(Army*);

    bool	isValid(void) const;

    void        ResetSelected(void);
    void        Redraw(Surface & dstsf = Display::Get());

    bool        ActionBarSingleClick(const Point &, ArmyTroop &, const Rect &);
    bool        ActionBarSingleClick(const Point &, ArmyTroop &, const Rect &, ArmyTroop &, const Rect &);
    bool        ActionBarDoubleClick(const Point &, ArmyTroop &, const Rect &);
    bool        ActionBarPressRight(const Point &, ArmyTroop &, const Rect &);
    bool        ActionBarPressRight(const Point &, ArmyTroop &, const Rect &, ArmyTroop &, const Rect &);

    bool	ActionBarCursor(const Point &, ArmyTroop &, const Rect &);
    bool	ActionBarCursor(const Point &, ArmyTroop &, const Rect &, ArmyTroop &, const Rect &);

    bool	QueueEventProcessing(std::string* = NULL);
    bool	QueueEventProcessing(ArmyBar &, std::string* = NULL);

protected:
    Army*		army;
    Surface             backsf;
    SpriteMove          spcursor;
    bool                use_mini_sprite;
    bool                read_only;
    bool                can_change;
    std::string		msg;
};

#endif
