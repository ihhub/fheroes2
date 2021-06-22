/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <sstream>

#include "agg_image.h"
#include "army_bar.h"
#include "buildinginfo.h"
#include "castle.h"
#include "cursor.h"
#include "game.h"
#include "game_interface.h"
#include "heroes.h"
#include "icn.h"
#include "interface_icons.h"
#include "interface_list.h"
#include "kingdom.h"
#include "skill_bar.h"
#include "text.h"
#include "tools.h"
#include "ui_button.h"
#include "ui_window.h"
#include "world.h"

#include <cassert>

struct HeroRow
{
    Heroes * hero;
    std::unique_ptr<ArmyBar> armyBar;
    std::unique_ptr<ArtifactsBar> artifactsBar;
    std::unique_ptr<SecondarySkillsBar> secskillsBar;
    std::unique_ptr<PrimarySkillsBar> primskillsBar;

    explicit HeroRow( Heroes * ptr = nullptr )
    {
        assert( ptr != nullptr );
        Init( ptr );
    }

    HeroRow( const HeroRow & ) = delete;
    HeroRow & operator=( const HeroRow & ) = delete;
    HeroRow( HeroRow && ) = default;
    HeroRow & operator=( HeroRow && ) = default;

    void Init( Heroes * ptr )
    {
        hero = ptr;

        armyBar.reset( new ArmyBar( &hero->GetArmy(), true, false ) );
        armyBar->SetBackground( fheroes2::Size( 41, 53 ), fheroes2::GetColorId( 72, 28, 0 ) );
        armyBar->SetColRows( 5, 1 );
        armyBar->SetHSpace( -1 );

        artifactsBar.reset( new ArtifactsBar( hero, true, false, false, true, nullptr ) );
        artifactsBar->SetColRows( 7, 2 );
        artifactsBar->SetHSpace( 1 );
        artifactsBar->SetVSpace( 8 );
        artifactsBar->SetContent( hero->GetBagArtifacts() );

        secskillsBar.reset( new SecondarySkillsBar( *hero ) );
        secskillsBar->SetColRows( 4, 2 );
        secskillsBar->SetHSpace( -1 );
        secskillsBar->SetVSpace( 8 );
        secskillsBar->SetContent( hero->GetSecondarySkills().ToVector() );

        primskillsBar.reset( new PrimarySkillsBar( ptr, true ) );
        primskillsBar->SetColRows( 4, 1 );
        primskillsBar->SetHSpace( 2 );
        primskillsBar->SetTextOff( 20, -13 );
    }
};

class StatsHeroesList : public Interface::ListBox<HeroRow>
{
public:
    StatsHeroesList( const fheroes2::Point & pt, const KingdomHeroes & );

    bool Refresh( KingdomHeroes & heroes );

    void RedrawItem( const HeroRow &, s32, s32, bool ) override;
    void RedrawBackground( const fheroes2::Point & ) override;

    void ActionCurrentUp() override {}
    void ActionCurrentDn() override {}
    void ActionListSingleClick( HeroRow & ) override {}
    void ActionListDoubleClick( HeroRow & ) override {}
    void ActionListPressRight( HeroRow & ) override {}

    void ActionListSingleClick( HeroRow &, const fheroes2::Point &, s32, s32 ) override;
    void ActionListDoubleClick( HeroRow &, const fheroes2::Point &, s32, s32 ) override;
    void ActionListPressRight( HeroRow &, const fheroes2::Point &, s32, s32 ) override;
    bool ActionListCursor( HeroRow &, const fheroes2::Point & ) override;

private:
    std::vector<HeroRow> content;

    void SetContent( const KingdomHeroes & heroes );
};

