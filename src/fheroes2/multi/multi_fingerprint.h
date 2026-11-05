/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace Multi
{
    constexpr uint64_t fnv1aOffsetBasis{ 0xCBF29CE484222325ULL };

    // Pass the previous result as 'hash' to hash data in several chunks.
    uint64_t computeFnv1aHash( const uint8_t * data, const size_t size, uint64_t hash = fnv1aOffsetBasis );

    struct StateFingerprint
    {
        uint64_t hash{ 0 };

        // Serialized state size, in bytes.
        size_t size{ 0 };

        std::string hashString() const;
    };

    // The world and the victory and loss conditions, without local settings (language, game type, control types)
    StateFingerprint computeStateFingerprint();

    // Network games only. Context example: "after loading 'file.sav'".
    void logStateFingerprint( const std::string_view context );
}
