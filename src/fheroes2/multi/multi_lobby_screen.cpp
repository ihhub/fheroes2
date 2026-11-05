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

#include "multi_lobby_screen.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "cursor.h"
#include "dialog.h"
#include "editor_ui_helper.h"
#include "game_assets.h"
#include "game_hotkeys.h"
#include "game_mainmenu_ui.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "logging.h"
#include "maps.h"
#include "multi_game.h"
#include "multi_lobby.h"
#include "multi_lobby_client.h"
#include "multi_menu.h"
#include "multi_network.h"
#include "multi_protocol.h"
#include "multi_settings.h"
#include "multi_text.h"
#include "multi_waiting_room.h"
#include "multi_waiting_screen.h"
#include "multi_websocket.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    // The window fits the smallest screen of the game, 640 x 480.
    constexpr int32_t windowWidth{ 600 };
    constexpr int32_t windowHeight{ 440 };

    constexpr int32_t margin{ 16 };
    constexpr int32_t rowHeight{ 20 };
    constexpr int32_t visibleRooms{ 12 };
    constexpr int32_t scrollbarWidth{ 18 };
    constexpr int32_t detailsHeight{ 56 };

    // Columns of the list, from the left of the list.
    constexpr int32_t playersColumnX{ 4 };
    constexpr int32_t sizeColumnX{ 24 };
    constexpr int32_t nameColumnX{ 46 };
    constexpr int32_t nameColumnWidth{ 150 };
    constexpr int32_t mapColumnX{ 200 };
    constexpr int32_t mapColumnWidth{ 106 };
    constexpr int32_t turnsColumnX{ 310 };
    constexpr int32_t turnsColumnWidth{ 134 };
    constexpr int32_t hostColumnX{ 448 };
    constexpr int32_t hostColumnWidth{ 98 };

    // Lobby texts are UTF-8.
    std::string shown( const std::string & utf8 )
    {
        return Multi::fromUtf8( utf8 );
    }

    const fheroes2::Sprite & getPlayersIcon( const uint8_t players )
    {
        return Assets::getImage( ICN::REQUESTS, 19 + std::clamp<uint32_t>( players, 1, 6 ) );
    }

    // As in the list of scenarios.
    void drawMapSizeIcon( const uint16_t width, fheroes2::Image & output, const int32_t x, const int32_t y )
    {
        uint32_t index = 0;
        switch ( width ) {
        case Maps::SMALL:
            index = 26;
            break;
        case Maps::MEDIUM:
            index = 27;
            break;
        case Maps::LARGE:
            index = 28;
            break;
        case Maps::XLARGE:
            index = 29;
            break;
        default:
            break;
        }

        if ( index != 0 ) {
            fheroes2::Blit( Assets::getImage( ICN::REQUESTS, index ), output, x, y );
            return;
        }

        fheroes2::Image icon( 17, 17 );
        icon.reset();
        fheroes2::Fill( icon, 1, 1, 15, 15, fheroes2::GetColorId( 0x8D, 0x73, 0xFF ) );
        const fheroes2::Text text( _( "N" ), fheroes2::FontType::smallWhite() );
        text.draw( ( 17 - text.width() ) / 2, ( 17 - text.height() ) / 2, icon );
        fheroes2::Blit( icon, output, x, y );
    }

    const char * describeTurns( const Multi::RoomTurns turns )
    {
        return turns == Multi::RoomTurns::SIMULTANEOUS ? _( "lobby|simultaneous" ) : _( "lobby|one by one" );
    }

    std::string describeVersion( const Multi::HandshakeInfo & game )
    {
        return std::to_string( game.gameVersion[0] ) + '.' + std::to_string( game.gameVersion[1] ) + '.' + std::to_string( game.gameVersion[2] );
    }

    // How a guest would connect, as far as the lobby knows.
    std::string describeReach( const Multi::RoomReach & reach )
    {
        if ( !reach.isProbeDone ) {
            return _( "The lobby is checking how to reach the host." );
        }
        if ( reach.isIPv4 || reach.isIPv6 ) {
            return _( "Direct connection." );
        }
        if ( reach.isRelayAvailable ) {
            return _( "Through the relay of the lobby." );
        }

        return _( "The host cannot be reached." );
    }

    std::string describeLobbyName( const Multi::LobbyState & lobby )
    {
        return lobby.welcome.lobbyName.empty() ? lobby.address : shown( lobby.welcome.lobbyName );
    }

    class RoomListBox final : public Interface::ListBox<Multi::ListedRoom>
    {
    public:
        using Interface::ListBox<Multi::ListedRoom>::ActionListDoubleClick;
        using Interface::ListBox<Multi::ListedRoom>::ActionListSingleClick;
        using Interface::ListBox<Multi::ListedRoom>::ActionListPressRight;

        using ListBox::ListBox;

        void RedrawItem( const Multi::ListedRoom & room, int32_t dstx, int32_t dsty, bool current ) override
        {
            fheroes2::Display & display = fheroes2::Display::instance();

            fheroes2::Blit( getPlayersIcon( room.info.game.players ), display, dstx + playersColumnX, dsty );
            drawMapSizeIcon( room.info.game.mapWidth, display, dstx + sizeColumnX, dsty );

            // Games of another version are greyed.
            fheroes2::FontColor color = fheroes2::FontColor::WHITE;
            if ( !room.isSameGame ) {
                color = fheroes2::FontColor::GRAY;
            }
            else if ( current ) {
                color = fheroes2::FontColor::YELLOW;
            }
            const fheroes2::FontType font{ fheroes2::FontSize::NORMAL, color };

            const auto drawColumn = [&display, dstx, dsty, &font]( std::string text, const int32_t columnX, const int32_t width ) {
                fheroes2::Text column( std::move( text ), font );
                column.fitToOneRow( width );
                column.draw( dstx + columnX, dsty + 3, display );
            };

            drawColumn( shown( room.info.gameName ), nameColumnX, nameColumnWidth );
            drawColumn( shown( room.info.game.mapName ), mapColumnX, mapColumnWidth );
            drawColumn( describeTurns( room.info.game.turns ), turnsColumnX, turnsColumnWidth );
            drawColumn( shown( room.info.hostNickname ), hostColumnX, hostColumnWidth );
        }

        void RedrawBackground( const fheroes2::Point & /* unused */ ) override
        {
            if ( _background ) {
                _background->restore();
            }
        }

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( Multi::ListedRoom & /* room */ ) override
        {
            _isDoubleClicked = true;
        }

        void ActionListSingleClick( Multi::ListedRoom & /* room */ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( Multi::ListedRoom & room ) override
        {
            fheroes2::MultiFontText body;
            body.add( { _( "Map: " ), fheroes2::FontType::normalYellow() } );
            body.add( { shown( room.info.game.mapName ), fheroes2::FontType::normalWhite() } );
            body.add( { std::string( "\n" ) + _( "Host: " ), fheroes2::FontType::normalYellow() } );
            body.add( { shown( room.info.hostNickname ), fheroes2::FontType::normalWhite() } );
            body.add( { std::string( "\n" ) + _( "Version: " ), fheroes2::FontType::normalYellow() } );
            body.add( { describeVersion( room.info.hostGame ) + ", " + room.info.hostGame.platform, fheroes2::FontType::normalWhite() } );
            body.add( { "\n\n" + describeReach( room.info.reach ), fheroes2::FontType::normalWhite() } );

            fheroes2::showMessage( fheroes2::Text( shown( room.info.gameName ), fheroes2::FontType::normalYellow() ), body, Dialog::ZERO );
        }

        void initBackground( const fheroes2::Rect & roi )
        {
            _background = std::make_unique<fheroes2::ImageRestorer>( fheroes2::Display::instance(), roi.x, roi.y, roi.width, roi.height );
        }

        bool takeDoubleClick()
        {
            return std::exchange( _isDoubleClicked, false );
        }

    private:
        std::unique_ptr<fheroes2::ImageRestorer> _background;
        bool _isDoubleClicked{ false };
    };

    // The line at the top of the lobby screen.
    std::string describeLobbies( const std::vector<Multi::LobbyState> & lobbies )
    {
        uint32_t players = 0;
        uint32_t rooms = 0;
        size_t openCount = 0;
        const Multi::LobbyState * firstOpen = nullptr;
        const Multi::LobbyState * firstLost = nullptr;

        for ( const Multi::LobbyState & lobby : lobbies ) {
            if ( lobby.status == Multi::LobbyState::Status::OPEN ) {
                ++openCount;
                players += lobby.stats.playersOnline;
                rooms += lobby.stats.waitingRooms;
                if ( firstOpen == nullptr ) {
                    firstOpen = &lobby;
                }
            }
            else if ( lobby.status != Multi::LobbyState::Status::CONNECTING && firstLost == nullptr ) {
                firstLost = &lobby;
            }
        }

        if ( openCount == 0 ) {
            if ( firstLost == nullptr ) {
                return _( "Connecting to the lobby..." );
            }

            std::string text = _( "The lobby cannot be reached: %{reason}." );
            StringReplace( text, "%{reason}", Multi::translateError( firstLost->error ) );
            return text;
        }

        std::string text = openCount == 1 ? _( "%{lobby}. Players online: %{players}, games waiting: %{rooms}." )
                                          : _( "%{count} lobbies. Players online: %{players}, games waiting: %{rooms}." );
        StringReplace( text, "%{lobby}", describeLobbyName( *firstOpen ) );
        StringReplace( text, "%{count}", static_cast<int>( openCount ) );
        StringReplace( text, "%{players}", static_cast<int>( players ) );
        StringReplace( text, "%{rooms}", static_cast<int>( rooms ) );
        return text;
    }

    const Multi::ListedRoom * getSelectedRoom( const RoomListBox & list, const std::vector<Multi::ListedRoom> & rooms )
    {
        const int32_t current = list.getCurrentId();
        if ( !list.isSelected() || current < 0 || static_cast<size_t>( current ) >= rooms.size() ) {
            return nullptr;
        }

        return &rooms[static_cast<size_t>( current )];
    }

    // Below the list: the selected game, or the message of the day.
    std::string describeSelection( const Multi::ListedRoom * room, const std::vector<Multi::LobbyState> & lobbies )
    {
        if ( room == nullptr ) {
            for ( const Multi::LobbyState & lobby : lobbies ) {
                if ( lobby.status == Multi::LobbyState::Status::OPEN && !lobby.welcome.messageOfTheDay.empty() ) {
                    return shown( lobby.welcome.messageOfTheDay );
                }
            }
            return {};
        }

        if ( !room->isSameGame ) {
            std::string text = _( "This game was started with another version of fheroes2 (%{version}). Every player needs the same version." );
            StringReplace( text, "%{version}", describeVersion( room->info.hostGame ) );
            return text;
        }

        const Multi::RoomGame & game = room->info.game;
        std::string text = game.day > 1 ? _( "%{map}, %{width} x %{height}, day %{day}. Players: %{players}, of them human: %{humans}, for you: %{guests}." )
                                        : _( "%{map}, %{width} x %{height}, new game. Players: %{players}, of them human: %{humans}, for you: %{guests}." );
        StringReplace( text, "%{map}", shown( game.mapName ) );
        StringReplace( text, "%{width}", game.mapWidth );
        StringReplace( text, "%{height}", game.mapHeight );
        StringReplace( text, "%{day}", static_cast<int>( game.day ) );
        StringReplace( text, "%{players}", game.players );
        StringReplace( text, "%{humans}", game.humanPlayers );
        StringReplace( text, "%{guests}", game.guestHumanPlayers );

        return text + ' ' + describeReach( room->info.reach );
    }

    // Six letters and digits, typed in any case and with spaces.
    std::string normalizeInviteCode( const std::string & text )
    {
        std::string code;
        for ( const char c : text ) {
            if ( c != ' ' ) {
                code += static_cast<char>( std::toupper( static_cast<unsigned char>( c ) ) );
            }
        }
        return code;
    }

    std::string describeJoinRefusal( const Multi::JoinResult result, const bool isByCode )
    {
        switch ( result ) {
        case Multi::JoinResult::NO_SUCH_ROOM:
            return isByCode ? _( "No game has this invite code. Check it with the host." ) : _( "This game is not on the lobby any more." );
        case Multi::JoinResult::ROOM_IN_GAME:
            return _( "This game has already started, or its players are all there." );
        case Multi::JoinResult::OTHER_VERSION:
            return _( "This game was started with another version of fheroes2. Every player needs the same version." );
        default:
            return _( "The lobby refused to let you join this game." );
        }
    }

    // Joins a room of the list, or the room of an invite code. Returns the next mode once the game starts, nothing to go
    // back to the list.
    struct JoinOutcome
    {
        // Set once the game starts, or when the player quits the game.
        std::optional<fheroes2::GameMode> mode;
        // Why the game did not start, shown over the list.
        std::string title;
        std::string message;
    };

    JoinOutcome makeJoinFailure( const std::string & error )
    {
        ERROR_LOG( "[MULTI] " << error )
        fheroes2::engine().requestAttention();
        return { {}, _( "Network game" ), std::string( _( "Unable to join the network game:" ) ) + "\n\n" + Multi::translateError( error ) };
    }

    JoinOutcome joinLobbyRoom( Multi::LobbyBrowser & browser, const Multi::ListedRoom * room, const std::string & code, const Multi::NetworkSettings & settings )
    {
        std::string message = room != nullptr ? _( "Joining the game %{game}..." ) : _( "Looking for the game of this invite code..." );
        if ( room != nullptr ) {
            StringReplace( message, "%{game}", shown( room->info.gameName ) );
        }

        Multi::WaitingScreen screen( message );

        const auto leave = [&screen]() {
            JoinOutcome outcome;
            if ( screen.isQuitRequested() ) {
                outcome.mode = fheroes2::GameMode::QUIT_GAME;
            }
            return outcome;
        };

        if ( room != nullptr ) {
            browser.join( *room );
        }
        else {
            browser.joinByCode( code );
        }

        std::optional<Multi::JoinAnswer> answer;
        while ( !( answer = browser.takeJoinAnswer() ) ) {
            browser.update();
            if ( !screen.keepWaiting() ) {
                return leave();
            }
        }

        if ( answer->plan.result != Multi::JoinResult::READY ) {
            return { {}, _( "Online lobby" ), describeJoinRefusal( answer->plan.result, room == nullptr ) };
        }

        message = _( "Connecting to the game %{game}..." );
        StringReplace( message, "%{game}", shown( answer->plan.room.gameName ) );
        screen.setMessage( message );

        // Connected and with the handshake done.
        std::string error;
        std::unique_ptr<Multi::NetworkSession> session = Multi::joinRoom( answer->plan, answer->relayAddress, settings.isRelayAlways, screen.callback(), error );
        if ( !session ) {
            return screen.isLeftByPlayer() ? leave() : makeJoinFailure( error );
        }

        // Then the waiting room, until the host starts the game.
        switch ( Multi::runGuestWaitingRoom( *session, error ) ) {
        case Multi::WaitingRoomEnd::STARTED:
            Multi::startGuestGame( std::move( session ) );
            return { fheroes2::GameMode::MULTI_WAIT_FOR_TURN, {}, {} };
        case Multi::WaitingRoomEnd::QUIT_GAME:
            return { fheroes2::GameMode::QUIT_GAME, {}, {} };
        case Multi::WaitingRoomEnd::FAILED:
            ERROR_LOG( "[MULTI] " << error )
            return { {}, _( "Network game" ), std::string( _( "The network game has been stopped:" ) ) + "\n\n" + Multi::translateError( error ) };
        default:
            return {};
        }
    }

    fheroes2::ButtonSprite makeButton( const fheroes2::Rect & area, const char * untranslatedText )
    {
        return Multi::makeTextButton( area, untranslatedText );
    }

    void updateButtonState( fheroes2::ButtonSprite & button, const bool isEnabled )
    {
        if ( isEnabled ) {
            button.enable();
        }
        else {
            button.disable();
        }
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

    // The choices of the host before the scenario screen: name, list or invite code, turns, lobby. Returns the next mode
    // of the main loop, nothing to go back to the list.
    std::optional<fheroes2::GameMode> askWhatToHost( Multi::NetworkSettings & settings, const std::vector<Multi::LobbyState> & lobbies )
    {
        std::vector<const Multi::LobbyState *> openLobbies;
        for ( const Multi::LobbyState & lobby : lobbies ) {
            if ( lobby.status == Multi::LobbyState::Status::OPEN ) {
                openLobbies.push_back( &lobby );
            }
        }

        if ( openLobbies.empty() ) {
            fheroes2::showStandardTextMessage( _( "Host a game" ), _( "No lobby answers for now. You can still host a game by address from the network menu." ),
                                               Dialog::OK );
            return {};
        }

        fheroes2::Display & display = fheroes2::Display::instance();
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        const fheroes2::Sprite & tallButton = Assets::getImage( isEvilInterface ? ICN::BUTTONS_NEW_GAME_MENU_EVIL : ICN::BUTTONS_NEW_GAME_MENU_GOOD, 12 );
        const int32_t smallButtonHeight = Assets::getImage( isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0 ).height();

        const int32_t width = 420;
        const int32_t tallButtonWidth = ( width - 2 * margin - 12 ) / 2;
        const bool isLobbyChoice = openLobbies.size() > 1;
        const int32_t height = 200 + ( isLobbyChoice ? 26 : 0 ) + tallButton.height() + smallButtonHeight;

        fheroes2::StandardWindow window( width, height, true, display );
        const fheroes2::Rect & area = window.activeArea();

        const fheroes2::Text title( _( "Host a game" ), fheroes2::FontType::normalYellow() );
        title.draw( area.x + ( area.width - title.width() ) / 2, area.y + 14, display );

        const fheroes2::Text nameTitle( _( "Name of the game:" ), fheroes2::FontType::normalYellow() );
        nameTitle.draw( area.x + margin, area.y + 46, display );

        const fheroes2::Rect nameRoi( area.x + margin, area.y + 66, area.width - 2 * margin, 26 );
        window.applyTextBackgroundShading( nameRoi );
        fheroes2::ImageRestorer nameBackground( display, nameRoi.x, nameRoi.y, nameRoi.width, nameRoi.height );

        int32_t optionsY = nameRoi.y + nameRoi.height + 18;
        fheroes2::MovableSprite publicMark;
        const fheroes2::Rect publicArea
            = Editor::drawCheckboxWithText( publicMark, _( "In the list of the lobby" ), display, area.x + margin, optionsY, isEvilInterface, area.width - 2 * margin );
        optionsY += 30;
        fheroes2::MovableSprite simultaneousMark;
        const fheroes2::Rect simultaneousArea
            = Editor::drawCheckboxWithText( simultaneousMark, _( "Simultaneous turns" ), display, area.x + margin, optionsY, isEvilInterface, area.width - 2 * margin );
        optionsY += 30;

        // The lobby where the game goes, when there are several.
        const fheroes2::Rect lobbyRoi( area.x + margin, optionsY, area.width - 2 * margin, 22 );
        std::optional<fheroes2::ImageRestorer> lobbyBackground;
        if ( isLobbyChoice ) {
            lobbyBackground.emplace( display, lobbyRoi.x, lobbyRoi.y, lobbyRoi.width, lobbyRoi.height );
            optionsY += 26;
        }

        const int32_t buttonsY = area.y + area.height - margin - smallButtonHeight - 10 - tallButton.height();
        std::array<fheroes2::ButtonSprite, 3> buttons{ makeButton( { area.x + margin, buttonsY, tallButtonWidth, tallButton.height() },
                                                                   gettext_noop( "HOST A\nNEW GAME" ) ),
                                                       makeButton( { area.x + area.width - margin - tallButtonWidth, buttonsY, tallButtonWidth, tallButton.height() },
                                                                   gettext_noop( "HOST A\nSAVED GAME" ) ),
                                                       makeButton( { area.x + ( area.width - 120 ) / 2, area.y + area.height - margin - smallButtonHeight, 120,
                                                                     smallButtonHeight },
                                                                   gettext_noop( "CANCEL" ) ) };
        const fheroes2::ButtonSprite & buttonNewGame = buttons[0];
        const fheroes2::ButtonSprite & buttonSavedGame = buttons[1];
        const fheroes2::ButtonSprite & buttonCancel = buttons[2];

        for ( const fheroes2::ButtonSprite & button : buttons ) {
            button.draw();
            button.drawShadow( display );
        }

        // Typed in the code page of the game, sent in UTF-8.
        std::string gameName;
        size_t lobbyIndex = 0;

        const auto redraw = [&]() {
            nameBackground.restore();
            const bool isMapName = gameName.empty();
            fheroes2::Text name( isMapName ? _( "The name of the map" ) : gameName,
                                 isMapName ? fheroes2::FontType{ fheroes2::FontSize::NORMAL, fheroes2::FontColor::GRAY } : fheroes2::FontType::normalWhite() );
            name.fitToOneRow( nameRoi.width - 12 );
            name.draw( nameRoi.x + 6, nameRoi.y + 7, display );

            showMark( publicMark, settings.isPublic );
            showMark( simultaneousMark, settings.isSimultaneous );

            if ( lobbyBackground ) {
                lobbyBackground->restore();
                std::string text = _( "Lobby: %{lobby}" );
                StringReplace( text, "%{lobby}", describeLobbyName( *openLobbies[lobbyIndex] ) );
                fheroes2::Text lobbyText( text, fheroes2::FontType::normalWhite() );
                lobbyText.fitToOneRow( lobbyRoi.width );
                lobbyText.draw( lobbyRoi.x, lobbyRoi.y + 4, display );
            }

            display.render( window.totalArea() );
        };

        redraw();

        const auto host = [&]( const bool isSavedGame ) {
            saveNetworkSettings( settings );

            Multi::LobbyHosting hosting;
            hosting.lobbyAddress = openLobbies[lobbyIndex]->address;
            hosting.nickname = settings.nickname;
            hosting.gameName = Multi::toUtf8( gameName );
            hosting.isPublic = settings.isPublic;
            hosting.isRelayOnly = settings.isRelayAlways;

            return Multi::hostOnLobby( hosting, isSavedGame, settings.isSimultaneous );
        };

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            for ( fheroes2::ButtonSprite & button : buttons ) {
                button.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( button.area() ) );
            }

            if ( le.MouseClickLeft( buttonNewGame.area() ) ) {
                return host( false );
            }
            if ( le.MouseClickLeft( buttonSavedGame.area() ) ) {
                return host( true );
            }
            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return {};
            }

            if ( le.MouseClickLeft( nameRoi ) ) {
                std::string text = gameName;
                const fheroes2::Text inputTitle( _( "Name of the game" ), fheroes2::FontType::normalYellow() );
                const fheroes2::Text inputBody( _( "The name other players see in the list. Leave it empty for the name of the map." ),
                                                fheroes2::FontType::normalWhite() );
                if ( Dialog::inputString( inputTitle, inputBody, text, 40, false, {} ) ) {
                    gameName = Multi::trimText( text );
                }
                redraw();
            }
            else if ( le.MouseClickLeft( publicArea ) ) {
                settings.isPublic = !settings.isPublic;
                redraw();
            }
            else if ( le.MouseClickLeft( simultaneousArea ) ) {
                settings.isSimultaneous = !settings.isSimultaneous;
                redraw();
            }
            else if ( isLobbyChoice && le.MouseClickLeft( lobbyRoi ) ) {
                lobbyIndex = ( lobbyIndex + 1 ) % openLobbies.size();
                redraw();
            }

            if ( le.isMouseRightButtonPressedInArea( nameRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Name of the game" ), _( "The name other players see in the list. Click to change it." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( publicArea ) ) {
                fheroes2::showStandardTextMessage( _( "In the list of the lobby" ),
                                                   _( "Every player of the lobby sees your game and can join it. Otherwise only the players you give its "
                                                      "invite code to can join it." ),
                                                   Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( simultaneousArea ) ) {
                fheroes2::showStandardTextMessage( _( "Simultaneous turns" ),
                                                   _( "Every player plays their day at the same time and sees what the others do. The computer players "
                                                      "play afterwards. The host makes the choice." ),
                                                   Dialog::ZERO );
            }
            else if ( isLobbyChoice && le.isMouseRightButtonPressedInArea( lobbyRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Lobby" ), _( "The lobby where your game is published. Click to choose another one." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonNewGame.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Host a new game" ), _( "Choose a map. Your game is published on the lobby once the scenario is chosen." ),
                                                   Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonSavedGame.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Host a saved game" ), _( "Load a Hot Seat game. It is published on the lobby once loaded." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Back to the list of games." ), Dialog::ZERO );
            }
        }

        return fheroes2::GameMode::QUIT_GAME;
    }
}

