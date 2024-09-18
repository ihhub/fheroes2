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

#include "h2d.h"

#include <ostream>
#include <stdexcept>

#include "h2d_file.h"
#include "logging.h"
#include "settings.h"
#include "system.h"

namespace
{
    fheroes2::H2DReader reader;

    bool getH2DFilePath( const std::string & fileName, std::string & path )
    {
#if defined( MACOS_APP_BUNDLE )
        return Settings::findFile( "h2d", fileName, path );
#else
        return Settings::findFile( System::concatPath( "files", "data" ), fileName, path );
#endif
    }
}

namespace fheroes2::h2d
{
    H2DInitializer::H2DInitializer()
    {
        const std::string fileName{ "resurrection.h2d" };
        std::string filePath;
        if ( !getH2DFilePath( fileName, filePath ) ) {
            VERBOSE_LOG( "'" << fileName << "' file cannot be found in the system." )
            throw std::logic_error( fileName + " is not found." );
        }

        if ( !reader.open( filePath ) ) {
            VERBOSE_LOG( "Failed to open '" << filePath << "' file." )
            throw std::logic_error( std::string( "Cannot open file: " ) + filePath );
        }
    }

    bool readImage( const std::string & name, Sprite & image )
    {
        return readImageFromH2D( reader, name, image );
    }
}
