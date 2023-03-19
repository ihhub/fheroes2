/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "agg_image.h"
#include "army_bar.h"
#include "artifact.h"
#include "buildinginfo.h"
#include "captain.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "heroes.h"
#include "heroes_base.h"
#include "icn.h"
#include "image.h"
#include "interface_icons.h"
#include "interface_list.h"
#include "kingdom.h"
#include "localevent.h"
#include "math_base.h"
#include "mp2.h"
#include "resource.h"
#include "screen.h"
#include "skill.h"
#include "skill_bar.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_kingdom.h"
#include "ui_scrollbar.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    const int32_t scrollbarOffset = 626;

    std::string CapturedExtInfoString( int res, int color, const Funds & funds )
    {
        std::string output = std::to_string( world.CountCapturedMines( res, color ) );

        const int32_t vals = funds.Get( res );

        if ( vals != 0 ) {
            output += " (";
            if ( vals > 0 ) {
                output += '+';
            }
            output += std::to_string( vals );
            output += ')';
        }

        return output;
    }
}

struct HeroRow
{
    Heroes * hero;
    std::unique_ptr<ArmyBar> armyBar;
    std::unique_ptr<ArtifactsBar> artifactsBar;
    std::unique_ptr<SecondarySkillsBar> secSkillsBar;
    std::unique_ptr<PrimarySkillsBar> primSkillsBar;

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

        armyBar = std::make_unique<ArmyBar>( &hero->GetArmy(), true, false );
        armyBar->SetBackground( { 41, 53 }, fheroes2::GetColorId( 72, 28, 0 ) );
        armyBar->setTableSize( { 5, 1 } );
        armyBar->setInBetweenItemsOffset( { -1, 0 } );
        armyBar->setTroopWindowOffsetY( -60 );

        artifactsBar = std::make_unique<ArtifactsBar>( hero, true, false, false, true, nullptr );
        artifactsBar->setTableSize( { 7, 2 } );
        artifactsBar->setInBetweenItemsOffset( { 1, 8 } );
        artifactsBar->SetContent( hero->GetBagArtifacts() );

        secSkillsBar = std::make_unique<SecondarySkillsBar>( *hero );
        secSkillsBar->setTableSize( { 4, 2 } );
        secSkillsBar->setInBetweenItemsOffset( { -1, 8 } );
        secSkillsBar->SetContent( hero->GetSecondarySkills().ToVector() );

        primSkillsBar = std::make_unique<PrimarySkillsBar>( ptr, true );
        primSkillsBar->setTableSize( { 4, 1 } );
        primSkillsBar->setInBetweenItemsOffset( { 2, 0 } );
        primSkillsBar->SetTextOff( 20, -13 );
    }
};

class StatsHeroesList : public Interface::ListBox<HeroRow>
{
public:
    StatsHeroesList( const fheroes2::Rect & windowArea, const fheroes2::Point & offset, const KingdomHeroes & heroes );

    bool Refresh( KingdomHeroes & heroes );

    void RedrawItem( const HeroRow & row, int32_t dstx, int32_t dsty, bool current ) override;
    void RedrawBackground( const fheroes2::Point & ) override;

    void ActionCurrentUp() override
    {
        // Do nothing.
    }

    void ActionCurrentDn() override
    {
        // Do nothing.
    }

    void ActionListSingleClick( HeroRow & ) override
    {
        // Do nothing.
    }

    void ActionListDoubleClick( HeroRow & ) override
    {
        // Do nothing.
    }

    void ActionListPressRight( HeroRow & ) override
    {
        // Do nothing.
    }

    void ActionListSingleClick( HeroRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy ) override;
    void ActionListDoubleClick( HeroRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy ) override;
    void ActionListPressRight( HeroRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy ) override;
    bool ActionListCursor( HeroRow & row, const fheroes2::Point & cursor ) override;

private:
    void SetContent( const KingdomHeroes & heroes );

    std::vector<HeroRow> content;
    const fheroes2::Rect _windowArea;
};

