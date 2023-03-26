/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include "game.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <list>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "ai.h"
#include "army.h"
#include "audio.h"
#include "audio_manager.h"
#include "battle_only.h"
#include "castle.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "direction.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_io.h"
#include "game_mode.h"
#include "game_over.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "interface_buttons.h"
#include "interface_cpanel.h"
#include "interface_gamearea.h"
#include "interface_icons.h"
#include "interface_radar.h"
#include "interface_status.h"
#include "kingdom.h"
#include "localevent.h"
#include "logging.h"
#include "m82.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "monster.h"
#include "mp2.h"
#include "mus.h"
#include "players.h"
#include "resource.h"
#include "route.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "week.h"
#include "world.h"

namespace
{
    bool SortPlayers( const Player * player1, const Player * player2 )
    {
        return ( player1->isControlHuman() && !player2->isControlHuman() )
               || ( ( player1->isControlHuman() == player2->isControlHuman() ) && ( player1->GetColor() < player2->GetColor() ) );
    }

    // Get colors value of players to use in fog directions update.
    // For human allied AI returns colors of this alliance, for hostile AI - colors of all human players and their allies.
    int32_t hotSeatAIFogColors( const Player * player )
    {
        assert( player != nullptr );

        // This function should be called when AI makes a move.
        assert( world.GetKingdom( player->GetColor() ).GetControl() == CONTROL_AI );

        const int32_t humanColors = Players::HumanColors();
        // Check if the current AI player is a friend of any of human players to fully show his move and revealed map,
        // otherwise his revealed map will not be shown - instead of it we will show the revealed map by all human players.
        const bool isFriendlyAI = Players::isFriends( player->GetColor(), humanColors );

#if defined( WITH_DEBUG )
        if ( isFriendlyAI || player->isAIAutoControlMode() ) {
#else
        if ( isFriendlyAI ) {
#endif
            // Fully update fog directions for allied AI players in Hot Seat mode as the previous move could be done by opposing player.
            return player->GetFriends();
        }

        // If AI is hostile for all human players then fully update fog directions for all human players to see enemy AI hero move on tiles with
        // discovered fog.

        int32_t friendColors = 0;

        for ( const int32_t color : Colors( humanColors ) ) {
            const Player * humanPlayer = Players::Get( color );
            if ( humanPlayer ) {
                friendColors |= humanPlayer->GetFriends();
            }
        }

        return friendColors;
    }
}

fheroes2::GameMode Game::StartBattleOnly()
{
    Battle::Only main;

    if ( main.ChangeSettings() )
        main.StartBattle();

    return fheroes2::GameMode::MAIN_MENU;
}

fheroes2::GameMode Game::StartGame()
{
    AI::Get().Reset();

    const Settings & conf = Settings::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    if ( !conf.LoadedGameVersion() )
        GameOver::Result::Get().Reset();

    return Interface::Basic::Get().StartGame();
}

void Game::DialogPlayers( int color, std::string str )
{
    const Player * player = Players::Get( color );
    StringReplace( str, "%{color}", ( player ? player->GetName() : Color::String( color ) ) );

    const fheroes2::Sprite & border = fheroes2::AGG::GetICN( ICN::BRCREST, 6 );
    fheroes2::Sprite sign = border;

    switch ( color ) {
    case Color::BLUE:
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::BRCREST, 0 ), sign, 4, 4 );
        break;
    case Color::GREEN:
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::BRCREST, 1 ), sign, 4, 4 );
        break;
    case Color::RED:
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::BRCREST, 2 ), sign, 4, 4 );
        break;
    case Color::YELLOW:
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::BRCREST, 3 ), sign, 4, 4 );
        break;
    case Color::ORANGE:
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::BRCREST, 4 ), sign, 4, 4 );
        break;
    case Color::PURPLE:
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::BRCREST, 5 ), sign, 4, 4 );
        break;
    default:
        // Did you add a new color? Add the logic for it!
        assert( 0 );
        break;
    }

    const fheroes2::CustomImageDialogElement imageUI( std::move( sign ) );
    fheroes2::showMessage( fheroes2::Text( "", {} ), fheroes2::Text( std::move( str ), fheroes2::FontType::normalWhite() ), Dialog::OK, { &imageUI } );
}

void Game::OpenCastleDialog( Castle & castle, bool updateFocus /* = true */ )
{
    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    // Stop all sounds, but not the music - it will be replaced by the music of the castle
    AudioManager::stopSounds();

    const Settings & conf = Settings::Get();
    Kingdom & myKingdom = world.GetKingdom( conf.CurrentColor() );
    const KingdomCastles & myCastles = myKingdom.GetCastles();
    KingdomCastles::const_iterator it = std::find( myCastles.begin(), myCastles.end(), &castle );

    const size_t heroCountBefore = myKingdom.GetHeroes().size();

    if ( it != myCastles.end() ) {
        Castle::CastleDialogReturnValue result = Castle::CastleDialogReturnValue::DoNothing;

        while ( result != Castle::CastleDialogReturnValue::Close ) {
            assert( it != myCastles.end() );

            const bool openConstructionWindow
                = ( result == Castle::CastleDialogReturnValue::PreviousCostructionWindow ) || ( result == Castle::CastleDialogReturnValue::NextCostructionWindow );

            result = ( *it )->OpenDialog( openConstructionWindow );

            if ( result == Castle::CastleDialogReturnValue::PreviousCastle || result == Castle::CastleDialogReturnValue::PreviousCostructionWindow ) {
                if ( it == myCastles.begin() )
                    it = myCastles.end();
                --it;
            }
            else if ( result == Castle::CastleDialogReturnValue::NextCastle || result == Castle::CastleDialogReturnValue::NextCostructionWindow ) {
                ++it;
                if ( it == myCastles.end() )
                    it = myCastles.begin();
            }
        }
    }
    else {
        assert( 0 );
    }

    Interface::Basic & basicInterface = Interface::Basic::Get();

    if ( updateFocus ) {
        if ( heroCountBefore < myKingdom.GetHeroes().size() ) {
            basicInterface.SetFocus( myKingdom.GetHeroes()[heroCountBefore] );
        }
        else if ( it != myCastles.end() ) {
            Heroes * heroInCastle = world.GetTiles( ( *it )->GetIndex() ).GetHeroes();
            if ( heroInCastle == nullptr ) {
                basicInterface.SetFocus( *it );
            }
            else {
                basicInterface.SetFocus( heroInCastle );
            }
        }
        else {
            basicInterface.ResetFocus( GameFocus::HEROES );
        }
    }
    else {
        // If we don't update focus, we still have to restore environment sounds and terrain music theme
        restoreSoundsForCurrentFocus();
    }

    // The castle garrison can change
    basicInterface.RedrawFocus();
}

