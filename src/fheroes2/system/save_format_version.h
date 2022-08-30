/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2022                                             *
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

enum SaveFileFormat : uint16_t
{
    // !!! IMPORTANT !!!
    // If you're adding a new version you must assign it to CURRENT_FORMAT_VERSION located at the bottom.
    // If you're removing an old version you must assign the oldest available to LAST_SUPPORTED_FORMAT_VERSION located at the bottom.

    // 10000 value must be used for 1.0 release so all version before it should have lower than this value.
    FORMAT_VERSION_0919_RELEASE = 9930,
    FORMAT_VERSION_0918_RELEASE = 9921,
    FORMAT_VERSION_PRE_0918_RELEASE = 9920,
    FORMAT_VERSION_0917_RELEASE = 9911,
    FORMAT_VERSION_PRE_0917_RELEASE = 9910,

    LAST_SUPPORTED_FORMAT_VERSION = FORMAT_VERSION_PRE_0917_RELEASE,

    CURRENT_FORMAT_VERSION = FORMAT_VERSION_0919_RELEASE
};
