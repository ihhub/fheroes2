/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2SELECT_SCENARIO_H
#define H2SELECT_SCENARIO_H

#include "interface_list.h"
#include "maps_fileinfo.h"

struct ScenarioListItemCoordinates
{
public:
    int32_t playersCountCoordX;
    int32_t mapSizeCoordX;
    int32_t mapTypeCoordX;
    int32_t mapNameWidth;
    int32_t mapNameCoordX;
    int32_t winConditionsCoordX;
    int32_t lossConditionsCoordX;
    ScenarioListItemCoordinates() = default;

    ScenarioListItemCoordinates( int32_t playersCountCoordX, int32_t mapSizeCoordX, int32_t mapTypeCoordX, int32_t mapNameWidth, int32_t mapNameCoordX,
                                 int32_t winConditionsCoordX, int32_t lossConditionsCoordX )
    {
        this->playersCountCoordX = playersCountCoordX;
        this->mapSizeCoordX = mapSizeCoordX;
        this->mapTypeCoordX = mapTypeCoordX;
        this->mapNameWidth = mapNameWidth;
        this->mapNameCoordX = mapNameCoordX;
        this->winConditionsCoordX = winConditionsCoordX;
        this->lossConditionsCoordX = lossConditionsCoordX;
    }
};

struct SelectedScenarioCoordinates
{
public:
    int32_t playersCountCoordX;
    int32_t mapSizeCoordX;
    int32_t mapTypeCoordX;
    int32_t mapNameWidth;
    int32_t mapNameCoordX;
    int32_t winConditionsCoordX;
    int32_t lossConditionsCoordX;
    int32_t difficultyCoordX;
    int32_t difficultyWidth;
    int32_t descriptionCoordX;

    SelectedScenarioCoordinates() = default;

    SelectedScenarioCoordinates( int32_t playersCountCoordX, int32_t mapSizeCoordX, int32_t mapTypeCoordX, int32_t mapNameWidth, int32_t mapNameCoordX,
                                 int32_t winConditionsCoordX, int32_t lossConditionsCoordX, int32_t difficultyCoordX, int32_t descriptionCoordX, int32_t difficultyWidth )
    {
        this->playersCountCoordX = playersCountCoordX;
        this->mapSizeCoordX = mapSizeCoordX;
        this->mapTypeCoordX = mapTypeCoordX;
        this->mapNameWidth = mapNameWidth;
        this->mapNameCoordX = mapNameCoordX;
        this->winConditionsCoordX = winConditionsCoordX;
        this->lossConditionsCoordX = lossConditionsCoordX;
        this->difficultyCoordX = difficultyCoordX;
        this->difficultyWidth = difficultyWidth;
        this->descriptionCoordX = descriptionCoordX;
    }
};

class ScenarioListBox : public Interface::ListBox<Maps::FileInfo>
{
public:
    using Interface::ListBox<Maps::FileInfo>::ActionListDoubleClick;
    using Interface::ListBox<Maps::FileInfo>::ActionListSingleClick;
    using Interface::ListBox<Maps::FileInfo>::ActionListPressRight;

    explicit ScenarioListBox( const fheroes2::Point & pt, const ScenarioListItemCoordinates & listItemCoords, const SelectedScenarioCoordinates & selectedCoords )
        : Interface::ListBox<Maps::FileInfo>( pt )
        , selectOk( false )
        , _listItemCoords( listItemCoords )
        , _selectedCoords( selectedCoords )
    {}

    void RedrawItem( const Maps::FileInfo & info, int32_t /*dstx*/, int32_t dsty, bool current ) override;
    void RedrawBackground( const fheroes2::Point & dst ) override;

    void ActionCurrentUp() override
    {
        // Do nothing.
    }

    void ActionCurrentDn() override
    {
        // Do nothing.
    }

    void ActionListDoubleClick( Maps::FileInfo & ) override;

    void ActionListSingleClick( Maps::FileInfo & ) override
    {
        // Do nothing.
    }

    void ActionListPressRight( Maps::FileInfo & /* info */ ) override
    {
        // Do nothing.
    }

    bool selectOk;

private:
    const ScenarioListItemCoordinates & _listItemCoords;
    const SelectedScenarioCoordinates & _selectedCoords;
    const short _offsetY = 4;

    void _renderMapListItem( const Maps::FileInfo & info, fheroes2::Display & display, int32_t & dsty, bool current );
    void _renderSelectedMapInfo( fheroes2::Display & display, const fheroes2::Point & dst );
    void _renderMapName( const Maps::FileInfo & info, bool selected, const int32_t & baseYOffset, fheroes2::Display & display ) const;
    const fheroes2::Sprite & _getPlayersCountIcon( uint8_t playersCount );
    const fheroes2::Image & _getMapSizeIcon( uint16_t size );
    const fheroes2::Sprite & _getMapTypeIcon( GameVersion version );
    const fheroes2::Sprite & _getWinConditionsIcon( uint8_t condition );
    const fheroes2::Sprite & _getLossConditionsIcon( uint8_t condition );
};

namespace Dialog
{
    const Maps::FileInfo * SelectScenario( const MapsFileInfoList & all );
}

#endif