void Game::OpenHeroesDialog( Heroes & hero, bool updateFocus, bool windowIsGameWorld, bool disableDismiss /* = false */ )
{
    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    bool needFade = Settings::isFadeEffectEnabled() && fheroes2::Display::instance().isDefaultSize();

    Interface::Basic & basicInterface = Interface::Basic::Get();

    const KingdomHeroes & myHeroes = hero.GetKingdom().GetHeroes();
    KingdomHeroes::const_iterator it = std::find( myHeroes.begin(), myHeroes.end(), &hero );

    int result = Dialog::ZERO;

    while ( it != myHeroes.end() && result != Dialog::CANCEL ) {
        result = ( *it )->OpenDialog( false, needFade, disableDismiss, false, windowIsGameWorld );
        if ( needFade )
            needFade = false;

        switch ( result ) {
        case Dialog::PREV:
            if ( it == myHeroes.begin() )
                it = myHeroes.end();
            --it;
            break;

        case Dialog::NEXT:
            ++it;
            if ( it == myHeroes.end() )
                it = myHeroes.begin();
            break;

        case Dialog::DISMISS:
            AudioManager::PlaySound( M82::KILLFADE );

            ( *it )->GetPath().Hide();
            basicInterface.SetRedraw( Interface::REDRAW_GAMEAREA );

            if ( windowIsGameWorld ) {
                ( *it )->FadeOut();
            }

            ( *it )->SetFreeman( 0 );
            it = myHeroes.end();

            updateFocus = true;

            result = Dialog::CANCEL;
            break;

        default:
            break;
        }
    }

    if ( updateFocus ) {
        if ( it != myHeroes.end() ) {
            basicInterface.SetFocus( *it );
        }
        else {
            basicInterface.ResetFocus( GameFocus::HEROES );
        }
    }

    // The hero's army can change
    basicInterface.RedrawFocus();
}

void ShowNewWeekDialog()
{
    // restore the original music on exit
    const AudioManager::MusicRestorer musicRestorer;

    AudioManager::PlayMusic( world.BeginMonth() ? MUS::NEW_MONTH : MUS::NEW_WEEK, Music::PlaybackMode::PLAY_ONCE );

    const Week & week = world.GetWeekType();

    // head
    std::string message = world.BeginMonth() ? _( "Astrologers proclaim the Month of the %{name}." ) : _( "Astrologers proclaim the Week of the %{name}." );
    StringReplace( message, "%{name}", week.GetName() );
    message += "\n \n";

    if ( week.GetType() == WeekName::MONSTERS ) {
        const Monster monster( week.GetMonster() );
        const uint32_t count = world.BeginMonth() ? Castle::GetGrownMonthOf() : Castle::GetGrownWeekOf();

        if ( monster.isValid() && count ) {
            if ( world.BeginMonth() )
                message += 100 == Castle::GetGrownMonthOf() ? _( "After regular growth, the population of %{monster} is doubled!" )
                                                            : _n( "After regular growth, the population of %{monster} increases by %{count} percent!",
                                                                  "After regular growth, the population of %{monster} increases by %{count} percent!", count );
            else
                message += _( "%{monster} growth +%{count}." );
            StringReplaceWithLowercase( message, "%{monster}", monster.GetMultiName() );
            StringReplace( message, "%{count}", count );
            message += "\n \n";
        }
    }

    if ( week.GetType() == WeekName::PLAGUE )
        message += _( " All populations are halved." );
    else
        message += _( " All dwellings increase population." );

    fheroes2::showStandardTextMessage( "", message, Dialog::OK );
}

void ShowEventDayDialog()
{
    const Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    const EventsDate events = world.GetEventsDate( myKingdom.GetColor() );

    for ( const EventDate & event : events ) {
        if ( event.resource.GetValidItemsCount() ) {
            fheroes2::showResourceMessage( fheroes2::Text( event.title, fheroes2::FontType::normalYellow() ),
                                           fheroes2::Text( event.message, fheroes2::FontType::normalWhite() ), Dialog::OK, event.resource );
        }
        else if ( !event.message.empty() ) {
            fheroes2::showStandardTextMessage( event.title, event.message, Dialog::OK );
        }
    }
}

void ShowWarningLostTownsDialog()
{
    const Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    const uint32_t lostTownDays = myKingdom.GetLostTownDays();

    if ( lostTownDays == 1 ) {
        Game::DialogPlayers( myKingdom.GetColor(), _( "%{color} player, this is your last day to capture a town, or you will be banished from this land." ) );
    }
    else if ( lostTownDays > 0 && lostTownDays <= Game::GetLostTownDays() ) {
        std::string str = _( "%{color} player, you only have %{day} days left to capture a town, or you will be banished from this land." );
        StringReplace( str, "%{day}", lostTownDays );
        Game::DialogPlayers( myKingdom.GetColor(), str );
    }
}

int Interface::Basic::GetCursorFocusCastle( const Castle & from_castle, const Maps::Tiles & tile )
{
    switch ( tile.GetObject() ) {
    case MP2::OBJ_NON_ACTION_CASTLE:
    case MP2::OBJ_CASTLE: {
        const Castle * to_castle = world.getCastle( tile.GetCenter() );

        if ( nullptr != to_castle )
            return to_castle->GetColor() == from_castle.GetColor() ? Cursor::CASTLE : Cursor::POINTER;
        break;
    }

    case MP2::OBJ_HEROES: {
        const Heroes * heroes = tile.GetHeroes();

        if ( nullptr != heroes )
            return heroes->GetColor() == from_castle.GetColor() ? Cursor::HEROES : Cursor::POINTER;
        break;
    }

    default:
        break;
    }

    return Cursor::POINTER;
}

