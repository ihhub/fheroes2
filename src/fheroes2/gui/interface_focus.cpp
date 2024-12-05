/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <cassert>
#include <vector>

#include "audio.h"
#include "audio_manager.h"
#include "castle.h"
#include "game.h"
#include "game_interface.h" // IWYU pragma: associated
#include "heroes.h"
#include "interface_base.h"
#include "interface_gamearea.h"
#include "interface_icons.h"
#include "interface_status.h"
#include "kingdom.h"
#include "maps_tiles.h"
#include "mus.h"
#include "players.h"
#include "settings.h"
#include "world.h"

void Interface::AdventureMap::SetFocus( Heroes * hero, const bool retainScrollBarPosition )
{
    assert( hero != nullptr );

    Player * player = Settings::Get().GetPlayers().GetCurrent();
    if ( player == nullptr ) {
        return;
    }

#ifndef NDEBUG
#if defined( WITH_DEBUG )
    const bool isAIAutoControlMode = player->isAIAutoControlMode();
#else
    const bool isAIAutoControlMode = false;
#endif // WITH_DEBUG
#endif // !NDEBUG

    assert( player->GetColor() == hero->GetColor() && ( player->isControlHuman() || ( player->isControlAI() && isAIAutoControlMode ) ) );

    Focus & focus = player->GetFocus();

    Heroes * focusedHero = focus.GetHeroes();
    if ( focusedHero && focusedHero != hero ) {
        // Focus has been changed to another hero. Hide the path of the previous hero.
        focusedHero->ShowPath( false );
    }

    // Heroes::calculatePath() uses PlayerWorldPathfinder and should not be used for an AI-controlled hero
    if ( !hero->isMoveEnabled() && hero->isControlHuman() ) {
        hero->calculatePath( -1 );
    }

    hero->ShowPath( true );
    focus.Set( hero );

    setRedraw( REDRAW_BUTTONS );

    if ( !retainScrollBarPosition ) {
        _iconsPanel.select( hero );
    }

    _gameArea.SetCenter( hero->GetCenter() );
    _statusPanel.SetState( StatusType::STATUS_ARMY );

    const int heroIndex = hero->GetIndex();
    if ( Game::UpdateSoundsOnFocusUpdate() && heroIndex >= 0 ) {
        Game::EnvironmentSoundMixer();
        AudioManager::PlayMusicAsync( MUS::FromGround( world.getTile( heroIndex ).GetGround() ), Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );
    }
}

void Interface::AdventureMap::SetFocus( Castle * castle )
{
    assert( castle != nullptr );

    Player * player = Settings::Get().GetPlayers().GetCurrent();
    if ( player == nullptr ) {
        return;
    }

#ifndef NDEBUG
#if defined( WITH_DEBUG )
    const bool isAIAutoControlMode = player->isAIAutoControlMode();
#else
    const bool isAIAutoControlMode = false;
#endif // WITH_DEBUG
#endif // !NDEBUG

    assert( player->GetColor() == castle->GetColor() && ( player->isControlHuman() || ( player->isControlAI() && isAIAutoControlMode ) ) );

    Focus & focus = player->GetFocus();

    Heroes * focusedHero = focus.GetHeroes();
    if ( focusedHero ) {
        focusedHero->ShowPath( false );
    }

    focus.Set( castle );

    setRedraw( REDRAW_BUTTONS );

    _iconsPanel.select( castle );
    _gameArea.SetCenter( castle->GetCenter() );
    _statusPanel.SetState( StatusType::STATUS_FUNDS );

    if ( Game::UpdateSoundsOnFocusUpdate() ) {
        Game::EnvironmentSoundMixer();
        AudioManager::PlayMusicAsync( MUS::FromGround( world.getTile( castle->GetIndex() ).GetGround() ), Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );
    }
}

void Interface::AdventureMap::updateFocus()
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
            SetFocus( hero, false );
        }
    }
}