StatsHeroesList::StatsHeroesList( const fheroes2::Point & pt, const KingdomHeroes & heroes )
    : Interface::ListBox<HeroRow>( pt )
{
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 );

    SetTopLeft( pt );
    SetScrollBar( fheroes2::AGG::GetICN( ICN::SCROLL, 4 ), fheroes2::Rect( pt.x + 629, pt.y + 18, back.width(), back.height() - 2 ) );
    SetScrollButtonUp( ICN::SCROLL, 0, 1, fheroes2::Point( pt.x + 626, pt.y ) );
    SetScrollButtonDn( ICN::SCROLL, 2, 3, fheroes2::Point( pt.x + 626, pt.y + 20 + back.height() ) );
    SetAreaMaxItems( 4 );
    SetAreaItems( fheroes2::Rect( pt.x + 30, pt.y + 17, 594, 344 ) );
    SetContent( heroes );
}

void StatsHeroesList::SetContent( const KingdomHeroes & heroes )
{
    content.clear();
    content.reserve( heroes.size() );
    for ( Heroes * hero : heroes )
        content.emplace_back( hero );
    SetListContent( content );
}

// Updates the UI list according to current list of kingdom heroes.
// Returns true if we updated something
bool StatsHeroesList::Refresh( KingdomHeroes & heroes )
{
    if ( heroes.size() != content.size() ) {
        SetContent( heroes );
        return true;
    }
    for ( size_t i = 0; i < content.size(); ++i ) {
        if ( heroes[i] != content[i].hero ) {
            SetContent( heroes );
            return true;
        }
    }
    return false;
}

void StatsHeroesList::ActionListDoubleClick( HeroRow & row, const fheroes2::Point & cursor, s32 ox, s32 oy )
{
    ActionListSingleClick( row, cursor, ox, oy );
}

void StatsHeroesList::ActionListSingleClick( HeroRow & row, const fheroes2::Point & cursor, s32 ox, s32 oy )
{
    if ( row.hero && ( fheroes2::Rect( ox + 5, oy + 4, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight() ) & cursor ) )
        Game::OpenHeroesDialog( *row.hero, false, false );
}

void StatsHeroesList::ActionListPressRight( HeroRow & row, const fheroes2::Point & cursor, s32 ox, s32 oy )
{
    if ( row.hero && ( fheroes2::Rect( ox + 5, oy + 4, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight() ) & cursor ) )
        Dialog::QuickInfo( *row.hero );
}

bool StatsHeroesList::ActionListCursor( HeroRow & row, const fheroes2::Point & cursor )
{
    const fheroes2::Point cursorPos( cursor.x, cursor.y );

    if ( ( row.armyBar->GetArea() & cursorPos ) && row.armyBar->QueueEventProcessing() ) {
        if ( row.artifactsBar->isSelected() )
            row.artifactsBar->ResetSelected();
        return true;
    }
    else if ( ( row.artifactsBar->GetArea() & cursorPos ) && row.artifactsBar->QueueEventProcessing() ) {
        if ( row.armyBar->isSelected() )
            row.armyBar->ResetSelected();
        return true;
    }
    else if ( ( row.primskillsBar->GetArea() & cursorPos ) && row.primskillsBar->QueueEventProcessing() ) {
        return true;
    }
    else if ( ( row.secskillsBar->GetArea() & cursorPos ) && row.secskillsBar->QueueEventProcessing() ) {
        return true;
    }

    return false;
}

void StatsHeroesList::RedrawItem( const HeroRow & row, s32 dstx, s32 dsty, bool current )
{
    (void)current;

    if ( row.hero ) {
        Text text( "", Font::SMALL );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERVIEW, 10 ), fheroes2::Display::instance(), dstx, dsty );

        // base info
        Interface::RedrawHeroesIcon( *row.hero, dstx + 5, dsty + 4 );

        text.Set( std::to_string( row.hero->GetAttack() ) );
        text.Blit( dstx + 90 - text.w(), dsty + 20 );

        text.Set( std::to_string( row.hero->GetDefense() ) );
        text.Blit( dstx + 125 - text.w(), dsty + 20 );

        text.Set( std::to_string( row.hero->GetPower() ) );
        text.Blit( dstx + 160 - text.w(), dsty + 20 );

        text.Set( std::to_string( row.hero->GetKnowledge() ) );
        text.Blit( dstx + 195 - text.w(), dsty + 20 );

        // primary skills info
        row.primskillsBar->SetPos( dstx + 56, dsty - 3 );
        row.primskillsBar->Redraw();

        // secondary skills info
        row.secskillsBar->SetPos( dstx + 206, dsty + 3 );
        row.secskillsBar->Redraw();

        // artifacts info
        row.artifactsBar->SetPos( dstx + 348, dsty + 3 );
        row.artifactsBar->Redraw();

        // army info
        row.armyBar->SetPos( dstx - 1, dsty + 30 );
        row.armyBar->Redraw();
    }
}

