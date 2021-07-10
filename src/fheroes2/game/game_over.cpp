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

#include "game_over.h"
#include "agg.h"
#include "castle.h"
#include "dialog.h"
#include "game.h"
#include "game_interface.h"
#include "game_video.h"
#include "gamedefs.h"
#include "kingdom.h"
#include "mus.h"
#include "settings.h"
#include "text.h"
#include "tools.h"
#include "world.h"

#include <cassert>

namespace
{
    void DialogWins( int cond )
    {
        const Settings & conf = Settings::Get();
        std::string body;

        switch ( cond ) {
        case GameOver::WINS_ALL:
            break;

        case GameOver::WINS_TOWN: {
            body = _( "You captured %{name}!\nYou are victorious." );
            const Castle * town = world.GetCastle( conf.WinsMapsPositionObject() );
            if ( town )
                StringReplace( body, "%{name}", town->GetName() );
        } break;

        case GameOver::WINS_HERO: {
            body = _( "You have captured the enemy hero %{name}!\nYour quest is complete." );
            const Heroes * hero = world.GetHeroesCondWins();
            if ( hero )
                StringReplace( body, "%{name}", hero->GetName() );
            break;
        }

        case GameOver::WINS_ARTIFACT: {
            body = _( "You have found the %{name}.\nYour quest is complete." );
            if ( conf.WinsFindUltimateArtifact() )
                StringReplace( body, "%{name}", "Ultimate Artifact" );
            else {
                const Artifact art = conf.WinsFindArtifactID();
                StringReplace( body, "%{name}", art.GetName() );
            }
            break;
        }

        case GameOver::WINS_SIDE:
            body = _( "The enemy is beaten.\nYour side has triumphed!" );
            break;

        case GameOver::WINS_GOLD: {
            body = _( "You have built up over %{count} gold in your treasury.\nAll enemies bow before your wealth and power." );
            StringReplace( body, "%{count}", conf.WinsAccumulateGold() );
            break;
        }

        default:
            break;
        }

        AGG::PlayMusic( MUS::VICTORY, false );

        if ( body.size() )
            Dialog::Message( "", body, Font::BIG, Dialog::OK );
    }

    void DialogLoss( int cond, int color )
    {
        const Settings & conf = Settings::Get();
        std::string body;

        switch ( cond ) {
        case GameOver::WINS_ARTIFACT: {
            body = _( "The enemy has found the %{name}.\nYour quest is a failure." );
            const Artifact art = conf.WinsFindArtifactID();
            StringReplace( body, "%{name}", art.GetName() );
            break;
        }

        case GameOver::WINS_SIDE: {
            body = _( "%{color} has fallen!\nAll is lost." );
            StringReplace( body, "%{color}", Color::String( color ) );
            break;
        }

        case GameOver::WINS_GOLD: {
            body = _( "The enemy has built up over %{count} gold in his treasury.\nYou must bow done in defeat before his wealth and power." );
            StringReplace( body, "%{count}", conf.WinsAccumulateGold() );
            break;
        }

        case GameOver::LOSS_ALL:
            body = _( "You have been eliminated from the game!!!" );
            break;

        case GameOver::LOSS_TOWN: {
            body = _( "The enemy has captured %{name}!\nThey are triumphant." );
            const Castle * town = world.GetCastle( conf.LossMapsPositionObject() );
            if ( town )
                StringReplace( body, "%{name}", town->GetName() );
            break;
        }

        case GameOver::LOSS_STARTHERO: {
            const Heroes * hero = world.GetKingdom( color ).GetFirstHeroStartCondLoss();
            body = _( "You have lost the hero %{name}.\nYour quest is over." );
            if ( hero )
                StringReplace( body, "%{name}", hero->GetName() );
            break;
        }

        case GameOver::LOSS_HERO: {
            body = _( "You have lost the hero %{name}.\nYour quest is over." );
            const Heroes * hero = world.GetHeroesCondLoss();
            if ( hero )
                StringReplace( body, "%{name}", hero->GetName() );
            else
                StringReplace( body, "%{name}", "" );
            break;
        }

        case GameOver::LOSS_TIME:
            body = _( "You have failed to complete your quest in time.\nAll is lost." );
            break;

        default:
            break;
        }

        AGG::PlayMusic( MUS::LOSTGAME, false );

        if ( body.size() )
            Dialog::Message( "", body, Font::BIG, Dialog::OK );
    }
}

