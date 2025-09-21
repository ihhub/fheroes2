/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "army.h"
#include "army_bar.h"
#include "artifact.h"
#include "heroes.h"
#include "heroes_indicator.h"
#include "math_base.h"
#include "players.h"
#include "skill_bar.h"

namespace fheroes2
{
    class Image;
}

namespace Battle
{
    struct ControlInfo
    {
        ControlInfo( const fheroes2::Point & pt, int ctrl )
            : result( ctrl )
            , rtLocal( pt.x, pt.y, 24, 24 )
            , rtAI( pt.x + 75, pt.y, 24, 24 )
        {
            // Do nothing.
        }

        ControlInfo( const ControlInfo & ) = delete;

        ControlInfo & operator=( const ControlInfo & ) = delete;

        void Redraw() const;

        int result{ 0 };

        const fheroes2::Rect rtLocal;
        const fheroes2::Rect rtAI;
    };

    class Only
    {
    public:
        Only();
        Only( const Only & ) = delete;

        Only & operator=( const Only & ) = delete;

        bool setup( const bool allowBackup, bool & reset );
        void StartBattle();

        void reset();

    private:
        struct ArmyUI
        {
            std::unique_ptr<MoraleIndicator> morale;
            std::unique_ptr<LuckIndicator> luck;
            std::unique_ptr<PrimarySkillsBar> primarySkill;
            std::unique_ptr<SecondarySkillsBar> secondarySkill;
            std::unique_ptr<ArtifactsBar> artifact;
            std::unique_ptr<ArmyBar> army;

            void redraw( fheroes2::Image & output ) const;

            // Resets the state to empty and removes the morale & luck indicators from the screen
            void resetForNewHero();
        };

        struct ArmyInfo
        {
            Heroes * hero{ nullptr };

            Player player;

            int controlType{ CONTROL_HUMAN };

            Army monster;

            Heroes heroBackup;

            Army monsterBackup;

            bool isHeroPresent{ false };

            ArmyUI ui;

            fheroes2::Rect portraitRoi;

            uint8_t armyId{ 0 };

            bool needRedraw{ false };

            void reset();
        };

        std::array<ArmyInfo, 2> armyInfo;

        std::unique_ptr<ControlInfo> attackedArmyControlInfo;

        bool _backupCompleted{ false };

        void redrawOpponents( const fheroes2::Point & top ) const;

        void redrawOpponentsStats( const fheroes2::Point & top ) const;

        static void updateHero( ArmyInfo & info, const fheroes2::Point & offset );

        static void updateArmyUI( ArmyUI & ui, Heroes * hero, const fheroes2::Point & offset, const uint8_t armyId );

        static void copyHero( const Heroes & in, Heroes & out );
    };
}
