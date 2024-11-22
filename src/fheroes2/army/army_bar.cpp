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

#include "army_bar.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

#include "agg_image.h"
#include "army.h"
#include "army_troop.h"
#include "castle.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game_hotkeys.h"
#include "heroes_base.h"
#include "icn.h"
#include "kingdom.h"
#include "localevent.h"
#include "monster.h"
#include "pal.h"
#include "race.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_monster.h"
#include "ui_text.h"
#include "world.h"

namespace
{
    void RedistributeArmy( ArmyTroop & troopFrom, ArmyTroop & troopTarget, Army * armyTarget )
    {
        const Army * armyFrom = troopFrom.GetArmy();
        const bool saveLastTroop = armyFrom->SaveLastTroop() && armyFrom != armyTarget;
        const bool isSameTroopType = troopTarget.isValid() && troopFrom.GetID() == troopTarget.GetID();
        const uint32_t overallCount = isSameTroopType ? troopFrom.GetCount() + troopTarget.GetCount() : troopFrom.GetCount();

        assert( overallCount > 0 );

        if ( overallCount == 1 ) {
            // Prevent units from moving between armies if this would result in no units remaining in the hero's army
            if ( saveLastTroop ) {
                return;
            }

            // It seems that we are just moving a single unit to an empty slot
            assert( !troopTarget.isValid() );

            Army::SwapTroops( troopFrom, troopTarget );
        }
        else if ( !troopTarget.isValid() && troopFrom.GetCount() == 2 ) {
            // Player splits a slot with two monsters into an empty slot; move one monster from the source slot to the target slot
            troopFrom.SetCount( 1 );
            troopTarget.Set( troopFrom.GetMonster(), 1 );
        }
        else if ( isSameTroopType && troopFrom.isValid() && troopFrom.GetCount() == 1 && troopTarget.isValid() && troopTarget.GetCount() == 1 ) {
            // Prevent units from moving between armies if this would result in no units remaining in the hero's army
            if ( saveLastTroop ) {
                return;
            }

            // Player splits a slot with just one monster into a slot with just one monster of the same type; add a monster from the source slot to the target slot
            troopFrom.Reset();
            troopTarget.SetCount( 2 );
        }
        else {
            int32_t freeSlots = static_cast<int32_t>( 1 + armyTarget->Size() - armyTarget->GetOccupiedSlotCount() );
            if ( isSameTroopType ) {
                ++freeSlots;
            }

            const int32_t maxCount = static_cast<int32_t>( saveLastTroop ? troopFrom.GetCount() - 1 : troopFrom.GetCount() );
            int32_t redistributeCount = isSameTroopType ? std::min( 1, maxCount ) : static_cast<int32_t>( troopFrom.GetCount() ) / 2;

            bool useFastSplit = !isSameTroopType;
            const int32_t slots = Dialog::ArmySplitTroop( ( freeSlots > static_cast<int32_t>( overallCount ) ? static_cast<int32_t>( overallCount ) : freeSlots ),
                                                          maxCount, redistributeCount, useFastSplit, troopFrom.GetName() );

            if ( slots < 2 || slots > 6 ) {
                return;
            }

            uint32_t totalSplitTroopCount = troopFrom.GetCount();

            if ( !useFastSplit && slots == 2 ) {
                // This logic is used when splitting to a stack with the same unit
                if ( isSameTroopType ) {
                    troopTarget.SetCount( troopTarget.GetCount() + static_cast<uint32_t>( redistributeCount ) );
                }
                else {
                    troopTarget.Set( troopFrom, static_cast<uint32_t>( redistributeCount ) );
                }

                troopFrom.SetCount( totalSplitTroopCount - static_cast<uint32_t>( redistributeCount ) );
            }
            else {
                if ( isSameTroopType ) {
                    totalSplitTroopCount += troopTarget.GetCount();
                }

                const uint32_t troopFromSplitCount = ( totalSplitTroopCount + slots - 1 ) / slots;
                troopFrom.SetCount( troopFromSplitCount );

                const uint32_t troopTargetSplitCount = ( totalSplitTroopCount + slots - 2 ) / slots;

                if ( !isSameTroopType ) {
                    troopTarget.SetMonster( troopFrom.GetID() );
                }

                troopTarget.SetCount( troopTargetSplitCount );

                totalSplitTroopCount -= troopFromSplitCount;
                totalSplitTroopCount -= troopTargetSplitCount;

                if ( slots > 2 ) {
                    armyTarget->SplitTroopIntoFreeSlots( Troop( troopFrom, totalSplitTroopCount ), troopTarget, slots - 2 );
                }
            }
        }
    }

