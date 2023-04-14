/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "agg_image.h"
#include "army.h"
#include "army_troop.h"
#include "audio_manager.h"
#include "castle.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "direction.h"
#include "game_interface.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "interface_gamearea.h"
#include "interface_icons.h"
#include "interface_list.h"
#include "interface_radar.h"
#include "kingdom.h"
#include "localevent.h"
#include "logging.h"
#include "m82.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "monster.h"
#include "mp2.h"
#include "payment.h"
#include "route.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "spell_info.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_scrollbar.h"
#include "ui_window.h"
#include "view_world.h"
#include "world.h"

namespace
{
    class CastleIndexListBox : public Interface::ListBox<int32_t>
    {
    public:
        using Interface::ListBox<int32_t>::ActionListDoubleClick;
        using Interface::ListBox<int32_t>::ActionListSingleClick;
        using Interface::ListBox<int32_t>::ActionListPressRight;

        CastleIndexListBox( const fheroes2::Rect & windowArea, const fheroes2::Point & offset, int & res, const int townFrameIcnId, const int listBoxIcnId )
            : Interface::ListBox<int32_t>( offset )
            , result( res )
            , _townFrameIcnId( townFrameIcnId )
            , _listBoxIcnId( listBoxIcnId )
            , _windowArea( windowArea )
        {}

        void RedrawItem( const int32_t & index, int32_t dstx, int32_t dsty, bool current ) override;
        void RedrawBackground( const fheroes2::Point & dst ) override;

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( int32_t & /* unused */ ) override
        {
            result = Dialog::OK;
        }

        void ActionListSingleClick( int32_t & /* unused */ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( int32_t & index ) override
        {
            const Castle * castle = world.getCastleEntrance( Maps::GetPoint( index ) );

            if ( castle != nullptr ) {
                Dialog::QuickInfo( *castle, {}, true, _windowArea );
            }
        }

        int & result;

    private:
        int _townFrameIcnId;
        int _listBoxIcnId;
        const fheroes2::Rect _windowArea;
    };

    void CastleIndexListBox::RedrawItem( const int32_t & index, int32_t dstx, int32_t dsty, bool current )
    {
        const Castle * castle = world.getCastleEntrance( Maps::GetPoint( index ) );

        if ( castle ) {
            fheroes2::Blit( fheroes2::AGG::GetICN( _townFrameIcnId, 0 ), 481, 177, fheroes2::Display::instance(), dstx, dsty, 54, 30 );
            Interface::RedrawCastleIcon( *castle, dstx + 4, dsty + 4 );
            Text text( castle->GetName(), ( current ? Font::YELLOW_BIG : Font::BIG ) );

            if ( VisibleItemCount() > 0 ) {
                const int32_t heightPerItem = ( rtAreaItems.height - VisibleItemCount() ) / VisibleItemCount();
                text.Blit( dstx + 60, dsty + ( heightPerItem - text.h() ) / 2, 196 );
            }
            else {
                assert( 0 ); // this should never happen!
                text.Blit( dstx + 60, dsty, 196 );
            }
        }
    }

    void CastleIndexListBox::RedrawBackground( const fheroes2::Point & dst )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        Text text( _( "Town Portal" ), Font::YELLOW_BIG );
        text.Blit( dst.x + 145 - text.w() / 2, dst.y + 5 );

        text.Set( _( "Select town to port to." ), Font::BIG );
        text.Blit( dst.x + 145 - text.w() / 2, dst.y + 25 );

        const fheroes2::Sprite & upperPart = fheroes2::AGG::GetICN( _listBoxIcnId, 0 );
        const fheroes2::Sprite & middlePart = fheroes2::AGG::GetICN( _listBoxIcnId, 1 );
        const fheroes2::Sprite & lowerPart = fheroes2::AGG::GetICN( _listBoxIcnId, 2 );

