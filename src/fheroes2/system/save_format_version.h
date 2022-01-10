/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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
    FORMAT_VERSION_0912_RELEASE = 9800,
    FORMAT_VERSION_097_RELEASE = 9701,
    FORMAT_VERSION_PRE_097_RELEASE = 9700,
    FORMAT_VERSION_096_RELEASE = 9600,
    FORMAT_VERSION_095_RELEASE = 9502,

    LAST_SUPPORTED_FORMAT_VERSION = FORMAT_VERSION_095_RELEASE,

    CURRENT_FORMAT_VERSION = FORMAT_VERSION_0912_RELEASE
};
