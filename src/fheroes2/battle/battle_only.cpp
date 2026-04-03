/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2026                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "battle_only.h"

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "army_bar.h"
#include "army_troop.h"
#include "battle.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game_hotkeys.h"
#include "heroes.h"
#include "heroes_base.h"
#include "heroes_indicator.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "monster.h"
#include "pal.h"
#include "race.h"
#include "rand.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "skill_bar.h"
#include "spell_book.h"
#include "til.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    const std::array<PlayerColor, 2> playerColor{ PlayerColor::BLUE, PlayerColor::RED };
    const std::array<int32_t, 2> moraleAndLuckOffsetX{ 34, 571 };
    const std::array<int32_t, 2> armyOffsetX{ 36, 381 };

    constexpr fheroes2::Size terrainIconSize{ 32, 32 };

    const std::array<fheroes2::Rect, 2> primarySkillArea{ fheroes2::Rect{ 216, 51, 34, 133 }, fheroes2::Rect{ 389, 51, 34, 133 } };
    const std::array<fheroes2::Rect, 2> secondarySkillArea{ fheroes2::Rect{ 22, 199, 265, 34 }, fheroes2::Rect{ 353, 199, 265, 34 } };
    const std::array<fheroes2::Rect, 2> artifactArea{ fheroes2::Rect{ 23, 347, 250, 70 }, fheroes2::Rect{ 367, 347, 250, 70 } };

    const Troop defaultMonster{ Monster::PEASANT, 100 };

    const fheroes2::Rect primarySkillTextRoi{ 262, 61, 115, 109 };
    const fheroes2::Rect titleTextRoi{ 72, 26, 496, 19 };

    constexpr fheroes2::Rect defendingHeroTypeCheckboxArea{ 616, 422, 19, 19 };
    constexpr int32_t defendingHeroTypeNameMaxWidth{ 342 };

    const std::array<fheroes2::Rect, 2> heroPortraitArea{ fheroes2::Rect{ 88, 66, 111, 105 }, fheroes2::Rect{ 440, 66, 111, 105 } };
    const std::array<fheroes2::Rect, 2> heroPortraitInternalArea{ fheroes2::Rect{ 93, 72, 101, 93 }, fheroes2::Rect{ 445, 72, 101, 93 } };

    const fheroes2::Point shadowOffset{ -5, 5 };

    void prepareBackround( fheroes2::Image & output, const fheroes2::Point offset, const bool isEvilInterface )
    {
        const auto & heroMeetingImage = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );

        // Render hero portrait area.
        for ( const fheroes2::Rect & area : heroPortraitArea ) {
            fheroes2::Blit( heroMeetingImage, area.x, area.y, output, area.x + offset.x, area.y + offset.y, area.width, area.height );
        }

        // Render dialog's title.
        fheroes2::Blit( heroMeetingImage, titleTextRoi.x, titleTextRoi.y, output, titleTextRoi.x + offset.x, titleTextRoi.y + offset.y, titleTextRoi.width,
                        titleTextRoi.height );
        if ( isEvilInterface ) {
            fheroes2::ApplyPalette( output, titleTextRoi.x + offset.x, titleTextRoi.y + offset.y, output, titleTextRoi.x + offset.x, titleTextRoi.y + offset.y,
                                    titleTextRoi.width, titleTextRoi.height, PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );
        }

        // Shadow generation requires an input image. We know that the image has no transparent pixels.
        // So, instead of cropping an image we just create 'fake' black images because it's faster to do.
        fheroes2::Sprite tempImage( heroPortraitArea[0].width, heroPortraitArea[0].height );
        tempImage.fill( 0 );

        for ( const fheroes2::Rect & area : heroPortraitArea ) {
            fheroes2::addGradientShadow( tempImage, output, { area.x + offset.x, area.y + offset.y }, shadowOffset );
        }

        tempImage.resize( titleTextRoi.width, titleTextRoi.height );
        tempImage.fill( 0 );

        fheroes2::addGradientShadow( tempImage, output, { titleTextRoi.x + offset.x, titleTextRoi.y + offset.y }, shadowOffset );
    }

    int32_t getMaxDefendingHeroAreaWidth()
    {
        const fheroes2::Sprite & cell = fheroes2::AGG::GetICN( ICN::CELLWIN, 1 );
        fheroes2::Text text( _( "Human" ), fheroes2::FontType::smallWhite() );
        text.fitToOneRow( defendingHeroTypeNameMaxWidth );

        return cell.width() + 5 + text.width();
    }

    void renderDefendingHeroTypeUI( const int heroType, const fheroes2::Point offset, const bool isEvilInterface, fheroes2::Image & output )
    {
        const fheroes2::Sprite & cell = fheroes2::AGG::GetICN( ICN::CELLWIN, 1 );

        fheroes2::Blit( cell, output, offset.x, offset.y );
        if ( ( heroType & CONTROL_HUMAN ) == CONTROL_HUMAN ) {
            const fheroes2::Sprite & mark = fheroes2::AGG::GetICN( ICN::CELLWIN, 2 );
            fheroes2::Blit( mark, output, offset.x + mark.x(), offset.y + mark.y() );
        }

        if ( isEvilInterface ) {
            fheroes2::ApplyPalette( output, offset.x, offset.y, output, offset.x, offset.y, cell.width(), cell.height(),
                                    PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );
        }

        fheroes2::Text text( _( "Human" ), fheroes2::FontType::smallWhite() );
        text.fitToOneRow( defendingHeroTypeNameMaxWidth );
        text.draw( offset.x + cell.width() + 5, offset.y + 5, output );
    }

    void renderTerrain( const fheroes2::Point offset, const int32_t terrainType, fheroes2::Image & output )
    {
        const fheroes2::Text text{ _( "Terrain" ), fheroes2::FontType::smallWhite() };
        text.draw( offset.x + ( terrainIconSize.width + 2 - text.width() ) / 2, offset.y - 10, output );

        uint32_t imageIndex{ 0 };
        if ( terrainType == Maps::Ground::UNKNOWN ) {
            imageIndex = Maps::Ground::getRandomTerrainImageIndex( Maps::Ground::SWAMP, false );
        }
        else {
            imageIndex = Maps::Ground::getRandomTerrainImageIndex( terrainType, false );
        }

        fheroes2::DrawRect( output, { offset.x, offset.y, terrainIconSize.width + 2, terrainIconSize.height + 2 }, 113 );

        const auto & terrainSelectionSprite = fheroes2::AGG::GetTIL( TIL::GROUND32, imageIndex, 0 );

        fheroes2::Copy( terrainSelectionSprite, 0, 0, output, offset.x + 1, offset.y + 1, terrainIconSize.width, terrainIconSize.height );
        if ( terrainType == Maps::Ground::UNKNOWN ) {
            fheroes2::ApplyPalette( output, offset.x + 1, offset.y + 1, output, offset.x + 1, offset.y + 1, terrainIconSize.width, terrainIconSize.height,
                                    PAL::GetPalette( PAL::PaletteType::PURPLE ) );
        }
    }

    void copyImage( const fheroes2::Image & input, fheroes2::Image & output, const fheroes2::Rect area, const fheroes2::Point offset )
    {
        fheroes2::Copy( input, area.x, area.y, output, area.x + offset.x, area.y + offset.y, area.width, area.height );
    }

    constexpr int32_t getNextTerrain( const int32_t terrainType )
    {
        switch ( terrainType ) {
        case Maps::Ground::UNKNOWN:
            return Maps::Ground::WATER;
        case Maps::Ground::WATER:
            return Maps::Ground::GRASS;
        case Maps::Ground::GRASS:
            return Maps::Ground::SNOW;
        case Maps::Ground::SNOW:
            return Maps::Ground::SWAMP;
        case Maps::Ground::SWAMP:
            return Maps::Ground::LAVA;
        case Maps::Ground::LAVA:
            return Maps::Ground::DESERT;
        case Maps::Ground::DESERT:
            return Maps::Ground::DIRT;
        case Maps::Ground::DIRT:
            return Maps::Ground::WASTELAND;
        case Maps::Ground::WASTELAND:
            return Maps::Ground::BEACH;
        case Maps::Ground::BEACH:
            return Maps::Ground::UNKNOWN;
        default:
            // How did you end up here? Did you add a new terrain type?
            assert( 0 );
            break;
        }

        return Maps::Ground::UNKNOWN;
    }

    constexpr int32_t getPrevTerrain( const int32_t terrainType )
    {
        switch ( terrainType ) {
        case Maps::Ground::BEACH:
            return Maps::Ground::WASTELAND;
        case Maps::Ground::WASTELAND:
            return Maps::Ground::DIRT;
        case Maps::Ground::DIRT:
            return Maps::Ground::DESERT;
        case Maps::Ground::DESERT:
            return Maps::Ground::LAVA;
        case Maps::Ground::LAVA:
            return Maps::Ground::SWAMP;
        case Maps::Ground::SWAMP:
            return Maps::Ground::SNOW;
        case Maps::Ground::SNOW:
            return Maps::Ground::GRASS;
        case Maps::Ground::GRASS:
            return Maps::Ground::WATER;
        case Maps::Ground::WATER:
            return Maps::Ground::UNKNOWN;
        case Maps::Ground::UNKNOWN:
            return Maps::Ground::BEACH;
        default:
            // How did you end up here? Did you add a new terrain type?
            assert( 0 );
            break;
        }

        return Maps::Ground::UNKNOWN;
    }
}

