/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2025                                             *
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
#include <type_traits>

namespace Video
{
    enum VideoControl : uint32_t
    {
        PLAY_NONE = 0,
        // Play file once and wait user's input
        PLAY_WAIT = 1 << 0,
        // Play file with infinite loop
        PLAY_LOOP = 1 << 1,
        // Play audio stream from file
        PLAY_AUDIO = 1 << 2,
        // Play video stream from file
        PLAY_VIDEO = 1 << 3,
        // Set of common flags for convenient
        // Play only video in loop
        PLAY_VIDEO_LOOP = PLAY_LOOP | PLAY_VIDEO,
        // Play cutscene only once
        PLAY_CUTSCENE = PLAY_AUDIO | PLAY_VIDEO,
        // Play cutscene and wait for user input
        PLAY_CUTSCENE_WAIT = PLAY_WAIT | PLAY_AUDIO | PLAY_VIDEO,
        // Play cutscene in loop
        PLAY_CUTSCENE_LOOP = PLAY_LOOP | PLAY_AUDIO | PLAY_VIDEO,
    };
}
