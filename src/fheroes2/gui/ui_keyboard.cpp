/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "game_delays.h"
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
#include "ui_language.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    const int32_t buttonOffset{ 8 };
    const int32_t defaultButtonHeight{ 25 };
    const int32_t defaultSpecialButtonWidth{ 54 };
    const int32_t spacebarButtonWidth{ 175 };
    const int32_t defaultWindowWidth{ 520 };
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
        case fheroes2::SupportedLanguage::Czech:
        case fheroes2::SupportedLanguage::English:
            // English is a default language so it is not considered as an extra language.
            return false;
        case fheroes2::SupportedLanguage::Belarusian:
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
        AddLetter,
        UpperCase,
        LowerCase,
        Numeric,
        ChangeLanguage,
        Backspace,
        Close
    };

    enum class LayoutType : int
    {
        LowerCase,
        UpperCase,
        Numeric
    };

    class KeyboardRenderer
    {
    public:
        KeyboardRenderer( fheroes2::Display & output, std::string & info, const bool evilInterface )
            : _output( output )
            , _info( info )
            , _isEvilInterface( evilInterface )
            , _cursorPosition( info.size() )
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

        fheroes2::Rect getTextRoi() const
        {
            const fheroes2::Rect windowRoi{ getWindowRoi() };
            return { windowRoi.x + ( windowRoi.width - inputAreaSize.width ) / 2 + inputAreaBorders, windowRoi.y + inputAreaOffsetFromWindowTop + inputAreaBorders,
                     inputAreaSize.width - inputAreaBorders * 2, inputAreaSize.height - inputAreaBorders * 2 };
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

            const fheroes2::Point defaultOffset{ ( _output.width() - defaultWindowWidth ) / 2, ( _output.height() - defaultWindowHeight ) / 2 };
            const fheroes2::Point offset{ defaultOffset.x - ( size.width - defaultWindowWidth ), defaultOffset.y - ( size.height - defaultWindowHeight ) };

            // It is important to destroy the previous window to avoid rendering issues.
            _window.reset();

            _window = std::make_unique<fheroes2::StandardWindow>( offset.x, offset.y, size.width, size.height, true, _output );

            _window->render();

            renderInputArea();

            return true;
        }

        void insertCharacter( const char character )
        {
            if ( _info.size() >= 255 ) {
                // Do not add more characters as the string is already long enough.
                return;
            }

            _info.insert( _cursorPosition, 1, character );

            ++_cursorPosition;

            _output.render( renderInputArea() );
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

            _output.render( renderInputArea() );
        }

        void changeCursorState()
        {
            _isCursorVisible = !_isCursorVisible;

            renderInputArea();
        }

        void setCursorPosition( const int32_t clickXPosition, const int32_t startXPosition )
        {
            _cursorPosition = fheroes2::getTextInputCursorPosition( _info, fheroes2::FontType::normalWhite(), _cursorPosition, clickXPosition, startXPosition );

            renderInputArea();
        }

    private:
        fheroes2::Display & _output;
        std::string & _info;
        std::unique_ptr<fheroes2::StandardWindow> _window;
        const bool _isEvilInterface{ false };
        bool _isCursorVisible{ true };
        size_t _cursorPosition{ 0 };

        fheroes2::Rect renderInputArea()
        {
            const fheroes2::Rect & windowRoi = _window->activeArea();
            const fheroes2::Rect outputRoi{ windowRoi.x + ( windowRoi.width - inputAreaSize.width ) / 2, windowRoi.y + inputAreaOffsetFromWindowTop, inputAreaSize.width,
                                            inputAreaSize.height };

            const fheroes2::Sprite & initialWindow = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
            fheroes2::Copy( initialWindow, 40, 286, _output, outputRoi.x, outputRoi.y, outputRoi.width, outputRoi.height );

            if ( _isEvilInterface ) {
                fheroes2::ApplyPalette( _output, outputRoi.x, outputRoi.y, _output, outputRoi.x, outputRoi.y, outputRoi.width, outputRoi.height,
                                        PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );
            }

            fheroes2::Text textUI( insertCharToString( _info, _cursorPosition, _isCursorVisible ? '_' : '\x7F' ), fheroes2::FontType::normalWhite() );
            textUI.fitToOneRow( inputAreaSize.width - inputAreaBorders * 2 );

            textUI.draw( windowRoi.x + ( windowRoi.width - inputAreaSize.width ) / 2 + inputAreaBorders,
                         windowRoi.y + inputAreaSize.height + ( inputAreaSize.height - textUI.height() ) / 2 + inputAreaBorders, _output );

            return outputRoi;
        }
    };

    struct KeyboardButton
    {
        KeyboardButton() = default;

        KeyboardButton( std::string input, const int32_t buttonWidth, const bool isEvilInterface, std::function<DialogAction( KeyboardRenderer & )> actionEvent )
            : text( std::move( input ) )
            , action( std::move( actionEvent ) )
        {
            fheroes2::Sprite released;
            fheroes2::Sprite pressed;
            makeButtonSprites( released, pressed, text, buttonWidth, isEvilInterface, false );
            button.setSprite( released, pressed );

            // Make Image with shadow for button to Blit it during render.
            buttonShadow.resize( released.width() + std::abs( buttonShadowOffset.x ), released.height() + std::abs( buttonShadowOffset.y ) );
            buttonShadow.reset();
            fheroes2::addGradientShadow( released, buttonShadow, { -std::min( 0, buttonShadowOffset.x ), -std::min( 0, buttonShadowOffset.y ) }, buttonShadowOffset );
        }

        KeyboardButton( const KeyboardButton & ) = delete;

        KeyboardButton( KeyboardButton && ) noexcept = default;

        ~KeyboardButton() = default;

        KeyboardButton & operator=( const KeyboardButton & ) = delete;

        KeyboardButton & operator=( KeyboardButton && ) = default;

        std::string text;

        std::function<DialogAction( KeyboardRenderer & )> action;

        fheroes2::ButtonSprite button;
        fheroes2::Image buttonShadow;

        // This is used only for buttons which should have pressed state for some layouts.
        bool isInvertedRenderingLogic{ false };
    };

    std::vector<std::string> getNumericCharacterLayout( const fheroes2::SupportedLanguage language )
    {
        // Numeric layout can be used for special letters as well.
        switch ( language ) {
        case fheroes2::SupportedLanguage::Belarusian:
        case fheroes2::SupportedLanguage::Czech:
        case fheroes2::SupportedLanguage::English:
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

    std::vector<std::string> getCapitalCharacterLayout( const fheroes2::SupportedLanguage language )
    {
        switch ( language ) {
        case fheroes2::SupportedLanguage::Belarusian:
            return { "\xC9\xD6\xD3\xCA\xC5\xCD\xC3\xD8\xA1\xC7\xD5\x92", "\xD4\xDB\xC2\xC0\xCF\xD0\xCE\xCB\xC4\xC6\xDD", "\xDF\xD7\xD1\xCC\xB2\xD2\xDC\xC1\xDE\xA8" };
        case fheroes2::SupportedLanguage::Czech:
            return { "\xCC\x8A\xC8\xD8\x8E\xDD\xC1\xCD\xC9", "QWERTZUIOP\xDA", "ASDFGHJKL\xD9", "YXCVBNM" };
        case fheroes2::SupportedLanguage::English:
            return { "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };
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
        case fheroes2::SupportedLanguage::English:
            return { "qwertyuiop", "asdfghjkl", "zxcvbnm" };
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
        case LayoutType::Numeric:
            buttonLetters = getNumericCharacterLayout( language );
            returnLetters = buttonLetters;
            break;
        default:
            // Did you add a new layout type? Add the logic above!
            assert( 0 );
            break;
        }
    }

    int32_t getDefaultButtonWidth( const fheroes2::SupportedLanguage language )
    {
        // Different languages have different number of letters per row.
        // We cannot expand the virtual keyboard window beyond 640 pixels but we can change the size of buttons.
        switch ( language ) {
        case fheroes2::SupportedLanguage::Czech:
        case fheroes2::SupportedLanguage::English:
        case fheroes2::SupportedLanguage::Polish:
            return 30;
        case fheroes2::SupportedLanguage::Belarusian:
        case fheroes2::SupportedLanguage::Russian:
        case fheroes2::SupportedLanguage::Slovak:
        case fheroes2::SupportedLanguage::Ukrainian:
            return 24;
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

        std::vector<std::vector<KeyboardButton>> buttons;
        buttons.resize( buttonLetters.size() );

        const int32_t buttonWidth
            = ( layoutType == LayoutType::Numeric ) ? getDefaultButtonWidth( fheroes2::SupportedLanguage::English ) : getDefaultButtonWidth( language );

        for ( size_t i = 0; i < buttonLetters.size(); ++i ) {
            assert( buttonLetters[i].size() == returnLetters[i].size() );
            for ( size_t buttonId = 0; buttonId < buttonLetters[i].size(); ++buttonId ) {
                buttons[i].emplace_back( std::string( 1, buttonLetters[i][buttonId] ), buttonWidth, isEvilInterface,
                                         [letter = returnLetters[i][buttonId]]( KeyboardRenderer & renderer ) {
                                             renderer.insertCharacter( letter );
                                             return DialogAction::AddLetter;
                                         } );
            }
        }

        return buttons;
    }

    void addExtraStandardButtons( std::vector<std::vector<KeyboardButton>> & buttons, const LayoutType layoutType, const bool isEvilInterface,
                                  const bool isExtraLanguageSupported, const fheroes2::SupportedLanguage /* unused */ )
    {
        auto & lastButtonRow = buttons.emplace_back();

        switch ( layoutType ) {
        case LayoutType::LowerCase:
            lastButtonRow.emplace_back( "|", defaultSpecialButtonWidth, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::UpperCase; } );

            lastButtonRow.emplace_back( _( "Keyboard|123" ), defaultSpecialButtonWidth, isEvilInterface,
                                        []( const KeyboardRenderer & ) { return DialogAction::Numeric; } );

            lastButtonRow.emplace_back( _( "Keyboard|SPACE" ), spacebarButtonWidth, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.insertCharacter( ' ' );
                return DialogAction::AddLetter;
            } );

            lastButtonRow.emplace_back( "\x7F", defaultSpecialButtonWidth, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::ChangeLanguage; } );
            if ( !isExtraLanguageSupported ) {
                lastButtonRow.back().button.hide();
            }

            lastButtonRow.emplace_back( "~", defaultSpecialButtonWidth, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.removeCharacter();
                return DialogAction::Backspace;
            } );
            break;
        case LayoutType::UpperCase:
            lastButtonRow.emplace_back( "|", defaultSpecialButtonWidth, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::LowerCase; } );
            lastButtonRow.back().isInvertedRenderingLogic = true;

            lastButtonRow.emplace_back( _( "Keyboard|123" ), defaultSpecialButtonWidth, isEvilInterface,
                                        []( const KeyboardRenderer & ) { return DialogAction::Numeric; } );

            lastButtonRow.emplace_back( _( "Keyboard|SPACE" ), spacebarButtonWidth, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.insertCharacter( ' ' );
                return DialogAction::AddLetter;
            } );

            lastButtonRow.emplace_back( "\x7F", defaultSpecialButtonWidth, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::ChangeLanguage; } );
            if ( !isExtraLanguageSupported ) {
                lastButtonRow.back().button.hide();
            }

            lastButtonRow.emplace_back( "~", defaultSpecialButtonWidth, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.removeCharacter();
                return DialogAction::Backspace;
            } );
            break;
        case LayoutType::Numeric:
            lastButtonRow.emplace_back( "|", defaultSpecialButtonWidth, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::DoNothing; } );
            lastButtonRow.back().button.hide();

            lastButtonRow.emplace_back( _( "Keyboard|ABC" ), defaultSpecialButtonWidth, isEvilInterface,
                                        []( const KeyboardRenderer & ) { return DialogAction::LowerCase; } );

            lastButtonRow.emplace_back( _( "Keyboard|SPACE" ), spacebarButtonWidth, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.insertCharacter( ' ' );
                return DialogAction::AddLetter;
            } );

            lastButtonRow.emplace_back( "\x7F", defaultSpecialButtonWidth, isEvilInterface, []( const KeyboardRenderer & ) { return DialogAction::ChangeLanguage; } );
            lastButtonRow.back().button.hide();

            lastButtonRow.emplace_back( "~", defaultSpecialButtonWidth, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.removeCharacter();
                return DialogAction::Backspace;
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
        case fheroes2::SupportedLanguage::English:
        case fheroes2::SupportedLanguage::Polish:
        case fheroes2::SupportedLanguage::Russian:
        case fheroes2::SupportedLanguage::Slovak:
        case fheroes2::SupportedLanguage::Ukrainian:
            addExtraStandardButtons( buttons, layoutType, isEvilInterface, isExtraLanguageSupported, language );
            break;
        default:
            assert( 0 );
            break;
        }
    }

    fheroes2::Rect getButtonsRoi( const std::vector<std::vector<KeyboardButton>> & buttonLayout, const fheroes2::Point & offset )
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
            rowOffset = ( defaultWindowWidth - 2 * offsetFromWindowBorders.x - rowOffset ) / 2;
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

    void renderButtons( std::vector<std::vector<KeyboardButton>> & buttonLayout, const fheroes2::Point & offset, fheroes2::Image & output )
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
            rowOffset = ( defaultWindowWidth - 2 * offsetFromWindowBorders.x - rowOffset ) / 2;
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

    DialogAction handleButtonEvents( const std::vector<std::vector<KeyboardButton>> & buttonLayout, LocalEvent & le, KeyboardRenderer & renderer )
    {
        for ( const auto & buttonRow : buttonLayout ) {
            for ( const auto & buttonInfo : buttonRow ) {
                if ( buttonInfo.button.isVisible() && le.MouseClickLeft( buttonInfo.button.area() ) ) {
                    assert( buttonInfo.action );
                    return buttonInfo.action( renderer );
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
                    le.isMouseLeftButtonPressedInArea( buttonInfo.button.area() ) ? buttonInfo.button.drawOnRelease() : buttonInfo.button.drawOnPress();
                }
                else {
                    le.isMouseLeftButtonPressedInArea( buttonInfo.button.area() ) ? buttonInfo.button.drawOnPress() : buttonInfo.button.drawOnRelease();
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

        const bool isResized = renderer.resize(
            { defaultWindowWidth,
              defaultWindowHeight + ( static_cast<int32_t>( buttonLetters.size() ) - defaultLetterRows ) * ( defaultButtonHeight + buttonOffset * 2 ) } );

        const bool isEvilInterface = renderer.isEvilInterface();
        auto buttons = generateButtons( buttonLetters, returnLetters, layoutType, language, isEvilInterface );
        addExtraButtons( buttons, layoutType, language, isEvilInterface, isExtraLanguageSupported );

        const fheroes2::Rect windowRoi{ renderer.getWindowRoi() };
        const fheroes2::Rect buttonsRoi = getButtonsRoi( buttons, windowRoi.getPosition() + offsetFromWindowBorders );
        const fheroes2::Rect textRoi{ renderer.getTextRoi() };

        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::ImageRestorer restorer( display, buttonsRoi.x, buttonsRoi.y, buttonsRoi.width, buttonsRoi.height );

        renderButtons( buttons, windowRoi.getPosition() + offsetFromWindowBorders, display );

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

        Game::AnimateResetDelay( Game::DelayType::CURSOR_BLINK_DELAY );

        while ( le.HandleEvents( Game::isDelayNeeded( { Game::DelayType::CURSOR_BLINK_DELAY } ) ) ) {
            if ( le.MouseClickLeft( okayButton.area() ) || Game::HotKeyCloseWindow() ) {
                break;
            }

            action = handleButtonEvents( buttons, le, renderer );
            switch ( action ) {
            case DialogAction::DoNothing:
            case DialogAction::AddLetter:
            case DialogAction::Backspace:
                // Do nothing.
                break;
            default:
                return action;
            }

            updateButtonStates( buttons, le );

            if ( le.isMouseLeftButtonPressedInArea( okayButton.area() ) ) {
                okayButton.drawOnPress();
            }
            else {
                okayButton.drawOnRelease();
            }

            if ( le.MouseClickLeft( textRoi ) ) {
                renderer.setCursorPosition( le.getMouseCursorPos().x, textRoi.x );
            }

            // Text input cursor blink.
            if ( Game::validateAnimationDelay( Game::DelayType::CURSOR_BLINK_DELAY ) ) {
                renderer.changeCursorState();
            }
        }

        return DialogAction::Close;
    }
}

namespace fheroes2
{
    void openVirtualKeyboard( std::string & output )
    {
        const std::vector<SupportedLanguage> supportedLanguages = getSupportedLanguages();
        SupportedLanguage language = SupportedLanguage::English;
        DialogAction action = DialogAction::AddLetter;
        LayoutType layoutType = LayoutType::LowerCase;

        const SupportedLanguage currentGameLanguage = getCurrentLanguage();

        if ( currentGameLanguage == lastSelectedLanguage ) {
            language = lastSelectedLanguage;
        }

        KeyboardRenderer renderer( fheroes2::Display::instance(), output, Settings::Get().isEvilInterfaceEnabled() );

        while ( action != DialogAction::Close ) {
            action = processVirtualKeyboardEvent( layoutType, language, isSupportedForLanguageSwitching( currentGameLanguage ), renderer );
            switch ( action ) {
            case DialogAction::DoNothing:
            case DialogAction::AddLetter:
            case DialogAction::Backspace:
                // These actions must not be processed here!
                assert( 0 );
                break;
            case DialogAction::LowerCase:
                layoutType = LayoutType::LowerCase;
                break;
            case DialogAction::UpperCase:
                layoutType = LayoutType::UpperCase;
                break;
            case DialogAction::Numeric:
                layoutType = LayoutType::Numeric;
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
}
