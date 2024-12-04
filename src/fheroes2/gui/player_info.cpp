/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#include "player_info.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>

#include "agg_image.h"
#include "color.h"
#include "dialog.h"
#include "game.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "maps_fileinfo.h"
#include "players.h"
#include "race.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"

void Interface::PlayersInfo::UpdateInfo( Players & players, const fheroes2::Point & playerTypeOffset, const fheroes2::Point & classOffset )
{
    clear();

    const fheroes2::Sprite & playerTypeImage = fheroes2::AGG::GetICN( ICN::NGEXTRA, 3 );
    const fheroes2::Sprite & classImage = fheroes2::AGG::GetICN( ICN::NGEXTRA, 51 );
    const fheroes2::Sprite & handicapImage = fheroes2::AGG::GetICN( ICN::NGEXTRA, 0 );

    const int32_t playerCount = static_cast<int32_t>( players.size() ); // safe to cast as the number of players <= 8.

    reserve( playerCount );

    for ( int32_t i = 0; i < playerCount; ++i ) {
        emplace_back();

        PlayerInfo & info = back();

        info.player = players[i];
        assert( info.player != nullptr );

        info.playerTypeRoi = { playerTypeOffset.x + Game::GetStep4Player( i, playerTypeImage.width(), playerCount ), playerTypeOffset.y, playerTypeImage.width(),
                               playerTypeImage.height() };
        info.nameRoi = { info.playerTypeRoi.x, info.playerTypeRoi.y + info.playerTypeRoi.height, info.playerTypeRoi.width, 10 };
        info.classRoi = { classOffset.x + Game::GetStep4Player( i, classImage.width(), playerCount ), classOffset.y, classImage.width(), classImage.height() };
        info.handicapRoi
            = { classOffset.x + Game::GetStep4Player( i, handicapImage.width(), playerCount ), classOffset.y + 69, handicapImage.width(), handicapImage.height() };
    }
}

bool Interface::PlayersInfo::SwapPlayers( Player & player1, Player & player2 ) const
{
    const Settings & conf = Settings::Get();
    const Maps::FileInfo & mapInfo = conf.getCurrentMapInfo();

    const int player1Color = player1.GetColor();
    const int player2Color = player2.GetColor();

    bool swap = false;

    if ( player1.isControlAI() == player2.isControlAI() ) {
        swap = true;
    }
    else if ( ( player1Color & mapInfo.AllowCompHumanColors() ) && ( player2Color & mapInfo.AllowCompHumanColors() ) ) {
        const int humans = conf.GetPlayers().GetColors( CONTROL_HUMAN, true );

        if ( humans & player1Color ) {
            Players::SetPlayerControl( player1Color, CONTROL_AI | CONTROL_HUMAN );
            Players::SetPlayerControl( player2Color, CONTROL_HUMAN );
        }
        else {
            Players::SetPlayerControl( player2Color, CONTROL_AI | CONTROL_HUMAN );
            Players::SetPlayerControl( player1Color, CONTROL_HUMAN );
        }

        swap = true;
    }

    if ( swap ) {
        const int player1Race = player1.GetRace();
        const int player2Race = player2.GetRace();

        const Player::HandicapStatus player1HandicapStatus = player1.getHandicapStatus();
        const Player::HandicapStatus player2HandicapStatus = player2.getHandicapStatus();

        player2.setHandicapStatus( player1HandicapStatus );
        player1.setHandicapStatus( player2HandicapStatus );

        if ( player1Race != player2Race && mapInfo.AllowChangeRace( player1Color ) && mapInfo.AllowChangeRace( player2Color ) ) {
            player1.SetRace( player2Race );
            player2.SetRace( player1Race );
        }

        const std::string player1Name = player1.GetName();
        const std::string player2Name = player2.GetName();

        const std::string player1DefaultName = player1.GetDefaultName();
        const std::string player2DefaultName = player2.GetDefaultName();

        if ( player2Name == player2DefaultName ) {
            player1.SetName( player1DefaultName );
        }
        else {
            player1.SetName( player2Name );
        }
        if ( player1Name == player1DefaultName ) {
            player2.SetName( player2DefaultName );
        }
        else {
            player2.SetName( player1Name );
        }
    }

    return swap;
}

