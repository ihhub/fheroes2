/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include "ui_dialog.h"
#include "agg_image.h"
#include "artifact.h"
#include "cursor.h"
#include "dialog.h"
#include "experience.h"
#include "heroes_indicator.h"
#include "icn.h"
#include "localevent.h"
#include "luck.h"
#include "morale.h"
#include "resource.h"
#include "screen.h"
#include "spell.h"
#include "tools.h"
#include "ui_button.h"
#include "ui_text.h"

#include <cassert>
#include <string>

namespace
{
    const int32_t textOffsetY = 10;
    const int32_t elementOffsetX = 10;
}

namespace fheroes2
{
    int showMessage( const TextBase & header, const TextBase & body, const int buttons, const std::vector<const DialogElement *> & elements )
    {
        const bool isProperDialog = ( buttons != 0 );

        // setup cursor
        const CursorRestorer cursorRestorer( isProperDialog, ::Cursor::POINTER );

        const int32_t headerHeight = header.empty() ? 0 : header.height( BOXAREA_WIDTH ) + textOffsetY;

        const int overallTextHeight = textOffsetY + headerHeight + body.height( BOXAREA_WIDTH );

        std::vector<int32_t> rowElementIndex;
        std::vector<int32_t> rowHeight;
        std::vector<size_t> rowId;
        std::vector<int32_t> rowMaxElementWidth;
        std::vector<int32_t> rowElementCount;

        int32_t elementHeight = 0;
        size_t elementId = 0;
        for ( const DialogElement * element : elements ) {
            assert( element != nullptr );

            const int32_t currentElementWidth = element->area().width;
            if ( rowHeight.empty() ) {
                rowElementIndex.emplace_back( 0 );
                rowHeight.emplace_back( element->area().height );
                rowId.emplace_back( elementId );
                rowMaxElementWidth.emplace_back( currentElementWidth );
                rowElementCount.emplace_back( 1 );

                ++elementId;
            }
            else if ( ( std::max( rowMaxElementWidth.back(), currentElementWidth ) + elementOffsetX ) * ( rowElementCount.back() + 1 ) <= BOXAREA_WIDTH ) {
                rowElementIndex.emplace_back( rowElementIndex.back() + 1 );
                rowHeight.back() = std::max( rowHeight.back(), element->area().height );
                rowId.emplace_back( rowId.back() );
                rowMaxElementWidth.back() = std::max( rowMaxElementWidth.back(), currentElementWidth );
                ++rowElementCount.back();
            }
            else {
                elementHeight += textOffsetY;
                elementHeight += rowHeight.back();

                rowElementIndex.emplace_back( 0 );
                rowHeight.emplace_back( element->area().height );
                rowId.emplace_back( elementId );
                rowMaxElementWidth.emplace_back( currentElementWidth );
                rowElementCount.emplace_back( 1 );

                ++elementId;
            }
        }

        if ( !rowHeight.empty() ) {
            // UI elements are offset from the dialog body.
            if ( body.height( BOXAREA_WIDTH ) > 0 ) {
                elementHeight += textOffsetY;
            }
            elementHeight += textOffsetY;
            elementHeight += rowHeight.back();
        }

        Dialog::FrameBox box( overallTextHeight + elementHeight, isProperDialog );
        const Rect & pos = box.GetArea();

        Display & display = Display::instance();
        header.draw( pos.x, pos.y + textOffsetY, BOXAREA_WIDTH, display );
        body.draw( pos.x, pos.y + textOffsetY + headerHeight, BOXAREA_WIDTH, display );

        elementHeight = overallTextHeight + textOffsetY;
        if ( body.height( BOXAREA_WIDTH ) > 0 ) {
            elementHeight += textOffsetY;
        }

        elementId = 0;
        size_t prevRowId = 0;
        int32_t currentRowHeight = 0;

        std::vector<Point> elementOffsets;

        for ( const DialogElement * element : elements ) {
            const size_t currentRowId = rowId[elementId];
            if ( prevRowId != currentRowId ) {
                prevRowId = currentRowId;

                elementHeight += textOffsetY;
                elementHeight += currentRowHeight;
            }

            currentRowHeight = rowHeight[currentRowId];
            const int32_t currentRowElementIndex = rowElementIndex[elementId];
            const int32_t currentRowElementCount = rowElementCount[currentRowId];
            const int32_t currentRowMaxElementWidth = rowMaxElementWidth[currentRowId];

            const int32_t emptyWidth = BOXAREA_WIDTH - currentRowElementCount * currentRowMaxElementWidth;
            const int32_t offsetBetweenElements = emptyWidth / ( currentRowElementCount + 1 );

            const int32_t widthOffset = offsetBetweenElements + currentRowElementIndex * ( currentRowMaxElementWidth  + offsetBetweenElements );
            elementOffsets.emplace_back( pos.x + widthOffset + ( currentRowMaxElementWidth - element->area().width ) / 2,
                                         pos.y + elementHeight + currentRowHeight - element->area().height );

            element->draw( display, elementOffsets.back() );

            ++elementId;
        }

        ButtonGroup group( pos, buttons );
        group.draw();

        display.render();

        int result = Dialog::ZERO;
        LocalEvent & le = LocalEvent::Get();

        while ( result == Dialog::ZERO && le.HandleEvents() ) {
            if ( !buttons && !le.MousePressRight() ) {
                break;
            }

            if ( isProperDialog ) {
                elementId = 0;
                for ( const DialogElement * element : elements ) {
                    element->processEvents( elementOffsets[elementId] );
                    ++elementId;
                }
            }

            result = group.processEvents();
        }

        return result;
    }

