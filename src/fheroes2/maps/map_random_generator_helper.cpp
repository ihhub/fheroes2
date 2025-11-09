/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
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

#include "map_random_generator_helper.h"

#include <cassert>

#include <maps_tiles_helper.h>
#include <world_object_uid.h>

#include "ground.h"
#include "map_format_helper.h"
#include "map_format_info.h"
#include "map_object_info.h"
#include "rand.h"
#include "ui_map_object.h"
#include "world.h"

namespace
{
    constexpr int randomCastleIndex{ 12 };
    constexpr int randomTownIndex{ 13 };
    constexpr int maxPlacementAttempts{ 30 };

    const std::map<int, std::vector<int>> obstaclesPerGround = {
        { Maps::Ground::DESERT, { 24, 25, 26, 27, 28, 29 } },    { Maps::Ground::SNOW, { 30, 31, 32, 33, 34, 35 } },  { Maps::Ground::SWAMP, { 18, 19, 20, 21, 22, 23 } },
        { Maps::Ground::WASTELAND, { 12, 13, 14, 15, 16, 17 } }, { Maps::Ground::BEACH, { 24, 25, 26, 27, 28, 29 } }, { Maps::Ground::LAVA, { 6, 7, 8, 9, 10, 11 } },
        { Maps::Ground::DIRT, { 18, 19, 20, 21, 22, 23 } },      { Maps::Ground::GRASS, { 0, 1, 2, 3, 4, 5 } },       { Maps::Ground::WATER, {} },
    };
}

namespace Maps::Random_Generator
{
    constexpr int32_t treeTypeFromGroundType( const int groundType )
    {
        switch ( groundType ) {
        case Maps::Ground::WATER:
            assert( 0 );
            return 0;
        case Maps::Ground::GRASS:
            return 0;
        case Maps::Ground::SNOW:
            return 30;
        case Maps::Ground::SWAMP:
            return 0;
        case Maps::Ground::LAVA:
            return 6;
        case Maps::Ground::DESERT:
            return 24;
        case Maps::Ground::DIRT:
            return 18;
        case Maps::Ground::WASTELAND:
            return 12;
        case Maps::Ground::BEACH:
            return 24;
        default:
            // Have you added a new ground? Add the logic above!
            assert( 0 );
            break;
        }
        return 0;
    }

    constexpr int32_t mountainTypeFromGroundType( const int groundType )
    {
        switch ( groundType ) {
        case Maps::Ground::WATER:
            assert( 0 );
            return 0;
        case Maps::Ground::GRASS:
            return 6;
        case Maps::Ground::SNOW:
            return 12;
        case Maps::Ground::SWAMP:
            return 18;
        case Maps::Ground::LAVA:
            return 24;
        case Maps::Ground::DESERT:
            return 30;
        case Maps::Ground::DIRT:
            return 36;
        case Maps::Ground::WASTELAND:
            return 44;
        case Maps::Ground::BEACH:
            return 0;
        default:
            // Have you added a new ground? Add the logic above!
            assert( 0 );
            break;
        }
        return 0;
    }

    int32_t localizeObjectToTerrain( const Maps::ObjectGroup groupType, const int32_t objectIndex, const int groundType )
    {
        if ( groupType == Maps::ObjectGroup::LANDSCAPE_TREES ) {
            assert( objectIndex < 6 );
            return treeTypeFromGroundType( groundType ) + objectIndex;
        }

        if ( groupType == Maps::ObjectGroup::LANDSCAPE_MOUNTAINS ) {
            assert( objectIndex < 8 );
            return mountainTypeFromGroundType( groundType ) + objectIndex;
        }
        return objectIndex;
    }

    template <class F>
    bool iterateOverObjectParts( const Maps::ObjectInfo & info, const F & lambda )
    {
        for ( const auto & objectPart : info.groundLevelParts ) {
            if ( objectPart.layerType == Maps::SHADOW_LAYER || objectPart.layerType == Maps::TERRAIN_LAYER ) {
                // Shadow and terrain layer parts are ignored.
                continue;
            }

            lambda( objectPart );
        }
        for ( const auto & objectPart : info.topLevelParts ) {
            lambda( objectPart );
        }
        return true;
    }

