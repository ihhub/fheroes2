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

#include "map_format_helper.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <utility>

#include "army.h"
#include "army_troop.h"
#include "castle.h"
#include "color.h"
#include "direction.h"
#include "ground.h"
#include "heroes.h"
#include "logging.h"
#include "map_format_info.h"
#include "map_object_info.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "monster.h"
#include "mp2.h"
#include "players.h"
#include "race.h"
#include "rand.h"
#include "world.h"
#include "world_object_uid.h"

namespace
{
    void addObjectToTile( Maps::Map_Format::TileInfo & info, const Maps::ObjectGroup group, const uint32_t index, const uint32_t uid )
    {
        auto & object = info.objects.emplace_back();
        object.id = uid;
        object.group = group;
        object.index = index;
    }

    struct IndexedObjectInfo
    {
        int32_t tileIndex{ -1 };

        const Maps::Map_Format::TileObjectInfo * info{ nullptr };
    };

    void loadArmyFromMetadata( Army & army, const std::array<int32_t, 5> & unitType, const std::array<int32_t, 5> & unitCount )
    {
        std::vector<Troop> troops( unitType.size() );
        for ( size_t i = 0; i < troops.size(); ++i ) {
            assert( unitType[i] >= 0 && unitCount[i] >= 0 );
            troops[i] = Troop{ unitType[i], static_cast<uint32_t>( unitCount[i] ) };
        }

        army.Assign( troops.data(), troops.data() + troops.size() );
    }

    void saveArmyToMetadata( const Army & army, std::array<int32_t, 5> & unitType, std::array<int32_t, 5> & unitCount )
    {
        const size_t armySize = army.Size();
        assert( unitType.size() == armySize );

        // Update army metadata.
        for ( size_t i = 0; i < armySize; ++i ) {
            const Troop * troop = army.GetTroop( i );
            assert( troop != nullptr );

            unitType[i] = troop->GetID();
            unitCount[i] = static_cast<int32_t>( troop->GetCount() );
        }
    }

