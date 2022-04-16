/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
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

#include "game_video.h"
#include "audio.h"
#include "cursor.h"
#include "game_delays.h"
#include "localevent.h"
#include "logging.h"
#include "screen.h"
#include "settings.h"
#include "smk_decoder.h"
#include "system.h"
#include "tools.h"
#include "ui_tool.h"

namespace
{
    // Anim2 directory is used in Russian Buka version of the game.
    const std::vector<std::string> videoDir = { "anim", "anim2", System::ConcatePath( "heroes2", "anim" ), "data" };

    void drawRectangle( const fheroes2::Rect & roi, fheroes2::Image & image, const uint8_t color )
    {
        fheroes2::DrawRect( image, roi, color );
        fheroes2::DrawRect( image, fheroes2::Rect( roi.x - 1, roi.y - 1, roi.width + 2, roi.height + 2 ), color );
    }

    void playAudio( const std::vector<std::vector<uint8_t>> & audioChannels )
    {
        Mixer::Volume( -1, Mixer::MaxVolume() * Settings::Get().SoundVolume() / 10 );

        for ( const std::vector<uint8_t> & audio : audioChannels ) {
            if ( !audio.empty() ) {
                Mixer::Play( &audio[0], static_cast<uint32_t>( audio.size() ) );
            }
        }
    }
}

namespace Video
{
    bool getVideoFilePath( const std::string & fileName, std::string & path )
    {
        for ( const std::string & rootDir : Settings::GetRootDirs() ) {
            for ( const std::string & localDir : videoDir ) {
                const std::string fullDirPath = System::ConcatePath( rootDir, localDir );

                if ( System::IsDirectory( fullDirPath ) ) {
                    ListFiles videoFiles;
                    videoFiles.FindFileInDir( fullDirPath, fileName, false );

                    std::string targetFileName = System::ConcatePath( fullDirPath, fileName );
                    targetFileName = StringLower( targetFileName );

                    for ( const std::string & filePath : videoFiles ) {
                        if ( StringLower( filePath ) == targetFileName ) {
                            path = filePath;
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    int ShowVideo( const std::string & fileName, const VideoAction action, const std::vector<fheroes2::Rect> & roi )
    {
        // Stop any cycling animation.
        const fheroes2::ScreenPaletteRestorer screenRestorer;

        std::string videoPath;
        if ( !getVideoFilePath( fileName, videoPath ) ) {
            // File doesn't exist, so no need to even try to load it.
            DEBUG_LOG( DBG_GAME, DBG_INFO, fileName << " video file does not exist." );
            return 0;
        }

        SMKVideoSequence video( videoPath );
        if ( video.frameCount() < 1 ) // nothing to show
            return 0;

        const std::vector<std::vector<uint8_t>> & audioChannels = video.getAudioChannels();
        const bool hasAudio = Audio::isValid() && !audioChannels.empty();
        if ( action == VideoAction::IGNORE_VIDEO ) {
            // Since no video is rendered play audio if available.
            if ( hasAudio ) {
                playAudio( audioChannels );
            }

            return 0;
        }

        const bool isLooped = ( action == VideoAction::LOOP_VIDEO || action == VideoAction::PLAY_TILL_AUDIO_END );

        // setup cursor
        std::unique_ptr<const CursorRestorer> cursorRestorer;

        if ( roi.empty() ) {
            cursorRestorer.reset( new CursorRestorer( false, Cursor::Get().Themes() ) );
        }
        else {
            cursorRestorer.reset( new CursorRestorer( true, Cursor::Get().Themes() ) );

            Cursor::Get().setVideoPlaybackCursor();
        }

        fheroes2::Display & display = fheroes2::Display::instance();
        display.fill( 0 );

        unsigned int currentFrame = 0;
        fheroes2::Rect frameRoi( ( display.width() - video.width() ) / 2, ( display.height() - video.height() ) / 2, 0, 0 );

        const uint32_t delay = static_cast<uint32_t>( 1000.0 / video.fps() + 0.5 ); // This might be not very accurate but it's the best we can have now

        std::vector<uint8_t> palette;
        std::vector<uint8_t> prevPalette;

        bool isFrameReady = false;

        int roiChosenId = 0;

        const uint8_t selectionColor = 51;

        Game::passAnimationDelay( Game::CUSTOM_DELAY );

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

            if ( roi.empty() ) {
                if ( le.KeyPress() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() ) {
                    userMadeAction = true;
                    Mixer::Stop();
                    break;
                }
            }
            else {
                bool roiChosen = false;
                for ( size_t i = 0; i < roi.size(); ++i ) {
                    if ( le.MouseClickLeft( roi[i] ) ) {
                        roiChosenId = static_cast<int>( i );
                        roiChosen = true;
                        break;
                    }
                }

                if ( roiChosen ) {
                    Mixer::Stop();
                    break;
                }
            }

            if ( Game::validateCustomAnimationDelay( delay ) ) {
                if ( !isFrameReady ) {
                    if ( currentFrame == 0 )
                        video.resetFrame();

                    video.getNextFrame( display, frameRoi.x, frameRoi.y, frameRoi.width, frameRoi.height, palette );

                    for ( size_t i = 0; i < roi.size(); ++i ) {
                        if ( le.MouseCursor( roi[i] ) ) {
                            drawRectangle( roi[i], display, selectionColor );
                            break;
                        }
                    }
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

                    for ( size_t i = 0; i < roi.size(); ++i ) {
                        if ( le.MouseCursor( roi[i] ) ) {
                            drawRectangle( roi[i], display, selectionColor );
                            break;
                        }
                    }

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

        return roiChosenId;
    }
}
