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
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "serialize.h"

namespace fheroes2
{
    class Sprite;

    // Heroes 2 Data (H2D) file format used for storing files needed for the project. This format is not a part of original HoMM II.
    class H2DReader
    {
    public:
        // Returns true if file opening is successful.
        bool open( const std::string & path );

        // Returns non-empty vector if requested file exists.
        std::vector<uint8_t> getFile( const std::string & fileName );

        std::set<std::string> getAllFileNames() const;

    private:
        // Relationship between file name in non-capital letters and its offset from the start of the archive.
        std::map<std::string, std::pair<uint32_t, uint32_t>> _fileNameAndOffset;

        // Stream for reading h2d file.
        StreamFile _fileStream;
    };

    // This class is not designed to be performance optimized as it will be used very rarely and out of game running session.
    class H2DWriter
    {
    public:
        // Returns true if file opening is successful.
        bool write( const std::string & path ) const;

        bool add( const std::string & name, const std::vector<uint8_t> & data );

        // Add all entries from a H2D reader.
        bool add( H2DReader & reader );

    private:
        std::map<std::string, std::vector<uint8_t>> _fileData;
    };

    bool readImageFromH2D( H2DReader & reader, const std::string & name, Sprite & image );

    bool writeImageToH2D( H2DWriter & writer, const std::string & name, const Sprite & image );
}
