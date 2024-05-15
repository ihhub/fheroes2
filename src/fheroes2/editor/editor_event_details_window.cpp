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
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    const std::array<int, 7> resourceTypes = { Resource::WOOD, Resource::SULFUR, Resource::CRYSTAL, Resource::MERCURY, Resource::ORE, Resource::GEMS, Resource::GOLD };

    const fheroes2::Size messageArea{ 300, 200 };
    const fheroes2::Size playerArea{ 300, 200 };
    const int32_t elementOffset{ 9 };

    class Checkbox
    {
    public:
        Checkbox( const int32_t x, const int32_t y, const int boxColor, const bool checked, fheroes2::Display & output = fheroes2::Display::instance() )
            : color( boxColor )
        {
            const int32_t icnIndex = Color::GetIndex( color ) + 43;
            const fheroes2::Sprite & playerIcon = fheroes2::AGG::GetICN( ICN::CELLWIN, icnIndex );

            rect = { x, y, playerIcon.width(), playerIcon.height() };

            fheroes2::Copy( playerIcon, 0, 0, output, rect.x, rect.y, rect.width, rect.height );

            checkmark.setPosition( rect.x + 2, rect.y + 2 );

            if ( checked ) {
                checkmark.show();
            }
            else {
                checkmark.hide();
            }
        }

        Checkbox( Checkbox && other ) = delete;
        ~Checkbox() = default;
        Checkbox( Checkbox & ) = delete;
        Checkbox & operator=( const Checkbox & ) = delete;

        const fheroes2::Rect & getRect() const
        {
            return rect;
        }

        int getColor() const
        {
            return color;
        }

        bool toggle( fheroes2::Display & output = fheroes2::Display::instance() )
        {
            checkmark.isHidden() ? checkmark.show() : checkmark.hide();

            output.render( rect );

            return !checkmark.isHidden();
        }

    private:
        int color = Color::NONE;
        fheroes2::Rect rect;
        fheroes2::MovableSprite checkmark{ fheroes2::AGG::GetICN( ICN::CELLWIN, 2 ) };
    };
}