int Interface::Basic::GetCursorFocusShipmaster( const Heroes & from_hero, const Maps::Tiles & tile )
{
    const bool water = tile.isWater();

    switch ( tile.GetObject() ) {
    case MP2::OBJ_MONSTER:
        return water ? Cursor::DistanceThemes( Cursor::CURSOR_HERO_FIGHT, from_hero.getNumOfTravelDays( tile.GetIndex() ) ) : Cursor::POINTER;

    case MP2::OBJ_BOAT:
        return Cursor::POINTER;

    case MP2::OBJ_NON_ACTION_CASTLE:
    case MP2::OBJ_CASTLE: {
        const Castle * castle = world.getCastle( tile.GetCenter() );

        if ( castle ) {
            if ( tile.GetObject() == MP2::OBJ_NON_ACTION_CASTLE && water && tile.isPassableFrom( Direction::CENTER, true, false, from_hero.GetColor() ) ) {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_BOAT, from_hero.getNumOfTravelDays( tile.GetIndex() ) );
            }

            return from_hero.GetColor() == castle->GetColor() ? Cursor::CASTLE : Cursor::POINTER;
        }
        break;
    }

    case MP2::OBJ_HEROES: {
        const Heroes * to_hero = tile.GetHeroes();

        if ( to_hero ) {
            if ( !to_hero->isShipMaster() )
                return from_hero.GetColor() == to_hero->GetColor() ? Cursor::HEROES : Cursor::POINTER;
            else if ( to_hero->GetCenter() == from_hero.GetCenter() )
                return Cursor::HEROES;
            else if ( from_hero.GetColor() == to_hero->GetColor() )
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_MEET, from_hero.getNumOfTravelDays( tile.GetIndex() ) );
            else if ( from_hero.isFriends( to_hero->GetColor() ) )
                return Cursor::POINTER;
            else
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_FIGHT, from_hero.getNumOfTravelDays( tile.GetIndex() ) );
        }
        break;
    }

    case MP2::OBJ_COAST:
        return Cursor::DistanceThemes( Cursor::CURSOR_HERO_ANCHOR, from_hero.getNumOfTravelDays( tile.GetIndex() ) );

    default:
        if ( water ) {
            if ( MP2::isWaterActionObject( tile.GetObject() ) ) {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_BOAT_ACTION, from_hero.getNumOfTravelDays( tile.GetIndex() ) );
            }
            if ( tile.isPassableFrom( Direction::CENTER, true, false, from_hero.GetColor() ) ) {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_BOAT, from_hero.getNumOfTravelDays( tile.GetIndex() ) );
            }
        }
        break;
    }

    return Cursor::POINTER;
}

int Interface::Basic::GetCursorFocusHeroes( const Heroes & from_hero, const Maps::Tiles & tile )
{
    if ( from_hero.Modes( Heroes::ENABLEMOVE ) ) {
        return Cursor::Get().Themes();
    }

    if ( from_hero.isShipMaster() ) {
        return GetCursorFocusShipmaster( from_hero, tile );
    }

    switch ( tile.GetObject() ) {
    case MP2::OBJ_MONSTER:
        return Cursor::DistanceThemes( Cursor::CURSOR_HERO_FIGHT, from_hero.getNumOfTravelDays( tile.GetIndex() ) );

    case MP2::OBJ_NON_ACTION_CASTLE:
    case MP2::OBJ_CASTLE: {
        const Castle * castle = world.getCastle( tile.GetCenter() );

        if ( castle ) {
            if ( tile.GetObject() == MP2::OBJ_NON_ACTION_CASTLE ) {
                if ( tile.GetPassable() == 0 ) {
                    return ( from_hero.GetColor() == castle->GetColor() ) ? Cursor::CASTLE : Cursor::POINTER;
                }
                else {
                    const bool protection = Maps::isTileUnderProtection( tile.GetIndex() );

                    return Cursor::DistanceThemes( ( protection ? Cursor::CURSOR_HERO_FIGHT : Cursor::CURSOR_HERO_MOVE ),
                                                   from_hero.getNumOfTravelDays( tile.GetIndex() ) );
                }
            }
            else if ( from_hero.GetIndex() == castle->GetIndex() ) {
                return from_hero.GetColor() == castle->GetColor() ? Cursor::CASTLE : Cursor::POINTER;
            }
            else if ( from_hero.GetColor() == castle->GetColor() ) {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_ACTION, from_hero.getNumOfTravelDays( castle->GetIndex() ) );
            }
            else if ( from_hero.isFriends( castle->GetColor() ) ) {
                return Cursor::POINTER;
            }
            else if ( castle->GetActualArmy().isValid() ) {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_FIGHT, from_hero.getNumOfTravelDays( castle->GetIndex() ) );
            }
            else {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_ACTION, from_hero.getNumOfTravelDays( castle->GetIndex() ) );
            }
        }
        break;
    }

    case MP2::OBJ_HEROES: {
        const Heroes * to_hero = tile.GetHeroes();

        if ( nullptr != to_hero ) {
            if ( to_hero->GetCenter() == from_hero.GetCenter() ) {
                return Cursor::HEROES;
            }
            else if ( from_hero.GetColor() == to_hero->GetColor() ) {
                const int cursor = Cursor::DistanceThemes( Cursor::CURSOR_HERO_MEET, from_hero.getNumOfTravelDays( tile.GetIndex() ) );

                return cursor != Cursor::POINTER ? cursor : Cursor::HEROES;
            }
            else if ( from_hero.isFriends( to_hero->GetColor() ) ) {
                return Cursor::POINTER;
            }
            else {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_FIGHT, from_hero.getNumOfTravelDays( tile.GetIndex() ) );
            }
        }
        break;
    }

    case MP2::OBJ_BOAT:
        return Cursor::DistanceThemes( Cursor::CURSOR_HERO_BOAT, from_hero.getNumOfTravelDays( tile.GetIndex() ) );

    case MP2::OBJ_BARRIER:
        return Cursor::DistanceThemes( Cursor::CURSOR_HERO_ACTION, from_hero.getNumOfTravelDays( tile.GetIndex() ) );

    default:
        if ( MP2::isActionObject( tile.GetObject() ) ) {
            bool protection = false;

            if ( MP2::isPickupObject( tile.GetObject() ) ) {
                protection = Maps::isTileUnderProtection( tile.GetIndex() );
            }
            else {
                protection = ( Maps::isTileUnderProtection( tile.GetIndex() ) || ( !from_hero.isFriends( tile.QuantityColor() ) && tile.isCaptureObjectProtected() ) );
            }

            return Cursor::DistanceThemes( ( protection ? Cursor::CURSOR_HERO_FIGHT : Cursor::CURSOR_HERO_ACTION ), from_hero.getNumOfTravelDays( tile.GetIndex() ) );
        }
        else if ( tile.isPassableFrom( Direction::CENTER, from_hero.isShipMaster(), false, from_hero.GetColor() ) ) {
            const bool protection = Maps::isTileUnderProtection( tile.GetIndex() );

            return Cursor::DistanceThemes( ( protection ? Cursor::CURSOR_HERO_FIGHT : Cursor::CURSOR_HERO_MOVE ), from_hero.getNumOfTravelDays( tile.GetIndex() ) );
        }
        break;
    }

    return Cursor::POINTER;
}

