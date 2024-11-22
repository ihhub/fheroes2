/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ostream>
#include <string>
#include <utility>

#include "agg_image.h"
#include "army_troop.h"
#include "cursor.h"
#include "dialog.h"
#include "experience.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "heroes_indicator.h"
#include "icn.h"
#include "localevent.h"
#include "logging.h"
#include "luck.h"
#include "morale.h"
#include "resource.h"
#include "screen.h"
#include "spell_info.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_monster.h"
#include "ui_text.h"

class HeroBase;

namespace
{
    const int32_t textOffsetY = 10;

    const int32_t elementOffsetX = 10;

    const int32_t textOffsetFromElement = 2;

    const int32_t defaultElementPopupButtons = Dialog::ZERO;

    void outputInTextSupportMode( const fheroes2::TextBase & header, const fheroes2::TextBase & body, const int buttonTypes )
    {
        START_TEXT_SUPPORT_MODE

        COUT( header.text() )
        COUT( '\n' )
        COUT( body.text() )

        if ( buttonTypes & Dialog::YES ) {
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_OKAY ) << " to choose YES." )
        }
        if ( buttonTypes & Dialog::NO ) {
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_CANCEL ) << " to choose NO." )
        }
        if ( buttonTypes & Dialog::OK ) {
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_OKAY ) << " to choose OK." )
        }
        if ( buttonTypes & Dialog::CANCEL ) {
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_CANCEL ) << " to choose CANCEL." )
        }
    }
}

