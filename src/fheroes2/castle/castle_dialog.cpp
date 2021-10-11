/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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
#include <string>

#include "agg.h"
#include "agg_image.h"
#include "army_bar.h"
#include "castle.h"
#include "castle_building_info.h"
#include "castle_ui.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "heroes.h"
#include "icn.h"
#include "kingdom.h"
#include "m82.h"
#include "mus.h"
#include "payment.h"
#include "resource.h"
#include "settings.h"
#include "statusbar.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    uint32_t castleAnimationIndex = 0;

    building_t getPressedBuildingHotkey()
    {
        if ( HotKeyPressEvent( Game::EVENT_TOWN_DWELLING_LEVEL_1 ) ) {
            return DWELLING_MONSTER1;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_DWELLING_LEVEL_2 ) ) {
            return DWELLING_MONSTER2;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_DWELLING_LEVEL_3 ) ) {
            return DWELLING_MONSTER3;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_DWELLING_LEVEL_4 ) ) {
            return DWELLING_MONSTER4;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_DWELLING_LEVEL_5 ) ) {
            return DWELLING_MONSTER5;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_DWELLING_LEVEL_6 ) ) {
            return DWELLING_MONSTER6;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_MARKETPLACE ) ) {
            return BUILD_MARKETPLACE;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_WELL ) ) {
            return BUILD_WELL;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_MAGE_GUILD ) ) {
            return BUILD_MAGEGUILD;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_SHIPYARD ) ) {
            return BUILD_SHIPYARD;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_THIEVES_GUILD ) ) {
            return BUILD_THIEVESGUILD;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_TAVERN ) ) {
            return BUILD_TAVERN;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_JUMP_TO_BUILD_SELECTION ) ) {
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
        StringReplace( msgStatus, "%{name}", monster.GetMultiName() );
        return msgStatus;
    }

    class MeetingButton : public fheroes2::ButtonSprite
    {
    public:
        MeetingButton( const int32_t px, const int32_t py )
        {
            const fheroes2::Sprite & sprite = fheroes2::getHeroExchangeImage();
            setSprite( sprite, sprite );
            setPosition( px, py );
        }
    };

    class SwapButton : public fheroes2::ButtonSprite
    {
    public:
        SwapButton( const int32_t px, const int32_t py )
        {
            const fheroes2::Sprite & in = fheroes2::getHeroExchangeImage();
            fheroes2::Sprite sprite( in.height(), in.width() );
            Transpose( in, sprite );
            setSprite( sprite, sprite );
            setPosition( px, py );
        }
    };

    void RedrawIcons( const Castle & castle, const CastleHeroes & heroes, const fheroes2::Point & pt )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const Heroes * hero1 = heroes.Guard();
        const Heroes * hero2 = heroes.Guest();

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 0 ), display, pt.x, pt.y + 256 );

        fheroes2::Sprite icon1;
        fheroes2::Sprite icon2;

        if ( hero1 )
            icon1 = hero1->GetPortrait( PORT_BIG );
        else if ( castle.isBuild( BUILD_CAPTAIN ) )
            icon1 = castle.GetCaptain().GetPortrait( PORT_BIG );
        else
            icon1 = fheroes2::AGG::GetICN( ICN::CREST, Color::GetIndex( castle.GetColor() ) );

        if ( hero2 )
            icon2 = hero2->GetPortrait( PORT_BIG );
        else
            icon2 = fheroes2::AGG::GetICN( ICN::STRIP, 3 );

        if ( !icon1.empty() )
            fheroes2::Blit( icon1, display, pt.x + 5, pt.y + 262 );
        if ( !icon2.empty() )
            fheroes2::Blit( icon2, display, pt.x + 5, pt.y + 361 );

        if ( !hero2 )
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 11 ), display, pt.x + 112, pt.y + 361 );
    }

    std::string getDateString()
    {
        std::string output( _( "Month: %{month}, Week: %{week}, Day: %{day}" ) );
        StringReplace( output, "%{month}", world.GetMonth() );
        StringReplace( output, "%{week}", world.GetWeek() );
        StringReplace( output, "%{day}", world.GetDay() );

        return output;
    }

    void verifyMagicBookPresence( const Castle & castle, CastleHeroes & heroes )
    {
        bool noFreeSpaceForMagicBook = false;

        if ( heroes.Guard() && !heroes.Guard()->HaveSpellBook() ) {
            if ( heroes.Guard()->IsFullBagArtifacts() ) {
                noFreeSpaceForMagicBook = true;
            }
            else {
                heroes.Guard()->BuySpellBook( &castle );
            }
        }

        if ( heroes.Guest() && !heroes.Guest()->HaveSpellBook() ) {
            if ( heroes.Guest()->IsFullBagArtifacts() ) {
                noFreeSpaceForMagicBook = true;
            }
            else {
                heroes.Guest()->BuySpellBook( &castle );
            }
        }

        if ( noFreeSpaceForMagicBook ) {
            const Heroes * hero = heroes.Guard();
            if ( !hero || hero->HaveSpellBook() || !hero->IsFullBagArtifacts() )
                hero = heroes.Guest();

            Dialog::Message(
                hero->GetName(),
                _( "You must purchase a spell book to use the mage guild, but you currently have no room for a spell book. Try giving one of your artifacts to another hero." ),
                Font::BIG, Dialog::OK );
        }
    }

    void openHeroDialog( ArmyBar & army1, ArmyBar & army2, Heroes & hero )
    {
        Game::SetUpdateSoundsOnFocusUpdate( false );
        Game::OpenHeroesDialog( hero, false, false );

        if ( army2.isValid() && army1.isSelected() )
            army1.ResetSelected();
        if ( army2.isValid() && army2.isSelected() )
            army2.ResetSelected();
    }
}

