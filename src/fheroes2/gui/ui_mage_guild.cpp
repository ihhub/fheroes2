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

#include "ui_mage_guild.h"

#include <cassert>
#include <cstddef>
#include <string>

#include "agg_image.h"
#include "dialog.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_tools.h"
#include "pal.h"
#include "race.h"
#include "screen.h"
#include "spell.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace fheroes2
{
    void renderMageGuildBuilding( const int raceType, const int guildLevel, const Point & offset )
    {
        int guildIcn = ICN::UNKNOWN;
        switch ( raceType ) {
        case Race::KNGT:
            guildIcn = ICN::MAGEGLDK;
            break;
        case Race::BARB:
            guildIcn = ICN::MAGEGLDB;
            break;
        case Race::SORC:
            guildIcn = ICN::MAGEGLDS;
            break;
        case Race::WRLK:
            guildIcn = ICN::MAGEGLDW;
            break;
        case Race::RAND:
        case Race::WZRD:
            guildIcn = ICN::MAGEGLDZ;
            break;
        case Race::NECR:
            guildIcn = ICN::MAGEGLDN;
            break;
        default:
            assert( 0 );
            break;
        }

        assert( guildLevel >= 1 && guildLevel <= 5 );
        const Sprite & sprite = AGG::GetICN( guildIcn, guildLevel - 1 );

        const Rect area = GetActiveROI( sprite );

        Point inPos( 0, 0 );
        Point outPos( offset.x + 100 - area.x - area.width / 2, offset.y + 290 - sprite.height() );
        Size inSize( sprite.width(), sprite.height() );

        auto & display = Display::instance();

        if ( FitToRoi( sprite, inPos, display, outPos, inSize, { offset.x, offset.y, 200, Display::DEFAULT_HEIGHT } ) ) {
            if ( raceType == Race::RAND ) {
                // For the random race we use Wizard's Mage Guild in purple palette because Wizard's castle has Library
                // and random Race can potentially have Library.
                Sprite guildSprite = sprite;
                ApplyPalette( guildSprite, PAL::GetPalette( PAL::PaletteType::PURPLE ) );
                Blit( guildSprite, inPos, display, outPos, inSize );
            }
            else {
                Blit( sprite, inPos, display, outPos, inSize );
            }
        }
    }

    void SpellsInOneRow::setPosition( const Point & offset )
    {
        _coords.clear();

        const Sprite & spellScrollOpened = AGG::GetICN( ICN::TOWNWIND, 0 );
        const Sprite & spellScroll = AGG::GetICN( ICN::TOWNWIND, 1 );

        for ( size_t i = 0; i < _spells.size(); ++i ) {
            const Spell & spell = _spells[i];

            if ( spell == Spell::NONE ) {
                _coords.emplace_back( offset.x + static_cast<int32_t>( i ) * 110 - spellScroll.width() / 2, offset.y + 7, spellScroll.width(), spellScroll.height() );
            }
            else {
                _coords.emplace_back( offset.x + static_cast<int32_t>( i ) * 110 - spellScrollOpened.width() / 2, offset.y, spellScrollOpened.width(),
                                      spellScrollOpened.height() );
            }
        }
    }

    void SpellsInOneRow::redraw( Image & output )
    {
        if ( _spells.empty() || _coords.size() != _spells.size() ) {
            return;
        }

        const Sprite & spellScrollOpened = AGG::GetICN( ICN::TOWNWIND, 0 );
        const Sprite & spellScroll = AGG::GetICN( ICN::TOWNWIND, 1 );

        for ( size_t i = 0; i < _spells.size(); ++i ) {
            const Spell & spell = _spells[i];
            const Rect & dst = _coords[i];

            if ( spell == Spell::NONE ) {
                // Draw folded scroll when there is no spell.
                Blit( spellScroll, output, dst.x, dst.y );
            }
            else {
                // Draw scroll with a spell over it.
                Blit( spellScrollOpened, output, dst.x, dst.y );

                const Sprite & icon = AGG::GetICN( ICN::SPELLS, spell.IndexSprite() );
                Blit( icon, output, dst.x + 3 + ( dst.width - icon.width() ) / 2, dst.y + 31 - icon.height() / 2 );

                const Text text( spell.GetName(), FontType::smallWhite() );
                text.draw( dst.x + 18, dst.y + 57, 78, output );
            }
        }
    }

    void SpellsInOneRow::redrawCurrentSpell( Image & output )
    {
        if ( _currentSpellIndex < 0 || static_cast<size_t>( _currentSpellIndex ) >= _spells.size() || _coords.size() != _spells.size() ) {
            return;
        }

        const Spell & spell = _spells[_currentSpellIndex];
        const Rect & dst = _coords[_currentSpellIndex];

        if ( spell == Spell::NONE ) {
            // Draw folded scroll when there is no spell.
            const Sprite & spellScroll = AGG::GetICN( ICN::TOWNWIND, 1 );
            Blit( spellScroll, output, dst.x, dst.y );
        }
        else {
            // Draw scroll with a spell over it.
            const Sprite & spellScrollOpened = AGG::GetICN( ICN::TOWNWIND, 0 );
            Blit( spellScrollOpened, output, dst.x, dst.y );

            const Sprite & icon = AGG::GetICN( ICN::SPELLS, spell.IndexSprite() );
            Blit( icon, output, dst.x + 3 + ( dst.width - icon.width() ) / 2, dst.y + 31 - icon.height() / 2 );

            const Text text( spell.GetName(), FontType::smallWhite() );
            text.draw( dst.x + 18, dst.y + 57, 78, output );
        }
    }

    bool SpellsInOneRow::queueEventProcessing( const bool isEditor )
    {
        LocalEvent & le = LocalEvent::Get();

        _currentSpellIndex = GetRectIndex( _coords, le.getMouseCursorPos() );

        if ( _currentSpellIndex < 0 ) {
            return false;
        }

        const bool rightMouseButtonPressed = le.isMouseRightButtonPressed();
        if ( rightMouseButtonPressed || ( !isEditor && le.MouseClickLeft() ) ) {
            const Spell & spell = _spells[_currentSpellIndex];

            if ( spell != Spell::NONE ) {
                SpellDialogElement( spell, nullptr ).showPopup( rightMouseButtonPressed ? Dialog::ZERO : Dialog::OK );
            }
        }

        return true;
    }

    Spell SpellsInOneRow::getCurrentSpell() const
    {
        if ( _currentSpellIndex < 0 ) {
            return Spell::NONE;
        }

        return _spells[_currentSpellIndex];
    }

    void SpellsInOneRow::setCurrentSpell( const Spell spell )
    {
        if ( _currentSpellIndex < 0 ) {
            return;
        }

        // Allow RANDOM spell IDs to appear multiple times in the same row.
        if ( spell < Spell::RANDOM && spell > Spell::RANDOM5 && _spells.isPresentSpell( spell ) ) {
            return;
        }

        _spells[_currentSpellIndex] = spell;
    }

    bool SpellsInOneRow::checkSpellAndMakeItCurrent( const Spell spellToFind )
    {
        for ( size_t i = 0; i < _spells.size(); ++i ) {
            if ( _spells[i] == spellToFind ) {
                _currentSpellIndex = static_cast<int32_t>( i );
                return true;
            }
        }

        _currentSpellIndex = -1;
        return false;
    }
}
