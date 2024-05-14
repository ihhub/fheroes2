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

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "artifact.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "map_format_info.h"
#include "math_base.h"
#include "mp2.h"
#include "pal.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    const std::vector<int> colorList = { Color::BLUE, Color::GREEN, Color::RED, Color::YELLOW, Color::ORANGE, Color::PURPLE };
    const std::array<int, 7> resourceTypes = { Resource::WOOD, Resource::SULFUR, Resource::CRYSTAL, Resource::MERCURY, Resource::ORE, Resource::GEMS, Resource::GOLD };

    const fheroes2::Size messageArea{ 300, 200 };
    const fheroes2::Size playerArea{ 300, 200 };
    const int32_t elementOffset{ 9 };

    void renderResources( const Funds & resources, const fheroes2::Rect & roi, fheroes2::Image & output, std::array<fheroes2::Rect, 7> & resourceRoi )
    {
        const int32_t offsetFromEdge{ 7 };

        // Maximum width is 39 pixels (except gold), maximum height is 32 pixels
        const int32_t maxWidth = 39;
        const int32_t maxHeight = 32;

        const int32_t midElementOffsetX = ( ( roi.width - 2 * offsetFromEdge ) - maxWidth * 4 ) / 3;

        const int32_t firstColumnOffset = roi.x + offsetFromEdge;
        const int32_t columnStep = maxWidth + midElementOffsetX;
        const int32_t secondColumnOffset = firstColumnOffset + columnStep;
        const int32_t thirdColumnOffset = secondColumnOffset + columnStep;
        const int32_t forthColumnOffset = thirdColumnOffset + columnStep;

        const fheroes2::FontType fontType( fheroes2::FontSize::SMALL, fheroes2::FontColor::WHITE );
        const int32_t fontHeight = fheroes2::Text( std::string(), fontType ).height();

        const std::array<int32_t, 2> offsetY = { roi.y + offsetFromEdge + maxHeight, roi.y + offsetFromEdge + 2 * maxHeight + fontHeight + 2 };

        const fheroes2::Sprite & woodImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 0 );
        const fheroes2::Sprite & mercuryImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 1 );
        const fheroes2::Sprite & oreImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 2 );
        const fheroes2::Sprite & sulfurImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 3 );
        const fheroes2::Sprite & crystalImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 4 );
        const fheroes2::Sprite & gemsImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 5 );
        const fheroes2::Sprite & goldImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 6 );

        resourceRoi[0] = { firstColumnOffset + ( maxWidth - woodImage.width() ) / 2, offsetY[0] - woodImage.height(), woodImage.width(), woodImage.height() };
        resourceRoi[1] = { secondColumnOffset + ( maxWidth - sulfurImage.width() ) / 2, offsetY[0] - sulfurImage.height(), sulfurImage.width(), sulfurImage.height() };
        resourceRoi[2] = { thirdColumnOffset + ( maxWidth - crystalImage.width() ) / 2, offsetY[0] - crystalImage.height(), crystalImage.width(), crystalImage.height() };
        resourceRoi[3] = { forthColumnOffset + ( maxWidth - mercuryImage.width() ) / 2, offsetY[0] - mercuryImage.height(), mercuryImage.width(), mercuryImage.height() };
        resourceRoi[4] = { firstColumnOffset + ( maxWidth - oreImage.width() ) / 2, offsetY[1] - oreImage.height(), oreImage.width(), oreImage.height() };
        resourceRoi[5] = { secondColumnOffset + ( maxWidth - gemsImage.width() ) / 2, offsetY[1] - gemsImage.height(), gemsImage.width(), gemsImage.height() };
        resourceRoi[6] = { thirdColumnOffset + ( maxWidth * 2 + midElementOffsetX - goldImage.width() ) / 2, offsetY[1] - goldImage.height(), goldImage.width(),
                           goldImage.height() };

        fheroes2::Blit( woodImage, output, resourceRoi[0].x, resourceRoi[0].y );
        fheroes2::Blit( sulfurImage, output, resourceRoi[1].x, resourceRoi[1].y );
        fheroes2::Blit( crystalImage, output, resourceRoi[2].x, resourceRoi[2].y );
        fheroes2::Blit( mercuryImage, output, resourceRoi[3].x, resourceRoi[3].y );
        fheroes2::Blit( oreImage, output, resourceRoi[4].x, resourceRoi[4].y );
        fheroes2::Blit( gemsImage, output, resourceRoi[5].x, resourceRoi[5].y );
        fheroes2::Blit( goldImage, output, resourceRoi[6].x, resourceRoi[6].y );

        fheroes2::Text text( std::to_string( resources.wood ), fontType );
        text.draw( firstColumnOffset + ( maxWidth - text.width() ) / 2, offsetY[0] + 1, output );

        text.set( std::to_string( resources.sulfur ), fontType );
        text.draw( secondColumnOffset + ( maxWidth - text.width() ) / 2, offsetY[0] + 1, output );

        text.set( std::to_string( resources.crystal ), fontType );
        text.draw( thirdColumnOffset + ( maxWidth - text.width() ) / 2, offsetY[0] + 1, output );

        text.set( std::to_string( resources.mercury ), fontType );
        text.draw( forthColumnOffset + ( maxWidth - text.width() ) / 2, offsetY[0] + 1, output );

        text.set( std::to_string( resources.ore ), fontType );
        text.draw( firstColumnOffset + ( maxWidth - text.width() ) / 2, offsetY[1] + 1, output );

        text.set( std::to_string( resources.gems ), fontType );
        text.draw( secondColumnOffset + ( maxWidth - text.width() ) / 2, offsetY[1] + 1, output );

        text.set( std::to_string( resources.gold ), fontType );
        text.draw( thirdColumnOffset + ( maxWidth * 2 + midElementOffsetX - text.width() ) / 2, offsetY[1] + 1, output );
    }

    class Checkbox
    {
    public:
        Checkbox( fheroes2::Display & display, const int32_t x, const int32_t y, const int color, const bool checked )
            : color( color )
            , checkmark( fheroes2::AGG::GetICN( ICN::CELLWIN, 2 ) )
        {
            rect.x = x;
            rect.y = y;

            const int32_t icnIndex = Color::GetIndex( color ) + 43;
            const fheroes2::Sprite & playerIcon = fheroes2::AGG::GetICN( ICN::CELLWIN, icnIndex );
            rect.width = playerIcon.width();
            rect.height = playerIcon.height();
            fheroes2::Copy( playerIcon, 0, 0, display, rect.x, rect.y, rect.width, rect.height );

            checkmark.setPosition( rect.x + 2, rect.y + 2 );

            if ( checked ) {
                checkmark.show();
            }
            else {
                checkmark.hide();
            }
        }

        Checkbox( Checkbox && other ) noexcept
            : color( other.color )
            , rect( other.rect )
            , checkmark( fheroes2::AGG::GetICN( ICN::CELLWIN, 2 ) )
        {
            checkmark.setPosition( rect.x + 2, rect.y + 2 );

            if ( other.checkmark.isHidden() ) {
                checkmark.hide();
            }
            else {
                checkmark.show();
            }
        }

        ~Checkbox() = default;
        Checkbox( Checkbox & ) = delete;
        Checkbox & operator=( const Checkbox & ) = delete;

        fheroes2::Rect & getRect()
        {
            return rect;
        }

        int getColor() const
        {
            return color;
        }

        bool toggle()
        {
            checkmark.isHidden() ? checkmark.show() : checkmark.hide();
            return !checkmark.isHidden();
        }

    private:
        int color;
        fheroes2::Rect rect;
        fheroes2::MovableSprite checkmark;
    };
}

