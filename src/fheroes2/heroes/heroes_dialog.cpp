/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include <string>

#include "agg.h"
#include "army_bar.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "heroes.h"
#include "heroes_indicator.h"
#include "kingdom.h"
#include "payment.h"
#include "race.h"
#include "settings.h"
#include "skill_bar.h"
#include "statusbar.h"
#include "text.h"
#include "ui_button.h"
#include "ui_tool.h"
#include "world.h"

/* readonly: false, fade: false */
int Heroes::OpenDialog( bool readonly, bool fade )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    // fade
    if ( fade && Settings::Get().ExtGameUseFade() )
        fheroes2::FadeDisplay();

    Dialog::FrameBorder background( Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT ) );
    const Point & cur_pt = background.GetArea();
    Point dst_pt( cur_pt );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HEROBKG, 0 ), display, dst_pt.x, dst_pt.y );
    fheroes2::Blit( fheroes2::AGG::GetICN( Settings::Get().ExtGameEvilInterface() ? ICN::HEROEXTE : ICN::HEROEXTG, 0 ), display, dst_pt.x, dst_pt.y );

    std::string message;

    // portrait
    dst_pt.x = cur_pt.x + 49;
    dst_pt.y = cur_pt.y + 31;
    const Rect portPos( dst_pt.x, dst_pt.y, 101, 93 );
    PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );

    // name
    message = _( "%{name} the %{race} (Level %{level})" );
    StringReplace( message, "%{name}", name );
    StringReplace( message, "%{race}", Race::String( race ) );
    StringReplace( message, "%{level}", GetLevel() );
    Text text( message, Font::BIG );
    text.Blit( cur_pt.x + 320 - text.w() / 2, cur_pt.y + 1 );

    PrimarySkillsBar primskill_bar( this, false );
    primskill_bar.SetColRows( 4, 1 );
    primskill_bar.SetHSpace( 6 );
    primskill_bar.SetPos( cur_pt.x + 156, cur_pt.y + 31 );
    primskill_bar.Redraw();

    // morale
    dst_pt.x = cur_pt.x + 514;
    dst_pt.y = cur_pt.y + 35;

    MoraleIndicator moraleIndicator( this );
    moraleIndicator.SetPos( dst_pt );
    moraleIndicator.Redraw();

    // luck
    dst_pt.x = cur_pt.x + 552;
    dst_pt.y = cur_pt.y + 35;

    LuckIndicator luckIndicator( this );
    luckIndicator.SetPos( dst_pt );
    luckIndicator.Redraw();

    // army format spread
    dst_pt.x = cur_pt.x + 516;
    dst_pt.y = cur_pt.y + 63;
    const fheroes2::Sprite & sprite1 = fheroes2::AGG::GetICN( ICN::HSICONS, 9 );
    fheroes2::Blit( sprite1, display, dst_pt.x, dst_pt.y );

    const Rect rectSpreadArmyFormat( dst_pt, sprite1.width(), sprite1.height() );
    const std::string descriptionSpreadArmyFormat
        = _( "'Spread' combat formation spreads your armies from the top to the bottom of the battlefield, with at least one empty space between each army." );
    const Point army1_pt( dst_pt.x - 1, dst_pt.y - 1 );

    // army format grouped
    dst_pt.x = cur_pt.x + 552;
    dst_pt.y = cur_pt.y + 63;
    const fheroes2::Sprite & sprite2 = fheroes2::AGG::GetICN( ICN::HSICONS, 10 );
    fheroes2::Blit( sprite2, display, dst_pt.x, dst_pt.y );

    const Rect rectGroupedArmyFormat( dst_pt, sprite2.width(), sprite2.height() );
    const std::string descriptionGroupedArmyFormat = _( "'Grouped' combat formation bunches your army together in the center of your side of the battlefield." );
    const Point army2_pt( dst_pt.x - 1, dst_pt.y - 1 );

    // cursor format
    fheroes2::MovableSprite cursorFormat( fheroes2::AGG::GetICN( ICN::HSICONS, 11 ) );
    const Point cursorFormatPos = army.isSpreadFormat() ? army1_pt : army2_pt;
    cursorFormat.setPosition( cursorFormatPos.x, cursorFormatPos.y );

    // experience
    ExperienceIndicator experienceInfo( this );
    experienceInfo.SetPos( Point( cur_pt.x + 512, cur_pt.y + 86 ) );
    experienceInfo.Redraw();

    // spell points
    SpellPointsIndicator spellPointsInfo( this );
    spellPointsInfo.SetPos( Point( cur_pt.x + 550, cur_pt.y + 88 ) );
    spellPointsInfo.Redraw();

    // crest
    dst_pt.x = cur_pt.x + 49;
    dst_pt.y = cur_pt.y + 130;

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CREST, Color::NONE == GetColor() ? Color::GetIndex( Settings::Get().CurrentColor() ) : Color::GetIndex( GetColor() ) ),
                    display, dst_pt.x, dst_pt.y );

    // monster
    dst_pt.x = cur_pt.x + 156;
    dst_pt.y = cur_pt.y + 130;

    ArmyBar selectArmy( &army, false, readonly );
    selectArmy.SetColRows( 5, 1 );
    selectArmy.SetPos( dst_pt.x, dst_pt.y );
    selectArmy.SetHSpace( 6 );
    selectArmy.Redraw();

    // secskill
    SecondarySkillsBar secskill_bar( false );
    secskill_bar.SetColRows( 8, 1 );
    secskill_bar.SetHSpace( 5 );
    secskill_bar.SetContent( secondary_skills.ToVector() );
    secskill_bar.SetPos( cur_pt.x + 3, cur_pt.y + 233 );
    secskill_bar.Redraw();

    dst_pt.x = cur_pt.x + 51;
    dst_pt.y = cur_pt.y + 308;

    ArtifactsBar selectArtifacts( this, false, readonly );

    selectArtifacts.SetColRows( 7, 2 );
    selectArtifacts.SetHSpace( 15 );
    selectArtifacts.SetVSpace( 15 );
    selectArtifacts.SetContent( GetBagArtifacts() );
    selectArtifacts.SetPos( dst_pt.x, dst_pt.y );
    selectArtifacts.Redraw();

    // bottom small bar
    dst_pt.x = cur_pt.x + 22;
    dst_pt.y = cur_pt.y + 460;
    const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( ICN::HSBTNS, 8 );
    fheroes2::Blit( bar, display, dst_pt.x, dst_pt.y );

    StatusBar statusBar;
    statusBar.SetCenter( dst_pt.x + bar.width() / 2, dst_pt.y + 13 );

    // button prev
    dst_pt.x = cur_pt.x;
    dst_pt.y = cur_pt.y + fheroes2::Display::DEFAULT_HEIGHT - 20;
    fheroes2::Button buttonPrevHero( dst_pt.x, dst_pt.y, ICN::HSBTNS, 4, 5 );

    // button next
    dst_pt.x = cur_pt.x + fheroes2::Display::DEFAULT_WIDTH - 22;
    dst_pt.y = cur_pt.y + fheroes2::Display::DEFAULT_HEIGHT - 20;
    fheroes2::Button buttonNextHero( dst_pt.x, dst_pt.y, ICN::HSBTNS, 6, 7 );

    // button dismiss
    dst_pt.x = cur_pt.x + 4;
    dst_pt.y = cur_pt.y + 318;
    fheroes2::Button buttonDismiss( dst_pt.x, dst_pt.y, ICN::HSBTNS, 0, 1 );

    // button exit
    dst_pt.x = cur_pt.x + 603;
    dst_pt.y = cur_pt.y + 318;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::HSBTNS, 2, 3 );

    LocalEvent & le = LocalEvent::GetClean();

    if ( inCastle() || readonly || Modes( NOTDISMISS ) ) {
        buttonDismiss.disable();
        if ( readonly )
            buttonDismiss.hide();
    }

    if ( readonly || 2 > GetKingdom().GetHeroes().size() ) {
        buttonNextHero.disable();
        buttonPrevHero.disable();
    }

    buttonPrevHero.draw();
    buttonNextHero.draw();
    buttonDismiss.draw();
    buttonExit.draw();

    cursor.Show();
    display.render();

    bool redrawMorale = false;
    bool redrawLuck = false;
    message.clear();

    // dialog menu loop
    while ( le.HandleEvents() ) {
        if ( redrawMorale ) {
            cursor.Hide();
            moraleIndicator.Redraw();
            cursor.Show();
            display.render();
            redrawMorale = false;
        }

        if ( redrawLuck ) {
            cursor.Hide();
            luckIndicator.Redraw();
            cursor.Show();
            display.render();
            redrawLuck = false;
        }

        // exit
        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) )
            return Dialog::CANCEL;

        // heroes troops
        if ( le.MouseCursor( selectArmy.GetArea() ) && selectArmy.QueueEventProcessing( &message ) ) {
            cursor.Hide();
            if ( selectArtifacts.isSelected() )
                selectArtifacts.ResetSelected();
            selectArmy.Redraw();
            redrawMorale = true;
            redrawLuck = true;
        }

        if ( le.MouseCursor( selectArtifacts.GetArea() ) && selectArtifacts.QueueEventProcessing( &message ) ) {
            cursor.Hide();
            if ( selectArmy.isSelected() )
                selectArmy.ResetSelected();
            selectArtifacts.Redraw();
            spellPointsInfo.Redraw();
            redrawMorale = true;
            redrawLuck = true;
        }

        // button click
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
        if ( buttonDismiss.isEnabled() )
            le.MousePressLeft( buttonDismiss.area() ) ? buttonDismiss.drawOnPress() : buttonDismiss.drawOnRelease();
        if ( buttonPrevHero.isEnabled() )
            le.MousePressLeft( buttonPrevHero.area() ) ? buttonPrevHero.drawOnPress() : buttonPrevHero.drawOnRelease();
        if ( buttonNextHero.isEnabled() )
            le.MousePressLeft( buttonNextHero.area() ) ? buttonNextHero.drawOnPress() : buttonNextHero.drawOnRelease();

        // prev hero
        if ( buttonPrevHero.isEnabled() && ( le.MouseClickLeft( buttonPrevHero.area() ) || HotKeyPressEvent( Game::EVENT_MOVELEFT ) ) ) {
            return Dialog::PREV;
        }

        // next hero
        if ( buttonNextHero.isEnabled() && ( le.MouseClickLeft( buttonNextHero.area() ) || HotKeyPressEvent( Game::EVENT_MOVERIGHT ) ) ) {
            return Dialog::NEXT;
        }

        // dismiss
        if ( buttonDismiss.isEnabled() && buttonDismiss.isVisible() && le.MouseClickLeft( buttonDismiss.area() )
             && Dialog::YES == Dialog::Message( GetName(), _( "Are you sure you want to dismiss this Hero?" ), Font::BIG, Dialog::YES | Dialog::NO ) ) {
            return Dialog::DISMISS;
        }

        if ( le.MouseCursor( moraleIndicator.GetArea() ) )
            MoraleIndicator::QueueEventProcessing( moraleIndicator );
        else if ( le.MouseCursor( luckIndicator.GetArea() ) )
            LuckIndicator::QueueEventProcessing( luckIndicator );
        else if ( le.MouseCursor( experienceInfo.GetArea() ) )
            experienceInfo.QueueEventProcessing();
        else if ( le.MouseCursor( spellPointsInfo.GetArea() ) )
            spellPointsInfo.QueueEventProcessing();

        // left click info
        if ( !readonly && le.MouseClickLeft( rectSpreadArmyFormat ) && !army.isSpreadFormat() ) {
            cursor.Hide();
            cursorFormat.setPosition( army1_pt.x, army1_pt.y );
            cursor.Show();
            display.render();
            army.SetSpreadFormat( true );
        }
        else if ( !readonly && le.MouseClickLeft( rectGroupedArmyFormat ) && army.isSpreadFormat() ) {
            cursor.Hide();
            cursorFormat.setPosition( army2_pt.x, army2_pt.y );
            cursor.Show();
            display.render();
            army.SetSpreadFormat( false );
        }
        else if ( le.MouseCursor( secskill_bar.GetArea() ) && secskill_bar.QueueEventProcessing( &message ) ) {
            cursor.Show();
            display.render();
        }
        else if ( le.MouseCursor( primskill_bar.GetArea() ) && primskill_bar.QueueEventProcessing( &message ) ) {
            cursor.Show();
            display.render();
        }

        // right info
        if ( !readonly && le.MousePressRight( portPos ) )
            Dialog::QuickInfo( *this );
        else if ( le.MousePressRight( rectSpreadArmyFormat ) )
            Dialog::Message( _( "Spread Formation" ), descriptionSpreadArmyFormat, Font::BIG );
        else if ( le.MousePressRight( rectGroupedArmyFormat ) )
            Dialog::Message( _( "Grouped Formation" ), descriptionGroupedArmyFormat, Font::BIG );

        // status message
        if ( le.MouseCursor( portPos ) )
            message = _( "View Stats" );
        else if ( le.MouseCursor( moraleIndicator.GetArea() ) ) {
            message = _( "View %{morale} Info" );
            StringReplace( message, "%{morale}", fheroes2::MoraleString( army.GetMorale() ) );
        }
        else if ( le.MouseCursor( luckIndicator.GetArea() ) ) {
            message = _( "View %{luck} Info" );
            StringReplace( message, "%{luck}", fheroes2::LuckString( army.GetLuck() ) );
        }
        else if ( le.MouseCursor( experienceInfo.GetArea() ) )
            message = _( "View Experience Info" );
        else if ( le.MouseCursor( spellPointsInfo.GetArea() ) )
            message = _( "View Spell Points Info" );
        else if ( le.MouseCursor( rectSpreadArmyFormat ) )
            message = _( "Set army combat formation to 'Spread'" );
        else if ( le.MouseCursor( rectGroupedArmyFormat ) )
            message = _( "Set army combat formation to 'Grouped'" );
        else if ( le.MouseCursor( buttonExit.area() ) )
            message = _( "Exit Hero Screen" );
        else if ( le.MouseCursor( buttonDismiss.area() ) ) {
            if ( buttonDismiss.isVisible() ) {
                if ( Modes( NOTDISMISS ) ) {
                    message = "Dismiss disabled, see game info";
                }
                else {
                    message = _( "Dismiss %{name} the %{race}" );
                    StringReplace( message, "%{name}", name );
                    StringReplace( message, "%{race}", Race::String( race ) );
                }
            }
        }
        else if ( buttonPrevHero.isEnabled() && le.MouseCursor( buttonPrevHero.area() ) )
            message = _( "Show previous hero" );
        else if ( buttonNextHero.isEnabled() && le.MouseCursor( buttonNextHero.area() ) )
            message = _( "Show next hero" );

        if ( message.empty() )
            statusBar.ShowMessage( _( "Hero Screen" ) );
        else {
            statusBar.ShowMessage( message );
            message.clear();
        }
    }

    return Dialog::CANCEL;
}
