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
#include <vector>

#ifdef AI
#undef AI
#endif

#include "agg.h"
#include "ai.h"
#include "battle_only.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "engine.h"
#include "game.h"
#include "game_interface.h"
#include "game_io.h"
#include "game_over.h"
#include "ground.h"
#include "heroes.h"
#include "kingdom.h"
#include "m82.h"
#include "maps_tiles.h"
#include "mus.h"
#include "route.h"
#include "settings.h"
#include "system.h"
#include "world.h"

namespace
{
    bool SortPlayers( const Player * player1, const Player * player2 )
    {
        return ( player1->isControlHuman() && !player2->isControlHuman() )
               || ( ( player1->isControlHuman() == player2->isControlHuman() ) && ( player1->GetColor() < player2->GetColor() ) );
    }
}

int Game::StartBattleOnly( void )
{
    Battle::Only main;

    if ( main.ChangeSettings() )
        main.StartBattle();

    return Game::MAINMENU;
}

int Game::StartGame( void )
{
    AI::Get().Reset();

    // cursor
    Cursor & cursor = Cursor::Get();
    Settings & conf = Settings::Get();

    if ( !conf.LoadedGameVersion() )
        GameOver::Result::Get().Reset();

    cursor.Hide();
    AGG::ResetMixer();

    Interface::Basic::Get().Reset();

    return Interface::Basic::Get().StartGame();
}

void Game::DialogPlayers( int color, std::string str )
{
    const Player * player = Settings::Get().GetPlayers().Get( color );
    StringReplace( str, "%{color}", ( player ? player->GetName() : Color::String( color ) ) );

    const fheroes2::Sprite & border = fheroes2::AGG::GetICN( ICN::BRCREST, 6 );
    fheroes2::Image sign = border;

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
        break;
    }

    Dialog::SpriteInfo( "", str, sign );
}

/* open castle wrapper */
void Game::OpenCastleDialog( Castle & castle )
{
    Mixer::Pause();

    const Settings & conf = Settings::Get();
    Kingdom & myKingdom = world.GetKingdom( conf.CurrentColor() );
    const KingdomCastles & myCastles = myKingdom.GetCastles();
    KingdomCastles::const_iterator it = std::find( myCastles.begin(), myCastles.end(), &castle );
    Interface::StatusWindow::ResetTimer();

    const size_t heroCountBefore = myKingdom.GetHeroes().size();

    if ( it != myCastles.end() ) {
        int result = Dialog::ZERO;
        while ( Dialog::CANCEL != result ) {
            result = ( *it )->OpenDialog( false );

            if ( it != myCastles.end() ) {
                if ( Dialog::PREV == result ) {
                    if ( it == myCastles.begin() )
                        it = myCastles.end();
                    --it;
                }
                else if ( Dialog::NEXT == result ) {
                    ++it;
                    if ( it == myCastles.end() )
                        it = myCastles.begin();
                }
            }
        }
    }
    else if ( castle.isFriends( conf.CurrentColor() ) ) {
        ( *it )->OpenDialog( true );
    }

    Interface::Basic & basicInterface = Interface::Basic::Get();
    if ( heroCountBefore < myKingdom.GetHeroes().size() ) {
        basicInterface.SetFocus( myKingdom.GetHeroes()[heroCountBefore] );
    }
    else {
        basicInterface.SetFocus( *it );
    }
    basicInterface.RedrawFocus();
}

/* open heroes wrapper */
void Game::OpenHeroesDialog( Heroes & hero, bool updateFocus )
{
    const Settings & conf = Settings::Get();
    Kingdom & myKingdom = hero.GetKingdom();
    const KingdomHeroes & myHeroes = myKingdom.GetHeroes();
    KingdomHeroes::const_iterator it = std::find( myHeroes.begin(), myHeroes.end(), &hero );
    Interface::StatusWindow::ResetTimer();
    Interface::Basic & I = Interface::Basic::Get();
    Interface::GameArea & gameArea = I.GetGameArea();
    bool needFade = conf.ExtGameUseFade() && fheroes2::Display::instance().isDefaultSize();

    if ( it != myHeroes.end() ) {
        int result = Dialog::ZERO;

        while ( Dialog::CANCEL != result ) {
            result = ( *it )->OpenDialog( false, needFade );
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
                AGG::PlaySound( M82::KILLFADE );

                ( *it )->GetPath().Hide();
                gameArea.SetRedraw();

                ( *it )->FadeOut();
                ( *it )->SetFreeman( 0 );
                it = myHeroes.begin();
                updateFocus = true;
                result = Dialog::CANCEL;
                break;

            default:
                break;
            }
        }
    }

    if ( updateFocus ) {
        if ( it != myHeroes.end() ) {
            Interface::Basic::Get().SetFocus( *it );
        }
        else {
            Interface::Basic::Get().ResetFocus( GameFocus::HEROES );
        }
    }

    Interface::Basic::Get().RedrawFocus();
}

