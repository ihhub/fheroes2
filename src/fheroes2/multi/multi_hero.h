/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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
#include <set>
#include <vector>

#include "multi_protocol.h"

class HeroBase;
class Heroes;
class Spell;
struct ArtifactSetData;

namespace Skill
{
    class Secondary;
}

namespace Multi
{
    // A defending hero of the other computer can gain a level during another player's turn. Choices and messages go to
    // the hero's own player.

    // Returns true if what happens to the hero can be shown here
    bool isPlayedHere( const HeroBase & hero );

    // Human player on the other computer
    bool isPlayedOnOtherComputer( const HeroBase & hero );

    // Waits for the answer only if there is a choice. If the other computer is gone the AI picks the skill.
    Skill::Secondary chooseSecondarySkillOnOtherComputer( const Heroes & hero, const int primarySkill, const Skill::Secondary & first, const Skill::Secondary & second );

    // After a battle, only for heroes of the other computer
    void notifyRaisedSkeletons( const HeroBase & hero, const uint32_t count );
    void notifyLearnedSpells( const HeroBase & hero, const std::vector<Spell> & spells );
    void notifyAssembledArtifacts( const HeroBase & hero, const std::set<ArtifactSetData> & artifactSets );

    // The local player picks a skill which is sent back
    void answerHeroLevelUp( const Message & message );

    void showHeroNotice( const Message & message );

    struct HeroEventStats
    {
        uint32_t levelUpsSent{ 0 };
        uint32_t choicesAsked{ 0 };
        uint32_t choicesReceived{ 0 };
        uint32_t levelUpsReceived{ 0 };
        uint32_t noticesSent{ 0 };
        uint32_t noticesReceived{ 0 };
    };

    const HeroEventStats & getHeroEventStats();
}
