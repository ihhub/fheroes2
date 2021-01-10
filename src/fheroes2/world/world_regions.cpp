/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "ground.h"
#include "world.h"

namespace
{
    // Aliases to make data structures easier to work with
    // TileData.first is index, TileData.second is payload
    using TileData = std::pair<int, int>;
    using TileDataVector = std::vector<std::pair<int, int> >;

    // Values in Direction namespace can't be used as index, use a custom value here
    // Converted into bitfield later
    enum
    {
        TOP_LEFT = 0,
        TOP = 1,
        TOP_RIGHT = 2,
        RIGHT = 3,
        BOTTOM_RIGHT = 4,
        BOTTOM = 5,
        BOTTOM_LEFT = 6,
        LEFT = 7
    };

    uint16_t GetDirectionBitmask( uint8_t direction, bool reflect = false )
    {
        return 1 << ( reflect ? ( direction + 4 ) % 8 : direction );
    }

    std::vector<int> GetDirectionOffsets( const int width )
    {
        std::vector<int> offsets( 8 );
        offsets[TOP_LEFT] = -width - 1;
        offsets[TOP] = -width;
        offsets[TOP_RIGHT] = -width + 1;
        offsets[RIGHT] = 1;
        offsets[BOTTOM_RIGHT] = width + 1;
        offsets[BOTTOM] = width;
        offsets[BOTTOM_LEFT] = width - 1;
        offsets[LEFT] = -1;
        return offsets;
    }

    int ConvertExtendedIndex( int index, uint32_t width )
    {
        const uint32_t originalWidth = width - 2;
        return ( index / originalWidth + 1 ) * width + ( index % originalWidth ) + 1;
    }

    bool AppendIfFarEnough( std::vector<int> & dataSet, int value, uint32_t distance )
    {
        for ( const int current : dataSet ) {
            if ( Maps::GetApproximateDistance( current, value ) < distance )
                return false;
        }

        dataSet.push_back( value );

        return true;
    }

    void CheckAdjacentTiles( std::vector<MapRegionNode> & rawData, MapRegion & region, uint32_t rawDataWidth, const std::vector<int> & offsets )
    {
        const int nodeIndex = region._nodes[region._lastProcessedNode].index;

        for ( uint8_t direction = 0; direction < 8; ++direction ) {
            const int newIndex = ConvertExtendedIndex( nodeIndex, rawDataWidth ) + offsets[direction];
            MapRegionNode & newTile = rawData[newIndex];
            if ( newTile.passable & GetDirectionBitmask( direction, true ) ) {
                if ( newTile.type == REGION_NODE_OPEN && newTile.isWater == region._isWater ) {
                    newTile.type = region._id;
                    region._nodes.push_back( newTile );
                }
                else if ( newTile.type > REGION_NODE_FOUND && newTile.type != region._id ) {
                    region._neighbours.insert( newTile.type );
                }
            }
        }
    }

    void RegionExpansion( std::vector<MapRegionNode> & rawData, uint32_t rawDataWidth, MapRegion & region, const std::vector<int> & offsets )
    {
        // Process only "open" nodes that exist at the start of the loop and ignore what's added
        const size_t nodesEnd = region._nodes.size();

        while ( region._lastProcessedNode < nodesEnd ) {
            CheckAdjacentTiles( rawData, region, rawDataWidth, offsets );
            ++region._lastProcessedNode;
        }
    }

    void FindMissingRegions( std::vector<MapRegionNode> & rawData, const Size & mapSize, std::vector<MapRegion> & regions )
    {
        const uint32_t extendedWidth = mapSize.w + 2;

        MapRegionNode * currentTile = rawData.data() + extendedWidth + 1;
        MapRegionNode * mapEnd = rawData.data() + extendedWidth * ( mapSize.h + 1 );
        const std::vector<int> & offsets = GetDirectionOffsets( static_cast<int>( extendedWidth ) );

        for ( ; currentTile != mapEnd; ++currentTile ) {
            if ( currentTile->type == REGION_NODE_OPEN ) {
                regions.emplace_back( static_cast<int>( regions.size() ), currentTile->index, currentTile->isWater, extendedWidth );

                MapRegion & region = regions.back();
                do {
                    CheckAdjacentTiles( rawData, region, extendedWidth, offsets );
                    ++region._lastProcessedNode;
                } while ( region._lastProcessedNode != region._nodes.size() );
            }
        }
    }
}

MapRegion::MapRegion( int regionIndex, int mapIndex, bool water, size_t expectedSize )
    : _id( regionIndex )
    , _isWater( water )
{
    _nodes.reserve( expectedSize );
    _nodes.emplace_back( mapIndex );
    _nodes[0].type = regionIndex;
}

