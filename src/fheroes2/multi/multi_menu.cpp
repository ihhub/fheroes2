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

#include "multi_menu.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "dialog.h"
#include "editor_ui_helper.h"
#include "game.h"
#include "game_assets.h"
#include "game_hotkeys.h"
#include "game_mainmenu_ui.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "multi_game.h"
#include "multi_lobby_screen.h"
#include "multi_network.h"
#include "multi_settings.h"
#include "multi_simultaneous.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    enum class PendingHost
    {
        NONE,
        NEW_GAME,
        SAVED_GAME
    };

    // Set by the network menu until the chosen game starts.
    PendingHost pendingHost{ PendingHost::NONE };

    // Set when the chosen game goes on a lobby.
    std::optional<Multi::LobbyHosting> pendingLobbyHosting;

    // The network menu shows at once the screen the player comes back to.
    enum class NextScreen
    {
        NONE,
        LOBBY,
        ADDRESS
    };

    NextScreen nextScreen{ NextScreen::NONE };

    // The player types the address as "host" or "host:port".
    fheroes2::GameMode joinMenu()
    {
        Multi::NetworkSettings settings = Multi::loadNetworkSettings();
        std::string text = settings.address;

        const fheroes2::Text title( _( "Join a network game" ), fheroes2::FontType::normalYellow() );
        const fheroes2::Text body( _( "Address of the host:" ), fheroes2::FontType::normalWhite() );

        if ( !Dialog::inputString( title, body, text, 64, false, {} ) ) {
            return fheroes2::GameMode::MULTI_NETWORK_MENU;
        }

        const std::optional<std::pair<std::string, uint16_t>> address = Multi::parseHostAddress( text );
        if ( !address ) {
            fheroes2::showStandardTextMessage( _( "Network game" ), _( "This address is not valid." ), Dialog::OK );
            return fheroes2::GameMode::MULTI_NETWORK_MENU;
        }

        settings.address = text;
        Multi::saveNetworkSettings( settings );

        return Multi::joinGame( address->first, address->second );
    }

    void showMark( fheroes2::MovableSprite & mark, const bool isShown )
    {
        if ( isShown ) {
            mark.show();
        }
        else {
            mark.hide();
        }
    }

    // A window of tall buttons as in the new game menu, wider for the longer texts, with an optional checkbox below.
    class MenuWindow
    {
    public:
        MenuWindow( const char * titleText, const std::vector<const char *> & buttonTexts, const char * checkboxText )
        {
            fheroes2::Display & display = fheroes2::Display::instance();
            const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

            fheroes2::drawMainMenuScreen();

            const fheroes2::Sprite & buttonSample = Assets::getImage( isEvilInterface ? ICN::BUTTONS_NEW_GAME_MENU_EVIL : ICN::BUTTONS_NEW_GAME_MENU_GOOD, 12 );
            const fheroes2::Size buttonSize{ buttonSample.width() + 50, buttonSample.height() };

            const fheroes2::Text title( titleText, fheroes2::FontType::normalYellow() );

            const int32_t margin = 20;
            const int32_t spacing = 10;
            const auto buttonCount = static_cast<int32_t>( buttonTexts.size() );
            const int32_t buttonsTop = margin + title.height() + spacing;
            const int32_t checkboxHeight = checkboxText != nullptr ? 25 : 0;

            _window = std::make_unique<fheroes2::StandardWindow>( buttonSize.width + 2 * margin,
                                                                  buttonsTop + buttonCount * ( buttonSize.height + spacing ) + checkboxHeight + margin, true, display );
            _window->applyGemDecoratedCorners();

            const fheroes2::Rect & area = _window->activeArea();

            title.draw( area.x + ( area.width - title.width() ) / 2, area.y + margin, display );

            for ( int32_t i = 0; i < buttonCount; ++i ) {
                const fheroes2::Rect buttonArea{ area.x + ( area.width - buttonSize.width ) / 2, area.y + buttonsTop + i * ( buttonSize.height + spacing ),
                                                 buttonSize.width, buttonSize.height };
                _buttons.push_back( Multi::makeTextButton( buttonArea, buttonTexts[static_cast<size_t>( i )] ) );
                _buttons.back().draw();
                _buttons.back().drawShadow( display );
            }

            if ( checkboxText != nullptr ) {
                _checkboxArea
                    = Editor::drawCheckboxWithText( _checkboxMark, checkboxText, display, area.x + margin + 5,
                                                    area.y + buttonsTop + buttonCount * ( buttonSize.height + spacing ), isEvilInterface, buttonSize.width - 10 );
            }
        }

        fheroes2::ButtonSprite & button( const size_t index )
        {
            return _buttons[index];
        }

        std::vector<fheroes2::ButtonSprite> & buttons()
        {
            return _buttons;
        }

        const fheroes2::Rect & checkboxArea() const
        {
            return _checkboxArea;
        }

        void setChecked( const bool isChecked )
        {
            showMark( _checkboxMark, isChecked );
            fheroes2::Display::instance().render();
        }

    private:
        std::unique_ptr<fheroes2::StandardWindow> _window;
        std::vector<fheroes2::ButtonSprite> _buttons;
        fheroes2::MovableSprite _checkboxMark;
        fheroes2::Rect _checkboxArea;
    };

    // How many human players a new network game has, with the buttons of the game for it. Returns the option of
    // Game::NewHotSeat(), nothing if the player cancels.
    std::optional<size_t> askHumanPlayerCount()
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        const int icn = Settings::Get().isEvilInterfaceEnabled() ? ICN::BUTTONS_NEW_GAME_MENU_EVIL : ICN::BUTTONS_NEW_GAME_MENU_GOOD;

        fheroes2::drawMainMenuScreen();

        const fheroes2::Text title( _( "Human players" ), fheroes2::FontType::normalYellow() );
        const fheroes2::Sprite & sample = Assets::getImage( icn, 14 );

        constexpr int32_t margin = 20;
        constexpr int32_t spacing = 10;
        constexpr size_t countOptions = 5;
        const int32_t buttonsTop = margin + title.height() + spacing;
        const int32_t step = sample.height() + spacing;

        fheroes2::StandardWindow window( sample.width() + 2 * margin, buttonsTop + static_cast<int32_t>( countOptions + 1 ) * step - spacing + margin, true, display );
        window.applyGemDecoratedCorners();

        const fheroes2::Rect & area = window.activeArea();
        title.draw( area.x + ( area.width - title.width() ) / 2, area.y + margin, display );

        fheroes2::ButtonGroup counts;
        for ( size_t i = 0; i < countOptions; ++i ) {
            const auto index = static_cast<uint32_t>( ( i + 7 ) * 2 );
            counts.createButton( area.x + margin, area.y + buttonsTop + static_cast<int32_t>( i ) * step, icn, index, index + 1, static_cast<int>( i ) );
        }
        counts.draw();
        counts.drawShadows( display );

        fheroes2::ButtonSprite cancel
            = Multi::makeTextButton( { area.x + margin, area.y + buttonsTop + static_cast<int32_t>( countOptions ) * step, sample.width(), sample.height() },
                                     gettext_noop( "CANCEL" ) );
        cancel.draw();
        cancel.drawShadow( display );

        display.render( window.totalArea() );

        const std::array<fheroes2::Key, countOptions> hotkeys{ fheroes2::Key::KEY_2, fheroes2::Key::KEY_3, fheroes2::Key::KEY_4, fheroes2::Key::KEY_5,
                                                               fheroes2::Key::KEY_6 };
        const std::array<std::pair<const char *, const char *>, countOptions>
            helps{ std::pair<const char *, const char *>{ gettext_noop( "2 Players" ),
                                                          gettext_noop( "Play with 2 human players, and optionally, up to 4 additional computer players." ) },
                   { gettext_noop( "3 Players" ), gettext_noop( "Play with 3 human players, and optionally, up to 3 additional computer players." ) },
                   { gettext_noop( "4 Players" ), gettext_noop( "Play with 4 human players, and optionally, up to 2 additional computer players." ) },
                   { gettext_noop( "5 Players" ), gettext_noop( "Play with 5 human players, and optionally, up to 1 additional computer player." ) },
                   { gettext_noop( "6 Players" ), gettext_noop( "Play with 6 human players." ) } };

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            counts.drawOnState( le );
            cancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( cancel.area() ) );

            for ( size_t i = 0; i < countOptions; ++i ) {
                if ( le.MouseClickLeft( counts.button( i ).area() ) || le.isKeyPressed( hotkeys[i] ) ) {
                    return i;
                }
                if ( le.isMouseRightButtonPressedInArea( counts.button( i ).area() ) ) {
                    fheroes2::showStandardTextMessage( _( helps[i].first ), _( helps[i].second ), Dialog::ZERO );
                }
            }

            if ( le.MouseClickLeft( cancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return {};
            }
            if ( le.isMouseRightButtonPressedInArea( cancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Back to the network menu." ), Dialog::ZERO );
            }
        }

        return {};
    }

    // Host a new or a saved game, or join one, with the address of the host.
    fheroes2::GameMode addressMenu()
    {
        MenuWindow menu( _( "Play by address" ),
                         { gettext_noop( "HOST A\nNEW GAME" ), gettext_noop( "HOST A\nSAVED GAME" ), gettext_noop( "JOIN\nA GAME" ), gettext_noop( "CANCEL" ) },
                         _( "Simultaneous turns" ) );

        const fheroes2::ButtonSprite & buttonNewGame = menu.button( 0 );
        const fheroes2::ButtonSprite & buttonSavedGame = menu.button( 1 );
        const fheroes2::ButtonSprite & buttonJoin = menu.button( 2 );
        const fheroes2::ButtonSprite & buttonCancel = menu.button( 3 );
        const fheroes2::Rect & simultaneousArea = menu.checkboxArea();

        // The checkbox applies only to the games hosted from this menu.
        Multi::NetworkSettings settings = Multi::loadNetworkSettings();
        menu.setChecked( settings.isSimultaneous );

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            for ( fheroes2::ButtonSprite & button : menu.buttons() ) {
                button.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( button.area() ) );
            }

            if ( le.MouseClickLeft( simultaneousArea ) ) {
                settings.isSimultaneous = !settings.isSimultaneous;
                Multi::saveNetworkSettings( settings );
                menu.setChecked( settings.isSimultaneous );
            }

            // MouseClickLeft() consumes the click, check every area only once.
            if ( le.MouseClickLeft( buttonNewGame.area() ) ) {
                const std::optional<size_t> playerCount = askHumanPlayerCount();
                if ( !playerCount ) {
                    nextScreen = NextScreen::ADDRESS;
                    return fheroes2::GameMode::MULTI_NETWORK_MENU;
                }

                Multi::setSimultaneousTurns( settings.isSimultaneous );
                pendingHost = PendingHost::NEW_GAME;
                return Game::NewHotSeat( *playerCount );
            }
            if ( le.MouseClickLeft( buttonSavedGame.area() ) ) {
                Multi::setSimultaneousTurns( settings.isSimultaneous );
                pendingHost = PendingHost::SAVED_GAME;
                return fheroes2::GameMode::LOAD_HOT_SEAT;
            }
            if ( le.MouseClickLeft( buttonJoin.area() ) ) {
                // Back here after a wrong address.
                const fheroes2::GameMode mode = joinMenu();
                if ( mode == fheroes2::GameMode::MULTI_NETWORK_MENU ) {
                    nextScreen = NextScreen::ADDRESS;
                }
                return mode;
            }
            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return fheroes2::GameMode::MULTI_NETWORK_MENU;
            }

            if ( le.isMouseRightButtonPressedInArea( buttonNewGame.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Host a new game" ), _( "Start a new game with other players. Each one takes a color when joining." ),
                                                   Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonSavedGame.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Host a saved game" ),
                                                   _( "Load a Hot Seat game and play it with other players. Each one takes a color when joining." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonJoin.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Join a network game" ), _( "Join the game of another player." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Back to the network menu." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( simultaneousArea ) ) {
                fheroes2::showStandardTextMessage( _( "Simultaneous turns" ),
                                                   _( "Every player plays their day at the same time and sees what the others do. The computer players play "
                                                      "afterwards. The host makes the choice." ),
                                                   Dialog::ZERO );
            }
        }

        return fheroes2::GameMode::QUIT_GAME;
    }
}

