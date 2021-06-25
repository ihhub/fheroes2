/****************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
 *                                                                          *
 *   This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation; either version 2 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with this program; if not, write to the                          *
 *   Free Software Foundation, Inc.,                                        *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.              *
 ****************************************************************************/

#include <algorithm>
#include <string>

#include "agg_image.h"
#include "army.h"
#include "army_bar.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "heroes.h"
#include "heroes_indicator.h"
#include "icn.h"
#include "logging.h"
#include "settings.h"
#include "skill_bar.h"
#include "text.h"
#include "tools.h"
#include "ui_button.h"

namespace
{
    void createMoveButton( fheroes2::ButtonSprite & button, const int32_t icnId, const fheroes2::Point & offset, const fheroes2::Point & screenOffset )
    {
        const fheroes2::Sprite & buttonBackground = fheroes2::AGG::GetICN( ICN::STONEBAK, 0 );
        const fheroes2::Sprite & originalReleasedImage = fheroes2::AGG::GetICN( icnId, 0 );
        const fheroes2::Sprite & originalPressedImage = fheroes2::AGG::GetICN( icnId, 1 );

        const int32_t offsetX = std::min( originalReleasedImage.x(), originalPressedImage.x() );
        const int32_t width = std::min( originalReleasedImage.width(), originalPressedImage.width() );
        const int32_t height = std::min( originalReleasedImage.height(), originalPressedImage.height() );

        fheroes2::Point extraOffset( 2, 1 );

        const int32_t extendedWidth = width + extraOffset.x * 2;
        const int32_t extendedHeight = height + extraOffset.y * 2;

        fheroes2::Sprite releasedButton( extendedWidth, extendedHeight );
        fheroes2::Copy( buttonBackground, offset.x + offsetX - extraOffset.x, offset.y - extraOffset.y, releasedButton, 0, 0, extendedWidth, extendedHeight );
        fheroes2::Blit( originalReleasedImage, 0, 0, releasedButton, extraOffset.x, extraOffset.y, originalReleasedImage.width(), originalReleasedImage.height() );

        fheroes2::Sprite pressedButton( extendedWidth, height + extraOffset.y * 2 );
        fheroes2::Copy( buttonBackground, offset.x + offsetX - extraOffset.x, offset.y - extraOffset.y, pressedButton, 0, 0, extendedWidth, extendedHeight );
        fheroes2::Blit( originalPressedImage, 0, 0, pressedButton, extraOffset.x, extraOffset.y + 1, originalPressedImage.width(), originalPressedImage.height() );

        button.setPosition( screenOffset.x + offset.x + offsetX - extraOffset.x, screenOffset.y + offset.y - extraOffset.y );
        button.setSprite( releasedButton, pressedButton );
    }

    void moveArtifacts( BagArtifacts & bagFrom, BagArtifacts & bagTo )
    {
        size_t toIdx = 0;

        for ( size_t fromIdx = 0; fromIdx < bagFrom.size(); ++fromIdx ) {
            if ( bagFrom[fromIdx]() != Artifact::UNKNOWN && bagFrom[fromIdx]() != Artifact::MAGIC_BOOK ) {
                while ( toIdx < bagTo.size() ) {
                    if ( bagTo[toIdx]() == Artifact::UNKNOWN )
                        break;

                    ++toIdx;
                }

                if ( toIdx == bagTo.size() )
                    break;

                std::swap( bagFrom[fromIdx], bagTo[toIdx] );
            }
        }
    }
}

class MeetingArmyBar : public ArmyBar
{
public:
    explicit MeetingArmyBar( Army * army )
        : ArmyBar( army, true, false, false )
    {}

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

        Text text( std::to_string( troop.GetCount() ), Font::SMALL );

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

        text.Blit( roi.x + ( roi.width - text.w() ) / 2, roi.y + roi.height - 1, image );

        if ( isSelected ) {
            spcursor.setPosition( roi.x, roi.y );
            spcursor.show();
        }
    }