void StatsHeroesList::RedrawBackground( const fheroes2::Point & dst )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Text text( "", Font::SMALL );

    // header
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERVIEW, 6 ), display, dst.x + 30, dst.y );

    text.Set( _( "Hero/Stats" ) );
    text.Blit( dst.x + 130 - text.w() / 2, dst.y + 1 );

    text.Set( _( "Skills" ) );
    text.Blit( dst.x + 300 - text.w() / 2, dst.y + 1 );

    text.Set( _( "Artifacts" ) );
    text.Blit( dst.x + 500 - text.w() / 2, dst.y + 1 );

    // scrollbar background
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 ), display, dst.x + 628, dst.y + 17 );

    // items background
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::OVERVIEW, 8 );
    for ( int ii = 0; ii < VisibleItemCount(); ++ii ) {
        fheroes2::Blit( back, display, dst.x + 30, dst.y + 17 + ii * ( back.height() + 4 ) );
    }
}

struct CstlRow
{
    Castle * castle;
    std::unique_ptr<ArmyBar> armyBarGuard;
    std::unique_ptr<ArmyBar> armyBarGuest;
    std::unique_ptr<DwellingsBar> dwellingsBar;

    explicit CstlRow( Castle * ptr = nullptr )
    {
        assert( ptr != nullptr );
        Init( ptr );
    }

    CstlRow( const CstlRow & ) = delete;
    CstlRow & operator=( const CstlRow & ) = delete;
    CstlRow( CstlRow && ) = default;
    CstlRow & operator=( CstlRow && ) = default;

    void Init( Castle * ptr )
    {
        castle = ptr;

        const uint8_t fill = fheroes2::GetColorId( 40, 12, 0 );

        armyBarGuard.reset( new ArmyBar( &castle->GetArmy(), true, false ) );
        armyBarGuard->SetBackground( fheroes2::Size( 41, 41 ), fill );
        armyBarGuard->SetColRows( 5, 1 );
        armyBarGuard->SetHSpace( -1 );

        CastleHeroes heroes = world.GetHeroes( *castle );

        if ( heroes.Guest() ) {
            armyBarGuest.reset( new ArmyBar( &heroes.Guest()->GetArmy(), true, false ) );
            armyBarGuest->SetBackground( fheroes2::Size( 41, 41 ), fill );
            armyBarGuest->SetColRows( 5, 1 );
            armyBarGuest->SetHSpace( -1 );
        }
        else {
            armyBarGuest.reset();
        }

        dwellingsBar.reset( new DwellingsBar( *castle, fheroes2::Size( 39, 52 ) ) );
        dwellingsBar->SetColRows( 6, 1 );
        dwellingsBar->SetHSpace( 2 );
    }
};

class StatsCastlesList : public Interface::ListBox<CstlRow>
{
public:
    StatsCastlesList( const fheroes2::Point & pt, const KingdomCastles & );

    void RedrawItem( const CstlRow &, s32, s32, bool ) override;
    void RedrawBackground( const fheroes2::Point & ) override;

    void ActionCurrentUp( void ) override {}
    void ActionCurrentDn( void ) override {}
    void ActionListDoubleClick( CstlRow & ) override {}
    void ActionListSingleClick( CstlRow & ) override {}
    void ActionListPressRight( CstlRow & ) override {}

