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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <vector>

#include "audio.h"
#include "cursor.h"
#include "dir.h"
#include "game_delays.h"
#include "game_video.h"
#include "localevent.h"
#include "logging.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "smk_decoder.h"
#include "system.h"
#include "ui_tool.h"

#include <array>

namespace
{
    // Anim2 directory is used in Russian Buka version of the game.
    std::array<std::string, 4> videoDir = { "anim", "anim2", System::concatPath( "heroes2", "anim" ), "data" };

    void playAudio( const std::vector<std::vector<uint8_t>> & audioChannels )
    {
        Mixer::setVolume( -1, 100 * Settings::Get().SoundVolume() / 10 );

        for ( const std::vector<uint8_t> & audio : audioChannels ) {
            if ( !audio.empty() ) {
                Mixer::Play( &audio[0], static_cast<uint32_t>( audio.size() ), -1, false );
            }
        }
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

    bool ShowVideo( const std::string & fileName, const VideoAction action )
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
        if ( video.frameCount() < 1 ) // nothing to show
            return false;

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

        // setup cursor
        const CursorRestorer cursorRestorer( false, Cursor::Get().Themes() );

        fheroes2::Display & display = fheroes2::Display::instance();
        display.fill( 0 );
        display.updateNextRenderRoi( { 0, 0, display.width(), display.height() } );

        unsigned int currentFrame = 0;
        fheroes2::Rect frameRoi( ( display.width() - video.width() ) / 2, ( display.height() - video.height() ) / 2, 0, 0 );

        const uint32_t delay = static_cast<uint32_t>( 1000.0 / video.fps() + 0.5 ); // This might be not very accurate but it's the best we can have now

        std::vector<uint8_t> palette;
        std::vector<uint8_t> prevPalette;

        bool isFrameReady = false;

        Game::passCustomAnimationDelay( delay );
        // Make sure that the first run is passed immediately.
        assert( !Game::isCustomDelayNeeded( delay ) );

        bool userMadeAction = false;

        // Play audio just before rendering the frame. This is important to minimize synchronization issues between audio and video.
        if ( hasAudio ) {
            playAudio( audioChannels );
        }

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents( Game::isCustomDelayNeeded( delay ) ) ) {
            if ( action == VideoAction::PLAY_TILL_AUDIO_END ) {
                if ( !Mixer::isPlaying( -1 ) ) {
                    break;
                }
            }
            else if ( action != VideoAction::LOOP_VIDEO ) {
                if ( currentFrame >= video.frameCount() ) {
                    break;
                }
            }

            if ( le.KeyPress() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() ) {
                userMadeAction = true;
                Mixer::Stop();
                break;
            }

            if ( Game::validateCustomAnimationDelay( delay ) ) {
                if ( !isFrameReady ) {
                    if ( currentFrame == 0 )
                        video.resetFrame();

                    video.getNextFrame( display, frameRoi.x, frameRoi.y, frameRoi.width, frameRoi.height, palette );
                }
                isFrameReady = false;

                if ( prevPalette != palette ) {
                    screenRestorer.changePalette( palette.data() );
                    std::swap( prevPalette, palette );
                }

                display.render( frameRoi );

                ++currentFrame;

                if ( isLooped && currentFrame >= video.frameCount() ) {
                    currentFrame = 0;

                    if ( hasAudio ) {
                        playAudio( audioChannels );
                    }
                }
            }
            else {
                // Don't waste CPU resources, do some calculations while we're waiting for the next frame time position
                if ( !isFrameReady ) {
                    if ( currentFrame == 0 )
                        video.resetFrame();

                    video.getNextFrame( display, frameRoi.x, frameRoi.y, frameRoi.width, frameRoi.height, palette );

                    isFrameReady = true;
                }
            }
        }

        if ( action == VideoAction::WAIT_FOR_USER_INPUT && !userMadeAction ) {
            while ( le.HandleEvents() ) {
                if ( le.KeyPress() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() ) {
                    break;
                }
            }
        }

        display.fill( 0 );
        display.updateNextRenderRoi( { 0, 0, display.width(), display.height() } );

        return true;
    }
}