void ShowNewWeekDialog( void )
{
    const Week & week = world.GetWeekType();

    // head
    std::string message = world.BeginMonth() ? _( "Astrologers proclaim Month of the %{name}." ) : _( "Astrologers proclaim Week of the %{name}." );
    AGG::PlayMusic( world.BeginMonth() ? MUS::NEW_MONTH : MUS::NEW_WEEK, false );
    StringReplace( message, "%{name}", week.GetName() );
    message += "\n \n";

    if ( week.GetType() == Week::MONSTERS ) {
        const Monster monster( week.GetMonster() );
        const u32 count = world.BeginMonth() ? Castle::GetGrownMonthOf() : Castle::GetGrownWeekOf( monster );

        if ( monster.isValid() && count ) {
            if ( world.BeginMonth() )
                message += 100 == Castle::GetGrownMonthOf() ? _( "After regular growth, population of %{monster} is doubled!" )
                                                            : _n( "After regular growth, population of %{monter} increase on %{count} percent!",
                                                                  "After regular growth, population of %{monter} increase on %{count} percent!", count );
            else
                message += _n( "%{monster} population increases by +%{count}.", "%{monster} population increases by +%{count}.", count );
            StringReplace( message, "%{monster}", monster.GetMultiName() );
            StringReplace( message, "%{count}", count );
            message += "\n";
        }
    }

    if ( week.GetType() == Week::PLAGUE )
        message += _( " All populations are halved." );
    else
        message += _( " All dwellings increase population." );

    Dialog::Message( "", message, Font::BIG, Dialog::OK );
}

void ShowEventDayDialog( void )
{
    Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    EventsDate events = world.GetEventsDate( myKingdom.GetColor() );

    for ( EventsDate::const_iterator it = events.begin(); it != events.end(); ++it ) {
        if ( ( *it ).resource.GetValidItemsCount() )
            Dialog::ResourceInfo( "", ( *it ).message, ( *it ).resource );
        else if ( ( *it ).message.size() )
            Dialog::Message( "", ( *it ).message, Font::BIG, Dialog::OK );
    }
}

int ShowWarningLostTownsDialog( void )
{
    const Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );

    if ( 0 == myKingdom.GetLostTownDays() ) {
        Game::DialogPlayers( myKingdom.GetColor(), _( "%{color} player, your heroes abandon you, and you are banished from this land." ) );
        GameOver::Result::Get().SetResult( GameOver::LOSS_ALL );
        return Game::MAINMENU;
    }
    else if ( 1 == myKingdom.GetLostTownDays() ) {
        Game::DialogPlayers( myKingdom.GetColor(), _( "%{color} player, this is your last day to capture a town, or you will be banished from this land." ) );
    }
    else if ( Game::GetLostTownDays() >= myKingdom.GetLostTownDays() ) {
        std::string str = _( "%{color} player, you only have %{day} days left to capture a town, or you will be banished from this land." );
        StringReplace( str, "%{day}", myKingdom.GetLostTownDays() );
        Game::DialogPlayers( myKingdom.GetColor(), str );
    }

    return Game::CANCEL;
}

/* return changee cursor */
int Interface::Basic::GetCursorFocusCastle( const Castle & from_castle, const Maps::Tiles & tile )
{
    switch ( tile.GetObject() ) {
    case MP2::OBJN_CASTLE:
    case MP2::OBJ_CASTLE: {
        const Castle * to_castle = world.GetCastle( tile.GetCenter() );

        if ( NULL != to_castle )
            return to_castle->GetColor() == from_castle.GetColor() ? Cursor::CASTLE : Cursor::POINTER;
    } break;

    case MP2::OBJ_HEROES: {
        const Heroes * heroes = tile.GetHeroes();

        if ( NULL != heroes )
            return heroes->GetColor() == from_castle.GetColor() ? Cursor::HEROES : Cursor::POINTER;
    } break;

    default:
        break;
    }

    return Cursor::POINTER;
}

int Interface::Basic::GetCursorFocusShipmaster( const Heroes & from_hero, const Maps::Tiles & tile )
{
    const Settings & conf = Settings::Get();
    const bool water = tile.isWater();

    switch ( tile.GetObject() ) {
    case MP2::OBJ_MONSTER:
        return water ? Cursor::DistanceThemes( Cursor::FIGHT, from_hero.GetRangeRouteDays( tile.GetIndex() ) ) : Cursor::POINTER;

    case MP2::OBJ_BOAT:
        return Cursor::POINTER;

    case MP2::OBJN_CASTLE:
    case MP2::OBJ_CASTLE: {
        const Castle * castle = world.GetCastle( tile.GetCenter() );

        if ( castle )
            return from_hero.GetColor() == castle->GetColor() ? Cursor::CASTLE : Cursor::POINTER;
    } break;

    case MP2::OBJ_HEROES: {
        const Heroes * to_hero = tile.GetHeroes();

        if ( to_hero ) {
            if ( !to_hero->isShipMaster() )
                return from_hero.GetColor() == to_hero->GetColor() ? Cursor::HEROES : Cursor::POINTER;
            else if ( to_hero->GetCenter() == from_hero.GetCenter() )
                return Cursor::HEROES;
            else if ( from_hero.GetColor() == to_hero->GetColor() )
                return Cursor::DistanceThemes( Cursor::CHANGE, from_hero.GetRangeRouteDays( tile.GetIndex() ) );
            else if ( from_hero.isFriends( to_hero->GetColor() ) )
                return conf.ExtUnionsAllowHeroesMeetings() ? Cursor::CHANGE : Cursor::POINTER;
            else if ( to_hero->AllowBattle( false ) )
                return Cursor::DistanceThemes( Cursor::FIGHT, from_hero.GetRangeRouteDays( tile.GetIndex() ) );
        }
    } break;

    case MP2::OBJ_COAST:
        return Cursor::DistanceThemes( Cursor::ANCHOR, from_hero.GetRangeRouteDays( tile.GetIndex() ) );

    default:
        if ( water ) {
            if ( MP2::isWaterObject( tile.GetObject() ) )
                return Cursor::DistanceThemes( Cursor::REDBOAT, from_hero.GetRangeRouteDays( tile.GetIndex() ) );
            else if ( tile.isPassable( Direction::CENTER, true, false ) )
                return Cursor::DistanceThemes( Cursor::BOAT, from_hero.GetRangeRouteDays( tile.GetIndex() ) );
        }
        break;
    }

    return Cursor::POINTER;
}