    bool canFitObject( const NodeCache & data, const Maps::ObjectInfo & info, const fheroes2::Point & mainTilePos, const bool isAction, const bool skipBorders )
    {
        bool invalid = false;
        fheroes2::Rect objectRect;

        iterateOverObjectParts( info, [&data, &mainTilePos, &objectRect, skipBorders, &invalid]( const auto & partInfo ) {
            const Node & node = data.getNode( mainTilePos + partInfo.tileOffset );

            if ( node.index == -1 || node.region == 0 ) {
                invalid = true;
                return;
            }
            if ( node.type != NodeType::OPEN && ( skipBorders || node.type != NodeType::BORDER ) ) {
                invalid = true;
                return;
            }

            objectRect.x = std::min( objectRect.x, partInfo.tileOffset.x );
            objectRect.width = std::max( objectRect.width, partInfo.tileOffset.x );
        } );

        if ( invalid ) {
            return false;
        }

        if ( isAction ) {
            for ( int x = objectRect.x - 1; x <= objectRect.width + 1; ++x ) {
                const Node & pathNode = data.getNode( mainTilePos + fheroes2::Point{ x, 1 } );
                if ( pathNode.index == -1 || pathNode.type == NodeType::OBSTACLE ) {
                    return false;
                }
                if ( pathNode.type == NodeType::BORDER ) {
                    return false;
                }
            }
        }

        return true;
    }

    bool canFitObjectSet( const NodeCache & data, const ObjectSet & set, const fheroes2::Point & mainTilePos )
    {
        for ( const fheroes2::Point offset : set.entranceCheck ) {
            const Node & node = data.getNode( mainTilePos + offset );
            if ( node.index == -1 || node.type != NodeType::OPEN ) {
                return false;
            }
        }

        for ( const auto * placements : { &set.obstacles, &set.valuables, &set.monsters } ) {
            for ( const auto & placement : *placements ) {
                const auto & info = Maps::getObjectInfo( placement.groupType, placement.objectIndex );
                const bool isAction = MP2::isInGameActionObject( info.objectType );
                const fheroes2::Point position = mainTilePos + placement.offset;
                bool invalid = false;

                iterateOverObjectParts( info, [&data, &position, isAction, &invalid]( const auto & partInfo ) {
                    const Node & node = data.getNode( position + partInfo.tileOffset );

                    if ( node.index == -1 || node.region == 0 ) {
                        invalid = true;
                        return;
                    }
                    if ( node.type != NodeType::OPEN && ( isAction || node.type != NodeType::BORDER ) ) {
                        invalid = true;
                        return;
                    }
                } );

                if ( invalid ) {
                    return false;
                }
            }
        }

        return true;
    }

    void markObjectPlacement( NodeCache & data, const Maps::ObjectInfo & info, const fheroes2::Point & mainTilePos, const bool isAction )
    {
        fheroes2::Rect objectRect;

        iterateOverObjectParts( info, [&data, &mainTilePos, &objectRect]( const auto & partInfo ) {
            Node & node = data.getNode( mainTilePos + partInfo.tileOffset );
            objectRect.x = std::min( objectRect.x, partInfo.tileOffset.x );
            objectRect.width = std::max( objectRect.width, partInfo.tileOffset.x );

            node.type = NodeType::OBSTACLE;
        } );

        if ( !isAction ) {
            return;
        }

        for ( int x = objectRect.x - 1; x <= objectRect.width + 1; ++x ) {
            data.getNode( mainTilePos + fheroes2::Point{ x, objectRect.height + 1 } ).type = NodeType::PATH;
        }

        // Mark extra nodes as path to avoid objects clumping together
        data.getNode( mainTilePos + fheroes2::Point{ objectRect.x - 1, 0 } ).type = NodeType::PATH;
        data.getNode( mainTilePos + fheroes2::Point{ objectRect.width + 1, 0 } ).type = NodeType::PATH;

        data.getNode( mainTilePos ).type = NodeType::ACTION;
    }

