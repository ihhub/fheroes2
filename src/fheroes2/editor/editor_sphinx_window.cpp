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

#include "editor_sphinx_window.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "artifact.h"
#include "artifact_info.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "editor_ui_helper.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
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
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    const fheroes2::Size riddleArea{ 250, 180 };

    const fheroes2::Size answerArea{ 250, 155 };

    const int32_t elementOffset{ 9 };

    const int32_t listAreaHeightDeduction{ 8 };

    const size_t longestAnswer{ 64 };

    // TODO: expand the riddle area to support more characters.
    //       At the moment only up to 140 of the biggest characters can be added.
    const size_t longestRiddle{ 140 };

    const std::array<int, 7> resourceTypes = { Resource::WOOD, Resource::SULFUR, Resource::CRYSTAL, Resource::MERCURY, Resource::ORE, Resource::GEMS, Resource::GOLD };

    class AnswerListBox : public Interface::ListBox<std::string>
    {
    public:
        using Interface::ListBox<std::string>::ActionListDoubleClick;
        using Interface::ListBox<std::string>::ActionListSingleClick;
        using Interface::ListBox<std::string>::ActionListPressRight;

        AnswerListBox( const fheroes2::Point & pt, const fheroes2::SupportedLanguage language )
            : ListBox( pt )
            , _language( language )
        {
            // Do nothing.
        }

        void RedrawItem( const std::string & answer, int32_t posX, int32_t posY, bool current ) override
        {
            fheroes2::Text text{ answer, ( current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() ), _language };
            text.fitToOneRow( answerArea.width - 10 );
            text.draw( posX + 5, posY + 5, fheroes2::Display::instance() );
        }

        void RedrawBackground( const fheroes2::Point & /*unused*/ ) override
        {
            _listBackground->restore();
        }

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( std::string & /*unused*/ ) override
        {
            _isDoubleClicked = true;
        }

        bool isDoubleClicked() const
        {
            return _isDoubleClicked;
        }

        void resetDoubleClickedState()
        {
            _isDoubleClicked = false;
        }

        void ActionListSingleClick( std::string & /*unused*/ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( std::string & /*unused*/ ) override
        {
            // Do nothing.
        }

        int getCurrentId() const
        {
            return _currentId;
        }

        void initListBackgroundRestorer( fheroes2::Rect roi )
        {
            _listBackground = std::make_unique<fheroes2::ImageRestorer>( fheroes2::Display::instance(), roi.x, roi.y, roi.width, roi.height );
        }

        void updateScrollBarImage()
        {
            const int32_t scrollBarWidth = _scrollbar.width();

            setScrollBarImage( fheroes2::generateScrollbarSlider( _scrollbar, false, _scrollbar.getArea().height, VisibleItemCount(), _size(),
                                                                  { 0, 0, scrollBarWidth, 8 }, { 0, 7, scrollBarWidth, 8 } ) );
            _scrollbar.moveToIndex( _topId );
        }

    private:
        std::unique_ptr<fheroes2::ImageRestorer> _listBackground;

        bool _isDoubleClicked{ false };

        const fheroes2::SupportedLanguage _language;
    };
}

