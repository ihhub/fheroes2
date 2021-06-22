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

#include "army_bar.h"
#include "agg_image.h"
#include "army.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game.h"
#include "icn.h"
#include "race.h"
#include "text.h"
#include "tools.h"
#include "world.h"

#include <cassert>

namespace
{
    void RedistributeArmy( ArmyTroop & troopFrom, ArmyTroop & troopTarget, Army * armyTarget )
    {
        const Army * armyFrom = troopFrom.GetArmy();
        const bool saveLastTroop = armyFrom->SaveLastTroop() && armyFrom != armyTarget;
        const bool isSameTroopType = troopTarget.isValid() && troopFrom.GetID() == troopTarget.GetID();

        if ( troopFrom.GetCount() <= 1 ) {
            // cross-army split logic - prevent splits where we'd lose the last stack of a hero
            if ( saveLastTroop )
                return;
            // join the two stacks if the troop types are same and the source stack is just 1 unit
            else if ( isSameTroopType ) {
                troopTarget.SetCount( troopTarget.GetCount() + troopFrom.GetCount() );
                troopFrom.Reset();
            }
            // or else just move the source troop around
            else if ( !troopTarget.isValid() ) {
                Army::SwapTroops( troopFrom, troopTarget );
            }
        }
        else {
            uint32_t freeSlots = static_cast<uint32_t>( 1 + armyTarget->Size() - armyTarget->GetCount() );

            if ( isSameTroopType )
                ++freeSlots;

            const uint32_t maxCount = saveLastTroop ? troopFrom.GetCount() - 1 : troopFrom.GetCount();
            uint32_t redistributeCount = isSameTroopType ? 1 : troopFrom.GetCount() / 2;

            // if splitting to the same troop type, use this bool to turn off fast split option at the beginning of the dialog
            bool useFastSplit = !isSameTroopType;
            const uint32_t slots = Dialog::ArmySplitTroop( ( freeSlots > maxCount ? maxCount : freeSlots ), maxCount, saveLastTroop, redistributeCount, useFastSplit );

            if ( slots < 2 || slots > 6 )
                return;

            uint32_t totalSplitTroopCount = troopFrom.GetCount();

            if ( !useFastSplit && slots == 2 ) {
                // this logic is used when splitting to a stack with the same unit
                if ( isSameTroopType )
                    troopTarget.SetCount( troopTarget.GetCount() + redistributeCount );
                else
                    troopTarget.Set( troopFrom, redistributeCount );

                troopFrom.SetCount( totalSplitTroopCount - redistributeCount );
            }
            else {
                if ( isSameTroopType )
                    totalSplitTroopCount += troopTarget.GetCount();

                const uint32_t troopFromSplitCount = ( totalSplitTroopCount + slots - 1 ) / slots;
                troopFrom.SetCount( troopFromSplitCount );

                const uint32_t troopTargetSplitCount = ( totalSplitTroopCount + slots - 2 ) / slots;

                if ( !isSameTroopType )
                    troopTarget.SetMonster( troopFrom.GetID() );

                troopTarget.SetCount( troopTargetSplitCount );

                totalSplitTroopCount -= troopFromSplitCount;
                totalSplitTroopCount -= troopTargetSplitCount;
                armyTarget->SplitTroopIntoFreeSlots( Troop( troopFrom, totalSplitTroopCount ), troopTarget, slots - 2 );
            }
        }
    }

    void RedistributeTroopToFirstFreeSlot( ArmyTroop & troopFrom, Army * armyTarget, const uint32_t count )
    {
        // can't split up a stack with just 1 unit, and obviously on count == 0, there's no splitting at all
        if ( troopFrom.GetCount() <= 1 || count == 0 )
            return;

        const size_t freeSlots = armyTarget->Size() - armyTarget->GetCount();
        if ( freeSlots == 0 )
            return;

        armyTarget->AssignToFirstFreeSlot( troopFrom, count );
        troopFrom.SetCount( troopFrom.GetCount() - count );
    }

    void RedistributeTroopByOne( ArmyTroop & troopFrom, Army * armyTarget )
    {
        RedistributeTroopToFirstFreeSlot( troopFrom, armyTarget, 1 );
    }

    void RedistributeTroopEvenly( ArmyTroop & troopFrom, Army * armyTarget )
    {
        RedistributeTroopToFirstFreeSlot( troopFrom, armyTarget, troopFrom.GetCount() / 2 );
    }

