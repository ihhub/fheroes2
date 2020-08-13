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

#include <set>

#include "world.h"

namespace
{
    // Aliases to make data structures easier to work with
    // TileData.first is index, TileData.second is payload
    using TileData = std::pair<int, int>;
    using TileDataVector = std::vector<std::pair<int, int> >;

    enum
    {
        BLOCKED = 0,
        OPEN = 1,
        BORDER = 2,
        REGION = 3
    };

    void FillRegion( std::vector<uint8_t> & data, const Size & mapSize )
    {
        const uint32_t extendedWidth = mapSize.w + 2;

        uint8_t * currentTile = data.data() + extendedWidth + 1;
        uint8_t * mapEnd = data.data() + extendedWidth * ( mapSize.h + 1 );

        uint32_t regionID = 10;

        for ( ; currentTile != mapEnd; ++currentTile ) {
            if ( *currentTile == OPEN ) {
                std::vector<Point> regionTiles;
                std::vector<Point> edge;

                *currentTile = regionID;

                const size_t currentPosition = currentTile - data.data();
                regionTiles.push_back( Point( currentPosition % extendedWidth - 1, currentPosition / extendedWidth - 1 ) );

                size_t tileIdx = 0;
                do {
                    Point pt = regionTiles[tileIdx++];

                    uint8_t neighbourCount = 0;
                    uint8_t * position = data.data() + ( pt.y + 1 ) * extendedWidth + ( pt.x + 1 );

                    position = position - extendedWidth - 1;
                    if ( *( position ) != BLOCKED ) {
                        if ( *( position ) == OPEN ) {
                            regionTiles.push_back( Point( pt.x - 1, pt.y - 1 ) );
                            *( position ) = regionID;
                        }
                        ++neighbourCount;
                    }

                    ++position;
                    if ( *( position ) != BLOCKED ) {
                        if ( *( position ) == OPEN ) {
                            regionTiles.push_back( Point( pt.x, pt.y - 1 ) );
                            *( position ) = regionID;
                        }
                        ++neighbourCount;
                    }

                    ++position;
                    if ( *( position ) != BLOCKED ) {
                        if ( *( position ) == OPEN ) {
                            regionTiles.push_back( Point( pt.x + 1, pt.y - 1 ) );
                            *( position ) = regionID;
                        }
                        ++neighbourCount;
                    }

                    position = position + mapSize.w; // (mapWidth - 2) is width
                    if ( *( position ) != BLOCKED ) {
                        if ( *( position ) == OPEN ) {
                            regionTiles.push_back( Point( pt.x - 1, pt.y ) );
                            *( position ) = regionID;
                        }
                        ++neighbourCount;
                    }

                    position = position + 2;
                    if ( *( position ) != BLOCKED ) {
                        if ( *( position ) == OPEN ) {
                            regionTiles.push_back( Point( pt.x + 1, pt.y ) );
                            *( position ) = regionID;
                        }
                        ++neighbourCount;
                    }

                    position = position + mapSize.w; // (mapWidth - 2) is width
                    if ( *( position ) != BLOCKED ) {
                        if ( *( position ) == OPEN ) {
                            regionTiles.push_back( Point( pt.x - 1, pt.y + 1 ) );
                            *( position ) = regionID;
                        }
                        ++neighbourCount;
                    }

                    ++position;
                    if ( *( position ) != BLOCKED ) {
                        if ( *( position ) == OPEN ) {
                            regionTiles.push_back( Point( pt.x, pt.y + 1 ) );
                            *( position ) = regionID;
                        }
                        ++neighbourCount;
                    }

                    ++position;
                    if ( *( position ) != BLOCKED ) {
                        if ( *( position ) == OPEN ) {
                            regionTiles.push_back( Point( pt.x + 1, pt.y + 1 ) );
                            *( position ) = regionID;
                        }
                        ++neighbourCount;
                    }

                    // if ( neighbourCount != 8 ) {
                    //    edge.push_back( pt );
                    //    *( position - 1 - extendedWidth ) = BORDER;
                    //}

                } while ( tileIdx != regionTiles.size() );

                ++regionID;
            }
        }
    }

