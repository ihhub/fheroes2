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

#include "editor_spell_selection.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <utility>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "math_tools.h"
#include "pal.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    const int32_t spellRowOffsetY{ 90 };
    const int32_t spellItemWidth{ 110 };

    // Up to 5 spells can be displayed in a row.
    // Up to 5 rows can be displayed.
    // So far the number of spells of any level is much more than 25.
    class SpellContainerUI final
    {
    public:
        SpellContainerUI( fheroes2::Point offset, std::vector<std::pair<Spell, bool>> & spells )
            : _spells( spells )
        {
            assert( !spells.empty() && spells.size() < 25 );

            // Figure out how many rows and columns we want to display.
            for ( size_t i = 1; i < 5; ++i ) {
                if ( i * i >= _spells.size() ) {
                    _spellsPerRow = i;
                    break;
                }
            }

            offset.x += ( fheroes2::Display::DEFAULT_WIDTH - static_cast<int32_t>( _spellsPerRow ) * spellItemWidth ) / 2;

            const int32_t rowCount{ static_cast<int32_t>( ( _spells.size() + _spellsPerRow - 1 ) / _spellsPerRow ) };
            offset.y += ( fheroes2::Display::DEFAULT_HEIGHT - 50 - spellRowOffsetY * rowCount ) / 2;

            // Calculate all areas where we are going to render spells.
            _spellRoi.reserve( _spells.size() );

            const fheroes2::Sprite & scrollImage = fheroes2::AGG::GetICN( ICN::TOWNWIND, 0 );

            const int32_t lastRowColumns = static_cast<int32_t>( _spells.size() % _spellsPerRow );
            const int32_t lastRowOffsetX = ( lastRowColumns > 0 ) ? ( static_cast<int32_t>( _spellsPerRow ) - lastRowColumns ) * spellItemWidth / 2 : 0;

            for ( size_t i = 0; i < _spells.size(); ++i ) {
                const int32_t rowId = static_cast<int32_t>( i / _spellsPerRow );
                const int32_t columnId = static_cast<int32_t>( i % _spellsPerRow );

                if ( rowId == static_cast<int32_t>( _spells.size() / _spellsPerRow ) ) {
                    // This is the last row.
                    _spellRoi.emplace_back( offset.x + columnId * spellItemWidth + lastRowOffsetX, offset.y + rowId * spellRowOffsetY, scrollImage.width(),
                                            scrollImage.height() );
                }
                else {
                    _spellRoi.emplace_back( offset.x + columnId * spellItemWidth, offset.y + rowId * spellRowOffsetY, scrollImage.width(), scrollImage.height() );
                }
            }
        }

        void draw( fheroes2::Image & output )
        {
            const fheroes2::Sprite & scrollImage = fheroes2::AGG::GetICN( ICN::TOWNWIND, 0 );

            fheroes2::Sprite inactiveScrollImage( scrollImage );
            fheroes2::ApplyPalette( inactiveScrollImage, PAL::GetPalette( PAL::PaletteType::GRAY ) );

            for ( size_t i = 0; i < _spells.size(); ++i ) {
                if ( !_spells[i].second ) {
                    // The spell is being inactive.
                    fheroes2::Blit( inactiveScrollImage, output, _spellRoi[i].x, _spellRoi[i].y );
                }
                else {
                    fheroes2::Blit( scrollImage, output, _spellRoi[i].x, _spellRoi[i].y );
                }

                const fheroes2::Sprite & spellImage = fheroes2::AGG::GetICN( ICN::SPELLS, _spells[i].first.IndexSprite() );

                if ( !_spells[i].second ) {
                    // The spell is being inactive.
                    fheroes2::Sprite inactiveSpellImage( spellImage );
                    fheroes2::ApplyPalette( inactiveSpellImage, PAL::GetPalette( PAL::PaletteType::GRAY ) );

                    fheroes2::Blit( inactiveSpellImage, output, _spellRoi[i].x + 3 + ( _spellRoi[i].width - inactiveSpellImage.width() ) / 2,
                                    _spellRoi[i].y + 31 - inactiveSpellImage.height() / 2 );
                }
                else {
                    fheroes2::Blit( spellImage, output, _spellRoi[i].x + 3 + ( _spellRoi[i].width - spellImage.width() ) / 2,
                                    _spellRoi[i].y + 31 - spellImage.height() / 2 );
                }

                const fheroes2::Text text( _spells[i].first.GetName(), fheroes2::FontType::smallWhite() );
                text.draw( _spellRoi[i].x + 18, _spellRoi[i].y + 57, 78, fheroes2::Display::instance() );
            }
        }

        bool processEvents( LocalEvent & eventProcessor )
        {
            const int32_t spellIndex = GetRectIndex( _spellRoi, eventProcessor.getMouseCursorPos() );
            if ( spellIndex < 0 ) {
                return false;
            }

            assert( static_cast<size_t>( spellIndex ) < _spellRoi.size() );

            const fheroes2::Rect spellRoi = _spellRoi[spellIndex];

            if ( eventProcessor.MouseClickLeft( spellRoi ) ) {
                _spells[spellIndex].second = !_spells[spellIndex].second;

                return true;
            }

            if ( eventProcessor.isMouseRightButtonPressedInArea( spellRoi ) ) {
                fheroes2::SpellDialogElement( _spells[spellIndex].first, nullptr ).showPopup( Dialog::ZERO );
            }

            return false;
        }

    private:
        std::vector<std::pair<Spell, bool>> & _spells;

        std::vector<fheroes2::Rect> _spellRoi;

        size_t _spellsPerRow{ 0 };
    };
}

