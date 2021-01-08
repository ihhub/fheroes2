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

#include "image_tool.h"
#include "palette_h2.h"

#include <SDL_version.h>
#include <SDL_video.h>

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#include <SDL_image.h>
#endif

namespace
{
    std::vector<uint8_t> PALPAlette()
    {
        std::vector<uint8_t> palette;
        if ( palette.empty() ) {
            palette.resize( 256 * 3 );
            for ( size_t i = 0; i < palette.size(); ++i ) {
                palette[i] = kb_pal[i] << 2;
            }
        }

        return palette;
    }

    bool SaveImage( const fheroes2::Image & image, const std::string & path )
    {
        const std::vector<uint8_t> & palette = PALPAlette();
        const uint8_t * currentPalette = palette.data();

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        SDL_Surface * surface = SDL_CreateRGBSurface( 0, image.width(), image.height(), 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000 );
#else
        SDL_Surface * surface = SDL_CreateRGBSurface( SDL_SWSURFACE, image.width(), image.height(), 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000 );
#endif
        if ( surface == NULL )
            return false;

        const uint32_t width = image.width();
        const uint32_t height = image.height();

        uint32_t * out = static_cast<uint32_t *>( surface->pixels );
        const uint32_t * outEnd = out + width * height;
        const uint8_t * in = image.image();

        if ( surface->format->Amask > 0 ) {
            for ( ; out != outEnd; ++out, ++in ) {
                const uint8_t * value = currentPalette + *in * 3;
                *out = SDL_MapRGBA( surface->format, *( value ), *( value + 1 ), *( value + 2 ), 255 );
            }
        }
        else {
            for ( ; out != outEnd; ++out, ++in ) {
                const uint8_t * value = currentPalette + *in * 3;
                *out = SDL_MapRGB( surface->format, *( value ), *( value + 1 ), *( value + 2 ) );
            }
        }

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        int res = 0;
        const std::string pngExtension(".png");
        if ( path.size() > pngExtension.size() && path.compare( path.size() - pngExtension.size(), pngExtension.size(), pngExtension ) ) {
            res = IMG_SavePNG( surface, path.c_str() );
        }
        else {
            res = SDL_SaveBMP( surface, path.c_str() );
        }
#else
        const int res = SDL_SaveBMP( surface, path.c_str() );
#endif

        SDL_FreeSurface( surface );

        return res == 0;
    }
}

namespace fheroes2
{
    bool Save( const Image & image, const std::string & path, uint8_t background )
    {
        if ( image.empty() || path.empty() )
            return false;

        Image temp( image.width(), image.height() );
        temp.fill( background );

        Blit( image, temp );

        return SaveImage( temp, path );
    }
}
