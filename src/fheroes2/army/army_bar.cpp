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
#include "agg.h"
#include "army.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game.h"
#include "race.h"
#include "text.h"
#include "world.h"

void RedistributeArmy( ArmyTroop & troopFrom, ArmyTroop & troopTarget, Army * armyTarget, bool & isTroopInfoVisible )
{
    const Army * armyFrom = troopFrom.GetArmy();
    const bool saveLastTroop = armyFrom->SaveLastTroop() && armyFrom != armyTarget;

    if ( troopFrom.GetCount() <= 1 ) {
        if ( saveLastTroop || troopTarget.isValid() ) {
            return;
        }

        Army::SwapTroops( troopFrom, troopTarget );
        isTroopInfoVisible = false;
    }
    else {
        uint32_t freeSlots = 1 + armyTarget->Size() - armyTarget->GetCount();
        const bool isSameTroopType = troopFrom.GetID() == troopTarget.GetID();

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
            if ( troopFrom.GetID() == troopTarget.GetID() )
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

    const uint32_t freeSlots = armyTarget->Size() - armyTarget->GetCount();
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

ArmyBar::ArmyBar( Army * ptr, bool mini, bool ro, bool change /* false */ )
    : spcursor( fheroes2::AGG::GetICN( ICN::STRIP, 1 ) )
    , _army( nullptr )
    , use_mini_sprite( mini )
    , read_only( ro )
    , can_change( change )
    , _isTroopInfoVisible( true )
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

void ArmyBar::RedrawBackground( const Rect & pos, fheroes2::Image & dstsf )
{
    if ( use_mini_sprite )
        fheroes2::Blit( backsf, dstsf, pos.x, pos.y );
    else
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STRIP, 2 ), dstsf, pos.x, pos.y );
}

void ArmyBar::RedrawItem( ArmyTroop & troop, const Rect & pos, bool selected, fheroes2::Image & dstsf )
{
    if ( troop.isValid() ) {
        Text text( std::to_string( troop.GetCount() ), ( use_mini_sprite ? Font::SMALL : Font::BIG ) );

        if ( use_mini_sprite ) {
            const fheroes2::Sprite & mons32 = fheroes2::AGG::GetICN( ICN::MONS32, troop.GetSpriteIndex() );
            fheroes2::Rect srcrt( 0, 0, mons32.width(), mons32.height() );

            if ( mons32.width() > pos.w ) {
                srcrt.x = ( mons32.width() - pos.w ) / 2;
                srcrt.width = pos.w;
            }

            if ( mons32.height() > pos.h ) {
                srcrt.y = ( mons32.height() - pos.h ) / 2;
                srcrt.height = pos.h;
            }

            fheroes2::Blit( mons32, srcrt.x, srcrt.y, dstsf, pos.x + ( pos.w - mons32.width() ) / 2, pos.y + pos.h - mons32.height() - 1, srcrt.width, srcrt.height );
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
            text.Blit( pos.x + pos.w - text.w() - 3, pos.y + pos.h - text.h(), dstsf );
        }
        else {
            text.Blit( pos.x + pos.w - text.w() - 3, pos.y + pos.h - text.h() - 1, dstsf );
        }

        if ( selected ) {
            spcursor.setPosition( pos.x, pos.y );
            spcursor.show();
        }
    }
}

void ArmyBar::ResetSelected( void )
{
    Cursor::Get().Hide();
    spcursor.hide();
    _isTroopInfoVisible = true;
    Interface::ItemsActionBar<ArmyTroop>::ResetSelected();
}

void ArmyBar::Redraw( fheroes2::Image & dstsf )
{
    Cursor::Get().Hide();
    spcursor.hide();
    Interface::ItemsActionBar<ArmyTroop>::Redraw( dstsf );
}