namespace Editor
{
    bool openSpellSelectionWindow( std::string title, int & spellLevel, std::vector<int32_t> & selectedSpells, const bool isMultiLevelSelectionEnabled,
                                   const int32_t minimumEnabledSpells, const bool pickDisabledSpells )
    {
        if ( spellLevel < 1 || spellLevel > 5 ) {
            // What are you trying to achieve?!
            assert( 0 );
            return false;
        }

        const std::vector<int32_t> & availableSpells = Spell::getAllSpellIdsSuitableForSpellBook( spellLevel );
        assert( !availableSpells.empty() );

        // Create a container of active and disabled spells.
        std::vector<std::pair<Spell, bool>> spells;
        spells.reserve( availableSpells.size() );

        bool isAnySpellEnabled = false;

        for ( const int spell : availableSpells ) {
            const bool isSelected = ( ( std::find( selectedSpells.begin(), selectedSpells.end(), spell ) != selectedSpells.end() ) != pickDisabledSpells );

            spells.emplace_back( spell, isSelected );

            if ( isSelected ) {
                isAnySpellEnabled = true;
            }
        }

        // If no spells are selected, select all of them.
        if ( !isAnySpellEnabled ) {
            for ( auto & [spell, isSelected] : spells ) {
                isSelected = true;
            }
        }

        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        const bool isDefaultScreenSize = display.isDefaultSize();

        fheroes2::StandardWindow background( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT, !isDefaultScreenSize );
        const fheroes2::Rect activeArea( background.activeArea() );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        if ( isDefaultScreenSize ) {
            const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK, 0 );
            fheroes2::Copy( backgroundImage, 0, 0, display, activeArea );
        }

        const fheroes2::Text text( std::move( title ), fheroes2::FontType::normalYellow() );
        text.draw( activeArea.x + ( activeArea.width - text.width() ) / 2, activeArea.y + 10, display );

        // Buttons.
        fheroes2::Button buttonOk;
        fheroes2::Button buttonCancel;
        background.renderOkayCancelButtons( buttonOk, buttonCancel );

        fheroes2::Button buttonToggleOn;
        fheroes2::Button buttonToggleOff;

