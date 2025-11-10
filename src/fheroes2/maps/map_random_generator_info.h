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

#pragma once

#include <cstdint>
#include <vector>

#include "map_object_info.h"
#include "math_base.h"
#include "resource.h"

namespace Maps::Random_Generator
{
    inline int neutralColorIndex{ Color::GetIndex( PlayerColor::UNUSED ) };
    inline constexpr std::array<int, 4> secondaryResources = { Resource::CRYSTAL, Resource::SULFUR, Resource::GEMS, Resource::MERCURY };

    enum class NodeType : uint8_t
    {
        OPEN,
        BORDER,
        ACTION,
        OBSTACLE,
        CONNECTOR,
        PATH
    };

    struct Node final
    {
        int index{ -1 };
        uint32_t region{ 0 };
        NodeType type{ NodeType::OPEN };

        Node() = default;

        explicit Node( const int index_ )
            : index( index_ )
        {
            // Do nothing
        }
    };

    class NodeCache final
    {
    public:
        NodeCache( const int32_t width, const int32_t height );

        Node & getNode( const fheroes2::Point position )
        {
            if ( position.x < 0 || position.x >= _mapSize || position.y < 0 || position.y >= _mapSize ) {
                // We shouldn't try to get a tile with an invalid index.
                // TODO: here we must add an assertion and make sure that we never reach this place.
                return _outOfBounds;
            }

            return _data[position.y * _mapSize + position.x];
        }

        const Node & getNode( const fheroes2::Point position ) const
        {
            if ( position.x < 0 || position.x >= _mapSize || position.y < 0 || position.y >= _mapSize ) {
                // We shouldn't try to get a tile with an invalid index.
                // However, since we are accessing const value it is fine to get an invalid item.
                return _outOfBounds;
            }

            return _data[position.y * _mapSize + position.x];
        }

        Node & getNode( const int32_t index )
        {
            if ( index < 0 || index >= _mapSize * _mapSize ) {
                // We shouldn't try to get a tile with an invalid index.
                assert( 0 );
                return _outOfBounds;
            }

            return _data[index];
        }

    private:
        const int32_t _mapSize{ 0 };
        Node _outOfBounds;
        std::vector<Node> _data;
    };

    struct Region final
    {
        uint32_t id{ 0 };
        int32_t centerIndex{ -1 };
        std::set<uint32_t> neighbours;
        std::vector<std::reference_wrapper<Node>> nodes;
        std::map<uint32_t, int32_t> connections;
        size_t sizeLimit{ 0 };
        size_t lastProcessedNode{ 0 };
        int colorIndex{ neutralColorIndex };
        int groundType{ Maps::Ground::GRASS };

        Region() = default;

        Region( const uint32_t regionIndex, Node & centerNode, const int playerColor, const int ground, const size_t expectedSize )
            : id( regionIndex )
            , centerIndex( centerNode.index )
            , sizeLimit( expectedSize )
            , colorIndex( playerColor )
            , groundType( ground )
        {
            assert( expectedSize > 0 );

            centerNode.region = regionIndex;
            nodes.reserve( expectedSize );
            nodes.emplace_back( centerNode );
        }
    };

    struct MapEconomy final
    {
        std::map<int, uint32_t> minesCount{ { Resource::WOOD, 0 }, { Resource::ORE, 0 },     { Resource::CRYSTAL, 0 }, { Resource::SULFUR, 0 },
                                            { Resource::GEMS, 0 }, { Resource::MERCURY, 0 }, { Resource::GOLD, 0 } };

        void increaseMineCount( const int resource );
        int pickNextMineResource();
    };

    struct RegionalObjects final
    {
        uint8_t castleCount{ 0 };
        uint8_t mineCount{ 0 };
        uint8_t objectCount{ 0 };
        uint8_t powerUpsCount{ 0 };
        uint8_t treasureCount{ 0 };
    };

    struct ObjectPlacement final
    {
        fheroes2::Point offset;
        Maps::ObjectGroup groupType{ Maps::ObjectGroup::NONE };
        int32_t objectIndex{ 0 };
    };

    struct ObjectSet final
    {
        std::vector<ObjectPlacement> obstacles;
        std::vector<ObjectPlacement> valuables;
        std::vector<ObjectPlacement> monsters;
        std::vector<fheroes2::Point> entranceCheck;
    };
}
