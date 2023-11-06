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

#include "render_processor.h"

#include <cstdint>

#include "pal.h"

namespace
{
    // The maximum FPS the engine can handle is 125.
    // This means that a frame should be generated only every 8 ms.
    const uint64_t frameHalfInterval{ 4 };
}

namespace fheroes2
{
    RenderProcessor & RenderProcessor::instance()
    {
        static RenderProcessor processor;
        return processor;
    }

    bool RenderProcessor::preRenderAction( std::vector<uint8_t> & palette )
    {
        if ( !_enableCycling ) {
            return false;
        }

        if ( _enableRenderers && _preRenderer ) {
            _preRenderer();
        }

        if ( _cyclingTimer.getMs() < ( _cyclingInterval - frameHalfInterval ) ) {
            // If the current timer is less than cycling internal minus half of the frame generation then nothing is needed.
            return false;
        }

        // TODO: here we need to deduct possible time difference from the current time to have consistent FPS.
        _cyclingTimer.reset();
        palette = PAL::GetCyclingPalette( _cyclingCounter );
        ++_cyclingCounter;
        return true;
    }

    void RenderProcessor::postRenderAction()
    {
        _lastRenderCall.reset();

        if ( _enableCycling && _enableRenderers && _postRenderer ) {
            _postRenderer();
        }
    }
}
