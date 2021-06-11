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

#include "agg_image.h"
#include "audio_mixer.h"
#include "audio_music.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_interface.h"
#include "icn.h"
#include "localevent.h"
#include "settings.h"
#include "system.h"
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

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

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

    for ( int32_t y = 0; y < 3; ++y ) {
        for ( int32_t x = 0; x < 3; ++x ) {
            rects.emplace_back( optionOffset.x + x * optionStep.x, optionOffset.y + y * optionStep.y, optionSprite.width(), optionSprite.height() );
        }
    }

    const fheroes2::Rect & rect1 = rects[0];
    const fheroes2::Rect & rect2 = rects[1];
    const fheroes2::Rect & rect3 = rects[2];
    const fheroes2::Rect & rect4 = rects[3];
    const fheroes2::Rect & rect5 = rects[4];
    const fheroes2::Rect & rect6 = rects[5];
    const fheroes2::Rect & rect7 = rects[6];
    const fheroes2::Rect & rect8 = rects[7];
    const fheroes2::Rect & rect9 = rects[8];

    DrawSystemInfo( rects );

    LocalEvent & le = LocalEvent::Get();

    const fheroes2::Point buttonOffset( 113 + dialogArea.x, 362 + dialogArea.y );
    fheroes2::Button buttonOkay( buttonOffset.x, buttonOffset.y, isEvilInterface ? ICN::SPANBTNE : ICN::SPANBTN, 0, 1 );
    buttonOkay.draw();

    display.render();

    int result = 0;
    bool redraw = false;
    bool saveConfig = false;

    const bool externalMusicSupported = System::IsDirectory( "music" );

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
            if ( type == MUSIC_MIDI_EXPANSION && !conf.isPriceOfLoyaltySupported() )
                ++type;
            if ( type == MUSIC_EXTERNAL && !externalMusicSupported )
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
            const int prevAISpeed = conf.AIMoveSpeed();
            conf.SetAIMoveSpeed( prevAISpeed >= 10 ? 0 : prevAISpeed + 1 );
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

        // toggle manual/auto battles
        if ( le.MouseClickLeft( rect9 ) ) {
            if ( conf.BattleAutoResolve() ) {
                if ( conf.BattleAutoSpellcast() ) {
                    conf.setBattleAutoSpellcast( false );
                }
                else {
                    conf.setBattleAutoResolve( false );
                }
            }
            else {
                conf.setBattleAutoResolve( true );
                conf.setBattleAutoSpellcast( true );
            }

            result |= 0x20;
            redraw = true;
            saveConfig = true;
        }

        if ( le.MousePressRight( rect1 ) )
            Dialog::Message( _( "Music" ), _( "Toggle ambient music level." ), Font::BIG );
        else if ( le.MousePressRight( rect2 ) )
            Dialog::Message( _( "Effects" ), _( "Toggle foreground sounds level." ), Font::BIG );
        else if ( le.MousePressRight( rect3 ) )
            Dialog::Message( _( "Music Type" ), _( "Change the type of music." ), Font::BIG );
        else if ( le.MousePressRight( rect4 ) )
            Dialog::Message( _( "Hero Speed" ), _( "Change the speed at which your heroes move on the main screen." ), Font::BIG );
        else if ( le.MousePressRight( rect5 ) )
            Dialog::Message( _( "Enemy Speed" ), _( "Sets the speed that A.I. heroes move at.  You can also elect not to view A.I. movement at all." ), Font::BIG );
        else if ( le.MousePressRight( rect6 ) )
            Dialog::Message( _( "Scroll Speed" ), _( "Sets the speed at which you scroll the window." ), Font::BIG );
        else if ( le.MousePressRight( rect7 ) )
            Dialog::Message( _( "Interface Type" ), _( "Toggle the type of interface you want to use." ), Font::BIG );
        else if ( le.MousePressRight( rect8 ) )
            Dialog::Message( _( "Interface" ), _( "Toggle interface visibility." ), Font::BIG );
        else if ( le.MousePressRight( rect9 ) )
            Dialog::Message( _( "Battles" ), _( "Toggle instant battle mode." ), Font::BIG );
        else if ( le.MousePressRight( buttonOkay.area() ) )
            Dialog::Message( _( "OK" ), _( "Exit this menu." ), Font::BIG );

        if ( redraw ) {
            fheroes2::Blit( dialog, display, dialogArea.x, dialogArea.y );
            DrawSystemInfo( rects );
            buttonOkay.draw();
            display.render();
            redraw = false;
        }
    }

    if ( saveConfig ) {
        conf.Save( "fheroes2.cfg" );
    }

    return result;
}

