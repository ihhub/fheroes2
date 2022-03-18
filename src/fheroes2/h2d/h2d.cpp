/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
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

#include "h2d.h"
#include "h2d_file.h"
#include "settings.h"
#include "system.h"

#include <stdexcept>

namespace
{
    fheroes2::H2RReader reader;

    bool getH2DFilePath( const std::string & fileName, std::string & path )
    {
        std::string fullPath;

        const std::string internalDirectory( System::ConcatePath( "files", "data" ) );

        for ( const std::string & rootDir : Settings::GetRootDirs() ) {
            fullPath = System::ConcatePath( rootDir, internalDirectory );
            fullPath = System::ConcatePath( fullPath, fileName );
            if ( System::IsFile( fullPath ) ) {
                path.swap( fullPath );
                return true;
            }
        }

        return false;
    }
}

namespace fheroes2
{
    namespace h2d
    {
        H2DInitializer::H2DInitializer()
        {
            std::string filePath;
            if ( !getH2DFilePath( "resurrection.h2d", filePath ) ) {
                throw std::logic_error( "No H2D data files found." );
            }

            if ( reader.open( filePath ) ) {
                throw std::logic_error( "Cannot open H2D file." );
            }
        }

        bool readImage( const std::string & name, Sprite & image )
        {
            return readImageFromH2D( reader, name, image );
        }
    }
}
