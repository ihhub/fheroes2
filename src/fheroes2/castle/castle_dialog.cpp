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

#include <algorithm>
#include <string>
#include <utility>

#include "agg.h"
#include "army_bar.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "heroes.h"
#include "kingdom.h"
#include "m82.h"
#include "mus.h"
#include "payment.h"
#include "profit.h"
#include "race.h"
#include "resource.h"
#include "settings.h"
#include "statusbar.h"
#include "text.h"
#include "tools.h"
#include "ui_tool.h"
#include "world.h"

void CastleRedrawTownName( const Castle & castle, const Point & dst );

bool AllowFlashBuilding( u32 build )
{
    switch ( build ) {
    case BUILD_TAVERN:
    case BUILD_SHRINE:
    case BUILD_SHIPYARD:
    case BUILD_WELL:
    case BUILD_STATUE:
    case BUILD_LEFTTURRET:
    case BUILD_RIGHTTURRET:
    case BUILD_MARKETPLACE:
    case BUILD_WEL2:
    case BUILD_MOAT:
    case BUILD_SPEC:
    case BUILD_CASTLE:
    case BUILD_CAPTAIN:
    case BUILD_MAGEGUILD1:
    case BUILD_MAGEGUILD2:
    case BUILD_MAGEGUILD3:
    case BUILD_MAGEGUILD4:
    case BUILD_MAGEGUILD5:
    case BUILD_TENT:
    case DWELLING_UPGRADE2:
    case DWELLING_UPGRADE3:
    case DWELLING_UPGRADE4:
    case DWELLING_UPGRADE5:
    case DWELLING_UPGRADE6:
    case DWELLING_UPGRADE7:
    case DWELLING_MONSTER1:
    case DWELLING_MONSTER2:
    case DWELLING_MONSTER3:
    case DWELLING_MONSTER4:
    case DWELLING_MONSTER5:
    case DWELLING_MONSTER6:
        return true;

    default:
        break;
    }

    return false;
}

fheroes2::Sprite GetActualSpriteBuilding( const Castle & castle, u32 build )
{
    u32 index = 0;
    // correct index (mage guild)
    switch ( build ) {
    case BUILD_MAGEGUILD1:
        index = 0;
        break;
    case BUILD_MAGEGUILD2:
        index = Race::NECR == castle.GetRace() ? 6 : 1;
        break;
    case BUILD_MAGEGUILD3:
        index = Race::NECR == castle.GetRace() ? 12 : 2;
        break;
    case BUILD_MAGEGUILD4:
        index = Race::NECR == castle.GetRace() ? 18 : 3;
        break;
    case BUILD_MAGEGUILD5:
        index = Race::NECR == castle.GetRace() ? 24 : 4;
        break;
    default:
        break;
    }

    return fheroes2::AGG::GetICN( castle.GetICNBuilding( build, castle.GetRace() ), index );
}

void RedrawIcons( const Castle & castle, const CastleHeroes & heroes, const Point & pt )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const Heroes * hero1 = heroes.Guard();
    const Heroes * hero2 = heroes.Guest();

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 0 ), display, pt.x, pt.y + 256 );

    fheroes2::Image icon1, icon2;

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

fheroes2::Image GetMeetingSprite( void )
{
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::ADVMCO, 8 );

    fheroes2::Image result( sprite.width() + 4, sprite.height() + 4 );
    result.fill( 0 );

    fheroes2::DrawBorder( result, fheroes2::GetColorId( 0xe0, 0xb4, 0 ) );
    fheroes2::Blit( sprite, result, 2, 2 );

    return result;
}

MeetingButton::MeetingButton( s32 px, s32 py )
{
    const fheroes2::Image & sprite = GetMeetingSprite();
    setSprite( sprite, sprite );
    setPosition( px, py );
}

SwapButton::SwapButton( s32 px, s32 py )
{
    const fheroes2::Image & sprite = GetMeetingSprite();
    // Custom graphics: rotate existing sprtie
    // sf = GetMeetingSprite().RenderRotate( 1 );
    setSprite( sprite, sprite );
    setPosition( px, py );
}

