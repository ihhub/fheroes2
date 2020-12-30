/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include "agg.h"
#include "audio_music.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_interface.h"
#include "settings.h"
#include "text.h"
#include "ui_button.h"

namespace Dialog
{
    void DrawSystemInfo( const std::vector<fheroes2::Rect> & );
}

/* return 0x01 - change speed, 0x02 - change sound, 0x04 - hide interface, 0x08 - change interface, 0x10 - change scroll  */
int Dialog::SystemOptions( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Settings & conf = Settings::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    const int oldcursor = cursor.Themes();
    cursor.SetThemes( cursor.POINTER );

    const bool isEvilInterface = conf.ExtGameEvilInterface();

    const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::SPANBKGE : ICN::SPANBKG ), 0 );
    const fheroes2::Sprite & dialogShadow = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::SPANBKGE : ICN::SPANBKG ), 1 );

    const fheroes2::Point dialogOffset( ( display.width() - dialog.width() ) / 2, ( display.height() - dialog.height() ) / 2 );
    const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

    fheroes2::ImageRestorer back( display, shadowOffset.x, shadowOffset.y, dialog.width() + BORDERWIDTH, dialog.height() + BORDERWIDTH );
    const fheroes2::Rect dialogArea( dialogOffset.x, dialogOffset.y, dialog.width(), dialog.height() );

    fheroes2::Fill( display, dialogArea.x, dialogArea.y, dialogArea.width, dialogArea.height, 0 );
    fheroes2::Blit( dialogShadow, display, dialogArea.x - BORDERWIDTH, dialogArea.y + BORDERWIDTH );
    fheroes2::Blit( dialog, display, dialogArea.x, dialogArea.y );

    const fheroes2::Sprite & optionSprite = fheroes2::AGG::GetICN( ICN::SPANEL, 0 );
    const fheroes2::Point optionOffset( 36 + dialogArea.x, 47 + dialogArea.y );
    const fheroes2::Point optionStep( 92, 110 );

    std::vector<fheroes2::Rect> rects;
    rects.push_back( fheroes2::Rect( optionOffset.x, optionOffset.y, optionSprite.width(), optionSprite.height() ) );
    rects.push_back( fheroes2::Rect( optionOffset.x + optionStep.x, optionOffset.y, optionSprite.width(), optionSprite.height() ) );
    rects.push_back( fheroes2::Rect( optionOffset.x + 2 * optionStep.x, optionOffset.y, optionSprite.width(), optionSprite.height() ) );
    rects.push_back( fheroes2::Rect( optionOffset.x, optionOffset.y + optionStep.y, optionSprite.width(), optionSprite.height() ) );
    rects.push_back( fheroes2::Rect( optionOffset.x + optionStep.x, optionOffset.y + optionStep.y, optionSprite.width(), optionSprite.height() ) );
    rects.push_back( fheroes2::Rect( optionOffset.x + 2 * optionStep.x, optionOffset.y + optionStep.y, optionSprite.width(), optionSprite.height() ) );
    rects.push_back( fheroes2::Rect( optionOffset.x, optionOffset.y + 2 * optionStep.y, optionSprite.width(), optionSprite.height() ) );
    rects.push_back( fheroes2::Rect( optionOffset.x + optionStep.x, optionOffset.y + 2 * optionStep.y, optionSprite.width(), optionSprite.height() ) );
    rects.push_back( fheroes2::Rect( optionOffset.x + 2 * optionStep.x, optionOffset.y + 2 * optionStep.y, optionSprite.width(), optionSprite.height() ) ); // not in use

    const fheroes2::Rect & rect1 = rects[0];
    const fheroes2::Rect & rect2 = rects[1];
    const fheroes2::Rect & rect3 = rects[2];
    const fheroes2::Rect & rect4 = rects[3];
    const fheroes2::Rect & rect5 = rects[4];
    const fheroes2::Rect & rect6 = rects[5];
    const fheroes2::Rect & rect7 = rects[6];
    const fheroes2::Rect & rect8 = rects[7];

    DrawSystemInfo( rects );

    LocalEvent & le = LocalEvent::Get();

    const fheroes2::Point buttonOffset( 113 + dialogArea.x, 362 + dialogArea.y );
    fheroes2::Button buttonOkay( buttonOffset.x, buttonOffset.y, isEvilInterface ? ICN::SPANBTNE : ICN::SPANBTN, 0, 1 );
    buttonOkay.draw();

    display.render();

    int result = 0;
    bool redraw = false;
    bool saveConfig = false;

    // dialog menu loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonOkay.area() ) ? buttonOkay.drawOnPress() : buttonOkay.drawOnRelease();
        if ( le.MouseClickLeft( buttonOkay.area() ) ) {
            break;
        }

        // set music volume
        if ( conf.Music() && le.MouseClickLeft( rect1 ) ) {
            conf.SetMusicVolume( 10 > conf.MusicVolume() ? conf.MusicVolume() + 1 : 0 );
            redraw = true;
            Music::Volume( static_cast<int16_t>( Mixer::MaxVolume() * conf.MusicVolume() / 10 ) );
            saveConfig = true;
        }

        // set sound volume
        if ( conf.Sound() && le.MouseClickLeft( rect2 ) ) {
            conf.SetSoundVolume( 10 > conf.SoundVolume() ? conf.SoundVolume() + 1 : 0 );
            redraw = true;
            Game::EnvironmentSoundMixer();
            saveConfig = true;
        }

        // set music type
        if ( le.MouseClickLeft( rect3 ) ) {
            int type = conf.MusicType() + 1;
            // If there's no expansion files we skip this option
            if ( type == MUSIC_MIDI_EXPANSION && !conf.PriceLoyaltyVersion() )
                ++type;
            // CD music is currently not implemented correctly even on SDL1; remove this when done
            if ( type == MUSIC_CDROM )
                ++type;

            conf.SetMusicType( type > MUSIC_CDROM ? 0 : type );
            result |= 0x02;
            redraw = true;
            saveConfig = true;
        }

        // set hero speed
        if ( le.MouseClickLeft( rect4 ) ) {
            conf.SetHeroesMoveSpeed( conf.HeroesMoveSpeed() % 10 + 1 );
            result |= 0x01;
            redraw = true;
            Game::UpdateGameSpeed();
            saveConfig = true;
        }

        // set ai speed
        if ( le.MouseClickLeft( rect5 ) ) {
            conf.SetAIMoveSpeed( conf.AIMoveSpeed() % 10 + 1 );
            result |= 0x01;
            redraw = true;
            Game::UpdateGameSpeed();
            saveConfig = true;
        }

        // set scroll speed
        if ( le.MouseClickLeft( rect6 ) ) {
            conf.SetScrollSpeed( SCROLL_FAST2 > conf.ScrollSpeed() ? conf.ScrollSpeed() << 1 : SCROLL_SLOW );
            result |= 0x10;
            redraw = true;
            saveConfig = true;
        }

        // set interface theme
        if ( le.MouseClickLeft( rect7 ) ) {
            conf.SetEvilInterface( !conf.ExtGameEvilInterface() );
            result |= 0x08;
            redraw = true;
            saveConfig = true;
        }

        // set interface hide/show
        if ( le.MouseClickLeft( rect8 ) ) {
            conf.SetHideInterface( !conf.ExtGameHideInterface() );
            result |= 0x04;
            redraw = true;
            saveConfig = true;
        }

        if ( redraw ) {
            fheroes2::Blit( dialog, display, dialogArea.x, dialogArea.y );
            DrawSystemInfo( rects );
            buttonOkay.draw();
            display.render();
            redraw = false;
        }
    }

    // restore background
    cursor.SetThemes( oldcursor );
    display.render();

    if ( saveConfig ) {
        conf.Save( "fheroes2.cfg" );
    }

    return result;
}

