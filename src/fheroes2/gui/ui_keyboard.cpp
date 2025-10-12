/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2025                                             *
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

#include "ui_keyboard.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "game_language.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "pal.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    const int32_t buttonOffset{ 8 };
    const int32_t defaultButtonHeight{ 25 };
    const fheroes2::Size defaultSpecialButtonSize{ 54, defaultButtonHeight };
    const fheroes2::Size spacebarButtonSize{ 175, defaultButtonHeight };
    const int32_t defaultWindowWidth{ 520 };
    const int32_t numpadWindowWidth{ 320 };
    const int32_t defaultWindowHeight{ 250 };
    const int32_t defaultLetterRows{ 3 };
    const fheroes2::Point buttonShadowOffset{ -5, 5 };
    const fheroes2::Point offsetFromWindowBorders{ 25, 50 };
    const fheroes2::Size inputAreaSize{ 268, 21 };
    const int32_t inputAreaBorders{ 2 };
    const int32_t inputAreaOffsetFromWindowTop{ 20 };

    fheroes2::SupportedLanguage lastSelectedLanguage{ fheroes2::SupportedLanguage::English };

    bool isSupportedForLanguageSwitching( const fheroes2::SupportedLanguage language )
    {
        switch ( language ) {
        case fheroes2::SupportedLanguage::English:
            // English is a default language so it is not considered as an extra language.
            return false;
        case fheroes2::SupportedLanguage::Belarusian:
        case fheroes2::SupportedLanguage::Czech:
        case fheroes2::SupportedLanguage::Danish:
        case fheroes2::SupportedLanguage::French:
        case fheroes2::SupportedLanguage::German:
        case fheroes2::SupportedLanguage::Polish:
        case fheroes2::SupportedLanguage::Russian:
        case fheroes2::SupportedLanguage::Slovak:
        case fheroes2::SupportedLanguage::Ukrainian:
            return true;
        default:
            break;
        }

        return false;
    }

    enum class DialogAction : int
    {
        DoNothing,
        UpperCase,
        LowerCase,
        AlphaNumeric,
        ChangeLanguage,
        Close
    };

    enum class LayoutType : int
    {
        LowerCase,
        UpperCase,
        AlphaNumeric,
        SignedNumeric,
        UnsignedNumeric
    };

    enum class CursorPosition
    {
        PrevChar,
        NextChar,
        BegOfText,
        EndOfText
    };

    class KeyboardRenderer
    {
    public:
        KeyboardRenderer( fheroes2::Display & output, std::string & info, const size_t lengthLimit, const bool evilInterface )
            : _output( output )
            , _info( info )
            , _lengthLimit( lengthLimit )
            , _cursorPosition( info.size() )
            , _isEvilInterface( evilInterface )
        {
            // Do nothing.
        }

        fheroes2::Rect getWindowRoi() const
        {
            if ( !_window ) {
                // You are calling this method for an empty renderer!
                assert( 0 );
                return {};
            }

            return _window->activeArea();
        }

        const fheroes2::Rect & getTextRoi() const
        {
            return _textInputArea;
        }

        bool isEvilInterface() const
        {
            return _isEvilInterface;
        }

        // Returns true if keyboard dialog resize was made.
        bool resize( const fheroes2::Size & size )
        {
            if ( _window && size.width == _window->activeArea().width && size.height == _window->activeArea().height ) {
                // This is the same window size. Nothing to do.
                return false;
            }

            assert( size.width > 0 && size.height > 0 );

            const fheroes2::Point defaultOffset{ ( _output.width() - size.width ) / 2, ( _output.height() - defaultWindowHeight ) / 2 };
            const fheroes2::Point offset{ defaultOffset.x, defaultOffset.y - ( size.height - defaultWindowHeight ) };

            // It is important to destroy the previous window to avoid rendering issues.
            _textUI.reset();
            _window.reset();

            _window = std::make_unique<fheroes2::StandardWindow>( offset.x, offset.y, size.width, size.height, true, _output );

            _window->render();

            const fheroes2::Rect & windowRoi = _window->activeArea();
            _textInputArea.x = windowRoi.x + ( windowRoi.width - inputAreaSize.width ) / 2;
            _textInputArea.y = windowRoi.y + inputAreaOffsetFromWindowTop;

            // Draw the text input background.
            const fheroes2::Sprite & initialWindow = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
            fheroes2::Copy( initialWindow, 40, 286, _output, _textInputArea );

            if ( _isEvilInterface ) {
                fheroes2::ApplyPalette( _output, _textInputArea.x, _textInputArea.y, _output, _textInputArea.x, _textInputArea.y, _textInputArea.width,
                                        _textInputArea.height, PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );
            }

            _textUI
                = std::make_unique<fheroes2::TextInputField>( fheroes2::Rect{ _textInputArea.x + inputAreaBorders, _textInputArea.y + inputAreaBorders,
                                                                              inputAreaSize.width - 2 * inputAreaBorders, inputAreaSize.height - 2 * inputAreaBorders },
                                                              false, false, _output );
            _textUI->draw( _info, static_cast<int32_t>( _cursorPosition ) );

            return true;
        }

        void insertCharacter( const char character )
        {
            if ( _info.size() >= _lengthLimit ) {
                // Do not add more characters as the string is already long enough.
                return;
            }

            _info.insert( _cursorPosition, 1, character );

            ++_cursorPosition;

            _renderInputArea();
        }

        void removeCharacter()
        {
            if ( _info.empty() || _cursorPosition == 0 ) {
                return;
            }

            if ( _cursorPosition >= _info.size() ) {
                _info.pop_back();
            }
            else {
                _info.erase( _cursorPosition - 1, 1 );
            }

            --_cursorPosition;

            _renderInputArea();
        }

        void swapSign()
        {
            if ( _info.empty() ) {
                return;
            }

            if ( _info.front() == '-' ) {
                _info = _info.substr( 1 );

                if ( _cursorPosition > 0 ) {
                    --_cursorPosition;
                }

                _renderInputArea();

                return;
            }

            _info.insert( _info.begin(), '-' );
            ++_cursorPosition;

            _renderInputArea();
        }

        void eventProcessing()
        {
            if ( _textUI && _textUI->eventProcessing() ) {
                _output.render( _textUI->getCursorArea() );
            }
        }

        void setCursorPosition( const fheroes2::Point & clickPosition )
        {
            if ( _textUI ) {
                const size_t newPos = _textUI->getCursorInTextPosition( clickPosition );
                if ( _cursorPosition != newPos ) {
                    _cursorPosition = newPos;
                    _renderInputArea();
                }
            }
        }

        void setCursorPosition( const CursorPosition pos )
        {
            switch ( pos ) {
            case CursorPosition::PrevChar:
                if ( _cursorPosition == 0 ) {
                    return;
                }

                --_cursorPosition;

                break;
            case CursorPosition::NextChar:
                assert( _cursorPosition <= _info.size() );

                if ( _cursorPosition == _info.size() ) {
                    return;
                }

                ++_cursorPosition;

                break;
            case CursorPosition::BegOfText:
                _cursorPosition = 0;

                break;
            case CursorPosition::EndOfText:
                _cursorPosition = _info.size();

                break;
            default:
                assert( 0 );

                return;
            }

            _renderInputArea();
        }

    private:
        fheroes2::Display & _output;
        std::string & _info;
        size_t _lengthLimit{ 255 };
        std::unique_ptr<fheroes2::StandardWindow> _window;
        std::unique_ptr<fheroes2::TextInputField> _textUI;
        size_t _cursorPosition{ 0 };
        fheroes2::Rect _textInputArea{ 0, 0, inputAreaSize.width, inputAreaSize.height };
        const bool _isEvilInterface{ false };

        void _renderInputArea()
        {
            if ( _textUI ) {
                _textUI->draw( _info, static_cast<int32_t>( _cursorPosition ) );
                _output.render( _textUI->getOverallArea() );
            }
        }
    };

    struct KeyboardButton
    {
        KeyboardButton() = default;

        KeyboardButton( std::string input, const fheroes2::Key kbKey, const fheroes2::Size & buttonSize, const bool isEvilInterface,
                        std::function<DialogAction( KeyboardRenderer & )> actionEvent )
            : text( std::move( input ) )
            , key( kbKey )
            , action( std::move( actionEvent ) )
        {
            fheroes2::Sprite released;
            fheroes2::Sprite pressed;
            makeButtonSprites( released, pressed, text, buttonSize, isEvilInterface, isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );
            button.setSprite( released, pressed );

            // Make Image with shadow for button to Blit it during render.
            buttonShadow.resize( released.width() + std::abs( buttonShadowOffset.x ), released.height() + std::abs( buttonShadowOffset.y ) );
            buttonShadow.reset();
            fheroes2::addGradientShadow( released, buttonShadow, { -std::min( 0, buttonShadowOffset.x ), -std::min( 0, buttonShadowOffset.y ) }, buttonShadowOffset );
        }

        KeyboardButton( std::string input, const fheroes2::Size & buttonSize, const bool isEvilInterface, std::function<DialogAction( KeyboardRenderer & )> actionEvent )
            : KeyboardButton( std::move( input ), fheroes2::Key::NONE, buttonSize, isEvilInterface, std::move( actionEvent ) )
        {
            // Do nothing.
        }

        KeyboardButton( const KeyboardButton & ) = delete;

        KeyboardButton( KeyboardButton && ) noexcept = default;

        ~KeyboardButton() = default;

        KeyboardButton & operator=( const KeyboardButton & ) = delete;

        KeyboardButton & operator=( KeyboardButton && ) = default;

        std::string text;

        fheroes2::Key key{ fheroes2::Key::NONE };

        std::function<DialogAction( KeyboardRenderer & )> action;

        fheroes2::ButtonSprite button;
        fheroes2::Image buttonShadow;

        // This is used only for buttons which should have pressed state for some layouts.
        bool isInvertedRenderingLogic{ false };
    };

    std::vector<std::string> getAlphaNumericCharacterLayout( const fheroes2::SupportedLanguage language )
    {
        // Numeric layout can be used for special letters as well.
        switch ( language ) {
        case fheroes2::SupportedLanguage::Belarusian:
        case fheroes2::SupportedLanguage::Czech:
        case fheroes2::SupportedLanguage::Danish:
        case fheroes2::SupportedLanguage::English:
        case fheroes2::SupportedLanguage::French:
        case fheroes2::SupportedLanguage::German:
        case fheroes2::SupportedLanguage::Polish:
        case fheroes2::SupportedLanguage::Russian:
        case fheroes2::SupportedLanguage::Slovak:
        case fheroes2::SupportedLanguage::Ukrainian:
            return { "1234567890", "-:;()_+=", "[].,!'?" };
        default:
            // Did you add a new layout type? Add the logic above!
            assert( 0 );
            break;
        }

        return {};
    }

    std::vector<std::string> getNumericCharacterLayout()
    {
        return { "123", "456", "789" };
    }

    std::vector<std::string> getCapitalCharacterLayout( const fheroes2::SupportedLanguage language )
    {
        switch ( language ) {
        case fheroes2::SupportedLanguage::Belarusian:
            return { "\xC9\xD6\xD3\xCA\xC5\xCD\xC3\xD8\xA1\xC7\xD5\x92", "\xD4\xDB\xC2\xC0\xCF\xD0\xCE\xCB\xC4\xC6\xDD", "\xDF\xD7\xD1\xCC\xB2\xD2\xDC\xC1\xDE\xA8" };
        case fheroes2::SupportedLanguage::Czech:
            return { "\xCC\x8A\xC8\xD8\x8E\xDD\xC1\xCD\xC9", "QWERTZUIOP\xDA", "ASDFGHJKL\xD9", "YXCVBNM" };
        case fheroes2::SupportedLanguage::Danish:
            return { "QWERTYUIOP\xC5", "ASDFGHJKL\xC6\xD8", "ZXCVBNM" };
        case fheroes2::SupportedLanguage::English:
            return { "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };
        case fheroes2::SupportedLanguage::French:
            return { "\xC9\xC8\xC7\xC0\xC2\xCA\xCB\xCD\xCE\xCF\xDB\xDC", "AZERTYUIOP\xD4", "QSDFGHJKLM\xD9", "WXCVBN" };
        case fheroes2::SupportedLanguage::German:
            return { "QWERTZUIOP\xDC", "ASDFGHJKL\xD6\xC4", "YXCVBNM\xDF" };
        case fheroes2::SupportedLanguage::Polish:
            return { "\x8C\x8F\xA3\xA5\xAF\xC6\xCA\xD1\xD3", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };
        case fheroes2::SupportedLanguage::Russian:
            return { "\xC9\xD6\xD3\xCA\xC5\xCD\xC3\xD8\xD9\xC7\xD5\xDA", "\xD4\xDB\xC2\xC0\xCF\xD0\xCE\xCB\xC4\xC6\xDD", "\xDF\xD7\xD1\xCC\xC8\xD2\xDC\xC1\xDE\xA8" };
        case fheroes2::SupportedLanguage::Slovak:
            return { "\xCF\xBC\x8A\xC8\x8D\x8E\xDD\xC1\xCD\xC9\xD3", "QWERTZUIOP\xDA", "ASDFGHJKL\xD4\xD2", "\xC4YXCVBNM\xC5\xC0" };
        case fheroes2::SupportedLanguage::Ukrainian:
            return { "\xC9\xD6\xD3\xCA\xC5\xCD\xC3\xD8\xD9\xC7\xD5\xAF", "\xD4\xB2\xC2\xC0\xCF\xD0\xCE\xCB\xC4\xC6\xAA", "\xDF\xD7\xD1\xCC\xC8\xD2\xDC\xC1\xDE\xA5" };
        default:
            // Did you add a new layout type? Add the logic above!
            assert( 0 );
            break;
        }

        return {};
    }

    std::vector<std::string> getNonCapitalCharacterLayout( const fheroes2::SupportedLanguage language )
    {
        switch ( language ) {
        case fheroes2::SupportedLanguage::Belarusian:
            return { "\xE9\xF6\xF3\xEA\xE5\xED\xE3\xF8\xA2\xE7\xF5\x92", "\xF4\xFB\xE2\xE0\xEF\xF0\xEE\xEB\xE4\xE6\xFD", "\xFF\xF7\xF1\xEC\xB3\xF2\xFC\xE1\xFE\xB8" };
        case fheroes2::SupportedLanguage::Czech:
            return { "\xEC\x9A\xE8\xF8\xBE\xFD\xE1\xED\xE9", "qwertzuiop\xFA", "asdfghjkl\xF9", "yxcvbnm" };
        case fheroes2::SupportedLanguage::Danish:
            return { "qwertyuiop\xE5", "asdfghjkl\xE6\xF8", "zxcvbnm" };
        case fheroes2::SupportedLanguage::English:
            return { "qwertyuiop", "asdfghjkl", "zxcvbnm" };
        case fheroes2::SupportedLanguage::French:
            return { "\xE9\xE8\xE7\xE0\xE2\xEA\xEB\xED\xEE\xEF\xFB\xFC", "azertyuiop\xF4", "qsdfghjklm\xF9", "wxcvbn" };
        case fheroes2::SupportedLanguage::German:
            return { "qwertzuiop\xFC", "asdfghjkl\xF6\xE4", "yxcvbnm\xDF" };
        case fheroes2::SupportedLanguage::Polish:
            return { "\x9C\x9F\xB3\xB9\xBF\xE6\xEA\xF1\xF3", "qwertyuiop", "asdfghjkl", "zxcvbnm" };
        case fheroes2::SupportedLanguage::Russian:
            return { "\xE9\xF6\xF3\xEA\xE5\xED\xE3\xF8\xF9\xE7\xF5\xFA", "\xF4\xFB\xE2\xE0\xEF\xF0\xEE\xEB\xE4\xE6\xFD", "\xFF\xF7\xF1\xEC\xE8\xF2\xFC\xE1\xFE\xB8" };
        case fheroes2::SupportedLanguage::Slovak:
            return { "\xEF\xBE\x9A\xE8\x9D\x9E\xFD\xE1\xED\xE9\xF3", "qwertzuiop\xFA", "asdfghjkl\xF4\xF2", "\xE4yxcvbnm\xE5\xE0" };
        case fheroes2::SupportedLanguage::Ukrainian:
            return { "\xE9\xF6\xF3\xEA\xE5\xED\xE3\xF8\xF9\xE7\xF5\xBF", "\xF4\xB3\xE2\xE0\xEF\xF0\xEE\xEB\xE4\xE6\xBA", "\xFF\xF7\xF1\xEC\xE8\xF2\xFC\xE1\xFE\xB4" };
        default:
            // Did you add a new layout type? Add the logic above!
            assert( 0 );
            break;
        }

        return {};
    }

    void getCharacterLayout( const LayoutType layoutType, const fheroes2::SupportedLanguage language, std::vector<std::string> & buttonLetters,
                             std::vector<std::string> & returnLetters )
    {
        switch ( layoutType ) {
        case LayoutType::LowerCase:
            buttonLetters = getCapitalCharacterLayout( language );
            returnLetters = getNonCapitalCharacterLayout( language );
            break;
        case LayoutType::UpperCase:
            buttonLetters = getCapitalCharacterLayout( language );
            returnLetters = buttonLetters;
            break;
        case LayoutType::AlphaNumeric:
            buttonLetters = getAlphaNumericCharacterLayout( language );
            returnLetters = buttonLetters;
            break;
        case LayoutType::SignedNumeric:
        case LayoutType::UnsignedNumeric:
            buttonLetters = getNumericCharacterLayout();
            returnLetters = buttonLetters;
            break;
        default:
            // Did you add a new layout type? Add the logic above!
            assert( 0 );
            break;
        }
    }

    fheroes2::Size getDefaultButtonSize( const fheroes2::SupportedLanguage language )
    {
        // Different languages have different number of letters per row.
        // We cannot expand the virtual keyboard window beyond 640 pixels but we can change the size of buttons.
        switch ( language ) {
        case fheroes2::SupportedLanguage::Czech:
        case fheroes2::SupportedLanguage::English:
        case fheroes2::SupportedLanguage::Polish:
            return { 30, defaultButtonHeight };
        case fheroes2::SupportedLanguage::Belarusian:
        case fheroes2::SupportedLanguage::Danish:
        case fheroes2::SupportedLanguage::German:
        case fheroes2::SupportedLanguage::Russian:
        case fheroes2::SupportedLanguage::French:
        case fheroes2::SupportedLanguage::Slovak:
        case fheroes2::SupportedLanguage::Ukrainian:
            return { 24, defaultButtonHeight };
        default:
            // Did you add a new supported language? Add the value above!
            assert( 0 );
            break;
        }

        return {};
    }

    std::vector<std::vector<KeyboardButton>> generateButtons( const std::vector<std::string> & buttonLetters, const std::vector<std::string> & returnLetters,
                                                              const LayoutType layoutType, const fheroes2::SupportedLanguage language, const bool isEvilInterface )
    {
        assert( buttonLetters.size() == returnLetters.size() );

        // This is required in order to render proper text on buttons but do not change Okay button in the window.
        const fheroes2::LanguageSwitcher switcher( language );

        const auto getKeyForCharacter = [layoutType]( const char ch ) {
            // At the moment, only these layouts support input from the hardware keyboard
            if ( layoutType != LayoutType::SignedNumeric && layoutType != LayoutType::UnsignedNumeric ) {
                return fheroes2::Key::NONE;
            }

            switch ( ch ) {
            case '0':
                return fheroes2::Key::KEY_0;
            case '1':
                return fheroes2::Key::KEY_1;
            case '2':
                return fheroes2::Key::KEY_2;
            case '3':
                return fheroes2::Key::KEY_3;
            case '4':
                return fheroes2::Key::KEY_4;
            case '5':
                return fheroes2::Key::KEY_5;
            case '6':
                return fheroes2::Key::KEY_6;
            case '7':
                return fheroes2::Key::KEY_7;
            case '8':
                return fheroes2::Key::KEY_8;
            case '9':
                return fheroes2::Key::KEY_9;
            default:
                break;
            }

            return fheroes2::Key::NONE;
        };

        std::vector<std::vector<KeyboardButton>> buttons;
        buttons.resize( buttonLetters.size() );

        const fheroes2::Size buttonSize
            = ( layoutType == LayoutType::AlphaNumeric ) ? getDefaultButtonSize( fheroes2::SupportedLanguage::English ) : getDefaultButtonSize( language );

        for ( size_t i = 0; i < buttonLetters.size(); ++i ) {
            assert( buttonLetters[i].size() == returnLetters[i].size() );
            for ( size_t buttonId = 0; buttonId < buttonLetters[i].size(); ++buttonId ) {
                const char ch = buttonLetters[i][buttonId];

                buttons[i].emplace_back( std::string( 1, ch ), getKeyForCharacter( ch ), buttonSize, isEvilInterface,
                                         [letter = returnLetters[i][buttonId]]( KeyboardRenderer & renderer ) {
                                             renderer.insertCharacter( letter );
                                             return DialogAction::DoNothing;
                                         } );
            }
        }

        return buttons;
    }

    void addExtraStandardButtons( std::vector<std::vector<KeyboardButton>> & buttons, const LayoutType layoutType, const bool isEvilInterface,
                                  const bool isExtraLanguageSupported )
    {
        auto & lastButtonRow = buttons.emplace_back();

        switch ( layoutType ) {
        case LayoutType::LowerCase:
            lastButtonRow.emplace_back( "|", defaultSpecialButtonSize, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::UpperCase; } );

            lastButtonRow.emplace_back( _( "Keyboard|123" ), defaultSpecialButtonSize, isEvilInterface,
                                        []( const KeyboardRenderer & ) { return DialogAction::AlphaNumeric; } );

            lastButtonRow.emplace_back( _( "Keyboard|SPACE" ), spacebarButtonSize, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.insertCharacter( ' ' );
                return DialogAction::DoNothing;
            } );

            lastButtonRow.emplace_back( "\x7F", defaultSpecialButtonSize, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::ChangeLanguage; } );
            if ( !isExtraLanguageSupported ) {
                lastButtonRow.back().button.hide();
            }

            lastButtonRow.emplace_back( "~", defaultSpecialButtonSize, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.removeCharacter();
                return DialogAction::DoNothing;
            } );
            break;
        case LayoutType::UpperCase:
            lastButtonRow.emplace_back( "|", defaultSpecialButtonSize, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::LowerCase; } );
            lastButtonRow.back().isInvertedRenderingLogic = true;

            lastButtonRow.emplace_back( _( "Keyboard|123" ), defaultSpecialButtonSize, isEvilInterface,
                                        []( const KeyboardRenderer & ) { return DialogAction::AlphaNumeric; } );

            lastButtonRow.emplace_back( _( "Keyboard|SPACE" ), spacebarButtonSize, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.insertCharacter( ' ' );
                return DialogAction::DoNothing;
            } );

            lastButtonRow.emplace_back( "\x7F", defaultSpecialButtonSize, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::ChangeLanguage; } );
            if ( !isExtraLanguageSupported ) {
                lastButtonRow.back().button.hide();
            }

            lastButtonRow.emplace_back( "~", defaultSpecialButtonSize, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.removeCharacter();
                return DialogAction::DoNothing;
            } );
            break;
        case LayoutType::AlphaNumeric:
            lastButtonRow.emplace_back( "|", defaultSpecialButtonSize, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::DoNothing; } );
            lastButtonRow.back().button.hide();

            lastButtonRow.emplace_back( _( "Keyboard|ABC" ), defaultSpecialButtonSize, isEvilInterface,
                                        []( const KeyboardRenderer & ) { return DialogAction::LowerCase; } );

            lastButtonRow.emplace_back( _( "Keyboard|SPACE" ), spacebarButtonSize, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.insertCharacter( ' ' );
                return DialogAction::DoNothing;
            } );

            lastButtonRow.emplace_back( "\x7F", defaultSpecialButtonSize, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::DoNothing; } );
            lastButtonRow.back().button.hide();

            lastButtonRow.emplace_back( "~", defaultSpecialButtonSize, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.removeCharacter();
                return DialogAction::DoNothing;
            } );
            break;
        case LayoutType::SignedNumeric:
            lastButtonRow.emplace_back( "|", defaultSpecialButtonSize, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::DoNothing; } );
            lastButtonRow.back().button.hide();

            lastButtonRow.emplace_back( "-", fheroes2::Key::KEY_MINUS, getDefaultButtonSize( fheroes2::SupportedLanguage::English ), isEvilInterface,
                                        []( KeyboardRenderer & renderer ) {
                                            renderer.swapSign();
                                            return DialogAction::DoNothing;
                                        } );

            lastButtonRow.emplace_back( "0", fheroes2::Key::KEY_0, getDefaultButtonSize( fheroes2::SupportedLanguage::English ), isEvilInterface,
                                        []( KeyboardRenderer & renderer ) {
                                            renderer.insertCharacter( '0' );
                                            return DialogAction::DoNothing;
                                        } );

            lastButtonRow.emplace_back( "\x7F", getDefaultButtonSize( fheroes2::SupportedLanguage::English ), isEvilInterface,
                                        []( const KeyboardRenderer & ) { return DialogAction::DoNothing; } );
            lastButtonRow.back().button.hide();

            lastButtonRow.emplace_back( "~", fheroes2::Key::KEY_BACKSPACE, defaultSpecialButtonSize, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.removeCharacter();
                return DialogAction::DoNothing;
            } );
            break;
        case LayoutType::UnsignedNumeric:
            lastButtonRow.emplace_back( "|", defaultSpecialButtonSize, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::DoNothing; } );
            lastButtonRow.back().button.hide();

            lastButtonRow.emplace_back( "-", getDefaultButtonSize( fheroes2::SupportedLanguage::English ), isEvilInterface,
                                        []( const KeyboardRenderer & ) { return DialogAction::DoNothing; } );
            lastButtonRow.back().button.hide();

            lastButtonRow.emplace_back( "0", fheroes2::Key::KEY_0, getDefaultButtonSize( fheroes2::SupportedLanguage::English ), isEvilInterface,
                                        []( KeyboardRenderer & renderer ) {
                                            renderer.insertCharacter( '0' );
                                            return DialogAction::DoNothing;
                                        } );

            lastButtonRow.emplace_back( "\x7F", getDefaultButtonSize( fheroes2::SupportedLanguage::English ), isEvilInterface,
                                        []( const KeyboardRenderer & ) { return DialogAction::DoNothing; } );
            lastButtonRow.back().button.hide();

            lastButtonRow.emplace_back( "~", fheroes2::Key::KEY_BACKSPACE, defaultSpecialButtonSize, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.removeCharacter();
                return DialogAction::DoNothing;
            } );
            break;
        default:
            // Did you add a new layout type? Add the logic above!
            assert( 0 );
            break;
        }
    }

    void addExtraButtons( std::vector<std::vector<KeyboardButton>> & buttons, const LayoutType layoutType, const fheroes2::SupportedLanguage language,
                          const bool isEvilInterface, const bool isExtraLanguageSupported )
    {
        switch ( language ) {
        case fheroes2::SupportedLanguage::Belarusian:
        case fheroes2::SupportedLanguage::Czech:
        case fheroes2::SupportedLanguage::Danish:
        case fheroes2::SupportedLanguage::English:
        case fheroes2::SupportedLanguage::French:
        case fheroes2::SupportedLanguage::German:
        case fheroes2::SupportedLanguage::Polish:
        case fheroes2::SupportedLanguage::Russian:
        case fheroes2::SupportedLanguage::Slovak:
        case fheroes2::SupportedLanguage::Ukrainian:
            addExtraStandardButtons( buttons, layoutType, isEvilInterface, isExtraLanguageSupported );
            break;
        default:
            assert( 0 );
            break;
        }
    }

    fheroes2::Rect getButtonsRoi( const std::vector<std::vector<KeyboardButton>> & buttonLayout, const fheroes2::Point & offset, const int32_t windowWidth )
    {
        std::vector<int32_t> offsets;

        int32_t maximumLength = 0;
        for ( const auto & buttonRow : buttonLayout ) {
            int32_t length = 0;
            for ( const auto & buttonInfo : buttonRow ) {
                length += buttonInfo.button.area().width;
            }

            length += ( static_cast<int32_t>( buttonRow.size() ) - 1 ) * buttonOffset;

            offsets.push_back( length );
            maximumLength = std::max( maximumLength, length );
        }

        for ( int32_t & rowOffset : offsets ) {
            rowOffset = ( windowWidth - 2 * offsetFromWindowBorders.x - rowOffset ) / 2;
        }

        fheroes2::Rect roi{ offset.x + offsets.front(), offset.y, 1, 1 };

        const size_t buttonRows = buttonLayout.size();

        for ( size_t i = 0; i < buttonRows; ++i ) {
            int32_t xOffset = offset.x + offsets[i];
            const int32_t newX = std::min( xOffset, roi.x );
            roi.width = ( roi.x - newX ) + roi.width;
            roi.x = newX;

            for ( const auto & buttonInfo : buttonLayout[i] ) {
                xOffset += buttonInfo.button.area().width + buttonOffset;
            }

            roi.width = std::max( xOffset - buttonOffset - roi.x, roi.width );
        }

        const int32_t yOffset = offset.y + static_cast<int32_t>( buttonRows * defaultButtonHeight + ( buttonRows - 1 ) * buttonOffset * 2 );

        // Take button shadow offset into account.
        roi.x += std::min( 0, buttonShadowOffset.x );
        roi.y += std::min( 0, buttonShadowOffset.y );
        roi.width += std::abs( buttonShadowOffset.x );
        roi.height = yOffset - roi.y + std::abs( buttonShadowOffset.y );

        return roi;
    }

    void renderButtons( std::vector<std::vector<KeyboardButton>> & buttonLayout, const fheroes2::Point & offset, fheroes2::Image & output, const int32_t windowWidth )
    {
        std::vector<int32_t> offsets;

        int32_t maximumLength = 0;
        for ( const auto & buttonRow : buttonLayout ) {
            int32_t length = 0;
            for ( const auto & buttonInfo : buttonRow ) {
                length += buttonInfo.button.area().width;
            }

            length += ( static_cast<int32_t>( buttonRow.size() ) - 1 ) * buttonOffset;

            offsets.push_back( length );
            maximumLength = std::max( maximumLength, length );
        }

        for ( int32_t & rowOffset : offsets ) {
            rowOffset = ( windowWidth - 2 * offsetFromWindowBorders.x - rowOffset ) / 2;
        }

        int32_t yOffset = offset.y;
        for ( size_t i = 0; i < buttonLayout.size(); ++i ) {
            int32_t xOffset = offset.x + offsets[i];
            for ( auto & buttonInfo : buttonLayout[i] ) {
                buttonInfo.button.setPosition( xOffset, yOffset );

                if ( buttonInfo.isInvertedRenderingLogic ) {
                    buttonInfo.button.press();
                }
                if ( buttonInfo.button.draw( output ) ) {
                    fheroes2::Blit( buttonInfo.buttonShadow, output, xOffset + buttonShadowOffset.x, yOffset );
                }
                xOffset += buttonInfo.button.area().width + buttonOffset;
            }

            yOffset += defaultButtonHeight + buttonOffset * 2;
        }
    }

    DialogAction handleButtonAndKeyboardEvents( const std::vector<std::vector<KeyboardButton>> & buttonLayout, LocalEvent & le, KeyboardRenderer & renderer )
    {
        const fheroes2::Key key = [&le]() {
            if ( !le.isAnyKeyPressed() ) {
                return fheroes2::Key::NONE;
            }

            const fheroes2::Key keyValue = le.getPressedKeyValue();

            // Convert the numpad keys to regular keys if needed
            switch ( keyValue ) {
            case fheroes2::Key::KEY_KP_MINUS:
                return fheroes2::Key::KEY_MINUS;
            case fheroes2::Key::KEY_KP_0:
                return fheroes2::Key::KEY_0;
            case fheroes2::Key::KEY_KP_1:
                return fheroes2::Key::KEY_1;
            case fheroes2::Key::KEY_KP_2:
                return fheroes2::Key::KEY_2;
            case fheroes2::Key::KEY_KP_3:
                return fheroes2::Key::KEY_3;
            case fheroes2::Key::KEY_KP_4:
                return fheroes2::Key::KEY_4;
            case fheroes2::Key::KEY_KP_5:
                return fheroes2::Key::KEY_5;
            case fheroes2::Key::KEY_KP_6:
                return fheroes2::Key::KEY_6;
            case fheroes2::Key::KEY_KP_7:
                return fheroes2::Key::KEY_7;
            case fheroes2::Key::KEY_KP_8:
                return fheroes2::Key::KEY_8;
            case fheroes2::Key::KEY_KP_9:
                return fheroes2::Key::KEY_9;
            default:
                break;
            }

            return keyValue;
        }();

        if ( const std::optional<CursorPosition> pos = [key]() -> std::optional<CursorPosition> {
                 switch ( key ) {
                 case fheroes2::Key::KEY_LEFT:
                     return CursorPosition::PrevChar;
                 case fheroes2::Key::KEY_RIGHT:
                     return CursorPosition::NextChar;
                 case fheroes2::Key::KEY_HOME:
                     return CursorPosition::BegOfText;
                 case fheroes2::Key::KEY_END:
                     return CursorPosition::EndOfText;
                 default:
                     break;
                 }

                 return {};
             }();
             pos ) {
            renderer.setCursorPosition( *pos );

            return DialogAction::DoNothing;
        }

        for ( const auto & buttonRow : buttonLayout ) {
            for ( const auto & buttonInfo : buttonRow ) {
                if ( !buttonInfo.button.isVisible() ) {
                    continue;
                }

                if ( le.MouseClickLeft( buttonInfo.button.area() ) || ( key != fheroes2::Key::NONE && buttonInfo.key == key ) ) {
                    assert( buttonInfo.action );

                    return buttonInfo.action( renderer );
                }

                if ( le.MouseLongPressLeft( buttonInfo.button.area() ) ) {
                    assert( buttonInfo.action );

                    const auto actionType = buttonInfo.action( renderer );
                    if ( actionType == DialogAction::DoNothing ) {
                        // Only for the event of entering a character.
                        le.resetLongPress();
                    }

                    return actionType;
                }
            }
        }

        return DialogAction::DoNothing;
    }

    void updateButtonStates( std::vector<std::vector<KeyboardButton>> & buttonLayout, const LocalEvent & le )
    {
        for ( auto & buttonRow : buttonLayout ) {
            for ( auto & buttonInfo : buttonRow ) {
                if ( buttonInfo.isInvertedRenderingLogic ) {
                    buttonInfo.button.drawOnState( !le.isMouseLeftButtonPressedAndHeldInArea( buttonInfo.button.area() ) );
                }
                else {
                    buttonInfo.button.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonInfo.button.area() ) );
                }
            }
        }
    }

    DialogAction processVirtualKeyboardEvent( const LayoutType layoutType, const fheroes2::SupportedLanguage language, const bool isExtraLanguageSupported,
                                              KeyboardRenderer & renderer )
    {
        std::vector<std::string> buttonLetters;
        std::vector<std::string> returnLetters;

        getCharacterLayout( layoutType, language, buttonLetters, returnLetters );

        const bool isNumericOnlyLayout = ( layoutType == LayoutType::SignedNumeric ) || ( layoutType == LayoutType::UnsignedNumeric );

        const int32_t windowWidth = isNumericOnlyLayout ? numpadWindowWidth : defaultWindowWidth;

        const bool isResized = renderer.resize(
            { windowWidth, defaultWindowHeight + ( static_cast<int32_t>( buttonLetters.size() ) - defaultLetterRows ) * ( defaultButtonHeight + buttonOffset * 2 ) } );

        const bool isEvilInterface = renderer.isEvilInterface();
        auto buttons = generateButtons( buttonLetters, returnLetters, layoutType, language, isEvilInterface );
        addExtraButtons( buttons, layoutType, language, isEvilInterface, isExtraLanguageSupported );

        const fheroes2::Rect windowRoi{ renderer.getWindowRoi() };
        const fheroes2::Rect buttonsRoi = getButtonsRoi( buttons, windowRoi.getPosition() + offsetFromWindowBorders, windowWidth );
        const fheroes2::Rect & textRoi = renderer.getTextRoi();

        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::ImageRestorer restorer( display, buttonsRoi.x, buttonsRoi.y, buttonsRoi.width, buttonsRoi.height );

        renderButtons( buttons, windowRoi.getPosition() + offsetFromWindowBorders, display, windowWidth );

        const int buttonIcnId = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        const fheroes2::Sprite & okayButtonReleasedImage = fheroes2::AGG::GetICN( buttonIcnId, 0 );
        const fheroes2::Sprite & okayButtonPressedImage = fheroes2::AGG::GetICN( buttonIcnId, 1 );

        const fheroes2::Point okayButtonPosition{ windowRoi.x + ( windowRoi.width - okayButtonReleasedImage.width() ) / 2, windowRoi.y + windowRoi.height - 35 };

        fheroes2::ButtonSprite okayButton( okayButtonPosition.x, okayButtonPosition.y, okayButtonReleasedImage, okayButtonPressedImage );

        // Render OKAY button and its shadow only if the keyboard dialog was resized.
        if ( isResized ) {
            okayButton.draw();
            fheroes2::addGradientShadow( okayButtonReleasedImage, display, okayButtonPosition, buttonShadowOffset );
        }

        display.render();

        DialogAction action = DialogAction::DoNothing;

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            okayButton.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( okayButton.area() ) );

            if ( le.MouseClickLeft( okayButton.area() ) || Game::HotKeyCloseWindow() ) {
                // Reset all event states including the hotkey pressed state so the caller dialog will not process it right after closing this dialog.
                le.reset();
                break;
            }

            action = handleButtonAndKeyboardEvents( buttons, le, renderer );
            if ( action != DialogAction::DoNothing ) {
                return action;
            }

            updateButtonStates( buttons, le );

            if ( le.MouseClickLeft( textRoi ) ) {
                renderer.setCursorPosition( le.getMouseLeftButtonPressedPos() );
            }

            // Text input cursor blink.
            renderer.eventProcessing();
        }

        return DialogAction::Close;
    }
}

