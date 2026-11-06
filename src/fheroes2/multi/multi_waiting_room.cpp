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

#include "multi_waiting_room.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <thread>
#include <utility>
#include <vector>

#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_assets.h"
#include "game_delays.h"
#include "game_exit.h"
#include "game_mainmenu_ui.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "logging.h"
#include "multi_lobby.h"
#include "multi_lobby_client.h"
#include "multi_machines.h"
#include "multi_menu.h"
#include "multi_network.h"
#include "multi_settings.h"
#include "multi_text.h"
#include "players.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_keyboard.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    // Same size as the lobby screen, it fits the smallest screen of the game.
    constexpr int32_t windowWidth{ 600 };
    constexpr int32_t windowHeight{ 440 };
    constexpr int32_t margin{ 16 };
    constexpr int32_t leftWidth{ 236 };
    constexpr int32_t columnSpacing{ 16 };
    constexpr int32_t headerHeight{ 20 };
    // One line for each of the six colors.
    constexpr int32_t playerLineHeight{ 18 };
    constexpr int32_t playersHeight{ 6 * playerLineHeight + 8 };
    constexpr int32_t inputHeight{ 26 };
    constexpr int32_t sendButtonWidth{ 124 };
    constexpr int32_t bottomButtonWidth{ 150 };

    // In the code page of the game, which takes one byte per character.
    constexpr size_t maxChatCharacters{ 120 };
    constexpr size_t maxChatLines{ 50 };

    // A guest cut for longer is gone, the room waits for another one.
    constexpr std::chrono::seconds lostGuestTimeout{ 20 };

    constexpr std::array<PlayerColor, 6> colorOrder{ PlayerColor::BLUE,   PlayerColor::GREEN,  PlayerColor::RED,
                                                     PlayerColor::YELLOW, PlayerColor::ORANGE, PlayerColor::PURPLE };

    uint8_t lowestColor( const uint8_t colors )
    {
        return static_cast<uint8_t>( colors & ( ~colors + 1 ) );
    }

    std::string describeGame( const Multi::RoomGame & game )
    {
        std::string text = game.day > 1 ? _( "%{map}, %{width} x %{height}, day %{day}." ) : _( "%{map}, %{width} x %{height}, new game." );
        StringReplace( text, "%{map}", Multi::fromUtf8( game.mapName ) );
        StringReplace( text, "%{width}", game.mapWidth );
        StringReplace( text, "%{height}", game.mapHeight );
        StringReplace( text, "%{day}", static_cast<int>( game.day ) );

        text += ' ';
        text += game.turns == Multi::RoomTurns::SIMULTANEOUS ? _( "Simultaneous turns." ) : _( "Turns one after the other." );
        return text;
    }

    // The name shown for a player, in the code page of the game.
    std::string getPlayerName( const Multi::RoomPlayer & player )
    {
        if ( !player.nickname.empty() ) {
            return Multi::fromUtf8( player.nickname );
        }

        if ( player.machine == Multi::hostMachine ) {
            return _( "player|Host" );
        }

        std::string name = _( "Guest %{number}" );
        StringReplace( name, "%{number}", static_cast<int>( player.machine ) );
        return name;
    }

    const Multi::RoomPlayer * findPlayer( const Multi::WaitingRoomInfo & info, const Multi::MachineId machine )
    {
        const auto iter = std::find_if( info.players.begin(), info.players.end(), [machine]( const Multi::RoomPlayer & player ) { return player.machine == machine; } );
        return iter != info.players.end() ? &*iter : nullptr;
    }

    // The other computers of the room.
    Multi::MachineSet getOthersInRoom( const Multi::WaitingRoomInfo & info, const Multi::MachineId self )
    {
        Multi::MachineSet machines = 0;
        for ( const Multi::RoomPlayer & player : info.players ) {
            if ( player.machine != self ) {
                machines |= Multi::machineBit( player.machine );
            }
        }
        return machines;
    }

    struct ChatLine
    {
        // Empty for a note of the game.
        std::string name;
        std::string text;
    };

    // The window, the same for every computer.
    class RoomScreen
    {
    public:
        enum class Action
        {
            NONE,
            START,
            SEND,
            // A click on the line of a color.
            COLOR,
            LEAVE,
            QUIT_GAME
        };

        explicit RoomScreen( const bool isHost )
            : _isHost( isHost )
        {
            _draw();
        }

        void setInfo( const Multi::WaitingRoomInfo & info )
        {
            _info = info;
            _isChanged = true;
        }

        const std::optional<Multi::WaitingRoomInfo> & info() const
        {
            return _info;
        }

        Multi::MachineId ownMachine() const
        {
            return ( _isHost || !_info ) ? Multi::hostMachine : _info->receiver;
        }

        // The name of a player of the room, or nothing.
        std::string nameOf( const Multi::MachineId machine ) const
        {
            const Multi::RoomPlayer * player = _info ? findPlayer( *_info, machine ) : nullptr;
            return player != nullptr ? getPlayerName( *player ) : std::string();
        }

        uint8_t ownColors() const
        {
            const Multi::RoomPlayer * player = _info ? findPlayer( *_info, ownMachine() ) : nullptr;
            return player != nullptr ? player->colors : 0;
        }

        // Host only.
        void setStartEnabled( const bool isEnabled )
        {
            if ( isEnabled != _isStartEnabled ) {
                _isStartEnabled = isEnabled;
                _isChanged = true;
            }
        }

        void setJoinHelp( const Multi::JoinHelp & help )
        {
            if ( help.inviteCode != _help.inviteCode || help.text != _help.text ) {
                _help = help;
                _isChanged = true;
            }
        }

        // Guest only: a few words about the connection.
        void setStatus( std::string status )
        {
            if ( status != _status ) {
                _status = std::move( status );
                _isChanged = true;
            }
        }

        void setStarting( const bool isStarting )
        {
            _isStarting = isStarting;
            _isChanged = true;
        }

        void addLine( std::string name, std::string text )
        {
            _lines.push_back( { std::move( name ), std::move( text ) } );
            if ( _lines.size() > maxChatLines ) {
                _lines.pop_front();
            }
            _isChanged = true;
        }

        void addNote( std::string text )
        {
            addLine( {}, std::move( text ) );
        }

        std::string takeTypedText()
        {
            _cursorPosition = 0;
            _isChanged = true;
            return std::exchange( _typed, {} );
        }

        // The color of the last click on a line.
        PlayerColor clickedColor() const
        {
            return _clickedColor;
        }

        void redrawIfChanged()
        {
            if ( _isChanged ) {
                _redraw();
            }
        }

        // Handles the events of one frame.
        Action handleEvents()
        {
            LocalEvent & le = LocalEvent::Get();

            if ( !le.HandleEvents( true, true ) ) {
                if ( Game::processExitEvent() == fheroes2::GameMode::QUIT_GAME ) {
                    return Action::QUIT_GAME;
                }
                _draw();
            }

            if ( _isChanged ) {
                _redraw();
            }

            for ( fheroes2::ButtonSprite * button : { &_buttonStart, &_buttonSend, &_buttonLeave } ) {
                if ( button->isEnabled() && button->isVisible() ) {
                    button->drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( button->area() ) );
                }
            }

            if ( le.isKeyPressed( fheroes2::Key::KEY_ESCAPE ) || le.MouseClickLeft( _buttonLeave.area() ) ) {
                const char * question = _isHost ? _( "Are you sure you want to close this network game?" ) : _( "Are you sure you want to leave the network game?" );
                if ( fheroes2::showStandardTextMessage( _( "Network game" ), question, Dialog::YES | Dialog::NO ) == Dialog::YES ) {
                    return Action::LEAVE;
                }
                return Action::NONE;
            }

            if ( _isHost && _buttonStart.isEnabled() && le.MouseClickLeft( _buttonStart.area() ) ) {
                return Action::START;
            }

            if ( !_isStarting && _info && le.MouseClickLeft( _playersRoi ) ) {
                for ( const auto & [area, color] : _colorLines ) {
                    if ( area & le.getMouseLeftButtonPressedPos() ) {
                        _clickedColor = color;
                        return Action::COLOR;
                    }
                }
            }

            if ( _hasOthers() && !_isStarting ) {
                if ( const Action action = _handleTyping(); action != Action::NONE ) {
                    return action;
                }
            }

            if ( _isHost && le.isMouseRightButtonPressedInArea( _buttonStart.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Start" ), _( "Start the game once every player has a color. The colors nobody took are played by the computer." ),
                                                   Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _buttonLeave.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Leave" ), _isHost ? _( "Close this game." ) : _( "Leave this game." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _buttonSend.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Send" ), _( "Send your message to the other players. Enter does the same." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _playersRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Players" ), _( "Click on a free color to play it, or on one of yours to give it back." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _chatRoi ) || le.isMouseRightButtonPressedInArea( _inputRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Chat" ), _( "Only the players of this game see these messages. The chat ends when the game starts." ),
                                                   Dialog::ZERO );
            }

            return Action::NONE;
        }

    private:
        bool _hasOthers() const
        {
            return _info && _info->players.size() > 1;
        }

        Action _handleTyping()
        {
            LocalEvent & le = LocalEvent::Get();
            fheroes2::Display & display = fheroes2::Display::instance();

            if ( ( le.isKeyPressed( fheroes2::Key::KEY_ENTER ) || le.MouseClickLeft( _buttonSend.area() ) ) ) {
                return Multi::trimText( _typed ).empty() ? Action::NONE : Action::SEND;
            }

            bool isTextChanged = false;

            if ( System::isVirtualKeyboardSupported() && le.MouseClickLeft( _inputRoi ) ) {
                fheroes2::openVirtualKeyboard( _typed, maxChatCharacters, false );
                _cursorPosition = _typed.size();
                isTextChanged = true;
            }
            else if ( le.isAnyKeyPressed() ) {
                const fheroes2::Key key = le.getPressedKeyValue();
                const bool isErasing = ( key == fheroes2::Key::KEY_BACKSPACE || key == fheroes2::Key::KEY_DELETE );
                const bool isMoving
                    = ( key == fheroes2::Key::KEY_LEFT || key == fheroes2::Key::KEY_RIGHT || key == fheroes2::Key::KEY_HOME || key == fheroes2::Key::KEY_END );

                if ( isErasing || isMoving || _typed.size() < maxChatCharacters ) {
                    _cursorPosition = InsertKeySym( _typed, _cursorPosition, key, LocalEvent::getCurrentKeyModifiers() );
                    isTextChanged = true;
                }
            }
            else if ( le.MouseClickLeft( _inputRoi ) && _input ) {
                _cursorPosition = _input->getCursorInTextPosition( le.getMouseLeftButtonPressedPos() );
                isTextChanged = true;
            }

            if ( isTextChanged && _input ) {
                _input->draw( _typed, static_cast<int32_t>( _cursorPosition ) );
                display.render( _input->getOverallArea() );
            }
            else if ( _input && _input->eventProcessing() ) {
                display.render( _input->getCursorArea() );
            }

            return Action::NONE;
        }

        // The whole window, again after a dialog over the whole screen.
        void _draw()
        {
            fheroes2::Display & display = fheroes2::Display::instance();
            const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

            _input.reset();
            _window.reset();

            fheroes2::drawMainMenuScreen();

            _window = std::make_unique<fheroes2::StandardWindow>( windowWidth, windowHeight, true, display );
            const fheroes2::Rect & area = _window->activeArea();

            const int32_t rightX = area.x + margin + leftWidth + columnSpacing;
            const int32_t rightWidth = area.x + area.width - margin - rightX;
            const int32_t buttonHeight = Assets::getImage( isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0 ).height();
            const int32_t buttonsY = area.y + area.height - margin - buttonHeight;
            const int32_t boxesY = area.y + 54 + headerHeight;

            _titleRoi = { area.x + margin, area.y + 8, area.width - 2 * margin, 44 };
            _playersRoi = { area.x + margin, boxesY, leftWidth, playersHeight };
            _helpRoi = { area.x + margin, _playersRoi.y + _playersRoi.height + 8 + headerHeight, leftWidth, 0 };
            _helpRoi.height = buttonsY - 10 - _helpRoi.y;
            _inputRoi = { rightX, buttonsY - 10 - inputHeight, rightWidth - sendButtonWidth - 8, inputHeight };
            _chatRoi = { rightX, boxesY, rightWidth, _inputRoi.y - 8 - boxesY };

            for ( const fheroes2::Rect & roi : { _playersRoi, _helpRoi, _chatRoi, _inputRoi } ) {
                _window->applyTextBackgroundShading( roi );
            }

            const auto drawHeader = [&display]( const char * text, const fheroes2::Rect & roi ) {
                const fheroes2::Text header( text, fheroes2::FontType::normalYellow() );
                header.draw( roi.x, roi.y - headerHeight + 3, display );
            };
            drawHeader( _( "Players" ), _playersRoi );
            drawHeader( _isHost ? _( "To join this game" ) : _( "The game" ), _helpRoi );
            drawHeader( _( "Chat" ), _chatRoi );

            _titleBackground = std::make_unique<fheroes2::ImageRestorer>( display, _titleRoi.x, _titleRoi.y, _titleRoi.width, _titleRoi.height );
            _playersBackground = std::make_unique<fheroes2::ImageRestorer>( display, _playersRoi.x, _playersRoi.y, _playersRoi.width, _playersRoi.height );
            _helpBackground = std::make_unique<fheroes2::ImageRestorer>( display, _helpRoi.x, _helpRoi.y, _helpRoi.width, _helpRoi.height );
            _chatBackground = std::make_unique<fheroes2::ImageRestorer>( display, _chatRoi.x, _chatRoi.y, _chatRoi.width, _chatRoi.height );

            const fheroes2::Rect inputTextRoi{ _inputRoi.x + 6, _inputRoi.y + 5, _inputRoi.width - 12, _inputRoi.height - 8 };
            _input = std::make_unique<fheroes2::TextInputField>( inputTextRoi, false, false, display );

            _buttonSend = Multi::makeTextButton( { _inputRoi.x + _inputRoi.width + 8, _inputRoi.y, sendButtonWidth, buttonHeight }, gettext_noop( "lobby|SEND" ) );
            _buttonStart = Multi::makeTextButton( { area.x + margin, buttonsY, bottomButtonWidth, buttonHeight }, gettext_noop( "lobby|START" ) );
            _buttonLeave
                = Multi::makeTextButton( { area.x + area.width - margin - bottomButtonWidth, buttonsY, bottomButtonWidth, buttonHeight }, gettext_noop( "lobby|LEAVE" ) );

            if ( !_isHost ) {
                _buttonStart.hide();
            }

            for ( const fheroes2::ButtonSprite * button : { &_buttonStart, &_buttonSend, &_buttonLeave } ) {
                if ( button->isVisible() ) {
                    button->drawShadow( display );
                }
            }

            Game::AnimateResetDelay( Game::DelayType::CURSOR_BLINK_DELAY );
            _redraw();
        }

        void _drawPlayers( fheroes2::Display & display )
        {
            _colorLines.clear();
            if ( !_info ) {
                return;
            }

            int32_t y = _playersRoi.y + 5;
            const Multi::MachineId self = ownMachine();

            for ( const PlayerColor color : colorOrder ) {
                const auto bit = static_cast<uint8_t>( color );
                const bool isHuman = ( _info->humanColors & bit ) != 0;
                if ( !isHuman && ( _info->computerColors & bit ) == 0 ) {
                    continue;
                }

                const Multi::RoomPlayer * owner = nullptr;
                for ( const Multi::RoomPlayer & player : _info->players ) {
                    if ( ( player.colors & bit ) != 0 ) {
                        owner = &player;
                    }
                }

                std::string name;
                fheroes2::FontColor fontColor = fheroes2::FontColor::GRAY;
                if ( owner != nullptr ) {
                    name = getPlayerName( *owner );
                    fontColor = ( owner->machine == self ) ? fheroes2::FontColor::YELLOW : fheroes2::FontColor::WHITE;
                }
                else if ( isHuman ) {
                    name = _( "free (computer)" );
                }
                else {
                    name = _( "computer" );
                }

                std::string text = _( "%{color}: %{name}" );
                StringReplace( text, "%{color}", Color::String( color ) );
                StringReplace( text, "%{name}", name );

                fheroes2::Text line( text, { fheroes2::FontSize::NORMAL, fontColor } );
                line.fitToOneRow( _playersRoi.width - 12 );
                line.draw( _playersRoi.x + 6, y + 2, display );

                if ( isHuman ) {
                    _colorLines.emplace_back( fheroes2::Rect{ _playersRoi.x, y, _playersRoi.width, playerLineHeight }, color );
                }

                y += playerLineHeight;
            }
        }

        void _redraw()
        {
            _isChanged = false;

            fheroes2::Display & display = fheroes2::Display::instance();

            // Title and game.
            _titleBackground->restore();
            const fheroes2::Text title( _info ? Multi::fromUtf8( _info->gameName ) : std::string( _( "Network game" ) ), fheroes2::FontType::normalYellow() );
            title.draw( _titleRoi.x + ( _titleRoi.width - title.width() ) / 2, _titleRoi.y + 2, display );
            if ( _info ) {
                fheroes2::Text game( describeGame( _info->game ), fheroes2::FontType::normalWhite() );
                game.fitToOneRow( _titleRoi.width );
                game.draw( _titleRoi.x + ( _titleRoi.width - game.width() ) / 2, _titleRoi.y + 24, display );
            }

            // Players: colors and names.
            _playersBackground->restore();
            _drawPlayers( display );

            // How to join, or what the guest waits for.
            _helpBackground->restore();
            int32_t helpY = _helpRoi.y + 6;
            const int32_t helpWidth = _helpRoi.width - 12;
            if ( !_help.inviteCode.empty() ) {
                const fheroes2::Text codeTitle( _( "Invite code:" ), fheroes2::FontType::normalWhite() );
                codeTitle.draw( _helpRoi.x + ( _helpRoi.width - codeTitle.width() ) / 2, helpY, display );
                helpY += codeTitle.height() + 2;

                const fheroes2::Text code( _help.inviteCode, { fheroes2::FontSize::LARGE, fheroes2::FontColor::GOLDEN_GRADIENT } );
                code.draw( _helpRoi.x + ( _helpRoi.width - code.width() ) / 2, helpY, display );
                helpY += code.height() + 8;
            }
            // The long advice about the internet box comes with addresses: a smaller font keeps all of them in view.
            const std::string helpText = _isHost ? _help.text : _status;
            fheroes2::Text help( helpText, fheroes2::FontType::normalWhite() );
            if ( help.height( helpWidth ) > _helpRoi.y + _helpRoi.height - 4 - helpY ) {
                help = fheroes2::Text( helpText, fheroes2::FontType::smallWhite() );
            }
            help.drawInRoi( _helpRoi.x + 6, helpY, helpWidth, display, _helpRoi );

            // The chat, the newest lines at the bottom.
            _chatBackground->restore();
            const int32_t chatWidth = _chatRoi.width - 12;
            int32_t chatBottom = _chatRoi.y + _chatRoi.height - 4;
            if ( !_hasOthers() && _lines.empty() ) {
                const fheroes2::Text hint( _isHost ? _( "You can chat with the other players once they are here." ) : _( "Connecting to the host..." ),
                                           { fheroes2::FontSize::NORMAL, fheroes2::FontColor::GRAY } );
                hint.draw( _chatRoi.x + 6, _chatRoi.y + 6, chatWidth, display );
            }
            for ( auto line = _lines.rbegin(); line != _lines.rend(); ++line ) {
                fheroes2::MultiFontText text;
                if ( line->name.empty() ) {
                    text.add( { line->text, { fheroes2::FontSize::NORMAL, fheroes2::FontColor::GRAY } } );
                }
                else {
                    std::string name = _( "%{name}: " );
                    StringReplace( name, "%{name}", line->name );
                    text.add( { name, fheroes2::FontType::normalYellow() } );
                    text.add( { line->text, fheroes2::FontType::normalWhite() } );
                }

                const int32_t height = text.height( chatWidth );
                if ( chatBottom - height < _chatRoi.y + 2 ) {
                    break;
                }
                chatBottom -= height;
                text.draw( _chatRoi.x + 6, chatBottom, chatWidth, display );
                chatBottom -= 2;
            }

            // The line being typed.
            if ( _input ) {
                _input->draw( _typed, static_cast<int32_t>( _cursorPosition ) );
            }

            _updateButton( _buttonStart, _isHost && _isStartEnabled && !_isStarting );
            _updateButton( _buttonSend, _hasOthers() && !_isStarting );
            for ( const fheroes2::ButtonSprite * button : { &_buttonStart, &_buttonSend, &_buttonLeave } ) {
                if ( button->isVisible() ) {
                    button->draw();
                }
            }

            display.render( _window->totalArea() );
        }

        static void _updateButton( fheroes2::ButtonSprite & button, const bool isEnabled )
        {
            if ( isEnabled ) {
                button.enable();
            }
            else {
                button.disable();
            }
        }

        bool _isHost{ false };
        bool _isStartEnabled{ false };
        bool _isStarting{ false };
        std::optional<Multi::WaitingRoomInfo> _info;
        Multi::JoinHelp _help;
        std::string _status;
        std::deque<ChatLine> _lines;
        std::string _typed;
        size_t _cursorPosition{ 0 };
        bool _isChanged{ true };
        std::vector<std::pair<fheroes2::Rect, PlayerColor>> _colorLines;
        PlayerColor _clickedColor{ PlayerColor::NONE };

        std::unique_ptr<fheroes2::StandardWindow> _window;
        fheroes2::Rect _titleRoi;
        fheroes2::Rect _playersRoi;
        fheroes2::Rect _helpRoi;
        fheroes2::Rect _chatRoi;
        fheroes2::Rect _inputRoi;
        std::unique_ptr<fheroes2::ImageRestorer> _titleBackground;
        std::unique_ptr<fheroes2::ImageRestorer> _playersBackground;
        std::unique_ptr<fheroes2::ImageRestorer> _helpBackground;
        std::unique_ptr<fheroes2::ImageRestorer> _chatBackground;
        std::unique_ptr<fheroes2::TextInputField> _input;
        fheroes2::ButtonSprite _buttonStart;
        fheroes2::ButtonSprite _buttonSend;
        fheroes2::ButtonSprite _buttonLeave;
    };

    // What the host knows of the room: the players and the colors they take.
    class HostRoom
    {
    public:
        explicit HostRoom( const Multi::WaitingRoomInfo & info )
            : _info( info )
        {
            Multi::RoomPlayer host;
            host.nickname = Multi::loadNetworkSettings().nickname;
            host.colors = lowestColor( info.humanColors );

            _info.players = { host };
        }

        uint8_t freeColors() const
        {
            return static_cast<uint8_t>( _info.humanColors & ~_takenColors() );
        }

        size_t guestCount() const
        {
            return _info.players.size() - 1;
        }

        // The lowest number which no guest has.
        std::optional<Multi::MachineId> freeMachine() const
        {
            for ( Multi::MachineId machine = Multi::hostMachine + 1; machine <= Multi::maxMachineId; ++machine ) {
                if ( findPlayer( _info, machine ) == nullptr ) {
                    return machine;
                }
            }
            return {};
        }

        void addGuest( const Multi::MachineId machine, std::string nickname, const uint64_t machineKey, const uint8_t colors )
        {
            Multi::RoomPlayer player;
            player.machine = machine;
            player.nickname = std::move( nickname );
            player.colors = static_cast<uint8_t>( colors & freeColors() );

            const auto position
                = std::find_if( _info.players.begin(), _info.players.end(), [machine]( const Multi::RoomPlayer & other ) { return other.machine > machine; } );
            _info.players.insert( position, std::move( player ) );
            _machineKeys[machine] = machineKey;
        }

        void removeGuest( const Multi::MachineId machine )
        {
            _info.players.erase( std::remove_if( _info.players.begin(), _info.players.end(),
                                                 [machine]( const Multi::RoomPlayer & player ) { return player.machine == machine; } ),
                                 _info.players.end() );
            _machineKeys.erase( machine );
            _readyGuests.erase( machine );
        }

        uint8_t colorsOf( const Multi::MachineId machine ) const
        {
            const Multi::RoomPlayer * player = findPlayer( _info, machine );
            return player != nullptr ? player->colors : 0;
        }

        // A guest asks for these colors: it keeps those it has and gets the free ones, one at least.
        bool setGuestColors( const Multi::MachineId machine, const uint8_t wanted )
        {
            Multi::RoomPlayer * player = _findMutablePlayer( machine );
            if ( player == nullptr || machine == Multi::hostMachine ) {
                return false;
            }

            const auto colors = static_cast<uint8_t>( wanted & ( player->colors | freeColors() ) );
            if ( colors == 0 || colors == player->colors ) {
                return false;
            }

            player->colors = colors;
            return true;
        }

        // The host takes a free color, or gives back one of its own as long as it keeps one.
        bool toggleHostColor( const PlayerColor color )
        {
            Multi::RoomPlayer & host = _info.players.front();
            const auto bit = static_cast<uint8_t>( color );

            if ( ( host.colors & bit ) != 0 ) {
                if ( host.colors == bit ) {
                    return false;
                }
                host.colors = static_cast<uint8_t>( host.colors & ~bit );
                return true;
            }

            if ( ( freeColors() & bit ) == 0 ) {
                return false;
            }

            host.colors |= bit;
            return true;
        }

        // Every guest has a color.
        bool canStart() const
        {
            return guestCount() > 0 && std::all_of( _info.players.begin(), _info.players.end(), []( const Multi::RoomPlayer & player ) { return player.colors != 0; } );
        }

        Multi::WaitingRoomInfo infoFor( const Multi::MachineId receiver ) const
        {
            Multi::WaitingRoomInfo info = _info;
            info.receiver = receiver;
            info.game.guestHumanPlayers = static_cast<uint8_t>( Color::Count( freeColors() ) );
            return info;
        }

        Multi::MachineSet guests() const
        {
            return getOthersInRoom( _info, Multi::hostMachine );
        }

        std::string nameOf( const Multi::MachineId machine ) const
        {
            const Multi::RoomPlayer * player = findPlayer( _info, machine );
            return player != nullptr ? getPlayerName( *player ) : std::string();
        }

        void setReady( const Multi::MachineId machine )
        {
            if ( findPlayer( _info, machine ) != nullptr ) {
                _readyGuests.insert( machine );
            }
        }

        void clearReady()
        {
            _readyGuests.clear();
        }

        bool areGuestsReady() const
        {
            return guestCount() > 0 && _readyGuests.size() == guestCount();
        }

        Multi::RoomPlayers result() const
        {
            Multi::RoomPlayers players;
            players.hostColors = _info.players.front().colors;

            for ( const Multi::RoomPlayer & player : _info.players ) {
                if ( player.machine == Multi::hostMachine ) {
                    continue;
                }

                const auto key = _machineKeys.find( player.machine );
                players.guests.push_back( { player.machine, player.nickname, key != _machineKeys.end() ? key->second : 0, player.colors } );
            }

            return players;
        }

        // Sends the room to each guest.
        void send( Multi::NetworkSession & session ) const
        {
            for ( const Multi::RoomPlayer & player : _info.players ) {
                if ( player.machine != Multi::hostMachine ) {
                    session.send( player.machine, Multi::makeWaitingRoom( infoFor( player.machine ) ) );
                }
            }
        }

    private:
        uint8_t _takenColors() const
        {
            uint8_t colors = 0;
            for ( const Multi::RoomPlayer & player : _info.players ) {
                colors |= player.colors;
            }
            return colors;
        }

        Multi::RoomPlayer * _findMutablePlayer( const Multi::MachineId machine )
        {
            const auto iter
                = std::find_if( _info.players.begin(), _info.players.end(), [machine]( const Multi::RoomPlayer & player ) { return player.machine == machine; } );
            return iter != _info.players.end() ? &*iter : nullptr;
        }

        Multi::WaitingRoomInfo _info;
        std::map<Multi::MachineId, uint64_t> _machineKeys;
        std::set<Multi::MachineId> _readyGuests;
    };

    // Sends the line typed by the player to the others, and shows it.
    void sendTypedLine( RoomScreen & screen, Multi::NetworkSession & session, const Multi::MachineSet others )
    {
        const std::string typed = Multi::trimText( screen.takeTypedText() );
        const std::string utf8 = Multi::toUtf8( typed );
        if ( Multi::isValidChatText( utf8 ) ) {
            session.sendToMachines( others, Multi::makeChat( utf8 ) );
            screen.addLine( screen.nameOf( screen.ownMachine() ), typed );
        }
    }

    std::string describeArrival( const std::string & name )
    {
        std::string text = _( "%{name} has joined the game." );
        StringReplace( text, "%{name}", name );
        return text;
    }

    std::string describeLeave( const std::string & name )
    {
        std::string text = _( "%{name} has left." );
        StringReplace( text, "%{name}", name );
        return text;
    }

    // The colors of a guest which comes in: those it claims, from before a restart of the host, otherwise the first free one.
    uint8_t giveColors( const uint8_t claimed, const uint8_t free )
    {
        const auto colors = static_cast<uint8_t>( claimed & free );
        return colors != 0 ? colors : lowestColor( free );
    }

    using ColorChoice = std::function<uint8_t( const uint8_t claimed, const uint8_t free )>;

    // Host. Takes in the computers which connected. Returns true if the room changed.
    bool takeNewcomers( Multi::NetworkSession & session, HostRoom & room, RoomScreen * screen, const bool isStarting, const ColorChoice & chooseColors )
    {
        bool isChanged = false;

        while ( const std::optional<Multi::NetworkSession::Newcomer> newcomer = session.takeNewcomer() ) {
            std::string refusal;
            const std::optional<Multi::HandshakeInfo> handshake = Multi::readNewcomerHandshake( session, newcomer->machine, newcomer->firstMessage, refusal );
            if ( !handshake ) {
                // Another version, or a connection which went wrong: the game still waits for players.
                ERROR_LOG( "[MULTI] " << refusal )
                if ( screen != nullptr ) {
                    std::string note = _( "A player could not come in: %{reason}." );
                    StringReplace( note, "%{reason}", Multi::translateError( refusal ) );
                    screen->addNote( note );
                }
                continue;
            }

            const std::optional<Multi::MachineId> machine = room.freeMachine();
            if ( isStarting || !machine || room.freeColors() == 0 ) {
                VERBOSE_LOG( "[MULTI] A computer was refused: " << ( isStarting ? "the game starts" : "no free color" ) )
                Multi::refuseNewcomer( session, newcomer->machine, isStarting ? Multi::HandshakeResult::GAME_STARTED : Multi::HandshakeResult::GAME_FULL );
                continue;
            }

            Multi::acceptNewcomer( session, newcomer->machine, *machine );
            room.addGuest( *machine, handshake->nickname, handshake->machineKey, chooseColors( handshake->claimedColors, room.freeColors() ) );
            isChanged = true;

            VERBOSE_LOG( "[MULTI] A player is in the waiting room, computer " << static_cast<int>( *machine ) )

            if ( screen != nullptr ) {
                screen->addNote( describeArrival( room.nameOf( *machine ) ) );
                fheroes2::engine().requestAttention();
            }
        }

        return isChanged;
    }
}

