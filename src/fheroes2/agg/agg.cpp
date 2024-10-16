/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <list>
#include <stdexcept>
#include <utility>

#include "agg.h"
#include "agg_file.h"
#include "dir.h"
#include "settings.h"
#include "tools.h"

namespace
{
    fheroes2::AGGFile heroes2_agg;
    fheroes2::AGGFile heroes2x_agg;
}

std::vector<uint8_t> AGG::getDataFromAggFile( const std::string & key, const bool ignoreExpansion )
{
    if ( !ignoreExpansion && heroes2x_agg.isGood() ) {
        // Make sure that the below container is not const and not a reference
        // so returning it from the function will invoke a move constructor instead of copy constructor.
        std::vector<uint8_t> buf = heroes2x_agg.read( key );
        if ( !buf.empty() )
            return buf;
    }

    return heroes2_agg.read( key );
}

AGG::AGGInitializer::AGGInitializer()
{
    if ( init() ) {
        return;
    }

    throw std::logic_error( "No AGG data files found." );
}

bool AGG::AGGInitializer::init()
{
    const ListFiles aggFileNames = Settings::FindFiles( "data", ".agg", false );
    if ( aggFileNames.empty() ) {
        return false;
    }

    const std::string heroes2AggFileName( "heroes2.agg" );
    std::string heroes2AggFilePath;
    std::string aggLowerCaseFilePath;

    for ( const std::string & path : aggFileNames ) {
        if ( path.size() < heroes2AggFileName.size() ) {
            // Obviously this is not a correct file.
            continue;
        }

        std::string tempPath = StringLower( path );

        if ( tempPath.compare( tempPath.size() - heroes2AggFileName.size(), heroes2AggFileName.size(), heroes2AggFileName ) == 0 ) {
            heroes2AggFilePath = path;
            aggLowerCaseFilePath = std::move( tempPath );
            break;
        }
    }

    if ( heroes2AggFilePath.empty() ) {
        // The main game resource file was not found.
        return false;
    }

    if ( !heroes2_agg.open( heroes2AggFilePath ) ) {
        return false;
    }

    _originalAGGFilePath = std::move( heroes2AggFilePath );

    // Find "heroes2x.agg" file.
    std::string heroes2XAggFilePath;
    fheroes2::replaceStringEnding( aggLowerCaseFilePath, ".agg", "x.agg" );

    for ( const std::string & path : aggFileNames ) {
        const std::string tempPath = StringLower( path );
        if ( tempPath == aggLowerCaseFilePath ) {
            heroes2XAggFilePath = path;
            break;
        }
    }

    if ( !heroes2XAggFilePath.empty() && heroes2x_agg.open( heroes2XAggFilePath ) ) {
        _expansionAGGFilePath = std::move( heroes2XAggFilePath );
    }

    Settings::Get().EnablePriceOfLoyaltySupport( heroes2x_agg.isGood() );

    return true;
}
