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

    void resetFrame();
    void getNextFrame( fheroes2::Image & image, std::vector<uint8_t> & palette );
    const std::vector<std::vector<uint8_t> > & getAudioChannels() const;

    unsigned long width() const;
    unsigned long height() const;
    double fps() const;
    unsigned long frameCount() const;

private:
    std::vector<std::vector<uint8_t> > _audioChannel;
    unsigned long _width;
    unsigned long _height;
    double _fps;
    unsigned long _frameCount;
    unsigned long _currentFrameId;

    struct smk_t * _videoFile;
};
