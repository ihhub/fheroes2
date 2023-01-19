/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2023                                             *
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

    FORMAT_VERSION_1001_RELEASE = 10003,
    FORMAT_VERSION_PRE2_1001_RELEASE = 10002,
    FORMAT_VERSION_PRE1_1001_RELEASE = 10001,
    FORMAT_VERSION_1000_RELEASE = 10000,
    FORMAT_VERSION_PRE5_1000_RELEASE = 9964,
    FORMAT_VERSION_PRE4_1000_RELEASE = 9963,
    FORMAT_VERSION_PRE3_1000_RELEASE = 9962,
    FORMAT_VERSION_PRE2_1000_RELEASE = 9961,
    FORMAT_VERSION_PRE1_1000_RELEASE = 9960,
    FORMAT_VERSION_0921_RELEASE = 9950,

    LAST_SUPPORTED_FORMAT_VERSION = FORMAT_VERSION_0921_RELEASE,

    CURRENT_FORMAT_VERSION = FORMAT_VERSION_1001_RELEASE
};
