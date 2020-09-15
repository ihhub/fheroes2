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

#include "serialize.h"
#include "smacker.h"
#include "smk_decoder.h"

SMKVideoSequence::SMKVideoSequence( const std::string & filePath )
    : _width( 0 )
    , _height( 0 )
    , _fps( 0 )
    , _frameCount( 0 )
    , _currentFrameId( 0 )
    , _videoFile( NULL )
{
    _videoFile = smk_open_file( filePath.c_str(), SMK_MODE_MEMORY );
    if ( _videoFile == NULL )
        return;

    double usf = 0;

    const uint8_t audioChannelCount = 7;

    uint8_t trackMask = 0;
    uint8_t channel[audioChannelCount] = {0};
    uint8_t audioBitDepth[audioChannelCount] = {0};
    unsigned long audioRate[audioChannelCount] = {0};
    std::vector<std::vector<uint8_t> > soundBuffer( audioChannelCount );

    smk_info_all( _videoFile, NULL, &_frameCount, &usf );
    smk_info_video( _videoFile, &_width, &_height, NULL );
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

    smk_first( _videoFile );
    unsigned long currentFrame = 0;
    smk_info_all( _videoFile, &currentFrame, NULL, NULL );

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
            std::vector<uint8_t> & wavData = _audioChannel[channelCount++];

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

    smk_first( _videoFile );
}

SMKVideoSequence::~SMKVideoSequence()
{
    if ( _videoFile != NULL ) {
        smk_close( _videoFile );
    }
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

unsigned long SMKVideoSequence::frameCount() const
{
    return _frameCount;
}

void SMKVideoSequence::resetFrame()
{
    if ( _videoFile == NULL )
        return;

    smk_first( _videoFile );
    _currentFrameId = 0;
}

void SMKVideoSequence::getNextFrame( fheroes2::Image & image, std::vector<uint8_t> & palette )
{
    if ( _videoFile == NULL )
        return;

    image.resize( _width, _height );

    const uint8_t * data = smk_get_video( _videoFile );
    const uint8_t * paletteData = smk_get_palette( _videoFile );

    const size_t size = static_cast<size_t>( _width ) * _height;
    std::copy( data, data + size, image.image() );
    std::fill( image.transform(), image.transform() + size, 0 );

    palette.resize( 256 * 3 );
    memcpy( palette.data(), paletteData, 256 * 3 );

    ++_currentFrameId;
    if ( _currentFrameId < _frameCount ) {
        smk_next( _videoFile );
    }
}

const std::vector<std::vector<uint8_t> > & SMKVideoSequence::getAudioChannels() const
{
    return _audioChannel;
}