        int32_t offsetY = 45;
        fheroes2::Blit( upperPart, display, dst.x + 7, dst.y + offsetY );

        offsetY += upperPart.height();

        int32_t totalHeight = rtAreaItems.height + 6;
        int32_t middlePartCount = ( totalHeight - upperPart.height() - lowerPart.height() + middlePart.height() - 1 ) / middlePart.height();

        for ( int32_t i = 0; i < middlePartCount; ++i ) {
            fheroes2::Blit( fheroes2::AGG::GetICN( _listBoxIcnId, 1 ), display, dst.x + 7, dst.y + offsetY );
            offsetY += middlePart.height();
        }

        fheroes2::Blit( lowerPart, display, dst.x + 7, dst.y + totalHeight - lowerPart.height() + 45 );

        const fheroes2::Sprite & upperScrollbarArrow = fheroes2::AGG::GetICN( _listBoxIcnId, 3 );
        const fheroes2::Sprite & lowerScrollbarArrow = fheroes2::AGG::GetICN( _listBoxIcnId, 5 );

        totalHeight = rtAreaItems.height + 8 - upperScrollbarArrow.height() - lowerScrollbarArrow.height();

        const fheroes2::Sprite & upperScrollbar = fheroes2::AGG::GetICN( _listBoxIcnId, 7 );
        const fheroes2::Sprite & middleScrollbar = fheroes2::AGG::GetICN( _listBoxIcnId, 8 );
        const fheroes2::Sprite & lowerScrollbar = fheroes2::AGG::GetICN( _listBoxIcnId, 9 );

        offsetY = upperScrollbarArrow.height() + 44;
        fheroes2::Blit( upperScrollbar, display, dst.x + 262, dst.y + offsetY );
        offsetY += upperScrollbar.height();

        middlePartCount = ( totalHeight - upperScrollbar.height() - lowerScrollbar.height() + middleScrollbar.height() - 1 ) / middleScrollbar.height();

        for ( int32_t i = 0; i < middlePartCount; ++i ) {
            fheroes2::Blit( middleScrollbar, display, dst.x + 262, dst.y + offsetY );
            offsetY += middleScrollbar.height();
        }

        offsetY = upperScrollbarArrow.height() + 44 + totalHeight - lowerScrollbar.height();
        fheroes2::Blit( lowerScrollbar, display, dst.x + 262, dst.y + offsetY );
    }

    void DialogSpellFailed( const Spell & spell )
    {
        std::string str = _( "%{spell} failed!!!" );
        StringReplace( str, "%{spell}", spell.GetName() );
        Dialog::Message( "", str, Font::BIG, Dialog::OK );
    }

    void HeroesTownGate( Heroes & hero, const Castle * castle )
    {
        assert( castle != nullptr );

        Interface::Basic & I = Interface::Basic::Get();

        const fheroes2::Point fromPosition = hero.GetCenter();
        // Position of Hero on radar before casting the spell to clear it after casting.
        const fheroes2::Rect fromRoi( fromPosition.x, fromPosition.y, 1, 1 );

        // Before casting the spell, make sure that the game area is centered on the hero
        I.GetGameArea().SetCenter( fromPosition );
        I.Redraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR_CURSOR );

        const int32_t dst = castle->GetIndex();
        assert( Maps::isValidAbsIndex( dst ) );

        AudioManager::PlaySound( M82::KILLFADE );
        hero.GetPath().Hide();
        hero.FadeOut();

        hero.Move2Dest( dst );

        // Clear previous hero position on radar.
        I.GetRadar().SetRenderArea( fromRoi );

        I.Redraw( Interface::REDRAW_RADAR );

        I.GetGameArea().SetCenter( hero.GetCenter() );