Battle::Only::Only()
{
    armyInfo[1].armyId = 1;

    for ( auto & info : armyInfo ) {
        info.monster.GetTroop( 0 )->Set( defaultMonster );
        info.monsterBackup.Assign( info.monster );
    }

    armyInfo[0].controlType = CONTROL_HUMAN;
    armyInfo[0].player.SetControl( armyInfo[0].controlType );
    armyInfo[0].player.SetColor( playerColor[0] );

    armyInfo[1].controlType = CONTROL_AI;
    armyInfo[1].player.SetControl( armyInfo[1].controlType );
    armyInfo[1].player.SetColor( playerColor[1] );
}

bool Battle::Only::setup( const bool allowBackup, bool & resetBattleSetup )
{
    resetBattleSetup = false;

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::StandardWindow frameborder( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT, true, display );

    const fheroes2::Point windowOffset( frameborder.activeArea().x, frameborder.activeArea().y );

    for ( size_t i = 0; i < armyInfo.size(); ++i ) {
        armyInfo[i].portraitRoi = { windowOffset.x + heroPortraitInternalArea[i].x, windowOffset.y + heroPortraitInternalArea[i].y, heroPortraitInternalArea[i].width,
                                    heroPortraitInternalArea[i].height };
    }

    armyInfo[0].player.SetControl( armyInfo[0].controlType );
    armyInfo[1].player.SetControl( armyInfo[1].controlType );

    for ( auto & info : armyInfo ) {
        info.hero = nullptr;
        info.monster.Reset();
        info.monster.GetTroop( 0 )->Set( defaultMonster );

        if ( !_backupCompleted || !allowBackup ) {
            continue;
        }

        if ( info.isHeroPresent ) {
            info.hero = world.GetHeroes( info.heroBackup.GetID() );
            info.hero->GetSecondarySkills().FillMax( Skill::Secondary() );

            copyHero( info.heroBackup, *info.hero );
        }
        else {
            info.monster.Assign( info.monsterBackup );
        }
    }

    if ( !_backupCompleted || !allowBackup ) {
        armyInfo[0].hero = world.GetHeroes( Heroes::LORDKILBURN );
        armyInfo[0].isHeroPresent = true;
    }

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    prepareBackround( display, windowOffset, isEvilInterface );

    fheroes2::ImageRestorer primaryTextRoiRestorer( display, windowOffset.x + primarySkillTextRoi.x, windowOffset.y + primarySkillTextRoi.y, primarySkillTextRoi.width,
                                                    primarySkillTextRoi.height );
    fheroes2::ImageRestorer titleRoiRestorer( display, windowOffset.x + titleTextRoi.x, windowOffset.y + titleTextRoi.y, titleTextRoi.width, titleTextRoi.height );

    redrawOpponents( windowOffset );
    redrawOpponentsStats( windowOffset );

    for ( auto & info : armyInfo ) {
        if ( info.hero != nullptr ) {
            info.hero->GetSecondarySkills().FillMax( Skill::Secondary() );

            updateHero( info, windowOffset );
        }
        else {
            info.ui.army = std::make_unique<ArmyBar>( &info.monster, true, false, true );
            info.ui.army->setTableSize( { 5, 1 } );
            info.ui.army->setRenderingOffset( { windowOffset.x + armyOffsetX[info.armyId], windowOffset.y + 267 } );
            info.ui.army->setInBetweenItemsOffset( { 2, 0 } );
        }

        info.ui.redraw( display );
    }

    const int32_t heroTypeAreaWidth = getMaxDefendingHeroAreaWidth();
    const fheroes2::Rect defendingHeroTypeRoi{ defendingHeroTypeCheckboxArea.x + windowOffset.x - heroTypeAreaWidth, defendingHeroTypeCheckboxArea.y + windowOffset.y,
                                               heroTypeAreaWidth, defendingHeroTypeCheckboxArea.height };

    if ( armyInfo[1].hero != nullptr ) {
        renderDefendingHeroTypeUI( armyInfo[1].player.GetControl(), defendingHeroTypeRoi.getPosition(), isEvilInterface, display );
    }

    const int32_t buttonResetIcn = ( isEvilInterface ? ICN::BUTTON_RESET_EVIL : ICN::BUTTON_RESET_GOOD );
    const int32_t buttonStartIcn = ( isEvilInterface ? ICN::BUTTON_START_EVIL : ICN::BUTTON_START_GOOD );
    const int32_t buttonExitIcn = ( isEvilInterface ? ICN::BUTTON_SMALL_EXIT_EVIL : ICN::BUTTON_SMALL_EXIT_GOOD );

    constexpr int32_t buttonYOffset{ 446 };
    fheroes2::Button buttonReset( windowOffset.x + 30, windowOffset.y + buttonYOffset, buttonResetIcn, 0, 1 );
    fheroes2::Button buttonStart( windowOffset.x + 178, windowOffset.y + buttonYOffset, buttonStartIcn, 0, 1 );
    fheroes2::Button buttonExit( windowOffset.x + 366, windowOffset.y + buttonYOffset, buttonExitIcn, 0, 1 );

    fheroes2::addGradientShadow( fheroes2::AGG::GetICN( buttonResetIcn, 0 ), display, buttonReset.area().getPosition(), shadowOffset );
    fheroes2::addGradientShadow( fheroes2::AGG::GetICN( buttonStartIcn, 0 ), display, buttonStart.area().getPosition(), shadowOffset );
    fheroes2::addGradientShadow( fheroes2::AGG::GetICN( buttonExitIcn, 0 ), display, buttonExit.area().getPosition(), shadowOffset );

    buttonStart.draw();
    buttonExit.draw();
    buttonReset.draw();

    const fheroes2::Rect terrainArea{ windowOffset.x + 306, windowOffset.y + 272, terrainIconSize.width, terrainIconSize.height };
    renderTerrain( terrainArea.getPosition(), _terrainType, display );

    display.render();

    bool result = false;

    bool renderAttackingHeroType{ false };

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        bool updateSpellPoints = false;
        bool needRender = false;
        bool needRedrawOpponentsStats = false;
        bool needRedrawControlInfo = false;

        if ( buttonStart.isEnabled() ) {
            buttonStart.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonStart.area() ) );
        }
        if ( buttonExit.isEnabled() ) {
            buttonExit.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonExit.area() ) );
        }
        if ( buttonReset.isEnabled() ) {
            buttonReset.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonReset.area() ) );
        }

        if ( ( buttonStart.isEnabled() && le.MouseClickLeft( buttonStart.area() ) ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
            result = true;

            break;
        }
        if ( le.MouseClickLeft( buttonReset.area() ) ) {
            resetBattleSetup = true;
            result = true;

            break;
        }

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            break;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonStart.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Start" ), _( "Start the battle." ), 0 );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonExit.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Exit" ), _( "Exit this menu." ), 0 );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonReset.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Reset" ), _( "Reset to default settings." ), 0 );
        }
        else if ( le.isMouseRightButtonPressedInArea( terrainArea ) ) {
            if ( _terrainType == Maps::Ground::UNKNOWN ) {
                fheroes2::showStandardTextMessage( _( "Terrain" ), _( "The battle will take place on a randomly selected terrain. Click to change the terrain type." ),
                                                   0 );
            }
            else {
                std::string message = _( "The battle will take place on %{terrain-type} terrain. Click to change the terrain type." );
                StringReplace( message, "%{terrain-type}", StringLower( Maps::Ground::String( _terrainType ) ) );
                fheroes2::showStandardTextMessage( _( "Terrain" ), std::move( message ), 0 );
            }
        }

        if ( le.MouseClickLeft( terrainArea ) || le.isMouseWheelDownInArea( terrainArea ) ) {
            _terrainType = getNextTerrain( _terrainType );
            renderTerrain( terrainArea.getPosition(), _terrainType, display );
            needRender = true;
        }
        else if ( le.isMouseWheelUpInArea( terrainArea ) ) {
            _terrainType = getPrevTerrain( _terrainType );
            renderTerrain( terrainArea.getPosition(), _terrainType, display );
            needRender = true;
        }

        for ( const auto & [firstId, secondId] : { std::pair<int32_t, int32_t>( 0, 1 ), std::pair<int32_t, int32_t>( 1, 0 ) } ) {
            ArmyInfo & first = armyInfo[firstId];
            const ArmyInfo & second = armyInfo[secondId];

            if ( le.MouseClickLeft( first.portraitRoi ) ) {
                const int hid = Dialog::selectHeroes( first.hero ? first.hero->GetID() : Heroes::UNKNOWN );
                if ( second.hero && hid == second.hero->GetID() ) {
                    fheroes2::showStandardTextMessage( _( "Error" ), _( "Please select another hero." ), Dialog::OK );
                }
                else if ( Heroes::UNKNOWN != hid ) {
                    first.hero = world.GetHeroes( hid );

                    if ( first.hero ) {
                        first.hero->GetSecondarySkills().FillMax( Skill::Secondary() );
                    }

                    updateHero( first, windowOffset );
                }

                titleRoiRestorer.restore();
                redrawOpponents( windowOffset );

                first.needRedraw = true;
                needRedrawOpponentsStats = true;

                // User can not click two hero portraits at the same time so we can break the loop.
                break;
            }
            else if ( le.isMouseRightButtonPressedInArea( first.portraitRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Hero" ), _( "Click to select a hero." ), Dialog::ZERO );

                // User can not click two hero portraits at the same time so we can break the loop.
                break;
            }
        }

        if ( !renderAttackingHeroType && armyInfo[1].hero != nullptr ) {
            needRedrawControlInfo = true;
            renderAttackingHeroType = true;
        }

        for ( const auto & [firstId, secondId] : { std::pair<int32_t, int32_t>{ 0, 1 }, std::pair<int32_t, int32_t>{ 1, 0 } } ) {
            const ArmyUI & firstUI = armyInfo[firstId].ui;
            const ArmyUI & secondUI = armyInfo[secondId].ui;

            if ( firstUI.army != nullptr && le.isMouseCursorPosInArea( firstUI.army->GetArea() ) && firstUI.army->QueueEventProcessing() ) {
                if ( firstUI.artifact != nullptr && firstUI.artifact->isSelected() ) {
                    firstUI.artifact->ResetSelected();
                }

                if ( secondUI.artifact != nullptr && secondUI.artifact->isSelected() ) {
                    secondUI.artifact->ResetSelected();
                }

                if ( secondUI.army != nullptr && secondUI.army->isSelected() ) {
                    secondUI.army->ResetSelected();
                }

                armyInfo[firstId].needRedraw = true;

                const bool isArmyValid = ( armyInfo[firstId].monster.isValid() && armyInfo[secondId].monster.isValid() );
                if ( isArmyValid && !buttonStart.isEnabled() ) {
                    buttonStart.enable();
                    buttonStart.draw();
                    needRender = true;
                }
                else if ( !isArmyValid && buttonStart.isEnabled() ) {
                    buttonStart.disable();
                    buttonStart.draw();
                    needRender = true;
                }
            }
            else if ( firstUI.artifact != nullptr && le.isMouseCursorPosInArea( firstUI.artifact->GetArea() ) && firstUI.artifact->QueueEventProcessing() ) {
                if ( firstUI.army != nullptr && firstUI.army->isSelected() ) {
                    firstUI.army->ResetSelected();
                }

                if ( secondUI.artifact != nullptr && secondUI.artifact->isSelected() ) {
                    secondUI.artifact->ResetSelected();
                }

                if ( secondUI.army != nullptr && secondUI.army->isSelected() ) {
                    secondUI.army->ResetSelected();
                }

                armyInfo[firstId].needRedraw = true;
                updateSpellPoints = true;
                needRedrawOpponentsStats = true;
            }
            else if ( firstUI.morale != nullptr && le.isMouseCursorPosInArea( firstUI.morale->GetArea() ) ) {
                MoraleIndicator::QueueEventProcessing( *firstUI.morale );
            }
            else if ( firstUI.luck != nullptr && le.isMouseCursorPosInArea( firstUI.luck->GetArea() ) ) {
                LuckIndicator::QueueEventProcessing( *firstUI.luck );
            }
            else if ( firstUI.primarySkill != nullptr && le.isMouseCursorPosInArea( firstUI.primarySkill->GetArea() ) ) {
                if ( firstUI.primarySkill->QueueEventProcessing() ) {
                    updateSpellPoints = true;
                    needRedrawOpponentsStats = true;
                }
            }
            else if ( firstUI.secondarySkill != nullptr && le.isMouseCursorPosInArea( firstUI.secondarySkill->GetArea() )
                      && firstUI.secondarySkill->QueueEventProcessing() ) {
                armyInfo[firstId].needRedraw = true;
            }
        }

        if ( armyInfo[1].hero != nullptr ) {
            if ( le.MouseClickLeft( defendingHeroTypeRoi ) ) {
                if ( armyInfo[1].player.isControlAI() ) {
                    armyInfo[1].player.SetControl( CONTROL_HUMAN );
                }
                else {
                    armyInfo[1].player.SetControl( CONTROL_AI );
                }

                needRedrawControlInfo = true;
            }
            else if ( le.isMouseRightButtonPressedInArea( defendingHeroTypeRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Human" ), _( "If this checkbox is checked, the defending hero is going be human-controlled." ), 0 );
            }
        }

        if ( updateSpellPoints ) {
            for ( Heroes * hero : { armyInfo[0].hero, armyInfo[1].hero } ) {
                if ( hero == nullptr ) {
                    continue;
                }

                hero->SetSpellPoints( hero->GetMaxSpellPoints() );
            }
        }

        if ( needRedrawOpponentsStats ) {
            primaryTextRoiRestorer.restore();
            redrawOpponentsStats( windowOffset );

            needRender = true;
        }

        for ( const int32_t i : { 0, 1 } ) {
            if ( armyInfo[i].needRedraw ) {
                armyInfo[i].ui.redraw( display );
                armyInfo[i].needRedraw = false;

                needRender = true;
            }
        }

        if ( needRedrawControlInfo ) {
            assert( armyInfo[1].hero != nullptr );
            renderDefendingHeroTypeUI( armyInfo[1].player.GetControl(), defendingHeroTypeRoi.getPosition(), isEvilInterface, display );

            needRender = true;
        }

        if ( needRender ) {
            display.render();
        }
    }

    armyInfo[0].ui = {};
    armyInfo[1].ui = {};

    return result;
}