    void updateWorldCastlesHeroes( const Maps::Map_Format::MapFormat & map )
    {
        assert( map.size == world.w() && map.size == world.h() );

        const auto & townObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_TOWNS );
        const auto & heroObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_HEROES );

        for ( size_t i = 0; i < map.tiles.size(); ++i ) {
            for ( const auto & object : map.tiles[i].objects ) {
                if ( object.group == Maps::ObjectGroup::KINGDOM_TOWNS ) {
                    const uint8_t color = Color::IndexToColor( Maps::getTownColorIndex( map, i, object.id ) );
                    const uint8_t race = Race::IndexToRace( static_cast<int>( townObjects[object.index].metadata[0] ) );

                    world.addCastle( static_cast<int32_t>( i ), race, color );
                }
                else if ( object.group == Maps::ObjectGroup::KINGDOM_HEROES ) {
                    const auto & metadata = heroObjects[object.index].metadata;
                    const uint8_t color = Color::IndexToColor( static_cast<int>( metadata[0] ) );

                    Heroes * hero = world.GetHeroForHire( static_cast<int>( metadata[1] ) );
                    if ( hero ) {
                        hero->SetCenter( { static_cast<int32_t>( i ) % world.w(), static_cast<int32_t>( i ) / world.w() } );
                        hero->SetColor( color );
                    }
                }
            }
        }
    }

    // This function only checks for Streams and ignores River Deltas.
    bool isStreamPresent( const Maps::Map_Format::TileInfo & mapTile )
    {
        return std::any_of( mapTile.objects.cbegin(), mapTile.objects.cend(),
                            []( const Maps::Map_Format::TileObjectInfo & object ) { return object.group == Maps::ObjectGroup::STREAMS; } );
    }

    bool isStreamToDeltaConnectionNeeded( Maps::Map_Format::MapFormat & map, const int32_t tileId, const int direction )
    {
        if ( direction != Direction::TOP && direction != Direction::BOTTOM && direction != Direction::RIGHT && direction != Direction::LEFT ) {
            return false;
        }

        if ( !Maps::isValidDirection( tileId, direction ) ) {
            return false;
        }

        // The center tile of river delta is located in the next tile.
        const int32_t nextTileId = Maps::GetDirectionIndex( tileId, direction );

        return std::any_of( map.tiles[nextTileId].objects.cbegin(), map.tiles[nextTileId].objects.cend(),
                            [&direction]( const Maps::Map_Format::TileObjectInfo & object ) {
                                return Maps::getRiverDeltaDirectionByIndex( object.group, static_cast<int32_t>( object.index ) ) == Direction::Reflect( direction );
                            } );
    }

    // Returns the direction vector bits from 'centerTileIndex' to the around tiles with streams.
    int getStreamDirecton( Maps::Map_Format::MapFormat & map, const int32_t currentTileId, const bool forceStreamOnTile )
    {
        assert( currentTileId >= 0 && map.tiles.size() > static_cast<size_t>( currentTileId ) );

        // Stream includes also the Deltas. For the current tile we need to check only streams excluding Deltas.
        if ( !forceStreamOnTile && !isStreamPresent( map.tiles[currentTileId] ) ) {
            // Current tile has no streams.
            return Direction::UNKNOWN;
        }

        int streamDirection = Direction::CENTER;

        // For streams we can check only the next four directions.
        for ( const int direction : { Direction::LEFT, Direction::TOP, Direction::RIGHT, Direction::BOTTOM } ) {
            if ( !Maps::isValidDirection( currentTileId, direction ) ) {
                continue;
            }

            const int32_t tileId = Maps::GetDirectionIndex( currentTileId, direction );
            assert( tileId >= 0 && map.tiles.size() > static_cast<size_t>( tileId ) );

            // Check also for Deltas connection.
            if ( isStreamPresent( map.tiles[tileId] ) || isStreamToDeltaConnectionNeeded( map, tileId, direction ) ) {
                streamDirection |= direction;
            }
        }

        return streamDirection;
    }

    bool hasBits( const int value, const int bits )
    {
        return ( value & bits ) == bits;
    }

    bool hasNoBits( const int value, const int bits )
    {
        return ( value & bits ) == 0;
    }

    uint8_t getStreamIndex( const int streamDirection )
    {
        if ( hasNoBits( streamDirection, Direction::CENTER ) ) {
            // This tile should not have a stream image.
            assert( 0 );
            return 255U;
        }

        if ( hasBits( streamDirection, Direction::LEFT | Direction::BOTTOM ) && hasNoBits( streamDirection, Direction::TOP | Direction::RIGHT ) ) {
            // \ - stream from the left to the bottom.
            return 0U;
        }
        if ( hasBits( streamDirection, Direction::RIGHT | Direction::BOTTOM ) && hasNoBits( streamDirection, Direction::TOP | Direction::LEFT ) ) {
            // / - stream from the right to the bottom.
            return 1U;
        }
        if ( hasBits( streamDirection, Direction::RIGHT | Direction::TOP ) && hasNoBits( streamDirection, Direction::BOTTOM | Direction::LEFT ) ) {
            // \ - stream from the top to the right.
            return 4U;
        }
        if ( hasBits( streamDirection, Direction::LEFT | Direction::TOP ) && hasNoBits( streamDirection, Direction::BOTTOM | Direction::RIGHT ) ) {
            // / - stream from the top to the left.
            return 7U;
        }
        if ( hasBits( streamDirection, Direction::LEFT | Direction::TOP | Direction::RIGHT ) && hasNoBits( streamDirection, Direction::BOTTOM ) ) {
            // _|_ - stream from the top to the left and right.
            return 8U;
        }
        if ( hasBits( streamDirection, Direction::BOTTOM | Direction::TOP | Direction::RIGHT ) && hasNoBits( streamDirection, Direction::LEFT ) ) {
            // |- - stream from the top to the right and bottom.
            return 9U;
        }
        if ( hasBits( streamDirection, Direction::BOTTOM | Direction::TOP | Direction::LEFT ) && hasNoBits( streamDirection, Direction::RIGHT ) ) {
            // -| - stream from the top to the left and bottom.
            return 10U;
        }
        if ( hasBits( streamDirection, Direction::BOTTOM | Direction::LEFT | Direction::RIGHT ) && hasNoBits( streamDirection, Direction::TOP ) ) {
            // \/ - stream from the left and right to the bottom.
            return 11U;
        }
        if ( hasBits( streamDirection, Direction::BOTTOM | Direction::LEFT | Direction::TOP | Direction::RIGHT ) ) {
            // -|- - streams are all around.
            return 6U;
        }
        if ( ( hasBits( streamDirection, Direction::LEFT ) || hasBits( streamDirection, Direction::RIGHT ) )
             && hasNoBits( streamDirection, Direction::TOP | Direction::BOTTOM ) ) {
            // - - horizontal stream.
            return Rand::Get( 1 ) ? 2U : 5U;
        }

        // | - in all other cases are the vertical stream sprite, including the case when there are no other streams around.
        return Rand::Get( 1 ) ? 3U : 12U;
    }

    void updateStreamObjectOnMapTile( Maps::Map_Format::MapFormat & map, const int32_t tileId, const bool forceStreamOnTile )
    {
        const int direction = getStreamDirecton( map, tileId, forceStreamOnTile );

        if ( hasNoBits( direction, Direction::CENTER ) ) {
            // There is no stream on this tile.
            return;
        }

        const uint8_t objectIndex = getStreamIndex( direction );

        if ( objectIndex == 255U ) {
            return;
        }

        assert( tileId >= 0 && map.tiles.size() > static_cast<size_t>( tileId ) );

        auto & tileObjects = map.tiles[tileId].objects;

        auto streamIter = std::find_if( tileObjects.begin(), tileObjects.end(),
                                        []( const Maps::Map_Format::TileObjectInfo & object ) { return object.group == Maps::ObjectGroup::STREAMS; } );

        if ( streamIter == tileObjects.end() ) {
            // Add stream to this tile.
            const auto & objectInfo = Maps::getObjectInfo( Maps::ObjectGroup::STREAMS, objectIndex );

            if ( !Maps::setObjectOnTile( world.getTile( tileId ), objectInfo, true ) ) {
                assert( 0 );
                return;
            }

            Maps::addObjectToMap( map, tileId, Maps::ObjectGroup::STREAMS, objectIndex );
        }
        else {
            streamIter->index = objectIndex;

            // Update image index for the `world` tile.
            Maps::Tile::updateTileObjectIcnIndex( world.getTile( tileId ), streamIter->id, objectIndex );
        }
    }

    bool isCastleObject( const MP2::MapObjectType type )
    {
        return ( type == MP2::OBJ_CASTLE ) || ( type == MP2::OBJ_RANDOM_TOWN ) || ( type == MP2::OBJ_RANDOM_CASTLE );
    }

    void removeRoads( Maps::Map_Format::TileInfo & tile, const int32_t tileIndex )
    {
        tile.objects.erase( std::remove_if( tile.objects.begin(), tile.objects.end(), []( const auto & object ) { return object.group == Maps::ObjectGroup::ROADS; } ),
                            tile.objects.end() );

        world.getTile( tileIndex ).removeObjects( MP2::OBJ_ICN_TYPE_ROAD );
    }

    bool doesContainStreams( const Maps::Map_Format::TileInfo & tile )
    {
        for ( const auto & object : tile.objects ) {
            if ( object.group == Maps::ObjectGroup::STREAMS ) {
                return true;
            }
        }

        return false;
    }

    bool doesContainCastleEntrance( const Maps::Map_Format::TileInfo & tile )
    {
        const auto & townObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_TOWNS );

        for ( const auto & object : tile.objects ) {
            if ( ( object.group == Maps::ObjectGroup::KINGDOM_TOWNS ) && isCastleObject( townObjects[object.index].objectType ) ) {
                // A castle has an entrance with a road.
                return true;
            }
        }

        return false;
    }

    // Returns the direction vector bits from 'centerTileIndex' where '_tileIsRoad' bit is set for the tiles around.
    int getRoadDirecton( const Maps::Map_Format::MapFormat & map, const int32_t mainTileIndex )
    {
        const auto & tile = map.tiles[mainTileIndex];

        // Castle entrance (active tile) is considered as a road, but it is not a real road so it should not be taken into account here.
        int roadDirection = 0;

        if ( !doesContainCastleEntrance( tile ) && Maps::doesContainRoads( tile ) ) {
            roadDirection = Direction::CENTER;
        }

        const Maps::Indexes around = Maps::getAroundIndexes( mainTileIndex, map.size, map.size, 1 );

        for ( const int32_t tileIndex : around ) {
            assert( tileIndex >= 0 && tileIndex < map.size * map.size );

            if ( Maps::doesContainRoads( map.tiles[tileIndex] ) ) {
                roadDirection |= Maps::GetDirection( mainTileIndex, tileIndex );
            }
        }

        return roadDirection;
    }

    uint8_t getRoadImageForTile( const Maps::Map_Format::MapFormat & map, const int32_t tileIndex, const int roadDirection )
    {
        // To place some roads we need to check not only the road directions around this tile, but also the road ICN index at the nearby tile.
        auto checkRoadIcnIndex = [&map]( const int32_t mapTileIndex, const std::vector<uint8_t> & roadIcnIndexes ) {
            const auto & currentTile = map.tiles[mapTileIndex];

            const auto & roadObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::ROADS );

            for ( const auto & object : currentTile.objects ) {
                if ( object.group == Maps::ObjectGroup::ROADS ) {
                    const uint32_t icnIndex = roadObjects[object.index].groundLevelParts.front().icnIndex;

                    return std::any_of( roadIcnIndexes.begin(), roadIcnIndexes.end(), [icnIndex]( const uint8_t index ) { return icnIndex == index; } );
                }
            }

            return false;
        };

        if ( hasNoBits( roadDirection, Direction::CENTER ) ) {
            if ( hasBits( roadDirection, Direction::TOP ) && hasNoBits( roadDirection, Direction::TOP_LEFT ) ) {
                // We can do this without 'isValidDirection()' check because we have Direction::TOP.
                const int32_t upperTileIndex = tileIndex - map.size;
                if ( checkRoadIcnIndex( upperTileIndex, { 7, 17, 20, 22, 24, 29 } ) ) {
                    return 8U;
                }
            }

            if ( hasBits( roadDirection, Direction::TOP ) && hasNoBits( roadDirection, Direction::TOP_RIGHT ) ) {
                // We can do this without 'isValidDirection()' check because we have Direction::TOP.
                const int32_t upperTileIndex = tileIndex - map.size;
                if ( checkRoadIcnIndex( upperTileIndex, { 16, 18, 19, 23, 25, 30 } ) ) {
                    return 15U;
                }
            }
            if ( hasBits( roadDirection, Direction::TOP )
                 && ( hasBits( roadDirection, Direction::TOP_LEFT ) || hasBits( roadDirection, Direction::TOP_RIGHT )
                      || hasBits( roadDirection, Direction::LEFT | Direction::RIGHT ) ) ) {
                // We can do this without 'isValidDirection()' check because we have Direction::TOP.
                const int32_t upperTileIndex = tileIndex - map.size;
                if ( checkRoadIcnIndex( upperTileIndex, { 2, 3, 21, 28 } ) ) {
                    return Rand::Get( 1 ) ? 1U : 27U;
                }
            }
            if ( hasBits( roadDirection, Direction::BOTTOM | Direction::RIGHT ) && hasNoBits( roadDirection, Direction::TOP | Direction::LEFT ) ) {
                // We can do this without 'isValidDirection()' check because we have Direction::BOTTOM.
                const int32_t lowerTileIndex = tileIndex + map.size;
                if ( checkRoadIcnIndex( lowerTileIndex, { 8, 9, 18, 20, 30 } ) ) {
                    return Rand::Get( 1 ) ? 22U : 24U;
                }
            }
            if ( hasBits( roadDirection, Direction::BOTTOM | Direction::LEFT ) && hasNoBits( roadDirection, Direction::TOP | Direction::RIGHT ) ) {
                // We can do this without 'isValidDirection()' check because we have Direction::BOTTOM.
                const int32_t lowerTileIndex = tileIndex + map.size;
                if ( checkRoadIcnIndex( lowerTileIndex, { 12, 15, 17, 19, 29 } ) ) {
                    return Rand::Get( 1 ) ? 23U : 25U;
                }
            }

            // The next 4 conditions are to end the horizontal roads.
            if ( hasBits( roadDirection, Direction::LEFT ) && hasNoBits( roadDirection, Direction::TOP ) && checkRoadIcnIndex( tileIndex - 1, { 2, 21, 28 } ) ) {
                return Rand::Get( 1 ) ? 23U : 25U;
            }
            if ( hasBits( roadDirection, Direction::RIGHT ) && hasNoBits( roadDirection, Direction::TOP ) && checkRoadIcnIndex( tileIndex + 1, { 2, 21, 28 } ) ) {
                return Rand::Get( 1 ) ? 22U : 24U;
            }
            if ( hasBits( roadDirection, Direction::TOP_LEFT ) && hasNoBits( roadDirection, Direction::TOP ) && checkRoadIcnIndex( tileIndex - 1, { 1, 4, 21, 27 } ) ) {
                return 15U;
            }
            if ( hasBits( roadDirection, Direction::TOP_RIGHT ) && hasNoBits( roadDirection, Direction::TOP ) && checkRoadIcnIndex( tileIndex + 1, { 1, 4, 21, 27 } ) ) {
                return 8U;
            }

            // This tile should not have a road image.
            return 255U;
        }

        // The rest checks are made for the tile with the road on it: it has Direction::CENTER.

        // There might be a castle entrance above. Check for it to properly connect the road to it.
        if ( tileIndex >= map.size ) {
            const auto & aboveTile = map.tiles[tileIndex - map.size];
            if ( doesContainCastleEntrance( aboveTile ) ) {
                return 31U;
            }
        }

        if ( hasBits( roadDirection, Direction::TOP | DIRECTION_CENTER_ROW )
             && ( hasBits( roadDirection, Direction::TOP_LEFT ) || hasBits( roadDirection, Direction::TOP_RIGHT ) ) ) {
            // = - horizontal road in this and in the upper tile.
            return 21U;
        }
        if ( ( ( ( hasBits( roadDirection, Direction::BOTTOM_RIGHT ) || hasBits( roadDirection, Direction::TOP_LEFT ) ) && hasNoBits( roadDirection, Direction::RIGHT ) )
               || hasBits( roadDirection, Direction::RIGHT | Direction::TOP_LEFT ) )
             && hasNoBits( roadDirection, Direction::TOP | Direction::BOTTOM | Direction::LEFT | Direction::TOP_RIGHT | Direction::BOTTOM_LEFT ) ) {
            // \ - diagonal road from top-left to bottom-right.
            return Rand::Get( 1 ) ? 17U : 29U;
        }
        if ( ( ( ( hasBits( roadDirection, Direction::BOTTOM_LEFT ) || hasBits( roadDirection, Direction::TOP_RIGHT ) ) && hasNoBits( roadDirection, Direction::LEFT ) )
               || hasBits( roadDirection, Direction::LEFT | Direction::TOP_RIGHT ) )
             && hasNoBits( roadDirection, Direction::TOP | Direction::RIGHT | Direction::BOTTOM | Direction::TOP_LEFT | Direction::BOTTOM_RIGHT ) ) {
            // / - diagonal road from top-right to bottom-left.
            return Rand::Get( 1 ) ? 18U : 30U;
        }
        if ( hasBits( roadDirection, Direction::TOP )
             && ( hasBits( roadDirection, Direction::LEFT | Direction::RIGHT ) || hasBits( roadDirection, Direction::BOTTOM_LEFT | Direction::RIGHT )
                  || hasBits( roadDirection, Direction::LEFT | Direction::BOTTOM_RIGHT )
                  || ( hasBits( roadDirection, Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT ) && hasNoBits( roadDirection, Direction::BOTTOM ) ) )
             && hasNoBits( roadDirection, Direction::TOP_LEFT | Direction::TOP_RIGHT ) ) {
            // _|_ - cross.
            return 3U;
        }
        if ( hasBits( roadDirection, Direction::TOP )
             && ( hasBits( roadDirection, Direction::TOP_LEFT | Direction::TOP_RIGHT ) || ( checkRoadIcnIndex( tileIndex - map.size, { 2, 28 } ) ) )
             && hasNoBits( roadDirection, Direction::LEFT | Direction::RIGHT ) ) {
            // T - cross. Also used for 90 degrees turn from the bottom to the left/right.
            return 4U;
        }
        if ( hasBits( roadDirection, Direction::TOP | Direction::TOP_RIGHT ) && hasNoBits( roadDirection, Direction::TOP_LEFT | Direction::RIGHT | Direction::LEFT ) ) {
            // Vertical road and branch to the right in the upper tile.
            return 5U;
        }
        if ( hasBits( roadDirection, Direction::TOP | Direction::RIGHT | Direction::BOTTOM ) && hasNoBits( roadDirection, Direction::TOP_RIGHT | Direction::LEFT ) ) {
            // L - cross.
            return 6U;
        }
        if ( hasBits( roadDirection, Direction::TOP ) && ( hasBits( roadDirection, Direction::RIGHT ) || hasBits( roadDirection, Direction::BOTTOM_RIGHT ) )
             && hasNoBits( roadDirection, Direction::BOTTOM | Direction::LEFT ) ) {
            // Road turn from the top tile to the right tile.
            return 7U;
        }
        if ( hasBits( roadDirection, Direction::TOP_RIGHT | Direction::BOTTOM )
             && hasNoBits( roadDirection, Direction::TOP | Direction::TOP_LEFT | Direction::LEFT | Direction::RIGHT ) ) {
            // Road turn from the bottom tile to the right tile.
            return 9U;
        }
        if ( hasBits( roadDirection, Direction::TOP_LEFT | Direction::BOTTOM )
             && hasNoBits( roadDirection, Direction::TOP | Direction::TOP_RIGHT | Direction::RIGHT | Direction::LEFT ) ) {
            // Road turn from the bottom tile to the left tile.
            return 12U;
        }
        if ( hasBits( roadDirection, Direction::TOP | Direction::TOP_LEFT ) && hasNoBits( roadDirection, Direction::TOP_RIGHT | Direction::RIGHT | Direction::LEFT ) ) {
            // Vertical road and branch to the left in the upper tile.
            return 13U;
        }
        if ( hasBits( roadDirection, Direction::TOP | Direction::LEFT | Direction::BOTTOM ) && hasNoBits( roadDirection, Direction::TOP_LEFT | Direction::RIGHT ) ) {
            // _| - cross.
            return 14U;
        }
        if ( hasBits( roadDirection, Direction::TOP ) && ( hasBits( roadDirection, Direction::LEFT ) || hasBits( roadDirection, Direction::BOTTOM_LEFT ) )
             && hasNoBits( roadDirection, Direction::BOTTOM | Direction::RIGHT ) ) {
            // Road turn from the top tile to the left tile.
            return 16U;
        }
        if ( hasBits( roadDirection, Direction::TOP_LEFT ) && ( hasBits( roadDirection, Direction::LEFT ) || hasBits( roadDirection, Direction::BOTTOM_LEFT ) )
             && hasNoBits( roadDirection, DIRECTION_RIGHT_COL ) && !checkRoadIcnIndex( tileIndex - 1, { 0, 3, 6, 7, 14, 16, 26 } ) ) {
            // ) - road.
            return 19U;
        }
        if ( hasBits( roadDirection, Direction::TOP_RIGHT ) && ( hasBits( roadDirection, Direction::RIGHT ) || hasBits( roadDirection, Direction::BOTTOM_RIGHT ) )
             && hasNoBits( roadDirection, DIRECTION_LEFT_COL ) && !checkRoadIcnIndex( tileIndex + 1, { 0, 3, 6, 7, 14, 16, 26 } ) ) {
            // ( - road.
            return 20U;
        }
        if ( ( hasBits( roadDirection, Direction::LEFT ) || hasBits( roadDirection, Direction::RIGHT )
               || ( hasBits( roadDirection, Direction::BOTTOM_RIGHT | Direction::BOTTOM_LEFT ) && hasNoBits( roadDirection, Direction::BOTTOM ) ) )
             && hasNoBits( roadDirection, Direction::TOP ) ) {
            // _ - horizontal road.
            return Rand::Get( 1 ) ? 2U : 28U;
        }
        if ( hasNoBits( roadDirection, Direction::LEFT | Direction::TOP_LEFT | Direction::TOP_RIGHT | Direction::RIGHT ) ) {
            // | - vertical road.
            return Rand::Get( 1 ) ? 0U : 26U;
        }

        // We have not found the appropriate road image and return the value for the incorrect image index.
        DEBUG_LOG( DBG_DEVEL, DBG_WARN, "No proper road image found for tile " << tileIndex << " with road directions: " << Direction::String( roadDirection ) )

        return 255U;
    }

    void updateRoadSpritesInArea( Maps::Map_Format::MapFormat & map, const int32_t centerTileIndex, const int32_t centerToRectBorderDistance,
                                  const bool updateNonRoadTilesFromEdgesToCenter )
    {
        // We should update road sprites step by step starting from the tiles close connected to the center tile.
        const int32_t centerX = centerTileIndex % map.size;
        const int32_t centerY = centerTileIndex / map.size;

        // We avoid getting out of map boundaries.
        const int32_t minTileX = std::max( centerX - centerToRectBorderDistance, 0 );
        const int32_t minTileY = std::max( centerY - centerToRectBorderDistance, 0 );
        const int32_t maxTileX = std::min( centerX + centerToRectBorderDistance + 1, map.size );
        const int32_t maxTileY = std::min( centerY + centerToRectBorderDistance + 1, map.size );

        const int32_t distanceMax = centerToRectBorderDistance * 2 + 1;

        for ( int32_t distance = 1; distance < distanceMax; ++distance ) {
            const int32_t correctedDistance = updateNonRoadTilesFromEdgesToCenter ? distanceMax - distance : distance;

            for ( int32_t tileY = minTileY; tileY < maxTileY; ++tileY ) {
                const int32_t indexOffsetY = tileY * map.size;
                const int32_t distanceY = std::abs( tileY - centerY );

                for ( int32_t tileX = minTileX; tileX < maxTileX; ++tileX ) {
                    if ( std::abs( tileX - centerX ) + distanceY != correctedDistance ) {
                        continue;
                    }

                    const auto & tile = map.tiles[indexOffsetY + tileX];
                    if ( updateNonRoadTilesFromEdgesToCenter && Maps::doesContainRoads( tile ) ) {
                        continue;
                    }

                    Maps::updateRoadSpriteOnTile( map, indexOffsetY + tileX, false );
                }
            }
        }
    }

    void updateRoadSpritesAround( Maps::Map_Format::MapFormat & map, const int32_t centerTileIndex )
    {
        updateRoadSpritesInArea( map, centerTileIndex, 2, false );
        // To properly update the around sprites we call the update function the second time
        // for tiles not marked as road in reverse order and for 1 tile more distance from the center.
        updateRoadSpritesInArea( map, centerTileIndex, 3, true );
    }

    void setTerrain( Maps::Map_Format::TileInfo & tile, const uint16_t terrainImageIndex, const bool horizontalFlip, const bool verticalFlip, const int32_t tileIndex )
    {
        tile.terrainFlag = ( verticalFlip ? 1 : 0 ) + ( horizontalFlip ? 2 : 0 );

        const int newGround = Maps::Ground::getGroundByImageIndex( terrainImageIndex );

        if ( ( Maps::doesContainRoads( tile ) || doesContainStreams( tile ) ) && ( newGround != Maps::Ground::WATER )
             && Maps::Ground::doesTerrainImageIndexContainEmbeddedObjects( terrainImageIndex ) ) {
            // There cannot be extra objects under the roads and streams.
            tile.terrainIndex = Maps::Ground::getRandomTerrainImageIndex( Maps::Ground::getGroundByImageIndex( terrainImageIndex ), false );
        }
        else {
            tile.terrainIndex = terrainImageIndex;
        }

        if ( tileIndex >= 0 && tileIndex < world.w() * world.h() ) {
            world.getTile( tileIndex ).setTerrain( tile.terrainIndex, tile.terrainFlag );
        }
    }
}