    bool AppendIfFarEnough( std::vector<int> & dataSet, int value, uint32_t distance )
    {
        bool match = true;
        for ( int & current : dataSet ) {
            if ( Maps::GetApproximateDistance( current, value ) < distance )
                match = false;
        }

        if ( match )
            dataSet.push_back( value );

        return match;
    }

    struct MapRegionNode
    {
        int index;
        int region;
    };

    struct MapRegion
    {
        int id;
        std::vector<MapRegion *> neighbours;
        std::vector<MapRegionNode> nodes;
    };

    struct RegionLinkRoute : std::list<int>
    {
        int _indexFrom;
        int _indexTo;
        size_t _length;
        uint32_t _basePenalty;
        uint32_t _roughTerrainPenalty;
    };

    struct MapNode
    {
        uint8_t type = BLOCKED;
        uint16_t passable = 0;
        bool isWater = false;
    };
}

void FillRegion2( std::vector<MapNode> & data, const Size & mapSize )
{
    static const Directions directions = Direction::All();
    const uint32_t extendedWidth = mapSize.w + 2;

    MapNode * currentTile = data.data() + extendedWidth + 1;
    MapNode * mapEnd = data.data() + extendedWidth * ( mapSize.h + 1 );

    uint32_t regionID = REGION;

    for ( ; currentTile != mapEnd; ++currentTile ) {
        if ( currentTile->type == OPEN ) {
            std::vector<Point> regionTiles;
            std::vector<Point> edge;

            currentTile->type = regionID;

            const size_t currentPosition = currentTile - data.data();
            regionTiles.push_back( Point( currentPosition % extendedWidth - 1, currentPosition / extendedWidth - 1 ) );
            bool isWater = false;

            size_t tileIdx = 0;
            do {
                Point pt = regionTiles[tileIdx++];

                const int tileIndex = ( pt.y + 1 ) * extendedWidth + pt.x + 1;
                uint8_t neighbourCount = 0;

                for ( const int direction : directions ) {
                    const int newIndex = Direction::GetDirectionIndex( tileIndex, direction, extendedWidth );
                    const MapNode & newTile = data[newIndex];
                    if ( newTile.type == OPEN && ( newTile.passable & Direction::Reflect( direction ) ) && newTile.isWater == isWater ) {
                        Point coord( newIndex % extendedWidth - 1, newIndex / extendedWidth - 1 );
                        regionTiles.push_back( coord );
                        data[( coord.y + 1 ) * extendedWidth + coord.x + 1].type = regionID;
                        ++neighbourCount;
                    }
                }

                if ( neighbourCount < 5 ) {
                    edge.push_back( pt );
                    data[currentPosition].type = BORDER;
                }

            } while ( tileIdx != regionTiles.size() );

            ++regionID;
        }
    }
}

void World::GrowRegion( std::set<int> & openTiles, std::unordered_map<int, int> & connection, int regionID )
{
    static const Directions directions = Direction::All();

    std::set<int> newTiles;
    for ( const int tileIndex : openTiles ) {
        if ( !vec_tiles[tileIndex]._region ) {
            vec_tiles[tileIndex]._region = regionID;

            for ( const int direction : directions ) {
                if ( Maps::isValidDirection( tileIndex, direction ) && ( vec_tiles[tileIndex].GetPassable() & direction ) ) {
                    const int newIndex = Maps::GetDirectionIndex( tileIndex, direction );
                    const Maps::Tiles & newTile = vec_tiles[newIndex];
                    if ( ( newTile.GetPassable() & Direction::Reflect( direction ) ) && newTile.isWater() == vec_tiles[tileIndex].isWater() ) {
                        if ( newTile._region ) {
                            if ( newTile._region != regionID ) {
                                auto currentValue = connection.find( newIndex );
                                if ( currentValue != connection.end() ) {
                                    ++currentValue->second;
                                }
                                else {
                                    connection.emplace( newIndex, 1 );
                                }
                            }
                        }
                        else {
                            newTiles.insert( Maps::GetDirectionIndex( tileIndex, direction ) );
                        }
                    }
                }
            }
        }
    }
    openTiles = std::move( newTiles );
}

