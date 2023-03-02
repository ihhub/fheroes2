/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2BUILDINGINFO_H
#define H2BUILDINGINFO_H

#include <cstdint>
#include <string>
#include <vector>

#include "castle.h"
#include "image.h"
#include "interface_itemsbar.h"
#include "math_base.h"
#include "monster.h"
#include "payment.h"

namespace fheroes2
{
    class ButtonBase;
}

class StatusBar;

class BuildingInfo
{
public:
    BuildingInfo( const Castle & c, const building_t b );

    uint32_t getBuilding() const
    {
        return building;
    }

    void SetPos( int32_t, int32_t );

    const fheroes2::Rect & GetArea() const
    {
        return area;
    }

    const char * GetName() const;
    void SetStatusMessage( StatusBar & ) const;
    bool IsDwelling() const;
    void Redraw() const;
    bool QueueEventProcessing( fheroes2::ButtonBase & exitButton ) const;
    bool DialogBuyBuilding( bool buttons ) const;

    static payment_t GetCost( uint32_t, int );

private:
    void RedrawCaptain() const;
    std::string GetConditionDescription() const;

    const Castle & castle;
    uint32_t building;
    std::string description;
    fheroes2::Rect area;
    int bcond;
};

struct DwellingItem
{
    DwellingItem( const Castle &, uint32_t dw );

    uint32_t type;
    Monster mons;
};

class DwellingsBar : public Interface::ItemsBar<DwellingItem>
{
public:
    DwellingsBar( Castle &, const fheroes2::Size & );

    void RedrawBackground( const fheroes2::Rect &, fheroes2::Image & ) override;
    void RedrawItem( DwellingItem &, const fheroes2::Rect &, fheroes2::Image & ) override;

    bool ActionBarLeftMouseSingleClick( DwellingItem & dwelling ) override;
    bool ActionBarRightMouseHold( DwellingItem & dwelling ) override;

protected:
    Castle & castle;
    fheroes2::Image backsf;
    std::vector<DwellingItem> content;
};

#endif
