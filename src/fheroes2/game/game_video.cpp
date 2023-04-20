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

#include "game_video.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <vector>

#include "audio.h"
#include "cursor.h"
#include "dir.h"
#include "game_delays.h"
#include "image_palette.h"
#include "localevent.h"
#include "logging.h"
#include "screen.h"
#include "settings.h"
#include "smk_decoder.h"
#include "system.h"
#include "timing.h"
#include "ui_text.h"
#include "ui_tool.h"

namespace
{
    // Anim2 directory is used in Russian Buka version of the game.
    std::array<std::string, 4> videoDir = { "anim", "anim2", System::concatPath( "heroes2", "anim" ), "data" };

    void playAudio( const std::vector<std::vector<uint8_t>> & audioChannels )
    {
        Mixer::setVolume( -1, 100 * Settings::Get().SoundVolume() / 10 );

        for ( const std::vector<uint8_t> & audio : audioChannels ) {
            if ( !audio.empty() ) {
                Mixer::Play( audio.data(), static_cast<uint32_t>( audio.size() ), -1, false );
            }
        }
    }

    void colorFade( const std::vector<uint8_t> & palette, const fheroes2::Rect & frameRoi, const uint32_t durationMs, const uint32_t fps )
    {
        // Do a color fade only for valid palette.
        if ( palette.size() != 768 ) {
            return;
        }

        // The biggest problem here is that the palette can be not the same as in the game.
        // Since we want to do color fading to gray-scale colors we take the original palette and
        // find the nearest colors in video's palette to gray-scale colors of the original palette.
        // Then we gradually change the current palette to be only gray-scale and after reaching the
        // last frame change all colors of the frame to be only gray-scale colors of the original palette.

        const uint8_t * originalPalette = fheroes2::getGamePalette();

        // Yes, these values are hardcoded. There are ways to do it programmatically.
        const int32_t startGrayScaleColorId = 10;
        const int32_t endGrayScaleColorId = 36;

        std::array<uint8_t, 256> assignedValue{ 0 };

        for ( size_t id = 0; id < 256; ++id ) {
            int32_t nearestDistance = INT32_MAX;

            for ( uint8_t colorId = startGrayScaleColorId; colorId <= endGrayScaleColorId; ++colorId ) {
                const int32_t redDiff = static_cast<int32_t>( palette[id * 3] ) - static_cast<int32_t>( originalPalette[static_cast<size_t>( colorId ) * 3] ) * 4;
                const int32_t greenDiff
                    = static_cast<int32_t>( palette[id * 3 + 1] ) - static_cast<int32_t>( originalPalette[static_cast<size_t>( colorId ) * 3 + 1] ) * 4;
                const int32_t blueDiff
                    = static_cast<int32_t>( palette[id * 3 + 2] ) - static_cast<int32_t>( originalPalette[static_cast<size_t>( colorId ) * 3 + 2] ) * 4;

                const int32_t distance = redDiff * redDiff + greenDiff * greenDiff + blueDiff * blueDiff;
                if ( nearestDistance > distance ) {
                    nearestDistance = distance;
                    assignedValue[id] = colorId;
                }
            }
        }

        std::array<uint8_t, 768> endPalette{ 0 };
        for ( size_t i = 0; i < 256; ++i ) {
            // Red color.
            endPalette[i * 3] = originalPalette[static_cast<size_t>( assignedValue[i] ) * 3] * 4;
            // Green color.
            endPalette[i * 3 + 1] = originalPalette[static_cast<size_t>( assignedValue[i] ) * 3 + 1] * 4;
            // Blue color.
            endPalette[i * 3 + 2] = originalPalette[static_cast<size_t>( assignedValue[i] ) * 3 + 2] * 4;
        }

        // Gradually fade the palette.
        const uint32_t delay = 1000 / fps;
        const uint32_t gradingSteps = durationMs / delay;

        std::vector<uint8_t> gradingPalette( 768 );
        std::vector<uint8_t> prevPalette( 768 );

        const fheroes2::ScreenPaletteRestorer screenRestorer;
        fheroes2::Display & display = fheroes2::Display::instance();

        for ( uint32_t gradingId = 1; gradingId < gradingSteps; ++gradingId ) {
            for ( int32_t i = 0; i < 768; ++i ) {
                gradingPalette[i] = static_cast<uint8_t>( ( palette[i] * ( gradingSteps - gradingId ) + endPalette[i] * gradingId ) / gradingSteps );
            }

            screenRestorer.changePalette( gradingPalette.data() );
            // We need to swap the palettes so the next call of 'changePalette()' will think that the palette is new.
            std::swap( prevPalette, gradingPalette );

            display.render( frameRoi );

            fheroes2::delayforMs( delay );
        }

        // Convert all video frame colors to original game palette colors.
        for ( size_t id = 0; id < 256; ++id ) {
            // TODO: Modify the ReplaceColorId to replace colors only in ROI.
            fheroes2::ReplaceColorId( display, static_cast<uint8_t>( id ), assignedValue[id] );
        }

        screenRestorer.changePalette( originalPalette );
    }
}

