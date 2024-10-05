/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

#include "exception.h"
#include "image.h"
#include "serialize.h"
#include "smacker.h"

namespace
{
    const size_t audioHeaderSize = 44;

    void verifyVideoFile( const std::string & filePath )
    {
        if ( filePath.empty() ) {
            // Why are you trying to even play a file from an empty path?
            assert( 0 );
            return;
        }

        // Verify that the file is valid. We use C-code on purpose since libsmacker library does the same.
        const std::unique_ptr<std::FILE, int ( * )( std::FILE * )> file( std::fopen( filePath.c_str(), "rb" ), []( std::FILE * f ) { return std::fclose( f ); } );
        if ( !file ) {
            return;
        }

        std::fseek( file.get(), 0, SEEK_END );
        const size_t fileSize = std::ftell( file.get() );

        // According to https://wiki.multimedia.cx/index.php/Smacker the minimum size of a file must be 56 bytes.
        if ( fileSize < 56 ) {
            throw fheroes2::InvalidDataResources( "Video file " + filePath + " is being corrupted. Make sure that you own an official version of the game." );
        }
    }
}

SMKVideoSequence::SMKVideoSequence( const std::string & filePath )
    : _width( 0 )
    , _height( 0 )
    , _heightScaleFactor( 1 )
    , _fps( 0 )
    , _frameCount( 0 )
    , _currentFrameId( 0 )
    , _videoFile( nullptr )
{
    verifyVideoFile( filePath );

    _videoFile = smk_open_file( filePath.c_str(), SMK_MODE_MEMORY );
    if ( _videoFile == nullptr )
        return;

    double usf = 0;

    const uint8_t audioChannelCount = 7;

    uint8_t trackMask = 0;
    uint8_t channelsPerTrack[audioChannelCount] = { 0 };
    uint8_t audioBitDepth[audioChannelCount] = { 0 };
    unsigned long audioRate[audioChannelCount] = { 0 };
    std::array<std::vector<uint8_t>, audioChannelCount> soundBuffer;

    unsigned long width = 0;
    unsigned long height = 0;
    unsigned char scaledYMode = 1;

    smk_info_all( _videoFile, nullptr, &_frameCount, &usf );
    smk_info_video( _videoFile, &width, &height, &scaledYMode );

    _heightScaleFactor = scaledYMode;

    if ( _heightScaleFactor < 1 ) {
        // This is some corrupted video file. Let's still proceed with it.
        _heightScaleFactor = 1;
    }
    else if ( _heightScaleFactor > 2 ) {
        // None of formats supports scaling more than 2.
        _heightScaleFactor = 2;
    }

    _width = static_cast<int32_t>( width );
    _height = static_cast<int32_t>( height ) * _heightScaleFactor;

    smk_info_audio( _videoFile, &trackMask, channelsPerTrack, audioBitDepth, audioRate );

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
            if ( length == 0 ) {
                continue;
            }

            if ( soundBuffer[i].empty() ) {
                soundBuffer[i].resize( audioHeaderSize );
            }

            const uint8_t * data = smk_get_audio( _videoFile, i );
            soundBuffer[i].insert( soundBuffer[i].end(), data, data + length );
        }
    }

    for ( currentFrame = 1; currentFrame < _frameCount; ++currentFrame ) {
        smk_next( _videoFile );

        for ( uint8_t i = 0; i < audioChannelCount; ++i ) {
            if ( trackMask & ( 1 << i ) ) {
                const unsigned long length = smk_get_audio_size( _videoFile, i );
                if ( length == 0 ) {
                    continue;
                }

                if ( soundBuffer[i].empty() ) {
                    soundBuffer[i].resize( audioHeaderSize );
                }

                const uint8_t * data = smk_get_audio( _videoFile, i );
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

    // Compose the soundtrack
    for ( size_t i = 0; i < soundBuffer.size(); ++i ) {
        if ( !soundBuffer[i].empty() ) {
            std::vector<uint8_t> & wavData = _audioChannel[channelCount];
            std::swap( wavData, soundBuffer[i] );

            const uint32_t originalSize = static_cast<uint32_t>( wavData.size() - audioHeaderSize );

            ++channelCount;

            RWStreamBuf wavHeader( audioHeaderSize );
            wavHeader.putLE32( 0x46464952 ); // RIFF marker ("RIFF")
            wavHeader.putLE32( originalSize + 0x24 ); // Total size minus the size of this and previous fields
            wavHeader.putLE32( 0x45564157 ); // File type header ("WAVE")
            wavHeader.putLE32( 0x20746D66 ); // Format sub-chunk marker ("fmt ")
            wavHeader.putLE32( 0x10 ); // Size of the format sub-chunk
            wavHeader.putLE16( 0x01 ); // Audio format (1 for PCM)
            wavHeader.putLE16( channelsPerTrack[i] ); // Number of channels
            wavHeader.putLE32( audioRate[i] ); // Sample rate
            wavHeader.putLE32( audioRate[i] * audioBitDepth[i] * channelsPerTrack[i] / 8 ); // Byte rate
            wavHeader.putLE16( audioBitDepth[i] * channelsPerTrack[i] / 8 ); // Block align
            wavHeader.putLE16( audioBitDepth[i] ); // Bits per sample
            wavHeader.putLE32( 0x61746164 ); // Data sub-chunk marker ("data")
            wavHeader.putLE32( originalSize ); // Size of the data sub-chunk

            memcpy( wavData.data(), wavHeader.data(), audioHeaderSize );
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

    assert( _heightScaleFactor == 1 || _heightScaleFactor == 2 );
    assert( ( _height % _heightScaleFactor ) == 0 );

    if ( image.width() == _width && image.height() == _height && x == 0 && y == 0 ) {
        const size_t size = static_cast<size_t>( _width ) * _height;

        if ( _heightScaleFactor == 2 ) {
            assert( ( height % _heightScaleFactor ) == 0 );

            const uint8_t * inY = data;
            uint8_t * outY = image.image();
            const uint8_t * outYEnd = outY + height * _width;

            for ( ; outY != outYEnd; outY += _heightScaleFactor * _width, inY += _width ) {
                std::copy( inY, inY + width, outY );
            }
        }
        else {
            std::copy( data, data + size, image.image() );
        }
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
        const uint8_t * outYEnd = outY + ( height / _heightScaleFactor ) * _heightScaleFactor * imageWidth;

        if ( _heightScaleFactor == 2 ) {
            assert( ( height % _heightScaleFactor ) == 0 );

            for ( ; outY != outYEnd; outY += _heightScaleFactor * imageWidth, inY += _width ) {
                std::copy( inY, inY + width, outY );
            }
        }
        else {
            for ( ; outY != outYEnd; outY += imageWidth, inY += _width ) {
                std::copy( inY, inY + width, outY );
            }
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

const std::vector<std::vector<uint8_t>> & SMKVideoSequence::getAudioChannels() const
{
    return _audioChannel;
}