namespace Maps
{
    bool readMapInEditor( const Map_Format::MapFormat & map )
    {
        world.generateForEditor( map.size );

        if ( !readAllTiles( map ) ) {
            return false;
        }

        world.updatePassabilities();

        updateWorldCastlesHeroes( map );

        return true;
    }

    bool readAllTiles( const Map_Format::MapFormat & map )
    {
        assert( map.size == world.w() && map.size == world.h() );

        // We must clear all tiles before writing something on them.
        for ( size_t i = 0; i < map.tiles.size(); ++i ) {
            auto & tile = world.getTile( static_cast<int32_t>( i ) );
            tile = {};

            tile.setIndex( static_cast<int32_t>( i ) );
        }

        for ( size_t i = 0; i < map.tiles.size(); ++i ) {
            auto & worldTile = world.getTile( static_cast<int32_t>( i ) );
            worldTile.setTerrain( map.tiles[i].terrainIndex, map.tiles[i].terrainFlag );
        }

        // Read objects from all tiles and place them based on their IDs.
        auto sortObjects = []( const IndexedObjectInfo & left, const IndexedObjectInfo & right ) { return left.info->id < right.info->id; };
        std::multiset<IndexedObjectInfo, decltype( sortObjects )> sortedObjects( sortObjects );

        for ( size_t i = 0; i < map.tiles.size(); ++i ) {
            for ( const auto & object : map.tiles[i].objects ) {
                IndexedObjectInfo info;
                info.tileIndex = static_cast<int32_t>( i );
                info.info = &object;
                sortedObjects.emplace( info );
            }
        }

        for ( const auto & info : sortedObjects ) {
            assert( info.info != nullptr );
            if ( !readTileObject( world.getTile( info.tileIndex ), *info.info ) ) {
                return false;
            }
        }

        return true;
    }

