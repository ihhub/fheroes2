/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
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
#include <utility>
#include <vector>

#include "math_base.h"
#include "spell.h"
#include "spell_storage.h"

namespace fheroes2
{
    class Image;

    void renderMageGuildBuilding( const int raceType, const int guildLevel, const Point & offset );

    class SpellsInOneRow final
    {
    public:
        explicit SpellsInOneRow( SpellStorage spells )
            : _spells( std::move( spells ) )
        {
            // Do nothing.
        }

        void setPosition( const Point & offset );

        void redraw( Image & output );
        void redrawCurrentSpell( Image & output );

        bool queueEventProcessing( const bool isEditor );

        // Get spell that is currently selected by pointing mouse cursor over its scroll.
        Spell getCurrentSpell() const;
        // Change the spell under the mouse pointer to the given one.
        void setCurrentSpell( const Spell spell );

        // Returns `true` if spell is found. The first found spell index is made current.
        // If spell is not found the current index is reset and method returns `false`.
        bool checkSpellAndMakeItCurrent( const Spell spellToFind );

        const SpellStorage & getSpells() const
        {
            return _spells;
        }

    private:
        std::vector<Rect> _coords;
        SpellStorage _spells;
        int32_t _currentSpellIndex{ -1 };
    };
}
