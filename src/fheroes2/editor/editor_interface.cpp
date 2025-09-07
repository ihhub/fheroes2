/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2025                                             *
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
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "artifact.h"
#include "audio_manager.h"
#include "castle.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "direction.h"
#include "editor_castle_details_window.h"
#include "editor_event_details_window.h"
#include "editor_map_specs_window.h"
#include "editor_object_popup_window.h"
#include "editor_save_map_window.h"
#include "editor_secondary_skill_selection.h"
#include "editor_spell_selection.h"
#include "editor_sphinx_window.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_static.h"
#include "ground.h"
#include "heroes.h"
#include "history_manager.h"
#include "icn.h"
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
#include "players.h"
#include "puzzle.h"
#include "race.h"
#include "render_processor.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_map_object.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"
#include "view_world.h"
#include "world.h"
#include "world_object_uid.h"

namespace fheroes2
{
    class Image;
}

namespace
{
    const uint32_t mapUpdateFlags = Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR;

    // In original Editor map name is limited to 17 characters.
    // However, we have no such limitation but to be reasonable we still have a limit.
    const int32_t maxMapNameLength = 50;

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

    class MonsterMultiSelection final : public fheroes2::DialogElement
    {
    public:
        MonsterMultiSelection( std::vector<int> allowed, std::vector<int> selected, std::string text, const bool isEvilInterface )
            : _allowed( std::move( allowed ) )
            , _selected( std::move( selected ) )
            , _text( std::move( text ), fheroes2::FontType::normalWhite() )
            , _buttonSelection( 0, 0, ( isEvilInterface ? ICN::BUTTON_SELECT_EVIL : ICN::BUTTON_SELECT_GOOD ), 0, 1 )
        {
            const int32_t offset{ 5 };

            const fheroes2::Size textArea{ _text.width(), _text.height() };
            const fheroes2::Size buttonArea{ _buttonSelection.area().width, _buttonSelection.area().height };

            const int32_t maxWidth = std::max( textArea.width, buttonArea.width );
            _textArea = { ( maxWidth - textArea.width ) / 2, 0, textArea.width, textArea.height };
            _buttonArea = { ( maxWidth - buttonArea.width ) / 2, textArea.height + offset, buttonArea.width, buttonArea.height };

            _area = { maxWidth, textArea.height };
            _area.height += offset;
            _area.height += buttonArea.height;
        }

        ~MonsterMultiSelection() override = default;

        void draw( fheroes2::Image & output, const fheroes2::Point & offset ) const override
        {
            _text.draw( offset.x + _textArea.x, offset.y + _textArea.y, output );

            _buttonSelection.setPosition( offset.x + _buttonArea.x, offset.y + _buttonArea.y );
            _buttonSelection.draw( output );
        }

        void processEvents( const fheroes2::Point & offset ) const override
        {
            LocalEvent & le = LocalEvent::Get();
            const fheroes2::Rect buttonRect{ offset.x + _buttonArea.x, offset.y + _buttonArea.y, _buttonArea.width, _buttonArea.height };

            if ( le.isMouseRightButtonPressedInArea( buttonRect ) ) {
                fheroes2::showStandardTextMessage( _( "SELECT" ), _( "Click to make selection." ), 0 );
            }
            else if ( le.MouseClickLeft( buttonRect ) ) {
                Dialog::multiSelectMonsters( _allowed, _selected );
            }
        }

        void showPopup( const int /*buttons*/ ) const override
        {
            // Do nothing.
        }

        bool update( fheroes2::Image & /*output*/, const fheroes2::Point & offset ) const override
        {
            _buttonSelection.setPosition( offset.x + _buttonArea.x, offset.y + _buttonArea.y );

            const LocalEvent & le = LocalEvent::Get();
            return _buttonSelection.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( _buttonSelection.area() ) );
        }

        std::vector<int> getSelected() const
        {
            return _selected;
        }

    private:
        std::vector<int> _allowed;
        mutable std::vector<int> _selected;
        fheroes2::Rect _textArea;
        fheroes2::Rect _buttonArea;