    bool saveMapInEditor( Map_Format::MapFormat & map )
    {
        assert( world.w() > 0 && world.w() == world.h() );

        map.size = world.w();

        const size_t size = static_cast<size_t>( map.size ) * map.size;

        map.tiles.resize( size );

        for ( size_t i = 0; i < size; ++i ) {
            writeTile( world.getTile( static_cast<int32_t>( i ) ), map.tiles[i] );
        }

        return true;
    }

    bool readTileObject( Tile & tile, const Map_Format::TileObjectInfo & object )
    {
        const auto & objectInfos = getObjectsByGroup( object.group );
        if ( object.index >= objectInfos.size() ) {
            // This is a bad map format!
            assert( 0 );
            return false;
        }

        // Object UID is set through global object UID counter. Therefore, we need to update it before running the operation.
        if ( object.id == 0 ) {
            // This object UID is not set!
            assert( 0 );
            return false;
        }

        setLastObjectUID( object.id - 1 );
        // We don't update map passabilities as it is a very expensive process.
        // Let's do it once everything is being loaded.
        return setObjectOnTile( tile, objectInfos[object.index], false );
    }

    void writeTile( const Tile & tile, Map_Format::TileInfo & info )
    {
        info.terrainIndex = tile.getTerrainImageIndex();
        info.terrainFlag = tile.getTerrainFlags();
    }

