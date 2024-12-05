/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include "interface_base.h"
#include "interface_buttons.h"
#include "interface_cpanel.h"
#include "interface_icons.h"
#include "interface_status.h"
#include "players.h"

class Castle;
class Heroes;

namespace Maps
{
    class Tile;
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
    Castle * GetFocusCastle();
    Heroes * GetFocusHeroes();
    int GetFocusType();

    class AdventureMap final : public BaseInterface
    {
    public:
        // This class is used to lock rendering of Basic class. This is useful when we have to generate only a single frame.
        // Use this class ONLY when you are going to call rendering after all other operations.
        class RedrawLocker
        {
        public:
            explicit RedrawLocker( AdventureMap & interface_ )
                : _interface( interface_ )
            {
                _interface._lockRedraw = true;
            }

            RedrawLocker( const RedrawLocker & ) = delete;
            RedrawLocker( RedrawLocker && ) = delete;

            RedrawLocker & operator=( const RedrawLocker & ) = delete;
            RedrawLocker & operator=( RedrawLocker && ) = delete;

            ~RedrawLocker()
            {
                _interface._lockRedraw = false;
            }

        private:
            AdventureMap & _interface;
        };

        static AdventureMap & Get();

        void redraw( const uint32_t force ) override;

        int32_t GetDimensionDoorDestination( const int32_t from, const int32_t distance, const bool water );

        IconsPanel & GetIconsPanel()
        {
            return _iconsPanel;
        }

        ControlPanel & getControlPanel()
        {
            return _controlPanel;
        }

        StatusPanel & getStatusPanel()
        {
            return _statusPanel;
        }

        void SetFocus( Heroes *, const bool retainScrollBarPosition );
        void SetFocus( Castle * );
        void ResetFocus( const int priority, const bool retainScrollBarPosition );
        void RedrawFocus();
        void updateFocus();

        void EventSwitchHeroSleeping();
        fheroes2::GameMode EventDefaultAction();
        void EventOpenFocus() const;
        fheroes2::GameMode EventSaveGame() const;
        void EventPuzzleMaps() const;
        static fheroes2::GameMode EventScenarioInformation();
        void EventSystemDialog() const;
        void EventSwitchFocusedHero( const int32_t tileIndex );
        void EventNextHero();
        void EventNextTown();
        fheroes2::GameMode EventHeroMovement();
        void EventResetHeroPath();
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
        fheroes2::GameMode EventDigArtifact();
        void EventKeyArrowPress( int direct );

        fheroes2::GameMode StartGame();

        void mouseCursorAreaClickLeft( const int32_t tileIndex ) override;
        void mouseCursorAreaPressRight( const int32_t tileIndex ) const override;
        void mouseCursorAreaLongPressLeft( const int32_t tileIndex ) override;

        void updateCursor( const int32_t tileIndex ) override;

        // Regenerates the game area and updates the panel positions depending on the UI settings
        void reset() override;

    private:
        AdventureMap();

        static int GetCursorFocusCastle( const Castle & castle, const Maps::Tile & tile );
        static int GetCursorFocusHeroes( const Heroes & hero, const Maps::Tile & tile );
        static int GetCursorFocusShipmaster( const Heroes & hero, const Maps::Tile & tile );
        static int _getCursorNoFocus( const Maps::Tile & tile );
        static int GetCursorTileIndex( int32_t dstIndex );

        void ShowPathOrStartMoveHero( Heroes * hero, const int32_t destinationIdx );
        void MoveHeroFromArrowKeys( Heroes & hero, const int direction );
        void _startHeroMove( Heroes & hero );

        fheroes2::GameMode HumanTurn( const bool isLoadedFromSave );

        IconsPanel _iconsPanel;
        ButtonsPanel _buttonsPanel;
        ControlPanel _controlPanel;
        StatusPanel _statusPanel;

        bool _lockRedraw;
    };
}

#endif