std::optional<std::pair<std::string, uint16_t>> Multi::parseHostAddress( const std::string & text )
{
    const size_t begin = text.find_first_not_of( ' ' );
    if ( begin == std::string::npos ) {
        return {};
    }

    const std::string address = text.substr( begin, text.find_last_not_of( ' ' ) - begin + 1 );

    std::string host = address;
    std::optional<std::string> portText;

    if ( address.front() == '[' ) {
        // An IPv6 address with a port is written in brackets: [2001:db8::1]:47219
        const size_t close = address.find( ']' );
        if ( close == std::string::npos || ( close + 1 < address.size() && address[close + 1] != ':' ) ) {
            return {};
        }

        host = address.substr( 1, close - 1 );
        if ( host.find( ':' ) == std::string::npos ) {
            return {};
        }

        if ( close + 1 < address.size() ) {
            portText = address.substr( close + 2 );
        }
    }
    else if ( std::count( address.begin(), address.end(), ':' ) == 1 ) {
        const size_t colon = address.find( ':' );
        host = address.substr( 0, colon );
        portText = address.substr( colon + 1 );
    }

    int port = defaultPort;
    if ( portText ) {
        const auto [end, error] = std::from_chars( portText->data(), portText->data() + portText->size(), port );
        if ( error != std::errc() || end != portText->data() + portText->size() || port < 1 || port > 65535 ) {
            return {};
        }
    }

    // Several colons: an IPv6 address.
    if ( host.find( ':' ) != std::string::npos ) {
        if ( !isIPv6Address( host ) ) {
            return {};
        }

        return std::make_pair( host, static_cast<uint16_t>( port ) );
    }

    // A host name can only contain letters, digits, dots and dashes.
    const auto isNameCharacter = []( const char c ) { return std::isalnum( static_cast<unsigned char>( c ) ) != 0 || c == '.' || c == '-'; };
    if ( host.empty() || host.size() > 253 || !std::all_of( host.begin(), host.end(), isNameCharacter ) ) {
        return {};
    }

    // Only digits and dots means it must be a valid IPv4 address.
    const auto isNumberCharacter = []( const char c ) { return std::isdigit( static_cast<unsigned char>( c ) ) != 0 || c == '.'; };
    if ( std::all_of( host.begin(), host.end(), isNumberCharacter ) ) {
        const char * position = host.data();
        const char * const hostEnd = host.data() + host.size();

        for ( int part = 0; part < 4; ++part ) {
            int value = 0;
            const auto [end, error] = std::from_chars( position, hostEnd, value );
            if ( error != std::errc() || end == position || value > 255 || end - position > 3 ) {
                return {};
            }

            position = end;
            if ( part < 3 ) {
                if ( position == hostEnd || *position != '.' ) {
                    return {};
                }
                ++position;
            }
        }

        if ( position != hostEnd ) {
            return {};
        }
    }

    return std::make_pair( host, static_cast<uint16_t>( port ) );
}

