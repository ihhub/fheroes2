/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "interface_itemsbar.h"
#include "skill.h"

class PrimarySkillsBar : public Interface::ItemsBar<int>
{
public:
    PrimarySkillsBar( const Heroes *, bool mini );

    void SetTextOff( s32, s32 );
    virtual void RedrawBackground( const Rect &, fheroes2::Image & ) override;
    virtual void RedrawItem( int &, const Rect &, fheroes2::Image & ) override;

    virtual bool ActionBarLeftMouseSingleClick( int & skill ) override;
    virtual bool ActionBarRightMouseHold( int & skill ) override;
    virtual bool ActionBarCursor( int & skill ) override;

    bool QueueEventProcessing( std::string * = NULL );

protected:
    const Heroes * hero;
    fheroes2::Image backsf;
    bool use_mini_sprite;
    std::vector<int> content;
    fheroes2::Point toff;
    std::string msg;
};

class SecondarySkillsBar : public Interface::ItemsBar<Skill::Secondary>
{
public:
    SecondarySkillsBar( bool mini = true, bool change = false );

    virtual void RedrawBackground( const Rect &, fheroes2::Image & ) override;
    virtual void RedrawItem( Skill::Secondary &, const Rect &, fheroes2::Image & ) override;

    virtual bool ActionBarLeftMouseSingleClick( Skill::Secondary & skill ) override;
    virtual bool ActionBarRightMouseHold( Skill::Secondary & skill ) override;
    virtual bool ActionBarCursor( Skill::Secondary & skill ) override;

    bool QueueEventProcessing( std::string * = NULL );

protected:
    fheroes2::Image backsf;
    bool use_mini_sprite;
    bool can_change;
    std::string msg;
};