int Castle::OpenDialog( bool readonly )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Settings & conf = Settings::Get();

    const bool interface = conf.ExtGameEvilInterface();
    if ( conf.ExtGameDynamicInterface() )
        conf.SetEvilInterface( ( GetRace() & ( Race::BARB | Race::WRLK | Race::NECR ) ) != 0 );

    CastleHeroes heroes = world.GetHeroes( *this );

    // cursor
    Cursor & cursor = Cursor::Get();

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    // fade
    if ( conf.ExtGameUseFade() )
        fheroes2::FadeDisplay();

    Dialog::FrameBorder background( Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT ) );

    const Point & cur_pt = background.GetArea();
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

    ArmyBar selectArmy2( NULL, false, readonly );
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
    const Rect & rectResource = RedrawResourcePanel( cur_pt );
    const fheroes2::Rect resActiveArea( rectResource.x, rectResource.y, rectResource.w, buttonExit.area().y - rectResource.y );

    // button swap
    SwapButton buttonSwap( cur_pt.x + 4, cur_pt.y + 345 );
    MeetingButton buttonMeeting( cur_pt.x + 88, cur_pt.y + 345 );

    if ( heroes.Guest() && heroes.Guard() && !readonly ) {
        buttonSwap.draw();
        buttonMeeting.draw();
    }

    // fill cache buildings
    CastleDialog::CacheBuildings cacheBuildings( *this, cur_pt );

    // draw building
    CastleDialog::RedrawAllBuilding( *this, cur_pt, cacheBuildings );

    if ( 2 > world.GetKingdom( GetColor() ).GetCastles().size() || readonly ) {
        buttonPrevCastle.disable();
        buttonNextCastle.disable();
    }

    buttonPrevCastle.draw();
    buttonNextCastle.draw();
    buttonExit.draw();

    AGG::PlayMusic( MUS::FromRace( race ) );

    LocalEvent & le = LocalEvent::Get();
    cursor.Show();

    bool firstDraw = true;

    int result = Dialog::CANCEL;
    bool need_redraw = false;

    // dialog menu loop
    while ( le.HandleEvents() ) {
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
            cursor.Hide();
            need_redraw = true;
        }

        if ( conf.ExtCastleAllowGuardians() && !readonly ) {
            Army * army1 = NULL;
            Army * army2 = NULL;

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
                cursor.Hide();
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
                    selectArmy2.SetArmy( NULL );
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
            Game::DisableChangeMusic( true );
            Game::OpenHeroesDialog( *heroes.Guard(), false );

            if ( selectArmy1.isSelected() )
                selectArmy1.ResetSelected();
            if ( selectArmy2.isValid() && selectArmy2.isSelected() )
                selectArmy2.ResetSelected();

            need_redraw = true;
        }
        else
            // view hero
            if ( !readonly && heroes.Guest() && le.MouseClickLeft( rectSign2 ) ) {
            Game::DisableChangeMusic( true );
            Game::OpenHeroesDialog( *heroes.Guest(), false );

            if ( selectArmy1.isSelected() )
                selectArmy1.ResetSelected();
            if ( selectArmy2.isValid() && selectArmy2.isSelected() )
                selectArmy2.ResetSelected();

            need_redraw = true;
        }

        // prev castle
        if ( buttonPrevCastle.isEnabled() && ( le.MouseClickLeft( buttonPrevCastle.area() ) || HotKeyPressEvent( Game::EVENT_MOVELEFT ) ) ) {
            result = Dialog::PREV;
            break;
        }
        else
            // next castle
            if ( buttonNextCastle.isEnabled() && ( le.MouseClickLeft( buttonNextCastle.area() ) || HotKeyPressEvent( Game::EVENT_MOVERIGHT ) ) ) {
            result = Dialog::NEXT;
            break;
        }

        // buildings event
#if defined( ANDROID )
        CastleDialog::CacheBuildings::const_reverse_iterator crend = cacheBuildings.rend();
        for ( CastleDialog::CacheBuildings::const_reverse_iterator it = cacheBuildings.rbegin(); it != crend; ++it )
#else
        for ( CastleDialog::CacheBuildings::const_reverse_iterator it = cacheBuildings.rbegin(); it != cacheBuildings.rend(); ++it )