namespace Editor
{
    bool eventDetailsDialog( Maps::Map_Format::AdventureMapEventMetadata & eventMetadata, const uint8_t humanPlayerColors, const uint8_t computerPlayerColors )
    {
        // First, make sure that the event has proper player colors according to the map specification.
        eventMetadata.humanPlayerColors = eventMetadata.humanPlayerColors & humanPlayerColors;
        eventMetadata.computerPlayerColors = eventMetadata.computerPlayerColors & computerPlayerColors;

        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        fheroes2::StandardWindow background( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT, true, display );
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

        text.set( eventMetadata.message, fheroes2::FontType::normalWhite() );
        text.draw( messageRoi.x + 5, messageRoi.y + 5, messageRoi.width - 10, display );

        auto createColorCheckboxes = [&display]( std::vector<std::unique_ptr<Checkbox>> & list, const int32_t availableColors, const int32_t selectedColors,
                                                 const int32_t boxOffsetX, const int32_t boxOffsetY ) {
            int32_t colorsAdded = 0;

            for ( const int color : Colors( availableColors ) ) {
                list.emplace_back( std::make_unique<Checkbox>( boxOffsetX + colorsAdded * 32, boxOffsetY, color, ( color & selectedColors ) != 0, display ) );
                ++colorsAdded;
            }
        };

        offsetY += 32;

        const fheroes2::Rect playerRoi{ dialogRoi.x + elementOffset + messageRoi.width + elementOffset, offsetY + text.height(), playerArea.width, playerArea.height };
        text.set( _( "Player colors allowed to get event:" ), fheroes2::FontType::normalWhite() );
        text.draw( playerRoi.x + ( playerRoi.width - text.width() ) / 2, offsetY, display );

        const int32_t availablePlayersCount = Color::Count( humanPlayerColors | computerPlayerColors );
        const int32_t checkOffX = ( playerRoi.width - availablePlayersCount * 32 ) / 2;

        std::vector<std::unique_ptr<Checkbox>> humanCheckboxes;
        createColorCheckboxes( humanCheckboxes, humanPlayerColors, eventMetadata.humanPlayerColors, playerRoi.x + checkOffX, offsetY + 32 );

        assert( humanCheckboxes.size() == static_cast<size_t>( Color::Count( humanPlayerColors ) ) );

        offsetY += 64;

        const fheroes2::Rect computersRoi{ dialogRoi.x + elementOffset + messageRoi.width + elementOffset, offsetY + text.height(), playerArea.width, playerArea.height };
        text.set( _( "Computer colors allowed to get event:" ), fheroes2::FontType::normalWhite() );
        text.draw( computersRoi.x + ( computersRoi.width - text.width() ) / 2, offsetY, display );

        std::vector<std::unique_ptr<Checkbox>> computerCheckboxes;
        createColorCheckboxes( computerCheckboxes, computerPlayerColors, eventMetadata.computerPlayerColors, computersRoi.x + checkOffX, offsetY + 32 );

        assert( computerCheckboxes.size() == static_cast<size_t>( Color::Count( computerPlayerColors ) ) );

        const fheroes2::Point recurringEventPos{ playerRoi.x, offsetY + 64 };

        fheroes2::MovableSprite recurringEventCheckbox;
        const fheroes2::Rect recurringEventArea
            = drawCheckboxWithText( recurringEventCheckbox, _( "Cancel event after first visit" ), display, recurringEventPos.x, recurringEventPos.y, isEvilInterface );
        if ( eventMetadata.isRecurringEvent ) {
            recurringEventCheckbox.show();
        }
        else {
            recurringEventCheckbox.hide();
        }

        // Bottom row
        offsetY = messageRoi.y + messageRoi.height + text.height() + elementOffset;

        text.set( _( "Reward:" ), fheroes2::FontType::normalWhite() );
        text.draw( dialogRoi.x + ( dialogRoi.width - text.width() ) / 2, offsetY, display );

        const fheroes2::Sprite & artifactFrame = fheroes2::AGG::GetICN( ICN::RESOURCE, 7 );
        const fheroes2::Rect artifactRoi{ messageRoi.x + ( messageRoi.width - artifactFrame.width() ) / 2, offsetY + text.height(), artifactFrame.width(),
                                          artifactFrame.height() };

        fheroes2::Blit( artifactFrame, display, artifactRoi.x, artifactRoi.y );

        auto redrawArtifactImage = [&display, &artifactRoi]( const int32_t artifactId ) {
            const fheroes2::Sprite & artifactImage = fheroes2::AGG::GetICN( ICN::ARTIFACT, Artifact( artifactId ).IndexSprite64() );
            fheroes2::Copy( artifactImage, 0, 0, display, artifactRoi.x + 6, artifactRoi.y + 6, artifactImage.width(), artifactImage.height() );
        };

        redrawArtifactImage( eventMetadata.artifact );

        const fheroes2::Sprite & buttonImage = fheroes2::AGG::GetICN( ICN::CELLWIN, 17 );
        const int32_t buttonWidth = buttonImage.width();

        fheroes2::Button buttonDeleteArtifact( artifactRoi.x + ( artifactRoi.width - buttonWidth ) / 2, artifactRoi.y + artifactRoi.height + 5, ICN::CELLWIN, 17, 18 );
        buttonDeleteArtifact.draw();

        // Resources
        const fheroes2::Rect resourceRoi{ playerRoi.x, offsetY + text.height(), playerRoi.width, 99 };
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
            buttonOk.drawOnState( le.MousePressLeft( buttonOk.area() ) );
            buttonCancel.drawOnState( le.MousePressLeft( buttonCancel.area() ) );
            buttonDeleteArtifact.drawOnState( le.MousePressLeft( buttonDeleteArtifact.area() ) );

            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return false;
            }