        fheroes2::Text _text;
        mutable fheroes2::Button _buttonSelection;
    };

    size_t getObeliskCount( const Maps::Map_Format::MapFormat & _mapFormat )
    {
        const auto & miscellaneousObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS );

        std::set<size_t> obeliskIndex;
        for ( size_t i = 0; i < miscellaneousObjects.size(); ++i ) {
            if ( miscellaneousObjects[i].objectType == MP2::OBJ_OBELISK ) {
                obeliskIndex.emplace( i );
            }
        }

        size_t obeliskCount = 0;
        for ( const auto & mapTile : _mapFormat.tiles ) {
            for ( const auto & object : mapTile.objects ) {
                if ( object.group == Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS && obeliskIndex.count( object.index ) > 0 ) {
                    assert( object.index < miscellaneousObjects.size() );

                    ++obeliskCount;
                }
            }
        }

        return obeliskCount;
    }

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

            const auto & tile = world.getTile( pos.x, pos.y );

            if ( MP2::isOffGameActionObject( tile.getMainObjectType() ) ) {
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
                           const std::function<bool( const Maps::Tile & tile )> & condition )
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

            if ( !condition( world.getTile( temp.x, temp.y ) ) ) {
                return false;
            }
        }

        return true;
    }

    bool checkConditionForUsedTiles( const Maps::ObjectInfo & info, const fheroes2::Point & mainTilePos,
                                     const std::function<bool( const Maps::Tile & tile )> & condition )
    {
        return isConditionValid( Maps::getGroundLevelUsedTileOffset( info ), mainTilePos, condition );
    }

    bool removeObjects( Maps::Map_Format::MapFormat & mapFormat, std::set<uint32_t> objectsUids, const std::set<Maps::ObjectGroup> & objectGroups )
    {
        if ( objectsUids.empty() || objectGroups.empty() ) {
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

                // Remove the object also from the `world` tiles. It is needed for proper rendering of the map.
                Maps::removeObjectFromMapByUID( static_cast<int32_t>( mapTileIndex ), objectIter->id );

                const auto & objects = Maps::getObjectsByGroup( objectIter->group );
                assert( objectIter->index < objects.size() );
                const auto objectType = objects[objectIter->index].objectType;

                // Remove ownership data for capturable objects.
                if ( Maps::isCapturableObject( objectType ) ) {
                    mapFormat.capturableObjectsMetadata.erase( static_cast<int32_t>( objectIter->id ) );
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
                    const size_t bottomTileIndex = mapTileIndex + mapFormat.width;
                    assert( bottomTileIndex < mapFormat.tiles.size() );

                    if ( Maps::doesContainRoads( mapFormat.tiles[bottomTileIndex] ) ) {
                        Maps::updateRoadOnTile( mapFormat, static_cast<int32_t>( bottomTileIndex ), false );
                    }

                    // The castle entrance is marked as road. Update this tile to remove the mark.
                    Maps::updateRoadSpriteOnTile( mapFormat, static_cast<int32_t>( mapTileIndex ), false );

                    // Remove the castle from `world` castles vector.
                    world.removeCastle( Maps::GetPoint( static_cast<int32_t>( mapTileIndex ) ) );

                    needRedraw = true;
                    updateMapPlayerInformation = true;
                }
                else if ( objectIter->group == Maps::ObjectGroup::ROADS ) {
                    assert( mapTileIndex < world.getSize() );

                    if ( Maps::doesContainRoads( mapTile ) ) {
                        needRedraw |= Maps::updateRoadOnTile( mapFormat, static_cast<int32_t>( mapTileIndex ), false );

                        // Current 'objectIter'is deleted. Update it to the begin.
                        objectIter = mapTile.objects.begin();
                    }
                    else {
                        ++objectIter;
                    }
                }
                else if ( objectIter->group == Maps::ObjectGroup::STREAMS ) {
                    objectIter = mapTile.objects.erase( objectIter );
                    needRedraw = true;

                    Maps::updateStreamsAround( mapFormat, static_cast<int32_t>( mapTileIndex ) );
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
                    assert( mapFormat.monsterMetadata.find( objectIter->id ) != mapFormat.monsterMetadata.end() );
                    mapFormat.monsterMetadata.erase( objectIter->id );

                    objectIter = mapTile.objects.erase( objectIter );
                    needRedraw = true;
                }
                else if ( objectIter->group == Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS ) {
                    switch ( objectType ) {
                    case MP2::OBJ_EVENT:
                        assert( mapFormat.adventureMapEventMetadata.find( objectIter->id ) != mapFormat.adventureMapEventMetadata.end() );
                        mapFormat.adventureMapEventMetadata.erase( objectIter->id );
                        break;
                    case MP2::OBJ_PYRAMID:
                        mapFormat.selectionObjectMetadata.erase( objectIter->id );
                        break;
                    case MP2::OBJ_SIGN:
                        assert( mapFormat.signMetadata.find( objectIter->id ) != mapFormat.signMetadata.end() );
                        mapFormat.signMetadata.erase( objectIter->id );
                        break;
                    case MP2::OBJ_SPHINX:
                        assert( mapFormat.sphinxMetadata.find( objectIter->id ) != mapFormat.sphinxMetadata.end() );
                        mapFormat.sphinxMetadata.erase( objectIter->id );
                        break;
                    case MP2::OBJ_WITCHS_HUT:
                        mapFormat.selectionObjectMetadata.erase( objectIter->id );
                        break;
                    default:
                        break;
                    }

                    objectIter = mapTile.objects.erase( objectIter );
                    needRedraw = true;
                }
                else if ( objectIter->group == Maps::ObjectGroup::ADVENTURE_WATER ) {
                    if ( objectType == MP2::OBJ_BOTTLE ) {
                        assert( mapFormat.signMetadata.find( objectIter->id ) != mapFormat.signMetadata.end() );
                        mapFormat.signMetadata.erase( objectIter->id );
                    }

                    objectIter = mapTile.objects.erase( objectIter );
                    needRedraw = true;
                }
                else if ( objectIter->group == Maps::ObjectGroup::ADVENTURE_ARTIFACTS ) {
                    assert( mapFormat.artifactMetadata.find( objectIter->id ) != mapFormat.artifactMetadata.end() );
                    mapFormat.artifactMetadata.erase( objectIter->id );

                    objectIter = mapTile.objects.erase( objectIter );
                    needRedraw = true;
                }
                else if ( objectIter->group == Maps::ObjectGroup::ADVENTURE_TREASURES ) {
                    if ( objectType == MP2::OBJ_RESOURCE ) {
                        mapFormat.resourceMetadata.erase( objectIter->id );
                    }

                    objectIter = mapTile.objects.erase( objectIter );
                    needRedraw = true;
                }
                else if ( objectIter->group == Maps::ObjectGroup::LANDSCAPE_MISCELLANEOUS ) {
                    // We need to check if the object being removed is a River Delta, and if so, use that data to update the nearby Stream correctly.
                    const int riverDeltaDirection = Maps::getRiverDeltaDirectionByIndex( objectIter->group, static_cast<int32_t>( objectIter->index ) );

                    objectIter = mapTile.objects.erase( objectIter );

                    if ( riverDeltaDirection != Direction::UNKNOWN ) {
                        // For River Deltas we update the nearby Streams to properly disconnect from them.
                        Maps::updateStreamsToDeltaConnection( mapFormat, static_cast<int32_t>( mapTileIndex ), riverDeltaDirection );
                    }

                    needRedraw = true;
                }
                else if ( objectIter->group == Maps::ObjectGroup::ADVENTURE_POWER_UPS ) {
                    switch ( objectType ) {
                    case MP2::OBJ_SHRINE_FIRST_CIRCLE:
                    case MP2::OBJ_SHRINE_SECOND_CIRCLE:
                    case MP2::OBJ_SHRINE_THIRD_CIRCLE:
                        // We cannot assert non-existing metadata as these objects could have been created by an older Editor version.
                        mapFormat.selectionObjectMetadata.erase( objectIter->id );
                        break;
                    default:
                        break;
                    }

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

            if ( objectsUids.empty() ) {
                break;
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

            if ( !checkConditionForUsedTiles( objectInfo, tilePos, []( const Maps::Tile & tileToCheck ) { return !tileToCheck.isWater(); } ) ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            if ( objectInfo.objectType == MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT && !checkConditionForUsedTiles( objectInfo, tilePos, []( const Maps::Tile & tileToCheck ) {
                     // This check is being run for every action in the Editor.
                     // Therefore, after the placement of an Ultimate Artifact on this tile the check would fail since the tile contains something on it.
                     // So, before checking for correctness on placement an Ultimate Artifact we check whether the current tile already has one.
                     // If this is true then we consider that the placement is allowed.
                     if ( tileToCheck.getMainObjectType() == MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT ) {
                         return true;
                     }

                     return tileToCheck.isSuitableForUltimateArtifact();
                 } ) ) {
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
        case Maps::ObjectGroup::MONSTERS:
        case Maps::ObjectGroup::STREAMS: {
            const auto & objectInfo = Maps::getObjectInfo( groupType, objectType );
            if ( !checkConditionForUsedTiles( objectInfo, tilePos, []( const Maps::Tile & tileToCheck ) { return !tileToCheck.isWater(); } ) ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            break;
        }
        case Maps::ObjectGroup::ADVENTURE_WATER:
        case Maps::ObjectGroup::LANDSCAPE_WATER: {
            const auto & objectInfo = Maps::getObjectInfo( groupType, objectType );
            if ( !checkConditionForUsedTiles( objectInfo, tilePos, []( const Maps::Tile & tileToCheck ) { return tileToCheck.isWater(); } ) ) {
                errorMessage = _( "%{objects} must be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            break;
        }
        case Maps::ObjectGroup::LANDSCAPE_MISCELLANEOUS: {
            // River deltas are only objects that can be placed on water and on land.
            if ( Maps::isRiverDeltaObject( groupType, objectType ) ) {
                // This is a river delta. Just don't check the terrain type.
            }
            else if ( !checkConditionForUsedTiles( Maps::getObjectInfo( groupType, objectType ), tilePos,
                                                   []( const Maps::Tile & tileToCheck ) { return !tileToCheck.isWater(); } ) ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            break;
        }
        case Maps::ObjectGroup::KINGDOM_TOWNS: {
            const Maps::Tile & tile = world.getTile( tilePos.x, tilePos.y );

            if ( tile.isWater() ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            const int groundType = Maps::Ground::getGroundByImageIndex( tile.getTerrainImageIndex() );
            const int32_t basementId = fheroes2::getTownBasementId( groundType );

            const auto & townObjectInfo = Maps::getObjectInfo( groupType, objectType );
            const auto & basementObjectInfo = Maps::getObjectInfo( Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId );

            if ( !checkConditionForUsedTiles( townObjectInfo, tilePos, []( const Maps::Tile & tileToCheck ) { return !tileToCheck.isWater(); } ) ) {
                errorMessage = _( "%{objects} cannot be placed on water." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                return false;
            }

            if ( !checkConditionForUsedTiles( basementObjectInfo, tilePos, []( const Maps::Tile & tileToCheck ) { return !tileToCheck.isWater(); } ) ) {
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
            else if ( !checkConditionForUsedTiles( objectInfo, tilePos, []( const Maps::Tile & tileToCheck ) { return !tileToCheck.isWater(); } ) ) {
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
            // TODO: Allow castles with custom names to exceed the default castle names limit.
            // To do this we'll need also to check that the custom name is not present in default names.
            if ( world.getCastleCount() >= AllCastles::getMaximumAllowedCastles() ) {
                errorMessage = _( "A maximum of %{count} %{objects} can be placed on the map." );
                StringReplace( errorMessage, "%{objects}", Interface::EditorPanel::getObjectGroupName( groupType ) );
                StringReplace( errorMessage, "%{count}", AllCastles::getMaximumAllowedCastles() );
                return false;
            }

            const Maps::Tile & tile = world.getTile( tilePos.x, tilePos.y );

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

            if ( !isActionObjectAllowed( townObjectInfo, tilePos ) || !isActionObjectAllowed( basementObjectInfo, tilePos ) ) {
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

    std::vector<int> getAllowedMonsters( const Monster & monster, const MP2::MapObjectType objectType )
    {
        std::vector<int> allowedMonsters;

        switch ( objectType ) {
        case MP2::OBJ_RANDOM_MONSTER_MEDIUM:
        case MP2::OBJ_RANDOM_MONSTER_STRONG:
        case MP2::OBJ_RANDOM_MONSTER_VERY_STRONG:
        case MP2::OBJ_RANDOM_MONSTER_WEAK: {
            assert( monster.isRandomMonster() );
            const auto level = monster.GetRandomUnitLevel();

            for ( int monsterId = Monster::UNKNOWN + 1; monsterId < Monster::MONSTER_COUNT; ++monsterId ) {
                const Monster temp{ monsterId };
                if ( temp.isValid() && temp.GetRandomUnitLevel() == level ) {
                    allowedMonsters.emplace_back( monsterId );
                }
            }
            break;
        }
        case MP2::OBJ_RANDOM_MONSTER: {
            for ( int monsterId = Monster::UNKNOWN + 1; monsterId < Monster::MONSTER_COUNT; ++monsterId ) {
                if ( Monster{ monsterId }.isValid() ) {
                    allowedMonsters.emplace_back( monsterId );
                }
            }
            break;
        }
        default:
            break;
        }

        return allowedMonsters;
    }
}

namespace Interface
{
    void EditorInterface::reset()
    {
        const HideInterfaceModeDisabler hideInterfaceModeDisabler;

        const fheroes2::Display & display = fheroes2::Display::instance();

        const int32_t xOffset = display.width() - fheroes2::borderWidthPx - fheroes2::radarWidthPx;
        _radar.SetPos( xOffset, fheroes2::borderWidthPx );

        _editorPanel.setPos( xOffset, _radar.GetArea().y + _radar.GetArea().height
                                          + ( ( display.height() > fheroes2::Display::DEFAULT_HEIGHT + fheroes2::borderWidthPx ) ? fheroes2::borderWidthPx : 0 ) );

        const fheroes2::Point prevCenter = _gameArea.getCurrentCenterInPixels();
        const fheroes2::Rect prevRoi = _gameArea.GetROI();

        _gameArea.SetAreaPosition( fheroes2::borderWidthPx, fheroes2::borderWidthPx, display.width() - fheroes2::radarWidthPx - 3 * fheroes2::borderWidthPx,
                                   display.height() - 2 * fheroes2::borderWidthPx );

        const fheroes2::Rect newRoi = _gameArea.GetROI();

        if ( prevRoi == fheroes2::Rect{} ) {
            // This is the first initialization of the game area for the Editor.
            // Make the top-left corner of the first tile to be at the top-left corner of the shown game area.
            _gameArea.SetCenterInPixels( { newRoi.width / 2, newRoi.height / 2 } );

            return;
        }

        _gameArea.SetCenterInPixels( prevCenter + fheroes2::Point( newRoi.x + newRoi.width / 2, newRoi.y + newRoi.height / 2 )
                                     - fheroes2::Point( prevRoi.x + prevRoi.width / 2, prevRoi.y + prevRoi.height / 2 ) );
    }

    void EditorInterface::redraw( const uint32_t force )
    {
        const uint32_t combinedRedraw = _redraw | force;

        if ( combinedRedraw & REDRAW_GAMEAREA ) {
            int renderFlags = LEVEL_OBJECTS | LEVEL_HEROES | LEVEL_ROUTES;
            if ( combinedRedraw & REDRAW_PASSABILITIES ) {
                renderFlags |= LEVEL_PASSABILITIES;
            }

            fheroes2::Display & display = fheroes2::Display::instance();

            // Render all except the fog.
            _gameArea.Redraw( display, renderFlags );

            if ( _warningMessage.isValid() ) {
                const fheroes2::Rect & roi = _gameArea.GetROI();

                fheroes2::Text text{ _warningMessage.message(), fheroes2::FontType::normalWhite() };
                // Keep 4 pixels from each edge.
                text.fitToOneRow( roi.width - 8 );

                const bool isSystemInfoShown = Settings::Get().isSystemInfoEnabled();

                text.draw( roi.x + 4, roi.y + roi.height - text.height() - ( isSystemInfoShown ? 14 : 4 ), display );
            }

            // TODO:: Render horizontal and vertical map tiles scale and highlight with yellow text cursor position.

            if ( _editorPanel.showAreaSelectRect() && ( _tileUnderCursor > -1 ) ) {
                const fheroes2::Rect brushSize = _editorPanel.getBrushArea();

                if ( brushSize.width > 0 && brushSize.height > 0 ) {
                    const fheroes2::Point indices = getBrushAreaIndicies( brushSize, _tileUnderCursor );

                    assert( Maps::isValidAbsIndex( indices.x ) );
                    const bool isActionObject = ( _editorPanel.isDetailEdit() && brushSize.width == 1 && brushSize.height == 1
                                                  && MP2::isOffGameActionObject( world.getTile( indices.x ).getMainObjectType() ) );

                    _gameArea.renderTileAreaSelect( display, indices.x, indices.y, isActionObject );
                }
                else if ( _editorPanel.isTerrainEdit() || _editorPanel.isEraseMode() ) {
                    assert( brushSize == fheroes2::Rect() );
                    // Render area selection from the tile where the left mouse button was pressed till the tile under the cursor.
                    _gameArea.renderTileAreaSelect( display, _areaSelectionStartTileId, _tileUnderCursor, false );
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

    fheroes2::GameMode EditorInterface::startEdit()
    {
        // The Editor has a special option to disable animation. This affects cycling animation as well.
        // First, we disable it to make sure to enable it back while exiting this function.
        const fheroes2::ScreenPaletteRestorer restorer;

        Settings & conf = Settings::Get();

        if ( conf.isEditorAnimationEnabled() ) {
            fheroes2::RenderProcessor::instance().startColorCycling();
        }

        reset();

        _historyManager.reset();

        // Stop all sounds and music.
        AudioManager::ResetAudio();

        _radar.Build();
        _radar.SetHide( false );

        fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

        _gameArea.SetUpdateCursor();

        // The cursor parameters may contain values from a previously edited map that are not suitable for this one. Reset them.
        _tileUnderCursor = -1;
        _areaSelectionStartTileId = -1;

        uint32_t redrawFlags = REDRAW_GAMEAREA | REDRAW_RADAR | REDRAW_PANEL | REDRAW_STATUS | REDRAW_BORDER;
        if ( conf.isEditorPassabilityEnabled() ) {
            redrawFlags |= REDRAW_PASSABILITIES;
        }

        setRedraw( redrawFlags );

        int32_t fastScrollRepeatCount = 0;
        const int32_t fastScrollStartThreshold = 2;

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

            // Hotkeys
            if ( le.isAnyKeyPressed() ) {
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
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_LEFT ) ) {
                    if ( !_gameArea.isDragScroll() ) {
                        _gameArea.SetScroll( SCROLL_LEFT );
                    }
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_RIGHT ) ) {
                    if ( !_gameArea.isDragScroll() ) {
                        _gameArea.SetScroll( SCROLL_RIGHT );
                    }
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_UP ) ) {
                    if ( !_gameArea.isDragScroll() ) {
                        _gameArea.SetScroll( SCROLL_TOP );
                    }
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_DOWN ) ) {
                    if ( !_gameArea.isDragScroll() ) {
                        _gameArea.SetScroll( SCROLL_BOTTOM );
                    }
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
                        res = fheroes2::GameMode::MAIN_MENU;
                    }
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::EDITOR_TOGGLE_PASSABILITY ) ) {
                    conf.setEditorPassability( !conf.isEditorPassabilityEnabled() );
                    // This is not an ideal solution as we should save the whole configuration while tweaking one option.
                    // However, we can improve it later if it leads to slowdowns while using the Editor.
                    conf.Save( Settings::configFileName );

                    setRedraw( REDRAW_GAMEAREA );
                }
            }

            if ( res != fheroes2::GameMode::CANCEL ) {
                break;
            }

            bool isCursorOverGameArea = false;

            // Mouse is captured by radar
            if ( _radar.isMouseCaptured() ) {
                cursor.SetThemes( Cursor::POINTER );

                _radar.QueueEventProcessing();
            }
            // Mouse is captured by the game area for scrolling by dragging
            else if ( _gameArea.isDragScroll() ) {
                _gameArea.QueueEventProcessing();
            }
            else {
                if ( fheroes2::cursor().isFocusActive() && conf.ScrollSpeed() != SCROLL_SPEED_NONE ) {
                    int scrollDirection = SCROLL_NONE;

                    if ( isScrollLeft( le.getMouseCursorPos() ) ) {
                        scrollDirection |= SCROLL_LEFT;
                    }
                    else if ( isScrollRight( le.getMouseCursorPos() ) ) {
                        scrollDirection |= SCROLL_RIGHT;
                    }
                    if ( isScrollTop( le.getMouseCursorPos() ) ) {
                        scrollDirection |= SCROLL_TOP;
                    }
                    else if ( isScrollBottom( le.getMouseCursorPos() ) ) {
                        scrollDirection |= SCROLL_BOTTOM;
                    }

                    if ( scrollDirection != SCROLL_NONE && _gameArea.isFastScrollEnabled() ) {
                        if ( Game::validateAnimationDelay( Game::SCROLL_START_DELAY ) && fastScrollRepeatCount < fastScrollStartThreshold ) {
                            ++fastScrollRepeatCount;
                        }

                        if ( fastScrollRepeatCount >= fastScrollStartThreshold ) {
                            _gameArea.SetScroll( scrollDirection );
                        }
                    }
                    else {
                        fastScrollRepeatCount = 0;
                    }
                }
                else {
                    fastScrollRepeatCount = 0;
                }

                // Re-enable fast scrolling if the cursor movement indicates the need
                if ( !_gameArea.isFastScrollEnabled() && _gameArea.mouseIndicatesFastScroll( le.getMouseCursorPos() ) ) {
                    _gameArea.setFastScrollStatus( true );
                }

                // Cursor is over the radar
                if ( le.isMouseCursorPosInArea( _radar.GetRect() ) ) {
                    cursor.SetThemes( Cursor::POINTER );

                    // TODO: Add checks for object placing/moving, and other Editor functions that uses mouse dragging.
                    if ( _editorPanel.getBrushArea().width > 0 || _areaSelectionStartTileId == -1 ) {
                        _radar.QueueEventProcessing();
                    }
                }
                // Cursor is over the editor panel
                else if ( le.isMouseCursorPosInArea( _editorPanel.getRect() ) ) {
                    // At lower resolutions, the Editor panel has no border at the bottom. If the mouse cursor is over
                    // this bottom section, then the game area may scroll. In this case, the mouse cursor shouldn't be
                    // changed, but the editor panel should still handle events.
                    if ( !_gameArea.NeedScroll() ) {
                        cursor.SetThemes( Cursor::POINTER );
                    }

                    res = _editorPanel.queueEventProcessing();
                }
                else if ( !_gameArea.NeedScroll() ) {
                    // Cursor is over the game area
                    if ( le.isMouseCursorPosInArea( _gameArea.GetROI() ) ) {
                        _gameArea.QueueEventProcessing();

                        isCursorOverGameArea = true;
                    }
                    // Cursor is somewhere else
                    else {
                        cursor.SetThemes( Cursor::POINTER );
                    }
                }
            }

            if ( res != fheroes2::GameMode::CANCEL ) {
                break;
            }

            if ( isCursorOverGameArea ) {
                // Get relative tile position under the cursor. This position can be outside the map size.
                const fheroes2::Point posInGameArea = _gameArea.getInternalPosition( le.getMouseCursorPos() );
                const fheroes2::Point tilePos{ posInGameArea.x / fheroes2::tileWidthPx, posInGameArea.y / fheroes2::tileWidthPx };
                const bool isValidTile = ( tilePos.x >= 0 && tilePos.y >= 0 && tilePos.x < world.w() && tilePos.y < world.h() );
                const bool isBrushEmpty = ( _editorPanel.getBrushArea() == fheroes2::Rect() );

                if ( isValidTile ) {
                    const int32_t tileIndex = tilePos.y * world.w() + tilePos.x;
                    if ( _tileUnderCursor != tileIndex ) {
                        _tileUnderCursor = tileIndex;

                        // Force redraw if cursor position was changed as area rectangle is also changed.
                        if ( _editorPanel.showAreaSelectRect() && ( !isBrushEmpty || _areaSelectionStartTileId != -1 ) ) {
                            _redraw |= REDRAW_GAMEAREA;
                        }
                    }
                }
                else if ( _areaSelectionStartTileId != -1 ) {
                    assert( _editorPanel.showAreaSelectRect() && isBrushEmpty );

                    const fheroes2::Point clampedPoint{ std::clamp( tilePos.x, 0, world.w() - 1 ), std::clamp( tilePos.y, 0, world.h() - 1 ) };
                    const int32_t tileIndex = clampedPoint.y * world.w() + clampedPoint.x;
                    if ( _tileUnderCursor != tileIndex ) {
                        _tileUnderCursor = tileIndex;

                        // Force redraw if cursor position was changed as area rectangle is also changed.
                        _redraw |= REDRAW_GAMEAREA;
                    }
                }

                if ( _areaSelectionStartTileId == -1 && isValidTile && isBrushEmpty && le.isMouseLeftButtonPressed() ) {
                    _areaSelectionStartTileId = tilePos.y * world.w() + tilePos.x;
                    _redraw |= REDRAW_GAMEAREA;
                }
            }
            else if ( _tileUnderCursor != -1 ) {
                _tileUnderCursor = -1;
                _redraw |= REDRAW_GAMEAREA;
            }

            if ( _areaSelectionStartTileId > -1 && le.isMouseLeftButtonReleased() ) {
                if ( isCursorOverGameArea && _tileUnderCursor > -1 && _editorPanel.getBrushArea().width == 0 ) {
                    if ( _editorPanel.isTerrainEdit() ) {
                        // Fill the selected area in terrain edit mode.
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );

                        const int groundId = _editorPanel.selectedGroundType();
                        Maps::setTerrainOnTiles( _mapFormat, _areaSelectionStartTileId, _tileUnderCursor, groundId );
                        _validateObjectsOnTerrainUpdate();

                        action.commit();

                        _redraw |= mapUpdateFlags;
                    }
                    else if ( _editorPanel.isEraseMode() ) {
                        // Erase objects in the selected area.
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );

                        if ( removeObjects( _mapFormat, Maps::getObjectUidsInArea( _areaSelectionStartTileId, _tileUnderCursor ),
                                            _editorPanel.getEraseObjectGroups() ) ) {
                            action.commit();
                            _redraw |= mapUpdateFlags;
                        }
                    }
                }

                // Reset the area start tile.
                _areaSelectionStartTileId = -1;

                _redraw |= mapUpdateFlags;
            }

            // Scrolling the game area
            if ( _gameArea.NeedScroll() && Game::validateAnimationDelay( Game::SCROLL_DELAY ) ) {
                assert( !_gameArea.isDragScroll() );

                if ( isScrollLeft( le.getMouseCursorPos() ) || isScrollRight( le.getMouseCursorPos() ) || isScrollTop( le.getMouseCursorPos() )
                     || isScrollBottom( le.getMouseCursorPos() ) ) {
                    cursor.SetThemes( _gameArea.GetScrollCursor() );
                }

                _gameArea.Scroll();

                setRedraw( REDRAW_GAMEAREA | REDRAW_RADAR_CURSOR );
            }
            else if ( _gameArea.needDragScrollRedraw() ) {
                setRedraw( REDRAW_GAMEAREA | REDRAW_RADAR_CURSOR );
            }

            assert( res == fheroes2::GameMode::CANCEL );

            // Map objects animation
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

        // When exiting the editor we must reset the players data to properly load the new maps.
        conf.GetPlayers().clear();
        // And reset the players configuration for the selected map to properly initialize it when starting a new map.
        Game::SavePlayers( "", {} );

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
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();
        const Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();
        const int bigButtonsICN = isEvilInterface ? ICN::BUTTONS_EDITOR_FILE_DIALOG_EVIL : ICN::BUTTONS_EDITOR_FILE_DIALOG_GOOD;
        fheroes2::ButtonGroup optionButtons( bigButtonsICN );
        fheroes2::StandardWindow background( optionButtons, false, 0, display );
        background.renderSymmetricButtons( optionButtons, 0, false );

        const fheroes2::ButtonBase & buttonNewMap = optionButtons.button( 0 );
        const fheroes2::ButtonBase & buttonLoadMap = optionButtons.button( 1 );
        const fheroes2::ButtonBase & buttonStartMap = optionButtons.button( 2 );
        const fheroes2::ButtonBase & buttonSaveMap = optionButtons.button( 3 );
        const fheroes2::ButtonBase & buttonMainMenu = optionButtons.button( 4 );
        const fheroes2::ButtonBase & buttonQuit = optionButtons.button( 5 );

        fheroes2::Button buttonCancel;

        background.renderButton( buttonCancel, isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1, { 0, 11 },
                                 fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

        display.render( background.totalArea() );

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            optionButtons.drawOnState( le );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );

            if ( le.MouseClickLeft( buttonNewMap.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_NEW_MAP_MENU ) ) {
                if ( eventNewMap() == fheroes2::GameMode::EDITOR_NEW_MAP ) {
                    return fheroes2::GameMode::EDITOR_NEW_MAP;
                }
            }
            if ( le.MouseClickLeft( buttonLoadMap.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME ) ) {
                if ( eventLoadMap() == fheroes2::GameMode::EDITOR_LOAD_MAP ) {
                    return fheroes2::GameMode::EDITOR_LOAD_MAP;
                }
            }
            if ( le.MouseClickLeft( buttonSaveMap.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_SAVE_GAME ) ) {
                // Special case: since we show a window about file saving we don't want to display the current dialog anymore.
                background.hideWindow();
                display.render( background.totalArea() );
                Get().saveMapToFile();
                return fheroes2::GameMode::CANCEL;
            }

            if ( le.MouseClickLeft( buttonQuit.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_QUIT ) ) {
                if ( EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                    return fheroes2::GameMode::QUIT_GAME;
                }
            }
            if ( le.MouseClickLeft( buttonMainMenu.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_TO_GAME_MAIN_MENU ) ) {
                if ( fheroes2::showStandardTextMessage( _( "Main Menu" ),
                                                        _( "Do you wish to return to the game's Main Menu? (Any unsaved changes to the current map will be lost.)" ),
                                                        Dialog::YES | Dialog::NO )
                     == Dialog::YES ) {
                    return fheroes2::GameMode::MAIN_MENU;
                }
            }
            else if ( le.MouseClickLeft( buttonStartMap.area() ) ) {
                bool isNameEmpty = conf.getCurrentMapInfo().name.empty();
                if ( isNameEmpty
                     && fheroes2::showStandardTextMessage(
                            _( "Unsaved Changes" ),
                            _( "This map has either terrain changes, undo history or has not yet been saved to a file.\n\nDo you wish to save the current map?" ),
                            Dialog::YES | Dialog::NO )
                            == Dialog::NO ) {
                    continue;
                }
                if ( isNameEmpty ) {
                    Get().saveMapToFile();
                    isNameEmpty = conf.getCurrentMapInfo().name.empty();
                    if ( isNameEmpty ) {
                        // Saving was aborted.
                        display.render( background.totalArea() );
                        continue;
                    }
                }
                if ( conf.getCurrentMapInfo().colorsAvailableForHumans == 0 ) {
                    fheroes2::showStandardTextMessage( _( "Unplayable Map" ),
                                                       _( "This map is not playable. You need at least one human player for the map to be playable." ), Dialog::OK );
                }
                else {
                    if ( fheroes2::
                             showStandardTextMessage( _( "Start Map" ),
                                                      _( "Do you wish to leave the Editor and start the map? (Any unsaved changes to the current map will be lost.)" ),
                                                      Dialog::YES | Dialog::NO )
                         == Dialog::YES ) {
                        return fheroes2::GameMode::NEW_STANDARD;
                    }
                }
            }
            else if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyCloseWindow() ) {
                return fheroes2::GameMode::CANCEL;
            }

            if ( le.isMouseRightButtonPressedInArea( buttonNewMap.area() ) ) {
                // TODO: update this text once random map generator is ready.
                //       The original text should be "Create a new map, either from scratch or using the random map generator."
                fheroes2::showStandardTextMessage( _( "New Map" ), _( "Create a new map from scratch." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonLoadMap.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Load Map" ), _( "Load an existing map." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonSaveMap.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Save Map" ), _( "Save the current map." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonQuit.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Quit" ), _( "Quit out of the map editor." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonMainMenu.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Main Menu" ), _( "Return to the game's Main Menu." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonStartMap.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Start Map" ), _( "Leave the Editor and play the map in the Standard Game mode." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
        }
        return fheroes2::GameMode::CANCEL;
    }

    void EditorInterface::eventViewWorld()
    {
        // TODO: Make proper borders restoration for low height resolutions, like for hide interface mode.
        ViewWorld::ViewWorldWindow( PlayerColor::NONE, ViewWorldMode::ViewAll, *this );
    }

    void EditorInterface::mouseCursorAreaClickLeft( const int32_t tileIndex )
    {
        assert( tileIndex >= 0 && tileIndex < static_cast<int32_t>( world.getSize() ) );

        Maps::Tile & tile = world.getTile( tileIndex );

        if ( _editorPanel.isDetailEdit() ) {
            // Trigger an action only when metadata has been changed to avoid expensive computations and bloated list of actions.
            // Comparing a metadata structure is much faster than restoring the whole map.

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

                    const PlayerColor color = ( objectType == MP2::OBJ_JAIL ) ? PlayerColor::NONE : static_cast<PlayerColor>( 1 << objectInfo.metadata[0] );

                    // Make a temporary hero to edit his details.
                    Heroes hero;
                    hero.SetColor( color );
                    hero.applyHeroMetadata( _mapFormat.heroMetadata[object.id], objectType == MP2::OBJ_JAIL, true );

                    hero.OpenDialog( false, false, true, true, true, true, _mapFormat.mainLanguage );
                    Maps::Map_Format::HeroMetadata heroNewMetadata = hero.getHeroMetadata();
                    if ( heroNewMetadata != _mapFormat.heroMetadata[object.id] ) {
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );
                        _mapFormat.heroMetadata[object.id] = std::move( heroNewMetadata );
                        action.commit();
                    }
                }
                else if ( objectType == MP2::OBJ_CASTLE || objectType == MP2::OBJ_RANDOM_TOWN || objectType == MP2::OBJ_RANDOM_CASTLE ) {
                    assert( _mapFormat.castleMetadata.find( object.id ) != _mapFormat.castleMetadata.end() );

                    const int race = Race::IndexToRace( static_cast<int>( objectInfo.metadata[0] ) );
                    const PlayerColor color = Color::IndexToColor( Maps::getTownColorIndex( _mapFormat, tileIndex, object.id ) );

                    auto & castleMetadata = _mapFormat.castleMetadata[object.id];
                    Maps::Map_Format::CastleMetadata newCastleMetadata = castleMetadata;

                    if ( Editor::castleDetailsDialog( newCastleMetadata, race, color, _mapFormat.mainLanguage ) && ( castleMetadata != newCastleMetadata ) ) {
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );
                        castleMetadata = std::move( newCastleMetadata );
                        action.commit();
                    }
                }
                else if ( objectType == MP2::OBJ_SIGN || objectType == MP2::OBJ_BOTTLE ) {
                    std::string header = _( "Input %{object} text" );
                    StringReplace( header, "%{object}", MP2::StringObject( objectType ) );

                    auto & originalMessage = _mapFormat.signMetadata[object.id].message;
                    std::string signText = originalMessage;

                    const fheroes2::Text body{ std::move( header ), fheroes2::FontType::normalWhite() };
                    if ( Dialog::inputString( fheroes2::Text{}, body, signText, 0, true, _mapFormat.mainLanguage ) && originalMessage != signText ) {
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );
                        originalMessage = std::move( signText );
                        action.commit();
                    }
                }
                else if ( objectType == MP2::OBJ_EVENT ) {
                    assert( _mapFormat.adventureMapEventMetadata.find( object.id ) != _mapFormat.adventureMapEventMetadata.end() );

                    auto & eventMetadata = _mapFormat.adventureMapEventMetadata[object.id];
                    Maps::Map_Format::AdventureMapEventMetadata newEventData = eventMetadata;

                    if ( Editor::eventDetailsDialog( newEventData, _mapFormat.humanPlayerColors, _mapFormat.computerPlayerColors, _mapFormat.mainLanguage )
                         && newEventData != eventMetadata ) {
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );
                        eventMetadata = std::move( newEventData );
                        action.commit();
                    }
                }
                else if ( object.group == Maps::ObjectGroup::MONSTERS ) {
                    assert( _mapFormat.monsterMetadata.find( object.id ) != _mapFormat.monsterMetadata.end() );

                    auto monsterMetadata = _mapFormat.monsterMetadata.find( object.id );
                    int32_t monsterCount = monsterMetadata->second.count;
                    const std::vector<int> selectedMonsters = monsterMetadata->second.selected;

                    const Monster tempMonster( static_cast<int>( object.index ) + 1 );

                    std::string str = _( "Set %{monster} Count" );
                    StringReplace( str, "%{monster}", tempMonster.GetName() );

                    std::unique_ptr<const fheroes2::MonsterDialogElement> monsterUi = nullptr;

                    if ( tempMonster.isValid() ) {
                        monsterUi = std::make_unique<const fheroes2::MonsterDialogElement>( tempMonster );
                    }

                    std::vector<int> allowedMonsters = getAllowedMonsters( tempMonster, objectType );

                    std::unique_ptr<const MonsterMultiSelection> selectionUi{ nullptr };
                    if ( !allowedMonsters.empty() ) {
                        selectionUi = std::make_unique<const MonsterMultiSelection>( std::move( allowedMonsters ), selectedMonsters, std::string( "Select Monsters:" ),
                                                                                     Settings::Get().isEvilInterfaceEnabled() );
                    }

                    if ( Dialog::SelectCount( std::move( str ), 0, 500000, monsterCount, 1, monsterUi.get(), selectionUi.get() )
                         && ( _mapFormat.monsterMetadata[object.id].count != monsterCount || ( selectionUi && selectedMonsters != selectionUi->getSelected() ) ) ) {
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );
                        _mapFormat.monsterMetadata[object.id].count = monsterCount;

                        if ( selectionUi ) {
                            _mapFormat.monsterMetadata[object.id].selected = selectionUi->getSelected();
                        }
                        action.commit();
                    }
                }
                else if ( objectInfo.objectType == MP2::OBJ_RESOURCE ) {
                    assert( _mapFormat.resourceMetadata.find( object.id ) != _mapFormat.resourceMetadata.end() );

                    auto resourceMetadata = _mapFormat.resourceMetadata.find( object.id );
                    int32_t resourceCount = resourceMetadata->second.count;

                    const int32_t resourceType = static_cast<int32_t>( objectInfo.metadata[0] );

                    const fheroes2::ResourceDialogElement resourceUI( resourceType, {} );

                    std::string str = _( "Set %{resource-type} Count" );
                    StringReplace( str, "%{resource-type}", Resource::String( resourceType ) );

                    // We cannot support more than 6 digits in the dialog due to its UI element size.
                    if ( Dialog::SelectCount( std::move( str ), 0, 999999, resourceCount, 1, &resourceUI ) && resourceMetadata->second.count != resourceCount ) {
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );
                        resourceMetadata->second.count = resourceCount;
                        action.commit();
                    }
                }
                else if ( object.group == Maps::ObjectGroup::ADVENTURE_ARTIFACTS ) {
                    if ( objectInfo.objectType == MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT ) {
                        assert( _mapFormat.artifactMetadata.find( object.id ) != _mapFormat.artifactMetadata.end() );

                        auto & originalRadius = _mapFormat.artifactMetadata[object.id].radius;
                        int32_t radius = originalRadius;

                        if ( Dialog::SelectCount( _( "Set Random Ultimate Artifact Radius" ), 0, 100, radius ) && radius != originalRadius ) {
                            fheroes2::ActionCreator action( _historyManager, _mapFormat );
                            originalRadius = radius;
                            action.commit();
                        }
                    }
                    else if ( objectInfo.objectType == MP2::OBJ_ARTIFACT && objectInfo.metadata[0] == Artifact::SPELL_SCROLL ) {
                        assert( _mapFormat.artifactMetadata.find( object.id ) != _mapFormat.artifactMetadata.end() );

                        auto & selected = _mapFormat.artifactMetadata[object.id].selected;

                        const auto artifactSpellId = selected.empty() ? 0 : selected.front();

                        const int newSpellId = Dialog::selectSpell( artifactSpellId, true ).GetID();

                        if ( newSpellId == Spell::NONE || artifactSpellId == newSpellId ) {
                            // We do not place the Spell Scroll artifact if the spell for it was not selected
                            // or when the same spell was chosen.
                            return;
                        }

                        fheroes2::ActionCreator action( _historyManager, _mapFormat );

                        // As of now only one spell can be selected for Spell Scroll.
                        selected.clear();
                        selected.emplace_back( newSpellId );

                        Maps::setSpellOnTile( tile, newSpellId );

                        action.commit();
                    }
                    else if ( Artifact( static_cast<int>( objectInfo.metadata[0] ) ).isValid() ) {
                        fheroes2::ArtifactDialogElement( static_cast<int>( objectInfo.metadata[0] ) ).showPopup( Dialog::OK );
                    }
                    else {
                        std::string msg = _( "%{object} has no properties to change." );
                        StringReplace( msg, "%{object}", _( "This artifact" ) );
                        _warningMessage.reset( std::move( msg ) );
                    }
                }
                else if ( objectType == MP2::OBJ_SPHINX ) {
                    assert( _mapFormat.sphinxMetadata.find( object.id ) != _mapFormat.sphinxMetadata.end() );

                    auto & originalMetadata = _mapFormat.sphinxMetadata[object.id];
                    Maps::Map_Format::SphinxMetadata newMetadata = originalMetadata;

                    if ( Editor::openSphinxWindow( newMetadata, _mapFormat.mainLanguage ) && newMetadata != originalMetadata ) {
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );
                        originalMetadata = std::move( newMetadata );
                        action.commit();
                    }
                }
                else if ( objectType == MP2::OBJ_SHRINE_FIRST_CIRCLE || objectType == MP2::OBJ_SHRINE_SECOND_CIRCLE || objectType == MP2::OBJ_SHRINE_THIRD_CIRCLE ) {
                    if ( _mapFormat.selectionObjectMetadata.find( object.id ) == _mapFormat.selectionObjectMetadata.end() ) {
                        _mapFormat.selectionObjectMetadata[object.id] = {};
                    }

                    auto & originalMetadata = _mapFormat.selectionObjectMetadata[object.id];
                    auto newMetadata = originalMetadata;

                    int spellLevel = 0;
                    if ( objectType == MP2::OBJ_SHRINE_FIRST_CIRCLE ) {
                        spellLevel = 1;
                    }
                    else if ( objectType == MP2::OBJ_SHRINE_SECOND_CIRCLE ) {
                        spellLevel = 2;
                    }
                    else if ( objectType == MP2::OBJ_SHRINE_THIRD_CIRCLE ) {
                        spellLevel = 3;
                    }
                    else {
                        assert( 0 );
                        spellLevel = 1;
                    }

                    if ( Editor::openSpellSelectionWindow( MP2::StringObject( objectType ), spellLevel, newMetadata.selectedItems, false, 1, false )
                         && originalMetadata.selectedItems != newMetadata.selectedItems ) {
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );
                        originalMetadata = std::move( newMetadata );
                        action.commit();
                    }
                }
                else if ( objectType == MP2::OBJ_WITCHS_HUT ) {
                    if ( _mapFormat.selectionObjectMetadata.find( object.id ) == _mapFormat.selectionObjectMetadata.end() ) {
                        _mapFormat.selectionObjectMetadata[object.id] = {};
                    }

                    auto & originalMetadata = _mapFormat.selectionObjectMetadata[object.id];
                    auto newMetadata = originalMetadata;

                    if ( Editor::openSecondarySkillSelectionWindow( MP2::StringObject( objectType ), 1, newMetadata.selectedItems )
                         && originalMetadata.selectedItems != newMetadata.selectedItems ) {
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );
                        originalMetadata = std::move( newMetadata );
                        action.commit();
                    }
                }
                else if ( objectType == MP2::OBJ_PYRAMID ) {
                    if ( _mapFormat.selectionObjectMetadata.find( object.id ) == _mapFormat.selectionObjectMetadata.end() ) {
                        _mapFormat.selectionObjectMetadata[object.id] = {};
                    }

                    auto & originalMetadata = _mapFormat.selectionObjectMetadata[object.id];
                    auto newMetadata = originalMetadata;

                    int spellLevel = 5;
                    if ( Editor::openSpellSelectionWindow( MP2::StringObject( objectType ), spellLevel, newMetadata.selectedItems, false, 1, false )
                         && originalMetadata.selectedItems != newMetadata.selectedItems ) {
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );
                        originalMetadata = std::move( newMetadata );
                        action.commit();
                    }
                }
                else if ( objectType == MP2::OBJ_OBELISK ) {
                    std::string str = _( "The total number of obelisks is %{count}." );
                    StringReplace( str, "%{count}", getObeliskCount( _mapFormat ) );

                    fheroes2::showStandardTextMessage( MP2::StringObject( objectType ), std::move( str ), Dialog::OK );
                }
                else if ( Maps::isCapturableObject( objectType ) ) {
                    if ( Color::Count( _mapFormat.availablePlayerColors ) == 0 ) {
                        _warningMessage.reset( _( "There are no players on the map, so no one can own this object." ) );
                        return;
                    }

                    auto ownershipMetadata = _mapFormat.capturableObjectsMetadata.find( object.id );
                    const bool hasOwnershipMetadata = ( ownershipMetadata != _mapFormat.capturableObjectsMetadata.end() );
                    const PlayerColor ownerColor = hasOwnershipMetadata ? ownershipMetadata->second.ownerColor : PlayerColor::NONE;

                    const PlayerColor newColor = Dialog::selectPlayerColor( ownerColor, _mapFormat.availablePlayerColors );

                    if ( newColor != ownerColor ) {
                        fheroes2::ActionCreator action( _historyManager, _mapFormat );

                        if ( newColor == PlayerColor::NONE ) {
                            _mapFormat.capturableObjectsMetadata.erase( object.id );
                        }
                        else if ( hasOwnershipMetadata ) {
                            ownershipMetadata->second.ownerColor = newColor;
                        }
                        else {
                            _mapFormat.capturableObjectsMetadata[object.id].ownerColor = newColor;
                        }

                        world.CaptureObject( tileIndex, newColor );
                        setRedraw( mapUpdateFlags );

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

                Maps::setTerrainOnTiles( _mapFormat, indices.x, indices.y, groundId );
            }
            else {
                assert( brushSize.width == 0 );

                // This is a case when area was not selected but a single tile was clicked.
                Maps::setTerrainOnTiles( _mapFormat, tileIndex, tileIndex, groundId );

                _areaSelectionStartTileId = -1;
            }

            _validateObjectsOnTerrainUpdate();

            _redraw |= mapUpdateFlags;

            action.commit();
        }
        else if ( _editorPanel.isRoadDraw() ) {
            if ( tile.isWater() ) {
                std::string msg = _( "%{objects} cannot be placed on water." );
                StringReplace( msg, "%{objects}", _( "Roads" ) );
                _warningMessage.reset( std::move( msg ) );
                return;
            }

            fheroes2::ActionCreator action( _historyManager, _mapFormat );

            if ( Maps::updateRoadOnTile( _mapFormat, tile.GetIndex(), true ) ) {
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

            if ( Maps::addStream( _mapFormat, tileIndex ) ) {
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
            }

            if ( brushSize.width == 0 ) {
                // This is a case when area was not selected but a single tile was clicked.
                _areaSelectionStartTileId = -1;
            }
        }
        else if ( _editorPanel.isObjectMode() ) {
            _handleObjectMouseLeftClick( tile );
        }
    }

    void EditorInterface::_handleObjectMouseLeftClick( Maps::Tile & tile )
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
            size_t kingdomHeroCount = 0;
            size_t mapHeroCount = 0;
            for ( const auto & mapTile : _mapFormat.tiles ) {
                for ( const auto & object : mapTile.objects ) {
                    if ( object.group == groupType ) {
                        assert( object.index < objects.size() );
                        if ( objects[object.index].metadata[0] == color ) {
                            ++kingdomHeroCount;
                        }

                        ++mapHeroCount;
                    }
                    else if ( Maps::isJailObject( object.group, object.index ) ) {
                        // Jails are also heroes, but in jail.
                        ++mapHeroCount;
                    }
                }
            }

            if ( mapHeroCount >= AllHeroes::getMaximumAllowedHeroes() ) {
                // TODO: Add new hero portraits and allow heroes with custom names (and portraits) exceed this limit.

                std::string warning( _( "A maximum of %{count} heroes including jailed heroes can be placed on the map." ) );
                StringReplace( warning, "%{count}", AllHeroes::getMaximumAllowedHeroes() );
                _warningMessage.reset( std::move( warning ) );
                return;
            }

            if ( kingdomHeroCount >= GameStatic::GetKingdomMaxHeroes() ) {
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
                assert( _mapFormat.artifactMetadata.find( insertedObject.id ) != _mapFormat.artifactMetadata.end() );

                auto & selected = _mapFormat.artifactMetadata[insertedObject.id].selected;
                selected.clear();
                selected.emplace_back( spellId );

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

            const int32_t bottomIndex = Maps::GetDirectionIndex( tile.GetIndex(), Direction::BOTTOM );

            if ( Maps::isValidAbsIndex( bottomIndex ) && Maps::doesContainRoads( _mapFormat.tiles[bottomIndex] ) ) {
                // Update road if there is one in front of the town/castle entrance.
                Maps::updateRoadSpriteOnTile( _mapFormat, bottomIndex, false );
            }

            // By default use random (default) army for the neutral race town/castle.
            if ( Color::IndexToColor( color ) == PlayerColor::NONE ) {
                Maps::setDefaultCastleDefenderArmy( _mapFormat.castleMetadata[Maps::getLastObjectUID()] );
            }

            // Add flags.
            assert( tile.GetIndex() > 0 && tile.GetIndex() < world.w() * world.h() - 1 );
            Maps::setLastObjectUID( objectId );

            if ( !_setObjectOnTile( world.getTile( tile.GetIndex() - 1 ), Maps::ObjectGroup::LANDSCAPE_FLAGS, color * 2 ) ) {
                return;
            }

            Maps::setLastObjectUID( objectId );

            if ( !_setObjectOnTile( world.getTile( tile.GetIndex() + 1 ), Maps::ObjectGroup::LANDSCAPE_FLAGS, color * 2 + 1 ) ) {
                return;
            }

            world.addCastle( tile.GetIndex(), Race::IndexToRace( static_cast<int>( townObjectInfo.metadata[0] ) ), Color::IndexToColor( color ) );

            action.commit();

            if ( !Maps::updateMapPlayers( _mapFormat ) ) {
                _warningMessage.reset( _( "Failed to update player information." ) );
            }
        }
        else if ( groupType == Maps::ObjectGroup::ADVENTURE_MINES ) {
            if ( objectType < 0 ) {
                // Check your logic!
                assert( 0 );
                return;
            }

            if ( !verifyObjectPlacement( tilePos, groupType, objectType, errorMessage ) ) {
                _warningMessage.reset( std::move( errorMessage ) );
                return;
            }

            fheroes2::ActionCreator action( _historyManager, _mapFormat );

            if ( !_setObjectOnTile( tile, groupType, objectType ) ) {
                return;
            }

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
            if ( const int riverDeltaDirection = Maps::getRiverDeltaDirectionByIndex( groupType, objectType ); riverDeltaDirection != Direction::UNKNOWN ) {
                Maps::updateStreamsToDeltaConnection( _mapFormat, tile.GetIndex(), riverDeltaDirection );
            }

            action.commit();
        }
        else if ( groupType == Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS ) {
            const auto & objectInfo = Maps::getObjectInfo( groupType, objectType );

            if ( objectInfo.objectType == MP2::OBJ_OBELISK ) {
                const size_t obeliskCount = getObeliskCount( _mapFormat );
                if ( obeliskCount >= numOfPuzzleTiles ) {
                    std::string warning( _( "A maximum of %{count} obelisks can be placed on the map." ) );
                    StringReplace( warning, "%{count}", numOfPuzzleTiles );
                    _warningMessage.reset( std::move( warning ) );
                    return;
                }
            }
            else if ( objectInfo.objectType == MP2::OBJ_JAIL ) {
                size_t mapHeroCount = 0;
                for ( const auto & mapTile : _mapFormat.tiles ) {
                    for ( const auto & object : mapTile.objects ) {
                        if ( object.group == Maps::ObjectGroup::KINGDOM_HEROES || Maps::isJailObject( object.group, object.index ) ) {
                            ++mapHeroCount;
                        }
                    }
                }

                if ( mapHeroCount >= AllHeroes::getMaximumAllowedHeroes() ) {
                    // TODO: Add new hero portraits and allow heroes with custom names (and portraits) exceed this limit.

                    std::string warning( _( "A maximum of %{count} heroes including jailed heroes can be placed on the map." ) );
                    StringReplace( warning, "%{count}", AllHeroes::getMaximumAllowedHeroes() );
                    _warningMessage.reset( std::move( warning ) );
                    return;
                }
            }

            if ( !verifyObjectPlacement( tilePos, groupType, objectType, errorMessage ) ) {
                _warningMessage.reset( std::move( errorMessage ) );
                return;
            }

            _setObjectOnTileAsAction( tile, groupType, objectType );
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
        Editor::showPopupWindow( world.getTile( tileIndex ), _mapFormat );
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

    bool EditorInterface::_setObjectOnTile( Maps::Tile & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex )
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

    bool EditorInterface::_setObjectOnTileAsAction( Maps::Tile & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex )
    {
        fheroes2::ActionCreator action( _historyManager, _mapFormat );

        if ( _setObjectOnTile( tile, groupType, objectIndex ) ) {
            action.commit();
            return true;
        }

        return false;
    }

    bool EditorInterface::generateNewMap( const int32_t mapWidth )
    {
        if ( mapWidth <= 0 ) {
            return false;
        }

        Settings & conf = Settings::Get();

        if ( !conf.isPriceOfLoyaltySupported() ) {
            assert( 0 );

            return false;
        }

        _mapFormat = {};

        world.generateUninitializedMap( mapWidth );

        if ( world.w() != mapWidth || world.h() != mapWidth ) {
            assert( 0 );

            return false;
        }

        _mapFormat.width = mapWidth;

        // Only square maps are supported so map height is the same as width.
        const int32_t tilesCount = mapWidth * mapWidth;

        _mapFormat.tiles.resize( tilesCount );

        for ( int32_t i = 0; i < tilesCount; ++i ) {
            world.getTile( i ).setIndex( i );
        }

        Maps::setTerrainOnTiles( _mapFormat, 0, tilesCount - 1, Maps::Ground::WATER );

        Maps::resetObjectUID();

        _loadedFileName.clear();

        conf.getCurrentMapInfo().version = GameVersion::RESURRECTION;

        return true;
    }

    bool EditorInterface::loadMap( const std::string & filePath )
    {
        if ( !Maps::Map_Format::loadMap( filePath, _mapFormat ) ) {
            fheroes2::showStandardTextMessage( _( "Error" ), "Failed to load the map.", Dialog::OK );
            return false;
        }

        if ( !Maps::readMapInEditor( _mapFormat ) ) {
            fheroes2::showStandardTextMessage( _( "Error" ), "Failed to read the map.", Dialog::OK );
            return false;
        }

        _loadedFileName = System::GetStem( filePath );

        // Set the loaded map as a default map for the new Standard Game.
        Maps::FileInfo fi;
        if ( fi.loadResurrectionMap( _mapFormat, filePath ) ) {
            Settings::Get().setCurrentMapInfo( std::move( fi ) );
        }
        else {
            assert( 0 );
        }

        return true;
    }

    void EditorInterface::saveMapToFile()
    {
        if ( !Maps::updateMapPlayers( _mapFormat ) ) {
            fheroes2::showStandardTextMessage( _( "Error" ), _( "The map is corrupted." ), Dialog::OK );
            return;
        }

        const std::string dataPath = System::GetDataDirectory( "fheroes2" );
        if ( dataPath.empty() ) {
            fheroes2::showStandardTextMessage( _( "Error" ), _( "Unable to locate data directory to save the map." ), Dialog::OK );
            return;
        }

        std::string mapDirectory = System::concatPath( dataPath, "maps" );

        if ( !System::IsDirectory( mapDirectory ) && !System::MakeDirectory( mapDirectory ) ) {
            fheroes2::showStandardTextMessage( _( "Error" ), _( "Unable to create a directory to save the map." ), Dialog::OK );
            return;
        }

        // Since the name of the map directory can be in arbitrary case, we need to get its real case-sensitive name first
        {
            std::string correctedMapDirectory;

            if ( !System::GetCaseInsensitivePath( mapDirectory, correctedMapDirectory ) ) {
                fheroes2::showStandardTextMessage( _( "Error" ), _( "Unable to locate a directory to save the map." ), Dialog::OK );
                return;
            }

            mapDirectory = std::move( correctedMapDirectory );
        }

        std::string fileName = _loadedFileName;
        std::string mapName = _mapFormat.name;
        std::string fullPath;

        while ( true ) {
            if ( !Editor::mapSaveSelectFile( fileName, mapName, _mapFormat.mainLanguage, maxMapNameLength ) ) {
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
                // Update the default map info to allow to start this map without the need to select it from the all maps list.
                Settings::Get().setCurrentMapInfo( std::move( fi ) );
            }
            else {
                assert( 0 );
            }

            _warningMessage.reset( _( "Map saved to: " ) + std::move( fullPath ) );

            return;
        }

        fheroes2::showStandardTextMessage( _( "Error" ), _( "Failed to save the map." ), Dialog::OK );
    }

    void EditorInterface::openMapSpecificationsDialog()
    {
        Maps::Map_Format::MapFormat mapBackup = _mapFormat;

        if ( Editor::mapSpecificationsDialog( _mapFormat, maxMapNameLength ) ) {
            fheroes2::ActionCreator action( _historyManager, _mapFormat );
            action.commit();
        }
        else {
            _mapFormat = std::move( mapBackup );
        }
    }

    void EditorInterface::_validateObjectsOnTerrainUpdate()
    {
        std::string errorMessage;

        std::set<uint32_t> uids;
        std::set<Maps::ObjectGroup> groups;

        for ( size_t i = 0; i < _mapFormat.tiles.size(); ++i ) {
            const fheroes2::Point pos{ static_cast<int32_t>( i ) % world.w(), static_cast<int32_t>( i ) / world.w() };

            bool removeRoad = false;

            for ( const auto & object : _mapFormat.tiles[i].objects ) {
                if ( object.group == Maps::ObjectGroup::LANDSCAPE_FLAGS || object.group == Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS ) {
                    // These objects belong to the main objects and will be checked with them.
                    continue;
                }

                if ( object.group == Maps::ObjectGroup::ROADS ) {
                    if ( world.getTile( static_cast<int32_t>( i ) ).isWater() ) {
                        removeRoad = true;
                    }

                    continue;
                }

                if ( !verifyTerrainPlacement( pos, object.group, static_cast<int32_t>( object.index ), errorMessage ) ) {
                    uids.emplace( object.id );
                    groups.emplace( object.group );
                }
            }

            if ( removeRoad ) {
                Maps::updateRoadOnTile( _mapFormat, static_cast<int32_t>( i ), false );
            }
        }

        if ( !uids.empty() ) {
            removeObjects( _mapFormat, uids, groups );
        }

        // Run through each town and castle and update its terrain.
        for ( size_t i = 0; i < _mapFormat.tiles.size(); ++i ) {
            for ( auto & object : _mapFormat.tiles[i].objects ) {
                if ( object.group == Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS ) {
                    const auto & worldTile = world.getTile( static_cast<int32_t>( i ) );
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
            const int32_t type = _editorPanel.getSelectedObjectType();

            if ( type < 0 ) {
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

        [[maybe_unused]] size_t objectsReplaced = 0;

        // This logic is based on an assumption that only one action object can exist on one tile.
        if ( replaceKey( _mapFormat.resourceMetadata, object.id, newObjectUID ) ) {
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

        if ( replaceKey( _mapFormat.selectionObjectMetadata, object.id, newObjectUID ) ) {
            ++objectsReplaced;
        }

        if ( replaceKey( _mapFormat.capturableObjectsMetadata, object.id, newObjectUID ) ) {
            ++objectsReplaced;
        }

        if ( replaceKey( _mapFormat.monsterMetadata, object.id, newObjectUID ) ) {
            ++objectsReplaced;
        }

        if ( replaceKey( _mapFormat.artifactMetadata, object.id, newObjectUID ) ) {
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
