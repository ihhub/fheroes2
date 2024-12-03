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

#include "editor_event_details_window.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "artifact.h"
#include "artifact_info.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "editor_ui_helper.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "map_format_info.h"
#include "math_base.h"
#include "mp2.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    const std::array<int, 7> resourceTypes = { Resource::WOOD, Resource::SULFUR, Resource::CRYSTAL, Resource::MERCURY, Resource::ORE, Resource::GEMS, Resource::GOLD };

    const fheroes2::Size messageArea{ 300, 210 };
    const int32_t elementOffset{ 9 };
    const int32_t playerAreaWidth{ fheroes2::Display::DEFAULT_WIDTH - fheroes2::borderWidthPx * 2 - 3 * elementOffset - messageArea.width };
}

namespace Editor
{
    bool eventDetailsDialog( Maps::Map_Format::AdventureMapEventMetadata & eventMetadata, const uint8_t humanPlayerColors, const uint8_t computerPlayerColors,
                             const fheroes2::SupportedLanguage language )
    {
        // First, make sure that the event has proper player colors according to the map specification.
        eventMetadata.humanPlayerColors = eventMetadata.humanPlayerColors & humanPlayerColors;
        eventMetadata.computerPlayerColors = eventMetadata.computerPlayerColors & computerPlayerColors;

        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        fheroes2::StandardWindow background( fheroes2::Display::DEFAULT_WIDTH - fheroes2::borderWidthPx * 2, messageArea.height + 140, true, display );
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

        const fheroes2::Point recurringEventPos{ messageRoi.x + elementOffset, messageRoi.y + messageRoi.height + 2 * elementOffset };

        fheroes2::MovableSprite recurringEventCheckbox;
        const fheroes2::Rect recurringEventArea
            = drawCheckboxWithText( recurringEventCheckbox, _( "Cancel event after first visit" ), display, recurringEventPos.x, recurringEventPos.y, isEvilInterface );
        if ( eventMetadata.isRecurringEvent ) {
            recurringEventCheckbox.hide();
        }
        else {
            recurringEventCheckbox.show();
        }

        const int32_t playerAreaOffsetX = dialogRoi.x + elementOffset + messageRoi.width + elementOffset;

        text.set( _( "Player colors allowed to get event:" ), fheroes2::FontType::normalWhite() );

        int32_t textWidth = playerAreaWidth;
        // If the text fits on one line, make it span two lines.
        while ( text.rows( textWidth ) < 2 ) {
            textWidth = textWidth * 2 / 3;
        }

        text.draw( playerAreaOffsetX + ( playerAreaWidth - textWidth ) / 2, offsetY, textWidth, display );

        const int32_t availablePlayersCount = Color::Count( humanPlayerColors | computerPlayerColors );
        const int32_t checkOffX = ( playerAreaWidth - availablePlayersCount * 32 ) / 2;

        offsetY += 3 + text.height( textWidth );
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

        text.set( _( "Reward:" ), fheroes2::FontType::normalWhite() );
        text.draw( playerAreaOffsetX + ( playerAreaWidth - text.width() ) / 2, offsetY, display );

        const fheroes2::Sprite & artifactFrame = fheroes2::AGG::GetICN( ICN::RESOURCE, 7 );
        const fheroes2::Rect artifactRoi{ playerAreaOffsetX, offsetY + text.height() + 4, artifactFrame.width(), artifactFrame.height() };

        fheroes2::Blit( artifactFrame, display, artifactRoi.x, artifactRoi.y );

        auto redrawArtifactImage = [&display, &artifactRoi]( const int32_t artifactId ) {
            const fheroes2::Sprite & artifactImage = fheroes2::AGG::GetICN( ICN::ARTIFACT, Artifact( artifactId ).IndexSprite64() );
            fheroes2::Copy( artifactImage, 0, 0, display, artifactRoi.x + 6, artifactRoi.y + 6, artifactImage.width(), artifactImage.height() );
        };

        redrawArtifactImage( eventMetadata.artifact );

        const int minibuttonIcnId = isEvilInterface ? ICN::CELLWIN_EVIL : ICN::CELLWIN;

        const fheroes2::Sprite & buttonImage = fheroes2::AGG::GetICN( minibuttonIcnId, 17 );
        const int32_t buttonWidth = buttonImage.width();

        fheroes2::Button buttonDeleteArtifact( artifactRoi.x + ( artifactRoi.width - buttonWidth ) / 2, artifactRoi.y + artifactRoi.height + 5, minibuttonIcnId, 17, 18 );
        buttonDeleteArtifact.draw();

        // Resources
        const int32_t resourceOffsetX = artifactRoi.width + elementOffset;
        const fheroes2::Rect resourceRoi{ playerAreaOffsetX + resourceOffsetX, artifactRoi.y, playerAreaWidth - resourceOffsetX, 99 };
        background.applyTextBackgroundShading( resourceRoi );

        fheroes2::ImageRestorer resourceRoiRestorer( display, resourceRoi.x, resourceRoi.y, resourceRoi.width, resourceRoi.height );

        std::array<fheroes2::Rect, 7> individualResourceRoi;
        renderResources( eventMetadata.resources, resourceRoi, display, individualResourceRoi );

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
            buttonDeleteArtifact.drawOnState( le.isMouseLeftButtonPressedInArea( buttonDeleteArtifact.area() ) );

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
            else if ( le.MouseClickLeft( recurringEventArea ) ) {
                eventMetadata.isRecurringEvent = !eventMetadata.isRecurringEvent;
                eventMetadata.isRecurringEvent ? recurringEventCheckbox.hide() : recurringEventCheckbox.show();
                display.render( recurringEventCheckbox.getArea() );
            }
            else if ( le.MouseClickLeft( artifactRoi ) ) {
                const Artifact artifact = Dialog::selectArtifact( eventMetadata.artifact, false );
                if ( artifact.isValid() ) {
                    int32_t artifactMetadata = eventMetadata.artifactMetadata;

                    if ( artifact.GetID() == Artifact::SPELL_SCROLL ) {
                        artifactMetadata = Dialog::selectSpell( artifactMetadata, true ).GetID();

                        if ( artifactMetadata == Spell::NONE ) {
                            // No spell for the Spell Scroll artifact was selected - cancel the artifact selection.
                            continue;
                        }
                    }
                    else {
                        artifactMetadata = 0;
                    }

                    eventMetadata.artifact = artifact.GetID();
                    eventMetadata.artifactMetadata = artifactMetadata;

                    redrawArtifactImage( eventMetadata.artifact );
                }

                // The opened selectArtifact() dialog might be bigger than this dialog so we render the whole screen.
                display.render();

                isRedrawNeeded = false;
            }
            else if ( le.MouseClickLeft( buttonDeleteArtifact.area() ) ) {
                eventMetadata.artifact = 0;
                eventMetadata.artifactMetadata = 0;

                const fheroes2::Sprite & artifactImage = fheroes2::AGG::GetICN( ICN::ARTIFACT, Artifact( eventMetadata.artifact ).IndexSprite64() );
                fheroes2::Copy( artifactImage, 0, 0, display, artifactRoi.x + 6, artifactRoi.y + 6, artifactImage.width(), artifactImage.height() );

                display.render( artifactRoi );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to save the Event properties." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( artifactRoi ) ) {
                // Since Artifact class does not allow to set a random spell (for obvious reasons),
                // we have to use special UI code to render the popup window with all needed information.
                const Artifact artifact( eventMetadata.artifact );

                if ( artifact.isValid() ) {
                    fheroes2::ArtifactDialogElement artifactUI( artifact );

                    fheroes2::showStandardTextMessage( artifact.GetName(),
                                                       fheroes2::getArtifactData( eventMetadata.artifact ).getDescription( eventMetadata.artifactMetadata ), Dialog::ZERO,
                                                       { &artifactUI } );
                }
                else {
                    fheroes2::showStandardTextMessage( _( "Artifact" ), _( "No artifact will be given as a reward." ), Dialog::ZERO );
                }
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonDeleteArtifact.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Delete Artifact" ), _( "Delete an artifact from the reward." ), Dialog::ZERO );
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
            else if ( le.isMouseRightButtonPressedInArea( recurringEventArea ) ) {
                fheroes2::showStandardTextMessage(
                    _( "Cancel event after first visit" ),
                    _( "If this checkbox is checked, the event will trigger only once. If not checked, the event will trigger every time one of the specified players crosses the event tile." ),
                    Dialog::ZERO );
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