int Interface::Basic::GetCursorFocusHeroes( const Heroes & from_hero, const Maps::Tiles & tile )
{
    const Settings & conf = Settings::Get();

    if ( from_hero.Modes( Heroes::ENABLEMOVE ) )
        return Cursor::Get().Themes();
    else if ( from_hero.isShipMaster() )
        return GetCursorFocusShipmaster( from_hero, tile );

    switch ( tile.GetObject() ) {
    case MP2::OBJ_MONSTER:
        if ( from_hero.Modes( Heroes::GUARDIAN ) )
            return Cursor::POINTER;
        else
            return Cursor::DistanceThemes( Cursor::FIGHT, from_hero.GetRangeRouteDays( tile.GetIndex() ) );

    case MP2::OBJN_CASTLE:
    case MP2::OBJ_CASTLE: {
        const Castle * castle = world.GetCastle( tile.GetCenter() );

        if ( NULL != castle ) {
            if ( tile.GetObject() == MP2::OBJN_CASTLE ) {
                if ( from_hero.GetColor() == castle->GetColor() )
                    return Cursor::CASTLE;
                else
                    return Cursor::POINTER;
            }
            else if ( from_hero.Modes( Heroes::GUARDIAN ) || from_hero.GetIndex() == castle->GetIndex() )
                return from_hero.GetColor() == castle->GetColor() ? Cursor::CASTLE : Cursor::POINTER;
            else if ( from_hero.GetColor() == castle->GetColor() )
                return Cursor::DistanceThemes( Cursor::ACTION, from_hero.GetRangeRouteDays( castle->GetIndex() ) );
            else if ( from_hero.isFriends( castle->GetColor() ) )
                return conf.ExtUnionsAllowCastleVisiting() ? Cursor::ACTION : Cursor::POINTER;
            else if ( castle->GetActualArmy().isValid() )
                return Cursor::DistanceThemes( Cursor::FIGHT, from_hero.GetRangeRouteDays( castle->GetIndex() ) );
            else
                return Cursor::DistanceThemes( Cursor::ACTION, from_hero.GetRangeRouteDays( castle->GetIndex() ) );
        }
    } break;

    case MP2::OBJ_HEROES: {
        const Heroes * to_hero = tile.GetHeroes();

        if ( NULL != to_hero ) {
            if ( from_hero.Modes( Heroes::GUARDIAN ) )
                return from_hero.GetColor() == to_hero->GetColor() ? Cursor::HEROES : Cursor::POINTER;
            else if ( to_hero->GetCenter() == from_hero.GetCenter() )
                return Cursor::HEROES;
            else if ( from_hero.GetColor() == to_hero->GetColor() ) {
                int newcur = Cursor::DistanceThemes( Cursor::CHANGE, from_hero.GetRangeRouteDays( tile.GetIndex() ) );
                return newcur != Cursor::POINTER ? newcur : Cursor::HEROES;
            }
            else if ( from_hero.isFriends( to_hero->GetColor() ) ) {
                int newcur = Cursor::DistanceThemes( Cursor::CHANGE, from_hero.GetRangeRouteDays( tile.GetIndex() ) );
                return conf.ExtUnionsAllowHeroesMeetings() ? newcur : Cursor::POINTER;
            }
            else
                return Cursor::DistanceThemes( Cursor::FIGHT, from_hero.GetRangeRouteDays( tile.GetIndex() ) );
        }
    } break;

    case MP2::OBJ_BOAT:
        return from_hero.Modes( Heroes::GUARDIAN ) ? Cursor::POINTER : Cursor::DistanceThemes( Cursor::BOAT, from_hero.GetRangeRouteDays( tile.GetIndex() ) );

    default:
        if ( from_hero.Modes( Heroes::GUARDIAN ) )
            return Cursor::POINTER;
        else if ( MP2::isGroundObject( tile.GetObject() ) ) {
            bool protection = false;
            if ( !MP2::isPickupObject( tile.GetObject() ) && !MP2::isAbandonedMine( tile.GetObject() ) ) {
                protection = ( Maps::TileIsUnderProtection( tile.GetIndex() ) || ( !from_hero.isFriends( tile.QuantityColor() ) && tile.CaptureObjectIsProtection() ) );
            }
            else {
                protection = Maps::TileIsUnderProtection( tile.GetIndex() );
            }

            return Cursor::DistanceThemes( ( protection ? Cursor::FIGHT : Cursor::ACTION ), from_hero.GetRangeRouteDays( tile.GetIndex() ) );
        }
        else if ( tile.isPassable( Direction::CENTER, from_hero.isShipMaster(), false ) ) {
            bool protection = Maps::TileIsUnderProtection( tile.GetIndex() );

            return Cursor::DistanceThemes( ( protection ? Cursor::FIGHT : Cursor::MOVE ), from_hero.GetRangeRouteDays( tile.GetIndex() ) );
        }
        break;
    }

    return Cursor::POINTER;
}