namespace Video
{
    bool getVideoFilePath( const std::string & fileName, std::string & path )
    {
        for ( const std::string & rootDir : Settings::GetRootDirs() ) {
            for ( size_t dirIdx = 0; dirIdx < videoDir.size(); ++dirIdx ) {
                const std::string fullDirPath = System::concatPath( rootDir, videoDir[dirIdx] );

                if ( System::IsDirectory( fullDirPath ) ) {
                    ListFiles videoFiles;
                    videoFiles.FindFileInDir( fullDirPath, fileName, false );
                    if ( videoFiles.empty() ) {
                        continue;
                    }

                    std::swap( videoFiles.front(), path );

                    if ( dirIdx > 0 ) {
                        // Put the current directory at the first place to increase cache hit chance.
                        std::swap( videoDir[0], videoDir[dirIdx] );
                    }

                    return true;
                }
            }
        }

        return false;
    }

    bool ShowVideo( const std::string & fileName, const VideoAction action, const bool fadeColorsOnEnd /* = false */ )
    {
        // The video without subtitles will be rendered so create an empty subtitles vector.
        std::vector<Subtitle> subtitles;
        return ShowVideo( fileName, action, subtitles, fadeColorsOnEnd );
    }

    bool ShowVideo( const std::string & fileName, const VideoAction action, std::vector<Subtitle> & subtitles, const bool fadeColorsOnEnd /* = false */ )
    {
        // Stop any cycling animation.
        const fheroes2::ScreenPaletteRestorer screenRestorer;

        std::string videoPath;
        if ( !getVideoFilePath( fileName, videoPath ) ) {
            // File doesn't exist, so no need to even try to load it.
            DEBUG_LOG( DBG_GAME, DBG_INFO, fileName << " video file does not exist." )
            return false;
        }

        SMKVideoSequence video( videoPath );

        const uint32_t frameCount = video.frameCount();

        if ( frameCount < 1 ) {
            // Nothing to show.
            return false;
        }

        const std::vector<std::vector<uint8_t>> & audioChannels = video.getAudioChannels();
        const bool hasAudio = Audio::isValid() && !audioChannels.empty();
        if ( action == VideoAction::IGNORE_VIDEO ) {
            // Since no video is rendered play audio if available.
            if ( hasAudio ) {
                playAudio( audioChannels );
            }

            return true;
        }

        const bool isLooped = ( action == VideoAction::LOOP_VIDEO || action == VideoAction::PLAY_TILL_AUDIO_END );

        // Hide mouse cursor.
        const CursorRestorer cursorRestorer( false, Cursor::Get().Themes() );

        fheroes2::Display & display = fheroes2::Display::instance();
        display.fill( 0 );
        display.updateNextRenderRoi( { 0, 0, display.width(), display.height() } );

        uint32_t currentFrame = 0;
        fheroes2::Rect frameRoi( ( display.width() - video.width() ) / 2, ( display.height() - video.height() ) / 2, 0, 0 );

        const uint32_t delay = static_cast<uint32_t>( 1000.0 / video.fps() + 0.5 ); // This might be not very accurate but it's the best we can have now

        std::vector<uint8_t> palette;
        std::vector<uint8_t> prevPalette;

        // Prepare the first frame.
        video.resetFrame();
        video.getNextFrame( display, frameRoi.x, frameRoi.y, frameRoi.width, frameRoi.height, prevPalette );
        screenRestorer.changePalette( prevPalette.data() );

        LocalEvent & le = LocalEvent::Get();

        Game::passCustomAnimationDelay( delay );
        // Make sure that the first run is passed immediately.
        assert( !Game::isCustomDelayNeeded( delay ) );

        // Play audio just before rendering the frame. This is important to minimize synchronization issues between audio and video.
        if ( hasAudio ) {
            playAudio( audioChannels );
        }

        while ( le.HandleEvents( Game::isCustomDelayNeeded( delay ) ) ) {
            if ( ( action == VideoAction::PLAY_TILL_AUDIO_END ) && !Mixer::isPlaying( -1 ) ) {
                break;
            }

            if ( le.KeyPress() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() ) {
                Mixer::Stop();
                break;
            }

            if ( Game::validateCustomAnimationDelay( delay ) ) {
                if ( currentFrame < frameCount ) {
                    // Render the prepared frame.
                    display.render( frameRoi );

                    ++currentFrame;

                    if ( ( currentFrame == frameCount ) && isLooped ) {
                        currentFrame = 0;
                        video.resetFrame();

                        if ( hasAudio ) {
                            playAudio( audioChannels );
                        }
                    }

                    // Prepare the next frame for render.
                    video.getNextFrame( display, frameRoi.x, frameRoi.y, frameRoi.width, frameRoi.height, palette );

                    if ( prevPalette != palette ) {
                        screenRestorer.changePalette( palette.data() );
                        std::swap( prevPalette, palette );
                    }

                    for ( const Video::Subtitle & subtitle : subtitles ) {
                        if ( subtitle.needRender( currentFrame * delay ) ) {
                            subtitle.blitSubtitles( display, frameRoi );
                        }
                    }
                }
                else if ( action != VideoAction::WAIT_FOR_USER_INPUT ) {
                    break;
                }
            }
        }

        if ( fadeColorsOnEnd ) {
            colorFade( palette, frameRoi, 1500, 8 );
        }
        else {
            display.fill( 0 );
        }
        display.updateNextRenderRoi( { 0, 0, display.width(), display.height() } );

        return true;
    }

