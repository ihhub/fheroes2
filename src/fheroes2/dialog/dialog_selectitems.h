/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "artifact.h"
#include "color.h"
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

namespace fheroes2
{
    struct LocalizedString;
    class TextBase;
}

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

        // An image with text should have offset of 10 pixels from all left and right edges.
        void renderItem( const fheroes2::Sprite & itemSprite, std::string itemText, const fheroes2::Point & destination, const int32_t middleImageOffsetX,
                         const int32_t textOffsetX, const int32_t itemOffsetY, const bool current ) const;

        void renderItem( const fheroes2::Sprite & itemSprite, std::vector<fheroes2::LocalizedString> itemText, const fheroes2::Point & destination,
                         const int32_t middleImageOffsetX, const int32_t textOffsetX, const int32_t itemOffsetY, const bool current ) const;

        int32_t selectItemsEventProcessing();

        fheroes2::Rect getBackgroundArea() const;

    protected:
        void setButtonOkayStatus( const bool enabled )
        {
            if ( enabled == _buttonOk.isEnabled() ) {
                return;
            }

            if ( enabled ) {
                _buttonOk.enable();
            }
            else {
                _buttonOk.disable();
            }

            _buttonOk.draw();
        }

        void enableToggleButtons();

        virtual void onToggleOn()
        {
            // Do nothing.
        }

        virtual void onToggleOff()
        {
            // Do nothing.
        }

    private:
        bool _isDoubleClicked{ false };
        std::unique_ptr<fheroes2::StandardWindow> _window;
        std::unique_ptr<fheroes2::ImageRestorer> _backgroundRestorer;
        fheroes2::Button _buttonOk;
        fheroes2::Button _buttonCancel;
        fheroes2::Button _buttonToggleOn;
        fheroes2::Button _buttonToggleOff;

        virtual bool isDoubleClicked()
        {
            return _isDoubleClicked;
        }

        void renderText( fheroes2::TextBase & text, const fheroes2::Point & destination, const int32_t textOffsetX, const int32_t itemOffsetY ) const;
    };

    Monster selectMonster( const int monsterId );

    int selectHeroes( const int heroId = Heroes::UNKNOWN );

    Artifact selectArtifact( const int artifactId, const bool isForVictoryConditions );

    Spell selectSpell( const int spellId, const bool includeRandomSpells, const std::set<int32_t> & excludeSpellsList = {}, const int32_t spellsLevel = -1 );

    int32_t selectKingdomCastle( const Kingdom & kingdom, const bool notOccupiedByHero, std::string title, std::string description = {},
                                 int32_t castlePositionIndex = -1 );

    Skill::Secondary selectSecondarySkill( const Heroes & hero, const int skillId = Skill::Secondary::UNKNOWN );

    void multiSelectMonsters( std::vector<int> allowed, std::vector<int> & selected );

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

    int32_t selectMineType( const int32_t type );

    int selectMountainType( const int mountainType );

    int selectRockType( const int rockType );

    int selectTreeType( const int treeType );

    int selectPowerUpObjectType( const int powerUpObjectType );

    int selectAdventureMiscellaneousObjectType( const int objectType );

    PlayerColor selectPlayerColor( const PlayerColor color, const uint8_t availableColors );
}
