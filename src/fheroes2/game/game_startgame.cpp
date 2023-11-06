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
#include "interface_base.h"
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
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "monster.h"
#include "mp2.h"
#include "mus.h"
#include "players.h"
#include "resource.h"
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

    return Interface::AdventureMap::Get().StartGame();
}

void Game::DialogPlayers( int color, std::string title, std::string message )
{
    const Player * player = Players::Get( color );
    StringReplace( message, "%{color}", ( player ? player->GetName() : Color::String( color ) ) );

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
    fheroes2::showMessage( fheroes2::Text( std::move( title ), fheroes2::FontType::normalYellow() ),
                           fheroes2::Text( std::move( message ), fheroes2::FontType::normalWhite() ), Dialog::OK, { &imageUI } );
}

void Game::OpenCastleDialog( Castle & castle, bool updateFocus /* = true */, const bool renderBackgroundDialog /* = true */ )
{
    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    // Stop all sounds, but not the music - it will be replaced by the music of the castle
    AudioManager::stopSounds();

    const Settings & conf = Settings::Get();
    Kingdom & myKingdom = world.GetKingdom( conf.CurrentColor() );
    const VecCastles & myCastles = myKingdom.GetCastles();
    VecCastles::const_iterator it = std::find( myCastles.begin(), myCastles.end(), &castle );

    const size_t heroCountBefore = myKingdom.GetHeroes().size();

    assert( it != myCastles.end() );

    Castle::CastleDialogReturnValue result = ( *it )->OpenDialog( false, true, renderBackgroundDialog );

    while ( result != Castle::CastleDialogReturnValue::Close ) {
        if ( result == Castle::CastleDialogReturnValue::PreviousCastle || result == Castle::CastleDialogReturnValue::PreviousCostructionWindow ) {
            if ( it == myCastles.begin() ) {
                it = myCastles.end();
            }
            --it;
        }
        else if ( result == Castle::CastleDialogReturnValue::NextCastle || result == Castle::CastleDialogReturnValue::NextCostructionWindow ) {
            ++it;
            if ( it == myCastles.end() ) {
                it = myCastles.begin();
            }
        }

        assert( it != myCastles.end() );

        const bool openConstructionWindow
            = ( result == Castle::CastleDialogReturnValue::PreviousCostructionWindow ) || ( result == Castle::CastleDialogReturnValue::NextCostructionWindow );

        result = ( *it )->OpenDialog( openConstructionWindow, false, renderBackgroundDialog );
    }

    // If Castle dialog background was not rendered than we have opened it from other dialog (Kingdom Overview)
    // and there is no need update Adventure map interface at this time.
    if ( renderBackgroundDialog ) {
        Interface::AdventureMap & adventureMapInterface = Interface::AdventureMap::Get();

        if ( updateFocus ) {
            if ( heroCountBefore < myKingdom.GetHeroes().size() ) {
                adventureMapInterface.SetFocus( myKingdom.GetHeroes()[heroCountBefore], false );
            }
            else if ( it != myCastles.end() ) {
                Heroes * heroInCastle = world.GetTiles( ( *it )->GetIndex() ).getHero();
                if ( heroInCastle == nullptr ) {
                    adventureMapInterface.SetFocus( *it );
                }
                else {
                    adventureMapInterface.SetFocus( heroInCastle, false );
                }
            }
            else {
                adventureMapInterface.ResetFocus( GameFocus::HEROES, false );
            }
        }
        else {
            // If we don't update focus, we still have to restore environment sounds and terrain music theme
            restoreSoundsForCurrentFocus();
        }

        // The castle garrison can change
        adventureMapInterface.RedrawFocus();
        adventureMapInterface.ResetFocus( Interface::GetFocusType(), false );

        // Fade-in game screen only for 640x480 resolution.
        if ( fheroes2::Display::instance().isDefaultSize() ) {
            setDisplayFadeIn();
        }
    }
    else {
        // If we opened the castle dialog from other dialog, we have to restore environment sounds and terrain music theme instead of the castle's music theme
        restoreSoundsForCurrentFocus();
    }
}