void World::ComputeStaticAnalysis()
{
    int obstacles = 0;
    const int width = w();
    const int height = h();
    const int mapSize = std::max( width, height );

    std::unordered_map<int, int> connectionMap;
    std::vector<int> connectionVector( ( width + 1 ) * ( height + 1 ), 0 );
    std::vector<int> regionVector( connectionVector );
    connectionMap.reserve( static_cast<size_t>( mapSize ) * 3 ); // average amount of connections created

    const Directions directions = Direction::All();
    TileDataVector obsByColumn;
    TileDataVector obsByRow;
    TileDataVector castleCenters;
    std::vector<int> regionCenters;

    const uint32_t castleRegionSize = 16;
    const uint32_t extraRegionSize = 16;
    const uint32_t emptyLineFrequency = 8;
    const int waterRegionSize = mapSize / 3;

    for ( int x = 0; x < width; ++x )
        obsByColumn.emplace_back( x, 0 );
    for ( int y = 0; y < height; ++y )
        obsByRow.emplace_back( y, 0 );

    for ( int y = 0; y < height; ++y ) {
        const int rowIndex = y * width;
        for ( int x = 0; x < width; ++x ) {
            const int index = rowIndex + x;
            Maps::Tiles & tile = vec_tiles[index];
            if ( tile.GetPassable() == 0 || tile.isWater() ) {
                ++obstacles;
                ++obsByColumn[x].second;
                ++obsByRow[y].second;
            }
        }
    }

    auto smallerThanSort = []( const TileData & left, TileData & right ) { return left.second < right.second; };
    std::sort( obsByColumn.begin(), obsByColumn.end(), smallerThanSort );
    std::vector<int> emptyColumns;
    for ( const TileData & column : obsByColumn ) {
        AppendIfFarEnough( emptyColumns, column.first, emptyLineFrequency );
    }

    std::sort( obsByRow.begin(), obsByRow.end(), smallerThanSort );
    std::vector<int> emptyRows;
    for ( const TileData & row : obsByRow ) {
        AppendIfFarEnough( emptyRows, row.first, emptyLineFrequency );
    }

    // Values used to tweak region generation parameters
    const int tilesTotal = width * height;
    const int usableTiles = tilesTotal - obstacles;
    double freeTilesPercentage = usableTiles * 100.0 / tilesTotal;
    if ( vec_castles.size() ) {
        const int tilesPerCastle = usableTiles / vec_castles.size();
    }

    for ( Castle * castle : vec_castles ) {
        castleCenters.emplace_back( castle->GetIndex(), castle->GetColor() );
    }
    std::sort( castleCenters.begin(), castleCenters.end(), []( const TileData & left, const TileData & right ) {
        if ( left.second == right.second )
            return left.first < right.first;
        return left.second > right.second;
    } );

    for ( const TileData & castleTile : castleCenters ) {
        // Check if a lot of players next to each other? (Slugfest map)
        // GetCastle( Point( val % width, val / width ) )->GetColor();
        const int castleIndex = castleTile.first + width;
        AppendIfFarEnough( regionCenters, ( castleIndex > vec_tiles.size() ) ? castleTile.first : castleIndex, castleRegionSize );
    }

    for ( const int rowID : emptyRows ) {
        for ( const int colID : emptyColumns ) {
            int centerIndex = -1;

            const int tileIndex = rowID * width + colID;
            const Maps::Tiles & tile = vec_tiles[tileIndex];
            if ( tile.GetPassable() && !tile.isWater() ) {
                centerIndex = tileIndex;
            }
            else {
                for ( const int direction : directions ) {
                    if ( Maps::isValidDirection( tileIndex, direction ) ) {
                        const int newIndex = Maps::GetDirectionIndex( tileIndex, direction );
                        const Maps::Tiles & newTile = vec_tiles[newIndex];
                        if ( newTile.GetPassable() && !newTile.isWater() ) {
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

    std::vector<std::set<int> > openTiles( regionCenters.size() );
    for ( size_t regionID = 0; regionID < regionCenters.size(); ++regionID ) {
        openTiles[regionID].insert( regionCenters[regionID] );
    }

    // Region growing
    for ( int radius = 0; radius < mapSize / 2; ++radius ) {
        for ( size_t regionID = 0; regionID < regionCenters.size(); ++regionID ) {
            GrowRegion( openTiles[regionID], connectionMap, regionID + 1 );
        }
    }

    // Fix missing islands and divide seas
    size_t nextRegionID = regionCenters.size();
    for ( int y = 0; y < height; ++y ) {
        for ( int x = 0; x < width; ++x ) {
            const int tileIndex = y * width + x;
            Maps::Tiles & tile = vec_tiles[tileIndex];
            if ( tile.GetPassable() && tile._region == 0 ) {
                std::set<int> openTiles;
                openTiles.insert( tileIndex );
                const int islandID = nextRegionID++;

                for ( int iteration = 0; iteration < waterRegionSize; ++iteration ) {
                    GrowRegion( openTiles, connectionMap, islandID );
                }
            }
        }
    }

    // Create region connection clusters
    TileDataVector regionLinks;

    while ( !connectionMap.empty() ) {
        // begin() should be always valid if map is not empty
        TileData link = *connectionMap.begin();
        bool isRoad = vec_tiles[link.first].isRoad();

        std::set<int> openTiles;
        openTiles.insert( link.first );

        // Loop to find all tiles in a cluster, we only need 1 to make it a region link
        while ( !openTiles.empty() ) {
            // pop_first() for std::set
            const int tileIndex = *openTiles.begin();
            openTiles.erase( openTiles.begin() );

            std::unordered_map<int, int>::iterator currentTile = connectionMap.find( tileIndex );
            if ( currentTile != connectionMap.end() ) {
                const bool isCurrentTileRoad = vec_tiles[currentTile->first].isRoad();
                if ( ( isRoad == isCurrentTileRoad && currentTile->second > link.second ) || ( !isRoad && isCurrentTileRoad ) ) {
                    link = *currentTile;
                    isRoad = isCurrentTileRoad;
                }
                connectionMap.erase( currentTile );
            }

            // find if there's more tiles around
            for ( int direction : directions ) {
                if ( Maps::isValidDirection( tileIndex, direction ) ) {
                    const int newIndex = Maps::GetDirectionIndex( tileIndex, direction );

                    if ( connectionMap.find( newIndex ) != connectionMap.end() )
                        openTiles.insert( newIndex );
                }
            }
        }

        regionLinks.push_back( link );
    }

    std::vector<MapNode> data( ( width + 2 ) * ( height + 2 ) );
    for ( int y = 0; y < height; ++y ) {
        const int rowIndex = y * width;
        for ( int x = 0; x < width; ++x ) {
            const size_t index = rowIndex + x;
            const Maps::Tiles & tile = vec_tiles[index];
            MapNode & node = data[index + width + 1];

            node.passable = tile.GetPassable();
            node.isWater = tile.isWater();
            if ( node.passable != 0 && !node.isWater ) {
                node.type = OPEN;
            }
        }
    }

    FillRegion2( data, Size( width, height ) );

    for ( int y = 0; y < height; ++y ) {
        const int rowIndex = y * width;
        for ( int x = 0; x < width; ++x ) {
            const size_t index = rowIndex + x;
            vec_tiles[index]._metadata = data[index + width + 1].type;
        }
    }

    // DEBUG: view the hot spots
    // for ( const int center : regionCenters ) {
    //    vec_tiles[center]._metadata = vec_tiles[center]._region;
    //}
    // for ( const TileData & link : regionLinks ) {
    //    vec_tiles[link.first]._metadata = vec_tiles[link.first]._region;
    //}
}
