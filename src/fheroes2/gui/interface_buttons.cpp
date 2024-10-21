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
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_dialog.h"
#include "world.h"

void Interface::ButtonsPanel::SavePosition()
{
    Settings & conf = Settings::Get();

    conf.SetPosButtons( GetRect().getPosition() );
    conf.Save( Settings::configFileName );
}

void Interface::ButtonsPanel::setRedraw() const
{
    _interface.setRedraw( REDRAW_BUTTONS );
}

void Interface::ButtonsPanel::SetPos( int32_t x, int32_t y )
{
    BorderWindow::SetPosition( x, y );

    const int icnbtn = Settings::Get().isEvilInterfaceEnabled() ? ICN::ADVEBTNS : ICN::ADVBTNS;

    _buttonNextHero.setICNInfo( icnbtn, 0, 1 );
    _buttonHeroMovement.setICNInfo( icnbtn, 2, 3 );
    _buttonKingdom.setICNInfo( icnbtn, 4, 5 );
    _buttonSpell.setICNInfo( icnbtn, 6, 7 );
    _buttonEndTurn.setICNInfo( icnbtn, 8, 9 );
    _buttonAdventure.setICNInfo( icnbtn, 10, 11 );
    _buttonFile.setICNInfo( icnbtn, 12, 13 );
    _buttonSystem.setICNInfo( icnbtn, 14, 15 );

    _setButtonStatus();

    x = GetArea().x;
    y = GetArea().y;

    // Top row
    _buttonNextHero.setPosition( x, y );
    _nextHeroRect = _buttonNextHero.area();

    _buttonHeroMovement.setPosition( _nextHeroRect.x + _nextHeroRect.width, y );
    _heroMovementRect = _buttonHeroMovement.area();

    _buttonKingdom.setPosition( _heroMovementRect.x + _heroMovementRect.width, y );
    _kingdomRect = _buttonKingdom.area();

    _buttonSpell.setPosition( _kingdomRect.x + _kingdomRect.width, y );
    _spellRect = _buttonSpell.area();

    // Bottom row
    y = _nextHeroRect.y + _nextHeroRect.height;

    _buttonEndTurn.setPosition( x, y );
    _endTurnRect = _buttonEndTurn.area();

    _buttonAdventure.setPosition( _endTurnRect.x + _endTurnRect.width, y );
    _adventureRect = _buttonAdventure.area();

    _buttonFile.setPosition( _adventureRect.x + _adventureRect.width, y );
    _fileRect = _buttonFile.area();

    _buttonSystem.setPosition( _fileRect.x + _fileRect.width, y );
    _systemRect = _buttonSystem.area();
}

void Interface::ButtonsPanel::_redraw()
{
    const Settings & conf = Settings::Get();

    if ( conf.isHideInterfaceEnabled() ) {
        if ( !conf.ShowButtons() ) {
            return;
        }

        BorderWindow::Redraw();
    }

    _setButtonStatus();

    _buttonNextHero.draw();
    _buttonHeroMovement.draw();
    _buttonKingdom.draw();
    _buttonSpell.draw();
    _buttonEndTurn.draw();
    _buttonAdventure.draw();
    _buttonFile.draw();
    _buttonSystem.draw();
}

