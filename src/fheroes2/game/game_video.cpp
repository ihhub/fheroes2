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
#include "settings.h"
#include "smk_decoder.h"

namespace Video
{
    void ShowVideo( const std::string & videoPath, bool isLooped )
    {
        const SMKVideoSequence video( videoPath );
        if ( video.getFrames().empty() ) // nothing to show
            return;

        Cursor & cursor = Cursor::Get();
        cursor.Hide();

        Display & display = Display::Get();
        display.Fill( RGBA() );

        size_t currentFrame = 0;
        const Point offset( ( display.GetSize().w - video.width() ) / 2, ( display.GetSize().h - video.height() ) / 2 );
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

        LocalEvent & le = LocalEvent::Get();
        while ( ( isLooped || currentFrame < video.getFrames().size() ) && le.HandleEvents() ) {
            if ( le.KeyPress() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() )
                break;

            if ( isFirstFrame || Game::AnimateCustomDelay( delay ) ) {
                isFirstFrame = false;

                video.getFrames()[currentFrame++].Blit( offset, display );
                display.Flip();

                if ( isLooped && currentFrame >= video.getFrames().size() ) {
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

        cursor.Show();
        Mixer::Reset();

        return;
    }
}