StatsHeroesList::StatsHeroesList( const fheroes2::Rect & windowArea, const fheroes2::Point & offset, const KingdomHeroes & heroes )
    : Interface::ListBox<HeroRow>( offset )
    , _windowArea( windowArea )
{
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 );

    SetTopLeft( offset );
    setScrollBarArea( { offset.x + scrollbarOffset + 2, offset.y + 18, back.width(), back.height() - 2 } );

    const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::SCROLL, 4 );
    const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, back.height() - 2, 4, static_cast<int32_t>( heroes.size() ),
                                                                               { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );

    setScrollBarImage( scrollbarSlider );
    SetScrollButtonUp( ICN::SCROLL, 0, 1, { offset.x + scrollbarOffset, offset.y } );
    SetScrollButtonDn( ICN::SCROLL, 2, 3, { offset.x + scrollbarOffset, offset.y + 20 + back.height() } );
    SetAreaMaxItems( 4 );
    SetAreaItems( { offset.x + 30, offset.y + 17, 594, 344 } );
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
        const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 );
        const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::SCROLL, 4 );
        const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, back.height() - 2, 4, static_cast<int32_t>( heroes.size() ),
                                                                                   { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );
        setScrollBarImage( scrollbarSlider );

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

void StatsHeroesList::ActionListDoubleClick( HeroRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy )
{
    ActionListSingleClick( row, cursor, ox, oy );
}

void StatsHeroesList::ActionListSingleClick( HeroRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy )
{
    if ( row.hero && ( fheroes2::Rect( ox + 5, oy + 4, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight() ) & cursor ) )
        Game::OpenHeroesDialog( *row.hero, false, false );
}

void StatsHeroesList::ActionListPressRight( HeroRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy )
{
    if ( row.hero && ( fheroes2::Rect( ox + 5, oy + 4, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight() ) & cursor ) ) {
        Dialog::QuickInfo( *row.hero, {}, true, _windowArea );
    }
}

bool StatsHeroesList::ActionListCursor( HeroRow & row, const fheroes2::Point & cursor )
{
    const fheroes2::Point cursorPos( cursor.x, cursor.y );

    if ( ( row.armyBar->GetArea() & cursorPos ) && row.armyBar->QueueEventProcessing() ) {
        if ( row.artifactsBar->isSelected() ) {
            row.artifactsBar->ResetSelected();
        }
        return true;
    }

    if ( ( row.artifactsBar->GetArea() & cursorPos ) && row.artifactsBar->QueueEventProcessing() ) {
        if ( row.armyBar->isSelected() ) {
            row.armyBar->ResetSelected();
        }
        return true;
    }

    if ( ( row.primSkillsBar->GetArea() & cursorPos ) && row.primSkillsBar->QueueEventProcessing() ) {
        return true;
    }

    if ( ( row.secSkillsBar->GetArea() & cursorPos ) && row.secSkillsBar->QueueEventProcessing() ) {
        return true;
    }

    return false;
}

void StatsHeroesList::RedrawItem( const HeroRow & row, int32_t dstx, int32_t dsty, bool current )
{
    (void)current;

    if ( row.hero == nullptr ) {
        // No hero to draw.
        return;
    }

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
    fheroes2::Display & display = fheroes2::Display::instance();

    row.primSkillsBar->setRenderingOffset( { dstx + 56, dsty - 3 } );
    row.primSkillsBar->Redraw( display );

    // secondary skills info
    row.secSkillsBar->setRenderingOffset( { dstx + 206, dsty + 3 } );
    row.secSkillsBar->Redraw( display );

    // artifacts info
    row.artifactsBar->setRenderingOffset( { dstx + 348, dsty + 3 } );
    row.artifactsBar->Redraw( display );

    // army info
    row.armyBar->setRenderingOffset( { dstx - 1, dsty + 30 } );
    row.armyBar->Redraw( display );
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
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 ), display, dst.x + scrollbarOffset + 1, dst.y + 17 );

    // items background
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::OVERVIEW, 8 );
    for ( int ii = 0; ii < VisibleItemCount(); ++ii ) {
        fheroes2::Blit( back, display, dst.x + 30, dst.y + 17 + ii * ( back.height() + 4 ) );
    }
}

