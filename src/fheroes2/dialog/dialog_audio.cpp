/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2025                                             *
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

#include "dialog_audio.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

#include "agg_image.h"
#include "audio.h"
#include "audio_manager.h"
#include "cursor.h"
#include "game.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_option_item.h"
#include "ui_window.h"

namespace
{
    void drawDialog( const std::vector<fheroes2::Rect> & rects )
    {
        assert( rects.size() == 4 );

        const Settings & conf = Settings::Get();

        // Music volume.
        const bool isMusicOn = ( Audio::isValid() && conf.MusicVolume() > 0 );
        const fheroes2::Sprite & musicVolumeIcon = fheroes2::AGG::GetICN( ICN::SPANEL, isMusicOn ? 1 : 0 );
        std::string value;
        if ( isMusicOn ) {
            value = std::to_string( conf.MusicVolume() );
        }
        else {
            value = _( "off" );
        }

        fheroes2::drawOption( rects[0], musicVolumeIcon, _( "Music" ), value, fheroes2::UiOptionTextWidth::TWO_ELEMENTS_ROW );

        // Sound volume.
        const bool isAudioOn = ( Audio::isValid() && conf.SoundVolume() > 0 );
        const fheroes2::Sprite & soundVolumeOption = fheroes2::AGG::GetICN( ICN::SPANEL, isAudioOn ? 3 : 2 );
        if ( isAudioOn ) {
            value = std::to_string( conf.SoundVolume() );
        }
        else {
            value = _( "off" );
        }

        fheroes2::drawOption( rects[1], soundVolumeOption, _( "Effects" ), value, fheroes2::UiOptionTextWidth::TWO_ELEMENTS_ROW );

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

        fheroes2::drawOption( rects[2], musicTypeIcon, _( "Music Type" ), value, fheroes2::UiOptionTextWidth::TWO_ELEMENTS_ROW );

        // 3D Audio.
        const bool is3DAudioEnabled = conf.is3DAudioEnabled();
        const fheroes2::Sprite & interfaceStateIcon = is3DAudioEnabled ? fheroes2::AGG::GetICN( ICN::SPANEL, 11 ) : fheroes2::AGG::GetICN( ICN::SPANEL, 10 );
        if ( is3DAudioEnabled ) {
            value = _( "On" );
        }
        else {
            value = _( "Off" );
        }

        fheroes2::drawOption( rects[3], interfaceStateIcon, _( "3D Audio" ), value, fheroes2::UiOptionTextWidth::TWO_ELEMENTS_ROW );
    }
}

namespace Dialog
{
    bool openAudioSettingsDialog( const bool fromAdventureMap )
    {
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::StandardWindow background( 289, 272, true, display );

        const fheroes2::Rect windowRoi = background.activeArea();

        Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        fheroes2::Button buttonOk;
        const int buttonOkIcnId = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        background.renderButton( buttonOk, buttonOkIcnId, 0, 1, { 0, 11 }, fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

        fheroes2::ImageRestorer emptyDialogRestorer( display, windowRoi.x, windowRoi.y, windowRoi.width, windowRoi.height );

        const fheroes2::Sprite & optionSprite = fheroes2::AGG::GetICN( ICN::SPANEL, 0 );
        const fheroes2::Point optionOffset( windowRoi.x + 53, windowRoi.y + 31 );
        const fheroes2::Point optionStep( 118, 110 );

        std::vector<fheroes2::Rect> roi;
        roi.reserve( 4 );
        roi.emplace_back( optionOffset.x, optionOffset.y, optionSprite.width(), optionSprite.height() );
        roi.emplace_back( optionOffset.x + optionStep.x, optionOffset.y, optionSprite.width(), optionSprite.height() );
        roi.emplace_back( optionOffset.x, optionOffset.y + optionStep.y, optionSprite.width(), optionSprite.height() );
        roi.emplace_back( optionOffset.x + optionStep.x, optionOffset.y + optionStep.y, optionSprite.width(), optionSprite.height() );

        const fheroes2::Rect & musicVolumeRoi = roi[0];
        const fheroes2::Rect & soundVolumeRoi = roi[1];
        const fheroes2::Rect & musicTypeRoi = roi[2];
        const fheroes2::Rect & audio3D = roi[3];

        drawDialog( roi );

        display.render( background.totalArea() );

        bool saveConfig = false;

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonOk.area() ) );

