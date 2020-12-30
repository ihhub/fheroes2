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

void RedistributeArmy( ArmyTroop & troop1 /* from */, ArmyTroop & troop2 /* to */, bool & isTroopInfoVisible )
{
    const Army * army1 = troop1.GetArmy();
    const Army * army2 = troop2.GetArmy();

    bool save_last_troop = army1->SaveLastTroop() && army1 != army2;

    if ( 2 > troop1.GetCount() ) {
        if ( !save_last_troop || troop2.isValid() ) {
            Army::SwapTroops( troop1, troop2 );
            isTroopInfoVisible = false;
        }
    }
    else {
        const u32 free_slots = ( army1 == army2 ? 1 : 0 ) + army2->Size() - army2->GetCount();
        const u32 max_count = save_last_troop ? troop1.GetCount() - 1 : troop1.GetCount();
        u32 redistr_count = troop1.GetCount() / 2;
        const u32 slots = Dialog::ArmySplitTroop( ( free_slots > max_count ? max_count : free_slots ), max_count, redistr_count, save_last_troop );

        switch ( slots ) {
        case 3:
        case 4:
        case 5:
            if ( save_last_troop ) {
                const Troop troop( troop1, troop1.GetCount() - 1 );
                troop1.SetCount( 1 );
                const_cast<Army *>( army2 )->SplitTroopIntoFreeSlots( troop, slots );
            }
            else {
                const Troop troop( troop1 );
                troop1.Reset();
                const_cast<Army *>( army2 )->SplitTroopIntoFreeSlots( troop, slots );
            }
            break;

        case 2:
            // this logic is used when splitting to a stack with the same unit
            if ( troop1.GetID() == troop2.GetID() )
                troop2.SetCount( troop2.GetCount() + redistr_count );
            else
                troop2.Set( troop1, redistr_count );

            troop1.SetCount( troop1.GetCount() - redistr_count );
            break;

        default:
            break;
        }
    }
}

void RedistributeArmyByOne( ArmyTroop & troopFrom, Army * armyTarget )
{
    // can't split up a stack with just 1 unit...
    if ( troopFrom.GetCount() <= 1 )
        return;

    const uint32_t freeSlots = armyTarget->Size() - armyTarget->GetCount();
    if ( freeSlots == 0 )
        return;

    armyTarget->AssignToFirstFreeSlot( troopFrom, 1 );
    troopFrom.SetCount( troopFrom.GetCount() - 1 );
}

ArmyBar::ArmyBar( Army * ptr, bool mini, bool ro, bool change /* false */ )
    : army( NULL )
    , spcursor( fheroes2::AGG::GetICN( ICN::STRIP, 1 ) )
    , use_mini_sprite( mini )
    , read_only( ro )
    , can_change( change )
    , _isTroopInfoVisible( true )
{
    if ( use_mini_sprite )
        SetBackground( Size( 43, 43 ), fheroes2::GetColorId( 0, 45, 0 ) );
    else {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::STRIP, 2 );
        SetItemSize( sprite.width(), sprite.height() );
    }

    SetArmy( ptr );
}

void ArmyBar::SetArmy( Army * ptr )
{
    if ( army && isSelected() )
        ResetSelected();

    army = ptr;
    items.clear();

    if ( ptr )
        for ( u32 ii = 0; ii < ptr->Size(); ++ii )
            items.push_back( reinterpret_cast<ArmyTroop *>( ptr->GetTroop( ii ) ) );

    SetContentItems();
}

bool ArmyBar::isValid( void ) const
{
    return army != NULL;
}

void ArmyBar::SetBackground( const Size & sz, const uint8_t fillColor )
{
    if ( use_mini_sprite ) {
        SetItemSize( sz.w, sz.h );

        backsf.resize( sz.w, sz.h );
        backsf.fill( fillColor );

        fheroes2::DrawBorder( backsf, fheroes2::GetColorId( 0xd0, 0xc0, 0x48 ) );

        spcursor.resize( sz.w, sz.h );
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
        Text text( GetString( troop.GetCount() ), ( use_mini_sprite ? Font::SMALL : Font::BIG ) );

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
    if ( isSelected() ) {
        ArmyTroop * troop2 = GetSelectedItem();

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
                ResetSelected();
                RedistributeArmy( *selectedTroop, troop, _isTroopInfoVisible );

                return false;
            }
        }

        // combine
        if ( selectedTroop && troop.GetID() == selectedTroop->GetID() ) {
            troop.SetCount( troop.GetCount() + selectedTroop->GetCount() );
            selectedTroop->Reset();
        }
        // exchange
        else
            Army::SwapTroops( troop, *selectedTroop );

        return false; // reset cursor
    }
    else if ( troop.isValid() ) {
        if ( !read_only ) // select
        {
            if ( Game::HotKeyHoldEvent( Game::EVENT_STACKSPLIT_CTRL ) ) {
                RedistributeArmyByOne( troop, army );
                return false;
            }

            Cursor::Get().Hide();
            spcursor.hide();
        }
    }
    else {
        if ( can_change ) // add troop
        {
            int cur = Monster::UNKNOWN;

            if ( army->GetCommander() )
                switch ( army->GetCommander()->GetRace() ) {
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
            ResetSelected();
            RedistributeArmy( selectedTroop, destTroop, _isTroopInfoVisible );
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
    ArmyTroop * troop2 = GetSelectedItem();

    if ( &troop == troop2 ) {
        int flags = ( read_only || army->SaveLastTroop() ? Dialog::READONLY | Dialog::BUTTONS : Dialog::BUTTONS );
        const Castle * castle = army->inCastle();

        if ( troop.isAllowUpgrade() &&
             // allow upgrade
             castle && castle->GetRace() == troop.GetRace() && castle->isBuild( troop.GetUpgrade().GetDwelling() ) ) {
            flags |= Dialog::UPGRADE;

            if ( !world.GetKingdom( army->GetColor() ).AllowPayment( troop.GetUpgradeCost() ) )
                flags |= Dialog::UPGRADE_DISABLE;
        }

        switch ( Dialog::ArmyInfo( troop, flags ) ) {
        case Dialog::UPGRADE:
            world.GetKingdom( army->GetColor() ).OddFundsResource( troop.GetUpgradeCost() );
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
        RedistributeArmy( *troopPress, troop, _isTroopInfoVisible );
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
    if ( troop.isValid() && _isTroopInfoVisible ) {
        ResetSelected();

        if ( can_change && !army->SaveLastTroop() )
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

    if ( troop.GetID() == selectedTroop.GetID() )
        return false;

    if ( !troop.isValid() || selectedTroop.GetMonster() == troop.GetMonster() ) {
        ResetSelected();
        RedistributeArmy( selectedTroop, troop, _isTroopInfoVisible );
    }

    return true;
}

bool ArmyBar::ActionBarRightMouseSingleClick( ArmyTroop & destTroop, ArmyTroop & selectedTroop )
{
    if ( !destTroop.isValid() || destTroop.GetMonster() == selectedTroop.GetMonster() ) {
        ResetSelected();
        RedistributeArmy( selectedTroop, destTroop, _isTroopInfoVisible );
    }

    return true;
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