Multi::WaitingRoomEnd Multi::runHostWaitingRoom( const WaitingRoomInfo & info, const HostRoomHooks & hooks, std::unique_ptr<NetworkSession> & session,
                                                 RoomPlayers & players, std::string & error )
{
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    session = hooks.openSession( error );
    if ( !session ) {
        return WaitingRoomEnd::FAILED;
    }

    HostRoom room( info );
    RoomScreen screen( true );
    screen.setInfo( room.infoFor( hostMachine ) );
    hooks.onPlayersChange( Color::Count( room.freeColors() ) );

    JoinHelp help;

    // Each guest answers the start once it has read every line sent before: none comes into the game.
    bool isStarting = false;

    // A game which stopped on another computer never comes back.
    std::map<MachineId, std::chrono::steady_clock::time_point> lostSince;

    while ( true ) {
        session->update();

        if ( !hooks.update( *session, help, error ) ) {
            session->close();
            return WaitingRoomEnd::FAILED;
        }
        screen.setJoinHelp( help );

        bool isChanged = takeNewcomers( *session, room, &screen, isStarting, giveColors );

        while ( std::optional<Message> message = session->receive() ) {
            switch ( message->type ) {
            case MessageType::PLAYER_INFO:
                if ( const std::optional<uint8_t> wanted = readPlayerInfo( *message ); wanted && !isStarting && room.setGuestColors( message->sender, *wanted ) ) {
                    VERBOSE_LOG( "[MULTI] Computer " << static_cast<int>( message->sender ) << " now plays " << describeColors( room.colorsOf( message->sender ) ) )
                    isChanged = true;
                }
                break;
            case MessageType::CHAT:
                if ( const std::optional<std::string> text = readChat( *message ); text ) {
                    screen.addLine( room.nameOf( message->sender ), fromUtf8( *text ) );
                    fheroes2::engine().requestAttention();
                }
                break;
            case MessageType::GAME_START:
                if ( isStarting ) {
                    room.setReady( message->sender );
                }
                break;
            default:
                VERBOSE_LOG( "[MULTI] Message " << static_cast<uint32_t>( message->type ) << " ignored in the waiting room" )
                break;
            }
        }

        for ( const MachineId machine : session->guests() ) {
            const NetworkSession::State state = session->state( machine );

            if ( state == NetworkSession::State::RECONNECTING ) {
                if ( lostSince.emplace( machine, std::chrono::steady_clock::now() ).second ) {
                    std::string note = _( "The connection with %{name} is cut." );
                    StringReplace( note, "%{name}", room.nameOf( machine ) );
                    screen.addNote( note );
                }
            }
            else if ( lostSince.erase( machine ) > 0 && state == NetworkSession::State::CONNECTED ) {
                screen.addNote( _( "The connection is back." ) );
            }

            const auto lost = lostSince.find( machine );
            const bool isLostForGood = lost != lostSince.end() && std::chrono::steady_clock::now() - lost->second > lostGuestTimeout;

            if ( state == NetworkSession::State::CLOSED || isLostForGood ) {
                VERBOSE_LOG( "[MULTI] Computer " << static_cast<int>( machine ) << " left the waiting room: " << session->error( machine ) )
                screen.addNote( describeLeave( room.nameOf( machine ) ) );
                session->closeLink( machine );
                room.removeGuest( machine );
                lostSince.erase( machine );
                isChanged = true;
            }
        }

        if ( isChanged ) {
            room.send( *session );
            screen.setInfo( room.infoFor( hostMachine ) );
            hooks.onPlayersChange( Color::Count( room.freeColors() ) );
        }

        if ( isStarting && room.guestCount() == 0 ) {
            isStarting = false;
            screen.setStarting( false );
        }
        else if ( isStarting && room.areGuestsReady() ) {
            VERBOSE_LOG( "[MULTI] Every guest is ready, the game starts" )
            players = room.result();
            return WaitingRoomEnd::STARTED;
        }

        screen.setStartEnabled( room.canStart() );

        switch ( screen.handleEvents() ) {
        case RoomScreen::Action::SEND:
            sendTypedLine( screen, *session, room.guests() );
            break;
        case RoomScreen::Action::START:
            if ( room.canStart() ) {
                isStarting = true;
                room.clearReady();
                session->sendToMachines( room.guests(), makeEmptyLobbyMessage( MessageType::GAME_START ) );
                screen.setStarting( true );
                screen.addNote( _( "The game starts..." ) );
            }
            break;
        case RoomScreen::Action::COLOR:
            if ( room.toggleHostColor( screen.clickedColor() ) ) {
                VERBOSE_LOG( "[MULTI] The host now plays " << describeColors( room.colorsOf( hostMachine ) ) )
                room.send( *session );
                screen.setInfo( room.infoFor( hostMachine ) );
                hooks.onPlayersChange( Color::Count( room.freeColors() ) );
            }
            break;
        case RoomScreen::Action::LEAVE:
            session->close();
            return WaitingRoomEnd::LEFT;
        case RoomScreen::Action::QUIT_GAME:
            session->close();
            return WaitingRoomEnd::QUIT_GAME;
        default:
            break;
        }
    }
}

