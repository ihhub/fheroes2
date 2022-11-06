/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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
#include "dialog_graphics_settings.h"
#include "dialog_resolution.h"
#include "game_hotkeys.h"
#include "game_mainmenu_ui.h"
#include "icn.h"
#include "localevent.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_option_item.h"
#include "ui_text.h"

namespace
{
    enum class SelectedWindow : int
    {
        Configuration,
        Resolution,
        Mode,
        VSync,
        SystemInfo,
        Exit
    };

    const fheroes2::Size offsetBetweenOptions{ 118, 110 };
    const fheroes2::Point optionOffset{ 69, 47 };
    const int32_t optionWindowSize{ 65 };

    const fheroes2::Rect resolutionRoi{ optionOffset.x, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect modeRoi{ optionOffset.x + offsetBetweenOptions.width, optionOffset.y, optionWindowSize, optionWindowSize };
    const fheroes2::Rect vSyncRoi{ optionOffset.x, optionOffset.y + offsetBetweenOptions.height, optionWindowSize, optionWindowSize };
    const fheroes2::Rect systemInfoRoi{ optionOffset.x + offsetBetweenOptions.width, optionOffset.y + offsetBetweenOptions.height, optionWindowSize, optionWindowSize };

    void drawResolution( const fheroes2::Rect & optionRoi )
    {
        const fheroes2::Display & display = fheroes2::Display::instance();
        std::string resolutionName = std::to_string( display.width() ) + 'x' + std::to_string( display.height() );

        fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::SPANEL, Settings::Get().ExtGameEvilInterface() ? 17 : 16 ), _( "Resolution" ),
                              std::move( resolutionName ) );
    }

    void drawMode( const fheroes2::Rect & optionRoi )
    {
        const fheroes2::Sprite & originalIcon = fheroes2::AGG::GetICN( ICN::SPANEL, Settings::Get().ExtGameEvilInterface() ? 17 : 16 );

        if ( fheroes2::engine().isFullScreen() ) {
            fheroes2::Sprite icon = originalIcon;
            fheroes2::Resize( originalIcon, 6, 6, 53, 53, icon, 2, 2, 61, 61 );

            fheroes2::drawOption( optionRoi, icon, _( "window|Mode" ), _( "Fullscreen" ) );
        }
        else {
            fheroes2::drawOption( optionRoi, originalIcon, _( "window|Mode" ), _( "Windowed" ) );
        }
    }

    void drawVSync( const fheroes2::Rect & optionRoi )
    {
        const bool isVSyncEnabled = Settings::Get().isVSyncEnabled();

        fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::SPANEL, isVSyncEnabled ? 18 : 19 ), _( "V-Sync" ), isVSyncEnabled ? _( "on" ) : _( "off" ) );
    }

    void drawSystemInfo( const fheroes2::Rect & optionRoi )
    {
        const bool isSystemInfoDisplayed = Settings::Get().isSystemInfoEnabled();

        fheroes2::Sprite image = fheroes2::Crop( fheroes2::AGG::GetICN( ICN::ESPANBKG, 0 ), 69, 47, 65, 65 );
        fheroes2::Text info;
        if ( isSystemInfoDisplayed ) {
            info.set( _( "FPS" ), fheroes2::FontType( fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW ) );
        }
        else {
            info.set( _( "N/A" ), fheroes2::FontType( fheroes2::FontSize::NORMAL, fheroes2::FontColor::GRAY ) );
        }
        info.draw( ( image.width() - info.width() ) / 2, ( image.height() - info.height() ) / 2, image );

        fheroes2::drawOption( optionRoi, image, _( "System Info" ), isSystemInfoDisplayed ? _( "on" ) : _( "off" ) );
    }

    SelectedWindow showConfigurationWindow()
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.ExtGameEvilInterface();
        const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::ESPANBKG_EVIL : ICN::ESPANBKG ), 0 );
        const fheroes2::Sprite & dialogShadow = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::CSPANBKE : ICN::CSPANBKG ), 1 );

        const fheroes2::Point dialogOffset( ( display.width() - dialog.width() ) / 2, ( display.height() - dialog.height() ) / 2 );
        const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

        const fheroes2::Rect windowRoi{ dialogOffset.x, dialogOffset.y, dialog.width(), dialog.height() };

        const fheroes2::ImageRestorer restorer( display, shadowOffset.x, shadowOffset.y, dialog.width() + BORDERWIDTH, dialog.height() + BORDERWIDTH );

        fheroes2::Blit( dialogShadow, display, windowRoi.x - BORDERWIDTH, windowRoi.y + BORDERWIDTH );
        fheroes2::Blit( dialog, display, windowRoi.x, windowRoi.y );

        const fheroes2::Rect windowResolutionRoi( resolutionRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowModeRoi( modeRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowVSyncRoi( vSyncRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowSystemInfoRoi( systemInfoRoi + windowRoi.getPosition() );

        drawResolution( windowResolutionRoi );
        drawMode( windowModeRoi );
        drawVSync( windowVSyncRoi );
        drawSystemInfo( windowSystemInfoRoi );

        const fheroes2::Point buttonOffset( 112 + windowRoi.x, 252 + windowRoi.y );
        fheroes2::Button okayButton( buttonOffset.x, buttonOffset.y, isEvilInterface ? ICN::SPANBTNE : ICN::SPANBTN, 0, 1 );
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
            if ( le.MouseClickLeft( windowResolutionRoi ) ) {
                return SelectedWindow::Resolution;
            }
            if ( le.MouseClickLeft( windowModeRoi ) ) {
                return SelectedWindow::Mode;
            }
            if ( le.MouseClickLeft( windowVSyncRoi ) ) {
                return SelectedWindow::VSync;
            }
            if ( le.MouseClickLeft( windowSystemInfoRoi ) ) {
                return SelectedWindow::SystemInfo;
            }

            if ( le.MousePressRight( windowResolutionRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Select Game Resolution" ), _( "Change the resolution of the game." ), 0 );
            }
            else if ( le.MousePressRight( windowModeRoi ) ) {
                fheroes2::showStandardTextMessage( _( "window|Mode" ), _( "Toggle between fullscreen and windowed modes." ), 0 );
            }
            else if ( le.MousePressRight( windowVSyncRoi ) ) {
                fheroes2::showStandardTextMessage( _( "V-Sync" ), _( "Enabled V-Sync option is useful for some monitors to resolve flickering issues." ), 0 );
            }
            if ( le.MousePressRight( windowSystemInfoRoi ) ) {
                fheroes2::showStandardTextMessage( _( "System Info" ), _( "Show extra information such as FPS and current date and time." ), 0 );
            }
        }

        return SelectedWindow::Exit;
    }
}

namespace fheroes2
{
    void openGraphicsSettingsDialog()
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
            case SelectedWindow::Mode:
                conf.setFullScreen( !conf.FullScreen() );
                conf.Save( Settings::configFileName );
                windowType = SelectedWindow::Configuration;
                break;
            case SelectedWindow::VSync:
                conf.setVSync( !conf.isVSyncEnabled() );
                conf.Save( Settings::configFileName );
                windowType = SelectedWindow::Configuration;
                break;
            case SelectedWindow::SystemInfo:
                conf.setSystemInfo( !conf.isSystemInfoEnabled() );
                conf.Save( Settings::configFileName );
                windowType = SelectedWindow::Configuration;
                break;
            default:
                return;
            }
        }
    }
}
