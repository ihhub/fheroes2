/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2EDITOR_DIALOGS_H
#define H2EDITOR_DIALOGS_H

#include <cstdint>
#include <memory>
#include <string>

#include "artifact.h"
#include "heroes.h"
#include "image.h"
#include "interface_list.h"
#include "math_base.h"
#include "monster.h"
#include "skill.h"
#include "spell.h"
#include "ui_button.h"
#include "ui_window.h"

class Kingdom;

namespace Dialog
{
    class ItemSelectionWindow : public Interface::ListBox<int>
    {
    public:
        using Interface::ListBox<int>::ActionListDoubleClick;
        using Interface::ListBox<int>::ActionListSingleClick;
        using Interface::ListBox<int>::ActionListPressRight;

        ItemSelectionWindow() = delete;

        explicit ItemSelectionWindow( const fheroes2::Size & dialogSize, std::string title, std::string description = {} );

        void RedrawBackground( const fheroes2::Point & /* unused */ ) override
        {
            _backgroundRestorer->restore();
        }

        void ActionListDoubleClick( int & /* unused */ ) override
        {
            _isDoubleClicked = true;
        }

        void RedrawItem( const int & /* unused */, int32_t /* ox */, int32_t /* oy */, bool /* current */ ) override
        {
            // Do nothing.
        }

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListSingleClick( int & /* unused */ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( int & /* unused */ ) override
        {
            // Do nothing.
        }

        void updateScrollBarImage();

        // An image with text should have offset of 10 pixels from all left and right edges.
        void renderItem( const fheroes2::Sprite & itemSprite, std::string itemText, const fheroes2::Point & destination, const int32_t middleImageOffsetX,
                         const int32_t textOffsetX, const int32_t itemOffsetY, const bool current ) const;

        int32_t selectItemsEventProcessing();

        fheroes2::Rect getBackgroundArea() const;

    private:
        bool _isDoubleClicked{ false };
        std::unique_ptr<fheroes2::StandardWindow> _window;
        std::unique_ptr<fheroes2::ImageRestorer> _backgroundRestorer;
        fheroes2::Button _buttonOk;
        fheroes2::Button _buttonCancel;
    };

    Monster selectMonster( const int monsterId );

    int selectHeroes( const int heroId = Heroes::UNKNOWN );

    Artifact selectArtifact( const int artifactId, const bool isForVictoryConditions );

    Spell selectSpell( const int spellId, const bool includeRandomSpells );

    int32_t selectKingdomCastle( const Kingdom & kingdom, const bool notOccupiedByHero, std::string title, std::string description = {},
                                 int32_t castlePositionIndex = -1 );

    Skill::Secondary selectSecondarySkill( const Heroes & hero, const int skillId = Skill::Secondary::UNKNOWN );

    // These functions should be called only from the Editor as they rely on Maps::ObjectInfo structures that are not the same as in-game items.
    int selectHeroType( const int heroType );

    int selectMonsterType( const int monsterType );

    int selectArtifactType( const int artifactType );

    int selectTreasureType( const int resourceType );

    int selectOceanObjectType( const int objectType );

    int selectLandscapeOceanObjectType( const int objectType );

    void selectTownType( int & type, int & color );

    int selectDwellingType( const int dwellingType );

    int selectLandscapeMiscellaneousObjectType( const int objectType );

    void selectMineType( int32_t & type, int32_t & color );

    int selectMountainType( const int mountainType );

    int selectRockType( const int rockType );

    int selectTreeType( const int treeType );

    int selectPowerUpObjectType( const int powerUpObjectType );

    int selectAdventureMiscellaneousObjectType( const int objectType );
}

#endif
