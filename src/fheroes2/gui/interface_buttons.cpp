/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "interface_buttons.h"
#include "agg.h"
#include "dialog.h"
#include "game.h"
#include "game_interface.h"
#include "heroes.h"
#include "settings.h"
#include "world.h"

Interface::ButtonsArea::ButtonsArea( Basic & basic )
    : BorderWindow( Rect( 0, 0, 144, 72 ) )
    , interface( basic )
{}

void Interface::ButtonsArea::SavePosition( void )
{
    Settings::Get().SetPosButtons( GetRect() );
}

void Interface::ButtonsArea::SetRedraw( void ) const
{
    interface.SetRedraw( REDRAW_BUTTONS );
}

void Interface::ButtonsArea::SetPos( s32 ox, s32 oy )
{
    BorderWindow::SetPosition( ox, oy );

    const int icnbtn = Settings::Get().ExtGameEvilInterface() ? ICN::ADVEBTNS : ICN::ADVBTNS;

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

    buttonMovement.setPosition( nextHeroRect.x + nextHeroRect.w, oy );
    movementRect = buttonMovement.area();

    buttonKingdom.setPosition( movementRect.x + movementRect.w, oy );
    kingdomRect = buttonKingdom.area();

    buttonSpell.setPosition( kingdomRect.x + kingdomRect.w, oy );
    spellRect = buttonSpell.area();

    // Bottom row
    oy = nextHeroRect.y + nextHeroRect.h;

    buttonEndTurn.setPosition( ox, oy );
    endTurnRect = buttonEndTurn.area();

    buttonAdventure.setPosition( endTurnRect.x + endTurnRect.w, oy );
    adventureRect = buttonAdventure.area();

    buttonFile.setPosition( adventureRect.x + adventureRect.w, oy );
    fileRect = buttonFile.area();

    buttonSystem.setPosition( fileRect.x + fileRect.w, oy );
    systemRect = buttonSystem.area();
}

void Interface::ButtonsArea::Redraw( void )
{
    const Settings & conf = Settings::Get();

    if ( !conf.ExtGameHideInterface() || conf.ShowButtons() ) {
        if ( conf.ExtGameHideInterface() )
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

void Interface::ButtonsArea::ResetButtons( void )
{
    if ( buttonNextHero.isEnabled() )
        buttonNextHero.drawOnRelease();
    buttonMovement.drawOnRelease();
    buttonKingdom.drawOnRelease();
    if ( buttonSpell.isEnabled() )
        buttonSpell.drawOnRelease();
    buttonEndTurn.drawOnRelease();
    buttonAdventure.drawOnRelease();
    buttonFile.drawOnRelease();
    buttonSystem.drawOnRelease();
    LocalEvent & le = LocalEvent::Get();
    le.ResetPressLeft();
}

int Interface::ButtonsArea::QueueEventProcessing( void )
{
    Settings & conf = Settings::Get();
    LocalEvent & le = LocalEvent::Get();
    int res = Game::CANCEL;

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

    if ( conf.ShowButtons() &&
         // move border window
         BorderWindow::QueueEventProcessing() ) {
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
        Dialog::Message( _( "Next Hero" ), _( "Select the next Hero." ), Font::BIG );
    else if ( le.MousePressRight( movementRect ) )
        Dialog::Message( _( "Continue Movement" ), _( "Continue the Hero's movement along the current path." ), Font::BIG );
    else if ( le.MousePressRight( kingdomRect ) )
        Dialog::Message( _( "Kingdom Summary" ), _( "View a Summary of your Kingdom." ), Font::BIG );
    else if ( le.MousePressRight( spellRect ) )
        Dialog::Message( _( "Cast Spell" ), _( "Cast an adventure spell." ), Font::BIG );
    else if ( le.MousePressRight( endTurnRect ) )
        Dialog::Message( _( "End Turn" ), _( "End your turn and left the computer take its turn." ), Font::BIG );
    else if ( le.MousePressRight( adventureRect ) )
        Dialog::Message( _( "Adventure Options" ), _( "Bring up the adventure options menu." ), Font::BIG );
    else if ( le.MousePressRight( fileRect ) )
        Dialog::Message( _( "File Options" ), _( "Bring up the file options menu, alloving you to load menu, save etc." ), Font::BIG );
    else if ( le.MousePressRight( systemRect ) )
        Dialog::Message( _( "System Options" ), _( "Bring up the system options menu, alloving you to customize your game." ), Font::BIG );

    return res;
}

void Interface::ButtonsArea::SetButtonStatus()
{
    Heroes * currentHero = GetFocusHeroes();
    if ( currentHero == NULL || !currentHero->GetPath().isValid() || !currentHero->MayStillMove() )
        buttonMovement.disable();
    else
        buttonMovement.enable();

    if ( currentHero == NULL || !currentHero->HaveSpellBook() )
        buttonSpell.disable();
    else
        buttonSpell.enable();

    const Kingdom & kingdom = world.GetKingdom( Settings::Get().CurrentColor() );
    const KingdomHeroes & heroes = kingdom.GetHeroes();

    bool isMovableHeroPresent = false;
    for ( size_t i = 0; i < heroes.size(); ++i ) {
        if ( heroes[i]->MayStillMove() ) {
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