struct CstlRow
{
    Castle * castle;
    std::unique_ptr<ArmyBar> garrisonArmyBar;
    std::unique_ptr<ArmyBar> heroArmyBar;
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

        garrisonArmyBar = std::make_unique<ArmyBar>( &castle->GetArmy(), true, false );
        garrisonArmyBar->SetBackground( { 41, 41 }, fill );
        garrisonArmyBar->setTableSize( { 5, 1 } );
        garrisonArmyBar->setInBetweenItemsOffset( { -1, 0 } );
        garrisonArmyBar->setTroopWindowOffsetY( -60 );

        Heroes * hero = world.GetHero( *castle );

        if ( hero ) {
            heroArmyBar = std::make_unique<ArmyBar>( &hero->GetArmy(), true, false );
            heroArmyBar->SetBackground( { 41, 41 }, fill );
            heroArmyBar->setTableSize( { 5, 1 } );
            heroArmyBar->setInBetweenItemsOffset( { -1, 0 } );
        }
        else {
            heroArmyBar.reset();
        }

        dwellingsBar = std::make_unique<DwellingsBar>( *castle, fheroes2::Size{ 39, 52 } );
        dwellingsBar->setTableSize( { 6, 1 } );
        dwellingsBar->setInBetweenItemsOffset( { 2, 0 } );
    }
};

class StatsCastlesList : public Interface::ListBox<CstlRow>
{
public:
    StatsCastlesList( const fheroes2::Rect & windowArea, const fheroes2::Point & offset, const KingdomCastles & castles );

    void RedrawItem( const CstlRow & row, int32_t dstx, int32_t dsty, bool current ) override;
    void RedrawBackground( const fheroes2::Point & ) override;

    void ActionCurrentUp() override
    {
        // Do nothing.
    }

    void ActionCurrentDn() override
    {
        // Do nothing.
    }

    void ActionListDoubleClick( CstlRow & ) override
    {
        // Do nothing.
    }

    void ActionListSingleClick( CstlRow & ) override
    {
        // Do nothing.
    }

    void ActionListPressRight( CstlRow & ) override
    {
        // Do nothing.
    }

    void ActionListSingleClick( CstlRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy ) override;
    void ActionListDoubleClick( CstlRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy ) override;
    void ActionListPressRight( CstlRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy ) override;
    bool ActionListCursor( CstlRow &, const fheroes2::Point & ) override;

private:
    std::vector<CstlRow> content;
    const fheroes2::Rect _windowArea;
};

StatsCastlesList::StatsCastlesList( const fheroes2::Rect & windowArea, const fheroes2::Point & offset, const KingdomCastles & castles )
    : Interface::ListBox<CstlRow>( offset )
    , _windowArea( windowArea )
{
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 );

    SetTopLeft( offset );
    setScrollBarArea( { offset.x + scrollbarOffset + 2, offset.y + 18, back.width(), back.height() - 2 } );

    const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::SCROLL, 4 );
    const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, back.height() - 2, 4, static_cast<int32_t>( castles.size() ),
                                                                               { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );

    setScrollBarImage( scrollbarSlider );
    SetScrollButtonUp( ICN::SCROLL, 0, 1, { offset.x + scrollbarOffset, offset.y } );
    SetScrollButtonDn( ICN::SCROLL, 2, 3, { offset.x + scrollbarOffset, offset.y + 20 + back.height() } );
    SetAreaMaxItems( 4 );
    SetAreaItems( { offset.x + 30, offset.y + 17, 594, 344 } );

    content.reserve( castles.size() );

    for ( Castle * castle : castles )
        content.emplace_back( castle );

    SetListContent( content );
}