int Interface::Basic::GetCursorTileIndex( s32 dst_index )
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

int Interface::Basic::StartGame( void )
{
    Cursor & cursor = Cursor::Get();
    Settings & conf = Settings::Get();
    fheroes2::Display & display = fheroes2::Display::instance();

    // draw interface
    gameArea.Build();
    iconsPanel.ResetIcons();
    radar.Build();

    if ( conf.ExtGameHideInterface() )
        SetHideInterface( true );

    Redraw( REDRAW_ICONS | REDRAW_BUTTONS | REDRAW_BORDER );
    iconsPanel.HideIcons();

    bool skip_turns = conf.LoadedGameVersion();
    GameOver::Result & gameResult = GameOver::Result::Get();
    int res = Game::ENDTURN;

    std::vector<Player *> sortedPlayers = conf.GetPlayers();
    std::sort( sortedPlayers.begin(), sortedPlayers.end(), SortPlayers );

    while ( res == Game::ENDTURN ) {
        if ( !skip_turns )
            world.NewDay();

        for ( Players::const_iterator it = sortedPlayers.begin(); it != sortedPlayers.end(); ++it )
            if ( *it ) {
                const Player & player = ( **it );
                Kingdom & kingdom = world.GetKingdom( player.GetColor() );

                if ( !kingdom.isPlay() || ( skip_turns && !player.isColor( conf.CurrentColor() ) ) )
                    continue;

                DEBUG( DBG_GAME, DBG_INFO,
                       std::endl
                           << world.DateString() << ", "
                           << "color: " << Color::String( player.GetColor() ) << ", resource: " << kingdom.GetFunds().String() );

                radar.SetHide( true );
                radar.SetRedraw();
                if ( player.GetControl() == CONTROL_HUMAN ) {
                    conf.SetCurrentColor( -1 ); // we need to hide world map in hot seat mode
                }
                else {
                    conf.SetCurrentColor( player.GetColor() );
                    world.ClearFog( player.GetColor() );
                    kingdom.ActionBeforeTurn();
                }

                switch ( kingdom.GetControl() ) {
                case CONTROL_HUMAN:
                    if ( conf.IsGameType( Game::TYPE_HOTSEAT ) ) {
                        cursor.Hide();
                        iconsPanel.HideIcons();
                        statusWindow.Reset();
                        SetRedraw( REDRAW_GAMEAREA | REDRAW_STATUS | REDRAW_ICONS );
                        Redraw();
                        display.render();
                        Game::DialogPlayers( player.GetColor(), _( "%{color} player's turn." ) );
                    }
                    conf.SetCurrentColor( player.GetColor() );
                    world.ClearFog( player.GetColor() );
                    kingdom.ActionBeforeTurn();
                    iconsPanel.SetRedraw();
                    iconsPanel.ShowIcons();
                    res = HumanTurn( skip_turns );
                    if ( skip_turns )
                        skip_turns = false;
                    break;

                // CONTROL_AI turn
                default:
                    if ( res == Game::ENDTURN ) {
                        statusWindow.Reset();
                        statusWindow.SetState( STATUS_AITURN );

                        cursor.Hide();
                        cursor.SetThemes( Cursor::WAIT );
                        Redraw();
                        cursor.Show();
                        display.render();

                        AI::Get().KingdomTurn( kingdom );
                    }
                    break;
                }

                if ( res != Game::ENDTURN )
                    break;

                res = gameResult.LocalCheckGameOver();

                if ( Game::CANCEL != res )
                    break;
                else
                    res = Game::ENDTURN;
            }

        DELAY( 10 );
    }

    if ( res == Game::ENDTURN )
        display.fill( 0 );
    else if ( conf.ExtGameUseFade() )
        fheroes2::FadeDisplay();

    return res == Game::ENDTURN ? Game::QUITGAME : res;
}