void Dialog::DrawSystemInfo( const std::vector<fheroes2::Rect> & rects )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Settings & conf = Settings::Get();

    std::string str;
    Text text;

    const int textOffset = 2;

    // music
    const fheroes2::Sprite & sprite1 = fheroes2::AGG::GetICN( ICN::SPANEL, conf.Music() ? 1 : 0 );
    const Rect & rect1 = rects[0];
    fheroes2::Blit( sprite1, display, rect1.x, rect1.y );
    str = _( "Music" );
    text.Set( str, Font::SMALL );
    text.Blit( rect1.x + ( rect1.w - text.w() ) / 2, rect1.y - text.h() - textOffset );

    if ( conf.Music() && conf.MusicVolume() )
        str = GetString( conf.MusicVolume() );
    else
        str = _( "off" );
    text.Set( str );
    text.Blit( rect1.x + ( rect1.w - text.w() ) / 2, rect1.y + rect1.h + textOffset );

    // sound
    const fheroes2::Sprite & sprite2 = fheroes2::AGG::GetICN( ICN::SPANEL, conf.Sound() ? 3 : 2 );
    const Rect & rect2 = rects[1];
    fheroes2::Blit( sprite2, display, rect2.x, rect2.y );
    str = _( "Effects" );
    text.Set( str, Font::SMALL );
    text.Blit( rect2.x + ( rect2.w - text.w() ) / 2, rect2.y - text.h() - textOffset );

    if ( conf.Sound() && conf.SoundVolume() )
        str = GetString( conf.SoundVolume() );
    else
        str = _( "off" );
    text.Set( str, Font::SMALL );
    text.Blit( rect2.x + ( rect2.w - text.w() ) / 2, rect2.h + rect2.y + textOffset );

    // Music Type
    const MusicSource musicType = conf.MusicType();
    const fheroes2::Sprite & sprite3 = fheroes2::AGG::GetICN( ICN::SPANEL, ( musicType == MUSIC_CDROM || musicType == MUSIC_EXTERNAL ) ? 11 : 10 );
    const Rect & rect3 = rects[2];
    fheroes2::Blit( sprite3, display, rect3.x, rect3.y );
    str = _( "Music Type" );
    text.Set( str, Font::SMALL );
    text.Blit( rect3.x + ( rect3.w - text.w() ) / 2, rect3.y - text.h() - textOffset );

    if ( musicType == MUSIC_MIDI_ORIGINAL ) {
        str = _( "MIDI" );
    }
    else if ( musicType == MUSIC_MIDI_EXPANSION ) {
        str = _( "MIDI Expansion" );
    }
    else if ( musicType == MUSIC_CDROM ) {
        str = _( "CD Stereo" );
    }
    else if ( musicType == MUSIC_EXTERNAL ) {
        str = _( "External" );
    }
    text.Set( str );
    text.Blit( rect3.x + ( rect3.w - text.w() ) / 2, rect3.y + rect3.h + textOffset );

    // hero move speed
    const int heroSpeed = conf.HeroesMoveSpeed();
    const u32 is4 = heroSpeed ? ( heroSpeed < 4 ? 4 : 3 + heroSpeed / 2 ) : 9;
    const fheroes2::Sprite & sprite4 = fheroes2::AGG::GetICN( ICN::SPANEL, is4 );
    const Rect & rect4 = rects[3];
    fheroes2::Blit( sprite4, display, rect4.x, rect4.y );
    str = _( "Hero Speed" );
    text.Set( str );
    text.Blit( rect4.x + ( rect4.w - text.w() ) / 2, rect4.y - text.h() - textOffset );

    if ( heroSpeed )
        str = GetString( heroSpeed );
    else
        str = _( "off" );
    text.Set( str );
    text.Blit( rect4.x + ( rect4.w - text.w() ) / 2, rect4.y + rect4.h + textOffset );

    // ai move speed
    const int aiSpeed = conf.AIMoveSpeed();
    const u32 is5 = aiSpeed ? ( aiSpeed < 4 ? 4 : 3 + aiSpeed / 2 ) : 9;
    const fheroes2::Sprite & sprite5 = fheroes2::AGG::GetICN( ICN::SPANEL, is5 );
    const Rect & rect5 = rects[4];
    fheroes2::Blit( sprite5, display, rect5.x, rect5.y );
    str = _( "Enemy Speed" );
    text.Set( str );
    text.Blit( rect5.x + ( rect5.w - text.w() ) / 2, rect5.y - text.h() - textOffset );

    if ( aiSpeed )
        str = GetString( aiSpeed );
    else
        str = _( "off" );
    text.Set( str );
    text.Blit( rect5.x + ( rect5.w - text.w() ) / 2, rect5.y + rect5.h + textOffset );

    // scroll speed
    const u32 is6 = ( conf.ScrollSpeed() < SCROLL_FAST2 ? ( conf.ScrollSpeed() < SCROLL_FAST1 ? ( conf.ScrollSpeed() < SCROLL_NORMAL ? 4 : 5 ) : 6 ) : 7 );
    const fheroes2::Sprite & sprite6 = fheroes2::AGG::GetICN( ICN::SPANEL, is6 );
    const Rect & rect6 = rects[5];
    fheroes2::Blit( sprite6, display, rect6.x, rect6.y );
    str = _( "Scroll Speed" );
    text.Set( str );
    text.Blit( rect6.x + ( rect6.w - text.w() ) / 2, rect5.y - text.h() - textOffset );

    str = GetString( conf.ScrollSpeed() );
    text.Set( str );
    text.Blit( rect6.x + ( rect6.w - text.w() ) / 2, rect6.y + rect6.h + textOffset );

    // interface themes
    const fheroes2::Sprite & sprite7 = fheroes2::AGG::GetICN( ICN::SPANEL, ( conf.ExtGameEvilInterface() ? 17 : 16 ) );
    const Rect & rect7 = rects[6];
    fheroes2::Blit( sprite7, display, rect7.x, rect7.y );
    str = _( "Interface" );
    text.Set( str );
    text.Blit( rect7.x + ( rect7.w - text.w() ) / 2, rect7.y - text.h() - textOffset );

    if ( conf.ExtGameEvilInterface() )
        str = _( "Evil" );
    else
        str = _( "Good" );
    text.Set( str );
    text.Blit( rect7.x + ( rect7.w - text.w() ) / 2, rect7.y + rect7.h + textOffset );

    // interface show/hide
    const fheroes2::Sprite & sprite8 = fheroes2::AGG::GetICN( ICN::SPANEL, 16 );
    const fheroes2::Sprite & sprite81 = fheroes2::AGG::GetICN( ICN::ESPANEL, 4 );
    const Rect & rect8 = rects[7];
    str = _( "Interface" );
    text.Set( str );
    text.Blit( rect8.x + ( rect8.w - text.w() ) / 2, rect8.y - text.h() - textOffset );

    if ( conf.ExtGameHideInterface() ) {
        fheroes2::Blit( sprite81, display, rect8.x, rect8.y );
        str = _( "Hide" );
    }
    else {
        fheroes2::Blit( sprite8, display, rect8.x, rect8.y );
        fheroes2::Blit( sprite81, 13, 13, display, rect8.x + 13, rect8.y + 13, 38, 38 );
        str = _( "Show" );
    }
    text.Set( str );
    text.Blit( rect8.x + ( rect8.w - text.w() ) / 2, rect8.y + rect8.h + textOffset );

    // unused
    // const fheroes2::Sprite & sprite9 = fheroes2::AGG::GetICN(ICN::SPANEL, 17);
    const Rect & rect9 = rects[8];
    str = "unused";
    text.Set( str );
    text.Blit( rect9.x + ( rect9.w - text.w() ) / 2, rect9.y + rect9.h + textOffset );
}