fheroes2::ButtonSprite Multi::makeTextButton( const fheroes2::Rect & area, const char * untranslatedText )
{
    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    fheroes2::Sprite released;
    fheroes2::Sprite pressed;

    // Generated buttons are 10 pixels wider, as in the original game.
    fheroes2::makeButtonSprites( released, pressed, fheroes2::getSupportedText( untranslatedText, fheroes2::FontType::buttonReleasedWhite() ),
                                 { area.width - 10, area.height }, isEvilInterface, isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

    return { area.x, area.y, std::move( released ), std::move( pressed ) };
}

fheroes2::ButtonSprite Multi::makeNetworkButton( const fheroes2::Rect & area )
{
    // An empty button is neither drawn nor clicked.
    if ( !isNetworkAvailable() ) {
        return fheroes2::ButtonSprite();
    }

    return makeTextButton( area, gettext_noop( "NETWORK" ) );
}

void Multi::rememberSimultaneousTurns( const bool isEnabled )
{
    NetworkSettings settings = loadNetworkSettings();
    settings.isSimultaneous = isEnabled;
    saveNetworkSettings( settings );
}

fheroes2::ButtonSprite Multi::makeFileDialogButton( const fheroes2::Rect & cancelArea )
{
    if ( !isNetworkGame() ) {
        return fheroes2::ButtonSprite();
    }

    const int32_t width = cancelArea.width * 3 / 2;
    fheroes2::ButtonSprite button = makeTextButton( { cancelArea.x - width - 12, cancelArea.y, width, cancelArea.height }, gettext_noop( "NETWORK" ) );

    fheroes2::Display & display = fheroes2::Display::instance();
    button.draw();
    button.drawShadow( display );
    display.render( button.area() );

    return button;
}

bool Multi::handleFileDialogButton( fheroes2::ButtonSprite & button )
{
    if ( !isNetworkGame() ) {
        return false;
    }

    LocalEvent & le = LocalEvent::Get();
    button.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( button.area() ) );

    if ( le.isMouseRightButtonPressedInArea( button.area() ) ) {
        fheroes2::showStandardTextMessage( _( "Network game" ), _( "Switch between simultaneous turns and turns one after the other." ), Dialog::ZERO );
        return false;
    }

    if ( !le.MouseClickLeft( button.area() ) ) {
        return false;
    }

    const bool isEnabled = areSimultaneousTurnsEnabled();
    const char * question
        = isEnabled ? gettext_noop( "Switch back to turns one after the other from tomorrow?" ) : gettext_noop( "Switch to simultaneous turns from tomorrow?" );

    if ( fheroes2::showStandardTextMessage( _( "Simultaneous turns" ), _( question ), Dialog::YES | Dialog::NO ) == Dialog::YES ) {
        changeSimultaneousTurns( !isEnabled );
    }

    return true;
}