    bool IsSplitHotkeyUsed( ArmyTroop & troopFrom, Army * armyTarget )
    {
        if ( Game::HotKeyHoldEvent( Game::EVENT_STACKSPLIT_CTRL ) ) {
            RedistributeTroopByOne( troopFrom, armyTarget );
            return true;
        }
        else if ( Game::HotKeyHoldEvent( Game::EVENT_JOINSTACKS ) ) {
            armyTarget->JoinAllTroopsOfType( troopFrom );
            return true;
        }
        else if ( Game::HotKeyHoldEvent( Game::EVENT_STACKSPLIT_SHIFT ) ) {
            RedistributeTroopEvenly( troopFrom, armyTarget );
            return true;
        }

        return false;
    }
}

ArmyBar::ArmyBar( Army * ptr, bool mini, bool ro, bool change /* false */ )
    : spcursor( fheroes2::AGG::GetICN( ICN::STRIP, 1 ) )
    , _army( nullptr )
    , use_mini_sprite( mini )
    , read_only( ro )
    , can_change( change )
{
    if ( use_mini_sprite )
        SetBackground( fheroes2::Size( 43, 43 ), fheroes2::GetColorId( 0, 45, 0 ) );
    else {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::STRIP, 2 );
        SetItemSize( sprite.width(), sprite.height() );
    }

    SetArmy( ptr );
}

void ArmyBar::SetArmy( Army * ptr )
{
    if ( _army && isSelected() )
        ResetSelected();

    _army = ptr;
    items.clear();

    if ( ptr )
        for ( u32 ii = 0; ii < ptr->Size(); ++ii )
            items.push_back( reinterpret_cast<ArmyTroop *>( ptr->GetTroop( ii ) ) );

    SetContentItems();
}

bool ArmyBar::isValid() const
{
    return _army != nullptr;
}

void ArmyBar::SetBackground( const fheroes2::Size & sz, const uint8_t fillColor )
{
    if ( use_mini_sprite ) {
        SetItemSize( sz.width, sz.height );

        backsf.resize( sz.width, sz.height );
        backsf.fill( fillColor );

        fheroes2::DrawBorder( backsf, fheroes2::GetColorId( 0xd0, 0xc0, 0x48 ) );

        spcursor.resize( sz.width, sz.height );
        spcursor.reset();
        fheroes2::DrawBorder( spcursor, 214 );
    }
}

void ArmyBar::RedrawBackground( const fheroes2::Rect & pos, fheroes2::Image & dstsf )
{
    if ( use_mini_sprite )
        fheroes2::Blit( backsf, dstsf, pos.x, pos.y );
    else
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 2 ), dstsf, pos.x, pos.y );
}

void ArmyBar::RedrawItem( ArmyTroop & troop, const fheroes2::Rect & pos, bool selected, fheroes2::Image & dstsf )
{
    if ( troop.isValid() ) {
        Text text( std::to_string( troop.GetCount() ), ( use_mini_sprite ? Font::SMALL : Font::BIG ) );

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
        }
        else {
            switch ( troop.GetRace() ) {
            case Race::KNGT:
                fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 4 ), dstsf, pos.x, pos.y );
                break;
            case Race::BARB:
                fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 5 ), dstsf, pos.x, pos.y );
                break;
            case Race::SORC:
                fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 6 ), dstsf, pos.x, pos.y );
                break;
            case Race::WRLK:
                fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 7 ), dstsf, pos.x, pos.y );
                break;
            case Race::WZRD:
                fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 8 ), dstsf, pos.x, pos.y );
                break;
            case Race::NECR:
                fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 9 ), dstsf, pos.x, pos.y );
                break;
            default:
                fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 10 ), dstsf, pos.x, pos.y );
                break;
            }

            const fheroes2::Sprite & spmonh = fheroes2::AGG::GetICN( troop.ICNMonh(), 0 );
            fheroes2::Blit( spmonh, dstsf, pos.x + spmonh.x(), pos.y + spmonh.y() );
        }

        if ( use_mini_sprite ) {
            text.Blit( pos.x + pos.width - text.w() - 3, pos.y + pos.height - text.h(), dstsf );
        }
        else {
            text.Blit( pos.x + pos.width - text.w() - 3, pos.y + pos.height - text.h() - 1, dstsf );
        }

        if ( selected ) {
            spcursor.setPosition( pos.x, pos.y );
            spcursor.show();
        }
    }
}

void ArmyBar::ResetSelected( void )
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
            StringReplace( msg, "%{name}", troop.GetName() );
        }
        else if ( !troop.isValid() ) {
            if ( !read_only ) {
                msg = _( "Move or right click to redistribute %{name}" );
                StringReplace( msg, "%{name}", troop2->GetName() );
            }
        }
        else if ( troop.GetID() == troop2->GetID() ) {
            if ( !read_only ) {
                msg = _( "Combine %{name} armies" );
                StringReplace( msg, "%{name}", troop.GetName() );
            }
        }
        else if ( !read_only ) {
            msg = _( "Exchange %{name2} with %{name}" );
            StringReplace( msg, "%{name}", troop.GetName() );
            StringReplace( msg, "%{name2}", troop2->GetName() );
        }
    }
    else if ( troop.isValid() ) {
        msg = _( "Select %{name}" );
        StringReplace( msg, "%{name}", troop.GetName() );
    }

    return false;
}

