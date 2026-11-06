/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include "multi_connection.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "exception.h"
#include "game_assets.h"
#include "game_exit.h"
#include "game_hotkeys.h"
#include "game_mode.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "logging.h"
#include "math_base.h"
#include "multi_game.h"
#include "multi_lobby.h"
#include "multi_machines.h"
#include "multi_menu.h"
#include "multi_network.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    // Shorter losses are not shown because the connection usually comes back before.
    constexpr std::chrono::seconds windowDelay{ 2 };

    enum class Choice
    {
        NONE,
        RETRY,
        GO_ON,
        QUIT
    };

    // Shown while the game is paused by a lost connection. A guest can try to connect again from it, the host can go on
    // without the players who are not there.
    class ReconnectionWindow
    {
    public:
        ReconnectionWindow( const bool isHostSide, const bool isRetrying )
            : _isHostSide( isHostSide )
        {
            _show( isRetrying );
        }

        // The guest stops trying after a while, then the window offers to try again.
        void setRetrying( const bool isRetrying )
        {
            if ( isRetrying != _isRetrying ) {
                _show( isRetrying );
            }
        }

        // Host only: who is missing.
        void setAbsentPlayers( std::string text )
        {
            if ( text != _absentPlayers ) {
                _absentPlayers = std::move( text );
                _show( _isRetrying );
            }
        }

        // Should be called after another dialog was shown over this window.
        void redraw()
        {
            _show( _isRetrying );
        }

        Choice processEvents()
        {
            LocalEvent & le = LocalEvent::Get();

            _quitButton.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( _quitButton.area() ) );
            if ( _hasOtherButton ) {
                _otherButton.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( _otherButton.area() ) );

                if ( le.MouseClickLeft( _otherButton.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
                    return _isHostSide ? Choice::GO_ON : Choice::RETRY;
                }
            }

            if ( le.MouseClickLeft( _quitButton.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return Choice::QUIT;
            }

            return Choice::NONE;
        }

    private:
        void _show( const bool isRetrying )
        {
            _isRetrying = isRetrying;

            // Restore the screen behind before drawing a window of a different size.
            _window.reset();

            std::string message = _( "The connection with the host is lost.\n\nConnecting again..." );
            if ( _isHostSide ) {
                message = _absentPlayers + "\n\n" + _( "The game waits for them. You can also go on: the computer plays for them until they come back." );
            }
            else if ( !_isRetrying ) {
                message = _( "The connection with the host could not be restored." );
            }

            fheroes2::Display & display = fheroes2::Display::instance();

            const fheroes2::Text title( _( "Network game" ), fheroes2::FontType::normalYellow() );
            const fheroes2::Text text( message, fheroes2::FontType::normalWhite() );

            const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
            const fheroes2::Sprite & buttonSample = Assets::getImage( isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0 );

            const int32_t textWidth = 300;
            const int32_t buttonWidth = 150;
            const int32_t buttonHeight = buttonSample.height();
            const int32_t titleHeight = title.height( textWidth );
            const int32_t textHeight = text.height( textWidth );

            _window = std::make_unique<fheroes2::StandardWindow>( textWidth + 40, 20 + titleHeight + 15 + textHeight + 25 + buttonHeight + 20, true, display );

            const fheroes2::Rect & area = _window->activeArea();
            title.draw( area.x + 20, area.y + 20, textWidth, display );
            text.draw( area.x + 20, area.y + 20 + titleHeight + 15, textWidth, display );

            const int32_t buttonY = area.y + area.height - 20 - buttonHeight;

            // The guest connects again, the host can go on without the others.
            _hasOtherButton = _isHostSide || !_isRetrying;

            if ( _hasOtherButton ) {
                _otherButton = Multi::makeTextButton( { area.x + area.width / 2 - buttonWidth - 10, buttonY, buttonWidth, buttonHeight },
                                                      _isHostSide ? gettext_noop( "network|GO ON" ) : gettext_noop( "TRY AGAIN" ) );
                _quitButton = Multi::makeTextButton( { area.x + area.width / 2 + 10, buttonY, buttonWidth, buttonHeight }, gettext_noop( "LEAVE" ) );

                _otherButton.draw();
                _otherButton.drawShadow( display );
            }
            else {
                _quitButton = Multi::makeTextButton( { area.x + ( area.width - buttonWidth ) / 2, buttonY, buttonWidth, buttonHeight }, gettext_noop( "LEAVE" ) );
            }

            _quitButton.draw();
            _quitButton.drawShadow( display );

            display.render( _window->totalArea() );
        }

        bool _isHostSide{ false };
        bool _isRetrying{ false };
        bool _hasOtherButton{ false };
        std::string _absentPlayers;

        std::unique_ptr<fheroes2::StandardWindow> _window;
        fheroes2::ButtonSprite _otherButton;
        fheroes2::ButtonSprite _quitButton;
    };

    // Closes the application as Quit in the file menu. The host can resume later from its last autosave.
    [[noreturn]] void quitGame()
    {
        VERBOSE_LOG( "[MULTI] The player left the game while the connection was lost" )

        Multi::endGame();
        throw fheroes2::UserRequestedApplicationClosure();
    }

    // One line for each guest the host waits for.
    std::string describeAbsentGuests()
    {
        std::string text;

        for ( const Multi::MachineInfo & info : Multi::getMachineTable().machines ) {
            const char * line = nullptr;
            if ( info.state == Multi::MachineState::LOST ) {
                line = gettext_noop( "The connection with %{name} is cut." );
            }
            else if ( info.state == Multi::MachineState::LEFT ) {
                line = gettext_noop( "%{name} has left the game." );
            }

            if ( line == nullptr ) {
                continue;
            }

            std::string described = _( line );
            StringReplace( described, "%{name}", Multi::getMachineName( info.id ) );

            if ( !text.empty() ) {
                text += '\n';
            }
            text += described;
        }

        return text;
    }
}