            if ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyCloseWindow() ) {
                break;
            }

            bool haveSettingsChanged = false;

            if ( Audio::isValid() ) {
                {
                    bool haveMusicSettingsChanged = false;

                    if ( le.MouseClickLeft( musicVolumeRoi ) ) {
                        conf.SetMusicVolume( ( conf.MusicVolume() + 1 ) % 11 );
                        haveMusicSettingsChanged = true;
                    }
                    else if ( le.isMouseWheelUpInArea( musicVolumeRoi ) ) {
                        conf.SetMusicVolume( conf.MusicVolume() + 1 );
                        haveMusicSettingsChanged = true;
                    }
                    else if ( le.isMouseWheelDownInArea( musicVolumeRoi ) ) {
                        conf.SetMusicVolume( conf.MusicVolume() - 1 );
                        haveMusicSettingsChanged = true;
                    }

                    if ( haveMusicSettingsChanged ) {
                        Music::setVolume( 100 * conf.MusicVolume() / 10 );

                        haveSettingsChanged = true;
                    }
                }

                {
                    bool haveSoundSettingsChanged = false;

                    if ( le.MouseClickLeft( soundVolumeRoi ) ) {
                        conf.SetSoundVolume( ( conf.SoundVolume() + 1 ) % 11 );
                        haveSoundSettingsChanged = true;
                    }
                    else if ( le.isMouseWheelUpInArea( soundVolumeRoi ) ) {
                        conf.SetSoundVolume( conf.SoundVolume() + 1 );
                        haveSoundSettingsChanged = true;
                    }
                    else if ( le.isMouseWheelDownInArea( soundVolumeRoi ) ) {
                        conf.SetSoundVolume( conf.SoundVolume() - 1 );
                        haveSoundSettingsChanged = true;
                    }

                    if ( haveSoundSettingsChanged ) {
                        Mixer::setVolume( 100 * conf.SoundVolume() / 10 );

                        haveSettingsChanged = true;
                    }
                }
            }

            if ( le.MouseClickLeft( musicTypeRoi ) ) {
                int type = conf.MusicType() + 1;
                // If there's no expansion files we skip this option
                if ( type == MUSIC_MIDI_EXPANSION && !conf.isPriceOfLoyaltySupported() ) {
                    ++type;
                }

                conf.SetMusicType( type > MUSIC_EXTERNAL ? 0 : type );

                AudioManager::PlayCurrentMusic();

                haveSettingsChanged = true;
            }

            if ( le.MouseClickLeft( audio3D ) ) {
                conf.set3DAudio( !conf.is3DAudioEnabled() );

                if ( fromAdventureMap ) {
                    Game::EnvironmentSoundMixer();
                }

                haveSettingsChanged = true;
            }

            if ( le.isMouseRightButtonPressedInArea( musicVolumeRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Music" ), _( "Toggle ambient music level." ), 0 );
            }

            else if ( le.isMouseRightButtonPressedInArea( soundVolumeRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Effects" ), _( "Toggle foreground sounds level." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( musicTypeRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Music Type" ), _( "Change the type of music." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( audio3D ) ) {
                fheroes2::showStandardTextMessage( _( "3D Audio" ), _( "Toggle the 3D effect of foreground sounds." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Exit this menu." ), 0 );
            }

            if ( haveSettingsChanged ) {
                emptyDialogRestorer.restore();
                drawDialog( roi );
                display.render( background.totalArea() );

                saveConfig = true;
            }
        }

        return saveConfig;
    }
}
