/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "army.h"
#include "army_bar.h"
#include "army_troop.h"
#include "artifact.h"
#include "artifact_info.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_hotkeys.h"
#include "heroes.h" // IWYU pragma: associated
#include "heroes_base.h"
#include "heroes_indicator.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "monster.h"
#include "screen.h"
#include "skill.h"
#include "skill_bar.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"

namespace
{
    fheroes2::ButtonSprite createMoveButton( const int32_t icnId, const int32_t offsetX, const int32_t offsetY, const fheroes2::Image & display )
    {
        const fheroes2::Sprite & originalReleasedImage = fheroes2::AGG::GetICN( icnId, 0 );
        const fheroes2::Sprite & originalPressedImage = fheroes2::AGG::GetICN( icnId, 1 );

        const int32_t minX = std::min( originalReleasedImage.x(), originalPressedImage.x() );
        const int32_t minY = std::min( originalReleasedImage.y(), originalPressedImage.y() );

        return fheroes2::makeButtonWithShadow( offsetX + minX, offsetY + minY, originalReleasedImage, originalPressedImage, display, fheroes2::Point( -3, 3 ) );
    }

    void moveArtifacts( BagArtifacts & bagFrom, BagArtifacts & bagTo )
    {
        size_t toIdx = 0;

        for ( size_t fromIdx = 0; fromIdx < bagFrom.size(); ++fromIdx ) {
            if ( bagFrom[fromIdx].isValid() && bagFrom[fromIdx].GetID() != Artifact::MAGIC_BOOK ) {
                while ( toIdx < bagTo.size() ) {
                    if ( !bagTo[toIdx].isValid() )
                        break;

                    ++toIdx;
                }

                if ( toIdx == bagTo.size() )
                    break;

                std::swap( bagFrom[fromIdx], bagTo[toIdx] );
            }
        }
    }

    void swapArtifacts( BagArtifacts & firstBag, BagArtifacts & secondBag )
    {
        const auto moveRemainingArtifact = []( BagArtifacts & from, BagArtifacts & to, const BagArtifacts::reverse_iterator & fromIter ) {
            if ( fromIter == from.rend() ) {
                return;
            }

            // If there is any artifact left that has not yet been moved, it is assumed that it goes first in the list of artifacts (in place of the missing Magic Book)
            assert( fromIter == std::prev( from.rend() ) );

            if ( !fromIter->isValid() ) {
                return;
            }

            assert( *fromIter != Artifact::MAGIC_BOOK );

            // Just try to put this artifact to the first empty slot (if any)
            if ( !to.PushArtifact( *fromIter ) ) {
                return;
            }

            fromIter->Reset();
        };

        for ( auto firstBagIter = firstBag.rbegin(), secondBagIter = secondBag.rbegin(); firstBagIter != firstBag.rend() && secondBagIter != secondBag.rend();
              ++firstBagIter, ++secondBagIter ) {
            // It is assumed that the only non-transferable artifact is a Magic Book, and that it always comes first in the list of artifacts
            if ( *firstBagIter == Artifact::MAGIC_BOOK ) {
                assert( firstBagIter == std::prev( firstBag.rend() ) );

                ++firstBagIter;
            }
            if ( *secondBagIter == Artifact::MAGIC_BOOK ) {
                assert( secondBagIter == std::prev( secondBag.rend() ) );

                ++secondBagIter;
            }

            if ( firstBagIter == firstBag.rend() ) {
                moveRemainingArtifact( secondBag, firstBag, secondBagIter );

                break;
            }
            if ( secondBagIter == secondBag.rend() ) {
                moveRemainingArtifact( firstBag, secondBag, firstBagIter );

                break;
            }

            std::swap( *firstBagIter, *secondBagIter );
        }
    }
}

class MeetingArmyBar final : public ArmyBar
{
public:
    using ArmyBar::RedrawItem;

    explicit MeetingArmyBar( Army * army )
        : ArmyBar( army, true, false, false )
    {
        // Do nothing.
    }

    void RedrawBackground( const fheroes2::Rect & roi, fheroes2::Image & image ) override
    {
        if ( _cachedBackground.empty() ) {
            _cachedBackground.resize( roi.width, roi.height );
            fheroes2::Copy( image, roi.x, roi.y, _cachedBackground, 0, 0, roi.width, roi.height );
        }

        fheroes2::Blit( _cachedBackground, 0, 0, image, roi.x, roi.y, roi.width, roi.height );
    }

