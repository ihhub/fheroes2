/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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
#include <string>
#include <vector>

#include "agg_image.h"
#include "army.h"
#include "army_bar.h"
#include "army_troop.h"
#include "audio.h"
#include "audio_manager.h"
#include "captain.h"
#include "castle.h"
#include "castle_building_info.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "heroes.h"
#include "heroes_base.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "localevent.h"
#include "m82.h"
#include "math_base.h"
#include "monster.h"
#include "mus.h"
#include "screen.h"
#include "settings.h"
#include "statusbar.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_castle.h"
#include "ui_kingdom.h"
#include "ui_tool.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    uint32_t castleAnimationIndex = 0;

    building_t getPressedBuildingHotkey()
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
            return fheroes2::getBuildingName( race, static_cast<building_t>( buildingId ) );
        }

        const Monster monster( race, buildingId );
        std::string msgStatus = _( "Recruit %{name}" );
        StringReplace( msgStatus, "%{name}", Translation::StringLower( monster.GetMultiName() ) );
        return msgStatus;
    }

    void RedrawIcons( const Castle & castle, const Heroes * hero, const fheroes2::Point & pt )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 0 ), display, pt.x, pt.y + 256 );

        if ( castle.isBuild( BUILD_CAPTAIN ) ) {
            fheroes2::Blit( castle.GetCaptain().GetPortrait( PORT_BIG ), display, pt.x + 5, pt.y + 262 );
        }
        else {
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CREST, Color::GetIndex( castle.GetColor() ) ), display, pt.x + 5, pt.y + 262 );
        }

        if ( hero ) {
            fheroes2::Blit( hero->GetPortrait( PORT_BIG ), display, pt.x + 5, pt.y + 361 );
        }
        else {
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 3 ), display, pt.x + 5, pt.y + 361 );
        }

        if ( hero == nullptr ) {
            const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( ICN::STRIP, 11 );

            fheroes2::Blit( backgroundImage, display, pt.x + 112, pt.y + 361 );
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
            Dialog::Message(
                hero->GetName(),
                _( "You must purchase a spell book to use the mage guild, but you currently have no room for a spell book. Try giving one of your artifacts to another hero." ),
                Font::BIG, Dialog::OK );

            return false;
        }

        return hero->BuySpellBook( &castle );
    }

    void openHeroDialog( ArmyBar & topArmyBar, ArmyBar & bottomArmyBar, Heroes & hero )
    {
        Game::SetUpdateSoundsOnFocusUpdate( false );
        Game::OpenHeroesDialog( hero, false, false );

        if ( topArmyBar.isValid() && topArmyBar.isSelected() )
            topArmyBar.ResetSelected();
        if ( bottomArmyBar.isValid() && bottomArmyBar.isSelected() )
            bottomArmyBar.ResetSelected();
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
        fheroes2::Blit( port, image, 5, 5 );

        ArmyBar armyBar( &hero->GetArmy(), false, false );
        armyBar.setTableSize( { 5, 1 } );
        armyBar.setRenderingOffset( { 112, 5 } );
        armyBar.setInBetweenItemsOffset( { 6, 0 } );
        armyBar.Redraw( image );
    }
}

