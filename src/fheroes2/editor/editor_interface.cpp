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

#include "editor_interface.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "agg_image.h"
#include "artifact.h"
#include "audio_manager.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "editor_castle_details_window.h"
#include "editor_event_details_window.h"
#include "editor_map_specs_window.h"
#include "editor_object_popup_window.h"
#include "editor_save_map_window.h"
#include "editor_sphinx_window.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_static.h"
#include "gamedefs.h"
#include "ground.h"
#include "heroes.h"
#include "history_manager.h"
#include "icn.h"
#include "image.h"
#include "interface_base.h"
#include "interface_border.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "localevent.h"
#include "map_format_helper.h"
#include "map_object_info.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "monster.h"
#include "mp2.h"
#include "race.h"
#include "render_processor.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_map_object.h"
#include "ui_monster.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "view_world.h"
#include "world.h"
#include "world_object_uid.h"

namespace
{
    const uint32_t mapUpdateFlags = Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR;

    class HideInterfaceModeDisabler
    {
    public:
        HideInterfaceModeDisabler()
        {
            // If Hide Interface mode is enabled we temporary disable it to allow editor properly place all interface items.
            if ( _isHideInterfaceEnabled ) {
                Settings::Get().setHideInterface( false );
            }
        }

        HideInterfaceModeDisabler & operator=( const HideInterfaceModeDisabler & ) = delete;
        HideInterfaceModeDisabler( const HideInterfaceModeDisabler & ) = delete;

        ~HideInterfaceModeDisabler()
        {
            if ( _isHideInterfaceEnabled ) {
                // Restore Hide Interface mode if it was enabled.
                Settings::Get().setHideInterface( true );
            }
        }

    private:
        const bool _isHideInterfaceEnabled{ Settings::Get().isHideInterfaceEnabled() };
    };

    fheroes2::Point getBrushAreaIndicies( const fheroes2::Rect & brushSize, const int32_t startIndex )
    {
        if ( brushSize.width <= 0 || brushSize.height <= 0 ) {
            return { startIndex, startIndex };
        }

        const int32_t worldWidth = world.w();
        const int32_t worldHeight = world.h();

        fheroes2::Point startPos{ ( startIndex % worldWidth ) + brushSize.x, ( startIndex / worldWidth ) + brushSize.y };
        fheroes2::Point endPos{ startPos.x + brushSize.width - 1, startPos.y + brushSize.height - 1 };

        startPos.x = std::max( startPos.x, 0 );
        startPos.y = std::max( startPos.y, 0 );
        endPos.x = std::min( endPos.x, worldWidth - 1 );
        endPos.y = std::min( endPos.y, worldHeight - 1 );

        return { startPos.x + startPos.y * worldWidth, endPos.x + endPos.y * worldWidth };
    }

    bool isObjectPlacementAllowed( const Maps::ObjectInfo & info, const fheroes2::Point & mainTilePos )
    {
        // Run through all tile offsets and check that all objects parts can be put on the map.
        for ( const auto & objectPart : info.groundLevelParts ) {
            if ( objectPart.layerType == Maps::SHADOW_LAYER ) {
                // Shadow layer parts are ignored.
                continue;
            }

            if ( !Maps::isValidAbsPoint( mainTilePos.x + objectPart.tileOffset.x, mainTilePos.y + objectPart.tileOffset.y ) ) {
                return false;
            }
        }

        for ( const auto & objectPart : info.topLevelParts ) {
            if ( !Maps::isValidAbsPoint( mainTilePos.x + objectPart.tileOffset.x, mainTilePos.y + objectPart.tileOffset.y ) ) {
                return false;
            }
        }

        return true;
    }

    bool isActionObjectAllowed( const Maps::ObjectInfo & info, const fheroes2::Point & mainTilePos )
    {
        // Active action object parts must be placed on a tile without any other objects.
        // Only ground parts should be checked for this condition.
        for ( const auto & objectPart : info.groundLevelParts ) {
            if ( objectPart.layerType == Maps::SHADOW_LAYER || objectPart.layerType == Maps::TERRAIN_LAYER ) {
                // Shadow and terrain layer parts are ignored.
                continue;
            }

            const fheroes2::Point pos{ mainTilePos.x + objectPart.tileOffset.x, mainTilePos.y + objectPart.tileOffset.y };
            if ( !Maps::isValidAbsPoint( pos.x, pos.y ) ) {
                return false;
            }

            const auto & tile = world.GetTiles( pos.x, pos.y );

            if ( MP2::isOffGameActionObject( tile.GetObject() ) ) {
                // An action object already exist. We cannot allow to put anything on top of it.
                return false;
            }

            if ( MP2::isOffGameActionObject( objectPart.objectType ) && !Maps::isClearGround( tile ) ) {
                // We are trying to place an action object on a tile that has some other objects.
                return false;
            }
        }

        return true;
    }

    bool isConditionValid( const std::vector<fheroes2::Point> & offsets, const fheroes2::Point & mainTilePos,
                           const std::function<bool( const Maps::Tiles & tile )> & condition )
    {
        if ( offsets.empty() ) {
            return true;
        }

        assert( condition );

        for ( const auto & offset : offsets ) {
            const fheroes2::Point temp{ mainTilePos.x + offset.x, mainTilePos.y + offset.y };
            if ( !Maps::isValidAbsPoint( temp.x, temp.y ) ) {
                return false;
            }

            if ( !condition( world.GetTiles( temp.x, temp.y ) ) ) {
                return false;
            }
        }

        return true;
    }

    bool checkConditionForUsedTiles( const Maps::ObjectInfo & info, const fheroes2::Point & mainTilePos,
                                     const std::function<bool( const Maps::Tiles & tile )> & condition )
    {
        return isConditionValid( Maps::getGroundLevelUsedTileOffset( info ), mainTilePos, condition );
    }