    void RedrawItem( ArmyTroop & troop, const fheroes2::Rect & roi, bool isSelected, fheroes2::Image & image ) override
    {
        if ( !troop.isValid() )
            return;

        const fheroes2::Text text( std::to_string( troop.GetCount() ), fheroes2::FontType::smallWhite() );

        const fheroes2::Sprite & mons32 = fheroes2::AGG::GetICN( ICN::MONS32, troop.GetSpriteIndex() );
        fheroes2::Rect srcrt( 0, 0, mons32.width(), mons32.height() );

        if ( mons32.width() > roi.width ) {
            srcrt.x = ( mons32.width() - roi.width ) / 2;
            srcrt.width = roi.width;
        }

        if ( mons32.height() > roi.height ) {
            srcrt.y = ( mons32.height() - roi.height ) / 2;
            srcrt.height = roi.height;
        }

        int32_t offsetX = ( roi.width - mons32.width() ) / 2;
        int32_t offsetY = roi.height - mons32.height() - 3;

        if ( offsetX < 1 )
            offsetX = 1;

        if ( offsetY < 1 )
            offsetY = 1;

        fheroes2::Blit( mons32, srcrt.x, srcrt.y, image, roi.x + offsetX, roi.y + offsetY, srcrt.width, srcrt.height );

        text.draw( roi.x + ( roi.width - text.width() ) / 2, roi.y + roi.height + 1, image );

        if ( isSelected ) {
            spcursor.setPosition( roi.x, roi.y );
            spcursor.show();
        }
    }

private:
    fheroes2::Image _cachedBackground;
};

class MeetingArtifactBar final : public ArtifactsBar
{
public:
    using ArtifactsBar::RedrawItem;

    explicit MeetingArtifactBar( Heroes * hero )
        : ArtifactsBar( hero, true, false, false, false, nullptr )
    {
        // Do nothing.
    }

    void RedrawBackground( const fheroes2::Rect & roi, fheroes2::Image & image ) override
    {
        if ( _cachedBackground.empty() ) {
            _cachedBackground.resize( roi.width, roi.height );
            fheroes2::Copy( image, roi.x, roi.y, _cachedBackground, 0, 0, roi.width, roi.height );
        }

        fheroes2::Blit( _cachedBackground, 0, 0, image, roi.x, roi.y, roi.width, roi.height );
    }

    void RedrawItem( Artifact & arifact, const fheroes2::Rect & roi, bool isSelected, fheroes2::Image & image ) override
    {
        if ( !arifact.isValid() )
            return;

        const fheroes2::Sprite & artifactSprite = fheroes2::AGG::GetICN( ICN::ARTFX, arifact.IndexSprite32() );
        fheroes2::Blit( artifactSprite, image, roi.x + 1, roi.y + 1 );

        if ( isSelected ) {
            spcursor.setPosition( roi.x, roi.y );
            spcursor.show();
        }
    }

private:
    fheroes2::Image _cachedBackground;
};

class MeetingPrimarySkillsBar final : public PrimarySkillsBar
{
public:
    explicit MeetingPrimarySkillsBar( Heroes * hero )
        : PrimarySkillsBar( hero, true, false, false )
    {
        // Do nothing.
    }

    void RedrawBackground( const fheroes2::Rect &, fheroes2::Image & ) override
    {
        // Just do nothing
    }
};

class MeetingSecondarySkillsBar final : public SecondarySkillsBar
{
public:
    explicit MeetingSecondarySkillsBar( const Heroes & hero )
        : SecondarySkillsBar( hero )
    {
        // Do nothing.
    }

    void RedrawBackground( const fheroes2::Rect & roi, fheroes2::Image & image ) override
    {
        if ( _cachedBackground.empty() ) {
            _cachedBackground.resize( roi.width, roi.height );
            fheroes2::Copy( image, roi.x, roi.y, _cachedBackground, 0, 0, roi.width, roi.height );
        }

        fheroes2::Blit( _cachedBackground, 0, 0, image, roi.x, roi.y, roi.width, roi.height );
    }

    void RedrawItem( Skill::Secondary & skill, const fheroes2::Rect & roi, fheroes2::Image & image ) override
    {
        if ( !skill.isValid() )
            return;

        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MINISS, skill.GetIndexSprite2() );
        fheroes2::Blit( sprite, image, roi.x + ( roi.width - sprite.width() ) / 2, roi.y + ( roi.height - sprite.height() ) / 2 );