    void addObjectToMap( Map_Format::MapFormat & map, const int32_t tileId, const ObjectGroup group, const uint32_t index )
    {
        assert( tileId >= 0 && map.tiles.size() > static_cast<size_t>( tileId ) );

        // At this time it is assumed that object was added into world object to be rendered using Maps::setObjectOnTile() function.
        const uint32_t uid = getLastObjectUID();
        assert( uid > 0 );

        addObjectToTile( map.tiles[tileId], group, index, uid );

        // Towns and heroes have extra metadata.
        if ( group == ObjectGroup::KINGDOM_HEROES ) {
            auto [heroMetadata, isMetadataEmplaced] = map.heroMetadata.try_emplace( uid );
            assert( isMetadataEmplaced );

            const auto & objects = getObjectsByGroup( group );
            assert( index < objects.size() );
            // Set race according the object metadata.
            heroMetadata->second.race = Race::IndexToRace( static_cast<int>( objects[index].metadata[1] ) );
        }
        else if ( group == ObjectGroup::KINGDOM_TOWNS ) {
            auto [metadata, isMetadataEmplaced] = map.castleMetadata.try_emplace( uid );
            assert( isMetadataEmplaced );

            const auto & objects = getObjectsByGroup( group );
            assert( index < objects.size() );
            // Add town or castle main buildings.
            metadata->second.builtBuildings.push_back( objects[index].metadata[1] == 0 ? BUILD_TENT : BUILD_CASTLE );
        }
        else if ( isJailObject( group, index ) ) {
            auto [heroMetadata, isMetadataEmplaced] = map.heroMetadata.try_emplace( uid );
            assert( isMetadataEmplaced );

            // Set Random race for the jailed hero by default.
            heroMetadata->second.race = Race::RAND;
        }
        else if ( group == ObjectGroup::MONSTERS ) {
            const auto [dummy, isMetadataEmplaced] = map.standardMetadata.try_emplace( uid );
            assert( isMetadataEmplaced );

#ifdef NDEBUG
            (void)isMetadataEmplaced;
#endif
        }
        else if ( group == ObjectGroup::ADVENTURE_MISCELLANEOUS ) {
            const auto & objects = getObjectsByGroup( group );

            assert( index < objects.size() );
            const auto objectType = objects[index].objectType;

            switch ( objectType ) {
            case MP2::OBJ_EVENT: {
                const auto [dummy, isMetadataEmplaced] = map.adventureMapEventMetadata.try_emplace( uid );
                assert( isMetadataEmplaced );

#ifdef NDEBUG
                (void)isMetadataEmplaced;
#endif
                break;
            }
            case MP2::OBJ_SIGN: {
                const auto [dummy, isMetadataEmplaced] = map.signMetadata.try_emplace( uid );
                assert( isMetadataEmplaced );

#ifdef NDEBUG
                (void)isMetadataEmplaced;
#endif
                break;
            }
            case MP2::OBJ_SPHINX: {
                const auto [dummy, isMetadataEmplaced] = map.sphinxMetadata.try_emplace( uid );
                assert( isMetadataEmplaced );

#ifdef NDEBUG
                (void)isMetadataEmplaced;
#endif
                break;
            }
            default:
                break;
            }
        }
        else if ( group == ObjectGroup::ADVENTURE_WATER ) {
            const auto & objects = getObjectsByGroup( group );

            assert( index < objects.size() );
            const auto objectType = objects[index].objectType;
            if ( objectType == MP2::OBJ_BOTTLE ) {
                const auto [dummy, isMetadataEmplaced] = map.signMetadata.try_emplace( uid );
                assert( isMetadataEmplaced );

#ifdef NDEBUG
                (void)isMetadataEmplaced;
#endif
            }
        }
        else if ( group == ObjectGroup::ADVENTURE_ARTIFACTS ) {
            assert( index < getObjectsByGroup( group ).size() );

            const auto [dummy, isMetadataEmplaced] = map.standardMetadata.try_emplace( uid );
            assert( isMetadataEmplaced );

#ifdef NDEBUG
            (void)isMetadataEmplaced;
#endif
        }
    }