    void ActionListSingleClick( CstlRow &, const fheroes2::Point &, s32, s32 ) override;
    void ActionListDoubleClick( CstlRow &, const fheroes2::Point &, s32, s32 ) override;
    void ActionListPressRight( CstlRow &, const fheroes2::Point &, s32, s32 ) override;
    bool ActionListCursor( CstlRow &, const fheroes2::Point & ) override;

private:
    std::vector<CstlRow> content;
};

StatsCastlesList::StatsCastlesList( const fheroes2::Point & pt, const KingdomCastles & castles )
    : Interface::ListBox<CstlRow>( pt )
{
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 );

    SetTopLeft( pt );
    SetScrollBar( fheroes2::AGG::GetICN( ICN::SCROLL, 4 ), fheroes2::Rect( pt.x + 629, pt.y + 18, back.width(), back.height() - 2 ) );
    SetScrollButtonUp( ICN::SCROLL, 0, 1, fheroes2::Point( pt.x + 626, pt.y ) );
    SetScrollButtonDn( ICN::SCROLL, 2, 3, fheroes2::Point( pt.x + 626, pt.y + 20 + back.height() ) );
    SetAreaMaxItems( 4 );
    SetAreaItems( fheroes2::Rect( pt.x + 30, pt.y + 17, 594, 344 ) );

    content.reserve( castles.size() );

    for ( Castle * castle : castles )
        content.emplace_back( castle );

    SetListContent( content );
}

void StatsCastlesList::ActionListDoubleClick( CstlRow & row, const fheroes2::Point & cursor, s32 ox, s32 oy )
{
    ActionListSingleClick( row, cursor, ox, oy );
}

void StatsCastlesList::ActionListSingleClick( CstlRow & row, const fheroes2::Point & cursor, s32 ox, s32 oy )
{
    if ( row.castle ) {
        // click castle icon
        if ( fheroes2::Rect( ox + 17, oy + 19, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight() ) & cursor ) {
            Game::OpenCastleDialog( *row.castle, false );
            row.Init( row.castle );
        }
        else
            // click hero icon
            if ( fheroes2::Rect( ox + 82, oy + 19, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight() ) & cursor ) {
            Heroes * hero = row.castle->GetHeroes().GuardFirst();
            if ( hero ) {
                Game::OpenHeroesDialog( *hero, false, false );
                row.Init( row.castle );
            }
        }
    }
}

void StatsCastlesList::ActionListPressRight( CstlRow & row, const fheroes2::Point & cursor, s32 ox, s32 oy )
{
    if ( row.castle ) {
        if ( fheroes2::Rect( ox + 17, oy + 19, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight() ) & cursor )
            Dialog::QuickInfo( *row.castle );
        else if ( fheroes2::Rect( ox + 82, oy + 19, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight() ) & cursor ) {
            const Heroes * hero = row.castle->GetHeroes().GuardFirst();
            if ( hero )
                Dialog::QuickInfo( *hero );
        }
    }
}

bool StatsCastlesList::ActionListCursor( CstlRow & row, const fheroes2::Point & cursor )
{
    const fheroes2::Point cursorPos( cursor.x, cursor.y );

    if ( row.armyBarGuard && ( row.armyBarGuard->GetArea() & cursorPos )
         && ( row.armyBarGuest ? row.armyBarGuard->QueueEventProcessing( *row.armyBarGuest ) : row.armyBarGuard->QueueEventProcessing() ) ) {
        if ( row.armyBarGuest && row.armyBarGuest->isSelected() )
            row.armyBarGuest->ResetSelected();
        return true;
    }
    else if ( row.armyBarGuest && ( row.armyBarGuest->GetArea() & cursorPos )
              && ( row.armyBarGuard ? row.armyBarGuest->QueueEventProcessing( *row.armyBarGuard ) : row.armyBarGuest->QueueEventProcessing() ) ) {
        if ( row.armyBarGuard && row.armyBarGuard->isSelected() )
            row.armyBarGuard->ResetSelected();
        return true;
    }
    else if ( row.dwellingsBar && ( row.dwellingsBar->GetArea() & cursorPos ) && row.dwellingsBar->QueueEventProcessing() ) {
        if ( row.armyBarGuest && row.armyBarGuest->isSelected() )
            row.armyBarGuest->ResetSelected();
        if ( row.armyBarGuard && row.armyBarGuard->isSelected() )
            row.armyBarGuard->ResetSelected();
        return true;
    }

    return false;
}

