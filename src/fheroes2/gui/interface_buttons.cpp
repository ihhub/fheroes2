/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#include <cstddef>
#include <vector>

#include "dialog.h"
#include "game_interface.h"
#include "heroes.h"
#include "icn.h"
#include "kingdom.h"
#include "localevent.h"
#include "route.h"
#include "settings.h"
#include "translations.h"
#include "ui_dialog.h"
#include "world.h"

Interface::ButtonsArea::ButtonsArea( Basic & basic )
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
    interface.SetRedraw( REDRAW_BUTTONS );
}

void Interface::ButtonsArea::SetPos( int32_t ox, int32_t oy )
{
    BorderWindow::SetPosition( ox, oy );

    const int icnbtn = Settings::Get().isEvilInterfaceEnabled() ? ICN::ADVEBTNS : ICN::ADVBTNS;

    buttonNextHero.setICNInfo( icnbtn, 0, 1 );
    buttonMovement.setICNInfo( icnbtn, 2, 3 );
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

    buttonMovement.setPosition( nextHeroRect.x + nextHeroRect.width, oy );
    movementRect = buttonMovement.area();

    buttonKingdom.setPosition( movementRect.x + movementRect.width, oy );
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

void Interface::ButtonsArea::Redraw()
{
    const Settings & conf = Settings::Get();

    if ( !conf.isHideInterfaceEnabled() || conf.ShowButtons() ) {
        if ( conf.isHideInterfaceEnabled() )
            BorderWindow::Redraw();

        SetButtonStatus();

        buttonNextHero.draw();
        buttonMovement.draw();
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
    if ( buttonNextHero.isEnabled() ) {
        buttonNextHero.drawOnRelease();
    }

    buttonMovement.drawOnRelease();
    buttonKingdom.drawOnRelease();

    if ( buttonSpell.isEnabled() ) {
        buttonSpell.drawOnRelease();
    }

    buttonEndTurn.drawOnRelease();
    buttonAdventure.drawOnRelease();
    buttonFile.drawOnRelease();
    buttonSystem.drawOnRelease();
}

fheroes2::GameMode Interface::ButtonsArea::QueueEventProcessing()
{
    LocalEvent & le = LocalEvent::Get();
    fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

    if ( buttonNextHero.isEnabled() )
        le.MousePressLeft( nextHeroRect ) ? buttonNextHero.drawOnPress() : buttonNextHero.drawOnRelease();
    le.MousePressLeft( movementRect ) ? buttonMovement.drawOnPress() : buttonMovement.drawOnRelease();
    le.MousePressLeft( kingdomRect ) ? buttonKingdom.drawOnPress() : buttonKingdom.drawOnRelease();
    if ( buttonSpell.isEnabled() )
        le.MousePressLeft( spellRect ) ? buttonSpell.drawOnPress() : buttonSpell.drawOnRelease();
    le.MousePressLeft( endTurnRect ) ? buttonEndTurn.drawOnPress() : buttonEndTurn.drawOnRelease();
    le.MousePressLeft( adventureRect ) ? buttonAdventure.drawOnPress() : buttonAdventure.drawOnRelease();
    le.MousePressLeft( fileRect ) ? buttonFile.drawOnPress() : buttonFile.drawOnRelease();
    le.MousePressLeft( systemRect ) ? buttonSystem.drawOnPress() : buttonSystem.drawOnRelease();

    // Move border window
    if ( Settings::Get().ShowButtons() && BorderWindow::QueueEventProcessing() ) {
        SetRedraw();
    }
    else if ( buttonNextHero.isEnabled() && le.MouseClickLeft( nextHeroRect ) ) {
        interface.EventNextHero();
    }
    else if ( le.MouseClickLeft( movementRect ) ) {
        interface.EventContinueMovement();
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

    if ( le.MousePressRight( nextHeroRect ) )
        fheroes2::showStandardTextMessage( _( "Next Hero" ), _( "Select the next Hero." ), Dialog::ZERO );
    else if ( le.MousePressRight( movementRect ) )
        fheroes2::showStandardTextMessage( _( "Continue Movement" ), _( "Continue the Hero's movement along the current path." ), Dialog::ZERO );
    else if ( le.MousePressRight( kingdomRect ) )
        fheroes2::showStandardTextMessage( _( "Kingdom Summary" ), _( "View a Summary of your Kingdom." ), Dialog::ZERO );
    else if ( le.MousePressRight( spellRect ) )
        fheroes2::showStandardTextMessage( _( "Cast Spell" ), _( "Cast an adventure spell." ), Dialog::ZERO );
    else if ( le.MousePressRight( endTurnRect ) )
        fheroes2::showStandardTextMessage( _( "End Turn" ), _( "End your turn and left the computer take its turn." ), Dialog::ZERO );
    else if ( le.MousePressRight( adventureRect ) )
        fheroes2::showStandardTextMessage( _( "Adventure Options" ), _( "Bring up the adventure options menu." ), Dialog::ZERO );
    else if ( le.MousePressRight( fileRect ) )
        fheroes2::showStandardTextMessage( _( "File Options" ), _( "Bring up the file options menu, allowing you to load, save, start a new game or quit." ),
                                           Dialog::ZERO );
    else if ( le.MousePressRight( systemRect ) )
        fheroes2::showStandardTextMessage( _( "System Options" ), _( "Bring up the system options menu, allowing you to customize your game." ), Dialog::ZERO );

    return res;
}

void Interface::ButtonsArea::SetButtonStatus()
{
    Heroes * currentHero = GetFocusHeroes();
    if ( currentHero == nullptr || !currentHero->GetPath().isValid() || !currentHero->MayStillMove( false, true ) )
        buttonMovement.disable();
    else
        buttonMovement.enable();

    if ( currentHero == nullptr || !currentHero->HaveSpellBook() || !currentHero->MayCastAdventureSpells() )
        buttonSpell.disable();
    else
        buttonSpell.enable();

    const Kingdom & kingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    const KingdomHeroes & heroes = kingdom.GetHeroes();

    bool isMovableHeroPresent = false;
    for ( size_t i = 0; i < heroes.size(); ++i ) {
        if ( heroes[i]->MayStillMove( false, false ) ) {
            isMovableHeroPresent = true;
            break;
        }
    }

    if ( isMovableHeroPresent ) {
        buttonNextHero.enable();
    }
    else {
        buttonNextHero.disable();
    }
}