    bool addStream( Map_Format::MapFormat & map, const int32_t tileId )
    {
        assert( tileId >= 0 && map.tiles.size() > static_cast<size_t>( tileId ) );

        Map_Format::TileInfo & thisTile = map.tiles[tileId];

        if ( isStreamPresent( thisTile ) || Ground::getGroundByImageIndex( thisTile.terrainIndex ) == Ground::WATER ) {
            // We cannot place streams on the water or on already placed streams.
            return false;
        }

        // Force set stream on this tile and update its sprite.
        updateStreamObjectOnMapTile( map, tileId, true );

        updateStreamsAround( map, tileId );

        if ( Ground::doesTerrainImageIndexContainEmbeddedObjects( thisTile.terrainIndex ) ) {
            // We need to set terrain image without extra objects under the stream.
            thisTile.terrainIndex = Ground::getRandomTerrainImageIndex( Ground::getGroundByImageIndex( thisTile.terrainIndex ), false );
        }

        return true;
    }

    void updateStreamsAround( Map_Format::MapFormat & map, const int32_t centerTileId )
    {
        // For streams we should update only the next four directions.
        for ( const int direction : { Direction::LEFT, Direction::TOP, Direction::RIGHT, Direction::BOTTOM } ) {
            if ( isValidDirection( centerTileId, direction ) ) {
                updateStreamObjectOnMapTile( map, GetDirectionIndex( centerTileId, direction ), false );
            }
        }
    }

    void updateStreamsToDeltaConnection( Map_Format::MapFormat & map, const int32_t tileId, const int deltaDirection )
    {
        if ( !isValidDirection( tileId, deltaDirection ) ) {
            return;
        }

        const int32_t nextTileId = GetDirectionIndex( tileId, deltaDirection );

        if ( !isValidDirection( nextTileId, deltaDirection ) ) {
            return;
        }

        updateStreamObjectOnMapTile( map, GetDirectionIndex( nextTileId, deltaDirection ), false );
    }

    int getRiverDeltaDirectionByIndex( const ObjectGroup group, const int32_t objectIndex )
    {
        if ( group != ObjectGroup::LANDSCAPE_MISCELLANEOUS ) {
            return Direction::UNKNOWN;
        }

        const auto & objectInfo = getObjectInfo( ObjectGroup::LANDSCAPE_MISCELLANEOUS, objectIndex );

        assert( !objectInfo.groundLevelParts.empty() );

        const auto & firstObjectPart = objectInfo.groundLevelParts.front();

        // Yes, the below code is very hacky but so far this is the best we can do.
        if ( firstObjectPart.icnType == MP2::OBJ_ICN_TYPE_OBJNMUL2 ) {
            switch ( firstObjectPart.icnIndex ) {
            case 2U:
                return Direction::TOP;
            case 11U:
                return Direction::BOTTOM;
            case 218U + 2U:
                return Direction::LEFT;
            case 218U + 11U:
                return Direction::RIGHT;
            default:
                break;
            }
        }

        return Direction::UNKNOWN;
    }

    bool isRiverDeltaObject( const ObjectGroup group, const int32_t objectIndex )
    {
        return getRiverDeltaDirectionByIndex( group, objectIndex ) != Direction::UNKNOWN;
    }

