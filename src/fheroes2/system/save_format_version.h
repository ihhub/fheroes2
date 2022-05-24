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
    // TODO: if you're adding a new version you must assign it to CURRENT_FORMAT_VERSION located at the bottom.
    FORMAT_VERSION_0916_RELEASE = 9900,
    FORMAT_VERSION_0912_RELEASE = 9803,
    FORMAT_VERSION_PRE3_0912_RELEASE = 9802,
    FORMAT_VERSION_PRE2_0912_RELEASE = 9801,
    FORMAT_VERSION_PRE1_0912_RELEASE = 9800,
    FORMAT_VERSION_097_RELEASE = 9701,

    LAST_SUPPORTED_FORMAT_VERSION = FORMAT_VERSION_097_RELEASE,

    CURRENT_FORMAT_VERSION = FORMAT_VERSION_0916_RELEASE
};
