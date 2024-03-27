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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "image.h"
#include "interface_itemsbar.h"
#include "math_base.h"

class Heroes;
namespace Skill
{
    class Secondary;
}

class PrimarySkillsBar : public Interface::ItemsBar<int>
{
public:
    PrimarySkillsBar( Heroes * hero, const bool useSmallSize, const bool isEditMode, const bool allowSkillReset );

    void SetTextOff( int32_t, int32_t );
    void RedrawBackground( const fheroes2::Rect &, fheroes2::Image & ) override;
    void RedrawItem( int &, const fheroes2::Rect &, fheroes2::Image & ) override;

    bool ActionBarLeftMouseSingleClick( int & skill ) override;
    bool ActionBarRightMouseHold( int & skill ) override;
    bool ActionBarCursor( int & skill ) override;

    bool QueueEventProcessing( std::string * = nullptr );

private:
    Heroes * _hero;
    fheroes2::Image backsf;
    bool _useSmallSize;
    bool _isEditMode;
    bool _allowSkillReset;
    std::vector<int> content;
    fheroes2::Point toff;
    std::string msg;
};

class SecondarySkillsBar : public Interface::ItemsBar<Skill::Secondary>
{
public:
    SecondarySkillsBar( const Heroes & hero, const bool mini = true, const bool change = false, const bool showDefaultSkillsMessage = false );

    void RedrawBackground( const fheroes2::Rect &, fheroes2::Image & ) override;
    void RedrawItem( Skill::Secondary &, const fheroes2::Rect &, fheroes2::Image & ) override;

    bool ActionBarLeftMouseSingleClick( Skill::Secondary & skill ) override;
    bool ActionBarRightMouseHold( Skill::Secondary & skill ) override;
    bool ActionBarCursor( Skill::Secondary & skill ) override;

    bool QueueEventProcessing( std::string * = nullptr );

private:
    fheroes2::Image backsf;
    const bool use_mini_sprite;
    const bool can_change;
    const bool _showDefaultSkillsMessage;
    std::string msg;
    const Heroes & _hero;
};

namespace fheroes2
{
    void RedrawPrimarySkillInfo( const fheroes2::Point & pos, PrimarySkillsBar * bar1, PrimarySkillsBar * bar2 );
}
