/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

#include <cassert>
#include <vector>

#include "agg_image.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "localevent.h"
#include "pal.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_language.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    const int32_t buttonOffset{ 5 };
    const int32_t defaultButtonWidth{ 20 };
    const int32_t defaultButtonHeight{ 25 };
    const fheroes2::Size windowSize{ 420, 250 };
    const fheroes2::Point offsetFromWindowBorders{ 25, 50 };
    const fheroes2::Size inputAreaSize{ 268, 21 };
    const int32_t inputAreaOffset{ 2 };

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
        KeyboardRenderer( fheroes2::Display & output, std::string & info, const bool isEvilInterface )
            : _output( output )
            , _info( info )
            , _window( windowSize.width, windowSize.height, true, output )
            , _isEvilInterface( isEvilInterface )
        {
            // Do nothing.
        }

        fheroes2::Rect getWindowRoi() const
        {
            return _window.activeArea();
        }

        void fullRender()
        {
            _window.render();

            renderInputArea();

            _output.render( getWindowRoi() );
        }

        void appendCharacter( const uint8_t character )
        {
            _info += static_cast<char>( character );
            if ( fheroes2::Text( _info, fheroes2::FontType::normalWhite() ).width() > inputAreaSize.width - inputAreaOffset * 2 ) {
                _info.pop_back();
            }

            renderInputArea();
        }

        void removeLastCharacter()
        {
            if ( _info.empty() ) {
                return;
            }

            _info.pop_back();

            renderInputArea();
        }

    private:
        fheroes2::Display & _output;
        std::string & _info;
        fheroes2::StandardWindow _window;
        const bool _isEvilInterface;

        void renderInputArea()
        {
            const int32_t offsetFromWindowTop{ 20 };

            const fheroes2::Rect outputRoi{ _window.activeArea().x + ( _window.activeArea().width - inputAreaSize.width ) / 2,
                                            _window.activeArea().y + offsetFromWindowTop, inputAreaSize.width, inputAreaSize.height };

            const fheroes2::Sprite & initialWindow = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
            fheroes2::Copy( initialWindow, 40, 286, _output, outputRoi.x, outputRoi.y, outputRoi.width, outputRoi.height );

            if ( _isEvilInterface ) {
                fheroes2::ApplyPalette( _output, outputRoi.x, outputRoi.y, _output, outputRoi.x, outputRoi.y, outputRoi.width, outputRoi.height,
                                        PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );
            }

            fheroes2::Text textUI( _info, fheroes2::FontType::normalWhite() );
            textUI.draw( _window.activeArea().x + ( _window.activeArea().width - inputAreaSize.width ) / 2 + inputAreaOffset,
                         _window.activeArea().y + inputAreaSize.height + ( inputAreaSize.height - textUI.height() ) / 2 + inputAreaOffset, _output );

            _output.render( outputRoi );
        }
    };

    fheroes2::ButtonSprite generateButton( const std::string & info, const int32_t buttonWidth, const bool isEvilInterface )
    {
        fheroes2::Sprite released;
        fheroes2::Sprite pressed;
        fheroes2::Point releasedOffset;
        fheroes2::Point pressedOffset;
        fheroes2::getCustomNormalButton( released, pressed, isEvilInterface, buttonWidth, releasedOffset, pressedOffset );

        const fheroes2::FontType releasedFont{ fheroes2::FontSize::BUTTON_RELEASED, fheroes2::FontColor::WHITE };
        const fheroes2::Text text( StringUpper( info ), releasedFont );

        const fheroes2::Point textOffset{ ( buttonWidth - text.width() ) / 2 - 1, ( 16 - fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) ) / 2 };

        text.draw( releasedOffset.x + textOffset.x, releasedOffset.y + textOffset.y, released );
        text.draw( pressedOffset.x + textOffset.x, pressedOffset.y + textOffset.y, pressed );

        return { 0, 0, released, pressed };
    }

    struct KeyboardButton
    {
        KeyboardButton() = default;

        KeyboardButton( std::string input, const int32_t buttonWidth, const bool isEvilInterface, std::function<DialogAction( KeyboardRenderer & )> actionEvent )
            : text( std::move( input ) )
            , action( std::move( actionEvent ) )
            , button( generateButton( text, buttonWidth, isEvilInterface ) )
        {
            // Do nothing.
        }

        std::string text;

        std::function<DialogAction( KeyboardRenderer & )> action;

        fheroes2::ButtonSprite button;

        // This is used only for buttons which should have pressed state for some layouts.
        bool isInvertedRenderingLogic{ false };
    };

    std::vector<std::vector<uint8_t>> getNumericCharacterLayout( const fheroes2::SupportedLanguage language )
    {
        // Numeric character can be used for special letters as well
        switch ( language ) {
        case fheroes2::SupportedLanguage::English:
            return { { { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' }, {}, {} } };
        default:
            assert( 0 );
        }

        return {};
    }

    std::vector<std::vector<uint8_t>> getCapitalCharacterLayout( const fheroes2::SupportedLanguage language )
    {
        switch ( language ) {
        case fheroes2::SupportedLanguage::English:
            return { { { 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P' }, { 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L' }, { 'Z', 'X', 'C', 'V', 'B', 'N', 'M' } } };
        default:
            assert( 0 );
        }

        return {};
    }

    std::vector<std::vector<uint8_t>> getNonCapitalCharacterLayout( const fheroes2::SupportedLanguage language )
    {
        auto layout = getCapitalCharacterLayout( language );
        for ( auto & letters : layout ) {
            for ( uint8_t & letter : letters ) {
                std::string singleCharString( 1, static_cast<char>( letter ) );
                singleCharString = StringLower( singleCharString );
                letter = static_cast<uint8_t>( singleCharString.front() );
            }
        }

        return layout;
    }

    std::vector<std::vector<uint8_t>> getCharacterLayout( const LayoutType layoutType, const fheroes2::SupportedLanguage language )
    {
        switch ( layoutType ) {
        case LayoutType::LowerCase:
            return getNonCapitalCharacterLayout( language );
        case LayoutType::UpperCase:
            return getCapitalCharacterLayout( language );
        case LayoutType::Numeric:
            return getNumericCharacterLayout( language );
        default:
            // Did you add a new layout type? Add the logic above!
            assert( 0 );
            break;
        }

        return {};
    }

    std::vector<std::vector<KeyboardButton>> generateButtons( const std::vector<std::vector<uint8_t>> & letterRows, const bool isEvilInterface )
    {
        std::vector<std::vector<KeyboardButton>> buttons;
        buttons.resize( letterRows.size() );

        for ( size_t i = 0; i < letterRows.size(); ++i ) {
            for ( const uint8_t letter : letterRows[i] ) {
                buttons[i].emplace_back( std::string( 1, static_cast<char>( letter ) ), defaultButtonWidth, isEvilInterface, [letter]( KeyboardRenderer & renderer ) {
                    renderer.appendCharacter( letter );
                    return DialogAction::AddLetter;
                } );
            }
        }

        return buttons;
    }

    void addExtraEnglishButtons( std::vector<std::vector<KeyboardButton>> & buttons, const LayoutType layoutType, const bool isEvilInterface )
    {
        switch ( layoutType ) {
        case LayoutType::LowerCase:
            buttons.back().emplace_back( _( "Keyboard|BACK" ), 60, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.removeLastCharacter();
                return DialogAction::Backspace;
            } );
            buttons.back().emplace( buttons.back().begin(), _( "Keyboard|UPP" ), 60, isEvilInterface, []( KeyboardRenderer & ) { return DialogAction::UpperCase; } );

            buttons.emplace_back();
            buttons.back().emplace_back( _( "Keyboard|123" ), 60, isEvilInterface, []( KeyboardRenderer & ) { return DialogAction::Numeric; } );
            buttons.back().emplace_back( _( "Keyboard|SPACE" ), 160, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.appendCharacter( ' ' );
                return DialogAction::AddLetter;
            } );
            buttons.back().emplace_back( _( "Keyboard|LANG" ), 60, isEvilInterface, []( KeyboardRenderer & ) { return DialogAction::DoNothing; } );
            break;
        case LayoutType::UpperCase:
            buttons.back().emplace_back( _( "Keyboard|BACK" ), 60, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.removeLastCharacter();
                return DialogAction::Backspace;
            } );
            buttons.back().emplace( buttons.back().begin(), _( "Keyboard|UPP" ), 60, isEvilInterface, []( KeyboardRenderer & ) { return DialogAction::LowerCase; } );
            buttons.back().front().isInvertedRenderingLogic = true;

            buttons.emplace_back();
            buttons.back().emplace_back( _( "Keyboard|123" ), 60, isEvilInterface, []( KeyboardRenderer & ) { return DialogAction::Numeric; } );
            buttons.back().emplace_back( _( "Keyboard|SPACE" ), 160, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.appendCharacter( ' ' );
                return DialogAction::AddLetter;
            } );
            buttons.back().emplace_back( _( "Keyboard|LANG" ), 60, isEvilInterface, []( KeyboardRenderer & ) { return DialogAction::DoNothing; } );
            break;
        case LayoutType::Numeric:
            buttons.back().emplace_back( _( "Keyboard|BACK" ), 60, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.removeLastCharacter();
                return DialogAction::Backspace;
            } );

            buttons.emplace_back();
            buttons.back().emplace_back( _( "Keyboard|ABC" ), 60, isEvilInterface, []( KeyboardRenderer & ) { return DialogAction::LowerCase; } );
            buttons.back().emplace_back( _( "Keyboard|SPACE" ), 160, isEvilInterface, []( KeyboardRenderer & renderer ) {
                renderer.appendCharacter( ' ' );
                return DialogAction::AddLetter;
            } );
            buttons.back().emplace_back( _( "Keyboard|LANG" ), 60, isEvilInterface, []( KeyboardRenderer & ) { return DialogAction::DoNothing; } );
            break;
        default:
            // Did you add a new layout type? Add the logic above!
            assert( 0 );
            break;
        }
    }

    void addExtraButtons( std::vector<std::vector<KeyboardButton>> & buttons, const LayoutType layoutType, const fheroes2::SupportedLanguage language,
                          const bool isEvilInterface )
    {
        switch ( language ) {
        case fheroes2::SupportedLanguage::English:
            addExtraEnglishButtons( buttons, layoutType, isEvilInterface );
            break;
        default:
            assert( 0 );
        }
    }

    fheroes2::Rect getButtonsRoi( const std::vector<std::vector<KeyboardButton>> & buttonLayout, const fheroes2::Point offset )
    {
        std::vector<int32_t> offsets;

        int32_t maximumLength = 0;
        for ( const auto & buttons : buttonLayout ) {
            int32_t length = 0;
            for ( const auto & button : buttons ) {
                length += button.button.area().width;
            }

            length += ( static_cast<int32_t>( buttons.size() ) - 1 ) * buttonOffset;

            offsets.push_back( length );
            maximumLength = std::max( maximumLength, length );
        }

        for ( int32_t & rowOffset : offsets ) {
            rowOffset = ( windowSize.width - 2 * offsetFromWindowBorders.x - rowOffset ) / 2;
        }

        fheroes2::Rect roi{ offset.x + offsets.front(), offset.y, 1, 1 };

        int32_t yOffset = offset.y;
        for ( size_t i = 0; i < buttonLayout.size(); ++i ) {
            int32_t xOffset = offset.x + offsets[i];
            const int32_t newX = std::min( xOffset, roi.x );
            roi.width = ( roi.x - newX ) + roi.width;
            roi.x = newX;

            for ( const auto & button : buttonLayout[i] ) {
                xOffset += button.button.area().width + buttonOffset;
            }

            yOffset += defaultButtonHeight + buttonOffset * 2;
            roi.width = std::max( xOffset - buttonOffset - roi.x, roi.width );
        }

        roi.height = yOffset - roi.y;

        return roi;
    }

    void renderButtons( std::vector<std::vector<KeyboardButton>> & buttonLayout, const fheroes2::Point offset, fheroes2::Image & output )
    {
        std::vector<int32_t> offsets;

        int32_t maximumLength = 0;
        for ( const auto & buttons : buttonLayout ) {
            int32_t length = 0;
            for ( const auto & button : buttons ) {
                length += button.button.area().width;
            }

            length += ( static_cast<int32_t>( buttons.size() ) - 1 ) * buttonOffset;

            offsets.push_back( length );
            maximumLength = std::max( maximumLength, length );
        }

        for ( int32_t & rowOffset : offsets ) {
            rowOffset = ( windowSize.width - 2 * offsetFromWindowBorders.x - rowOffset ) / 2;
        }

        int32_t yOffset = offset.y;
        for ( size_t i = 0; i < buttonLayout.size(); ++i ) {
            int32_t xOffset = offset.x + offsets[i];
            for ( auto & button : buttonLayout[i] ) {
                button.button.setPosition( xOffset, yOffset );
                if ( button.isInvertedRenderingLogic ) {
                    button.button.press();
                }
                button.button.draw( output );
                xOffset += button.button.area().width + buttonOffset;
            }

            yOffset += defaultButtonHeight + buttonOffset * 2;
        }
    }

    DialogAction handleButtonEvents( std::vector<std::vector<KeyboardButton>> & buttonLayout, LocalEvent & le, KeyboardRenderer & renderer )
    {
        for ( auto & buttons : buttonLayout ) {
            for ( auto & button : buttons ) {
                if ( button.isInvertedRenderingLogic ) {
                    le.MousePressLeft( button.button.area() ) ? button.button.drawOnRelease() : button.button.drawOnPress();
                }
                else {
                    le.MousePressLeft( button.button.area() ) ? button.button.drawOnPress() : button.button.drawOnRelease();
                }
            }
        }

        for ( const auto & buttons : buttonLayout ) {
            for ( const auto & button : buttons ) {
                if ( le.MouseClickLeft( button.button.area() ) ) {
                    return button.action( renderer );
                }
            }
        }

        return DialogAction::DoNothing;
    }

    DialogAction processVirtualKeyboardEvent( const LayoutType layoutType, const fheroes2::SupportedLanguage language, KeyboardRenderer & renderer )
    {
        fheroes2::LanguageSwitcher switcher( language );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        auto currentLayout = getCharacterLayout( layoutType, language );
        auto buttons = generateButtons( currentLayout, isEvilInterface );
        addExtraButtons( buttons, layoutType, language, isEvilInterface );

        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::Rect windowRoi{ renderer.getWindowRoi() };

        const fheroes2::Rect buttonsRoi = getButtonsRoi( buttons, windowRoi.getPosition() + offsetFromWindowBorders );

        fheroes2::ImageRestorer restorer( display, buttonsRoi.x, buttonsRoi.y, buttonsRoi.width, buttonsRoi.height );

        renderButtons( buttons, windowRoi.getPosition() + offsetFromWindowBorders, display );

        const int buttonIcnId = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        const fheroes2::Sprite & buttonSprite = fheroes2::AGG::GetICN( buttonIcnId, 0 );

        fheroes2::ButtonSprite okayButton( windowRoi.x + ( windowRoi.width - buttonSprite.width() ) / 2, windowRoi.y + windowRoi.height - 35, buttonSprite,
                                           fheroes2::AGG::GetICN( buttonIcnId, 1 ) );
        okayButton.draw();

        display.render();

        DialogAction action = DialogAction::DoNothing;

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            if ( le.MousePressLeft( okayButton.area() ) ) {
                okayButton.drawOnPress();
            }
            else {
                okayButton.drawOnRelease();
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

            if ( le.MouseClickLeft( okayButton.area() ) || Game::HotKeyCloseWindow() ) {
                break;
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

        KeyboardRenderer renderer( fheroes2::Display::instance(), output, Settings::Get().isEvilInterfaceEnabled() );
        renderer.fullRender();

        while ( action != DialogAction::Close ) {
            action = processVirtualKeyboardEvent( layoutType, language, renderer );
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
                // TODO: do something here.
                break;
            default:
                break;
            }
        }
    }
}