    void RedistributeTroopToFirstFreeSlot( ArmyTroop & troopFrom, const Army * armyTarget, const uint32_t count )
    {
        // can't split up a stack with just 1 unit, and obviously on count == 0, there's no splitting at all
        if ( troopFrom.GetCount() <= 1 || count == 0 )
            return;

        const size_t freeSlots = armyTarget->Size() - armyTarget->GetOccupiedSlotCount();
        if ( freeSlots == 0 )
            return;

        armyTarget->AssignToFirstFreeSlot( troopFrom, count );
        troopFrom.SetCount( troopFrom.GetCount() - count );
    }

    void RedistributeTroopByOne( ArmyTroop & troopFrom, const Army * armyTarget )
    {
        RedistributeTroopToFirstFreeSlot( troopFrom, armyTarget, 1 );
    }

    void RedistributeTroopEvenly( ArmyTroop & troopFrom, const Army * armyTarget )
    {
        RedistributeTroopToFirstFreeSlot( troopFrom, armyTarget, troopFrom.GetCount() / 2 );
    }

    bool IsSplitHotkeyUsed( ArmyTroop & troopFrom, const Army * armyTarget )
    {
        if ( Game::HotKeyHoldEvent( Game::HotKeyEvent::ARMY_SPLIT_STACK_BY_ONE ) ) {
            RedistributeTroopByOne( troopFrom, armyTarget );
            return true;
        }
        if ( Game::HotKeyHoldEvent( Game::HotKeyEvent::ARMY_JOIN_STACKS ) ) {
            armyTarget->JoinAllTroopsOfType( troopFrom );
            return true;
        }
        if ( Game::HotKeyHoldEvent( Game::HotKeyEvent::ARMY_SPLIT_STACK_BY_HALF ) ) {
            RedistributeTroopEvenly( troopFrom, armyTarget );
            return true;
        }

        return false;
    }
}

ArmyBar::ArmyBar( Army * ptr, const bool miniSprites, const bool readOnly, const bool isEditMode /* false */, const bool saveLastTroop /* true */ )
    : spcursor( fheroes2::AGG::GetICN( ICN::STRIP, 1 ) )
    , use_mini_sprite( miniSprites )
    , read_only( readOnly )
    , can_change( isEditMode )
    , _saveLastTroop( saveLastTroop )
{
    if ( use_mini_sprite )
        SetBackground( { 43, 43 }, fheroes2::GetColorId( 0, 45, 0 ) );
    else {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::STRIP, 2 );
        setSingleItemSize( { sprite.width(), sprite.height() } );
    }

    SetArmy( ptr );
}

void ArmyBar::SetArmy( Army * ptr )
{
    if ( _army && isSelected() )
        ResetSelected();

    _army = ptr;
    items.clear();

    if ( ptr ) {
        for ( uint32_t ii = 0; ii < ptr->Size(); ++ii ) {
            ArmyTroop * troop = dynamic_cast<ArmyTroop *>( ptr->GetTroop( ii ) );
            assert( troop != nullptr );

            items.push_back( troop );
        }
    }

    SetContentItems();
}

bool ArmyBar::isValid() const
{
    return _army != nullptr;
}

void ArmyBar::SetBackground( const fheroes2::Size & sz, const uint8_t fillColor )
{
    if ( !use_mini_sprite ) {
        // Nothing to draw.
        return;
    }

    setSingleItemSize( sz );

    backsf._disableTransformLayer();
    backsf.resize( sz.width, sz.height );
    backsf.fill( fillColor );

    fheroes2::DrawBorder( backsf, fheroes2::GetColorId( 0xd0, 0xc0, 0x48 ) );

    spcursor.resize( sz.width, sz.height );
    spcursor.reset();
    fheroes2::DrawBorder( spcursor, 214 );
}

