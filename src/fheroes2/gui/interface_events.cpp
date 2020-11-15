/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <vector>

#include "agg.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_interface.h"
#include "game_io.h"
#include "game_over.h"
#include "heroes.h"
#include "kingdom.h"
#include "m82.h"
#include "settings.h"
#include "world.h"

void Interface::Basic::CalculateHeroPath( Heroes * hero, s32 destinationIdx )
{
    if ( ( hero == NULL ) || hero->Modes( Heroes::GUARDIAN ) )
        return;

    hero->ResetModes( Heroes::SLEEPER );
    hero->SetMove( false );

    Route::Path & path = hero->GetPath();
    if ( destinationIdx == -1 )
        destinationIdx = path.GetDestinedIndex(); // returns -1 at the time of launching new game (because of no path history)
    if ( destinationIdx != -1 ) {
        hero->GetPath().setPath( world.getPath( *hero, destinationIdx ), destinationIdx );
        DEBUG( DBG_GAME, DBG_TRACE, hero->GetName() << ", distance: " << world.getDistance( *hero, destinationIdx ) << ", route: " << path.String() );
        gameArea.SetRedraw();

        LocalEvent & le = LocalEvent::Get();
        const int32_t cursorIndex = gameArea.GetValidTileIdFromPoint( le.GetMouseCursor() );
        Cursor::Get().SetThemes( GetCursorTileIndex( cursorIndex ) );
        Interface::Basic::Get().buttonsArea.Redraw();
    }
}

void Interface::Basic::ShowPathOrStartMoveHero( Heroes * hero, s32 destinationIdx )
{
    if ( !hero || hero->Modes( Heroes::GUARDIAN ) )
        return;

    Route::Path & path = hero->GetPath();

    // show path
    if ( path.GetDestinedIndex() != destinationIdx && path.GetDestinationIndex() != destinationIdx ) {
        CalculateHeroPath( hero, destinationIdx );
    }
    // start move
    else if ( path.isValid() ) {
        SetFocus( hero );
        RedrawFocus();

        hero->SetMove( true );
        Cursor::Get().SetThemes( Cursor::WAIT );
    }
}

void Interface::Basic::MoveHeroFromArrowKeys( Heroes & hero, int direct )
{
    const bool fromWater = hero.isShipMaster();
    if ( Maps::isValidDirection( hero.GetIndex(), direct ) ) {
        s32 dst = Maps::GetDirectionIndex( hero.GetIndex(), direct );
        const Maps::Tiles & tile = world.GetTiles( dst );
        bool allow = false;

        switch ( tile.GetObject() ) {
        case MP2::OBJN_CASTLE: {
            const Castle * to_castle = world.GetCastle( hero.GetCenter() );
            if ( to_castle ) {
                dst = to_castle->GetIndex();
                allow = true;
            }
            break;
        }

        case MP2::OBJ_BOAT:
        case MP2::OBJ_CASTLE:
        case MP2::OBJ_HEROES:
        case MP2::OBJ_MONSTER:
            allow = true;
            break;

        default:
            allow = ( tile.isPassable( Direction::CENTER, fromWater, false ) || MP2::isActionObject( tile.GetObject(), fromWater ) );
            break;
        }

        if ( allow )
            ShowPathOrStartMoveHero( &hero, dst );
    }
}

void Interface::Basic::EventNextHero( void )
{
    const Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    const KingdomHeroes & myHeroes = myKingdom.GetHeroes();

    if ( myHeroes.empty() )
        return;

    if ( GetFocusHeroes() ) {
        KingdomHeroes::const_iterator it = std::find( myHeroes.begin(), myHeroes.end(), GetFocusHeroes() );
        KingdomHeroes::const_iterator currentHero = it;
        do {
            ++it;
            if ( it == myHeroes.end() )
                it = myHeroes.begin();
            if ( ( *it )->MayStillMove() ) {
                SetFocus( *it );
                break;
            }
        } while ( it != currentHero );
    }
    else {
        const size_t heroesCount = myHeroes.size();
        for ( size_t i = 0; i < heroesCount; ++i ) {
            if ( myHeroes[i]->MayStillMove() ) {
                SetFocus( myHeroes[i] );
                break;
            }
        }
    }
    RedrawFocus();
}

