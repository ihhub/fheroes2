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
}

void World::ComputeStaticAnalysis()
{
    int latestRegionID = 1;
    // plain for loop to make sure tile access is sequential
    // for ( size_t index = 0; index < vec_tiles.size(); ++index ) {
    //    Maps::Tiles & tile = vec_tiles[index];
    //    if ( tile.GetPassable() ) {
    //        if ( index >= w() ) {
    //            const Maps::Tiles & topTile = vec_tiles[index - w()];
    //            if ( topTile._region && topTile.isWater() == tile.isWater() ) {
    //                tile._region = topTile._region;
    //            }
    //        }
    //        //if ( tile._region == 0 && index >= w() && index + 1 % w() != 0 ) {
    //        if ( tile._region == 0 && index + 1 % w() != 0 ) {
    //            const Maps::Tiles & topRightTile = vec_tiles[index + 1];
    //            if ( topRightTile._region && topRightTile.isWater() == tile.isWater() ) {
    //                tile._region = topRightTile._region;
    //            }
    //        }
    //        if ( tile._region == 0 && index % w() ) {
    //            const Maps::Tiles & leftTile = vec_tiles[index - 1];
    //            if ( leftTile._region && leftTile.isWater() == tile.isWater() ) {
    //                tile._region = leftTile._region;
    //            }
    //        }

    //        if ( tile._region == 0 ) {
    //            tile._region = latestRegionID++;
    //        }
    //    }
    //}

    double obstacles = 0;
    const int width = w();
    const int heigth = h();
    const Directions directions = Direction::All();
    std::vector<std::pair<int, int> > obsColumns;
    std::vector<std::pair<int, int> > obsRows;
    std::vector<std::pair<int, int> > connection;
    std::vector<int> regionCenters;

    const uint32_t castleRegionSize = 16;
    const uint32_t extraRegionSize = 16;
    const int emptyLineFrequency = 8;

    for ( int x = 0; x < width; x++ )
        obsColumns.emplace_back( x, 0 );
    for ( int y = 0; y < heigth; y++ )
        obsRows.emplace_back( y, 0 );

    for ( int y = 0; y < heigth; y++ ) {
        for ( int x = 0; x < width; x++ ) {
            // initialize
            connection.emplace_back( y * width + x, 0 );

            const int index = y * width + x;
            Maps::Tiles & tile = vec_tiles[index];
            if ( tile.GetPassable() == 0 || tile.isWater() ) {
                obstacles++;
                obsColumns[x].second++;
                obsRows[y].second++;
            }
        }
    }

    std::sort( obsColumns.begin(), obsColumns.end(), []( const std::pair<int, int> & x, const std::pair<int, int> & y ) { return x.second < y.second; } );
    std::vector<int> emptyColumns;
    for ( auto column : obsColumns ) {
        AppendIfFarEnough( emptyColumns, column.first, emptyLineFrequency );
    }

    std::sort( obsRows.begin(), obsRows.end(), []( const std::pair<int, int> & x, const std::pair<int, int> & y ) { return x.second < y.second; } );
    std::vector<int> emptyRows;
    for ( auto row : obsRows ) {
        AppendIfFarEnough( emptyRows, row.first, emptyLineFrequency );
    }

    const int mapSize = width * heigth;
    const int usableTiles = mapSize - obstacles;
    double freeTilesPercentage = usableTiles * 100.0 / mapSize;
    const int tilesPerCastle = usableTiles / vec_castles.size();

    std::vector<std::pair<int, int> > castleCenters;
    for ( auto castle : vec_castles ) {
        castleCenters.emplace_back( castle->GetIndex(), castle->GetColor() );
    }
    std::sort( castleCenters.begin(), castleCenters.end(), []( const std::pair<int, int> & left, const std::pair<int, int> & right ) {
        if ( left.second == right.second )
            return left.first < right.first;
        return left.second > right.second;
    } );

    for ( auto castleTile : castleCenters ) {
        bool match = true;
        for ( int & val : regionCenters ) {
            // Check if different colors? (Slugfest)
            // GetCastle( Point( val % width, val / width ) )->GetColor();
            if ( Maps::GetApproximateDistance( val, castleTile.first ) < castleRegionSize ) {
                match = false;
                break;
            }
        }

        if ( match ) {
            const int castleIndex = castleTile.first + width;
            regionCenters.push_back( ( castleIndex > vec_tiles.size() ) ? castleTile.first : castleIndex );
        }
    }

    for ( auto rowID : emptyRows ) {
        for ( auto colID : emptyColumns ) {
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
                bool match = true;
                for ( int & val : regionCenters ) {
                    if ( Maps::GetApproximateDistance( val, centerIndex ) < extraRegionSize ) {
                        match = false;
                        break;
                    }
                }

                if ( match ) {
                    regionCenters.push_back( centerIndex );
                }
            }
        }
    }

    std::vector<std::set<int> > openTiles( regionCenters.size() );
    for ( size_t regionID = 0; regionID < regionCenters.size(); ++regionID ) {
        openTiles[regionID].insert( regionCenters[regionID] );
    }

    // Region growing
    for ( int radius = 0; radius < width / 2; ++radius ) {
        for ( size_t regionID = 0; regionID < regionCenters.size(); ++regionID ) {
            std::set<int> & tileSet = openTiles[regionID];

            std::set<int> newTiles;
            for ( const int & tileIndex : tileSet ) {
                vec_tiles[tileIndex]._region = regionID + ( ( radius ) ? 1 : 101 );
                for ( int direction : directions ) {
                    if ( Maps::isValidDirection( tileIndex, direction ) && ( vec_tiles[tileIndex].GetPassable() & direction ) ) {
                        const int newIndex = Maps::GetDirectionIndex( tileIndex, direction );
                        const Maps::Tiles & newTile = vec_tiles[newIndex];
                        if ( ( newTile.GetPassable() & Direction::Reflect( direction ) ) && newTile.isWater() == vec_tiles[tileIndex].isWater() ) {
                            if ( newTile._region ) {
                                if ( newTile._region != regionID + 1 ) {
                                    connection[newIndex].second++;
                                    // std::cout << "Map tile " << newIndex << " hit! " << newTile._region << " to " << regionID + 1 << std::endl;
                                }
                            }
                            else {
                                newTiles.insert( Maps::GetDirectionIndex( tileIndex, direction ) );
                            }
                        }
                    }
                }
            }
            tileSet = std::move( newTiles );
        }
    }

    for ( int y = 0; y < heigth; y++ ) {
        for ( int x = 0; x < width; x++ ) {
            // initialize
            connection.emplace_back( y * width + x, 0 );

            const int index = y * width + x;
            Maps::Tiles & tile = vec_tiles[index];
            if ( tile.GetPassable() == 0 || tile.isWater() ) {
                obstacles++;
                obsColumns[x].second++;
                obsRows[y].second++;
            }
        }
    }

    std::sort( connection.begin(), connection.end(), []( const std::pair<int, int> & x, const std::pair<int, int> & y ) { return x.second > y.second; } );
    // view the hot spots
    // for ( auto conn : connection ) {
    //    vec_tiles[conn.first]._region = conn.second;
    //}
}