void ArmyBar::RedrawBackground( const fheroes2::Rect & pos, fheroes2::Image & dstsf )
{
    if ( use_mini_sprite ) {
        fheroes2::Copy( backsf, 0, 0, dstsf, pos );
    }
    else {
        if ( can_change && !_saveLastTroop && _army->GetOccupiedSlotCount() == 0 ) {
            // If none of army's slot is set within the Editor, then a default army will be applied at the game start.
            fheroes2::ApplyPalette( fheroes2::AGG::GetICN( ICN::STRIP, 2 ), 0, 0, dstsf, pos.x, pos.y, pos.width, pos.height,
                                    PAL::GetPalette( PAL::PaletteType::DARKENING ) );

            const fheroes2::Text text( _( "Default\ntroop" ), fheroes2::FontType::normalWhite() );
            text.drawInRoi( pos.x, pos.y + pos.height / 2 - 17, pos.width, dstsf, pos );
        }
        else {
            fheroes2::Copy( fheroes2::AGG::GetICN( ICN::STRIP, 2 ), 0, 0, dstsf, pos );
        }
    }
}

void ArmyBar::RedrawItem( ArmyTroop & troop, const fheroes2::Rect & pos, bool selected, fheroes2::Image & dstsf )
{
    if ( !troop.isValid() ) {
        // Nothing to draw.
        return;
    }

    const fheroes2::Text text( std::to_string( troop.GetCount() ), use_mini_sprite ? fheroes2::FontType::smallWhite() : fheroes2::FontType::normalWhite() );

    if ( use_mini_sprite ) {
        const fheroes2::Sprite & mons32 = fheroes2::AGG::GetICN( ICN::MONS32, troop.GetSpriteIndex() );
        fheroes2::Rect srcrt( 0, 0, mons32.width(), mons32.height() );

        if ( mons32.width() > pos.width ) {
            srcrt.x = ( mons32.width() - pos.width ) / 2;
            srcrt.width = pos.width;
        }

        if ( mons32.height() > pos.height ) {
            srcrt.y = ( mons32.height() - pos.height ) / 2;
            srcrt.height = pos.height;
        }

        fheroes2::Blit( mons32, srcrt.x, srcrt.y, dstsf, pos.x + ( pos.width - mons32.width() ) / 2, pos.y + pos.height - mons32.height() - 1, srcrt.width,
                        srcrt.height );

        text.draw( pos.x + pos.width - text.width() - 3, pos.y + pos.height - text.height() + 2, dstsf );
    }
    else {
        fheroes2::renderMonsterFrame( troop, dstsf, pos.getPosition() );

        text.draw( pos.x + pos.width - text.width() - 3, pos.y + pos.height - text.height() + 1, dstsf );
    }

    if ( selected ) {
        spcursor.setPosition( pos.x, pos.y );
        spcursor.show();
    }
}

void ArmyBar::ResetSelected()
{
    spcursor.hide();
    Interface::ItemsActionBar<ArmyTroop>::ResetSelected();
}

void ArmyBar::Redraw( fheroes2::Image & dstsf )
{
    spcursor.hide();
    Interface::ItemsActionBar<ArmyTroop>::Redraw( dstsf );
}

bool ArmyBar::ActionBarCursor( ArmyTroop & troop )
{
    if ( troop.isValid() && !read_only && LocalEvent::Get().MouseClickMiddle() ) {
        RedistributeTroopByOne( troop, _army );
        return true;
    }

    if ( isSelected() ) {
        const ArmyTroop * troop2 = GetSelectedItem();
        assert( troop2 != nullptr );

        if ( &troop == troop2 ) {
            msg = _( "View %{name}" );
            StringReplaceWithLowercase( msg, "%{name}", troop.GetName() );
        }
        else if ( !troop.isValid() ) {
            if ( !read_only ) {
                if ( troop2->GetCount() == 1 ) {
                    msg = _( "Move the %{name} " );
                }
                else {
                    msg = _( "Move or right click to redistribute %{name}" );
                }

                StringReplaceWithLowercase( msg, "%{name}", troop2->GetName() );
            }
        }
        else if ( troop.GetID() == troop2->GetID() ) {
            if ( !read_only ) {
                msg = _( "Combine %{name} armies" );
                StringReplaceWithLowercase( msg, "%{name}", troop.GetName() );
            }
        }
        else if ( !read_only ) {
            msg = _( "Exchange %{name2} with %{name}" );
            StringReplaceWithLowercase( msg, "%{name}", troop.GetName() );
            StringReplaceWithLowercase( msg, "%{name2}", troop2->GetName() );
        }
    }
    else if ( troop.isValid() ) {
        msg = _( "Select %{name}" );
        StringReplaceWithLowercase( msg, "%{name}", troop.GetName() );
    }

    return false;
}