    bool updateMapPlayers( Map_Format::MapFormat & map )
    {
        static_assert( Color::BLUE == 1 << 0, "The kingdom color values have changed. You are going to break map format!" );
        static_assert( Color::GREEN == 1 << 1, "The kingdom color values have changed. You are going to break map format!" );
        static_assert( Color::RED == 1 << 2, "The kingdom color values have changed. You are going to break map format!" );
        static_assert( Color::YELLOW == 1 << 3, "The kingdom color values have changed. You are going to break map format!" );
        static_assert( Color::ORANGE == 1 << 4, "The kingdom color values have changed. You are going to break map format!" );
        static_assert( Color::PURPLE == 1 << 5, "The kingdom color values have changed. You are going to break map format!" );

        static_assert( Race::NONE == 0, "The race values have changed. You are going to break map format!" );
        static_assert( Race::KNGT == 1 << 0, "The race values have changed. You are going to break map format!" );
        static_assert( Race::BARB == 1 << 1, "The race values have changed. You are going to break map format!" );
        static_assert( Race::SORC == 1 << 2, "The race values have changed. You are going to break map format!" );
        static_assert( Race::WRLK == 1 << 3, "The race values have changed. You are going to break map format!" );
        static_assert( Race::WZRD == 1 << 4, "The race values have changed. You are going to break map format!" );
        static_assert( Race::NECR == 1 << 5, "The race values have changed. You are going to break map format!" );
        static_assert( Race::MULT == 1 << 6, "The race values have changed. You are going to break map format!" );
        static_assert( Race::RAND == 1 << 7, "The race values have changed. You are going to break map format!" );

        constexpr size_t mainColors{ maxNumOfPlayers };

        if ( map.playerRace.size() != mainColors ) {
            // Possibly corrupted map.
            assert( 0 );
            return false;
        }

        // Gather all information about all kingdom colors and races.
        std::array<bool, mainColors> heroColorsPresent{ false };
        // Towns can be neutral so 1 more color for them.
        std::array<bool, mainColors + 1> townColorsPresent{ false };
        std::array<uint8_t, mainColors> heroRacesPresent{ 0 };
        std::array<uint8_t, mainColors + 1> townRacesPresent{ 0 };

        const auto & heroObjects = getObjectsByGroup( ObjectGroup::KINGDOM_HEROES );
        const auto & townObjects = getObjectsByGroup( ObjectGroup::KINGDOM_TOWNS );

        for ( size_t tileIndex = 0; tileIndex < map.tiles.size(); ++tileIndex ) {
            const auto & mapTile = map.tiles[tileIndex];

            for ( const auto & object : mapTile.objects ) {
                if ( object.group == ObjectGroup::KINGDOM_HEROES ) {
                    if ( object.index >= heroObjects.size() ) {
                        assert( 0 );
                        return false;
                    }

                    const auto & metadata = heroObjects[object.index].metadata;

                    const uint32_t color = metadata[0];
                    if ( color >= heroColorsPresent.size() ) {
                        assert( 0 );
                        return false;
                    }

                    heroColorsPresent[color] = true;

                    const uint32_t race = metadata[1];
                    heroRacesPresent[color] |= ( 1 << race );
                }
                else if ( object.group == ObjectGroup::KINGDOM_TOWNS ) {
                    if ( object.index >= townObjects.size() ) {
                        assert( 0 );
                        return false;
                    }

                    // Towns and Castles have 2 flags on both sides on the entrance. Verify, that they exist and have the same color.
                    assert( tileIndex > 0 && tileIndex < map.tiles.size() - 1 );

                    const uint8_t color = getTownColorIndex( map, tileIndex, object.id );

                    if ( color >= townColorsPresent.size() ) {
                        assert( 0 );
                        return false;
                    }

                    townColorsPresent[color] = true;

                    const uint32_t race = townObjects[object.index].metadata[0];
                    townRacesPresent[color] |= ( 1 << race );
                }
            }
        }

        // Update map format settings based on the gathered information.
        map.availablePlayerColors = 0;
        for ( size_t i = 0; i < mainColors; ++i ) {
            map.playerRace[i] = ( heroRacesPresent[i] | townRacesPresent[i] );

            if ( map.playerRace[i] != 0 ) {
                map.availablePlayerColors += static_cast<uint8_t>( 1 << i );
            }

            // Only one race can be present.
            if ( ( map.playerRace[i] & Race::RAND ) != 0 ) {
                map.playerRace[i] = Race::RAND;
            }
            if ( ( map.playerRace[i] & Race::MULT ) != 0 ) {
                map.playerRace[i] = Race::MULT;
            }
            else {
                size_t raceCount = 0;
                for ( uint8_t raceIdx = 0; raceIdx < 6; ++raceIdx ) {
                    if ( ( map.playerRace[i] & ( 1 << raceIdx ) ) != 0 ) {
                        ++raceCount;
                    }
                }

                if ( raceCount > 1 ) {
                    map.playerRace[i] = Race::MULT;
                }
            }
        }

        map.computerPlayerColors = map.computerPlayerColors & map.availablePlayerColors;
        map.humanPlayerColors = map.humanPlayerColors & map.availablePlayerColors;

        if ( map.availablePlayerColors > 0 ) {
            // Human and computer player colors might be set previously. If they are, do not update them.
            for ( size_t i = 0; i < mainColors; ++i ) {
                const uint8_t color = static_cast<uint8_t>( 1 << i );
                if ( ( map.availablePlayerColors & color ) != 0 && ( map.computerPlayerColors & color ) == 0 && ( map.humanPlayerColors & color ) == 0 ) {
                    // This color was not set for anyone.
                    map.computerPlayerColors |= color;
                    map.humanPlayerColors |= color;
                }
            }

            // Make sure that at least one human color exist.
            if ( map.humanPlayerColors == 0 ) {
                for ( size_t i = 0; i < mainColors; ++i ) {
                    const uint8_t color = static_cast<uint8_t>( 1 << i );
                    if ( ( map.availablePlayerColors & color ) != 0 ) {
                        map.humanPlayerColors |= color;
                        break;
                    }
                }
            }

            if ( !map.alliances.empty() ) {
                // Verify that alliances are set correctly:
                // - each alliance has at least one color
                // - no color should be repeated more than once
                std::array<bool, mainColors> usedAllianceColors{ false };

                for ( auto iter = map.alliances.begin(); iter != map.alliances.end(); ) {
                    uint8_t & allianceColors = *iter;

                    // Only available players should be in the alliances.
                    allianceColors &= map.availablePlayerColors;

                    for ( size_t i = 0; i < mainColors; ++i ) {
                        const uint8_t color = static_cast<uint8_t>( 1 << i );
                        if ( ( allianceColors & color ) != 0 ) {
                            if ( usedAllianceColors[i] ) {
                                // This color is used in another alliance. Remove it from here.
                                allianceColors = allianceColors & ( ~color );
                            }
                            else {
                                usedAllianceColors[i] = true;
                            }
                        }
                    }

                    if ( allianceColors == 0 ) {
                        // This alliance is invalid.
                        iter = map.alliances.erase( iter );
                        continue;
                    }

                    ++iter;
                }

                if ( map.alliances.size() == 1 ) {
                    // Everyone cannot be in one alliance!
                    map.alliances.clear();
                }
                else {
                    // Check that all colors being used in alliances. If not then add missing colors to the last alliance.
                    for ( size_t i = 0; i < mainColors; ++i ) {
                        const uint8_t color = static_cast<uint8_t>( 1 << i );
                        if ( ( map.availablePlayerColors & color ) != 0 && !usedAllianceColors[i] ) {
                            if ( map.alliances.empty() ) {
                                map.alliances.push_back( color );
                            }
                            else {
                                map.alliances.back() |= color;
                            }
                        }
                    }
                }
            }
        }
        else {
            // No colors are set so no alliances should exist.
            map.alliances = { 0 };

            // No races are set.
            map.playerRace = { 0 };
        }

        // Update events according to the possible changes in human and/or AI player colors.
        for ( auto & [dummy, eventMetadata] : map.adventureMapEventMetadata ) {
            eventMetadata.humanPlayerColors = eventMetadata.humanPlayerColors & map.humanPlayerColors;
            eventMetadata.computerPlayerColors = eventMetadata.computerPlayerColors & map.computerPlayerColors;
        }

        // Check and update the special victory and loss conditions that depend on player objects.

        // Returns true if all is OK.
        auto checkSpecialCondition = [&map, &heroObjects, &townObjects]( const std::vector<uint32_t> & conditionMetadata, const ObjectGroup objectGroup ) {
            if ( conditionMetadata.size() != 2 ) {
                return false;
            }

            // Verify that this is a valid map object.
            const uint32_t tileIndex = conditionMetadata[0];

            assert( tileIndex < map.tiles.size() );

            for ( const auto & object : map.tiles[tileIndex].objects ) {
                if ( object.group != objectGroup ) {
                    continue;
                }

                switch ( objectGroup ) {
                case ObjectGroup::KINGDOM_TOWNS: {
                    if ( object.index >= townObjects.size() ) {
                        assert( 0 );
                        continue;
                    }

                    const uint32_t color = Color::IndexToColor( getTownColorIndex( map, tileIndex, object.id ) );
                    if ( color != conditionMetadata[1] ) {
                        // Current town color is incorrect.
                        continue;
                    }

                    return true;
                }
                case ObjectGroup::KINGDOM_HEROES: {
                    if ( object.index >= heroObjects.size() ) {
                        assert( 0 );
                        continue;
                    }

                    const uint32_t color = 1 << heroObjects[object.index].metadata[0];
                    if ( color != conditionMetadata[1] ) {
                        // Current hero color is incorrect.
                        continue;
                    }

                    return true;
                }
                default:
                    // Have you added a new object type for victory or loss conditions? Update the logic!
                    assert( 0 );
                    break;
                }
            }

            return false;
        };

        switch ( map.victoryConditionType ) {
        case FileInfo::VICTORY_CAPTURE_TOWN:
            if ( !checkSpecialCondition( map.victoryConditionMetadata, ObjectGroup::KINGDOM_TOWNS ) ) {
                map.victoryConditionMetadata.clear();
                map.victoryConditionType = FileInfo::VICTORY_DEFEAT_EVERYONE;
            }

            break;
        case FileInfo::VICTORY_KILL_HERO:
            if ( !checkSpecialCondition( map.victoryConditionMetadata, ObjectGroup::KINGDOM_HEROES ) ) {
                map.victoryConditionMetadata.clear();
                map.victoryConditionType = FileInfo::VICTORY_DEFEAT_EVERYONE;
            }

            break;
        default:
            break;
        }

        switch ( map.lossConditionType ) {
        case FileInfo::LOSS_TOWN:
            if ( !checkSpecialCondition( map.lossConditionMetadata, ObjectGroup::KINGDOM_TOWNS ) ) {
                map.lossConditionMetadata.clear();
                map.lossConditionType = FileInfo::LOSS_EVERYTHING;
            }

            break;
        case FileInfo::LOSS_HERO:
            if ( !checkSpecialCondition( map.lossConditionMetadata, ObjectGroup::KINGDOM_HEROES ) ) {
                map.lossConditionMetadata.clear();
                map.lossConditionType = FileInfo::LOSS_EVERYTHING;
            }

            break;
        default:
            break;
        }

        return true;
    }

