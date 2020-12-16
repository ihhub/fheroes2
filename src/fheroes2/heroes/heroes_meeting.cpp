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

#include "agg.h"
#include "army.h"
#include "army_bar.h"
#include "cursor.h"
#include "game.h"
#include "game_interface.h"
#include "heroes.h"
#include "heroes_indicator.h"
#include "settings.h"
#include "skill_bar.h"
#include "text.h"

class MeetingArmyBar : public ArmyBar
{
public:
    explicit MeetingArmyBar( Army * army )
        : ArmyBar( army, true, false, false )
    {}

    virtual void RedrawBackground( const Rect & roi, fheroes2::Image & image ) override
    {
        if ( _cachedBackground.empty() ) {
            _cachedBackground.resize( roi.w, roi.h );
            fheroes2::Copy( image, roi.x, roi.y, _cachedBackground, 0, 0, roi.w, roi.h );
        }

        fheroes2::Blit( _cachedBackground, 0, 0, image, roi.x, roi.y, roi.w, roi.h );
    }

    virtual void RedrawItem( ArmyTroop & troop, const Rect & roi, bool isSelected, fheroes2::Image & image ) override
    {
        if ( !troop.isValid() )
            return;

        Text text( GetString( troop.GetCount() ), Font::SMALL );

        const fheroes2::Sprite & mons32 = fheroes2::AGG::GetICN( ICN::MONS32, troop.GetSpriteIndex() );
        fheroes2::Rect srcrt( 0, 0, mons32.width(), mons32.height() );

        if ( mons32.width() > roi.w ) {
            srcrt.x = ( mons32.width() - roi.w ) / 2;
            srcrt.width = roi.w;
        }

        if ( mons32.height() > roi.h ) {
            srcrt.y = ( mons32.height() - roi.h ) / 2;
            srcrt.height = roi.h;
        }

        int32_t offsetX = ( roi.w - mons32.width() ) / 2;
        int32_t offsetY = roi.h - mons32.height() - 3;

        if ( offsetX < 1 )
            offsetX = 1;

        if ( offsetY < 1 )
            offsetY = 1;

        fheroes2::Blit( mons32, srcrt.x, srcrt.y, image, roi.x + offsetX, roi.y + offsetY, srcrt.width, srcrt.height );

        text.Blit( roi.x + ( roi.w - text.w() ) / 2, roi.y + roi.h - 1, image );

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
        : ArtifactsBar( hero, true, false, false )
    {}

    virtual void RedrawBackground( const Rect & roi, fheroes2::Image & image ) override
    {
        if ( _cachedBackground.empty() ) {
            _cachedBackground.resize( roi.w, roi.h );
            fheroes2::Copy( image, roi.x, roi.y, _cachedBackground, 0, 0, roi.w, roi.h );
        }

        fheroes2::Blit( _cachedBackground, 0, 0, image, roi.x, roi.y, roi.w, roi.h );
    }

    virtual void RedrawItem( Artifact & arifact, const Rect & roi, bool isSelected, fheroes2::Image & image ) override
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

    virtual void RedrawBackground( const Rect &, fheroes2::Image & ) override
    {
        // Just do nothing
    }
};

class MeetingSecondarySkillsBar : public SecondarySkillsBar
{
public:
    explicit MeetingSecondarySkillsBar() {}

    virtual void RedrawBackground( const Rect & roi, fheroes2::Image & image ) override
    {
        if ( _cachedBackground.empty() ) {
            _cachedBackground.resize( roi.w, roi.h );
            fheroes2::Copy( image, roi.x, roi.y, _cachedBackground, 0, 0, roi.w, roi.h );
        }

        fheroes2::Blit( _cachedBackground, 0, 0, image, roi.x, roi.y, roi.w, roi.h );
    }

    virtual void RedrawItem( Skill::Secondary & skill, const Rect & roi, fheroes2::Image & image ) override
    {
        if ( !skill.isValid() )
            return;

        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MINISS, skill.GetIndexSprite2() );
        fheroes2::Blit( sprite, image, roi.x + ( roi.w - sprite.width() ) / 2, roi.y + ( roi.h - sprite.height() ) / 2 );

        Text text( GetString( skill.Level() ), Font::SMALL );
        text.Blit( roi.x + ( roi.w - text.w() ) - 3, roi.y + roi.h - text.h(), image );
    }

private:
    fheroes2::Image _cachedBackground;
};

void RedrawPrimarySkillInfo( const Point &, PrimarySkillsBar *, PrimarySkillsBar * );

