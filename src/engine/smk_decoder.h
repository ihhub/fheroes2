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

#pragma once

#include <string>
#include <vector>

struct smk_t;

namespace fheroes2
{
    class Image;
}

class SMKVideoSequence
{
public:
    explicit SMKVideoSequence( const std::string & filePath );
    ~SMKVideoSequence();

    SMKVideoSequence( const SMKVideoSequence & ) = delete;
    SMKVideoSequence & operator=( const SMKVideoSequence & ) = delete;

    void resetFrame();

    // Input image must be resized to accomodate the frame and also it must be a single layer image as video frames shouldn't have any transform-related information.
    // If the image is smaller than the frame then only a part of the frame will be drawn.
    void getNextFrame( fheroes2::Image & image, const int32_t x, const int32_t y, int32_t & width, int32_t & height, std::vector<uint8_t> & palette );

    std::vector<uint8_t> getCurrentPalette() const;

    const std::vector<std::vector<uint8_t> > & getAudioChannels() const;

    int32_t width() const;
    int32_t height() const;
    double fps() const;
    unsigned long frameCount() const;

private:
    std::vector<std::vector<uint8_t> > _audioChannel;
    int32_t _width;
    int32_t _height;
    double _fps;
    unsigned long _frameCount;
    unsigned long _currentFrameId;

    struct smk_t * _videoFile;
};
