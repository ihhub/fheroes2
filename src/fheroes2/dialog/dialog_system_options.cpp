/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
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
#include "game_hotkeys.h"
#include "game_interface.h"
#include "icn.h"
#include "localevent.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

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

    void drawOption( const fheroes2::Rect & optionRoi, const fheroes2::Sprite & icon, const std::string & titleText, const std::string & valueText )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::FontType smallWhite = fheroes2::FontType::smallWhite();

        const fheroes2::Text title( titleText, smallWhite );
        const fheroes2::Text value( valueText, smallWhite );

        const int16_t textMaxWidth = 87;

        title.draw( optionRoi.x - 12, optionRoi.y - title.height( textMaxWidth ), textMaxWidth, display );
        value.draw( optionRoi.x + ( optionRoi.width - value.width() ) / 2, optionRoi.y + optionRoi.height + 4, display );

        fheroes2::Blit( icon, display, optionRoi.x, optionRoi.y );
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

            if ( le.MouseClickLeft( buttonOkay.area() ) || Game::HotKeyCloseWindow() ) {
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

            const fheroes2::FontType normalYellow = fheroes2::FontType::normalYellow();
            const fheroes2::FontType normalWhite = fheroes2::FontType::normalWhite();

            if ( le.MousePressRight( musicVolumeRoi ) ) {
                fheroes2::Text header( _( "Music" ), normalYellow );
                fheroes2::Text body( _( "Toggle ambient music level." ), normalWhite );

                fheroes2::showMessage( header, body, 0 );
            }

            else if ( le.MousePressRight( soundVolumeRoi ) ) {
                fheroes2::Text header( _( "Effects" ), normalYellow );
                fheroes2::Text body( _( "Toggle foreground sounds level." ), normalWhite );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( musicTypeRoi ) ) {
                fheroes2::Text header( _( "Music Type" ), normalYellow );
                fheroes2::Text body( _( "Change the type of music." ), normalWhite );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( heroSpeedRoi ) ) {
                fheroes2::Text header( _( "Hero Speed" ), normalYellow );
                fheroes2::Text body( _( "Change the speed at which your heroes move on the main screen." ), normalWhite );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( aiSpeedRoi ) ) {
                fheroes2::Text header( _( "Enemy Speed" ), normalYellow );
                fheroes2::Text body( _( "Sets the speed that A.I. heroes move at.  You can also elect not to view A.I. movement at all." ), normalWhite );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( scrollSpeedRoi ) ) {
                fheroes2::Text header( _( "Scroll Speed" ), normalYellow );
                fheroes2::Text body( _( "Sets the speed at which you scroll the window." ), normalWhite );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( interfaceTypeRoi ) ) {
                fheroes2::Text header( _( "Interface Type" ), normalYellow );
                fheroes2::Text body( _( "Toggle the type of interface you want to use." ), normalWhite );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( interfaceStateRoi ) ) {
                fheroes2::Text header( _( "Interface" ), normalYellow );
                fheroes2::Text body( _( "Toggle interface visibility." ), normalWhite );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( battleResolveRoi ) ) {
                fheroes2::Text header( _( "Battles" ), normalYellow );
                fheroes2::Text body( _( "Toggle instant battle mode." ), normalWhite );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( buttonOkay.area() ) ) {
                fheroes2::Text header( _( "Okay" ), normalYellow );
                fheroes2::Text body( _( "Exit this menu." ), normalWhite );

                fheroes2::showMessage( header, body, 0 );
            }

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

                gamearea.SetCenterInPixels( prevCenter + fheroes2::Point( newRoi.x + newRoi.width / 2, newRoi.y + newRoi.height / 2 )
                                            - fheroes2::Point( prevRoi.x + prevRoi.width / 2, prevRoi.y + prevRoi.height / 2 ) );

                // We need to redraw radar first due to the nature of restorers. Only then we can redraw everything.
                basicInterface.Redraw( Interface::REDRAW_RADAR );
                basicInterface.Redraw( Interface::REDRAW_ALL );

                action = openSystemOptionsDialog();
                break;
            }
            case DialogAction::SaveConfiguration:
                Settings::Get().Save( Settings::configFileName );
                return;
            default:
                break;
            }
        }

        if ( saveConfiguration ) {
            Settings::Get().Save( Settings::configFileName );
        }
    }
}
