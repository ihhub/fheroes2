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

#include "logging.h"

namespace
{
    bool isInitialized = false;
    fheroes2::H2RReader reader;

    void initialize()
    {
        if ( isInitialized ) {
            return;
        }

        isInitialized = true;

#if defined( MACOS_APP_BUNDLE )
        ListFiles files = Settings::FindFiles( "h2d", ".h2d", false );
#else
        ListFiles files = Settings::FindFiles( System::ConcatePath( "files", "data" ), ".h2d", false );
#endif
        if ( files.empty() ) {
            return;
        }

        for ( const std::string & fileName : files ) {
            if ( reader.open( fileName ) ) {
                return;
            }
        }
    }
}

namespace fheroes2
{
    namespace h2d
    {
        bool readImage( const std::string & name, Sprite & image )
        {
            // Initialize only when it's requested.
            initialize();

            return readImageFromH2D( reader, name, image );
        }
    }
}