Multi::WaitingRoomEnd Multi::runGuestWaitingRoom( NetworkSession & session, std::string & error )
{
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    RoomScreen screen( false );
    screen.setStatus( _( "The game starts when the host is ready." ) );

    while ( true ) {
        session.update();

        while ( std::optional<Message> message = session.receive() ) {
            switch ( message->type ) {
            case MessageType::WAITING_ROOM:
                if ( const std::optional<WaitingRoomInfo> info = readWaitingRoom( *message ); info ) {
                    if ( !screen.info() ) {
                        screen.addNote( _( "You have joined the game." ) );
                    }
                    screen.setInfo( *info );
                }
                break;
            case MessageType::CHAT:
                if ( const std::optional<std::string> text = readChat( *message ); text ) {
                    screen.addLine( screen.nameOf( message->sender ), fromUtf8( *text ) );
                    fheroes2::engine().requestAttention();
                }
                break;
            case MessageType::GAME_START:
                session.send( hostMachine, makeEmptyLobbyMessage( MessageType::GAME_START ) );
                VERBOSE_LOG( "[MULTI] The host starts the game" )
                return WaitingRoomEnd::STARTED;
            default:
                // A game already going on, joined again after a restart of this computer.
                session.putBack( std::move( *message ) );
                VERBOSE_LOG( "[MULTI] The host sends a game already going on" )
                return WaitingRoomEnd::STARTED;
            }
        }

        if ( session.state() == NetworkSession::State::CLOSED ) {
            error = session.isClosedByOther() ? gettext_noop( "the host closed the game" ) : session.error();
            return WaitingRoomEnd::FAILED;
        }

        if ( session.state() == NetworkSession::State::RECONNECTING ) {
            screen.setStatus( _( "The connection with the host is cut. The game connects again..." ) );
        }
        else {
            screen.setStatus( _( "The game starts when the host is ready." ) );
        }

        switch ( screen.handleEvents() ) {
        case RoomScreen::Action::SEND:
            if ( screen.info() ) {
                sendTypedLine( screen, session, getOthersInRoom( *screen.info(), screen.ownMachine() ) );
            }
            break;
        case RoomScreen::Action::COLOR:
            // A player keeps one color at least. The host answers with the room as it now is.
            if ( const auto wanted = static_cast<uint8_t>( screen.ownColors() ^ static_cast<uint8_t>( screen.clickedColor() ) ); wanted != 0 ) {
                session.send( hostMachine, makePlayerInfo( wanted ) );
            }
            break;
        case RoomScreen::Action::LEAVE:
            session.close();
            return WaitingRoomEnd::LEFT;
        case RoomScreen::Action::QUIT_GAME:
            session.close();
            return WaitingRoomEnd::QUIT_GAME;
        default:
            break;
        }
    }
}

