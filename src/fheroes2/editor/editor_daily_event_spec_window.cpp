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

#include "editor_daily_event_spec_window.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "editor_ui_helper.h"
#include "game_hotkeys.h"
#include "image.h"
#include "localevent.h"
#include "map_format_info.h"
#include "math_base.h"
#include "mp2.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    const std::array<int, 7> resourceTypes = { Resource::WOOD, Resource::SULFUR, Resource::CRYSTAL, Resource::MERCURY, Resource::ORE, Resource::GEMS, Resource::GOLD };

    const fheroes2::Size messageArea{ 300, 210 };
    const int32_t elementOffset{ 9 };
    const int32_t playerAreaWidth{ fheroes2::Display::DEFAULT_WIDTH - fheroes2::borderWidthPx * 2 - 3 * elementOffset - messageArea.width };

    const int32_t daysInYear{ 7 * 4 * 12 };
    const int32_t lastDayForEvents{ daysInYear * 100 };
}

namespace Editor
{
    bool editDailyEvent( Maps::Map_Format::DailyEvent & eventMetadata, const uint8_t humanPlayerColors, const uint8_t computerPlayerColors,
                         const fheroes2::SupportedLanguage language )
    {
        // An event can be outdated in terms of players since we don't update players while placing or removing heroes and castles.
        eventMetadata.humanPlayerColors = eventMetadata.humanPlayerColors & humanPlayerColors;
        eventMetadata.computerPlayerColors = eventMetadata.computerPlayerColors & computerPlayerColors;

        // First occurrence day logically has no limits but we still have to check it for some human logical value.
        eventMetadata.firstOccurrenceDay = std::clamp( eventMetadata.firstOccurrenceDay, 1U, static_cast<uint32_t>( lastDayForEvents ) );

        eventMetadata.repeatPeriodInDays = std::min( eventMetadata.repeatPeriodInDays, static_cast<uint32_t>( daysInYear ) );

        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        fheroes2::StandardWindow background( fheroes2::Display::DEFAULT_WIDTH - fheroes2::borderWidthPx * 2, messageArea.height + 220, true, display );
        const fheroes2::Rect dialogRoi = background.activeArea();

        int32_t offsetY = dialogRoi.y + elementOffset;

        const fheroes2::Text title( MP2::StringObject( MP2::OBJ_EVENT ), fheroes2::FontType::normalYellow() );
        title.draw( dialogRoi.x + ( dialogRoi.width - title.width() ) / 2, offsetY, display );

        offsetY += title.height() + elementOffset;

        fheroes2::Text text{ _( "Message Text:" ), fheroes2::FontType::normalWhite() };

        const fheroes2::Rect messageRoi{ dialogRoi.x + elementOffset, offsetY + text.height(), messageArea.width, messageArea.height };
        background.applyTextBackgroundShading( messageRoi );

        fheroes2::ImageRestorer messageRoiRestorer( display, messageRoi.x, messageRoi.y, messageRoi.width, messageRoi.height );

        text.draw( messageRoi.x + ( messageRoi.width - text.width() ) / 2, offsetY, display );

        text.set( eventMetadata.message, fheroes2::FontType::normalWhite(), language );
        text.draw( messageRoi.x + 5, messageRoi.y + 5, messageRoi.width - 10, display );

        // Resources
        text.set( _( "Reward:" ), fheroes2::FontType::normalWhite() );
        text.draw( messageRoi.x + ( messageRoi.width - text.width() ) / 2, offsetY + messageArea.height + 2 * elementOffset, display );

        const fheroes2::Rect resourceRoi{ messageRoi.x, text.height() + offsetY + messageArea.height + 2 * elementOffset, messageRoi.width, 99 };
        background.applyTextBackgroundShading( resourceRoi );

        fheroes2::ImageRestorer resourceRoiRestorer( display, resourceRoi.x, resourceRoi.y, resourceRoi.width, resourceRoi.height );

        std::array<fheroes2::Rect, 7> individualResourceRoi;
        renderResources( eventMetadata.resources, resourceRoi, display, individualResourceRoi );

        const int32_t playerAreaOffsetX = dialogRoi.x + elementOffset + messageRoi.width + elementOffset;

        text.set( _( "Player colors allowed to get event:" ), fheroes2::FontType::normalWhite() );

        int32_t textWidth = playerAreaWidth;
        // If the text fits on one line, make it span two lines.
        while ( text.rows( textWidth ) < 2 ) {
            textWidth = textWidth * 2 / 3;
        }

        text.draw( playerAreaOffsetX + ( playerAreaWidth - textWidth ) / 2, offsetY, textWidth, display );
        offsetY += 3 + text.height( textWidth );

        const int32_t availablePlayersCount = Color::Count( humanPlayerColors | computerPlayerColors );
        const int32_t checkOffX = ( playerAreaWidth - availablePlayersCount * 32 ) / 2;

        std::vector<std::unique_ptr<Checkbox>> humanCheckboxes;
        createColorCheckboxes( humanCheckboxes, humanPlayerColors, eventMetadata.humanPlayerColors, playerAreaOffsetX + checkOffX, offsetY, display );

        assert( humanCheckboxes.size() == static_cast<size_t>( Color::Count( humanPlayerColors ) ) );

        offsetY += 35;

        text.set( _( "Computer colors allowed to get event:" ), fheroes2::FontType::normalWhite() );

        textWidth = playerAreaWidth;

        // If the text fits on one line, make it span two lines.
        while ( text.rows( textWidth ) < 2 ) {
            textWidth = textWidth * 2 / 3;
        }

        text.draw( playerAreaOffsetX + ( playerAreaWidth - textWidth ) / 2, offsetY, textWidth, display );
        offsetY += 3 + text.height( textWidth );

        std::vector<std::unique_ptr<Checkbox>> computerCheckboxes;
        createColorCheckboxes( computerCheckboxes, computerPlayerColors, eventMetadata.computerPlayerColors, playerAreaOffsetX + checkOffX, offsetY, display );

        assert( computerCheckboxes.size() == static_cast<size_t>( Color::Count( computerPlayerColors ) ) );

        offsetY += 35;

        text.set( _( "First day of occurrence:" ), fheroes2::FontType::normalWhite() );

        textWidth = playerAreaWidth;

        // If the text fits on one line, make it span two lines.
        while ( text.rows( textWidth ) < 2 ) {
            textWidth = textWidth * 2 / 3;
        }

        text.draw( playerAreaOffsetX + ( playerAreaWidth - textWidth ) / 2, offsetY, textWidth, display );
        offsetY += 3 + text.height( textWidth );

        const fheroes2::Point firstDayOccurrencePos{ playerAreaOffsetX + ( playerAreaWidth - fheroes2::ValueSelectionDialogElement::getArea().width ) / 2, offsetY };

        fheroes2::ValueSelectionDialogElement firstDaySelection( 1, lastDayForEvents, static_cast<int32_t>( eventMetadata.firstOccurrenceDay ), 1,
                                                                 firstDayOccurrencePos );

        firstDaySelection.draw( display );

        offsetY += 10 + fheroes2::ValueSelectionDialogElement::getArea().height;

        fheroes2::ImageRestorer firstDateDescription( display, playerAreaOffsetX, offsetY, playerAreaWidth, 35 );

        text.set( getDateDescription( firstDaySelection.getValue() ), fheroes2::FontType::normalWhite() );
        text.draw( playerAreaOffsetX + ( playerAreaWidth - text.width() ) / 2, offsetY, text.width(), display );

        offsetY += 35;

        text.set( _( "Repeat period (days):" ), fheroes2::FontType::normalWhite() );

        textWidth = playerAreaWidth;

        // If the text fits on one line, make it span two lines.
        while ( text.rows( textWidth ) < 2 ) {
            textWidth = textWidth * 2 / 3;
        }

        text.draw( playerAreaOffsetX + ( playerAreaWidth - textWidth ) / 2, offsetY, textWidth, display );
        offsetY += 3 + text.height( textWidth );

        const fheroes2::Point repeatPeriodPos{ playerAreaOffsetX + ( playerAreaWidth - fheroes2::ValueSelectionDialogElement::getArea().width ) / 2, offsetY };

        fheroes2::ValueSelectionDialogElement repeatPeriodSelection( 0, daysInYear, static_cast<int32_t>( eventMetadata.repeatPeriodInDays ), 1, repeatPeriodPos );

        repeatPeriodSelection.draw( display );

        // Window buttons
        fheroes2::Button buttonOk;
        fheroes2::Button buttonCancel;

        background.renderOkayCancelButtons( buttonOk, buttonCancel, isEvilInterface );

        display.render( background.totalArea() );

        bool isRedrawNeeded = false;

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedInArea( buttonOk.area() ) );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) );

            if ( firstDaySelection.processEvents() ) {
                firstDaySelection.draw( display );

                eventMetadata.firstOccurrenceDay = static_cast<uint32_t>( firstDaySelection.getValue() );

                firstDateDescription.restore();

                text.set( getDateDescription( firstDaySelection.getValue() ), fheroes2::FontType::normalWhite() );
                text.draw( playerAreaOffsetX + ( playerAreaWidth - text.width() ) / 2, firstDateDescription.y(), text.width(), display );

                isRedrawNeeded = true;
            }

            if ( repeatPeriodSelection.processEvents() ) {
                repeatPeriodSelection.draw( display );

                eventMetadata.repeatPeriodInDays = static_cast<uint32_t>( repeatPeriodSelection.getValue() );

                isRedrawNeeded = true;
            }

            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return false;
            }

            if ( buttonOk.isEnabled() && ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) ) {
                break;
            }

            for ( const auto & humanCheckbox : humanCheckboxes ) {
                const fheroes2::Rect & checkboxRect = humanCheckbox->getRect();

                if ( le.MouseClickLeft( checkboxRect ) ) {
                    const int color = humanCheckbox->getColor();
                    if ( humanCheckbox->toggle() ) {
                        eventMetadata.humanPlayerColors |= color;
                    }
                    else {
                        eventMetadata.humanPlayerColors ^= color;
                    }

                    break;
                }

                if ( le.isMouseRightButtonPressedInArea( checkboxRect ) ) {
                    std::string header = _( "Allow %{color} human player to get event" );
                    std::string messageText = _( "If this checkbox is checked, this event will trigger for the %{color} player if they are controlled by a human." );

                    const std::string colorString = Color::String( humanCheckbox->getColor() );
                    StringReplace( header, "%{color}", colorString );
                    StringReplace( messageText, "%{color}", colorString );

                    fheroes2::showStandardTextMessage( std::move( header ), std::move( messageText ), Dialog::ZERO );

                    break;
                }
            }

            for ( const auto & computerCheckbox : computerCheckboxes ) {
                const fheroes2::Rect & checkboxRect = computerCheckbox->getRect();

                if ( le.MouseClickLeft( checkboxRect ) ) {
                    const int color = computerCheckbox->getColor();
                    if ( computerCheckbox->toggle() ) {
                        eventMetadata.computerPlayerColors |= color;
                    }
                    else {
                        eventMetadata.computerPlayerColors ^= color;
                    }

                    break;
                }

                if ( le.isMouseRightButtonPressedInArea( checkboxRect ) ) {
                    std::string header = _( "Allow %{color} computer player to get event" );
                    std::string messageText = _( "If this checkbox is checked, this event will trigger for the %{color} player if they are controlled by a computer." );

                    const std::string colorString = Color::String( computerCheckbox->getColor() );
                    StringReplace( header, "%{color}", colorString );
                    StringReplace( messageText, "%{color}", colorString );

                    fheroes2::showStandardTextMessage( std::move( header ), std::move( messageText ), Dialog::ZERO );

                    break;
                }
            }

            for ( size_t i = 0; i < individualResourceRoi.size(); ++i ) {
                if ( le.MouseClickLeft( individualResourceRoi[i] ) ) {
                    const int resourceType = resourceTypes[i];
                    int32_t * resourcePtr = eventMetadata.resources.GetPtr( resourceType );
                    assert( resourcePtr != nullptr );

                    int32_t temp = *resourcePtr;

                    if ( Dialog::SelectCount( Resource::String( resourceType ), -99999, 999999, temp, 1 ) ) {
                        *resourcePtr = temp;
                    }

                    resourceRoiRestorer.restore();

                    renderResources( eventMetadata.resources, resourceRoi, display, individualResourceRoi );
                    display.render( resourceRoi );
                    break;
                }
            }

            if ( le.MouseClickLeft( messageRoi ) ) {
                std::string temp = eventMetadata.message;

                const fheroes2::Text body{ _( "Message:" ), fheroes2::FontType::normalWhite() };
                if ( Dialog::inputString( fheroes2::Text{}, body, temp, 200, true, language ) ) {
                    eventMetadata.message = std::move( temp );

                    messageRoiRestorer.restore();
                    text.set( eventMetadata.message, fheroes2::FontType::normalWhite(), language );
                    text.draw( messageRoi.x + 5, messageRoi.y + 5, messageRoi.width - 10, display );
                    isRedrawNeeded = true;
                }
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to save the Event properties." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( resourceRoi ) ) {
                if ( eventMetadata.resources.GetValidItemsCount() == 0 ) {
                    fheroes2::showStandardTextMessage( _( "Resources" ), _( "No resources will be given as a reward." ), Dialog::ZERO );
                }
                else {
                    fheroes2::showResourceMessage( fheroes2::Text( _( "Resources" ), fheroes2::FontType::normalYellow() ),
                                                   fheroes2::Text{ _( "Resources will be given as a reward." ), fheroes2::FontType::normalWhite() }, Dialog::ZERO,
                                                   eventMetadata.resources );
                }
            }
            else if ( le.isMouseRightButtonPressedInArea( messageRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Event Message Text" ), _( "Click here to change the event message." ), Dialog::ZERO );
            }

            if ( isRedrawNeeded ) {
                isRedrawNeeded = false;

                display.render( dialogRoi );
            }
        }

        return true;
    }
}