bool ArmyBar::ActionBarCursor( ArmyTroop & destTroop, ArmyTroop & selectedTroop )
{
    const bool saveLastTroop = _saveLastTroop && ( selectedTroop.GetArmy()->getTotalCount() <= 1 ) && selectedTroop.GetArmy()->SaveLastTroop();

    if ( destTroop.isValid() ) {
        if ( destTroop.GetID() != selectedTroop.GetID() ) {
            msg = _( "Exchange %{name2} with %{name}" );
            StringReplaceWithLowercase( msg, "%{name}", destTroop.GetName() );
            StringReplaceWithLowercase( msg, "%{name2}", selectedTroop.GetName() );
        }
        else if ( saveLastTroop ) {
            msg = _( "Cannot move last troop" );
        }
        else {
            msg = _( "Combine %{name} armies" );
            StringReplaceWithLowercase( msg, "%{name}", destTroop.GetName() );
        }
    }
    else if ( saveLastTroop ) {
        msg = _( "Cannot move last troop" );
    }
    else {
        if ( selectedTroop.GetCount() == 1 ) {
            msg = _( "Move the %{name}" );
        }
        else {
            msg = _( "Move or right click to redistribute %{name}" );
        }

        StringReplaceWithLowercase( msg, "%{name}", selectedTroop.GetName() );
    }

    return false;
}

bool ArmyBar::ActionBarLeftMouseSingleClick( ArmyTroop & troop )
{
    if ( isSelected() ) {
        if ( read_only ) {
            return false; // reset cursor
        }

        ArmyTroop * selectedTroop = GetSelectedItem();
        assert( selectedTroop != nullptr );

        const bool isSameTroopType = troop.isValid() && troop.GetID() == selectedTroop->GetID();

        // prioritize standard split via shift hotkey
        if ( ( !troop.isValid() || isSameTroopType ) && Game::HotKeyHoldEvent( Game::HotKeyEvent::ARMY_SPLIT_STACK_BY_HALF ) ) {
            RedistributeArmy( *selectedTroop, troop, _army );
            ResetSelected();
        }
        else if ( selectedTroop && isSameTroopType ) {
            if ( IsSplitHotkeyUsed( troop, _army ) ) {
                return false;
            }
            else { // combine
                troop.SetCount( troop.GetCount() + selectedTroop->GetCount() );
                selectedTroop->Reset();
            }
        }
        // exchange
        else if ( selectedTroop ) {
            // count this as an attempt to split to a troop type that is not the same
            if ( Game::HotKeyHoldEvent( Game::HotKeyEvent::ARMY_SPLIT_STACK_BY_HALF ) )
                ResetSelected();
            else if ( IsSplitHotkeyUsed( troop, _army ) )
                return false;
            else
                Army::SwapTroops( troop, *selectedTroop );
        }

        return false; // reset cursor
    }
    else if ( troop.isValid() ) {
        if ( !read_only ) // select
        {
            if ( IsSplitHotkeyUsed( troop, _army ) )
                return false;

            spcursor.hide();
        }
    }
    else {
        if ( can_change ) // add troop
        {
            int cur = Monster::UNKNOWN;

            if ( _army->GetCommander() )
                switch ( _army->GetCommander()->GetRace() ) {
                case Race::KNGT:
                    cur = Monster::PEASANT;
                    break;
                case Race::BARB:
                    cur = Monster::GOBLIN;
                    break;
                case Race::SORC:
                    cur = Monster::SPRITE;
                    break;
                case Race::WRLK:
                    cur = Monster::CENTAUR;
                    break;
                case Race::WZRD:
                    cur = Monster::HALFLING;
                    break;
                case Race::NECR:
                    cur = Monster::SKELETON;
                    break;
                default:
                    break;
                }

            const Monster mons = Dialog::selectMonster( cur );

            if ( mons.isValid() ) {
                std::string str = _( "Set %{monster} Count" );
                StringReplace( str, "%{monster}", mons.GetName() );

                int32_t count = 1;
                auto monsUi = std::make_unique<const fheroes2::MonsterDialogElement>( mons );

                if ( Dialog::SelectCount( str, 1, 500000, count, 1, monsUi.get() ) ) {
                    troop.Set( mons, static_cast<uint32_t>( count ) );
                }
            }
        }

        return false;
    }

    return true;
}