    ArtifactDialogElement::ArtifactDialogElement( const Artifact & artifact )
        : _artifact( artifact )
    {
        assert( artifact.isValid() );

        const Sprite & frame = AGG::GetICN( ICN::RESOURCE, 7 );
        _area = { frame.width(), frame.height() };
    }

    void ArtifactDialogElement::draw( Image & output, const Point & offset ) const
    {
        const Sprite & frame = AGG::GetICN( ICN::RESOURCE, 7 );
        Blit( frame, 0, 0, output, offset.x, offset.y, frame.width(), frame.height() );

        const Sprite & artifact = AGG::GetICN( ICN::ARTIFACT, _artifact.IndexSprite64() );
        Blit( artifact, 0, 0, output, offset.x + 5, offset.y + 5, artifact.width(), artifact.height() );
    }

    void ArtifactDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().MousePressRight( { offset.x, offset.y, _area.width, _area.height } ) ) {
            const Text header( _artifact.GetName(), { FontSize::NORMAL, FontColor::YELLOW } );
            const Text description( _artifact.GetDescription(), { FontSize::NORMAL, FontColor::WHITE } );

            // Make sure you never pass any buttons here to avoid call stack overflow!
            showMessage( header, description, 0, { this } );
        }
    }

    ResourceDialogElement::ResourceDialogElement( const int32_t resourceType, const int32_t quantity )
        : _resourceType( resourceType )
        , _quantity( quantity )
        , _icnIndex( Resource::getIconIcnIndex( resourceType ) )
    {
        const Text quantityText( std::to_string( _quantity ), { FontSize::SMALL, FontColor::WHITE } );

        const Sprite & icn = AGG::GetICN( ICN::RESOURCE, _icnIndex );
        _area = { std::max( icn.width(), quantityText.width() ), icn.height() + TEXT_OFFSET + quantityText.height() };
    }

    void ResourceDialogElement::draw( Image & output, const Point & offset ) const
    {
        const Sprite & icn = AGG::GetICN( ICN::RESOURCE, _icnIndex );
        const Text quantityText( std::to_string( _quantity ), { FontSize::SMALL, FontColor::WHITE } );

        const int32_t maxWidth = std::max( icn.width(), quantityText.width() );

        Blit( icn, 0, 0, output, offset.x + ( maxWidth - icn.width() ) / 2, offset.y, icn.width(), icn.height() );

        quantityText.draw( offset.x + ( maxWidth - quantityText.width() ) / 2, offset.y + icn.height() + TEXT_OFFSET, output );
    }

    void ResourceDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().MousePressRight( { offset.x, offset.y, _area.width, _area.height } ) ) {
            const Text header( Resource::String( _resourceType ), { FontSize::NORMAL, FontColor::YELLOW } );
            const Text description( Resource::getDescription(), { FontSize::NORMAL, FontColor::WHITE } );

            // Make sure you never pass any buttons here to avoid call stack overflow!
            showMessage( header, description, 0 );
        }
    }

    std::vector<ResourceDialogElement> getResourceDialogElements( const Funds & funds )
    {
        std::vector<ResourceDialogElement> elements;

        if ( funds.gold > 0 ) {
            elements.emplace_back( Resource::GOLD, funds.gold );
        }
        if ( funds.wood > 0 ) {
            elements.emplace_back( Resource::WOOD, funds.wood );
        }
        if ( funds.mercury > 0 ) {
            elements.emplace_back( Resource::MERCURY, funds.mercury );
        }
        if ( funds.ore > 0 ) {
            elements.emplace_back( Resource::ORE, funds.ore );
        }
        if ( funds.sulfur > 0 ) {
            elements.emplace_back( Resource::SULFUR, funds.sulfur );
        }
        if ( funds.crystal > 0 ) {
            elements.emplace_back( Resource::CRYSTAL, funds.crystal );
        }
        if ( funds.gems > 0 ) {
            elements.emplace_back( Resource::GEMS, funds.gems );
        }

        return elements;
    }

    SpellDialogElement::SpellDialogElement( const Spell & spell )
        : _spell( spell )
    {
        assert( spell.isValid() );

        const Text spellNameText( std::string( _spell.GetName() ) + " [" + std::to_string( _spell.SpellPoint( nullptr ) ) + "]", { FontSize::SMALL, FontColor::WHITE } );

        const Sprite & icn = AGG::GetICN( ICN::SPELLS, _spell.IndexSprite() );
        _area = { std::max( icn.width(), spellNameText.width() ), icn.height() + TEXT_OFFSET + spellNameText.height() };
    }

    void SpellDialogElement::draw( Image & output, const Point & offset ) const
    {
        const Text spellNameText( std::string( _spell.GetName() ) + " [" + std::to_string( _spell.SpellPoint( nullptr ) ) + "]", { FontSize::SMALL, FontColor::WHITE } );
        const Sprite & icn = AGG::GetICN( ICN::SPELLS, _spell.IndexSprite() );

        const int32_t maxWidth = std::max( icn.width(), spellNameText.width() );

        Blit( icn, 0, 0, output, offset.x + ( maxWidth - icn.width() ) / 2, offset.y, icn.width(), icn.height() );

        spellNameText.draw( offset.x + ( maxWidth - spellNameText.width() ) / 2, offset.y + icn.height() + TEXT_OFFSET, output );
    }

    void SpellDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().MousePressRight( { offset.x, offset.y, _area.width, _area.height } ) ) {
            const Text header( _spell.GetName(), { FontSize::NORMAL, FontColor::YELLOW } );
            const Text description( _spell.GetDescription(), { FontSize::NORMAL, FontColor::WHITE } );

            // Make sure you never pass any buttons here to avoid call stack overflow!
            showMessage( header, description, 0, { this } );
        }
    }

    LuckDialogElement::LuckDialogElement( const bool goodLuck )
        : _goodLuck( goodLuck )
    {
        const fheroes2::Sprite & icn = fheroes2::AGG::GetICN( ICN::EXPMRL, ( _goodLuck ? 0 : 1 ) );
        _area = { icn.width(), icn.height() };
    }

    void LuckDialogElement::draw( Image & output, const Point & offset ) const
    {
        const fheroes2::Sprite & icn = fheroes2::AGG::GetICN( ICN::EXPMRL, ( _goodLuck ? 0 : 1 ) );
        Blit( icn, 0, 0, output, offset.x, offset.y, icn.width(), icn.height() );
    }

    void LuckDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().MousePressRight( { offset.x, offset.y, _area.width, _area.height } ) ) {
            const int luckType = _goodLuck ? Luck::GOOD : Luck::BAD;

            const Text header( LuckString( luckType ), { FontSize::NORMAL, FontColor::YELLOW } );
            const Text description( Luck::Description( luckType ), { FontSize::NORMAL, FontColor::WHITE } );

            // Make sure you never pass any buttons here to avoid call stack overflow!
            showMessage( header, description, 0, { this } );
        }
    }

    MoraleDialogElement::MoraleDialogElement( const bool goodMorale )
        : _goodMorale( goodMorale )
    {
        const fheroes2::Sprite & icn = fheroes2::AGG::GetICN( ICN::EXPMRL, ( _goodMorale ? 2 : 3 ) );
        _area = { icn.width(), icn.height() };
    }

    void MoraleDialogElement::draw( Image & output, const Point & offset ) const
    {
        const fheroes2::Sprite & icn = fheroes2::AGG::GetICN( ICN::EXPMRL, ( _goodMorale ? 2 : 3 ) );
        Blit( icn, 0, 0, output, offset.x, offset.y, icn.width(), icn.height() );
    }

    void MoraleDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().MousePressRight( { offset.x, offset.y, _area.width, _area.height } ) ) {
            const int moraleType = _goodMorale ? Morale::GOOD : Morale::POOR;

            const Text header( MoraleString( moraleType ), { FontSize::NORMAL, FontColor::YELLOW } );
            const Text description( Morale::Description( moraleType ), { FontSize::NORMAL, FontColor::WHITE } );

            // Make sure you never pass any buttons here to avoid call stack overflow!
            showMessage( header, description, 0, { this } );
        }
    }

    ExperienceDialogElement::ExperienceDialogElement( const int32_t experience )
        : _experience( experience )
    {
        const Text experienceText( std::to_string( _experience ), { FontSize::SMALL, FontColor::WHITE } );

        const Sprite & icn = AGG::GetICN( ICN::EXPMRL, 4 );
        if ( experience != 0 ) {
            _area = { std::max( icn.width(), experienceText.width() ), icn.height() + TEXT_OFFSET + experienceText.height() };
        }
        else {
            _area = { icn.width(), icn.height() };
        }
    }

    void ExperienceDialogElement::draw( Image & output, const Point & offset ) const
    {
        const Sprite & icn = AGG::GetICN( ICN::EXPMRL, 4 );

        if ( _experience != 0 ) {
            const Text experienceText( std::to_string( _experience ), { FontSize::SMALL, FontColor::WHITE } );
            const int32_t maxWidth = std::max( icn.width(), experienceText.width() );

            Blit( icn, 0, 0, output, offset.x + ( maxWidth - icn.width() ) / 2, offset.y, icn.width(), icn.height() );

            experienceText.draw( offset.x + ( maxWidth - experienceText.width() ) / 2, offset.y + icn.height() + TEXT_OFFSET, output );
        }
        else {
            Blit( icn, 0, 0, output, offset.x, offset.y, icn.width(), icn.height() );
        }
    }

    void ExperienceDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().MousePressRight( { offset.x, offset.y, _area.width, _area.height } ) ) {
            const Text header( getExperienceName(), { FontSize::NORMAL, FontColor::YELLOW } );
            const Text description( getExperienceDescription(), { FontSize::NORMAL, FontColor::WHITE } );

            // Make sure you never pass any buttons here to avoid call stack overflow!
            showMessage( header, description, 0, { this } );
        }
    }
}