        const fheroes2::Text text( std::to_string( skill.Level() ), fheroes2::FontType::smallWhite() );
        text.draw( roi.x + ( roi.width - text.width() ) - 3, roi.y + roi.height - text.height() + 2, image );
    }

private:
    fheroes2::Image _cachedBackground;
};

void Heroes::MeetingDialog( Heroes & otherHero )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // Set the cursor image. This dialog is called after hero's move and does not require a cursor restorer:
    // Game Area event processor will change it to the appropriate one after this dialog is closed.
    Cursor::Get().SetThemes( Cursor::POINTER );

    const fheroes2::Sprite & backSprite = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );
    const fheroes2::Point cur_pt( ( display.width() - backSprite.width() ) / 2, ( display.height() - backSprite.height() ) / 2 );
    fheroes2::ImageRestorer restorer( display, cur_pt.x, cur_pt.y, backSprite.width(), backSprite.height() );

    const fheroes2::Rect src_rt( 0, 0, fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );

    // Fade-out game screen only for 640x480 resolution.
    const fheroes2::Rect fadeRoi( src_rt + cur_pt );
    const bool isDefaultScreenSize = display.isDefaultSize();
    if ( isDefaultScreenSize ) {
        fheroes2::fadeOutDisplay();
    }

    // background
    fheroes2::Point dst_pt( cur_pt );
    fheroes2::Blit( backSprite, src_rt.x, src_rt.y, display, dst_pt.x, dst_pt.y, src_rt.width, src_rt.height );

    // shadow
    if ( !isDefaultScreenSize ) {
        fheroes2::addGradientShadowForArea( display, dst_pt, backSprite.width(), backSprite.height(), fheroes2::borderWidthPx );
    }

    // header
    std::string message( _( "%{name1} meets %{name2}" ) );
    StringReplace( message, "%{name1}", GetName() );
    StringReplace( message, "%{name2}", otherHero.GetName() );
    fheroes2::Text text( std::move( message ), fheroes2::FontType::normalWhite() );
    text.draw( cur_pt.x + 320 - text.width() / 2, cur_pt.y + 29, display );

    // Render hero's levels.
    message = _( "hero|Level %{level}" );
    StringReplace( message, "%{level}", GetLevel() );
    text.set( std::move( message ), fheroes2::FontType::smallWhite() );
    text.draw( cur_pt.x + 143 - text.width() / 2, cur_pt.y + 52, display );

    message = _( "hero|Level %{level}" );
    StringReplace( message, "%{level}", otherHero.GetLevel() );
    text.set( std::move( message ), fheroes2::FontType::smallWhite() );
    text.draw( cur_pt.x + 495 - text.width() / 2, cur_pt.y + 52, display );

    const int iconsH1XOffset = 34;
    const int iconsH2XOffset = 571;
    const int portraitYOffset = 72;

    // portrait
    dst_pt.x = cur_pt.x + 93;
    dst_pt.y = cur_pt.y + portraitYOffset;
    const fheroes2::Sprite & portrait1 = GetPortrait( PORT_BIG );
    const fheroes2::Rect hero1Area( dst_pt.x, dst_pt.y, portrait1.width(), portrait1.height() );
    PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );

    dst_pt.x = cur_pt.x + 445;
    dst_pt.y = cur_pt.y + portraitYOffset;
    const fheroes2::Sprite & portrait2 = otherHero.GetPortrait( PORT_BIG );
    const fheroes2::Rect hero2Area( dst_pt.x, dst_pt.y, portrait2.width(), portrait2.height() );
    otherHero.PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );

    MoraleIndicator moraleIndicator1( this );
    dst_pt.x = cur_pt.x + iconsH1XOffset;
    dst_pt.y = cur_pt.y + portraitYOffset + moraleIndicator1.GetArea().height / 3;
    moraleIndicator1.SetPos( dst_pt );
    moraleIndicator1.Redraw();

    LuckIndicator luckIndicator1( this );
    dst_pt.x = cur_pt.x + iconsH1XOffset;
    dst_pt.y = cur_pt.y + portraitYOffset + portrait1.height() - luckIndicator1.GetArea().height * 4 / 3;
    luckIndicator1.SetPos( dst_pt );
    luckIndicator1.Redraw();

    MoraleIndicator moraleIndicator2( &otherHero );
    dst_pt.x = cur_pt.x + iconsH2XOffset;
    dst_pt.y = cur_pt.y + portraitYOffset + moraleIndicator2.GetArea().height / 3;
    moraleIndicator2.SetPos( dst_pt );
    moraleIndicator2.Redraw();

    LuckIndicator luckIndicator2( &otherHero );
    dst_pt.x = cur_pt.x + iconsH2XOffset;
    dst_pt.y = cur_pt.y + portraitYOffset + portrait2.height() - luckIndicator2.GetArea().height * 4 / 3;
    luckIndicator2.SetPos( dst_pt );
    luckIndicator2.Redraw();

    // primary skill
    fheroes2::ImageRestorer backPrimary( display, cur_pt.x + 255, cur_pt.y + 50, 130, 135 );

    MeetingPrimarySkillsBar primskill_bar1( this );
    primskill_bar1.setTableSize( { 1, 4 } );
    primskill_bar1.setInBetweenItemsOffset( { 0, -1 } );
    primskill_bar1.SetTextOff( 70, -25 );
    primskill_bar1.setRenderingOffset( { cur_pt.x + 216, cur_pt.y + 51 } );

    MeetingPrimarySkillsBar primskill_bar2( &otherHero );
    primskill_bar2.setTableSize( { 1, 4 } );
    primskill_bar2.setInBetweenItemsOffset( { 0, -1 } );
    primskill_bar2.SetTextOff( -70, -25 );
    primskill_bar2.setRenderingOffset( { cur_pt.x + 389, cur_pt.y + 51 } );

    fheroes2::RedrawPrimarySkillInfo( cur_pt, &primskill_bar1, &primskill_bar2 );

    // secondary skill
    MeetingSecondarySkillsBar secskill_bar1( *this );
    secskill_bar1.setTableSize( { 8, 1 } );
    secskill_bar1.setInBetweenItemsOffset( { -1, 0 } );
    secskill_bar1.SetContent( _secondarySkills.ToVector() );
    secskill_bar1.setRenderingOffset( { cur_pt.x + 22, cur_pt.y + 199 } );
    secskill_bar1.Redraw( display );

    MeetingSecondarySkillsBar secskill_bar2( otherHero );
    secskill_bar2.setTableSize( { 8, 1 } );
    secskill_bar2.setInBetweenItemsOffset( { -1, 0 } );
    secskill_bar2.SetContent( otherHero.GetSecondarySkills().ToVector() );
    secskill_bar2.setRenderingOffset( { cur_pt.x + 353, cur_pt.y + 199 } );
    secskill_bar2.Redraw( display );

    const fheroes2::Sprite & moveButtonBackground = fheroes2::AGG::GetICN( ICN::STONEBAK, 0 );
    fheroes2::Blit( moveButtonBackground, 292, 270, display, cur_pt.x + 292, cur_pt.y + 270, 48, 44 );

    // The original resources do not have such animated buttons so we have to create those.
    fheroes2::ButtonSprite moveArmyToHero2 = createMoveButton( ICN::SWAP_ARROW_LEFT_TO_RIGHT, cur_pt.x + 126, cur_pt.y + 319, display );
    fheroes2::ButtonSprite moveArmyToHero1 = createMoveButton( ICN::SWAP_ARROW_RIGHT_TO_LEFT, cur_pt.x + 472, cur_pt.y + 319, display );
    fheroes2::ButtonSprite swapArmies = createMoveButton( ICN::SWAP_ARROWS_CIRCULAR, cur_pt.x + 297, cur_pt.y + 268, display );

    fheroes2::ImageRestorer armyCountBackgroundRestorerLeft( display, cur_pt.x + 36, cur_pt.y + 311, 223, 8 );
    fheroes2::ImageRestorer armyCountBackgroundRestorerRight( display, cur_pt.x + 381, cur_pt.y + 311, 223, 8 );

    // army
    dst_pt.x = cur_pt.x + 36;
    dst_pt.y = cur_pt.y + 267;

    MeetingArmyBar selectArmy1( &GetArmy() );
    selectArmy1.setTableSize( { 5, 1 } );
    selectArmy1.setRenderingOffset( dst_pt );
    selectArmy1.setInBetweenItemsOffset( { 2, 0 } );
    selectArmy1.Redraw( display );

    dst_pt.x = cur_pt.x + 381;
    dst_pt.y = cur_pt.y + 267;

    MeetingArmyBar selectArmy2( &otherHero.GetArmy() );
    selectArmy2.setTableSize( { 5, 1 } );
    selectArmy2.setRenderingOffset( dst_pt );
    selectArmy2.setInBetweenItemsOffset( { 2, 0 } );
    selectArmy2.Redraw( display );

    // artifact
    dst_pt.x = cur_pt.x + 23;
    dst_pt.y = cur_pt.y + 347;

    MeetingArtifactBar selectArtifacts1( this );
    selectArtifacts1.setTableSize( { 7, 2 } );
    selectArtifacts1.setInBetweenItemsOffset( { 2, 2 } );
    selectArtifacts1.SetContent( GetBagArtifacts() );
    selectArtifacts1.setRenderingOffset( dst_pt );
    selectArtifacts1.Redraw( display );

    dst_pt.x = cur_pt.x + 367;
    dst_pt.y = cur_pt.y + 347;

    MeetingArtifactBar selectArtifacts2( &otherHero );
    selectArtifacts2.setTableSize( { 7, 2 } );
    selectArtifacts2.setInBetweenItemsOffset( { 2, 2 } );
    selectArtifacts2.SetContent( otherHero.GetBagArtifacts() );
    selectArtifacts2.setRenderingOffset( dst_pt );
    selectArtifacts2.Redraw( display );

    fheroes2::Blit( moveButtonBackground, 292, 363, display, cur_pt.x + 292, cur_pt.y + 363, 48, 44 );
    fheroes2::ButtonSprite moveArtifactsToHero2 = createMoveButton( ICN::SWAP_ARROW_LEFT_TO_RIGHT, cur_pt.x + 126, cur_pt.y + 426, display );
    fheroes2::ButtonSprite moveArtifactsToHero1 = createMoveButton( ICN::SWAP_ARROW_RIGHT_TO_LEFT, cur_pt.x + 472, cur_pt.y + 426, display );
    fheroes2::ButtonSprite swapArtifacts = createMoveButton( ICN::SWAP_ARROWS_CIRCULAR, cur_pt.x + 297, cur_pt.y + 361, display );

    // button exit
    dst_pt.x = cur_pt.x + 280;
    dst_pt.y = cur_pt.y + 428;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::BUTTON_EXIT_HEROES_MEETING, 0, 1 );

    moveArmyToHero2.draw();
    moveArmyToHero1.draw();
    swapArmies.draw();
    moveArtifactsToHero2.draw();
    moveArtifactsToHero1.draw();
    swapArtifacts.draw();
    buttonExit.draw();

    // Fade-in heroes meeting dialog. Use half fade if game resolution is not 640x480.
    // This dialog currently does not have borders so its ROI is the same as fade ROI.
    fheroes2::fadeInDisplay( fadeRoi, !isDefaultScreenSize );

    const int32_t hero1ScoutAreaBonus = _bagArtifacts.getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::AREA_REVEAL_DISTANCE );
    const int32_t hero2ScoutAreaBonus = otherHero.GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::AREA_REVEAL_DISTANCE );

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        buttonExit.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonExit.area() ) );

        if ( le.isMouseLeftButtonPressedInArea( moveArmyToHero2.area() ) || HotKeyHoldEvent( Game::HotKeyEvent::DEFAULT_RIGHT ) ) {
            moveArmyToHero2.drawOnPress();
            moveArmyToHero1.drawOnRelease();
        }
        else if ( le.isMouseLeftButtonPressedInArea( moveArmyToHero1.area() ) || HotKeyHoldEvent( Game::HotKeyEvent::DEFAULT_LEFT ) ) {
            moveArmyToHero1.drawOnPress();
            moveArmyToHero2.drawOnRelease();
        }
        else if ( le.isMouseLeftButtonPressedInArea( swapArmies.area() ) || HotKeyHoldEvent( Game::HotKeyEvent::ARMY_SWAP ) ) {
            swapArmies.drawOnPress();
        }
        else {
            moveArmyToHero1.drawOnRelease();
            moveArmyToHero2.drawOnRelease();
            swapArmies.drawOnRelease();
        }

        if ( le.isMouseLeftButtonPressedInArea( moveArtifactsToHero2.area() ) ) {
            moveArtifactsToHero2.drawOnPress();
            moveArtifactsToHero1.drawOnRelease();
        }
        else if ( le.isMouseLeftButtonPressedInArea( moveArtifactsToHero1.area() ) ) {
            moveArtifactsToHero1.drawOnPress();
            moveArtifactsToHero2.drawOnRelease();
        }
        else if ( le.isMouseLeftButtonPressedInArea( swapArtifacts.area() ) ) {
            swapArtifacts.drawOnPress();
        }
        else {
            moveArtifactsToHero1.drawOnRelease();
            moveArtifactsToHero2.drawOnRelease();
            swapArtifacts.drawOnRelease();
        }

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
            break;
        }

        // selector troops event
        if ( ( le.isMouseCursorPosInArea( selectArmy1.GetArea() ) && selectArmy1.QueueEventProcessing( selectArmy2 ) )
             || ( le.isMouseCursorPosInArea( selectArmy2.GetArea() ) && selectArmy2.QueueEventProcessing( selectArmy1 ) ) ) {
            if ( selectArtifacts1.isSelected() )
                selectArtifacts1.ResetSelected();
            else if ( selectArtifacts2.isSelected() )
                selectArtifacts2.ResetSelected();

            armyCountBackgroundRestorerLeft.restore();
            armyCountBackgroundRestorerRight.restore();

            selectArmy1.Redraw( display );
            selectArmy2.Redraw( display );

            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();
            luckIndicator1.Redraw();
            luckIndicator2.Redraw();

            display.render();
        }

        // selector artifacts event
        if ( ( le.isMouseCursorPosInArea( selectArtifacts1.GetArea() ) && selectArtifacts1.QueueEventProcessing( selectArtifacts2 ) )
             || ( le.isMouseCursorPosInArea( selectArtifacts2.GetArea() ) && selectArtifacts2.QueueEventProcessing( selectArtifacts1 ) ) ) {
            if ( selectArmy1.isSelected() )
                selectArmy1.ResetSelected();
            else if ( selectArmy2.isSelected() )
                selectArmy2.ResetSelected();

            std::set<ArtifactSetData> assembledArtifacts = _bagArtifacts.assembleArtifactSetIfPossible();
            std::set<ArtifactSetData> otherHeroAssembledArtifacts = otherHero._bagArtifacts.assembleArtifactSetIfPossible();

            // MSVC 2017 fails to use the std::set<...>::merge( std::set<...> && ) overload here, so we have to use a temporary variable
            assembledArtifacts.merge( otherHeroAssembledArtifacts );

            std::for_each( assembledArtifacts.begin(), assembledArtifacts.end(), Dialog::ArtifactSetAssembled );

            selectArtifacts1.Redraw( display );
            selectArtifacts2.Redraw( display );

            backPrimary.restore();
            fheroes2::RedrawPrimarySkillInfo( cur_pt, &primskill_bar1, &primskill_bar2 );
            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();
            luckIndicator1.Redraw();
            luckIndicator2.Redraw();

            display.render();
        }

        if ( ( le.isMouseCursorPosInArea( primskill_bar1.GetArea() ) && primskill_bar1.QueueEventProcessing() )
             || ( le.isMouseCursorPosInArea( primskill_bar2.GetArea() ) && primskill_bar2.QueueEventProcessing() )
             || ( le.isMouseCursorPosInArea( secskill_bar1.GetArea() ) && secskill_bar1.QueueEventProcessing() )
             || ( le.isMouseCursorPosInArea( secskill_bar2.GetArea() ) && secskill_bar2.QueueEventProcessing() ) ) {
            display.render();
        }

        if ( le.isMouseCursorPosInArea( moraleIndicator1.GetArea() ) ) {
            MoraleIndicator::QueueEventProcessing( moraleIndicator1 );
        }
        else if ( le.isMouseCursorPosInArea( moraleIndicator2.GetArea() ) ) {
            MoraleIndicator::QueueEventProcessing( moraleIndicator2 );
        }
        else if ( le.isMouseCursorPosInArea( luckIndicator1.GetArea() ) ) {
            LuckIndicator::QueueEventProcessing( luckIndicator1 );
        }
        else if ( le.isMouseCursorPosInArea( luckIndicator2.GetArea() ) ) {
            LuckIndicator::QueueEventProcessing( luckIndicator2 );
        }

        const bool isHero1LeftClicked = le.MouseClickLeft( hero1Area );

        if ( isHero1LeftClicked || le.MouseClickLeft( hero2Area ) ) {
            // Currently we are calling hero dialog with borders from meeting dialog without borders
            // so the engine thinks that we are opening there was now window before to fade-out.
            // We also have to cache the display image to properly restore it after closing hero dialog.

            const fheroes2::Rect restorerRoi( cur_pt.x - 2 * fheroes2::borderWidthPx, cur_pt.y - fheroes2::borderWidthPx, src_rt.width + 3 * fheroes2::borderWidthPx,
                                              src_rt.height + 3 * fheroes2::borderWidthPx );
            fheroes2::ImageRestorer dialogRestorer( display, restorerRoi.x, restorerRoi.y, restorerRoi.width, restorerRoi.height );

            // If game display resolution is 640x480 then all fade effects are done in 'OpenHeroesDialog()' except fade-in after dialog close.
            if ( !isDefaultScreenSize ) {
                fheroes2::fadeOutDisplay( fadeRoi, true );
            }

            Game::OpenHeroesDialog( isHero1LeftClicked ? *this : otherHero, false, true, true );

            if ( !isDefaultScreenSize ) {
                dialogRestorer.restore();
            }
            else {
                dialogRestorer.reset();
            }

            armyCountBackgroundRestorerLeft.restore();
            armyCountBackgroundRestorerRight.restore();

            selectArtifacts1.ResetSelected();
            selectArtifacts2.ResetSelected();
            selectArtifacts1.Redraw( display );
            selectArtifacts2.Redraw( display );

            selectArmy1.ResetSelected();
            selectArmy2.ResetSelected();
            selectArmy1.Redraw( display );
            selectArmy2.Redraw( display );

            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();
            luckIndicator1.Redraw();
            luckIndicator2.Redraw();

            display.updateNextRenderRoi( restorerRoi );
            fheroes2::fadeInDisplay( fadeRoi, !isDefaultScreenSize );
        }
        else if ( le.MouseClickLeft( moveArmyToHero2.area() ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_RIGHT ) ) {
            const ArmyTroop * keep = nullptr;

            if ( selectArmy1.isSelected() ) {
                keep = selectArmy1.GetSelectedItem();
            }
            else if ( selectArmy2.isSelected() ) {
                keep = selectArmy2.GetSelectedItem();
            }

            otherHero.GetArmy().MoveTroops( GetArmy(), keep ? keep->GetID() : Monster::UNKNOWN );

            armyCountBackgroundRestorerLeft.restore();
            armyCountBackgroundRestorerRight.restore();

            selectArmy1.ResetSelected();
            selectArmy2.ResetSelected();
            selectArmy1.Redraw( display );
            selectArmy2.Redraw( display );

            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();

            display.render();
        }
        else if ( le.MouseClickLeft( moveArmyToHero1.area() ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_LEFT ) ) {
            const ArmyTroop * keep = nullptr;

            if ( selectArmy1.isSelected() ) {
                keep = selectArmy1.GetSelectedItem();
            }
            else if ( selectArmy2.isSelected() ) {
                keep = selectArmy2.GetSelectedItem();
            }

            GetArmy().MoveTroops( otherHero.GetArmy(), keep ? keep->GetID() : Monster::UNKNOWN );

            armyCountBackgroundRestorerLeft.restore();
            armyCountBackgroundRestorerRight.restore();

            selectArmy1.ResetSelected();
            selectArmy2.ResetSelected();
            selectArmy1.Redraw( display );
            selectArmy2.Redraw( display );

            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();

            display.render();
        }
        else if ( le.MouseClickLeft( swapArmies.area() ) || HotKeyPressEvent( Game::HotKeyEvent::ARMY_SWAP ) ) {
            GetArmy().SwapTroops( otherHero.GetArmy() );

            armyCountBackgroundRestorerLeft.restore();
            armyCountBackgroundRestorerRight.restore();

            selectArmy1.ResetSelected();
            selectArmy2.ResetSelected();

            selectArmy1.Redraw( display );
            selectArmy2.Redraw( display );

            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();

            display.render();
        }
        else if ( le.MouseClickLeft( moveArtifactsToHero2.area() ) ) {
            moveArtifacts( GetBagArtifacts(), otherHero.GetBagArtifacts() );

            selectArtifacts1.ResetSelected();
            selectArtifacts2.ResetSelected();

            selectArtifacts1.Redraw( display );
            selectArtifacts2.Redraw( display );

            backPrimary.restore();

            fheroes2::RedrawPrimarySkillInfo( cur_pt, &primskill_bar1, &primskill_bar2 );

            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();

            luckIndicator1.Redraw();
            luckIndicator2.Redraw();

            display.render();
        }
        else if ( le.MouseClickLeft( moveArtifactsToHero1.area() ) ) {
            moveArtifacts( otherHero.GetBagArtifacts(), GetBagArtifacts() );

            selectArtifacts1.ResetSelected();
            selectArtifacts2.ResetSelected();

            selectArtifacts1.Redraw( display );
            selectArtifacts2.Redraw( display );

            backPrimary.restore();

            fheroes2::RedrawPrimarySkillInfo( cur_pt, &primskill_bar1, &primskill_bar2 );

            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();

            luckIndicator1.Redraw();
            luckIndicator2.Redraw();

            display.render();
        }
        else if ( le.MouseClickLeft( swapArtifacts.area() ) ) {
            ::swapArtifacts( GetBagArtifacts(), otherHero.GetBagArtifacts() );

            selectArtifacts1.ResetSelected();
            selectArtifacts2.ResetSelected();

            selectArtifacts1.Redraw( display );
            selectArtifacts2.Redraw( display );

            backPrimary.restore();

            fheroes2::RedrawPrimarySkillInfo( cur_pt, &primskill_bar1, &primskill_bar2 );

            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();

            luckIndicator1.Redraw();
            luckIndicator2.Redraw();

            display.render();
        }

        if ( le.isMouseRightButtonPressedInArea( hero1Area ) ) {
            Dialog::QuickInfo( *this );
        }
        else if ( le.isMouseRightButtonPressedInArea( hero2Area ) ) {
            Dialog::QuickInfo( otherHero );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonExit.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Exit" ), _( "Exit this menu." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( moveArtifactsToHero1.area() ) ) {
            message = _( "Move all artifacts from %{from_hero_name} to %{to_hero_name}." );
            StringReplace( message, "%{from_hero_name}", otherHero.GetName() );
            StringReplace( message, "%{to_hero_name}", GetName() );

            fheroes2::showStandardTextMessage( _( "Artifact Transfer" ), std::move( message ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( moveArtifactsToHero2.area() ) ) {
            message = _( "Move all artifacts from %{from_hero_name} to %{to_hero_name}." );
            StringReplace( message, "%{from_hero_name}", GetName() );
            StringReplace( message, "%{to_hero_name}", otherHero.GetName() );

            fheroes2::showStandardTextMessage( _( "Artifact Transfer" ), std::move( message ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( swapArtifacts.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Swap Artifacts" ), _( "Swap all artifacts between heroes." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( moveArmyToHero1.area() ) ) {
            message = _( "Move army from %{from_hero_name} to %{to_hero_name}." );
            StringReplace( message, "%{from_hero_name}", otherHero.GetName() );
            StringReplace( message, "%{to_hero_name}", GetName() );

            fheroes2::showStandardTextMessage( _( "Army Transfer" ), std::move( message ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( moveArmyToHero2.area() ) ) {
            message = _( "Move army from %{from_hero_name} to %{to_hero_name}." );
            StringReplace( message, "%{from_hero_name}", GetName() );
            StringReplace( message, "%{to_hero_name}", otherHero.GetName() );

            fheroes2::showStandardTextMessage( _( "Army Transfer" ), std::move( message ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( swapArmies.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Swap Armies" ), _( "Swap armies between heroes." ), Dialog::ZERO );
        }
    }

    selectArmy1.ResetSelected();
    selectArmy2.ResetSelected();
    selectArtifacts1.ResetSelected();
    selectArtifacts2.ResetSelected();

    backPrimary.reset();
    armyCountBackgroundRestorerLeft.reset();
    armyCountBackgroundRestorerRight.reset();

    // Fade-out heroes meeting dialog.
    fheroes2::fadeOutDisplay( fadeRoi, !isDefaultScreenSize );

    restorer.restore();

    // If the scout area bonus is increased with the new artifact we reveal the fog and update the radar.
    if ( hero1ScoutAreaBonus < _bagArtifacts.getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::AREA_REVEAL_DISTANCE ) ) {
        Scout( GetIndex() );
        ScoutRadar();
    }
    if ( hero2ScoutAreaBonus < otherHero.GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::AREA_REVEAL_DISTANCE ) ) {
        otherHero.Scout( otherHero.GetIndex() );
        otherHero.ScoutRadar();
    }

    // Set fade-in game screen only for 640x480 resolution.
    if ( isDefaultScreenSize ) {
        Game::setDisplayFadeIn();
    }
    else {
        // Heroes meeting dialog currently does not have borders so its ROI is the same as fade ROI.
        display.render( fadeRoi );
    }
}