int Interface::Basic::HumanTurn( bool isload )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    Settings & conf = Settings::Get();
    int res = Game::CANCEL;

    LocalEvent & le = LocalEvent::Get();
    cursor.Hide();

    Kingdom & myKingdom = world.GetKingdom( conf.CurrentColor() );
    const KingdomCastles & myCastles = myKingdom.GetCastles();
    const KingdomHeroes & myHeroes = myKingdom.GetHeroes();

    GameOver::Result & gameResult = GameOver::Result::Get();

    // set focus
    if ( conf.LoadedGameVersion() && conf.ExtGameRememberLastFocus() ) {
        if ( GetFocusHeroes() )
            ResetFocus( GameFocus::HEROES );
        else
            ResetFocus( GameFocus::CASTLE );
    }
    else
        ResetFocus( GameFocus::FIRSTHERO );

    radar.SetHide( false );
    statusWindow.Reset();
    gameArea.SetUpdateCursor();
    Redraw( REDRAW_GAMEAREA | REDRAW_RADAR | REDRAW_ICONS | REDRAW_BUTTONS | REDRAW_STATUS | REDRAW_BORDER );

    Game::EnvironmentSoundMixer();

    cursor.Show();
    display.render();

    if ( !isload ) {
        // new week dialog
        if ( 1 < world.CountWeek() && world.BeginWeek() )
            ShowNewWeekDialog();

        // show event day
        ShowEventDayDialog();

        // autosave
        if ( conf.ExtGameAutosaveOn() && conf.ExtGameAutosaveBeginOfDay() )
            Game::AutoSave();
    }

    // check game over
    res = gameResult.LocalCheckGameOver();

    // warning lost all town
    if ( myCastles.empty() )
        res = ShowWarningLostTownsDialog();

    // check around actions (and skip for h2 orig, bug?)
    if ( !conf.ExtWorldOnlyFirstMonsterAttack() )
        myKingdom.HeroesActionNewPosition();

    int fastScrollRepeatCount = 0;
    const int fastScrollThreshold = 2;
    bool isOngoingFastScrollEvent = false;

    bool isMovingHero = false;
    bool stopHero = false;

    int heroAnimationFrameCount = 0;
    Point heroAnimationOffset;
    int heroAnimationSpriteId = 0;

    bool isCursorOverButtons = false;

    // startgame loop
    while ( Game::CANCEL == res ) {
        if ( !le.HandleEvents( true, true ) ) {
            if ( EventExit() == Game::QUITGAME ) {
                res = Game::QUITGAME;
                break;
            }
        }

        if ( !isOngoingFastScrollEvent )
            fastScrollRepeatCount = 0;

        isOngoingFastScrollEvent = false;

        // hot keys
        if ( le.KeyPress() ) {
            if ( isMovingHero )
                stopHero = true;
            // exit dialog
            else if ( HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) )
                res = EventExit();
            // end turn
            else if ( HotKeyPressEvent( Game::EVENT_ENDTURN ) )
                res = EventEndTurn();
            // next hero
            else if ( HotKeyPressEvent( Game::EVENT_NEXTHERO ) )
                EventNextHero();
            // next town
            else if ( HotKeyPressEvent( Game::EVENT_NEXTTOWN ) )
                EventNextTown();
            // new game
            else if ( HotKeyPressEvent( Game::EVENT_BUTTON_NEWGAME ) )
                res = EventNewGame();
            // save game
            else if ( HotKeyPressEvent( Game::EVENT_SAVEGAME ) )
                EventSaveGame();
            // load game
            else if ( HotKeyPressEvent( Game::EVENT_LOADGAME ) ) {
                res = EventLoadGame();
                if ( Game::LOADGAME == res )
                    break;
            }
            // file options
            else if ( HotKeyPressEvent( Game::EVENT_FILEOPTIONS ) )
                res = EventFileDialog();
            // system options
            else if ( HotKeyPressEvent( Game::EVENT_SYSTEMOPTIONS ) )
                EventSystemDialog();
            // puzzle map
            else if ( HotKeyPressEvent( Game::EVENT_PUZZLEMAPS ) )
                EventPuzzleMaps();
            // info game
            else if ( HotKeyPressEvent( Game::EVENT_INFOGAME ) )
                EventGameInfo();
            // cast spell
            else if ( HotKeyPressEvent( Game::EVENT_CASTSPELL ) )
                EventCastSpell();
            // show/hide control panel
            else if ( HotKeyPressEvent( Game::EVENT_CTRLPANEL ) )
                EventSwitchShowControlPanel();
            // hide/show radar
            else if ( HotKeyPressEvent( Game::EVENT_SHOWRADAR ) )
                EventSwitchShowRadar();
            // hide/show buttons
            else if ( HotKeyPressEvent( Game::EVENT_SHOWBUTTONS ) )
                EventSwitchShowButtons();
            // hide/show status window
            else if ( HotKeyPressEvent( Game::EVENT_SHOWSTATUS ) )
                EventSwitchShowStatus();
            // hide/show hero/town icons
            else if ( HotKeyPressEvent( Game::EVENT_SHOWICONS ) )
                EventSwitchShowIcons();
            // hero movement
            else if ( HotKeyPressEvent( Game::EVENT_CONTINUE ) )
                EventContinueMovement();
            // dig artifact
            else if ( HotKeyPressEvent( Game::EVENT_DIGARTIFACT ) )
                res = EventDigArtifact();
            // sleep hero
            else if ( HotKeyPressEvent( Game::EVENT_SLEEPHERO ) )
                EventSwitchHeroSleeping();
            // move hero
            else if ( HotKeyPressEvent( Game::EVENT_MOVELEFT ) )
                EventKeyArrowPress( Direction::LEFT );
            else if ( HotKeyPressEvent( Game::EVENT_MOVERIGHT ) )
                EventKeyArrowPress( Direction::RIGHT );
            else if ( HotKeyPressEvent( Game::EVENT_MOVETOP ) )
                EventKeyArrowPress( Direction::TOP );
            else if ( HotKeyPressEvent( Game::EVENT_MOVEBOTTOM ) )
                EventKeyArrowPress( Direction::BOTTOM );
            else if ( HotKeyPressEvent( Game::EVENT_MOVETOPLEFT ) )
                EventKeyArrowPress( Direction::TOP_LEFT );
            else if ( HotKeyPressEvent( Game::EVENT_MOVETOPRIGHT ) )
                EventKeyArrowPress( Direction::TOP_RIGHT );
            else if ( HotKeyPressEvent( Game::EVENT_MOVEBOTTOMLEFT ) )
                EventKeyArrowPress( Direction::BOTTOM_LEFT );
            else if ( HotKeyPressEvent( Game::EVENT_MOVEBOTTOMRIGHT ) )
                EventKeyArrowPress( Direction::BOTTOM_RIGHT );
            // scroll maps
            else if ( HotKeyPressEvent( Game::EVENT_SCROLLLEFT ) )
                gameArea.SetScroll( SCROLL_LEFT );
            else if ( HotKeyPressEvent( Game::EVENT_SCROLLRIGHT ) )
                gameArea.SetScroll( SCROLL_RIGHT );
            else if ( HotKeyPressEvent( Game::EVENT_SCROLLUP ) )
                gameArea.SetScroll( SCROLL_TOP );
            else if ( HotKeyPressEvent( Game::EVENT_SCROLLDOWN ) )
                gameArea.SetScroll( SCROLL_BOTTOM );
            // default action
            else if ( HotKeyPressEvent( Game::EVENT_DEFAULTACTION ) )
                EventDefaultAction();
            // open focus
            else if ( HotKeyPressEvent( Game::EVENT_OPENFOCUS ) )
                EventOpenFocus();
        }

        if ( conf.ExtPocketTapMode() ) {
            // scroll area maps left
            if ( le.MouseCursor( GetScrollLeft() ) && le.MousePressLeft() )
                gameArea.SetScroll( SCROLL_LEFT );
            else
                // scroll area maps right
                if ( le.MouseCursor( GetScrollRight() ) && le.MousePressLeft() )
                gameArea.SetScroll( SCROLL_RIGHT );
            else
                // scroll area maps top
                if ( le.MouseCursor( GetScrollTop() ) && le.MousePressLeft() )
                gameArea.SetScroll( SCROLL_TOP );
            else
                // scroll area maps bottom
                if ( le.MouseCursor( GetScrollBottom() ) && le.MousePressLeft() )
                gameArea.SetScroll( SCROLL_BOTTOM );

            // disable right click emulation
            if ( gameArea.NeedScroll() )
                le.SetTapMode( false );
        }
        else {
            if ( fheroes2::cursor().isFocusActive() ) {
                int scrollPosition = SCROLL_NONE;
                if ( le.MouseCursor( GetScrollLeft() ) )
                    scrollPosition |= SCROLL_LEFT;
                else if ( le.MouseCursor( GetScrollRight() ) )
                    scrollPosition |= SCROLL_RIGHT;
                if ( le.MouseCursor( GetScrollTop() ) )
                    scrollPosition |= SCROLL_TOP;
                else if ( le.MouseCursor( GetScrollBottom() ) )
                    scrollPosition |= SCROLL_BOTTOM;
                if ( scrollPosition != SCROLL_NONE )
                    gameArea.SetScroll( scrollPosition );
            }
        }

        const fheroes2::Rect displayArea( 0, 0, display.width(), display.height() );
        const bool isHiddenInterface = conf.ExtGameHideInterface();
        const bool prevIsCursorOverButtons = isCursorOverButtons;
        isCursorOverButtons = false;
        // Stop moving hero first
        if ( isMovingHero && ( le.MouseClickLeft( displayArea ) || le.MousePressRight( displayArea ) ) ) {
            stopHero = true;
        }
        // cursor over radar
        else if ( ( !isHiddenInterface || conf.ShowRadar() ) && le.MouseCursor( radar.GetRect() ) ) {
            if ( Cursor::POINTER != cursor.Themes() )
                cursor.SetThemes( Cursor::POINTER );
            radar.QueueEventProcessing();
        }
        // cursor over icons panel
        else if ( ( !isHiddenInterface || conf.ShowIcons() ) && le.MouseCursor( iconsPanel.GetRect() ) ) {
            if ( Cursor::POINTER != cursor.Themes() )
                cursor.SetThemes( Cursor::POINTER );
            iconsPanel.QueueEventProcessing();
        }
        // cursor over buttons area
        else if ( ( !isHiddenInterface || conf.ShowButtons() ) && le.MouseCursor( buttonsArea.GetRect() ) ) {
            if ( Cursor::POINTER != cursor.Themes() )
                cursor.SetThemes( Cursor::POINTER );
            res = buttonsArea.QueueEventProcessing();
            isCursorOverButtons = true;
        }
        // cursor over status area
        else if ( ( !isHiddenInterface || conf.ShowStatus() ) && le.MouseCursor( statusWindow.GetRect() ) ) {
            if ( Cursor::POINTER != cursor.Themes() )
                cursor.SetThemes( Cursor::POINTER );
            statusWindow.QueueEventProcessing();
        }
        // cursor over control panel
        else if ( isHiddenInterface && conf.ShowControlPanel() && le.MouseCursor( controlPanel.GetArea() ) ) {
            if ( Cursor::POINTER != cursor.Themes() )
                cursor.SetThemes( Cursor::POINTER );
            res = controlPanel.QueueEventProcessing();
        }
        // cursor over game area
        else if ( le.MouseCursor( gameArea.GetROI() ) && !gameArea.NeedScroll() ) {
            gameArea.QueueEventProcessing();
        }
        else if ( !gameArea.NeedScroll() ) { // empty interface area so we set cursor to a normal pointer
            if ( Cursor::POINTER != cursor.Themes() )
                cursor.SetThemes( Cursor::POINTER );
            gameArea.ResetCursorPosition();
        }

        if ( prevIsCursorOverButtons && !isCursorOverButtons ) {
            buttonsArea.ResetButtons();
        }

        // fast scroll
        if ( gameArea.NeedScroll() )
            isOngoingFastScrollEvent = true;

        if ( gameArea.NeedScroll() && Game::AnimateInfrequentDelay( Game::SCROLL_DELAY ) ) {
            ++fastScrollRepeatCount;
            if ( fastScrollRepeatCount < fastScrollThreshold )
                continue;

            cursor.Hide();

            if ( le.MouseCursor( GetScrollLeft() ) || le.MouseCursor( GetScrollRight() ) || le.MouseCursor( GetScrollTop() ) || le.MouseCursor( GetScrollBottom() ) )
                cursor.SetThemes( gameArea.GetScrollCursor() );

            gameArea.Scroll();

            gameArea.SetRedraw();
            radar.SetRedraw();
            Redraw();
            cursor.Show();
            display.render();

            // enable right click emulation
            if ( conf.ExtPocketTapMode() )
                le.SetTapMode( true );

            continue;
        }

        // heroes move animation
        if ( Game::AnimateInfrequentDelay( Game::CURRENT_HERO_DELAY ) ) {
            Heroes * hero = GetFocusHeroes();

            if ( hero ) {
                bool resetHeroSprite = false;
                if ( heroAnimationFrameCount > 0 ) {
                    gameArea.ShiftCenter( Point( heroAnimationOffset.x * Game::HumanHeroAnimSkip(), heroAnimationOffset.y * Game::HumanHeroAnimSkip() ) );
                    gameArea.SetRedraw();
                    heroAnimationFrameCount -= Game::HumanHeroAnimSkip();
                    if ( ( heroAnimationFrameCount & 0x3 ) == 0 ) { // % 4
                        hero->SetSpriteIndex( heroAnimationSpriteId );

                        if ( heroAnimationFrameCount == 0 )
                            resetHeroSprite = true;
                        else
                            ++heroAnimationSpriteId;
                    }
                    const int offsetStep = ( ( 4 - ( heroAnimationFrameCount & 0x3 ) ) & 0x3 ); // % 4
                    hero->SetOffset( fheroes2::Point( heroAnimationOffset.x * offsetStep, heroAnimationOffset.y * offsetStep ) );
                }

                if ( heroAnimationFrameCount == 0 ) {
                    if ( resetHeroSprite ) {
                        hero->SetSpriteIndex( heroAnimationSpriteId - 1 );
                    }
                    if ( hero->isMoveEnabled() ) {
                        if ( hero->Move( 10 == conf.HeroesMoveSpeed() ) ) {
                            if ( !isOngoingFastScrollEvent ) {
                                gameArea.SetCenter( hero->GetCenter() );
                                ResetFocus( GameFocus::HEROES );
                                RedrawFocus();
                            }

                            if ( stopHero ) {
                                hero->SetMove( false );
                                stopHero = false;
                            }

                            gameArea.SetUpdateCursor();
                        }
                        else {
                            if ( !isOngoingFastScrollEvent ) {
                                Point movement( hero->MovementDirection() );
                                if ( movement != Point() ) { // don't waste resources for no movement
                                    heroAnimationOffset = movement;
                                    gameArea.ShiftCenter( movement );
                                    ResetFocus( GameFocus::HEROES );
                                    heroAnimationFrameCount = 32 - Game::HumanHeroAnimSkip();
                                    heroAnimationSpriteId = hero->GetSpriteIndex();
                                    if ( Game::HumanHeroAnimSkip() < 4 ) {
                                        hero->SetSpriteIndex( heroAnimationSpriteId - 1 );
                                        hero->SetOffset(
                                            fheroes2::Point( heroAnimationOffset.x * Game::HumanHeroAnimSkip(), heroAnimationOffset.y * Game::HumanHeroAnimSkip() ) );
                                    }
                                    else {
                                        ++heroAnimationSpriteId;
                                    }
                                }
                            }
                            gameArea.SetRedraw();
                        }

                        isMovingHero = true;

                        if ( hero->isAction() ) {
                            // check game over
                            res = gameResult.LocalCheckGameOver();
                            hero->ResetAction();
                        }
                    }
                    else {
                        isMovingHero = false;
                        stopHero = false;
                        hero->SetMove( false );
                        if ( Cursor::WAIT == cursor.Themes() )
                            gameArea.SetUpdateCursor();
                    }
                }
            }
            else {
                isMovingHero = false;
            }
        }

        // slow maps objects animation
        if ( Game::AnimateInfrequentDelay( Game::MAPS_DELAY ) ) {
            u32 & frame = Game::MapsAnimationFrame();
            ++frame;
            gameArea.SetRedraw();
        }

        if ( Game::AnimateInfrequentDelay( Game::HEROES_PICKUP_DELAY ) ) {
            Game::ObjectFadeAnimation::Info & fadeInfo = Game::ObjectFadeAnimation::Get();
            if ( fadeInfo.object != MP2::OBJ_ZERO ) {
                if ( fadeInfo.isFadeOut && fadeInfo.alpha < 20 ) {
                    fadeInfo.object = MP2::OBJ_ZERO;
                }
                else if ( !fadeInfo.isFadeOut && fadeInfo.alpha > 235 ) {
                    Maps::Tiles & objectTile = world.GetTiles( fadeInfo.tile );
                    objectTile.SetObject( fadeInfo.object );
                    // TODO: we need to expand the logic to all objects.
                    if ( fadeInfo.object == MP2::OBJ_BOAT ) {
                        objectTile.SetObjectSpriteIndex( fadeInfo.index );
                    }
                    fadeInfo.object = MP2::OBJ_ZERO;
                }
                else {
                    fadeInfo.alpha += ( fadeInfo.isFadeOut ) ? -20 : 20;
                }
                gameArea.SetRedraw();
            }
        }

        if ( NeedRedraw() ) {
            cursor.Hide();
            Redraw();
            cursor.Show();
            display.render();
        }
        else if ( !cursor.isVisible() ) {
            cursor.Show();
            display.render();
        }
    }

    if ( Game::ENDTURN == res ) {
        // warning lost all town
        if ( myHeroes.size() && myCastles.empty() && Game::GetLostTownDays() < myKingdom.GetLostTownDays() ) {
            Game::DialogPlayers( conf.CurrentColor(),
                                 _( "%{color} player, you have lost your last town. If you do not conquer another town in next week, you will be eliminated." ) );
        }

        if ( GetFocusHeroes() ) {
            GetFocusHeroes()->ShowPath( false );
            RedrawFocus();
        }

        if ( conf.ExtGameAutosaveOn() && !conf.ExtGameAutosaveBeginOfDay() )
            Game::AutoSave();
    }

    return res;
}

