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

#include <array>
#include <cassert>
#include <cstdint>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "artifact.h"
#include "editor_interface.h"
#include "ground.h"
#include "interface_gamearea.h"
#include "logging.h"
#include "map_object_info.h"
#include "maps_tiles.h"
#include "monster.h"
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
            for ( const auto & info : Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_TREASURES ) ) {
                assert( !info.groundLevelParts.empty() );

                if ( info.objectType == MP2::OBJ_RESOURCE && info.groundLevelParts.front().icnIndex == tile.getMainObjectPart().icnIndex
                     && info.groundLevelParts.front().icnType == tile.getMainObjectPart().icnType ) {
                    return Resource::String( static_cast<int32_t>( info.metadata[0] ) );
                }
            }

            // This is an invalid object!
            assert( 0 );
            break;
        }
        case MP2::OBJ_ARTIFACT: {
            for ( const auto & info : Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_ARTIFACTS ) ) {
                assert( !info.groundLevelParts.empty() );

                if ( info.objectType == MP2::OBJ_ARTIFACT && info.groundLevelParts.front().icnIndex == tile.getMainObjectPart().icnIndex
                     && info.groundLevelParts.front().icnType == tile.getMainObjectPart().icnType ) {
                    return MP2::StringObject( MP2::OBJ_ARTIFACT ) + std::string( "\n" ) + Artifact( static_cast<int32_t>( info.metadata[0] ) ).GetName();
                }
            }

            // This is an invalid object!
            assert( 0 );
            break;
        }
        case MP2::OBJ_MONSTER: {
            for ( const auto & info : Maps::getObjectsByGroup( Maps::ObjectGroup::MONSTERS ) ) {
                assert( !info.groundLevelParts.empty() );

                if ( info.objectType == MP2::OBJ_MONSTER && info.groundLevelParts.front().icnIndex == tile.getMainObjectPart().icnIndex
                     && info.groundLevelParts.front().icnType == tile.getMainObjectPart().icnType ) {
                    return MP2::StringObject( MP2::OBJ_MONSTER ) + std::string( "\n" ) + Monster( static_cast<int32_t>( info.metadata[0] ) ).GetMultiName();
                }
            }

            // This is an invalid object!
            assert( 0 );
            break;
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