bool ArmyBar::ActionBarCursor( ArmyTroop & destTroop, ArmyTroop & selectedTroop )
{
    bool save_last_troop = ( selectedTroop.GetArmy()->getTotalCount() <= 1 ) && selectedTroop.GetArmy()->SaveLastTroop();

    if ( destTroop.isValid() ) {
        if ( destTroop.GetID() != selectedTroop.GetID() ) {
            msg = _( "Exchange %{name2} with %{name}" );
            StringReplace( msg, "%{name}", destTroop.GetName() );
            StringReplace( msg, "%{name2}", selectedTroop.GetName() );
        }
        else if ( save_last_troop )
            msg = _( "Cannot move last troop" );
        else {
            msg = _( "Combine %{name} armies" );
            StringReplace( msg, "%{name}", destTroop.GetName() );
        }
    }
    else if ( save_last_troop )
        msg = _( "Cannot move last troop" );
    else {
        msg = _( "Move or right click to redistribute %{name}" );
        StringReplace( msg, "%{name}", selectedTroop.GetName() );
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
        if ( ( !troop.isValid() || isSameTroopType ) && Game::HotKeyHoldEvent( Game::EVENT_STACKSPLIT_SHIFT ) ) {
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
            if ( Game::HotKeyHoldEvent( Game::EVENT_STACKSPLIT_SHIFT ) )
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

            const Monster mons = Dialog::SelectMonster( cur );

            if ( mons.isValid() ) {
                u32 count = 1;

                if ( Dialog::SelectCount( "Set Count", 1, 500000, count ) )
                    troop.Set( mons, count );
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
    // this will ensure that clicking on a different troop type while shift key is pressed will not show the split dialogue, which can be ambiguous
    if ( Game::HotKeyHoldEvent( Game::EVENT_STACKSPLIT_SHIFT ) ) {
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
            if ( selectedTroop.GetArmy()->SaveLastTroop() ) { // this is their army's only troop
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
        if ( selectedTroop.GetArmy()->SaveLastTroop() ) {
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
        int flags = ( read_only || _army->SaveLastTroop() ? Dialog::READONLY | Dialog::BUTTONS : Dialog::BUTTONS );
        const Castle * castle = _army->inCastle();

        if ( troop.isAllowUpgrade() &&
             // allow upgrade
             castle && castle->GetRace() == troop.GetRace() && castle->isBuild( troop.GetUpgrade().GetDwelling() ) ) {
            flags |= Dialog::UPGRADE;

            if ( !world.GetKingdom( _army->GetColor() ).AllowPayment( troop.GetUpgradeCost() ) )
                flags |= Dialog::UPGRADE_DISABLE;
        }

        switch ( Dialog::ArmyInfo( troop, flags ) ) {
        case Dialog::UPGRADE:
            world.GetKingdom( _army->GetColor() ).OddFundsResource( troop.GetUpgradeCost() );
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
    if ( !read_only ) {
        // drag drop - redistribute troops
        LocalEvent & le = LocalEvent::Get();
        ArmyTroop * troopPress = GetItem( le.GetMousePressLeft() );

        const bool isTroopPressValid = troopPress && troopPress->isValid();

        if ( isTroopPressValid && ( !troop.isValid() || troop.GetID() == troopPress->GetID() ) ) {
            RedistributeArmy( *troopPress, troop, _army );
            le.ResetPressLeft();

            if ( isSelected() )
                ResetSelected();
        }
    }

    return true;
}

bool ArmyBar::ActionBarLeftMouseRelease( ArmyTroop & destTroop, ArmyTroop & selectedTroop )
{
    if ( isSelected() )
        ResetSelected();

    // cross-army drag split
    if ( selectedTroop.isValid() && ( !destTroop.isValid() || selectedTroop.GetID() == destTroop.GetID() ) ) {
        RedistributeArmy( selectedTroop, destTroop, _army );
        return true;
    }

    return false;
}

bool ArmyBar::ActionBarRightMouseHold( ArmyTroop & troop )
{
    // don't handle this event if we are going to call RedistributeArmy() on right mouse button click
    if ( AbleToRedistributeArmyOnRightMouseSingleClick( troop ) ) {
        return false;
    }

    if ( troop.isValid() ) {
        ResetSelected();

        if ( can_change && !_army->SaveLastTroop() )
            troop.Reset();
        else
            Dialog::ArmyInfo( troop, 0 );
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
