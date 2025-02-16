/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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
#include "world_object_uid.h"

namespace
{
    // This class holds 2 copies of MapFormat objects:
    // - one copy before the action
    // - one copy after the action
    // Ideally, we need to store only difference between before and after an action
    // but for simplification purposes we chose this way.
    // TODO: optimize this class (if possible) to store less data.
    class MapAction final : public fheroes2::Action
    {
    public:
        explicit MapAction( Maps::Map_Format::MapFormat & mapFormat )
            : _mapFormat( mapFormat )
            , _latestObjectUIDBefore( Maps::getLastObjectUID() )
        {
            if ( !Maps::saveMapInEditor( _mapFormat ) ) {
                // If this assertion blows up then something is really wrong with the Editor.
                assert( 0 );
            }

            _beforeMapFormat = _mapFormat;
        }

        bool prepare()
        {
            if ( !Maps::saveMapInEditor( _mapFormat ) ) {
                // If this assertion blows up then something is really wrong with the Editor.
                assert( 0 );
                return false;
            }

            _afterMapFormat = _mapFormat;

            _latestObjectUIDAfter = Maps::getLastObjectUID();

            return true;
        }

        bool redo() override
        {
            _mapFormat = _afterMapFormat;
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
            _mapFormat = _beforeMapFormat;
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

        Maps::Map_Format::MapFormat _beforeMapFormat;
        Maps::Map_Format::MapFormat _afterMapFormat;

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