    Subtitle::Subtitle( const fheroes2::TextBase & subtitleText, const uint32_t startTimeMS, const uint32_t durationMS /* = UINT32_MAX */,
                        const int32_t maxWidth /* = fheroes2::Display::DEFAULT_WIDTH */ )
        : _startTimeMS( startTimeMS )
    {
        const int32_t textWidth = subtitleText.width( ( maxWidth > 0 ) ? maxWidth : fheroes2::Display::DEFAULT_WIDTH );
        // We add extra 1 to have space for contour.
        _subtitleImage.resize( textWidth + 1, subtitleText.height( textWidth ) + 1 );
        _subtitleImage.reset();

        // Draw text and remove all shadow data is it could not be properly applied to video palette.
        // We use the black color with id = 36 so no shadow will be applied to it.
        const uint8_t blackColor = 36;
        _subtitleImage.fill( blackColor );

        // At the left and bottom there is space for contour left by original font shadows, we leave 1 extra pixel from the right and top.
        subtitleText.draw( 0, 1, textWidth, _subtitleImage );
        fheroes2::ReplaceColorIdByTransformId( _subtitleImage, blackColor, 1 );
        // Add black contour to the text.
        fheroes2::Blit( fheroes2::CreateContour( _subtitleImage, blackColor ), _subtitleImage );

        // This is made to avoid overflow when calculating the end frame.
        _endTimeMS = _startTimeMS + std::min( durationMS, UINT32_MAX - _startTimeMS );

        // Position subtitles at the bottom center by using default screen size (it is currently equal to video size).
        _position.x = ( fheroes2::Display::DEFAULT_WIDTH - _subtitleImage.width() ) / 2;
        _position.y = fheroes2::Display::DEFAULT_HEIGHT - _subtitleImage.height();
    }

    void Subtitle::blitSubtitles( fheroes2::Image & output, const fheroes2::Rect & frameRoi ) const
    {
        fheroes2::Blit( _subtitleImage, 0, 0, output, frameRoi.x + _position.x, frameRoi.y + _position.y, _subtitleImage.width(), _subtitleImage.height() );
    }
}