void StatsCastlesList::RedrawItem( const CstlRow & row, s32 dstx, s32 dsty, bool current )
{
    (void)current;

    if ( row.castle ) {
        Text text( "", Font::SMALL );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERVIEW, 11 ), fheroes2::Display::instance(), dstx, dsty );

        // base info
        Interface::RedrawCastleIcon( *row.castle, dstx + 17, dsty + 19 );

        const Heroes * hero = row.castle->GetHeroes().GuardFirst();

        if ( hero ) {
            Interface::RedrawHeroesIcon( *hero, dstx + 82, dsty + 19 );
            const std::string sep = "-";
            text.Set( std::to_string( hero->GetAttack() ) + sep + std::to_string( hero->GetDefense() ) + sep + std::to_string( hero->GetPower() ) + sep
                      + std::to_string( hero->GetKnowledge() ) );
            text.Blit( dstx + 104 - text.w() / 2, dsty + 43 );
        }
        else {
            row.castle->GetCaptain().PortraitRedraw( dstx + 82, dsty + 19, PORT_SMALL, fheroes2::Display::instance() );
        }

        text.Set( row.castle->GetName() );
        text.Blit( dstx + 72 - text.w() / 2, dsty + 62 );

        // army info
        if ( row.armyBarGuard ) {
            row.armyBarGuard->SetPos( dstx + 146, row.armyBarGuest ? dsty : dsty + 20 );
            row.armyBarGuard->Redraw();
        }

        if ( row.armyBarGuest ) {
            row.armyBarGuest->SetPos( dstx + 146, row.armyBarGuard ? dsty + 41 : dsty + 20 );
            row.armyBarGuest->Redraw();
        }

        row.dwellingsBar->SetPos( dstx + 349, dsty + 15 );
        row.dwellingsBar->Redraw();
    }
}

void StatsCastlesList::RedrawBackground( const fheroes2::Point & dst )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Text text( "", Font::SMALL );

    // header
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERVIEW, 7 ), display, dst.x + 30, dst.y );

    text.Set( _( "Town/Castle" ) );
    text.Blit( dst.x + 105 - text.w() / 2, dst.y + 1 );

    text.Set( _( "Garrison" ) );
    text.Blit( dst.x + 275 - text.w() / 2, dst.y + 1 );

    text.Set( _( "Available" ) );
    text.Blit( dst.x + 500 - text.w() / 2, dst.y + 1 );

    // scrollbar background
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 ), display, dst.x + 628, dst.y + 17 );

    // items background
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::OVERVIEW, 8 );
    const fheroes2::Sprite & overback = fheroes2::AGG::GetICN( ICN::OVERBACK, 0 );
    for ( int i = 0; i < VisibleItemCount(); ++i ) {
        fheroes2::Copy( back, 0, 0, display, dst.x + 30, dst.y + 17 + i * ( back.height() + 4 ), back.width(), back.height() );
        // fix bar
        fheroes2::Copy( overback, 30, 12, display, dst.x + 29, dst.y + 12 + i * ( back.height() + 4 ), 595, 6 );
    }

    // Copy one vertical line in case of previous army selection
    fheroes2::Copy( overback, 29, 12, display, dst.x + 29, dst.y + 12, 1, 357 );
}

std::string CapturedExtInfoString( int res, int color, const Funds & funds )
{
    std::ostringstream os;
    os << world.CountCapturedMines( res, color );
    const s32 vals = funds.Get( res );

    if ( vals ) {
        os << " "
           << "(";
        if ( vals > 0 )
            os << "+";
        os << vals << ")";
    }

    return os.str();
}

