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

#include <array>
#include <string>

#include "agg.h"
#include "agg_image.h"
#include "army_bar.h"
#include "castle.h"
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

void CastleRedrawTownName( const Castle & castle, const fheroes2::Point & dst );

namespace
{
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
}

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

fheroes2::Sprite GetMeetingSprite()
{
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::ADVMCO, 8 );

    fheroes2::Sprite result( sprite.width() + 4, sprite.height() + 4 );
    result.fill( 0 );

    fheroes2::DrawBorder( result, fheroes2::GetColorId( 0xe0, 0xb4, 0 ) );
    fheroes2::Blit( sprite, result, 2, 2 );

    return result;
}

MeetingButton::MeetingButton( s32 px, s32 py )
{
    const fheroes2::Sprite & sprite = GetMeetingSprite();
    setSprite( sprite, sprite );
    setPosition( px, py );
}

SwapButton::SwapButton( s32 px, s32 py )
{
    const fheroes2::Sprite & in = GetMeetingSprite();
    fheroes2::Sprite sprite( in.height(), in.width() );
    Transpose( in, sprite );
    setSprite( sprite, sprite );
    setPosition( px, py );
}

int Castle::OpenDialog( bool readonly )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Settings & conf = Settings::Get();

    CastleHeroes heroes = world.GetHeroes( *this );

    // fade
    if ( conf.ExtGameUseFade() )
        fheroes2::FadeDisplay();

    const fheroes2::StandardWindow background( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::Point cur_pt( background.activeArea().x, background.activeArea().y );
    fheroes2::Point dst_pt( cur_pt.x, cur_pt.y );
    std::string msg_date, msg_status;

    // date string
    msg_date = _( "Month: %{month}, Week: %{week}, Day: %{day}" );
    StringReplace( msg_date, "%{month}", world.GetMonth() );
    StringReplace( msg_date, "%{week}", world.GetWeek() );
    StringReplace( msg_date, "%{day}", world.GetDay() );

    // button prev castle
    dst_pt.y += 480 - 19;
    fheroes2::Button buttonPrevCastle( dst_pt.x, dst_pt.y, ICN::SMALLBAR, 1, 2 );
    fheroes2::TimedEventValidator timedButtonPrevCastle( [&buttonPrevCastle]() { return buttonPrevCastle.isPressed(); } );
    buttonPrevCastle.subscribe( &timedButtonPrevCastle );

    // bottom small bar
    const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( ICN::SMALLBAR, 0 );
    dst_pt.x += buttonPrevCastle.area().width;
    fheroes2::Blit( bar, display, dst_pt.x, dst_pt.y );

    StatusBar statusBar;
    statusBar.SetFont( Font::BIG );
    statusBar.SetCenter( dst_pt.x + bar.width() / 2, dst_pt.y + 12 );

    // button next castle
    dst_pt.x += bar.width();
    fheroes2::Button buttonNextCastle( dst_pt.x, dst_pt.y, ICN::SMALLBAR, 3, 4 );
    fheroes2::TimedEventValidator timedButtonNextCastle( [&buttonNextCastle]() { return buttonNextCastle.isPressed(); } );
    buttonNextCastle.subscribe( &timedButtonNextCastle );

    // color crest
    const fheroes2::Sprite & crest = fheroes2::AGG::GetICN( ICN::CREST, Color::GetIndex( GetColor() ) );
    dst_pt.x = cur_pt.x + 5;
    dst_pt.y = cur_pt.y + 262;
    const fheroes2::Rect rectSign1( dst_pt.x, dst_pt.y, crest.width(), crest.height() );

    RedrawIcons( *this, heroes, cur_pt );

    // castle troops selector
    dst_pt.x = cur_pt.x + 112;
    dst_pt.y = cur_pt.y + 262;

    // castle army bar
    ArmyBar selectArmy1( ( heroes.Guard() ? &heroes.Guard()->GetArmy() : &army ), false, readonly );
    selectArmy1.SetColRows( 5, 1 );
    selectArmy1.SetPos( dst_pt.x, dst_pt.y );
    selectArmy1.SetHSpace( 6 );
    selectArmy1.Redraw();

    // portrait heroes or captain or sign
    dst_pt.x = cur_pt.x + 5;
    dst_pt.y = cur_pt.y + 361;

    const fheroes2::Rect rectSign2( dst_pt.x, dst_pt.y, 100, 92 );

    // castle_heroes troops background
    dst_pt.x = cur_pt.x + 112;
    dst_pt.y = cur_pt.y + 361;

    ArmyBar selectArmy2( nullptr, false, readonly );
    selectArmy2.SetColRows( 5, 1 );
    selectArmy2.SetPos( dst_pt.x, dst_pt.y );
    selectArmy2.SetHSpace( 6 );

    if ( heroes.Guest() ) {
        heroes.Guest()->MovePointsScaleFixed();
        selectArmy2.SetArmy( &heroes.Guest()->GetArmy() );
        selectArmy2.Redraw();
    }

    // button exit
    dst_pt.x = cur_pt.x + 553;
    dst_pt.y = cur_pt.y + 428;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::TREASURY, 1, 2 );

    // resource
    const fheroes2::Rect & rectResource = RedrawResourcePanel( cur_pt );
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
    CastleDialog::RedrawAllBuilding( *this, cur_pt, cacheBuildings, fadeBuilding );

    if ( 2 > world.GetKingdom( GetColor() ).GetCastles().size() || readonly ) {
        buttonPrevCastle.disable();
        buttonNextCastle.disable();
    }

    buttonPrevCastle.draw();
    buttonNextCastle.draw();
    buttonExit.draw();

    AGG::PlayMusic( MUS::FromRace( race ), true, true );

    LocalEvent & le = LocalEvent::Get();

    bool firstDraw = true;

    int result = Dialog::CANCEL;
    bool need_redraw = false;

    int alphaHero = 255;
    fheroes2::Image surfaceHero( 552, 105 );

    // dialog menu loop
    while ( le.HandleEvents() ) {
        // During hero purchase or building construction disable any interaction
        if ( alphaHero >= 255 && fadeBuilding.IsFadeDone() ) {
            // exit
            if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
                result = Dialog::CANCEL;
                break;
            }

            if ( buttonPrevCastle.isEnabled() )
                le.MousePressLeft( buttonPrevCastle.area() ) ? buttonPrevCastle.drawOnPress() : buttonPrevCastle.drawOnRelease();
            if ( buttonNextCastle.isEnabled() )
                le.MousePressLeft( buttonNextCastle.area() ) ? buttonNextCastle.drawOnPress() : buttonNextCastle.drawOnRelease();

            le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

            if ( le.MouseClickLeft( resActiveArea ) ) {
                fheroes2::ButtonRestorer exitRestorer( buttonExit );
                Dialog::ResourceInfo( _( "Income" ), "", world.GetKingdom( GetColor() ).GetIncome( INCOME_ALL ), Dialog::OK );
            }
            else if ( le.MousePressRight( resActiveArea ) ) {
                Dialog::ResourceInfo( _( "Income" ), "", world.GetKingdom( GetColor() ).GetIncome( INCOME_ALL ), 0 );
            }

            // selector troops event
            if ( ( selectArmy2.isValid()
                   && ( ( le.MouseCursor( selectArmy1.GetArea() ) && selectArmy1.QueueEventProcessing( selectArmy2, &msg_status ) )
                        || ( le.MouseCursor( selectArmy2.GetArea() ) && selectArmy2.QueueEventProcessing( selectArmy1, &msg_status ) ) ) )
                 || ( !selectArmy2.isValid() && le.MouseCursor( selectArmy1.GetArea() ) && selectArmy1.QueueEventProcessing( &msg_status ) ) ) {
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

            // view guardian
            if ( !readonly && heroes.Guard() && le.MouseClickLeft( rectSign1 ) ) {
                Game::SetUpdateSoundsOnFocusUpdate( false );
                Game::OpenHeroesDialog( *heroes.Guard(), false, false );

                if ( selectArmy1.isSelected() )
                    selectArmy1.ResetSelected();
                if ( selectArmy2.isValid() && selectArmy2.isSelected() )
                    selectArmy2.ResetSelected();

                need_redraw = true;
            }
            else
                // view hero
                if ( !readonly && heroes.Guest() && le.MouseClickLeft( rectSign2 ) ) {
                Game::SetUpdateSoundsOnFocusUpdate( false );
                Game::OpenHeroesDialog( *heroes.Guest(), false, false );

                if ( selectArmy1.isSelected() )
                    selectArmy1.ResetSelected();
                if ( selectArmy2.isValid() && selectArmy2.isSelected() )
                    selectArmy2.ResetSelected();

                need_redraw = true;
            }

            // prev castle
            if ( buttonPrevCastle.isEnabled()
                 && ( le.MouseClickLeft( buttonPrevCastle.area() ) || HotKeyPressEvent( Game::EVENT_MOVELEFT ) || timedButtonPrevCastle.isDelayPassed() ) ) {
                result = Dialog::PREV;
                break;
            }
            else
                // next castle
                if ( buttonNextCastle.isEnabled()
                     && ( le.MouseClickLeft( buttonNextCastle.area() ) || HotKeyPressEvent( Game::EVENT_MOVERIGHT ) || timedButtonNextCastle.isDelayPassed() ) ) {
                result = Dialog::NEXT;
                break;
            }

            // buildings event
            for ( auto it = cacheBuildings.crbegin(); it != cacheBuildings.crend(); ++it ) {
                const uint32_t actualBuildingID = GetActualDwelling( ( *it ).id );
                const bool isBuildingHotkeyPressed = actualBuildingID == GetActualDwelling( getPressedBuildingHotkey() );

                if ( ( *it ).id == actualBuildingID && isBuild( ( *it ).id ) ) {
                    if ( !readonly && ( le.MouseClickLeft( ( *it ).coord ) || isBuildingHotkeyPressed ) ) {
                        fheroes2::ButtonRestorer exitRestorer( buttonExit );
                        if ( Castle::RecruitMonster(
                                 Dialog::RecruitMonster( Monster( race, GetActualDwelling( ( *it ).id ) ), getMonstersInDwelling( ( *it ).id ), true ) ) ) {
                            need_redraw = true;
                        }
                    }
                    else if ( le.MousePressRight( ( *it ).coord ) ) {
                        Dialog::DwellingInfo( Monster( race, GetActualDwelling( ( *it ).id ) ), getMonstersInDwelling( ( *it ).id ) );
                    }

                    if ( le.MouseCursor( ( *it ).coord ) )
                        msg_status = Monster( race, ( *it ).id ).GetName();
                }
            }

            for ( auto it = cacheBuildings.cbegin(); it != cacheBuildings.cend(); ++it ) {
                if ( BUILD_MAGEGUILD & ( *it ).id ) {
                    const int mageGuildLevel = GetLevelMageGuild();
                    if ( mageGuildLevel > 0 && ( *it ).id == ( BUILD_MAGEGUILD1 << ( mageGuildLevel - 1 ) ) ) {
                        if ( le.MouseClickLeft( ( *it ).coord ) || getPressedBuildingHotkey() == BUILD_MAGEGUILD ) {
                            fheroes2::ButtonRestorer exitRestorer( buttonExit );
                            bool noFreeSpaceForMagicBook = false;

                            if ( heroes.Guard() && !heroes.Guard()->HaveSpellBook() ) {
                                if ( heroes.Guard()->IsFullBagArtifacts() ) {
                                    noFreeSpaceForMagicBook = true;
                                }
                                else if ( heroes.Guard()->BuySpellBook( this ) ) {
                                    need_redraw = true;
                                }
                            }

                            if ( heroes.Guest() && !heroes.Guest()->HaveSpellBook() ) {
                                if ( heroes.Guest()->IsFullBagArtifacts() ) {
                                    noFreeSpaceForMagicBook = true;
                                }
                                else if ( heroes.Guest()->BuySpellBook( this ) ) {
                                    need_redraw = true;
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

                            OpenMageGuild( heroes );
                        }
                        else if ( le.MousePressRight( ( *it ).coord ) )
                            Dialog::Message( GetStringBuilding( ( *it ).id ), GetDescriptionBuilding( ( *it ).id ), Font::BIG );

                        if ( le.MouseCursor( ( *it ).coord ) )
                            msg_status = GetStringBuilding( ( *it ).id );
                    }
                }
                else if ( isBuild( ( *it ).id ) ) {
                    if ( le.MouseClickLeft( ( *it ).coord ) || getPressedBuildingHotkey() == ( *it ).id ) {
                        if ( selectArmy1.isSelected() )
                            selectArmy1.ResetSelected();
                        if ( selectArmy2.isValid() && selectArmy2.isSelected() )
                            selectArmy2.ResetSelected();

                        if ( readonly && ( ( *it ).id & ( BUILD_SHIPYARD | BUILD_MARKETPLACE | BUILD_WELL | BUILD_TENT | BUILD_CASTLE ) ) )
                            Dialog::Message( GetStringBuilding( ( *it ).id ), GetDescriptionBuilding( ( *it ).id ), Font::BIG, Dialog::OK );
                        else
                            switch ( ( *it ).id ) {
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
                                Dialog::Message( GetStringBuilding( ( *it ).id ), GetDescriptionBuilding( ( *it ).id ), Font::BIG, Dialog::OK );
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
                                Dialog::Marketplace( world.GetKingdom( GetColor() ), false );
                                need_redraw = true;
                                break;
                            }

                            case BUILD_WELL:
                                OpenWell();
                                need_redraw = true;
                                break;

                            case BUILD_TENT: {
                                fheroes2::ButtonRestorer exitRestorer( buttonExit );
                                if ( !Modes( ALLOWCASTLE ) )
                                    Dialog::Message( _( "Town" ), _( "This town may not be upgraded to a castle." ), Font::BIG, Dialog::OK );
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

                                    RedrawResourcePanel( cur_pt );
                                    alphaHero = 0;
                                }
                                break;
                            }

                            default:
                                break;
                            }
                    }
                    else if ( le.MousePressRight( ( *it ).coord ) )
                        Dialog::Message( GetStringBuilding( ( *it ).id ), GetDescriptionBuilding( ( *it ).id ), Font::BIG );

                    if ( le.MouseCursor( ( *it ).coord ) ) {
                        msg_status = buildingStatusMessage( ( *it ).id );
                    }
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
            CastleRedrawTownName( *this, cur_pt );
            RedrawResourcePanel( cur_pt );
            if ( heroes.Guest() && heroes.Guard() && !readonly ) {
                buttonSwap.draw();
                buttonMeeting.draw();
            }
            if ( buttonExit.isPressed() )
                buttonExit.draw();
            display.render();
        }

        // status message exit
        if ( le.MouseCursor( buttonExit.area() ) )
            msg_status = isCastle() ? _( "Exit Castle" ) : _( "Exit Town" );
        else if ( le.MouseCursor( resActiveArea ) )
            msg_status = _( "Show Income" );
        else
            // status message prev castle
            if ( buttonPrevCastle.isEnabled() && le.MouseCursor( buttonPrevCastle.area() ) )
            msg_status = _( "Show previous town" );
        else
            // status message next castle
            if ( buttonNextCastle.isEnabled() && le.MouseCursor( buttonNextCastle.area() ) )
            msg_status = _( "Show next town" );
        else if ( heroes.Guest() && heroes.Guard() && le.MouseCursor( buttonSwap.area() ) )
            msg_status = _( "Swap Heroes" );
        else if ( heroes.Guest() && heroes.Guard() && le.MouseCursor( buttonMeeting.area() ) )
            msg_status = _( "Meeting Heroes" );
        else
            // status message over sign
            if ( ( heroes.Guard() && le.MouseCursor( rectSign1 ) ) || ( heroes.Guest() && le.MouseCursor( rectSign2 ) ) )
            msg_status = _( "View Hero" );

        if ( msg_status.empty() )
            statusBar.ShowMessage( msg_date );
        else {
            statusBar.ShowMessage( msg_status );
            msg_status.clear();
        }

        need_redraw = fadeBuilding.UpdateFadeBuilding();
        if ( fadeBuilding.IsFadeDone() ) {
            const uint32_t build = fadeBuilding.GetBuild();
            if ( build != BUILD_NOTHING ) {
                BuyBuilding( build );
                if ( BUILD_CAPTAIN == build )
                    RedrawIcons( *this, heroes, cur_pt );
                CastleRedrawTownName( *this, cur_pt );
                RedrawResourcePanel( cur_pt );
                display.render();
            }
            fadeBuilding.StopFadeBuilding();
        }
        // animation sprite
        if ( firstDraw || Game::validateAnimationDelay( Game::CASTLE_AROUND_DELAY ) ) {
            firstDraw = false;
            CastleDialog::RedrawAllBuilding( *this, cur_pt, cacheBuildings, fadeBuilding );
            display.render();

            Game::CastleAnimationFrame() += 1; // this function returns variable by reference
        }
        else if ( need_redraw ) {
            CastleDialog::RedrawAllBuilding( *this, cur_pt, cacheBuildings, fadeBuilding );
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

fheroes2::Rect Castle::RedrawResourcePanel( const fheroes2::Point & pt ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Funds & kingdomTreasures = world.GetKingdom( GetColor() ).GetFunds();

    const fheroes2::Rect roi( pt.x + 552, pt.y + 262, 82, 192 );
    fheroes2::Fill( display, roi.x, roi.y, roi.width, roi.height, 0 );

    // Maximum width is 39 pixels (except gold), maximum height is 32 pixels
    const int32_t maxWidth = 39;
    const int32_t maxHeight = 32;
    const int32_t leftColumnOffset = roi.x + 1;
    const int32_t rightColumnOffset = roi.x + 1 + maxWidth + 2;

    const fheroes2::FontType fontType( fheroes2::FontSize::SMALL, fheroes2::FontColor::WHITE );
    const int32_t fontHeight = fheroes2::Text( std::string(), fontType ).height();

    const std::array<int32_t, 4> offsetY = { 0, maxHeight + fontHeight + 2, ( maxHeight + fontHeight ) * 2 - 1, ( maxHeight + fontHeight ) * 3 + 1 };

    const fheroes2::Sprite & woodImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 0 );
    const fheroes2::Sprite & mercuryImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 1 );
    const fheroes2::Sprite & oreImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 2 );
    const fheroes2::Sprite & sulfurImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 3 );
    const fheroes2::Sprite & crystalImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 4 );
    const fheroes2::Sprite & gemsImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 5 );
    const fheroes2::Sprite & goldImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 6 );

    fheroes2::Blit( woodImage, display, leftColumnOffset + ( maxWidth - woodImage.width() ) / 2, roi.y + offsetY[0] + maxHeight - woodImage.height() );
    fheroes2::Blit( sulfurImage, display, rightColumnOffset + ( maxWidth - sulfurImage.width() ) / 2, roi.y + offsetY[0] + maxHeight - sulfurImage.height() );

    fheroes2::Blit( crystalImage, display, leftColumnOffset + ( maxWidth - crystalImage.width() ) / 2, roi.y + offsetY[1] + maxHeight - crystalImage.height() );
    fheroes2::Blit( mercuryImage, display, rightColumnOffset + ( maxWidth - mercuryImage.width() ) / 2, roi.y + offsetY[1] + maxHeight - mercuryImage.height() );

    fheroes2::Blit( oreImage, display, leftColumnOffset + ( maxWidth - oreImage.width() ) / 2, roi.y + offsetY[2] + maxHeight - oreImage.height() );
    fheroes2::Blit( gemsImage, display, rightColumnOffset + ( maxWidth - gemsImage.width() ) / 2, roi.y + offsetY[2] + maxHeight - gemsImage.height() );

    fheroes2::Blit( goldImage, display, roi.x + ( roi.width - goldImage.width() ) / 2, roi.y + offsetY[3] );

    fheroes2::Text text;
    text.set( std::to_string( kingdomTreasures.wood ), fontType );
    text.draw( leftColumnOffset + ( maxWidth - text.width() ) / 2, roi.y + offsetY[0] + maxHeight + 1, display );

    text.set( std::to_string( kingdomTreasures.sulfur ), fontType );
    text.draw( rightColumnOffset + ( maxWidth - text.width() ) / 2, roi.y + offsetY[0] + maxHeight + 1, display );

    text.set( std::to_string( kingdomTreasures.crystal ), fontType );
    text.draw( leftColumnOffset + ( maxWidth - text.width() ) / 2, roi.y + offsetY[1] + maxHeight + 1, display );

    text.set( std::to_string( kingdomTreasures.mercury ), fontType );
    text.draw( rightColumnOffset + ( maxWidth - text.width() ) / 2, roi.y + offsetY[1] + maxHeight + 1, display );

    text.set( std::to_string( kingdomTreasures.ore ), fontType );
    text.draw( leftColumnOffset + ( maxWidth - text.width() ) / 2, roi.y + offsetY[2] + maxHeight + 1, display );

    text.set( std::to_string( kingdomTreasures.gems ), fontType );
    text.draw( rightColumnOffset + ( maxWidth - text.width() ) / 2, roi.y + offsetY[2] + maxHeight + 1, display );

    text.set( std::to_string( kingdomTreasures.gold ), fontType );
    text.draw( roi.x + ( roi.width - text.width() ) / 2, roi.y + offsetY[3] + goldImage.height() + 1, display );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::TREASURY, 1 ), display, roi.x + 1, roi.y + 166 );

    return roi;
}