namespace fheroes2
{
    void openVirtualKeyboard( std::string & output, size_t lengthLimit )
    {
        if ( lengthLimit == 0 ) {
            // A string longer than 64KB is extremely impossible.
            lengthLimit = std::numeric_limits<uint16_t>::max();
        }

        SupportedLanguage language = SupportedLanguage::English;
        DialogAction action = DialogAction::DoNothing;
        LayoutType layoutType = LayoutType::LowerCase;

        const SupportedLanguage currentGameLanguage = getCurrentLanguage();
        if ( currentGameLanguage == lastSelectedLanguage ) {
            language = lastSelectedLanguage;
        }

        KeyboardRenderer renderer( Display::instance(), output, lengthLimit, Settings::Get().isEvilInterfaceEnabled() );

        while ( action != DialogAction::Close ) {
            action = processVirtualKeyboardEvent( layoutType, language, isSupportedForLanguageSwitching( currentGameLanguage ), renderer );
            switch ( action ) {
            case DialogAction::DoNothing:
                // This action must not be processed here!
                assert( 0 );
                break;
            case DialogAction::LowerCase:
                layoutType = LayoutType::LowerCase;
                break;
            case DialogAction::UpperCase:
                layoutType = LayoutType::UpperCase;
                break;
            case DialogAction::AlphaNumeric:
                layoutType = LayoutType::AlphaNumeric;
                break;
            case DialogAction::ChangeLanguage:
                assert( isSupportedForLanguageSwitching( currentGameLanguage ) );

                if ( currentGameLanguage != SupportedLanguage::English ) {
                    if ( language == SupportedLanguage::English ) {
                        language = currentGameLanguage;
                    }
                    else {
                        language = SupportedLanguage::English;
                    }

                    lastSelectedLanguage = language;
                }
                break;
            case DialogAction::Close:
                return;
            default:
                // Did you add a new state? Add the logic above!
                assert( 0 );
                break;
            }
        }
    }

