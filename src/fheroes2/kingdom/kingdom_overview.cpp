/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include "interface_base.h"
#include "interface_gamearea.h"
#include "interface_icons.h"
#include "interface_list.h"
#include "kingdom.h" // IWYU pragma: associated
#include "localevent.h"
#include "math_base.h"
#include "mp2.h"
#include "resource.h"
#include "screen.h"
#include "skill.h"
#include "skill_bar.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_kingdom.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    const int32_t scrollbarOffset = 626;

    bool needFadeIn{ false };

    std::string CapturedExtInfoString( const int resource, const int color, const Funds & funds )
    {
        std::string output = std::to_string( world.CountCapturedMines( resource, color ) );

        const int32_t vals = funds.Get( resource );

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

    void redrawCommonBackground( const fheroes2::Point & dst, const int visibleItems, fheroes2::Image & output )
    {
        // Scrollbar background.
        const fheroes2::Sprite & scrollbar = fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 );
        fheroes2::Copy( scrollbar, 0, 0, output, dst.x + scrollbarOffset + 1, dst.y + 17, scrollbar.width(), scrollbar.height() );

        // Items background.
        const fheroes2::Sprite & itemsBack = fheroes2::AGG::GetICN( ICN::OVERVIEW, 8 );
        const fheroes2::Sprite & overback = fheroes2::AGG::GetICN( ICN::OVERBACK, 0 );
        const int32_t itemsBackWidth = itemsBack.width();
        const int32_t itemsBackHeight = itemsBack.height();
        const int32_t offsetX = dst.x + 30;
        int32_t offsetY = dst.y + 17;
        const int32_t stepY = itemsBackHeight + 4;
        const int32_t overbackOffsetX = dst.x + 29;
        int32_t overbackOffsetY = dst.y + 13;

        for ( int i = 0; i < visibleItems; ++i, offsetY += stepY, overbackOffsetY += stepY ) {
            fheroes2::Copy( itemsBack, 0, 0, output, offsetX, offsetY, itemsBackWidth, itemsBackHeight );
            // Horizontal yellow "grid" lines.
            fheroes2::Copy( overback, 29, 13, output, overbackOffsetX, overbackOffsetY, 595, 4 );
        }

        // Copy one vertical line in case of previous army selection.
        fheroes2::Copy( overback, 29, 12, output, overbackOffsetX, dst.y + 12, 1, 357 );
    }

    struct HeroRow
    {
        Heroes * hero{ nullptr };
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
        ~HeroRow() = default;

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

            primSkillsBar = std::make_unique<PrimarySkillsBar>( hero, true, false, false );
            primSkillsBar->setTableSize( { 4, 1 } );
            primSkillsBar->setInBetweenItemsOffset( { 2, 0 } );
            primSkillsBar->SetTextOff( 20, -13 );
        }
    };

    class StatsHeroesList : public Interface::ListBox<HeroRow>
    {
    public:
        StatsHeroesList( const fheroes2::Rect & windowArea, const fheroes2::Point & offset, const VecHeroes & heroes );

        bool Refresh( VecHeroes & heroes );

        void RedrawItem( const HeroRow & row, int32_t dstx, int32_t dsty, bool current ) override;
        void RedrawBackground( const fheroes2::Point & dst ) override;

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListSingleClick( HeroRow & /* unused */ ) override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( HeroRow & /* unused */ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( HeroRow & /* unused */ ) override
        {
            // Do nothing.
        }

        void ActionListSingleClick( HeroRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy ) override;
        void ActionListDoubleClick( HeroRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy ) override;
        void ActionListPressRight( HeroRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy ) override;
        bool ActionListCursor( HeroRow & row, const fheroes2::Point & cursor ) override;

    private:
        void SetContent( const VecHeroes & heroes );

        std::vector<HeroRow> content;
        const fheroes2::Rect _windowArea;
    };

    StatsHeroesList::StatsHeroesList( const fheroes2::Rect & windowArea, const fheroes2::Point & offset, const VecHeroes & heroes )
        : Interface::ListBox<HeroRow>( offset )
        , _windowArea( windowArea )
    {
        const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 );
        const int32_t backHeight = back.height();

        SetTopLeft( offset );
        const int32_t offsetX = offset.x + scrollbarOffset;
        setScrollBarArea( { offsetX + 2, offset.y + 18, back.width(), backHeight - 2 } );

        const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::SCROLL, 4 );
        const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, backHeight - 2, 4, static_cast<int32_t>( heroes.size() ),
                                                                                   { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );

        setScrollBarImage( scrollbarSlider );
        SetScrollButtonUp( ICN::SCROLL, 0, 1, { offsetX, offset.y } );
        SetScrollButtonDn( ICN::SCROLL, 2, 3, { offsetX, offset.y + 20 + backHeight } );
        SetAreaMaxItems( 4 );
        SetAreaItems( { offset.x + 30, offset.y + 17, 594, 344 } );
        SetContent( heroes );
    }

    void StatsHeroesList::SetContent( const VecHeroes & heroes )
    {
        content.clear();
        content.reserve( heroes.size() );
        for ( Heroes * hero : heroes ) {
            content.emplace_back( hero );
        }
        SetListContent( content );
    }

    // Updates the UI list according to current list of kingdom heroes.
    // Returns true if we updated something
    bool StatsHeroesList::Refresh( VecHeroes & heroes )
    {
        const size_t contentSize = content.size();

        if ( heroes.size() != contentSize ) {
            const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 );
            const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::SCROLL, 4 );
            const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, back.height() - 2, 4, static_cast<int32_t>( heroes.size() ),
                                                                                       { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );
            setScrollBarImage( scrollbarSlider );

            SetContent( heroes );

            return true;
        }
        for ( size_t i = 0; i < contentSize; ++i ) {
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
        if ( row.hero && ( fheroes2::Rect( ox + 5, oy + 4, Interface::IconsBar::getItemWidth(), Interface::IconsBar::getItemHeight() ) & cursor ) ) {
            Game::OpenHeroesDialog( *row.hero, false, false );

            needFadeIn = true;
        }
    }

    void StatsHeroesList::ActionListPressRight( HeroRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy )
    {
        if ( row.hero && ( fheroes2::Rect( ox + 5, oy + 4, Interface::IconsBar::getItemWidth(), Interface::IconsBar::getItemHeight() ) & cursor ) ) {
            Dialog::QuickInfoWithIndicationOnRadar( *row.hero, _windowArea );
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

        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERVIEW, 10 ), display, dstx, dsty );

        // base info
        Interface::redrawHeroesIcon( *row.hero, dstx + 5, dsty + 4 );

        int32_t offsetX = dstx + 90;
        const int32_t offsetY = dsty + 22;

        fheroes2::Text text( std::to_string( row.hero->GetAttack() ), fheroes2::FontType::smallWhite() );
        text.draw( offsetX - text.width(), offsetY, display );

        offsetX += 35;
        text.set( std::to_string( row.hero->GetDefense() ), fheroes2::FontType::smallWhite() );
        text.draw( offsetX - text.width(), offsetY, display );

        offsetX += 35;
        text.set( std::to_string( row.hero->GetPower() ), fheroes2::FontType::smallWhite() );
        text.draw( offsetX - text.width(), offsetY, display );

        offsetX += 35;
        text.set( std::to_string( row.hero->GetKnowledge() ), fheroes2::FontType::smallWhite() );
        text.draw( offsetX - text.width(), offsetY, display );

        // primary skills info
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

        const fheroes2::Sprite & header = fheroes2::AGG::GetICN( ICN::OVERVIEW, 6 );
        fheroes2::Copy( header, 0, 0, display, dst.x + 30, dst.y, header.width(), header.height() );

        const int32_t offsetY = dst.y + 3;

        fheroes2::Text text( _( "Hero/Stats" ), fheroes2::FontType::smallWhite() );
        text.draw( dst.x + 130 - text.width() / 2, offsetY, display );

        text.set( _( "Skills" ), fheroes2::FontType::smallWhite() );
        text.draw( dst.x + 300 - text.width() / 2, offsetY, display );

        text.set( _( "Artifacts" ), fheroes2::FontType::smallWhite() );
        text.draw( dst.x + 500 - text.width() / 2, offsetY, display );

        redrawCommonBackground( dst, VisibleItemCount(), display );
    }

    struct CstlRow
    {
        Castle * castle{ nullptr };
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
        ~CstlRow() = default;

        void Init( Castle * ptr )
        {
            castle = ptr;

            garrisonArmyBar = std::make_unique<ArmyBar>( &castle->GetArmy(), true, false );
            garrisonArmyBar->SetBackground( { 41, 41 }, fheroes2::GetColorId( 40, 12, 0 ) );
            garrisonArmyBar->setTableSize( { 5, 1 } );
            garrisonArmyBar->setInBetweenItemsOffset( { -1, 0 } );
            garrisonArmyBar->setTroopWindowOffsetY( -60 );

            updateHeroArmyBar();

            dwellingsBar = std::make_unique<DwellingsBar>( *castle, fheroes2::Size{ 39, 52 } );
            dwellingsBar->setTableSize( { 6, 1 } );
            dwellingsBar->setInBetweenItemsOffset( { 2, 0 } );
        }

        void updateHeroArmyBar()
        {
            Heroes * hero = world.GetHero( *castle );

            if ( hero ) {
                heroArmyBar = std::make_unique<ArmyBar>( &hero->GetArmy(), true, false );
                heroArmyBar->SetBackground( { 41, 41 }, fheroes2::GetColorId( 40, 12, 0 ) );
                heroArmyBar->setTableSize( { 5, 1 } );
                heroArmyBar->setInBetweenItemsOffset( { -1, 0 } );
            }
            else {
                heroArmyBar.reset();
            }
        }
    };

    class StatsCastlesList : public Interface::ListBox<CstlRow>
    {
    public:
        StatsCastlesList( const fheroes2::Rect & windowArea, const fheroes2::Point & offset, const VecCastles & castles );

        void RedrawItem( const CstlRow & row, int32_t dstx, int32_t dsty, bool current ) override;
        void RedrawBackground( const fheroes2::Point & dst ) override;

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( CstlRow & /* unused */ ) override
        {
            // Do nothing.
        }

        void ActionListSingleClick( CstlRow & /* unused */ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( CstlRow & /* unused */ ) override
        {
            // Do nothing.
        }

        void ActionListSingleClick( CstlRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy ) override;
        void ActionListDoubleClick( CstlRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy ) override;
        void ActionListPressRight( CstlRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy ) override;
        bool ActionListCursor( CstlRow & row, const fheroes2::Point & cursor ) override;

        void updateHeroArmyBars()
        {
            for ( CstlRow & row : content ) {
                row.updateHeroArmyBar();
            }
        }

    private:
        std::vector<CstlRow> content;
        const fheroes2::Rect _windowArea;
    };

    StatsCastlesList::StatsCastlesList( const fheroes2::Rect & windowArea, const fheroes2::Point & offset, const VecCastles & castles )
        : Interface::ListBox<CstlRow>( offset )
        , _windowArea( windowArea )
    {
        const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::OVERVIEW, 13 );
        const int32_t backHeight = back.height();

        SetTopLeft( offset );
        const int32_t offsetX = offset.x + scrollbarOffset;
        setScrollBarArea( { offsetX + 2, offset.y + 18, back.width(), backHeight - 2 } );

        const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::SCROLL, 4 );
        const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, back.height() - 2, 4, static_cast<int32_t>( castles.size() ),
                                                                                   { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );

        setScrollBarImage( scrollbarSlider );
        SetScrollButtonUp( ICN::SCROLL, 0, 1, { offsetX, offset.y } );
        SetScrollButtonDn( ICN::SCROLL, 2, 3, { offsetX, offset.y + 20 + backHeight } );
        SetAreaMaxItems( 4 );
        SetAreaItems( { offset.x + 30, offset.y + 17, 594, 344 } );

        content.reserve( castles.size() );

        for ( Castle * castle : castles ) {
            content.emplace_back( castle );
        }

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
            if ( fheroes2::Rect( ox + 17, oy + 19, Interface::IconsBar::getItemWidth(), Interface::IconsBar::getItemHeight() ) & cursor ) {
                Game::OpenCastleDialog( *row.castle, false, false );
            }

            // click hero icon
            else if ( fheroes2::Rect( ox + 82, oy + 19, Interface::IconsBar::getItemWidth(), Interface::IconsBar::getItemHeight() ) & cursor ) {
                Heroes * hero = row.castle->GetHero();

                if ( !hero ) {
                    return;
                }

                Game::OpenHeroesDialog( *hero, false, false );
            }
            else {
                return;
            }

            needFadeIn = true;
        }
    }

    void StatsCastlesList::ActionListPressRight( CstlRow & row, const fheroes2::Point & cursor, int32_t ox, int32_t oy )
    {
        if ( row.castle ) {
            if ( fheroes2::Rect( ox + 17, oy + 19, Interface::IconsBar::getItemWidth(), Interface::IconsBar::getItemHeight() ) & cursor ) {
                Dialog::QuickInfoWithIndicationOnRadar( *row.castle, _windowArea );
            }
            else if ( fheroes2::Rect( ox + 82, oy + 19, Interface::IconsBar::getItemWidth(), Interface::IconsBar::getItemHeight() ) & cursor ) {
                const Heroes * hero = row.castle->GetHero();
                if ( hero ) {
                    Dialog::QuickInfoWithIndicationOnRadar( *hero, _windowArea );
                }
                else if ( row.castle->isBuild( BUILD_CAPTAIN ) ) {
                    Dialog::QuickInfoWithIndicationOnRadar( row.castle->GetCaptain(), _windowArea );
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

        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERVIEW, 11 ), display, dstx, dsty );

        // base info
        Interface::redrawCastleIcon( *row.castle, dstx + 17, dsty + 19 );

        const Heroes * hero = row.castle->GetHero();

        if ( hero ) {
            Interface::redrawHeroesIcon( *hero, dstx + 82, dsty + 19 );
            const std::string sep = "-";

            const fheroes2::Text text( std::to_string( hero->GetAttack() ) + sep + std::to_string( hero->GetDefense() ) + sep + std::to_string( hero->GetPower() ) + sep
                                           + std::to_string( hero->GetKnowledge() ),
                                       fheroes2::FontType::smallWhite() );
            text.draw( dstx + 104 - text.width() / 2, dsty + 45, display );
        }
        else if ( row.castle->GetCaptain().isValid() ) {
            const Captain & captain = row.castle->GetCaptain();
            captain.PortraitRedraw( dstx + 82, dsty + 19, PORT_SMALL, display );
            const std::string sep = "-";

            const fheroes2::Text text( std::to_string( captain.GetAttack() ) + sep + std::to_string( captain.GetDefense() ) + sep + std::to_string( captain.GetPower() )
                                           + sep + std::to_string( captain.GetKnowledge() ),
                                       fheroes2::FontType::smallWhite() );
            text.draw( dstx + 104 - text.width() / 2, dsty + 45, display );
        }

        const fheroes2::Text text( row.castle->GetName(), fheroes2::FontType::smallWhite() );
        text.draw( dstx + 72 - text.width() / 2, dsty + 63, display );

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

        const fheroes2::Sprite & header = fheroes2::AGG::GetICN( ICN::OVERVIEW, 7 );
        fheroes2::Copy( header, 0, 0, display, dst.x + 30, dst.y, header.width(), header.height() );

        const int32_t offsetY = dst.y + 3;

        fheroes2::Text text( _( "Town/Castle" ), fheroes2::FontType::smallWhite() );
        text.draw( dst.x + 105 - text.width() / 2, offsetY, display );

        text.set( _( "Garrison" ), fheroes2::FontType::smallWhite() );
        text.draw( dst.x + 275 - text.width() / 2, offsetY, display );

        text.set( _( "Available" ), fheroes2::FontType::smallWhite() );
        text.draw( dst.x + 500 - text.width() / 2, offsetY, display );

        redrawCommonBackground( dst, VisibleItemCount(), display );
    }

    void RedrawIncomeInfo( const fheroes2::Point & pt, const Kingdom & myKingdom )
    {
        const Funds income = myKingdom.GetIncome( Kingdom::INCOME_ARTIFACTS | Kingdom::INCOME_HERO_SKILLS | Kingdom::INCOME_CAMPAIGN_BONUS );
        const int32_t offsetY = pt.y + 410;
        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::Text text( CapturedExtInfoString( Resource::WOOD, myKingdom.GetColor(), income ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 54 - text.width() / 2, offsetY, display );

        text.set( CapturedExtInfoString( Resource::MERCURY, myKingdom.GetColor(), income ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 146 - text.width() / 2, offsetY, display );

        text.set( CapturedExtInfoString( Resource::ORE, myKingdom.GetColor(), income ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 228 - text.width() / 2, offsetY, display );

        text.set( CapturedExtInfoString( Resource::SULFUR, myKingdom.GetColor(), income ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 294 - text.width() / 2, offsetY, display );

        text.set( CapturedExtInfoString( Resource::CRYSTAL, myKingdom.GetColor(), income ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 360 - text.width() / 2, offsetY, display );

        text.set( CapturedExtInfoString( Resource::GEMS, myKingdom.GetColor(), income ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 428 - text.width() / 2, offsetY, display );

        text.set( CapturedExtInfoString( Resource::GOLD, myKingdom.GetColor(), income ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 494 - text.width() / 2, offsetY, display );
    }

    void RedrawFundsInfo( const fheroes2::Point & pt, const Kingdom & myKingdom )
    {
        const Funds & funds = myKingdom.GetFunds();
        int32_t offsetY = pt.y + 450;

        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::OVERBACK, 0 ), 4, 422, display, pt.x + 4, pt.y + 422, 530, 56 );

        fheroes2::Text text( std::to_string( funds.wood ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 56 - text.width() / 2, offsetY, display );

        text.set( std::to_string( funds.mercury ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 146 - text.width() / 2, offsetY, display );

        text.set( std::to_string( funds.ore ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 226 - text.width() / 2, offsetY, display );

        text.set( std::to_string( funds.sulfur ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 294 - text.width() / 2, offsetY, display );

        text.set( std::to_string( funds.crystal ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 362 - text.width() / 2, offsetY, display );

        text.set( std::to_string( funds.gems ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 428 - text.width() / 2, offsetY, display );

        text.set( std::to_string( funds.gold ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 496 - text.width() / 2, offsetY, display );

        offsetY += 14;
        text.set( _( "Gold Per Day:" ) + std::string( " " ) + std::to_string( myKingdom.GetIncome().Get( Resource::GOLD ) ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 180, offsetY, display );

        std::string msg = _( "Day: %{day}" );
        StringReplace( msg, "%{day}", world.GetDay() );
        text.set( msg, fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 360, offsetY, display );

        // Show Lighthouse count
        const uint32_t lighthouseCount = world.CountCapturedObject( MP2::OBJ_LIGHTHOUSE, myKingdom.GetColor() );
        text.set( std::to_string( lighthouseCount ), fheroes2::FontType::smallWhite() );
        text.draw( pt.x + 105, offsetY, display );

        const fheroes2::Sprite & lighthouse = fheroes2::AGG::GetICN( ICN::OVERVIEW, 14 );
        fheroes2::Blit( lighthouse, 0, 0, display, pt.x + 100 - lighthouse.width(), pt.y + 459, lighthouse.width(), lighthouse.height() );
    }
}

void Kingdom::openOverviewDialog()
{
    Game::SetUpdateSoundsOnFocusUpdate( false );

    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::StandardWindow background( display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT, false );

    // Fade-out game screen only for 640x480 resolution.
    const bool isDefaultScreenSize = display.isDefaultSize();
    if ( isDefaultScreenSize ) {
        fheroes2::fadeOutDisplay();
    }

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
    fheroes2::Button buttonHeroes( dst_pt.x, dst_pt.y, ICN::BUTTON_KINGDOM_HEROES, 0, 1 );

    // We need to additionally render the background between HEROES and TOWNS/CASTLES buttons.
    dst_pt.y += 42;
    fheroes2::Copy( fheroes2::AGG::GetICN( ICN::OVERBACK, 0 ), 540, 444, display, dst_pt.x, dst_pt.y, 99, 5 );

    dst_pt.y += 3;
    fheroes2::Button buttonCastle( dst_pt.x, dst_pt.y, ICN::BUTTON_KINGDOM_TOWNS, 0, 1 );

    dst_pt.y += 48;
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

    // Fade-in Kingdom overview dialog.
    if ( !isDefaultScreenSize ) {
        // We need to expand the ROI for the next render to properly render window borders and shadow.
        display.updateNextRenderRoi( background.totalArea() );
    }

    fheroes2::fadeInDisplay( background.activeArea(), !isDefaultScreenSize );

    LocalEvent & le = LocalEvent::Get();
    bool redraw = true;
    uint32_t worldMapRedrawMask = 0;

    // dialog menu loop
    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.isMouseRightButtonPressedInArea( buttonHeroes.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Heroes" ), _( "View Heroes." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonCastle.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Towns/Castles" ), _( "View Towns and Castles." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonExit.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Exit" ), _( "Exit this menu." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( rectIncome ) || le.isMouseRightButtonPressedInArea( rectGoldPerDay ) ) {
            fheroes2::showKingdomIncome( *this, Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( rectLighthouse ) ) {
            fheroes2::showLighthouseInfo( *this, Dialog::ZERO );
        }

        // Exit this dialog.
        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
            // Disable fast scroll for resolutions where the exit button is directly above the border.
            Interface::AdventureMap::Get().getGameArea().setFastScrollStatus( false );

            // Fade-out Kingdom overview dialog.
            fheroes2::fadeOutDisplay( background.activeArea(), !isDefaultScreenSize );
            if ( isDefaultScreenSize ) {
                Game::setDisplayFadeIn();
            }

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

        // Check if graphics in main world map window should change,
        // this can happen if hero was hired or dismissed: hero icon list is updated.
        // So, it's equivalent to check if hero list changed.
        if ( listHeroes.Refresh( heroes ) ) {
            worldMapRedrawMask |= Interface::AdventureMap::Get().getRedrawMask();
            worldMapRedrawMask |= Interface::REDRAW_HEROES;

            // Update army bars in Castles.
            listCastles.updateHeroArmyBars();
        }

        listStats->Redraw();

        RedrawIncomeInfo( cur_pt, *this );

        if ( Modes( KINGDOM_OVERVIEW_CASTLE_SELECTION ) ) {
            // Funds could be changed only after an action in the castle.
            RedrawFundsInfo( cur_pt, *this );
        }

        if ( needFadeIn ) {
            needFadeIn = false;

            fheroes2::fadeInDisplay( background.activeArea(), !isDefaultScreenSize );
        }
        else {
            display.render();
        }

        redraw = false;
    }

    _topCastleInKingdomView = listCastles.getTopId();
    _topHeroInKingdomView = listHeroes.getTopId();

    Game::SetUpdateSoundsOnFocusUpdate( true );

    Interface::AdventureMap & adventureMapInterface = Interface::AdventureMap::Get();

    if ( worldMapRedrawMask != 0 ) {
        // Force redraw of all UI elements that changed, that were masked by Kingdom window
        adventureMapInterface.setRedraw( worldMapRedrawMask );

        // Update focus because there were some changes made in the Kingdom overview dialog.
        adventureMapInterface.ResetFocus( Interface::GetFocusType(), false );
    }

    // The army of the selected hero / castle may have been changed.
    adventureMapInterface.RedrawFocus();
}