void Battle::Only::updateHero( ArmyInfo & info, const fheroes2::Point & offset )
{
    info.ui.resetForNewHero();

    if ( info.hero == nullptr ) {
        return;
    }

    updateArmyUI( info.ui, info.hero, offset, info.armyId );
}

void Battle::Only::redrawOpponents( const fheroes2::Point & top ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Rect textRoi( top.x + 89, top.y + 27, 462, 17 );

    std::string message = _( "%{race1} %{name1} vs %{race2} %{name2}" );

    if ( armyInfo[0].hero ) {
        StringReplace( message, "%{name1}", armyInfo[0].hero->GetName() );
        StringReplace( message, "%{race1}", std::string( Race::String( armyInfo[0].hero->GetRace() ) ) );
    }
    else {
        StringReplace( message, "%{race1}", "" );
        StringReplace( message, " %{name1}", _( "Monsters" ) );
    }
    if ( armyInfo[1].hero ) {
        StringReplace( message, "%{name2}", armyInfo[1].hero->GetName() );
        StringReplace( message, "%{race2}", std::string( Race::String( armyInfo[1].hero->GetRace() ) ) );
    }
    else {
        StringReplace( message, "%{race2}", "" );
        StringReplace( message, " %{name2}", _( "Monsters" ) );
    }

    fheroes2::Text text( std::move( message ), fheroes2::FontType::normalWhite() );
    text.drawInRoi( top.x + 320 - text.width() / 2, top.y + 29, display, textRoi );

    for ( const size_t idx : { 0, 1 } ) {
        if ( armyInfo[idx].hero ) {
            const fheroes2::Sprite & port = armyInfo[idx].hero->GetPortrait( PORT_BIG );
            if ( !port.empty() ) {
                fheroes2::Copy( port, 0, 0, display, armyInfo[idx].portraitRoi );
            }
        }
        else {
            fheroes2::Fill( display, armyInfo[idx].portraitRoi.x, armyInfo[idx].portraitRoi.y, armyInfo[idx].portraitRoi.width, armyInfo[idx].portraitRoi.height, 0 );
            text.set( _( "N/A" ), fheroes2::FontType::normalWhite() );
            text.draw( armyInfo[idx].portraitRoi.x + ( armyInfo[idx].portraitRoi.width - text.width() ) / 2,
                       armyInfo[idx].portraitRoi.y + armyInfo[idx].portraitRoi.height / 2 - 8, display );
        }
    }
}