void Interface::Basic::MouseCursorAreaClickLeft( s32 index_maps )
{
    Heroes * from_hero = GetFocusHeroes();
    const Maps::Tiles & tile = world.GetTiles( index_maps );

    switch ( Cursor::WithoutDistanceThemes( Cursor::Get().Themes() ) ) {
    case Cursor::HEROES: {
        Heroes * to_hero = tile.GetHeroes();
        // focus change/open hero
        if ( NULL != to_hero ) {
            if ( !from_hero || from_hero != to_hero ) {
                SetFocus( to_hero );
                RedrawFocus();
            }
            else
                Game::OpenHeroesDialog( *to_hero );
        }
    } break;

    case Cursor::CASTLE: {
        // correct index for castle
        const int tileObjId = tile.GetObject();
        if ( MP2::OBJN_CASTLE != tileObjId && MP2::OBJ_CASTLE != tileObjId )
            break;

        Castle * to_castle = world.GetCastle( tile.GetCenter() );
        if ( to_castle == NULL )
            break;

        index_maps = to_castle->GetIndex();

        Castle * from_castle = GetFocusCastle();
        if ( !from_castle || from_castle != to_castle ) {
            SetFocus( to_castle );
            RedrawFocus();
        }
        else {
            Game::OpenCastleDialog( *to_castle );
        }
    } break;
    case Cursor::FIGHT:
    case Cursor::MOVE:
    case Cursor::BOAT:
    case Cursor::ANCHOR:
    case Cursor::CHANGE:
    case Cursor::ACTION:
    case Cursor::REDBOAT:
        if ( from_hero == NULL )
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

void Interface::Basic::MouseCursorAreaPressRight( s32 index_maps )
{
    Heroes * hero = GetFocusHeroes();

    // stop hero
    if ( hero && hero->isMoveEnabled() ) {
        hero->SetMove( false );
        Cursor::Get().SetThemes( GetCursorTileIndex( index_maps ) );
    }
    else {
        Settings & conf = Settings::Get();
        Maps::Tiles & tile = world.GetTiles( index_maps );

        DEBUG( DBG_DEVEL, DBG_INFO, std::endl << tile.String() );

        if ( !IS_DEVEL() && tile.isFog( conf.CurrentColor() ) )
            Dialog::QuickInfo( tile );
        else
            switch ( tile.GetObject() ) {
            case MP2::OBJN_CASTLE:
            case MP2::OBJ_CASTLE: {
                const Castle * castle = world.GetCastle( tile.GetCenter() );
                if ( castle )
                    Dialog::QuickInfo( *castle );
            } break;

            case MP2::OBJ_HEROES: {
                const Heroes * heroes = tile.GetHeroes();
                if ( heroes )
                    Dialog::QuickInfo( *heroes );
            } break;

            default:
                Dialog::QuickInfo( tile );
                break;
            }
    }
}
