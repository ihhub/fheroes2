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
#include "serialize.h"
#include "smacker.h"

SMKVideoSequence::SMKVideoSequence( const std::string & filePath )
    : _width( 0 )
    , _height( 0 )
    , _fps( 0 )
{
    _load( filePath );
}

bool SMKVideoSequence::_load( const std::string & filePath )
{
    smk videoFile = smk_open_file( filePath.c_str(), SMK_MODE_MEMORY );
    if ( videoFile == NULL )
        return false;

    unsigned long frameCount = 0;
    double usf = 0;

    uint8_t trackMask = 0;
    uint8_t channel[7] = {0};
    uint8_t audioBitDepth[7] = {0};
    unsigned long audioRate[7] = {0};
    std::vector<std::vector<uint8_t> > soundBuffer( 7 );

    smk_info_all( videoFile, NULL, &frameCount, &usf );
    smk_info_video( videoFile, &_width, &_height, NULL );
    smk_info_audio( videoFile, &trackMask, channel, audioBitDepth, audioRate );
    smk_enable_video( videoFile, 1 );

    if ( usf > 0 )
        _fps = 1000000.0 / usf;
    else
        _fps = 15; // let's use as a default

    for ( int i = 0; i < 7; ++i ) {
        if ( trackMask & ( 1 << i ) ) {
            smk_enable_audio( videoFile, i, 1 );
        }
    }

    smk_first( videoFile );
    unsigned long currentFrame = 0;
    smk_info_all( videoFile, &currentFrame, NULL, NULL );

    for ( size_t i = 0; i < soundBuffer.size(); ++i ) {
        if ( trackMask & ( 1 << i ) ) {
            const unsigned long length = smk_get_audio_size( videoFile, i );
            const uint8_t * data = smk_get_audio( videoFile, i );
            soundBuffer[i].reserve( soundBuffer[i].size() + length );
            soundBuffer[i].insert( soundBuffer[i].end(), data, data + length );
        }
    }

    _addNewFrame( smk_get_video( videoFile ), smk_get_palette( videoFile ) );

    for ( currentFrame = 1; currentFrame < frameCount; ++currentFrame ) {
        smk_next( videoFile );

        _addNewFrame( smk_get_video( videoFile ), smk_get_palette( videoFile ) );

        for ( size_t i = 0; i < soundBuffer.size(); ++i ) {
            if ( trackMask & ( 1 << i ) ) {
                const unsigned long length = smk_get_audio_size( videoFile, i );
                const uint8_t * data = smk_get_audio( videoFile, i );
                soundBuffer[i].reserve( soundBuffer[i].size() + length );
                soundBuffer[i].insert( soundBuffer[i].end(), data, data + length );
            }
        }
    }

    size_t channelCount = 0;
    for ( size_t i = 0; i < soundBuffer.size(); ++i ) {
        if ( !soundBuffer[i].empty() ) {
            ++channelCount;
        }
    }
    _audioChannel.resize( channelCount );

    channelCount = 0;
    // compose sound track
    for ( size_t i = 0; i < soundBuffer.size(); ++i ) {
        if ( !soundBuffer[i].empty() ) {
            std::vector<uint8_t> & wavData = _audioChannel[channelCount++];

            // set up WAV header
            StreamBuf wavHeader( 44 );
            wavHeader.putLE32( 0x46464952 ); // RIFF
            wavHeader.putLE32( soundBuffer[i].size() + 0x24 ); // size
            wavHeader.putLE32( 0x45564157 ); // WAVE
            wavHeader.putLE32( 0x20746D66 ); // FMT
            wavHeader.putLE32( 0x10 ); // 16 == PCM
            wavHeader.putLE16( 0x01 ); // format
            wavHeader.putLE16( 0x01 ); // channels
            wavHeader.putLE32( audioRate[i] ); // samples
            wavHeader.putLE32( audioRate[i] * audioBitDepth[i] / 8 ); // byteper
            wavHeader.putLE16( 0x01 ); // align
            wavHeader.putLE16( audioBitDepth[i] ); // bitsper
            wavHeader.putLE32( 0x61746164 ); // DATA
            wavHeader.putLE32( soundBuffer[i].size() ); // size

            wavData.resize( soundBuffer[i].size() + 44 );
            wavData.assign( wavHeader.data(), wavHeader.data() + 44 );
            wavData.insert( wavData.begin() + 44, soundBuffer[i].begin(), soundBuffer[i].end() );
        }
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

double SMKVideoSequence::fps() const
{
    return _fps;
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

const std::vector<Surface> & SMKVideoSequence::getFrames() const
{
    return _frames;
}

const std::vector<std::vector<uint8_t> > & SMKVideoSequence::getAudioChannels() const
{
    return _audioChannel;
}