void Battle::Only::redrawOpponentsStats( const fheroes2::Point & top ) const
{
    fheroes2::RedrawPrimarySkillInfo( top, armyInfo[0].ui.primarySkill.get(), armyInfo[1].ui.primarySkill.get() );
}

void Battle::Only::StartBattle()
{
    assert( armyInfo[0].hero != nullptr );

    Settings & conf = Settings::Get();

    conf.GetPlayers().Init( armyInfo[0].player.GetColor() | armyInfo[1].player.GetColor() );
    world.InitKingdoms();

    conf.SetCurrentColor( armyInfo[0].player.GetColor() );

    for ( const int32_t idx : { 0, 1 } ) {
        Players::SetPlayerRace( armyInfo[idx].player.GetColor(), armyInfo[idx].player.GetRace() );
        Players::SetPlayerControl( armyInfo[idx].player.GetColor(), armyInfo[idx].player.GetControl() );
        armyInfo[idx].controlType = armyInfo[idx].player.GetControl();

        armyInfo[idx].isHeroPresent = ( armyInfo[idx].hero != nullptr );

        if ( !armyInfo[idx].isHeroPresent ) {
            armyInfo[idx].monsterBackup.Assign( armyInfo[idx].monster );
            continue;
        }

        armyInfo[idx].hero->SetSpellPoints( armyInfo[idx].hero->GetMaxSpellPoints() );
        armyInfo[idx].hero->Recruit( armyInfo[idx].player.GetColor(), { idx, idx } );

        copyHero( *armyInfo[idx].hero, armyInfo[idx].heroBackup );

        armyInfo[idx].monster.Reset();
        armyInfo[idx].monster.GetTroop( 0 )->Set( Monster::PEASANT, 100 );
        armyInfo[idx].monsterBackup.Assign( armyInfo[idx].monster );
    }

    _backupCompleted = true;

    Battle::Loader( ( armyInfo[0].hero ? armyInfo[0].hero->GetArmy() : armyInfo[0].monster ), ( armyInfo[1].hero ? armyInfo[1].hero->GetArmy() : armyInfo[1].monster ),
                    1 );

    conf.SetCurrentColor( PlayerColor::NONE );
}