    void openVirtualNumpad( int32_t & output, const int32_t minValue, const int32_t maxValue )
    {
        std::string strValue = std::to_string( output );
        DialogAction action = DialogAction::DoNothing;

        // Lets limit to 11 digits: minus and 10 digits for INT32_MIN
        KeyboardRenderer renderer( Display::instance(), strValue, 10, Settings::Get().isEvilInterfaceEnabled() );

        const LayoutType layoutType = ( minValue < 0 ) ? LayoutType::SignedNumeric : LayoutType::UnsignedNumeric;

        while ( action != DialogAction::Close ) {
            action = processVirtualKeyboardEvent( layoutType, SupportedLanguage::English, false, renderer );
            switch ( action ) {
            case DialogAction::AlphaNumeric:
            case DialogAction::ChangeLanguage:
            case DialogAction::DoNothing:
            case DialogAction::LowerCase:
            case DialogAction::UpperCase:
                // These actions must not be processed here!
                assert( 0 );
                break;
            case DialogAction::Close: {
                const auto handleInvalidValue = [&action]() {
                    showStandardTextMessage( _( "Error" ), _( "The entered value is invalid." ), Dialog::OK );

                    action = DialogAction::DoNothing;
                };

                const auto handleOutOfRange = [&action, &minValue, &maxValue]() {
                    std::string errorMessage = _( "The entered value is out of range.\nIt should be not less than %{minValue} and not more than %{maxValue}." );
                    StringReplace( errorMessage, "%{minValue}", minValue );
                    StringReplace( errorMessage, "%{maxValue}", maxValue );

                    showStandardTextMessage( _( "Error" ), std::move( errorMessage ), Dialog::OK );

                    action = DialogAction::DoNothing;
                };

                try {
                    const int32_t intValue = std::stoi( strValue );
                    if ( intValue < minValue || intValue > maxValue ) {
                        handleOutOfRange();
                        break;
                    }

                    output = intValue;
                    return;
                }
                // The string can be empty or contain only a minus sign (when entering a negative number and then deleting numeric characters using the Backspace key)
                catch ( std::invalid_argument & ) {
                    handleInvalidValue();
                }
                catch ( std::out_of_range & ) {
                    handleOutOfRange();
                }

                break;
            }
            default:
                // Did you add a new state? Add the logic above!
                assert( 0 );
                break;
            }
        }
    }
}
