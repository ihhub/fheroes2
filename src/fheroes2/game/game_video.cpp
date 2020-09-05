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
#include "cursor.h"
#include "game.h"
#include "screen.h"
#include "settings.h"
#include "smk_decoder.h"
#include "ui_tool.h"

namespace Video
{
    void ShowVideo( const std::string & videoPath, bool isLooped )
    {
        SMKVideoSequence video( videoPath );
        if ( video.frameCount() < 1 ) // nothing to show
            return;

        Cursor & cursor = Cursor::Get();
        cursor.Hide();
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
                    Mixer::Play( &( *it )[0], it->size(), -1, false );
            }
        }

        fheroes2::ScreenPaletteRestorer screenRestorer;

        fheroes2::Image frame;
        std::vector<uint8_t> palette;

        LocalEvent & le = LocalEvent::Get();
        while ( ( isLooped || currentFrame < video.frameCount() ) && le.HandleEvents() ) {
            if ( le.KeyPress() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() )
                break;

            if ( isFirstFrame || Game::AnimateCustomDelay( delay ) ) {
                isFirstFrame = false;

                if ( currentFrame == 0 )
                    video.resetFrame();

                video.getNextFrame( frame, palette );

                screenRestorer.changePalette( palette.data() );

                fheroes2::Blit( frame, display, offset.x, offset.y );
                display.render();

                ++currentFrame;

                if ( isLooped && currentFrame >= video.frameCount() ) {
                    currentFrame = 0;

                    if ( hasSound ) {
                        for ( std::vector<std::vector<uint8_t> >::const_iterator it = sound.begin(); it != sound.end(); ++it ) {
                            if ( it->size() )
                                Mixer::Play( &( *it )[0], it->size(), -1, false );
                        }
                    }
                }
            }
        }

        display.fill( 0 );
        cursor.Show();

        Mixer::Reset();

        return;
    }
}
