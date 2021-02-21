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

#include "agg.h"
#include "castle.h"
#include "cursor.h"
#include "game.h"
#include "game_interface.h"
#include "heroes.h"
#include "kingdom.h"
#include "mus.h"
#include "settings.h"
#include "world.h"

void Interface::Basic::SetFocus( Heroes * hero )
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    if ( player ) {
        Focus & focus = player->GetFocus();

        if ( focus.GetHeroes() && focus.GetHeroes() != hero ) {
            focus.GetHeroes()->SetMove( false );
            focus.GetHeroes()->ShowPath( false );
        }

        hero->RescanPath();
        hero->ShowPath( true );
        focus.Set( hero );

        GetButtonsArea().Redraw();

        iconsPanel.Select( hero );
        gameArea.SetCenter( hero->GetCenter() );
        statusWindow.SetState( StatusType::STATUS_ARMY );

        const int heroIndexPos = hero->GetIndex();
        if ( !Game::ChangeMusicDisabled() && heroIndexPos >= 0 ) {
            Game::EnvironmentSoundMixer();
            AGG::PlayMusic( MUS::FromGround( world.GetTiles( heroIndexPos ).GetGround() ), true, true );
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

        GetButtonsArea().Redraw();

        iconsPanel.Select( castle );
        gameArea.SetCenter( castle->GetCenter() );
        statusWindow.SetState( StatusType::STATUS_FUNDS );

        Game::EnvironmentSoundMixer();
        AGG::PlayMusic( MUS::FromGround( world.GetTiles( castle->GetIndex() ).GetGround() ), true, true );
    }
}

void Interface::Basic::ResetFocus( int priority )
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    if ( player ) {
        Focus & focus = player->GetFocus();
        Kingdom & myKingdom = world.GetKingdom( player->GetColor() );

        iconsPanel.ResetIcons();

        switch ( priority ) {
        case GameFocus::FIRSTHERO: {
            const KingdomHeroes & heroes = myKingdom.GetHeroes();
            // skip sleeping
            KingdomHeroes::const_iterator it = std::find_if( heroes.begin(), heroes.end(), []( const Heroes * hero ) { return !hero->Modes( Heroes::SLEEPER ); } );

            if ( it != heroes.end() )
                SetFocus( *it );
            else
                ResetFocus( GameFocus::CASTLE );
        } break;

        case GameFocus::HEROES:
            if ( focus.GetHeroes() && focus.GetHeroes()->GetColor() == player->GetColor() )
                SetFocus( focus.GetHeroes() );
            else if ( myKingdom.GetHeroes().size() )
                SetFocus( myKingdom.GetHeroes().front() );
            else if ( myKingdom.GetCastles().size() ) {
                iconsPanel.SetRedraw( ICON_HEROES );
                SetFocus( myKingdom.GetCastles().front() );
            }
            else
                focus.Reset();
            break;

        case GameFocus::CASTLE:
            if ( focus.GetCastle() && focus.GetCastle()->GetColor() == player->GetColor() )
                SetFocus( focus.GetCastle() );
            else if ( myKingdom.GetCastles().size() )
                SetFocus( myKingdom.GetCastles().front() );
            else if ( myKingdom.GetHeroes().size() ) {
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
}

int Interface::GetFocusType( void )
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    if ( player ) {
        Focus & focus = player->GetFocus();

        if ( focus.GetHeroes() )
            return GameFocus::HEROES;
        else if ( focus.GetCastle() )
            return GameFocus::CASTLE;
    }

    return GameFocus::UNSEL;
}

Castle * Interface::GetFocusCastle( void )
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    return player ? player->GetFocus().GetCastle() : NULL;
}

Heroes * Interface::GetFocusHeroes( void )
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    return player ? player->GetFocus().GetHeroes() : NULL;
}

Army * Interface::GetFocusArmy()
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    if ( player == nullptr )
        return nullptr;

    Heroes * focusedHero = player->GetFocus().GetHeroes();
    if ( focusedHero != nullptr ) {
        return &focusedHero->GetArmy();
    }

    Castle * focusedCastle = player->GetFocus().GetCastle();
    if ( focusedCastle != nullptr ) {
        return &focusedCastle->GetArmy();
    }

    return nullptr;
}

Point Interface::GetFocusCenter( void )
{
    Player * player = Settings::Get().GetPlayers().GetCurrent();

    if ( player ) {
        Focus & focus = player->GetFocus();

        if ( focus.GetHeroes() )
            return focus.GetHeroes()->GetCenter();
        else if ( focus.GetCastle() )
            return focus.GetCastle()->GetCenter();
    }

    return Point( world.w() / 2, world.h() / 2 );
}

void Interface::Basic::RedrawFocus( void )
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

    SetRedraw( REDRAW_GAMEAREA | REDRAW_RADAR );

    if ( type == FOCUS_HEROES )
        iconsPanel.SetRedraw( ICON_HEROES );
    else if ( type == FOCUS_CASTLE )
        iconsPanel.SetRedraw( ICON_CASTLES );

    statusWindow.SetRedraw();
}
