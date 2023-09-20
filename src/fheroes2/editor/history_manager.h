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

#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace fheroes2
{
    class HistoryManager;

    class Action
    {
    public:
        virtual ~Action() = default;

        virtual bool redo() = 0;

        virtual bool undo() = 0;
    };

    // Remember the map state and create an action if the map has changed.
    class ActionCreator
    {
    public:
        explicit ActionCreator( HistoryManager & manager );

        ~ActionCreator();

        ActionCreator( const ActionCreator & ) = delete;

        ActionCreator & operator=( const ActionCreator & ) = delete;

    private:
        HistoryManager & _manager;

        std::unique_ptr<Action> _action;
    };

    class HistoryManager
    {
    public:
        void add( std::unique_ptr<Action> action )
        {
            _actions.resize( _lastActionId );

            _actions.push_back( std::move( action ) );

            ++_lastActionId;
        }

        bool undo()
        {
            if ( _lastActionId == 0 ) {
                // Nothing to do.
                return false;
            }

            --_lastActionId;
            return _actions[_lastActionId]->undo();
        }

        bool redo()
        {
            if ( _lastActionId == _actions.size() ) {
                // Nothing to do.
                return false;
            }

            const bool result = _actions[_lastActionId]->redo();
            ++_lastActionId;

            return result;
        }

        bool isUndoAvailable() const
        {
            return ( _lastActionId > 0 );
        }

        bool isRedoAvailable() const
        {
            return ( _lastActionId < _actions.size() );
        }

    private:
        std::vector<std::unique_ptr<Action>> _actions;

        size_t _lastActionId{ 0 };
    };
}
