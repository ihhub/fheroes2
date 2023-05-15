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

#ifndef H2GAMEINTERFACE_H
#define H2GAMEINTERFACE_H

#include <cstdint>

#include "game_mode.h"
#include "gamedefs.h"
#include "interface_buttons.h"
#include "interface_cpanel.h"
#include "interface_gamearea.h"
#include "interface_icons.h"
#include "interface_radar.h"
#include "interface_status.h"
#include "math_base.h"
#include "players.h"
#include "screen.h"

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
    enum redraw_t : uint32_t
    {
        // To render the cursor over the previously generated radar map image.
        REDRAW_RADAR_CURSOR = 0x01,
        // To render radar map fully or in ROI and then the cursor over it.
        REDRAW_RADAR = 0x02,
        REDRAW_HEROES = 0x04,
        REDRAW_CASTLES = 0x08,
        REDRAW_BUTTONS = 0x10,
        REDRAW_STATUS = 0x20,
        REDRAW_BORDER = 0x40,
        REDRAW_GAMEAREA = 0x80,

        REDRAW_ICONS = REDRAW_HEROES | REDRAW_CASTLES,
        REDRAW_ALL = 0xFF
    };

    Castle * GetFocusCastle();
    Heroes * GetFocusHeroes();
    int GetFocusType();

    class Basic
    {
    public:
        // This class is used to lock rendering of Basic class. This is useful when we have to generate only a single frame.
        // Use this class ONLY when you are going to call rendering after all other operations.
        class RedrawLocker
        {
        public:
            explicit RedrawLocker( Basic & basic )
                : _basic( basic )
            {
                _basic._lockRedraw = true;
            }

            RedrawLocker( const RedrawLocker & ) = delete;
            RedrawLocker( RedrawLocker && ) = delete;

            RedrawLocker & operator=( const RedrawLocker & ) = delete;
            RedrawLocker & operator=( RedrawLocker && ) = delete;

            ~RedrawLocker()
            {
                _basic._lockRedraw = false;
            }

        private:
            Basic & _basic;
        };

        static Basic & Get();

        bool NeedRedraw() const
        {
            return redraw != 0;
        }

        void SetRedraw( const uint32_t r )
        {
            redraw |= r;
        }

        uint32_t GetRedrawMask() const
        {
            return redraw;
        }

        void Redraw( const uint32_t force = 0 );

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

        StatusWindow & GetStatusWindow()
        {
            return statusWindow;
        }

        ControlPanel & GetControlPanel()
        {
            return controlPanel;
        }

        void SetFocus( Heroes *, const bool retainScrollBarPosition );
        void SetFocus( Castle * );
        void ResetFocus( const int priority, const bool retainScrollBarPosition );
        void RedrawFocus();
        void updateFocus();

        void EventSwitchHeroSleeping();
        fheroes2::GameMode EventDefaultAction( const fheroes2::GameMode gameMode );
        void EventOpenFocus() const;
        fheroes2::GameMode EventSaveGame() const;
        void EventPuzzleMaps() const;
        static fheroes2::GameMode EventScenarioInformation();
        void EventSystemDialog() const;
        void EventNextHero();
        void EventNextTown();
        void EventContinueMovement() const;
        void EventKingdomInfo() const;
        void EventCastSpell();
        void EventSwitchShowRadar() const;
        void EventSwitchShowStatus() const;
        void EventSwitchShowButtons() const;
        void EventSwitchShowIcons() const;
        void EventSwitchShowControlPanel() const;

        fheroes2::GameMode EventNewGame() const;
        fheroes2::GameMode EventLoadGame() const;
        fheroes2::GameMode EventAdventureDialog();
        void EventViewWorld();
        fheroes2::GameMode EventFileDialog() const;
        fheroes2::GameMode EventEndTurn() const;
        static fheroes2::GameMode EventExit();
        fheroes2::GameMode EventDigArtifact();
        void EventKeyArrowPress( int direct );

        fheroes2::GameMode StartGame();

        void MouseCursorAreaClickLeft( const int32_t index_maps );
        void MouseCursorAreaPressRight( int32_t ) const;

        static int GetCursorTileIndex( int32_t );
        static int GetCursorFocusCastle( const Castle &, const Maps::Tiles & );
        static int GetCursorFocusHeroes( const Heroes &, const Maps::Tiles & );
        static int GetCursorFocusShipmaster( const Heroes &, const Maps::Tiles & );
        void CalculateHeroPath( Heroes * hero, int32_t destinationIdx ) const;

        // Regenerates the game area and updates the panel positions depending on the UI settings
        void Reset();

    private:
        Basic();
        void ShowPathOrStartMoveHero( Heroes *, int32_t );
        void MoveHeroFromArrowKeys( Heroes & hero, int direct );
        fheroes2::GameMode HumanTurn( const bool isload );

        GameArea gameArea;
        Radar radar;
        IconsPanel iconsPanel;
        ButtonsArea buttonsArea;
        StatusWindow statusWindow;
        ControlPanel controlPanel;

        uint32_t redraw;

        bool _lockRedraw;
    };
}

#endif