    bool putObjectOnMap( Maps::Map_Format::MapFormat & mapFormat, Maps::Tile & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex )
    {
        const auto & objectInfo = Maps::getObjectInfo( groupType, objectIndex );
        if ( objectInfo.empty() ) {
            assert( 0 );
            return false;
        }

        const MP2::MapObjectType mp2Type = objectInfo.objectType;
        // Maps::setObjectOnTile isn't idempotent, check if object was already placed
        if ( MP2::isInGameActionObject( mp2Type ) && tile.getMainObjectType() == mp2Type ) {
            return false;
        }

        // Do not update passabilities after every object placement.
        if ( !Maps::setObjectOnTile( tile, objectInfo, false ) ) {
            return false;
        }

        Maps::addObjectToMap( mapFormat, tile.GetIndex(), groupType, static_cast<uint32_t>( objectIndex ) );

        return true;
    }

    bool actionObjectPlacer( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, Maps::Tile & tile, const Maps::ObjectGroup groupType, const int32_t type )
    {
        const fheroes2::Point tilePos = tile.GetCenter();
        const auto & objectInfo = Maps::getObjectInfo( groupType, type );
        if ( canFitObject( data, objectInfo, tilePos, true, true ) && putObjectOnMap( mapFormat, tile, groupType, type ) ) {
            markObjectPlacement( data, objectInfo, tilePos, true );
            return true;
        }

        return false;
    }

    bool placeCastle( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, const Region & region, const fheroes2::Point tilePos, const bool isCastle )
    {
        auto & tile = world.getTile( tilePos.x, tilePos.y );
        if ( tile.isWater() ) {
            return false;
        }

        const int32_t basementId = fheroes2::getTownBasementId( tile.GetGround() );
        const int32_t castleObjectId = isCastle ? randomCastleIndex : randomTownIndex;

        const auto & basementInfo = Maps::getObjectInfo( Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId );
        const auto & castleInfo = Maps::getObjectInfo( Maps::ObjectGroup::KINGDOM_TOWNS, castleObjectId );

        if ( canFitObject( data, basementInfo, tilePos, false, true ) && canFitObject( data, castleInfo, tilePos, true, true ) ) {
            if ( !putObjectOnMap( mapFormat, tile, Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId ) ) {
                return false;
            }

            // Since the whole object consists of multiple "objects" we have to put the same ID for all of them.
            // Every time an object is being placed on a map the counter is going to be increased by 1.
            // Therefore, we set the counter by 1 less for each object to match object UID for all of them.
            assert( Maps::getLastObjectUID() > 0 );
            const uint32_t objectId = Maps::getLastObjectUID() - 1;

            Maps::setLastObjectUID( objectId );

            if ( !putObjectOnMap( mapFormat, tile, Maps::ObjectGroup::KINGDOM_TOWNS, castleObjectId ) ) {
                return false;
            }

            const int32_t bottomIndex = Maps::GetDirectionIndex( tile.GetIndex(), Direction::BOTTOM );

            if ( Maps::isValidAbsIndex( bottomIndex ) && Maps::doesContainRoads( mapFormat.tiles[bottomIndex] ) ) {
                // Update road if there is one in front of the town/castle entrance.
                Maps::updateRoadSpriteOnTile( mapFormat, bottomIndex, false );
            }

            // By default use random (default) army for the neutral race town/castle.
            const PlayerColor color = Color::IndexToColor( region.colorIndex );
            if ( color == PlayerColor::NONE ) {
                Maps::setDefaultCastleDefenderArmy( mapFormat.castleMetadata[Maps::getLastObjectUID()] );
            }

            // Add flags.
            assert( tile.GetIndex() > 0 && tile.GetIndex() < world.w() * world.h() - 1 );
            Maps::setLastObjectUID( objectId );

            if ( !putObjectOnMap( mapFormat, world.getTile( tile.GetIndex() - 1 ), Maps::ObjectGroup::LANDSCAPE_FLAGS, Color::GetIndex( color ) * 2 ) ) {
                return false;
            }

            Maps::setLastObjectUID( objectId );

            if ( !putObjectOnMap( mapFormat, world.getTile( tile.GetIndex() + 1 ), Maps::ObjectGroup::LANDSCAPE_FLAGS, Color::GetIndex( color ) * 2 + 1 ) ) {
                return false;
            }

            const Maps::ObjectInfo & townObjectInfo = Maps::getObjectInfo( Maps::ObjectGroup::KINGDOM_TOWNS, castleObjectId );
            const uint8_t race = Race::IndexToRace( static_cast<int>( townObjectInfo.metadata[0] ) );

            world.addCastle( tile.GetIndex(), race, color );

            markObjectPlacement( data, basementInfo, tilePos, false );
            markObjectPlacement( data, castleInfo, tilePos, true );

            // Force roads coming from the castle
            const int32_t nextIndex = Maps::GetDirectionIndex( bottomIndex, Direction::BOTTOM );
            if ( Maps::isValidAbsIndex( nextIndex ) ) {
                Maps::updateRoadOnTile( mapFormat, bottomIndex, true );
                Maps::updateRoadOnTile( mapFormat, nextIndex, true );
            }

            return true;
        }

        return false;
    }