Player * Interface::PlayersInfo::GetFromOpponentClick( const fheroes2::Point & pt )
{
    for ( const PlayerInfo & info : *this ) {
        if ( info.playerTypeRoi & pt ) {
            return info.player;
        }
    }

    return nullptr;
}

Player * Interface::PlayersInfo::GetFromOpponentNameClick( const fheroes2::Point & pt )
{
    for ( const PlayerInfo & info : *this ) {
        if ( info.nameRoi & pt ) {
            return info.player;
        }
    }

    return nullptr;
}

Player * Interface::PlayersInfo::GetFromClassClick( const fheroes2::Point & pt )
{
    for ( const PlayerInfo & info : *this ) {
        if ( info.classRoi & pt ) {
            return info.player;
        }
    }

    return nullptr;
}

Player * Interface::PlayersInfo::getPlayerFromHandicapRoi( const fheroes2::Point & point )
{
    for ( const PlayerInfo & info : *this ) {
        if ( info.handicapRoi & point ) {
            return info.player;
        }
    }

    return nullptr;
}

void Interface::PlayersInfo::RedrawInfo( const bool displayInGameInfo ) const
{
    const Settings & conf = Settings::Get();
    fheroes2::Display & display = fheroes2::Display::instance();
    const Maps::FileInfo & mapInfo = conf.getCurrentMapInfo();

    const int32_t playerCount = static_cast<int32_t>( conf.GetPlayers().size() );
    const uint32_t humanColors = conf.GetPlayers().GetColors( CONTROL_HUMAN, true );

    // We need to render icon shadows and since shadows are drawn on left side from images we have to render images from right to left.
    for ( auto iter = crbegin(); iter != crend(); ++iter ) {
        const PlayerInfo & info = *iter;

        uint32_t playerTypeIcnIndex = 0;
        if ( humanColors & info.player->GetColor() ) {
            // Current human.
            playerTypeIcnIndex = 9 + Color::GetIndex( info.player->GetColor() );
        }
        else if ( mapInfo.ComputerOnlyColors() & info.player->GetColor() ) {
            // Computer only.
            playerTypeIcnIndex = 15 + Color::GetIndex( info.player->GetColor() );
        }
        else {
            // Computer or human.
            playerTypeIcnIndex = 3 + Color::GetIndex( info.player->GetColor() );
        }

        // wide sprite offset
        playerTypeIcnIndex += 24;

        const fheroes2::Sprite & playerIconShadow = fheroes2::AGG::GetICN( ICN::NGEXTRA, 60 );
        const fheroes2::Sprite & playerIcon = fheroes2::AGG::GetICN( ICN::NGEXTRA, playerTypeIcnIndex );

        fheroes2::Blit( playerIconShadow, display, info.playerTypeRoi.x - 5, info.playerTypeRoi.y + 3 );

        fheroes2::Blit( playerIcon, display, info.playerTypeRoi.x, info.playerTypeRoi.y );
        if ( currentSelectedPlayer != nullptr && info.player == currentSelectedPlayer ) {
            // TODO: add an overloaded DrawBorder() function to draw a border inside an image.
            fheroes2::Image selection( playerIcon.width(), playerIcon.height() );
            selection.reset();
            fheroes2::DrawBorder( selection, 214 );
            fheroes2::Blit( selection, display, info.playerTypeRoi.x, info.playerTypeRoi.y );
        }

        // draw player name
        fheroes2::Text name( info.player->GetName(), fheroes2::FontType::smallWhite() );
        const int32_t maximumTextWidth = playerIcon.width() - 4;
        name.fitToOneRow( maximumTextWidth );

        name.draw( info.playerTypeRoi.x + 2 + ( maximumTextWidth - name.width() ) / 2, info.playerTypeRoi.y + info.playerTypeRoi.height + 1, display );

        // 2. redraw class
        const bool isActivePlayer = displayInGameInfo ? info.player->isPlay() : mapInfo.AllowChangeRace( info.player->GetColor() );

        const fheroes2::Sprite & classIcon = fheroes2::AGG::GetICN( ICN::NGEXTRA, Race::getRaceIcnIndex( info.player->GetRace(), isActivePlayer ) );
        const fheroes2::Sprite & classIconShadow = fheroes2::AGG::GetICN( ICN::NGEXTRA, 61 );

        fheroes2::Blit( classIconShadow, display, info.classRoi.x - 5, info.classRoi.y + 3 );
        fheroes2::Blit( classIcon, display, info.classRoi.x, info.classRoi.y );

        const char * raceName;

        if ( playerCount > 4 ) {
            raceName = Race::DoubleLinedString( info.player->GetRace() );
        }
        else {
            raceName = Race::String( info.player->GetRace() );
        }
        const int32_t maxClassNameTextWidth = classIcon.width() + 60;

        const fheroes2::Text text( raceName, fheroes2::FontType::smallWhite() );
        text.draw( info.classRoi.x - 31, info.classRoi.y + info.classRoi.height + 4, maxClassNameTextWidth, display );

        // Display a handicap icon.
        uint32_t handicapIcnIndex = 0;
        if ( humanColors & info.player->GetColor() ) {
            switch ( info.player->getHandicapStatus() ) {
            case Player::HandicapStatus::NONE:
                handicapIcnIndex = 0;
                break;
            case Player::HandicapStatus::MILD:
                handicapIcnIndex = 1;
                break;
            case Player::HandicapStatus::SEVERE:
                handicapIcnIndex = 2;
                break;
            default:
                // Did you add a new handicap status? Add the logic above!
                assert( 0 );
                break;
            }
        }
        else {
            assert( info.player->getHandicapStatus() == Player::HandicapStatus::NONE );
            handicapIcnIndex = 78;
        }

        const fheroes2::Sprite & handicapIcon = fheroes2::AGG::GetICN( ICN::NGEXTRA, handicapIcnIndex );
        const fheroes2::Sprite & handicapIconShadow = fheroes2::AGG::GetICN( ICN::NGEXTRA, 59 );

        fheroes2::Blit( handicapIconShadow, display, info.handicapRoi.x - 5, info.handicapRoi.y + 3 );
        fheroes2::Blit( handicapIcon, display, info.handicapRoi.x, info.handicapRoi.y );
    }
}