void Game::OpenHeroesDialog( Heroes & hero, bool updateFocus, const bool renderBackgroundDialog, const bool disableDismiss /* = false */ )
{
    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    Interface::AdventureMap & adventureMapInterface = Interface::AdventureMap::Get();

    const VecHeroes & myHeroes = hero.GetKingdom().GetHeroes();
    VecHeroes::const_iterator it = std::find( myHeroes.begin(), myHeroes.end(), &hero );

    const bool isDefaultScreenSize = fheroes2::Display::instance().isDefaultSize();
    bool needFade = true;
    int result = Dialog::ZERO;

    while ( it != myHeroes.end() && result != Dialog::CANCEL ) {
        result = ( *it )->OpenDialog( false, needFade, disableDismiss, false, renderBackgroundDialog );

        if ( needFade ) {
            needFade = false;
        }

        switch ( result ) {
        case Dialog::PREV:
            if ( it == myHeroes.begin() ) {
                it = myHeroes.end();
            }
            --it;
            break;

        case Dialog::NEXT:
            ++it;
            if ( it == myHeroes.end() ) {
                it = myHeroes.begin();
            }
            break;

        case Dialog::DISMISS:
            AudioManager::PlaySound( M82::KILLFADE );

            ( *it )->ShowPath( false );

            // Check if this dialog is not opened from the other dialog and we will be exiting to the Adventure map.
            if ( renderBackgroundDialog ) {
                // Redraw Adventure map with hidden hero path.
                adventureMapInterface.redraw( Interface::REDRAW_GAMEAREA );

                // Fade-in game screen only for 640x480 resolution.
                if ( isDefaultScreenSize ) {
                    fheroes2::fadeInDisplay();
                }

                ( *it )->FadeOut();
                updateFocus = true;
            }

            ( *it )->Dismiss( 0 );
            it = myHeroes.end();

            result = Dialog::CANCEL;
            break;

        case Dialog::CANCEL:
            needFade = true;
            break;

        default:
            break;
        }
    }

    // If Hero dialog background was not rendered than we have opened it from other dialog (Kingdom Overview or Castle dialog)
    // and there is no need update Adventure map interface at this time.
    if ( renderBackgroundDialog ) {
        if ( updateFocus ) {
            if ( it != myHeroes.end() ) {
                adventureMapInterface.SetFocus( *it, false );
            }
            else {
                adventureMapInterface.ResetFocus( GameFocus::HEROES, false );
            }
        }
        // The hero's army can change
        adventureMapInterface.RedrawFocus();

        // Fade-in game screen only for 640x480 resolution.
        if ( needFade && renderBackgroundDialog && isDefaultScreenSize ) {
            setDisplayFadeIn();
        }
    }
}

void ShowNewWeekDialog()
{
    // restore the original music on exit
    const AudioManager::MusicRestorer musicRestorer;

    const bool isNewMonth = world.BeginMonth();

    AudioManager::PlayMusic( isNewMonth ? MUS::NEW_MONTH : MUS::NEW_WEEK, Music::PlaybackMode::PLAY_ONCE );

    const Week & week = world.GetWeekType();

    // head
    std::string message = isNewMonth ? _( "Astrologers proclaim the Month of the %{name}." ) : _( "Astrologers proclaim the Week of the %{name}." );
    StringReplace( message, "%{name}", week.GetName() );
    message += "\n\n";

    if ( week.GetType() == WeekName::MONSTERS ) {
        const Monster monster( week.GetMonster() );
        const uint32_t count = isNewMonth ? Castle::GetGrownMonthOf() : Castle::GetGrownWeekOf();

        if ( monster.isValid() && count ) {
            if ( isNewMonth )
                message += 100 == Castle::GetGrownMonthOf() ? _( "After regular growth, the population of %{monster} is doubled!" )
                                                            : _n( "After regular growth, the population of %{monster} increases by %{count} percent!",
                                                                  "After regular growth, the population of %{monster} increases by %{count} percent!", count );
            else
                message += _( "%{monster} growth +%{count}." );
            StringReplaceWithLowercase( message, "%{monster}", monster.GetMultiName() );
            StringReplace( message, "%{count}", count );
            message += "\n\n";
        }
    }

    if ( week.GetType() == WeekName::PLAGUE )
        message += _( " All populations are halved." );
    else
        message += _( " All dwellings increase population." );

    fheroes2::showStandardTextMessage( isNewMonth ? _( "New Month!" ) : _( "New Week!" ), message, Dialog::OK );
}

void ShowWarningLostTownsDialog()
{
    const Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    const uint32_t lostTownDays = myKingdom.GetLostTownDays();

    if ( lostTownDays == 1 ) {
        Game::DialogPlayers( myKingdom.GetColor(), _( "Beware!" ),
                             _( "%{color} player, this is your last day to capture a town, or you will be banished from this land." ) );
    }
    else if ( lostTownDays > 0 && lostTownDays <= Game::GetLostTownDays() ) {
        std::string str = _( "%{color} player, you only have %{day} days left to capture a town, or you will be banished from this land." );
        StringReplace( str, "%{day}", lostTownDays );
        Game::DialogPlayers( myKingdom.GetColor(), _( "Beware!" ), str );
    }
}

int Interface::AdventureMap::GetCursorFocusCastle( const Castle & castle, const Maps::Tiles & tile )
{
    switch ( tile.GetObject() ) {
    case MP2::OBJ_NON_ACTION_CASTLE:
    case MP2::OBJ_CASTLE: {
        const Castle * otherCastle = world.getCastle( tile.GetCenter() );

        if ( otherCastle ) {
            return otherCastle->GetColor() == castle.GetColor() ? Cursor::CASTLE : Cursor::POINTER;
        }

        break;
    }

    case MP2::OBJ_HEROES: {
        const Heroes * hero = tile.getHero();

        if ( hero ) {
            return hero->GetColor() == castle.GetColor() ? Cursor::HEROES : Cursor::POINTER;
        }

        break;
    }

    default:
        break;
    }

    return Cursor::POINTER;
}

