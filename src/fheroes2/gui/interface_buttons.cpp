/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "interface_buttons.h"

#include <algorithm>
#include <cassert>

#include "dialog.h"
#include "game_interface.h"
#include "heroes.h"
#include "icn.h"
#include "interface_base.h"
#include "kingdom.h"
#include "localevent.h"
#include "mp2.h"
#include "route.h"
#include "settings.h"
#include "translations.h"
#include "ui_dialog.h"
#include "world.h"

Interface::ButtonsArea::ButtonsArea( AdventureMap & basic )
    : BorderWindow( { 0, 0, 144, 72 } )
    , interface( basic )
{}

void Interface::ButtonsArea::SavePosition()
{
    Settings & conf = Settings::Get();

    conf.SetPosButtons( GetRect().getPosition() );
    conf.Save( Settings::configFileName );
}

void Interface::ButtonsArea::SetRedraw() const
{
    interface.setRedraw( REDRAW_BUTTONS );
}

void Interface::ButtonsArea::SetPos( int32_t ox, int32_t oy )
{
    BorderWindow::SetPosition( ox, oy );

    const int icnbtn = Settings::Get().isEvilInterfaceEnabled() ? ICN::ADVEBTNS : ICN::ADVBTNS;

    buttonNextHero.setICNInfo( icnbtn, 0, 1 );
    buttonHeroMovement.setICNInfo( icnbtn, 2, 3 );
    buttonKingdom.setICNInfo( icnbtn, 4, 5 );
    buttonSpell.setICNInfo( icnbtn, 6, 7 );
    buttonEndTurn.setICNInfo( icnbtn, 8, 9 );
    buttonAdventure.setICNInfo( icnbtn, 10, 11 );
    buttonFile.setICNInfo( icnbtn, 12, 13 );
    buttonSystem.setICNInfo( icnbtn, 14, 15 );

    SetButtonStatus();

    ox = GetArea().x;
    oy = GetArea().y;

    // Top row
    buttonNextHero.setPosition( ox, oy );
    nextHeroRect = buttonNextHero.area();

    buttonHeroMovement.setPosition( nextHeroRect.x + nextHeroRect.width, oy );
    heroMovementRect = buttonHeroMovement.area();

    buttonKingdom.setPosition( heroMovementRect.x + heroMovementRect.width, oy );
    kingdomRect = buttonKingdom.area();

    buttonSpell.setPosition( kingdomRect.x + kingdomRect.width, oy );
    spellRect = buttonSpell.area();

    // Bottom row
    oy = nextHeroRect.y + nextHeroRect.height;

    buttonEndTurn.setPosition( ox, oy );
    endTurnRect = buttonEndTurn.area();

    buttonAdventure.setPosition( endTurnRect.x + endTurnRect.width, oy );
    adventureRect = buttonAdventure.area();

    buttonFile.setPosition( adventureRect.x + adventureRect.width, oy );
    fileRect = buttonFile.area();

    buttonSystem.setPosition( fileRect.x + fileRect.width, oy );
    systemRect = buttonSystem.area();
}

void Interface::ButtonsArea::_redraw()
{
    const Settings & conf = Settings::Get();

    if ( !conf.isHideInterfaceEnabled() || conf.ShowButtons() ) {
        if ( conf.isHideInterfaceEnabled() )
            BorderWindow::Redraw();

        SetButtonStatus();

        buttonNextHero.draw();
        buttonHeroMovement.draw();
        buttonKingdom.draw();
        buttonSpell.draw();
        buttonEndTurn.draw();
        buttonAdventure.draw();
        buttonFile.draw();
        buttonSystem.draw();
    }
}

void Interface::ButtonsArea::ResetButtons()
{
    buttonNextHero.drawOnRelease();
    buttonHeroMovement.drawOnRelease();
    buttonKingdom.drawOnRelease();
    buttonSpell.drawOnRelease();
    buttonEndTurn.drawOnRelease();
    buttonAdventure.drawOnRelease();
    buttonFile.drawOnRelease();
    buttonSystem.drawOnRelease();
}