bool Interface::PlayersInfo::QueueEventProcessing()
{
    Settings & conf = Settings::Get();
    const LocalEvent & le = LocalEvent::Get();

    if ( le.isMouseRightButtonPressed() ) {
        const Player * player = GetFromOpponentClick( le.getMouseCursorPos() );
        if ( player != nullptr ) {
            fheroes2::showStandardTextMessage(
                _( "Opponents" ),
                _( "This lets you change player starting positions and colors. A particular color will always start in a particular location. Some positions may only be played by a computer player or only by a human player." ),
                Dialog::ZERO );
            return true;
        }

        player = GetFromClassClick( le.getMouseCursorPos() );
        if ( player != nullptr ) {
            fheroes2::showStandardTextMessage(
                _( "Class" ),
                _( "This lets you change the class of a player. Classes are not always changeable. Depending on the scenario, a player may receive additional towns and/or heroes not of their primary alignment." ),
                Dialog::ZERO );
            return true;
        }

        player = getPlayerFromHandicapRoi( le.getMouseCursorPos() );
        if ( player != nullptr ) {
            fheroes2::showStandardTextMessage( _( "Handicap" ),
                                               _( "This lets you change the handicap of a particular player. Only human players may have a handicap. Handicapped players "
                                                  "start with fewer resources and earn 15 or 30% fewer resources per turn for mild and severe handicaps, "
                                                  "respectively." ),
                                               Dialog::ZERO );
            return true;
        }

        return false;
    }

    if ( le.isMouseWheelUp() ) {
        Player * player = GetFromClassClick( le.getMouseCursorPos() );
        if ( player != nullptr && conf.getCurrentMapInfo().AllowChangeRace( player->GetColor() ) ) {
            player->SetRace( Race::getPreviousRace( player->GetRace() ) );

            return true;
        }

        return false;
    }

    if ( le.isMouseWheelDown() ) {
        Player * player = GetFromClassClick( le.getMouseCursorPos() );
        if ( player != nullptr && conf.getCurrentMapInfo().AllowChangeRace( player->GetColor() ) ) {
            player->SetRace( Race::getNextRace( player->GetRace() ) );

            return true;
        }
        return false;
    }

    Player * player = GetFromOpponentClick( le.getMouseCursorPos() );
    if ( player != nullptr ) {
        const Maps::FileInfo & fi = conf.getCurrentMapInfo();

        if ( conf.IsGameType( Game::TYPE_MULTI ) ) {
            if ( currentSelectedPlayer == nullptr ) {
                currentSelectedPlayer = player;
            }
            else if ( currentSelectedPlayer == player ) {
                currentSelectedPlayer = nullptr;
            }
            else if ( SwapPlayers( *player, *currentSelectedPlayer ) ) {
                currentSelectedPlayer = nullptr;
            }
        }
        else {
            const int playerColor = player->GetColor();

            if ( playerColor & fi.colorsAvailableForHumans ) {
                const int human = conf.GetPlayers().GetColors( CONTROL_HUMAN, true );

                if ( playerColor != human ) {
                    Player * currentPlayer = Players::Get( human );
                    Player * nextPlayer = Players::Get( playerColor );
                    assert( currentPlayer != nullptr && nextPlayer != nullptr );
                    const Player::HandicapStatus currentHandicapStatus = currentPlayer->getHandicapStatus();

                    Players::SetPlayerControl( human, CONTROL_AI | CONTROL_HUMAN );
                    Players::SetPlayerControl( playerColor, CONTROL_HUMAN );

                    nextPlayer->setHandicapStatus( currentHandicapStatus );
                    currentPlayer->setHandicapStatus( Player::HandicapStatus::NONE );
                }
            }
        }

        return true;
    }

    player = GetFromOpponentNameClick( le.getMouseCursorPos() );
    if ( player != nullptr ) {
        std::string str = _( "%{color} player" );
        StringReplace( str, "%{color}", Color::String( player->GetColor() ) );

        std::string res = player->GetName();
        if ( Dialog::inputString( fheroes2::Text{}, fheroes2::Text{ str, fheroes2::FontType::normalWhite() }, res, 0, false, {} ) && !res.empty() ) {
            player->SetName( res );
        }

        return true;
    }

    player = GetFromClassClick( le.getMouseCursorPos() );
    if ( player != nullptr && conf.getCurrentMapInfo().AllowChangeRace( player->GetColor() ) ) {
        player->SetRace( Race::getNextRace( player->GetRace() ) );

        return true;
    }

    player = getPlayerFromHandicapRoi( le.getMouseCursorPos() );
    if ( player != nullptr ) {
        if ( !( player->GetControl() & CONTROL_AI ) ) {
            switch ( player->getHandicapStatus() ) {
            case Player::HandicapStatus::NONE:
                player->setHandicapStatus( Player::HandicapStatus::MILD );
                break;
            case Player::HandicapStatus::MILD:
                player->setHandicapStatus( Player::HandicapStatus::SEVERE );
                break;
            case Player::HandicapStatus::SEVERE:
                player->setHandicapStatus( Player::HandicapStatus::NONE );
                break;
            default:
                // Did you add a new handicap status? Add the logic above!
                assert( 0 );
                break;
            }
        }
        return true;
    }

    return false;
}