        // Update radar image in scout area around Hero after teleport.
        I.GetRadar().SetRenderArea( hero.GetScoutRoi() );
        I.SetRedraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );

        AudioManager::PlaySound( M82::KILLFADE );
        hero.FadeIn();
        hero.GetPath().Reset();
        // Path::Reset() puts the hero's path into the hidden mode, we have to make it visible again
        hero.GetPath().Show();

        castle->MageGuildEducateHero( hero );
    }

    bool ActionSpellViewMines()
    {
        ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::ViewMines, Interface::Basic::Get() );
        return true;
    }

    bool ActionSpellViewResources()
    {
        ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::ViewResources, Interface::Basic::Get() );
        return true;
    }

    bool ActionSpellViewArtifacts()
    {
        ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::ViewArtifacts, Interface::Basic::Get() );
        return true;
    }

    bool ActionSpellViewTowns()
    {
        ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::ViewTowns, Interface::Basic::Get() );
        return true;
    }

    bool ActionSpellViewHeroes()
    {
        ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::ViewHeroes, Interface::Basic::Get() );
        return true;
    }

    bool ActionSpellViewAll()
    {
        ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::ViewAll, Interface::Basic::Get() );
        return true;
    }

    bool ActionSpellIdentifyHero( const Heroes & hero )
    {
        if ( hero.GetKingdom().Modes( Kingdom::IDENTIFYHERO ) ) {
            Message( "", _( "This spell is already in use." ), Font::BIG, Dialog::OK );
            return false;
        }

        hero.GetKingdom().SetModes( Kingdom::IDENTIFYHERO );
        Message( "", _( "Enemy heroes are now fully identifiable." ), Font::BIG, Dialog::OK );

        return true;
    }

    bool ActionSpellSummonBoat( const Heroes & hero )
    {
        assert( !hero.isShipMaster() );

        const int32_t center = hero.GetIndex();
        const int32_t boatDestination = fheroes2::getPossibleBoatPosition( hero );
        assert( Maps::isValidAbsIndex( boatDestination ) );

        for ( const int32_t boatSource : Maps::GetObjectPositions( center, MP2::OBJ_BOAT, false ) ) {
            assert( Maps::isValidAbsIndex( boatSource ) );

            Maps::Tiles & tileSource = world.GetTiles( boatSource );
            const int boatColor = tileSource.getBoatOwnerColor();
            const int heroColor = hero.GetColor();

            if ( boatColor != Color::NONE && boatColor != heroColor ) {
                continue;
            }

            const uint32_t distance = Maps::GetStraightLineDistance( boatSource, hero.GetIndex() );

            if ( distance > 1 ) {
                Interface::GameArea & gameArea = Interface::Basic::Get().GetGameArea();
                gameArea.runSingleObjectAnimation( std::make_shared<Interface::ObjectFadingOutInfo>( tileSource.GetObjectUID(), boatSource, MP2::OBJ_BOAT ) );

                Maps::Tiles & tileDest = world.GetTiles( boatDestination );
                tileDest.setBoat( Direction::RIGHT, heroColor );
                tileSource.resetBoatOwnerColor();

                gameArea.runSingleObjectAnimation( std::make_shared<Interface::ObjectFadingInInfo>( tileDest.GetObjectUID(), boatDestination, MP2::OBJ_BOAT ) );

                return true;
            }
        }

        DialogSpellFailed( Spell::SUMMONBOAT );
        return true;
    }

    bool ActionSpellDimensionDoor( Heroes & hero )
    {
        Interface::Basic & I = Interface::Basic::Get();

        const fheroes2::Point fromPosition = hero.GetCenter();
        // Position of Hero on radar before casting the spell to clear it after casting.
        const fheroes2::Rect fromRoi( fromPosition.x, fromPosition.y, 1, 1 );

        // Before casting the spell, make sure that the game area is centered on the hero
        I.GetGameArea().SetCenter( hero.GetCenter() );
        I.Redraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR_CURSOR );

        const int32_t src = hero.GetIndex();
        assert( Maps::isValidAbsIndex( src ) );

        const int32_t dst = I.GetDimensionDoorDestination( src, Spell::CalculateDimensionDoorDistance(), hero.isShipMaster() );
        if ( !Maps::isValidAbsIndex( dst ) ) {
            return false;
        }

        AudioManager::PlaySound( M82::KILLFADE );
        hero.GetPath().Hide();
        hero.FadeOut();

        hero.SpellCasted( Spell::DIMENSIONDOOR );

        hero.Move2Dest( dst );

        // Clear previous hero position on radar.
        I.GetRadar().SetRenderArea( fromRoi );

        I.Redraw( Interface::REDRAW_RADAR );

        I.GetGameArea().SetCenter( hero.GetCenter() );

        // Update radar image in scout area around Hero after teleport.
        I.GetRadar().SetRenderArea( hero.GetScoutRoi() );
        I.SetRedraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );

        AudioManager::PlaySound( M82::KILLFADE );
        hero.FadeIn();
        hero.GetPath().Reset();
        // Path::Reset() puts the hero's path into the hidden mode, we have to make it visible again
        hero.GetPath().Show();
        hero.ActionNewPosition( false );

        // SpellCasted() has already been called, we should not call it once again
        return false;
    }

    bool ActionSpellTownGate( Heroes & hero )
    {
        assert( !hero.isShipMaster() );

        const Castle * castle = fheroes2::getNearestCastleTownGate( hero );
        if ( !castle ) {
            // A hero must be able to have a destination castle. Something is wrong with the logic!
            assert( 0 );
            return false;
        }

        if ( castle->GetHero() && castle->GetHero() != &hero ) {
            // The nearest town occupation must be checked before casting this spell. Something is wrong with the logic!
            assert( 0 );
            return false;
        }

        HeroesTownGate( hero, castle );

        return true;
    }

    bool ActionSpellTownPortal( Heroes & hero )
    {
        assert( !hero.isShipMaster() );

        const Kingdom & kingdom = hero.GetKingdom();
        std::vector<int32_t> castles;

        fheroes2::Display & display = fheroes2::Display::instance();

        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        LocalEvent & le = LocalEvent::Get();

        for ( const Castle * castle : kingdom.GetCastles() ) {
            assert( castle != nullptr );

            if ( !castle->GetHero() ) {
                castles.push_back( castle->GetIndex() );
            }
        }

        if ( castles.empty() ) {
            // This should never happen. The logic behind this must not allow to call this function.
            assert( 0 );
            return false;
        }

        std::unique_ptr<fheroes2::StandardWindow> frameborder = std::make_unique<fheroes2::StandardWindow>( 290, 252, true );
        const fheroes2::Rect & windowArea = frameborder->windowArea();
        const fheroes2::Rect & activeArea = frameborder->activeArea();

        int result = Dialog::ZERO;

        const int townIcnId = isEvilInterface ? ICN::ADVBORDE : ICN::ADVBORD;
        const int listIcnId = isEvilInterface ? ICN::LISTBOX_EVIL : ICN::LISTBOX;
        CastleIndexListBox listbox( windowArea, activeArea.getPosition(), result, townIcnId, listIcnId );

        listbox.SetScrollButtonUp( listIcnId, 3, 4, { activeArea.x + 262, activeArea.y + 45 } );
        listbox.SetScrollButtonDn( listIcnId, 5, 6, { activeArea.x + 262, activeArea.y + 190 } );
        listbox.setScrollBarArea( { activeArea.x + 266, activeArea.y + 68, 14, 119 } );

        const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( listIcnId, 10 );
        const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, 119, 5, static_cast<int32_t>( castles.size() ),
                                                                                   { 0, 0, originalSlider.width(), 4 }, { 0, 4, originalSlider.width(), 8 } );

        listbox.setScrollBarImage( scrollbarSlider );
        listbox.SetAreaMaxItems( 5 );
        listbox.SetAreaItems( { activeArea.x + 11, activeArea.y + 49, 250, 160 } );
        listbox.SetListContent( castles );
        listbox.Unselect();
        listbox.RedrawBackground( activeArea.getPosition() );
        listbox.Redraw();

        const int okIcnId = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        const int cancelIcnId = isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD;
        const fheroes2::Sprite & buttonOkSprite = fheroes2::AGG::GetICN( okIcnId, 0 );
        const fheroes2::Sprite & buttonCancelSprite = fheroes2::AGG::GetICN( cancelIcnId, 0 );

        const int32_t border = 10;
        fheroes2::ButtonGroup btnGroup;
        btnGroup.addButton( fheroes2::makeButtonWithShadow( activeArea.x + border, activeArea.y + activeArea.height - border - buttonOkSprite.height(), buttonOkSprite,
                                                            fheroes2::AGG::GetICN( okIcnId, 1 ), display ),
                            Dialog::OK );

        btnGroup.button( 0 ).disable();

        btnGroup.addButton( fheroes2::makeButtonWithShadow( activeArea.x + activeArea.width - border - buttonCancelSprite.width(),
                                                            activeArea.y + activeArea.height - border - buttonCancelSprite.height(), buttonCancelSprite,
                                                            fheroes2::AGG::GetICN( cancelIcnId, 1 ), display ),
                            Dialog::CANCEL );
        btnGroup.draw();

        display.render();

        while ( result == Dialog::ZERO && le.HandleEvents() ) {
            result = btnGroup.processEvents();
            listbox.QueueEventProcessing();

            if ( !listbox.IsNeedRedraw() ) {
                continue;
            }

            if ( listbox.isSelected() ) {
                btnGroup.button( 0 ).enable();
                btnGroup.draw();
            }

            listbox.Redraw();
            display.render();
        }

        // restore background *before* the spell animation to avoid rendering issues
        frameborder.reset();

        if ( result == Dialog::OK ) {
            HeroesTownGate( hero, world.getCastleEntrance( Maps::GetPoint( listbox.GetCurrent() ) ) );

            return true;
        }

        return false;
    }

    bool ActionSpellVisions( Heroes & hero )
    {
        const uint32_t dist = hero.GetVisionsDistance();
        MapsIndexes monsters = Maps::ScanAroundObjectWithDistance( hero.GetIndex(), dist, MP2::OBJ_MONSTER );

        const int32_t heroColor = hero.GetColor();
        monsters.erase( std::remove_if( monsters.begin(), monsters.end(), [heroColor]( const int32_t index ) { return world.GetTiles( index ).isFog( heroColor ); } ),
                        monsters.end() );

        if ( monsters.empty() ) {
            std::string msg = _( "You must be within %{count} spaces of a monster for the Visions spell to work." );
            StringReplace( msg, "%{count}", dist );
            Dialog::Message( "", msg, Font::BIG, Dialog::OK );
            return false;
        }

        for ( const int32_t monsterIndex : monsters ) {
            const Maps::Tiles & tile = world.GetTiles( monsterIndex );

            Troop troop = tile.QuantityTroop();
            const NeutralMonsterJoiningCondition join = Army::GetJoinSolution( hero, tile, troop );

            std::string hdr;
            std::string msg;

            hdr = std::string( "%{count} " ) + troop.GetPluralName( join.monsterCount );
            StringReplace( hdr, "%{count}", troop.GetCount() );

            switch ( join.reason ) {
            case NeutralMonsterJoiningCondition::Reason::Free:
            case NeutralMonsterJoiningCondition::Reason::Alliance:
                msg = _( "The creatures are willing to join us!" );
                break;

            case NeutralMonsterJoiningCondition::Reason::ForMoney:
                if ( join.monsterCount == troop.GetCount() )
                    msg = _( "All the creatures will join us..." );
                else {
                    msg = _n( "The creature will join us...", "%{count} of the creatures will join us...", join.monsterCount );
                    StringReplace( msg, "%{count}", join.monsterCount );
                }

                msg += '\n';
                msg.append( _( "\n for a fee of %{gold} gold." ) );
                StringReplace( msg, "%{gold}", troop.GetTotalCost().gold );
                break;

            case NeutralMonsterJoiningCondition::Reason::RunAway:
            case NeutralMonsterJoiningCondition::Reason::Bane:
                msg = _( "These weak creatures will surely flee before us." );
                break;
            default:
                msg = _( "I fear these creatures are in the mood for a fight." );
                break;
            }

            Dialog::Message( hdr, msg, Font::BIG, Dialog::OK );
        }

        hero.SetModes( Heroes::VISIONS );

        return true;
    }

    bool ActionSpellSetGuardian( const Heroes & hero, const Spell & spell )
    {
        Maps::Tiles & tile = world.GetTiles( hero.GetIndex() );

        if ( MP2::OBJ_MINES != tile.GetObject( false ) ) {
            Dialog::Message( "", _( "You must be standing on the entrance to a mine (sawmills and alchemists don't count) to cast this spell." ), Font::BIG, Dialog::OK );
            return false;
        }

        const uint32_t count = fheroes2::getGuardianMonsterCount( spell, hero.GetPower(), &hero );

        if ( count == 0 ) {
            return false;
        }

        Maps::setSpellOnTile( tile, spell.GetID() );

        if ( spell == Spell::HAUNT ) {
            world.CaptureObject( tile.GetIndex(), Color::NONE );
            tile.removeOwnershipFlag( MP2::OBJ_MINES );

            // Update the color of haunted mine on radar.
            Interface::Basic & I = Interface::Basic::Get();
            const fheroes2::Point heroPosition = hero.GetCenter();
            I.GetRadar().SetRenderArea( { heroPosition.x - 1, heroPosition.y - 1, 3, 2 } );

            I.SetRedraw( Interface::REDRAW_RADAR );
        }

        world.GetCapturedObject( tile.GetIndex() ).GetTroop().Set( Monster( spell ), count );

        return true;
    }
}

