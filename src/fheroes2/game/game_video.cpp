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

#include "game_video.h"
#include "agg.h"
#include "audio_mixer.h"
#include "cursor.h"
#include "game_delays.h"
#include "localevent.h"
#include "logging.h"
#include "screen.h"
#include "settings.h"
#include "smk_decoder.h"
#include "system.h"
#include "ui_tool.h"

namespace
{
    const std::vector<std::string> videoDir = { "anim", System::ConcatePath( "heroes2", "anim" ), "data" };

    void drawRectangle( const fheroes2::Rect & roi, fheroes2::Image & image, const uint8_t color )
    {
        fheroes2::DrawRect( image, roi, color );
        fheroes2::DrawRect( image, fheroes2::Rect( roi.x - 1, roi.y - 1, roi.width + 2, roi.height + 2 ), color );
    }
}

namespace Video
{
    bool isVideoFile( const std::string & fileName, std::string & path )
    {
        std::string temp;

        for ( size_t i = 0; i < videoDir.size(); ++i ) {
            ListFiles files = Settings::FindFiles( videoDir[i], fileName );
            for ( std::string & name : files ) {
                if ( System::IsFile( name ) ) { // file doesn't exist, so no need to even try to load it
                    path.swap( name );
                    return true;
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
        if ( !isVideoFile( fileName, videoPath ) ) { // file doesn't exist, so no need to even try to load it
            DEBUG_LOG( DBG_GAME, DBG_INFO, fileName << " file does not exist" );
            return 0;
        }

        SMKVideoSequence video( videoPath );
        if ( video.frameCount() < 1 ) // nothing to show
            return 0;

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

        const bool hasSound = Settings::Get().Sound();
        const std::vector<std::vector<uint8_t> > & sound = video.getAudioChannels();
        if ( hasSound ) {
            for ( std::vector<std::vector<uint8_t> >::const_iterator it = sound.begin(); it != sound.end(); ++it ) {
                if ( it->size() )
                    Mixer::Play( &( *it )[0], static_cast<uint32_t>( it->size() ), -1, false );
            }
        }

        if ( action == VideoAction::IGNORE_VIDEO ) {
            return 0;
        }

        std::vector<uint8_t> palette;
        std::vector<uint8_t> prevPalette;

        bool isFrameReady = false;

        int roiChosenId = 0;

        const uint8_t selectionColor = 51;

        Game::passAnimationDelay( Game::CUSTOM_DELAY );

        bool userMadeAction = false;

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
                    Mixer::Reset();
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
                    Mixer::Reset();
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

                    if ( hasSound ) {
                        for ( std::vector<std::vector<uint8_t> >::const_iterator it = sound.begin(); it != sound.end(); ++it ) {
                            if ( it->size() )
                                Mixer::Play( &( *it )[0], static_cast<uint32_t>( it->size() ), -1, false );
                        }
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
