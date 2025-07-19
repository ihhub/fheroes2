/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024 - 2025                                             *
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
#include "interface_base.h"
#include "localevent.h"
#include "math_base.h"
#include "render_processor.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "ui_option_item.h"
#include "ui_window.h"

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
        InterfaceType,
        CursorType,
        UpdateScrollSpeed,
        IncreaseScrollSpeed,
        DecreaseScrollSpeed,
        Close
    };

    const fheroes2::Rect languageRoi{ fheroes2::threeOptionsOffsetX, fheroes2::optionsOffsetY, fheroes2::optionIconSize, fheroes2::optionIconSize };
    const fheroes2::Rect graphicsRoi{ fheroes2::threeOptionsOffsetX + fheroes2::threeOptionsStepX, fheroes2::optionsOffsetY, fheroes2::optionIconSize,
                                      fheroes2::optionIconSize };
    const fheroes2::Rect audioRoi{ fheroes2::threeOptionsOffsetX + fheroes2::threeOptionsStepX * 2, fheroes2::optionsOffsetY, fheroes2::optionIconSize,
                                   fheroes2::optionIconSize };

    const fheroes2::Rect hotKeyRoi{ fheroes2::threeOptionsOffsetX, fheroes2::optionsOffsetY + fheroes2::optionsStepY, fheroes2::optionIconSize,
                                    fheroes2::optionIconSize };
    const fheroes2::Rect animationRoi{ fheroes2::threeOptionsOffsetX + fheroes2::threeOptionsStepX, fheroes2::optionsOffsetY + fheroes2::optionsStepY,
                                       fheroes2::optionIconSize, fheroes2::optionIconSize };
    const fheroes2::Rect passabilityRoi{ fheroes2::threeOptionsOffsetX + fheroes2::threeOptionsStepX * 2, fheroes2::optionsOffsetY + fheroes2::optionsStepY,
                                         fheroes2::optionIconSize, fheroes2::optionIconSize };

    const fheroes2::Rect interfaceTypeRoi{ fheroes2::threeOptionsOffsetX, fheroes2::optionsOffsetY + fheroes2::optionsStepY * 2, fheroes2::optionIconSize,
                                           fheroes2::optionIconSize };
    const fheroes2::Rect cursorTypeRoi{ fheroes2::threeOptionsOffsetX + fheroes2::threeOptionsStepX, fheroes2::optionsOffsetY + fheroes2::optionsStepY * 2,
                                        fheroes2::optionIconSize, fheroes2::optionIconSize };
    const fheroes2::Rect scrollSpeedRoi{ fheroes2::threeOptionsOffsetX + fheroes2::threeOptionsStepX * 2, fheroes2::optionsOffsetY + fheroes2::optionsStepY * 2,
                                         fheroes2::optionIconSize, fheroes2::optionIconSize };

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

        fheroes2::StandardWindow background( 289, fheroes2::optionsStepY * 3 + 52, true, display );

        const fheroes2::Rect windowRoi = background.activeArea();

        const fheroes2::Rect windowLanguageRoi( languageRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowGraphicsRoi( graphicsRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowAudioRoi( audioRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowHotKeyRoi( hotKeyRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowAnimationRoi( animationRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowPassabilityRoi( passabilityRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowInterfaceTypeRoi( interfaceTypeRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowCursorTypeRoi( cursorTypeRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowScrollSpeedRoi( scrollSpeedRoi + windowRoi.getPosition() );

        const auto drawOptions = [&windowLanguageRoi, &windowGraphicsRoi, &windowAudioRoi, &windowHotKeyRoi, &windowAnimationRoi, &windowPassabilityRoi,
                                  &windowInterfaceTypeRoi, &windowCursorTypeRoi, &windowScrollSpeedRoi]() {
            const Settings & conf = Settings::Get();

            drawLanguage( windowLanguageRoi );
            drawGraphics( windowGraphicsRoi );
            drawAudioOptions( windowAudioRoi );
            drawHotKeyOptions( windowHotKeyRoi );
            drawAnimationOptions( windowAnimationRoi );
            drawPassabilityOptions( windowPassabilityRoi );
            drawInterfaceType( windowInterfaceTypeRoi, conf.getInterfaceType(), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
            drawCursorType( windowCursorTypeRoi, conf.isMonochromeCursorEnabled(), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
            drawScrollSpeed( windowScrollSpeedRoi, conf.ScrollSpeed() );
        };

        drawOptions();

        const Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        fheroes2::Button buttonOk;
        const int buttonOkIcnId = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        background.renderButton( buttonOk, buttonOkIcnId, 0, 1, { 0, 11 }, fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

        // Render the whole screen as interface type or resolution could have been changed.
        display.render();

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonOk.area() ) );

            if ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyCloseWindow() ) {
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
            if ( le.MouseClickLeft( windowInterfaceTypeRoi ) ) {
                return DialogAction::InterfaceType;
            }
            if ( le.MouseClickLeft( windowCursorTypeRoi ) ) {
                return DialogAction::CursorType;
            }
            if ( le.MouseClickLeft( windowScrollSpeedRoi ) ) {
                return DialogAction::UpdateScrollSpeed;
            }
            if ( le.isMouseWheelUpInArea( windowScrollSpeedRoi ) ) {
                return DialogAction::IncreaseScrollSpeed;
            }
            if ( le.isMouseWheelDownInArea( windowScrollSpeedRoi ) ) {
                return DialogAction::DecreaseScrollSpeed;
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
            else if ( le.isMouseRightButtonPressedInArea( windowInterfaceTypeRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Interface Type" ), _( "Toggle the type of interface you want to use." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( windowCursorTypeRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Mouse Cursor" ), _( "Toggle colored cursor on or off. This is only an aesthetic choice." ), 0 );
            }
            if ( le.isMouseRightButtonPressedInArea( windowScrollSpeedRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Scroll Speed" ), _( "Sets the speed at which you scroll the window." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
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

        auto redrawEditor = [&conf]() {
            Interface::EditorInterface & editorInterface = Interface::EditorInterface::Get();

            // Since the radar interface has a restorer we must redraw it first to avoid the restorer doing something nasty.
            editorInterface.redraw( Interface::REDRAW_RADAR );

            uint32_t redrawOptions = Interface::REDRAW_ALL;
            if ( conf.isEditorPassabilityEnabled() ) {
                redrawOptions |= Interface::REDRAW_PASSABILITIES;
            }

            editorInterface.redraw( redrawOptions & ( ~Interface::REDRAW_RADAR ) );
        };

        auto rebuildEditor = [&redrawEditor]() {
            Interface::EditorInterface::Get().reset();

            redrawEditor();
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

                redrawEditor();
                saveConfiguration = true;
                action = DialogAction::Configuration;
                break;
            }
            case DialogAction::Graphics:
                saveConfiguration |= fheroes2::openGraphicsSettingsDialog( rebuildEditor );

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

                redrawEditor();

                action = DialogAction::Configuration;
                break;
            case DialogAction::InterfaceType:
                if ( conf.getInterfaceType() == InterfaceType::DYNAMIC ) {
                    conf.setInterfaceType( InterfaceType::GOOD );
                }
                else if ( conf.getInterfaceType() == InterfaceType::GOOD ) {
                    conf.setInterfaceType( InterfaceType::EVIL );
                }
                else {
                    conf.setInterfaceType( InterfaceType::DYNAMIC );
                }
                rebuildEditor();
                saveConfiguration = true;

                action = DialogAction::Configuration;
                break;
            case DialogAction::CursorType:
                conf.setMonochromeCursor( !conf.isMonochromeCursorEnabled() );
                saveConfiguration = true;

                action = DialogAction::Configuration;
                break;
            case DialogAction::UpdateScrollSpeed:
                conf.SetScrollSpeed( ( conf.ScrollSpeed() + 1 ) % ( SCROLL_SPEED_VERY_FAST + 1 ) );
                saveConfiguration = true;

                action = DialogAction::Configuration;
                break;
            case DialogAction::IncreaseScrollSpeed:
                conf.SetScrollSpeed( conf.ScrollSpeed() + 1 );
                saveConfiguration = true;

                action = DialogAction::Configuration;
                break;
            case DialogAction::DecreaseScrollSpeed:
                conf.SetScrollSpeed( conf.ScrollSpeed() - 1 );
                saveConfiguration = true;

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