fheroes2::GameMode Interface::ButtonsArea::QueueEventProcessing()
{
    LocalEvent & le = LocalEvent::Get();

    le.isMouseLeftButtonPressedInArea( nextHeroRect ) ? buttonNextHero.drawOnPress() : buttonNextHero.drawOnRelease();
    le.isMouseLeftButtonPressedInArea( heroMovementRect ) ? buttonHeroMovement.drawOnPress() : buttonHeroMovement.drawOnRelease();
    le.isMouseLeftButtonPressedInArea( kingdomRect ) ? buttonKingdom.drawOnPress() : buttonKingdom.drawOnRelease();
    le.isMouseLeftButtonPressedInArea( spellRect ) ? buttonSpell.drawOnPress() : buttonSpell.drawOnRelease();
    le.isMouseLeftButtonPressedInArea( endTurnRect ) ? buttonEndTurn.drawOnPress() : buttonEndTurn.drawOnRelease();
    le.isMouseLeftButtonPressedInArea( adventureRect ) ? buttonAdventure.drawOnPress() : buttonAdventure.drawOnRelease();
    le.isMouseLeftButtonPressedInArea( fileRect ) ? buttonFile.drawOnPress() : buttonFile.drawOnRelease();
    le.isMouseLeftButtonPressedInArea( systemRect ) ? buttonSystem.drawOnPress() : buttonSystem.drawOnRelease();

    fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

    // Move border window
    if ( Settings::Get().ShowButtons() && BorderWindow::QueueEventProcessing() ) {
        SetRedraw();
    }
    else if ( buttonNextHero.isEnabled() && le.MouseClickLeft( nextHeroRect ) ) {
        interface.EventNextHero();
    }
    else if ( buttonHeroMovement.isEnabled() && le.MouseClickLeft( heroMovementRect ) ) {
        res = interface.EventHeroMovement();
    }
    else if ( buttonHeroMovement.isEnabled() && le.MouseLongPressLeft( heroMovementRect ) ) {
        interface.EventResetHeroPath();
    }
    else if ( le.MouseClickLeft( kingdomRect ) ) {
        interface.EventKingdomInfo();
    }
    else if ( buttonSpell.isEnabled() && le.MouseClickLeft( spellRect ) ) {
        interface.EventCastSpell();
    }
    else if ( le.MouseClickLeft( endTurnRect ) ) {
        res = interface.EventEndTurn();
    }
    else if ( le.MouseClickLeft( adventureRect ) ) {
        res = interface.EventAdventureDialog();
    }
    else if ( le.MouseClickLeft( fileRect ) ) {
        res = interface.EventFileDialog();
    }
    else if ( le.MouseClickLeft( systemRect ) ) {
        interface.EventSystemDialog();
    }

    if ( le.isMouseRightButtonPressedInArea( nextHeroRect ) ) {
        fheroes2::showStandardTextMessage( _( "Next Hero" ), _( "Select the next Hero." ), Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( heroMovementRect ) ) {
        fheroes2::showStandardTextMessage(
            _( "Hero Movement" ),
            _( "Start the Hero's movement along the current path or re-visit the object occupied by the Hero. Press and hold this button to reset the Hero's path." ),
            Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( kingdomRect ) ) {
        fheroes2::showStandardTextMessage( _( "Kingdom Summary" ), _( "View a summary of your Kingdom." ), Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( spellRect ) ) {
        fheroes2::showStandardTextMessage( _( "Cast Spell" ), _( "Cast an adventure spell." ), Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( endTurnRect ) ) {
        fheroes2::showStandardTextMessage( _( "End Turn" ), _( "End your turn and let the computer take its turn." ), Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( adventureRect ) ) {
        fheroes2::showStandardTextMessage( _( "Adventure Options" ), _( "Bring up the adventure options menu." ), Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( fileRect ) ) {
        fheroes2::showStandardTextMessage( _( "File Options" ), _( "Bring up the file options menu, allowing you to load, save, start a new game or quit." ),
                                           Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( systemRect ) ) {
        fheroes2::showStandardTextMessage( _( "System Options" ), _( "Bring up the system options menu, allowing you to customize your game." ), Dialog::ZERO );
    }

    return res;
}

void Interface::ButtonsArea::SetButtonStatus()
{
    Heroes * currentHero = GetFocusHeroes();

    if ( currentHero && currentHero->GetPath().isValidForMovement() && currentHero->MayStillMove( false, true ) ) {
        buttonHeroMovement.setICNIndexes( 2, 3 );
        buttonHeroMovement.enable();
    }
    else if ( currentHero && MP2::isInGameActionObject( currentHero->getObjectTypeUnderHero(), currentHero->isShipMaster() ) ) {
        buttonHeroMovement.setICNIndexes( 16, 17 );
        buttonHeroMovement.enable();
    }
    else {
        buttonHeroMovement.setICNIndexes( 2, 3 );
        buttonHeroMovement.disable();
    }

    if ( currentHero && currentHero->HaveSpellBook() && currentHero->MayCastAdventureSpells() ) {
        buttonSpell.enable();
    }
    else {
        buttonSpell.disable();
    }

    const Kingdom & kingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    const VecHeroes & heroes = kingdom.GetHeroes();

    const bool isMovableHeroPresent = std::any_of( heroes.begin(), heroes.end(), []( const Heroes * hero ) {
        assert( hero != nullptr );

        return hero->MayStillMove( false, false );
    } );

    if ( isMovableHeroPresent ) {
        buttonNextHero.enable();
    }
    else {
        buttonNextHero.disable();
    }
}