Multi::WaitingRoomInfo Multi::makeWaitingRoomInfo( const std::string & gameName )
{
    WaitingRoomInfo info;
    info.gameName = gameName;

    // Self-tests have the human players played by the AI: the human colors come from the list kept at the start.
    const PlayerColorsSet humanColors = Players::HumanColors();
    for ( const Player * player : Settings::Get().GetPlayers().getVector() ) {
        if ( !player->isPlay() ) {
            continue;
        }

        const auto color = static_cast<uint8_t>( player->GetColor() );
        if ( ( humanColors & color ) != 0 ) {
            info.humanColors |= color;
        }
        else {
            info.computerColors |= color;
        }
    }

    info.game = makeRoomGameOfCurrentGame( static_cast<uint8_t>( Color::Count( info.humanColors ) - 1 ) );

    return info;
}

bool Multi::startGameForTest( NetworkSession & session, const WaitingRoomInfo & info, const std::vector<uint8_t> & guestColors, const std::function<bool()> & keepWaiting,
                              RoomPlayers & players )
{
    HostRoom room( info );
    bool isStarting = false;

    // A guest gets the colors it had in this game before the host restarted, otherwise the next ones of the test.
    const ColorChoice chooseColors = [&guestColors]( const uint8_t claimed, const uint8_t free ) {
        if ( ( claimed & free ) != 0 ) {
            return static_cast<uint8_t>( claimed & free );
        }

        const auto colors
            = std::find_if( guestColors.begin(), guestColors.end(), [free]( const uint8_t testColors ) { return testColors != 0 && ( testColors & ~free ) == 0; } );
        return colors != guestColors.end() ? *colors : lowestColor( free );
    };

    while ( keepWaiting() && session.state() != NetworkSession::State::CLOSED ) {
        session.update();

        if ( takeNewcomers( session, room, nullptr, isStarting, chooseColors ) ) {
            room.send( session );
        }

        while ( std::optional<Message> message = session.receive() ) {
            if ( message->type == MessageType::GAME_START && isStarting ) {
                room.setReady( message->sender );
            }
        }

        if ( !isStarting && room.guestCount() == guestColors.size() ) {
            isStarting = true;
            session.sendToMachines( room.guests(), makeEmptyLobbyMessage( MessageType::GAME_START ) );
        }

        if ( isStarting && room.areGuestsReady() ) {
            players = room.result();
            return true;
        }
    }

    return false;
}