#endif
        {
            if ( ( *it ).id == GetActualDwelling( ( *it ).id ) && isBuild( ( *it ).id ) ) {
                if ( !readonly && le.MouseClickLeft( ( *it ).coord ) ) {
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

        for ( CastleDialog::CacheBuildings::const_iterator it = cacheBuildings.begin(); it != cacheBuildings.end(); ++it ) {
            if ( BUILD_MAGEGUILD & ( *it ).id ) {
                const int mageGuildLevel = GetLevelMageGuild();
                if ( ( *it ).id == ( BUILD_MAGEGUILD1 << ( mageGuildLevel - 1 ) ) ) {
                    if ( le.MouseClickLeft( ( *it ).coord ) ) {
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
                            Heroes * hero = heroes.Guard();
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
                if ( le.MouseClickLeft( ( *it ).coord ) ) {
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

                                int alpha = 1;
                                uint32_t buildFrame = 0;
                                const int boatICN = GetICNBoat( GetRace() );
                                while ( le.HandleEvents() && alpha < 255 ) {
                                    if ( Game::AnimateInfrequentDelay( Game::CASTLE_BUILD_DELAY ) ) {
                                        cursor.Hide();

                                        const uint32_t castleAnimationFrame = Game::CastleAnimationFrame();

                                        for ( CastleDialog::CacheBuildings::const_iterator buildingIt = cacheBuildings.begin(); buildingIt != cacheBuildings.end();
                                              ++buildingIt ) {
                                            const uint32_t currentBuildId = it->id;
                                            if ( isBuild( currentBuildId ) ) {
                                                if ( currentBuildId == BUILD_SHIPYARD ) {
                                                    CastleDialog::CastleRedrawBuilding( *this, cur_pt, currentBuildId, castleAnimationFrame );
                                                    const fheroes2::Sprite & shipyardSprite = fheroes2::AGG::GetICN( boatICN, 0 );
                                                    fheroes2::AlphaBlit( shipyardSprite, display, cur_pt.x + shipyardSprite.x(), cur_pt.y + shipyardSprite.y(), alpha );
                                                    const fheroes2::Sprite & boatSprite = fheroes2::AGG::GetICN( boatICN, 1 );
                                                    fheroes2::AlphaBlit( boatSprite, display, cur_pt.x + boatSprite.x(), cur_pt.y + boatSprite.y(), alpha );
                                                }
                                                else {
                                                    CastleDialog::CastleRedrawBuilding( *this, cur_pt, currentBuildId, castleAnimationFrame );
                                                    CastleDialog::CastleRedrawBuildingExtended( *this, cur_pt, currentBuildId, castleAnimationFrame );
                                                    if ( CastleDialog::RoadConnectionNeeded( *this, currentBuildId, false ) ) {
                                                        CastleDialog::RedrawRoadConnection( *this, cur_pt, currentBuildId );
                                                    }
                                                }
                                            }
                                        }

                                        CastleRedrawTownName( *this, cur_pt );

                                        cursor.Show();
                                        display.render();
                                        alpha += 15;
                                    }
                                    ++buildFrame;
                                }

                                need_redraw = true;
                            }
                            break;
                        }

                        case BUILD_MARKETPLACE: {
                            fheroes2::ButtonRestorer exitRestorer( buttonExit );
                            Dialog::Marketplace();
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

                                CastleDialog::RedrawAnimationBuilding( *this, cur_pt, cacheBuildings, BUILD_CASTLE );
                                BuyBuilding( BUILD_CASTLE );

                                need_redraw = true;
                            }
                            break;
                        }

                        case BUILD_CASTLE: {
                            const Heroes * prev = heroes.Guest();
                            const u32 build = OpenTown();
                            heroes = world.GetHeroes( *this );
                            bool buyhero = ( heroes.Guest() && ( heroes.Guest() != prev ) );

                            if ( BUILD_NOTHING != build ) {
                                AGG::PlaySound( M82::BUILDTWN );

                                CastleDialog::RedrawAnimationBuilding( *this, cur_pt, cacheBuildings, build );
                                BuyBuilding( build );

                                if ( BUILD_CAPTAIN == build )
                                    RedrawIcons( *this, heroes, cur_pt );

                                need_redraw = true;
                            }

                            if ( buyhero ) {
                                if ( prev ) {
                                    selectArmy1.SetArmy( &heroes.Guard()->GetArmy() );
                                    selectArmy2.SetArmy( NULL );
                                    cursor.Hide();
                                    RedrawIcons( *this, CastleHeroes( NULL, heroes.Guard() ), cur_pt );
                                    selectArmy1.Redraw();
                                    if ( selectArmy2.isValid() )
                                        selectArmy2.Redraw();
                                    cursor.Show();
                                    display.render();
                                }
                                selectArmy2.SetArmy( &heroes.Guest()->GetArmy() );
                                AGG::PlaySound( M82::BUILDTWN );

                                // animate fade in for hero army bar
                                const Rect rt( 0, 100, 552, 107 );
                                fheroes2::Image sf( 552, 107 );
                                sf.reset();
                                fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 0 ), 0, 100, sf, 0, 0, 552, 107 );
                                fheroes2::Image port = heroes.Guest()->GetPortrait( PORT_BIG );
                                if ( !port.empty() )
                                    fheroes2::Blit( port, sf, 5, 5 );

                                const fheroes2::Point savept = selectArmy2.GetPos();
                                selectArmy2.SetPos( 112, 5 );
                                selectArmy2.Redraw( sf );
                                selectArmy2.SetPos( savept.x, savept.y );

                                RedrawResourcePanel( cur_pt );

                                int alpha = 0;
                                while ( le.HandleEvents() && alpha < 240 ) {
                                    if ( Game::AnimateInfrequentDelay( Game::CASTLE_BUYHERO_DELAY ) ) {
                                        cursor.Hide();
                                        fheroes2::AlphaBlit( sf, display, cur_pt.x, cur_pt.y + 356, alpha );
                                        cursor.Show();
                                        display.render();
                                        alpha += 10;
                                    }
                                }

                                fheroes2::Blit( sf, display, cur_pt.x, cur_pt.y + 356 );

                                RedrawIcons( *this, heroes, cur_pt );
                                need_redraw = true;
                            }
                        } break;

                        default:
                            break;
                        }
                }
                else if ( le.MousePressRight( ( *it ).coord ) )
                    Dialog::Message( GetStringBuilding( ( *it ).id ), GetDescriptionBuilding( ( *it ).id ), Font::BIG );

                if ( le.MouseCursor( ( *it ).coord ) )
                    msg_status = GetStringBuilding( ( *it ).id );
            }
        }

        if ( need_redraw ) {
            cursor.Hide();
            selectArmy1.Redraw();
            if ( selectArmy2.isValid() )
                selectArmy2.Redraw();
            CastleRedrawTownName( *this, cur_pt );
            RedrawResourcePanel( cur_pt );
            if ( heroes.Guest() && heroes.Guard() && !readonly ) {
                buttonSwap.draw();
                buttonMeeting.draw();
            }
            if ( buttonExit.isPressed() )
                buttonExit.draw();
            cursor.Show();
            display.render();
            need_redraw = false;
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

        // animation sprite
        if ( firstDraw || Game::AnimateInfrequentDelay( Game::CASTLE_AROUND_DELAY ) ) {
            firstDraw = false;
            cursor.Hide();
            CastleDialog::RedrawAllBuilding( *this, cur_pt, cacheBuildings );
            cursor.Show();
            display.render();

            Game::CastleAnimationFrame() += 1; // this function returns variable by reference
        }
    }

    if ( heroes.Guest() && conf.ExtHeroRecalculateMovement() )
        heroes.Guest()->RecalculateMovePoints();

    if ( conf.ExtGameDynamicInterface() )
        conf.SetEvilInterface( interface );

    Game::DisableChangeMusic( false );

    return result;
}