    bool removeObjects( Maps::Map_Format::MapFormat & mapFormat, std::set<uint32_t> objectsUids, const std::set<Maps::ObjectGroup> & objectGroups )
    {
        if ( objectsUids.empty() ) {
            return false;
        }

        bool needRedraw = false;
        bool updateMapPlayerInformation = false;

        // Filter objects by group and remove them from '_mapFormat'.
        for ( size_t mapTileIndex = 0; mapTileIndex < mapFormat.tiles.size(); ++mapTileIndex ) {
            Maps::Map_Format::TileInfo & mapTile = mapFormat.tiles[mapTileIndex];

            for ( auto objectIter = mapTile.objects.begin(); objectIter != mapTile.objects.end(); ) {
                // LANDSCAPE_FLAGS and LANDSCAPE_TOWN_BASEMENTS are special objects that should be erased only when erasing the main object.
                if ( objectIter->group == Maps::ObjectGroup::LANDSCAPE_FLAGS || objectIter->group == Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS
                     || objectsUids.find( objectIter->id ) == objectsUids.end() ) {
                    // No main object UID was found.
                    ++objectIter;
                    continue;
                }

                // The object with this UID is found, remove UID not to search for it more.
                objectsUids.erase( objectIter->id );

                if ( std::none_of( objectGroups.begin(), objectGroups.end(), [&objectIter]( const Maps::ObjectGroup group ) { return group == objectIter->group; } ) ) {
                    // This object is not in the selected to erase groups.
                    if ( objectsUids.empty() ) {
                        break;
                    }

                    ++objectIter;
                    continue;
                }

                if ( objectIter->group == Maps::ObjectGroup::KINGDOM_TOWNS ) {
                    // Towns and castles consist of four objects. We need to search them all and remove from map.
                    const uint32_t objectId = objectIter->id;

                    auto findTownPart = [objectId]( const Maps::Map_Format::TileInfo & tileToSearch, const Maps::ObjectGroup group ) {
                        auto foundObjectIter = std::find_if( tileToSearch.objects.begin(), tileToSearch.objects.end(),
                                                             [objectId, group]( const Maps::Map_Format::TileObjectInfo & mapObject ) {
                                                                 return mapObject.group == group && mapObject.id == objectId;
                                                             } );

                        // The town part should exist on the map. If no then there might be issues in towns placing.
                        assert( foundObjectIter != tileToSearch.objects.end() );

                        return foundObjectIter;
                    };

                    // Remove the town object.
                    mapTile.objects.erase( objectIter );

                    // Town basement is also located at this tile. Find and remove it.
                    mapTile.objects.erase( findTownPart( mapTile, Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS ) );

                    // Remove flags.
                    assert( mapTileIndex > 0 );
                    Maps::Map_Format::TileInfo & previousMapTile = mapFormat.tiles[mapTileIndex - 1];
                    previousMapTile.objects.erase( findTownPart( previousMapTile, Maps::ObjectGroup::LANDSCAPE_FLAGS ) );

                    assert( mapTileIndex < mapFormat.tiles.size() - 1 );
                    Maps::Map_Format::TileInfo & nextMapTile = mapFormat.tiles[mapTileIndex + 1];
                    nextMapTile.objects.erase( findTownPart( nextMapTile, Maps::ObjectGroup::LANDSCAPE_FLAGS ) );

                    // Two objects have been removed from this tile. Start search from the beginning.
                    objectIter = mapTile.objects.begin();

                    // Remove this town metadata.
                    assert( mapFormat.castleMetadata.find( objectId ) != mapFormat.castleMetadata.end() );
                    mapFormat.castleMetadata.erase( objectId );

                    // There could be a road in front of the castle entrance. Remove it because there is no entrance to the castle anymore.
                    const size_t bottomTileIndex = mapTileIndex + mapFormat.size;
                    assert( bottomTileIndex < mapFormat.tiles.size() );
                    auto & bottomTileObjects = mapFormat.tiles[bottomTileIndex].objects;
                    const bool isRoadAtBottom
                        = std::find_if( bottomTileObjects.begin(), bottomTileObjects.end(),
                                        []( const Maps::Map_Format::TileObjectInfo & mapObject ) { return mapObject.group == Maps::ObjectGroup::ROADS; } )
                          != bottomTileObjects.end();
                    if ( isRoadAtBottom ) {
                        // TODO: Update (not remove) the road. It may be done properly only after roads handling will be moved from 'world' tiles to 'Map_Format' tiles.
                        Maps::updateRoadOnTile( world.GetTiles( static_cast<int32_t>( bottomTileIndex ) ), false );
                    }

                    needRedraw = true;
                    updateMapPlayerInformation = true;
                }
                else if ( objectIter->group == Maps::ObjectGroup::ROADS ) {
                    assert( mapTileIndex < world.getSize() );

                    needRedraw |= Maps::updateRoadOnTile( world.GetTiles( static_cast<int32_t>( mapTileIndex ) ), false );

                    ++objectIter;
                }
                else if ( objectIter->group == Maps::ObjectGroup::STREAMS ) {
                    assert( mapTileIndex < world.getSize() );

                    needRedraw |= Maps::updateStreamOnTile( world.GetTiles( static_cast<int32_t>( mapTileIndex ) ), false );

                    ++objectIter;
                }
                else if ( objectIter->group == Maps::ObjectGroup::KINGDOM_HEROES || Maps::isJailObject( objectIter->group, objectIter->index ) ) {
                    // Remove this hero metadata.
                    assert( mapFormat.heroMetadata.find( objectIter->id ) != mapFormat.heroMetadata.end() );
                    mapFormat.heroMetadata.erase( objectIter->id );

                    objectIter = mapTile.objects.erase( objectIter );
                    needRedraw = true;

                    updateMapPlayerInformation = true;
                }
                else if ( objectIter->group == Maps::ObjectGroup::MONSTERS ) {
                    assert( mapFormat.standardMetadata.find( objectIter->id ) != mapFormat.standardMetadata.end() );
                    mapFormat.standardMetadata.erase( objectIter->id );

                    objectIter = mapTile.objects.erase( objectIter );
                    needRedraw = true;
                }
                else if ( objectIter->group == Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS ) {
                    const auto & objects = Maps::getObjectsByGroup( objectIter->group );

                    assert( objectIter->index < objects.size() );
                    const auto objectType = objects[objectIter->index].objectType;
                    switch ( objectType ) {
                    case MP2::OBJ_EVENT:
                        assert( mapFormat.adventureMapEventMetadata.find( objectIter->id ) != mapFormat.adventureMapEventMetadata.end() );
                        mapFormat.adventureMapEventMetadata.erase( objectIter->id );
                        break;
                    case MP2::OBJ_SIGN:
                        assert( mapFormat.signMetadata.find( objectIter->id ) != mapFormat.signMetadata.end() );
                        mapFormat.signMetadata.erase( objectIter->id );
                        break;
                    case MP2::OBJ_SPHINX:
                        assert( mapFormat.sphinxMetadata.find( objectIter->id ) != mapFormat.sphinxMetadata.end() );
                        mapFormat.sphinxMetadata.erase( objectIter->id );
                        break;
                    default:
                        break;
                    }

                    objectIter = mapTile.objects.erase( objectIter );
                    needRedraw = true;
                }
                else if ( objectIter->group == Maps::ObjectGroup::ADVENTURE_WATER ) {
                    const auto & objects = Maps::getObjectsByGroup( objectIter->group );

                    assert( objectIter->index < objects.size() );
                    const auto objectType = objects[objectIter->index].objectType;
                    if ( objectType == MP2::OBJ_BOTTLE ) {
                        assert( mapFormat.signMetadata.find( objectIter->id ) != mapFormat.signMetadata.end() );
                        mapFormat.signMetadata.erase( objectIter->id );
                    }

                    objectIter = mapTile.objects.erase( objectIter );
                    needRedraw = true;
                }
                else if ( objectIter->group == Maps::ObjectGroup::ADVENTURE_ARTIFACTS ) {
                    assert( mapFormat.standardMetadata.find( objectIter->id ) != mapFormat.standardMetadata.end() );
                    mapFormat.standardMetadata.erase( objectIter->id );

                    objectIter = mapTile.objects.erase( objectIter );
                    needRedraw = true;
                }
                else {
                    objectIter = mapTile.objects.erase( objectIter );
                    needRedraw = true;
                }

                if ( objectsUids.empty() ) {
                    break;
                }
            }
        }

        if ( updateMapPlayerInformation && !Maps::updateMapPlayers( mapFormat ) ) {
            assert( 0 );
        }

        return needRedraw;
    }

