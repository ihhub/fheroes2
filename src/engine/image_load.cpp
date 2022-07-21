/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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

#include "image_load.h"

#include <SDL_version.h>
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#include <SDL_surface.h>
#else
#include <SDL_video.h>
#endif

namespace fheroes2
{
    bool Load( const std::string & path, Image & image )
    {
        SDL_Surface * surface = SDL_LoadBMP( path.c_str() );
        if ( surface == nullptr ) {
            return false;
        }

        if ( surface->format->BytesPerPixel == 3 ) {
            image.resize( surface->w, surface->h );
            memset( image.transform(), 0, surface->w * surface->h );

            const uint8_t * inY = reinterpret_cast<uint8_t *>( surface->pixels );
            uint8_t * outY = image.image();

            const uint8_t * inYEnd = inY + surface->h * surface->pitch;

            for ( ; inY != inYEnd; inY += surface->pitch, outY += surface->w ) {
                const uint8_t * inX = inY;
                uint8_t * outX = outY;
                const uint8_t * inXEnd = inX + surface->w * 3;

                for ( ; inX != inXEnd; inX += 3, ++outX ) {
                    *outX = GetColorId( *( inX + 2 ), *( inX + 1 ), *inX );
                }
            }
        }
        else if ( surface->format->BytesPerPixel == 4 ) {
            image.resize( surface->w, surface->h );
            image.reset();

            const uint8_t * inY = reinterpret_cast<uint8_t *>( surface->pixels );
            uint8_t * outY = image.image();
            uint8_t * transformY = image.transform();

            const uint8_t * inYEnd = inY + surface->h * surface->pitch;

            for ( ; inY != inYEnd; inY += surface->pitch, outY += surface->w, transformY += surface->w ) {
                const uint8_t * inX = inY;
                uint8_t * outX = outY;
                uint8_t * transformX = transformY;
                const uint8_t * inXEnd = inX + surface->w * 4;

                for ( ; inX != inXEnd; inX += 4, ++outX, ++transformX ) {
                    const uint8_t alpha = *( inX + 3 );
                    if ( alpha < 255 ) {
                        if ( alpha == 0 ) {
                            *transformX = 1;
                        }
                        else if ( *inX == 0 && *( inX + 1 ) == 0 && *( inX + 2 ) == 0 ) {
                            *transformX = 2;
                        }
                        else {
                            *outX = GetColorId( *( inX + 2 ), *( inX + 1 ), *inX );
                            *transformX = 0;
                        }
                    }
                    else {
                        *outX = GetColorId( *( inX + 2 ), *( inX + 1 ), *inX );
                        *transformX = 0;
                    }
                }
            }
        }
        else {
            SDL_FreeSurface( surface );
            return false;
        }

        SDL_FreeSurface( surface );

        return true;
    }
}
