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

#include "smk_decoder.h"
#include "smacker.h"

SMKVideoSequence::SMKVideoSequence( const std::string & filePath )
    : _width( 0 )
    , _height( 0 )
{
    _load( filePath );
}

bool SMKVideoSequence::_load( const std::string & filePath )
{
    smk videoFile = smk_open_file( filePath.c_str(), SMK_MODE_MEMORY );
    if ( videoFile == NULL )
        return false;

    unsigned long frameCount = 0;
    double frameRate = 0;

    smk_info_all( videoFile, NULL, &frameCount, &frameRate );
    smk_info_video( videoFile, &_width, &_height, NULL );
    smk_enable_video( videoFile, 1 );

    smk_first( videoFile );
    unsigned long currentFrame = 0;
    smk_info_all( videoFile, &currentFrame, NULL, NULL );

    _addNewFrame( smk_get_video( videoFile ), smk_get_palette( videoFile ) );

    for ( currentFrame = 1; currentFrame < frameCount; ++currentFrame ) {
        smk_next( videoFile );

        _addNewFrame( smk_get_video( videoFile ), smk_get_palette( videoFile ) );
    }

    smk_close( videoFile );
    return true;
}

unsigned long SMKVideoSequence::width() const
{
    return _width;
}

unsigned long SMKVideoSequence::height() const
{
    return _height;
}

void SMKVideoSequence::_addNewFrame( const uint8_t * data, const uint8_t * palette )
{
    if ( data == NULL || palette == NULL )
        return;

    Surface surface( data, _width, _height, 1, false, false );

    std::vector<SDL_Color> colors( 256 );
    for ( size_t i = 0; i < colors.size(); ++i ) {
        colors[i].r = *palette++;
        colors[i].g = *palette++;
        colors[i].b = *palette++;
    }

    surface.SetPalette( colors );

    _frames.push_back( surface );
}

const std::vector<Surface> & SMKVideoSequence::get() const
{
    return _frames;
}