        background.renderButton( buttonToggleOn, isEvilInterface ? ICN::BUTTON_TOGGLE_ALL_ON_EVIL : ICN::BUTTON_TOGGLE_ALL_ON_GOOD, 0, 1, { 0, 7 },
                                 fheroes2::StandardWindow::Padding::BOTTOM_CENTER );
        buttonToggleOn.disable();

        background.renderButton( buttonToggleOff, isEvilInterface ? ICN::BUTTON_TOGGLE_ALL_OFF_EVIL : ICN::BUTTON_TOGGLE_ALL_OFF_GOOD, 0, 1, { 0, 7 },
                                 fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

        fheroes2::ButtonGroup levelSelection;

        if ( isMultiLevelSelectionEnabled ) {
            const int32_t levelSelectionStepX{ 62 };
            const int32_t levelOffsetY{ 410 };
            const int32_t widthInBetweenButtons{ buttonCancel.area().x - buttonOk.area().x - buttonOk.area().width };
            const int32_t spellSelectionButtonOffsetX{ buttonOk.area().x + buttonOk.area().width + ( widthInBetweenButtons - levelSelectionStepX * 5 ) / 2 };

            levelSelection.createButton( spellSelectionButtonOffsetX, activeArea.y + levelOffsetY, isEvilInterface ? ICN::BUTTON_1_EVIL : ICN::BUTTON_1_GOOD, 0, 1, 1 );
            levelSelection.createButton( spellSelectionButtonOffsetX + levelSelectionStepX, activeArea.y + levelOffsetY,
                                         isEvilInterface ? ICN::BUTTON_2_EVIL : ICN::BUTTON_2_GOOD, 0, 1, 2 );
            levelSelection.createButton( spellSelectionButtonOffsetX + levelSelectionStepX * 2, activeArea.y + levelOffsetY,
                                         isEvilInterface ? ICN::BUTTON_3_EVIL : ICN::BUTTON_3_GOOD, 0, 1, 3 );
            levelSelection.createButton( spellSelectionButtonOffsetX + levelSelectionStepX * 3, activeArea.y + levelOffsetY,
                                         isEvilInterface ? ICN::BUTTON_4_EVIL : ICN::BUTTON_4_GOOD, 0, 1, 4 );
            levelSelection.createButton( spellSelectionButtonOffsetX + levelSelectionStepX * 4, activeArea.y + levelOffsetY,
                                         isEvilInterface ? ICN::BUTTON_5_EVIL : ICN::BUTTON_5_GOOD, 0, 1, 5 );

            levelSelection.drawShadows( display );

            for ( int32_t i = 0; i < 5; ++i ) {
                if ( i + 1 == spellLevel ) {
                    levelSelection.button( i ).press();
                }

                levelSelection.button( i ).draw( display );
            }
        }

        fheroes2::ImageRestorer restorer( display, activeArea.x, activeArea.y, activeArea.width, activeArea.height );

        SpellContainerUI spellContainer( activeArea.getPosition(), spells );

        spellContainer.draw( display );

        display.render( background.totalArea() );

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancel.area() ) ) {
                return false;
            }

            if ( buttonOk.isEnabled() && ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( buttonOk.area() ) ) ) {
                break;
            }

            if ( buttonOk.isEnabled() ) {
                buttonOk.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonOk.area() ) );
            }

            if ( buttonToggleOn.isEnabled() ) {
                buttonToggleOn.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonToggleOn.area() ) );
            }
            else if ( buttonToggleOff.isEnabled() ) {
                buttonToggleOff.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonToggleOff.area() ) );
            }

            buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );

            if ( isMultiLevelSelectionEnabled ) {
                bool hasSpellLevelChanged = false;
                for ( int32_t i = 0; i < 5; ++i ) {
                    if ( le.isMouseRightButtonPressedInArea( levelSelection.button( i ).area() ) ) {
                        std::string str = _( "Click to show only level %{level} spells." );
                        StringReplace( str, "%{level}", i + 1 );
                        fheroes2::showStandardTextMessage( _( "Spells level" ), std::move( str ), Dialog::ZERO );
                        break;
                    }

                    if ( i + 1 == spellLevel || !levelSelection.button( i ).isEnabled() ) {
                        continue;
                    }

                    if ( le.MouseClickLeft( levelSelection.button( i ).area() ) ) {
                        spellLevel = i + 1;
                        hasSpellLevelChanged = true;
                        break;
                    }

                    levelSelection.button( i ).drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( levelSelection.button( i ).area() ) );
                }

                if ( hasSpellLevelChanged ) {
                    break;
                }
            }

            bool toggleAllOn = false;
            bool toggleAllOff = false;

            if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to accept the changes made." ), Dialog::ZERO );
            }
            else if ( buttonToggleOn.isEnabled() && le.isMouseRightButtonPressedInArea( buttonToggleOn.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Enable All Spells" ), _( "Click to enable all spells." ), Dialog::ZERO );
            }
            else if ( buttonToggleOff.isEnabled() && le.isMouseRightButtonPressedInArea( buttonToggleOff.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Disable All Spells" ), _( "Click to disable all spells." ), Dialog::ZERO );
            }
            else if ( buttonToggleOn.isEnabled() && le.MouseClickLeft( buttonToggleOn.area() ) ) {
                toggleAllOn = true;

                for ( auto & [spell, isSelected] : spells ) {
                    isSelected = true;
                }

                buttonToggleOn.disable();
                buttonToggleOff.enable();
            }
            else if ( buttonToggleOff.isEnabled() && le.MouseClickLeft( buttonToggleOff.area() ) ) {
                toggleAllOff = true;

                for ( auto & [spell, isSelected] : spells ) {
                    isSelected = false;
                }

                buttonToggleOff.disable();
                buttonToggleOn.enable();
            }

            if ( toggleAllOn || toggleAllOff || spellContainer.processEvents( le ) ) {
                restorer.restore();

                if ( toggleAllOn ) {
                    buttonToggleOff.draw();
                }
                else if ( toggleAllOff ) {
                    buttonToggleOn.draw();
                }

                spellContainer.draw( display );

                // Check if the count of non-disabled is mote then the minimum limit. If so then disable the OKAY button.
                bool disableChangesConfirmation = true;
                int32_t selectedSpellsCount = 0;

                for ( const auto & [spell, isSelected] : spells ) {
                    if ( isSelected ) {
                        ++selectedSpellsCount;
                        if ( selectedSpellsCount >= minimumEnabledSpells ) {
                            disableChangesConfirmation = false;
                            break;
                        }
                    }
                }

                if ( disableChangesConfirmation ) {
                    buttonOk.disable();
                }
                else {
                    buttonOk.enable();
                }
                buttonOk.draw( display );

                if ( isMultiLevelSelectionEnabled ) {
                    for ( int32_t i = 0; i < 5; ++i ) {
                        if ( i + 1 == spellLevel ) {
                            continue;
                        }
                        if ( disableChangesConfirmation ) {
                            levelSelection.button( i ).disable();
                        }
                        else {
                            levelSelection.button( i ).enable();
                        }
                        levelSelection.button( i ).draw( display );
                    }
                }

                display.render( activeArea );
            }
        }

        selectedSpells.clear();

        for ( const auto & [spell, isSelected] : spells ) {
            if ( isSelected != pickDisabledSpells ) {
                selectedSpells.emplace_back( spell.GetID() );
            }
        }

        // If all spells are selected, remove all spells from the selection since an empty container means the use of the default behavior of the game.
        if ( selectedSpells.size() == spells.size() ) {
            selectedSpells = {};
        }

        // Do not restore since StandardWindow has its own restorer.
        restorer.reset();

        return true;
    }
}
