/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "agg_image.h"
#include "army.h"
#include "army_bar.h"
#include "army_troop.h"
#include "audio.h"
#include "audio_manager.h"
#include "captain.h"
#include "castle.h" // IWYU pragma: associated
#include "castle_building_info.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "heroes.h"
#include "heroes_base.h"
#include "icn.h"
#include "image.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "localevent.h"
#include "m82.h"
#include "math_base.h"
#include "monster.h"
#include "mus.h"
#include "screen.h"
#include "statusbar.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_castle.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_kingdom.h"
#include "ui_tool.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    BuildingType getPressedBuildingHotkey()
    {
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_1 ) ) {
            return DWELLING_MONSTER1;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_2 ) ) {
            return DWELLING_MONSTER2;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_3 ) ) {
            return DWELLING_MONSTER3;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_4 ) ) {
            return DWELLING_MONSTER4;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_5 ) ) {
            return DWELLING_MONSTER5;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_6 ) ) {
            return DWELLING_MONSTER6;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_MARKETPLACE ) ) {
            return BUILD_MARKETPLACE;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_WELL ) ) {
            return BUILD_WELL;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_MAGE_GUILD ) ) {
            return BUILD_MAGEGUILD;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_SHIPYARD ) ) {
            return BUILD_SHIPYARD;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_THIEVES_GUILD ) ) {
            return BUILD_THIEVESGUILD;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_TAVERN ) ) {
            return BUILD_TAVERN;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_CONSTRUCTION ) ) {
            return BUILD_CASTLE;
        }

        return BUILD_NOTHING;
    }

    std::string buildingStatusMessage( const int race, const uint32_t buildingId )
    {
        // Check if building is a monster dwelling or its upgraded version
        if ( ( buildingId & DWELLING_MONSTERS ) == 0 && ( buildingId & DWELLING_UPGRADES ) == 0 ) {
            return fheroes2::getBuildingName( race, static_cast<BuildingType>( buildingId ) );
        }

        const Monster monster( race, buildingId );
        std::string msgStatus = _( "Recruit %{name}" );
        StringReplaceWithLowercase( msgStatus, "%{name}", monster.GetMultiName() );
        return msgStatus;
    }

    void RedrawIcons( const Castle & castle, const Heroes * hero, const fheroes2::Point & pt )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 0 ), display, pt.x, pt.y + 256 );

        if ( castle.isBuild( BUILD_CAPTAIN ) ) {
            const fheroes2::Sprite & captainImage = castle.GetCaptain().GetPortrait( PORT_BIG );
            fheroes2::Copy( captainImage, 0, 0, display, pt.x + 5, pt.y + 262, captainImage.width(), captainImage.height() );
        }
        else {
            const fheroes2::Sprite & crestImage = fheroes2::AGG::GetICN( ICN::CREST, Color::GetIndex( castle.GetColor() ) );
            fheroes2::Copy( crestImage, 0, 0, display, pt.x + 5, pt.y + 262, crestImage.width(), crestImage.height() );
        }

        if ( hero ) {
            hero->PortraitRedraw( pt.x + 5, pt.y + 361, PORT_BIG, display );
        }
        else {
            const fheroes2::Sprite & heroImage = fheroes2::AGG::GetICN( ICN::STRIP, 3 );
            const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( ICN::STRIP, 11 );
            fheroes2::Copy( heroImage, 0, 0, display, pt.x + 5, pt.y + 361, heroImage.width(), heroImage.height() );
            fheroes2::Copy( backgroundImage, 0, 0, display, pt.x + 112, pt.y + 361, backgroundImage.width(), backgroundImage.height() );
        }
    }

    std::string getDateString()
    {
        std::string output( _( "Month: %{month}, Week: %{week}, Day: %{day}" ) );
        StringReplace( output, "%{month}", world.GetMonth() );
        StringReplace( output, "%{week}", world.GetWeek() );
        StringReplace( output, "%{day}", world.GetDay() );

        return output;
    }

    bool purchaseSpellBookIfNecessary( const Castle & castle, Heroes * hero )
    {
        if ( hero == nullptr || hero->HaveSpellBook() ) {
            return false;
        }

        if ( hero->IsFullBagArtifacts() ) {
            fheroes2::showStandardTextMessage(
                hero->GetName(),
                _( "You must purchase a spell book to use the mage guild, but you currently have no room for a spell book. Try giving one of your artifacts to another hero." ),
                Dialog::OK );

            return false;
        }

        return hero->BuySpellBook( &castle );
    }

    void openHeroDialog( ArmyBar & topArmyBar, ArmyBar & bottomArmyBar, Heroes & hero )
    {
        Game::SetUpdateSoundsOnFocusUpdate( false );
        Game::OpenHeroesDialog( hero, false, false );

        if ( topArmyBar.isValid() && topArmyBar.isSelected() ) {
            topArmyBar.ResetSelected();
        }
        if ( bottomArmyBar.isValid() && bottomArmyBar.isSelected() ) {
            bottomArmyBar.ResetSelected();
        }
    }

    void generateHeroImage( fheroes2::Image & image, Heroes * hero )
    {
        assert( hero != nullptr );

        if ( image.empty() ) {
            image.resize( 552, 105 );
            image.reset();
        }

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 0 ), 0, 100, image, 0, 0, 552, 107 );
        const fheroes2::Sprite & port = hero->GetPortrait( PORT_BIG );
        fheroes2::Copy( port, 0, 0, image, 5, 5, port.width(), port.height() );

        ArmyBar armyBar( &hero->GetArmy(), false, false );
        armyBar.setTableSize( { 5, 1 } );
        armyBar.setRenderingOffset( { 112, 5 } );
        armyBar.setInBetweenItemsOffset( { 6, 0 } );
        armyBar.Redraw( image );
    }
}

