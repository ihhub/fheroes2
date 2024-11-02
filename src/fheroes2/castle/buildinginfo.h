/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#include "image.h"
#include "interface_itemsbar.h"
#include "math_base.h"
#include "resource.h"

namespace fheroes2
{
    class ButtonBase;
}

class Castle;
class StatusBar;

enum BuildingType : uint32_t;
enum class BuildingStatus : int32_t;

class BuildingInfo
{
public:
    BuildingInfo( const Castle & c, const BuildingType b );

    uint32_t getBuilding() const
    {
        return _buildingType;
    }

    void SetPos( int32_t, int32_t );

    const fheroes2::Rect & GetArea() const
    {
        return area;
    }

    const char * GetName() const;
    void SetStatusMessage( StatusBar & ) const;
    static bool isDwelling( const uint32_t building );
    static std::string getBuildingDescription( const int race, const uint32_t buildingId );
    void Redraw() const;
    bool QueueEventProcessing( fheroes2::ButtonBase & exitButton ) const;
    bool DialogBuyBuilding( bool buttons ) const;

    static Funds GetCost( uint32_t, int );

private:
    void RedrawCaptain() const;
    std::string GetConditionDescription() const;

    const Castle & castle;
    uint32_t _buildingType;
    std::string description;
    fheroes2::Rect area;
    BuildingStatus _status;
};

struct DwellingItem
{
    explicit DwellingItem( const uint32_t dw )
        : dwType( dw )
    {}

    const uint32_t dwType;
};

class DwellingsBar final : public Interface::ItemsBar<DwellingItem>
{
public:
    DwellingsBar( Castle & cstl, const fheroes2::Size & sz );

    void RedrawBackground( const fheroes2::Rect & pos, fheroes2::Image & dstsf ) override;
    void RedrawItem( DwellingItem & dwl, const fheroes2::Rect & pos, fheroes2::Image & dstsf ) override;

    bool ActionBarLeftMouseSingleClick( DwellingItem & dwl ) override;
    bool ActionBarRightMouseHold( DwellingItem & dwl ) override;

private:
    Castle & castle;
    fheroes2::Image backsf;
    std::vector<DwellingItem> content;
};

#endif