fheroes2::GameMode Interface::ButtonsPanel::queueEventProcessing()
{
    captureMouse();

    LocalEvent & le = LocalEvent::Get();

    // In the "no interface" mode, the buttons panel may be overlapped by other UI elements, so we can't render the
    // pressed or released buttons exclusively in a usual way. The overlapping UI elements should also be rendered.
    const auto drawOnPressOrRelease = [this, &le]( fheroes2::Button & button, const fheroes2::Rect & buttonRect ) {
        bool shouldRedraw = false;

        if ( le.isMouseLeftButtonPressedInArea( buttonRect ) ) {
            if ( !button.isPressed() && button.press() ) {
                shouldRedraw = true;
            }
        }
        else {
            if ( !button.isReleased() && button.release() ) {
                shouldRedraw = true;
            }
        }

        if ( shouldRedraw ) {
            _interface.redraw( REDRAW_BUTTONS );

            // Render now only the changed part of the screen.
            fheroes2::Display::instance().render( buttonRect );
        }
    };

    drawOnPressOrRelease( _buttonNextHero, _nextHeroRect );
    drawOnPressOrRelease( _buttonHeroMovement, _heroMovementRect );
    drawOnPressOrRelease( _buttonKingdom, _kingdomRect );
    drawOnPressOrRelease( _buttonSpell, _spellRect );
    drawOnPressOrRelease( _buttonEndTurn, _endTurnRect );
    drawOnPressOrRelease( _buttonAdventure, _adventureRect );
    drawOnPressOrRelease( _buttonFile, _fileRect );
    drawOnPressOrRelease( _buttonSystem, _systemRect );

    fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

    // Move the window border.
    if ( Settings::Get().ShowButtons() && BorderWindow::QueueEventProcessing() ) {
        setRedraw();
    }
    else if ( _buttonNextHero.isEnabled() && le.MouseClickLeft( _nextHeroRect ) ) {
        _interface.EventNextHero();
    }
    else if ( _buttonHeroMovement.isEnabled() && le.MouseClickLeft( _heroMovementRect ) ) {
        res = _interface.EventHeroMovement();
    }
    else if ( _buttonHeroMovement.isEnabled() && le.MouseLongPressLeft( _heroMovementRect ) ) {
        _interface.EventResetHeroPath();
    }
    else if ( le.MouseClickLeft( _kingdomRect ) ) {
        _interface.EventKingdomInfo();
    }
    else if ( _buttonSpell.isEnabled() && le.MouseClickLeft( _spellRect ) ) {
        _interface.EventCastSpell();
    }
    else if ( le.MouseClickLeft( _endTurnRect ) ) {
        res = _interface.EventEndTurn();
    }
    else if ( le.MouseClickLeft( _adventureRect ) ) {
        res = _interface.EventAdventureDialog();
    }
    else if ( le.MouseClickLeft( _fileRect ) ) {
        res = _interface.EventFileDialog();
    }
    else if ( le.MouseClickLeft( _systemRect ) ) {
        _interface.EventSystemDialog();
    }
    else if ( le.isMouseRightButtonPressedInArea( _nextHeroRect ) ) {
        fheroes2::showStandardTextMessage( _( "Next Hero" ), _( "Select the next Hero." ), Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( _heroMovementRect ) ) {
        fheroes2::showStandardTextMessage(
            _( "Hero Movement" ),
            _( "Start the Hero's movement along the current path or re-visit the object occupied by the Hero. Press and hold this button to reset the Hero's path." ),
            Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( _kingdomRect ) ) {
        fheroes2::showStandardTextMessage( _( "Kingdom Summary" ), _( "View a summary of your Kingdom." ), Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( _spellRect ) ) {
        fheroes2::showStandardTextMessage( _( "Cast Spell" ), _( "Cast an adventure spell." ), Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( _endTurnRect ) ) {
        fheroes2::showStandardTextMessage( _( "End Turn" ), _( "End your turn and let the computer take its turn." ), Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( _adventureRect ) ) {
        fheroes2::showStandardTextMessage( _( "Adventure Options" ), _( "Bring up the adventure options menu." ), Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( _fileRect ) ) {
        fheroes2::showStandardTextMessage( _( "File Options" ), _( "Bring up the file options menu, allowing you to load, save, start a new game or quit." ),
                                           Dialog::ZERO );
    }
    else if ( le.isMouseRightButtonPressedInArea( _systemRect ) ) {
        fheroes2::showStandardTextMessage( _( "System Options" ), _( "Bring up the system options menu, allowing you to customize your game." ), Dialog::ZERO );
    }

    return res;
}

void Interface::ButtonsPanel::_setButtonStatus()
{
    Heroes * currentHero = GetFocusHeroes();

    if ( currentHero && currentHero->GetPath().isValidForMovement() && currentHero->MayStillMove( false, true ) ) {
        _buttonHeroMovement.setICNIndexes( 2, 3 );
        _buttonHeroMovement.enable();
    }
    else if ( currentHero && MP2::isInGameActionObject( currentHero->getObjectTypeUnderHero(), currentHero->isShipMaster() ) ) {
        _buttonHeroMovement.setICNIndexes( 16, 17 );
        _buttonHeroMovement.enable();
    }
    else {
        _buttonHeroMovement.setICNIndexes( 2, 3 );
        _buttonHeroMovement.disable();
    }

    if ( currentHero && currentHero->HaveSpellBook() && currentHero->MayCastAdventureSpells() ) {
        _buttonSpell.enable();
    }
    else {
        _buttonSpell.disable();
    }

    const Kingdom & kingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    const VecHeroes & heroes = kingdom.GetHeroes();

    const bool isMovableHeroPresent = std::any_of( heroes.begin(), heroes.end(), []( const Heroes * hero ) {
        assert( hero != nullptr );

        return hero->MayStillMove( false, false );
    } );

    if ( isMovableHeroPresent ) {
        _buttonNextHero.enable();
    }
    else {
        _buttonNextHero.disable();
    }
}
