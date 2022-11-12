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

#ifndef H2BATTLE_ONLY_H
#define H2BATTLE_ONLY_H

#include <memory>

#include "army.h"
#include "army_bar.h"
#include "artifact.h"
#include "heroes_indicator.h"
#include "math_base.h"
#include "players.h"
#include "skill_bar.h"

class Heroes;

namespace Battle
{
    struct ControlInfo
    {
        ControlInfo( const fheroes2::Point & pt, int ctrl )
            : result( ctrl )
            , rtLocal( pt.x, pt.y, 24, 24 )
            , rtAI( pt.x + 75, pt.y, 24, 24 )
        {}

        ControlInfo( const ControlInfo & ) = delete;

        ControlInfo & operator=( const ControlInfo & ) = delete;

        void Redraw() const;

        int result;

        const fheroes2::Rect rtLocal;
        const fheroes2::Rect rtAI;
    };

    class Only
    {
    public:
        Only();
        Only( const Only & ) = delete;

        Only & operator=( const Only & ) = delete;

        bool ChangeSettings();
        void StartBattle();

    private:
        void RedrawBaseInfo( const fheroes2::Point & top ) const;

        void UpdateHero1( const fheroes2::Point & cur_pt );
        void UpdateHero2( const fheroes2::Point & cur_pt );

        Heroes * hero1;
        Heroes * hero2;

        Player player1;
        Player player2;

        Army * army1;
        Army * army2;
        Army monsters;

        std::unique_ptr<MoraleIndicator> moraleIndicator1;
        std::unique_ptr<MoraleIndicator> moraleIndicator2;

        std::unique_ptr<LuckIndicator> luckIndicator1;
        std::unique_ptr<LuckIndicator> luckIndicator2;

        std::unique_ptr<PrimarySkillsBar> primskill_bar1;
        std::unique_ptr<PrimarySkillsBar> primskill_bar2;

        std::unique_ptr<SecondarySkillsBar> secskill_bar1;
        std::unique_ptr<SecondarySkillsBar> secskill_bar2;

        std::unique_ptr<ArmyBar> selectArmy1;
        std::unique_ptr<ArmyBar> selectArmy2;

        std::unique_ptr<ArtifactsBar> selectArtifacts1;
        std::unique_ptr<ArtifactsBar> selectArtifacts2;

        std::unique_ptr<ControlInfo> cinfo2;

        fheroes2::Rect rtPortrait1;
        fheroes2::Rect rtPortrait2;
    };
}

#endif