private:
    fheroes2::Image _cachedBackground;
};

class MeetingArtifactBar : public ArtifactsBar
{
public:
    explicit MeetingArtifactBar( const Heroes * hero )
        : ArtifactsBar( hero, true, false, false, false, nullptr )
    {}

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

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::ARTFX, arifact.IndexSprite32() ), image, roi.x + 1, roi.y + 1 );

        if ( isSelected ) {
            spcursor.setPosition( roi.x, roi.y );
            spcursor.show();
        }
    }

private:
    fheroes2::Image _cachedBackground;
};

class MeetingPrimarySkillsBar : public PrimarySkillsBar
{
public:
    explicit MeetingPrimarySkillsBar( const Heroes * hero )
        : PrimarySkillsBar( hero, true )
    {}

    void RedrawBackground( const fheroes2::Rect &, fheroes2::Image & ) override
    {
        // Just do nothing
    }
};

class MeetingSecondarySkillsBar : public SecondarySkillsBar
{
public:
    explicit MeetingSecondarySkillsBar( const Heroes & hero )
        : SecondarySkillsBar( hero )
    {}

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

        Text text( std::to_string( skill.Level() ), Font::SMALL );
        text.Blit( roi.x + ( roi.width - text.w() ) - 3, roi.y + roi.height - text.h(), image );
    }

private:
    fheroes2::Image _cachedBackground;
};

