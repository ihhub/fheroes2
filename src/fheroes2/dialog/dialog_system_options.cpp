/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2023                                             *
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

#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_audio.h"
#include "dialog_graphics_settings.h"
#include "dialog_hotkeys.h"
#include "dialog_interface_settings.h"
#include "dialog_language_selection.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "gamedefs.h"
#include "icn.h"
#include "image.h"
#include "interface_base.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "ui_option_item.h"

namespace
{
    enum class DialogAction : int
    {
        Configuration,
        Language,
        Graphics,
        AudioSettings,
        HotKeys,
        InterfaceSettings,
        UpdateInterface,
        Close
    };

    const fheroes2::Size offsetBetweenOptions{ 92, 110 };
    const fheroes2::Point optionOffset{ 36, 47 };
    const int32_t optionWindowSize{ 65 };

    const fheroes2::Rect languageRoi{ optionOffset.x, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect graphicsRoi{ optionOffset.x + offsetBetweenOptions.width, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect audioRoi{ optionOffset.x + offsetBetweenOptions.width * 2, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect heroSpeedRoi{ optionOffset.x, optionOffset.y + offsetBetweenOptions.height, optionWindowSize, optionWindowSize };
    const fheroes2::Rect enemySpeedRoi{ optionOffset.x + offsetBetweenOptions.width, optionOffset.y + offsetBetweenOptions.height, optionWindowSize, optionWindowSize };
    const fheroes2::Rect hotKeyRoi{ optionOffset.x + offsetBetweenOptions.width * 2, optionOffset.y + offsetBetweenOptions.height, optionWindowSize, optionWindowSize };
    const fheroes2::Rect interfaceRoi{ optionOffset.x, optionOffset.y + offsetBetweenOptions.height * 2, optionWindowSize, optionWindowSize };
    const fheroes2::Rect textSupportModeRoi{ optionOffset.x + offsetBetweenOptions.width, optionOffset.y + offsetBetweenOptions.height * 2, optionWindowSize,
                                             optionWindowSize };
    const fheroes2::Rect battlesRoi{ optionOffset.x + offsetBetweenOptions.width * 2, optionOffset.y + offsetBetweenOptions.height * 2, optionWindowSize,
                                     optionWindowSize };

    void drawLanguage( const fheroes2::Rect & optionRoi )
    {
        const fheroes2::SupportedLanguage currentLanguage = fheroes2::getLanguageFromAbbreviation( Settings::Get().getGameLanguage() );
        fheroes2::LanguageSwitcher languageSwitcher( currentLanguage );

        fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::SPANEL, 18 ), _( "Language" ), fheroes2::getLanguageName( currentLanguage ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void drawGraphics( const fheroes2::Rect & optionRoi )
    {
        fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::GAME_OPTION_ICON, 1 ), _( "Graphics" ), _( "Settings" ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void drawAudioOptions( const fheroes2::Rect & optionRoi )
    {
        fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::SPANEL, 1 ), _( "Audio" ), _( "Settings" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void drawHeroSpeed( const fheroes2::Rect & optionRoi )
    {
        const Settings & conf = Settings::Get();
        const int heroSpeed = conf.HeroesMoveSpeed();
        uint32_t heroSpeedIconId = 9;
        if ( heroSpeed >= 4 ) {
            heroSpeedIconId = 3 + heroSpeed / 2;
        }
        else if ( heroSpeed > 0 ) {
            heroSpeedIconId = 4;
        }

        const fheroes2::Sprite & heroSpeedIcon = fheroes2::AGG::GetICN( ICN::SPANEL, heroSpeedIconId );
        std::string value;
        if ( heroSpeed == 10 ) {
            value = _( "Jump" );
        }
        else {
            value = std::to_string( heroSpeed );
        }

        fheroes2::drawOption( optionRoi, heroSpeedIcon, _( "Hero Speed" ), std::move( value ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void drawEnemySpeed( const fheroes2::Rect & optionRoi )
    {
        const Settings & conf = Settings::Get();
        const int aiSpeed = conf.AIMoveSpeed();
        uint32_t aiSpeedIconId = 9;
        if ( aiSpeed >= 4 ) {
            aiSpeedIconId = 3 + aiSpeed / 2;
        }
        else if ( aiSpeed > 0 ) {
            aiSpeedIconId = 4;
        }

        const fheroes2::Sprite & aiSpeedIcon = fheroes2::AGG::GetICN( ICN::SPANEL, aiSpeedIconId );

        std::string value;
        if ( aiSpeed == 0 ) {
            value = _( "Don't Show" );
        }
        else if ( aiSpeed == 10 ) {
            value = _( "Jump" );
        }
        else {
            value = std::to_string( aiSpeed );
        }

        fheroes2::drawOption( optionRoi, aiSpeedIcon, _( "Enemy Speed" ), std::move( value ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void drawHotKeyOptions( const fheroes2::Rect & optionRoi )
    {
        fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::GAME_OPTION_ICON, 0 ), _( "Hot Keys" ), _( "Configure" ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void drawInterfaceSettings( const fheroes2::Rect & optionRoi )
    {
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        const fheroes2::Sprite & interfaceThemeIcon = fheroes2::AGG::GetICN( ICN::SPANEL, isEvilInterface ? 17 : 16 );

        fheroes2::drawOption( optionRoi, interfaceThemeIcon, _( "Interface" ), _( "Settings" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void drawTextSupportModeOptions( const fheroes2::Rect & optionRoi )
    {
        if ( Settings::Get().isTextSupportModeEnabled() ) {
            fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::CSPANEL, 4 ), _( "Text Support" ), _( "On" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
        }
        else {
            fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::SPANEL, 9 ), _( "Text Support" ), _( "Off" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
        }
    }

    void drawBattles( const fheroes2::Rect & optionRoi )
    {
        const Settings & conf = Settings::Get();
        if ( conf.BattleAutoResolve() ) {
            const bool spellcast = conf.BattleAutoSpellcast();
            std::string value = spellcast ? _( "Auto Resolve" ) : _( "Auto, No Spells" );

            const fheroes2::Sprite & autoBattleIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, spellcast ? 7 : 6 );
            fheroes2::drawOption( optionRoi, autoBattleIcon, _( "Battles" ), std::move( value ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
        }
        else {
            const fheroes2::Sprite & autoBattleIcon = fheroes2::AGG::GetICN( ICN::SPANEL, 18 );
            fheroes2::drawOption( optionRoi, autoBattleIcon, _( "Battles" ), _( "autoBattle|Manual" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
        }
    }

    DialogAction openSystemOptionsDialog( bool & saveConfiguration )
    {
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::SPANBKGE : ICN::SPANBKG ), 0 );
        const fheroes2::Sprite & dialogShadow = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::SPANBKGE : ICN::SPANBKG ), 1 );

        const fheroes2::Point dialogOffset( ( display.width() - dialog.width() ) / 2, ( display.height() - dialog.height() ) / 2 );
        const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

        fheroes2::ImageRestorer restorer( display, shadowOffset.x, shadowOffset.y, dialog.width() + BORDERWIDTH, dialog.height() + BORDERWIDTH );
        const fheroes2::Rect windowRoi{ dialogOffset.x, dialogOffset.y, dialog.width(), dialog.height() };

        fheroes2::Blit( dialogShadow, display, windowRoi.x - BORDERWIDTH, windowRoi.y + BORDERWIDTH );
        fheroes2::Blit( dialog, display, windowRoi.x, windowRoi.y );

        fheroes2::ImageRestorer emptyDialogRestorer( display, windowRoi.x, windowRoi.y, windowRoi.width, windowRoi.height );

        const fheroes2::Rect windowLanguageRoi( languageRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowGraphicsRoi( graphicsRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowAudioRoi( audioRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowHeroSpeedRoi( heroSpeedRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowEnemySpeedRoi( enemySpeedRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowHotKeyRoi( hotKeyRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowInterfaceRoi( interfaceRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowTextSupportModeRoi( textSupportModeRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowBattlesRoi( battlesRoi + windowRoi.getPosition() );

        const auto drawOptions = [&windowLanguageRoi, &windowGraphicsRoi, &windowAudioRoi, &windowHeroSpeedRoi, &windowEnemySpeedRoi, &windowHotKeyRoi,
                                  &windowInterfaceRoi, &windowTextSupportModeRoi, &windowBattlesRoi]() {
            drawLanguage( windowLanguageRoi );
            drawGraphics( windowGraphicsRoi );
            drawAudioOptions( windowAudioRoi );
            drawHeroSpeed( windowHeroSpeedRoi );
            drawEnemySpeed( windowEnemySpeedRoi );
            drawHotKeyOptions( windowHotKeyRoi );
            drawInterfaceSettings( windowInterfaceRoi );
            drawTextSupportModeOptions( windowTextSupportModeRoi );
            drawBattles( windowBattlesRoi );
        };

        drawOptions();

        const auto refreshWindow = [&drawOptions, &emptyDialogRestorer, &display]() {
            emptyDialogRestorer.restore();
            drawOptions();
            display.render( emptyDialogRestorer.rect() );
        };

        const fheroes2::Point buttonOffset( 112 + windowRoi.x, 362 + windowRoi.y );
        fheroes2::Button okayButton( buttonOffset.x, buttonOffset.y, isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD, 0, 1 );
        okayButton.draw();

        display.render();

        bool isTextSupportModeEnabled = conf.isTextSupportModeEnabled();

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
                return DialogAction::Language;
            }
            if ( le.MouseClickLeft( windowGraphicsRoi ) ) {
                return DialogAction::Graphics;
            }
            if ( le.MouseClickLeft( windowAudioRoi ) ) {
                return DialogAction::AudioSettings;
            }

            if ( le.MouseClickLeft( windowHeroSpeedRoi ) ) {
                saveConfiguration = true;
                conf.SetHeroesMoveSpeed( conf.HeroesMoveSpeed() % 10 + 1 );
                Game::UpdateGameSpeed();
                refreshWindow();

                continue;
            }
            if ( le.MouseWheelUp( windowHeroSpeedRoi ) ) {
                saveConfiguration = true;
                conf.SetHeroesMoveSpeed( conf.HeroesMoveSpeed() + 1 );
                Game::UpdateGameSpeed();
                refreshWindow();

                continue;
            }
            if ( le.MouseWheelDn( windowHeroSpeedRoi ) ) {
                saveConfiguration = true;
                conf.SetHeroesMoveSpeed( conf.HeroesMoveSpeed() - 1 );
                Game::UpdateGameSpeed();
                refreshWindow();

                continue;
            }

            if ( le.MouseClickLeft( windowEnemySpeedRoi ) ) {
                saveConfiguration = true;
                conf.SetAIMoveSpeed( ( conf.AIMoveSpeed() + 1 ) % 11 );
                Game::UpdateGameSpeed();
                refreshWindow();

                continue;
            }
            if ( le.MouseWheelUp( windowEnemySpeedRoi ) ) {
                saveConfiguration = true;
                conf.SetAIMoveSpeed( conf.AIMoveSpeed() + 1 );
                Game::UpdateGameSpeed();
                refreshWindow();

                continue;
            }
            if ( le.MouseWheelDn( windowEnemySpeedRoi ) ) {
                saveConfiguration = true;
                conf.SetAIMoveSpeed( conf.AIMoveSpeed() - 1 );
                Game::UpdateGameSpeed();
                refreshWindow();

                continue;
            }

            if ( le.MouseClickLeft( windowHotKeyRoi ) ) {
                return DialogAction::HotKeys;
            }
            if ( le.MouseClickLeft( windowInterfaceRoi ) ) {
                return DialogAction::InterfaceSettings;
            }
            if ( le.MouseClickLeft( windowTextSupportModeRoi ) ) {
                saveConfiguration = true;
                conf.setTextSupportMode( !conf.isTextSupportModeEnabled() );
                refreshWindow();

                continue;
            }
            if ( le.MouseClickLeft( windowBattlesRoi ) ) {
                saveConfiguration = true;
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

                refreshWindow();

                continue;
            }

            if ( le.MousePressRight( windowLanguageRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Select Game Language" ), _( "Change the language of the game." ), 0 );
            }
            else if ( le.MousePressRight( windowGraphicsRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Graphics" ), _( "Change the graphics settings of the game." ), 0 );
            }
            else if ( le.MousePressRight( windowAudioRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Audio" ), _( "Change the audio settings of the game." ), 0 );
            }
            else if ( le.MousePressRight( windowHeroSpeedRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Hero Speed" ), _( "Change the speed at which your heroes move on the main screen." ), 0 );
            }
            else if ( le.MousePressRight( windowEnemySpeedRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Enemy Speed" ),
                                                   _( "Sets the speed that A.I. heroes move at. You can also elect not to view A.I. movement at all." ), 0 );
            }
            else if ( le.MousePressRight( windowHotKeyRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Hot Keys" ), _( "Check and configure all the hot keys present in the game." ), 0 );
            }
            else if ( le.MousePressRight( windowInterfaceRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Interface Settings" ), _( "Change the interface settings of the game." ), 0 );
            }
            else if ( le.MousePressRight( windowTextSupportModeRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Text Support" ), _( "Toggle text support mode to output extra information about windows and events in the game." ),
                                                   0 );
            }
            else if ( le.MousePressRight( windowBattlesRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Battles" ), _( "Toggle instant battle mode." ), 0 );
            }
            else if ( le.MousePressRight( okayButton.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Exit this menu." ), 0 );
            }

            // Text support mode can be toggled using a global hotkey, we need to properly reflect this change in the UI
            if ( isTextSupportModeEnabled != conf.isTextSupportModeEnabled() ) {
                isTextSupportModeEnabled = conf.isTextSupportModeEnabled();

                emptyDialogRestorer.restore();
                drawOptions();

                display.render( emptyDialogRestorer.rect() );
            }
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
        Settings & conf = Settings::Get();

        auto redrawAdventureMap = []() {
            Interface::AdventureMap & adventureMap = Interface::AdventureMap::Get();

            adventureMap.reset();
            // Since radar interface has a restorer we must redraw it first to avoid the restorer do some nasty work.
            adventureMap.redraw( Interface::REDRAW_RADAR );

            adventureMap.redraw( Interface::REDRAW_ALL & ( ~Interface::REDRAW_RADAR ) );
        };

        DialogAction action = DialogAction::Configuration;

        while ( action != DialogAction::Close ) {
            switch ( action ) {
            case DialogAction::Configuration:
                action = openSystemOptionsDialog( saveConfiguration );
                break;
            case DialogAction::Language: {
                const std::vector<SupportedLanguage> supportedLanguages = getSupportedLanguages();

                if ( supportedLanguages.size() > 1 ) {
                    selectLanguage( supportedLanguages, getLanguageFromAbbreviation( conf.getGameLanguage() ) );
                }
                else {
                    assert( supportedLanguages.front() == SupportedLanguage::English );

                    conf.setGameLanguage( getLanguageAbbreviation( SupportedLanguage::English ) );

                    fheroes2::showStandardTextMessage( "Attention", "Your version of Heroes of Might and Magic II does not support any other languages than English.",
                                                       Dialog::OK );
                }

                redrawAdventureMap();
                saveConfiguration = true;
                action = DialogAction::Configuration;
                break;
            }
            case DialogAction::Graphics:
                fheroes2::openGraphicsSettingsDialog( redrawAdventureMap );

                action = DialogAction::Configuration;
                break;
            case DialogAction::AudioSettings:
                Dialog::openAudioSettingsDialog( true );

                action = DialogAction::Configuration;
                break;
            case DialogAction::HotKeys:
                fheroes2::openHotkeysDialog();

                action = DialogAction::Configuration;
                break;
            case DialogAction::InterfaceSettings:
                fheroes2::openInterfaceSettingsDialog( redrawAdventureMap );

                action = DialogAction::Configuration;
                break;
            default:
                break;
            }
        }

        if ( saveConfiguration ) {
            conf.Save( Settings::configFileName );
        }
    }
}