int Interface::AdventureMap::GetCursorFocusShipmaster( const Heroes & hero, const Maps::Tiles & tile )
{
    const bool isWater = tile.isWater();

    switch ( tile.GetObject() ) {
    case MP2::OBJ_NON_ACTION_CASTLE:
    case MP2::OBJ_CASTLE: {
        const Castle * castle = world.getCastle( tile.GetCenter() );

        if ( castle ) {
            if ( tile.GetObject() == MP2::OBJ_NON_ACTION_CASTLE && isWater && tile.isPassableFrom( Direction::CENTER, true, false, hero.GetColor() ) ) {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_BOAT, hero.getNumOfTravelDays( tile.GetIndex() ) );
            }

            return hero.GetColor() == castle->GetColor() ? Cursor::CASTLE : Cursor::POINTER;
        }

        break;
    }

    case MP2::OBJ_HEROES: {
        const Heroes * otherHero = tile.getHero();

        if ( otherHero ) {
            if ( !otherHero->isShipMaster() ) {
                return hero.GetColor() == otherHero->GetColor() ? Cursor::HEROES : Cursor::POINTER;
            }

            if ( otherHero->GetCenter() == hero.GetCenter() ) {
                return Cursor::HEROES;
            }

            if ( hero.GetColor() == otherHero->GetColor() ) {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_MEET, hero.getNumOfTravelDays( tile.GetIndex() ) );
            }

            if ( hero.isFriends( otherHero->GetColor() ) ) {
                return Cursor::POINTER;
            }

            return Cursor::DistanceThemes( Cursor::CURSOR_HERO_FIGHT, hero.getNumOfTravelDays( tile.GetIndex() ) );
        }

        break;
    }

    // Some map editors allow to place monsters on water tiles
    case MP2::OBJ_MONSTER:
        return isWater ? Cursor::DistanceThemes( Cursor::CURSOR_HERO_FIGHT, hero.getNumOfTravelDays( tile.GetIndex() ) ) : Cursor::POINTER;

    case MP2::OBJ_COAST:
        return Cursor::DistanceThemes( Cursor::CURSOR_HERO_ANCHOR, hero.getNumOfTravelDays( tile.GetIndex() ) );

    default:
        if ( isWater ) {
            if ( MP2::isWaterActionObject( tile.GetObject() ) ) {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_BOAT_ACTION, hero.getNumOfTravelDays( tile.GetIndex() ) );
            }

            if ( tile.isPassableFrom( Direction::CENTER, true, false, hero.GetColor() ) ) {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_BOAT, hero.getNumOfTravelDays( tile.GetIndex() ) );
            }
        }

        break;
    }

    return Cursor::POINTER;
}

int Interface::AdventureMap::_getCursorNoFocus( const Maps::Tiles & tile )
{
    switch ( tile.GetObject() ) {
    case MP2::OBJ_NON_ACTION_CASTLE:
    case MP2::OBJ_CASTLE: {
        const Castle * castle = world.getCastle( tile.GetCenter() );
        if ( castle && castle->GetColor() == Settings::Get().CurrentColor() ) {
            return Cursor::CASTLE;
        }
        break;
    }
    case MP2::OBJ_HEROES: {
        const Heroes * hero = tile.getHero();
        if ( hero && hero->GetColor() == Settings::Get().CurrentColor() ) {
            return Cursor::HEROES;
        }
        break;
    }
    default:
        break;
    }

    return Cursor::POINTER;
}