int Castle::OpenDialog( bool readonly )
{
    // Fade screen.
    Settings & conf = Settings::Get();
    if ( conf.ExtGameUseFade() )
        fheroes2::FadeDisplay();

    const fheroes2::StandardWindow background( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

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

    CastleHeroes heroes = world.GetHeroes( *this );
    RedrawIcons( *this, heroes, cur_pt );

    // castle troops selector
    // castle army bar
    ArmyBar selectArmy1( ( heroes.Guard() ? &heroes.Guard()->GetArmy() : &army ), false, readonly );
    selectArmy1.SetColRows( 5, 1 );
    selectArmy1.SetPos( cur_pt.x + 112, cur_pt.y + 262 );
    selectArmy1.SetHSpace( 6 );
    selectArmy1.Redraw();

    // portrait heroes or captain or sign
    const fheroes2::Rect rectSign2( cur_pt.x + 5, cur_pt.y + 361, 100, 92 );

    // castle_heroes troops background
    ArmyBar selectArmy2( nullptr, false, readonly );
    selectArmy2.SetColRows( 5, 1 );
    selectArmy2.SetPos( cur_pt.x + 112, cur_pt.y + 361 );
    selectArmy2.SetHSpace( 6 );

    if ( heroes.Guest() ) {
        heroes.Guest()->MovePointsScaleFixed();
        selectArmy2.SetArmy( &heroes.Guest()->GetArmy() );
        selectArmy2.Redraw();
    }

    // button exit
    fheroes2::Button buttonExit( cur_pt.x + 553, cur_pt.y + 428, ICN::TREASURY, 1, 2 );

    // resource
    const fheroes2::Rect & rectResource = fheroes2::drawResourcePanel( GetKingdom().GetFunds(), display, cur_pt );
    const fheroes2::Rect resActiveArea( rectResource.x, rectResource.y, rectResource.width, buttonExit.area().y - rectResource.y - 3 );

    // button swap
    SwapButton buttonSwap( cur_pt.x + 4, cur_pt.y + 348 );
    MeetingButton buttonMeeting( cur_pt.x + 88, cur_pt.y + 346 );

    if ( heroes.Guest() && heroes.Guard() && !readonly ) {
        buttonSwap.draw();
        buttonMeeting.draw();
    }

    // fill cache buildings
    CastleDialog::CacheBuildings cacheBuildings( *this, cur_pt );

    CastleDialog::FadeBuilding fadeBuilding;

    // draw building
    CastleDialog::RedrawAllBuilding( *this, cur_pt, cacheBuildings, fadeBuilding, castleAnimationIndex );

    if ( readonly || GetKingdom().GetCastles().size() < 2 ) {
        buttonPrevCastle.disable();
        buttonNextCastle.disable();
    }

    buttonPrevCastle.draw();
    buttonNextCastle.draw();
    buttonExit.draw();

    AGG::PlayMusic( MUS::FromRace( race ), true, true );

    int result = Dialog::CANCEL;
    bool need_redraw = false;

    int alphaHero = 255;
    fheroes2::Image surfaceHero( 552, 105 );

    // dialog menu loop
    Game::passAnimationDelay( Game::CASTLE_AROUND_DELAY );

    // This variable must be declared out of the loop for performance reasons.
    std::string statusMessage;

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
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
            if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
                result = Dialog::CANCEL;
                break;
            }
            if ( buttonPrevCastle.isEnabled()
                 && ( le.MouseClickLeft( buttonPrevCastle.area() ) || HotKeyPressEvent( Game::EVENT_MOVELEFT ) || timedButtonPrevCastle.isDelayPassed() ) ) {
                result = Dialog::PREV;
                break;
            }
            if ( buttonNextCastle.isEnabled()
                 && ( le.MouseClickLeft( buttonNextCastle.area() ) || HotKeyPressEvent( Game::EVENT_MOVERIGHT ) || timedButtonNextCastle.isDelayPassed() ) ) {
                result = Dialog::NEXT;
                break;
            }

            if ( le.MouseClickLeft( resActiveArea ) ) {
                fheroes2::ButtonRestorer exitRestorer( buttonExit );
                Dialog::ResourceInfo( _( "Income" ), "", GetKingdom().GetIncome( INCOME_ALL ), Dialog::OK );
            }
            else if ( le.MousePressRight( resActiveArea ) ) {
                Dialog::ResourceInfo( _( "Income" ), "", GetKingdom().GetIncome( INCOME_ALL ), 0 );
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
            if ( ( selectArmy2.isValid()
                   && ( ( le.MouseCursor( selectArmy1.GetArea() ) && selectArmy1.QueueEventProcessing( selectArmy2, &statusMessage ) )
                        || ( le.MouseCursor( selectArmy2.GetArea() ) && selectArmy2.QueueEventProcessing( selectArmy1, &statusMessage ) ) ) )
                 || ( !selectArmy2.isValid() && le.MouseCursor( selectArmy1.GetArea() ) && selectArmy1.QueueEventProcessing( &statusMessage ) ) ) {
                need_redraw = true;
            }

            if ( conf.ExtCastleAllowGuardians() && !readonly ) {
                Army * army1 = nullptr;
                Army * army2 = nullptr;

                // swap guest <-> guardian
                if ( heroes.Guest() && heroes.Guard() ) {
                    if ( le.MouseClickLeft( buttonSwap.area() ) ) {
                        SwapCastleHeroes( heroes );
                        army1 = &heroes.Guard()->GetArmy();
                        army2 = &heroes.Guest()->GetArmy();
                    }
                    else if ( le.MouseClickLeft( buttonMeeting.area() ) ) {
                        heroes.Guest()->MeetingDialog( *heroes.Guard() );
                        need_redraw = true;
                    }
                }
                else
                    // move hero to guardian
                    if ( heroes.Guest() && !heroes.Guard() && le.MouseClickLeft( rectSign1 ) ) {
                    if ( !heroes.Guest()->GetArmy().CanJoinTroops( army ) ) {
                        // FIXME: correct message
                        Dialog::Message( _( "Join Error" ), _( "Army is full" ), Font::BIG, Dialog::OK );
                    }
                    else {
                        SwapCastleHeroes( heroes );
                        army1 = &heroes.Guard()->GetArmy();
                    }
                }
                else
                    // move guardian to hero
                    if ( !heroes.Guest() && heroes.Guard() && le.MouseClickLeft( rectSign2 ) ) {
                    SwapCastleHeroes( heroes );
                    army2 = &heroes.Guest()->GetArmy();
                }

                if ( army1 || army2 ) {
                    if ( selectArmy1.isSelected() )
                        selectArmy1.ResetSelected();
                    if ( selectArmy2.isValid() && selectArmy2.isSelected() )
                        selectArmy2.ResetSelected();

                    if ( army1 && army2 ) {
                        selectArmy1.SetArmy( army1 );
                        selectArmy2.SetArmy( army2 );
                    }
                    else if ( army1 ) {
                        selectArmy1.SetArmy( army1 );
                        selectArmy2.SetArmy( nullptr );
                    }
                    else if ( army2 ) {
                        selectArmy1.SetArmy( &army );
                        selectArmy2.SetArmy( army2 );
                    }

                    RedrawIcons( *this, heroes, cur_pt );
                    need_redraw = true;
                }
            }

            if ( !readonly && heroes.Guard() && le.MouseClickLeft( rectSign1 ) ) {
                // View guardian.
                openHeroDialog( selectArmy1, selectArmy2, *heroes.Guard() );
                need_redraw = true;
            }
            else if ( !readonly && heroes.Guest() && le.MouseClickLeft( rectSign2 ) ) {
                // View hero.
                openHeroDialog( selectArmy1, selectArmy2, *heroes.Guest() );
                need_redraw = true;
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
                    if ( selectArmy1.isSelected() )
                        selectArmy1.ResetSelected();
                    if ( selectArmy2.isValid() && selectArmy2.isSelected() )
                        selectArmy2.ResetSelected();

                    if ( readonly && ( it->id & ( BUILD_SHIPYARD | BUILD_MARKETPLACE | BUILD_WELL | BUILD_TENT | BUILD_CASTLE ) ) ) {
                        Dialog::Message( GetStringBuilding( it->id ), GetDescriptionBuilding( it->id ), Font::BIG, Dialog::OK );
                    }
                    else if ( isMagicGuild ) {
                        fheroes2::ButtonRestorer exitRestorer( buttonExit );

                        verifyMagicBookPresence( *this, heroes );

                        OpenMageGuild( heroes );
                    }
                    else if ( isMonsterDwelling ) {
                        if ( !readonly ) {
                            fheroes2::ButtonRestorer exitRestorer( buttonExit );

                            const Troop monsterToRecruit = Dialog::RecruitMonster( Monster( race, monsterDwelling ), getMonstersInDwelling( it->id ), true );
                            if ( Castle::RecruitMonster( monsterToRecruit ) ) {
                                need_redraw = true;
                            }
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
                                AGG::PlaySound( M82::BUILDTWN );
                                fadeBuilding.StartFadeBuilding( BUILD_CASTLE );
                            }
                            break;
                        }

                        case BUILD_CASTLE: {
                            uint32_t build = fadeBuilding.GetBuild();
                            if ( build != BUILD_NOTHING ) {
                                BuyBuilding( build );
                                if ( BUILD_CAPTAIN == build ) {
                                    RedrawIcons( *this, heroes, cur_pt );
                                    display.render();
                                }
                            }
                            fadeBuilding.StopFadeBuilding();
                            const Heroes * prev = heroes.Guest();
                            build = OpenTown();
                            heroes = world.GetHeroes( *this );
                            const bool buyhero = ( heroes.Guest() && ( heroes.Guest() != prev ) );

                            if ( BUILD_NOTHING != build ) {
                                AGG::PlaySound( M82::BUILDTWN );
                                fadeBuilding.StartFadeBuilding( build );
                            }

                            if ( buyhero ) {
                                if ( prev ) {
                                    selectArmy1.SetArmy( &heroes.Guard()->GetArmy() );
                                    selectArmy2.SetArmy( nullptr );
                                    RedrawIcons( *this, CastleHeroes( nullptr, heroes.Guard() ), cur_pt );
                                    selectArmy1.Redraw();
                                    if ( selectArmy2.isValid() )
                                        selectArmy2.Redraw();
                                    display.render();
                                }
                                selectArmy2.SetArmy( &heroes.Guest()->GetArmy() );
                                AGG::PlaySound( M82::BUILDTWN );

                                // animate fade in for hero army bar
                                fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 0 ), 0, 100, surfaceHero, 0, 0, 552, 107 );
                                const fheroes2::Sprite & port = heroes.Guest()->GetPortrait( PORT_BIG );
                                if ( !port.empty() )
                                    fheroes2::Blit( port, surfaceHero, 5, 5 );

                                const fheroes2::Point savept = selectArmy2.GetPos();
                                selectArmy2.SetPos( 112, 5 );
                                selectArmy2.Redraw( surfaceHero );
                                selectArmy2.SetPos( savept.x, savept.y );

                                fheroes2::drawResourcePanel( GetKingdom().GetFunds(), display, cur_pt );
                                alphaHero = 0;
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

        if ( alphaHero < 255 ) {
            if ( Game::validateAnimationDelay( Game::CASTLE_BUYHERO_DELAY ) ) {
                alphaHero += 10;
                if ( alphaHero >= 255 )
                    fheroes2::Blit( surfaceHero, display, cur_pt.x, cur_pt.y + 356 );
                else
                    fheroes2::AlphaBlit( surfaceHero, display, cur_pt.x, cur_pt.y + 356, alphaHero );
                if ( !need_redraw )
                    display.render();
            }
        }
        if ( need_redraw ) {
            selectArmy1.Redraw();
            if ( selectArmy2.isValid() && alphaHero >= 255 )
                selectArmy2.Redraw();
            fheroes2::drawCastleName( *this, display, cur_pt );
            fheroes2::drawResourcePanel( GetKingdom().GetFunds(), display, cur_pt );
            if ( heroes.Guest() && heroes.Guard() && !readonly ) {
                buttonSwap.draw();
                buttonMeeting.draw();
            }
            if ( buttonExit.isPressed() )
                buttonExit.draw();
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
        else if ( heroes.Guest() && heroes.Guard() && le.MouseCursor( buttonSwap.area() ) ) {
            statusMessage = _( "Swap Heroes" );
        }
        else if ( heroes.Guest() && heroes.Guard() && le.MouseCursor( buttonMeeting.area() ) ) {
            statusMessage = _( "Meeting Heroes" );
        }
        else if ( ( heroes.Guard() && le.MouseCursor( rectSign1 ) ) || ( heroes.Guest() && le.MouseCursor( rectSign2 ) ) ) {
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
                if ( BUILD_CAPTAIN == build )
                    RedrawIcons( *this, heroes, cur_pt );
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