void StatsCastlesList::ActionListDoubleClick( CstlRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy )
{
    ActionListSingleClick( row, cursor, ox, oy );
}

void StatsCastlesList::ActionListSingleClick( CstlRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy )
{
    if ( row.castle ) {
        // click castle icon
        if ( fheroes2::Rect( ox + 17, oy + 19, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight() ) & cursor ) {
            Game::OpenCastleDialog( *row.castle, false );
            row.Init( row.castle );
        }
        // click hero icon
        else if ( fheroes2::Rect( ox + 82, oy + 19, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight() ) & cursor ) {
            Heroes * hero = row.castle->GetHero();
            if ( hero ) {
                Game::OpenHeroesDialog( *hero, false, false );
                row.Init( row.castle );
            }
        }
    }
}

void StatsCastlesList::ActionListPressRight( CstlRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy )
{
    if ( row.castle ) {
        if ( fheroes2::Rect( ox + 17, oy + 19, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight() ) & cursor ) {
            Dialog::QuickInfo( *row.castle, {}, true, _windowArea );
        }
        else if ( fheroes2::Rect( ox + 82, oy + 19, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight() ) & cursor ) {
            const Heroes * hero = row.castle->GetHero();
            if ( hero ) {
                Dialog::QuickInfo( *hero, {}, true, _windowArea );
            }
            else if ( row.castle->isBuild( BUILD_CAPTAIN ) ) {
                Dialog::QuickInfo( row.castle->GetCaptain(), {}, true, _windowArea );
            }
        }
    }
}

bool StatsCastlesList::ActionListCursor( CstlRow & row, const fheroes2::Point & cursor )
{
    const fheroes2::Point cursorPos( cursor.x, cursor.y );

    if ( row.garrisonArmyBar && ( row.garrisonArmyBar->GetArea() & cursorPos )
         && ( row.heroArmyBar ? row.garrisonArmyBar->QueueEventProcessing( *row.heroArmyBar ) : row.garrisonArmyBar->QueueEventProcessing() ) ) {
        if ( row.heroArmyBar && row.heroArmyBar->isSelected() ) {
            row.heroArmyBar->ResetSelected();
        }
        return true;
    }

    if ( row.heroArmyBar && ( row.heroArmyBar->GetArea() & cursorPos )
         && ( row.garrisonArmyBar ? row.heroArmyBar->QueueEventProcessing( *row.garrisonArmyBar ) : row.heroArmyBar->QueueEventProcessing() ) ) {
        if ( row.garrisonArmyBar && row.garrisonArmyBar->isSelected() ) {
            row.garrisonArmyBar->ResetSelected();
        }
        return true;
    }

    if ( row.dwellingsBar && ( row.dwellingsBar->GetArea() & cursorPos ) && row.dwellingsBar->QueueEventProcessing() ) {
        if ( row.heroArmyBar && row.heroArmyBar->isSelected() ) {
            row.heroArmyBar->ResetSelected();
        }
        if ( row.garrisonArmyBar && row.garrisonArmyBar->isSelected() ) {
            row.garrisonArmyBar->ResetSelected();
        }
        return true;
    }

    return false;
}