void Heroes::ActionSpellCast( const Spell & spell )
{
    assert( spell.isValid() && !spell.isCombat() && CanCastSpell( spell ) );

    bool apply = false;

    switch ( spell.GetID() ) {
    case Spell::VIEWMINES:
        apply = ActionSpellViewMines();
        break;
    case Spell::VIEWRESOURCES:
        apply = ActionSpellViewResources();
        break;
    case Spell::VIEWARTIFACTS:
        apply = ActionSpellViewArtifacts();
        break;
    case Spell::VIEWTOWNS:
        apply = ActionSpellViewTowns();
        break;
    case Spell::VIEWHEROES:
        apply = ActionSpellViewHeroes();
        break;
    case Spell::VIEWALL:
        apply = ActionSpellViewAll();
        break;
    case Spell::IDENTIFYHERO:
        apply = ActionSpellIdentifyHero( *this );
        break;
    case Spell::SUMMONBOAT:
        apply = ActionSpellSummonBoat( *this );
        break;
    case Spell::DIMENSIONDOOR:
        apply = ActionSpellDimensionDoor( *this );
        break;
    case Spell::TOWNGATE:
        apply = ActionSpellTownGate( *this );
        break;
    case Spell::TOWNPORTAL:
        apply = ActionSpellTownPortal( *this );
        break;
    case Spell::VISIONS:
        apply = ActionSpellVisions( *this );
        break;
    case Spell::HAUNT:
    case Spell::SETEGUARDIAN:
    case Spell::SETAGUARDIAN:
    case Spell::SETFGUARDIAN:
    case Spell::SETWGUARDIAN:
        apply = ActionSpellSetGuardian( *this, spell );
        break;
    default:
        break;
    }

    if ( !apply ) {
        return;
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, GetName() << " cast spell: " << spell.GetName() )

    SpellCasted( spell );
}