int Interface::AdventureMap::GetCursorFocusHeroes( const Heroes & hero, const Maps::Tiles & tile )
{
    if ( hero.Modes( Heroes::ENABLEMOVE ) ) {
        return Cursor::Get().Themes();
    }

    if ( hero.isShipMaster() ) {
        return GetCursorFocusShipmaster( hero, tile );
    }

    switch ( tile.GetObject() ) {
    case MP2::OBJ_NON_ACTION_CASTLE:
    case MP2::OBJ_CASTLE: {
        const Castle * castle = world.getCastle( tile.GetCenter() );

        if ( castle ) {
            if ( tile.GetObject() == MP2::OBJ_NON_ACTION_CASTLE ) {
                if ( tile.GetPassable() == 0 ) {
                    return ( hero.GetColor() == castle->GetColor() ) ? Cursor::CASTLE : Cursor::POINTER;
                }

                return Cursor::DistanceThemes( Maps::isTileUnderProtection( tile.GetIndex() ) ? Cursor::CURSOR_HERO_FIGHT : Cursor::CURSOR_HERO_MOVE,
                                               hero.getNumOfTravelDays( tile.GetIndex() ) );
            }

            if ( hero.GetIndex() == castle->GetIndex() ) {
                return hero.GetColor() == castle->GetColor() ? Cursor::CASTLE : Cursor::POINTER;
            }

            if ( hero.GetColor() == castle->GetColor() ) {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_ACTION, hero.getNumOfTravelDays( castle->GetIndex() ) );
            }

            if ( hero.isFriends( castle->GetColor() ) ) {
                return Cursor::POINTER;
            }

            if ( castle->GetActualArmy().isValid() ) {
                return Cursor::DistanceThemes( Cursor::CURSOR_HERO_FIGHT, hero.getNumOfTravelDays( castle->GetIndex() ) );
            }

            return Cursor::DistanceThemes( Cursor::CURSOR_HERO_ACTION, hero.getNumOfTravelDays( castle->GetIndex() ) );
        }

        break;
    }

    case MP2::OBJ_HEROES: {
        const Heroes * otherHero = tile.getHero();

        if ( otherHero ) {
            if ( otherHero->GetCenter() == hero.GetCenter() ) {
                return Cursor::HEROES;
            }

            if ( hero.GetColor() == otherHero->GetColor() ) {
                const int cursor = Cursor::DistanceThemes( Cursor::CURSOR_HERO_MEET, hero.getNumOfTravelDays( tile.GetIndex() ) );

                return cursor != Cursor::POINTER ? cursor : Cursor::HEROES;
            }

            if ( hero.isFriends( otherHero->GetColor() ) ) {
                return Cursor::POINTER;
            }

            return Cursor::DistanceThemes( Cursor::CURSOR_HERO_FIGHT, hero.getNumOfTravelDays( tile.GetIndex() ) );
        }
        break;
    }

    case MP2::OBJ_BOAT:
        return Cursor::DistanceThemes( Cursor::CURSOR_HERO_BOAT, hero.getNumOfTravelDays( tile.GetIndex() ) );

    default:
        if ( MP2::isActionObject( tile.GetObject() ) ) {
            const bool isProtected
                = ( Maps::isTileUnderProtection( tile.GetIndex() ) || ( !hero.isFriends( getColorFromTile( tile ) ) && isCaptureObjectProtected( tile ) ) );

            return Cursor::DistanceThemes( isProtected ? Cursor::CURSOR_HERO_FIGHT : Cursor::CURSOR_HERO_ACTION, hero.getNumOfTravelDays( tile.GetIndex() ) );
        }

        if ( tile.isPassableFrom( Direction::CENTER, hero.isShipMaster(), false, hero.GetColor() ) ) {
            return Cursor::DistanceThemes( Maps::isTileUnderProtection( tile.GetIndex() ) ? Cursor::CURSOR_HERO_FIGHT : Cursor::CURSOR_HERO_MOVE,
                                           hero.getNumOfTravelDays( tile.GetIndex() ) );
        }

        break;
    }

    return Cursor::POINTER;
}

void Interface::AdventureMap::updateCursor( const int32_t tileIndex )
{
    Cursor::Get().SetThemes( GetCursorTileIndex( tileIndex ) );
}

int Interface::AdventureMap::GetCursorTileIndex( int32_t dstIndex )
{
    if ( !Maps::isValidAbsIndex( dstIndex ) ) {
        return Cursor::POINTER;
    }

    const Maps::Tiles & tile = world.GetTiles( dstIndex );

    if ( tile.isFog( Settings::Get().CurrentColor() ) ) {
        return Cursor::POINTER;
    }

    switch ( GetFocusType() ) {
    case GameFocus::HEROES:
        return GetCursorFocusHeroes( *GetFocusHeroes(), tile );

    case GameFocus::CASTLE:
        return GetCursorFocusCastle( *GetFocusCastle(), tile );

    case GameFocus::UNSEL:
        return _getCursorNoFocus( tile );

    default:
        break;
    }

    return Cursor::POINTER;
}