const char * GameOver::GetString( int cond )
{
    const char * cond_str[] = {"None",
                               _( "Defeat all enemy heroes and capture all enemy towns and castles." ),
                               _( "Capture a specific town." ),
                               _( "Defeat a specific hero." ),
                               _( "Find a specific artifact." ),
                               _( "Your side defeats the opposing side." ),
                               _( "Accumulate a large amount of gold." ),
                               _( "Lose all your heroes and towns." ),
                               _( "Lose a specific town." ),
                               _( "Lose a specific hero." ),
                               _( "Run out of time. (Fail to win by a certain point.)" )};

    switch ( cond ) {
    case WINS_ALL:
        return cond_str[1];
    case WINS_TOWN:
        return cond_str[2];
    case WINS_HERO:
        return cond_str[3];
    case WINS_ARTIFACT:
        return cond_str[4];
    case WINS_SIDE:
        return cond_str[5];
    case WINS_GOLD:
        return cond_str[6];

    case LOSS_ALL:
        return cond_str[7];
    case LOSS_TOWN:
        return cond_str[8];
    case LOSS_HERO:
        return cond_str[9];
    case LOSS_TIME:
        return cond_str[10];

    default:
        break;
    }

    return cond_str[0];
}

std::string GameOver::GetActualDescription( int cond )
{
    const Settings & conf = Settings::Get();
    std::string msg;

    if ( WINS_ALL == cond || WINS_SIDE == cond )
        msg = GetString( WINS_ALL );
    else if ( WINS_TOWN & cond ) {
        const Castle * town = world.GetCastle( conf.WinsMapsPositionObject() );
        if ( town ) {
            msg = town->isCastle() ? _( "Capture the castle '%{name}'" ) : _( "Capture the town '%{name}'" );
            ;
            StringReplace( msg, "%{name}", town->GetName() );
        }
    }
    else if ( WINS_HERO & cond ) {
        const Heroes * hero = world.GetHeroesCondWins();
        if ( hero ) {
            msg = _( "Defeat the hero '%{name}'" );
            StringReplace( msg, "%{name}", hero->GetName() );
        }
    }
    else if ( WINS_ARTIFACT & cond ) {
        if ( conf.WinsFindUltimateArtifact() )
            msg = _( "Find the ultimate artifact" );
        else {
            const Artifact art = conf.WinsFindArtifactID();
            msg = _( "Find the '%{name}' artifact" );
            StringReplace( msg, "%{name}", art.GetName() );
        }
    }
    else if ( WINS_GOLD & cond ) {
        msg = _( "Accumulate %{count} gold" );
        StringReplace( msg, "%{count}", conf.WinsAccumulateGold() );
    }

    if ( WINS_ALL != cond && ( WINS_ALL & cond ) )
        msg.append( _( ", or you may win by defeating all enemy heroes and capturing all enemy towns and castles." ) );

    if ( LOSS_ALL == cond )
        msg = GetString( LOSS_ALL );
    else if ( LOSS_TOWN & cond ) {
        const Castle * town = world.GetCastle( conf.LossMapsPositionObject() );
        if ( town ) {
            msg = town->isCastle() ? _( "Lose the castle '%{name}'." ) : _( "Lose the town '%{name}'." );
            StringReplace( msg, "%{name}", town->GetName() );
        }
    }
    else if ( LOSS_HERO & cond ) {
        const Heroes * hero = world.GetHeroesCondLoss();
        if ( hero ) {
            msg = _( "Lose the hero: %{name}." );
            StringReplace( msg, "%{name}", hero->GetName() );
        }
    }
    else if ( LOSS_TIME & cond ) {
        msg = _( "Fail to win by the end of month %{month}, week %{week}, day %{day}." );
        const uint32_t dayCount = conf.LossCountDays() - 1;
        const uint32_t month = dayCount / ( DAYOFWEEK * WEEKOFMONTH );
        const uint32_t week = ( dayCount - month * ( DAYOFWEEK * WEEKOFMONTH ) ) / DAYOFWEEK;
        const uint32_t day = dayCount % DAYOFWEEK;
        StringReplace( msg, "%{day}", day + 1 );
        StringReplace( msg, "%{week}", week + 1 );
        StringReplace( msg, "%{month}", month + 1 );
    }

    if ( conf.ExtWorldStartHeroLossCond4Humans() ) {
        const std::string names = world.GetKingdom( conf.CurrentColor() ).GetNamesHeroStartCondLoss();

        if ( !names.empty() ) {
            std::string str = std::string::npos == names.find( ',' ) ? _( "Lose the hero: %{name}." ) : _( "Lose the heroes: %{name}." );
            StringReplace( str, "%{name}", names );
            msg.append( "\n" );
            msg.append( str );
        }
    }

    return msg;
}