std::vector<int> MapRegion::getNeighbours() const
{
    return std::vector<int>( _neighbours.begin(), _neighbours.end() );
}

size_t MapRegion::getNeighboursCount() const
{
    return _neighbours.size();
}

std::vector<IndexObject> MapRegion::getObjectList() const
{
    std::vector<IndexObject> result;

    for ( const MapRegionNode & node : _nodes ) {
        if ( node.mapObject != 0 ) {
            result.emplace_back( node.index, node.mapObject );
        }
    }
    return result;
}

int MapRegion::getObjectCount() const
{
    int result = 0;
    for ( const MapRegionNode & node : _nodes ) {
        if ( node.mapObject != 0 )
            ++result;
    }
    return result;
}

double MapRegion::getFogRatio( int color ) const
{
    size_t fogCount = 0;
    for ( const MapRegionNode & node : _nodes ) {
        if ( world.GetTiles( node.index ).isFog( color ) )
            ++fogCount;
    }
    return static_cast<double>( fogCount ) / _nodes.size();
}

size_t World::getRegionCount() const
{
    return _regions.size();
}

const MapRegion & World::getRegion( size_t id ) const
{
    if ( id < _regions.size() )
        return _regions[id];

    // empty MapRegion object is still valid to use
    static const MapRegion region;
    return region;
}

