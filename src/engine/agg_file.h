/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include <map>
#include <string>
#include <vector>

#include "serialize.h"

namespace fheroes2
{
    class AGGFile
    {
    public:
        AGGFile();

        bool isGood() const;
        bool open( const std::string & fileName );
        std::vector<uint8_t> read( const std::string & fileName );

    private:
        static const size_t _maxFilenameSize = 15; // 8.3 ASCIIZ file name + 2-bytes padding

        StreamFile _stream;
        std::map<std::string, std::pair<uint32_t, uint32_t> > _files;
    };

    struct ICNHeader
    {
        ICNHeader()
            : offsetX( 0 )
            , offsetY( 0 )
            , width( 0 )
            , height( 0 )
            , animationFrames( 0 )
            , offsetData( 0 )
        {}

        uint16_t offsetX;
        uint16_t offsetY;
        uint16_t width;
        uint16_t height;
        uint8_t animationFrames; // used for adventure map animations, this can replace ICN::AnimationFrame
        uint32_t offsetData;
    };
}

StreamBase & operator>>( StreamBase & st, fheroes2::ICNHeader & icn );

#endif