GameOver::Result & GameOver::Result::Get( void )
{
    static Result gresult;
    return gresult;
}

GameOver::Result::Result()
    : colors( 0 )
    , result( 0 )
    , continueAfterVictory( false )
{}

void GameOver::Result::Reset( void )
{
    colors = Game::GetKingdomColors();
    result = GameOver::COND_NONE;
    continueAfterVictory = false;
}

void GameOver::Result::ResetResult()
{
    result = GameOver::COND_NONE;
}

int GameOver::Result::GetResult( void ) const
{
    return result;
}

fheroes2::GameMode GameOver::Result::LocalCheckGameOver()
{
    fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

    const bool isSinglePlayer = ( Colors( Players::HumanColors() ).size() == 1 );
    const int humanColors = Players::HumanColors();

    int activeHumanColors = 0;
    int activeColors = 0;

    for ( const int color : Colors( colors ) ) {
        if ( !world.GetKingdom( color ).isPlay() ) {
            if ( !isSinglePlayer || ( color & humanColors ) == 0 ) {
                Game::DialogPlayers( color, _( "%{color} player has been vanquished!" ) );
            }
            colors &= ( ~color );
        }
        else {
            ++activeColors;
            if ( color & humanColors ) {
                ++activeHumanColors;
            }
        }
    }

    if ( isSinglePlayer ) {
        assert( activeHumanColors <= 1 );

        const Kingdom & myKingdom = world.GetKingdom( humanColors );

        if ( myKingdom.isControlHuman() ) {
            if ( !continueAfterVictory && GameOver::COND_NONE != ( result = world.CheckKingdomWins( myKingdom ) ) ) {
                DialogWins( result );

                const Settings & conf = Settings::Get();

                if ( conf.isCampaignGameType() ) {
                    res = fheroes2::GameMode::COMPLETE_CAMPAIGN_SCENARIO;
                }
                else {
                    AGG::ResetMixer();
                    Video::ShowVideo( "WIN.SMK", Video::VideoAction::WAIT_FOR_USER_INPUT );
                    res = fheroes2::GameMode::HIGHSCORES;

                    if ( conf.ExtGameContinueAfterVictory() && myKingdom.isPlay() ) {
                        if ( Dialog::YES == Dialog::Message( "", "Do you wish to continue the game?", Font::BIG, Dialog::YES | Dialog::NO ) ) {
                            continueAfterVictory = true;

                            // Game::HighScores() calls ResetResult()
                            Game::HighScores();

                            Interface::Basic & I = Interface::Basic::Get();

                            I.ResetFocus( GameFocus::HEROES );
                            I.SetRedraw( Interface::REDRAW_ALL );

                            res = fheroes2::GameMode::CANCEL;
                        }
                    }
                }
            }
            else {
                if ( !continueAfterVictory ) {
                    // If the player's kingdom has been vanquished, he loses regardless of other conditions
                    if ( !myKingdom.isPlay() ) {
                        result = GameOver::LOSS_ALL;
                    }
                    else {
                        result = world.CheckKingdomLoss( myKingdom );
                    }
                }
                // If the player decided to continue the game after victory, just check that his kingdom is not vanquished
                else if ( !myKingdom.isPlay() ) {
                    result = GameOver::LOSS_ALL;
                }

                if ( result != GameOver::COND_NONE ) {
                    // Don't show the loss dialog if player's kingdom has been vanquished due to the expired countdown of days since the loss of the last town
                    // This case was already handled at the end of the Interface::Basic::HumanTurn()
                    if ( !( result == GameOver::LOSS_ALL && myKingdom.GetCastles().empty() && myKingdom.GetLostTownDays() == 0 ) ) {
                        DialogLoss( result, humanColors );
                    }

                    AGG::ResetMixer();
                    Video::ShowVideo( "LOSE.SMK", Video::VideoAction::LOOP_VIDEO );

                    res = fheroes2::GameMode::MAIN_MENU;
                }
            }
        }
    }
    else {
        if ( activeHumanColors == 0 || ( activeHumanColors == 1 && activeHumanColors == activeColors ) ) {
            res = fheroes2::GameMode::MAIN_MENU;
        }
    }

    return res;
}

StreamBase & GameOver::operator<<( StreamBase & msg, const Result & res )
{
    return msg << res.colors << res.result << res.continueAfterVictory;
}

StreamBase & GameOver::operator>>( StreamBase & msg, Result & res )
{
    return msg >> res.colors >> res.result >> res.continueAfterVictory;
}
