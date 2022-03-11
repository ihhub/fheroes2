/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021 - 2022                                             *
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

#include "dialog_system_options.h"
#include "agg_image.h"
#include "audio.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_interface.h"
#include "icn.h"
#include "localevent.h"
#include "screen.h"
#include "settings.h"
#include "text.h"
#include "translations.h"
#include "ui_button.h"

#include <cassert>

namespace
{
    enum class DialogAction : int
    {
        Open,
        ChangeInterfaceTheme,
        UpdateInterface,
        SaveConfiguration,
        Close
    };

    const int textOffset = 2;

    void drawOption( const fheroes2::Rect & optionRoi, const fheroes2::Sprite & icon, const std::string & title, const std::string & value )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::Blit( icon, display, optionRoi.x, optionRoi.y );
        Text text( title, Font::SMALL );
        text.Blit( optionRoi.x + ( optionRoi.width - text.w() ) / 2, optionRoi.y - text.h() - textOffset );

        text.Set( value );
        text.Blit( optionRoi.x + ( optionRoi.width - text.w() ) / 2, optionRoi.y + optionRoi.height + textOffset );
    }

    void drawDialog( const std::vector<fheroes2::Rect> & rects )
    {
        assert( rects.size() == 9 );

        const Settings & conf = Settings::Get();

        // Music volume.
        const fheroes2::Sprite & musicVolumeIcon = fheroes2::AGG::GetICN( ICN::SPANEL, Audio::isValid() ? 1 : 0 );
        std::string value;
        if ( Audio::isValid() && conf.MusicVolume() ) {
            value = std::to_string( conf.MusicVolume() );
        }
        else {
            value = _( "off" );
        }

        drawOption( rects[0], musicVolumeIcon, _( "Music" ), value );

        // Sound volume.
        const fheroes2::Sprite & soundVolumeOption = fheroes2::AGG::GetICN( ICN::SPANEL, Audio::isValid() ? 3 : 2 );
        if ( Audio::isValid() && conf.SoundVolume() ) {
            value = std::to_string( conf.SoundVolume() );
        }
        else {
            value = _( "off" );
        }

        drawOption( rects[1], soundVolumeOption, _( "Effects" ), value );

        // Music Type.
        const MusicSource musicType = conf.MusicType();
        const fheroes2::Sprite & musicTypeIcon = fheroes2::AGG::GetICN( ICN::SPANEL, musicType == MUSIC_EXTERNAL ? 11 : 10 );
        if ( musicType == MUSIC_MIDI_ORIGINAL ) {
            value = _( "MIDI" );
        }
        else if ( musicType == MUSIC_MIDI_EXPANSION ) {
            value = _( "MIDI Expansion" );
        }
        else if ( musicType == MUSIC_EXTERNAL ) {
            value = _( "External" );
        }

        drawOption( rects[2], musicTypeIcon, _( "Music Type" ), value );

        // Hero's movement speed.
        const int heroSpeed = conf.HeroesMoveSpeed();
        uint32_t heroSpeedIconId = 9;
        if ( heroSpeed >= 4 ) {
            heroSpeedIconId = 3 + heroSpeed / 2;
        }
        else if ( heroSpeed > 0 ) {
            heroSpeedIconId = 4;
        }

        const fheroes2::Sprite & heroSpeedIcon = fheroes2::AGG::GetICN( ICN::SPANEL, heroSpeedIconId );
        if ( heroSpeed == 10 ) {
            value = _( "Jump" );
        }
        else {
            value = std::to_string( heroSpeed );
        }

        drawOption( rects[3], heroSpeedIcon, _( "Hero Speed" ), value );

        // AI's movement speed.
        const int aiSpeed = conf.AIMoveSpeed();
        uint32_t aiSpeedIconId = 9;
        if ( aiSpeed >= 4 ) {
            aiSpeedIconId = 3 + aiSpeed / 2;
        }
        else if ( aiSpeed > 0 ) {
            aiSpeedIconId = 4;
        }

        const fheroes2::Sprite & aiSpeedIcon = fheroes2::AGG::GetICN( ICN::SPANEL, aiSpeedIconId );
        if ( aiSpeed == 0 ) {
            value = _( "Don't Show" );
        }
        else if ( aiSpeed == 10 ) {
            value = _( "Jump" );
        }
        else {
            value = std::to_string( aiSpeed );
        }

        drawOption( rects[4], aiSpeedIcon, _( "Enemy Speed" ), value );

        // Scrolling speed.
        const int scrollSpeed = conf.ScrollSpeed();
        uint32_t scrollSpeedIconId = 7;
        if ( scrollSpeed < SCROLL_NORMAL ) {
            scrollSpeedIconId = 4;
        }
        else if ( scrollSpeed < SCROLL_FAST1 ) {
            scrollSpeedIconId = 5;
        }
        else if ( scrollSpeed < SCROLL_FAST2 ) {
            scrollSpeedIconId = 6;
        }

        const fheroes2::Sprite & scrollSpeedIcon = fheroes2::AGG::GetICN( ICN::SPANEL, scrollSpeedIconId );
        drawOption( rects[5], scrollSpeedIcon, _( "Scroll Speed" ), std::to_string( scrollSpeed ) );

        // Interface theme.
        const bool isEvilInterface = conf.ExtGameEvilInterface();
        const fheroes2::Sprite & interfaceThemeIcon = fheroes2::AGG::GetICN( ICN::SPANEL, isEvilInterface ? 17 : 16 );
        if ( isEvilInterface ) {
            value = _( "Evil" );
        }
        else {
            value = _( "Good" );
        }

        drawOption( rects[6], interfaceThemeIcon, _( "Interface Type" ), value );

        // Interface show/hide state.
        const bool isHiddenInterface = conf.ExtGameHideInterface();
        const fheroes2::Sprite & interfaceStateIcon = isHiddenInterface ? fheroes2::AGG::GetICN( ICN::ESPANEL, 4 ) : fheroes2::AGG::GetICN( ICN::SPANEL, 16 );
        if ( isHiddenInterface ) {
            value = _( "Hide" );
        }
        else {
            value = _( "Show" );
        }

        drawOption( rects[7], interfaceStateIcon, _( "Interface" ), value );

        // Auto-battles.
        if ( conf.BattleAutoResolve() ) {
            const bool spellcast = conf.BattleAutoSpellcast();
            value = spellcast ? _( "Auto Resolve" ) : _( "Auto, No Spells" );

            const fheroes2::Sprite & autoBattleIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, spellcast ? 7 : 6 );
            drawOption( rects[8], autoBattleIcon, _( "Battles" ), value );
        }
        else {
            const fheroes2::Sprite & autoBattleIcon = fheroes2::AGG::GetICN( ICN::SPANEL, 18 );
            drawOption( rects[8], autoBattleIcon, _( "Battles" ), _( "Manual" ) );
        }
    }

    DialogAction openSystemOptionsDialog()
    {
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.ExtGameEvilInterface();

        fheroes2::Display & display = fheroes2::Display::instance();

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

        std::vector<fheroes2::Rect> roi;

        for ( int32_t y = 0; y < 3; ++y ) {
            for ( int32_t x = 0; x < 3; ++x ) {
                roi.emplace_back( optionOffset.x + x * optionStep.x, optionOffset.y + y * optionStep.y, optionSprite.width(), optionSprite.height() );
            }
        }

        const fheroes2::Rect & musicVolumeRoi = roi[0];
        const fheroes2::Rect & soundVolumeRoi = roi[1];
        const fheroes2::Rect & musicTypeRoi = roi[2];
        const fheroes2::Rect & heroSpeedRoi = roi[3];
        const fheroes2::Rect & aiSpeedRoi = roi[4];
        const fheroes2::Rect & scrollSpeedRoi = roi[5];
        const fheroes2::Rect & interfaceTypeRoi = roi[6];
        const fheroes2::Rect & interfaceStateRoi = roi[7];
        const fheroes2::Rect & battleResolveRoi = roi[8];

        drawDialog( roi );

        const fheroes2::Point buttonOffset( 112 + dialogArea.x, 362 + dialogArea.y );
        fheroes2::Button buttonOkay( buttonOffset.x, buttonOffset.y, isEvilInterface ? ICN::SPANBTNE : ICN::SPANBTN, 0, 1 );
        buttonOkay.draw();

        display.render();

        bool saveConfig = false;

        // dialog menu loop
        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonOkay.area() ) ? buttonOkay.drawOnPress() : buttonOkay.drawOnRelease();

            if ( le.MouseClickLeft( buttonOkay.area() ) || HotKeyCloseWindow ) {
                break;
            }

            // set music or sound volume
            bool saveMusicVolume = false;
            bool saveSoundVolume = false;
            if ( Audio::isValid() ) {
                if ( le.MouseClickLeft( musicVolumeRoi ) ) {
                    conf.SetMusicVolume( ( conf.MusicVolume() + 1 ) % 11 );
                    saveMusicVolume = true;
                }
                else if ( le.MouseWheelUp( musicVolumeRoi ) ) {
                    conf.SetMusicVolume( conf.MusicVolume() + 1 );
                    saveMusicVolume = true;
                }
                else if ( le.MouseWheelDn( musicVolumeRoi ) ) {
                    conf.SetMusicVolume( conf.MusicVolume() - 1 );
                    saveMusicVolume = true;
                }
                if ( saveMusicVolume ) {
                    Music::Volume( static_cast<int16_t>( Mixer::MaxVolume() * conf.MusicVolume() / 10 ) );
                }

                if ( le.MouseClickLeft( soundVolumeRoi ) ) {
                    conf.SetSoundVolume( ( conf.SoundVolume() + 1 ) % 11 );
                    saveSoundVolume = true;
                }
                else if ( le.MouseWheelUp( soundVolumeRoi ) ) {
                    conf.SetSoundVolume( conf.SoundVolume() + 1 );
                    saveSoundVolume = true;
                }
                else if ( le.MouseWheelDn( soundVolumeRoi ) ) {
                    conf.SetSoundVolume( conf.SoundVolume() - 1 );
                    saveSoundVolume = true;
                }
                if ( saveSoundVolume ) {
                    Game::EnvironmentSoundMixer();
                }
            }

            // set music type
            bool saveMusicType = false;
            if ( le.MouseClickLeft( musicTypeRoi ) ) {
                int type = conf.MusicType() + 1;
                // If there's no expansion files we skip this option
                if ( type == MUSIC_MIDI_EXPANSION && !conf.isPriceOfLoyaltySupported() )
                    ++type;

                const Game::MusicRestorer musicRestorer;

                conf.SetMusicType( type > MUSIC_EXTERNAL ? 0 : type );

                Game::SetCurrentMusic( MUS::UNKNOWN );

                saveMusicType = true;
            }

            // set hero speed
            bool saveHeroSpeed = false;
            if ( le.MouseClickLeft( heroSpeedRoi ) ) {
                conf.SetHeroesMoveSpeed( conf.HeroesMoveSpeed() % 10 + 1 );
                saveHeroSpeed = true;
            }
            else if ( le.MouseWheelUp( heroSpeedRoi ) ) {
                conf.SetHeroesMoveSpeed( conf.HeroesMoveSpeed() + 1 );
                saveHeroSpeed = true;
            }
            else if ( le.MouseWheelDn( heroSpeedRoi ) ) {
                conf.SetHeroesMoveSpeed( conf.HeroesMoveSpeed() - 1 );
                saveHeroSpeed = true;
            }

            // set ai speed
            bool saveAISpeed = false;
            if ( le.MouseClickLeft( aiSpeedRoi ) ) {
                conf.SetAIMoveSpeed( ( conf.AIMoveSpeed() + 1 ) % 11 );
                saveAISpeed = true;
            }
            else if ( le.MouseWheelUp( aiSpeedRoi ) ) {
                conf.SetAIMoveSpeed( conf.AIMoveSpeed() + 1 );
                saveAISpeed = true;
            }
            else if ( le.MouseWheelDn( aiSpeedRoi ) ) {
                conf.SetAIMoveSpeed( conf.AIMoveSpeed() - 1 );
                saveAISpeed = true;
            }

            if ( saveHeroSpeed || saveAISpeed ) {
                Game::UpdateGameSpeed();
            }

            // set scroll speed
            bool saveScrollSpeed = false;
            if ( le.MouseClickLeft( scrollSpeedRoi ) ) {
                conf.SetScrollSpeed( conf.ScrollSpeed() % SCROLL_FAST2 + 1 );
                saveScrollSpeed = true;
            }
            else if ( le.MouseWheelUp( scrollSpeedRoi ) ) {
                conf.SetScrollSpeed( conf.ScrollSpeed() + 1 );
                saveScrollSpeed = true;
            }
            else if ( le.MouseWheelDn( scrollSpeedRoi ) ) {
                conf.SetScrollSpeed( conf.ScrollSpeed() - 1 );
                saveScrollSpeed = true;
            }

            // set interface theme
            if ( le.MouseClickLeft( interfaceTypeRoi ) ) {
                return DialogAction::ChangeInterfaceTheme;
            }

            // set interface hide/show
            if ( le.MouseClickLeft( interfaceStateRoi ) ) {
                return DialogAction::UpdateInterface;
            }

            // toggle manual/auto battles
            bool saveAutoBattle = false;
            if ( le.MouseClickLeft( battleResolveRoi ) ) {
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
                saveAutoBattle = true;
            }

            if ( le.MousePressRight( musicVolumeRoi ) )
                Dialog::Message( _( "Music" ), _( "Toggle ambient music level." ), Font::BIG );
            else if ( le.MousePressRight( soundVolumeRoi ) )
                Dialog::Message( _( "Effects" ), _( "Toggle foreground sounds level." ), Font::BIG );
            else if ( le.MousePressRight( musicTypeRoi ) )
                Dialog::Message( _( "Music Type" ), _( "Change the type of music." ), Font::BIG );
            else if ( le.MousePressRight( heroSpeedRoi ) )
                Dialog::Message( _( "Hero Speed" ), _( "Change the speed at which your heroes move on the main screen." ), Font::BIG );
            else if ( le.MousePressRight( aiSpeedRoi ) )
                Dialog::Message( _( "Enemy Speed" ), _( "Sets the speed that A.I. heroes move at.  You can also elect not to view A.I. movement at all." ), Font::BIG );
            else if ( le.MousePressRight( scrollSpeedRoi ) )
                Dialog::Message( _( "Scroll Speed" ), _( "Sets the speed at which you scroll the window." ), Font::BIG );
            else if ( le.MousePressRight( interfaceTypeRoi ) )
                Dialog::Message( _( "Interface Type" ), _( "Toggle the type of interface you want to use." ), Font::BIG );
            else if ( le.MousePressRight( interfaceStateRoi ) )
                Dialog::Message( _( "Interface" ), _( "Toggle interface visibility." ), Font::BIG );
            else if ( le.MousePressRight( battleResolveRoi ) )
                Dialog::Message( _( "Battles" ), _( "Toggle instant battle mode." ), Font::BIG );
            else if ( le.MousePressRight( buttonOkay.area() ) )
                Dialog::Message( _( "Okay" ), _( "Exit this menu." ), Font::BIG );

            if ( saveMusicVolume || saveSoundVolume || saveMusicType || saveHeroSpeed || saveAISpeed || saveScrollSpeed || saveAutoBattle ) {
                // redraw
                fheroes2::Blit( dialog, display, dialogArea.x, dialogArea.y );
                drawDialog( roi );
                buttonOkay.draw();
                display.render();

                saveConfig = true;
            }
        }

        if ( saveConfig ) {
            return DialogAction::SaveConfiguration;
        }

        return DialogAction::Close;
    }
}