void RedrawIncomeInfo( const fheroes2::Point & pt, const Kingdom & myKingdom )
{
    const Funds income = myKingdom.GetIncome( INCOME_ARTIFACTS | INCOME_HEROSKILLS );
    Text text( "", Font::SMALL );

    text.Set( CapturedExtInfoString( Resource::WOOD, myKingdom.GetColor(), income ) );
    text.Blit( pt.x + 54 - text.w() / 2, pt.y + 408 );

    text.Set( CapturedExtInfoString( Resource::MERCURY, myKingdom.GetColor(), income ) );
    text.Blit( pt.x + 146 - text.w() / 2, pt.y + 408 );

    text.Set( CapturedExtInfoString( Resource::ORE, myKingdom.GetColor(), income ) );
    text.Blit( pt.x + 228 - text.w() / 2, pt.y + 408 );

    text.Set( CapturedExtInfoString( Resource::SULFUR, myKingdom.GetColor(), income ) );
    text.Blit( pt.x + 294 - text.w() / 2, pt.y + 408 );

    text.Set( CapturedExtInfoString( Resource::CRYSTAL, myKingdom.GetColor(), income ) );
    text.Blit( pt.x + 360 - text.w() / 2, pt.y + 408 );

    text.Set( CapturedExtInfoString( Resource::GEMS, myKingdom.GetColor(), income ) );
    text.Blit( pt.x + 428 - text.w() / 2, pt.y + 408 );

    text.Set( CapturedExtInfoString( Resource::GOLD, myKingdom.GetColor(), income ) );
    text.Blit( pt.x + 494 - text.w() / 2, pt.y + 408 );
}

void RedrawFundsInfo( const fheroes2::Point & pt, const Kingdom & myKingdom )
{
    const Funds & funds = myKingdom.GetFunds();
    Text text( "", Font::SMALL );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERBACK, 0 ), 4, 422, fheroes2::Display::instance(), pt.x + 4, pt.y + 422, 530, 56 );

    text.Set( std::to_string( funds.wood ) );
    text.Blit( pt.x + 56 - text.w() / 2, pt.y + 448 );

    text.Set( std::to_string( funds.mercury ) );
    text.Blit( pt.x + 146 - text.w() / 2, pt.y + 448 );

    text.Set( std::to_string( funds.ore ) );
    text.Blit( pt.x + 226 - text.w() / 2, pt.y + 448 );

    text.Set( std::to_string( funds.sulfur ) );
    text.Blit( pt.x + 294 - text.w() / 2, pt.y + 448 );

    text.Set( std::to_string( funds.crystal ) );
    text.Blit( pt.x + 362 - text.w() / 2, pt.y + 448 );

    text.Set( std::to_string( funds.gems ) );
    text.Blit( pt.x + 428 - text.w() / 2, pt.y + 448 );

    text.Set( std::to_string( funds.gold ) );
    text.Blit( pt.x + 496 - text.w() / 2, pt.y + 448 );

    text.Set( _( "Gold Per Day:" ) + std::string( " " ) + std::to_string( myKingdom.GetIncome().Get( Resource::GOLD ) ) );
    text.Blit( pt.x + 180, pt.y + 462 );

    std::string msg = _( "Day: %{day}" );
    StringReplace( msg, "%{day}", world.GetDay() );
    text.Set( msg );
    text.Blit( pt.x + 360, pt.y + 462 );

    // Show Lighthouse count
    const uint32_t lighthouseCount = world.CountCapturedObject( MP2::OBJ_LIGHTHOUSE, myKingdom.GetColor() );
    text.Set( std::to_string( lighthouseCount ) );
    text.Blit( pt.x + 105, pt.y + 462 );

    const fheroes2::Sprite & lighthouse = fheroes2::AGG::GetICN( ICN::OVERVIEW, 14 );
    fheroes2::Blit( lighthouse, 0, 0, fheroes2::Display::instance(), pt.x + 100 - lighthouse.width(), pt.y + 459, lighthouse.width(), lighthouse.height() );
}

