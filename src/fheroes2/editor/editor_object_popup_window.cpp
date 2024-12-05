/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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

#include "editor_object_popup_window.h"

#include <cassert>
#include <cstdint>
#include <ostream>
#include <string>
#include <utility>

#include "editor_interface.h"
#include "ground.h"
#include "interface_gamearea.h"
#include "logging.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "mp2.h"
#include "resource.h"
#include "translations.h"
#include "ui_map_interface.h"
#include "world.h"

namespace
{
    std::string getObjectInfoText( const Maps::Tile & tile )
    {
        const MP2::MapObjectType type = tile.getMainObjectType();
        switch ( type ) {
        case MP2::OBJ_NONE:
            if ( tile.isRoad() ) {
                return _( "Road" );
            }

            return Maps::Ground::String( tile.GetGround() );
        case MP2::OBJ_RESOURCE: {
            const Funds funds = getFundsFromTile( tile );
            assert( funds.GetValidItemsCount() == 1 );

            return Resource::String( funds.getFirstValidResource().first );
        }
        default:
            break;
        }

        return MP2::StringObject( type );
    }
}

namespace Editor
{
    void showPopupWindow( const Maps::Tile & tile )
    {
        DEBUG_LOG( DBG_DEVEL, DBG_INFO, '\n' << tile.String() )

        std::string infoString;
        const int32_t mainTileIndex = Maps::Tile::getIndexOfMainTile( tile );

        if ( mainTileIndex != -1 ) {
            infoString = getObjectInfoText( world.getTile( mainTileIndex ) );
        }
        else {
            infoString = getObjectInfoText( tile );
        }

        infoString += "\n[";
        infoString += std::to_string( tile.GetIndex() % world.w() );
        infoString += "; ";
        infoString += std::to_string( tile.GetIndex() / world.w() );
        infoString += ']';

        Interface::displayStandardPopupWindow( std::move( infoString ), Interface::EditorInterface::Get().getGameArea().GetROI() );
    }
}
