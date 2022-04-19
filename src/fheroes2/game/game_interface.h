/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2019 - 2022                                             *
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

#ifndef H2GAMEINTERFACE_H
#define H2GAMEINTERFACE_H

#include "interface_buttons.h"
#include "interface_cpanel.h"
#include "interface_gamearea.h"
#include "interface_icons.h"
#include "interface_radar.h"
#include "interface_status.h"
#include "players.h"

class Castle;
class Heroes;

namespace Maps
{
    class Tiles;
}

namespace GameFocus
{
    enum
    {
        UNSEL = FOCUS_UNSEL,
        HEROES = FOCUS_HEROES,
        CASTLE = FOCUS_CASTLE,
        FIRSTHERO
    };
}

namespace Interface
{
    enum redraw_t
    {
        REDRAW_RADAR = 0x01,
        REDRAW_HEROES = 0x02,
        REDRAW_CASTLES = 0x04,
        REDRAW_BUTTONS = 0x08,
        REDRAW_STATUS = 0x10,
        REDRAW_BORDER = 0x20,
        REDRAW_GAMEAREA = 0x40,
        REDRAW_CURSOR = 0x80,

        REDRAW_ICONS = REDRAW_HEROES | REDRAW_CASTLES,
        REDRAW_ALL = 0xFF
    };

    Castle * GetFocusCastle( void );
    Heroes * GetFocusHeroes( void );
    int GetFocusType( void );
    fheroes2::Point GetFocusCenter( void );

    class Basic
    {
    public:
        static Basic & Get();

        bool NeedRedraw() const
        {
            return redraw != 0;
        }

        void SetRedraw( int f )
        {
            redraw |= f;
        }

        int GetRedrawMask() const
        {
            return redraw;
        }

        void Redraw( int f = 0 );

        static bool isScrollLeft( const fheroes2::Point & cursorPos )
        {
            return cursorPos.x < BORDERWIDTH;
        }

        static bool isScrollRight( const fheroes2::Point & cursorPos )
        {
            const fheroes2::Display & display = fheroes2::Display::instance();

            return cursorPos.x >= display.width() - BORDERWIDTH;
        }

        static bool isScrollTop( const fheroes2::Point & cursorPos )
        {
            return cursorPos.y < BORDERWIDTH;
        }

        static bool isScrollBottom( const fheroes2::Point & cursorPos )
        {
            const fheroes2::Display & display = fheroes2::Display::instance();

            return cursorPos.y >= display.height() - BORDERWIDTH;
        }

        int32_t GetDimensionDoorDestination( const int32_t from, const int32_t distance, const bool water );

        GameArea & GetGameArea()
        {
            return gameArea;
        }

        Radar & GetRadar()
        {
            return radar;
        }

        IconsPanel & GetIconsPanel()
        {
            return iconsPanel;
        }

        ButtonsArea & GetButtonsArea()
        {
            return buttonsArea;
        }

        StatusWindow & GetStatusWindow()
        {
            return statusWindow;
        }

        ControlPanel & GetControlPanel()
        {
            return controlPanel;
        }

        void SetFocus( Heroes * );
        void SetFocus( Castle * );
        void ResetFocus( int );
        void RedrawFocus( void );

        void SetHideInterface( bool );

        void EventSwitchHeroSleeping( void );
        fheroes2::GameMode EventDefaultAction( const fheroes2::GameMode gameMode );
        void EventOpenFocus( void ) const;
        fheroes2::GameMode EventSaveGame() const;
        void EventPuzzleMaps( void ) const;
        static fheroes2::GameMode EventGameInfo();
        void EventSystemDialog() const;
        void EventNextHero( void );
        void EventNextTown( void );
        void EventContinueMovement( void ) const;
        void EventKingdomInfo( void ) const;
        void EventCastSpell( void );
        void EventSwitchShowRadar( void ) const;
        void EventSwitchShowStatus( void ) const;
        void EventSwitchShowButtons( void ) const;
        void EventSwitchShowIcons( void );
        void EventSwitchShowControlPanel( void ) const;

        fheroes2::GameMode EventNewGame() const;
        fheroes2::GameMode EventLoadGame() const;
        fheroes2::GameMode EventAdventureDialog();
        void EventViewWorld();
        fheroes2::GameMode EventFileDialog( void ) const;
        fheroes2::GameMode EventEndTurn() const;
        static fheroes2::GameMode EventExit();
        fheroes2::GameMode EventDigArtifact();
        void EventKeyArrowPress( int direct );

        fheroes2::GameMode StartGame();

        void MouseCursorAreaClickLeft( const int32_t index_maps );
        void MouseCursorAreaPressRight( s32 ) const;

        static int GetCursorTileIndex( s32 );
        static int GetCursorFocusCastle( const Castle &, const Maps::Tiles & );
        static int GetCursorFocusHeroes( const Heroes &, const Maps::Tiles & );
        static int GetCursorFocusShipmaster( const Heroes &, const Maps::Tiles & );
        void CalculateHeroPath( Heroes * hero, int32_t destinationIdx ) const;

        void Reset(); // call this function only when changing the resolution

    private:
        Basic();
        void ShowPathOrStartMoveHero( Heroes *, s32 );
        void MoveHeroFromArrowKeys( Heroes & hero, int direct );
        fheroes2::GameMode HumanTurn( bool );

        GameArea gameArea;
        Radar radar;
        IconsPanel iconsPanel;
        ButtonsArea buttonsArea;
        StatusWindow statusWindow;
        ControlPanel controlPanel;

        int redraw;
    };
}

#endif