    bool verifyTerrainPlacement( const fheroes2::Point & tilePos, const Maps::ObjectGroup groupType, const int32_t objectType, std::string & errorMessage )
    {
        switch ( groupType ) {
        case Maps::ObjectGroup::ADVENTURE_ARTIFACTS: {
            const auto & objectInfo = Maps::getObjectInfo( groupType, objectType );

            if ( !checkConditionForUsedTiles( objectInfo, tilePos, []( const Maps::Tiles & tileToCheck ) { return !tileToCheck.isWater(); } ) ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            if ( objectInfo.objectType == MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT
                 && !checkConditionForUsedTiles( objectInfo, tilePos, []( const Maps::Tiles & tileToCheck ) { return tileToCheck.GoodForUltimateArtifact(); } ) ) {
                errorMessage = _( "The Ultimate Artifact can only be placed on terrain where digging is possible." );
                return false;
            }

            break;
        }
        case Maps::ObjectGroup::ADVENTURE_DWELLINGS:
        case Maps::ObjectGroup::ADVENTURE_MINES:
        case Maps::ObjectGroup::ADVENTURE_POWER_UPS:
        case Maps::ObjectGroup::ADVENTURE_TREASURES:
        case Maps::ObjectGroup::KINGDOM_HEROES:
        case Maps::ObjectGroup::LANDSCAPE_MOUNTAINS:
        case Maps::ObjectGroup::LANDSCAPE_ROCKS:
        case Maps::ObjectGroup::LANDSCAPE_TREES:
        case Maps::ObjectGroup::MONSTERS: {
            const auto & objectInfo = Maps::getObjectInfo( groupType, objectType );
            if ( !checkConditionForUsedTiles( objectInfo, tilePos, []( const Maps::Tiles & tileToCheck ) { return !tileToCheck.isWater(); } ) ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            break;
        }
        case Maps::ObjectGroup::ADVENTURE_WATER:
        case Maps::ObjectGroup::LANDSCAPE_WATER: {
            const auto & objectInfo = Maps::getObjectInfo( groupType, objectType );
            if ( !checkConditionForUsedTiles( objectInfo, tilePos, []( const Maps::Tiles & tileToCheck ) { return tileToCheck.isWater(); } ) ) {
                errorMessage = _( "%{objects} must be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            break;
        }
        case Maps::ObjectGroup::LANDSCAPE_MISCELLANEOUS: {
            const auto & objectInfo = Maps::getObjectInfo( groupType, objectType );

            assert( !objectInfo.groundLevelParts.empty() );
            const auto & firstObjectPart = objectInfo.groundLevelParts.front();

            // River deltas are only objects that can be placed on water and on land.
            // Yes, the below code is very hacky but so far this is the best we can do.
            if ( firstObjectPart.icnType == MP2::OBJ_ICN_TYPE_OBJNMUL2 && ( firstObjectPart.icnIndex == 2U || firstObjectPart.icnIndex == 11U ) ) {
                // This is a river delta. Just don't check the terrain type.
            }
            else if ( !checkConditionForUsedTiles( objectInfo, tilePos, []( const Maps::Tiles & tileToCheck ) { return !tileToCheck.isWater(); } ) ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            break;
        }
        case Maps::ObjectGroup::KINGDOM_TOWNS: {
            const Maps::Tiles & tile = world.GetTiles( tilePos.x, tilePos.y );

            if ( tile.isWater() ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            const int groundType = Maps::Ground::getGroundByImageIndex( tile.getTerrainImageIndex() );
            const int32_t basementId = fheroes2::getTownBasementId( groundType );

            const auto & townObjectInfo = Maps::getObjectInfo( groupType, objectType );
            const auto & basementObjectInfo = Maps::getObjectInfo( Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId );

            if ( !checkConditionForUsedTiles( townObjectInfo, tilePos, []( const Maps::Tiles & tileToCheck ) { return !tileToCheck.isWater(); } ) ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            if ( !checkConditionForUsedTiles( basementObjectInfo, tilePos, []( const Maps::Tiles & tileToCheck ) { return !tileToCheck.isWater(); } ) ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            break;
        }
        case Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS: {
            const auto & objectInfo = Maps::getObjectInfo( groupType, objectType );

            if ( objectInfo.objectType == MP2::OBJ_EVENT ) {
                // Only event objects are allowed to be placed anywhere.
            }
            else if ( !checkConditionForUsedTiles( objectInfo, tilePos, []( const Maps::Tiles & tileToCheck ) { return !tileToCheck.isWater(); } ) ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            break;
        }
        default:
            // Did you add a new object group? Add the logic for it!
            assert( 0 );
            break;
        }

        return true;
    }

    bool verifyObjectPlacement( const fheroes2::Point & tilePos, const Maps::ObjectGroup groupType, const int32_t objectType, std::string & errorMessage )
    {
        switch ( groupType ) {
        case Maps::ObjectGroup::ADVENTURE_ARTIFACTS:
        case Maps::ObjectGroup::ADVENTURE_DWELLINGS:
        case Maps::ObjectGroup::ADVENTURE_MINES:
        case Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS:
        case Maps::ObjectGroup::ADVENTURE_POWER_UPS:
        case Maps::ObjectGroup::ADVENTURE_TREASURES:
        case Maps::ObjectGroup::ADVENTURE_WATER:
        case Maps::ObjectGroup::KINGDOM_HEROES:
        case Maps::ObjectGroup::LANDSCAPE_MOUNTAINS:
        case Maps::ObjectGroup::LANDSCAPE_MISCELLANEOUS:
        case Maps::ObjectGroup::LANDSCAPE_ROCKS:
        case Maps::ObjectGroup::LANDSCAPE_TREES:
        case Maps::ObjectGroup::LANDSCAPE_WATER:
        case Maps::ObjectGroup::MONSTERS: {
            const auto & objectInfo = Maps::getObjectInfo( groupType, objectType );

            if ( !isObjectPlacementAllowed( objectInfo, tilePos ) ) {
                errorMessage = _( "Objects cannot be placed outside the map." );
                return false;
            }

            if ( !isActionObjectAllowed( objectInfo, tilePos ) ) {
                errorMessage = _( "Action objects must be placed on clear tiles." );
                return false;
            }
            break;
        }
        case Maps::ObjectGroup::KINGDOM_TOWNS: {
            const Maps::Tiles & tile = world.GetTiles( tilePos.x, tilePos.y );

            if ( tile.isWater() ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            const int groundType = Maps::Ground::getGroundByImageIndex( tile.getTerrainImageIndex() );
            const int32_t basementId = fheroes2::getTownBasementId( groundType );

            const auto & townObjectInfo = Maps::getObjectInfo( groupType, objectType );
            const auto & basementObjectInfo = Maps::getObjectInfo( Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId );

            if ( !isObjectPlacementAllowed( townObjectInfo, tilePos ) || !isObjectPlacementAllowed( basementObjectInfo, tilePos ) ) {
                errorMessage = _( "Objects cannot be placed outside the map." );
                return false;
            }

            if ( !isActionObjectAllowed( townObjectInfo, tilePos ) ) {
                errorMessage = _( "Action objects must be placed on clear tiles." );
                return false;
            }
            break;
        }
        default: {
            // Did you add a new object group? Add the logic for it!
            assert( 0 );
            return false;
        }
        }

        return verifyTerrainPlacement( tilePos, groupType, objectType, errorMessage );
    }

    template <class T>
    bool replaceKey( std::map<uint32_t, T> & data, const uint32_t oldKey, const uint32_t newKey )
    {
        auto iter = data.find( oldKey );
        if ( iter == data.end() ) {
            return false;
        }

        auto node = data.extract( iter );
        assert( !node.empty() );

        node.key() = newKey;

        data.insert( std::move( node ) );
        return true;
    }
}

namespace Interface
{
    void EditorInterface::reset()
    {
        const HideInterfaceModeDisabler hideInterfaceModeDisabler;

        const fheroes2::Display & display = fheroes2::Display::instance();

        const int32_t xOffset = display.width() - BORDERWIDTH - RADARWIDTH;
        _radar.SetPos( xOffset, BORDERWIDTH );

        _editorPanel.setPos( xOffset, _radar.GetArea().y + _radar.GetArea().height + ( ( display.height() > display.DEFAULT_HEIGHT + BORDERWIDTH ) ? BORDERWIDTH : 0 ) );

        const fheroes2::Point prevCenter = _gameArea.getCurrentCenterInPixels();
        const fheroes2::Rect prevRoi = _gameArea.GetROI();

        _gameArea.SetAreaPosition( BORDERWIDTH, BORDERWIDTH, display.width() - RADARWIDTH - 3 * BORDERWIDTH, display.height() - 2 * BORDERWIDTH );

        const fheroes2::Rect newRoi = _gameArea.GetROI();

        _gameArea.SetCenterInPixels( prevCenter + fheroes2::Point( newRoi.x + newRoi.width / 2, newRoi.y + newRoi.height / 2 )
                                     - fheroes2::Point( prevRoi.x + prevRoi.width / 2, prevRoi.y + prevRoi.height / 2 ) );
    }

    void EditorInterface::redraw( const uint32_t force )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const uint32_t combinedRedraw = _redraw | force;

        if ( combinedRedraw & REDRAW_GAMEAREA ) {
            int renderFlags = LEVEL_OBJECTS | LEVEL_HEROES | LEVEL_ROUTES;
            if ( combinedRedraw & REDRAW_PASSABILITIES ) {
                renderFlags |= LEVEL_PASSABILITIES;
            }

            // Render all except the fog.
            _gameArea.Redraw( display, renderFlags );

            if ( _warningMessage.isValid() ) {
                const fheroes2::Rect & roi = _gameArea.GetROI();

                fheroes2::Text text{ _warningMessage.message(), fheroes2::FontType::normalWhite() };
                // Keep 4 pixels from each edge.
                text.fitToOneRow( roi.width - 8 );

                text.draw( roi.x + 4, roi.y + roi.height - text.height() - 4, display );
            }

            // TODO:: Render horizontal and vertical map tiles scale and highlight with yellow text cursor position.

            if ( _editorPanel.showAreaSelectRect() && ( _tileUnderCursor > -1 ) ) {
                const fheroes2::Rect brushSize = _editorPanel.getBrushArea();

                if ( brushSize.width > 0 && brushSize.height > 0 ) {
                    const fheroes2::Point indices = getBrushAreaIndicies( brushSize, _tileUnderCursor );

                    assert( Maps::isValidAbsIndex( indices.x ) );
                    const bool isActionObject = ( _editorPanel.isDetailEdit() && brushSize.width == 1 && brushSize.height == 1
                                                  && MP2::isOffGameActionObject( world.GetTiles( indices.x ).GetObject() ) );

                    _gameArea.renderTileAreaSelect( display, indices.x, indices.y, isActionObject );
                }
                else if ( _editorPanel.isTerrainEdit() || _editorPanel.isEraseMode() ) {
                    assert( brushSize == fheroes2::Rect() );
                    // Render area selection from the tile where the left mouse button was pressed till the tile under the cursor.
                    _gameArea.renderTileAreaSelect( display, _selectedTile, _tileUnderCursor, false );
                }
            }
        }

        if ( combinedRedraw & ( REDRAW_RADAR_CURSOR | REDRAW_RADAR ) ) {
            // Render the mini-map without fog.
            _radar.redrawForEditor( combinedRedraw & REDRAW_RADAR );
        }

        if ( combinedRedraw & REDRAW_BORDER ) {
            // Game border for the View World are the same as for the Map Editor.
            GameBorderRedraw( true );
        }

        if ( combinedRedraw & REDRAW_PANEL ) {
            _editorPanel._redraw();
        }

        _redraw = 0;
    }

    EditorInterface & EditorInterface::Get()
    {
        static EditorInterface editorInterface;
        return editorInterface;
    }

    fheroes2::GameMode EditorInterface::startEdit( const bool isNewMap )
    {
        // The Editor has a special option to disable animation. This affects cycling animation as well.
        // First, we disable it to make sure to enable it back while exiting this function.
        const fheroes2::ScreenPaletteRestorer restorer;

        const Settings & conf = Settings::Get();

        if ( conf.isEditorAnimationEnabled() ) {
            fheroes2::RenderProcessor::instance().startColorCycling();
        }

        reset();

        _historyManager.reset();

        if ( isNewMap ) {
            _mapFormat = {};
            Maps::saveMapInEditor( _mapFormat );
            _loadedFileName.clear();
        }

        // Stop all sounds and music.
        AudioManager::ResetAudio();

        _radar.Build();
        _radar.SetHide( false );

        fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

        _gameArea.SetUpdateCursor();

        uint32_t redrawFlags = REDRAW_GAMEAREA | REDRAW_RADAR | REDRAW_PANEL | REDRAW_STATUS | REDRAW_BORDER;
        if ( conf.isEditorPassabilityEnabled() ) {
            redrawFlags |= REDRAW_PASSABILITIES;
        }

        setRedraw( redrawFlags );

        int32_t fastScrollRepeatCount = 0;
        const int32_t fastScrollStartThreshold = 2;

        bool isCursorOverGamearea = false;

        const std::vector<Game::DelayType> delayTypes = { Game::MAPS_DELAY };

        LocalEvent & le = LocalEvent::Get();
        Cursor & cursor = Cursor::Get();

        while ( res == fheroes2::GameMode::CANCEL ) {
            if ( !le.HandleEvents( Game::isDelayNeeded( delayTypes ), true ) ) {
                if ( EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                    res = fheroes2::GameMode::QUIT_GAME;
                    break;
                }
                continue;
            }

            // Process hot-keys.
            if ( le.isAnyKeyPressed() ) {
                // adventure map control
                if ( HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_QUIT ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                    res = EventExit();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::EDITOR_NEW_MAP_MENU ) ) {
                    res = eventNewMap();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SAVE_GAME ) ) {
                    saveMapToFile();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME ) ) {
                    res = eventLoadMap();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_FILE_OPTIONS ) ) {
                    res = eventFileDialog();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCENARIO_INFORMATION ) ) {
                    openMapSpecificationsDialog();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_VIEW_WORLD ) ) {
                    eventViewWorld();
                }
                // map scrolling control
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_LEFT ) ) {
                    _gameArea.SetScroll( SCROLL_LEFT );
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_RIGHT ) ) {
                    _gameArea.SetScroll( SCROLL_RIGHT );
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_UP ) ) {
                    _gameArea.SetScroll( SCROLL_TOP );
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_DOWN ) ) {
                    _gameArea.SetScroll( SCROLL_BOTTOM );
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::EDITOR_UNDO_LAST_ACTION ) ) {
                    undoAction();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::EDITOR_REDO_LAST_ACTION ) ) {
                    redoAction();
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::EDITOR_TO_GAME_MAIN_MENU ) ) {
                    const int returnValue
                        = fheroes2::showStandardTextMessage( _( "Editor" ), _( "Do you wish to return to the game's Main Menu? All unsaved changes will be lost." ),
                                                             Dialog::YES | Dialog::NO );

                    if ( returnValue == Dialog::YES ) {
                        return fheroes2::GameMode::MAIN_MENU;
                    }
                }
            }

            if ( res != fheroes2::GameMode::CANCEL ) {
                break;
            }

            if ( fheroes2::cursor().isFocusActive() && !_gameArea.isDragScroll() && !_radar.isDragRadar() && ( conf.ScrollSpeed() != SCROLL_SPEED_NONE ) ) {
                int scrollPosition = SCROLL_NONE;

                if ( isScrollLeft( le.getMouseCursorPos() ) )
                    scrollPosition |= SCROLL_LEFT;
                else if ( isScrollRight( le.getMouseCursorPos() ) )
                    scrollPosition |= SCROLL_RIGHT;
                if ( isScrollTop( le.getMouseCursorPos() ) )
                    scrollPosition |= SCROLL_TOP;
                else if ( isScrollBottom( le.getMouseCursorPos() ) )
                    scrollPosition |= SCROLL_BOTTOM;

                if ( scrollPosition != SCROLL_NONE ) {
                    if ( Game::validateAnimationDelay( Game::SCROLL_START_DELAY ) && ( fastScrollRepeatCount < fastScrollStartThreshold ) ) {
                        ++fastScrollRepeatCount;
                    }

                    if ( fastScrollRepeatCount >= fastScrollStartThreshold ) {
                        _gameArea.SetScroll( scrollPosition );
                    }
                }
                else {
                    fastScrollRepeatCount = 0;
                }
            }
            else {
                fastScrollRepeatCount = 0;
            }

            isCursorOverGamearea = false;

            // cursor is over the radar
            if ( le.isMouseCursorPosInArea( _radar.GetRect() ) ) {
                cursor.SetThemes( Cursor::POINTER );

                // TODO: Add checks for object placing/moving, and other Editor functions that uses mouse dragging.
                if ( !_gameArea.isDragScroll() && ( _editorPanel.getBrushArea().width > 0 || _selectedTile == -1 ) ) {
                    _radar.QueueEventProcessing();
                }
            }
            // cursor is over the game area
            else if ( le.isMouseCursorPosInArea( _gameArea.GetROI() ) && !_gameArea.NeedScroll() ) {
                isCursorOverGamearea = true;
            }
            // cursor is over the buttons area
            else if ( le.isMouseCursorPosInArea( _editorPanel.getRect() ) ) {
                cursor.SetThemes( Cursor::POINTER );

                if ( !_gameArea.NeedScroll() ) {
                    res = _editorPanel.queueEventProcessing();
                }
            }
            // cursor is somewhere else
            else if ( !_gameArea.NeedScroll() ) {
                cursor.SetThemes( Cursor::POINTER );

                _gameArea.ResetCursorPosition();
            }

            // gamearea
            if ( !_gameArea.NeedScroll() ) {
                if ( !_radar.isDragRadar() ) {
                    _gameArea.QueueEventProcessing( isCursorOverGamearea );
                }
                else if ( !le.isMouseLeftButtonPressed() ) {
                    _radar.QueueEventProcessing();
                }
            }

            if ( isCursorOverGamearea ) {
                // Get tile index under the cursor.
                const int32_t tileIndex = _gameArea.GetValidTileIdFromPoint( le.getMouseCursorPos() );
                const fheroes2::Rect brushSize = _editorPanel.getBrushArea();

                if ( _tileUnderCursor != tileIndex ) {
                    _tileUnderCursor = tileIndex;

                    // Force redraw if cursor position was changed as area rectangle is also changed.
                    if ( _editorPanel.showAreaSelectRect() && ( brushSize.width > 0 || _selectedTile != -1 ) ) {
                        _redraw |= REDRAW_GAMEAREA;
                    }
                }

                if ( _selectedTile == -1 && tileIndex != -1 && brushSize.width == 0 && le.isMouseLeftButtonPressed() ) {
                    _selectedTile = tileIndex;
                    _redraw |= REDRAW_GAMEAREA;
                }
            }
            else if ( _tileUnderCursor != -1 ) {
                _tileUnderCursor = -1;
                _redraw |= REDRAW_GAMEAREA;
            }

            if ( _selectedTile > -1 && le.isMouseLeftButtonReleased() ) {
                if ( isCursorOverGamearea && _tileUnderCursor > -1 && _editorPanel.getBrushArea().width == 0 ) {
                    if ( _editorPanel.isTerrainEdit() ) {
                        // Fill the selected area in terrain edit mode.
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );

                        const int groundId = _editorPanel.selectedGroundType();
                        Maps::setTerrainOnTiles( _selectedTile, _tileUnderCursor, groundId );
                        _validateObjectsOnTerrainUpdate();

                        action.commit();

                        _redraw |= mapUpdateFlags;

                        // TODO: Make a proper function to remove all types of objects from the 'world tiles' not to do full reload of '_mapFormat'.
                        Maps::readMapInEditor( _mapFormat );
                    }
                    else if ( _editorPanel.isEraseMode() ) {
                        // Erase objects in the selected area.
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );

                        if ( removeObjects( _mapFormat, Maps::getObjectUidsInArea( _selectedTile, _tileUnderCursor ), _editorPanel.getEraseObjectGroups() ) ) {
                            action.commit();
                            _redraw |= mapUpdateFlags;

                            // TODO: Make a proper function to remove all types of objects from the 'world tiles' not to do full reload of '_mapFormat'.
                            Maps::readMapInEditor( _mapFormat );
                        }
                    }
                }

                // Reset the area start tile.
                _selectedTile = -1;

                _redraw |= mapUpdateFlags;
            }

            // fast scroll
            if ( ( Game::validateAnimationDelay( Game::SCROLL_DELAY ) && _gameArea.NeedScroll() ) || _gameArea.needDragScrollRedraw() ) {
                if ( ( isScrollLeft( le.getMouseCursorPos() ) || isScrollRight( le.getMouseCursorPos() ) || isScrollTop( le.getMouseCursorPos() )
                       || isScrollBottom( le.getMouseCursorPos() ) )
                     && !_gameArea.isDragScroll() ) {
                    cursor.SetThemes( _gameArea.GetScrollCursor() );
                }

                _gameArea.Scroll();

                _redraw |= REDRAW_GAMEAREA | REDRAW_RADAR_CURSOR;
            }

            if ( res == fheroes2::GameMode::CANCEL ) {
                // map objects animation
                if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
                    if ( conf.isEditorAnimationEnabled() ) {
                        Game::updateAdventureMapAnimationIndex();
                    }
                    _redraw |= REDRAW_GAMEAREA;
                }

                if ( needRedraw() ) {
                    if ( conf.isEditorPassabilityEnabled() ) {
                        _redraw |= REDRAW_PASSABILITIES;
                    }
                    redraw( 0 );

                    // If this assertion blows up it means that we are holding a RedrawLocker lock for rendering which should not happen.
                    assert( getRedrawMask() == 0 );

                    validateFadeInAndRender();
                }
            }
        }

        Game::setDisplayFadeIn();

        fheroes2::fadeOutDisplay();

        return res;
    }

    fheroes2::GameMode EditorInterface::eventLoadMap()
    {
        return Dialog::YES
                       == fheroes2::showStandardTextMessage( "", _( "Are you sure you want to load a new map? (Any unsaved changes to the current map will be lost.)" ),
                                                             Dialog::YES | Dialog::NO )
                   ? fheroes2::GameMode::EDITOR_LOAD_MAP
                   : fheroes2::GameMode::CANCEL;
    }

    fheroes2::GameMode EditorInterface::eventNewMap()
    {
        return Dialog::YES
                       == fheroes2::showStandardTextMessage( "", _( "Are you sure you want to create a new map? (Any unsaved changes to the current map will be lost.)" ),
                                                             Dialog::YES | Dialog::NO )
                   ? fheroes2::GameMode::EDITOR_NEW_MAP
                   : fheroes2::GameMode::CANCEL;
    }

    fheroes2::GameMode EditorInterface::eventFileDialog()
    {
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        const int cpanbkg = isEvilInterface ? ICN::CPANBKGE : ICN::CPANBKG;
        const fheroes2::Sprite & background = fheroes2::AGG::GetICN( cpanbkg, 0 );

        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        // Since the original image contains shadow it is important to remove it from calculation of window's position.
        const fheroes2::Point rb( ( display.width() - background.width() - BORDERWIDTH ) / 2, ( display.height() - background.height() + BORDERWIDTH ) / 2 );
        fheroes2::ImageRestorer back( display, rb.x, rb.y, background.width(), background.height() );
        fheroes2::Blit( background, display, rb.x, rb.y );

        fheroes2::Button buttonNew( rb.x + 62, rb.y + 31, isEvilInterface ? ICN::BUTTON_NEW_MAP_EVIL : ICN::BUTTON_NEW_MAP_GOOD, 0, 1 );
        fheroes2::Button buttonLoad( rb.x + 195, rb.y + 31, isEvilInterface ? ICN::BUTTON_LOAD_MAP_EVIL : ICN::BUTTON_LOAD_MAP_GOOD, 0, 1 );
        fheroes2::Button buttonSave( rb.x + 62, rb.y + 107, isEvilInterface ? ICN::BUTTON_SAVE_MAP_EVIL : ICN::BUTTON_SAVE_MAP_GOOD, 0, 1 );
        fheroes2::Button buttonQuit( rb.x + 195, rb.y + 107, isEvilInterface ? ICN::BUTTON_QUIT_EVIL : ICN::BUTTON_QUIT_GOOD, 0, 1 );
        fheroes2::Button buttonCancel( rb.x + 128, rb.y + 184, isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1 );

        buttonNew.draw();
        buttonLoad.draw();
        buttonSave.draw();
        buttonQuit.draw();
        buttonCancel.draw();

        display.render( back.rect() );

        fheroes2::GameMode result = fheroes2::GameMode::CANCEL;

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            le.isMouseLeftButtonPressedInArea( buttonNew.area() ) ? buttonNew.drawOnPress() : buttonNew.drawOnRelease();
            le.isMouseLeftButtonPressedInArea( buttonLoad.area() ) ? buttonLoad.drawOnPress() : buttonLoad.drawOnRelease();
            le.isMouseLeftButtonPressedInArea( buttonSave.area() ) ? buttonSave.drawOnPress() : buttonSave.drawOnRelease();
            le.isMouseLeftButtonPressedInArea( buttonQuit.area() ) ? buttonQuit.drawOnPress() : buttonQuit.drawOnRelease();
            le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

            if ( le.MouseClickLeft( buttonNew.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_NEW_MAP_MENU ) ) {
                if ( eventNewMap() == fheroes2::GameMode::EDITOR_NEW_MAP ) {
                    result = fheroes2::GameMode::EDITOR_NEW_MAP;
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonLoad.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME ) ) {
                if ( eventLoadMap() == fheroes2::GameMode::EDITOR_LOAD_MAP ) {
                    result = fheroes2::GameMode::EDITOR_LOAD_MAP;
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonSave.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_SAVE_GAME ) ) {
                back.restore();

                Get().saveMapToFile();

                break;
            }
            else if ( le.MouseClickLeft( buttonQuit.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_QUIT ) ) {
                if ( EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                    result = fheroes2::GameMode::QUIT_GAME;
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyCloseWindow() ) {
                break;
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonNew.area() ) ) {
                // TODO: update this text once random map generator is ready.
                //       The original text should be "Create a new map, either from scratch or using the random map generator."
                fheroes2::showStandardTextMessage( _( "New Map" ), _( "Create a new map from scratch." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonLoad.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Load Map" ), _( "Load an existing map." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonSave.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Save Map" ), _( "Save the current map." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonQuit.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Quit" ), _( "Quit out of the map editor." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
        }

        // restore background
        back.restore();
        display.render( back.rect() );

        return result;
    }

    void EditorInterface::eventViewWorld()
    {
        // TODO: Make proper borders restoration for low height resolutions, like for hide interface mode.
        ViewWorld::ViewWorldWindow( 0, ViewWorldMode::ViewAll, *this );
    }

    void EditorInterface::mouseCursorAreaClickLeft( const int32_t tileIndex )
    {
        assert( tileIndex >= 0 && tileIndex < static_cast<int32_t>( world.getSize() ) );

        Maps::Tiles & tile = world.GetTiles( tileIndex );

        if ( _editorPanel.isDetailEdit() ) {
            for ( const auto & object : _mapFormat.tiles[tileIndex].objects ) {
                const auto & objectGroupInfo = Maps::getObjectsByGroup( object.group );
                assert( object.index <= objectGroupInfo.size() );

                const auto & objectInfo = objectGroupInfo[object.index];
                assert( !objectInfo.groundLevelParts.empty() );

                const MP2::MapObjectType objectType = objectInfo.groundLevelParts.front().objectType;

                const bool isActionObject = MP2::isOffGameActionObject( objectType );
                if ( !isActionObject ) {
                    // Only action objects can have metadata.
                    continue;
                }

                if ( objectType == MP2::OBJ_HERO || objectType == MP2::OBJ_JAIL ) {
                    assert( _mapFormat.heroMetadata.find( object.id ) != _mapFormat.heroMetadata.end() );

                    const int color = ( objectType == MP2::OBJ_JAIL ) ? Color::NONE : ( 1 << objectInfo.metadata[0] );

                    // Make a temporary hero to edit his details.
                    Heroes hero;
                    hero.SetColor( color );
                    hero.applyHeroMetadata( _mapFormat.heroMetadata[object.id], objectType == MP2::OBJ_JAIL, true );

                    fheroes2::ActionCreator action( _historyManager, _mapFormat );
                    hero.OpenDialog( false, false, true, true, true, true );
                    Maps::Map_Format::HeroMetadata heroNewMetadata = hero.getHeroMetadata();
                    if ( heroNewMetadata != _mapFormat.heroMetadata[object.id] ) {
                        _mapFormat.heroMetadata[object.id] = std::move( heroNewMetadata );
                        action.commit();
                    }
                }
                else if ( objectType == MP2::OBJ_CASTLE || objectType == MP2::OBJ_RANDOM_TOWN || objectType == MP2::OBJ_RANDOM_CASTLE ) {
                    assert( _mapFormat.castleMetadata.find( object.id ) != _mapFormat.castleMetadata.end() );

                    const int race = Race::IndexToRace( static_cast<int>( objectInfo.metadata[0] ) );
                    const int color = Color::IndexToColor( Maps::getTownColorIndex( _mapFormat, tileIndex, object.id ) );
                    Editor::castleDetailsDialog( _mapFormat.castleMetadata[object.id], race, color );
                }
                else if ( objectType == MP2::OBJ_SIGN || objectType == MP2::OBJ_BOTTLE ) {
                    fheroes2::ActionCreator action( _historyManager, _mapFormat );

                    std::string header = _( "Input %{object} text" );
                    StringReplace( header, "%{object}", MP2::StringObject( objectType ) );

                    std::string signText = _mapFormat.signMetadata[object.id].message;
                    if ( Dialog::inputString( std::move( header ), signText, {}, 0, true, true ) ) {
                        _mapFormat.signMetadata[object.id].message = std::move( signText );
                        action.commit();
                    }
                }
                else if ( objectType == MP2::OBJ_EVENT ) {
                    assert( _mapFormat.adventureMapEventMetadata.find( object.id ) != _mapFormat.adventureMapEventMetadata.end() );

                    fheroes2::ActionCreator action( _historyManager, _mapFormat );
                    if ( Editor::eventDetailsDialog( _mapFormat.adventureMapEventMetadata[object.id], _mapFormat.humanPlayerColors, _mapFormat.computerPlayerColors ) ) {
                        action.commit();
                    }
                }
                else if ( object.group == Maps::ObjectGroup::MONSTERS ) {
                    uint32_t monsterCount = 0;

                    auto monsterMetadata = _mapFormat.standardMetadata.find( object.id );
                    if ( monsterMetadata != _mapFormat.standardMetadata.end() ) {
                        monsterCount = monsterMetadata->second.metadata[0];
                    }

                    const Monster tempMonster( static_cast<int>( object.index ) + 1 );

                    std::string str = _( "Set %{monster} Count" );
                    StringReplace( str, "%{monster}", tempMonster.GetName() );

                    fheroes2::Sprite surface;

                    if ( tempMonster.isValid() ) {
                        surface = fheroes2::AGG::GetICN( ICN::STRIP, 12 );
                        fheroes2::renderMonsterFrame( tempMonster, surface, { 6, 6 } );
                    }

                    fheroes2::ActionCreator action( _historyManager, _mapFormat );
                    if ( Dialog::SelectCount( str, 0, 500000, monsterCount, 1, surface ) ) {
                        _mapFormat.standardMetadata[object.id] = { static_cast<int32_t>( monsterCount ), 0, Monster::JOIN_CONDITION_UNSET };
                        action.commit();
                    }
                }
                else if ( object.group == Maps::ObjectGroup::ADVENTURE_ARTIFACTS ) {
                    if ( objectInfo.objectType == MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT ) {
                        assert( _mapFormat.standardMetadata.find( object.id ) != _mapFormat.standardMetadata.end() );

                        uint32_t radius = static_cast<uint32_t>( _mapFormat.standardMetadata[object.id].metadata[0] );

                        fheroes2::ActionCreator action( _historyManager, _mapFormat );
                        if ( Dialog::SelectCount( _( "Set Random Ultimate Artifact Radius" ), 0, 100, radius ) ) {
                            _mapFormat.standardMetadata[object.id].metadata[0] = static_cast<int32_t>( radius );
                            action.commit();
                        }
                    }
                    else if ( objectInfo.objectType == MP2::OBJ_ARTIFACT && objectInfo.metadata[0] == Artifact::SPELL_SCROLL ) {
                        // Find Spell Scroll object.
                        assert( _mapFormat.standardMetadata.find( object.id ) != _mapFormat.standardMetadata.end() );

                        const int spellId = Dialog::selectSpell( _mapFormat.standardMetadata[object.id].metadata[0], true ).GetID();

                        if ( spellId == Spell::NONE ) {
                            // We do not place the Spell Scroll artifact if the spell for it was not selected.
                            return;
                        }

                        fheroes2::ActionCreator action( _historyManager, _mapFormat );

                        _mapFormat.standardMetadata[object.id].metadata[0] = spellId;

                        Maps::setSpellOnTile( tile, spellId );

                        action.commit();
                    }
                    else {
                        std::string msg = _( "%{object} has no properties to change." );
                        StringReplace( msg, "%{object}", _( "This artifact" ) );
                        _warningMessage.reset( std::move( msg ) );
                    }
                }
                else if ( objectType == MP2::OBJ_SPHINX ) {
                    assert( _mapFormat.sphinxMetadata.find( object.id ) != _mapFormat.sphinxMetadata.end() );

                    fheroes2::ActionCreator action( _historyManager, _mapFormat );
                    if ( Editor::openSphinxWindow( _mapFormat.sphinxMetadata[object.id] ) ) {
                        action.commit();
                    }
                }
                else {
                    std::string msg = _( "%{object} has no properties to change." );
                    StringReplace( msg, "%{object}", MP2::StringObject( objectType ) );
                    _warningMessage.reset( std::move( msg ) );
                }
            }
        }
        else if ( _editorPanel.isTerrainEdit() ) {
            const fheroes2::Rect brushSize = _editorPanel.getBrushArea();
            assert( brushSize.width == brushSize.height );

            const int groundId = _editorPanel.selectedGroundType();

            fheroes2::ActionCreator action( _historyManager, _mapFormat );

            if ( brushSize.width > 0 ) {
                const fheroes2::Point indices = getBrushAreaIndicies( brushSize, tileIndex );

                Maps::setTerrainOnTiles( indices.x, indices.y, groundId );
            }
            else {
                assert( brushSize.width == 0 );

                // This is a case when area was not selected but a single tile was clicked.
                Maps::setTerrainOnTiles( tileIndex, tileIndex, groundId );

                _selectedTile = -1;
            }

            _validateObjectsOnTerrainUpdate();

            _redraw |= mapUpdateFlags;

            action.commit();

            // TODO: Make a proper function to remove all types of objects from the 'world tiles' not to do full reload of '_mapFormat'.
            Maps::readMapInEditor( _mapFormat );
        }
        else if ( _editorPanel.isRoadDraw() ) {
            if ( tile.isWater() ) {
                std::string msg = _( "%{objects} cannot be placed on water." );
                StringReplace( msg, "%{objects}", _( "Roads" ) );
                _warningMessage.reset( std::move( msg ) );
                return;
            }

            fheroes2::ActionCreator action( _historyManager, _mapFormat );

            if ( Maps::updateRoadOnTile( tile, true ) ) {
                _redraw |= mapUpdateFlags;

                action.commit();
            }
        }
        else if ( _editorPanel.isStreamDraw() ) {
            if ( tile.isWater() ) {
                std::string msg = _( "%{objects} cannot be placed on water." );
                StringReplace( msg, "%{objects}", _( "Streams" ) );
                _warningMessage.reset( std::move( msg ) );
                return;
            }

            fheroes2::ActionCreator action( _historyManager, _mapFormat );

            if ( Maps::updateStreamOnTile( tile, true ) ) {
                _redraw |= mapUpdateFlags;

                action.commit();
            }
        }
        else if ( _editorPanel.isEraseMode() ) {
            const fheroes2::Rect brushSize = _editorPanel.getBrushArea();
            assert( brushSize.width == brushSize.height );

            fheroes2::ActionCreator action( _historyManager, _mapFormat );

            const fheroes2::Point indices = getBrushAreaIndicies( brushSize, tileIndex );
            if ( removeObjects( _mapFormat, Maps::getObjectUidsInArea( indices.x, indices.y ), _editorPanel.getEraseObjectGroups() ) ) {
                action.commit();
                _redraw |= mapUpdateFlags;

                // TODO: Make a proper function to remove all types of objects from the 'world tiles' not to do full reload of '_mapFormat'.
                Maps::readMapInEditor( _mapFormat );
            }

            if ( brushSize.width == 0 ) {
                // This is a case when area was not selected but a single tile was clicked.
                _selectedTile = -1;
            }
        }
        else if ( _editorPanel.isObjectMode() ) {
            _handleObjectMouseLeftClick( tile );
        }
    }

    void EditorInterface::_handleObjectMouseLeftClick( Maps::Tiles & tile )
    {
        assert( _editorPanel.isObjectMode() );

        const int32_t objectType = _editorPanel.getSelectedObjectType();
        if ( objectType < 0 ) {
            return;
        }

        const fheroes2::Point tilePos = tile.GetCenter();

        const Maps::ObjectGroup groupType = _editorPanel.getSelectedObjectGroup();

        if ( _moveExistingObject( tile.GetIndex(), groupType, objectType ) ) {
            return;
        }

        std::string errorMessage;

        if ( groupType == Maps::ObjectGroup::KINGDOM_HEROES ) {
            if ( !verifyObjectPlacement( tilePos, groupType, objectType, errorMessage ) ) {
                _warningMessage.reset( std::move( errorMessage ) );
                return;
            }

            const auto & objectInfo = Maps::getObjectInfo( groupType, objectType );

            // Heroes are limited to 8 per color so all attempts to set more than 8 heroes must be prevented.
            const auto & objects = Maps::getObjectsByGroup( groupType );

            const uint32_t color = objectInfo.metadata[0];
            size_t heroCount = 0;
            for ( const auto & mapTile : _mapFormat.tiles ) {
                for ( const auto & object : mapTile.objects ) {
                    if ( object.group == groupType ) {
                        assert( object.index < objects.size() );
                        if ( objects[object.index].metadata[0] == color ) {
                            ++heroCount;
                        }
                    }
                }
            }

            if ( heroCount >= GameStatic::GetKingdomMaxHeroes() ) {
                std::string warning( _( "A maximum of %{count} heroes of the same color can be placed on the map." ) );
                StringReplace( warning, "%{count}", GameStatic::GetKingdomMaxHeroes() );
                _warningMessage.reset( std::move( warning ) );
                return;
            }

            if ( !_setObjectOnTileAsAction( tile, groupType, objectType ) ) {
                return;
            }

            Heroes * hero = world.GetHeroForHire( Race::IndexToRace( static_cast<int>( objectInfo.metadata[1] ) ) );
            if ( hero ) {
                hero->SetCenter( tilePos );
                hero->SetColor( Color::IndexToColor( static_cast<int>( color ) ) );
            }
            else {
                // How is it possible that the action was successful but no hero?
                assert( 0 );
            }

            if ( !Maps::updateMapPlayers( _mapFormat ) ) {
                _warningMessage.reset( _( "Failed to update player information." ) );
            }
        }
        else if ( groupType == Maps::ObjectGroup::ADVENTURE_ARTIFACTS ) {
            if ( !verifyObjectPlacement( tilePos, groupType, objectType, errorMessage ) ) {
                _warningMessage.reset( std::move( errorMessage ) );
                return;
            }

            const auto & objectInfo = Maps::getObjectInfo( groupType, objectType );

            if ( objectInfo.objectType == MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT ) {
                // First of all, verify that only one Random Ultimate artifact exists on the map.
                for ( const auto & tileInfo : _mapFormat.tiles ) {
                    for ( const auto & object : tileInfo.objects ) {
                        if ( groupType == object.group && static_cast<uint32_t>( objectType ) == object.index ) {
                            _warningMessage.reset( _( "Only one Random Ultimate Artifact can be placed on the map." ) );
                            return;
                        }
                    }
                }
            }

            // For each Spell Scroll artifact we select a spell.
            if ( objectInfo.objectType == MP2::OBJ_ARTIFACT && objectInfo.metadata[0] == Artifact::SPELL_SCROLL ) {
                const int spellId = Dialog::selectSpell( Spell::RANDOM, true ).GetID();

                if ( spellId == Spell::NONE ) {
                    // We do not place the Spell Scroll artifact if the spell for it was not selected.
                    return;
                }

                if ( !_setObjectOnTileAsAction( tile, groupType, objectType ) ) {
                    return;
                }

                assert( !_mapFormat.tiles[tile.GetIndex()].objects.empty() );

                const auto & insertedObject = _mapFormat.tiles[tile.GetIndex()].objects.back();
                assert( insertedObject.group == groupType && insertedObject.index == static_cast<uint32_t>( objectType ) );
                assert( _mapFormat.standardMetadata.find( insertedObject.id ) != _mapFormat.standardMetadata.end() );

                _mapFormat.standardMetadata[insertedObject.id].metadata[0] = spellId;

                Maps::setSpellOnTile( tile, spellId );
            }
            else {
                _setObjectOnTileAsAction( tile, groupType, objectType );
            }
        }
        else if ( groupType == Maps::ObjectGroup::KINGDOM_TOWNS ) {
            int32_t type = -1;
            int32_t color = -1;
            _editorPanel.getTownObjectProperties( type, color );
            if ( type < 0 || color < 0 ) {
                // Check your logic!
                assert( 0 );
                return;
            }

            if ( !verifyObjectPlacement( tilePos, groupType, type, errorMessage ) ) {
                _warningMessage.reset( std::move( errorMessage ) );
                return;
            }

            const int groundType = Maps::Ground::getGroundByImageIndex( tile.getTerrainImageIndex() );
            const int32_t basementId = fheroes2::getTownBasementId( groundType );

            const auto & townObjectInfo = Maps::getObjectInfo( groupType, type );

            fheroes2::ActionCreator action( _historyManager, _mapFormat );

            if ( !_setObjectOnTile( tile, Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId ) ) {
                return;
            }

            // Since the whole object consists of multiple "objects" we have to put the same ID for all of them.
            // Every time an object is being placed on a map the counter is going to be increased by 1.
            // Therefore, we set the counter by 1 less for each object to match object UID for all of them.
            assert( Maps::getLastObjectUID() > 0 );
            const uint32_t objectId = Maps::getLastObjectUID() - 1;

            Maps::setLastObjectUID( objectId );

            if ( !_setObjectOnTile( tile, groupType, type ) ) {
                return;
            }

            // By default use random (default) army for the neutral race town/castle.
            if ( Color::IndexToColor( color ) == Color::NONE ) {
                Maps::setDefaultCastleDefenderArmy( _mapFormat.castleMetadata[Maps::getLastObjectUID()] );
            }

            // Add flags.
            assert( tile.GetIndex() > 0 && tile.GetIndex() < world.w() * world.h() - 1 );
            Maps::setLastObjectUID( objectId );

            if ( !_setObjectOnTile( world.GetTiles( tile.GetIndex() - 1 ), Maps::ObjectGroup::LANDSCAPE_FLAGS, color * 2 ) ) {
                return;
            }

            Maps::setLastObjectUID( objectId );

            if ( !_setObjectOnTile( world.GetTiles( tile.GetIndex() + 1 ), Maps::ObjectGroup::LANDSCAPE_FLAGS, color * 2 + 1 ) ) {
                return;
            }

            world.addCastle( tile.GetIndex(), Race::IndexToRace( static_cast<int>( townObjectInfo.metadata[0] ) ), Color::IndexToColor( color ) );

            action.commit();

            if ( !Maps::updateMapPlayers( _mapFormat ) ) {
                _warningMessage.reset( _( "Failed to update player information." ) );
            }
        }
        else if ( groupType == Maps::ObjectGroup::ADVENTURE_MINES ) {
            int32_t type = -1;
            int32_t color = -1;

            _editorPanel.getMineObjectProperties( type, color );
            if ( type < 0 || color < 0 ) {
                // Check your logic!
                assert( 0 );
                return;
            }

            if ( !verifyObjectPlacement( tilePos, groupType, type, errorMessage ) ) {
                _warningMessage.reset( std::move( errorMessage ) );
                return;
            }

            fheroes2::ActionCreator action( _historyManager, _mapFormat );

            if ( !_setObjectOnTile( tile, groupType, type ) ) {
                return;
            }

            // TODO: Place owner flag according to the color state.
            action.commit();
        }
        else if ( groupType == Maps::ObjectGroup::LANDSCAPE_MISCELLANEOUS ) {
            if ( !verifyObjectPlacement( tilePos, groupType, objectType, errorMessage ) ) {
                _warningMessage.reset( std::move( errorMessage ) );
                return;
            }

            fheroes2::ActionCreator action( _historyManager, _mapFormat );

            if ( !_setObjectOnTile( tile, groupType, objectType ) ) {
                return;
            }

            // For River Deltas we update the nearby Streams to properly connect to them.
            const Maps::ObjectInfo & objectInfo = Maps::getObjectInfo( groupType, objectType );
            std::for_each( objectInfo.groundLevelParts.begin(), objectInfo.groundLevelParts.end(), [&tile]( const Maps::LayeredObjectPartInfo & info ) {
                if ( info.icnType != MP2::OBJ_ICN_TYPE_OBJNMUL2 && info.icnIndex != 0 && info.icnIndex != 13 ) {
                    return;
                }

                Maps::updateStreamsToDeltaConnection( tile, info.icnIndex == 13 );
            } );

            action.commit();
        }
        else {
            if ( !verifyObjectPlacement( tilePos, groupType, objectType, errorMessage ) ) {
                _warningMessage.reset( std::move( errorMessage ) );
                return;
            }

            _setObjectOnTileAsAction( tile, groupType, objectType );
        }
    }

    void EditorInterface::mouseCursorAreaPressRight( const int32_t tileIndex ) const
    {
        Editor::showPopupWindow( world.GetTiles( tileIndex ) );
    }

    void EditorInterface::updateCursor( const int32_t tileIndex )
    {
        if ( _cursorUpdater && tileIndex >= 0 ) {
            _cursorUpdater( tileIndex );
        }
        else {
            Cursor::Get().SetThemes( Cursor::POINTER );
        }
    }

    bool EditorInterface::_setObjectOnTile( Maps::Tiles & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex )
    {
        const auto & objectInfo = Maps::getObjectInfo( groupType, objectIndex );
        if ( objectInfo.empty() ) {
            // Check your logic as you are trying to insert an empty object!
            assert( 0 );
            return false;
        }

        _redraw |= mapUpdateFlags;

        if ( !Maps::setObjectOnTile( tile, objectInfo, true ) ) {
            return false;
        }

        Maps::addObjectToMap( _mapFormat, tile.GetIndex(), groupType, static_cast<uint32_t>( objectIndex ) );

        return true;
    }

    bool EditorInterface::_setObjectOnTileAsAction( Maps::Tiles & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex )
    {
        fheroes2::ActionCreator action( _historyManager, _mapFormat );

        if ( _setObjectOnTile( tile, groupType, objectIndex ) ) {
            action.commit();
            return true;
        }

        return false;
    }

    bool EditorInterface::loadMap( const std::string & filePath )
    {
        if ( !Maps::Map_Format::loadMap( filePath, _mapFormat ) ) {
            fheroes2::showStandardTextMessage( _( "Warning!" ), "Failed to load the map.", Dialog::OK );
            return false;
        }

        if ( !Maps::readMapInEditor( _mapFormat ) ) {
            fheroes2::showStandardTextMessage( _( "Warning!" ), "Failed to read the map.", Dialog::OK );
            return false;
        }

        _loadedFileName = System::truncateFileExtensionAndPath( filePath );

        // Set the loaded map as a default map for the new Standard Game.
        Maps::FileInfo fi;
        if ( fi.loadResurrectionMap( _mapFormat, filePath ) ) {
            Settings::Get().SetCurrentFileInfo( std::move( fi ) );
        }
        else {
            assert( 0 );
        }

        return true;
    }

    void EditorInterface::saveMapToFile()
    {
        if ( !Maps::updateMapPlayers( _mapFormat ) ) {
            fheroes2::showStandardTextMessage( _( "Warning!" ), _( "The map is corrupted." ), Dialog::OK );
            return;
        }

        const std::string dataPath = System::GetDataDirectory( "fheroes2" );
        if ( dataPath.empty() ) {
            fheroes2::showStandardTextMessage( _( "Warning!" ), _( "Unable to locate data directory to save the map." ), Dialog::OK );
            return;
        }

        const std::string mapDirectory = System::concatPath( dataPath, "maps" );

        if ( !System::IsDirectory( mapDirectory ) && !System::MakeDirectory( mapDirectory ) ) {
            fheroes2::showStandardTextMessage( _( "Warning!" ), _( "Unable to create a directory to save the map." ), Dialog::OK );
            return;
        }

        std::string fileName = _loadedFileName;
        std::string mapName = _mapFormat.name;
        std::string fullPath;

        while ( true ) {
            if ( !Editor::mapSaveSelectFile( fileName, mapName ) ) {
                return;
            }

            fullPath = System::concatPath( mapDirectory, fileName + ".fh2m" );

            if ( !System::IsFile( fullPath )
                 || fheroes2::showStandardTextMessage( "", _( "Are you sure you want to overwrite the existing map?" ), Dialog::YES | Dialog::NO ) == Dialog::YES ) {
                break;
            }
        }

        _mapFormat.name = std::move( mapName );
        _loadedFileName = std::move( fileName );

        if ( Maps::Map_Format::saveMap( fullPath, _mapFormat ) ) {
            // Set the saved map as a default map for the new Standard Game.
            Maps::FileInfo fi;
            if ( fi.loadResurrectionMap( _mapFormat, fullPath ) ) {
                Settings::Get().SetCurrentFileInfo( std::move( fi ) );
            }
            else {
                assert( 0 );
            }

            // On some OSes like Windows, the path may contain '\' symbols. This symbol doesn't exist in the resources.
            // To avoid this we have to replace all '\' symbols with '/' symbols.
            StringReplace( fullPath, "\\", "/" );

            _warningMessage.reset( _( "Map saved to: " ) + fullPath );

            return;
        }

        fheroes2::showStandardTextMessage( _( "Warning!" ), _( "Failed to save the map." ), Dialog::OK );
    }

    void EditorInterface::openMapSpecificationsDialog()
    {
        fheroes2::ActionCreator action( _historyManager, _mapFormat );

        if ( Editor::mapSpecificationsDialog( _mapFormat ) ) {
            action.commit();
        }
    }

    void EditorInterface::_validateObjectsOnTerrainUpdate()
    {
        std::string errorMessage;

        std::set<uint32_t> uids;

        for ( size_t i = 0; i < _mapFormat.tiles.size(); ++i ) {
            const fheroes2::Point pos{ static_cast<int32_t>( i ) % world.w(), static_cast<int32_t>( i ) / world.w() };

            bool removeRoadAndStream = false;

            for ( const auto & object : _mapFormat.tiles[i].objects ) {
                if ( object.group == Maps::ObjectGroup::LANDSCAPE_FLAGS || object.group == Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS ) {
                    // These objects belong to the main objects and will be checked with them.
                    continue;
                }

                if ( object.group == Maps::ObjectGroup::ROADS || object.group == Maps::ObjectGroup::STREAMS ) {
                    if ( world.GetTiles( static_cast<int32_t>( i ) ).isWater() ) {
                        removeRoadAndStream = true;
                    }

                    continue;
                }

                if ( !verifyTerrainPlacement( pos, object.group, static_cast<int32_t>( object.index ), errorMessage ) ) {
                    uids.emplace( object.id );
                }
            }

            if ( removeRoadAndStream ) {
                auto & worldTile = world.GetTiles( static_cast<int32_t>( i ) );

                Maps::updateRoadOnTile( worldTile, false );
                Maps::updateStreamOnTile( worldTile, false );
            }
        }

        if ( !uids.empty() ) {
            std::set<Maps::ObjectGroup> groups;
            for ( int32_t i = 0; i < static_cast<int32_t>( Maps::ObjectGroup::GROUP_COUNT ); ++i ) {
                groups.emplace( static_cast<Maps::ObjectGroup>( i ) );
            }

            removeObjects( _mapFormat, uids, groups );
        }

        // Run through each town and castle and update its terrain.
        for ( size_t i = 0; i < _mapFormat.tiles.size(); ++i ) {
            for ( auto & object : _mapFormat.tiles[i].objects ) {
                if ( object.group == Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS ) {
                    const auto & worldTile = world.GetTiles( static_cast<int32_t>( i ) );
                    const int groundType = Maps::Ground::getGroundByImageIndex( worldTile.getTerrainImageIndex() );
                    const int32_t basementId = fheroes2::getTownBasementId( groundType );
                    object.index = static_cast<uint32_t>( basementId );
                }
            }
        }
    }

    bool EditorInterface::_moveExistingObject( const int32_t tileIndex, const Maps::ObjectGroup groupType, int32_t objectIndex )
    {
        assert( tileIndex >= 0 && static_cast<size_t>( tileIndex ) < _mapFormat.tiles.size() );

        if ( groupType == Maps::ObjectGroup::KINGDOM_TOWNS ) {
            int32_t type = -1;
            int32_t color = -1;
            _editorPanel.getTownObjectProperties( type, color );
            if ( type < 0 || color < 0 ) {
                // Check your logic!
                assert( 0 );
                return false;
            }

            objectIndex = type;
        }
        else if ( groupType == Maps::ObjectGroup::ADVENTURE_MINES ) {
            int32_t type = -1;
            int32_t color = -1;

            _editorPanel.getMineObjectProperties( type, color );
            if ( type < 0 || color < 0 ) {
                // Check your logic!
                assert( 0 );
                return false;
            }

            objectIndex = type;
        }

        for ( const auto & object : _mapFormat.tiles[tileIndex].objects ) {
            if ( object.group == groupType && object.index == static_cast<uint32_t>( objectIndex ) ) {
                if ( object.id == Maps::getLastObjectUID() ) {
                    // Just do nothing since this is the last object.
                    return true;
                }

                const uint32_t oldObjectUID = object.id;

                fheroes2::ActionCreator action( _historyManager, _mapFormat );

                const uint32_t newObjectUID = Maps::getNewObjectUID();
                _updateObjectMetadata( object, newObjectUID );
                _updateObjectUID( oldObjectUID, newObjectUID );

                action.commit();

                // TODO: so far this is the only way to update objects for rendering.
                return Maps::readMapInEditor( _mapFormat );
            }
        }

        return false;
    }

    void EditorInterface::_updateObjectMetadata( const Maps::Map_Format::TileObjectInfo & object, const uint32_t newObjectUID )
    {
        const auto & objectGroupInfo = Maps::getObjectsByGroup( object.group );
        assert( object.index <= objectGroupInfo.size() );

        const auto & objectInfo = objectGroupInfo[object.index];
        assert( !objectInfo.groundLevelParts.empty() );

        const MP2::MapObjectType objectType = objectInfo.groundLevelParts.front().objectType;

        const bool isActionObject = MP2::isOffGameActionObject( objectType );
        if ( !isActionObject ) {
            // Only action objects have metadata.
            return;
        }

        size_t objectsReplaced = 0;

        // This logic is based on an assumption that only one action object can exist on one tile.
        if ( replaceKey( _mapFormat.standardMetadata, object.id, newObjectUID ) ) {
            ++objectsReplaced;
        }

        if ( replaceKey( _mapFormat.castleMetadata, object.id, newObjectUID ) ) {
            ++objectsReplaced;
        }

        if ( replaceKey( _mapFormat.heroMetadata, object.id, newObjectUID ) ) {
            ++objectsReplaced;
        }

        if ( replaceKey( _mapFormat.sphinxMetadata, object.id, newObjectUID ) ) {
            ++objectsReplaced;
        }

        if ( replaceKey( _mapFormat.signMetadata, object.id, newObjectUID ) ) {
            ++objectsReplaced;
        }

        if ( replaceKey( _mapFormat.adventureMapEventMetadata, object.id, newObjectUID ) ) {
            ++objectsReplaced;
        }

        if ( replaceKey( _mapFormat.shrineMetadata, object.id, newObjectUID ) ) {
            ++objectsReplaced;
        }

        assert( objectsReplaced == 0 || objectsReplaced == 1 );
    }

    void EditorInterface::_updateObjectUID( const uint32_t oldObjectUID, const uint32_t newObjectUID )
    {
        for ( auto & tile : _mapFormat.tiles ) {
            for ( size_t i = 0; i < tile.objects.size(); ) {
                if ( tile.objects[i].id == oldObjectUID ) {
                    tile.objects[i].id = newObjectUID;

                    if ( i != tile.objects.size() - 1 ) {
                        // Put the object on top of others.
                        std::swap( tile.objects[i], tile.objects[tile.objects.size() - 1] );
                        continue;
                    }
                }

                ++i;
            }
        }
    }
}