namespace Editor
{
    bool eventDetailsDialog( Maps::Map_Format::AdventureMapEventMetadata & eventMetadata, const uint8_t availablePlayerColors )
    {
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

        // Player configuration
        const int32_t availablePlayersCount = Color::Count( availablePlayerColors );
        const Colors availableColors( availablePlayerColors );

        auto createColorCheckboxes
            = [&availablePlayersCount, &availablePlayerColors, &display]( std::vector<Checkbox> & list, int colors, int32_t offsetX, int32_t offsetY ) {
                  auto currentColorIt = colorList.begin();

                  for ( int32_t i = 0; i < availablePlayersCount; ++i ) {
                      while ( !( availablePlayerColors & *currentColorIt ) ) {
                          ++currentColorIt;
                      }
                      const int currentColor = *currentColorIt;
                      list.emplace_back( display, offsetX + i * 32, offsetY, currentColor, colors & currentColor );
                      ++currentColorIt;
                  }
              };

        offsetY += 32;

        const fheroes2::Rect playerRoi{ dialogRoi.x + elementOffset + messageRoi.width + elementOffset, offsetY + text.height(), playerArea.width, playerArea.height };
        text.set( _( "Player colors allowed to get event:" ), fheroes2::FontType::normalWhite() );
        text.draw( playerRoi.x + ( playerRoi.width - text.width() ) / 2, offsetY, display );

        std::vector<Checkbox> playerCheckboxes;

        int checkOff = ( playerRoi.width - availablePlayersCount * 32 ) / 2;

        createColorCheckboxes( playerCheckboxes, eventMetadata.humanPlayerColors, playerRoi.x + checkOff, offsetY + 32 );

        assert( playerCheckboxes.size() == static_cast<size_t>( availablePlayersCount ) );

        offsetY += 64;

        const fheroes2::Rect computersRoi{ dialogRoi.x + elementOffset + messageRoi.width + elementOffset, offsetY + text.height(), playerArea.width, playerArea.height };
        text.set( _( "Computer colors allowed to get event:" ), fheroes2::FontType::normalWhite() );
        text.draw( computersRoi.x + ( computersRoi.width - text.width() ) / 2, offsetY, display );

        std::vector<Checkbox> computerCheckboxes;

        createColorCheckboxes( computerCheckboxes, eventMetadata.computerPlayerColors, computersRoi.x + checkOff, offsetY + 32 );

        assert( playerCheckboxes.size() == computerCheckboxes.size() );

        // Recurring event checkbox
        auto drawCheckboxBackground
            = [&display, &dialogRoi]( fheroes2::MovableSprite & checkSprite, std::string str, const int32_t posX, const int32_t posY, const bool isEvil ) {
                  const fheroes2::Sprite & checkboxBackground = fheroes2::AGG::GetICN( ICN::CELLWIN, 1 );
                  if ( isEvil ) {
                      fheroes2::ApplyPalette( checkboxBackground, 0, 0, display, posX, posY, checkboxBackground.width(), checkboxBackground.height(),
                                              PAL::CombinePalettes( PAL::GetPalette( PAL::PaletteType::GRAY ), PAL::GetPalette( PAL::PaletteType::DARKENING ) ) );
                  }
                  else {
                      fheroes2::Copy( checkboxBackground, 0, 0, display, posX, posY, checkboxBackground.width(), checkboxBackground.height() );
                  }

                  fheroes2::addGradientShadow( checkboxBackground, display, { posX, posY }, { -4, 4 } );
                  const fheroes2::Text checkboxText( std::move( str ), fheroes2::FontType::normalWhite() );
                  checkboxText.drawInRoi( posX + 23, posY + 4, display, dialogRoi );

                  checkSprite = fheroes2::AGG::GetICN( ICN::CELLWIN, 2 );
                  checkSprite.setPosition( posX + 2, posY + 2 );

                  return fheroes2::Rect( posX, posY, 23 + checkboxText.width(), checkboxBackground.height() );
              };

        fheroes2::Point dstPt( playerRoi.x, offsetY + 64 );

        fheroes2::MovableSprite recurringEventCheckbox;
        fheroes2::Rect recurringEventArea = drawCheckboxBackground( recurringEventCheckbox, _( "Cancel event after first visit" ), dstPt.x, dstPt.y, isEvilInterface );
        if ( eventMetadata.isRecurringEvent ) {
            recurringEventCheckbox.show();
        }
        else {
            recurringEventCheckbox.hide();
        }

        // Bottom row
        offsetY = messageRoi.y + messageRoi.height + text.height() + elementOffset;

        const fheroes2::Sprite & buttonImage = fheroes2::AGG::GetICN( ICN::CELLWIN, 13 );
        const int32_t buttonWidth = buttonImage.width();

        text.set( _( "Reward:" ), fheroes2::FontType::normalWhite() );
        text.draw( dialogRoi.x + ( dialogRoi.width - text.width() ) / 2, offsetY, display );

        const fheroes2::Sprite & artifactFrame = fheroes2::AGG::GetICN( ICN::RESOURCE, 7 );
        const fheroes2::Rect artifactRoi{ messageRoi.x + ( messageRoi.width - artifactFrame.width() ) / 2, offsetY + text.height(), artifactFrame.width(),
                                          artifactFrame.height() };

        fheroes2::Blit( artifactFrame, display, artifactRoi.x, artifactRoi.y );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::ARTIFACT, Artifact( eventMetadata.artifact ).IndexSprite64() ), display, artifactRoi.x + 6, artifactRoi.y + 6 );

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

            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return false;
            }

            if ( buttonOk.isEnabled() && ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) ) {
                break;
            }

            for ( int32_t i = 0; i < availablePlayersCount; ++i ) {
                if ( !( availablePlayerColors & availableColors[i] ) ) {
                    break;
                }

                if ( le.MouseClickLeft( playerCheckboxes[i].getRect() ) ) {
                    const int currentColor = playerCheckboxes[i].getColor();
                    if ( playerCheckboxes[i].toggle() ) {
                        eventMetadata.humanPlayerColors |= currentColor;
                    }
                    else {
                        eventMetadata.humanPlayerColors ^= currentColor;
                    }

                    break;
                }
                if ( le.MouseClickLeft( computerCheckboxes[i].getRect() ) ) {
                    const int currentColor = computerCheckboxes[i].getColor();
                    if ( computerCheckboxes[i].toggle() ) {
                        eventMetadata.computerPlayerColors |= currentColor;
                    }
                    else {
                        eventMetadata.computerPlayerColors ^= currentColor;
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

                    const fheroes2::Sprite & artifactImage = fheroes2::AGG::GetICN( ICN::ARTIFACT, Artifact( eventMetadata.artifact ).IndexSprite64() );
                    fheroes2::Copy( artifactImage, 0, 0, display, artifactRoi.x + 6, artifactRoi.y + 6, artifactImage.width(), artifactImage.height() );
                }

                // The opened selectArtifact() dialog might be bigger than the Sphinx dialog so we render the whole screen.
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

            if ( isRedrawNeeded ) {
                isRedrawNeeded = false;

                display.render( dialogRoi );
            }
        }

        return true;
    }
}
