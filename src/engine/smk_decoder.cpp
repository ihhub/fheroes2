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

#include <algorithm>

#include "image.h"
#include "serialize.h"
#include "smacker.h"
#include "smk_decoder.h"

#include <cassert>
#include <cstring>

SMKVideoSequence::SMKVideoSequence( const std::string & filePath )
    : _width( 0 )
    , _height( 0 )
    , _fps( 0 )
    , _frameCount( 0 )
    , _currentFrameId( 0 )
    , _videoFile( nullptr )
{
    _videoFile = smk_open_file( filePath.c_str(), SMK_MODE_MEMORY );
    if ( _videoFile == nullptr )
        return;

    double usf = 0;

    const uint8_t audioChannelCount = 7;

    uint8_t trackMask = 0;
    uint8_t channel[audioChannelCount] = {0};
    uint8_t audioBitDepth[audioChannelCount] = {0};
    unsigned long audioRate[audioChannelCount] = {0};
    std::vector<std::vector<uint8_t> > soundBuffer( audioChannelCount );

    unsigned long width = 0;
    unsigned long height = 0;

    smk_info_all( _videoFile, nullptr, &_frameCount, &usf );
    smk_info_video( _videoFile, &width, &height, nullptr );

    _width = static_cast<int32_t>( width );
    _height = static_cast<int32_t>( height );

    smk_info_audio( _videoFile, &trackMask, channel, audioBitDepth, audioRate );
    smk_enable_video( _videoFile, 1 );

    if ( usf > 0 )
        _fps = 1000000.0 / usf;
    else
        _fps = 15; // let's use as a default

    for ( uint8_t i = 0; i < audioChannelCount; ++i ) {
        if ( trackMask & ( 1 << i ) ) {
            smk_enable_audio( _videoFile, i, 1 );
        }
    }

    smk_enable_video( _videoFile, 0 ); // disable video reading
    smk_first( _videoFile );
    unsigned long currentFrame = 0;
    smk_info_all( _videoFile, &currentFrame, nullptr, nullptr );

    for ( uint8_t i = 0; i < audioChannelCount; ++i ) {
        if ( trackMask & ( 1 << i ) ) {
            const unsigned long length = smk_get_audio_size( _videoFile, i );
            const uint8_t * data = smk_get_audio( _videoFile, i );
            soundBuffer[i].reserve( soundBuffer[i].size() + length );
            soundBuffer[i].insert( soundBuffer[i].end(), data, data + length );
        }
    }

    for ( currentFrame = 1; currentFrame < _frameCount; ++currentFrame ) {
        smk_next( _videoFile );

        for ( uint8_t i = 0; i < audioChannelCount; ++i ) {
            if ( trackMask & ( 1 << i ) ) {
                const unsigned long length = smk_get_audio_size( _videoFile, i );
                const uint8_t * data = smk_get_audio( _videoFile, i );
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
            std::vector<uint8_t> & wavData = _audioChannel[channelCount];
            ++channelCount;

            // set up WAV header
            StreamBuf wavHeader( 44 );
            wavHeader.putLE32( 0x46464952 ); // RIFF
            wavHeader.putLE32( static_cast<uint32_t>( soundBuffer[i].size() ) + 0x24 ); // size
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
            wavHeader.putLE32( static_cast<uint32_t>( soundBuffer[i].size() ) ); // size

            wavData.resize( soundBuffer[i].size() + 44 );
            wavData.assign( wavHeader.data(), wavHeader.data() + 44 );
            wavData.insert( wavData.begin() + 44, soundBuffer[i].begin(), soundBuffer[i].end() );
        }
    }

    smk_enable_video( _videoFile, 1 ); // enable video reading
    smk_first( _videoFile );
}

SMKVideoSequence::~SMKVideoSequence()
{
    if ( _videoFile != nullptr ) {
        smk_close( _videoFile );
    }
}

int32_t SMKVideoSequence::width() const
{
    return _width;
}

int32_t SMKVideoSequence::height() const
{
    return _height;
}

double SMKVideoSequence::fps() const
{
    return _fps;
}

unsigned long SMKVideoSequence::frameCount() const
{
    return _frameCount;
}

void SMKVideoSequence::resetFrame()
{
    if ( _videoFile == nullptr )
        return;

    smk_first( _videoFile );
    _currentFrameId = 0;
}

void SMKVideoSequence::getNextFrame( fheroes2::Image & image, const int32_t x, const int32_t y, int32_t & width, int32_t & height, std::vector<uint8_t> & palette )
{
    if ( _videoFile == nullptr || image.empty() || x < 0 || y < 0 || x >= image.width() || y >= image.height() || !image.singleLayer() ) {
        width = 0;
        height = 0;
        return;
    }

    const uint8_t * data = smk_get_video( _videoFile );
    const uint8_t * paletteData = smk_get_palette( _videoFile );

    width = _width;
    height = _height;

    if ( image.width() == _width && image.height() == _height && x == 0 && y == 0 ) {
        const size_t size = static_cast<size_t>( _width ) * _height;
        std::copy( data, data + size, image.image() );
    }
    else {
        if ( x + width > image.width() ) {
            width = image.width() - x;
        }
        if ( y + height > image.height() ) {
            height = image.height() - y;
        }

        const uint8_t * inY = data;

        const int32_t imageWidth = image.width();
        uint8_t * outY = image.image() + x + y * imageWidth;
        const uint8_t * outYEnd = outY + height * imageWidth;

        for ( ; outY != outYEnd; outY += imageWidth, inY += _width ) {
            std::copy( inY, inY + width, outY );
        }
    }

    palette.resize( 256 * 3 );
    memcpy( palette.data(), paletteData, 256 * 3 );

    ++_currentFrameId;
    if ( _currentFrameId < _frameCount ) {
        smk_next( _videoFile );
    }
}

std::vector<uint8_t> SMKVideoSequence::getCurrentPalette() const
{
    const uint8_t * paletteData = smk_get_palette( _videoFile );
    assert( paletteData != nullptr );

    return std::vector<uint8_t>( paletteData, paletteData + 256 * 3 );
}

const std::vector<std::vector<uint8_t> > & SMKVideoSequence::getAudioChannels() const
{
    return _audioChannel;
}