/* redraw resource info panel */
Rect Castle::RedrawResourcePanel( const Point & pt ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Funds & resource = world.GetKingdom( GetColor() ).GetFunds();

    Point dst_pt = pt;

    Rect src_rt( dst_pt.x + 552, dst_pt.y + 262, 82, 192 );
    fheroes2::Fill( display, src_rt.x, src_rt.y, src_rt.w, src_rt.h, 0 );

    Text text;

    // sprite wood
    dst_pt.x = src_rt.x + 1;
    dst_pt.y = src_rt.y + 10;
    const fheroes2::Sprite & wood = fheroes2::AGG::GetICN( ICN::RESOURCE, 0 );
    fheroes2::Blit( wood, display, dst_pt.x, dst_pt.y );

    // count wood
    text.Set( GetString( resource.wood ), Font::SMALL );
    dst_pt.y += 22;
    text.Blit( dst_pt.x + ( wood.width() - text.w() ) / 2, dst_pt.y );

    // sprite sulfur
    dst_pt.x = src_rt.x + 42;
    dst_pt.y = src_rt.y + 6;
    const fheroes2::Sprite & sulfur = fheroes2::AGG::GetICN( ICN::RESOURCE, 3 );
    fheroes2::Blit( sulfur, display, dst_pt.x, dst_pt.y );

    // count sulfur
    text.Set( GetString( resource.sulfur ) );
    dst_pt.y += 26;
    text.Blit( dst_pt.x + ( sulfur.width() - text.w() ) / 2, dst_pt.y );

    // sprite crystal
    dst_pt.x = src_rt.x + 1;
    dst_pt.y = src_rt.y + 45;
    const fheroes2::Sprite & crystal = fheroes2::AGG::GetICN( ICN::RESOURCE, 4 );
    fheroes2::Blit( crystal, display, dst_pt.x, dst_pt.y );

    // count crystal
    text.Set( GetString( resource.crystal ) );
    dst_pt.y += 33;
    text.Blit( dst_pt.x + ( crystal.width() - text.w() ) / 2, dst_pt.y );

    // sprite mercury
    dst_pt.x = src_rt.x + 44;
    dst_pt.y = src_rt.y + 47;
    const fheroes2::Sprite & mercury = fheroes2::AGG::GetICN( ICN::RESOURCE, 1 );
    fheroes2::Blit( mercury, display, dst_pt.x, dst_pt.y );

    // count mercury
    text.Set( GetString( resource.mercury ) );
    dst_pt.y += 34;
    text.Blit( dst_pt.x + ( mercury.width() - text.w() ) / 2, dst_pt.y );

    // sprite ore
    dst_pt.x = src_rt.x + 1;
    dst_pt.y = src_rt.y + 92;
    const fheroes2::Sprite & ore = fheroes2::AGG::GetICN( ICN::RESOURCE, 2 );
    fheroes2::Blit( ore, display, dst_pt.x, dst_pt.y );

    // count ore
    text.Set( GetString( resource.ore ) );
    dst_pt.y += 26;
    text.Blit( dst_pt.x + ( ore.width() - text.w() ) / 2, dst_pt.y );

    // sprite gems
    dst_pt.x = src_rt.x + 45;
    dst_pt.y = src_rt.y + 92;
    const fheroes2::Sprite & gems = fheroes2::AGG::GetICN( ICN::RESOURCE, 5 );
    fheroes2::Blit( gems, display, dst_pt.x, dst_pt.y );

    // count gems
    text.Set( GetString( resource.gems ) );
    dst_pt.y += 26;
    text.Blit( dst_pt.x + ( gems.width() - text.w() ) / 2, dst_pt.y );

    // sprite gold
    dst_pt.x = src_rt.x + 6;
    dst_pt.y = src_rt.y + 130;
    const fheroes2::Sprite & gold = fheroes2::AGG::GetICN( ICN::RESOURCE, 6 );
    fheroes2::Blit( gold, display, dst_pt.x, dst_pt.y );

    // count gold
    text.Set( GetString( resource.gold ) );
    dst_pt.y += 24;
    text.Blit( dst_pt.x + ( gold.width() - text.w() ) / 2, dst_pt.y );

    // sprite button exit
    dst_pt.x = src_rt.x + 1;
    dst_pt.y = src_rt.y + 166;
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::TREASURY, 1 ), display, dst_pt.x, dst_pt.y );

    return src_rt;
}