fheroes2::GameMode Interface::AdventureMap::StartGame()
{
    Settings & conf = Settings::Get();

    reset();

    _radar.Build();
    _radar.SetHide( true );
    iconsPanel.HideIcons( ICON_ANY );
    _statusWindow.Reset();

    // Prepare for render the whole game interface with adventure map filled with fog as it was not uncovered by 'updateMapFogDirections()'.
    redraw( REDRAW_GAMEAREA | REDRAW_RADAR | REDRAW_ICONS | REDRAW_BUTTONS | REDRAW_STATUS | REDRAW_BORDER );

    bool loadedFromSave = conf.LoadedGameVersion();
    bool skipTurns = loadedFromSave;

    // Set need of fade-in of game screen.
    Game::setDisplayFadeIn();

    GameOver::Result & gameResult = GameOver::Result::Get();
    fheroes2::GameMode res = fheroes2::GameMode::END_TURN;

    std::vector<Player *> sortedPlayers = conf.GetPlayers().getVector();
    std::sort( sortedPlayers.begin(), sortedPlayers.end(), SortPlayers );

    if ( !loadedFromSave || world.CountDay() == 1 ) {
        // Clear fog around heroes, castles and mines for all players when starting a new map or if the save was done at the first day.
        for ( const Player * player : sortedPlayers ) {
            world.ClearFog( player->GetColor() );
        }
    }

    const bool isHotSeatGame = conf.IsGameType( Game::TYPE_HOTSEAT );
    if ( !isHotSeatGame ) {
        // It is not a Hot Seat (multiplayer) game so we set current color to the only human player.
        conf.SetCurrentColor( Players::HumanColors() );

        // Fully update fog directions if there will be only one human player.
        Interface::GameArea::updateMapFogDirections();
    }

    while ( res == fheroes2::GameMode::END_TURN ) {
        if ( !loadedFromSave ) {
            world.NewDay();
        }

        // check if the game is over at the beginning of a new day
        res = gameResult.checkGameOver();

        if ( res != fheroes2::GameMode::CANCEL ) {
            break;
        }

        res = fheroes2::GameMode::END_TURN;

        bool isFirstAIPlayer = true;

        for ( const Player * player : sortedPlayers ) {
            assert( player != nullptr );

            const int playerColor = player->GetColor();

            Kingdom & kingdom = world.GetKingdom( playerColor );

            if ( skipTurns && !player->isColor( conf.CurrentColor() ) ) {
                if ( kingdom.isPlay() && kingdom.GetControl() == CONTROL_AI ) {
                    // Only the first AI player can trigger AI bonuses.
                    // Since this player is the one the bonuses have been applied.
                    isFirstAIPlayer = false;
                }

                continue;
            }

            // player with conf.CurrentColor() was found, there is no need for further skips
            skipTurns = false;

            if ( kingdom.isPlay() ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, world.DateString() << ", color: " << Color::String( playerColor ) << ", resource: " << kingdom.GetFunds().String() )

                _radar.SetHide( true );
                _radar.SetRedraw( REDRAW_RADAR_CURSOR );

                switch ( kingdom.GetControl() ) {
                case CONTROL_HUMAN:
                    // Reset environment sounds and music theme at the beginning of the human turn
                    AudioManager::ResetAudio();

                    if ( isHotSeatGame ) {
                        iconsPanel.HideIcons( ICON_ANY );
                        _statusWindow.Reset();

                        // Fully update fog directions in Hot Seat mode to cover the map with fog on player change.
                        // TODO: Cover the Adventure map area with fog sprites without rendering the "Game Area" for player change.
                        Maps::updateFogDirectionsInArea( { 0, 0 }, { world.w(), world.h() }, Color::NONE );

                        redraw( REDRAW_GAMEAREA | REDRAW_ICONS | REDRAW_BUTTONS | REDRAW_STATUS );

                        validateFadeInAndRender();

                        // reset the music after closing the dialog
                        const AudioManager::MusicRestorer musicRestorer;

                        AudioManager::PlayMusic( MUS::NEW_MONTH, Music::PlaybackMode::PLAY_ONCE );

                        Game::DialogPlayers( playerColor, "", _( "%{color} player's turn." ) );
                    }

                    conf.SetCurrentColor( playerColor );

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
                case CONTROL_AI:
                    // TODO: remove this temporary assertion
                    assert( res == fheroes2::GameMode::END_TURN );

                    if ( isFirstAIPlayer ) {
                        isFirstAIPlayer = false;
                        // All bonuses for AI must be applied on the first AI player turn, not the first player in general.
                        // This prevents human players to abuse AI bonuses.
                        world.NewDayAI();
                    }

                    Cursor::Get().SetThemes( Cursor::WAIT );

                    conf.SetCurrentColor( playerColor );

                    _statusWindow.Reset();
                    _statusWindow.SetState( StatusType::STATUS_AITURN );

#if defined( WITH_DEBUG )
                    if ( player->isAIAutoControlMode() ) {
                        // If player gave control to AI we show the radar image and update it fully at the start of player's turn.
                        _radar.SetHide( false );
                        _radar.SetRedraw( REDRAW_RADAR );
                    }
#endif

                    redraw( 0 );
                    validateFadeInAndRender();

                    // In Hot Seat mode there could be different alliances so we have to update fog directions for some cases.
                    if ( isHotSeatGame ) {
                        Maps::updateFogDirectionsInArea( { 0, 0 }, { world.w(), world.h() }, hotSeatAIFogColors( player ) );
                    }

                    if ( !loadedFromSave ) {
                        kingdom.ActionNewDayResourceUpdate( nullptr );
                    }

                    kingdom.ActionBeforeTurn();

#if defined( WITH_DEBUG )
                    if ( !loadedFromSave && player->isAIAutoControlMode() && conf.isAutoSaveAtBeginningOfTurnEnabled() ) {
                        // This is a human player which gave control to AI so we need to do autosave here.
                        Game::AutoSave();
                    }
#endif

                    AI::Get().KingdomTurn( kingdom );

#if defined( WITH_DEBUG )
                    if ( !loadedFromSave && player->isAIAutoControlMode() && !conf.isAutoSaveAtBeginningOfTurnEnabled() ) {
                        // This is a human player which gave control to AI so we need to do autosave here.
                        Game::AutoSave();
                    }
#endif

                    break;
                default:
                    // So far no other player type is supported so this should not happen.
                    assert( 0 );
                    break;
                }

                if ( res != fheroes2::GameMode::END_TURN ) {
                    break;
                }

                // check if the game is over after each player's turn
                res = gameResult.checkGameOver();

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

        // Don't carry the current player color to the next turn.
        conf.SetCurrentColor( Color::NONE );
    }

    // if we are here, the res value should never be fheroes2::GameMode::END_TURN
    assert( res != fheroes2::GameMode::END_TURN );

    Game::setDisplayFadeIn();

    // Do not use fade-out effect when exiting to Highscores screen as in this case name input dialog will be rendered next.
    if ( res != fheroes2::GameMode::HIGHSCORES_STANDARD ) {
        fheroes2::fadeOutDisplay();
    }

    return res;
}

fheroes2::GameMode Interface::AdventureMap::HumanTurn( const bool isload )
{
    if ( isload ) {
        updateFocus();
    }
    else {
        ResetFocus( GameFocus::FIRSTHERO, false );
    }

    _radar.SetHide( false );
    _statusWindow.Reset();
    _gameArea.SetUpdateCursor();

    const Settings & conf = Settings::Get();
    if ( conf.IsGameType( Game::TYPE_HOTSEAT ) ) {
        // TODO: Cache fog directions for all Human players in array to not perform full update at every turn start.

        // Fully update fog directions at the start of player's move in Hot Seat mode as the previous move could be done by opposing player.
        Interface::GameArea::updateMapFogDirections();
    }

    redraw( REDRAW_GAMEAREA | REDRAW_RADAR | REDRAW_ICONS | REDRAW_BUTTONS | REDRAW_STATUS | REDRAW_BORDER );

    validateFadeInAndRender();

    Kingdom & myKingdom = world.GetKingdom( conf.CurrentColor() );

    if ( !isload ) {
        if ( 1 < world.CountWeek() && world.BeginWeek() ) {
            ShowNewWeekDialog();
        }

        myKingdom.ActionNewDayResourceUpdate( []( const EventDate & event, const Funds & funds ) {
            if ( funds.GetValidItemsCount() ) {
                fheroes2::showResourceMessage( fheroes2::Text( event.title, fheroes2::FontType::normalYellow() ),
                                               fheroes2::Text( event.message, fheroes2::FontType::normalWhite() ), Dialog::OK, funds );
            }
            else if ( !event.message.empty() ) {
                fheroes2::showStandardTextMessage( event.title, event.message, Dialog::OK );
            }
        } );

        // The amount of the kingdom resources has changed, the status window needs to be updated
        redraw( REDRAW_STATUS );
        fheroes2::Display::instance().render();

        if ( conf.isAutoSaveAtBeginningOfTurnEnabled() ) {
            Game::AutoSave();
        }
    }

    GameOver::Result & gameResult = GameOver::Result::Get();

    // check if the game is over at the beginning of each human-controlled player's turn
    fheroes2::GameMode res = gameResult.checkGameOver();

    const VecCastles & myCastles = myKingdom.GetCastles();
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

#if defined( WITH_DEBUG )
        {
            const Player * player = Players::Get( myKingdom.GetColor() );
            assert( player != nullptr );

            // Control has just been transferred to AI, end the turn immediately
            if ( player->isAIAutoControlMode() ) {
                return fheroes2::GameMode::END_TURN;
            }
        }
#endif

        // pending timer events
        _statusWindow.TimerEventProcessing();

        // hotkeys
        if ( le.KeyPress() ) {
            // if the hero is currently moving, pressing any key should stop him
            if ( isMovingHero ) {
                stopHero = true;
            }
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
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_START_HERO_MOVEMENT ) )
                res = EventHeroMovement();
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
                _gameArea.SetScroll( SCROLL_LEFT );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_RIGHT ) )
                _gameArea.SetScroll( SCROLL_RIGHT );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_UP ) )
                _gameArea.SetScroll( SCROLL_TOP );
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_DOWN ) )
                _gameArea.SetScroll( SCROLL_BOTTOM );
            // default action
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_DEFAULT_ACTION ) )
                res = EventDefaultAction();
            // open focus
            else if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_OPEN_FOCUS ) )
                EventOpenFocus();
        }

        if ( res != fheroes2::GameMode::CANCEL ) {
            break;
        }

        if ( fheroes2::cursor().isFocusActive() && !_gameArea.isDragScroll() && !_radar.isDragRadar() && ( conf.ScrollSpeed() != SCROLL_SPEED_NONE ) ) {
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
                    _gameArea.SetScroll( scrollPosition );
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
            cursor.SetThemes( Cursor::WAIT );

            // if the hero is currently moving, pressing any mouse button should stop him
            if ( le.MouseClickLeft() || le.MousePressRight() ) {
                stopHero = true;
            }
        }
        // cursor is over the status window
        else if ( ( !isHiddenInterface || conf.ShowStatus() ) && le.MouseCursor( _statusWindow.GetRect() ) ) {
            cursor.SetThemes( Cursor::POINTER );

            _statusWindow.QueueEventProcessing();
        }
        // cursor is over the buttons area
        else if ( ( !isHiddenInterface || conf.ShowButtons() ) && le.MouseCursor( buttonsArea.GetRect() ) ) {
            cursor.SetThemes( Cursor::POINTER );

            res = buttonsArea.QueueEventProcessing();
            isCursorOverButtons = true;
        }
        // cursor is over the icons panel
        else if ( ( !isHiddenInterface || conf.ShowIcons() ) && le.MouseCursor( iconsPanel.GetRect() ) ) {
            cursor.SetThemes( Cursor::POINTER );

            iconsPanel.QueueEventProcessing();
        }
        // cursor is over the radar
        else if ( ( !isHiddenInterface || conf.ShowRadar() ) && le.MouseCursor( _radar.GetRect() ) ) {
            cursor.SetThemes( Cursor::POINTER );

            if ( !_gameArea.isDragScroll() )
                _radar.QueueEventProcessing();
        }
        // cursor is over the control panel
        else if ( isHiddenInterface && conf.ShowControlPanel() && le.MouseCursor( controlPanel.GetArea() ) ) {
            cursor.SetThemes( Cursor::POINTER );

            res = controlPanel.QueueEventProcessing();
        }
        // cursor is over the game area
        else if ( le.MouseCursor( _gameArea.GetROI() ) && !_gameArea.NeedScroll() ) {
            isCursorOverGamearea = true;
        }
        // cursor is somewhere else
        else if ( !_gameArea.NeedScroll() ) {
            cursor.SetThemes( Cursor::POINTER );

            _gameArea.ResetCursorPosition();
        }

        // gamearea
        if ( !_gameArea.NeedScroll() && !isMovingHero ) {
            if ( !_radar.isDragRadar() )
                _gameArea.QueueEventProcessing( isCursorOverGamearea );
            else if ( !le.MousePressLeft() )
                _radar.QueueEventProcessing();
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

                    _gameArea.ShiftCenter( { heroAnimationOffset.x * heroMovementSkipValue, heroAnimationOffset.y * heroMovementSkipValue } );
                    _gameArea.SetRedraw();

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
                            Interface::AdventureMap::RedrawLocker redrawLocker( Interface::AdventureMap::Get() );

                            _gameArea.SetCenter( hero->GetCenter() );

                            if ( stopHero ) {
                                stopHero = false;

                                hero->SetMove( false );
                            }
                        }
                        else {
                            const fheroes2::Point movement( hero->MovementDirection() );
                            if ( movement != fheroes2::Point() ) { // don't waste resources for no movement
                                // Do not generate a frame as we are going to do it later.
                                Interface::AdventureMap::RedrawLocker redrawLocker( Interface::AdventureMap::Get() );

                                const int32_t heroMovementSkipValue = Game::HumanHeroAnimSkip();

                                heroAnimationOffset = movement;
                                _gameArea.ShiftCenter( movement );

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

                            _gameArea.SetRedraw();
                        }

                        // Update the hero's move status.
                        isMovingHero = hero->isMoveEnabled();

                        if ( hero->isAction() ) {
                            // The action can not be performed while moving, only after the move is ended.
                            assert( !isMovingHero );

                            // Check if the game is over after the hero's action.
                            res = gameResult.checkGameOver();

                            hero->ResetAction();
                        }

                        if ( !isMovingHero ) {
                            // Reset the 'ENABLEMOVE' state on this loop to properly update the cursor in this frame and not in the next.
                            hero->SetMove( false );

                            // During the action and/or movement the adventure map and/or cursor position may have changed, so we should update the cursor image.
                            if ( Game::isFadeInNeeded() ) {
                                // Do not change cursor right now because fade-in is scheduled.
                                _gameArea.SetUpdateCursor();
                            }
                            else {
                                if ( le.MouseCursor( _gameArea.GetROI() ) ) {
                                    // We do not use '_gameArea.SetUpdateCursor()' here because we need to update the cursor before rendering the current frame
                                    // and '_gameArea.QueueEventProcessing()' was called earlier in this loop and will only be able to update the cursor in the
                                    // next loop for the next frame.
                                    cursor.SetThemes( GetCursorTileIndex( _gameArea.GetValidTileIdFromPoint( le.GetMouseCursor() ) ) );
                                }
                                else {
                                    // When the cursor is not over the game area we use the Pointer cursor.
                                    cursor.SetThemes( Cursor::POINTER );
                                }
                            }
                        }
                    }
                    else {
                        isMovingHero = false;
                        stopHero = false;

                        hero->SetMove( false );

                        _gameArea.SetUpdateCursor();
                    }
                }
            }
            else {
                isMovingHero = false;
                stopHero = false;
            }
        }

        // fast scroll
        if ( ( _gameArea.NeedScroll() && !isMovingHero ) || _gameArea.needDragScrollRedraw() ) {
            if ( Game::validateAnimationDelay( Game::SCROLL_DELAY ) ) {
                if ( ( isScrollLeft( le.GetMouseCursor() ) || isScrollRight( le.GetMouseCursor() ) || isScrollTop( le.GetMouseCursor() )
                       || isScrollBottom( le.GetMouseCursor() ) )
                     && !_gameArea.isDragScroll() ) {
                    cursor.SetThemes( _gameArea.GetScrollCursor() );
                }

                _gameArea.Scroll();

                _gameArea.SetRedraw();
                _radar.SetRedraw( REDRAW_RADAR_CURSOR );
            }
        }

        // Check that the kingdom is not vanquished yet (has at least one hero or castle).
        if ( res == fheroes2::GameMode::CANCEL && !myKingdom.isPlay() ) {
            res = fheroes2::GameMode::END_TURN;
        }

        // Render map only if the turn is not over.
        if ( res == fheroes2::GameMode::CANCEL ) {
            // map objects animation
            if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
                Game::updateAdventureMapAnimationIndex();
                _gameArea.SetRedraw();
            }

            if ( needRedraw() ) {
                redraw( 0 );

                // If this assertion blows up it means that we are holding a RedrawLocker lock for rendering which should not happen.
                assert( getRedrawMask() == 0 );

                validateFadeInAndRender();
            }
        }
    }

    if ( res == fheroes2::GameMode::END_TURN ) {
        if ( GetFocusHeroes() ) {
            GetFocusHeroes()->ShowPath( false );

            setRedraw( REDRAW_GAMEAREA );
        }

        if ( myKingdom.isPlay() ) {
            // these warnings should be shown at the end of the turn
            if ( myCastles.empty() ) {
                const uint32_t lostTownDays = myKingdom.GetLostTownDays();

                if ( lostTownDays > Game::GetLostTownDays() ) {
                    Game::DialogPlayers( conf.CurrentColor(), _( "Beware!" ),
                                         _( "%{color} player, you have lost your last town. If you do not conquer another town in next week, you will be eliminated." ) );
                }
                else if ( lostTownDays == 1 ) {
                    Game::DialogPlayers( conf.CurrentColor(), _( "Defeat!" ), _( "%{color} player, your heroes abandon you, and you are banished from this land." ) );
                }
            }

            if ( !conf.isAutoSaveAtBeginningOfTurnEnabled() ) {
                Game::AutoSave();
            }
        }
    }

    return res;
}

