/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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

#include "editor_options.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_audio.h"
#include "dialog_graphics_settings.h"
#include "dialog_hotkeys.h"
#include "dialog_language_selection.h"
#include "editor_interface.h"
#include "game_hotkeys.h"
#include "game_language.h"
#include "icn.h"
#include "image.h"
#include "interface_base.h"
#include "localevent.h"
#include "math_base.h"
#include "render_processor.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "ui_option_item.h"

namespace
{
    enum class DialogAction : uint8_t
    {
        Configuration,
        Language,
        Graphics,
        AudioSettings,
        HotKeys,
        Animation,
        Passabiility,
        UpdateSettings,
        Close
    };

    const fheroes2::Size offsetBetweenOptions{ 92, 110 };
    const fheroes2::Point optionOffset{ 36, 47 };
    const int32_t optionWindowSize{ 65 };

    const fheroes2::Rect languageRoi{ optionOffset.x, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect graphicsRoi{ optionOffset.x + offsetBetweenOptions.width, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect audioRoi{ optionOffset.x + offsetBetweenOptions.width * 2, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect hotKeyRoi{ optionOffset.x, optionOffset.y + offsetBetweenOptions.height, optionWindowSize, optionWindowSize };
    const fheroes2::Rect animationRoi{ optionOffset.x + offsetBetweenOptions.width, optionOffset.y + offsetBetweenOptions.height, optionWindowSize, optionWindowSize };
    const fheroes2::Rect passabilityRoi{ optionOffset.x + offsetBetweenOptions.width * 2, optionOffset.y + offsetBetweenOptions.height, optionWindowSize,
                                         optionWindowSize };

    void drawLanguage( const fheroes2::Rect & optionRoi )
    {
        const fheroes2::SupportedLanguage currentLanguage = fheroes2::getLanguageFromAbbreviation( Settings::Get().getGameLanguage() );
        const fheroes2::LanguageSwitcher languageSwitcher( currentLanguage );

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

    void drawHotKeyOptions( const fheroes2::Rect & optionRoi )
    {
        fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::GAME_OPTION_ICON, 0 ), _( "Hot Keys" ), _( "Configure" ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void drawAnimationOptions( const fheroes2::Rect & optionRoi )
    {
        if ( Settings::Get().isEditorAnimationEnabled() ) {
            fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::ESPANEL, 1 ), _( "Animation" ), _( "On" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
        }
        else {
            fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::ESPANEL, 0 ), _( "Animation" ), _( "Off" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
        }
    }

    void drawPassabilityOptions( const fheroes2::Rect & optionRoi )
    {
        if ( Settings::Get().isEditorPassabilityEnabled() ) {
            fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::ESPANEL, 5 ), _( "Passability" ), _( "Show" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
        }
        else {
            fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::ESPANEL, 4 ), _( "Passability" ), _( "Hide" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
        }
    }

    DialogAction openEditorOptionsDialog()
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();
        const int dialogIcnId = isEvilInterface ? ICN::CSPANBKE : ICN::CSPANBKG;
        const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( dialogIcnId, 0 );
        const fheroes2::Sprite & dialogShadow = fheroes2::AGG::GetICN( dialogIcnId, 1 );

        const fheroes2::Point dialogOffset( ( display.width() - dialog.width() ) / 2, ( display.height() - dialog.height() ) / 2 );
        const fheroes2::Point shadowOffset( dialogOffset.x - fheroes2::borderWidthPx, dialogOffset.y );

        const fheroes2::Rect windowRoi{ dialogOffset.x, dialogOffset.y, dialog.width(), dialog.height() };

        const fheroes2::ImageRestorer restorer( display, shadowOffset.x, shadowOffset.y, dialog.width() + fheroes2::borderWidthPx,
                                                dialog.height() + fheroes2::borderWidthPx );

        fheroes2::Blit( dialogShadow, display, windowRoi.x - fheroes2::borderWidthPx, windowRoi.y + fheroes2::borderWidthPx );
        fheroes2::Blit( dialog, display, windowRoi.x, windowRoi.y );

        const fheroes2::ImageRestorer emptyDialogRestorer( display, windowRoi.x, windowRoi.y, windowRoi.width, windowRoi.height );

        const int buttonIcnId = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;

        const fheroes2::Rect windowLanguageRoi( languageRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowGraphicsRoi( graphicsRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowAudioRoi( audioRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowHotKeyRoi( hotKeyRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowAnimationRoi( animationRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowPassabilityRoi( passabilityRoi + windowRoi.getPosition() );

        const auto drawOptions = [&windowLanguageRoi, &windowGraphicsRoi, &windowAudioRoi, &windowHotKeyRoi, &windowAnimationRoi, &windowPassabilityRoi]() {
            drawLanguage( windowLanguageRoi );
            drawGraphics( windowGraphicsRoi );
            drawAudioOptions( windowAudioRoi );
            drawHotKeyOptions( windowHotKeyRoi );
            drawAnimationOptions( windowAnimationRoi );
            drawPassabilityOptions( windowPassabilityRoi );
        };

        drawOptions();

        fheroes2::ButtonSprite okayButton( windowRoi.x + 112, windowRoi.y + 252, fheroes2::AGG::GetICN( buttonIcnId, 0 ), fheroes2::AGG::GetICN( buttonIcnId, 1 ) );
        okayButton.draw();

        display.render();

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            if ( le.isMouseLeftButtonPressedInArea( okayButton.area() ) ) {
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
            if ( le.MouseClickLeft( windowHotKeyRoi ) ) {
                return DialogAction::HotKeys;
            }
            if ( le.MouseClickLeft( windowAnimationRoi ) ) {
                return DialogAction::Animation;
            }
            if ( le.MouseClickLeft( windowPassabilityRoi ) ) {
                return DialogAction::Passabiility;
            }

            if ( le.isMouseRightButtonPressedInArea( windowLanguageRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Select Game Language" ), _( "Change the language of the game." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( windowGraphicsRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Graphics" ), _( "Change the graphics settings of the game." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( windowAudioRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Audio" ), _( "Change the audio settings of the game." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( windowHotKeyRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Hot Keys" ), _( "Check and configure all the hot keys present in the game." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( windowAnimationRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Animation" ), _( "Toggle animation of the objects." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( windowPassabilityRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Passability" ), _( "Toggle display of objects' passability." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( okayButton.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Exit this menu." ), 0 );
            }
        }

        return DialogAction::Close;
    }
}

namespace Editor
{
    void openEditorSettings()
    {
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        // We should write to the configuration file only once to avoid extra I/O operations.
        bool saveConfiguration = false;
        Settings & conf = Settings::Get();

        auto redrawEditorMap = [&conf]() {
            Interface::EditorInterface & editorInterface = Interface::EditorInterface::Get();

            editorInterface.reset();
            // Since the radar interface has a restorer we must redraw it first to avoid the restorer doing something nasty.
            editorInterface.redraw( Interface::REDRAW_RADAR );

            uint32_t redrawOptions = Interface::REDRAW_ALL;
            if ( conf.isEditorPassabilityEnabled() ) {
                redrawOptions |= Interface::REDRAW_PASSABILITIES;
            }

            editorInterface.redraw( redrawOptions & ( ~Interface::REDRAW_RADAR ) );
        };

        DialogAction action = DialogAction::Configuration;

        while ( action != DialogAction::Close ) {
            switch ( action ) {
            case DialogAction::Configuration:
                action = openEditorOptionsDialog();
                break;
            case DialogAction::Language: {
                const std::vector<fheroes2::SupportedLanguage> supportedLanguages = fheroes2::getSupportedLanguages();

                if ( supportedLanguages.size() > 1 ) {
                    selectLanguage( supportedLanguages, fheroes2::getLanguageFromAbbreviation( conf.getGameLanguage() ), true );
                }
                else {
                    assert( supportedLanguages.front() == fheroes2::SupportedLanguage::English );

                    conf.setGameLanguage( fheroes2::getLanguageAbbreviation( fheroes2::SupportedLanguage::English ) );

                    fheroes2::showStandardTextMessage( "Attention", "Your version of Heroes of Might and Magic II does not support any other languages than English.",
                                                       Dialog::OK );
                }

                redrawEditorMap();
                saveConfiguration = true;
                action = DialogAction::Configuration;
                break;
            }
            case DialogAction::Graphics:
                saveConfiguration |= fheroes2::openGraphicsSettingsDialog( redrawEditorMap );

                action = DialogAction::Configuration;
                break;
            case DialogAction::AudioSettings:
                saveConfiguration |= Dialog::openAudioSettingsDialog( false );

                action = DialogAction::Configuration;
                break;
            case DialogAction::HotKeys:
                fheroes2::openHotkeysDialog();

                action = DialogAction::Configuration;
                break;
            case DialogAction::Animation:
                conf.setEditorAnimation( !conf.isEditorAnimationEnabled() );
                saveConfiguration = true;

                if ( conf.isEditorAnimationEnabled() ) {
                    fheroes2::RenderProcessor::instance().startColorCycling();
                }
                else {
                    fheroes2::RenderProcessor::instance().stopColorCycling();
                }

                action = DialogAction::Configuration;
                break;
            case DialogAction::Passabiility:
                conf.setEditorPassability( !conf.isEditorPassabilityEnabled() );
                saveConfiguration = true;

                redrawEditorMap();

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