fheroes2::GameMode Multi::networkMenu()
{
    pendingHost = PendingHost::NONE;
    pendingLobbyHosting.reset();

    switch ( std::exchange( nextScreen, NextScreen::NONE ) ) {
    case NextScreen::LOBBY:
        return lobbyScreen();
    case NextScreen::ADDRESS:
        return addressMenu();
    default:
        break;
    }

    MenuWindow menu( _( "Network game" ),
                     { gettext_noop( "ONLINE\nLOBBY" ), gettext_noop( "PLAY BY\nADDRESS" ), gettext_noop( "network|SETTINGS" ), gettext_noop( "CANCEL" ) }, nullptr );

    const fheroes2::ButtonSprite & buttonLobby = menu.button( 0 );
    const fheroes2::ButtonSprite & buttonAddress = menu.button( 1 );
    const fheroes2::ButtonSprite & buttonSettings = menu.button( 2 );
    const fheroes2::ButtonSprite & buttonCancel = menu.button( 3 );

    fheroes2::Display::instance().render();

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        for ( fheroes2::ButtonSprite & button : menu.buttons() ) {
            button.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( button.area() ) );
        }

        if ( le.MouseClickLeft( buttonLobby.area() ) ) {
            return lobbyScreen();
        }
        if ( le.MouseClickLeft( buttonAddress.area() ) ) {
            return addressMenu();
        }
        if ( le.MouseClickLeft( buttonSettings.area() ) ) {
            NetworkSettings settings = loadNetworkSettings();
            networkSettingsDialog( settings );
            return fheroes2::GameMode::MULTI_NETWORK_MENU;
        }
        if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            return fheroes2::GameMode::MAIN_MENU;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonLobby.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Online lobby" ), _( "Find a game to join, or offer yours to the players of the lobby." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonAddress.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Play by address" ),
                                               _( "Host a game, or join one with the address of its host: on the same network, through Tailscale or through an "
                                                  "open internet box." ),
                                               Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonSettings.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Network settings" ), _( "Your nickname, the lobbies and the relay." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Cancel back to the main menu." ), Dialog::ZERO );
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}

