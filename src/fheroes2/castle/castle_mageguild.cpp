/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

#include <cstdint>
#include <string>
#include <utility>

#include "agg_image.h"
#include "castle.h" // IWYU pragma: associated
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "localevent.h"
#include "mageguild.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "spell_storage.h"
#include "statusbar.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_mage_guild.h"
#include "ui_tool.h"

namespace
{
    const int32_t bottomBarOffsetY = 461;
}

Castle::MageGuildDialogResult Castle::_openMageGuild( const Heroes * hero ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::ImageRestorer restorer( display, ( display.width() - fheroes2::Display::DEFAULT_WIDTH ) / 2,
                                            ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2, fheroes2::Display::DEFAULT_WIDTH,
                                            fheroes2::Display::DEFAULT_HEIGHT );

    const fheroes2::Point dialogOffset{ restorer.x(), restorer.y() };
    const fheroes2::Point bottomUIOffset{ dialogOffset.x, dialogOffset.y + bottomBarOffsetY };

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    fheroes2::Blit( fheroes2::AGG::GetICN( isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK, 0 ), display, dialogOffset.x, dialogOffset.y );

    // Create Previous Castle button.
    fheroes2::Button buttonPrevCastle( bottomUIOffset.x, bottomUIOffset.y, ICN::SMALLBAR, 1, 2 );
    fheroes2::TimedEventValidator timedButtonPrevCastle( [&buttonPrevCastle]() { return buttonPrevCastle.isPressed(); } );
    buttonPrevCastle.subscribe( &timedButtonPrevCastle );

    // Exit button.
    const int32_t exitWidth = fheroes2::AGG::GetICN( ICN::BUTTON_GUILDWELL_EXIT, 0 ).width();
    fheroes2::Button buttonExit( bottomUIOffset.x + fheroes2::Display::DEFAULT_WIDTH - exitWidth, bottomUIOffset.y, ICN::BUTTON_GUILDWELL_EXIT, 0, 1 );

    // Create Next Castle button.
    fheroes2::Button buttonNextCastle( buttonExit.area().x - fheroes2::AGG::GetICN( ICN::SMALLBAR, 3 ).width(), bottomUIOffset.y, ICN::SMALLBAR, 3, 4 );
    fheroes2::TimedEventValidator timedButtonNextCastle( [&buttonNextCastle]() { return buttonNextCastle.isPressed(); } );
    buttonNextCastle.subscribe( &timedButtonNextCastle );

    // Create the status bar UI element.
    const fheroes2::Sprite & bottomBar = fheroes2::AGG::GetICN( ICN::SMALLBAR, 0 );
    const int32_t barHeight = bottomBar.height();
    const fheroes2::Point bottonBarOffset{ buttonPrevCastle.area().x + buttonPrevCastle.area().width, bottomUIOffset.y };
    const int32_t bottomBarWidth{ fheroes2::Display::DEFAULT_WIDTH - exitWidth - buttonNextCastle.area().width - buttonPrevCastle.area().width };

    // ICN::SMALLBAR image's first column contains all black pixels. This should not be drawn.
    fheroes2::Copy( bottomBar, 1, 0, display, bottonBarOffset.x, bottonBarOffset.y, bottomBarWidth / 2, barHeight );
    fheroes2::Copy( bottomBar, bottomBar.width() - ( bottomBarWidth - bottomBarWidth / 2 ), 0, display, bottonBarOffset.x + bottomBarWidth / 2, bottonBarOffset.y,
                    bottomBarWidth - bottomBarWidth / 2, barHeight );

    StatusBar statusBar;
    // Status bar must be smaller due to extra art on both sides.
    statusBar.setRoi( { bottomUIOffset.x + 16, bottomUIOffset.y, bottomBarWidth, 0 } );

    // Default text for status bar.
    std::string defaultStatusBarText;
    if ( hero == nullptr || !hero->HaveSpellBook() ) {
        defaultStatusBarText = _( "The above spells are available here." );
    }
    else {
        defaultStatusBarText = _( "The spells the hero can learn have been added to their book." );
    }

    const int guildLevel = GetLevelMageGuild();

    fheroes2::renderMageGuildBuilding( _race, guildLevel, dialogOffset );

    const bool haveLibraryCapability = HaveLibraryCapability();
    const bool hasLibrary = isLibraryBuilt();

    std::array<std::unique_ptr<fheroes2::SpellsInOneRow>, 5> spellRows;

    for ( size_t levelIndex = 0; levelIndex < spellRows.size(); ++levelIndex ) {
        const int32_t spellsLevel = static_cast<int32_t>( levelIndex ) + 1;
        const int32_t count = MageGuild::getMaxSpellsCount( spellsLevel, haveLibraryCapability );

        SpellStorage spells = GetMageGuild().GetSpells( guildLevel, hasLibrary, spellsLevel );
        spells.resize( count, Spell::NONE );

        spellRows[levelIndex] = std::make_unique<fheroes2::SpellsInOneRow>( std::move( spells ) );

        spellRows[levelIndex]->setPosition( { dialogOffset.x + 250, dialogOffset.y + 365 - 90 * static_cast<int32_t>( levelIndex ) } );
        spellRows[levelIndex]->redraw( display );
    }

    if ( GetKingdom().GetCastles().size() < 2 ) {
        buttonPrevCastle.disable();
        buttonNextCastle.disable();
    }

    buttonPrevCastle.draw();
    buttonNextCastle.draw();

    buttonExit.draw();

    LocalEvent & le = LocalEvent::Get();

    auto updateStatusBar = [&le, &buttonExit, &buttonPrevCastle, &buttonNextCastle, &statusBar, &defaultStatusBarText]() {
        std::string statusMessage;

        if ( le.isMouseCursorPosInArea( buttonExit.area() ) ) {
            statusMessage = _( "Exit Mage Guild" );
        }
        else if ( buttonPrevCastle.isEnabled() && le.isMouseCursorPosInArea( buttonPrevCastle.area() ) ) {
            statusMessage = _( "Show previous town" );
        }
        else if ( buttonNextCastle.isEnabled() && le.isMouseCursorPosInArea( buttonNextCastle.area() ) ) {
            statusMessage = _( "Show next town" );
        }

        if ( statusMessage.empty() ) {
            statusBar.ShowMessage( defaultStatusBarText );
        }
        else {
            statusBar.ShowMessage( statusMessage );
            statusMessage.clear();
        }
    };

    updateStatusBar();

    display.render( restorer.rect() );

    // message loop
    while ( le.HandleEvents() ) {
        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
            break;
        }

        buttonExit.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonExit.area() ) );

        if ( buttonPrevCastle.isEnabled() ) {
            buttonPrevCastle.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonPrevCastle.area() ) );

            if ( le.MouseClickLeft( buttonPrevCastle.area() ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_LEFT ) || timedButtonPrevCastle.isDelayPassed() ) {
                return MageGuildDialogResult::PrevMageGuildWindow;
            }
        }
        if ( buttonNextCastle.isEnabled() ) {
            buttonNextCastle.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonNextCastle.area() ) );

            if ( le.MouseClickLeft( buttonNextCastle.area() ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_RIGHT ) || timedButtonNextCastle.isDelayPassed() ) {
                return MageGuildDialogResult::NextMageGuildWindow;
            }
        }

        spellRows[0]->queueEventProcessing( false ) || spellRows[1]->queueEventProcessing( false ) || spellRows[2]->queueEventProcessing( false )
            || spellRows[3]->queueEventProcessing( false ) || spellRows[4]->queueEventProcessing( false );

        if ( le.isMouseRightButtonPressedInArea( buttonExit.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Exit" ), _( "Exit Mage Guild" ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonNextCastle.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Show next town" ), _( "Click to show the next town." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonPrevCastle.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Show previous town" ), _( "Click to show the previous town." ), Dialog::ZERO );
        }

        updateStatusBar();
    }

    return MageGuildDialogResult::DoNothing;
}