    uint8_t getTownColorIndex( const Map_Format::MapFormat & map, const size_t tileIndex, const uint32_t id )
    {
        if ( map.tiles.empty() ) {
            assert( 0 );
            return 0;
        }

        if ( tileIndex == 0 || tileIndex >= map.tiles.size() - 1 ) {
            assert( 0 );
            return 0;
        }

        const auto & flagObjects = getObjectsByGroup( ObjectGroup::LANDSCAPE_FLAGS );

        uint32_t leftFlagColor = 0;
        uint32_t rightFlagColor = 0;

        for ( const auto & tempObject : map.tiles[tileIndex - 1].objects ) {
            if ( tempObject.group == ObjectGroup::LANDSCAPE_FLAGS && tempObject.id == id ) {
                if ( tempObject.index >= flagObjects.size() ) {
                    assert( 0 );
                    return 0;
                }

                leftFlagColor = flagObjects[tempObject.index].metadata[0];
                break;
            }
        }

        for ( const auto & tempObject : map.tiles[tileIndex + 1].objects ) {
            if ( tempObject.group == ObjectGroup::LANDSCAPE_FLAGS && tempObject.id == id ) {
                if ( tempObject.index >= flagObjects.size() ) {
                    assert( 0 );
                    return 0;
                }

                rightFlagColor = flagObjects[tempObject.index].metadata[0];
                break;
            }
        }

        if ( leftFlagColor != rightFlagColor ) {
            assert( 0 );
            return 0;
        }

        return static_cast<uint8_t>( leftFlagColor );
    }

    bool isJailObject( const ObjectGroup group, const uint32_t index )
    {
        return ( group == ObjectGroup::ADVENTURE_MISCELLANEOUS && getObjectInfo( group, static_cast<int32_t>( index ) ).objectType == MP2::OBJ_JAIL );
    }

    uint32_t getBuildingsFromVector( const std::vector<uint32_t> & buildingsVector )
    {
        uint32_t buildings{ BUILD_NOTHING };
        for ( const uint32_t building : buildingsVector ) {
            buildings |= building;
        }

        return buildings;
    }

    void setDefaultCastleDefenderArmy( Map_Format::CastleMetadata & metadata )
    {
        for ( int32_t & type : metadata.defenderMonsterType ) {
            type = -1;
        }
        for ( int32_t & count : metadata.defenderMonsterCount ) {
            count = 0;
        }
    }

    bool isDefaultCastleDefenderArmy( const Map_Format::CastleMetadata & metadata )
    {
        return std::all_of( metadata.defenderMonsterType.begin(), metadata.defenderMonsterType.end(), []( const int32_t type ) { return type < 0; } );
    }

    bool loadCastleArmy( Army & army, const Map_Format::CastleMetadata & metadata )
    {
        if ( isDefaultCastleDefenderArmy( metadata ) ) {
            return false;
        }

        loadArmyFromMetadata( army, metadata.defenderMonsterType, metadata.defenderMonsterCount );
        return true;
    }

    void saveCastleArmy( const Army & army, Map_Format::CastleMetadata & metadata )
    {
        saveArmyToMetadata( army, metadata.defenderMonsterType, metadata.defenderMonsterCount );
    }

    bool loadHeroArmy( Army & army, const Map_Format::HeroMetadata & metadata )
    {
        if ( std::all_of( metadata.armyMonsterType.begin(), metadata.armyMonsterType.end(), []( const int32_t type ) { return type <= 0; } ) ) {
            // There is no custom army.
            return false;
        }

        loadArmyFromMetadata( army, metadata.armyMonsterType, metadata.armyMonsterCount );
        return true;
    }

    void saveHeroArmy( const Army & army, Map_Format::HeroMetadata & metadata )
    {
        saveArmyToMetadata( army, metadata.armyMonsterType, metadata.armyMonsterCount );
    }

    bool updateRoadOnTile( Map_Format::MapFormat & map, const int32_t tileIndex, const bool setRoad )
    {
        assert( static_cast<size_t>( tileIndex ) < map.tiles.size() );

        auto & tile = map.tiles[tileIndex];
        const int groundType = Ground::getGroundByImageIndex( tile.terrainIndex );
        if ( setRoad && groundType == Ground::WATER ) {
            // Roads are not allowed to set on water.
            return false;
        }

        if ( doesContainRoads( tile ) == setRoad ) {
            // Nothing to do here.
            return false;
        }

        if ( setRoad ) {
            // Force set road on this tile and update its sprite.
            updateRoadSpriteOnTile( map, tileIndex, true );

            if ( !doesContainRoads( tile ) ) {
                // The road was not set because there is no corresponding sprite for this place.
                return false;
            }

            updateRoadSpritesAround( map, tileIndex );

            if ( Ground::doesTerrainImageIndexContainEmbeddedObjects( tile.terrainIndex ) ) {
                // We need to set terrain image without extra objects under the road.
                setTerrain( tile, Ground::getRandomTerrainImageIndex( groundType, false ), false, false, tileIndex );
            }
        }
        else {
            removeRoads( tile, tileIndex );

            updateRoadSpritesAround( map, tileIndex );

            // After removing the road from the tile it may have road sprites for the nearby tiles with road.
            updateRoadSpriteOnTile( map, tileIndex, false );
        }

        return true;
    }

    void updateRoadSpriteOnTile( Map_Format::MapFormat & map, const int32_t tileIndex, const bool forceRoadOnTile )
    {
        auto & tile = map.tiles[tileIndex];

        const uint8_t imageIndex
            = getRoadImageForTile( map, tileIndex, getRoadDirecton( map, tileIndex ) | ( forceRoadOnTile ? Direction::CENTER : Direction::UNKNOWN ) );
        if ( imageIndex == 255U ) {
            // After the check this tile should not contain a road sprite.
            if ( !forceRoadOnTile && !doesContainRoads( tile ) ) {
                // We remove any existing road sprite if this tile does not contain (or was not forced to contain) the main road sprite.
                removeRoads( tile, tileIndex );
            }

            return;
        }

        auto roadObjectIter = std::find_if( tile.objects.begin(), tile.objects.end(), []( const auto & object ) { return object.group == ObjectGroup::ROADS; } );
        if ( roadObjectIter != tile.objects.end() ) {
            // Since the tile has a road object, update it.
            roadObjectIter->index = imageIndex;

            Maps::Tile & worldTile = world.getTile( tileIndex );
            Maps::Tile::updateTileObjectIcnIndex( worldTile, worldTile.getObjectIdByObjectIcnType( MP2::OBJ_ICN_TYPE_ROAD ), imageIndex );
        }
        else {
            // This tile has no roads. Add one.
            Map_Format::TileObjectInfo info;
            info.id = getNewObjectUID();
            info.group = ObjectGroup::ROADS;
            info.index = imageIndex;

            readTileObject( world.getTile( tileIndex ), info );

            tile.objects.emplace_back( std::move( info ) );
        }
    }

    bool doesContainRoads( const Map_Format::TileInfo & tile )
    {
        for ( const auto & object : tile.objects ) {
            if ( object.group == ObjectGroup::ROADS ) {
                // NOTICE: only the next original road sprites are considered as a road for hero. The others are extra road edges.
                static const std::set<uint32_t> allowedIndecies{ 0, 2, 3, 4, 5, 6, 7, 9, 12, 13, 14, 16, 17, 18, 19, 20, 21, 26, 28, 29, 30, 31 };
                return ( allowedIndecies.count( object.index ) == 1 );
            }
        }

        return false;
    }
}