namespace Editor
{
    bool openSphinxWindow( Maps::Map_Format::SphinxMetadata & metadata, const fheroes2::SupportedLanguage language )
    {
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::StandardWindow background{ riddleArea.width + answerArea.width + 50, riddleArea.height + 220, true, display };

        const fheroes2::Rect windowArea{ background.activeArea() };

        int32_t offsetY = windowArea.y + elementOffset;

        const fheroes2::Text title( MP2::StringObject( MP2::OBJ_SPHINX ), fheroes2::FontType::normalYellow() );
        title.draw( windowArea.x + ( windowArea.width - title.width() ) / 2, offsetY, display );

        offsetY += title.height() + elementOffset;

        fheroes2::Text text{ _( "Riddle:" ), fheroes2::FontType::normalWhite() };

        const fheroes2::Rect riddleRoi{ windowArea.x + elementOffset, offsetY + text.height(), riddleArea.width, riddleArea.height };
        background.applyTextBackgroundShading( riddleRoi );

        fheroes2::ImageRestorer riddleRoiRestorer( display, riddleRoi.x, riddleRoi.y, riddleRoi.width, riddleRoi.height );

        text.draw( riddleRoi.x + ( riddleRoi.width - text.width() ) / 2, offsetY, display );

        text.set( metadata.riddle, fheroes2::FontType::normalWhite(), language );
        text.draw( riddleRoi.x + 5, riddleRoi.y + 5, riddleRoi.width - 10, display );

        const fheroes2::Rect answerRoi{ windowArea.x + elementOffset + riddleRoi.width + elementOffset, offsetY + text.height(), answerArea.width, answerArea.height };
        background.applyTextBackgroundShading( answerRoi );

        text.set( _( "Answers:" ), fheroes2::FontType::normalWhite() );
        text.draw( answerRoi.x + ( answerRoi.width - text.width() ) / 2, offsetY, display );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        AnswerListBox answerList( answerRoi.getPosition(), language );
        answerList.initListBackgroundRestorer( answerRoi );

        answerList.SetAreaItems( { answerRoi.x, answerRoi.y, answerRoi.width, answerRoi.height - listAreaHeightDeduction } );

        int32_t scrollbarOffsetX = answerRoi.x + answerRoi.width + 5;
        background.renderScrollbarBackground( { scrollbarOffsetX, answerRoi.y, answerRoi.width, answerRoi.height }, isEvilInterface );

        const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;
        const int32_t topPartHeight = 19;
        ++scrollbarOffsetX;

        answerList.SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX, answerRoi.y + 1 } );
        answerList.SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX, answerRoi.y + answerRoi.height - 15 } );
        answerList.setScrollBarArea( { scrollbarOffsetX + 2, answerRoi.y + topPartHeight, 10, answerRoi.height - 2 * topPartHeight } );
        answerList.setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );
        answerList.SetAreaMaxItems( 10 );
        answerList.SetListContent( metadata.answers );
        answerList.updateScrollBarImage();

        answerList.Redraw();

        const int minibuttonIcnId = isEvilInterface ? ICN::CELLWIN_EVIL : ICN::CELLWIN;

        const fheroes2::Sprite & buttonImage = fheroes2::AGG::GetICN( minibuttonIcnId, 13 );
        const int32_t buttonWidth = buttonImage.width();
        const int32_t buttonOffset = ( answerArea.width - 3 * buttonWidth ) / 2 + buttonWidth;

        fheroes2::Button buttonAdd( answerRoi.x, answerRoi.y + answerRoi.height + 5, minibuttonIcnId, 13, 14 );
        buttonAdd.draw();

        fheroes2::Button buttonEdit( answerRoi.x + buttonOffset, answerRoi.y + answerRoi.height + 5, minibuttonIcnId, 15, 16 );
        buttonEdit.draw();

        fheroes2::Button buttonDelete( answerRoi.x + answerArea.width - buttonWidth, answerRoi.y + answerRoi.height + 5, minibuttonIcnId, 17, 18 );
        buttonDelete.draw();

        offsetY += text.height() + riddleArea.height + elementOffset;

        text.set( _( "Reward:" ), fheroes2::FontType::normalWhite() );
        text.draw( windowArea.x + ( windowArea.width - text.width() ) / 2, offsetY, display );

        const fheroes2::Sprite & artifactFrame = fheroes2::AGG::GetICN( ICN::RESOURCE, 7 );
        const fheroes2::Rect artifactRoi{ riddleRoi.x + ( riddleRoi.width - artifactFrame.width() ) / 2, offsetY + text.height(), artifactFrame.width(),
                                          artifactFrame.height() };

        fheroes2::Blit( artifactFrame, display, artifactRoi.x, artifactRoi.y );

        auto redrawArtifactImage = [&display, &artifactRoi]( const int32_t artifactId ) {
            const fheroes2::Sprite & artifactImage = fheroes2::AGG::GetICN( ICN::ARTIFACT, Artifact( artifactId ).IndexSprite64() );
            fheroes2::Copy( artifactImage, 0, 0, display, artifactRoi.x + 6, artifactRoi.y + 6, artifactImage.width(), artifactImage.height() );
        };

        redrawArtifactImage( metadata.artifact );

        fheroes2::Button buttonDeleteArtifact( artifactRoi.x + ( artifactRoi.width - buttonWidth ) / 2, artifactRoi.y + artifactRoi.height + 5, minibuttonIcnId, 17, 18 );
        buttonDeleteArtifact.draw();

        const fheroes2::Rect resourceRoi{ answerRoi.x, offsetY + text.height(), answerRoi.width, 99 };
        background.applyTextBackgroundShading( resourceRoi );

        fheroes2::ImageRestorer resourceRoiRestorer( display, resourceRoi.x, resourceRoi.y, resourceRoi.width, resourceRoi.height );

        std::array<fheroes2::Rect, 7> individualResourceRoi;
        renderResources( metadata.resources, resourceRoi, display, individualResourceRoi );

        // Prepare OKAY and CANCEL buttons and render their shadows.
        fheroes2::Button buttonOk;
        fheroes2::Button buttonCancel;

        background.renderOkayCancelButtons( buttonOk, buttonCancel, isEvilInterface );

        display.render( background.totalArea() );

        bool isRedrawNeeded = false;

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedInArea( buttonOk.area() ) );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) );
            buttonAdd.drawOnState( le.isMouseLeftButtonPressedInArea( buttonAdd.area() ) );
            buttonEdit.drawOnState( le.isMouseLeftButtonPressedInArea( buttonEdit.area() ) );
            buttonDelete.drawOnState( le.isMouseLeftButtonPressedInArea( buttonDelete.area() ) );
            buttonDeleteArtifact.drawOnState( le.isMouseLeftButtonPressedInArea( buttonDeleteArtifact.area() ) );

            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                break;
            }

            if ( buttonOk.isEnabled() && ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) ) {
                return true;
            }

            answerList.QueueEventProcessing();

            if ( answerList.IsNeedRedraw() ) {
                answerList.Redraw();
                isRedrawNeeded = true;
            }

            for ( size_t i = 0; i < individualResourceRoi.size(); ++i ) {
                if ( le.MouseClickLeft( individualResourceRoi[i] ) ) {
                    const int resourceType = resourceTypes[i];
                    int32_t * resourcePtr = metadata.resources.GetPtr( resourceType );
                    assert( resourcePtr != nullptr );

                    int32_t temp = *resourcePtr;

                    if ( Dialog::SelectCount( Resource::String( resourceType ), 0, 1000000, temp, 1 ) ) {
                        *resourcePtr = temp;
                    }

                    resourceRoiRestorer.restore();

                    renderResources( metadata.resources, resourceRoi, display, individualResourceRoi );
                    display.render( resourceRoi );
                    break;
                }
            }

            if ( le.MouseClickLeft( riddleRoi ) ) {
                std::string temp = metadata.riddle;

                const fheroes2::Text body{ _( "Riddle:" ), fheroes2::FontType::normalWhite() };
                if ( Dialog::inputString( fheroes2::Text{}, body, temp, longestRiddle, true, language ) ) {
                    metadata.riddle = std::move( temp );

                    riddleRoiRestorer.restore();
                    text.set( metadata.riddle, fheroes2::FontType::normalWhite(), language );
                    text.draw( riddleRoi.x + 5, riddleRoi.y + 5, riddleRoi.width - 10, display );
                    isRedrawNeeded = true;
                }
            }
            else if ( le.MouseClickLeft( buttonAdd.area() ) ) {
                std::string newAnswer;
                const fheroes2::Text body{ _( "Answer:" ), fheroes2::FontType::normalWhite() };
                if ( Dialog::inputString( fheroes2::Text{}, body, newAnswer, longestAnswer, false, language ) ) {
                    if ( std::any_of( metadata.answers.begin(), metadata.answers.end(), [&newAnswer]( const auto & answer ) { return answer == newAnswer; } ) ) {
                        fheroes2::showStandardTextMessage( _( "Answer" ), _( "This answer exists in the list." ), Dialog::OK );
                        continue;
                    }

                    metadata.answers.emplace_back( std::move( newAnswer ) );

                    answerList.updateScrollBarImage();
                    answerList.Redraw();
                    isRedrawNeeded = true;
                }
            }
            else if ( answerList.isDoubleClicked() || le.MouseClickLeft( buttonEdit.area() ) ) {
                if ( answerList.getCurrentId() < 0 ) {
                    continue;
                }

                answerList.resetDoubleClickedState();

                std::string temp = answerList.GetCurrent();

                const fheroes2::Text body{ _( "Answer:" ), fheroes2::FontType::normalWhite() };
                if ( Dialog::inputString( fheroes2::Text{}, body, temp, longestAnswer, false, language ) ) {
                    const auto count = std::count_if( metadata.answers.begin(), metadata.answers.end(), [&temp]( const auto & answer ) { return answer == temp; } );
                    if ( answerList.GetCurrent() != temp && count > 0 ) {
                        fheroes2::showStandardTextMessage( _( "Answer" ), _( "This answer exists in the list." ), Dialog::OK );
                        continue;
                    }

                    answerList.GetCurrent() = std::move( temp );

                    answerList.Redraw();
                    isRedrawNeeded = true;
                }
            }
            else if ( le.MouseClickLeft( buttonDelete.area() ) ) {
                if ( answerList.getCurrentId() < 0 ) {
                    continue;
                }

                answerList.RemoveSelected();
                answerList.updateScrollBarImage();
                answerList.Redraw();
                isRedrawNeeded = true;
            }
            else if ( le.MouseClickLeft( artifactRoi ) ) {
                const Artifact artifact = Dialog::selectArtifact( metadata.artifact, false );
                if ( artifact.isValid() ) {
                    int32_t artifactMetadata = metadata.artifactMetadata;

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

                    metadata.artifact = artifact.GetID();
                    metadata.artifactMetadata = artifactMetadata;

                    redrawArtifactImage( metadata.artifact );
                }

                // The opened selectArtifact() dialog might be bigger than the Sphinx dialog so we render the whole screen.
                display.render();

                isRedrawNeeded = false;
            }
            else if ( le.MouseClickLeft( buttonDeleteArtifact.area() ) ) {
                metadata.artifact = 0;
                metadata.artifactMetadata = 0;

                const fheroes2::Sprite & artifactImage = fheroes2::AGG::GetICN( ICN::ARTIFACT, Artifact( metadata.artifact ).IndexSprite64() );
                fheroes2::Copy( artifactImage, 0, 0, display, artifactRoi.x + 6, artifactRoi.y + 6, artifactImage.width(), artifactImage.height() );

                display.render( artifactRoi );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to save the Sphinx properties." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonAdd.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Add Answer" ), _( "Add an additional answer for the question." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonEdit.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Edit Answer" ), _( "Edit an existing answer for the question." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonDelete.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Delete Answer" ), _( "Delete an existing answer for the question." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( artifactRoi ) ) {
                // Since Artifact class does not allow to set a random spell (for obvious reasons),
                // we have to use special UI code to render the popup window with all needed information.
                const Artifact artifact( metadata.artifact );

                if ( artifact.isValid() ) {
                    fheroes2::ArtifactDialogElement artifactUI( artifact );

                    fheroes2::showStandardTextMessage( artifact.GetName(), fheroes2::getArtifactData( metadata.artifact ).getDescription( metadata.artifactMetadata ),
                                                       Dialog::ZERO, { &artifactUI } );
                }
                else {
                    fheroes2::showStandardTextMessage( _( "Artifact" ), _( "No artifact will be given as a reward." ), Dialog::ZERO );
                }
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonDeleteArtifact.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Delete Artifact" ), _( "Delete an artifact from the reward." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( resourceRoi ) ) {
                if ( metadata.resources.GetValidItemsCount() == 0 ) {
                    fheroes2::showStandardTextMessage( _( "Resources" ), _( "No resources will be given as a reward." ), Dialog::ZERO );
                }
                else {
                    fheroes2::showResourceMessage( fheroes2::Text( _( "Resources" ), fheroes2::FontType::normalYellow() ),
                                                   fheroes2::Text{ _( "Resources will be given as a reward." ), fheroes2::FontType::normalWhite() }, Dialog::ZERO,
                                                   metadata.resources );
                }
            }

            if ( isRedrawNeeded ) {
                isRedrawNeeded = false;

                display.render( windowArea );
            }
        }

        return false;
    }
}
