/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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
#include "ui_text.h"

namespace Video
{
    class Subtitle
    {
    public:
        // Generate the image from subtitles text and store it in Subtitle class.
        Subtitle( const fheroes2::TextBase & subtitleText, const uint32_t startTimeMS, const uint32_t durationMS = UINT32_MAX,
                  const int32_t maxWidth = fheroes2::Display::DEFAULT_WIDTH );

        // Set subtitle position on screen relative to the top left corner of video image.
        void setPosition( const fheroes2::Point position )
        {
            _position = position;
        }

        uint32_t getStartFrame( const uint32_t renderDelayMS )
        {
            return ( _startTimeMS / renderDelayMS );
        }

        // Check if subtitles needs to be rendered at the current time (in milliseconds).
        bool needRender( const uint32_t currentTimeMS ) const
        {
            return ( ( currentTimeMS >= _startTimeMS ) && ( currentTimeMS < ( _startTimeMS + _durationMS ) ) );
        }

        // Render subtitles image to the output image.
        void blitSubtitles( fheroes2::Image & output, const fheroes2::Rect & frameRoi ) const;

    private:
        fheroes2::Point _position{ 0, 0 };
        fheroes2::Image _subtitleImage;
        uint32_t _startTimeMS{ 0 };
        uint32_t _durationMS{ UINT32_MAX };
    };

    // Returns true if the file exists.
    bool getVideoFilePath( const std::string & fileName, std::string & path );

    // Returns false if the video is not present or it is corrupted.
    bool ShowVideo( const std::string & fileName, const VideoAction action, std::vector<Subtitle> & subtitles, const bool fadeColorsOnEnd = false );

    bool ShowVideo( const std::string & fileName, const VideoAction action, const bool fadeColorsOnEnd = false );
}