void Heroes::MeetingDialog( Heroes & heroes2 )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    const fheroes2::Sprite & backSprite = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );
    const Point cur_pt( ( display.width() - backSprite.width() ) / 2, ( display.height() - backSprite.height() ) / 2 );
    fheroes2::ImageRestorer restorer( display, cur_pt.x, cur_pt.y, backSprite.width(), backSprite.height() );
    Point dst_pt( cur_pt );
    std::string message;

    Rect src_rt( 0, 0, fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );

    // background
    dst_pt.x = cur_pt.x;
    dst_pt.y = cur_pt.y;
    fheroes2::Blit( backSprite, src_rt.x, src_rt.y, display, dst_pt.x, dst_pt.y, src_rt.w, src_rt.h );

    // header
    message = _( "%{name1} meets %{name2}" );
    StringReplace( message, "%{name1}", GetName() );
    StringReplace( message, "%{name2}", heroes2.GetName() );
    Text text( message, Font::BIG );
    text.Blit( cur_pt.x + 320 - text.w() / 2, cur_pt.y + 27 );

    // portrait
    dst_pt.x = cur_pt.x + 93;
    dst_pt.y = cur_pt.y + 72;
    const fheroes2::Image portrait1 = GetPortrait( PORT_BIG );
    fheroes2::Rect hero1Area( dst_pt.x, dst_pt.y, portrait1.width(), portrait1.height() );
    PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );

    dst_pt.x = cur_pt.x + 445;
    dst_pt.y = cur_pt.y + 72;
    const fheroes2::Image portrait2 = heroes2.GetPortrait( PORT_BIG );
    fheroes2::Rect hero2Area( dst_pt.x, dst_pt.y, portrait2.width(), portrait2.height() );
    heroes2.PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );

    dst_pt.x = cur_pt.x + 34;
    dst_pt.y = cur_pt.y + 75;
    MoraleIndicator moraleIndicator1( this );
    moraleIndicator1.SetPos( dst_pt );
    moraleIndicator1.Redraw();

    dst_pt.x = cur_pt.x + 34;
    dst_pt.y = cur_pt.y + 115;
    LuckIndicator luckIndicator1( this );
    luckIndicator1.SetPos( dst_pt );
    luckIndicator1.Redraw();

    dst_pt.x = cur_pt.x + 566;
    dst_pt.y = cur_pt.y + 75;
    MoraleIndicator moraleIndicator2( &heroes2 );
    moraleIndicator2.SetPos( dst_pt );
    moraleIndicator2.Redraw();

    dst_pt.x = cur_pt.x + 566;
    dst_pt.y = cur_pt.y + 115;
    LuckIndicator luckIndicator2( &heroes2 );
    luckIndicator2.SetPos( dst_pt );
    luckIndicator2.Redraw();

    // primary skill
    fheroes2::ImageRestorer backPrimary( display, cur_pt.x + 255, cur_pt.y + 50, 130, 135 );

    MeetingPrimarySkillsBar primskill_bar1( this );
    primskill_bar1.SetColRows( 1, 4 );
    primskill_bar1.SetVSpace( -1 );
    primskill_bar1.SetTextOff( 70, -25 );
    primskill_bar1.SetPos( cur_pt.x + 216, cur_pt.y + 51 );

    MeetingPrimarySkillsBar primskill_bar2( &heroes2 );
    primskill_bar2.SetColRows( 1, 4 );
    primskill_bar2.SetVSpace( -1 );
    primskill_bar2.SetTextOff( -70, -25 );
    primskill_bar2.SetPos( cur_pt.x + 389, cur_pt.y + 51 );

    RedrawPrimarySkillInfo( cur_pt, &primskill_bar1, &primskill_bar2 );

    // secondary skill
    MeetingSecondarySkillsBar secskill_bar1;
    secskill_bar1.SetColRows( 8, 1 );
    secskill_bar1.SetHSpace( -1 );
    secskill_bar1.SetContent( secondary_skills.ToVector() );
    secskill_bar1.SetPos( cur_pt.x + 22, cur_pt.y + 199 );
    secskill_bar1.Redraw();

    MeetingSecondarySkillsBar secskill_bar2;
    secskill_bar2.SetColRows( 8, 1 );
    secskill_bar2.SetHSpace( -1 );
    secskill_bar2.SetContent( heroes2.GetSecondarySkills().ToVector() );
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

    MeetingArmyBar selectArmy2( &heroes2.GetArmy() );
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

    MeetingArtifactBar selectArtifacts2( &heroes2 );
    selectArtifacts2.SetColRows( 7, 2 );
    selectArtifacts2.SetHSpace( 2 );
    selectArtifacts2.SetVSpace( 2 );
    selectArtifacts2.SetContent( heroes2.GetBagArtifacts() );
    selectArtifacts2.SetPos( dst_pt.x, dst_pt.y );
    selectArtifacts2.Redraw();

    // button exit
    dst_pt.x = cur_pt.x + 280;
    dst_pt.y = cur_pt.y + 428;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::SWAPBTN, 0, 1 );

    buttonExit.draw();

    cursor.Show();
    display.render();

    MovePointsScaleFixed();
    heroes2.MovePointsScaleFixed();

    // scholar action
    if ( Settings::Get().ExtWorldEyeEagleAsScholar() )
        Heroes::ScholarAction( *this, heroes2 );

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
            break;

        // selector troops event
        if ( ( le.MouseCursor( selectArmy1.GetArea() ) && selectArmy1.QueueEventProcessing( selectArmy2 ) )
             || ( le.MouseCursor( selectArmy2.GetArea() ) && selectArmy2.QueueEventProcessing( selectArmy1 ) ) ) {
            cursor.Hide();

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
            cursor.Show();
            display.render();
        }

        // selector artifacts event
        if ( ( le.MouseCursor( selectArtifacts1.GetArea() ) && selectArtifacts1.QueueEventProcessing( selectArtifacts2 ) )
             || ( le.MouseCursor( selectArtifacts2.GetArea() ) && selectArtifacts2.QueueEventProcessing( selectArtifacts1 ) ) ) {
            cursor.Hide();

            if ( selectArmy1.isSelected() )
                selectArmy1.ResetSelected();
            else if ( selectArmy2.isSelected() )
                selectArmy2.ResetSelected();

            if ( bag_artifacts.MakeBattleGarb() || heroes2.bag_artifacts.MakeBattleGarb() ) {
                Dialog::ArtifactInfo( "", _( "The three Anduran artifacts magically combine into one." ), Artifact::BATTLE_GARB );
            }

            selectArtifacts1.Redraw();
            selectArtifacts2.Redraw();

            backPrimary.restore();
            RedrawPrimarySkillInfo( cur_pt, &primskill_bar1, &primskill_bar2 );
            moraleIndicator1.Redraw();
            moraleIndicator2.Redraw();
            luckIndicator1.Redraw();
            luckIndicator2.Redraw();
            cursor.Show();
            display.render();
        }

        if ( ( le.MouseCursor( primskill_bar1.GetArea() ) && primskill_bar1.QueueEventProcessing() )
             || ( le.MouseCursor( primskill_bar2.GetArea() ) && primskill_bar2.QueueEventProcessing() )
             || ( le.MouseCursor( secskill_bar1.GetArea() ) && secskill_bar1.QueueEventProcessing() )
             || ( le.MouseCursor( secskill_bar2.GetArea() ) && secskill_bar2.QueueEventProcessing() ) ) {
            cursor.Show();
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
            OpenDialog( true, true );

            armyCountBackgroundRestorer.restore();
            selectArtifacts1.ResetSelected();
            selectArmy1.Redraw();
            selectArmy2.Redraw();
            moraleIndicator1.Redraw();
            luckIndicator1.Redraw();

            cursor.Show();
            display.render();
        }
        else if ( le.MouseClickLeft( hero2Area ) ) {
            heroes2.OpenDialog( true, true );

            armyCountBackgroundRestorer.restore();
            selectArtifacts2.ResetSelected();
            selectArmy1.Redraw();
            selectArmy2.Redraw();
            moraleIndicator2.Redraw();
            luckIndicator2.Redraw();

            cursor.Show();
            display.render();
        }
    }

    if ( Settings::Get().ExtHeroRecalculateMovement() ) {
        RecalculateMovePoints();
        heroes2.RecalculateMovePoints();
    }

    backPrimary.reset();
    armyCountBackgroundRestorer.reset();
    restorer.restore();
    display.render();
}