void Battle::Only::reset()
{
    armyInfo[0].reset();
    armyInfo[1].reset();

    _terrainType = Maps::Ground::UNKNOWN;
}

void Battle::Only::copyHero( const Heroes & in, Heroes & out )
{
    out.attack = in.attack;
    out.defense = in.defense;
    out.knowledge = in.knowledge;
    out.power = in.power;
    out._id = in._id;
    out._portrait = in._portrait;
    out._race = in._race;

    out._secondarySkills.ToVector() = in._secondarySkills.ToVector();
    out._army.Assign( in._army );

    out._bagArtifacts = in._bagArtifacts;
    out._spellBook = in._spellBook;

    out.SetSpellPoints( out.GetMaxSpellPoints() );
}

void Battle::Only::updateArmyUI( ArmyUI & ui, Heroes * hero, const fheroes2::Point & offset, const uint8_t armyId )
{
    assert( hero != nullptr );

    ui.morale = std::make_unique<MoraleIndicator>( hero );
    ui.morale->SetPos( { offset.x + moraleAndLuckOffsetX[armyId], offset.y + 75 } );

    ui.luck = std::make_unique<LuckIndicator>( hero );
    ui.luck->SetPos( { offset.x + moraleAndLuckOffsetX[armyId], offset.y + 115 } );

    ui.primarySkill = std::make_unique<PrimarySkillsBar>( hero, true, true, false );
    ui.primarySkill->setTableSize( { 1, 4 } );
    ui.primarySkill->setInBetweenItemsOffset( { 0, -1 } );
    ui.primarySkill->SetTextOff( armyId == 0 ? 70 : -70, -25 );
    ui.primarySkill->setRenderingOffset( { offset.x + primarySkillArea[armyId].x, offset.y + 51 } );

    ui.secondarySkill = std::make_unique<SecondarySkillsBar>( *hero, true, true );
    ui.secondarySkill->setTableSize( { 8, 1 } );
    ui.secondarySkill->setInBetweenItemsOffset( { -1, 0 } );
    ui.secondarySkill->SetContent( hero->GetSecondarySkills().ToVector() );
    ui.secondarySkill->setRenderingOffset( { offset.x + secondarySkillArea[armyId].x, offset.y + 199 } );

    ui.artifact = std::make_unique<ArtifactsBar>( hero, true, false, true, true, nullptr );
    ui.artifact->setTableSize( { 7, 2 } );
    ui.artifact->setInBetweenItemsOffset( { 2, 2 } );
    ui.artifact->SetContent( hero->GetBagArtifacts() );
    ui.artifact->setRenderingOffset( { offset.x + artifactArea[armyId].x, offset.y + 347 } );

    ui.army = std::make_unique<ArmyBar>( &hero->GetArmy(), true, false, true );
    ui.army->setTableSize( { 5, 1 } );
    ui.army->setRenderingOffset( { offset.x + armyOffsetX[armyId], offset.y + 267 } );
    ui.army->setInBetweenItemsOffset( { 2, 0 } );
}