bool Multi::waitForStartForTest( NetworkSession & session, const bool isPickingColors )
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes( 2 );

    uint8_t firstColors = 0;
    bool isAsked = false;
    bool isGivenBack = false;

    while ( std::chrono::steady_clock::now() < deadline && session.state() != NetworkSession::State::CLOSED ) {
        session.update();

        std::optional<Message> message = session.receive();
        if ( !message ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            continue;
        }

        switch ( message->type ) {
        case MessageType::WAITING_ROOM: {
            const std::optional<WaitingRoomInfo> info = readWaitingRoom( *message );
            const RoomPlayer * self = info ? findPlayer( *info, info->receiver ) : nullptr;
            if ( self == nullptr || !isPickingColors || isGivenBack ) {
                break;
            }

            const uint8_t colors = self->colors;
            uint8_t takenColors = 0;
            for ( const RoomPlayer & player : info->players ) {
                takenColors |= player.colors;
            }

            if ( !isAsked ) {
                if ( const auto freeColors = static_cast<uint8_t>( info->humanColors & ~takenColors ); freeColors != 0 ) {
                    firstColors = colors;
                    isAsked = true;
                    session.send( hostMachine, makePlayerInfo( static_cast<uint8_t>( colors | freeColors ) ) );
                    VERBOSE_LOG( "[MULTI] Self-test: every free color asked for" )
                }
            }
            else if ( colors != firstColors ) {
                isGivenBack = true;
                session.send( hostMachine, makePlayerInfo( firstColors ) );
                VERBOSE_LOG( "[MULTI] Self-test: colors given back" )
            }
            break;
        }
        case MessageType::CHAT:
            break;
        case MessageType::GAME_START:
            session.send( hostMachine, makeEmptyLobbyMessage( MessageType::GAME_START ) );
            return true;
        default:
            // A game already going on.
            session.putBack( std::move( *message ) );
            return true;
        }
    }

    return false;
}

