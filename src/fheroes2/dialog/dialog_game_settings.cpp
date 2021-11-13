/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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
    const int32_t offsetBetweenOptions = 92;
    const int32_t titleOffset = 20;
    const int32_t nameOffset = 10;

    const fheroes2::Rect languageRoi( 20, 31, 65, 65 );
    const fheroes2::Rect resolutionRoi( 20 + offsetBetweenOptions, 31, 65, 65 );
    const fheroes2::Rect optionsRoi( 20 + offsetBetweenOptions * 2, 31, 65, 65 );

    void drawBackground( const fheroes2::StandardWindow & window )
    {
        const bool isEvilInterface = Settings::Get().ExtGameEvilInterface();
        const fheroes2::Sprite & settingsImage = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::SPANBKGE : ICN::SPANBKG ), 0 );
        const fheroes2::Rect & windowRoi = window.activeArea();

        Copy( settingsImage, 16, 16, fheroes2::Display::instance(), windowRoi.x, windowRoi.y, 289, 120 );
    }

    void drawLanguage( const fheroes2::StandardWindow & window )
    {
        const fheroes2::Text title( _( "Language" ), { fheroes2::FontSize::SMALL, fheroes2::FontColor::WHITE } );

        const fheroes2::SupportedLanguage currentLanguage = fheroes2::getLanguageFromAbbreviation( Settings::Get().getGameLanguage() );
        const fheroes2::Text name( fheroes2::getLanguageName( currentLanguage ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } );

        const fheroes2::Rect & windowRoi = window.activeArea();
        fheroes2::Display & display = fheroes2::Display::instance();

        title.draw( languageRoi.x + windowRoi.x + ( languageRoi.width - title.width() ) / 2, languageRoi.y - titleOffset + windowRoi.y, display );
        name.draw( languageRoi.x + windowRoi.x + ( languageRoi.width - name.width() ) / 2, languageRoi.y + languageRoi.height + nameOffset + windowRoi.y, display );

        const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::SPANEL, 18 );
        fheroes2::Blit( icon, 0, 0, display, languageRoi.x + windowRoi.x, languageRoi.y + windowRoi.y, icon.width(), icon.height() );
    }

    void drawResolution( const fheroes2::StandardWindow & window )
    {
        const fheroes2::Text title( _( "Resolution" ), { fheroes2::FontSize::SMALL, fheroes2::FontColor::WHITE } );

        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Text name( std::to_string( display.width() ) + 'x' + std::to_string( display.height() ),
                                   { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } );

        const fheroes2::Rect & windowRoi = window.activeArea();

        title.draw( resolutionRoi.x + windowRoi.x + ( resolutionRoi.width - title.width() ) / 2, resolutionRoi.y - titleOffset + windowRoi.y, display );
        name.draw( resolutionRoi.x + windowRoi.x + ( resolutionRoi.width - name.width() ) / 2, resolutionRoi.y + resolutionRoi.height + nameOffset + windowRoi.y,
                   display );

        const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::SPANEL, 16 );
        fheroes2::Blit( icon, 0, 0, display, resolutionRoi.x + windowRoi.x, resolutionRoi.y + windowRoi.y, icon.width(), icon.height() );
    }

    void drawOptions( const fheroes2::StandardWindow & window )
    {
        const fheroes2::Text title( _( "Experimental" ), { fheroes2::FontSize::SMALL, fheroes2::FontColor::WHITE } );

        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Text name( _( "Settings" ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } );

        const fheroes2::Rect & windowRoi = window.activeArea();

        title.draw( optionsRoi.x + windowRoi.x + ( optionsRoi.width - title.width() ) / 2, optionsRoi.y - titleOffset + windowRoi.y, display );
        name.draw( optionsRoi.x + windowRoi.x + ( optionsRoi.width - name.width() ) / 2, optionsRoi.y + optionsRoi.height + nameOffset + windowRoi.y, display );

        const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::SPANEL, 14 );
        fheroes2::Blit( icon, 0, 0, display, optionsRoi.x + windowRoi.x, optionsRoi.y + windowRoi.y, icon.width(), icon.height() );
    }

    enum class SelectedWindow : int
    {
        Configuration,
        Resolution,
        Language,
        Options,
        Exit
    };

    SelectedWindow showConfigurationWindow()
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::StandardWindow window( 289, 163, display );
        const fheroes2::Rect windowRoi = window.activeArea();

        const bool isEvilInterface = Settings::Get().ExtGameEvilInterface();
        const int buttonIcnId = isEvilInterface ? ICN::NON_UNIFORM_EVIL_OKAY_BUTTON : ICN::NON_UNIFORM_GOOD_OKAY_BUTTON;
        const fheroes2::Sprite & buttonOkayReleased = fheroes2::AGG::GetICN( buttonIcnId, 0 );
        const fheroes2::Sprite & buttonOkayPressed = fheroes2::AGG::GetICN( buttonIcnId, 1 );

        window.render();
        drawBackground( window );
        drawLanguage( window );
        drawResolution( window );
        drawOptions( window );

        fheroes2::AutoShadowButton okayButton( display, windowRoi.x + ( windowRoi.width - buttonOkayReleased.width() ) / 2,
                                               windowRoi.y + windowRoi.height - 6 - buttonOkayReleased.height(), buttonOkayReleased, buttonOkayPressed );
        okayButton.draw();

        display.render();

        const fheroes2::Rect windowLanguageRoi( languageRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowResolutionRoi( resolutionRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowOptionsRoi( optionsRoi + windowRoi.getPosition() );

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

            if ( le.MousePressRight( windowLanguageRoi ) ) {
                fheroes2::Text header( _( "Select Game Language" ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } );
                fheroes2::Text body( _( "Change language of the game." ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowResolutionRoi ) ) {
                fheroes2::Text header( _( "Select Game Resolution" ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } );
                fheroes2::Text body( _( "Change resolution of the game." ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( windowOptionsRoi ) ) {
                fheroes2::Text header( _( "Settings" ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } );
                fheroes2::Text body( _( "Experimental game settings." ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } );

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
        fheroes2::drawMainMenuScreen();

        SelectedWindow windowType = SelectedWindow::Configuration;
        while ( windowType != SelectedWindow::Exit ) {
            switch ( windowType ) {
            case SelectedWindow::Configuration:
                windowType = showConfigurationWindow();
                break;
            case SelectedWindow::Resolution:
                if ( Dialog::SelectResolution() ) {
                    Settings::Get().Save( "fheroes2.cfg" );
                    // force interface to reset area and positions
                    Interface::Basic::Get().Reset();
                }
                fheroes2::drawMainMenuScreen();
                windowType = SelectedWindow::Configuration;
                break;
            case SelectedWindow::Language: {
                Settings & conf = Settings::Get();

                fheroes2::SupportedLanguage currentLanguage = fheroes2::getLanguageFromAbbreviation( conf.getGameLanguage() );
                const std::vector<fheroes2::SupportedLanguage> supportedLanguages = fheroes2::getSupportedLanguages();

                if ( supportedLanguages.size() > 1 ) {
                    currentLanguage = fheroes2::selectLanguage( supportedLanguages, currentLanguage );
                }
                else {
                    assert( supportedLanguages.front() == fheroes2::SupportedLanguage::English );

                    currentLanguage = fheroes2::SupportedLanguage::English;

                    fheroes2::Text header( _( "Attention" ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } );
                    fheroes2::Text body( _( "Your version of Heroes of Might and Magic II does not support any languages except English." ),
                                         { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } );

                    fheroes2::showMessage( header, body, Dialog::OK );
                }

                conf.setGameLanguage( fheroes2::getLanguageAbbreviation( currentLanguage ) );
                Settings::Get().Save( "fheroes2.cfg" );

                windowType = SelectedWindow::Configuration;
                break;
            }
            case SelectedWindow::Options:
                Dialog::ExtSettings( false );
                windowType = SelectedWindow::Configuration;
                break;
            default:
                return;
            }
        }
    }
}