bool Multi::askNickname( NetworkSettings & settings )
{
    std::string text = fromUtf8( settings.nickname );

    while ( true ) {
        const fheroes2::Text title( _( "Nickname" ), fheroes2::FontType::normalYellow() );
        const fheroes2::Text body( _( "The name other players see in the lobbies:" ), fheroes2::FontType::normalWhite() );

        if ( !Dialog::inputString( title, body, text, 20, false, {} ) ) {
            return !settings.nickname.empty();
        }

        const std::string nickname = toUtf8( trimText( text ) );
        if ( isValidNickname( nickname ) ) {
            settings.nickname = nickname;
            saveNetworkSettings( settings );
            return true;
        }

        fheroes2::showStandardTextMessage( _( "Nickname" ), _( "A nickname has 1 to 20 characters." ), Dialog::OK );
    }
}

fheroes2::GameMode Multi::lobbyScreen()
{
    NetworkSettings settings = loadNetworkSettings();
    if ( settings.nickname.empty() && !askNickname( settings ) ) {
        return fheroes2::GameMode::MULTI_NETWORK_MENU;
    }

    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();
    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    fheroes2::drawMainMenuScreen();

    fheroes2::StandardWindow window( windowWidth, windowHeight, true, display );
    const fheroes2::Rect & area = window.activeArea();

    // From the top: the state of the lobbies, the column headers, the list, the details of the selected game, the buttons.
    const fheroes2::Rect statusRoi( area.x + margin, area.y + 30, area.width - 2 * margin, 20 );
    const fheroes2::Rect headerRoi( area.x + margin, statusRoi.y + statusRoi.height + 4, area.width - 2 * margin - scrollbarWidth, 22 );
    const fheroes2::Rect listRoi( headerRoi.x, headerRoi.y + headerRoi.height + 2, headerRoi.width, visibleRooms * rowHeight + 6 );
    const fheroes2::Rect detailsRoi( listRoi.x, listRoi.y + listRoi.height + 6, area.width - 2 * margin, detailsHeight );

    // The list and the details have their own backgrounds, restored at each change.
    window.applyTextBackgroundShading( listRoi );
    window.applyTextBackgroundShading( detailsRoi );

    // What never changes, drawn again after a window over the whole screen.
    const auto drawFrame = [&]() {
        const fheroes2::Text title( _( "Online lobby" ), fheroes2::FontType::normalYellow() );
        title.draw( area.x + ( area.width - title.width() ) / 2, area.y + 10, display );

        window.applyTextBackgroundShading( headerRoi );
        window.renderScrollbarBackground( { listRoi.x + listRoi.width + 2, listRoi.y, scrollbarWidth - 2, listRoi.height }, isEvilInterface );

        const auto drawHeader = [&display, &headerRoi]( const char * text, const int32_t columnX, const int32_t width ) {
            fheroes2::Text header( text, fheroes2::FontType::normalYellow() );
            header.fitToOneRow( width );
            header.draw( headerRoi.x + columnX, headerRoi.y + 5, display );
        };
        drawHeader( _( "lobby|Game" ), nameColumnX, nameColumnWidth );
        drawHeader( _( "lobby|Map" ), mapColumnX, mapColumnWidth );
        drawHeader( _( "lobby|Turns" ), turnsColumnX, turnsColumnWidth );
        drawHeader( _( "lobby|Host" ), hostColumnX, hostColumnWidth );
    };
    drawFrame();

    fheroes2::ImageRestorer statusBackground( display, statusRoi.x, statusRoi.y, statusRoi.width, statusRoi.height );
    fheroes2::ImageRestorer detailsBackground( display, detailsRoi.x, detailsRoi.y, detailsRoi.width, detailsRoi.height );

    RoomListBox list( area.getPosition() );
    list.initBackground( listRoi );
    list.SetAreaItems( { listRoi.x, listRoi.y + 3, listRoi.width, visibleRooms * rowHeight } );
    list.SetAreaMaxItems( visibleRooms );

    const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;
    const int32_t scrollbarX = listRoi.x + listRoi.width + 3;
    list.SetScrollButtonUp( listIcnId, 0, 1, { scrollbarX, listRoi.y + 1 } );
    list.SetScrollButtonDn( listIcnId, 2, 3, { scrollbarX, listRoi.y + listRoi.height - 15 } );
    list.setScrollBarArea( { scrollbarX + 2, listRoi.y + 19, 10, listRoi.height - 38 } );
    list.setScrollBarImage( Assets::getImage( listIcnId, 4 ) );

    // Buttons at the bottom.
    const int32_t buttonHeight = Assets::getImage( isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0 ).height();
    constexpr int32_t buttonCount = 4;
    const int32_t buttonSpacing = 12;
    const int32_t buttonWidth = ( area.width - 2 * margin - ( buttonCount - 1 ) * buttonSpacing ) / buttonCount;
    const int32_t buttonsY = area.y + area.height - margin - buttonHeight;

    const auto buttonArea = [&area, buttonWidth, buttonHeight, buttonsY]( const int32_t index ) {
        return fheroes2::Rect{ area.x + margin + index * ( buttonWidth + buttonSpacing ), buttonsY, buttonWidth, buttonHeight };
    };

    std::array<fheroes2::ButtonSprite, buttonCount> buttons{ makeButton( buttonArea( 0 ), gettext_noop( "lobby|HOST" ) ),
                                                             makeButton( buttonArea( 1 ), gettext_noop( "lobby|JOIN" ) ),
                                                             makeButton( buttonArea( 2 ), gettext_noop( "lobby|CODE" ) ),
                                                             makeButton( buttonArea( 3 ), gettext_noop( "lobby|BACK" ) ) };
    const fheroes2::ButtonSprite & buttonHost = buttons[0];
    fheroes2::ButtonSprite & buttonJoin = buttons[1];
    const fheroes2::ButtonSprite & buttonCode = buttons[2];
    const fheroes2::ButtonSprite & buttonBack = buttons[3];

    const auto drawButtonShadows = [&buttons, &display]() {
        for ( const fheroes2::ButtonSprite & button : buttons ) {
            button.drawShadow( display );
        }
    };
    drawButtonShadows();

    auto browser = std::make_unique<LobbyBrowser>( getLobbyAddresses( settings ), settings.nickname );
    std::vector<ListedRoom> rooms;

    const auto redrawAll = [&]() {
        statusBackground.restore();
        fheroes2::Text status( describeLobbies( browser->lobbies() ), fheroes2::FontType::normalWhite() );
        status.fitToOneRow( statusRoi.width );
        status.draw( statusRoi.x + ( statusRoi.width - status.width() ) / 2, statusRoi.y + 4, display );

        list.Redraw();

        if ( rooms.empty() ) {
            const bool isAnyOpen = std::any_of( browser->lobbies().begin(), browser->lobbies().end(),
                                                []( const LobbyState & lobby ) { return lobby.status == LobbyState::Status::OPEN; } );

            const char * hint = isAnyOpen ? _( "No game is waiting for players.\n\nHost yours: other players will see it here." )
                                          : _( "Games of the lobby are shown here once it answers. You can also play by address from the network menu." );
            const fheroes2::Text text( hint, fheroes2::FontType::normalWhite() );
            const int32_t hintWidth = listRoi.width - 60;
            text.draw( listRoi.x + 30, listRoi.y + ( listRoi.height - text.height( hintWidth ) ) / 2, hintWidth, display );
        }

        const ListedRoom * selected = getSelectedRoom( list, rooms );

        detailsBackground.restore();
        const fheroes2::Text details( describeSelection( selected, browser->lobbies() ), fheroes2::FontType::normalWhite() );
        details.drawInRoi( detailsRoi.x + 6, detailsRoi.y + 5, detailsRoi.width - 12, display, detailsRoi );

        updateButtonState( buttonJoin, selected != nullptr && selected->isSameGame );
        for ( const fheroes2::ButtonSprite & button : buttons ) {
            button.draw();
        }

        display.render( window.totalArea() );
    };

    // Keeps the selected room selected when the list changes.
    const auto updateRooms = [&]() {
        std::optional<std::pair<size_t, uint64_t>> selectedKey;
        if ( const ListedRoom * selected = getSelectedRoom( list, rooms ); selected != nullptr ) {
            selectedKey = std::make_pair( selected->lobbyIndex, selected->info.roomId );
        }

        rooms = browser->rooms();
        list.SetListContent( rooms );

        const auto found = std::find_if( rooms.begin(), rooms.end(), [&selectedKey]( const ListedRoom & room ) {
            return selectedKey && room.lobbyIndex == selectedKey->first && room.info.roomId == selectedKey->second;
        } );

        if ( found != rooms.end() ) {
            list.SetCurrent( static_cast<size_t>( found - rooms.begin() ) );
        }
        else {
            list.Unselect();
        }

        list.updateScrollBarImage();
    };

    updateRooms();
    redrawAll();

    // Back to the list after a join which did not start a game, with the reason over it.
    const auto join = [&]( const ListedRoom * room, const std::string & code ) -> std::optional<fheroes2::GameMode> {
        const JoinOutcome outcome = joinLobbyRoom( *browser, room, code, settings );
        if ( outcome.mode ) {
            return outcome.mode;
        }

        fheroes2::drawMainMenuScreen();
        window.render();
        drawFrame();
        drawButtonShadows();
        updateRooms();
        redrawAll();

        if ( !outcome.message.empty() ) {
            fheroes2::showStandardTextMessage( outcome.title, outcome.message, Dialog::OK );
        }
        return {};
    };

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents( true, true ) ) {
        for ( fheroes2::ButtonSprite & button : buttons ) {
            if ( button.isEnabled() ) {
                button.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( button.area() ) );
            }
        }

        const int32_t selectedBefore = list.getCurrentId();
        list.QueueEventProcessing();

        const bool isChanged = browser->update();
        if ( isChanged ) {
            updateRooms();
        }
        if ( isChanged || list.getCurrentId() != selectedBefore || list.IsNeedRedraw() ) {
            redrawAll();
        }

        // MouseClickLeft() consumes the click, check every area only once.
        const bool isJoinAsked = list.takeDoubleClick() || ( buttonJoin.isEnabled() && le.MouseClickLeft( buttonJoin.area() ) )
                                 || ( buttonJoin.isEnabled() && Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) );
        if ( isJoinAsked ) {
            const ListedRoom * selected = getSelectedRoom( list, rooms );
            if ( selected != nullptr && !selected->isSameGame ) {
                fheroes2::showStandardTextMessage( _( "Online lobby" ), describeJoinRefusal( JoinResult::OTHER_VERSION, false ), Dialog::OK );
                redrawAll();
            }
            else if ( selected != nullptr ) {
                // The list may change while joining.
                const ListedRoom room = *selected;
                if ( const std::optional<fheroes2::GameMode> mode = join( &room, {} ); mode ) {
                    return *mode;
                }
            }
            continue;
        }

        if ( le.MouseClickLeft( buttonCode.area() ) ) {
            std::string text;
            const fheroes2::Text codeTitle( _( "Invite code" ), fheroes2::FontType::normalYellow() );
            const fheroes2::Text codeBody( _( "The invite code given by the host:" ), fheroes2::FontType::normalWhite() );

            if ( Dialog::inputString( codeTitle, codeBody, text, 9, false, {} ) && !trimText( text ).empty() ) {
                const std::string code = normalizeInviteCode( text );
                if ( !isValidInviteCode( code ) ) {
                    fheroes2::showStandardTextMessage( _( "Invite code" ), _( "An invite code has 6 letters and digits." ), Dialog::OK );
                }
                else if ( const std::optional<fheroes2::GameMode> mode = join( nullptr, code ); mode ) {
                    return *mode;
                }
            }
            redrawAll();
            continue;
        }

        if ( le.MouseClickLeft( buttonHost.area() ) ) {
            if ( const std::optional<fheroes2::GameMode> mode = askWhatToHost( settings, browser->lobbies() ); mode ) {
                return *mode;
            }
            redrawAll();
            continue;
        }

        if ( le.MouseClickLeft( buttonBack.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            return fheroes2::GameMode::MULTI_NETWORK_MENU;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonHost.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Host" ), _( "Host a new or a saved game. It is shown in this list until all its players are there." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonJoin.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Join" ), _( "Join the selected game. A double click on a game does the same." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonCode.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Code" ),
                                               _( "Join a game with the invite code its host gave you. Games for invited players only are not in the list." ),
                                               Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonBack.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Back" ), _( "Back to the network menu, where the network settings are." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( statusRoi ) ) {
            fheroes2::showStandardTextMessage( _( "Online lobby" ),
                                               _( "A lobby lists the games waiting for players and helps the computers to connect. Its relay passes the game "
                                                  "on when no direct connection works." ),
                                               Dialog::ZERO );
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}

void Multi::networkSettingsDialog( NetworkSettings & settings )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    constexpr int32_t lobbyRows = 5;

    fheroes2::StandardWindow window( 460, 330, true, display );
    const fheroes2::Rect & area = window.activeArea();

    const fheroes2::Text title( _( "Network settings" ), fheroes2::FontType::normalYellow() );
    title.draw( area.x + ( area.width - title.width() ) / 2, area.y + 12, display );

    const int32_t buttonHeight = Assets::getImage( isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0 ).height();
    const int32_t smallButtonWidth = 128;

    const fheroes2::Rect nicknameRoi( area.x + margin, area.y + 42, area.width - 2 * margin - smallButtonWidth - 8, 24 );
    const fheroes2::Rect lobbiesRoi( area.x + margin, nicknameRoi.y + nicknameRoi.height + 34, area.width - 2 * margin, lobbyRows * rowHeight + 6 );
    window.applyTextBackgroundShading( nicknameRoi );
    window.applyTextBackgroundShading( lobbiesRoi );

    const fheroes2::Text lobbiesTitle( _( "Lobbies:" ), fheroes2::FontType::normalYellow() );
    lobbiesTitle.draw( lobbiesRoi.x, lobbiesRoi.y - 20, display );

    fheroes2::ImageRestorer nicknameBackground( display, nicknameRoi.x, nicknameRoi.y, nicknameRoi.width, nicknameRoi.height );
    fheroes2::ImageRestorer lobbiesBackground( display, lobbiesRoi.x, lobbiesRoi.y, lobbiesRoi.width, lobbiesRoi.height );

    const int32_t lobbyButtonsY = lobbiesRoi.y + lobbiesRoi.height + 6;
    fheroes2::MovableSprite relayMark;
    const fheroes2::Rect relayArea = Editor::drawCheckboxWithText( relayMark, _( "Always go through the relay" ), display, area.x + margin,
                                                                   lobbyButtonsY + buttonHeight + 14, isEvilInterface, area.width - 2 * margin );

    std::array<fheroes2::ButtonSprite, 4>
        buttons{ makeButton( { area.x + area.width - margin - smallButtonWidth, nicknameRoi.y - 1, smallButtonWidth, buttonHeight }, gettext_noop( "lobby|CHANGE" ) ),
                 makeButton( { lobbiesRoi.x, lobbyButtonsY, smallButtonWidth, buttonHeight }, gettext_noop( "lobby|ADD" ) ),
                 makeButton( { lobbiesRoi.x + smallButtonWidth + 8, lobbyButtonsY, smallButtonWidth, buttonHeight }, gettext_noop( "lobby|REMOVE" ) ),
                 makeButton( { area.x + ( area.width - smallButtonWidth ) / 2, area.y + area.height - margin - buttonHeight, smallButtonWidth, buttonHeight },
                             gettext_noop( "OKAY" ) ) };
    const fheroes2::ButtonSprite & buttonNickname = buttons[0];
    fheroes2::ButtonSprite & buttonAdd = buttons[1];
    fheroes2::ButtonSprite & buttonRemove = buttons[2];
    const fheroes2::ButtonSprite & buttonOk = buttons[3];

    for ( const fheroes2::ButtonSprite & button : buttons ) {
        button.drawShadow( display );
    }

    // Index in the lobbies added by the player. The shared lobby cannot be removed.
    const int32_t firstAddedRow = settings.isSharedLobbyUsed ? 1 : 0;
    std::optional<size_t> selectedLobby;

    const auto lobbyRowArea
        = [&lobbiesRoi]( const int32_t row ) { return fheroes2::Rect{ lobbiesRoi.x, lobbiesRoi.y + 3 + row * rowHeight, lobbiesRoi.width, rowHeight }; };

    // Each lobby is asked whether it answers, again when the list or the nickname changes. A lobby refuses a game without
    // nickname.
    std::unique_ptr<LobbyBrowser> browser;
    const auto watchLobbies = [&browser, &settings]() {
        browser.reset();
        if ( !settings.nickname.empty() ) {
            browser = std::make_unique<LobbyBrowser>( getLobbyAddresses( settings ), settings.nickname );
        }
    };
    watchLobbies();

    const auto getLobbyState = [&browser]( const int32_t row ) -> const LobbyState * {
        if ( !browser || row < 0 || static_cast<size_t>( row ) >= browser->lobbies().size() ) {
            return nullptr;
        }
        return &browser->lobbies()[static_cast<size_t>( row )];
    };

    const auto describeState = []( const LobbyState & state ) -> const char * {
        switch ( state.status ) {
        case LobbyState::Status::OPEN:
            return _( "lobby|online" );
        case LobbyState::Status::CONNECTING:
            return _( "lobby|connecting..." );
        case LobbyState::Status::REFUSED:
            return _( "lobby|refused" );
        default:
            return _( "lobby|no answer" );
        }
    };

    const auto redraw = [&]() {
        nicknameBackground.restore();
        fheroes2::Text nickname( std::string( _( "Nickname: " ) ) + fromUtf8( settings.nickname ), fheroes2::FontType::normalWhite() );
        nickname.fitToOneRow( nicknameRoi.width - 12 );
        nickname.draw( nicknameRoi.x + 6, nicknameRoi.y + 6, display );

        lobbiesBackground.restore();
        const std::vector<std::string> addresses = getLobbyAddresses( settings );
        for ( int32_t row = 0; row < lobbyRows && static_cast<size_t>( row ) < addresses.size(); ++row ) {
            const bool isShared = ( settings.isSharedLobbyUsed && row == 0 );
            const bool isSelected = !isShared && selectedLobby && *selectedLobby == static_cast<size_t>( row - firstAddedRow );

            std::string text = addresses[static_cast<size_t>( row )];
            if ( isShared ) {
                text += std::string( " " ) + _( "(shared)" );
            }

            // The state on the right, greyed unless the lobby answers.
            int32_t stateWidth = 0;
            if ( const LobbyState * state = getLobbyState( row ); state != nullptr ) {
                const bool isOpen = ( state->status == LobbyState::Status::OPEN );
                const fheroes2::Text stateText( describeState( *state ),
                                                { fheroes2::FontSize::NORMAL, isOpen ? fheroes2::FontColor::WHITE : fheroes2::FontColor::GRAY } );
                stateWidth = stateText.width() + 12;
                stateText.draw( lobbiesRoi.x + lobbiesRoi.width - 6 - stateText.width(), lobbyRowArea( row ).y + 3, display );
            }

            fheroes2::Text line( text, isSelected ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
            line.fitToOneRow( lobbiesRoi.width - 12 - stateWidth );
            line.draw( lobbiesRoi.x + 6, lobbyRowArea( row ).y + 3, display );
        }

        showMark( relayMark, settings.isRelayAlways );

        updateButtonState( buttonRemove, selectedLobby.has_value() );
        updateButtonState( buttonAdd, settings.lobbies.size() + static_cast<size_t>( firstAddedRow ) < static_cast<size_t>( lobbyRows ) );

        for ( const fheroes2::ButtonSprite & button : buttons ) {
            button.draw();
        }

        display.render( window.totalArea() );
    };

    redraw();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents( true, true ) ) {
        for ( fheroes2::ButtonSprite & button : buttons ) {
            if ( button.isEnabled() ) {
                button.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( button.area() ) );
            }
        }

        if ( browser && browser->update() ) {
            redraw();
        }

        if ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY )
             || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            break;
        }

        const NetworkSettings beforeClick = settings;

        if ( le.MouseClickLeft( buttonNickname.area() ) ) {
            askNickname( settings );
            redraw();
        }
        else if ( buttonAdd.isEnabled() && le.MouseClickLeft( buttonAdd.area() ) ) {
            std::string address;
            const fheroes2::Text addTitle( _( "Add a lobby" ), fheroes2::FontType::normalYellow() );
            const fheroes2::Text addBody( _( "Address of the lobby, as given by the one who runs it:" ), fheroes2::FontType::normalWhite() );

            if ( Dialog::inputString( addTitle, addBody, address, 100, false, {} ) ) {
                address = trimText( address );
                if ( address.empty() ) {
                    // Nothing typed.
                }
                else if ( !parseLobbyAddress( address ) ) {
                    fheroes2::showStandardTextMessage( _( "Add a lobby" ), _( "This address is not valid." ), Dialog::OK );
                }
                else if ( std::find( settings.lobbies.begin(), settings.lobbies.end(), address ) == settings.lobbies.end() && address != sharedLobbyAddress ) {
                    settings.lobbies.push_back( address );
                }
            }
            redraw();
        }
        else if ( buttonRemove.isEnabled() && le.MouseClickLeft( buttonRemove.area() ) ) {
            if ( selectedLobby && *selectedLobby < settings.lobbies.size() ) {
                settings.lobbies.erase( settings.lobbies.begin() + static_cast<std::ptrdiff_t>( *selectedLobby ) );
            }
            selectedLobby.reset();
            redraw();
        }
        else if ( le.MouseClickLeft( relayArea ) ) {
            settings.isRelayAlways = !settings.isRelayAlways;
            redraw();
        }
        else {
            for ( int32_t row = firstAddedRow; row < lobbyRows; ++row ) {
                if ( static_cast<size_t>( row - firstAddedRow ) < settings.lobbies.size() && le.MouseClickLeft( lobbyRowArea( row ) ) ) {
                    selectedLobby = static_cast<size_t>( row - firstAddedRow );
                    redraw();
                    break;
                }
            }
        }

        if ( settings.nickname != beforeClick.nickname || settings.lobbies != beforeClick.lobbies ) {
            watchLobbies();
            redraw();
        }

        if ( le.isMouseRightButtonPressedInArea( relayArea ) ) {
            fheroes2::showStandardTextMessage( _( "Always go through the relay" ),
                                               _( "Your games go through the relay of the lobby even when a direct connection would work. The other players never "
                                                  "learn your address. The game may be a little slower." ),
                                               Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( nicknameRoi ) || le.isMouseRightButtonPressedInArea( buttonNickname.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Nickname" ), _( "The name other players see in the lobbies." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( lobbiesRoi ) ) {
            // On a lobby: what it says, or why it cannot be reached.
            std::string text = _( "The shared lobby, then the ones you added. A lobby is a small free program which anyone can run, for example for a "
                                  "group of friends. The list of games shows the games of all of them." );
            for ( int32_t row = 0; row < lobbyRows; ++row ) {
                const LobbyState * state = getLobbyState( row );
                if ( state == nullptr || !( lobbyRowArea( row ) & le.getMouseCursorPos() ) ) {
                    continue;
                }

                if ( state->status == LobbyState::Status::OPEN ) {
                    text = describeLobbyName( *state );
                    if ( !state->welcome.messageOfTheDay.empty() ) {
                        text += "\n\n" + shown( state->welcome.messageOfTheDay );
                    }
                }
                else if ( state->status != LobbyState::Status::CONNECTING ) {
                    text = _( "The lobby cannot be reached: %{reason}." );
                    StringReplace( text, "%{reason}", translateError( state->error ) );
                }
            }
            fheroes2::showStandardTextMessage( _( "Lobbies" ), text, Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonAdd.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Add a lobby" ), _( "Add the address of another lobby. Its games are shown in the same list." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonRemove.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Remove" ), _( "Remove the selected lobby. The shared lobby always stays." ), Dialog::ZERO );
        }
    }

    saveNetworkSettings( settings );
}