bool ArmyBar::ActionBarCursor( ArmyTroop & troop )
{
    if ( troop.isValid() && LocalEvent::Get().MouseClickMiddle() ) {
        RedistributeTroopByOne( troop, _army );
        return true;
    }

    if ( isSelected() ) {
        const ArmyTroop * troop2 = GetSelectedItem();

        if ( &troop == troop2 ) {
            msg = _( "View %{name}" );
            StringReplace( msg, "%{name}", troop.GetName() );
        }
        else if ( !troop.isValid() ) {
            msg = _( "Move or right click to redistribute %{name}" );
            StringReplace( msg, "%{name}", troop2->GetName() );
        }
        else if ( troop.GetID() == troop2->GetID() ) {
            msg = _( "Combine %{name} armies" );
            StringReplace( msg, "%{name}", troop.GetName() );
        }
        else {
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
    bool save_last_troop = selectedTroop.GetArmy()->SaveLastTroop();

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
        ArmyTroop * selectedTroop = GetSelectedItem();
        if ( selectedTroop && selectedTroop->isValid() && Game::HotKeyHoldEvent( Game::EVENT_STACKSPLIT_SHIFT ) ) {
            // redistribute when clicked troop is empty or is the same one as the selected troop
            if ( !troop.isValid() || troop.GetID() == selectedTroop->GetID() ) {
                RedistributeArmy( *selectedTroop, troop, _army, _isTroopInfoVisible );
                ResetSelected();

                return false;
            }
        }

        // combine
        if ( selectedTroop && troop.GetID() == selectedTroop->GetID() ) {
            troop.SetCount( troop.GetCount() + selectedTroop->GetCount() );
            selectedTroop->Reset();
        }
        // exchange
        else if ( selectedTroop ) {
            Army::SwapTroops( troop, *selectedTroop );
        }

        return false; // reset cursor
    }
    else if ( troop.isValid() ) {
        if ( !read_only ) // select
        {
            if ( IsSplitHotkeyUsed( troop, _army ) )
                return false;

            Cursor::Get().Hide();
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
    if ( Game::HotKeyHoldEvent( Game::EVENT_STACKSPLIT_SHIFT ) ) {
        if ( destTroop.isEmpty() || destTroop.GetID() == selectedTroop.GetID() ) {
            RedistributeArmy( selectedTroop, destTroop, _army, _isTroopInfoVisible );
            ResetSelected();
        }
        return false;
    }

    // destination troop is empty, source army would be emptied by moving all
    if ( destTroop.isEmpty() && selectedTroop.GetArmy()->SaveLastTroop() ) {
        // move all but one units into the empty destination slot
        destTroop.Set( selectedTroop, selectedTroop.GetCount() - 1 );
        selectedTroop.SetCount( 1 );
        return false;
    }

    // destination troop has units and both troops are the same creature type
    if ( !destTroop.isEmpty() && destTroop.GetID() == selectedTroop.GetID() ) {
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
        return false;
    }

    // no risk of emptying selected troop's army, swap the troops
    Army::SwapTroops( destTroop, selectedTroop );

    return false; // reset cursor
}

bool ArmyBar::ActionBarLeftMouseDoubleClick( ArmyTroop & troop )
{
    if ( troop.isValid() && !read_only && IsSplitHotkeyUsed( troop, _army ) ) {
        ResetSelected();
        return false;
    }

    const ArmyTroop * troop2 = GetSelectedItem();

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
    // drag drop - redistribute troops
    LocalEvent & le = LocalEvent::Get();
    ArmyTroop * troopPress = GetItem( le.GetMousePressLeft() );

    if ( !troop.isValid() && troopPress && troopPress->isValid() ) {
        RedistributeArmy( *troopPress, troop, _army, _isTroopInfoVisible );
        le.ResetPressLeft();

        if ( isSelected() )
            ResetSelected();
    }

    _isTroopInfoVisible = true;
    return true;
}

bool ArmyBar::ActionBarLeftMouseRelease( ArmyTroop & /*destTroop*/, ArmyTroop & /*selectedTroop*/ )
{
    if ( isSelected() )
        ResetSelected();

    _isTroopInfoVisible = true;
    return true;
}

bool ArmyBar::ActionBarRightMouseHold( ArmyTroop & troop )
{
    // Prioritize the click before press - aka prioritize split before showing troop info
    if ( ActionBarRightMouseSingleClick( troop ) )
        return true;

    if ( troop.isValid() && _isTroopInfoVisible ) {
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
    // try to redistribute troops if we have a selected troop
    if ( !isSelected() )
        return false;

    ArmyTroop & selectedTroop = *GetSelectedItem();

    // prevent troop from splitting into its own stack by checking against their pointers
    if ( &troop == &selectedTroop )
        return false;

    if ( !troop.isValid() || selectedTroop.GetID() == troop.GetID() ) {
        RedistributeArmy( selectedTroop, troop, _army, _isTroopInfoVisible );
        ResetSelected();

        return true;
    }

    return false;
}

bool ArmyBar::ActionBarRightMouseSingleClick( ArmyTroop & destTroop, ArmyTroop & selectedTroop )
{
    if ( !destTroop.isValid() || destTroop.GetID() == selectedTroop.GetID() ) {
        RedistributeArmy( selectedTroop, destTroop, _army, _isTroopInfoVisible );
        ResetSelected();

        return true;
    }

    return false;
}

bool ArmyBar::ActionBarRightMouseRelease( ArmyTroop & /*troop*/ )
{
    _isTroopInfoVisible = true;
    return true;
}

bool ArmyBar::ActionBarRightMouseRelease( ArmyTroop & /*destTroop*/, ArmyTroop & /*selectedTroop*/ )
{
    _isTroopInfoVisible = true;
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
