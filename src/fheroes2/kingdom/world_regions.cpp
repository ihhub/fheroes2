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

    struct MapRegion
    {
        int id;
        std::vector<MapRegion *> _neighbors;
    };

    struct MapRegionNode
    {};

    struct RegionLink
    {};

    struct RegionLinkRoute : std::list<int>
    {
        int _indexFrom;
        int _indexTo;
        size_t _length;
        uint32_t _basePenalty;
        uint32_t _roughTerrainPenalty;
    };
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
        else if ( vec_tiles[tileIndex]._region != regionID ) {
            auto currentValue = connection.find( tileIndex );
            if ( currentValue != connection.end() ) {
                ++currentValue->second;
            }
            else {
                //connection.emplace( tileIndex, 1 );
            }
            //std::cout << "Regions " << regionID << " and " << vec_tiles[tileIndex]._region << " met in tile " << tileIndex << std::endl;
        }
    }
    openTiles = std::move( newTiles );
}

void World::ComputeStaticAnalysis()
{
    int obstacles = 0;
    const int width = w();
    const int heigth = h();
    const int mapSize = std::max( width, heigth );

    std::unordered_map<int, int> connectionMap;
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

    for ( int x = 0; x < width; x++ )
        obsByColumn.emplace_back( x, 0 );
    for ( int y = 0; y < heigth; y++ )
        obsByRow.emplace_back( y, 0 );

    for ( int y = 0; y < heigth; y++ ) {
        for ( int x = 0; x < width; x++ ) {
            const int index = y * width + x;
            Maps::Tiles & tile = vec_tiles[index];
            if ( tile.GetPassable() == 0 || tile.isWater() ) {
                obstacles++;
                obsByColumn[x].second++;
                obsByRow[y].second++;
            }
        }
    }

    auto smallerThanSort = []( const TileData & left, TileData & right ) { return left.second < right.second; };
    std::sort( obsByColumn.begin(), obsByColumn.end(), smallerThanSort );
    std::vector<int> emptyColumns;
    for ( auto column : obsByColumn ) {
        AppendIfFarEnough( emptyColumns, column.first, emptyLineFrequency );
    }

    std::sort( obsByRow.begin(), obsByRow.end(), smallerThanSort );
    std::vector<int> emptyRows;
    for ( auto row : obsByRow ) {
        AppendIfFarEnough( emptyRows, row.first, emptyLineFrequency );
    }

    // Values used to tweak region generation parameters
    const int tilesTotal = width * heigth;
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

    for ( auto castleTile : castleCenters ) {
        // Check if different colors? (Slugfest map)
        // GetCastle( Point( val % width, val / width ) )->GetColor();
        const int castleIndex = castleTile.first + width;
        AppendIfFarEnough( regionCenters, ( castleIndex > vec_tiles.size() ) ? castleTile.first : castleIndex, castleRegionSize );
    }

    for ( int rowID : emptyRows ) {
        for ( int colID : emptyColumns ) {
            int centerIndex = -1;

            const int tileIndex = rowID * width + colID;
            const Maps::Tiles & tile = vec_tiles[tileIndex];
            if ( tile.GetPassable() && !tile.isWater() ) {
                centerIndex = tileIndex;
            }
            else {
                for ( int direction : directions ) {
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
    for ( int y = 0; y < heigth; y++ ) {
        for ( int x = 0; x < width; x++ ) {
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


        //const int x = tileIndex % width;
        //const int y = tileIndex / width;
        //for ( int newY = y - 2; newY < y + 2; ++newY ) {
        //    for ( int newX = x - 2; newX < x + 2; ++newX ) {
        //        if ( Maps::isValidAbsPoint( newX, newY ) ) {
        //            const int newIndex = Maps::GetIndexFromAbsPoint( newX, newY );

        //            std::unordered_map<int, int>::iterator nextTile = connectionMap.find( newIndex );
        //            if ( nextTile != connectionMap.end() ) {
        //                indiciesToRemove.push_back( newIndex );
        //                if ( nextTile->second > link.second ) {
        //                    link = *nextTile;
        //                }
        //            }
        //        }
        //    }
        //}

        // FIND ROAD

        std::set<int> openTiles;
        openTiles.insert( link.first );

        while ( !openTiles.empty() ) {
            // pop first element
            const int tileIndex = *openTiles.begin();
            openTiles.erase( openTiles.begin() );

            std::unordered_map<int, int>::iterator currentTile = connectionMap.find( tileIndex );
            if ( currentTile != connectionMap.end() ) {
                if ( currentTile->second > link.second ) {
                    link = *currentTile;
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

        //for ( int idx : indiciesToRemove )
        //    connectionMap.erase( idx );
    }

    // std::sort( regionLink.begin(), regionLink.end(), []( const TileData & x, const TileData & y ) { return x.second > y.second; } );

    // view the hot spots
    for ( auto conn : regionLinks ) {
        vec_tiles[conn.first]._metadata = conn.second;
    }
}