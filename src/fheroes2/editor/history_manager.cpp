/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

#include "history_manager.h"

#include <cassert>
#include <cstdint>
#include <vector>

#include "maps_tiles.h"
#include "world.h"
#include "world_object_uid.h"

namespace
{
    class MapAction : public fheroes2::Action
    {
    public:
        MapAction()
            : _latestObjectUIDBefore( Maps::getLastObjectUID() )
        {
            const int32_t size = world.w() * world.h();
            _before.reserve( size );

            for ( int32_t i = 0; i < size; ++i ) {
                _before.push_back( world.GetTiles( i ) );
            }
        }

        bool prepare()
        {
            std::vector<Maps::Tiles> temp;
            std::swap( temp, _before );

            const int32_t size = world.w() * world.h();
            if ( size != static_cast<int32_t>( temp.size() ) ) {
                assert( 0 );
                return false;
            }

            _latestObjectUIDAfter = Maps::getLastObjectUID();

            bool foundDifference = false;

            for ( int32_t i = 0; i < size; ++i ) {
                if ( temp[i] != world.GetTiles( i ) ) {
                    _before.push_back( std::move( temp[i] ) );
                    _after.push_back( world.GetTiles( i ) );
                    foundDifference = true;
                }
            }

            assert( _before.size() == _after.size() );

            // TODO: logically if the last object UID has been changed then we should mark the difference.

            return foundDifference;
        }

        bool redo() override
        {
            for ( const Maps::Tiles & tile : _after ) {
                // Copy operator is disabled for Maps::Tiles class.
                // This is done to avoid any tile copy made by a developer during the gameplay like map initialization.
                // Therefore, such a trick is required to assign a new value.
                Maps::Tiles temp{ tile };

                world.GetTiles( tile.GetIndex() ) = std::move( temp );
            }

            Maps::setLastObjectUID( _latestObjectUIDAfter );

            return ( !_after.empty() );
        }

        bool undo() override
        {
            for ( const Maps::Tiles & tile : _before ) {
                // Copy operator is disabled for Maps::Tiles class.
                // This is done to avoid any tile copy made by a developer during the gameplay like map initialization.
                // Therefore, such a trick is required to assign a new value.
                Maps::Tiles temp{ tile };

                world.GetTiles( tile.GetIndex() ) = std::move( temp );
            }

            Maps::setLastObjectUID( _latestObjectUIDBefore );

            return ( !_before.empty() );
        }

    private:
        std::vector<Maps::Tiles> _before;
        std::vector<Maps::Tiles> _after;

        const uint32_t _latestObjectUIDBefore{ 0 };
        uint32_t _latestObjectUIDAfter{ 0 };
    };
}

namespace fheroes2
{
    ActionCreator::ActionCreator( HistoryManager & manager )
        : _manager( manager )
    {
        _action = std::make_unique<MapAction>();
    }

    ActionCreator::~ActionCreator()
    {
        auto * action = dynamic_cast<MapAction *>( _action.get() );
        if ( action == nullptr ) {
            // How is it even possible?
            assert( 0 );
            return;
        }

        try {
            if ( action->prepare() ) {
                _manager.add( std::move( _action ) );
            }
        }
        catch ( ... ) {
            // If an exception happens here then something is very wrong with the code.
            assert( 0 );
        }
    }
}