void Interface::Basic::EventContinueMovement( void )
{
    Heroes * hero = GetFocusHeroes();

    if ( hero && hero->GetPath().isValid() )
        hero->SetMove( !hero->isMoveEnabled() );
}

void Interface::Basic::EventKingdomInfo( void )
{
    Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    myKingdom.OverviewDialog();

    iconsPanel.SetRedraw();
}

void Interface::Basic::EventCastSpell( void )
{
    Heroes * hero = GetFocusHeroes();

    if ( hero ) {
        SetRedraw( REDRAW_ALL );
        ResetFocus( GameFocus::HEROES );
        Redraw();

        const Spell spell = hero->OpenSpellBook( SpellBook::ADVN, true );
        // apply cast spell
        if ( spell.isValid() ) {
            hero->ActionSpellCast( spell );
            iconsPanel.SetRedraw();
        }
    }
}

int Interface::Basic::EventEndTurn( void )
{
    const Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );

    if ( GetFocusHeroes() )
        GetFocusHeroes()->SetMove( false );

    if ( !myKingdom.HeroesMayStillMove()
         || Dialog::YES == Dialog::Message( "", _( "One or more heroes may still move, are you sure you want to end your turn?" ), Font::BIG, Dialog::YES | Dialog::NO ) )
        return Game::ENDTURN;

    return Game::CANCEL;
}

int Interface::Basic::EventAdventureDialog( void )
{
    Mixer::Reduce();
    switch ( Dialog::AdventureOptions( GameFocus::HEROES == GetFocusType() ) ) {
    case Dialog::WORLD:
        break;

    case Dialog::PUZZLE:
        EventPuzzleMaps();
        break;

    case Dialog::INFO:
        EventGameInfo();
        break;

    case Dialog::DIG:
        return EventDigArtifact();

    default:
        break;
    }
    Mixer::Enhance();

    return Game::CANCEL;
}

int Interface::Basic::EventFileDialog( void )
{
    return Dialog::FileOptions();
}

void Interface::Basic::EventSystemDialog( void )
{
    const Settings & conf = Settings::Get();

    // Change and save system settings
    const int changes = Dialog::SystemOptions();

    // interface themes
    if ( 0x08 & changes ) {
        Interface::Basic::Get().Reset();
        SetRedraw( REDRAW_ICONS | REDRAW_BUTTONS | REDRAW_STATUS | REDRAW_BORDER );
        ResetFocus( GameFocus::HEROES );
        Redraw();
    }

    // interface hide/show
    if ( 0x04 & changes ) {
        GetRadar().ResetAreaSize();
        SetHideInterface( conf.ExtGameHideInterface() );
        SetRedraw( REDRAW_ALL );
        ResetFocus( GameFocus::HEROES );
        Redraw();
    }
}

int Interface::Basic::EventExit( void )
{
    if ( Dialog::YES & Dialog::Message( "", _( "Are you sure you want to quit?" ), Font::BIG, Dialog::YES | Dialog::NO ) )
        return Game::QUITGAME;

    return Game::CANCEL;
}

void Interface::Basic::EventNextTown( void )
{
    Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    KingdomCastles & myCastles = myKingdom.GetCastles();

    if ( myCastles.size() ) {
        if ( GetFocusCastle() ) {
            KingdomCastles::const_iterator it = std::find( myCastles.begin(), myCastles.end(), GetFocusCastle() );
            ++it;
            if ( it == myCastles.end() )
                it = myCastles.begin();
            SetFocus( *it );
        }
        else
            ResetFocus( GameFocus::CASTLE );

        RedrawFocus();
    }
}

int Interface::Basic::EventNewGame( void )
{
    return Dialog::YES == Dialog::Message( "", _( "Are you sure you want to restart? (Your current game will be lost)" ), Font::BIG, Dialog::YES | Dialog::NO )
               ? Game::NEWGAME
               : Game::CANCEL;
}

int Interface::Basic::EventSaveGame( void )
{
    std::string filename = Dialog::SelectFileSave();
    if ( filename.size() && Game::Save( filename ) )
        Dialog::Message( "", _( "Game saved successfully." ), Font::BIG, Dialog::OK );
    return Game::CANCEL;
}

int Interface::Basic::EventLoadGame( void )
{
    return Dialog::YES == Dialog::Message( "", _( "Are you sure you want to load a new game? (Your current game will be lost)" ), Font::BIG, Dialog::YES | Dialog::NO )
               ? Game::LOADGAME
               : Game::CANCEL;
}