void Dialog::DrawSystemInfo( const std::vector<fheroes2::Rect> & rects )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Settings & conf = Settings::Get();

    std::string str;
    Text text;

    const int textOffset = 2;

    // music
    const fheroes2::Sprite & sprite1 = fheroes2::AGG::GetICN( ICN::SPANEL, conf.Music() ? 1 : 0 );
    const fheroes2::Rect & rect1 = rects[0];
    fheroes2::Blit( sprite1, display, rect1.x, rect1.y );
    str = _( "Music" );
    text.Set( str, Font::SMALL );
    text.Blit( rect1.x + ( rect1.width - text.w() ) / 2, rect1.y - text.h() - textOffset );

    if ( conf.Music() && conf.MusicVolume() )
        str = std::to_string( conf.MusicVolume() );
    else
        str = _( "off" );
    text.Set( str );
    text.Blit( rect1.x + ( rect1.width - text.w() ) / 2, rect1.y + rect1.height + textOffset );

    // sound
    const fheroes2::Sprite & sprite2 = fheroes2::AGG::GetICN( ICN::SPANEL, conf.Sound() ? 3 : 2 );
    const fheroes2::Rect & rect2 = rects[1];
    fheroes2::Blit( sprite2, display, rect2.x, rect2.y );
    str = _( "Effects" );
    text.Set( str, Font::SMALL );
    text.Blit( rect2.x + ( rect2.width - text.w() ) / 2, rect2.y - text.h() - textOffset );

    if ( conf.Sound() && conf.SoundVolume() )
        str = std::to_string( conf.SoundVolume() );
    else
        str = _( "off" );
    text.Set( str, Font::SMALL );
    text.Blit( rect2.x + ( rect2.width - text.w() ) / 2, rect2.height + rect2.y + textOffset );

    // Music Type
    const MusicSource musicType = conf.MusicType();
    const fheroes2::Sprite & sprite3 = fheroes2::AGG::GetICN( ICN::SPANEL, ( musicType == MUSIC_CDROM || musicType == MUSIC_EXTERNAL ) ? 11 : 10 );
    const fheroes2::Rect & rect3 = rects[2];
    fheroes2::Blit( sprite3, display, rect3.x, rect3.y );
    str = _( "Music Type" );
    text.Set( str, Font::SMALL );
    text.Blit( rect3.x + ( rect3.width - text.w() ) / 2, rect3.y - text.h() - textOffset );

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
    text.Blit( rect3.x + ( rect3.width - text.w() ) / 2, rect3.y + rect3.height + textOffset );

    // hero move speed
    const int heroSpeed = conf.HeroesMoveSpeed();
    const u32 is4 = heroSpeed ? ( heroSpeed < 4 ? 4 : 3 + heroSpeed / 2 ) : 9;
    const fheroes2::Sprite & sprite4 = fheroes2::AGG::GetICN( ICN::SPANEL, is4 );
    const fheroes2::Rect & rect4 = rects[3];
    fheroes2::Blit( sprite4, display, rect4.x, rect4.y );
    str = _( "Hero Speed" );
    text.Set( str );
    text.Blit( rect4.x + ( rect4.width - text.w() ) / 2, rect4.y - text.h() - textOffset );

    if ( heroSpeed == 10 ) {
        str = _( "Jump" );
    }
    else {
        str = std::to_string( heroSpeed );
    }

    text.Set( str );
    text.Blit( rect4.x + ( rect4.width - text.w() ) / 2, rect4.y + rect4.height + textOffset );

    // ai move speed
    const int aiSpeed = conf.AIMoveSpeed();
    const u32 is5 = ( aiSpeed > 0 ) ? ( aiSpeed < 4 ? 4 : 3 + aiSpeed / 2 ) : 9;
    const fheroes2::Sprite & sprite5 = fheroes2::AGG::GetICN( ICN::SPANEL, is5 );
    const fheroes2::Rect & rect5 = rects[4];
    fheroes2::Blit( sprite5, display, rect5.x, rect5.y );
    str = _( "Enemy Speed" );
    text.Set( str );
    text.Blit( rect5.x + ( rect5.width - text.w() ) / 2, rect5.y - text.h() - textOffset );

    if ( aiSpeed == 0 ) {
        str = _( "Don't Show" );
    }
    else if ( aiSpeed == 10 ) {
        str = _( "Jump" );
    }
    else {
        str = std::to_string( aiSpeed );
    }

    text.Set( str );
    text.Blit( rect5.x + ( rect5.width - text.w() ) / 2, rect5.y + rect5.height + textOffset );

    // scroll speed
    const u32 is6 = ( conf.ScrollSpeed() < SCROLL_FAST2 ? ( conf.ScrollSpeed() < SCROLL_FAST1 ? ( conf.ScrollSpeed() < SCROLL_NORMAL ? 4 : 5 ) : 6 ) : 7 );
    const fheroes2::Sprite & sprite6 = fheroes2::AGG::GetICN( ICN::SPANEL, is6 );
    const fheroes2::Rect & rect6 = rects[5];
    fheroes2::Blit( sprite6, display, rect6.x, rect6.y );
    str = _( "Scroll Speed" );
    text.Set( str );
    text.Blit( rect6.x + ( rect6.width - text.w() ) / 2, rect5.y - text.h() - textOffset );

    str = std::to_string( conf.ScrollSpeed() );
    text.Set( str );
    text.Blit( rect6.x + ( rect6.width - text.w() ) / 2, rect6.y + rect6.height + textOffset );

    const bool isEvilInterface = conf.ExtGameEvilInterface();

    // interface themes
    const fheroes2::Sprite & sprite7 = fheroes2::AGG::GetICN( ICN::SPANEL, ( isEvilInterface ? 17 : 16 ) );
    const fheroes2::Rect & rect7 = rects[6];
    fheroes2::Blit( sprite7, display, rect7.x, rect7.y );
    str = _( "Interface Type" );
    text.Set( str );
    text.Blit( rect7.x + ( rect7.width - text.w() ) / 2, rect7.y - text.h() - textOffset );

    if ( isEvilInterface )
        str = _( "Evil" );
    else
        str = _( "Good" );
    text.Set( str );
    text.Blit( rect7.x + ( rect7.width - text.w() ) / 2, rect7.y + rect7.height + textOffset );

    // interface show/hide
    const fheroes2::Sprite & sprite8 = fheroes2::AGG::GetICN( ICN::SPANEL, 16 );
    const fheroes2::Sprite & sprite81 = fheroes2::AGG::GetICN( ICN::ESPANEL, 4 );
    const fheroes2::Rect & rect8 = rects[7];
    str = _( "Interface" );
    text.Set( str );
    text.Blit( rect8.x + ( rect8.width - text.w() ) / 2, rect8.y - text.h() - textOffset );

    if ( conf.ExtGameHideInterface() ) {
        fheroes2::Blit( sprite81, display, rect8.x, rect8.y );
        str = _( "Hide" );
    }
    else {
        fheroes2::Blit( sprite8, display, rect8.x, rect8.y );
        fheroes2::Blit( sprite81, 14, 14, display, rect8.x + 14, rect8.y + 14, 37, 37 );
        str = _( "Show" );
    }
    text.Set( str );
    text.Blit( rect8.x + ( rect8.width - text.w() ) / 2, rect8.y + rect8.height + textOffset );

    // auto-battles
    const fheroes2::Rect & rect9 = rects[8];
    str = _( "Battles" );
    text.Set( str );
    text.Blit( rect9.x + ( rect9.width - text.w() ) / 2, rect9.y - text.h() - textOffset );

    if ( conf.BattleAutoResolve() ) {
        const bool spellcast = conf.BattleAutoSpellcast();
        str = spellcast ? _( "Auto Resolve" ) : str = _( "Auto, No Spells" );

        const fheroes2::Sprite & sprite9 = fheroes2::AGG::GetICN( ICN::CSPANEL, spellcast ? 7 : 6 );
        fheroes2::Blit( sprite9, display, rect9.x, rect9.y );
    }
    else {
        str = _( "Manual" );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::SPANEL, 18 ), display, rect9.x, rect9.y );
    }
    text.Set( str );
    text.Blit( rect9.x + ( rect9.width - text.w() ) / 2, rect9.y + rect9.height + textOffset );
}
