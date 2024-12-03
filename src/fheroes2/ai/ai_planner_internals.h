/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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

#include "world_pathfinding.h"

namespace AI
{
    const double ARMY_ADVANTAGE_DESPERATE = 0.8;
    const double ARMY_ADVANTAGE_SMALL = 1.3;
    const double ARMY_ADVANTAGE_MEDIUM = 1.5;
    const double ARMY_ADVANTAGE_LARGE = 1.8;

    class AIWorldPathfinderStateRestorer
    {
    public:
        explicit AIWorldPathfinderStateRestorer( AIWorldPathfinder & pathfinder )
            : _pathfinder( pathfinder )
            , _originalMinimalArmyStrengthAdvantage( _pathfinder.getMinimalArmyStrengthAdvantage() )
            , _originalSpellPointsReserveRatio( _pathfinder.getSpellPointsReserveRatio() )
        {}

        AIWorldPathfinderStateRestorer( const AIWorldPathfinderStateRestorer & ) = delete;

        ~AIWorldPathfinderStateRestorer()
        {
            _pathfinder.setMinimalArmyStrengthAdvantage( _originalMinimalArmyStrengthAdvantage );
            _pathfinder.setSpellPointsReserveRatio( _originalSpellPointsReserveRatio );
        }

        AIWorldPathfinderStateRestorer & operator=( const AIWorldPathfinderStateRestorer & ) = delete;

    private:
        AIWorldPathfinder & _pathfinder;

        const double _originalMinimalArmyStrengthAdvantage;
        const double _originalSpellPointsReserveRatio;
    };
}