bool ArmyBar::ActionBarLeftMouseSingleClick( ArmyTroop & destTroop, ArmyTroop & selectedTroop )
{
    const bool isSameTroopType = destTroop.isValid() && destTroop.GetID() == selectedTroop.GetID();

    // specifically for shift hotkey, handle this logic before anything else
    // this will ensure that clicking on a different troop type while shift key is pressed will not show the split dialog, which can be ambiguous
    if ( Game::HotKeyHoldEvent( Game::HotKeyEvent::ARMY_SPLIT_STACK_BY_HALF ) ) {
        if ( destTroop.isEmpty() || isSameTroopType ) {
            RedistributeArmy( selectedTroop, destTroop, _army );
            ResetSelected();
        }
        return false;
    }

    if ( !destTroop.isEmpty() ) {
        // try to do hotkey-based splitting (except for shift, handled above)
        if ( IsSplitHotkeyUsed( destTroop, _army ) )
            return false;

        if ( !isSameTroopType ) {
            Army::SwapTroops( destTroop, selectedTroop );
        }
        // destination troop has units and both troops are the same creature type
        else {
            if ( _saveLastTroop && selectedTroop.GetArmy()->SaveLastTroop() ) { // this is their army's only troop
                // move all but one units to destination
                destTroop.SetCount( destTroop.GetCount() + selectedTroop.GetCount() - 1 );
                // leave a single unit behind
                selectedTroop.SetCount( 1 );
            }
            else { // source has other troops
                // move all troops to the destination slot
                destTroop.SetCount( destTroop.GetCount() + selectedTroop.GetCount() );
                // empty the source slot
                selectedTroop.Reset();
            }
        }
    }
    else {
        // destination troop is empty, source army would be emptied by moving all
        if ( _saveLastTroop && selectedTroop.GetArmy()->SaveLastTroop() ) {
            // move all but one units into the empty destination slot
            destTroop.Set( selectedTroop, selectedTroop.GetCount() - 1 );
            selectedTroop.SetCount( 1 );
        }
        // no risk of emptying selected troop's army, swap the troops
        else {
            Army::SwapTroops( destTroop, selectedTroop );
        }
    }

    return false; // reset cursor
}

bool ArmyBar::ActionBarLeftMouseDoubleClick( ArmyTroop & troop )
{
    if ( troop.isValid() && !read_only && IsSplitHotkeyUsed( troop, _army ) ) {
        ResetSelected();
        return true;
    }

    const ArmyTroop * troop2 = GetSelectedItem();
    assert( troop2 != nullptr );

    if ( &troop == troop2 ) {
        int flags = Dialog::BUTTONS;

        if ( !read_only ) {
            if ( !_saveLastTroop || !_army->SaveLastTroop() ) {
                flags |= Dialog::DISMISS;
            }

            if ( troop.isAllowUpgrade() ) {
                const Castle * castle = _army->inCastle();

                if ( castle && castle->GetRace() == troop.GetRace() && castle->isBuild( troop.GetUpgrade().GetDwelling() ) ) {
                    flags |= Dialog::UPGRADE;

                    if ( !world.GetKingdom( _army->GetColor() ).AllowPayment( troop.GetTotalUpgradeCost() ) ) {
                        flags |= Dialog::UPGRADE_DISABLE;
                    }
                }
            }
        }

        switch ( Dialog::ArmyInfo( troop, flags, false, _troopWindowOffsetY ) ) {
        case Dialog::UPGRADE:
            // If this assertion blows up then you are executing this code for a monster which has no upgrades.
            assert( troop.isAllowUpgrade() );

            world.GetKingdom( _army->GetColor() ).OddFundsResource( troop.GetTotalUpgradeCost() );
            troop.Upgrade();
            break;

        case Dialog::DISMISS:
            troop.Reset();
            break;

        default:
            break;
        }
    }

    ResetSelected();

    return true;
}

