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

#include "editor_secondary_skill_selection.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <utility>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "game_static.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "math_tools.h"
#include "pal.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    const int32_t skillRowOffsetY{ 90 };
    const int32_t skillItemWidth{ 90 };

    // Up to 5 skills can be displayed in a row.
    // Up to 5 rows can be displayed.
    // So far the number of skills of any level is much more than 25.
    class SecondarySkillContainerUI final
    {
    public:
        SecondarySkillContainerUI( fheroes2::Point offset, std::vector<std::pair<Skill::Secondary, bool>> & skills )
            : _skills( skills )
        {
            assert( !skills.empty() && skills.size() < 25 );

            // Figure out how many rows and columns we want to display.
            for ( size_t i = 1; i < 5; ++i ) {
                if ( i * i >= skills.size() ) {
                    _skillsPerRow = i;
                    break;
                }
            }

            offset.x += ( fheroes2::Display::DEFAULT_WIDTH - static_cast<int32_t>( _skillsPerRow ) * skillItemWidth ) / 2;
            offset.y += ( fheroes2::Display::DEFAULT_HEIGHT - 50 - skillRowOffsetY * static_cast<int32_t>( _skills.size() / _skillsPerRow ) ) / 2;

            // Calculate all areas where we are going to render spells.
            _skillRoi.reserve( _skills.size() );

            const fheroes2::Sprite & frameImage = fheroes2::AGG::GetICN( ICN::SECSKILL, 15 );

            const int32_t lastRowColumns = static_cast<int32_t>( _skills.size() % _skillsPerRow );
            const int32_t lastRowOffsetX = ( lastRowColumns > 0 ) ? ( static_cast<int32_t>( _skillsPerRow ) - lastRowColumns ) * skillItemWidth / 2 : 0;

            for ( size_t i = 0; i < _skills.size(); ++i ) {
                const int32_t rowId = static_cast<int32_t>( i / _skillsPerRow );
                const int32_t columnId = static_cast<int32_t>( i % _skillsPerRow );

                if ( rowId == static_cast<int32_t>( _skills.size() / _skillsPerRow ) ) {
                    // This is the last row.
                    _skillRoi.emplace_back( offset.x + columnId * skillItemWidth + lastRowOffsetX, offset.y + rowId * skillRowOffsetY, frameImage.width(),
                                            frameImage.height() );
                }
                else {
                    _skillRoi.emplace_back( offset.x + columnId * skillItemWidth, offset.y + rowId * skillRowOffsetY, frameImage.width(), frameImage.height() );
                }
            }
        }

        void draw( fheroes2::Image & output )
        {
            const fheroes2::Sprite & frameImage = fheroes2::AGG::GetICN( ICN::SECSKILL, 15 );

            fheroes2::Sprite inactiveFrameImage( frameImage );
            fheroes2::ApplyPalette( inactiveFrameImage, PAL::GetPalette( PAL::PaletteType::GRAY ) );

            for ( size_t i = 0; i < _skills.size(); ++i ) {
                if ( !_skills[i].second ) {
                    // The skill is being inactive.
                    fheroes2::Blit( inactiveFrameImage, output, _skillRoi[i].x, _skillRoi[i].y );
                }
                else {
                    fheroes2::Blit( frameImage, output, _skillRoi[i].x, _skillRoi[i].y );
                }

                const fheroes2::Sprite & skillImage = fheroes2::AGG::GetICN( ICN::SECSKILL, _skills[i].first.GetIndexSprite1() );

                if ( !_skills[i].second ) {
                    // The skill is being inactive.
                    fheroes2::Sprite inactiveSkillImage( skillImage );
                    fheroes2::ApplyPalette( inactiveSkillImage, PAL::GetPalette( PAL::PaletteType::GRAY ) );

                    fheroes2::Blit( inactiveSkillImage, output, _skillRoi[i].x + 3, _skillRoi[i].y + 3 );
                }
                else {
                    fheroes2::Blit( skillImage, output, _skillRoi[i].x + 3, _skillRoi[i].y + 3 );
                }

                fheroes2::Text text{ Skill::Secondary::String( _skills[i].first.Skill() ), fheroes2::FontType::smallWhite() };
                text.draw( _skillRoi[i].x + ( skillImage.width() - text.width() ) / 2, _skillRoi[i].y + 7, output );
                text.set( Skill::Level::String( _skills[i].first.Level() ), fheroes2::FontType::smallWhite() );
                text.draw( _skillRoi[i].x + ( skillImage.width() - text.width() ) / 2, _skillRoi[i].y + skillImage.height() - 10, output );
            }
        }

        bool processEvents( LocalEvent & eventProcessor )
        {
            const int32_t skillIndex = GetRectIndex( _skillRoi, eventProcessor.getMouseCursorPos() );
            if ( skillIndex < 0 ) {
                return false;
            }

            if ( eventProcessor.MouseClickLeft() ) {
                assert( static_cast<size_t>( skillIndex ) < _skillRoi.size() );

                _skills[skillIndex].second = !_skills[skillIndex].second;
                return true;
            }

            if ( eventProcessor.isMouseRightButtonPressed() ) {
                Heroes fakeHero;
                fheroes2::SecondarySkillDialogElement( _skills[skillIndex].first, fakeHero ).showPopup( Dialog::ZERO );
            }

            return false;
        }

    private:
        std::vector<std::pair<Skill::Secondary, bool>> & _skills;

        std::vector<fheroes2::Rect> _skillRoi;

        size_t _skillsPerRow{ 0 };
    };
}

