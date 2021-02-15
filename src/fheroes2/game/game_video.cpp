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
#include "audio_mixer.h"
#include "cursor.h"
#include "game.h"
#include "localevent.h"
#include "logging.h"
#include "screen.h"
#include "settings.h"
#include "smk_decoder.h"
#include "system.h"
#include "ui_tool.h"

namespace
{
    const std::vector<std::string> videoDir = {"anim", System::ConcatePath( "heroes2", "anim" )};

    bool IsFile( const std::string & fileName, std::string & path )
    {
        std::string temp;

        for ( size_t i = 0; i < videoDir.size(); ++i ) {
            temp = Settings::GetLastFile( videoDir[i], fileName );
            if ( System::IsFile( temp ) ) { // file doesn't exist, so no need to even try to load it
                path.swap( temp );
                return true;
            }
        }

        return false;
    }
}

namespace Video
{
    size_t ShowVideo( const std::string & fileName, bool isLooped, const std::vector<fheroes2::Rect> & roi )
    {
        std::string videoPath;
        if ( !IsFile( fileName, videoPath ) ) { // file doesn't exist, so no need to even try to load it
            DEBUG_LOG( DBG_GAME, DBG_INFO, fileName << " file does not exist" );
            return 0;
        }

        SMKVideoSequence video( videoPath );
        if ( video.frameCount() < 1 ) // nothing to show
            return 0;

        const bool hideCursor = roi.empty();

        if ( hideCursor ) {
            Cursor::Get().Hide();
        }

        fheroes2::Display & display = fheroes2::Display::instance();
        display.fill( 0 );

        unsigned int currentFrame = 0;
        const fheroes2::Point offset( ( display.width() - video.width() ) / 2, ( display.height() - video.height() ) / 2 );
        bool isFirstFrame = true;

        const uint32_t delay = static_cast<uint32_t>( 1000.0 / video.fps() + 0.5 ); // This might be not very accurate but it's the best we can have now

        const bool hasSound = Settings::Get().Sound();
        const std::vector<std::vector<uint8_t> > & sound = video.getAudioChannels();
        if ( hasSound ) {
            for ( std::vector<std::vector<uint8_t> >::const_iterator it = sound.begin(); it != sound.end(); ++it ) {
                if ( it->size() )
                    Mixer::Play( &( *it )[0], static_cast<uint32_t>( it->size() ), -1, false );
            }
        }

        fheroes2::ScreenPaletteRestorer screenRestorer;

        fheroes2::Image frame;
        std::vector<uint8_t> palette;
        std::vector<uint8_t> prevPalette;

        bool isFrameReady = false;

        size_t roiChosenId = 0;

        const uint8_t selectionColor = 51;

        LocalEvent & le = LocalEvent::Get();
        while ( ( isLooped || currentFrame < video.frameCount() ) && le.HandleEvents() ) {
            if ( roi.empty() ) {
                if ( le.KeyPress() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() ) {
                    Mixer::Reset();
                    break;
                }
            }
            else {
                bool roiChosen = false;
                for ( size_t i = 0; i < roi.size(); ++i ) {
                    if ( le.MouseClickLeft( roi[i] ) ) {
                        roiChosenId = i;
                        roiChosen = true;
                        break;
                    }
                }

                if ( roiChosen ) {
                    Mixer::Reset();
                    break;
                }
            }

            if ( isFirstFrame || Game::AnimateCustomDelay( delay ) ) {
                isFirstFrame = false;

                if ( !isFrameReady ) {
                    if ( currentFrame == 0 )
                        video.resetFrame();

                    video.getNextFrame( frame, palette );

                    fheroes2::Copy( frame, 0, 0, display, offset.x, offset.y, frame.width(), frame.height() );

                    for ( size_t i = 0; i < roi.size(); ++i ) {
                        if ( le.MouseCursor( roi[i] ) ) {
                            fheroes2::DrawRect( display, roi[i], selectionColor );
                            break;
                        }
                    }
                }
                isFrameReady = false;

                if ( prevPalette != palette ) {
                    screenRestorer.changePalette( palette.data() );
                    std::swap( prevPalette, palette );
                }

                display.render();

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

                    video.getNextFrame( frame, palette );
                    fheroes2::Copy( frame, 0, 0, display, offset.x, offset.y, frame.width(), frame.height() );

                    for ( size_t i = 0; i < roi.size(); ++i ) {
                        if ( le.MouseCursor( roi[i] ) ) {
                            fheroes2::DrawRect( display, roi[i], selectionColor );
                            break;
                        }
                    }

                    isFrameReady = true;
                }
            }
        }

        display.fill( 0 );

        if ( hideCursor ) {
            Cursor::Get().Show();
        }

        return roiChosenId;
    }
}
