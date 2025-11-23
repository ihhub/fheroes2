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

#include "history_manager.h"

#include <cassert>
#include <cstdint>

#include "map_format_helper.h"
#include "map_format_info.h"
#include "serialize.h"
#include "world_object_uid.h"

namespace
{
    // This class holds 2 copies of MapFormat objects in a compressed format:
    // - one copy before the action
    // - one copy after the action
    class MapAction final : public fheroes2::Action
    {
    public:
        explicit MapAction( Maps::Map_Format::MapFormat & mapFormat )
            : _mapFormat( mapFormat )
            , _latestObjectUIDBefore( Maps::getLastObjectUID() )
        {
            if ( !Maps::Map_Format::saveMap( _beforeMapFormat, _mapFormat ) ) {
                assert( 0 );
            }
        }

        // Disable the copy and move (implicitly) constructors and assignment operators.
        MapAction( const MapAction & ) = delete;
        MapAction & operator=( const MapAction & ) = delete;
        ~MapAction() override = default;

        bool prepare()
        {
            if ( !Maps::Map_Format::saveMap( _afterMapFormat, _mapFormat ) ) {
                assert( 0 );
                return false;
            }

            _latestObjectUIDAfter = Maps::getLastObjectUID();

            return true;
        }

        bool redo() override
        {
            if ( !Maps::Map_Format::loadMap( _afterMapFormat, _mapFormat ) ) {
                assert( 0 );
                return false;
            }

            _afterMapFormat.seek( 0 );

            if ( !Maps::readMapInEditor( _mapFormat ) ) {
                // If this assertion blows up then something is really wrong with the Editor.
                assert( 0 );
                return false;
            }

            Maps::setLastObjectUID( _latestObjectUIDAfter );

            return true;
        }

        bool undo() override
        {
            if ( !Maps::Map_Format::loadMap( _beforeMapFormat, _mapFormat ) ) {
                assert( 0 );
                return false;
            }

            _beforeMapFormat.seek( 0 );

            if ( !Maps::readMapInEditor( _mapFormat ) ) {
                // If this assertion blows up then something is really wrong with the Editor.
                assert( 0 );
                return false;
            }

            Maps::setLastObjectUID( _latestObjectUIDBefore );

            return true;
        }

    private:
        Maps::Map_Format::MapFormat & _mapFormat;

        RWStreamBuf _beforeMapFormat;
        RWStreamBuf _afterMapFormat;

        const uint32_t _latestObjectUIDBefore{ 0 };
        uint32_t _latestObjectUIDAfter{ 0 };
    };
}

namespace fheroes2
{
    ActionCreator::ActionCreator( HistoryManager & manager, Maps::Map_Format::MapFormat & mapFormat )
        : _manager( manager )
    {
        _action = std::make_unique<MapAction>( mapFormat );
    }

    void ActionCreator::commit()
    {
        auto * action = dynamic_cast<MapAction *>( _action.get() );
        if ( action == nullptr ) {
            // How is it even possible? Did you call this method twice?
            assert( 0 );
            return;
        }

        if ( action->prepare() ) {
            _manager.add( std::move( _action ) );
        }
    }
}