namespace Editor
{
    bool openSecondarySkillSelectionWindow( std::string title, const int skillLevel, std::vector<int32_t> & selectedSkills )
    {
        if ( skillLevel < 1 || skillLevel > 3 ) {
            // What are you trying to achieve?!
            assert( 0 );
            return false;
        }

        const std::vector<int32_t> existingSkills = GameStatic::getSecondarySkillsForWitchsHut();

        // Create a container of active and disabled skills.
        std::vector<std::pair<Skill::Secondary, bool>> skills;
        skills.reserve( existingSkills.size() );

        bool isAnySkillEnabled = false;

        for ( const int skill : existingSkills ) {
            const bool isSelected = ( std::find( selectedSkills.begin(), selectedSkills.end(), skill ) != selectedSkills.end() );

            skills.emplace_back( Skill::Secondary( skill, skillLevel ), isSelected );

            if ( isSelected ) {
                isAnySkillEnabled = true;
            }
        }

        // If no skills are selected, select all of them.
        if ( !isAnySkillEnabled ) {
            for ( auto & [skill, isSelected] : skills ) {
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

        background.renderOkayCancelButtons( buttonOk, buttonCancel, isEvilInterface );

        fheroes2::ImageRestorer restorer( display, activeArea.x, activeArea.y, activeArea.width, activeArea.height );

        SecondarySkillContainerUI skillContainer( activeArea.getPosition(), skills );

        skillContainer.draw( display );

        display.render( background.totalArea() );

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            if ( buttonOk.isEnabled() ) {
                buttonOk.drawOnState( le.isMouseLeftButtonPressedInArea( buttonOk.area() ) );
            }

            buttonCancel.drawOnState( le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) );

            if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancel.area() ) ) {
                return false;
            }

            if ( buttonOk.isEnabled() && ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( buttonOk.area() ) ) ) {
                break;
            }

            if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to accept the changes made." ), Dialog::ZERO );
            }

            if ( skillContainer.processEvents( le ) ) {
                restorer.restore();

                skillContainer.draw( display );

                // Check if all skills are being disabled. If they are disable the OKAY button.
                bool areAllSkillsDisabled = true;
                for ( const auto & [skill, isSelected] : skills ) {
                    if ( isSelected ) {
                        areAllSkillsDisabled = false;
                        break;
                    }
                }

                if ( areAllSkillsDisabled ) {
                    buttonOk.disable();
                    buttonOk.draw();
                }
                else {
                    buttonOk.enable();
                    buttonOk.draw();
                }

                display.render( activeArea );
            }
        }

        selectedSkills.clear();

        for ( const auto & [skill, isSelected] : skills ) {
            if ( isSelected ) {
                selectedSkills.emplace_back( skill.first );
            }
        }

        // If all skills are selected, remove all skills from the selection since an empty container means the use of the default behavior of the game.
        if ( selectedSkills.size() == skills.size() ) {
            selectedSkills = {};
        }

        return true;
    }
}
