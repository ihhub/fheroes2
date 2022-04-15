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

#include "dialog_game_settings.h"
#include "agg_image.h"
#include "audio.h"
#include "dialog.h"
#include "dialog_language_selection.h"
#include "dialog_resolution.h"
#include "game.h"
#include "game_interface.h"
#include "game_mainmenu_ui.h"
#include "icn.h"
#include "localevent.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "ui_text.h"
#include "ui_window.h"

#include <cassert>

namespace
{
    const fheroes2::Size offsetBetweenOptions{ 92, 110 };

    const int32_t titleOffset = 10;
    const int32_t nameOffset = 10;
    const fheroes2::Point optionOffset{ 36, 47 };
    const int32_t optionWindowSize{ 65 };

    const fheroes2::Rect languageRoi{ optionOffset.x, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect resolutionRoi{ optionOffset.x + offsetBetweenOptions.width, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect optionsRoi{ optionOffset.x + offsetBetweenOptions.width * 2, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect musicVolumeRoi{ optionOffset.x, optionOffset.y + offsetBetweenOptions.height, optionWindowSize, optionWindowSize };
    const fheroes2::Rect soundVolumeRoi{ optionOffset.x + offsetBetweenOptions.width, optionOffset.y + offsetBetweenOptions.height, optionWindowSize, optionWindowSize };
    const fheroes2::Rect musicTypeRoi{ optionOffset.x + offsetBetweenOptions.width * 2, optionOffset.y + offsetBetweenOptions.height, optionWindowSize,
                                       optionWindowSize };

    void drawOption( const fheroes2::Rect & optionRoi, const char * titleText, const char * nameText, const int icnId, const uint32_t icnIndex )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::Text title( titleText, fheroes2::FontType::smallWhite() );
        const fheroes2::Text name( nameText, fheroes2::FontType::smallWhite() );

        title.draw( optionRoi.x + ( languageRoi.width - title.width() ) / 2, optionRoi.y - titleOffset, display );
        name.draw( optionRoi.x + ( languageRoi.width - name.width() ) / 2, optionRoi.y + languageRoi.height + nameOffset, display );

        const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( icnId, icnIndex );
        fheroes2::Blit( icon, 0, 0, display, optionRoi.x, optionRoi.y, icon.width(), icon.height() );
    }

    void drawLanguage( const fheroes2::Rect & optionRoi )
    {
        const fheroes2::SupportedLanguage currentLanguage = fheroes2::getLanguageFromAbbreviation( Settings::Get().getGameLanguage() );
        fheroes2::LanguageSwitcher languageSwitcher( currentLanguage );

        drawOption( optionRoi, _( "Language" ), fheroes2::getLanguageName( currentLanguage ), ICN::SPANEL, 18 );
    }

    void drawResolution( const fheroes2::Rect & optionRoi )
    {
        const fheroes2::Display & display = fheroes2::Display::instance();
        const std::string resolutionName = std::to_string( display.width() ) + 'x' + std::to_string( display.height() );

        drawOption( optionRoi, _( "Resolution" ), resolutionName.c_str(), ICN::SPANEL, 16 );
    }

    void drawExperimentalOptions( const fheroes2::Rect & optionRoi )
    {
        drawOption( optionRoi, _( "Experimental" ), _( "Settings" ), ICN::SPANEL, 14 );
    }

    void drawMusicVolumeOptions( const fheroes2::Rect & optionRoi )
    {
        const Settings & configuration = Settings::Get();

        std::string value;
        if ( Audio::isValid() && configuration.MusicVolume() ) {
            value = std::to_string( configuration.MusicVolume() );
        }
        else {
            value = _( "off" );
        }

        drawOption( optionRoi, _( "Music" ), value.c_str(), ICN::SPANEL, Audio::isValid() ? 1 : 0 );
    }

    void drawSoundVolumeOptions( const fheroes2::Rect & optionRoi )
    {
        const Settings & configuration = Settings::Get();

        std::string value;
        if ( Audio::isValid() && configuration.SoundVolume() ) {
            value = std::to_string( configuration.SoundVolume() );
        }
        else {
            value = _( "off" );
        }

        drawOption( optionRoi, _( "Effects" ), value.c_str(), ICN::SPANEL, Audio::isValid() ? 3 : 2 );
    }

    void drawMusicTypeOptions( const fheroes2::Rect & optionRoi )
    {
        const Settings & configuration = Settings::Get();

        std::string value;
        const MusicSource musicType = configuration.MusicType();
        if ( musicType == MUSIC_MIDI_ORIGINAL ) {
            value = _( "MIDI" );
        }
        else if ( musicType == MUSIC_MIDI_EXPANSION ) {
            value = _( "MIDI Expansion" );
        }
        else if ( musicType == MUSIC_EXTERNAL ) {
            value = _( "External" );
        }

        drawOption( optionRoi, _( "Music Type" ), value.c_str(), ICN::SPANEL, Audio::isValid() ? 11 : 10 );
    }

    enum class SelectedWindow : int
    {
        Configuration,
        Resolution,
        Language,
        Options,
        UpdateSettings,
        Exit
    };

    SelectedWindow showConfigurationWindow()
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.ExtGameEvilInterface();
        const int dialogIcnId = isEvilInterface ? ICN::CSPANBKE : ICN::CSPANBKG;
        const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( dialogIcnId, 0 );
        const fheroes2::Sprite & dialogShadow = fheroes2::AGG::GetICN( dialogIcnId, 1 );

        const fheroes2::Point dialogOffset( ( display.width() - dialog.width() ) / 2, ( display.height() - dialog.height() ) / 2 );
        const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

        const fheroes2::Rect windowRoi{ dialogOffset.x, dialogOffset.y, dialog.width(), dialog.height() };

        const fheroes2::ImageRestorer restorer( display, shadowOffset.x, shadowOffset.y, dialog.width() + BORDERWIDTH, dialog.height() + BORDERWIDTH );

        fheroes2::Blit( dialogShadow, display, windowRoi.x - BORDERWIDTH, windowRoi.y + BORDERWIDTH );
        fheroes2::Blit( dialog, display, windowRoi.x, windowRoi.y );

        const int buttonIcnId = isEvilInterface ? ICN::NON_UNIFORM_EVIL_OKAY_BUTTON : ICN::NON_UNIFORM_GOOD_OKAY_BUTTON;
        const fheroes2::Sprite & buttonOkayReleased = fheroes2::AGG::GetICN( buttonIcnId, 0 );
        const fheroes2::Sprite & buttonOkayPressed = fheroes2::AGG::GetICN( buttonIcnId, 1 );

        const fheroes2::Rect windowLanguageRoi( languageRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowResolutionRoi( resolutionRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowOptionsRoi( optionsRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowMusicVolumeRoi( musicVolumeRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowSoundVolumeRoi( soundVolumeRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowMusicTypeRoi( musicTypeRoi + windowRoi.getPosition() );

        drawLanguage( windowLanguageRoi );
        drawResolution( windowResolutionRoi );
        drawExperimentalOptions( windowOptionsRoi );
        drawMusicVolumeOptions( windowMusicVolumeRoi );
        drawSoundVolumeOptions( windowSoundVolumeRoi );
        drawMusicTypeOptions( windowMusicTypeRoi );

        fheroes2::ButtonSprite okayButton( windowRoi.x + 112, windowRoi.y + 252, buttonOkayReleased, buttonOkayPressed );
        okayButton.draw();

        display.render();

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            if ( le.MousePressLeft( okayButton.area() ) ) {
                okayButton.drawOnPress();
            }
            else {
                okayButton.drawOnRelease();
            }

            if ( le.MouseClickLeft( okayButton.area() ) || HotKeyCloseWindow ) {
                break;
            }
            if ( le.MouseClickLeft( windowLanguageRoi ) ) {
                return SelectedWindow::Language;
            }
            if ( le.MouseClickLeft( windowResolutionRoi ) ) {
                return SelectedWindow::Resolution;
            }
            if ( le.MouseClickLeft( windowOptionsRoi ) ) {
                return SelectedWindow::Options;
            }
            if ( le.MouseClickLeft( windowMusicTypeRoi ) ) {
                int type = conf.MusicType() + 1;
                // If there's no expansion files we skip this option
                if ( type == MUSIC_MIDI_EXPANSION && !conf.isPriceOfLoyaltySupported() ) {
                    ++type;
                }

                const Game::MusicRestorer musicRestorer;

                conf.SetMusicType( type > MUSIC_EXTERNAL ? 0 : type );

                Game::SetCurrentMusic( MUS::UNKNOWN );

                return SelectedWindow::UpdateSettings;
            }

            // Set music or sound volume.
            if ( Audio::isValid() ) {
                bool saveMusicVolume = false;
                bool saveSoundVolume = false;
                if ( le.MouseClickLeft( windowMusicVolumeRoi ) ) {
                    conf.SetMusicVolume( ( conf.MusicVolume() + 1 ) % 11 );
                    saveMusicVolume = true;
                }
                else if ( le.MouseWheelUp( windowMusicVolumeRoi ) ) {
                    conf.SetMusicVolume( conf.MusicVolume() + 1 );
                    saveMusicVolume = true;
                }
                else if ( le.MouseWheelDn( windowMusicVolumeRoi ) ) {
                    conf.SetMusicVolume( conf.MusicVolume() - 1 );
                    saveMusicVolume = true;
                }
                if ( saveMusicVolume ) {
                    Music::Volume( static_cast<int16_t>( Mixer::MaxVolume() * conf.MusicVolume() / 10 ) );
                    return SelectedWindow::UpdateSettings;
                }

                if ( le.MouseClickLeft( windowSoundVolumeRoi ) ) {
                    conf.SetSoundVolume( ( conf.SoundVolume() + 1 ) % 11 );
                    saveSoundVolume = true;
                }
                else if ( le.MouseWheelUp( windowSoundVolumeRoi ) ) {
                    conf.SetSoundVolume( conf.SoundVolume() + 1 );
                    saveSoundVolume = true;
                }
                else if ( le.MouseWheelDn( windowSoundVolumeRoi ) ) {
                    conf.SetSoundVolume( conf.SoundVolume() - 1 );
                    saveSoundVolume = true;
                }
                if ( saveSoundVolume ) {
                    Game::EnvironmentSoundMixer();
                    return SelectedWindow::UpdateSettings;
                }
            }

            if ( le.MousePressRight( windowLanguageRoi ) ) {
                fheroes2::Text header( _( "Select Game Language" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Change language of the game." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowResolutionRoi ) ) {
                fheroes2::Text header( _( "Select Game Resolution" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Change resolution of the game." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowOptionsRoi ) ) {
                fheroes2::Text header( _( "Settings" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Experimental game settings." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowMusicVolumeRoi ) ) {
                fheroes2::Text header( _( "Music" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Toggle ambient music level." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowSoundVolumeRoi ) ) {
                fheroes2::Text header( _( "Effects" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Toggle foreground sounds level." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowMusicTypeRoi ) ) {
                fheroes2::Text header( _( "Music Type" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Change the type of music." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( okayButton.area() ) ) {
                fheroes2::Text header( _( "Okay" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Exit this menu." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
        }

        return SelectedWindow::Exit;
    }
}

namespace fheroes2
{
    void openGameSettings()
    {
        drawMainMenuScreen();

        SelectedWindow windowType = SelectedWindow::Configuration;
        while ( windowType != SelectedWindow::Exit ) {
            switch ( windowType ) {
            case SelectedWindow::Configuration:
                windowType = showConfigurationWindow();
                break;
            case SelectedWindow::Resolution:
                if ( Dialog::SelectResolution() ) {
                    Settings::Get().Save( Settings::configFileName );
                    // force interface to reset area and positions
                    Interface::Basic::Get().Reset();
                }
                drawMainMenuScreen();
                windowType = SelectedWindow::Configuration;
                break;
            case SelectedWindow::Language: {
                Settings & conf = Settings::Get();

                const std::vector<SupportedLanguage> supportedLanguages = getSupportedLanguages();

                if ( supportedLanguages.size() > 1 ) {
                    selectLanguage( supportedLanguages, getLanguageFromAbbreviation( conf.getGameLanguage() ) );
                }
                else {
                    assert( supportedLanguages.front() == SupportedLanguage::English );

                    conf.setGameLanguage( getLanguageAbbreviation( SupportedLanguage::English ) );

                    Text header( _( "Attention" ), FontType::normalYellow() );
                    Text body( _( "Your version of Heroes of Might and Magic II does not support any languages except English." ), FontType::normalWhite() );

                    showMessage( header, body, Dialog::OK );
                }

                Settings::Get().Save( Settings::configFileName );

                windowType = SelectedWindow::Configuration;
                break;
            }
            case SelectedWindow::Options:
                Dialog::ExtSettings( false );
                windowType = SelectedWindow::Configuration;
                break;
            case SelectedWindow::UpdateSettings:
                Settings::Get().Save( Settings::configFileName );
                windowType = SelectedWindow::Configuration;
                break;
            default:
                return;
            }
        }
    }
}
