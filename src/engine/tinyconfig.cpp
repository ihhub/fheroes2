/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <cctype>

#if defined( MACOS_APP_BUNDLE )
#include "logging.h"
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "serialize.h"
#include "tinyconfig.h"
#include "tools.h"

bool SpaceCompare( char a, char b )
{
    return std::isspace( a ) && std::isspace( b );
}

std::string ModifyKey( const std::string & str )
{
    std::string key = StringTrim( StringLower( str ) );

    // remove multiple space
    key.erase( std::unique( key.begin(), key.end(), SpaceCompare ), key.end() );

    // change space
    std::replace_if( key.begin(), key.end(), ::isspace, 0x20 );

    return key;
}

TinyConfig::TinyConfig( char sep, char com )
    : separator( sep )
    , comment( com )
{}

bool TinyConfig::Load( const std::string & cfile )
{
#if defined( MACOS_APP_BUNDLE )
    CFDataRef resourceData = NULL;
    CFPropertyListRef propertyList = NULL;
    CFDictionaryRef propertyListDictionary = NULL;
    CFStringRef configFileCFString = NULL;
    CFURLRef fileURL = NULL;

    configFileCFString = CFStringCreateWithCString( kCFAllocatorDefault, cfile.c_str(), kCFStringEncodingUTF8 );

    fileURL = CFURLCreateWithFileSystemPath( kCFAllocatorDefault, configFileCFString, kCFURLPOSIXPathStyle, false );

    CFRelease( configFileCFString );

    CFReadStreamRef streamRef = CFReadStreamCreateWithFile( kCFAllocatorDefault, fileURL );
    CFRelease( fileURL );

    if ( !streamRef ) {
        ERROR_LOG( "Unable to create stream reference for reading config plist" );
        return false;
    }

    if ( !CFReadStreamOpen( streamRef ) ) {
        CFRelease( streamRef );
        ERROR_LOG( "Unable to open stream reference for reading config plist" );
        return false;
    }

    if ( !CFReadStreamHasBytesAvailable( streamRef ) ) {
        CFRelease( streamRef );
        ERROR_LOG( "No data to read from config plist" );
        return false;
    }

    CFIndex bytesRead = 0;
    UInt8 fallbackBuffer[1048576];
    const UInt8 * dataBytes = CFReadStreamGetBuffer( streamRef, 0, &bytesRead );
    if ( !dataBytes ) {
        bytesRead = CFReadStreamRead( streamRef, fallbackBuffer, sizeof( fallbackBuffer ) );
        dataBytes = fallbackBuffer;
    }

    resourceData = CFDataCreate( kCFAllocatorDefault, dataBytes, bytesRead );

    CFReadStreamClose( streamRef );
    CFRelease( streamRef );

    if ( !resourceData ) {
        ERROR_LOG( "Unable to read data from config plist" );
        return false;
    }

    propertyList = static_cast<CFPropertyListRef>( CFPropertyListCreateWithData( kCFAllocatorDefault, resourceData, kCFPropertyListImmutable, NULL, NULL ) );

    if ( resourceData ) {
        CFRelease( resourceData );
    }
    else {
        ERROR_LOG( "Unable to fetch config resource data" );
        CFRelease( fileURL );
        return false;
    }

    if ( !propertyList ) {
        return false;
    }

    propertyListDictionary = static_cast<CFDictionaryRef>( propertyList );
    CFIndex numKeys = CFDictionaryGetCount( propertyListDictionary );

    const void ** keys = (const void **)malloc( sizeof( void * ) * numKeys );
    const void ** values = (const void **)malloc( sizeof( void * ) * numKeys );
    char keyBuf[2048];
    char valBuf[2048];

    CFDictionaryGetKeysAndValues( propertyListDictionary, keys, values );

    for ( CFIndex i = 0; i < numKeys; i++ ) {
        if ( !CFStringGetCString( static_cast<CFStringRef>( keys[i] ), keyBuf, 2048, kCFStringEncodingUTF8 ) ) {
            ERROR_LOG( "Error converting config key to string" );
            free( keys );
            free( values );
            return false;
        }
        if ( !CFStringGetCString( static_cast<CFStringRef>( values[i] ), valBuf, 2048, kCFStringEncodingUTF8 ) ) {
            ERROR_LOG( "Error converting config value to string" );
            free( keys );
            free( values );
            return false;
        }

        std::string keyString = std::string( keyBuf );
        std::string valueString = std::string( valBuf );

        emplace( ModifyKey( keyString ), valueString );
    }

    free( keys );
    free( values );

    CFRelease( propertyList );
#else
    StreamFile sf;
    if ( !sf.open( cfile, "rb" ) )
        return false;

    std::vector<std::string> rows = StringSplit( sf.toString(), "\n" );

    for ( std::vector<std::string>::const_iterator it = rows.begin(); it != rows.end(); ++it ) {
        std::string str = StringTrim( *it );

        if ( str.empty() || str[0] == comment )
            continue;

        size_t pos = str.find( separator );
        if ( std::string::npos != pos ) {
            std::string left( str.substr( 0, pos ) );
            std::string right( str.substr( pos + 1, str.length() - pos - 1 ) );

            left = StringTrim( left );
            right = StringTrim( right );

            emplace( ModifyKey( left ), right );
        }
    }
#endif

    return true;
}

int TinyConfig::IntParams( const std::string & key ) const
{
    const_iterator it = find( ModifyKey( key ) );
    return it != end() ? GetInt( it->second ) : 0;
}

std::string TinyConfig::StrParams( const std::string & key ) const
{
    const_iterator it = find( ModifyKey( key ) );
    return it != end() ? it->second : "";
}

bool TinyConfig::Exists( const std::string & key ) const
{
    return end() != find( ModifyKey( key ) );
}