int Interface::Basic::GetCursorTileIndex( int32_t dst_index )
{
    if ( dst_index < 0 || dst_index >= world.w() * world.h() )
        return Cursor::POINTER;

    const Maps::Tiles & tile = world.GetTiles( dst_index );
    if ( tile.isFog( Settings::Get().CurrentColor() ) )
        return Cursor::POINTER;

    switch ( GetFocusType() ) {
    case GameFocus::HEROES:
        return GetCursorFocusHeroes( *GetFocusHeroes(), tile );

    case GameFocus::CASTLE:
        return GetCursorFocusCastle( *GetFocusCastle(), tile );

    default:
        break;
    }

    return Cursor::POINTER;
}

fheroes2::GameMode Interface::Basic::StartGame()
{
    Settings & conf = Settings::Get();
    fheroes2::Display & display = fheroes2::Display::instance();

    Reset();

    radar.Build();
    radar.SetHide( true );

    // Hide the world map at the first drawing
    const int currentColor = conf.CurrentColor();
    conf.SetCurrentColor( -1 );

    iconsPanel.HideIcons( ICON_ANY );
    statusWindow.Reset();

    Redraw( REDRAW_GAMEAREA | REDRAW_RADAR | REDRAW_ICONS | REDRAW_BUTTONS | REDRAW_STATUS | REDRAW_BORDER );

    conf.SetCurrentColor( currentColor );

    bool loadedFromSave = conf.LoadedGameVersion();
    bool skipTurns = loadedFromSave;

    GameOver::Result & gameResult = GameOver::Result::Get();
    fheroes2::GameMode res = fheroes2::GameMode::END_TURN;

    std::vector<Player *> sortedPlayers = conf.GetPlayers().getVector();
    std::sort( sortedPlayers.begin(), sortedPlayers.end(), SortPlayers );

    if ( !loadedFromSave ) {
        // Clear fog around heroes, castles and mines for all players when starting a new map.
        for ( const Player * player : sortedPlayers ) {
            world.ClearFog( player->GetColor() );
        }
    }

    const bool isHotSeatGame = conf.IsGameType( Game::TYPE_HOTSEAT );
    if ( !isHotSeatGame ) {
        // Fully update fog directions if there will be only one human player.
        Interface::GameArea::updateMapFogDirections();
    }

    while ( res == fheroes2::GameMode::END_TURN ) {
        if ( !loadedFromSave ) {
            world.NewDay();
        }

        // check if the game is over at the beginning of a new day
        res = gameResult.LocalCheckGameOver();

        if ( res != fheroes2::GameMode::CANCEL ) {
            break;
        }

        res = fheroes2::GameMode::END_TURN;

        for ( const Player * player : sortedPlayers ) {
            assert( player != nullptr );

            Kingdom & kingdom = world.GetKingdom( player->GetColor() );

            if ( skipTurns && !player->isColor( conf.CurrentColor() ) ) {
                continue;
            }

            // player with conf.CurrentColor() was found, there is no need for further skips
            skipTurns = false;

            if ( kingdom.isPlay() ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, world.DateString() << ", color: " << Color::String( player->GetColor() ) << ", resource: " << kingdom.GetFunds().String() )

                radar.SetHide( true );
                radar.SetRedraw( REDRAW_RADAR_CURSOR );

                switch ( kingdom.GetControl() ) {
                case CONTROL_HUMAN:
                    // Reset environment sounds and music theme at the beginning of the human turn
                    AudioManager::ResetAudio();

                    if ( isHotSeatGame ) {
                        // we need to hide the world map in hot seat mode
                        conf.SetCurrentColor( -1 );

                        iconsPanel.HideIcons( ICON_ANY );
                        statusWindow.Reset();

                        // Fully update fog directions in Hot Seat mode to cover the map with fog on player change.
                        // TODO: Cover the Adventure map area with fog sprites without rendering the "Game Area" for player change.
                        Maps::Tiles::updateFogDirectionsInArea( { 0, 0 }, { world.w(), world.h() }, Color::NONE );

                        Redraw( REDRAW_GAMEAREA | REDRAW_ICONS | REDRAW_BUTTONS | REDRAW_STATUS );
                        display.render();

                        // reset the music after closing the dialog
                        const AudioManager::MusicRestorer musicRestorer;

                        AudioManager::PlayMusic( MUS::NEW_MONTH, Music::PlaybackMode::PLAY_ONCE );

                        Game::DialogPlayers( player->GetColor(), _( "%{color} player's turn." ) );
                    }

                    conf.SetCurrentColor( player->GetColor() );

                    kingdom.ActionBeforeTurn();

                    iconsPanel.ShowIcons( ICON_ANY );
                    iconsPanel.SetRedraw();

                    res = HumanTurn( loadedFromSave );

                    // Skip resetting Audio after winning scenario because MUS::VICTORY should continue playing.
                    if ( res == fheroes2::GameMode::HIGHSCORES_STANDARD ) {
                        break;
                    }

                    // Reset environment sounds and music theme at the end of the human turn.
                    AudioManager::ResetAudio();

                    break;

                // CONTROL_AI turn
                default:
                    // TODO: remove this temporary assertion
                    assert( res == fheroes2::GameMode::END_TURN );

                    Cursor::Get().SetThemes( Cursor::WAIT );

                    conf.SetCurrentColor( player->GetColor() );

                    statusWindow.Reset();
                    statusWindow.SetState( StatusType::STATUS_AITURN );

#if defined( WITH_DEBUG )
                    if ( player->isAIAutoControlMode() ) {
                        // If player gave control to AI we show the radar image and update it fully at the start of player's turn.
                        radar.SetHide( false );
                        radar.SetRedraw( REDRAW_RADAR );
                    }
#endif

                    Redraw();
                    display.render();

                    // In Hot Seat mode there could be different alliances so we have to update fog directions for some cases.
                    if ( isHotSeatGame ) {
                        Maps::Tiles::updateFogDirectionsInArea( { 0, 0 }, { world.w(), world.h() }, hotSeatAIFogColors( player ) );
                    }

                    kingdom.ActionBeforeTurn();

                    AI::Get().KingdomTurn( kingdom );

                    break;
                }

                if ( res != fheroes2::GameMode::END_TURN ) {
                    break;
                }

                // check if the game is over after each player's turn
                res = gameResult.LocalCheckGameOver();

                if ( res != fheroes2::GameMode::CANCEL ) {
                    break;
                }

                res = fheroes2::GameMode::END_TURN;
            }

            // reset this after potential HumanTurn() call, but regardless of whether current kingdom
            // is vanquished - next alive kingdom should start a new day from scratch
            loadedFromSave = false;
        }

        // we went through all the players, but the current player from the save file is still not found,
        // something is clearly wrong here
        if ( skipTurns ) {
            DEBUG_LOG( DBG_GAME, DBG_WARN,
                       "the current player from the save file was not found"
                           << ", player color: " << Color::String( conf.CurrentColor() ) )

            res = fheroes2::GameMode::MAIN_MENU;
        }

        // don't carry the current color from the last player to the next turn
        conf.SetCurrentColor( -1 );
    }

    // if we are here, the res value should never be fheroes2::GameMode::END_TURN
    assert( res != fheroes2::GameMode::END_TURN );

    if ( Settings::isFadeEffectEnabled() )
        fheroes2::FadeDisplay();

    return res;
}

