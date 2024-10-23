/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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
    FORMAT_VERSION_1104_RELEASE = 10025,
    FORMAT_VERSION_1103_RELEASE = 10024,
    FORMAT_VERSION_PRE2_1103_RELEASE = 10023,
    FORMAT_VERSION_PRE1_1103_RELEASE = 10022,
    FORMAT_VERSION_1101_RELEASE = 10021,
    FORMAT_VERSION_PRE1_1101_RELEASE = 10020,
    FORMAT_VERSION_1100_RELEASE = 10019,
    FORMAT_VERSION_PRE3_1100_RELEASE = 10018,
    FORMAT_VERSION_PRE2_1100_RELEASE = 10017,
    FORMAT_VERSION_PRE1_1100_RELEASE = 10016,
    FORMAT_VERSION_1010_RELEASE = 10015,
    FORMAT_VERSION_1009_RELEASE = 10014,
    FORMAT_VERSION_PRE2_1009_RELEASE = 10013,
    FORMAT_VERSION_PRE1_1009_RELEASE = 10012,
    FORMAT_VERSION_1007_RELEASE = 10011,
    FORMAT_VERSION_1005_RELEASE = 10010,

    LAST_SUPPORTED_FORMAT_VERSION = FORMAT_VERSION_1005_RELEASE,

    CURRENT_FORMAT_VERSION = FORMAT_VERSION_1104_RELEASE
};