void Interface::AdventureMap::mouseCursorAreaClickLeft( const int32_t tileIndex )
{
    Heroes * focusedHero = GetFocusHeroes();
    assert( focusedHero == nullptr || !focusedHero->Modes( Heroes::ENABLEMOVE ) );

    const Maps::Tiles & tile = world.GetTiles( tileIndex );

    switch ( Cursor::WithoutDistanceThemes( Cursor::Get().Themes() ) ) {
    case Cursor::HEROES: {
        Heroes * otherHero = tile.getHero();
        if ( otherHero == nullptr ) {
            break;
        }

        if ( focusedHero == nullptr || focusedHero != otherHero ) {
            SetFocus( otherHero, false );
            RedrawFocus();
        }
        else {
            Game::OpenHeroesDialog( *otherHero, true, true );
        }

        break;
    }

    case Cursor::CASTLE: {
        const MP2::MapObjectType objectType = tile.GetObject();
        if ( MP2::OBJ_NON_ACTION_CASTLE != objectType && MP2::OBJ_CASTLE != objectType ) {
            break;
        }

        Castle * otherCastle = world.getCastle( tile.GetCenter() );
        if ( otherCastle == nullptr ) {
            break;
        }

        const Castle * focusedCastle = GetFocusCastle();

        if ( focusedCastle == nullptr || focusedCastle != otherCastle ) {
            SetFocus( otherCastle );
            RedrawFocus();
        }
        else {
            Game::OpenCastleDialog( *otherCastle );
        }

        break;
    }
    case Cursor::CURSOR_HERO_FIGHT:
    case Cursor::CURSOR_HERO_MOVE:
    case Cursor::CURSOR_HERO_BOAT:
    case Cursor::CURSOR_HERO_ANCHOR:
    case Cursor::CURSOR_HERO_MEET:
    case Cursor::CURSOR_HERO_ACTION:
    case Cursor::CURSOR_HERO_BOAT_ACTION: {
        if ( focusedHero == nullptr ) {
            break;
        }

        ShowPathOrStartMoveHero( focusedHero, tileIndex );

        break;
    }

    default:
        break;
    }
}

void Interface::AdventureMap::mouseCursorAreaPressRight( const int32_t tileIndex ) const
{
#ifndef NDEBUG
    const Heroes * focusedHero = GetFocusHeroes();
#endif
    assert( focusedHero == nullptr || !focusedHero->Modes( Heroes::ENABLEMOVE ) );

    const Settings & conf = Settings::Get();
    const Maps::Tiles & tile = world.GetTiles( tileIndex );

    DEBUG_LOG( DBG_DEVEL, DBG_INFO, std::endl << tile.String() )

    if ( !IS_DEVEL() && tile.isFog( conf.CurrentColor() ) ) {
        Dialog::QuickInfo( tile );
    }
    else {
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
            const Heroes * heroes = tile.getHero();

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