fheroes2::GameMode Interface::Basic::HumanTurn( bool isload )
{
    fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

    const Settings & conf = Settings::Get();

    Kingdom & myKingdom = world.GetKingdom( conf.CurrentColor() );
    const KingdomCastles & myCastles = myKingdom.GetCastles();

    if ( isload ) {
        updateFocus();
    }
    else {
        ResetFocus( GameFocus::FIRSTHERO );
    }

    radar.SetHide( false );
    statusWindow.Reset();
    gameArea.SetUpdateCursor();

    if ( conf.IsGameType( Game::TYPE_HOTSEAT ) ) {
        // TODO: Cache fog directions for all Human players in array to not perform full update at every turn start.

        // Fully update fog directions at the start of player's move in Hot Seat mode as the previous move could be done by opposing player.
        Interface::GameArea::updateMapFogDirections();
    }

    Redraw( REDRAW_GAMEAREA | REDRAW_RADAR | REDRAW_ICONS | REDRAW_BUTTONS | REDRAW_STATUS | REDRAW_BORDER );

    fheroes2::Display & display = fheroes2::Display::instance();

    display.render();

    if ( !isload ) {
        if ( 1 < world.CountWeek() && world.BeginWeek() ) {
            ShowNewWeekDialog();
        }

        ShowEventDayDialog();

        if ( conf.isAutoSaveAtBeginningOfTurnEnabled() ) {
            Game::AutoSave();
        }
    }

    GameOver::Result & gameResult = GameOver::Result::Get();

    // check if the game is over at the beginning of each human-controlled player's turn
    res = gameResult.LocalCheckGameOver();

    if ( res == fheroes2::GameMode::CANCEL && myCastles.empty() ) {
        ShowWarningLostTownsDialog();
    }

    int fastScrollRepeatCount = 0;
    const int fastScrollStartThreshold = 2;

    bool isMovingHero = false;
    bool stopHero = false;

    int heroAnimationFrameCount = 0;
    fheroes2::Point heroAnimationOffset;
    int heroAnimationSpriteId = 0;

    bool isCursorOverButtons = false;
    bool isCursorOverGamearea = false;

    const std::vector<Game::DelayType> delayTypes = { Game::CURRENT_HERO_DELAY, Game::MAPS_DELAY };

    LocalEvent & le = LocalEvent::Get();
    Cursor & cursor = Cursor::Get();

    while ( res == fheroes2::GameMode::CANCEL ) {
        if ( !le.HandleEvents( Game::isDelayNeeded( delayTypes ), true ) ) {
            if ( EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                res = fheroes2::GameMode::QUIT_GAME;
                break;
            }
            continue;
        }

        // pending timer events
        statusWindow.TimerEventProcessing();

        // hotkeys
        if ( le.KeyPress() ) {
            // if the hero is currently moving, pressing any key should stop him
            if ( isMovingHero ) {
                stopHero = true;
            }

#if defined( WITH_DEBUG )
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_TRANSFER_CONTROL_TO_AI ) ) {
                if ( fheroes2::showMessage( fheroes2::Text( _( "Warning" ), fheroes2::FontType::normalYellow() ),
                                            fheroes2::Text( _( "Do you want to transfer control from you to the AI? The effect will take place only on the next turn." ),
                                                            fheroes2::FontType::normalWhite() ),
                                            Dialog::YES | Dialog::NO )
                     == Dialog::YES ) {
                    Players::Get( myKingdom.GetColor() )->setAIAutoControlMode( true );
                    return fheroes2::GameMode::END_TURN;
                }
            }
#endif

            // adventure map control
            else if ( HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_QUIT ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) )
                res = EventExit();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_END_TURN ) )
                res = EventEndTurn();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_NEXT_HERO ) )
                EventNextHero();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_NEXT_TOWN ) )
                EventNextTown();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_NEW_GAME ) )
                res = EventNewGame();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SAVE_GAME ) )
                EventSaveGame();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME ) )
                res = EventLoadGame();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_FILE_OPTIONS ) )
                res = EventFileDialog();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_ADVENTURE_OPTIONS ) )
                res = EventAdventureDialog();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SYSTEM_OPTIONS ) )
                EventSystemDialog();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_PUZZLE_MAP ) )
                EventPuzzleMaps();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCENARIO_INFORMATION ) )
                res = EventScenarioInformation();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_CAST_SPELL ) )
                EventCastSpell();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_KINGDOM_SUMMARY ) )
                EventKingdomInfo();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_VIEW_WORLD ) )
                EventViewWorld();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_TOGGLE_CONTROL_PANEL ) )
                EventSwitchShowControlPanel();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_TOGGLE_RADAR ) )
                EventSwitchShowRadar();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_TOGGLE_BUTTONS ) )
                EventSwitchShowButtons();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_TOGGLE_STATUS ) )
                EventSwitchShowStatus();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_TOGGLE_ICONS ) )
                EventSwitchShowIcons();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_CONTINUE_HERO_MOVEMENT ) )
                EventContinueMovement();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_DIG_ARTIFACT ) )
                res = EventDigArtifact();
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SLEEP_HERO ) )
                EventSwitchHeroSleeping();
            // hero movement control
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_LEFT ) )
                EventKeyArrowPress( Direction::LEFT );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_RIGHT ) )
                EventKeyArrowPress( Direction::RIGHT );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_UP ) )
                EventKeyArrowPress( Direction::TOP );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_DOWN ) )
                EventKeyArrowPress( Direction::BOTTOM );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_UP_LEFT ) )
                EventKeyArrowPress( Direction::TOP_LEFT );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_UP_RIGHT ) )
                EventKeyArrowPress( Direction::TOP_RIGHT );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_DOWN_LEFT ) )
                EventKeyArrowPress( Direction::BOTTOM_LEFT );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_DOWN_RIGHT ) )
                EventKeyArrowPress( Direction::BOTTOM_RIGHT );
            // map scrolling control
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_LEFT ) )
                gameArea.SetScroll( SCROLL_LEFT );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_RIGHT ) )
                gameArea.SetScroll( SCROLL_RIGHT );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_UP ) )
                gameArea.SetScroll( SCROLL_TOP );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_DOWN ) )
                gameArea.SetScroll( SCROLL_BOTTOM );
            // default action
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_DEFAULT_ACTION ) )
                res = EventDefaultAction( res );
            // open focus
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_OPEN_FOCUS ) )
                EventOpenFocus();
        }

        if ( res != fheroes2::GameMode::CANCEL ) {
            break;
        }

        if ( fheroes2::cursor().isFocusActive() && !gameArea.isDragScroll() && !radar.isDragRadar() && ( conf.ScrollSpeed() != SCROLL_SPEED_NONE ) ) {
            int scrollPosition = SCROLL_NONE;

            if ( isScrollLeft( le.GetMouseCursor() ) )
                scrollPosition |= SCROLL_LEFT;
            else if ( isScrollRight( le.GetMouseCursor() ) )
                scrollPosition |= SCROLL_RIGHT;
            if ( isScrollTop( le.GetMouseCursor() ) )
                scrollPosition |= SCROLL_TOP;
            else if ( isScrollBottom( le.GetMouseCursor() ) )
                scrollPosition |= SCROLL_BOTTOM;

            if ( scrollPosition != SCROLL_NONE ) {
                if ( Game::validateAnimationDelay( Game::SCROLL_START_DELAY ) ) {
                    if ( fastScrollRepeatCount < fastScrollStartThreshold ) {
                        ++fastScrollRepeatCount;
                    }
                }

                if ( fastScrollRepeatCount >= fastScrollStartThreshold ) {
                    gameArea.SetScroll( scrollPosition );
                }
            }
            else {
                fastScrollRepeatCount = 0;
            }
        }
        else {
            fastScrollRepeatCount = 0;
        }

        const bool isHiddenInterface = conf.isHideInterfaceEnabled();
        const bool prevIsCursorOverButtons = isCursorOverButtons;
        isCursorOverButtons = false;
        isCursorOverGamearea = false;

        if ( isMovingHero ) {
            // hero is moving, set the appropriate cursor
            if ( cursor.Themes() != Cursor::WAIT ) {
                cursor.SetThemes( Cursor::WAIT );
            }

            // if the hero is currently moving, pressing any mouse button should stop him
            const fheroes2::Rect displayArea{ 0, 0, display.width(), display.height() };
            if ( le.MouseClickLeft( displayArea ) || le.MousePressRight( displayArea ) ) {
                stopHero = true;
            }
        }
        // cursor is over the status window
        else if ( ( !isHiddenInterface || conf.ShowStatus() ) && le.MouseCursor( statusWindow.GetRect() ) ) {
            if ( Cursor::POINTER != cursor.Themes() )
                cursor.SetThemes( Cursor::POINTER );
            statusWindow.QueueEventProcessing();
        }
        // cursor is over the buttons area
        else if ( ( !isHiddenInterface || conf.ShowButtons() ) && le.MouseCursor( buttonsArea.GetRect() ) ) {
            if ( Cursor::POINTER != cursor.Themes() )
                cursor.SetThemes( Cursor::POINTER );
            res = buttonsArea.QueueEventProcessing();
            isCursorOverButtons = true;
        }
        // cursor is over the icons panel
        else if ( ( !isHiddenInterface || conf.ShowIcons() ) && le.MouseCursor( iconsPanel.GetRect() ) ) {
            if ( Cursor::POINTER != cursor.Themes() )
                cursor.SetThemes( Cursor::POINTER );
            iconsPanel.QueueEventProcessing();
        }
        // cursor is over the radar
        else if ( ( !isHiddenInterface || conf.ShowRadar() ) && le.MouseCursor( radar.GetRect() ) ) {
            if ( Cursor::POINTER != cursor.Themes() )
                cursor.SetThemes( Cursor::POINTER );
            if ( !gameArea.isDragScroll() )
                radar.QueueEventProcessing();
        }
        // cursor is over the control panel
        else if ( isHiddenInterface && conf.ShowControlPanel() && le.MouseCursor( controlPanel.GetArea() ) ) {
            if ( Cursor::POINTER != cursor.Themes() )
                cursor.SetThemes( Cursor::POINTER );
            res = controlPanel.QueueEventProcessing();
        }
        // cursor is over the game area
        else if ( le.MouseCursor( gameArea.GetROI() ) && !gameArea.NeedScroll() ) {
            isCursorOverGamearea = true;
        }
        // cursor is somewhere else
        else if ( !gameArea.NeedScroll() ) {
            if ( Cursor::POINTER != cursor.Themes() )
                cursor.SetThemes( Cursor::POINTER );
            gameArea.ResetCursorPosition();
        }

        // gamearea
        if ( !gameArea.NeedScroll() && !isMovingHero ) {
            if ( !radar.isDragRadar() )
                gameArea.QueueEventProcessing( isCursorOverGamearea );
            else if ( !le.MousePressLeft() )
                radar.QueueEventProcessing();
        }

        if ( prevIsCursorOverButtons && !isCursorOverButtons ) {
            buttonsArea.ResetButtons();
        }

        if ( res != fheroes2::GameMode::CANCEL ) {
            break;
        }

        // animation of the hero's movement
        if ( Game::validateAnimationDelay( Game::CURRENT_HERO_DELAY ) ) {
            Heroes * hero = GetFocusHeroes();

            if ( hero ) {
                bool resetHeroSprite = false;
                if ( heroAnimationFrameCount > 0 ) {
                    const int32_t heroMovementSkipValue = Game::HumanHeroAnimSkip();

                    gameArea.ShiftCenter( { heroAnimationOffset.x * heroMovementSkipValue, heroAnimationOffset.y * heroMovementSkipValue } );
                    gameArea.SetRedraw();

                    if ( heroAnimationOffset != fheroes2::Point() ) {
                        Game::EnvironmentSoundMixer();
                    }

                    heroAnimationFrameCount -= heroMovementSkipValue;
                    if ( ( heroAnimationFrameCount & 0x3 ) == 0 ) { // % 4
                        hero->SetSpriteIndex( heroAnimationSpriteId );

                        if ( heroAnimationFrameCount == 0 )
                            resetHeroSprite = true;
                        else
                            ++heroAnimationSpriteId;
                    }
                    const int offsetStep = ( ( 4 - ( heroAnimationFrameCount & 0x3 ) ) & 0x3 ); // % 4
                    hero->SetOffset( { heroAnimationOffset.x * offsetStep, heroAnimationOffset.y * offsetStep } );
                }

                if ( heroAnimationFrameCount == 0 ) {
                    if ( resetHeroSprite ) {
                        hero->SetSpriteIndex( heroAnimationSpriteId - 1 );
                    }

                    if ( hero->isMoveEnabled() ) {
                        if ( hero->Move( 10 == conf.HeroesMoveSpeed() ) ) {
                            // Do not generate a frame as we are going to do it later.
                            Interface::Basic::RedrawLocker redrawLocker( Interface::Basic::Get() );

                            gameArea.SetCenter( hero->GetCenter() );
                            ResetFocus( GameFocus::HEROES );

                            RedrawFocus();

                            if ( stopHero ) {
                                stopHero = false;

                                hero->SetMove( false );
                            }
                        }
                        else {
                            const fheroes2::Point movement( hero->MovementDirection() );
                            if ( movement != fheroes2::Point() ) { // don't waste resources for no movement
                                // Do not generate a frame as we are going to do it later.
                                Interface::Basic::RedrawLocker redrawLocker( Interface::Basic::Get() );

                                const int32_t heroMovementSkipValue = Game::HumanHeroAnimSkip();

                                heroAnimationOffset = movement;
                                gameArea.ShiftCenter( movement );

                                Game::SetUpdateSoundsOnFocusUpdate( false );
                                ResetFocus( GameFocus::HEROES );
                                Game::SetUpdateSoundsOnFocusUpdate( true );
                                heroAnimationFrameCount = 32 - heroMovementSkipValue;
                                heroAnimationSpriteId = hero->GetSpriteIndex();
                                if ( heroMovementSkipValue < 4 ) {
                                    hero->SetSpriteIndex( heroAnimationSpriteId - 1 );
                                    hero->SetOffset( { heroAnimationOffset.x * heroMovementSkipValue, heroAnimationOffset.y * heroMovementSkipValue } );
                                }
                                else {
                                    ++heroAnimationSpriteId;
                                }
                            }

                            gameArea.SetRedraw();
                        }

                        isMovingHero = true;

                        if ( hero->isAction() ) {
                            // check if the game is over after the hero's action
                            res = gameResult.LocalCheckGameOver();

                            hero->ResetAction();
                        }
                    }
                    else {
                        isMovingHero = false;
                        stopHero = false;

                        hero->SetMove( false );

                        gameArea.SetUpdateCursor();
                    }
                }
            }
            else {
                isMovingHero = false;
                stopHero = false;
            }
        }

        // fast scroll
        if ( ( gameArea.NeedScroll() && !isMovingHero ) || gameArea.needDragScrollRedraw() ) {
            if ( Game::validateAnimationDelay( Game::SCROLL_DELAY ) ) {
                if ( ( isScrollLeft( le.GetMouseCursor() ) || isScrollRight( le.GetMouseCursor() ) || isScrollTop( le.GetMouseCursor() )
                       || isScrollBottom( le.GetMouseCursor() ) )
                     && !gameArea.isDragScroll() ) {
                    cursor.SetThemes( gameArea.GetScrollCursor() );
                }

                gameArea.Scroll();

                gameArea.SetRedraw();
                radar.SetRedraw( REDRAW_RADAR_CURSOR );
            }
        }

        // map objects animation
        if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
            Game::updateAdventureMapAnimationIndex();
            gameArea.SetRedraw();
        }

        // check that the kingdom is not vanquished yet (has at least one hero or castle)
        if ( res == fheroes2::GameMode::CANCEL && !myKingdom.isPlay() ) {
            res = fheroes2::GameMode::END_TURN;
        }

        if ( NeedRedraw() ) {
            Redraw();

            // If this assertion blows up it means that we are holding a RedrawLocker lock for rendering which should not happen.
            assert( GetRedrawMask() == 0 );

            display.render();
        }
    }

    if ( res == fheroes2::GameMode::END_TURN ) {
        if ( GetFocusHeroes() ) {
            GetFocusHeroes()->ShowPath( false );

            SetRedraw( REDRAW_GAMEAREA );
        }

        if ( myKingdom.isPlay() ) {
            // these warnings should be shown at the end of the turn
            if ( myCastles.empty() ) {
                const uint32_t lostTownDays = myKingdom.GetLostTownDays();

                if ( lostTownDays > Game::GetLostTownDays() ) {
                    Game::DialogPlayers( conf.CurrentColor(),
                                         _( "%{color} player, you have lost your last town. If you do not conquer another town in next week, you will be eliminated." ) );
                }
                else if ( lostTownDays == 1 ) {
                    Game::DialogPlayers( conf.CurrentColor(), _( "%{color} player, your heroes abandon you, and you are banished from this land." ) );
                }
            }

            if ( !conf.isAutoSaveAtBeginningOfTurnEnabled() ) {
                Game::AutoSave();
            }
        }
    }

    return res;
}