void StatsCastlesList::RedrawItem( const CstlRow & row, int32_t dstx, int32_t dsty, bool current )
{
    (void)current;

    if ( row.castle == nullptr ) {
        // No castle to draw.
        return;
    }

    Text text( "", Font::SMALL );
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERVIEW, 11 ), fheroes2::Display::instance(), dstx, dsty );

    // base info
    Interface::RedrawCastleIcon( *row.castle, dstx + 17, dsty + 19 );

    const Heroes * hero = row.castle->GetHero();

    if ( hero ) {
        Interface::RedrawHeroesIcon( *hero, dstx + 82, dsty + 19 );
        const std::string sep = "-";
        text.Set( std::to_string( hero->GetAttack() ) + sep + std::to_string( hero->GetDefense() ) + sep + std::to_string( hero->GetPower() ) + sep
                  + std::to_string( hero->GetKnowledge() ) );
        text.Blit( dstx + 104 - text.w() / 2, dsty + 43 );
    }
    else if ( row.castle->GetCaptain().isValid() ) {
        const Captain & captain = row.castle->GetCaptain();
        captain.PortraitRedraw( dstx + 82, dsty + 19, PORT_SMALL, fheroes2::Display::instance() );
        const std::string sep = "-";
        text.Set( std::to_string( captain.GetAttack() ) + sep + std::to_string( captain.GetDefense() ) + sep + std::to_string( captain.GetPower() ) + sep
                  + std::to_string( captain.GetKnowledge() ) );
        text.Blit( dstx + 104 - text.w() / 2, dsty + 43 );
    }

    text.Set( row.castle->GetName() );
    text.Blit( dstx + 72 - text.w() / 2, dsty + 62 );

    fheroes2::Display & display = fheroes2::Display::instance();

    // army info
    if ( row.garrisonArmyBar ) {
        row.garrisonArmyBar->setRenderingOffset( { dstx + 146, row.heroArmyBar ? dsty : dsty + 20 } );
        row.garrisonArmyBar->Redraw( display );
    }

    if ( row.heroArmyBar ) {
        row.heroArmyBar->setRenderingOffset( { dstx + 146, row.garrisonArmyBar ? dsty + 41 : dsty + 20 } );
        row.heroArmyBar->Redraw( display );
    }

    row.dwellingsBar->setRenderingOffset( { dstx + 349, dsty + 15 } );
    row.dwellingsBar->Redraw( display );
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
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 ), display, dst.x + scrollbarOffset + 1, dst.y + 17 );

    // items background
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::OVERVIEW, 8 );
    const fheroes2::Sprite & overback = fheroes2::AGG::GetICN( ICN::OVERBACK, 0 );
    for ( int i = 0; i < VisibleItemCount(); ++i ) {
        fheroes2::Copy( back, 0, 0, display, dst.x + 30, dst.y + 17 + i * ( back.height() + 4 ), back.width(), back.height() );
        // fix bar
        fheroes2::Copy( overback, 29, 13, display, dst.x + 29, dst.y + 13 + i * ( back.height() + 4 ), 595, 4 );
    }

    // Copy one vertical line in case of previous army selection
    fheroes2::Copy( overback, 29, 12, display, dst.x + 29, dst.y + 12, 1, 357 );
}

