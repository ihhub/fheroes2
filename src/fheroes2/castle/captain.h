/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#ifndef H2CAPTAIN_H
#define H2CAPTAIN_H

#include "gamedefs.h"
#include "heroes_base.h"

class Castle;

class Captain : public HeroBase
{
public:
    explicit Captain( Castle & );

    bool isValid() const override;
    int GetAttack() const override;
    int GetDefense() const override;
    int GetPower() const override;
    int GetKnowledge() const override;
    int GetMorale() const override;
    int GetLuck() const override;
    int GetRace() const override;
    int GetColor() const override;
    int GetType() const override;
    int GetControl() const override;

    const std::string & GetName() const override;

    const Castle * inCastle() const override;

    int GetLevelSkill( int ) const override
    {
        return 0;
    }

    uint32_t GetSecondaryValues( int ) const override
    {
        return 0;
    }

    const Army & GetArmy() const override;
    Army & GetArmy() override;

    uint32_t GetMaxSpellPoints() const override
    {
        return knowledge * 10;
    }

    void ActionPreBattle() override;
    void ActionAfterBattle() override;

    void PortraitRedraw( s32 px, s32 py, PortraitType type, fheroes2::Image & dstsf ) const override;
    fheroes2::Sprite GetPortrait( const PortraitType type ) const;

private:
    int GetManaIndexSprite() const;

    Castle & home;
};

#endif
