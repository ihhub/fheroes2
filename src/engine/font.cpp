/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include "font.h"
#include "logging.h"
#include "tools.h"

FontPSF::FontPSF( const std::string & filePath, const fheroes2::Size & size )
    : _data( LoadFileToMem( filePath ) )
    , _size( size )
{
    if ( _data.empty() )
        ERROR_LOG( "empty buffer" );
}

fheroes2::Image FontPSF::RenderText( const std::string & text, const uint8_t color ) const
{
    if ( text.empty() )
        return fheroes2::Image();

    fheroes2::Image output( static_cast<int32_t>( text.size() ) * _size.width, _size.height );
    output.reset();

    int32_t posX = 0;

    for ( std::string::const_iterator it = text.begin(); it != text.end(); ++it ) {
        // render char
        int32_t offsetX = ( *it ) * _size.width * _size.height / 8; // bits -> byte

        for ( int32_t y = 0; y < _size.height; ++y ) {
            // It's safe to cast as all values are >= 0
            const size_t offset = static_cast<size_t>( ( y * _size.width / 8 ) + offsetX ); // bits -> byte

            if ( offset < _data.size() ) {
                const int32_t line = _data[offset];
                for ( int32_t x = 0; x < _size.width; ++x ) {
                    if ( 0x80 & ( line << x ) )
                        fheroes2::SetPixel( output, posX + x, y, color );
                }
            }
        }

        posX += _size.width;
    }

    return output;
}