            if ( buttonOk.isEnabled() && ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) ) {
                break;
            }

            for ( const auto & humanCheckbox : humanCheckboxes ) {
                if ( le.MouseClickLeft( humanCheckbox->getRect() ) ) {
                    const int color = humanCheckbox->getColor();
                    if ( humanCheckbox->toggle( display ) ) {
                        eventMetadata.humanPlayerColors |= color;
                    }
                    else {
                        eventMetadata.humanPlayerColors ^= color;
                    }

                    break;
                }
            }

            for ( const auto & computerCheckbox : computerCheckboxes ) {
                if ( le.MouseClickLeft( computerCheckbox->getRect() ) ) {
                    const int color = computerCheckbox->getColor();
                    if ( computerCheckbox->toggle( display ) ) {
                        eventMetadata.computerPlayerColors |= color;
                    }
                    else {
                        eventMetadata.computerPlayerColors ^= color;
                    }

                    break;
                }
            }

            for ( size_t i = 0; i < individualResourceRoi.size(); ++i ) {
                if ( le.MouseClickLeft( individualResourceRoi[i] ) ) {
                    const int resourceType = resourceTypes[i];
                    int32_t * resourcePtr = eventMetadata.resources.GetPtr( resourceType );
                    assert( resourcePtr != nullptr );

                    uint32_t temp = *resourcePtr;

                    if ( Dialog::SelectCount( Resource::String( resourceType ), 0, 1000000, temp, 1 ) ) {
                        *resourcePtr = static_cast<int32_t>( temp );
                    }

                    resourceRoiRestorer.restore();

                    renderResources( eventMetadata.resources, resourceRoi, display, individualResourceRoi );
                    display.render( resourceRoi );
                    break;
                }
            }

            if ( le.MouseClickLeft( messageRoi ) ) {
                std::string temp = eventMetadata.message;

                if ( Dialog::inputString( _( "Message:" ), temp, {}, 200, true ) ) {
                    eventMetadata.message = std::move( temp );

                    messageRoiRestorer.restore();
                    text.set( eventMetadata.message, fheroes2::FontType::normalWhite() );
                    text.draw( messageRoi.x + 5, messageRoi.y + 5, messageRoi.width - 10, display );
                    isRedrawNeeded = true;
                }
            }
            else if ( le.MouseClickLeft( recurringEventArea ) ) {
                eventMetadata.isRecurringEvent = !eventMetadata.isRecurringEvent;
                eventMetadata.isRecurringEvent ? recurringEventCheckbox.show() : recurringEventCheckbox.hide();
                display.render( recurringEventCheckbox.getArea() );
            }
            else if ( le.MouseClickLeft( artifactRoi ) ) {
                const Artifact artifact = Dialog::selectArtifact( eventMetadata.artifact );
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
            if ( le.MousePressRight( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to save the Event properties." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( artifactRoi ) ) {
                // Since Artifact class does not allow to set a random spell (for obvious reasons),
                // we have to use special UI code to render the popup window with all needed information.
                const Artifact artifact( eventMetadata.artifact );

                if ( artifact.isValid() ) {
                    const fheroes2::Text header( artifact.GetName(), fheroes2::FontType::normalYellow() );
                    const fheroes2::Text description( fheroes2::getArtifactData( eventMetadata.artifact ).getDescription( eventMetadata.artifactMetadata ),
                                                      fheroes2::FontType::normalWhite() );

                    fheroes2::ArtifactDialogElement artifactUI( artifact );
                    fheroes2::showMessage( header, description, Dialog::ZERO, { &artifactUI } );
                }
                else {
                    fheroes2::showStandardTextMessage( _( "Artifact" ), _( "No artifact will be given as a reward." ), Dialog::ZERO );
                }
            }
            else if ( le.MousePressRight( buttonDeleteArtifact.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Delete Artifact" ), _( "Delete an artifact from the reward." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( resourceRoi ) ) {
                if ( eventMetadata.resources.GetValidItemsCount() == 0 ) {
                    fheroes2::showStandardTextMessage( _( "Resources" ), _( "No resources will be given as a reward." ), Dialog::ZERO );
                }
                else {
                    fheroes2::showResourceMessage( fheroes2::Text( _( "Resources" ), fheroes2::FontType::normalYellow() ),
                                                   fheroes2::Text{ _( "Resources will be given as a reward." ), fheroes2::FontType::normalWhite() }, Dialog::ZERO,
                                                   eventMetadata.resources );
                }
            }

            if ( isRedrawNeeded ) {
                isRedrawNeeded = false;

                display.render( dialogRoi );
            }
        }

        return true;
    }
}