namespace fheroes2
{
    int showMessage( const TextBase & header, const TextBase & body, const int buttons, const std::vector<const DialogElement *> & elements /* = {} */ )
    {
        outputInTextSupportMode( header, body, buttons );

        const bool isProperDialog = ( buttons != 0 );

        const int cusorTheme = isProperDialog ? ::Cursor::POINTER : ::Cursor::Get().Themes();

        // setup cursor
        const CursorRestorer cursorRestorer( isProperDialog, cusorTheme );

        const int32_t headerHeight = header.empty() ? 0 : header.height( fheroes2::boxAreaWidthPx ) + textOffsetY;

        int overallTextHeight = headerHeight;

        const int32_t bodyTextHeight = body.height( fheroes2::boxAreaWidthPx );
        if ( bodyTextHeight > 0 ) {
            overallTextHeight += bodyTextHeight + textOffsetY;
        }

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
            else if ( ( std::max( rowMaxElementWidth.back(), currentElementWidth ) + elementOffsetX ) * ( rowElementCount.back() + 1 ) <= fheroes2::boxAreaWidthPx ) {
                rowElementIndex.emplace_back( rowElementIndex.back() + 1 );
                rowHeight.back() = std::max( rowHeight.back(), element->area().height );

                // We cannot use back() to insert it into the same container as it will be resized upon insertion.
                const size_t lastRoiId = rowId.back();
                rowId.emplace_back( lastRoiId );

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
            if ( bodyTextHeight > 0 ) {
                elementHeight += textOffsetY;
            }
            elementHeight += textOffsetY;
            elementHeight += rowHeight.back();
        }

        const Dialog::FrameBox box( overallTextHeight + elementHeight, isProperDialog );
        const Rect & pos = box.GetArea();

        Display & display = Display::instance();
        header.draw( pos.x, pos.y + textOffsetY, fheroes2::boxAreaWidthPx, display );
        body.draw( pos.x, pos.y + textOffsetY + headerHeight, fheroes2::boxAreaWidthPx, display );

        elementHeight = overallTextHeight + textOffsetY;
        if ( bodyTextHeight > 0 ) {
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

            const int32_t emptyWidth = fheroes2::boxAreaWidthPx - currentRowElementCount * currentRowMaxElementWidth;
            const int32_t offsetBetweenElements = emptyWidth / ( currentRowElementCount + 1 );

            const int32_t widthOffset = offsetBetweenElements + currentRowElementIndex * ( currentRowMaxElementWidth + offsetBetweenElements );
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

        bool delayInEventHandling = true;

        while ( result == Dialog::ZERO && le.HandleEvents( delayInEventHandling ) ) {
            if ( isProperDialog ) {
                elementId = 0;
                for ( const DialogElement * element : elements ) {
                    element->processEvents( elementOffsets[elementId] );
                    ++elementId;
                }
            }
            else if ( !le.isMouseRightButtonPressed() ) {
                break;
            }

            result = group.processEvents();

            delayInEventHandling = true;
            elementId = 0;
            for ( const DialogElement * element : elements ) {
                if ( element->update( display, elementOffsets[elementId] ) ) {
                    delayInEventHandling = false;
                }
                ++elementId;
            }

            if ( !delayInEventHandling ) {
                display.render( pos );
            }
        }

        return result;
    }

    int showStandardTextMessage( std::string headerText, std::string messageBody, const int buttons, const std::vector<const DialogElement *> & elements /* = {} */ )
    {
        const Text header( std::move( headerText ), FontType::normalYellow() );
        const Text body( std::move( messageBody ), FontType::normalWhite() );
        return showMessage( header, body, buttons, elements );
    }

    TextDialogElement::TextDialogElement( const std::shared_ptr<TextBase> & text )
        : _text( text )
    {
        // Text always occupies the whole width of the dialog.
        _area = { fheroes2::boxAreaWidthPx, _text->height( fheroes2::boxAreaWidthPx ) };
    }

    void TextDialogElement::draw( Image & output, const Point & offset ) const
    {
        _text->draw( offset.x, offset.y, fheroes2::boxAreaWidthPx, output );
    }

    void TextDialogElement::processEvents( const Point & /* offset */ ) const
    {
        // No events processed here.
    }

    void TextDialogElement::showPopup( const int /* buttons */ ) const
    {
        assert( 0 );
    }

    CustomImageDialogElement::CustomImageDialogElement( const Image & image )
        : _image( image )
    {
        _area = { _image.width(), _image.height() };
    }

    CustomImageDialogElement::CustomImageDialogElement( Image && image )
        : _image( std::move( image ) )
    {
        _area = { _image.width(), _image.height() };
    }

    void CustomImageDialogElement::draw( Image & output, const Point & offset ) const
    {
        Blit( _image, 0, 0, output, offset.x, offset.y, _image.width(), _image.height() );
    }

    void CustomImageDialogElement::processEvents( const Point & /* offset */ ) const
    {
        // No events processed here.
    }

    void CustomImageDialogElement::showPopup( const int /* buttons */ ) const
    {
        assert( 0 );
    }

    ArtifactDialogElement::ArtifactDialogElement( const Artifact & artifact )
        : _artifact( artifact )
    {
        assert( artifact.GetID() == Artifact::EDITOR_ANY_ULTIMATE_ARTIFACT || artifact.isValid() );

        const Sprite & frame = AGG::GetICN( ICN::RESOURCE, 7 );
        _area = { frame.width(), frame.height() };
    }

    void ArtifactDialogElement::draw( Image & output, const Point & offset ) const
    {
        const Sprite & frame = AGG::GetICN( ICN::RESOURCE, 7 );
        Blit( frame, 0, 0, output, offset.x, offset.y, frame.width(), frame.height() );

        const Sprite & artifact = AGG::GetICN( ICN::ARTIFACT, _artifact.IndexSprite64() );
        Blit( artifact, output, offset.x + 6, offset.y + 6 );
    }

    void ArtifactDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().isMouseRightButtonPressedInArea( { offset.x, offset.y, _area.width, _area.height } ) ) {
            // Make sure you never pass any buttons here to avoid call stack overflow!
            showPopup( defaultElementPopupButtons );
        }
    }

    void ArtifactDialogElement::showPopup( const int buttons ) const
    {
        showStandardTextMessage( _artifact.GetName(), _artifact.GetDescription(), buttons, { this } );
    }

    ResourceDialogElement::ResourceDialogElement( const int32_t resourceType, std::string text )
        : _resourceType( resourceType )
        , _icnIndex( Resource::getIconIcnIndex( resourceType ) )
        , _text( std::move( text ) )
    {
        const Text quantityText( _text, FontType::smallWhite() );

        const Sprite & icn = AGG::GetICN( ICN::RESOURCE, _icnIndex );
        _area = { std::max( icn.width(), quantityText.width() ), icn.height() + textOffsetFromElement + quantityText.height() };
    }

    void ResourceDialogElement::draw( Image & output, const Point & offset ) const
    {
        const Sprite & icn = AGG::GetICN( ICN::RESOURCE, _icnIndex );
        const Text quantityText( _text, FontType::smallWhite() );

        const int32_t maxWidth = std::max( icn.width(), quantityText.width() );

        Blit( icn, 0, 0, output, offset.x + ( maxWidth - icn.width() ) / 2, offset.y, icn.width(), icn.height() );

        quantityText.draw( offset.x + ( maxWidth - quantityText.width() ) / 2, offset.y + icn.height() + textOffsetFromElement, output );
    }

    void ResourceDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().isMouseRightButtonPressedInArea( { offset.x, offset.y, _area.width, _area.height } ) ) {
            // Make sure you never pass any buttons here to avoid call stack overflow!
            showPopup( defaultElementPopupButtons );
        }
    }

    void ResourceDialogElement::showPopup( const int buttons ) const
    {
        showStandardTextMessage( Resource::String( _resourceType ), Resource::getDescription(), buttons );
    }

    std::vector<ResourceDialogElement> getResourceDialogElements( const Funds & funds )
    {
        std::vector<ResourceDialogElement> elements;

        if ( funds.wood != 0 ) {
            elements.emplace_back( Resource::WOOD, std::to_string( funds.wood ) );
        }
        if ( funds.mercury != 0 ) {
            elements.emplace_back( Resource::MERCURY, std::to_string( funds.mercury ) );
        }
        if ( funds.ore != 0 ) {
            elements.emplace_back( Resource::ORE, std::to_string( funds.ore ) );
        }
        if ( funds.sulfur != 0 ) {
            elements.emplace_back( Resource::SULFUR, std::to_string( funds.sulfur ) );
        }
        if ( funds.crystal != 0 ) {
            elements.emplace_back( Resource::CRYSTAL, std::to_string( funds.crystal ) );
        }
        if ( funds.gems != 0 ) {
            elements.emplace_back( Resource::GEMS, std::to_string( funds.gems ) );
        }
        if ( funds.gold != 0 ) {
            elements.emplace_back( Resource::GOLD, std::to_string( funds.gold ) );
        }

        return elements;
    }

    int showResourceMessage( const TextBase & header, const TextBase & body, const int buttons, const Funds & funds )
    {
        const std::vector<ResourceDialogElement> elements = getResourceDialogElements( funds );

        std::vector<const DialogElement *> uiElements;
        uiElements.reserve( elements.size() );
        for ( const ResourceDialogElement & element : elements ) {
            uiElements.emplace_back( &element );
        }

        return showMessage( header, body, buttons, uiElements );
    }

    SpellDialogElement::SpellDialogElement( const Spell & spell, const HeroBase * hero )
        : _spell( spell )
        , _hero( hero )
    {
        assert( spell.isValid() );

        std::string spellText( _spell.GetName() );
        const uint32_t spellPoints = _spell.spellPoints( nullptr );
        if ( spellPoints > 0 ) {
            spellText += " [" + std::to_string( spellPoints ) + ']';
        }

        const Text spellNameText( std::move( spellText ), FontType::smallWhite() );

        const Sprite & icn = AGG::GetICN( ICN::SPELLS, _spell.IndexSprite() );
        _area = { std::max( icn.width(), spellNameText.width() ), icn.height() + textOffsetFromElement + spellNameText.height() };
    }

    void SpellDialogElement::draw( Image & output, const Point & offset ) const
    {
        std::string spellText( _spell.GetName() );
        const uint32_t spellPoints = _spell.spellPoints( nullptr );
        if ( spellPoints > 0 ) {
            spellText += " [" + std::to_string( spellPoints ) + ']';
        }

        const Text spellNameText( std::move( spellText ), FontType::smallWhite() );
        const Sprite & icn = AGG::GetICN( ICN::SPELLS, _spell.IndexSprite() );

        const int32_t maxWidth = std::max( icn.width(), spellNameText.width() );

        Blit( icn, 0, 0, output, offset.x + ( maxWidth - icn.width() ) / 2, offset.y, icn.width(), icn.height() );

        spellNameText.draw( offset.x + ( maxWidth - spellNameText.width() ) / 2, offset.y + icn.height() + textOffsetFromElement, output );
    }

    void SpellDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().isMouseRightButtonPressedInArea( { offset.x, offset.y, _area.width, _area.height } ) ) {
            // Make sure you never pass any buttons here to avoid call stack overflow!
            showPopup( defaultElementPopupButtons );
        }
    }

    void SpellDialogElement::showPopup( const int buttons ) const
    {
        showStandardTextMessage( _spell.GetName(), getSpellDescription( _spell, _hero ), buttons, { this } );
    }

    LuckDialogElement::LuckDialogElement( const bool goodLuck )
        : _goodLuck( goodLuck )
    {
        const Sprite & icn = AGG::GetICN( ICN::EXPMRL, ( _goodLuck ? 0 : 1 ) );
        _area = { icn.width(), icn.height() };
    }

    void LuckDialogElement::draw( Image & output, const Point & offset ) const
    {
        const Sprite & icn = AGG::GetICN( ICN::EXPMRL, ( _goodLuck ? 0 : 1 ) );
        Blit( icn, 0, 0, output, offset.x, offset.y, icn.width(), icn.height() );
    }

    void LuckDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().isMouseRightButtonPressedInArea( { offset.x, offset.y, _area.width, _area.height } ) ) {
            // Make sure you never pass any buttons here to avoid call stack overflow!
            showPopup( defaultElementPopupButtons );
        }
    }

    void LuckDialogElement::showPopup( const int buttons ) const
    {
        const int luckType = _goodLuck ? Luck::GOOD : Luck::BAD;

        showStandardTextMessage( LuckString( luckType ), Luck::Description( luckType ), buttons, { this } );
    }

    MoraleDialogElement::MoraleDialogElement( const bool goodMorale )
        : _goodMorale( goodMorale )
    {
        const Sprite & icn = AGG::GetICN( ICN::EXPMRL, ( _goodMorale ? 2 : 3 ) );
        _area = { icn.width(), icn.height() };
    }

    void MoraleDialogElement::draw( Image & output, const Point & offset ) const
    {
        const Sprite & icn = AGG::GetICN( ICN::EXPMRL, ( _goodMorale ? 2 : 3 ) );
        Blit( icn, 0, 0, output, offset.x, offset.y, icn.width(), icn.height() );
    }

    void MoraleDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().isMouseRightButtonPressedInArea( { offset.x, offset.y, _area.width, _area.height } ) ) {
            // Make sure you never pass any buttons here to avoid call stack overflow!
            showPopup( defaultElementPopupButtons );
        }
    }

    void MoraleDialogElement::showPopup( const int buttons ) const
    {
        const int moraleType = _goodMorale ? Morale::GOOD : Morale::POOR;

        showStandardTextMessage( MoraleString( moraleType ), Morale::Description( moraleType ), buttons, { this } );
    }

    ExperienceDialogElement::ExperienceDialogElement( const int32_t experience )
        : _experience( experience )
    {
        const Sprite & icn = AGG::GetICN( ICN::EXPMRL, 4 );
        if ( experience != 0 ) {
            const Text experienceText( std::to_string( _experience ), FontType::smallWhite() );
            _area = { std::max( icn.width(), experienceText.width() ), icn.height() + textOffsetFromElement + experienceText.height() };
        }
        else {
            _area = { icn.width(), icn.height() };
        }
    }

    void ExperienceDialogElement::draw( Image & output, const Point & offset ) const
    {
        const Sprite & icn = AGG::GetICN( ICN::EXPMRL, 4 );

        if ( _experience != 0 ) {
            const Text experienceText( std::to_string( _experience ), FontType::smallWhite() );
            const int32_t maxWidth = std::max( icn.width(), experienceText.width() );

            Blit( icn, 0, 0, output, offset.x + ( maxWidth - icn.width() ) / 2, offset.y, icn.width(), icn.height() );

            experienceText.draw( offset.x + ( maxWidth - experienceText.width() ) / 2, offset.y + icn.height() + textOffsetFromElement, output );
        }
        else {
            Blit( icn, 0, 0, output, offset.x, offset.y, icn.width(), icn.height() );
        }
    }

    void ExperienceDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().isMouseRightButtonPressedInArea( { offset.x, offset.y, _area.width, _area.height } ) ) {
            // Make sure you never pass any buttons here to avoid call stack overflow!
            showPopup( defaultElementPopupButtons );
        }
    }

    void ExperienceDialogElement::showPopup( const int buttons ) const
    {
        const ExperienceDialogElement experienceUI( 0 );

        showStandardTextMessage( getExperienceName(), getExperienceDescription(), buttons, { &experienceUI } );
    }

    PrimarySkillDialogElement::PrimarySkillDialogElement( const int32_t skillType, std::string text )
        : _skillType( skillType )
        , _text( std::move( text ) )
    {
        assert( _skillType >= Skill::Primary::ATTACK && _skillType <= Skill::Primary::KNOWLEDGE );

        const Sprite & background = AGG::GetICN( ICN::PRIMSKIL, 4 );
        _area = { background.width(), background.height() };
    }

    void PrimarySkillDialogElement::draw( Image & output, const Point & offset ) const
    {
        const Sprite & background = AGG::GetICN( ICN::PRIMSKIL, 4 );
        Blit( background, 0, 0, output, offset.x, offset.y, background.width(), background.height() );

        uint32_t icnId = 0;

        switch ( _skillType ) {
        case Skill::Primary::ATTACK:
            icnId = 0;
            break;
        case Skill::Primary::DEFENSE:
            icnId = 1;
            break;
        case Skill::Primary::POWER:
            icnId = 2;
            break;
        case Skill::Primary::KNOWLEDGE:
            icnId = 3;
            break;
        default:
            // Are you sure you are passing the correct Primary Skill type?
            assert( 0 );
            break;
        }

        const Sprite & icn = AGG::GetICN( ICN::PRIMSKIL, icnId );
        Blit( icn, 0, 0, output, offset.x + ( background.width() - icn.width() ) / 2, offset.y + ( background.height() - icn.height() ) / 2, icn.width(), icn.height() );

        const Text skillName( Skill::Primary::String( _skillType ), FontType::smallWhite() );
        skillName.draw( offset.x + ( background.width() - skillName.width() ) / 2, offset.y + 10, output );

        if ( !_text.empty() ) {
            const Text descriptionText( _text, FontType::normalWhite() );
            descriptionText.draw( offset.x + ( background.width() - descriptionText.width() ) / 2, offset.y + 82, output );
        }
    }

    void PrimarySkillDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().isMouseRightButtonPressedInArea( { offset.x, offset.y, _area.width, _area.height } ) ) {
            // Make sure you never pass any buttons here to avoid call stack overflow!
            showPopup( defaultElementPopupButtons );
        }
    }

    void PrimarySkillDialogElement::showPopup( const int buttons ) const
    {
        const PrimarySkillDialogElement elementUI( _skillType, std::string() );

        showStandardTextMessage( Skill::Primary::String( _skillType ), Skill::Primary::StringDescription( _skillType, nullptr ), buttons, { &elementUI } );
    }

    SmallPrimarySkillDialogElement::SmallPrimarySkillDialogElement( const int32_t skillType, std::string text )
        : PrimarySkillDialogElement( skillType, std::move( text ) )
    {
        _area = _iconSize;

        if ( !_text.empty() ) {
            _area.height += Text{ _text, FontType::smallWhite() }.height( _iconSize.width ) + 2;
        }
    }

    void SmallPrimarySkillDialogElement::draw( Image & output, const Point & offset ) const
    {
        const Sprite & originalImage = AGG::GetICN( ICN::SWAPWIN, 0 );

        switch ( _skillType ) {
        case Skill::Primary::ATTACK:
            Copy( originalImage, 216, 51, output, offset.x, offset.y, _iconSize.width, _iconSize.height );
            break;
        case Skill::Primary::DEFENSE:
            Copy( originalImage, 216, 84, output, offset.x, offset.y, _iconSize.width, _iconSize.height );
            break;
        case Skill::Primary::POWER:
            Copy( originalImage, 216, 117, output, offset.x, offset.y, _iconSize.width, _iconSize.height );
            break;
        case Skill::Primary::KNOWLEDGE:
            Copy( originalImage, 216, 150, output, offset.x, offset.y, _iconSize.width, _iconSize.height );
            break;
        default:
            // Are you sure you are passing the correct Primary Skill type?
            assert( 0 );
            break;
        }

        if ( !_text.empty() ) {
            const Text descriptionText( _text, FontType::smallWhite() );
            descriptionText.draw( offset.x, offset.y + _iconSize.height + 2, _iconSize.width, output );
        }
    }

    SecondarySkillDialogElement::SecondarySkillDialogElement( const Skill::Secondary & skill, const Heroes & hero )
        : _skill( skill )
        , _hero( hero )
    {
        const Sprite & background = AGG::GetICN( ICN::SECSKILL, 15 );
        _area = { background.width(), background.height() };
    }

    void SecondarySkillDialogElement::draw( Image & output, const Point & offset ) const
    {
        const Sprite & background = AGG::GetICN( ICN::SECSKILL, 15 );
        Blit( background, 0, 0, output, offset.x, offset.y, background.width(), background.height() );

        const Sprite & icn = AGG::GetICN( ICN::SECSKILL, _skill.GetIndexSprite1() );
        const Rect icnRect( offset.x + ( background.width() - icn.width() ) / 2, offset.y + ( background.height() - icn.height() ) / 2, icn.width(), icn.height() );
        Copy( icn, 0, 0, output, icnRect );

        Text skillName( Skill::Secondary::String( _skill.Skill() ), FontType::smallWhite() );
        skillName.fitToOneRow( icnRect.width );
        skillName.drawInRoi( offset.x + ( background.width() - skillName.width() ) / 2 - 1, offset.y + 8, output, icnRect );

        Text skillDescription( Skill::Level::StringWithBonus( _hero, _skill ), FontType::smallWhite() );
        skillDescription.fitToOneRow( icnRect.width );
        skillDescription.drawInRoi( offset.x + ( background.width() - skillDescription.width() ) / 2 - 1, offset.y + 56, output, icnRect );
    }

    void SecondarySkillDialogElement::processEvents( const Point & offset ) const
    {
        if ( LocalEvent::Get().isMouseRightButtonPressedInArea( { offset.x, offset.y, _area.width, _area.height } ) ) {
            // Make sure you never pass any buttons here to avoid call stack overflow!
            showPopup( defaultElementPopupButtons );
        }
    }

    void SecondarySkillDialogElement::showPopup( const int buttons ) const
    {
        showStandardTextMessage( _skill.GetNameWithBonus( _hero ), _skill.GetDescription( _hero ), buttons, { this } );
    }

    AnimationDialogElement::AnimationDialogElement( const int icnId, std::vector<uint32_t> backgroundIndices, const uint32_t animationIndexOffset, const uint64_t delay )
        : _icnId( icnId )
        , _backgroundIndices( std::move( backgroundIndices ) )
        , _animationIndexOffset( animationIndexOffset )
        , _delay( delay )
        , _currentIndex( 0 )
    {
        assert( !_backgroundIndices.empty() && _delay > 0 );

        for ( const uint32_t index : _backgroundIndices ) {
            const Sprite & image = AGG::GetICN( _icnId, index );
            _area.width = std::max( _area.width, image.width() );
            _area.height = std::max( _area.height, image.height() );

            _internalOffset = { ( _area.width - image.width() ) / 2, ( _area.height - image.height() ) / 2 };
        }
    }

    void AnimationDialogElement::draw( Image & output, const Point & offset ) const
    {
        if ( _currentIndex == 0 ) {
            // Since this is the first time to draw we have to draw the background.
            for ( const uint32_t index : _backgroundIndices ) {
                const Sprite & image = AGG::GetICN( _icnId, index );
                Blit( image, 0, 0, output, offset.x + ( _area.width - image.width() ) / 2, offset.y + ( _area.height - image.height() ) / 2, image.width(),
                      image.height() );
            }
        }

        const uint32_t animationFrameId = ICN::getAnimatedIcnIndex( _icnId, _animationIndexOffset, _currentIndex );
        ++_currentIndex;

        const Sprite & animationImage = AGG::GetICN( _icnId, animationFrameId );

        Blit( animationImage, 0, 0, output, offset.x + _internalOffset.x + animationImage.x(), offset.y + _internalOffset.y + animationImage.y(), animationImage.width(),
              animationImage.height() );
    }

    void AnimationDialogElement::processEvents( const Point & /* offset */ ) const
    {
        // No events processed here.
    }

    void AnimationDialogElement::showPopup( const int /* buttons */ ) const
    {
        assert( 0 );
    }

    bool AnimationDialogElement::update( Image & output, const Point & offset ) const
    {
        if ( Game::validateCustomAnimationDelay( _delay ) ) {
            draw( output, offset );
            return true;
        }

        return false;
    }

    CustomAnimationDialogElement::CustomAnimationDialogElement( const int icnId, Image staticImage, const Point & animationPositionOffset,
                                                                const uint32_t animationIndexOffset, const uint64_t delay )
        : _icnId( icnId )
        , _staticImage( std::move( staticImage ) )
        , _animationPosition( animationPositionOffset )
        , _animationIndexOffset( animationIndexOffset )
        , _delay( delay )
        , _currentIndex( 0 )
    {
        assert( delay > 0 );
        _area = { _staticImage.width(), _staticImage.height() };
    }

    void CustomAnimationDialogElement::draw( Image & output, const Point & offset ) const
    {
        if ( _currentIndex == 0 ) {
            // Since this is the first time to draw we have to draw the background.
            Blit( _staticImage, 0, 0, output, offset.x, offset.y, _staticImage.width(), _staticImage.height() );
        }

        const uint32_t animationFrameId = ICN::getAnimatedIcnIndex( _icnId, _animationIndexOffset, _currentIndex );
        ++_currentIndex;

        const Sprite & animationImage = AGG::GetICN( _icnId, animationFrameId );

        Blit( animationImage, 0, 0, output, offset.x + _animationPosition.x, offset.y + _animationPosition.y, animationImage.width(), animationImage.height() );
    }

    void CustomAnimationDialogElement::processEvents( const Point & /* offset */ ) const
    {
        // No events processed here.
    }

    void CustomAnimationDialogElement::showPopup( const int /* buttons */ ) const
    {
        assert( 0 );
    }

    bool CustomAnimationDialogElement::update( Image & output, const Point & offset ) const
    {
        if ( Game::validateCustomAnimationDelay( _delay ) ) {
            draw( output, offset );
            return true;
        }

        return false;
    }

    MonsterDialogElement::MonsterDialogElement( const Monster & monster )
        : _monster( monster )
    {
        assert( _monster.isValid() );
        const Sprite & sprite = AGG::GetICN( ICN::STRIP, 12 );
        _area = { sprite.width(), sprite.height() };
    }

    void MonsterDialogElement::draw( Image & output, const Point & offset ) const
    {
        Sprite sprite = AGG::GetICN( ICN::STRIP, 12 );
        sprite._disableTransformLayer();
        renderMonsterFrame( _monster, sprite, { 6, 6 } );
        Copy( sprite, 0, 0, output, offset.x, offset.y, sprite.width(), sprite.height() );
    }

    void MonsterDialogElement::processEvents( const Point & offset ) const
    {
        LocalEvent & le = LocalEvent::Get();
        const Rect rect{ offset.x, offset.y, _area.width, _area.height };
        if ( le.isMouseRightButtonPressedInArea( rect ) ) {
            showPopup( defaultElementPopupButtons );
        }
        else if ( le.MouseClickLeft( rect ) ) {
            showPopup( Dialog::BUTTONS );
        }
    }

    void MonsterDialogElement::showPopup( const int buttons ) const
    {
        Dialog::ArmyInfo( Troop{ _monster, 0 }, buttons );
    }

    ValueSelectionDialogElement::ValueSelectionDialogElement( const int32_t minimum, const int32_t maximum, const int32_t current, const int32_t step,
                                                              const Point & offset )
        : _minimum( std::min( maximum, minimum ) )
        , _maximum( std::max( maximum, minimum ) )
        , _step( std::max( step, 1 ) )
        , _value( current )
        , _timedButtonUp( [this]() { return _buttonUp.isPressed(); } )
        , _timedButtonDown( [this]() { return _buttonDown.isPressed(); } )
    {
        assert( step > 0 );
        assert( maximum >= minimum );
        assert( current >= minimum && current <= maximum );

        _buttonUp.setICNInfo( ICN::TOWNWIND, 5, 6 );
        _buttonDown.setICNInfo( ICN::TOWNWIND, 7, 8 );

        _buttonUp.subscribe( &_timedButtonUp );
        _buttonDown.subscribe( &_timedButtonDown );

        setOffset( offset );
    }

    void ValueSelectionDialogElement::setOffset( const fheroes2::Point & offset )
    {
        const Sprite & editBoxImage = AGG::GetICN( ICN::TOWNWIND, 4 );
        const Sprite & arrowImage = AGG::GetICN( ICN::TOWNWIND, 5 );

        _area = { offset.x, offset.y, editBoxImage.width() + 6 + arrowImage.width(), arrowImage.height() * 2 + 5 };

        _editBox = { offset.x, offset.y + ( _area.height - editBoxImage.height() ) / 2, editBoxImage.width(), editBoxImage.height() };

        _buttonUp.setPosition( offset.x + _editBox.width + 6 - arrowImage.x(), offset.y - arrowImage.y() );
        _buttonDown.setPosition( offset.x + _editBox.width + 6 - arrowImage.x(), offset.y - arrowImage.y() + _buttonUp.area().height + 5 );
    }

    void ValueSelectionDialogElement::draw( Image & output ) const
    {
        const Sprite & editBoxImage = AGG::GetICN( ICN::TOWNWIND, 4 );
        assert( _editBox.width == editBoxImage.width() && _editBox.height == editBoxImage.height() );

        Blit( editBoxImage, 0, 0, output, _editBox.x, _editBox.y, _editBox.width, _editBox.height );

        const Text text( std::to_string( _value ), FontType::normalWhite() );
        text.draw( _editBox.x + ( _editBox.width - text.width() ) / 2, _editBox.y + ( _editBox.height - 13 ) / 2, output );

        _buttonUp.draw( output );
        _buttonDown.draw( output );
    }

    bool ValueSelectionDialogElement::processEvents()
    {
        LocalEvent & le = LocalEvent::Get();

        _buttonUp.drawOnState( le.isMouseLeftButtonPressedInArea( _buttonUp.area() ) );
        _buttonDown.drawOnState( le.isMouseLeftButtonPressedInArea( _buttonDown.area() ) );

        if ( _value + _step <= _maximum && ( le.MouseClickLeft( _buttonUp.area() ) || _isMouseWheelUpEvent( le ) || _timedButtonUp.isDelayPassed() ) ) {
            _value += _step;
            return true;
        }

        if ( _value - _step >= _minimum && ( le.MouseClickLeft( _buttonDown.area() ) || _isMouseWheelDownEvent( le ) || _timedButtonDown.isDelayPassed() ) ) {
            _value -= _step;
            return true;
        }

        return false;
    }

    void ValueSelectionDialogElement::setValue( const int32_t value )
    {
        _value = std::clamp( value, _minimum, _maximum );
    }

    bool ValueSelectionDialogElement::_isMouseWheelUpEvent( const LocalEvent & eventHandler ) const
    {
        if ( _isIgnoreMouseWheelEventRoiCheck ) {
            return eventHandler.isMouseWheelUp();
        }

        return eventHandler.isMouseWheelUpInArea( _editBox );
    }

    bool ValueSelectionDialogElement::_isMouseWheelDownEvent( const LocalEvent & eventHandler ) const
    {
        if ( _isIgnoreMouseWheelEventRoiCheck ) {
            return eventHandler.isMouseWheelDown();
        }

        return eventHandler.isMouseWheelDownInArea( _editBox );
    }

    Size ValueSelectionDialogElement::getArea()
    {
        const Sprite & editBoxImage = AGG::GetICN( ICN::TOWNWIND, 4 );
        const Sprite & arrowImage = AGG::GetICN( ICN::TOWNWIND, 5 );

        return { editBoxImage.width() + 6 + arrowImage.width(), arrowImage.height() * 2 + 5 };
    }
}