void Interface::AdventureMap::ResetFocus( const int priority, const bool retainScrollBarPosition )
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();
    if ( player == nullptr ) {
        return;
    }

    Focus & focus = player->GetFocus();
    Kingdom & myKingdom = world.GetKingdom( player->GetColor() );

    if ( !retainScrollBarPosition ) {
        _iconsPanel.resetIcons( ICON_ANY );
    }

    switch ( priority ) {
    case GameFocus::FIRSTHERO: {
        const VecHeroes & heroes = myKingdom.GetHeroes();
        // Find first hero excluding sleeping ones.
        const VecHeroes::const_iterator it = std::find_if( heroes.begin(), heroes.end(), []( const Heroes * hero ) { return !hero->Modes( Heroes::SLEEPER ); } );

        if ( it != heroes.end() ) {
            SetFocus( *it, false );
        }
        else {
            // There are no non-sleeping heroes. Focus on the castle instead.
            ResetFocus( GameFocus::CASTLE, retainScrollBarPosition );
        }
        break;
    }

    case GameFocus::HEROES:
        if ( Heroes * hero = focus.GetHeroes(); ( hero != nullptr ) && ( hero->GetColor() == player->GetColor() ) ) {
            // Set focus on the previously focused hero.
            SetFocus( hero, retainScrollBarPosition );
        }
        else if ( !myKingdom.GetHeroes().empty() ) {
            // Reset scrollbar here because the current focused hero might have
            // lost a battle and is not in the heroes list anymore.
            _iconsPanel.resetIcons( ICON_HEROES );
            SetFocus( myKingdom.GetHeroes().front(), false );
        }
        else if ( !myKingdom.GetCastles().empty() ) {
            // There are no heroes left in the kingdom. Reset the heroes scrollbar and focus on the first castle.
            _iconsPanel.setRedraw( ICON_HEROES );
            SetFocus( myKingdom.GetCastles().front() );
        }
        else {
            focus.Reset();
        }
        break;

    case GameFocus::CASTLE:
        if ( Castle * castle = focus.GetCastle(); ( castle != nullptr ) && ( castle->GetColor() == player->GetColor() ) ) {
            // Focus on the previously focused castle.
            SetFocus( castle );
        }
        else if ( !myKingdom.GetCastles().empty() ) {
            // The previously focused castle is lost, so we update the castles scrollbar.
            _iconsPanel.resetIcons( ICON_CASTLES );
            SetFocus( myKingdom.GetCastles().front() );
        }
        else if ( !myKingdom.GetHeroes().empty() ) {
            // There are no castles left in the kingdom. Reset the castles scrollbar and focus on the first hero.
            _iconsPanel.setRedraw( ICON_CASTLES );
            SetFocus( myKingdom.GetHeroes().front(), false );
        }
        else {
            focus.Reset();
        }
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

        if ( focus.GetHeroes() ) {
            return GameFocus::HEROES;
        }
        if ( focus.GetCastle() ) {
            return GameFocus::CASTLE;
        }
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

void Interface::AdventureMap::RedrawFocus()
{
    const int type = GetFocusType();

    if ( type != FOCUS_HEROES && _iconsPanel.isSelected( ICON_HEROES ) ) {
        _iconsPanel.resetIcons( ICON_HEROES );
        _iconsPanel.setRedraw();
    }
    else if ( type == FOCUS_HEROES && !_iconsPanel.isSelected( ICON_HEROES ) ) {
        _iconsPanel.select( GetFocusHeroes() );
        _iconsPanel.setRedraw();
    }

    if ( type != FOCUS_CASTLE && _iconsPanel.isSelected( ICON_CASTLES ) ) {
        _iconsPanel.resetIcons( ICON_CASTLES );
        _iconsPanel.setRedraw();
    }
    else if ( type == FOCUS_CASTLE && !_iconsPanel.isSelected( ICON_CASTLES ) ) {
        _iconsPanel.select( GetFocusCastle() );
        _iconsPanel.setRedraw();
    }

    setRedraw( REDRAW_GAMEAREA | REDRAW_RADAR_CURSOR );

    if ( type == FOCUS_HEROES ) {
        _iconsPanel.setRedraw( ICON_HEROES );
    }
    else if ( type == FOCUS_CASTLE ) {
        _iconsPanel.setRedraw( ICON_CASTLES );
    }

    _statusPanel.setRedraw();
}
