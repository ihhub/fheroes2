/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "audio.h"
#include "audio_manager.h"
#include "castle.h"
#include "game.h"
#include "game_interface.h"
#include "heroes.h"
#include "interface_gamearea.h"
#include "interface_icons.h"
#include "interface_status.h"
#include "kingdom.h"
#include "maps_tiles.h"
#include "mus.h"
#include "players.h"
#include "settings.h"
#include "world.h"

void Interface::Basic::SetFocus( Heroes * hero )
{
    SetFocus( hero, false );
}

void Interface::Basic::SetFocus( Heroes * hero, const bool retainScrollBarPosition )
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    if ( player ) {
        Focus & focus = player->GetFocus();

        if ( focus.GetHeroes() && focus.GetHeroes() != hero ) {
            focus.GetHeroes()->SetMove( false );
            focus.GetHeroes()->ShowPath( false );
        }

        hero->ShowPath( true );
        focus.Set( hero );

        Redraw( REDRAW_BUTTONS );

        if ( !retainScrollBarPosition ) {
            iconsPanel.Select( hero );
        }

        gameArea.SetCenter( hero->GetCenter() );
        statusWindow.SetState( StatusType::STATUS_ARMY );

        const int heroIndex = hero->GetIndex();
        if ( Game::UpdateSoundsOnFocusUpdate() && heroIndex >= 0 ) {
            Game::EnvironmentSoundMixer();
            AudioManager::PlayMusicAsync( MUS::FromGround( world.GetTiles( heroIndex ).GetGround() ), Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );
        }
    }
}

void Interface::Basic::SetFocus( Castle * castle )
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    if ( player ) {
        Focus & focus = player->GetFocus();

        if ( focus.GetHeroes() ) {
            focus.GetHeroes()->SetMove( false );
            focus.GetHeroes()->ShowPath( false );
        }

        focus.Set( castle );

        Redraw( REDRAW_BUTTONS );

        iconsPanel.Select( castle );
        gameArea.SetCenter( castle->GetCenter() );
        statusWindow.SetState( StatusType::STATUS_FUNDS );

        if ( Game::UpdateSoundsOnFocusUpdate() ) {
            Game::EnvironmentSoundMixer();
            AudioManager::PlayMusicAsync( MUS::FromGround( world.GetTiles( castle->GetIndex() ).GetGround() ), Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );
        }
    }
}

void Interface::Basic::updateFocus()
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    if ( player == nullptr ) {
        return;
    }

    const Focus & focus = player->GetFocus();

    if ( focus.first == FOCUS_CASTLE ) {
        Castle * castle = GetFocusCastle();

        if ( castle != nullptr ) {
            SetFocus( castle );
        }
    }
    else if ( focus.first == FOCUS_HEROES ) {
        Heroes * hero = GetFocusHeroes();

        if ( hero != nullptr ) {
            SetFocus( hero );
        }
    }
}

void Interface::Basic::ResetFocus( const int priority, const bool retainScrollBarPosition )
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();
    if ( player == nullptr ) {
        return;
    }

    Focus & focus = player->GetFocus();
    Kingdom & myKingdom = world.GetKingdom( player->GetColor() );

    if ( !retainScrollBarPosition ) {
        iconsPanel.ResetIcons( ICON_ANY );
    }

    switch ( priority ) {
    case GameFocus::FIRSTHERO: {
        const KingdomHeroes & heroes = myKingdom.GetHeroes();
        // skip sleeping
        KingdomHeroes::const_iterator it = std::find_if( heroes.begin(), heroes.end(), []( const Heroes * hero ) { return !hero->Modes( Heroes::SLEEPER ); } );

        if ( it != heroes.end() )
            SetFocus( *it );
        else
            ResetFocus( GameFocus::CASTLE, retainScrollBarPosition );
        break;
    }

    case GameFocus::HEROES:
        if ( focus.GetHeroes() && focus.GetHeroes()->GetColor() == player->GetColor() )
            SetFocus( focus.GetHeroes(), retainScrollBarPosition );
        else if ( !myKingdom.GetHeroes().empty() )
            SetFocus( myKingdom.GetHeroes().front() );
        else if ( !myKingdom.GetCastles().empty() ) {
            iconsPanel.SetRedraw( ICON_HEROES );
            SetFocus( myKingdom.GetCastles().front() );
        }
        else
            focus.Reset();
        break;

    case GameFocus::CASTLE:
        if ( focus.GetCastle() && focus.GetCastle()->GetColor() == player->GetColor() )
            SetFocus( focus.GetCastle() );
        else if ( !myKingdom.GetCastles().empty() )
            SetFocus( myKingdom.GetCastles().front() );
        else if ( !myKingdom.GetHeroes().empty() ) {
            iconsPanel.SetRedraw( ICON_CASTLES );
            SetFocus( myKingdom.GetHeroes().front() );
        }
        else
            focus.Reset();
        break;

    default:
        focus.Reset();
        break;
    }
}

int Interface::GetFocusType()
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    if ( player ) {
        Focus & focus = player->GetFocus();

        if ( focus.GetHeroes() )
            return GameFocus::HEROES;
        if ( focus.GetCastle() )
            return GameFocus::CASTLE;
    }

    return GameFocus::UNSEL;
}

Castle * Interface::GetFocusCastle()
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    return player ? player->GetFocus().GetCastle() : nullptr;
}

Heroes * Interface::GetFocusHeroes()
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    return player ? player->GetFocus().GetHeroes() : nullptr;
}

void Interface::Basic::RedrawFocus()
{
    int type = GetFocusType();

    if ( type != FOCUS_HEROES && iconsPanel.IsSelected( ICON_HEROES ) ) {
        iconsPanel.ResetIcons( ICON_HEROES );
        iconsPanel.SetRedraw();
    }
    else if ( type == FOCUS_HEROES && !iconsPanel.IsSelected( ICON_HEROES ) ) {
        iconsPanel.Select( GetFocusHeroes() );
        iconsPanel.SetRedraw();
    }

    if ( type != FOCUS_CASTLE && iconsPanel.IsSelected( ICON_CASTLES ) ) {
        iconsPanel.ResetIcons( ICON_CASTLES );
        iconsPanel.SetRedraw();
    }
    else if ( type == FOCUS_CASTLE && !iconsPanel.IsSelected( ICON_CASTLES ) ) {
        iconsPanel.Select( GetFocusCastle() );
        iconsPanel.SetRedraw();
    }

    SetRedraw( REDRAW_GAMEAREA | REDRAW_RADAR_CURSOR );

    if ( type == FOCUS_HEROES )
        iconsPanel.SetRedraw( ICON_HEROES );
    else if ( type == FOCUS_CASTLE )
        iconsPanel.SetRedraw( ICON_CASTLES );

    statusWindow.SetRedraw();
}