void Heroes::MeetingDialog( Heroes & otherHero )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::Sprite & backSprite = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );
    const fheroes2::Point cur_pt( ( display.width() - backSprite.width() ) / 2, ( display.height() - backSprite.height() ) / 2 );
    fheroes2::ImageRestorer restorer( display, cur_pt.x, cur_pt.y, backSprite.width(), backSprite.height() );
    fheroes2::Point dst_pt( cur_pt );
    std::string message;

    fheroes2::Rect src_rt( 0, 0, fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );

    // background
    dst_pt.x = cur_pt.x;
    dst_pt.y = cur_pt.y;
    fheroes2::Blit( backSprite, src_rt.x, src_rt.y, display, dst_pt.x, dst_pt.y, src_rt.width, src_rt.height );

    // header
    message = _( "%{name1} meets %{name2}" );
    StringReplace( message, "%{name1}", GetName() );
    StringReplace( message, "%{name2}", otherHero.GetName() );
    Text text( message, Font::BIG );
    text.Blit( cur_pt.x + 320 - text.w() / 2, cur_pt.y + 27 );

    const int iconsH1XOffset = 34;
    const int iconsH2XOffset = 566;
    const int portraitYOffset = 72;

    // portrait
    dst_pt.x = cur_pt.x + 93;
    dst_pt.y = cur_pt.y + portraitYOffset;
    const fheroes2::Sprite & portrait1 = GetPortrait( PORT_BIG );
    fheroes2::Rect hero1Area( dst_pt.x, dst_pt.y, portrait1.width(), portrait1.height() );
    PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );

    dst_pt.x = cur_pt.x + 445;
    dst_pt.y = cur_pt.y + portraitYOffset;
    const fheroes2::Sprite & portrait2 = otherHero.GetPortrait( PORT_BIG );
    fheroes2::Rect hero2Area( dst_pt.x, dst_pt.y, portrait2.width(), portrait2.height() );
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
    primskill_bar1.SetColRows( 1, 4 );
    primskill_bar1.SetVSpace( -1 );
    primskill_bar1.SetTextOff( 70, -25 );
    primskill_bar1.SetPos( cur_pt.x + 216, cur_pt.y + 51 );

    MeetingPrimarySkillsBar primskill_bar2( &otherHero );
    primskill_bar2.SetColRows( 1, 4 );
    primskill_bar2.SetVSpace( -1 );
    primskill_bar2.SetTextOff( -70, -25 );
    primskill_bar2.SetPos( cur_pt.x + 389, cur_pt.y + 51 );

    fheroes2::RedrawPrimarySkillInfo( cur_pt, &primskill_bar1, &primskill_bar2 );

    // secondary skill
    MeetingSecondarySkillsBar secskill_bar1( *this );
    secskill_bar1.SetColRows( 8, 1 );
    secskill_bar1.SetHSpace( -1 );
    secskill_bar1.SetContent( secondary_skills.ToVector() );
    secskill_bar1.SetPos( cur_pt.x + 22, cur_pt.y + 199 );
    secskill_bar1.Redraw();

    MeetingSecondarySkillsBar secskill_bar2( otherHero );
    secskill_bar2.SetColRows( 8, 1 );
    secskill_bar2.SetHSpace( -1 );
    secskill_bar2.SetContent( otherHero.GetSecondarySkills().ToVector() );
    secskill_bar2.SetPos( cur_pt.x + 353, cur_pt.y + 199 );
    secskill_bar2.Redraw();

    // army
    dst_pt.x = cur_pt.x + 36;
    dst_pt.y = cur_pt.y + 267;

    fheroes2::ImageRestorer armyCountBackgroundRestorer( display, cur_pt.x + 36, cur_pt.y + 310, 567, 20 );

    MeetingArmyBar selectArmy1( &GetArmy() );
    selectArmy1.SetColRows( 5, 1 );
    selectArmy1.SetPos( dst_pt.x, dst_pt.y );
    selectArmy1.SetHSpace( 2 );
    selectArmy1.Redraw();

    dst_pt.x = cur_pt.x + 381;
    dst_pt.y = cur_pt.y + 267;

    MeetingArmyBar selectArmy2( &otherHero.GetArmy() );
    selectArmy2.SetColRows( 5, 1 );
    selectArmy2.SetPos( dst_pt.x, dst_pt.y );
    selectArmy2.SetHSpace( 2 );
    selectArmy2.Redraw();

    // artifact
    dst_pt.x = cur_pt.x + 23;
    dst_pt.y = cur_pt.y + 347;

    MeetingArtifactBar selectArtifacts1( this );
    selectArtifacts1.SetColRows( 7, 2 );
    selectArtifacts1.SetHSpace( 2 );
    selectArtifacts1.SetVSpace( 2 );
    selectArtifacts1.SetContent( GetBagArtifacts() );
    selectArtifacts1.SetPos( dst_pt.x, dst_pt.y );
    selectArtifacts1.Redraw();

    dst_pt.x = cur_pt.x + 367;
    dst_pt.y = cur_pt.y + 347;

    MeetingArtifactBar selectArtifacts2( &otherHero );
    selectArtifacts2.SetColRows( 7, 2 );
    selectArtifacts2.SetHSpace( 2 );
    selectArtifacts2.SetVSpace( 2 );
    selectArtifacts2.SetContent( otherHero.GetBagArtifacts() );
    selectArtifacts2.SetPos( dst_pt.x, dst_pt.y );
    selectArtifacts2.Redraw();

    // button exit
    dst_pt.x = cur_pt.x + 280;
    dst_pt.y = cur_pt.y + 428;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::SWAPBTN, 0, 1 );

    buttonExit.draw();

    // The original resources do not have such animated buttons so we have to create those.
    const fheroes2::Point windowOffset( cur_pt.x, cur_pt.y );
    fheroes2::ButtonSprite moveArmyToHero2;
    createMoveButton( moveArmyToHero2, ICN::SWAP_ARROW_LEFT_TO_RIGHT, fheroes2::Point( 297, 270 ), windowOffset );

    fheroes2::ButtonSprite moveArmyToHero1;
    createMoveButton( moveArmyToHero1, ICN::SWAP_ARROW_RIGHT_TO_LEFT, fheroes2::Point( 295, 291 ), windowOffset );

    fheroes2::ButtonSprite moveArtifactsToHero2;
    createMoveButton( moveArtifactsToHero2, ICN::SWAP_ARROW_LEFT_TO_RIGHT, fheroes2::Point( 297, 363 ), windowOffset );

    fheroes2::ButtonSprite moveArtifactsToHero1;
    createMoveButton( moveArtifactsToHero1, ICN::SWAP_ARROW_RIGHT_TO_LEFT, fheroes2::Point( 295, 384 ), windowOffset );

    moveArmyToHero2.draw();
    moveArmyToHero1.draw();
    moveArtifactsToHero2.draw();
    moveArtifactsToHero1.draw();

    display.render();

    MovePointsScaleFixed();
    otherHero.MovePointsScaleFixed();

    // scholar action
    if ( Settings::Get().ExtWorldEyeEagleAsScholar() )
        Heroes::ScholarAction( *this, otherHero );

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
        le.MousePressLeft( moveArmyToHero2.area() ) ? moveArmyToHero2.drawOnPress() : moveArmyToHero2.drawOnRelease();
        le.MousePressLeft( moveArmyToHero1.area() ) ? moveArmyToHero1.drawOnPress() : moveArmyToHero1.drawOnRelease();
        le.MousePressLeft( moveArtifactsToHero2.area() ) ? moveArtifactsToHero2.drawOnPress() : moveArtifactsToHero2.drawOnRelease();
        le.MousePressLeft( moveArtifactsToHero1.area() ) ? moveArtifactsToHero1.drawOnPress() : moveArtifactsToHero1.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
            break;

        // selector troops event
        if ( ( le.MouseCursor( selectArmy1.GetArea() ) && selectArmy1.QueueEventProcessing( selectArmy2 ) )
             || ( le.MouseCursor( selectArmy2.GetArea() ) && selectArmy2.QueueEventProcessing( selectArmy1 ) ) ) {
            if ( selectArtifacts1.isSelected() )
                selectArtifacts1.ResetSelected();
            else if ( selectArtifacts2.isSelected() )
                selectArtifacts2.ResetSelected();

            armyCountBackgroundRestorer.restore();

            selectArmy1.Redraw();
            selectArmy2.Redraw();

            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();
            luckIndicator1.Redraw();
            luckIndicator2.Redraw();

            display.render();
        }

        // selector artifacts event
        if ( ( le.MouseCursor( selectArtifacts1.GetArea() ) && selectArtifacts1.QueueEventProcessing( selectArtifacts2 ) )
             || ( le.MouseCursor( selectArtifacts2.GetArea() ) && selectArtifacts2.QueueEventProcessing( selectArtifacts1 ) ) ) {
            if ( selectArmy1.isSelected() )
                selectArmy1.ResetSelected();
            else if ( selectArmy2.isSelected() )
                selectArmy2.ResetSelected();

            if ( bag_artifacts.MakeBattleGarb() || otherHero.bag_artifacts.MakeBattleGarb() ) {
                Dialog::ArtifactInfo( "", _( "The three Anduran artifacts magically combine into one." ), Artifact::BATTLE_GARB );
            }

            selectArtifacts1.Redraw();
            selectArtifacts2.Redraw();

            backPrimary.restore();
            fheroes2::RedrawPrimarySkillInfo( cur_pt, &primskill_bar1, &primskill_bar2 );
            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();
            luckIndicator1.Redraw();
            luckIndicator2.Redraw();

            display.render();
        }

        if ( ( le.MouseCursor( primskill_bar1.GetArea() ) && primskill_bar1.QueueEventProcessing() )
             || ( le.MouseCursor( primskill_bar2.GetArea() ) && primskill_bar2.QueueEventProcessing() )
             || ( le.MouseCursor( secskill_bar1.GetArea() ) && secskill_bar1.QueueEventProcessing() )
             || ( le.MouseCursor( secskill_bar2.GetArea() ) && secskill_bar2.QueueEventProcessing() ) ) {
            display.render();
        }

        if ( le.MouseCursor( moraleIndicator1.GetArea() ) ) {
            MoraleIndicator::QueueEventProcessing( moraleIndicator1 );
        }
        else if ( le.MouseCursor( moraleIndicator2.GetArea() ) ) {
            MoraleIndicator::QueueEventProcessing( moraleIndicator2 );
        }
        else if ( le.MouseCursor( luckIndicator1.GetArea() ) ) {
            LuckIndicator::QueueEventProcessing( luckIndicator1 );
        }
        else if ( le.MouseCursor( luckIndicator2.GetArea() ) ) {
            LuckIndicator::QueueEventProcessing( luckIndicator2 );
        }

        if ( le.MouseClickLeft( hero1Area ) ) {
            Game::OpenHeroesDialog( *this, false, false, true );

            armyCountBackgroundRestorer.restore();

            selectArtifacts1.ResetSelected();
            selectArtifacts2.ResetSelected();
            selectArtifacts1.Redraw();
            selectArtifacts2.Redraw();

            selectArmy1.ResetSelected();
            selectArmy2.ResetSelected();
            selectArmy1.Redraw();
            selectArmy2.Redraw();

            moraleIndicator1.Redraw();
            luckIndicator1.Redraw();

            display.render();
        }
        else if ( le.MouseClickLeft( hero2Area ) ) {
            Game::OpenHeroesDialog( otherHero, false, false, true );

            armyCountBackgroundRestorer.restore();

            selectArtifacts1.ResetSelected();
            selectArtifacts2.ResetSelected();
            selectArtifacts1.Redraw();
            selectArtifacts2.Redraw();

            selectArmy1.ResetSelected();
            selectArmy2.ResetSelected();
            selectArmy1.Redraw();
            selectArmy2.Redraw();

            moraleIndicator2.Redraw();
            luckIndicator2.Redraw();

            display.render();
        }
        else if ( le.MouseClickLeft( moveArmyToHero2.area() ) ) {
            otherHero.GetArmy().MoveTroops( GetArmy() );

            armyCountBackgroundRestorer.restore();

            selectArmy1.ResetSelected();
            selectArmy2.ResetSelected();
            selectArmy1.Redraw();
            selectArmy2.Redraw();

            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();

            display.render();
        }
        else if ( le.MouseClickLeft( moveArmyToHero1.area() ) ) {
            GetArmy().MoveTroops( otherHero.GetArmy() );

            armyCountBackgroundRestorer.restore();

            selectArmy1.ResetSelected();
            selectArmy2.ResetSelected();
            selectArmy1.Redraw();
            selectArmy2.Redraw();

            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();

            display.render();
        }
        else if ( le.MouseClickLeft( moveArtifactsToHero2.area() ) ) {
            moveArtifacts( GetBagArtifacts(), otherHero.GetBagArtifacts() );

            selectArtifacts1.ResetSelected();
            selectArtifacts2.ResetSelected();
            selectArtifacts1.Redraw();
            selectArtifacts2.Redraw();

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
            selectArtifacts1.Redraw();
            selectArtifacts2.Redraw();

            backPrimary.restore();
            fheroes2::RedrawPrimarySkillInfo( cur_pt, &primskill_bar1, &primskill_bar2 );
            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();
            luckIndicator1.Redraw();
            luckIndicator2.Redraw();

            display.render();
        }
    }

    selectArmy1.ResetSelected();
    selectArmy2.ResetSelected();
    selectArtifacts1.ResetSelected();
    selectArtifacts2.ResetSelected();

    backPrimary.reset();
    armyCountBackgroundRestorer.reset();
    restorer.restore();
    display.render();
}