Castle::CastleDialogReturnValue Castle::OpenDialog( const bool openConstructionWindow )
{
    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    // Fade screen.
    if ( Settings::isFadeEffectEnabled() )
        fheroes2::FadeDisplay();

    const fheroes2::StandardWindow background( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );

    AudioManager::PlayMusicAsync( MUS::FromRace( race ), Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

    int alphaHero = 255;
    CastleDialog::FadeBuilding fadeBuilding;

    Heroes * hero = world.GetHero( *this );

    fheroes2::Image surfaceHero;

    if ( openConstructionWindow && isBuild( BUILD_CASTLE ) ) {
        uint32_t build = BUILD_NOTHING;
        const ConstructionDialogResult townResult = openConstructionDialog( build );

        if ( townResult == ConstructionDialogResult::NextConstructionWindow ) {
            return CastleDialogReturnValue::NextCostructionWindow;
        }
        if ( townResult == ConstructionDialogResult::PrevConstructionWindow ) {
            return CastleDialogReturnValue::PreviousCostructionWindow;
        }

        if ( build != BUILD_NOTHING ) {
            AudioManager::PlaySound( M82::BUILDTWN );
            fadeBuilding.StartFadeBuilding( build );
        }

        if ( townResult == ConstructionDialogResult::RecruitHero ) {
            hero = world.GetHero( *this );

            generateHeroImage( surfaceHero, hero );

            AudioManager::PlaySound( M82::BUILDTWN );
            alphaHero = 0;
        }
    }

    const fheroes2::Point cur_pt( background.activeArea().x, background.activeArea().y );
    const std::string currentDate = getDateString();

    // button prev castle
    const int32_t statusBarOffsetY = 480 - 19;

    fheroes2::Button buttonPrevCastle( cur_pt.x, cur_pt.y + statusBarOffsetY, ICN::SMALLBAR, 1, 2 );
    fheroes2::TimedEventValidator timedButtonPrevCastle( [&buttonPrevCastle]() { return buttonPrevCastle.isPressed(); } );
    buttonPrevCastle.subscribe( &timedButtonPrevCastle );

    // bottom small bar
    const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( ICN::SMALLBAR, 0 );
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Blit( bar, display, cur_pt.x + buttonPrevCastle.area().width, cur_pt.y + statusBarOffsetY );

    StatusBar statusBar;
    statusBar.SetFont( Font::BIG );
    statusBar.SetCenter( cur_pt.x + buttonPrevCastle.area().width + bar.width() / 2, cur_pt.y + statusBarOffsetY + 12 );

    // button next castle
    fheroes2::Button buttonNextCastle( cur_pt.x + buttonPrevCastle.area().width + bar.width(), cur_pt.y + statusBarOffsetY, ICN::SMALLBAR, 3, 4 );
    fheroes2::TimedEventValidator timedButtonNextCastle( [&buttonNextCastle]() { return buttonNextCastle.isPressed(); } );
    buttonNextCastle.subscribe( &timedButtonNextCastle );

    // color crest
    const fheroes2::Sprite & crest = fheroes2::AGG::GetICN( ICN::CREST, Color::GetIndex( GetColor() ) );
    const fheroes2::Rect rectSign1( cur_pt.x + 5, cur_pt.y + 262, crest.width(), crest.height() );

    RedrawIcons( *this, hero, cur_pt );

    // castle troops selector
    // castle army bar
    ArmyBar topArmyBar( &army, false, false );
    topArmyBar.setTableSize( { 5, 1 } );
    topArmyBar.setRenderingOffset( { cur_pt.x + 112, cur_pt.y + 262 } );
    topArmyBar.setInBetweenItemsOffset( { 6, 0 } );
    topArmyBar.Redraw( display );

    // portrait heroes or captain or sign
    const fheroes2::Rect rectSign2( cur_pt.x + 5, cur_pt.y + 361, 100, 92 );

    // castle_heroes troops background
    ArmyBar bottomArmyBar( nullptr, false, false );
    bottomArmyBar.setTableSize( { 5, 1 } );
    bottomArmyBar.setRenderingOffset( { cur_pt.x + 112, cur_pt.y + 361 } );
    bottomArmyBar.setInBetweenItemsOffset( { 6, 0 } );

    if ( hero ) {
        bottomArmyBar.SetArmy( &hero->GetArmy() );

        if ( alphaHero != 0 ) {
            // Draw bottom bar only if no hero fading animation is going.
            bottomArmyBar.Redraw( display );
        }
    }

    // button exit
    fheroes2::Button buttonExit( cur_pt.x + 553, cur_pt.y + 428, ICN::BUTTON_SMALLER_EXIT, 0, 1 );

    // resource
    const fheroes2::Rect & rectResource = fheroes2::drawResourcePanel( GetKingdom().GetFunds(), display, cur_pt );
    const fheroes2::Rect resActiveArea( rectResource.x, rectResource.y, rectResource.width, buttonExit.area().y - rectResource.y - 3 );

    // fill cache buildings
    CastleDialog::CacheBuildings cacheBuildings( *this, cur_pt );

    // draw building
    CastleDialog::RedrawAllBuilding( *this, cur_pt, cacheBuildings, fadeBuilding, castleAnimationIndex );

    if ( GetKingdom().GetCastles().size() < 2 ) {
        buttonPrevCastle.disable();
        buttonNextCastle.disable();
    }

    buttonPrevCastle.draw();
    buttonNextCastle.draw();
    buttonExit.draw();

    CastleDialogReturnValue result = CastleDialogReturnValue::DoNothing;
    bool need_redraw = false;

    // dialog menu loop
    Game::passAnimationDelay( Game::CASTLE_AROUND_DELAY );

    // This variable must be declared out of the loop for performance reasons.
    std::string statusMessage;

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() && result == CastleDialogReturnValue::DoNothing ) {
        // During hero purchase or building construction disable any interaction
        if ( alphaHero >= 255 && fadeBuilding.IsFadeDone() ) {
            if ( buttonPrevCastle.isEnabled() ) {
                le.MousePressLeft( buttonPrevCastle.area() ) ? buttonPrevCastle.drawOnPress() : buttonPrevCastle.drawOnRelease();
            }
            if ( buttonNextCastle.isEnabled() ) {
                le.MousePressLeft( buttonNextCastle.area() ) ? buttonNextCastle.drawOnPress() : buttonNextCastle.drawOnRelease();
            }

            le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

            // Check buttons for closing this castle's window.
            if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
                result = CastleDialogReturnValue::Close;
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
                fheroes2::ButtonRestorer exitRestorer( buttonExit );

                fheroes2::showKingdomIncome( GetKingdom(), Dialog::OK );
            }
            else if ( le.MousePressRight( resActiveArea ) ) {
                fheroes2::showKingdomIncome( GetKingdom(), 0 );
            }
            else if ( le.MousePressRight( buttonExit.area() ) ) {
                Dialog::Message( _( "Exit" ), _( "Exit this menu." ), Font::BIG );
            }
            else if ( le.MousePressRight( buttonNextCastle.area() ) ) {
                Dialog::Message( _( "Show next town" ), _( "Click to show next town." ), Font::BIG );
            }
            else if ( le.MousePressRight( buttonPrevCastle.area() ) ) {
                Dialog::Message( _( "Show previous town" ), _( "Click to show previous town." ), Font::BIG );
            }

            // selector troops event
            if ( ( bottomArmyBar.isValid()
                   && ( ( le.MouseCursor( topArmyBar.GetArea() ) && topArmyBar.QueueEventProcessing( bottomArmyBar, &statusMessage ) )
                        || ( le.MouseCursor( bottomArmyBar.GetArea() ) && bottomArmyBar.QueueEventProcessing( topArmyBar, &statusMessage ) ) ) )
                 || ( !bottomArmyBar.isValid() && le.MouseCursor( topArmyBar.GetArea() ) && topArmyBar.QueueEventProcessing( &statusMessage ) ) ) {
                need_redraw = true;
            }

            // Actions with hero armies.
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

                // Redraw and reset if any action modifying armies has been made.
                if ( isArmyActionPerformed ) {
                    if ( topArmyBar.isSelected() ) {
                        topArmyBar.ResetSelected();
                    }
                    if ( bottomArmyBar.isSelected() ) {
                        bottomArmyBar.ResetSelected();
                    }

                    need_redraw = true;
                }
            }

            if ( hero && le.MouseClickLeft( rectSign2 ) ) {
                // View hero.
                openHeroDialog( topArmyBar, bottomArmyBar, *hero );
                need_redraw = true;
            }

            if ( isBuild( BUILD_CAPTAIN ) && le.MousePressRight( rectSign1 ) ) {
                Dialog::QuickInfo( GetCaptain() );
            }
            else if ( hero && le.MousePressRight( rectSign2 ) ) {
                Dialog::QuickInfo( *hero );
            }

            // Get pressed hotkey.
            const building_t hotKeyBuilding = getPressedBuildingHotkey();

            // Interaction with buildings.
            // Animation queue starts from the lowest by Z-value buildings which means that they draw first and most likely overlap by the top buildings in the queue.
            // In this case we must revert the queue and finding the first suitable building.
            for ( auto it = cacheBuildings.crbegin(); it != cacheBuildings.crend(); ++it ) {
                if ( !isBuild( it->id ) ) {
                    continue;
                }

                const uint32_t monsterDwelling = GetActualDwelling( it->id );
                const bool isMonsterDwelling = ( monsterDwelling != BUILD_NOTHING );

                if ( le.MousePressRight( it->coord ) ) {
                    // Check mouse right click.
                    if ( isMonsterDwelling ) {
                        Dialog::DwellingInfo( Monster( race, it->id ), getMonstersInDwelling( it->id ) );
                    }
                    else {
                        Dialog::Message( GetStringBuilding( it->id ), GetDescriptionBuilding( it->id ), Font::BIG );
                    }

                    // Nothing we need to do after.
                    break;
                }

                const bool isMagicGuild = ( BUILD_MAGEGUILD & it->id ) != 0;

                if ( le.MouseClickLeft( it->coord ) || hotKeyBuilding == it->id || ( isMagicGuild && hotKeyBuilding == BUILD_MAGEGUILD ) ) {
                    if ( topArmyBar.isSelected() )
                        topArmyBar.ResetSelected();
                    if ( bottomArmyBar.isValid() && bottomArmyBar.isSelected() )
                        bottomArmyBar.ResetSelected();

                    if ( isMagicGuild ) {
                        fheroes2::ButtonRestorer exitRestorer( buttonExit );

                        if ( purchaseSpellBookIfNecessary( *this, hero ) ) {
                            // Guest hero purchased the spellbook, redraw the resource panel
                            need_redraw = true;
                        }

                        OpenMageGuild( hero );
                    }
                    else if ( isMonsterDwelling ) {
                        fheroes2::ButtonRestorer exitRestorer( buttonExit );

                        const int32_t recruitMonsterWindowOffsetY = -65;
                        const Troop monsterToRecruit
                            = Dialog::RecruitMonster( Monster( race, monsterDwelling ), getMonstersInDwelling( it->id ), true, recruitMonsterWindowOffsetY );
                        if ( Castle::RecruitMonster( monsterToRecruit ) ) {
                            need_redraw = true;
                        }
                    }
                    else {
                        switch ( it->id ) {
                        case BUILD_THIEVESGUILD:
                            Dialog::ThievesGuild( false );
                            break;

                        case BUILD_TAVERN: {
                            fheroes2::ButtonRestorer exitRestorer( buttonExit );
                            OpenTavern();
                            break;
                        }

                        case BUILD_CAPTAIN:
                        case BUILD_STATUE:
                        case BUILD_WEL2:
                        case BUILD_MOAT:
                        case BUILD_SPEC:
                        case BUILD_SHRINE: {
                            fheroes2::ButtonRestorer exitRestorer( buttonExit );
                            Dialog::Message( GetStringBuilding( it->id ), GetDescriptionBuilding( it->id ), Font::BIG, Dialog::OK );
                            break;
                        }

                        case BUILD_SHIPYARD: {
                            fheroes2::ButtonRestorer exitRestorer( buttonExit );
                            if ( Dialog::OK == Dialog::BuyBoat( AllowBuyBoat() ) ) {
                                BuyBoat();
                                fadeBuilding.StartFadeBuilding( BUILD_SHIPYARD );
                            }
                            break;
                        }

                        case BUILD_MARKETPLACE: {
                            fheroes2::ButtonRestorer exitRestorer( buttonExit );
                            Dialog::Marketplace( GetKingdom(), false );
                            need_redraw = true;
                            break;
                        }

                        case BUILD_WELL:
                            OpenWell();
                            need_redraw = true;
                            break;

                        case BUILD_TENT: {
                            fheroes2::ButtonRestorer exitRestorer( buttonExit );
                            if ( !Modes( ALLOWCASTLE ) ) {
                                Dialog::Message( _( "Town" ), _( "This town may not be upgraded to a castle." ), Font::BIG, Dialog::OK );
                            }
                            else if ( Dialog::OK == DialogBuyCastle( true ) ) {
                                AudioManager::PlaySound( M82::BUILDTWN );
                                fadeBuilding.StartFadeBuilding( BUILD_CASTLE );
                            }
                            break;
                        }

                        case BUILD_CASTLE: {
                            uint32_t build = BUILD_NOTHING;
                            const ConstructionDialogResult townResult = openConstructionDialog( build );

                            if ( townResult == ConstructionDialogResult::NextConstructionWindow ) {
                                result = CastleDialogReturnValue::NextCostructionWindow;
                                break;
                            }
                            if ( townResult == ConstructionDialogResult::PrevConstructionWindow ) {
                                result = CastleDialogReturnValue::PreviousCostructionWindow;
                                break;
                            }

                            if ( build != BUILD_NOTHING ) {
                                AudioManager::PlaySound( M82::BUILDTWN );
                                fadeBuilding.StartFadeBuilding( build );
                            }

                            hero = world.GetHero( *this );

                            if ( townResult == ConstructionDialogResult::RecruitHero ) {
                                AudioManager::PlaySound( M82::BUILDTWN );

                                bottomArmyBar.SetArmy( &hero->GetArmy() );
                                generateHeroImage( surfaceHero, hero );

                                alphaHero = 0;
                                need_redraw = true;
                            }
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

        if ( alphaHero < 255 ) {
            if ( Game::validateAnimationDelay( Game::CASTLE_BUYHERO_DELAY ) ) {
                alphaHero += 10;
                if ( alphaHero >= 255 ) {
                    alphaHero = 255;
                }

                fheroes2::AlphaBlit( surfaceHero, display, cur_pt.x, cur_pt.y + 356, static_cast<uint8_t>( alphaHero ) );

                if ( !need_redraw ) {
                    display.render();
                }
            }
        }

        if ( need_redraw ) {
            topArmyBar.Redraw( display );
            if ( bottomArmyBar.isValid() && alphaHero >= 255 )
                bottomArmyBar.Redraw( display );
            fheroes2::drawCastleName( *this, display, cur_pt );
            fheroes2::drawResourcePanel( GetKingdom().GetFunds(), display, cur_pt );

            if ( buttonExit.isPressed() ) {
                buttonExit.draw();
            }

            display.render();
        }

        // Update status bar. It doesn't depend on animation status.
        // Animation queue starts from the lowest by Z-value buildings which means that they draw first and most likely overlap by the top buildings in the queue.
        // In this case we must revert the queue and finding the first suitable building.
        for ( auto it = cacheBuildings.crbegin(); it != cacheBuildings.crend(); ++it ) {
            if ( isBuild( it->id ) && le.MouseCursor( it->coord ) ) {
                statusMessage = buildingStatusMessage( race, it->id );
                break;
            }
        }

        if ( le.MouseCursor( buttonExit.area() ) ) {
            statusMessage = isCastle() ? _( "Exit Castle" ) : _( "Exit Town" );
        }
        else if ( le.MouseCursor( resActiveArea ) ) {
            statusMessage = _( "Show Income" );
        }
        else if ( buttonPrevCastle.isEnabled() && le.MouseCursor( buttonPrevCastle.area() ) ) {
            statusMessage = _( "Show previous town" );
        }
        else if ( buttonNextCastle.isEnabled() && le.MouseCursor( buttonNextCastle.area() ) ) {
            statusMessage = _( "Show next town" );
        }
        else if ( hero && le.MouseCursor( rectSign2 ) ) {
            statusMessage = _( "View Hero" );
        }

        if ( statusMessage.empty() ) {
            statusBar.ShowMessage( currentDate );
        }
        else {
            statusBar.ShowMessage( statusMessage );
            statusMessage.clear();
        }

        need_redraw = fadeBuilding.UpdateFadeBuilding();
        if ( fadeBuilding.IsFadeDone() ) {
            const uint32_t build = fadeBuilding.GetBuild();

            if ( build != BUILD_NOTHING ) {
                BuyBuilding( build );
                if ( BUILD_CAPTAIN == build ) {
                    RedrawIcons( *this, hero, cur_pt );
                }

                fheroes2::drawCastleName( *this, display, cur_pt );
                fheroes2::drawResourcePanel( GetKingdom().GetFunds(), display, cur_pt );

                display.render();
            }

            fadeBuilding.StopFadeBuilding();
        }
        // animation sprite
        if ( Game::validateAnimationDelay( Game::CASTLE_AROUND_DELAY ) ) {
            CastleDialog::RedrawAllBuilding( *this, cur_pt, cacheBuildings, fadeBuilding, castleAnimationIndex );
            display.render();

            ++castleAnimationIndex;
        }
        else if ( need_redraw ) {
            CastleDialog::RedrawAllBuilding( *this, cur_pt, cacheBuildings, fadeBuilding, castleAnimationIndex );
            display.render();
            need_redraw = false;
        }
    }

    const uint32_t build = fadeBuilding.GetBuild();
    if ( build != BUILD_NOTHING ) {
        BuyBuilding( build );
    }

    Game::SetUpdateSoundsOnFocusUpdate( true );

    return result;
}