namespace
{
    // Self-tests give up after this delay, nobody clicks on the window.
    constexpr std::chrono::seconds testGiveUpDuration{ 90 };

    bool isAbsenceWindowTest{ false };

    // The guest tries to join a restarted host for this time, then waits for the player.
    constexpr std::chrono::seconds automaticJoinDuration{ 120 };

    // Guest only. Waits until the connection with the host is back or closed for good. No window in self-tests.
    void waitWhileReconnecting( Multi::NetworkSession & session )
    {
        if ( Multi::isTestMode() ) {
            const auto start = std::chrono::steady_clock::now();

            while ( session.state() == Multi::NetworkSession::State::RECONNECTING ) {
                session.update();

                if ( !session.isRetrying() ) {
                    session.retry();
                }

                if ( std::chrono::steady_clock::now() - start > testGiveUpDuration ) {
                    ERROR_LOG( "[MULTI] Self-test: the connection did not come back" )
                    session.close();
                    break;
                }

                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            }

            return;
        }

        VERBOSE_LOG( "[MULTI] The game is paused until the connection comes back" )

        fheroes2::engine().requestAttention();

        {
            ReconnectionWindow window( false, session.isRetrying() );
            LocalEvent & le = LocalEvent::Get();

            while ( session.state() == Multi::NetworkSession::State::RECONNECTING ) {
                if ( !le.HandleEvents( true, true ) ) {
                    if ( Game::processExitEvent() == fheroes2::GameMode::QUIT_GAME ) {
                        quitGame();
                    }

                    window.redraw();
                    continue;
                }

                session.update();
                window.setRetrying( session.isRetrying() );

                switch ( window.processEvents() ) {
                case Choice::RETRY:
                    session.retry();
                    window.setRetrying( true );
                    break;
                case Choice::QUIT:
                    if ( Game::processExitEvent() == fheroes2::GameMode::QUIT_GAME ) {
                        quitGame();
                    }

                    window.redraw();
                    break;
                default:
                    break;
                }
            }
        }

        // Restore the screen now, the waiting loop might not draw for a while.
        fheroes2::Display::instance().render();

        if ( session.state() == Multi::NetworkSession::State::CONNECTED ) {
            VERBOSE_LOG( "[MULTI] The connection is back, the game goes on" )
        }
    }

    // Host only. Waits until every absent guest is back, or the player goes on without them. In self-tests the game goes
    // on without them after a while.
    void waitForAbsentGuests()
    {
        if ( Multi::isTestMode() && !isAbsenceWindowTest ) {
            const auto start = std::chrono::steady_clock::now();

            while ( Multi::hasAbsentGuests() ) {
                Multi::updateGuests();
                Multi::updateLobby();

                if ( std::chrono::steady_clock::now() - start > testGiveUpDuration ) {
                    ERROR_LOG( "[MULTI] Self-test: a guest did not come back, the game goes on without it" )
                    Multi::abandonAbsentGuests();
                    break;
                }

                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            }

            return;
        }

        VERBOSE_LOG( "[MULTI] The game is paused until the absent players come back" )

        fheroes2::engine().requestAttention();

        {
            ReconnectionWindow window( true, false );
            window.setAbsentPlayers( describeAbsentGuests() );

            LocalEvent & le = LocalEvent::Get();

            // The guests still get their answers meanwhile: updateGuests() and the reservations are in the event hook.
            while ( Multi::hasAbsentGuests() ) {
                if ( !le.HandleEvents( true, true ) ) {
                    if ( Game::processExitEvent() == fheroes2::GameMode::QUIT_GAME ) {
                        quitGame();
                    }

                    window.redraw();
                    continue;
                }

                window.setAbsentPlayers( describeAbsentGuests() );

                switch ( window.processEvents() ) {
                case Choice::GO_ON:
                    Multi::abandonAbsentGuests();
                    break;
                case Choice::QUIT:
                    if ( Game::processExitEvent() == fheroes2::GameMode::QUIT_GAME ) {
                        quitGame();
                    }

                    window.redraw();
                    break;
                default:
                    break;
                }
            }
        }

        fheroes2::Display::instance().render();

        VERBOSE_LOG( "[MULTI] The game goes on" )
    }

