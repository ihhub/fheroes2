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

#include "h2d.h"

#include <ostream>
#include <stdexcept>

#include <CoreFoundation/CoreFoundation.h>

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

namespace fheroes2
{
    namespace h2d
    {
    
    std::string get_resources_dir2()
    {
    
        CFArrayRef paths = CFBundleCopyResourceURLsOfType(CFBundleGetMainBundle(), CFSTR("h2d"), NULL);
        CFURLRef resourceURL = static_cast<CFURLRef>(CFArrayGetValueAtIndex(paths, 0));
      char resourcePath[PATH_MAX];
      if (CFURLGetFileSystemRepresentation(resourceURL, true,
                                           (UInt8 *)resourcePath,
                                           PATH_MAX))
      {
        if (resourceURL != NULL)
        {
          CFRelease(resourceURL);
        }
        return resourcePath;
      }
    
        return nil;
    }

    
        H2DInitializer::H2DInitializer()
        {
            std::string filePath = get_resources_dir2();
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
}