Castle::CastleDialogReturnValue Castle::OpenDialog( const bool openConstructionWindow, const bool fade, const bool renderBackgroundDialog )
{
    // Set the cursor image. This dialog does not require a cursor restorer. It is called from other dialogs that have the same cursor
    // or from the Game Area that will set the appropriate cursor after this dialog is closed.
    Cursor::Get().SetThemes( Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::Rect dialogRoi;
    fheroes2::Rect dialogWithShadowRoi;
    std::unique_ptr<fheroes2::StandardWindow> background;
    std::unique_ptr<fheroes2::ImageRestorer> restorer;

    if ( renderBackgroundDialog ) {
        background = std::make_unique<fheroes2::StandardWindow>( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT, false );
        dialogRoi = background->activeArea();
        dialogWithShadowRoi = background->totalArea();
    }
    else {
        dialogRoi = { ( display.width() - fheroes2::Display::DEFAULT_WIDTH ) / 2, ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2,
                      fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT };
        dialogWithShadowRoi = { dialogRoi.x - 2 * fheroes2::borderWidthPx, dialogRoi.y - fheroes2::borderWidthPx, dialogRoi.width + 3 * fheroes2::borderWidthPx,
                                dialogRoi.height + 3 * fheroes2::borderWidthPx };
        restorer = std::make_unique<fheroes2::ImageRestorer>( display, dialogRoi.x, dialogRoi.y, dialogRoi.width, dialogRoi.height );
    }

    // Fade-out game screen only for 640x480 resolution and if 'renderBackgroundDialog' is false (we are replacing image in already opened dialog).
    const bool isDefaultScreenSize = display.isDefaultSize();
    if ( fade && ( isDefaultScreenSize || !renderBackgroundDialog ) ) {
        fheroes2::fadeOutDisplay( dialogRoi, !isDefaultScreenSize );
    }

    AudioManager::PlayMusicAsync( MUS::FromRace( _race ), Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

    int alphaHero = 255;
    CastleDialog::FadeBuilding fadeBuilding;

    Heroes * hero = world.GetHero( *this );

    fheroes2::Image surfaceHero;

    auto constructionDialogHandler = [this, &display, &dialogRoi, &fadeBuilding, &hero, &surfaceHero, &alphaHero]() {
        uint32_t build = BUILD_NOTHING;
        const Castle::ConstructionDialogResult result = _openConstructionDialog( build );

        switch ( result ) {
        case ConstructionDialogResult::NextConstructionWindow:
            return CastleDialogReturnValue::NextCostructionWindow;
        case ConstructionDialogResult::PrevConstructionWindow:
            return CastleDialogReturnValue::PreviousCostructionWindow;
        case ConstructionDialogResult::RecruitHero:
            hero = world.GetHero( *this );
            generateHeroImage( surfaceHero, hero );
            AudioManager::PlaySound( M82::BUILDTWN );
            alphaHero = 0;
            break;
        default:
            if ( build == BUILD_NOTHING ) {
                break;
            }

            if ( !BuyBuilding( build ) ) {
                assert( 0 );
            }

            AudioManager::PlaySound( M82::BUILDTWN );
            fadeBuilding.startFadeBuilding( build );
            break;
        }

        // Set next render ROI to dialog ROI. It is needed to properly update the castle dialog after the construction dialog is closed.
        display.updateNextRenderRoi( dialogRoi );

        return CastleDialogReturnValue::DoNothing;
    };

    if ( openConstructionWindow && isBuild( BUILD_CASTLE ) ) {
        const CastleDialogReturnValue constructionResult = constructionDialogHandler();
        if ( constructionResult != CastleDialogReturnValue::DoNothing ) {
            return constructionResult;
        }
    }

    const std::string currentDate = getDateString();

    // Previous castle button.
    fheroes2::Point statusBarPosition( dialogRoi.x, dialogRoi.y + 480 - 19 );

    fheroes2::Button buttonPrevCastle( statusBarPosition.x, statusBarPosition.y, ICN::SMALLBAR, 1, 2 );
    fheroes2::TimedEventValidator timedButtonPrevCastle( [&buttonPrevCastle]() { return buttonPrevCastle.isPressed(); } );
    buttonPrevCastle.subscribe( &timedButtonPrevCastle );

    statusBarPosition.x += buttonPrevCastle.area().width;

    // Status bar at the bottom of dialog.
    const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( ICN::SMALLBAR, 0 );
    fheroes2::Copy( bar, 0, 0, display, statusBarPosition.x, statusBarPosition.y, bar.width(), bar.height() );

    StatusBar statusBar;
    // Status bar must be smaller due to extra art on both sides.
    statusBar.setRoi( { statusBarPosition.x + 16, statusBarPosition.y + 3, bar.width() - 16 * 2, 0 } );

    // Next castle button.
    fheroes2::Button buttonNextCastle( statusBarPosition.x + bar.width(), statusBarPosition.y, ICN::SMALLBAR, 3, 4 );
    fheroes2::TimedEventValidator timedButtonNextCastle( [&buttonNextCastle]() { return buttonNextCastle.isPressed(); } );
    buttonNextCastle.subscribe( &timedButtonNextCastle );

    // Position of the captain/crest and hero portrait to the left of army bars.
    const fheroes2::Sprite & crest = fheroes2::AGG::GetICN( ICN::CREST, Color::GetIndex( GetColor() ) );
    const fheroes2::Rect rectSign1( dialogRoi.x + 5, dialogRoi.y + 262, crest.width(), crest.height() );
    const fheroes2::Rect rectSign2( rectSign1.x, dialogRoi.y + 361, 100, 92 );

    // For the case when new hero is recruited we need to render bottom army bar
    // without hero and his army to properly perform hero fade-in.
    RedrawIcons( *this, ( alphaHero == 0 ) ? nullptr : hero, dialogRoi.getPosition() );

    auto setArmyBarParameters = []( ArmyBar & armyBar, const fheroes2::Point & offset ) {
        armyBar.setTableSize( { 5, 1 } );
        armyBar.setRenderingOffset( offset );
        armyBar.setInBetweenItemsOffset( { 6, 0 } );
    };

    // Castle army (top) bar.
    ArmyBar topArmyBar( &_army, false, false );
    setArmyBarParameters( topArmyBar, { dialogRoi.x + 112, rectSign1.y } );
    topArmyBar.Redraw( display );

    // Hero army (bottom) bar.
    ArmyBar bottomArmyBar( hero ? &hero->GetArmy() : nullptr, false, false );
    setArmyBarParameters( bottomArmyBar, { dialogRoi.x + 112, rectSign2.y } );

    if ( hero && alphaHero != 0 ) {
        // Draw bottom bar only if no hero fading animation is going.
        bottomArmyBar.Redraw( display );
    }

    // Exit button.
    fheroes2::Button buttonExit( dialogRoi.x + 553, dialogRoi.y + 428, ICN::BUTTON_EXIT_TOWN, 0, 1 );

    // Kingdom resources.
    const fheroes2::Rect & rectResource = fheroes2::drawResourcePanel( GetKingdom().GetFunds(), display, dialogRoi.getPosition() );
    const fheroes2::Rect resActiveArea( rectResource.x, rectResource.y, rectResource.width, buttonExit.area().y - rectResource.y - 3 );

    // Cache a vector of castle buildings in the order they are rendered.
    const CastleDialog::BuildingsRenderQueue cacheBuildings( *this, dialogRoi.getPosition() );

    // Render castle buildings.
    CastleDialog::redrawAllBuildings( *this, dialogRoi.getPosition(), cacheBuildings, fadeBuilding, 0 );

    if ( GetKingdom().GetCastles().size() < 2 ) {
        buttonPrevCastle.disable();
        buttonNextCastle.disable();
    }

    buttonPrevCastle.draw();
    buttonNextCastle.draw();
    buttonExit.draw();

    // Fade-in castle dialog.
    if ( fade ) {
        if ( renderBackgroundDialog && !isDefaultScreenSize ) {
            // We need to expand the ROI for the next render to properly render window borders and shadow.
            display.updateNextRenderRoi( dialogWithShadowRoi );
        }

        // Use half fade if game resolution is not 640x480.
        fheroes2::fadeInDisplay( dialogRoi, !isDefaultScreenSize );
    }
    else {
        display.render( dialogWithShadowRoi );
    }

    CastleDialogReturnValue result = CastleDialogReturnValue::DoNothing;

    // This variable must be declared out of the loop for performance reasons.
    std::string statusMessage;
    uint32_t castleAnimationIndex = 1;

    Game::passAnimationDelay( Game::CASTLE_AROUND_DELAY );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() && result == CastleDialogReturnValue::DoNothing ) {
        bool needRedraw = false;
        bool needFadeIn = false;

        // During hero purchase or building construction skip any interaction with the dialog.
        if ( alphaHero >= 255 && fadeBuilding.isFadeDone() ) {
            if ( buttonPrevCastle.isEnabled() ) {
                le.isMouseLeftButtonPressedInArea( buttonPrevCastle.area() ) ? buttonPrevCastle.drawOnPress() : buttonPrevCastle.drawOnRelease();
            }
            if ( buttonNextCastle.isEnabled() ) {
                le.isMouseLeftButtonPressedInArea( buttonNextCastle.area() ) ? buttonNextCastle.drawOnPress() : buttonNextCastle.drawOnRelease();
            }

            le.isMouseLeftButtonPressedInArea( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

            // Check buttons for closing this castle's window.
            if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
                result = CastleDialogReturnValue::Close;

                // Disable fast scroll for resolutions where the exit button is directly above the border.
                Interface::AdventureMap::Get().getGameArea().setFastScrollStatus( false );

                // Fade-out castle dialog.
                fheroes2::fadeOutDisplay( dialogRoi, !isDefaultScreenSize );
                break;
            }
            if ( buttonPrevCastle.isEnabled()
                 && ( le.MouseClickLeft( buttonPrevCastle.area() ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_LEFT ) || timedButtonPrevCastle.isDelayPassed() ) ) {
                result = CastleDialogReturnValue::PreviousCastle;
                break;
            }
            if ( buttonNextCastle.isEnabled()
                 && ( le.MouseClickLeft( buttonNextCastle.area() ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_RIGHT ) || timedButtonNextCastle.isDelayPassed() ) ) {
                result = CastleDialogReturnValue::NextCastle;
                break;
            }

            if ( le.MouseClickLeft( resActiveArea ) ) {
                const fheroes2::ButtonRestorer exitRestorer( buttonExit );

                fheroes2::showKingdomIncome( GetKingdom(), Dialog::OK );
            }
            else if ( le.isMouseRightButtonPressedInArea( resActiveArea ) ) {
                fheroes2::showKingdomIncome( GetKingdom(), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonExit.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Exit" ), _( "Exit this menu." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonNextCastle.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Show next town" ), _( "Click to show next town." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonPrevCastle.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Show previous town" ), _( "Click to show previous town." ), Dialog::ZERO );
            }
            else if ( isBuild( BUILD_CAPTAIN ) && le.isMouseRightButtonPressedInArea( rectSign1 ) ) {
                Dialog::QuickInfo( GetCaptain() );
            }
            else if ( hero && le.isMouseRightButtonPressedInArea( rectSign2 ) ) {
                Dialog::QuickInfo( *hero );
            }

            // Army bar events processing.
            if ( ( bottomArmyBar.isValid()
                   && ( ( le.isMouseCursorPosInArea( topArmyBar.GetArea() ) && topArmyBar.QueueEventProcessing( bottomArmyBar, &statusMessage ) )
                        || ( le.isMouseCursorPosInArea( bottomArmyBar.GetArea() ) && bottomArmyBar.QueueEventProcessing( topArmyBar, &statusMessage ) ) ) )
                 || ( !bottomArmyBar.isValid() && le.isMouseCursorPosInArea( topArmyBar.GetArea() ) && topArmyBar.QueueEventProcessing( &statusMessage ) ) ) {
                needRedraw = true;
            }

            // Actions with hero army.
            if ( hero ) {
                bool isArmyActionPerformed = false;

                // Preselecting of troop.
                const ArmyTroop * keep = nullptr;

                if ( topArmyBar.isSelected() ) {
                    keep = topArmyBar.GetSelectedItem();
                }
                else if ( bottomArmyBar.isSelected() ) {
                    keep = bottomArmyBar.GetSelectedItem();
                }

                if ( HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_DOWN ) ) {
                    hero->GetArmy().MoveTroops( GetArmy(), keep ? keep->GetID() : Monster::UNKNOWN );
                    isArmyActionPerformed = true;
                }
                else if ( HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_UP ) ) {
                    GetArmy().MoveTroops( hero->GetArmy(), keep ? keep->GetID() : Monster::UNKNOWN );
                    isArmyActionPerformed = true;
                }

                // Reset selection and schedule dialog redraw if any action modifying armies has been made.
                if ( isArmyActionPerformed ) {
                    if ( topArmyBar.isSelected() ) {
                        topArmyBar.ResetSelected();
                    }
                    if ( bottomArmyBar.isSelected() ) {
                        bottomArmyBar.ResetSelected();
                    }

                    needRedraw = true;
                }
            }

            if ( hero && le.MouseClickLeft( rectSign2 ) ) {
                // View hero.
                openHeroDialog( topArmyBar, bottomArmyBar, *hero );

                needFadeIn = true;
                needRedraw = true;
            }

            // Get pressed hotkey.
            const BuildingType hotKeyBuilding = getPressedBuildingHotkey();

            // Interaction with buildings.
            // Animation queue starts from the lowest by Z-value buildings which means that they draw first and most likely overlap by the top buildings in the queue.
            // In this case we must revert the queue and finding the first suitable building.
            for ( auto it = cacheBuildings.crbegin(); it != cacheBuildings.crend(); ++it ) {
                if ( !isBuild( it->id ) ) {
                    continue;
                }

                const uint32_t monsterDwelling = GetActualDwelling( it->id );
                const bool isMonsterDwelling = ( monsterDwelling != BUILD_NOTHING );

                if ( le.isMouseRightButtonPressedInArea( it->coord ) ) {
                    // Check mouse right click.
                    if ( isMonsterDwelling ) {
                        Dialog::DwellingInfo( Monster( _race, it->id ), getMonstersInDwelling( it->id ) );
                    }
                    else {
                        fheroes2::showStandardTextMessage( GetStringBuilding( it->id ), GetDescriptionBuilding( it->id ), Dialog::ZERO );
                    }

                    // Nothing we need to do after.
                    break;
                }

                const bool isMagicGuild = ( BUILD_MAGEGUILD & it->id ) != 0;

                if ( le.MouseClickLeft( it->coord ) || hotKeyBuilding == it->id || ( isMagicGuild && hotKeyBuilding == BUILD_MAGEGUILD ) ) {
                    if ( topArmyBar.isSelected() ) {
                        topArmyBar.ResetSelected();
                    }
                    if ( bottomArmyBar.isValid() && bottomArmyBar.isSelected() ) {
                        bottomArmyBar.ResetSelected();
                    }

                    if ( isMagicGuild ) {
                        const fheroes2::ButtonRestorer exitRestorer( buttonExit );

                        if ( purchaseSpellBookIfNecessary( *this, hero ) ) {
                            // Guest hero purchased the spellbook, redraw the resource panel
                            needRedraw = true;
                        }

                        _openMageGuild( hero );
                    }
                    else if ( isMonsterDwelling ) {
                        const fheroes2::ButtonRestorer exitRestorer( buttonExit );

                        const int32_t recruitMonsterWindowOffsetY = -65;
                        const Troop monsterToRecruit
                            = Dialog::RecruitMonster( Monster( _race, monsterDwelling ), getMonstersInDwelling( it->id ), true, recruitMonsterWindowOffsetY );
                        if ( Castle::RecruitMonster( monsterToRecruit ) ) {
                            needRedraw = true;
                        }
                    }
                    else {
                        switch ( it->id ) {
                        case BUILD_THIEVESGUILD:
                            Dialog::ThievesGuild( false );
                            break;

                        case BUILD_TAVERN: {
                            const fheroes2::ButtonRestorer exitRestorer( buttonExit );
                            _openTavern();
                            break;
                        }

                        case BUILD_CAPTAIN:
                        case BUILD_STATUE:
                        case BUILD_WEL2:
                        case BUILD_MOAT:
                        case BUILD_SPEC:
                        case BUILD_SHRINE: {
                            const fheroes2::ButtonRestorer exitRestorer( buttonExit );
                            fheroes2::showStandardTextMessage( GetStringBuilding( it->id ), GetDescriptionBuilding( it->id ), Dialog::OK );
                            break;
                        }

                        case BUILD_SHIPYARD: {
                            const fheroes2::ButtonRestorer exitRestorer( buttonExit );

                            if ( Dialog::BuyBoat( AllowBuyBoat( true ) ) != Dialog::OK ) {
                                break;
                            }

                            if ( !BuyBoat() ) {
                                assert( 0 );
                            }

                            fadeBuilding.startFadeBoat();
                            needRedraw = true;
                            break;
                        }

                        case BUILD_MARKETPLACE: {
                            const fheroes2::ButtonRestorer exitRestorer( buttonExit );
                            Dialog::Marketplace( GetKingdom(), false );
                            needRedraw = true;
                            break;
                        }

                        case BUILD_WELL:
                            _openWell();
                            needRedraw = true;
                            break;

                        case BUILD_TENT: {
                            const fheroes2::ButtonRestorer exitRestorer( buttonExit );
                            if ( isBuildingDisabled( BUILD_CASTLE ) ) {
                                fheroes2::showStandardTextMessage( _( "Town" ), _( "This town may not be upgraded to a castle." ), Dialog::OK );
                                break;
                            }

                            if ( DialogBuyCastle( true ) != Dialog::OK ) {
                                break;
                            }

                            if ( !BuyBuilding( BUILD_CASTLE ) ) {
                                assert( 0 );
                            }

                            AudioManager::PlaySound( M82::BUILDTWN );
                            fadeBuilding.startFadeBuilding( BUILD_CASTLE );
                            needRedraw = true;
                            break;
                        }

                        case BUILD_CASTLE: {
                            result = constructionDialogHandler();
                            needRedraw = true;
                            break;
                        }

                        default:
                            // Some new building?
                            assert( 0 );
                            break;
                        }
                    }

                    break;
                }
            }
        }

        if ( result != CastleDialogReturnValue::DoNothing ) {
            break;
        }

        if ( alphaHero < 255 && Game::validateAnimationDelay( Game::CASTLE_BUYHERO_DELAY ) ) {
            alphaHero += 10;
            if ( alphaHero >= 255 ) {
                alphaHero = 255;

                // Hero fade-in animation is finished, we can set up his army bar.
                bottomArmyBar.SetArmy( &hero->GetArmy() );
            }

            fheroes2::AlphaBlit( surfaceHero, display, dialogRoi.x, dialogRoi.y + 356, static_cast<uint8_t>( alphaHero ) );

            if ( !needRedraw ) {
                display.render( dialogRoi );
            }
        }

        if ( needRedraw ) {
            // Redraw the bottom part of the castle dialog (army bars, resources, exit button).
            topArmyBar.Redraw( display );
            if ( bottomArmyBar.isValid() && alphaHero >= 255 ) {
                bottomArmyBar.Redraw( display );
            }
            fheroes2::drawResourcePanel( GetKingdom().GetFunds(), display, dialogRoi.getPosition() );

            if ( buttonExit.isPressed() ) {
                buttonExit.draw();
            }

            if ( needFadeIn ) {
                fheroes2::fadeInDisplay( dialogRoi, !isDefaultScreenSize );
            }
            else {
                display.render( dialogRoi );
            }
        }

        // Update status bar. It doesn't depend on animation status.
        // Animation queue starts from the lowest by Z-value buildings which means that they draw first and most likely overlap by the top buildings in the queue.
        // In this case we must revert the queue and finding the first suitable building.
        for ( auto it = cacheBuildings.crbegin(); it != cacheBuildings.crend(); ++it ) {
            if ( isBuild( it->id ) && le.isMouseCursorPosInArea( it->coord ) ) {
                statusMessage = buildingStatusMessage( _race, it->id );
                break;
            }
        }

        if ( le.isMouseCursorPosInArea( buttonExit.area() ) ) {
            statusMessage = isCastle() ? _( "Exit Castle" ) : _( "Exit Town" );
        }
        else if ( le.isMouseCursorPosInArea( resActiveArea ) ) {
            statusMessage = _( "Show Income" );
        }
        else if ( buttonPrevCastle.isEnabled() && le.isMouseCursorPosInArea( buttonPrevCastle.area() ) ) {
            statusMessage = _( "Show previous town" );
        }
        else if ( buttonNextCastle.isEnabled() && le.isMouseCursorPosInArea( buttonNextCastle.area() ) ) {
            statusMessage = _( "Show next town" );
        }
        else if ( hero && le.isMouseCursorPosInArea( rectSign2 ) ) {
            statusMessage = _( "View Hero" );
        }

        if ( statusMessage.empty() ) {
            statusBar.ShowMessage( currentDate );
        }
        else {
            statusBar.ShowMessage( statusMessage );
            statusMessage.clear();
        }

        needRedraw = fadeBuilding.updateFadeAlpha();
        if ( fadeBuilding.isFadeDone() ) {
            const uint32_t build = fadeBuilding.getBuilding();

            if ( build != BUILD_NOTHING ) {
                if ( BUILD_CAPTAIN == build ) {
                    RedrawIcons( *this, hero, dialogRoi.getPosition() );
                    fheroes2::drawCastleName( *this, fheroes2::Display::instance(), dialogRoi.getPosition() );
                }

                fheroes2::drawResourcePanel( GetKingdom().GetFunds(), display, dialogRoi.getPosition() );

                display.render( dialogRoi );
            }

            fadeBuilding.stopFade();
        }
        else if ( fadeBuilding.getBuilding() == BUILD_CAPTAIN ) {
            // Fade-in the captain image while fading-in his quarters.
            const fheroes2::Sprite & crestImage = fheroes2::AGG::GetICN( ICN::CREST, Color::GetIndex( GetColor() ) );
            fheroes2::Copy( crestImage, 0, 0, display, rectSign1.x, rectSign1.y, crestImage.width(), crestImage.height() );
            const fheroes2::Sprite & captainImage = GetCaptain().GetPortrait( PORT_BIG );
            fheroes2::AlphaBlit( captainImage, display, rectSign1.x, rectSign1.y, fadeBuilding.getAlpha() );
        }

        // Castle dialog animation.
        if ( Game::validateAnimationDelay( Game::CASTLE_AROUND_DELAY ) || needRedraw ) {
            CastleDialog::redrawAllBuildings( *this, dialogRoi.getPosition(), cacheBuildings, fadeBuilding, castleAnimationIndex );

            display.render( dialogRoi );

            if ( !needRedraw ) {
                ++castleAnimationIndex;
            }
        }
    }

    Game::SetUpdateSoundsOnFocusUpdate( true );

    return result;
}