void World::ComputeStaticAnalysis()
{
    // Parameters that control region generation: size and spacing between initial points
    const uint32_t castleRegionSize = 17;
    const uint32_t extraRegionSize = 18;
    const uint32_t emptyLineFrequency = 7;

    // Step 1. Split map into terrain, water and ground points
    // Initialize the obstacles vector
    const int width = w();
    const int height = h();
    TileDataVector obstacles[4];

    obstacles[0].reserve( width );
    obstacles[2].reserve( width );
    for ( int x = 0; x < width; ++x ) {
        obstacles[0].emplace_back( x, 0 ); // water, columns
        obstacles[2].emplace_back( x, 0 ); // ground, columns
    }
    obstacles[1].reserve( height );
    obstacles[3].reserve( height );
    for ( int y = 0; y < height; ++y ) {
        obstacles[1].emplace_back( y, 0 ); // water, rows
        obstacles[3].emplace_back( y, 0 ); // ground, rows
    }

    // Find the terrain
    for ( int y = 0; y < height; ++y ) {
        const int rowIndex = y * width;
        for ( int x = 0; x < width; ++x ) {
            const int index = rowIndex + x;
            Maps::Tiles & tile = vec_tiles[index];
            // If tile is blocked (mountain, trees, etc) then it's applied to both
            if ( tile.GetPassable() == 0 ) {
                ++obstacles[0][x].second;
                ++obstacles[1][y].second;
                ++obstacles[2][x].second;
                ++obstacles[3][y].second;
            }
            else if ( tile.isWater() ) {
                // if it's water then ground tiles consider it an obstacle
                ++obstacles[2][x].second;
                ++obstacles[3][y].second;
            }
            else {
                // else then ground is an obstacle for water navigation
                ++obstacles[0][x].second;
                ++obstacles[1][y].second;
            }
        }
    }

    // sort the map rows and columns based on amount of obstacles
    for ( int i = 0; i < 4; ++i )
        std::sort( obstacles[i].begin(), obstacles[i].end(), []( const TileData & left, const TileData & right ) { return left.second < right.second; } );

    // Step 2. Find lines (rows and columns) with most amount of usable space, use it for region centers later
    const int maxDimension = std::max( width, height );
    std::vector<int> emptyLines[4]; // 0,1 is water; 2,3 is ground
    for ( int i = 0; i < 4; ++i ) {
        emptyLines[i].reserve( maxDimension / emptyLineFrequency );
        for ( const TileData & line : obstacles[i] ) {
            AppendIfFarEnough( emptyLines[i], line.first, emptyLineFrequency );
        }
    }

    // Step 3. Check all castles on the map and create region centres based on them
    std::vector<int> regionCenters;
    TileDataVector castleCenters;
    for ( Castle * castle : vec_castles ) {
        castleCenters.emplace_back( castle->GetIndex(), castle->GetColor() );
    }
    std::sort( castleCenters.begin(), castleCenters.end(), []( const TileData & left, const TileData & right ) {
        // Sort castles by color primarily (NONE is last)
        // If same color then compare map index
        if ( left.second == right.second )
            return left.first < right.first;
        return left.second > right.second;
    } );

    const size_t totalMapTiles = vec_tiles.size();
    for ( const TileData & castleTile : castleCenters ) {
        // Check if a lot of players next to each other? (Slugfest map)
        // GetCastle( Point( val % width, val / width ) )->GetColor();
        const int castleIndex = castleTile.first + width;
        AppendIfFarEnough( regionCenters, ( castleIndex >= 0 && static_cast<size_t>( castleIndex ) > totalMapTiles ) ? castleTile.first : castleIndex, castleRegionSize );
    }

    // Step 4. Add missing region centres based on distance (for water or if there's big chunks of space without castles)
    const std::vector<int> & directionOffsets = GetDirectionOffsets( width );
    for ( int waterOrGround = 0; waterOrGround < 4; waterOrGround += 2 ) {
        for ( const int rowID : emptyLines[waterOrGround] ) {
            const int rowIndex = rowID * width;
            for ( const int colID : emptyLines[waterOrGround + 1] ) {
                int centerIndex = -1;

                const int tileIndex = rowIndex + colID;
                const Maps::Tiles & tile = vec_tiles[tileIndex];
                if ( tile.GetPassable() && tile.isWater() ) {
                    centerIndex = tileIndex;
                }
                else {
                    for ( uint8_t direction = 0; direction < 8; ++direction ) {
                        const int newIndex = tileIndex + directionOffsets[direction];
                        if ( newIndex >= 0 && static_cast<size_t>( newIndex ) < totalMapTiles ) {
                            const Maps::Tiles & newTile = vec_tiles[newIndex];
                            if ( newTile.GetPassable() && tile.isWater() == static_cast<bool>( waterOrGround ) ) {
                                centerIndex = newIndex;
                                break;
                            }
                        }
                    }
                }

                if ( centerIndex >= 0 ) {
                    AppendIfFarEnough( regionCenters, centerIndex, extraRegionSize );
                }
            }
        }
    }

    // Step 5. Initialize extended (by 2 tiles) map data used for region growing based on actual Maps::Tiles
    const uint32_t extendedWidth = width + 2;
    std::vector<MapRegionNode> data( extendedWidth * ( height + 2 ) );
    for ( int y = 0; y < height; ++y ) {
        const int rowIndex = y * width;
        for ( int x = 0; x < width; ++x ) {
            const int index = rowIndex + x;
            const Maps::Tiles & tile = vec_tiles[index];
            MapRegionNode & node = data[ConvertExtendedIndex( index, extendedWidth )];

            node.index = index;
            node.passable = tile.GetPassable();
            node.isWater = tile.isWater();

            const int object = tile.GetObject();
            node.mapObject = MP2::isActionObject( object, node.isWater ) ? object : 0;
            if ( node.passable != 0 ) {
                node.type = REGION_NODE_OPEN;
            }
        }
    }

    // Step 6. Initialize regions
    size_t averageRegionSize = ( static_cast<size_t>( width ) * height * 2 ) / regionCenters.size();
    _regions.clear();
    for ( int baseIDX = 0; baseIDX < REGION_NODE_FOUND; ++baseIDX ) {
        _regions.emplace_back( baseIDX, 0, false, 0 );
    }

    for ( const int tileIndex : regionCenters ) {
        const int regionID = static_cast<int>( _regions.size() ); // Safe to do as we can't have so many regions
        _regions.emplace_back( regionID, tileIndex, vec_tiles[tileIndex].isWater(), averageRegionSize );
        data[ConvertExtendedIndex( tileIndex, extendedWidth )].type = regionID;
    }

    // Step 7. Grow all regions one step at the time so they would compete for space
    const std::vector<int> & offsets = GetDirectionOffsets( static_cast<int>( extendedWidth ) );
    bool stillRoomToExpand = true;
    while ( stillRoomToExpand ) {
        stillRoomToExpand = false;
        for ( size_t regionID = REGION_NODE_FOUND; regionID < regionCenters.size(); ++regionID ) {
            MapRegion & region = _regions[regionID];
            RegionExpansion( data, extendedWidth, region, offsets );
            if ( region._lastProcessedNode != region._nodes.size() )
                stillRoomToExpand = true;
        }
    }

    // Step 8. Fill missing data (if there's a small island/lake or unreachable terrain)
    FindMissingRegions( data, Size( width, height ), _regions );

    // Step 9. Assign regions to the map tiles and finalize the data
    for ( MapRegion & reg : _regions ) {
        if ( reg._id < REGION_NODE_FOUND )
            continue;

        for ( const MapRegionNode & node : reg._nodes ) {
            vec_tiles[node.index].UpdateRegion( node.type );

            // connect regions through teleporters
            if ( node.mapObject == MP2::OBJ_STONELITHS ) {
                const MapsIndexes & exits = GetTeleportEndPoints( node.index );
                for ( const int exitIndex : exits ) {
                    // neighbours is a set that will force the uniqness
                    reg._neighbours.insert( vec_tiles[exitIndex].GetRegion() );
                }
            }
        }
    }
}
