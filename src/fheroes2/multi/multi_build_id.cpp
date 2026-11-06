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

#include "multi_build_id.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include "logging.h"
#include "multi_fingerprint.h"

#if __has_include( "multi_source_id.h" )
#include "multi_source_id.h"
#endif

#if !defined( MULTI_SOURCE_ID )
namespace
{
    std::filesystem::path getExecutablePath()
    {
#if defined( _WIN32 )
        std::wstring path( MAX_PATH, L'\0' );

        while ( true ) {
            const DWORD length = GetModuleFileNameW( nullptr, path.data(), static_cast<DWORD>( path.size() ) );
            if ( length == 0 ) {
                return {};
            }

            if ( length < path.size() ) {
                path.resize( length );
                return path;
            }

            path.resize( path.size() * 2 );
        }
#elif defined( __linux__ )
        std::error_code error;
        return std::filesystem::read_symlink( "/proc/self/exe", error );
#else
        return {};
#endif
    }

    uint64_t computeBuildId()
    {
        const std::filesystem::path path = getExecutablePath();
        if ( path.empty() ) {
            ERROR_LOG( "[MULTI] Unable to find the path of the executable file." )
            return 0;
        }

        std::ifstream file( path, std::ios::binary );
        if ( !file ) {
            ERROR_LOG( "[MULTI] Unable to open the executable file " << path.u8string() )
            return 0;
        }

        std::array<char, 64 * 1024> buffer{};
        uint64_t hash = Multi::fnv1aOffsetBasis;

        while ( file ) {
            file.read( buffer.data(), static_cast<std::streamsize>( buffer.size() ) );
            hash = Multi::computeFnv1aHash( reinterpret_cast<const uint8_t *>( buffer.data() ), static_cast<size_t>( file.gcount() ), hash );
        }

        return hash;
    }
}
#endif

namespace
{
    bool isOtherSourcesPretended{ false };
}

uint64_t Multi::getBuildId()
{
#if defined( MULTI_SOURCE_ID )
    const uint64_t buildId = MULTI_SOURCE_ID;
#else
    static const uint64_t buildId = computeBuildId();
#endif

    return isOtherSourcesPretended ? ~buildId : buildId;
}

void Multi::pretendOtherSourcesForTest()
{
    isOtherSourcesPretended = true;
}

std::string Multi::getPlatformName()
{
#if defined( __ANDROID__ )
    return "Android";
#elif defined( _WIN32 )
    return "Windows";
#elif defined( __APPLE__ )
    return "macOS";
#elif defined( __linux__ )
    return "Linux";
#else
    return "?";
#endif
}