void Interface::Basic::EventPuzzleMaps( void )
{
    world.GetKingdom( Settings::Get().CurrentColor() ).PuzzleMaps().ShowMapsDialog();
}

void Interface::Basic::EventGameInfo( void )
{
    Dialog::GameInfo();
}

void Interface::Basic::EventSwitchHeroSleeping( void )
{
    Heroes * hero = GetFocusHeroes();

    if ( hero ) {
        if ( hero->Modes( Heroes::SLEEPER ) )
            hero->ResetModes( Heroes::SLEEPER );
        else {
            hero->SetModes( Heroes::SLEEPER );
            hero->GetPath().Reset();
        }

        SetRedraw( REDRAW_HEROES );
    }
}

int Interface::Basic::EventDigArtifact( void )
{
    Heroes * hero = GetFocusHeroes();

    if ( hero ) {
        if ( hero->isShipMaster() )
            Dialog::Message( "", _( "Try looking on land!!!" ), Font::BIG, Dialog::OK );
        else if ( hero->GetMaxMovePoints() <= hero->GetMovePoints() ) {
            if ( world.GetTiles( hero->GetIndex() ).GoodForUltimateArtifact() ) {
                AGG::PlaySound( M82::DIGSOUND );

                hero->ResetMovePoints();

                if ( world.DiggingForUltimateArtifact( hero->GetCenter() ) ) {
                    AGG::PlaySound( M82::TREASURE );
                    const Artifact & ultimate = world.GetUltimateArtifact().GetArtifact();
                    hero->PickupArtifact( ultimate );
                    std::string msg( _( "After spending many hours digging here, you have uncovered the %{artifact}." ) );
                    StringReplace( msg, "%{artifact}", ultimate.GetName() );
                    Dialog::ArtifactInfo( _( "Congratulations!" ), msg, ultimate() );

                    // set all obelisks visited
                    Kingdom & kingdom = world.GetKingdom( hero->GetColor() );
                    const MapsIndexes obelisks = Maps::GetObjectPositions( MP2::OBJ_OBELISK, false );

                    for ( MapsIndexes::const_iterator it = obelisks.begin(); it != obelisks.end(); ++it )
                        if ( !hero->isVisited( world.GetTiles( *it ), Visit::GLOBAL ) )
                            hero->SetVisited( *it, Visit::GLOBAL );

                    kingdom.PuzzleMaps().Update( kingdom.CountVisitedObjects( MP2::OBJ_OBELISK ), world.CountObeliskOnMaps() );
                }
                else
                    Dialog::Message( "", _( "Nothing here. Where could it be?" ), Font::BIG, Dialog::OK );

                Cursor::Get().Hide();
                iconsPanel.RedrawIcons( ICON_HEROES );
                Cursor::Get().Show();
                fheroes2::Display::instance().render();

                // check game over for ultimate artifact
                return GameOver::Result::Get().LocalCheckGameOver();
            }
            else
                Dialog::Message( "", _( "Try searching on clear ground." ), Font::BIG, Dialog::OK );
        }
        else {
            Dialog::Message( "", _( "Digging for artifacts requires a whole day, try again tomorrow." ), Font::BIG, Dialog::OK );
        }
    }

    return Game::CANCEL;
}

void Interface::Basic::EventDefaultAction( void )
{
    Heroes * hero = GetFocusHeroes();

    if ( hero ) {
        const Maps::Tiles & tile = world.GetTiles( hero->GetIndex() );

        // 1. action object
        if ( MP2::isActionObject( hero->GetMapsObject(), hero->isShipMaster() ) && ( !MP2::isMoveObject( hero->GetMapsObject() ) || hero->CanMove() ) ) {
            hero->Action( hero->GetIndex() );
            if ( MP2::OBJ_STONELITHS == tile.GetObject( false ) || MP2::OBJ_WHIRLPOOL == tile.GetObject( false ) )
                SetRedraw( REDRAW_HEROES );
            SetRedraw( REDRAW_GAMEAREA );
        }
        else
            // 2. continue
            if ( hero->GetPath().isValid() )
            hero->SetMove( true );
        else
            // 3. hero dialog
            Game::OpenHeroesDialog( *hero );
    }
    else
        // 4. town dialog
        if ( GetFocusCastle() ) {
        Game::OpenCastleDialog( *GetFocusCastle() );
    }
}

