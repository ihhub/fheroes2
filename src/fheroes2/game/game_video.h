/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2025                                             *
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

#include <cstdint>
#include <string>
#include <vector>

#include "game_video_type.h"
#include "image.h"
#include "math_base.h"
#include "screen.h"

namespace fheroes2
{
    class TextBase;
}

namespace Video
{
    struct VideoInfo final
    {
        std::string fileName;
        VideoControl control{ VideoControl::PLAY_NONE };
        fheroes2::Point offset{ 0, 0 };
    };

    class Subtitle
    {
    public:
        // Generate the image from subtitles text and store it in Subtitle class.
        // The 'position' represents the top-center of subtitles image. All subtitles are center aligned.
        Subtitle( const fheroes2::TextBase & subtitleText, const uint32_t startTimeMS, const uint32_t durationMS, const fheroes2::Point & position = { -1, -1 },
                  const int32_t maxWidth = fheroes2::Display::DEFAULT_WIDTH );

        // Check if subtitles need to be rendered at the current time (in milliseconds).
        bool needRender( const uint32_t currentTimeMS ) const
        {
            return ( ( currentTimeMS >= _startTimeMS ) && ( currentTimeMS < _endTimeMS ) );
        }

        // Render subtitles image to the output image.
        void render( fheroes2::Image & output, const fheroes2::Rect & frameRoi ) const
        {
            fheroes2::Blit( _subtitleImage, 0, 0, output, frameRoi.x + _position.x, frameRoi.y + _position.y, _subtitleImage.width(), _subtitleImage.height() );
        }

    private:
        fheroes2::Point _position{ -1, -1 };
        fheroes2::Image _subtitleImage;
        const uint32_t _startTimeMS{ 0 };
        uint32_t _endTimeMS{ 0 };
    };

    // Returns true if the file exists.
    bool getVideoFilePath( const std::string & fileName, std::string & path );

    // Returns false if some videos are not present, or they are corrupted.
    bool ShowVideo( const std::vector<VideoInfo> & infos, const std::vector<Subtitle> & subtitles = {}, const bool fadeColorsOnEnd = false );
}