fheroes2::GameMode Multi::hostOnLobby( const LobbyHosting & hosting, const bool isSavedGame, const bool isSimultaneous )
{
    std::optional<size_t> playerCount;
    if ( !isSavedGame ) {
        playerCount = askHumanPlayerCount();
        if ( !playerCount ) {
            nextScreen = NextScreen::LOBBY;
            return fheroes2::GameMode::MULTI_NETWORK_MENU;
        }
    }

    setSimultaneousTurns( isSimultaneous );
    pendingHost = isSavedGame ? PendingHost::SAVED_GAME : PendingHost::NEW_GAME;
    pendingLobbyHosting = hosting;

    return isSavedGame ? fheroes2::GameMode::LOAD_HOT_SEAT : Game::NewHotSeat( *playerCount );
}

void Multi::onMainLoopMode( const fheroes2::GameMode mode )
{
    switch ( mode ) {
    case fheroes2::GameMode::START_GAME:
    case fheroes2::GameMode::MULTI_WAIT_FOR_TURN:
        return;
    case fheroes2::GameMode::SELECT_SCENARIO_TWO_HUMAN_PLAYERS:
    case fheroes2::GameMode::SELECT_SCENARIO_THREE_HUMAN_PLAYERS:
    case fheroes2::GameMode::SELECT_SCENARIO_FOUR_HUMAN_PLAYERS:
    case fheroes2::GameMode::SELECT_SCENARIO_FIVE_HUMAN_PLAYERS:
    case fheroes2::GameMode::SELECT_SCENARIO_SIX_HUMAN_PLAYERS:
    case fheroes2::GameMode::LOAD_HOT_SEAT:
        // The player is on the way to the game they chose to host.
        if ( !isNetworkGame() ) {
            return;
        }
        break;
    default:
        break;
    }

    pendingHost = PendingHost::NONE;
    pendingLobbyHosting.reset();

    if ( isNetworkGame() ) {
        endGame();
    }
}

fheroes2::GameMode Multi::startGame()
{
    if ( pendingHost == PendingHost::NONE ) {
        return Game::StartGame();
    }

    pendingHost = PendingHost::NONE;

    if ( !pendingLobbyHosting ) {
        return hostCurrentGame( defaultPort, false );
    }

    const LobbyHosting hosting = std::move( *pendingLobbyHosting );
    pendingLobbyHosting.reset();

    // Back to the list of the lobby if the player gives up or the lobby refuses the game.
    const fheroes2::GameMode mode = hostCurrentGame( defaultPort, false, hosting );
    if ( mode == fheroes2::GameMode::MAIN_MENU ) {
        nextScreen = NextScreen::LOBBY;
        return fheroes2::GameMode::MULTI_NETWORK_MENU;
    }

    return mode;
}