int32_t Battle::Only::terrainType() const
{
    if ( _terrainType == Maps::Ground::UNKNOWN ) {
        const std::vector<int32_t> terrainTypes{ Maps::Ground::DESERT, Maps::Ground::SNOW, Maps::Ground::SWAMP, Maps::Ground::WASTELAND, Maps::Ground::BEACH,
                                                 Maps::Ground::LAVA,   Maps::Ground::DIRT, Maps::Ground::GRASS, Maps::Ground::WATER };

        return Rand::Get( terrainTypes );
    }

    return _terrainType;
}

void Battle::Only::ArmyUI::redraw( fheroes2::Image & output ) const
{
    if ( morale ) {
        morale->Redraw();
    }

    if ( luck ) {
        luck->Redraw();
    }

    if ( primarySkill ) {
        primarySkill->Redraw( output );
    }

    if ( secondarySkill ) {
        secondarySkill->Redraw( output );
    }

    if ( artifact ) {
        artifact->Redraw( output );
    }

    if ( army ) {
        army->Redraw( output );
    }
}

void Battle::Only::ArmyUI::resetForNewHero()
{
    if ( morale ) {
        morale->redrawOnlyBackground();
    }

    if ( luck ) {
        luck->redrawOnlyBackground();
    }

    *this = {};
}

void Battle::Only::ArmyInfo::reset()
{
    ui = {};
    hero = nullptr;

    monster.Reset();
    monster.GetTroop( 0 )->Set( defaultMonster );
}