namespace fheroes2
{
    void showSystemOptionsDialog()
    {
        // We should make file writing only once.
        bool saveConfiguration = false;

        DialogAction action = DialogAction::Open;

        while ( action != DialogAction::Close ) {
            switch ( action ) {
            case DialogAction::Open:
                action = openSystemOptionsDialog();
                break;
            case DialogAction::ChangeInterfaceTheme: {
                Settings & conf = Settings::Get();
                conf.SetEvilInterface( !conf.ExtGameEvilInterface() );
                saveConfiguration = true;

                Interface::Basic & basicInterface = Interface::Basic::Get();
                Interface::GameArea & gamearea = basicInterface.GetGameArea();
                const fheroes2::Point prevCenter = gamearea.getCurrentCenterInPixels();

                basicInterface.Reset();
                gamearea.SetCenterInPixels( prevCenter );
                basicInterface.Redraw( Interface::REDRAW_ALL );

                action = openSystemOptionsDialog();
                break;
            }
            case DialogAction::UpdateInterface: {
                Settings & conf = Settings::Get();
                conf.SetHideInterface( !conf.ExtGameHideInterface() );
                saveConfiguration = true;

                Interface::Basic & basicInterface = Interface::Basic::Get();
                Interface::GameArea & gamearea = basicInterface.GetGameArea();
                const fheroes2::Point prevCenter = gamearea.getCurrentCenterInPixels();
                const fheroes2::Rect prevRoi = gamearea.GetROI();

                basicInterface.SetHideInterface( conf.ExtGameHideInterface() );

                basicInterface.Reset();

                const fheroes2::Rect newRoi = gamearea.GetROI();

                gamearea.SetCenterInPixels( prevCenter + fheroes2::Point( newRoi.width / 2, newRoi.height / 2 ) + fheroes2::Point( newRoi.x, newRoi.y )
                                            - fheroes2::Point( prevRoi.width / 2, prevRoi.height / 2 ) - fheroes2::Point( prevRoi.x, prevRoi.y ) );

                // We need to redraw radar first due to the nature of restorers. Only then we can redraw everything.
                basicInterface.Redraw( Interface::REDRAW_RADAR );
                basicInterface.Redraw( Interface::REDRAW_ALL );

                action = openSystemOptionsDialog();
                break;
            }
            case DialogAction::SaveConfiguration:
                Settings::Get().Save( "fheroes2.cfg" );
                return;
            default:
                break;
            }
        }

        if ( saveConfiguration ) {
            Settings::Get().Save( "fheroes2.cfg" );
        }
    }
}