void Multi::showWaitingRoomForTest( const WaitingRoomInfo & info, const JoinHelp & help, const bool hasGuests )
{
    RoomScreen screen( true );

    WaitingRoomInfo shown = info;
    RoomPlayer host;
    host.nickname = loadNetworkSettings().nickname;
    host.colors = lowestColor( info.humanColors );
    shown.players = { host };

    if ( hasGuests ) {
        uint8_t freeColors = static_cast<uint8_t>( info.humanColors & ~host.colors );
        MachineId machine = hostMachine;

        for ( const char * name : { "Marc", "Sophie" } ) {
            if ( freeColors == 0 ) {
                break;
            }

            RoomPlayer guest;
            guest.machine = ++machine;
            guest.nickname = name;
            guest.colors = lowestColor( freeColors );
            freeColors = static_cast<uint8_t>( freeColors & ~guest.colors );
            shown.players.push_back( guest );
        }
    }

    screen.setInfo( shown );
    screen.setJoinHelp( help );
    screen.setStartEnabled( hasGuests );

    if ( hasGuests ) {
        screen.addNote( describeArrival( "Marc" ) );
        screen.addLine( "Marc", "Hello! Ready when you are." );
        screen.addLine( screen.nameOf( hostMachine ), "Let me choose my castle first." );
    }

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        screen.redrawIfChanged();
        if ( le.isKeyPressed( fheroes2::Key::KEY_ENTER ) || le.isKeyPressed( fheroes2::Key::KEY_ESCAPE ) ) {
            break;
        }
    }
}

bool Multi::handleLateWaitingRoomMessage( NetworkSession & session, const Message & message )
{
    switch ( message.type ) {
    case MessageType::WAITING_ROOM:
    case MessageType::PLAYER_INFO:
    case MessageType::CHAT:
        VERBOSE_LOG( "[MULTI] Late message of the waiting room " << static_cast<uint32_t>( message.type ) << " ignored" )
        return true;
    case MessageType::GAME_START:
        // A host restarted from a save file starts the game again: this game is already in it.
        if ( !session.isHostSide() ) {
            session.send( hostMachine, makeEmptyLobbyMessage( MessageType::GAME_START ) );
        }
        return true;
    default:
        return false;
    }
}