bool Interface::PlayersInfo::readOnlyEventProcessing()
{
    const LocalEvent & le = LocalEvent::Get();
    if ( !le.isMouseRightButtonPressed() ) {
        // Read only mode works only for right click events.
        return false;
    }

    const Player * player = getPlayerFromHandicapRoi( le.getMouseCursorPos() );
    if ( player != nullptr ) {
        switch ( player->getHandicapStatus() ) {
        case Player::HandicapStatus::NONE:
            fheroes2::showStandardTextMessage( _( "No Handicap" ), _( "No special restrictions on starting resources and resource income per turn." ), Dialog::ZERO );
            break;
        case Player::HandicapStatus::MILD:
            fheroes2::showStandardTextMessage( _( "Mild Handicap" ), _( "Players with mild handicap start with fewer resources and earn 15% fewer resources per turn." ),
                                               Dialog::ZERO );
            break;
        case Player::HandicapStatus::SEVERE:
            fheroes2::showStandardTextMessage( _( "Severe Handicap" ),
                                               _( "Players with severe handicap start with fewer resources and earn 30% fewer resources per turn." ), Dialog::ZERO );
            break;
        default:
            // Did you add a new handicap status? Add the logic above!
            assert( 0 );
            break;
        }

        return true;
    }

    return false;
}
