/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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
    if ( Player * player = Settings::Get().GetPlayers().GetCurrent() ) {
        if ( const auto phero = player->GetFocus<Heroes*>(); phero && ( *phero ) != hero ) {
            ( *phero )->SetMove( false );
            ( *phero )->ShowPath( false );
        }

        hero->ShowPath( true );
        player->SetFocus( hero );

        Redraw( REDRAW_BUTTONS );

        iconsPanel.Select( hero );
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
    if ( Player * player = Settings::Get().GetPlayers().GetCurrent() ) {
        if ( const auto phero = player->GetFocus<Heroes*>() ) {
            ( *phero )->SetMove( false );
            ( *phero )->ShowPath( false );
        }
        player->SetFocus( castle );
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

void Interface::Basic::UpdateFocus()
{
    if ( const Player * player = Settings::Get().GetPlayers().GetCurrent() )
        if ( const auto pcastle = player->GetFocus<Castle *>() )
            SetFocus( *pcastle );
        else if ( const auto phero = player->GetFocus<Heroes *>() )
            SetFocus( *phero );
}

void Interface::Basic::ResetFocus( const int priority )
{
    if ( Player * player = Settings::Get().GetPlayers().GetCurrent() ) {
        Kingdom & myKingdom = world.GetKingdom( player->GetColor() );

        iconsPanel.ResetIcons( ICON_ANY );
        switch ( priority ) {
        case GameFocus::FIRSTHERO: {
            const KingdomHeroes & heroes = myKingdom.GetHeroes();
            // skip sleeping
            KingdomHeroes::const_iterator it = std::find_if( heroes.begin(), heroes.end(), []( const Heroes * hero ) { return !hero->Modes( Heroes::SLEEPER ); } );

            if ( it != heroes.end() )
                SetFocus( *it );
            else
                ResetFocus( GameFocus::CASTLE );
            break;
        }

        case GameFocus::HEROES:
            if ( const auto phero = player->GetFocus<Heroes *>(); phero && ( *phero )->GetColor() == player->GetColor() )
                SetFocus( *phero );
            else if ( !myKingdom.GetHeroes().empty() )
                SetFocus( myKingdom.GetHeroes().front() );
            else if ( !myKingdom.GetCastles().empty() ) {
                iconsPanel.SetRedraw( ICON_HEROES );
                SetFocus( myKingdom.GetCastles().front() );
            }
            else
                player->ResetFocus();
            break;

        case GameFocus::CASTLE:
            if ( const auto pcastle = player->GetFocus<Castle *>(); pcastle && ( *pcastle )->GetColor() == player->GetColor() )
                SetFocus( *pcastle );
            else if ( !myKingdom.GetCastles().empty() )
                SetFocus( myKingdom.GetCastles().front() );
            else if ( !myKingdom.GetHeroes().empty() ) {
                iconsPanel.SetRedraw( ICON_CASTLES );
                SetFocus( myKingdom.GetHeroes().front() );
            }
            else
                player->ResetFocus();
            break;

        default:
            player->ResetFocus();
            break;
        }
    }
}

int Interface::GetFocusType()
{
    if ( Player * player = Settings::Get().GetPlayers().GetCurrent() )
        if ( player->GetFocus<Heroes *>() )
            return GameFocus::HEROES;
        else if ( player->GetFocus<Castle *>() )
            return GameFocus::CASTLE;
    return GameFocus::UNSEL;
}

Castle * Interface::GetFocusCastle()
{
    if (const Player * player = Settings::Get().GetPlayers().GetCurrent() )
        if ( const auto obj = player->GetFocus<Castle *>() )
            return *obj;
    return nullptr;
}

Heroes * Interface::GetFocusHeroes()
{
    if ( const Player * player = Settings::Get().GetPlayers().GetCurrent() )
        if ( const auto obj = player->GetFocus<Heroes *>() )
            return *obj;
    return nullptr;
}

void Interface::Basic::RedrawFocus()
{
    if ( Player * player = Settings::Get().GetPlayers().GetCurrent() ) {
        if ( const auto phero = player->GetFocus<Heroes *>() ) {
            if ( !iconsPanel.IsSelected( ICON_HEROES ) ) {
                iconsPanel.Select( *phero );
            }
            if ( iconsPanel.IsSelected( ICON_CASTLES ) ) {
                iconsPanel.ResetIcons( ICON_CASTLES );
                iconsPanel.SetRedraw( ICON_CASTLES );
            }        
            iconsPanel.SetRedraw( ICON_HEROES );
        }
        else if ( const auto pcastle = player->GetFocus<Castle *>() ) {
            if ( !iconsPanel.IsSelected( ICON_CASTLES ) ) {
                iconsPanel.Select( *pcastle );
            }
            if ( iconsPanel.IsSelected( ICON_HEROES ) ) {
                iconsPanel.ResetIcons( ICON_HEROES );
                iconsPanel.SetRedraw( ICON_HEROES );
            }                
            iconsPanel.SetRedraw( ICON_CASTLES );
        }
        else {
            if ( iconsPanel.IsSelected( ICON_HEROES ) ) {
                iconsPanel.ResetIcons( ICON_HEROES );
                iconsPanel.SetRedraw( ICON_HEROES );
            }
            if ( iconsPanel.IsSelected( ICON_CASTLES ) ) {
                iconsPanel.ResetIcons( ICON_CASTLES );
                iconsPanel.SetRedraw( ICON_CASTLES );
            }
        }
    }

    SetRedraw( REDRAW_GAMEAREA | REDRAW_RADAR );
    statusWindow.SetRedraw();
}
