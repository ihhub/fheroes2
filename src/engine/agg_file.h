/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#ifndef AGG_FILE_H
#define AGG_FILE_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "serialize.h"

namespace fheroes2
{
    class AGGFile
    {
    public:
        bool isGood() const
        {
            return !_stream.fail() && !_files.empty();
        }

        bool open( const std::string & fileName );
        std::vector<uint8_t> read( const std::string & fileName );

    private:
        static const size_t _maxFilenameSize = 15; // 8.3 ASCIIZ file name + 2-bytes padding

        StreamFile _stream;
        std::map<std::string, std::pair<uint32_t, uint32_t>, std::less<>> _files;
    };

    struct ICNHeader
    {
        int16_t offsetX{ 0 };
        int16_t offsetY{ 0 };
        uint16_t width{ 0 };
        uint16_t height{ 0 };
        // Used for adventure map animations, this can replace ICN::AnimationFrame.
        // The frames count is always a modulus of 32 (only 5 bits are used): for animations with more than 31 frames the value is ( totalFrames - 32 ).
        // TODO: Find a way to detect that 32 was deducted from the animationFrames value if it is possible.
        // When the 6th bit in animationFrames is set then it is Monochromatic ICN image.
        uint8_t animationFrames{ 0 };
        uint32_t offsetData{ 0 };
    };

    uint32_t calculateAggFilenameHash( const std::string_view str );
}

IStreamBase & operator>>( IStreamBase & stream, fheroes2::ICNHeader & icn );

#endif