    bool placeMine( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & node, const int resource )
    {
        Maps::Tile & mineTile = world.getTile( node.index );
        const int32_t mineType = fheroes2::getMineObjectInfoId( resource, mineTile.GetGround() );
        return actionObjectPlacer( mapFormat, data, mineTile, Maps::ObjectGroup::ADVENTURE_MINES, mineType );
    }

    bool placeObstacle( Maps::Map_Format::MapFormat & mapFormat, const NodeCache & data, const Node & node, Rand::PCG32 & randomGenerator )
    {
        Maps::Tile & tile = world.getTile( node.index );
        const auto it = obstaclesPerGround.find( tile.GetGround() );
        if ( it == obstaclesPerGround.end() || it->second.empty() ) {
            return false;
        }

        std::vector<int> obstacleList = it->second;
        Rand::ShuffleWithGen( obstacleList, randomGenerator );

        const fheroes2::Point tilePos = tile.GetCenter();
        for ( const auto & obstacleId : obstacleList ) {
            const auto & objectInfo = Maps::getObjectInfo( Maps::ObjectGroup::LANDSCAPE_TREES, obstacleId );
            if ( canFitObject( data, objectInfo, tilePos, false, false ) && putObjectOnMap( mapFormat, tile, Maps::ObjectGroup::LANDSCAPE_TREES, obstacleId ) ) {
                return true;
            }
        }
        return false;
    }

    bool placeSimpleObject( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & centerNode, const ObjectPlacement & placement )
    {
        const fheroes2::Point position = Maps::GetPoint( centerNode.index ) + placement.offset;
        if ( !Maps::isValidAbsPoint( position.x, position.y ) ) {
            return false;
        }

        Maps::Tile & tile = world.getTile( position.x, position.y );
        const int32_t objectIndex = localizeObjectToTerrain( placement.groupType, placement.objectIndex, tile.GetGround() );
        const auto & objectInfo = Maps::getObjectInfo( placement.groupType, objectIndex );
        if ( putObjectOnMap( mapFormat, tile, placement.groupType, objectIndex ) ) {
            markObjectPlacement( data, objectInfo, position, false );
            return true;
        }
        return false;
    }

    void placeObjectSet( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, const Region & region, const std::vector<ObjectSet> & set,
                         const uint8_t expectedCount, Rand::PCG32 & randomGenerator )
    {
        int objectsPlaced = 0;
        for ( int attempt = 0; attempt < maxPlacementAttempts; ++attempt ) {
            if ( objectsPlaced == expectedCount ) {
                break;
            }

            const Node & node = Rand::GetWithGen( region.nodes, randomGenerator );

            std::vector<ObjectSet> shuffledObjectSets = set;
            Rand::ShuffleWithGen( shuffledObjectSets, randomGenerator );
            for ( const auto & prefab : shuffledObjectSets ) {
                if ( !canFitObjectSet( data, prefab, Maps::GetPoint( node.index ) ) ) {
                    continue;
                }

                for ( const auto & obstacle : prefab.obstacles ) {
                    placeSimpleObject( mapFormat, data, node, obstacle );
                }
                for ( const auto & treasure : prefab.valuables ) {
                    placeSimpleObject( mapFormat, data, node, treasure );
                }
                for ( const auto & monster : prefab.monsters ) {
                    const fheroes2::Point position = Maps::GetPoint( node.index ) + monster.offset;
                    putObjectOnMap( mapFormat, world.getTile( position.x, position.y ), Maps::ObjectGroup::MONSTERS, monster.objectIndex );
                }
                ++objectsPlaced;
                break;
            }
        }
    }
}
