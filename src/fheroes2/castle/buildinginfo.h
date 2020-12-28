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

#ifndef H2BUILDINGINFO_H
#define H2BUILDINGINFO_H

#include "castle.h"
#include "payment.h"

class StatusBar;

class BuildingInfo
{
public:
    BuildingInfo( const Castle &, building_t );

    u32 operator()( void ) const;
    void SetPos( s32, s32 );
    const fheroes2::Rect & GetArea( void ) const;
    const char * GetName( void ) const;
    const std::string & GetDescription( void ) const;
    void SetStatusMessage( StatusBar & ) const;
    bool IsDwelling( void ) const;
    void Redraw( void );
    bool QueueEventProcessing( fheroes2::ButtonBase & exitButton );
    bool DialogBuyBuilding( bool buttons ) const;

    static void UpdateCosts( const std::string & );
    static payment_t GetCost( u32, int );

private:
    void RedrawCaptain( void );
    std::string GetConditionDescription( void ) const;

    const Castle & castle;
    u32 building;
    std::string description;
    fheroes2::Rect area;
    int bcond;
};

struct DwellingItem
{
    DwellingItem( Castle &, u32 dw );

    u32 type;
    Monster mons;
};

class DwellingsBar : public Interface::ItemsBar<DwellingItem>
{
public:
    DwellingsBar( Castle &, const Size & );

    virtual void RedrawBackground( const Rect &, fheroes2::Image & ) override;
    virtual void RedrawItem( DwellingItem &, const Rect &, fheroes2::Image & ) override;

    virtual bool ActionBarLeftMouseSingleClick( DwellingItem & dwelling ) override;
    virtual bool ActionBarRightMouseHold( DwellingItem & dwelling ) override;

protected:
    Castle & castle;
    fheroes2::Image backsf;
    std::vector<DwellingItem> content;
};

#endif