void Interface::Basic::EventOpenFocus( void )
{
    if ( GetFocusHeroes() )
        Game::OpenHeroesDialog( *GetFocusHeroes() );
    else if ( GetFocusCastle() )
        Game::OpenCastleDialog( *GetFocusCastle() );
}

void Interface::Basic::EventSwitchShowRadar( void )
{
    Settings & conf = Settings::Get();

    if ( conf.ExtGameHideInterface() ) {
        if ( conf.ShowRadar() ) {
            conf.SetShowRadar( false );
            gameArea.SetRedraw();
        }
        else {
            conf.SetShowRadar( true );
            radar.SetRedraw();
        }
    }
}

void Interface::Basic::EventSwitchShowButtons( void )
{
    Settings & conf = Settings::Get();

    if ( conf.ExtGameHideInterface() ) {
        if ( conf.ShowButtons() ) {
            conf.SetShowButtons( false );
            gameArea.SetRedraw();
        }
        else {
            conf.SetShowButtons( true );
            buttonsArea.SetRedraw();
        }
    }
}

void Interface::Basic::EventSwitchShowStatus( void )
{
    Settings & conf = Settings::Get();

    if ( conf.ExtGameHideInterface() ) {
        if ( conf.ShowStatus() ) {
            conf.SetShowStatus( false );
            gameArea.SetRedraw();
        }
        else {
            conf.SetShowStatus( true );
            statusWindow.SetRedraw();
        }
    }
}

void Interface::Basic::EventSwitchShowIcons( void )
{
    Settings & conf = Settings::Get();

    if ( conf.ExtGameHideInterface() ) {
        if ( conf.ShowIcons() ) {
            conf.SetShowIcons( false );
            gameArea.SetRedraw();
        }
        else {
            conf.SetShowIcons( true );
            iconsPanel.SetCurrentVisible();
            iconsPanel.SetRedraw();
        }
    }
}

void Interface::Basic::EventSwitchShowControlPanel( void )
{
    Settings & conf = Settings::Get();

    if ( conf.ExtGameHideInterface() ) {
        conf.SetShowPanel( !conf.ShowControlPanel() );
        gameArea.SetRedraw();
    }
}

void Interface::Basic::EventKeyArrowPress( int dir )
{
    Heroes * hero = GetFocusHeroes();

    // move hero
    if ( hero )
        MoveHeroFromArrowKeys( *hero, dir );
    else
        // scroll map
        switch ( dir ) {
        case Direction::TOP_LEFT:
            gameArea.SetScroll( SCROLL_TOP );
            gameArea.SetScroll( SCROLL_LEFT );
            break;
        case Direction::TOP:
            gameArea.SetScroll( SCROLL_TOP );
            break;
        case Direction::TOP_RIGHT:
            gameArea.SetScroll( SCROLL_TOP );
            gameArea.SetScroll( SCROLL_RIGHT );
            break;
        case Direction::RIGHT:
            gameArea.SetScroll( SCROLL_RIGHT );
            break;
        case Direction::BOTTOM_RIGHT:
            gameArea.SetScroll( SCROLL_BOTTOM );
            gameArea.SetScroll( SCROLL_RIGHT );
            break;
        case Direction::BOTTOM:
            gameArea.SetScroll( SCROLL_BOTTOM );
            break;
        case Direction::BOTTOM_LEFT:
            gameArea.SetScroll( SCROLL_BOTTOM );
            gameArea.SetScroll( SCROLL_LEFT );
            break;
        case Direction::LEFT:
            gameArea.SetScroll( SCROLL_LEFT );
            break;
        default:
            break;
        }
}

void Interface::Basic::EventDebug1( void )
{
    VERBOSE( "" );
    /*
        Heroes* hero = GetFocusHeroes();

        if(hero)
        {
        int level = hero->GetLevelFromExperience(hero->GetExperience());
        u32 exp = hero->GetExperienceFromLevel(level + 1);

        hero->IncreaseExperience(exp - hero->GetExperience() + 100);
        }
    */
}

void Interface::Basic::EventDebug2( void )
{
    VERBOSE( "" );
}