void RedrawIncomeInfo( const fheroes2::Point & pt, const Kingdom & myKingdom )
{
    const Funds income = myKingdom.GetIncome( Kingdom::INCOME_ARTIFACTS | Kingdom::INCOME_HERO_SKILLS | Kingdom::INCOME_CAMPAIGN_BONUS );
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

void Kingdom::openOverviewDialog()
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::StandardWindow background( display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT, false );

    const fheroes2::Point cur_pt( background.activeArea().x, background.activeArea().y );
    fheroes2::Point dst_pt( cur_pt );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERBACK, 0 ), display, dst_pt.x, dst_pt.y );

    RedrawIncomeInfo( cur_pt, *this );
    RedrawFundsInfo( cur_pt, *this );

    StatsHeroesList listHeroes( background.windowArea(), dst_pt, heroes );
    StatsCastlesList listCastles( background.windowArea(), dst_pt, castles );

    // buttons
    dst_pt.x = cur_pt.x + 540;
    dst_pt.y = cur_pt.y + 360;
    fheroes2::Button buttonHeroes( dst_pt.x, dst_pt.y, ICN::OVERVIEW, 0, 1 );

    // We need to additionally render the background between HEROES and TOWNS/CASTLES buttons.
    dst_pt.y += 42;
    fheroes2::Copy( fheroes2::AGG::GetICN( ICN::OVERBACK, 0 ), 540, 444, display, dst_pt.x, dst_pt.y, 99, 5 );

    dst_pt.y = cur_pt.y + 405;
    fheroes2::Button buttonCastle( dst_pt.x, dst_pt.y, ICN::OVERVIEW, 2, 3 );

    dst_pt.y = cur_pt.y + 453;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::BUTTON_KINGDOM_EXIT, 0, 1 );

    const fheroes2::Rect rectIncome( cur_pt.x + 1, cur_pt.y + 360, 535, 60 );
    const fheroes2::Rect rectGoldPerDay( cur_pt.x + 124, cur_pt.y + 459, 289, 16 );

    const fheroes2::Sprite & lighthouse = fheroes2::AGG::GetICN( ICN::OVERVIEW, 14 );
    const fheroes2::Rect rectLighthouse( cur_pt.x + 100 - lighthouse.width(), cur_pt.y + 459, lighthouse.width() + 10, lighthouse.height() );

    Interface::ListBasic * listStats = nullptr;

    if ( Modes( KINGDOM_OVERVIEW_CASTLE_SELECTION ) ) {
        // set state view: castles
        buttonCastle.press();
        buttonHeroes.release();

        listStats = &listCastles;
    }
    else {
        // set state view: heroes
        buttonHeroes.press();
        buttonCastle.release();

        listStats = &listHeroes;
    }

    listCastles.setTopVisibleItem( _topCastleInKingdomView );
    listHeroes.setTopVisibleItem( _topHeroInKingdomView );

    listStats->Redraw();

    buttonHeroes.draw();
    buttonCastle.draw();
    buttonExit.draw();

    display.render();

    LocalEvent & le = LocalEvent::Get();
    bool redraw = true;
    uint32_t worldMapRedrawMask = 0;

    // dialog menu loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MousePressRight( buttonHeroes.area() ) ) {
            Dialog::Message( _( "Heroes" ), _( "View Heroes." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonCastle.area() ) ) {
            Dialog::Message( _( "Towns/Castles" ), _( "View Towns and Castles." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonExit.area() ) ) {
            Dialog::Message( _( "Exit" ), _( "Exit this menu." ), Font::BIG );
        }
        else if ( le.MousePressRight( rectIncome ) || le.MousePressRight( rectGoldPerDay ) ) {
            fheroes2::showKingdomIncome( *this, Dialog::ZERO );
        }
        else if ( le.MousePressRight( rectLighthouse ) ) {
            fheroes2::showLighthouseInfo( *this, Dialog::ZERO );
        }

        // Exit this dialog.
        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
            break;
        }

        // switch view: heroes/castle
        if ( buttonHeroes.isReleased() && le.MouseClickLeft( buttonHeroes.area() ) ) {
            buttonHeroes.drawOnPress();
            buttonCastle.drawOnRelease();
            listStats = &listHeroes;
            ResetModes( KINGDOM_OVERVIEW_CASTLE_SELECTION );
            redraw = true;
        }
        else if ( buttonCastle.isReleased() && le.MouseClickLeft( buttonCastle.area() ) ) {
            buttonCastle.drawOnPress();
            buttonHeroes.drawOnRelease();
            listStats = &listCastles;
            SetModes( KINGDOM_OVERVIEW_CASTLE_SELECTION );
            redraw = true;
        }

        redraw |= listStats->QueueEventProcessing();

        if ( le.MouseClickLeft( rectIncome ) || le.MouseClickLeft( rectGoldPerDay ) ) {
            fheroes2::showKingdomIncome( *this, Dialog::OK );
        }

        if ( le.MouseClickLeft( rectLighthouse ) ) {
            fheroes2::showLighthouseInfo( *this, Dialog::OK );
        }

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

    _topCastleInKingdomView = listCastles.getTopId();
    _topHeroInKingdomView = listHeroes.getTopId();

    if ( worldMapRedrawMask != 0 ) {
        // Force redraw of all UI elements that changed, that were masked by Kingdom window
        Interface::Basic::Get().SetRedraw( worldMapRedrawMask );
    }
}