void RedrawPrimarySkillInfo( const Point & cur_pt, PrimarySkillsBar * bar1, PrimarySkillsBar * bar2 )
{
    // attack skill
    Text text( _( "Attack Skill" ), Font::SMALL );
    text.Blit( cur_pt.x + 320 - text.w() / 2, cur_pt.y + 64 );

    // defense skill
    text.Set( _( "Defense Skill" ) );
    text.Blit( cur_pt.x + 320 - text.w() / 2, cur_pt.y + 96 );

    // spell power
    text.Set( _( "Spell Power" ) );
    text.Blit( cur_pt.x + 320 - text.w() / 2, cur_pt.y + 128 );

    // knowledge
    text.Set( _( "Knowledge" ) );
    text.Blit( cur_pt.x + 320 - text.w() / 2, cur_pt.y + 160 );

    if ( bar1 )
        bar1->Redraw();
    if ( bar2 )
        bar2->Redraw();
}

void Heroes::ScholarAction( Heroes & hero1, Heroes & hero2 )
{
    if ( !hero1.HaveSpellBook() || !hero2.HaveSpellBook() ) {
        DEBUG( DBG_GAME, DBG_INFO, "spell_book disabled" );
        return;
    }
    else if ( !Settings::Get().ExtWorldEyeEagleAsScholar() ) {
        DEBUG( DBG_GAME, DBG_WARN, "EyeEagleAsScholar settings disabled" );
        return;
    }

    const int scholar1 = hero1.GetLevelSkill( Skill::Secondary::EAGLEEYE );
    const int scholar2 = hero2.GetLevelSkill( Skill::Secondary::EAGLEEYE );
    int scholar = 0;

    Heroes * teacher = NULL;
    Heroes * learner = NULL;

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
        DEBUG( DBG_GAME, DBG_WARN, "Eagle Eye skill not found" );
        return;
    }

    // skip bag artifacts
    SpellStorage teach = teacher->spell_book.SetFilter( SpellBook::ALL );
    SpellStorage learn = learner->spell_book.SetFilter( SpellBook::ALL );

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
