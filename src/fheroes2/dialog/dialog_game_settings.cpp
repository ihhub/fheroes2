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

#include "dialog_game_settings.h"
#include "agg_image.h"
#include "audio.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_audio.h"
#include "dialog_hotkeys.h"
#include "dialog_language_selection.h"
#include "dialog_resolution.h"
#include "game.h"
#include "game_hotkeys.h"
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

    const fheroes2::Point textOffset{ 11, 12 };
    const int32_t nameOffset = 6;
    const fheroes2::Point optionOffset{ 36, 47 };
    const int32_t optionWindowSize{ 65 };

    const fheroes2::Rect languageRoi{ optionOffset.x, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect resolutionRoi{ optionOffset.x + offsetBetweenOptions.width, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect audioRoi{ optionOffset.x + offsetBetweenOptions.width * 2, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect optionsRoi{ optionOffset.x, optionOffset.y + offsetBetweenOptions.height, optionWindowSize, optionWindowSize };
    const fheroes2::Rect battleResolveRoi{ optionOffset.x + offsetBetweenOptions.width, optionOffset.y + offsetBetweenOptions.height, optionWindowSize,
                                           optionWindowSize };
    const fheroes2::Rect hotKeyRoi{ optionOffset.x + offsetBetweenOptions.width * 2, optionOffset.y + offsetBetweenOptions.height, optionWindowSize, optionWindowSize };
    const fheroes2::Rect interfaceTypeRoi{ optionOffset.x, optionOffset.y + 2 * offsetBetweenOptions.height, optionWindowSize, optionWindowSize };
    const fheroes2::Rect cursorTypeRoi{ optionOffset.x + offsetBetweenOptions.width, optionOffset.y + 2 * offsetBetweenOptions.height, optionWindowSize,
                                        optionWindowSize };
    const fheroes2::Rect textSupportModeRoi{ optionOffset.x + offsetBetweenOptions.width * 2, optionOffset.y + 2 * offsetBetweenOptions.height, optionWindowSize,
                                             optionWindowSize };

    void drawOption( const fheroes2::Rect & optionRoi, const char * titleText, const char * nameText, const int icnId, const uint32_t icnIndex )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::Text title( titleText, fheroes2::FontType::smallWhite() );
        const fheroes2::Text name( nameText, fheroes2::FontType::smallWhite() );

        const int32_t textMaxWidth = 87;

        title.draw( optionRoi.x - textOffset.x, optionRoi.y - textOffset.y + title.height() - title.height( textMaxWidth ), textMaxWidth, display );
        name.draw( optionRoi.x - textOffset.x, optionRoi.y + languageRoi.height + nameOffset, textMaxWidth, display );

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

        const Settings & configuration = Settings::Get();

        drawOption( optionRoi, _( "Resolution" ), resolutionName.c_str(), ICN::SPANEL, configuration.ExtGameEvilInterface() ? 17 : 16 );
    }

    void drawExperimentalOptions( const fheroes2::Rect & optionRoi )
    {
        drawOption( optionRoi, _( "Settings" ), _( "Experimental" ), ICN::SPANEL, 14 );
    }

    void drawAudioOptions( const fheroes2::Rect & optionRoi )
    {
        drawOption( optionRoi, _( "Audio" ), _( "Settings" ), ICN::SPANEL, 1 );
    }

    void drawHotKeyOptions( const fheroes2::Rect & optionRoi )
    {
        drawOption( optionRoi, _( "Hot Keys" ), _( "Configure" ), ICN::CSPANEL, 5 );
    }

    void drawCursorTypeOptions( const fheroes2::Rect & optionRoi )
    {
        if ( Settings::Get().isMonochromeCursorEnabled() ) {
            drawOption( optionRoi, _( "Mouse Cursor" ), _( "Black & White" ), ICN::SPANEL, 20 );
        }
        else {
            drawOption( optionRoi, _( "Mouse Cursor" ), _( "Color" ), ICN::SPANEL, 21 );
        }
    }

    void drawTextSupportModeOptions( const fheroes2::Rect & optionRoi )
    {
        if ( Settings::Get().isTextSupportModeEnabled() ) {
            drawOption( optionRoi, _( "Text Support" ), _( "On" ), ICN::CSPANEL, 4 );
        }
        else {
            drawOption( optionRoi, _( "Text Support" ), _( "Off" ), ICN::SPANEL, 9 );
        }
    }

    void drawInterfaceOptions( const fheroes2::Rect & optionRoi )
    {
        const bool isEvilInterface = Settings::Get().ExtGameEvilInterface();
        drawOption( optionRoi, _( "Interface Type" ), isEvilInterface ? _( "Evil" ) : _( "Good" ), ICN::SPANEL, isEvilInterface ? 17 : 16 );
    }

    void drawBattleResolveOptions( const fheroes2::Rect & optionRoi )
    {
        std::string value;
        int icnId = ICN::UNKNOWN;
        uint32_t icnIndex = 0;

        const Settings & conf = Settings::Get();
        if ( conf.BattleAutoResolve() ) {
            const bool spellcast = conf.BattleAutoSpellcast();
            value = spellcast ? _( "Auto Resolve" ) : _( "Auto, No Spells" );

            icnId = ICN::CSPANEL;
            icnIndex = spellcast ? 7 : 6;
        }
        else {
            value = _( "Manual" );

            icnId = ICN::SPANEL;
            icnIndex = 18;
        }

        drawOption( optionRoi, _( "Battles" ), value.c_str(), icnId, icnIndex );
    }

    enum class SelectedWindow : int
    {
        Configuration,
        Resolution,
        Language,
        Options,
        HotKeys,
        CursorType,
        TextSupportMode,
        UpdateSettings,
        AudioSettings,
        InterfaceTheme,
        BattleResolveType,
        Exit
    };

    SelectedWindow showConfigurationWindow()
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.ExtGameEvilInterface();
        const int dialogIcnId = isEvilInterface ? ICN::SPANBKGE : ICN::SPANBKG;
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
        const fheroes2::Rect windowAudioRoi( audioRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowOptionsRoi( optionsRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowBattleResolveRoi( battleResolveRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowHotKeyRoi( hotKeyRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowInterfaceTypeRoi( interfaceTypeRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowCursorTypeRoi( cursorTypeRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowTextSupportModeRoi( textSupportModeRoi + windowRoi.getPosition() );

        drawLanguage( windowLanguageRoi );
        drawResolution( windowResolutionRoi );
        drawAudioOptions( windowAudioRoi );
        drawExperimentalOptions( windowOptionsRoi );
        drawBattleResolveOptions( windowBattleResolveRoi );
        drawHotKeyOptions( windowHotKeyRoi );
        drawInterfaceOptions( windowInterfaceTypeRoi );
        drawCursorTypeOptions( windowCursorTypeRoi );
        drawTextSupportModeOptions( windowTextSupportModeRoi );

        fheroes2::ButtonSprite okayButton( windowRoi.x + 112, windowRoi.y + 362, buttonOkayReleased, buttonOkayPressed );
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

            if ( le.MouseClickLeft( okayButton.area() ) || Game::HotKeyCloseWindow() ) {
                break;
            }
            if ( le.MouseClickLeft( windowLanguageRoi ) ) {
                return SelectedWindow::Language;
            }
            if ( le.MouseClickLeft( windowResolutionRoi ) ) {
                return SelectedWindow::Resolution;
            }
            if ( le.MouseClickLeft( windowAudioRoi ) ) {
                return SelectedWindow::AudioSettings;
            }
            if ( le.MouseClickLeft( windowOptionsRoi ) ) {
                return SelectedWindow::Options;
            }
            if ( le.MouseClickLeft( windowBattleResolveRoi ) ) {
                return SelectedWindow::BattleResolveType;
            }
            if ( le.MouseClickLeft( windowHotKeyRoi ) ) {
                return SelectedWindow::HotKeys;
            }
            if ( le.MouseClickLeft( windowInterfaceTypeRoi ) ) {
                return SelectedWindow::InterfaceTheme;
            }
            if ( le.MouseClickLeft( windowCursorTypeRoi ) ) {
                return SelectedWindow::CursorType;
            }
            if ( le.MouseClickLeft( windowTextSupportModeRoi ) ) {
                return SelectedWindow::TextSupportMode;
            }

            if ( le.MousePressRight( windowLanguageRoi ) ) {
                fheroes2::Text header( _( "Select Game Language" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Change the language of the game." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowResolutionRoi ) ) {
                fheroes2::Text header( _( "Select Game Resolution" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Change the resolution of the game." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowAudioRoi ) ) {
                fheroes2::Text header( _( "Audio" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Change the audio settings of the game." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowOptionsRoi ) ) {
                fheroes2::Text header( _( "Settings" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Experimental game settings." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            if ( le.MousePressRight( windowBattleResolveRoi ) ) {
                fheroes2::Text header( _( "Battles" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Toggle instant battle mode." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowHotKeyRoi ) ) {
                fheroes2::Text header( _( "Hot Keys" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Check and configure all the hot keys present in the game." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            if ( le.MousePressRight( windowInterfaceTypeRoi ) ) {
                fheroes2::Text header( _( "Interface Type" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Toggle the type of interface you want to use." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowCursorTypeRoi ) ) {
                fheroes2::Text header( _( "Mouse Cursor" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Toggle colored cursor on or off. This is only an esthetic choice." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowTextSupportModeRoi ) ) {
                fheroes2::Text header( _( "Text Support" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Toggle text support mode to output extra information about windows and events in the game." ),
                                     fheroes2::FontType::normalWhite() );

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

        Settings & conf = Settings::Get();

        SelectedWindow windowType = SelectedWindow::Configuration;
        while ( windowType != SelectedWindow::Exit ) {
            switch ( windowType ) {
            case SelectedWindow::Configuration:
                windowType = showConfigurationWindow();
                break;
            case SelectedWindow::Resolution:
                if ( Dialog::SelectResolution() ) {
                    conf.Save( Settings::configFileName );
                }
                drawMainMenuScreen();
                windowType = SelectedWindow::Configuration;
                break;
            case SelectedWindow::Language: {
                const std::vector<SupportedLanguage> supportedLanguages = getSupportedLanguages();

                if ( supportedLanguages.size() > 1 ) {
                    selectLanguage( supportedLanguages, getLanguageFromAbbreviation( conf.getGameLanguage() ) );
                }
                else {
                    assert( supportedLanguages.front() == SupportedLanguage::English );

                    conf.setGameLanguage( getLanguageAbbreviation( SupportedLanguage::English ) );

                    Text header( _( "Attention" ), FontType::normalYellow() );
                    Text body( _( "Your version of Heroes of Might and Magic II does not support any other languages than English." ), FontType::normalWhite() );

                    showMessage( header, body, Dialog::OK );
                }

                windowType = SelectedWindow::UpdateSettings;
                break;
            }
            case SelectedWindow::Options:
                Dialog::ExtSettings( false );
                windowType = SelectedWindow::Configuration;
                break;
            case SelectedWindow::HotKeys:
                fheroes2::openHotkeysDialog();
                windowType = SelectedWindow::Configuration;
                break;
            case SelectedWindow::CursorType:
                conf.setMonochromeCursor( !conf.isMonochromeCursorEnabled() );
                windowType = SelectedWindow::UpdateSettings;
                break;
            case SelectedWindow::TextSupportMode:
                conf.setTextSupportMode( !conf.isTextSupportModeEnabled() );
                windowType = SelectedWindow::UpdateSettings;
                break;
            case SelectedWindow::UpdateSettings:
                conf.Save( Settings::configFileName );
                windowType = SelectedWindow::Configuration;
                break;
            case SelectedWindow::AudioSettings:
                Dialog::openAudioSettingsDialog( false );
                windowType = SelectedWindow::Configuration;
                break;
            case SelectedWindow::InterfaceTheme:
                conf.SetEvilInterface( !conf.ExtGameEvilInterface() );
                windowType = SelectedWindow::UpdateSettings;
                break;
            case SelectedWindow::BattleResolveType:
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

                windowType = SelectedWindow::UpdateSettings;
                break;
            default:
                return;
            }
        }
    }
}