bool ArmyBar::ActionBarLeftMouseRelease( ArmyTroop & troop )
{
    if ( read_only ) {
        return true;
    }

    ArmyTroop * srcTroop = GetItem( LocalEvent::Get().getMouseLeftButtonPressedPos() );
    if ( srcTroop == nullptr || !srcTroop->isValid() ) {
        return true;
    }

    if ( &troop == srcTroop ) {
        return true;
    }

    if ( troop.isValid() && troop.GetID() != srcTroop->GetID() ) {
        return true;
    }

    RedistributeArmy( *srcTroop, troop, _army );

    if ( isSelected() ) {
        ResetSelected();
    }

    return true;
}

bool ArmyBar::ActionBarLeftMouseRelease( ArmyTroop & destTroop, ArmyTroop & selectedTroop )
{
    assert( !read_only );

    if ( isSelected() ) {
        ResetSelected();
    }

    if ( !selectedTroop.isValid() ) {
        return false;
    }

    if ( destTroop.isValid() && selectedTroop.GetID() != destTroop.GetID() ) {
        return false;
    }

    assert( &selectedTroop != &destTroop );

    RedistributeArmy( selectedTroop, destTroop, _army );

    return true;
}

bool ArmyBar::ActionBarRightMouseHold( ArmyTroop & troop )
{
    // don't handle this event if we are going to call RedistributeArmy() on right mouse button click
    if ( AbleToRedistributeArmyOnRightMouseSingleClick( troop ) ) {
        return false;
    }

    if ( troop.isValid() ) {
        ResetSelected();

        if ( can_change && ( !_saveLastTroop || !_army->SaveLastTroop() ) ) {
            troop.Reset();
        }
        else {
            Dialog::ArmyInfo( troop, Dialog::ZERO, false, _troopWindowOffsetY );
        }
    }

    return true;
}

bool ArmyBar::ActionBarRightMouseSingleClick( ArmyTroop & troop )
{
    if ( AbleToRedistributeArmyOnRightMouseSingleClick( troop ) ) {
        ArmyTroop * selectedTroop = GetSelectedItem();
        assert( selectedTroop != nullptr );

        RedistributeArmy( *selectedTroop, troop, _army );
        ResetSelected();

        return true;
    }

    return false;
}

bool ArmyBar::ActionBarRightMouseSingleClick( ArmyTroop & destTroop, ArmyTroop & selectedTroop )
{
    if ( !destTroop.isValid() || destTroop.GetID() == selectedTroop.GetID() ) {
        RedistributeArmy( selectedTroop, destTroop, _army );
        ResetSelected();
    }

    return true;
}

bool ArmyBar::QueueEventProcessing( std::string * str )
{
    msg.clear();
    bool res = Interface::ItemsActionBar<ArmyTroop>::QueueEventProcessing();
    if ( str )
        *str = msg;
    return res;
}

bool ArmyBar::QueueEventProcessing( ArmyBar & bar, std::string * str )
{
    msg.clear();
    bool res = Interface::ItemsActionBar<ArmyTroop>::QueueEventProcessing( bar );
    if ( str )
        *str = msg;
    return res;
}

bool ArmyBar::AbleToRedistributeArmyOnRightMouseSingleClick( const ArmyTroop & troop )
{
    if ( read_only ) {
        return false;
    }

    // try to redistribute troops if we have a selected troop
    if ( !isSelected() ) {
        return false;
    }

    const ArmyTroop * selectedTroop = GetSelectedItem();
    assert( selectedTroop != nullptr );

    // prevent troop from splitting into its own stack by checking against their pointers
    if ( &troop == selectedTroop ) {
        return false;
    }

    // we can redistribute troops either to an empty slot or to a slot containing the same creatures
    return !troop.isValid() || selectedTroop->GetID() == troop.GetID();
}