void Interface::Basic::MouseCursorAreaClickLeft( const int32_t index_maps )
{
    Heroes * from_hero = GetFocusHeroes();
    const Maps::Tiles & tile = world.GetTiles( index_maps );

    switch ( Cursor::WithoutDistanceThemes( Cursor::Get().Themes() ) ) {
    case Cursor::HEROES: {
        Heroes * to_hero = tile.GetHeroes();
        // focus change/open hero
        if ( nullptr != to_hero ) {
            if ( !from_hero || from_hero != to_hero ) {
                SetFocus( to_hero );
                CalculateHeroPath( to_hero, -1 );
                RedrawFocus();
            }
            else {
                Game::OpenHeroesDialog( *to_hero, true, true );
                Cursor::Get().SetThemes( Cursor::HEROES );
            }
        }
        break;
    }

    case Cursor::CASTLE: {
        // correct index for castle
        const MP2::MapObjectType objectType = tile.GetObject();
        if ( MP2::OBJ_NON_ACTION_CASTLE != objectType && MP2::OBJ_CASTLE != objectType )
            break;

        Castle * to_castle = world.getCastle( tile.GetCenter() );
        if ( to_castle == nullptr )
            break;

        const Castle * from_castle = GetFocusCastle();
        if ( !from_castle || from_castle != to_castle ) {
            SetFocus( to_castle );
            RedrawFocus();
        }
        else {
            Game::OpenCastleDialog( *to_castle );
            Cursor::Get().SetThemes( Cursor::CASTLE );
        }
        break;
    }
    case Cursor::CURSOR_HERO_FIGHT:
    case Cursor::CURSOR_HERO_MOVE:
    case Cursor::CURSOR_HERO_BOAT:
    case Cursor::CURSOR_HERO_ANCHOR:
    case Cursor::CURSOR_HERO_MEET:
    case Cursor::CURSOR_HERO_ACTION:
    case Cursor::CURSOR_HERO_BOAT_ACTION:
        if ( from_hero == nullptr )
            break;

        if ( from_hero->isMoveEnabled() )
            from_hero->SetMove( false );
        else
            ShowPathOrStartMoveHero( from_hero, index_maps );
        break;

    default:
        if ( from_hero )
            from_hero->SetMove( false );
        break;
    }
}