void Heroes::ScholarAction( Heroes & hero1, Heroes & hero2 )
{
    if ( !hero1.HaveSpellBook() || !hero2.HaveSpellBook() ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, "spell_book disabled" );
        return;
    }
    else if ( !Settings::Get().ExtWorldEyeEagleAsScholar() ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "EyeEagleAsScholar settings disabled" );
        return;
    }

    const int scholar1 = hero1.GetLevelSkill( Skill::Secondary::EAGLEEYE );
    const int scholar2 = hero2.GetLevelSkill( Skill::Secondary::EAGLEEYE );
    int scholar = 0;

    Heroes * teacher = nullptr;
    Heroes * learner = nullptr;

    if ( scholar1 && scholar1 >= scholar2 ) {
        teacher = &hero1;
        learner = &hero2;
        scholar = scholar1;
    }
    else if ( scholar2 && scholar2 >= scholar1 ) {
        teacher = &hero2;
        learner = &hero1;
        scholar = scholar2;
    }
    else {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Eagle Eye skill not found" );
        return;
    }

    // skip bag artifacts
    SpellStorage teach = teacher->spell_book.SetFilter( SpellBook::Filter::ALL );
    SpellStorage learn = learner->spell_book.SetFilter( SpellBook::Filter::ALL );

    // remove_if for learn spells
    if ( learn.size() ) {
        SpellStorage::iterator res = std::remove_if( learn.begin(), learn.end(), [teacher]( const Spell & spell ) { return teacher->HaveSpell( spell ); } );
        learn.resize( std::distance( learn.begin(), res ) );
    }

    if ( learn.size() ) {
        SpellStorage::iterator res = std::remove_if( learn.begin(), learn.end(), [teacher]( const Spell & spell ) { return !teacher->CanTeachSpell( spell ); } );
        learn.resize( std::distance( learn.begin(), res ) );
    }

    // remove_if for teach spells
    if ( teach.size() ) {
        SpellStorage::iterator res = std::remove_if( teach.begin(), teach.end(), [learner]( const Spell & spell ) { return learner->HaveSpell( spell ); } );
        teach.resize( std::distance( teach.begin(), res ) );
    }

    if ( teach.size() ) {
        SpellStorage::iterator res = std::remove_if( teach.begin(), teach.end(), [teacher]( const Spell & spell ) { return !teacher->CanTeachSpell( spell ); } );
        teach.resize( std::distance( teach.begin(), res ) );
    }

    std::string message, spells1, spells2;

    // learning
    for ( SpellStorage::const_iterator it = learn.begin(); it != learn.end(); ++it ) {
        teacher->AppendSpellToBook( *it );
        if ( spells1.size() )
            spells1.append( it + 1 == learn.end() ? _( " and " ) : ", " );
        spells1.append( ( *it ).GetName() );
    }

    // teacher
    for ( SpellStorage::const_iterator it = teach.begin(); it != teach.end(); ++it ) {
        learner->AppendSpellToBook( *it );
        if ( spells2.size() )
            spells2.append( it + 1 == teach.end() ? _( " and " ) : ", " );
        spells2.append( ( *it ).GetName() );
    }

    if ( teacher->isControlHuman() || learner->isControlHuman() ) {
        if ( spells1.size() && spells2.size() )
            message = _( "%{teacher}, whose %{level} %{scholar} knows many magical secrets, learns %{spells1} from %{learner}, and teaches %{spells2} to %{learner}." );
        else if ( spells1.size() )
            message = _( "%{teacher}, whose %{level} %{scholar} knows many magical secrets, learns %{spells1} from %{learner}." );
        else if ( spells2.size() )
            message = _( "%{teacher}, whose %{level} %{scholar} knows many magical secrets, teaches %{spells2} to %{learner}." );

        if ( message.size() ) {
            StringReplace( message, "%{teacher}", teacher->GetName() );
            StringReplace( message, "%{learner}", learner->GetName() );
            StringReplace( message, "%{level}", Skill::Level::String( scholar ) );
            StringReplace( message, "%{scholar}", Skill::Secondary::String( Skill::Secondary::EAGLEEYE ) );
            StringReplace( message, "%{spells1}", spells1 );
            StringReplace( message, "%{spells2}", spells2 );

            Dialog::Message( _( "Scholar Ability" ), message, Font::BIG, Dialog::OK );
        }
    }
}
