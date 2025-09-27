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

#include "game_video.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "audio.h"
#include "cursor.h"
#include "dir.h"
#include "game_delays.h"
#include "game_video_type.h"
#include "localevent.h"
#include "logging.h"
#include "screen.h"
#include "settings.h"
#include "smk_decoder.h"
#include "system.h"
#include "ui_text.h"
#include "ui_tool.h"

namespace
{
    // Anim2 directory is used in Russian Buka version of the game.
    std::array<std::string, 4> videoDir = { "anim", "anim2", System::concatPath( "heroes2", "anim" ), "data" };

    void playAudio( const std::vector<std::vector<uint8_t>> & audioChannels )
    {
        for ( const std::vector<uint8_t> & audio : audioChannels ) {
            if ( !audio.empty() ) {
                Mixer::Play( audio.data(), static_cast<uint32_t>( audio.size() ), false );
            }
        }
    }

    // Internal video state structure during playback.
    struct VideoState final
    {
        Video::VideoControl control{ Video::VideoControl::PLAY_NONE };
        fheroes2::Rect area;
        int32_t maxFrameDelay{ 0 };
        int32_t currentFrameDelay{ 0 };
    };
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
                    videoFiles.FindFileInDir( fullDirPath, fileName );
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

    bool ShowVideo( const std::vector<VideoInfo> & infos, const std::vector<Subtitle> & subtitles /* = {} */, const bool fadeColorsOnEnd /* = false */ )
    {
        // Stop any cycling animation.
        const fheroes2::ScreenPaletteRestorer screenRestorer;

        std::vector<std::pair<VideoState, std::unique_ptr<SMKVideoSequence>>> sequences;
        // Minimal delay for playback in microseconds.
        // Since the engine operates only within millisecond range (see LocalEvent.cpp file)
        // this value is set to a minimal possibly handled by the engine value - 1000.
        int32_t minDelay = 1000;

        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::Rect firstFrameArea;

        for ( const auto & info : infos ) {
            std::string videoPath;
            if ( !getVideoFilePath( info.fileName, videoPath ) ) {
                // File doesn't exist, so no need to even try to load it.
                DEBUG_LOG( DBG_GAME, DBG_INFO, info.fileName << " video file does not exist." )
                return false;
            }
            auto video = std::make_unique<SMKVideoSequence>( videoPath );
            if ( video->frameCount() < 1 ) {
                // Something wrong with file, exit now
                DEBUG_LOG( DBG_GAME, DBG_INFO, info.fileName << " video file has no frames." )
                return false;
            }
            const int32_t delay = static_cast<int32_t>( std::lround( video->microsecondsPerFrame() / 1000 ) );
            minDelay = std::min( minDelay, delay );
            VideoState state;
            if ( info == infos.front() ) {
                // First file is background video
                firstFrameArea = { ( display.width() - video->width() - infos.front().offset.x ) / 2, ( display.height() - video->height() - infos.front().offset.y ) / 2,
                                   video->width(), video->height() };
                state = { info.control, firstFrameArea, delay, delay };
            }
            else {
                fheroes2::Rect const frame = { firstFrameArea.getPosition() + info.offset, { video->width(), video->height() } };
                state = { info.control, frame, delay, delay };
            }
            sequences.emplace_back( state, std::move( video ) );
        }

        // Hide mouse cursor.
        const CursorRestorer cursorRestorer( false );

        display.fill( 0 );
        display.updateNextRenderRoi( { 0, 0, display.width(), display.height() } );

        std::vector<uint8_t> currPalette;
        std::vector<uint8_t> prevPalette;

        // Construct first frame
        for ( auto & [state, video] : sequences ) {
            video->resetFrame();

            if ( state.control & VideoControl::PLAY_VIDEO ) {
                video->getNextFrame( display, state.area.x, state.area.y, state.area.width, state.area.height, prevPalette );
            }
            else {
                video->skipFrame();
            }
            screenRestorer.changePalette( prevPalette.data() );

            // Decrease counters
            state.currentFrameDelay -= minDelay;
        }
        // Render subtitles on the first frame
        for ( const Subtitle & subtitle : subtitles ) {
            if ( subtitle.needRender( 0 ) ) {
                subtitle.render( display, firstFrameArea );
            }
        }

        LocalEvent & le = LocalEvent::Get();

        Game::passCustomAnimationDelay( minDelay );
        // Make sure that the first run is passed immediately.
        assert( !Game::isCustomDelayNeeded( minDelay ) );

        // Play audio just before rendering the frame. This is important to minimize synchronization issues between audio and video.
        if ( Audio::isValid() ) {
            for ( const auto & [state, video] : sequences ) {
                if ( state.control & VideoControl::PLAY_AUDIO ) {
                    playAudio( video->getAudioChannels() );
                }
            }
        }

        bool endVideo = false;
        int32_t timePassed = 0;
        while ( le.HandleEvents( Game::isCustomDelayNeeded( minDelay ) ) ) {
            if ( le.isAnyKeyPressed() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() ) {
                Mixer::Stop();
                break;
            }

            if ( Game::validateCustomAnimationDelay( minDelay ) ) {
                // Render the prepared frame.
                display.render( firstFrameArea );
                for ( auto & [state, video] : sequences ) {
                    if ( video->getCurrentFrameId() < video->frameCount() ) {
                        if ( video->getCurrentFrameId() + 1 == video->frameCount() ) {
                            if ( state.control & VideoControl::PLAY_LOOP ) {
                                video->resetFrame();
                                // Restart audio
                                if ( Audio::isValid() && ( state.control & VideoControl::PLAY_AUDIO ) ) {
                                    playAudio( video->getAudioChannels() );
                                }
                            }
                            else {
                                // Play last frame as long as possible
                                if ( minDelay > state.currentFrameDelay ) {
                                    endVideo = true;
                                }
                            }
                        }

                        // Prepare the next frame for render.
                        if ( state.currentFrameDelay <= minDelay ) {
                            if ( state.control & VideoControl::PLAY_VIDEO ) {
                                video->getNextFrame( display, state.area.x, state.area.y, state.area.width, state.area.height, currPalette );
                            }
                            else {
                                video->skipFrame();
                            }
                            state.currentFrameDelay = state.maxFrameDelay;
                        }
                        else {
                            if ( state.control & VideoControl::PLAY_VIDEO ) {
                                video->getCurrentFrame( display, state.area.x, state.area.y, state.area.width, state.area.height, currPalette );
                            }
                            state.currentFrameDelay -= minDelay;
                        }

                        if ( prevPalette != currPalette ) {
                            screenRestorer.changePalette( currPalette.data() );
                            std::swap( currPalette, prevPalette );
                        }
                    }
                    else if ( !( state.control & VideoControl::PLAY_WAIT ) ) {
                        endVideo = true;
                    }
                }
                timePassed += minDelay;
                // Render subtitles on the prepared next frame
                for ( const Subtitle & subtitle : subtitles ) {
                    if ( subtitle.needRender( timePassed ) ) {
                        subtitle.render( display, firstFrameArea );
                    }
                }
            }
            if ( endVideo ) {
                break;
            }
        }

        if ( fadeColorsOnEnd ) {
            // Do color fade for 1 second with 15 FPS.
            fheroes2::colorFade( currPalette, firstFrameArea, 1000, 15.0 );
        }
        else {
            display.fill( 0 );
        }
        display.updateNextRenderRoi( { 0, 0, display.width(), display.height() } );

        return true;
    }

    Subtitle::Subtitle( const fheroes2::TextBase & subtitleText, const uint32_t startTimeMS, const uint32_t durationMS,
                        const fheroes2::Point & position /* = { -1, -1 } */, const int32_t maxWidth /* = fheroes2::Display::DEFAULT_WIDTH */ )
        : _position( position )
        , _startTimeMS( startTimeMS )
    {
        assert( maxWidth > 0 );
        const int32_t textWidth = subtitleText.width( maxWidth );

        // We add extra 1 to have space for contour.
        _subtitleImage.resize( textWidth + 1, subtitleText.height( textWidth ) + 1 );

        // Draw text and remove all shadow data if it could not be properly applied to video palette.
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

        // If position has negative value: position subtitles at the bottom center by using default screen size (it is currently equal to video size).
        if ( ( _position.x < 0 ) || ( _position.y < 0 ) ) {
            _position.x = ( fheroes2::Display::DEFAULT_WIDTH - _subtitleImage.width() ) / 2;
            _position.y = fheroes2::Display::DEFAULT_HEIGHT - _subtitleImage.height();
        }
        else {
            _position.x -= _subtitleImage.width() / 2;
        }
    }
}