void Interface::Basic::MouseCursorAreaPressRight( int32_t index_maps ) const
{
    Heroes * hero = GetFocusHeroes();

    // stop hero
    if ( hero && hero->isMoveEnabled() ) {
        hero->SetMove( false );
        Cursor::Get().SetThemes( GetCursorTileIndex( index_maps ) );
    }
    else {
        const Settings & conf = Settings::Get();
        const Maps::Tiles & tile = world.GetTiles( index_maps );

        DEBUG_LOG( DBG_DEVEL, DBG_INFO, std::endl << tile.String() )

        if ( !IS_DEVEL() && tile.isFog( conf.CurrentColor() ) )
            Dialog::QuickInfo( tile );
        else
            switch ( tile.GetObject() ) {
            case MP2::OBJ_NON_ACTION_CASTLE:
            case MP2::OBJ_CASTLE: {
                const Castle * castle = world.getCastle( tile.GetCenter() );
                if ( castle ) {
                    Dialog::QuickInfo( *castle );
                }
                else {
                    Dialog::QuickInfo( tile );
                }
                break;
            }

            case MP2::OBJ_HEROES: {
                const Heroes * heroes = tile.GetHeroes();
                if ( heroes ) {
                    Dialog::QuickInfo( *heroes );
                }
                break;
            }

            default:
                Dialog::QuickInfo( tile );
                break;
            }
    }
}