void Kingdom::OverviewDialog( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::StandardWindow background( display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT );

    const fheroes2::Point cur_pt( background.activeArea().x, background.activeArea().y );
    fheroes2::Point dst_pt( cur_pt );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERBACK, 0 ), display, dst_pt.x, dst_pt.y );

    RedrawIncomeInfo( cur_pt, *this );
    RedrawFundsInfo( cur_pt, *this );

    StatsHeroesList listHeroes( dst_pt, heroes );
    StatsCastlesList listCastles( dst_pt, castles );

    // buttons
    dst_pt.x = cur_pt.x + 540;
    dst_pt.y = cur_pt.y + 360;
    fheroes2::Button buttonHeroes( dst_pt.x, dst_pt.y, ICN::OVERVIEW, 0, 1 );

    dst_pt.x = cur_pt.x + 540;
    dst_pt.y = cur_pt.y + 405;
    fheroes2::Button buttonCastle( dst_pt.x, dst_pt.y, ICN::OVERVIEW, 2, 3 );

    dst_pt.x = cur_pt.x + 540;
    dst_pt.y = cur_pt.y + 453;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::OVERVIEW, 4, 5 );

    const fheroes2::Rect rectIncome( cur_pt.x + 1, cur_pt.y + 360, 535, 60 );

    Interface::ListBasic * listStats = nullptr;

    // set state view: castles
    if ( Modes( OVERVIEWCSTL ) ) {
        buttonCastle.press();
        buttonHeroes.release();
        listStats = &listCastles;
    }
    else
    // set state view: heroes
    {
        buttonHeroes.press();
        buttonCastle.release();
        listStats = &listHeroes;
    }

    listStats->Redraw();

    buttonHeroes.draw();
    buttonCastle.draw();
    buttonExit.draw();

    display.render();

    LocalEvent & le = LocalEvent::Get();
    bool redraw = true;
    int worldMapRedrawMask = 0;

    // dialog menu loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        // switch view: heroes/castle
        if ( buttonHeroes.isReleased() && le.MouseClickLeft( buttonHeroes.area() ) ) {
            buttonHeroes.drawOnPress();
            buttonCastle.drawOnRelease();
            listStats = &listHeroes;
            ResetModes( OVERVIEWCSTL );
            redraw = true;
        }
        else if ( buttonCastle.isReleased() && le.MouseClickLeft( buttonCastle.area() ) ) {
            buttonCastle.drawOnPress();
            buttonHeroes.drawOnRelease();
            listStats = &listCastles;
            SetModes( OVERVIEWCSTL );
            redraw = true;
        }

        // exit event
        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) )
            break;

        redraw |= listStats->QueueEventProcessing();

        if ( le.MouseClickLeft( rectIncome ) )
            Dialog::ResourceInfo( _( "Income" ), "", GetIncome( INCOME_ALL ), Dialog::OK );
        else if ( le.MousePressRight( rectIncome ) )
            Dialog::ResourceInfo( _( "Income" ), "", GetIncome( INCOME_ALL ), 0 );

        if ( !listStats->IsNeedRedraw() && !redraw ) {
            continue;
        }

        // check if graphics in main world map window should change, this can happen in several situations:
        // - hero dismissed -> hero icon list is updated and world map focus changed
        // - hero hired -> hero icon list is updated
        // So, it's equivalent to check if hero list changed
        if ( listHeroes.Refresh( heroes ) ) {
            worldMapRedrawMask |= Interface::Basic::Get().GetRedrawMask();
            // redraw the main game window on screen, which will also erase current kingdom window
            Interface::Basic::Get().Redraw();
            // redraw Kingdom window from scratch, because it's now invalid
            background.render();
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERBACK, 0 ), display, cur_pt.x, cur_pt.y );
            buttonHeroes.draw();
            buttonCastle.draw();
            buttonExit.draw();
        }

        listStats->Redraw();
        RedrawIncomeInfo( cur_pt, *this );
        RedrawFundsInfo( cur_pt, *this );
        display.render();

        redraw = false;
    }

    if ( worldMapRedrawMask != 0 ) {
        // Force redraw of all UI elements that changed, that were masked by Kingdom window
        Interface::Basic::Get().SetRedraw( worldMapRedrawMask );
    }
}
