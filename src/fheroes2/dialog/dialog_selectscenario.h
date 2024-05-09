/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2SELECT_SCENARIO_H
#define H2SELECT_SCENARIO_H

#include <cstdint>

#include "interface_list.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "math_base.h"

namespace fheroes2
{
    class Display;
    class Sprite;
}

class ScenarioListBox : public Interface::ListBox<Maps::FileInfo>
{
public:
    using Interface::ListBox<Maps::FileInfo>::ActionListDoubleClick;
    using Interface::ListBox<Maps::FileInfo>::ActionListSingleClick;
    using Interface::ListBox<Maps::FileInfo>::ActionListPressRight;

    explicit ScenarioListBox( const fheroes2::Point & pt )
        : Interface::ListBox<Maps::FileInfo>( pt )
        , _offsetX( pt.x )
    {}

    void RedrawItem( const Maps::FileInfo & info, int32_t /*dstx*/, int32_t dsty, bool current ) override;
    void RedrawBackground( const fheroes2::Point & dst ) override;
    void SelectMapSize( MapsFileInfoList & mapsList, const int selectedSize );
    void ActionCurrentUp() override
    {
        // Do nothing.
    }

    void ActionCurrentDn() override
    {
        // Do nothing.
    }

    void ActionListDoubleClick( Maps::FileInfo & ) override;

    void ActionListSingleClick( Maps::FileInfo & ) override
    {
        // Do nothing.
    }

    void ActionListPressRight( Maps::FileInfo & /* info */ ) override
    {
        // Do nothing.
    }

    bool isDoubleClicked() const
    {
        return _isDoubleClicked;
    }

    void setForEditorMode( const bool isForEditor )
    {
        _isForEditor = isForEditor;
    }

private:
    int selectedSize{ Maps::ZERO };
    const int32_t _offsetX;
    bool _isDoubleClicked{ false };
    bool _isForEditor{ false };

    void _renderScenarioListItem( const Maps::FileInfo & info, fheroes2::Display & display, const int32_t dsty, const bool current ) const;
    void _renderSelectedScenarioInfo( fheroes2::Display & display, const fheroes2::Point & dst );
    void _renderMapName( const Maps::FileInfo & info, bool selected, const int32_t & baseYOffset, fheroes2::Display & display ) const;
    static void _renderMapIcon( const uint16_t size, fheroes2::Display & display, const int32_t coordX, const int32_t coordY );
    static const fheroes2::Sprite & _getPlayersCountIcon( const uint8_t colors );
    static const fheroes2::Sprite & _getMapTypeIcon( const GameVersion version );
    static const fheroes2::Sprite & _getWinConditionsIcon( const uint8_t condition );
    static const fheroes2::Sprite & _getLossConditionsIcon( const uint8_t condition );
};

namespace Dialog
{
    const Maps::FileInfo * SelectScenario( const MapsFileInfoList & allMaps, const bool isForEditor );
}

#endif
