/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2SELECT_SCENARIO_H
#define H2SELECT_SCENARIO_H

#include "interface_list.h"
#include "maps_fileinfo.h"

class ScenarioListBox : public Interface::ListBox<Maps::FileInfo>
{
public:
    explicit ScenarioListBox( const fheroes2::Point & pt )
        : Interface::ListBox<Maps::FileInfo>( pt )
        , selectOk( false )
    {}

    void RedrawItem( const Maps::FileInfo &, s32, s32, bool ) override;
    void RedrawBackground( const fheroes2::Point & ) override;

    void ActionCurrentUp( void ) override {}
    void ActionCurrentDn( void ) override {}
    void ActionListDoubleClick( Maps::FileInfo & ) override;
    void ActionListSingleClick( Maps::FileInfo & ) override {}
    void ActionListPressRight( Maps::FileInfo & ) override {}

    bool selectOk;
};

namespace Dialog
{
    const Maps::FileInfo * SelectScenario( const MapsFileInfoList & maps, size_t selectedId );
}

#endif