    // The restarted host no longer knows this session. Join it again until it works or the player gives up.
    void joinRestartedHost( const std::string & address, const uint16_t port )
    {
        VERBOSE_LOG( "[MULTI] The game of the host was restarted, joining it again" )

        const bool isTest = Multi::isTestMode();
        std::unique_ptr<ReconnectionWindow> window;
        if ( !isTest ) {
            fheroes2::engine().requestAttention();
            window = std::make_unique<ReconnectionWindow>( false, true );
        }

        LocalEvent & le = LocalEvent::Get();

        auto start = std::chrono::steady_clock::now();
        auto nextAttempt = start;
        bool isRetrying = true;

        while ( true ) {
            if ( isTest ) {
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            }
            else {
                if ( !le.HandleEvents( true, true ) ) {
                    if ( Game::processExitEvent() == fheroes2::GameMode::QUIT_GAME ) {
                        quitGame();
                    }

                    window->redraw();
                    continue;
                }

                switch ( window->processEvents() ) {
                case Choice::RETRY:
                    isRetrying = true;
                    start = std::chrono::steady_clock::now();
                    nextAttempt = start;
                    window->setRetrying( true );
                    break;
                case Choice::QUIT:
                    if ( Game::processExitEvent() == fheroes2::GameMode::QUIT_GAME ) {
                        quitGame();
                    }

                    window->redraw();
                    break;
                default:
                    break;
                }
            }

            const auto now = std::chrono::steady_clock::now();

            if ( isRetrying && now - start > ( isTest ? testGiveUpDuration : automaticJoinDuration ) ) {
                if ( isTest ) {
                    ERROR_LOG( "[MULTI] Self-test: the restarted host could not be joined" )
                    return;
                }

                isRetrying = false;
                window->setRetrying( false );
            }

            if ( !isRetrying || now < nextAttempt ) {
                continue;
            }

            // The new game of the host gives this computer the colors it had.
            auto session = std::make_unique<Multi::NetworkSession>();
            const std::string error = Multi::joinHost(
                *session, address, port,
                [&le, isTest]() {
                    if ( isTest ) {
                        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                    }
                    else {
                        le.HandleEvents( true, true );
                    }
                    return true;
                },
                Multi::getColorsToClaim() );

            if ( error.empty() ) {
                Multi::replaceSessionAfterHostRestart( std::move( session ) );

                if ( window ) {
                    window.reset();
                    fheroes2::Display::instance().render();
                }
                return;
            }

            nextAttempt = std::chrono::steady_clock::now() + std::chrono::seconds( 2 );
        }
    }
}

bool Multi::keepConnection( const WaitKind kind )
{
    NetworkSession * session = getSession();
    if ( session == nullptr || isSessionReplaced() ) {
        return false;
    }

    if ( session->isHostSide() ) {
        updateGuests();

        if ( hasAbsentGuests() ) {
            waitForAbsentGuests();
        }

        // A guest which came back gets the game, but not in the middle of a battle or an answer.
        if ( kind == WaitKind::MAP ) {
            resyncReturningGuests();
        }

        return session->isAlive();
    }

    if ( session->state() == NetworkSession::State::RECONNECTING && session->lostFor() >= windowDelay ) {
        waitWhileReconnecting( *session );
    }

    if ( session->state() == NetworkSession::State::CLOSED && session->isResumeRefused() ) {
        // What we were waiting for now comes from the restarted game.
        joinRestartedHost( session->joinedAddress(), session->joinedPort() );
        return false;
    }

    return session->isAlive();
}

void Multi::enableTestAbsenceWindow()
{
    isAbsenceWindowTest = true;
}

void Multi::showReconnectionWindowForTest( const bool isHostSide, const bool isRetrying )
{
    ReconnectionWindow window( isHostSide, isRetrying );
    if ( isHostSide ) {
        std::string text = _( "The connection with %{name} is cut." );
        StringReplace( text, "%{name}", "Marc" );
        window.setAbsentPlayers( text );
    }

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        // The gallery closes every window with Enter.
        if ( window.processEvents() != Choice::NONE || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
            break;
        }
    }
}
