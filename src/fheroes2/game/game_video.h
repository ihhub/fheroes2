/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2022                                             *
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

#include "game_video_type.h"
#include "image.h"
#include "math_base.h"
#include "ui_text.h"

namespace Video
{
    class Subtitle
    {
    public:
        Subtitle() = default;
        Subtitle( const fheroes2::Text & subtitleText, const int32_t maxWidth, const int32_t offsetX, const int32_t offsetY, const uint32_t startFrame,
                  const uint32_t endFrame = UINT32_MAX );
        ~Subtitle() = default;

        // Add text string to subtitles.
        void addText( const fheroes2::Text & subtitleText )
        {
            _text.add( subtitleText );
        }
        
        // Generate the image from subtitles text and store it in Subtitle class.
        void makeSubtitleImage();

        // Render subtitles image to the output image.
        void blitSubtitles( fheroes2::Image & output, const fheroes2::Rect & frameRoi );

        // Check if subtitles needs to be rendered on this frame number.
        bool needRender( const uint32_t frameNumber )
        {
            return ( frameNumber >= _startFrame ) && ( frameNumber <= _endFrame );
        }

    private:
        fheroes2::Point _position{ 0, 0 };
        fheroes2::MultiFontText _text;
        uint32_t _startFrame{ 0 };
        uint32_t _endFrame{ UINT32_MAX };
        int32_t _maxWidth{ 640 };
        fheroes2::Image _subtitleImage;
    };

    // Returns true if the file exists.
    bool getVideoFilePath( const std::string & fileName, std::string & path );

    // Returns false if the video is not present or it is corrupted.
    bool ShowVideo( const std::string & fileName, const VideoAction action, std::vector<Subtitle> * subtitles = nullptr, const bool fadeColorsOnEnd = false );

    bool ShowVideo( const std::string & fileName, const VideoAction action, const bool fadeColorsOnEnd /* = false */ );
}
