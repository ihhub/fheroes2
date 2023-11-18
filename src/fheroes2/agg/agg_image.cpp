/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2023                                             *
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

#include "agg_image.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "agg.h"
#include "agg_file.h"
#include "battle_cell.h"
#include "exception.h"
#include "h2d.h"
#include "icn.h"
#include "image.h"
#include "image_tool.h"
#include "math_base.h"
#include "pal.h"
#include "rand.h"
#include "screen.h"
#include "serialize.h"
#include "til.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_font.h"
#include "ui_language.h"
#include "ui_text.h"

namespace
{
    const std::array<const char *, TIL::LASTTIL> tilFileName = { "UNKNOWN", "CLOF32.TIL", "GROUND32.TIL", "STON.TIL" };

    std::vector<std::vector<fheroes2::Sprite>> _icnVsSprite( ICN::LASTICN );
    std::array<std::vector<std::vector<fheroes2::Image>>, TIL::LASTTIL> _tilVsImage;
    const fheroes2::Sprite errorImage;

    const uint32_t headerSize = 6;

    std::map<int, std::vector<fheroes2::Sprite>> _icnVsScaledSprite;

    // Some resources are language dependent. These are mostly buttons with a text of them.
    // Once a user changes a language we have to update resources. To do this we need to clear the existing images.

    const std::set<int> languageDependentIcnId{ ICN::BUYMAX,
                                                ICN::BUTTON_NEW_GAME_GOOD,
                                                ICN::BUTTON_NEW_GAME_EVIL,
                                                ICN::BUTTON_SAVE_GAME_GOOD,
                                                ICN::BUTTON_SAVE_GAME_EVIL,
                                                ICN::BUTTON_LOAD_GAME_GOOD,
                                                ICN::BUTTON_LOAD_GAME_EVIL,
                                                ICN::BUTTON_INFO_GOOD,
                                                ICN::BUTTON_INFO_EVIL,
                                                ICN::BUTTON_QUIT_GOOD,
                                                ICN::BUTTON_QUIT_EVIL,
                                                ICN::BUTTON_SMALL_CANCEL_GOOD,
                                                ICN::BUTTON_SMALL_CANCEL_EVIL,
                                                ICN::BUTTON_SMALL_OKAY_GOOD,
                                                ICN::BUTTON_SMALL_OKAY_EVIL,
                                                ICN::BUTTON_SMALLER_OKAY_GOOD,
                                                ICN::BUTTON_SMALLER_OKAY_EVIL,
                                                ICN::BUTTON_SMALL_ACCEPT_GOOD,
                                                ICN::BUTTON_SMALL_ACCEPT_EVIL,
                                                ICN::BUTTON_SMALL_DECLINE_GOOD,
                                                ICN::BUTTON_SMALL_DECLINE_EVIL,
                                                ICN::BUTTON_SMALL_LEARN_GOOD,
                                                ICN::BUTTON_SMALL_LEARN_EVIL,
                                                ICN::BUTTON_SMALL_TRADE_GOOD,
                                                ICN::BUTTON_SMALL_TRADE_EVIL,
                                                ICN::BUTTON_SMALL_YES_GOOD,
                                                ICN::BUTTON_SMALL_YES_EVIL,
                                                ICN::BUTTON_SMALL_NO_GOOD,
                                                ICN::BUTTON_SMALL_NO_EVIL,
                                                ICN::BUTTON_SMALL_EXIT_GOOD,
                                                ICN::BUTTON_SMALL_EXIT_EVIL,
                                                ICN::BUTTON_EXIT_HEROES_MEETING,
                                                ICN::BUTTON_EXIT_TOWN,
                                                ICN::BUTTON_EXIT_PUZZLE_DDOOR_GOOD,
                                                ICN::BUTTON_EXIT_PUZZLE_DDOOR_EVIL,
                                                ICN::BUTTON_SMALL_DISMISS_GOOD,
                                                ICN::BUTTON_SMALL_DISMISS_EVIL,
                                                ICN::BUTTON_SMALL_UPGRADE_GOOD,
                                                ICN::BUTTON_SMALL_UPGRADE_EVIL,
                                                ICN::BUTTON_SMALL_RESTART_GOOD,
                                                ICN::BUTTON_SMALL_RESTART_EVIL,
                                                ICN::BUTTON_KINGDOM_EXIT,
                                                ICN::BUTTON_KINGDOM_HEROES,
                                                ICN::BUTTON_KINGDOM_TOWNS,
                                                ICN::BUTTON_MAPSIZE_SMALL,
                                                ICN::BUTTON_MAPSIZE_MEDIUM,
                                                ICN::BUTTON_MAPSIZE_LARGE,
                                                ICN::BUTTON_MAPSIZE_XLARGE,
                                                ICN::BUTTON_MAPSIZE_ALL,
                                                ICN::BUTTON_MAP_SELECT,
                                                ICN::BUTTON_STANDARD_GAME,
                                                ICN::BUTTON_CAMPAIGN_GAME,
                                                ICN::BUTTON_MULTIPLAYER_GAME,
                                                ICN::BUTTON_LARGE_CANCEL,
                                                ICN::BUTTON_LARGE_CONFIG,
                                                ICN::BUTTON_ORIGINAL_CAMPAIGN,
                                                ICN::BUTTON_EXPANSION_CAMPAIGN,
                                                ICN::BUTTON_HOT_SEAT,
                                                ICN::BUTTON_2_PLAYERS,
                                                ICN::BUTTON_3_PLAYERS,
                                                ICN::BUTTON_4_PLAYERS,
                                                ICN::BUTTON_5_PLAYERS,
                                                ICN::BUTTON_6_PLAYERS,
                                                ICN::BTNBATTLEONLY,
                                                ICN::BTNGIFT_GOOD,
                                                ICN::BTNGIFT_EVIL,
                                                ICN::UNIFORM_EVIL_MAX_BUTTON,
                                                ICN::UNIFORM_EVIL_MIN_BUTTON,
                                                ICN::UNIFORM_GOOD_MAX_BUTTON,
                                                ICN::UNIFORM_GOOD_MIN_BUTTON,
                                                ICN::UNIFORM_GOOD_OKAY_BUTTON,
                                                ICN::UNIFORM_EVIL_OKAY_BUTTON,
                                                ICN::UNIFORM_GOOD_CANCEL_BUTTON,
                                                ICN::UNIFORM_EVIL_CANCEL_BUTTON,
                                                ICN::UNIFORM_GOOD_EXIT_BUTTON,
                                                ICN::UNIFORM_EVIL_EXIT_BUTTON,
                                                ICN::BUTTON_SMALL_MIN_GOOD,
                                                ICN::BUTTON_SMALL_MIN_EVIL,
                                                ICN::BUTTON_SMALL_MAX_GOOD,
                                                ICN::BUTTON_SMALL_MAX_EVIL,
                                                ICN::BUTTON_GUILDWELL_EXIT,
                                                ICN::GOOD_CAMPAIGN_BUTTONS,
                                                ICN::EVIL_CAMPAIGN_BUTTONS,
                                                ICN::POL_CAMPAIGN_BUTTONS,
                                                ICN::BUTTON_VIEWWORLD_EXIT_GOOD,
                                                ICN::BUTTON_VIEWWORLD_EXIT_EVIL,
                                                ICN::BUTTON_VERTICAL_DISMISS,
                                                ICN::BUTTON_VERTICAL_EXIT,
                                                ICN::BUTTON_HSCORES_VERTICAL_CAMPAIGN,
                                                ICN::BUTTON_HSCORES_VERTICAL_EXIT,
                                                ICN::BUTTON_HSCORES_VERTICAL_STANDARD,
                                                ICN::DISMISS_HERO_DISABLED_BUTTON,
                                                ICN::NEW_CAMPAIGN_DISABLED_BUTTON };

#ifndef NDEBUG
    bool isLanguageDependentIcnId( const int id )
    {
        return languageDependentIcnId.count( id ) > 0;
    }
#endif

    bool useOriginalResources()
    {
        const fheroes2::SupportedLanguage currentLanguage = fheroes2::getCurrentLanguage();
        const fheroes2::SupportedLanguage resourceLanguage = fheroes2::getResourceLanguage();
        return ( currentLanguage == fheroes2::SupportedLanguage::English && resourceLanguage == fheroes2::SupportedLanguage::English )
               || ( currentLanguage == fheroes2::SupportedLanguage::Czech && resourceLanguage == fheroes2::SupportedLanguage::Czech )
               || ( currentLanguage == fheroes2::SupportedLanguage::Polish && resourceLanguage == fheroes2::SupportedLanguage::Polish )
               || ( currentLanguage == fheroes2::SupportedLanguage::French && resourceLanguage == fheroes2::SupportedLanguage::French )
               || ( currentLanguage == fheroes2::SupportedLanguage::German && resourceLanguage == fheroes2::SupportedLanguage::German )
               || ( currentLanguage == fheroes2::SupportedLanguage::Russian && resourceLanguage == fheroes2::SupportedLanguage::Russian );
    }

    bool IsValidICNId( int id )
    {
        return id >= 0 && static_cast<size_t>( id ) < _icnVsSprite.size();
    }

    bool IsValidTILId( int id )
    {
        return id >= 0 && static_cast<size_t>( id ) < _tilVsImage.size();
    }

    fheroes2::Image createDigit( const int32_t width, const int32_t height, const std::vector<fheroes2::Point> & points, const uint8_t pixelColor )
    {
        fheroes2::Image digit( width, height );
        digit.reset();

        fheroes2::SetPixel( digit, points, pixelColor );
        fheroes2::Blit( fheroes2::CreateContour( digit, 35 ), digit );

        return digit;
    }

    fheroes2::Image addDigit( const fheroes2::Sprite & original, const fheroes2::Image & digit, const fheroes2::Point & offset )
    {
        fheroes2::Image combined( original.width() + digit.width() + offset.x, original.height() + ( offset.y < 0 ? 0 : offset.y ) );
        combined.reset();

        fheroes2::Copy( original, 0, 0, combined, 0, 0, original.width(), original.height() );
        fheroes2::Blit( digit, 0, 0, combined, original.width() + offset.x, combined.height() - digit.height() + ( offset.y >= 0 ? 0 : offset.y ), digit.width(),
                        digit.height() );

        return combined;
    }

    void populateCursorIcons( std::vector<fheroes2::Sprite> & output, const fheroes2::Sprite & origin, const std::vector<fheroes2::Image> & digits,
                              const fheroes2::Point & offset )
    {
        output.emplace_back( origin );
        for ( const fheroes2::Image & digit : digits ) {
            output.emplace_back( addDigit( origin, digit, offset ) );
            output.back().setPosition( output.back().width() - origin.width(), output.back().height() - origin.height() );
        }
    }

    void fillRandomPixelsFromImage( const fheroes2::Image & original, const fheroes2::Rect & originalRoi, fheroes2::Image & output, const fheroes2::Rect & outputRoi,
                                    std::mt19937 & seededGen )
    {
        for ( int x = outputRoi.x; x < outputRoi.x + outputRoi.width; ++x ) {
            for ( int y = outputRoi.y; y < outputRoi.y + outputRoi.height; ++y ) {
                const fheroes2::Point pixelLocation{ static_cast<int32_t>( Rand::GetWithGen( originalRoi.x, originalRoi.x + originalRoi.width - 1, seededGen ) ),
                                                     static_cast<int32_t>( Rand::GetWithGen( originalRoi.y, originalRoi.y + originalRoi.height - 1, seededGen ) ) };

                fheroes2::Copy( original, pixelLocation.x, pixelLocation.y, output, x, y, 1, 1 );
            }
        }
    }

    // BMP files within AGG are not Bitmap images!
    fheroes2::Sprite loadBMPFile( const std::string & path )
    {
        const std::vector<uint8_t> & data = AGG::getDataFromAggFile( path );
        if ( data.size() < 6 ) {
            // It is an invalid BMP file.
            return {};
        }

        StreamBuf imageStream( data );

        const uint8_t blackColor = imageStream.get();
        const uint8_t whiteColor = 11;

        // Skip the second byte
        imageStream.get();

        const int32_t width = imageStream.get16();
        const int32_t height = imageStream.get16();

        if ( static_cast<int32_t>( data.size() ) != 6 + width * height ) {
            // It is an invalid BMP file.
            return {};
        }

        fheroes2::Sprite output( width, height );

        const uint8_t * input = data.data() + 6;
        uint8_t * image = output.image();
        const uint8_t * imageEnd = image + width * height;
        uint8_t * transform = output.transform();

        for ( ; image != imageEnd; ++image, ++transform, ++input ) {
            if ( *input == 1 ) {
                *image = whiteColor;
                *transform = 0;
            }
            else if ( *input == 2 ) {
                *image = blackColor;
                *transform = 0;
            }
            else {
                *transform = 1;
            }
        }

        return output;
    }

    // This class serves the purpose of preserving the original alphabet which is loaded from AGG files for cases when we generate new language alphabet.
    class OriginalAlphabetPreserver
    {
    public:
        void preserve()
        {
            if ( _isPreserved ) {
                return;
            }

            fheroes2::AGG::GetICN( ICN::FONT, 0 );
            fheroes2::AGG::GetICN( ICN::SMALFONT, 0 );
            fheroes2::AGG::GetICN( ICN::BUTTON_GOOD_FONT_RELEASED, 0 );
            fheroes2::AGG::GetICN( ICN::BUTTON_GOOD_FONT_PRESSED, 0 );
            fheroes2::AGG::GetICN( ICN::BUTTON_EVIL_FONT_RELEASED, 0 );
            fheroes2::AGG::GetICN( ICN::BUTTON_EVIL_FONT_PRESSED, 0 );

            _normalFont = _icnVsSprite[ICN::FONT];
            _smallFont = _icnVsSprite[ICN::SMALFONT];
            _buttonGoodReleasedFont = _icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED];
            _buttonGoodPressedFont = _icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED];
            _buttonEvilReleasedFont = _icnVsSprite[ICN::BUTTON_EVIL_FONT_RELEASED];
            _buttonEvilPressedFont = _icnVsSprite[ICN::BUTTON_EVIL_FONT_PRESSED];

            _isPreserved = true;
        }

        void restore() const
        {
            if ( !_isPreserved ) {
                return;
            }

            // Restore the original font.
            _icnVsSprite[ICN::FONT] = _normalFont;
            _icnVsSprite[ICN::SMALFONT] = _smallFont;
            _icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] = _buttonGoodReleasedFont;
            _icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED] = _buttonGoodPressedFont;
            _icnVsSprite[ICN::BUTTON_EVIL_FONT_RELEASED] = _buttonEvilReleasedFont;
            _icnVsSprite[ICN::BUTTON_EVIL_FONT_PRESSED] = _buttonEvilPressedFont;

            // Clear modified fonts.
            _icnVsSprite[ICN::YELLOW_FONT].clear();
            _icnVsSprite[ICN::YELLOW_SMALLFONT].clear();
            _icnVsSprite[ICN::GRAY_FONT].clear();
            _icnVsSprite[ICN::GRAY_SMALL_FONT].clear();
            _icnVsSprite[ICN::WHITE_LARGE_FONT].clear();
        }

        bool isPreserved() const
        {
            return _isPreserved;
        }

    private:
        bool _isPreserved = false;

        std::vector<fheroes2::Sprite> _normalFont;
        std::vector<fheroes2::Sprite> _smallFont;
        std::vector<fheroes2::Sprite> _buttonGoodReleasedFont;
        std::vector<fheroes2::Sprite> _buttonGoodPressedFont;
        std::vector<fheroes2::Sprite> _buttonEvilReleasedFont;
        std::vector<fheroes2::Sprite> _buttonEvilPressedFont;
    };

    OriginalAlphabetPreserver alphabetPreserver;

    // This class is used for situations when we need to remove letter-specific offsets, like when we display single letters in a row,
    // and then restore these offsets within the scope of the code
    class ButtonFontOffsetRestorer
    {
    public:
        ButtonFontOffsetRestorer( std::vector<fheroes2::Sprite> & font, const int32_t offsetX );
        ButtonFontOffsetRestorer( const ButtonFontOffsetRestorer & ) = delete;

        ~ButtonFontOffsetRestorer();

        ButtonFontOffsetRestorer & operator=( const ButtonFontOffsetRestorer & ) = delete;

    private:
        std::vector<fheroes2::Sprite> & _font;
        std::vector<int32_t> _originalXOffsets;
    };

    ButtonFontOffsetRestorer::ButtonFontOffsetRestorer( std::vector<fheroes2::Sprite> & font, const int32_t offsetX )
        : _font( font )
    {
        _originalXOffsets.reserve( _font.size() );

        for ( fheroes2::Sprite & characterSprite : _font ) {
            _originalXOffsets.emplace_back( characterSprite.x() );
            characterSprite.setPosition( offsetX, characterSprite.y() );
        }
    }

    ButtonFontOffsetRestorer::~ButtonFontOffsetRestorer()
    {
        if ( _originalXOffsets.size() != _font.size() ) {
            // If this assertion blows up then something is wrong with the fonts as they must have the same size.
            assert( 0 );
            return;
        }

        for ( size_t i = 0; i < _font.size(); ++i ) {
            _font[i].setPosition( _originalXOffsets[i], _font[i].y() );
        }
    }

    void invertTransparency( fheroes2::Image & image )
    {
        if ( image.singleLayer() ) {
            assert( 0 );
            return;
        }

        uint8_t * transform = image.transform();
        const uint8_t * transformEnd = transform + image.width() * image.height();
        for ( ; transform != transformEnd; ++transform ) {
            if ( *transform == 0 ) {
                *transform = 1;
            }
            else if ( *transform == 1 ) {
                *transform = 0;
            }
            // Other transform values are not relevant for transparency checks.
        }
    }

    uint8_t getButtonFillingColor( const bool isReleasedState, const bool isGoodInterface = true )
    {
        if ( isGoodInterface ) {
            return isReleasedState ? fheroes2::GetColorId( 216, 184, 152 ) : fheroes2::GetColorId( 184, 136, 96 );
        }
        return isReleasedState ? fheroes2::GetColorId( 180, 180, 180 ) : fheroes2::GetColorId( 144, 144, 144 );
    }

    const char * getSupportedText( const char * text, const fheroes2::FontType & font )
    {
        const char * translatedText = _( text );
        return fheroes2::isFontAvailable( translatedText, font ) ? translatedText : text;
    }

    void renderTextOnButton( fheroes2::Image & releasedState, fheroes2::Image & pressedState, const char * text, const fheroes2::Point & releasedTextOffset,
                             const fheroes2::Point & pressedTextOffset, const fheroes2::Size buttonSize, const fheroes2::FontColor fontColor )
    {
        const fheroes2::FontType releasedFont{ fheroes2::FontSize::BUTTON_RELEASED, fontColor };
        const fheroes2::FontType pressedFont{ fheroes2::FontSize::BUTTON_PRESSED, fontColor };

        const char * textSupported = getSupportedText( text, releasedFont );

        const fheroes2::Text releasedText( textSupported, releasedFont );
        const fheroes2::Text pressedText( textSupported, pressedFont );

        const fheroes2::Size releasedTextSize( releasedText.width( buttonSize.width ), releasedText.height( buttonSize.width ) );
        const fheroes2::Size pressedTextSize( pressedText.width( buttonSize.width ), pressedText.height( buttonSize.width ) );

        releasedText.draw( releasedTextOffset.x, releasedTextOffset.y + ( buttonSize.height - releasedTextSize.height ) / 2, buttonSize.width, releasedState );
        pressedText.draw( pressedTextOffset.x, pressedTextOffset.y + ( buttonSize.height - pressedTextSize.height ) / 2, buttonSize.width, pressedState );
    }

    void createNormalButton( fheroes2::Sprite & released, fheroes2::Sprite & pressed, int32_t textWidth, const char * text, const bool isEvilInterface )
    {
        fheroes2::Point releasedOffset;
        fheroes2::Point pressedOffset;
        fheroes2::getCustomNormalButton( released, pressed, isEvilInterface, textWidth, releasedOffset, pressedOffset );

        const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;

        renderTextOnButton( released, pressed, text, releasedOffset, pressedOffset, { textWidth, fheroes2::getFontHeight( fheroes2::FontSize::BUTTON_RELEASED ) },
                            buttonFontColor );
    }

    void createCampaignButtonSet( const int campaignSetIcnId, const std::array<const char *, 5> & texts )
    {
        int emptyButtonIcnID = 0;
        int buttonBackgroundICN = ICN::UNKNOWN;
        switch ( campaignSetIcnId ) {
        case ICN::GOOD_CAMPAIGN_BUTTONS:
            emptyButtonIcnID = ICN::EMPTY_GOOD_BUTTON;
            buttonBackgroundICN = ICN::STONEBAK;
            break;
        case ICN::EVIL_CAMPAIGN_BUTTONS:
            emptyButtonIcnID = ICN::EMPTY_EVIL_BUTTON;
            buttonBackgroundICN = ICN::STONEBAK_EVIL;
            break;
        case ICN::POL_CAMPAIGN_BUTTONS:
            emptyButtonIcnID = ICN::EMPTY_POL_BUTTON;
            buttonBackgroundICN = ICN::STONEBAK_SMALL_POL;
            break;
        default:
            // Was a new set of buttons added?
            assert( 0 );
            break;
        }

        for ( size_t i = 0; i < texts.size(); ++i ) {
            const size_t icnIndex = 2 * i;
            fheroes2::getTextAdaptedButton( _icnVsSprite[campaignSetIcnId][icnIndex], _icnVsSprite[campaignSetIcnId][icnIndex + 1], texts[i], emptyButtonIcnID,
                                            buttonBackgroundICN );
        }
    }

    void convertToEvilInterface( fheroes2::Sprite & image, const fheroes2::Rect & roi )
    {
        fheroes2::ApplyPalette( image, roi.x, roi.y, image, roi.x, roi.y, roi.width, roi.height, PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );
    }

    void copyEvilInterfaceElements( fheroes2::Sprite & image, const fheroes2::Rect & roi )
    {
        // Evil interface has special elements at each corner of the window.
        const fheroes2::Sprite & original = fheroes2::AGG::GetICN( ICN::CSPANBKE, 0 );

        // If this assertion blows up you are using some modded resources. Good luck!
        assert( original.width() == 321 && original.height() == 304 );

        // Top-left corner.
        fheroes2::Copy( original, 0, 0, image, roi.x, roi.y, 17, 43 );
        fheroes2::Copy( original, 17, 0, image, roi.x + 17, roi.y, 26, 14 );

        // Top-right corner.
        fheroes2::Copy( original, original.width() - 43, 0, image, roi.x + roi.width - 43, roi.y, 43, 14 );
        fheroes2::Copy( original, original.width() - 17, 14, image, roi.x + roi.width - 17, roi.y + 14, 17, 29 );

        // Bottom-right corner.
        fheroes2::Copy( original, original.width() - 13, original.height() - 43, image, roi.x + roi.width - 13, roi.y + roi.height - 43, 13, 27 );
        fheroes2::Copy( original, original.width() - 16, original.height() - 16, image, roi.x + roi.width - 16, roi.y + roi.height - 16, 16, 16 );
        fheroes2::Copy( original, original.width() - 43, original.height() - 13, image, roi.x + roi.width - 43, roi.y + roi.height - 13, 27, 13 );

        // Bottom-left corner.
        fheroes2::Copy( original, 0, original.height() - 43, image, roi.x, roi.y + roi.height - 43, 13, 27 );
        fheroes2::Copy( original, 0, original.height() - 16, image, roi.x, roi.y + roi.height - 16, 16, 16 );
        fheroes2::Copy( original, 16, original.height() - 13, image, roi.x + 16, roi.y + roi.height - 13, 27, 13 );
    }

    // Sets the upper left (offset 3 pixels), upper right (offset 2 pixels), lower right (offset 3 pixels) corners transparent.
    void setButtonCornersTransparent( fheroes2::Sprite & buttonSprite )
    {
        const ptrdiff_t imageWidth = buttonSprite.width();
        const ptrdiff_t imageHeight = buttonSprite.height();

        assert( imageWidth > 3 && imageHeight > 3 );

        uint8_t * imageTransform = buttonSprite.transform();
        const uint8_t transparencyValue = 1;
        std::fill( imageTransform, imageTransform + 4, transparencyValue );
        std::fill( imageTransform + imageWidth - 2, imageTransform + imageWidth + 3, transparencyValue );
        std::fill( imageTransform + 2 * imageWidth - 1, imageTransform + 2 * imageWidth + 2, transparencyValue );
        *( imageTransform + 3 * imageWidth ) = transparencyValue;
        *( imageTransform + ( imageHeight - 3 ) * imageWidth - 1 ) = transparencyValue;
        std::fill( imageTransform + ( imageHeight - 2 ) * imageWidth - 2, imageTransform + ( imageHeight - 2 ) * imageWidth, transparencyValue );
        std::fill( imageTransform + ( imageHeight - 1 ) * imageWidth - 3, imageTransform + ( imageHeight - 1 ) * imageWidth, transparencyValue );
        std::fill( imageTransform + imageHeight * imageWidth - 4, imageTransform + imageHeight * imageWidth, transparencyValue );
    }
}

namespace fheroes2
{
    namespace AGG
    {
        void loadICN( const int id );

        void LoadOriginalICN( const int id )
        {
            // If this assertion blows up then something wrong with your logic and you load resources more than once!
            assert( _icnVsSprite[id].empty() );

            const std::vector<uint8_t> & body = ::AGG::getDataFromAggFile( ICN::GetString( id ) );

            if ( body.empty() ) {
                return;
            }

            StreamBuf imageStream( body );

            const uint32_t count = imageStream.getLE16();
            const uint32_t blockSize = imageStream.getLE32();
            if ( count == 0 || blockSize == 0 ) {
                return;
            }

            _icnVsSprite[id].resize( count );

            for ( uint32_t i = 0; i < count; ++i ) {
                imageStream.seek( headerSize + i * 13 );

                ICNHeader header1;
                imageStream >> header1;

                uint32_t sizeData = 0;
                if ( i + 1 != count ) {
                    ICNHeader header2;
                    imageStream >> header2;
                    sizeData = header2.offsetData - header1.offsetData;
                }
                else {
                    sizeData = blockSize - header1.offsetData;
                }

                if ( headerSize + header1.offsetData + sizeData > body.size() ) {
                    // This is a corrupted AGG file.
                    throw fheroes2::InvalidDataResources( "ICN Id " + std::to_string( id ) + ", index " + std::to_string( i )
                                                          + " is being corrupted. "
                                                            "Make sure that you own an official version of the game." );
                }

                const uint8_t * data = body.data() + headerSize + header1.offsetData;

                _icnVsSprite[id][i] = decodeICNSprite( data, sizeData, header1.width, header1.height, header1.offsetX, header1.offsetY );
            }
        }

        // Helper function for LoadModifiedICN
        void CopyICNWithPalette( int icnId, int originalIcnId, const PAL::PaletteType paletteType )
        {
            assert( icnId != originalIcnId );

            GetICN( originalIcnId, 0 ); // always avoid calling LoadOriginalICN directly

            _icnVsSprite[icnId] = _icnVsSprite[originalIcnId];
            const std::vector<uint8_t> & palette = PAL::GetPalette( paletteType );
            for ( fheroes2::Sprite & sprite : _icnVsSprite[icnId] ) {
                ApplyPalette( sprite, palette );
            }
        }

        void generateDefaultImages( const int id )
        {
            switch ( id ) {
            case ICN::BTNBATTLEONLY: {
                _icnVsSprite[id].resize( 2 );

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );

                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "BATTLE\nONLY" ), { 12, 5 }, { 11, 6 }, { 117, 47 },
                                    fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_NEW_GAME_EVIL:
            case ICN::BUTTON_NEW_GAME_GOOD: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_NEW_GAME_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::CPANELE : ICN::CPANEL, 0 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::CPANELE : ICN::CPANEL, 1 );
                    break;
                }

                const int baseIcnId = isEvilInterface ? ICN::EMPTY_EVIL_MEDIUM_BUTTON : ICN::EMPTY_GOOD_MEDIUM_BUTTON;

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( baseIcnId, i );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "NEW\nGAME" ), { 7, 5 }, { 6, 6 }, { 86, 48 }, buttonFontColor );

                break;
            }
            case ICN::BUTTON_SAVE_GAME_EVIL:
            case ICN::BUTTON_SAVE_GAME_GOOD: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SAVE_GAME_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::CPANELE : ICN::CPANEL, 4 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::CPANELE : ICN::CPANEL, 5 );
                    break;
                }

                const int baseIcnId = isEvilInterface ? ICN::EMPTY_EVIL_MEDIUM_BUTTON : ICN::EMPTY_GOOD_MEDIUM_BUTTON;

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( baseIcnId, i );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "SAVE\nGAME" ), { 7, 5 }, { 6, 6 }, { 86, 48 }, buttonFontColor );

                break;
            }
            case ICN::BUTTON_LOAD_GAME_EVIL:
            case ICN::BUTTON_LOAD_GAME_GOOD: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_LOAD_GAME_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::CPANELE : ICN::CPANEL, 2 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::CPANELE : ICN::CPANEL, 3 );
                    break;
                }

                const int baseIcnId = isEvilInterface ? ICN::EMPTY_EVIL_MEDIUM_BUTTON : ICN::EMPTY_GOOD_MEDIUM_BUTTON;

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( baseIcnId, i );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "LOAD\nGAME" ), { 7, 5 }, { 6, 6 }, { 86, 48 }, buttonFontColor );

                break;
            }
            case ICN::BUTTON_INFO_EVIL:
            case ICN::BUTTON_INFO_GOOD: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_INFO_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::APANELE : ICN::APANEL, 4 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::APANELE : ICN::APANEL, 5 );
                    break;
                }

                const int baseIcnId = isEvilInterface ? ICN::EMPTY_EVIL_MEDIUM_BUTTON : ICN::EMPTY_GOOD_MEDIUM_BUTTON;

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( baseIcnId, i );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "INFO" ), { 7, 5 }, { 6, 6 }, { 86, 48 }, buttonFontColor );

                break;
            }
            case ICN::BUTTON_QUIT_EVIL:
            case ICN::BUTTON_QUIT_GOOD: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_QUIT_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::CPANELE : ICN::CPANEL, 6 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::CPANELE : ICN::CPANEL, 7 );
                    break;
                }

                const int baseIcnId = isEvilInterface ? ICN::EMPTY_EVIL_MEDIUM_BUTTON : ICN::EMPTY_GOOD_MEDIUM_BUTTON;

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( baseIcnId, i );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "QUIT" ), { 7, 5 }, { 6, 6 }, { 86, 48 }, buttonFontColor );

                break;
            }
            case ICN::BUTTON_SMALL_CANCEL_GOOD:
            case ICN::BUTTON_SMALL_CANCEL_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_CANCEL_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::CPANELE : ICN::CPANEL, 8 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::CPANELE : ICN::CPANEL, 9 );

                    // To properly generate shadows and Blit the button we need to make transparent pixels in its released state the same as in the pressed state.
                    CopyTransformLayer( _icnVsSprite[id][0], _icnVsSprite[id][1] );

                    break;
                }

                const int32_t textWidth = 86;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "CANCEL" ), isEvilInterface );

                break;
            }
            case ICN::BUTTON_SMALL_OKAY_GOOD:
            case ICN::BUTTON_SMALL_OKAY_EVIL:
            case ICN::BUTTON_SMALLER_OKAY_GOOD:
            case ICN::BUTTON_SMALLER_OKAY_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_OKAY_EVIL || id == ICN::BUTTON_SMALLER_OKAY_EVIL );
                const bool isSameResourceAsLanguage = useOriginalResources();

                if ( isSameResourceAsLanguage && ( id == ICN::BUTTON_SMALL_OKAY_EVIL || id == ICN::BUTTON_SMALL_OKAY_GOOD ) ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::SPANBTNE : ICN::SPANBTN, 0 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::SPANBTNE : ICN::SPANBTN, 1 );

                    // To properly generate shadows and Blit the button we need to make transparent pixels in its released state the same as in the pressed state.
                    CopyTransformLayer( _icnVsSprite[id][0], _icnVsSprite[id][1] );

                    break;
                }
                if ( isSameResourceAsLanguage && ( id == ICN::BUTTON_SMALLER_OKAY_EVIL || id == ICN::BUTTON_SMALLER_OKAY_GOOD ) ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::WINCMBBE : ICN::WINCMBTB, 0 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::WINCMBBE : ICN::WINCMBTB, 1 );
                    break;
                }

                int32_t textWidth = 86;
                const char * text = gettext_noop( "OKAY" );
                if ( id == ICN::BUTTON_SMALLER_OKAY_EVIL || id == ICN::BUTTON_SMALLER_OKAY_GOOD ) {
                    textWidth = 70;
                    text = gettext_noop( "smallerButton|OKAY" );
                }

                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, text, isEvilInterface );

                break;
            }
            case ICN::BUTTON_SMALL_ACCEPT_GOOD:
            case ICN::BUTTON_SMALL_ACCEPT_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_ACCEPT_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::SURRENDE : ICN::SURRENDR, 0 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::SURRENDE : ICN::SURRENDR, 1 );

                    // To properly generate shadows and Blit the button we need to make transparent pixels in its released state the same as in the pressed state.
                    CopyTransformLayer( _icnVsSprite[id][0], _icnVsSprite[id][1] );

                    break;
                }

                const int32_t textWidth = 108;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "ACCEPT" ), isEvilInterface );

                break;
            }
            case ICN::BUTTON_SMALL_DECLINE_GOOD:
            case ICN::BUTTON_SMALL_DECLINE_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_DECLINE_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::SURRENDE : ICN::SURRENDR, 2 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::SURRENDE : ICN::SURRENDR, 3 );

                    // To properly generate shadows and Blit the button we need to make transparent pixels in its released state the same as in the pressed state.
                    CopyTransformLayer( _icnVsSprite[id][0], _icnVsSprite[id][1] );

                    break;
                }

                const int32_t textWidth = 108;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "DECLINE" ), isEvilInterface );

                break;
            }
            case ICN::BUTTON_SMALL_LEARN_GOOD:
            case ICN::BUTTON_SMALL_LEARN_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_LEARN_EVIL );
                const int baseIcnID = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( baseIcnID, 9 );
                    _icnVsSprite[id][1] = GetICN( baseIcnID, 10 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( baseIcnID, 11 + i );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "LEARN" ), { 7, 5 }, { 5, 6 }, { 86, 16 }, buttonFontColor );

                break;
            }
            case ICN::BUTTON_SMALL_TRADE_GOOD:
            case ICN::BUTTON_SMALL_TRADE_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_TRADE_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::TRADPOSE : ICN::TRADPOST, 15 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::TRADPOSE : ICN::TRADPOST, 16 );
                    break;
                }

                const int baseIcnID = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    const Sprite & originalButton = GetICN( baseIcnID, 11 + i );
                    const int extendedAmount = 14;
                    out.resize( originalButton.width() + extendedAmount, originalButton.height() );
                    out.reset();

                    const int widthEndPart = 4 + 2 * i;
                    const int widthFirstPart = originalButton.width() - widthEndPart;
                    const int widthMiddlePart = extendedAmount + i;
                    const int offsetXEndPart = widthFirstPart + widthMiddlePart;
                    const int startOffsetXMiddlePart = 36;

                    // Copy left main body of button.
                    fheroes2::Copy( originalButton, 0, 0, out, 0, 0, widthFirstPart, originalButton.height() );

                    // Copy middle body of button.
                    fheroes2::Copy( originalButton, originalButton.width() - startOffsetXMiddlePart, 0, out, widthFirstPart, 0, widthMiddlePart,
                                    originalButton.height() );

                    // Copy terminating right margin of the button.
                    fheroes2::Copy( originalButton, originalButton.width() - widthEndPart, 0, out, offsetXEndPart, 0, widthEndPart, originalButton.height() );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "TRADE" ), { 6, 5 }, { 4, 6 }, { 100, 16 }, buttonFontColor );

                break;
            }
            case ICN::BUTTON_SMALL_YES_GOOD:
            case ICN::BUTTON_SMALL_YES_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_YES_EVIL );
                const int baseIcnID = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( baseIcnID, 5 );
                    _icnVsSprite[id][1] = GetICN( baseIcnID, 6 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( baseIcnID, 11 + i );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "YES" ), { 6, 5 }, { 5, 6 }, { 86, 16 }, buttonFontColor );

                break;
            }
            case ICN::BUTTON_SMALL_NO_GOOD:
            case ICN::BUTTON_SMALL_NO_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_NO_EVIL );
                const int baseIcnID = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( baseIcnID, 7 );
                    _icnVsSprite[id][1] = GetICN( baseIcnID, 8 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( baseIcnID, 11 + i );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "NO" ), { 6, 5 }, { 5, 6 }, { 86, 16 }, buttonFontColor );

                break;
            }
            case ICN::BUTTON_SMALL_EXIT_GOOD:
            case ICN::BUTTON_SMALL_EXIT_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_EXIT_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::VIEWARME : ICN::VIEWARMY, 3 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::VIEWARME : ICN::VIEWARMY, 4 );
                    break;
                }

                const int32_t textWidth = 85;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "EXIT" ), isEvilInterface );

                break;
            }
            case ICN::BUTTON_EXIT_HEROES_MEETING: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::SWAPBTN, 0 );
                    _icnVsSprite[id][1] = GetICN( ICN::SWAPBTN, 1 );
                    // fix some wrong pixels in the original pressed state
                    setButtonCornersTransparent( _icnVsSprite[id][1] );
                    break;
                }

                // The heroes meeting screen has an embedded shadow so the button needs to be fixed at the same size as the original one.
                // TODO: Remove the embedded shadow and button in the heroes meeting screen and use getTextAdaptedButton() instead.
                const int32_t textWidth = 70;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "smallerButton|EXIT" ), false );

                break;
            }
            case ICN::BUTTON_EXIT_TOWN: {
                std::vector<fheroes2::Sprite> & buttonStates = _icnVsSprite[id];
                buttonStates.resize( 2 );

                if ( useOriginalResources() ) {
                    buttonStates[0] = GetICN( ICN::TREASURY, 1 );
                    buttonStates[1] = GetICN( ICN::TREASURY, 2 );
                    break;
                }

                // Needs to be generated from original assets because it needs the black background from the pressed state.
                // TODO: Make a way to generate buttons with black background since it is needed for MAX and EXIT in the Well and Guilds.
                for ( int32_t i = 0; i < static_cast<int32_t>( buttonStates.size() ); ++i ) {
                    Sprite & out = buttonStates[i];
                    out = GetICN( ICN::TREASURY, 1 + i );

                    // clean the button.
                    Fill( out, 6 - i, 4 + i, 70, 17, getButtonFillingColor( i == 0 ) );
                }

                const int32_t textWidth = 70;
                renderTextOnButton( buttonStates[0], buttonStates[1], gettext_noop( "smallerButton|EXIT" ), { 7, 5 }, { 6, 6 },
                                    { textWidth, fheroes2::getFontHeight( fheroes2::FontSize::BUTTON_RELEASED ) }, fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_EXIT_PUZZLE_DDOOR_GOOD:
            case ICN::BUTTON_EXIT_PUZZLE_DDOOR_EVIL: {
                std::vector<fheroes2::Sprite> & buttonStates = _icnVsSprite[id];
                buttonStates.resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_EXIT_PUZZLE_DDOOR_EVIL );
                const int originalButtonICN = isEvilInterface ? ICN::LGNDXTRE : ICN::LGNDXTRA;

                if ( useOriginalResources() ) {
                    buttonStates[0] = GetICN( originalButtonICN, 4 );
                    buttonStates[1] = GetICN( originalButtonICN, 5 );
                    break;
                }

                // Needs to be generated from original assets because the background has a much darker shadow than normal.
                // TODO: Make the button generated as normal after removing the embedded shadow on the background.
                for ( int32_t i = 0; i < static_cast<int32_t>( buttonStates.size() ); ++i ) {
                    Sprite & out = buttonStates[i];
                    out = GetICN( originalButtonICN, 4 + i );

                    // clean the button.
                    Fill( out, 6 - i, 4 + i, 71 - i, 17, getButtonFillingColor( i == 0, !isEvilInterface ) );
                }

                const int32_t textWidth = 71;
                renderTextOnButton( buttonStates[0], buttonStates[1], gettext_noop( "smallerButton|EXIT" ), { 6, 5 }, { 5, 6 },
                                    { textWidth, fheroes2::getFontHeight( fheroes2::FontSize::BUTTON_RELEASED ) },
                                    isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_SMALL_DISMISS_GOOD:
            case ICN::BUTTON_SMALL_DISMISS_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_DISMISS_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::VIEWARME : ICN::VIEWARMY, 1 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::VIEWARME : ICN::VIEWARMY, 2 );
                    break;
                }

                const int32_t textWidth = 110;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "DISMISS" ), isEvilInterface );

                break;
            }
            case ICN::BUTTON_SMALL_UPGRADE_GOOD:
            case ICN::BUTTON_SMALL_UPGRADE_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_UPGRADE_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::VIEWARME : ICN::VIEWARMY, 5 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::VIEWARME : ICN::VIEWARMY, 6 );
                    break;
                }

                const int32_t textWidth = 110;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "UPGRADE" ), isEvilInterface );

                break;
            }
            case ICN::BUTTON_SMALL_RESTART_GOOD:
            case ICN::BUTTON_SMALL_RESTART_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_RESTART_EVIL );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( isEvilInterface ? ICN::NON_UNIFORM_EVIL_RESTART_BUTTON : ICN::NON_UNIFORM_GOOD_RESTART_BUTTON, 0 );
                    _icnVsSprite[id][1] = GetICN( isEvilInterface ? ICN::NON_UNIFORM_EVIL_RESTART_BUTTON : ICN::NON_UNIFORM_GOOD_RESTART_BUTTON, 1 );
                    break;
                }

                const int32_t textWidth = 102;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "RESTART" ), isEvilInterface );

                break;
            }
            case ICN::BUTTON_KINGDOM_EXIT: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::OVERVIEW, 4 );
                    _icnVsSprite[id][1] = GetICN( ICN::OVERVIEW, 5 );
                    break;
                }

                // Needs to be generated from original assets because the pressed state is 1px wider than normal.
                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::OVERVIEW, 4 + i );

                    // clean the button.
                    Fill( out, 6 - i, 4 + i, 89, 16, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "EXIT" ), { 6, 5 }, { 5, 6 }, { 89, 16 }, fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_KINGDOM_HEROES: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::OVERVIEW, 0 );
                    _icnVsSprite[id][1] = GetICN( ICN::OVERVIEW, 1 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::OVERVIEW, 0 + i );

                    // clean the button.
                    Fill( out, 5, 10 + i, 89, 20, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "HEROES" ), { 6, 5 }, { 5, 6 }, { 89, 34 }, fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_KINGDOM_TOWNS: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::OVERVIEW, 2 );
                    _icnVsSprite[id][1] = GetICN( ICN::OVERVIEW, 3 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::OVERVIEW, 2 + i );

                    // clean the button.
                    Fill( out, 6, 6 + i, 89 - i, 30, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "TOWNS/\nCASTLES" ), { 6, 5 }, { 5, 6 }, { 90, 34 },
                                    fheroes2::FontColor::WHITE );

                break;
            }

            case ICN::BUTTON_MAPSIZE_SMALL: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::REQUESTS, 9 );
                    _icnVsSprite[id][1] = GetICN( ICN::REQUESTS, 10 );
                    break;
                }

                const int32_t textWidth = 46;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "S" ), false );

                break;
            }
            case ICN::BUTTON_MAPSIZE_MEDIUM: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::REQUESTS, 11 );
                    _icnVsSprite[id][1] = GetICN( ICN::REQUESTS, 12 );
                    break;
                }

                const int32_t textWidth = 46;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "M" ), false );

                break;
            }
            case ICN::BUTTON_MAPSIZE_LARGE: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::REQUESTS, 13 );
                    _icnVsSprite[id][1] = GetICN( ICN::REQUESTS, 14 );
                    break;
                }

                const int32_t textWidth = 46;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "L" ), false );

                break;
            }
            case ICN::BUTTON_MAPSIZE_XLARGE: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::REQUESTS, 15 );
                    _icnVsSprite[id][1] = GetICN( ICN::REQUESTS, 16 );
                    break;
                }

                const int32_t textWidth = 46;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "X-L" ), false );

                break;
            }
            case ICN::BUTTON_MAPSIZE_ALL: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::REQUESTS, 17 );
                    _icnVsSprite[id][1] = GetICN( ICN::REQUESTS, 18 );
                    break;
                }

                const int32_t textWidth = 58;
                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], textWidth, gettext_noop( "ALL" ), false );

                break;
            }
            case ICN::BUTTON_MAP_SELECT: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::NGEXTRA, 64 );
                    _icnVsSprite[id][1] = GetICN( ICN::NGEXTRA, 65 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];

                    const Sprite & originalButton = GetICN( ICN::NGEXTRA, 64 + i );
                    const int32_t originalHeight = originalButton.height();
                    const int32_t originalWidth = originalButton.width();
                    const int32_t extensionWidth = 5;
                    out.resize( originalWidth + extensionWidth, originalHeight );
                    out.reset();

                    const int32_t rightPartWidth = 3 + i;
                    const int32_t leftPartWidth = originalWidth - rightPartWidth;

                    // copy left main body of button.
                    fheroes2::Copy( originalButton, 0, 0, out, 0, 0, leftPartWidth, originalHeight );

                    // copy middle extending part of button.
                    fheroes2::Copy( originalButton, 9, 0, out, leftPartWidth, 0, extensionWidth, originalHeight );

                    // copy terminating right margin of the button.
                    fheroes2::Copy( originalButton, leftPartWidth, 0, out, leftPartWidth + extensionWidth, 0, rightPartWidth, originalHeight );

                    // clean the button.
                    const int32_t leftMarginWidth = 6 - i;
                    Fill( out, leftMarginWidth, 2 + 2 * i, out.width() - rightPartWidth - leftMarginWidth, 15 - i, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "SELECT" ), { 7, 3 }, { 6, 4 }, { 76, 15 }, fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_STANDARD_GAME: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::BTNNEWGM, 0 );
                    _icnVsSprite[id][1] = GetICN( ICN::BTNNEWGM, 1 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "STANDARD\nGAME" ), { 12, 5 }, { 11, 6 }, { 120, 47 },
                                    fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_CAMPAIGN_GAME: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::BTNNEWGM, 2 );
                    _icnVsSprite[id][1] = GetICN( ICN::BTNNEWGM, 3 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "CAMPAIGN\nGAME" ), { 11, 5 }, { 10, 6 }, { 119, 47 },
                                    fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_MULTIPLAYER_GAME: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::BTNNEWGM, 4 );
                    _icnVsSprite[id][1] = GetICN( ICN::BTNNEWGM, 5 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "MULTI-\nPLAYER\nGAME" ), { 12, 5 }, { 11, 6 }, { 117, 47 },
                                    fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_LARGE_CANCEL: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::BTNNEWGM, 6 );
                    _icnVsSprite[id][1] = GetICN( ICN::BTNNEWGM, 7 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "CANCEL" ), { 12, 5 }, { 11, 6 }, { 117, 47 }, fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_LARGE_CONFIG: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::BTNDCCFG, 4 );
                    _icnVsSprite[id][1] = GetICN( ICN::BTNDCCFG, 5 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "CONFIG" ), { 12, 5 }, { 11, 6 }, { 117, 47 }, fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_ORIGINAL_CAMPAIGN: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::X_LOADCM, 0 );
                    _icnVsSprite[id][1] = GetICN( ICN::X_LOADCM, 1 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "ORIGINAL\nCAMPAIGN" ), { 12, 5 }, { 11, 6 }, { 117, 47 },
                                    fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_EXPANSION_CAMPAIGN: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::X_LOADCM, 2 );
                    _icnVsSprite[id][1] = GetICN( ICN::X_LOADCM, 3 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "EXPANSION\nCAMPAIGN" ), { 12, 5 }, { 11, 6 }, { 117, 47 },
                                    fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_HOT_SEAT: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::BTNMP, 0 );
                    _icnVsSprite[id][1] = GetICN( ICN::BTNMP, 1 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "HOT SEAT" ), { 11, 5 }, { 10, 6 }, { 120, 47 }, fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_2_PLAYERS: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::BTNHOTST, 0 );
                    _icnVsSprite[id][1] = GetICN( ICN::BTNHOTST, 1 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "2 PLAYERS" ), { 11, 5 }, { 10, 6 }, { 120, 47 },
                                    fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_3_PLAYERS: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::BTNHOTST, 2 );
                    _icnVsSprite[id][1] = GetICN( ICN::BTNHOTST, 3 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "3 PLAYERS" ), { 11, 5 }, { 10, 6 }, { 120, 47 },
                                    fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_4_PLAYERS: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::BTNHOTST, 4 );
                    _icnVsSprite[id][1] = GetICN( ICN::BTNHOTST, 5 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "4 PLAYERS" ), { 11, 5 }, { 10, 6 }, { 120, 47 },
                                    fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_5_PLAYERS: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::BTNHOTST, 6 );
                    _icnVsSprite[id][1] = GetICN( ICN::BTNHOTST, 7 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "5 PLAYERS" ), { 11, 5 }, { 10, 6 }, { 120, 47 },
                                    fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BUTTON_6_PLAYERS: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::BTNHOTST, 8 );
                    _icnVsSprite[id][1] = GetICN( ICN::BTNHOTST, 9 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNCOM, i );
                    // clean button.
                    Fill( out, 13, 11, 113, 31, getButtonFillingColor( i == 0 ) );
                }

                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "6 PLAYERS" ), { 11, 5 }, { 10, 6 }, { 120, 47 },
                                    fheroes2::FontColor::WHITE );

                break;
            }
            case ICN::BTNGIFT_GOOD:
            case ICN::BTNGIFT_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BTNGIFT_EVIL );
                const int baseIcnId = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( baseIcnId, 11 + i );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "GIFT" ), { 5, 5 }, { 4, 6 }, { 88, 16 }, buttonFontColor );

                break;
            }
            case ICN::BUYMAX: {
                _icnVsSprite[id].resize( 2 );

                getTextAdaptedButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "MAX" ), ICN::EMPTY_GUILDWELL_BUTTON, ICN::UNKNOWN );

                break;
            }
            case ICN::BUTTON_GUILDWELL_EXIT: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( ICN::WELLXTRA, 0 );
                    _icnVsSprite[id][1] = GetICN( ICN::WELLXTRA, 1 );
                    break;
                }
                fheroes2::getTextAdaptedButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "EXIT" ), ICN::EMPTY_GUILDWELL_BUTTON, ICN::UNKNOWN );

                break;
            }
            case ICN::BUTTON_VIEWWORLD_EXIT_GOOD:
            case ICN::BUTTON_VIEWWORLD_EXIT_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::BUTTON_VIEWWORLD_EXIT_EVIL );
                const int baseIcnId = isEvilInterface ? ICN::LGNDXTRE : ICN::LGNDXTRA;

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( baseIcnId, 0 + i );
                    // clean the button.
                    Fill( out, 27 - i, 4 + i, 25, 26, getButtonFillingColor( i == 0, !isEvilInterface ) );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "EXIT" ), { 6, 5 }, { 5, 6 }, { 71, 27 }, buttonFontColor );

                break;
            }
            case ICN::GOOD_CAMPAIGN_BUTTONS:
            case ICN::EVIL_CAMPAIGN_BUTTONS: {
                _icnVsSprite[id].resize( 10 );

                if ( useOriginalResources() ) {
                    const bool isEvilInterface = id == ICN::EVIL_CAMPAIGN_BUTTONS;
                    const int originalIcnId = isEvilInterface ? ICN::CAMPXTRE : ICN::CAMPXTRG;
                    const int buttonBackground = isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK;

                    // The evil buttons' released state are 2 pixels wider.
                    const int offsetEvilX = isEvilInterface ? 2 : 0;
                    // remove embedded shadows so that we can generate shadows with our own code later
                    for ( uint32_t i = 0; i < 8; i += 2 ) {
                        // released
                        const Sprite & originalReleased = GetICN( originalIcnId, i );

                        Sprite & released = _icnVsSprite[id][i];
                        released.resize( originalReleased.width() - 6 + offsetEvilX, originalReleased.height() - 8 );
                        released.reset();

                        Copy( originalReleased, 6 - offsetEvilX, 0, released, 0, 0, originalReleased.width() - 1, originalReleased.height() - 8 );

                        // pressed
                        const Sprite & originalPressed = GetICN( originalIcnId, i + 1 );

                        Sprite & pressed = _icnVsSprite[id][i + 1];
                        pressed.resize( originalPressed.width(), originalPressed.height() );
                        pressed.reset();

                        Copy( originalPressed, 0, 1, pressed, 0, 1, originalPressed.width() - 1, originalPressed.height() );
                        setButtonCornersTransparent( released );
                        fheroes2::makeTransparentBackground( released, pressed, buttonBackground );
                    }
                    // Generate the DIFFICULTY button because it is not present in the original resources
                    fheroes2::getTextAdaptedButton( _icnVsSprite[id][8], _icnVsSprite[id][9], gettext_noop( "DIFFICULTY" ),
                                                    isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON, buttonBackground );
                    break;
                }
                createCampaignButtonSet( id, { gettext_noop( "VIEW INTRO" ), gettext_noop( "RESTART" ), gettext_noop( "OKAY" ), gettext_noop( "CANCEL" ),
                                               gettext_noop( "DIFFICULTY" ) } );

                break;
            }
            case ICN::POL_CAMPAIGN_BUTTONS: {
                _icnVsSprite[id].resize( 10 );

                const int baseIcnId = ICN::X_CMPBTN;

                if ( useOriginalResources() ) {
                    for ( uint32_t i = 0; i < 8; i += 2 ) {
                        // released
                        const Sprite & originalReleased = GetICN( baseIcnId, i );

                        Sprite & released = _icnVsSprite[id][i];
                        released.resize( originalReleased.width() + 1, originalReleased.height() + 1 );
                        released.reset();

                        Copy( originalReleased, 0, 0, released, 1, 0, originalReleased.width(), originalReleased.height() );
                        const Sprite & originalPressed = GetICN( baseIcnId, i + 1 );
                        // the released state is missing the darker borders of the pressed state
                        Copy( originalPressed, 0, 0, released, 0, 1, 1, originalPressed.height() );
                        Copy( originalPressed, 0, 2, released, 0, originalPressed.height() - 1, 1, 2 );
                        Copy( originalPressed, 1, originalPressed.height() - 1, released, 1, originalPressed.height(), originalPressed.width(), 1 );
                        Copy( originalPressed, 0, 2, released, 1, originalPressed.height(), 1, 1 );
                        Copy( originalReleased, 0, 2, released, 1, originalPressed.height() - 2, 1, 1 );
                        Copy( originalReleased, 0, 2, released, 2, originalPressed.height() - 1, 1, 1 );
                        Copy( originalReleased, 0, 2, released, 1, originalPressed.height() - 1, 1, 1 );
                        Copy( originalReleased, 1, 2, released, 2, originalPressed.height() - 2, 1, 1 );

                        // pressed state
                        Sprite & pressed = _icnVsSprite[id][i + 1];
                        pressed.resize( originalPressed.width() + 1, originalPressed.height() + 1 );
                        pressed.reset();

                        Copy( originalPressed, 0, 0, pressed, 0, 1, originalPressed.width(), originalPressed.height() );
                        // pressed state has incomplete lower left corner
                        Copy( originalPressed, 0, 2, pressed, 0, originalPressed.height() - 1, 1, 2 );
                        Copy( originalPressed, 0, 2, pressed, 1, originalPressed.height(), 1, 1 );
                        Copy( originalPressed, 1, 2, pressed, 1, originalPressed.height() - 1, 1, 1 );
                        _icnVsSprite[id][i + 1].setPosition( 0, 0 );

                        fheroes2::makeTransparentBackground( released, pressed, ICN::STONEBAK_SMALL_POL );
                    }
                    // generate the DIFFICULTY button as it is not present in the original resources
                    fheroes2::getTextAdaptedButton( _icnVsSprite[id][8], _icnVsSprite[id][9], gettext_noop( "DIFFICULTY" ), ICN::EMPTY_POL_BUTTON,
                                                    ICN::STONEBAK_SMALL_POL );
                    break;
                }
                createCampaignButtonSet( id, { gettext_noop( "VIEW INTRO" ), gettext_noop( "RESTART" ), gettext_noop( "OKAY" ), gettext_noop( "CANCEL" ),
                                               gettext_noop( "DIFFICULTY" ) } );

                break;
            }
            case ICN::BUTTON_SMALL_MIN_GOOD: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    // Write the "MIN" text on original assets ICNs.
                    for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                        _icnVsSprite[id][i] = GetICN( ICN::BUTTON_SMALL_MAX_GOOD, 0 + i );

                        // Clean the button text.
                        Blit( GetICN( ICN::SYSTEM, 11 + i ), 10 - i, 4 + i, _icnVsSprite[id][i], 6 - i, 4 + i, 50, 16 );
                    }

                    renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "MIN" ), { 6, 5 }, { 5, 6 }, { 52, 16 }, fheroes2::FontColor::WHITE );

                    break;
                }

                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], 61, gettext_noop( "MIN" ), false );

                break;
            }
            case ICN::BUTTON_SMALL_MIN_EVIL: {
                _icnVsSprite[id].resize( 2 );

                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], 61, gettext_noop( "MIN" ), true );

                break;
            }
            case ICN::BUTTON_SMALL_MAX_GOOD: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    // The original assets ICN contains button with shadow. We crop only the button.
                    _icnVsSprite[id][0] = fheroes2::Crop( GetICN( ICN::RECRUIT, 4 ), 5, 0, 60, 25 );
                    _icnVsSprite[id][1] = fheroes2::Crop( GetICN( ICN::RECRUIT, 5 ), 5, 0, 60, 25 );

                    // To properly generate shadows and Blit the button we need to make some pixels transparent.
                    for ( fheroes2::Sprite & image : _icnVsSprite[id] ) {
                        setButtonCornersTransparent( image );
                    }

                    break;
                }

                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], 61, gettext_noop( "MAX" ), false );

                break;
            }
            case ICN::BUTTON_SMALL_MAX_EVIL: {
                _icnVsSprite[id].resize( 2 );

                createNormalButton( _icnVsSprite[id][0], _icnVsSprite[id][1], 61, gettext_noop( "MAX" ), true );

                break;
            }
            case ICN::UNIFORM_EVIL_MAX_BUTTON:
            case ICN::UNIFORM_EVIL_MIN_BUTTON:
            case ICN::UNIFORM_GOOD_MAX_BUTTON:
            case ICN::UNIFORM_GOOD_MIN_BUTTON: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::UNIFORM_EVIL_MAX_BUTTON || id == ICN::UNIFORM_EVIL_MIN_BUTTON );
                const int baseIcnId = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];

                    const Sprite & originalButton = GetICN( ICN::RECRUIT, 4 + i );
                    out.resize( originalButton.width() + 4, originalButton.height() - 5 + i );
                    out.reset();

                    const Sprite & emptyButton = GetICN( baseIcnId, 11 + i );

                    // Copy left main body of button.
                    fheroes2::Copy( emptyButton, 0, 0, out, 0, 0, originalButton.width() - 3 + i, originalButton.height() - 5 + i );

                    // Copy terminating right margin of the button.
                    fheroes2::Copy( emptyButton, 89 + i, 0, out, 63 + i, 0, 7 - i, originalButton.height() - i );
                }

                const char * text( ( id == ICN::UNIFORM_GOOD_MIN_BUTTON || id == ICN::UNIFORM_EVIL_MIN_BUTTON ) ? "MIN" : "MAX" );

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( text ), { 6, 5 }, { 4, 6 }, { 62, 16 }, buttonFontColor );

                break;
            }
            case ICN::UNIFORM_GOOD_OKAY_BUTTON:
            case ICN::UNIFORM_EVIL_OKAY_BUTTON: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::UNIFORM_EVIL_OKAY_BUTTON );
                const int baseIcnId = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( baseIcnId, 1 );
                    _icnVsSprite[id][1] = GetICN( baseIcnId, 2 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( baseIcnId, 11 + i );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "OKAY" ), { 6, 5 }, { 5, 6 }, { 86, 16 }, buttonFontColor );

                break;
            }
            case ICN::UNIFORM_GOOD_CANCEL_BUTTON:
            case ICN::UNIFORM_EVIL_CANCEL_BUTTON: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::UNIFORM_EVIL_CANCEL_BUTTON );
                const int baseIcnId = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( baseIcnId, 3 );
                    _icnVsSprite[id][1] = GetICN( baseIcnId, 4 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( baseIcnId, 11 + i );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "CANCEL" ), { 7, 5 }, { 6, 6 }, { 86, 16 }, buttonFontColor );

                break;
            }
            case ICN::UNIFORM_GOOD_EXIT_BUTTON:
            case ICN::UNIFORM_EVIL_EXIT_BUTTON: {
                _icnVsSprite[id].resize( 2 );

                const bool isEvilInterface = ( id == ICN::UNIFORM_EVIL_EXIT_BUTTON );
                const int baseIcnId = isEvilInterface ? ICN::TRADPOSE : ICN::TRADPOST;

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( baseIcnId, 17 );
                    _icnVsSprite[id][1] = GetICN( baseIcnId, 18 );
                    break;
                }

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM, 11 + i );
                }

                const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "EXIT" ), { 7, 5 }, { 6, 6 }, { 86, 16 }, buttonFontColor );

                break;
            }
            case ICN::BUTTON_VERTICAL_DISMISS: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    // The original DISMISS button has a broken transform layer and thinner pressed state than released state,
                    // so we use our empty vertical button as a template
                    _icnVsSprite[id][0] = GetICN( ICN::EMPTY_VERTICAL_GOOD_BUTTON, 0 );
                    _icnVsSprite[id][1] = GetICN( ICN::EMPTY_VERTICAL_GOOD_BUTTON, 1 );

                    // Copy the DISMISS text back from the original button
                    const Sprite & originalReleased = GetICN( ICN::HSBTNS, 0 );
                    const Sprite & originalPressed = GetICN( ICN::HSBTNS, 1 );

                    Copy( originalReleased, 9, 2, _icnVsSprite[id][0], 5, 2, 21, 112 );
                    Copy( originalPressed, 9, 5, _icnVsSprite[id][1], 4, 5, 19, 111 );

                    fheroes2::makeTransparentBackground( _icnVsSprite[id][0], _icnVsSprite[id][1], ICN::REDBAK_SMALL_VERTICAL );

                    break;
                }

                // We need to temporarily remove the letter specific X offsets in the font because if not the letters will
                // be off-centered when we are displaying one letter per line
                const ButtonFontOffsetRestorer fontReleased( _icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], -1 );
                const ButtonFontOffsetRestorer fontPressed( _icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED], -1 );
                getTextAdaptedButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "D\nI\nS\nM\nI\nS\nS" ), ICN::EMPTY_VERTICAL_GOOD_BUTTON,
                                      ICN::REDBAK_SMALL_VERTICAL );

                break;
            }
            case ICN::BUTTON_VERTICAL_EXIT: {
                _icnVsSprite[id].resize( 2 );

                if ( useOriginalResources() ) {
                    // The original EXIT button has a broken transform layer and we need to remove the shadows,
                    // so we use our empty vertical button as a template
                    _icnVsSprite[id][0] = GetICN( ICN::EMPTY_VERTICAL_GOOD_BUTTON, 0 );
                    _icnVsSprite[id][1] = GetICN( ICN::EMPTY_VERTICAL_GOOD_BUTTON, 1 );

                    // Copy the EXIT text back from the original button
                    const Sprite & originalReleased = GetICN( ICN::HSBTNS, 2 );
                    const Sprite & originalPressed = GetICN( ICN::HSBTNS, 3 );

                    Copy( originalReleased, 4, 2, _icnVsSprite[id][0], 5, 2, 21, 112 );
                    Copy( originalPressed, 3, 5, _icnVsSprite[id][1], 4, 5, 19, 111 );

                    fheroes2::makeTransparentBackground( _icnVsSprite[id][0], _icnVsSprite[id][1], ICN::REDBAK_SMALL_VERTICAL );

                    break;
                }

                // We need to temporarily remove the letter specific X offsets in the font because if not the letters will
                // be off-centered when we are displaying one letter per line
                const ButtonFontOffsetRestorer fontReleased( _icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], -1 );
                const ButtonFontOffsetRestorer fontPressed( _icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED], -1 );
                getTextAdaptedButton( _icnVsSprite[id][0], _icnVsSprite[id][1], gettext_noop( "E\nX\nI\nT" ), ICN::EMPTY_VERTICAL_GOOD_BUTTON,
                                      ICN::REDBAK_SMALL_VERTICAL );

                break;
            }
            case ICN::BUTTON_HSCORES_VERTICAL_CAMPAIGN:
            case ICN::BUTTON_HSCORES_VERTICAL_EXIT:
            case ICN::BUTTON_HSCORES_VERTICAL_STANDARD: {
                _icnVsSprite[id].resize( 2 );

                const int originalID = ICN::HISCORE;
                uint32_t originalICNIndex = 0;
                if ( id == ICN::BUTTON_HSCORES_VERTICAL_STANDARD ) {
                    originalICNIndex = 2;
                }
                else if ( id == ICN::BUTTON_HSCORES_VERTICAL_EXIT ) {
                    originalICNIndex = 4;
                }
                else {
                    assert( id == ICN::BUTTON_HSCORES_VERTICAL_CAMPAIGN );
                }

                if ( useOriginalResources() ) {
                    _icnVsSprite[id][0] = GetICN( originalID, originalICNIndex );
                    _icnVsSprite[id][1] = GetICN( originalID, originalICNIndex + 1 );
                    break;
                }

                for ( size_t i = 0; i < _icnVsSprite[id].size(); ++i ) {
                    const Sprite & originalButton = GetICN( originalID, originalICNIndex + static_cast<uint32_t>( i ) );
                    Sprite & out = _icnVsSprite[id][i];

                    out = originalButton;
                    // Clean the button
                    Fill( out, 4 - static_cast<int32_t>( i ), 4 + static_cast<int32_t>( i ), 19, 123, getButtonFillingColor( i == 0 ) );
                }

                const char * buttonText;

                if ( id == ICN::BUTTON_HSCORES_VERTICAL_CAMPAIGN ) {
                    buttonText = gettext_noop( "C\nA\nM\nP\nA\nI\nG\nN" );
                }
                else if ( id == ICN::BUTTON_HSCORES_VERTICAL_STANDARD ) {
                    buttonText = gettext_noop( "S\nT\nA\nN\nD\nA\nR\nD" );
                }
                else {
                    buttonText = gettext_noop( "E\nX\nI\nT" );
                }

                ButtonFontOffsetRestorer fontRestorerReleased( _icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], -1 );
                ButtonFontOffsetRestorer fontRestorerPressed( _icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED], -1 );
                renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], buttonText, { 4, 4 }, { 3, 5 }, { 21, 124 }, fheroes2::FontColor::WHITE );

                break;
            }
            default:
                // You're calling this function for non-specified ICN id. Check your logic!
                // Did you add a new image for one language without generating a default
                // for other languages?
                assert( 0 );
                break;
            }
        }

        bool generateGermanSpecificImages( const int id )
        {
            switch ( id ) {
            case ICN::BTNBATTLEONLY:
                _icnVsSprite[id].resize( 2 );
                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNNEWGM, 6 + i );
                    // Clean the button
                    Fill( out, 25, 18, 88, 23, getButtonFillingColor( i == 0 ) );
                    // Add 'K'
                    Blit( GetICN( ICN::BTNDCCFG, 4 + i ), 34 - i, 23, out, 40 - i, 23, 12, 14 );
                    //'Add 'A'
                    Blit( GetICN( ICN::BTNNEWGM, 4 + i ), 56 - i, 23, out, 52 - i, 23, 13, 14 );
                    Blit( out, 20, 20, out, 52 - i + 12, 25, 3, 3 );
                    // Add 'M'
                    Blit( GetICN( ICN::BTNNEWGM, 4 + i ), 39 - i, 8, out, 65 - i, 23, 14, 14 );
                    // Add 'F'
                    Blit( GetICN( ICN::BTNDCCFG, 4 + i ), 70 - i, 23, out, 87 - i, 23, 10, 14 );
                    // Add 'P'
                    Blit( GetICN( ICN::BTNNEWGM, 4 + i ), 36 - i, 23, out, 78 - i, 23, 10, 14 );
                }
                return true;
            case ICN::BUTTON_SMALL_MIN_GOOD:
                _icnVsSprite[id].resize( 2 );
                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::RECRUIT, 4 + i );
                    // clean the button
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 10, 6 + i, out, 30 - 2 * i, 5 + i, 31, 15 );
                    // add 'IN'
                    Copy( GetICN( ICN::APANEL, 4 + i ), 23 - i, 22 + i, out, 33 - i, 6 + i, 8, 14 ); // letter 'I'
                    Copy( GetICN( ICN::APANEL, 4 + i ), 31 - i, 22 + i, out, 44 - i, 6 + i, 17, 14 ); // letter 'N'
                }
                return true;
            default:
                break;
            }
            return false;
        }

        bool generateFrenchSpecificImages( const int id )
        {
            switch ( id ) {
            case ICN::BTNBATTLEONLY:
                _icnVsSprite[id].resize( 2 );
                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNNEWGM, 6 + i );
                    // Clean the button
                    Fill( out, 25, 18, 88, 23, getButtonFillingColor( i == 0 ) );

                    const int32_t secondLine = 28;
                    // Add 'MODE'
                    Blit( GetICN( ICN::BTNNEWGM, 4 + i ), 40 - i, 13, out, 45 - i, 13, 50, 15 );
                    // Clean up 'MODE'
                    Blit( GetICN( ICN::BTNEMAIN, 0 + i ), 114 - i, 18, out, 94 - i, 18, 1, 10 );
                    // Add 'BA'
                    Blit( GetICN( ICN::BTNBAUD, 2 + i ), 42 - i, 28, out, 28 - i, secondLine, 22, 15 );
                    // Clean up 'BA'
                    Blit( GetICN( ICN::BTNBAUD, 2 + i ), 42 - i, 31, out, 39 - i, secondLine, 1, 1 );
                    Blit( GetICN( ICN::BTNBAUD, 2 + i ), 39 - i, 31, out, 49 - i, secondLine + 4, 1, 2 );
                    // Add 'T'
                    Blit( GetICN( ICN::BTNDC, 2 + i ), 89 - i, 21, out, 50 - i, secondLine, 12, 15 );
                    // Clean up 'AT'
                    Blit( GetICN( ICN::BTNDC, 2 + i ), 89 - i, 18, out, 50 - i, secondLine, 1, 1 );
                    Blit( GetICN( ICN::BTNDC, 2 + i ), 92 - ( 5 * i ), 27 - i, out, 49 - i, secondLine + 4 + i, 1, 3 );
                    // Add 'AI'.
                    Blit( GetICN( ICN::BTNMP, 6 + i ), 56 - i, 13, out, 62 - i, secondLine, 18, 15 );
                    // Clean up 'TA'
                    Blit( GetICN( ICN::BTNBAUD, 2 + i ), 51 - i, 40, out, 60 - i, secondLine + 12, 3, 3 );
                    // Add 'LLE'
                    Blit( GetICN( ICN::BTNEMAIN, 0 + i ), 85 - i, 13, out, 81 - i, secondLine, 31, 15 );
                    // Clean up "IL"
                    Blit( GetICN( ICN::BTNEMAIN, 0 + i ), 85 - i, 18, out, 81 - i, secondLine + 7, 1, 1 );
                    Blit( GetICN( ICN::BTNEMAIN, 0 + i ), 94 - i, 17, out, 80 - i, secondLine + 4, 2, 2 );
                    Blit( GetICN( ICN::BTNEMAIN, 0 + i ), 93 - i, 25, out, 79 - i, secondLine + 12, 3, 3 );
                    Blit( GetICN( ICN::BTNDC, 4 + i ), 23 - i, 8, out, 79 - i, secondLine + 5, 1, 10 );
                    Blit( GetICN( ICN::BTNMP, 6 + i ), 73 - i, 22, out, 79 - i, secondLine + 9, 1, 1 );
                }
                return true;
            case ICN::BTNGIFT_GOOD:
                _icnVsSprite[id].resize( 2 );
                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::TRADPOST, 17 + i );
                    // clean the button
                    Fill( out, 33, 5, 31, 16, getButtonFillingColor( i == 0 ) );

                    const int32_t offsetY = 5;
                    // Add 'D'
                    const int32_t offsetXD = 14;
                    Blit( GetICN( ICN::CPANEL, 4 + i ), 48 - i, 28 + i, out, offsetXD - i, offsetY + i, 10, 15 );
                    // Clean up 'D' and restore button ornament
                    Blit( GetICN( ICN::CPANEL, 4 + i ), 48 - i, 36, out, offsetXD - 1 - i, offsetY + 4 + i, 1, 1 );
                    Blit( GetICN( ICN::CPANEL, 4 + i ), 48 - i, 35, out, offsetXD - i, offsetY + 9 + i, 1, 2 );
                    Blit( GetICN( ICN::CPANEL, 4 + i ), 48 - i, 35, out, offsetXD - 1 - i, offsetY + 13 + i, 1, 1 );
                    Fill( out, offsetXD + 9 - i, offsetY + 13 + i, 1, 1, getButtonFillingColor( i == 0 ) );
                    Blit( GetICN( ICN::TRADPOST, 17 + i ), offsetXD, offsetY, out, offsetXD, offsetY, 1, 1 );
                    // Add 'O'
                    const int32_t offsetXO = 10;
                    Blit( GetICN( ICN::CAMPXTRG, i ), 40 - ( 7 * i ), 5 + i, out, offsetXD + offsetXO + 1 - i, offsetY + i, 13 - i, 15 );
                    // Clean up 'DO'
                    Blit( GetICN( ICN::CPANEL, 4 + i ), 51 - i, 34, out, offsetXD + offsetXO - i, offsetY + 5, 2, 2 );
                    Blit( GetICN( ICN::CPANEL, 4 + i ), 51 - i, 34, out, offsetXD + offsetXO - i, offsetY + 7, 1, 1 + i );
                    Blit( GetICN( ICN::CPANEL, 4 + i ), 55 - i, 28 + i, out, offsetXD + 9 - i, offsetY + 2 + i, 3, 3 );
                    Fill( out, offsetXD + 11 - i, offsetY + i, 2, 2, getButtonFillingColor( i == 0 ) );
                    // Add 'N'
                    const int32_t offsetXN = 13;
                    Blit( GetICN( ICN::TRADPOST, 17 + i ), 50 - i, 5, out, offsetXD + offsetXO + offsetXN - i, offsetY, 14, 15 );
                    // Clean up 'ON'
                    Fill( out, offsetXD + offsetXO + offsetXN, offsetY, 1, 1, getButtonFillingColor( i == 0 ) );
                    Fill( out, offsetXD + offsetXO + offsetXN - i, offsetY + 9, 1, 1, getButtonFillingColor( i == 0 ) );
                    // Add 'N'
                    Blit( GetICN( ICN::TRADPOST, 17 + i ), 50 - i, 5, out, offsetXD + 10 + offsetXN + offsetXN - i, offsetY, 14, 15 );
                    // Clean up 'NN'
                    Fill( out, offsetXD + offsetXO + offsetXN + offsetXN - i, offsetY + 9, 1, 1, getButtonFillingColor( i == 0 ) );
                    // Add 'ER'
                    Blit( GetICN( ICN::CAMPXTRG, 2 + i ), 75 - ( 8 * i ), 5, out, offsetXD + offsetXO + offsetXN + offsetXN + offsetXN - ( 2 * i ), offsetY, 23, 15 );
                    // Restore button ornament
                    Blit( GetICN( ICN::TRADPOST, 17 + i ), offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 20, offsetY, out,
                          offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 20, offsetY, 1, 1 );
                    Blit( GetICN( ICN::TRADPOST, 17 + i ), offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 21, offsetY + 1, out,
                          offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 21, offsetY + 1, 2, 3 );
                    Blit( GetICN( ICN::TRADPOST, 17 + i ), offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 20, offsetY, out,
                          offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 21, offsetY + 4, 1, 1 );
                }
                return true;
            case ICN::BTNGIFT_EVIL:
                _icnVsSprite[id].resize( 2 );
                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::TRADPOSE, 17 + i );
                    // clean the button
                    Fill( out, 33, 5, 31, 16, getButtonFillingColor( i == 0, false ) );

                    const int32_t offsetY = 5;
                    // Add 'D'
                    const int32_t offsetXD = 14;
                    Blit( GetICN( ICN::CPANELE, 4 + i ), 48 - i, 28 + i, out, offsetXD - i, offsetY + i, 10, 15 );
                    // Clean up 'D' and restore button ornament
                    Blit( GetICN( ICN::CPANELE, 4 + i ), 48 - i, 36, out, offsetXD - 1 - i, offsetY + 4 + i, 1, 1 );
                    Blit( GetICN( ICN::CPANELE, 4 + i ), 48 - i, 35, out, offsetXD - i, offsetY + 9 + i, 1, 2 );
                    Blit( GetICN( ICN::CPANELE, 4 + i ), 48 - i, 35, out, offsetXD - 1 - i, offsetY + 13 + i, 1, 1 );
                    Fill( out, offsetXD + 9 - i, offsetY + 13 + i, 1, 1, getButtonFillingColor( i == 0, false ) );
                    Blit( GetICN( ICN::TRADPOSE, 17 + i ), offsetXD, offsetY, out, offsetXD, offsetY, 1, 1 );
                    Fill( out, offsetXD + 9 - i, offsetY + i, 1, 1, getButtonFillingColor( i == 0, false ) );
                    // Add 'O'
                    const int32_t offsetXO = 10;
                    Blit( GetICN( ICN::APANELE, 4 + i ), 50 - i, 20 + i, out, offsetXD + offsetXO + 1 - i, offsetY + i, 13 - i, 14 );
                    // Clean up 'DO'
                    Blit( GetICN( ICN::CPANELE, 4 + i ), 51 - i, 34, out, offsetXD + offsetXO - i, offsetY + 5, 2, 2 );
                    Blit( GetICN( ICN::CPANELE, 4 + i ), 51 - i, 34, out, offsetXD + offsetXO - i, offsetY + 7, 1, 1 + i );
                    Blit( GetICN( ICN::CPANELE, 4 + i ), 56 - i, 28 + i, out, offsetXD + 10 - i, offsetY + 2 + i, 1, 3 );
                    Blit( GetICN( ICN::CPANELE, 4 + i ), 56 - i, 28 + i, out, offsetXD + 11 - i, offsetY + 3 + i, 1, 2 );
                    Fill( out, offsetXD + 11 - i, offsetY + i, 3, 3, getButtonFillingColor( i == 0, false ) );
                    Fill( out, offsetXD + 12 - i, offsetY + 3 + i, 1, 2, getButtonFillingColor( i == 0, false ) );
                    // Add 'N'
                    const int32_t offsetXN = 13;
                    Blit( GetICN( ICN::TRADPOSE, 17 + i ), 50 - i, 5, out, offsetXD + offsetXO + offsetXN - i, offsetY, 14, 15 );
                    // Clean up 'ON'
                    Fill( out, offsetXD + offsetXO + offsetXN - 1 - i, offsetY + 11 + i, 1, 3, getButtonFillingColor( i == 0, false ) );
                    Fill( out, offsetXD + offsetXO + offsetXN - i, offsetY, 1, 1, getButtonFillingColor( i == 0, false ) );
                    Fill( out, offsetXD + offsetXO + offsetXN - i, offsetY + 9, 1, 1, getButtonFillingColor( i == 0, false ) );
                    // Add 'N'
                    Blit( GetICN( ICN::TRADPOSE, 17 + i ), 50 - i, 5, out, offsetXD + offsetXO + offsetXN + offsetXN - i, offsetY, 13, 15 );
                    // Clean up 'NN'
                    Fill( out, offsetXD + offsetXO + offsetXN + offsetXN - i, offsetY + 9, 1, 1, getButtonFillingColor( i == 0, false ) );
                    // Add 'ER'
                    Blit( GetICN( ICN::APANELE, 8 + i ), 66 - ( 3 * i ), 5 + ( 2 * i ), out, offsetXD + offsetXO + offsetXN + offsetXN + offsetXN - ( 2 * i ),
                          offsetY + ( 2 * i ), 23, 14 - i );
                    // Clean up 'NE'
                    Blit( GetICN( ICN::TRADPOSE, 17 + i ), 50 - i, 5, out, offsetXD + offsetXO + offsetXN + offsetXN + offsetXN - i, offsetY, 2, 10 );
                    Fill( out, offsetXD + offsetXO + offsetXN + offsetXN + offsetXN - ( 2 * i ), offsetY + 9 + i, 1 + i, 2 + i, getButtonFillingColor( i == 0, false ) );
                    // Restore button ornament
                    Blit( GetICN( ICN::TRADPOSE, 17 + i ), offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 20, offsetY, out,
                          offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 20, offsetY, 1, 1 );
                    Blit( GetICN( ICN::TRADPOSE, 17 + i ), offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 21, offsetY + 1, out,
                          offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 21, offsetY + 1, 2, 3 );
                    Blit( GetICN( ICN::TRADPOSE, 17 + i ), offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 20, offsetY, out,
                          offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 21, offsetY + 4, 1, 1 );
                }
                return true;
            case ICN::BUTTON_SMALL_MIN_GOOD:
                _icnVsSprite[id].resize( 2 );
                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::RECRUIT, 4 + i );
                    // Clean the button and leave 'M'
                    Fill( out, 31 - 2 * i, 5 + i, 25, 15, getButtonFillingColor( i == 0 ) );
                    Fill( out, 29 - 2 * i, 17 + i, 2, 2, getButtonFillingColor( i == 0 ) );
                    // Add 'I'
                    Blit( GetICN( ICN::APANEL, 4 + i ), 25 - i, 19 + i, out, 32 - i, 4 + i, 7 - i, 15 );
                    Blit( GetICN( ICN::RECRUIT, 4 + i ), 28 - i, 7 + i, out, 36 - i, 7 + i, 3, 9 );
                    Fill( out, 37 - i, 16 + i, 2, 3, getButtonFillingColor( i == 0 ) );
                    // Add 'N'
                    Blit( GetICN( ICN::TRADPOST, 17 + i ), 50 - i, 5, out, 41 - i, 5, 14, 15 );
                    Fill( out, 41 - i, 5, 1, 1, getButtonFillingColor( i == 0 ) );
                    Fill( out, 41 - i, 5 + 9, 1, 1, getButtonFillingColor( i == 0 ) );
                }
                return true;
            default:
                break;
            }
            return false;
        }

        bool generatePolishSpecificImages( const int id )
        {
            switch ( id ) {
            case ICN::BTNBATTLEONLY:
                _icnVsSprite[id].resize( 2 );
                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNNEWGM, 6 + i );
                    // clean the button
                    Fill( out, 25, 18, 88, 23, getButtonFillingColor( i == 0 ) );
                    const int32_t offsetX = 46;
                    const int32_t offsetY = 23;
                    // Add 'BI'
                    Blit( GetICN( ICN::BTNMCFG, 2 + i ), 58 - i, 29, out, offsetX - i, offsetY, 14, 11 );
                    // Add 'T'
                    Blit( GetICN( ICN::BTNNEWGM, 0 + i ), 24 - i, 29, out, offsetX + 14 - i, offsetY, 9, 11 );
                    // Add 'WA'
                    Blit( GetICN( ICN::BTNEMAIN, 0 + i ), 45 - i, 23, out, offsetX + 23 - i, offsetY, 24, 11 );
                    // Add pixel to 'W'
                    Blit( GetICN( ICN::BTNEMAIN, 0 + i ), 47 - i, 23 + i, out, offsetX + 38 - i, offsetY + i, 1, 1 );
                }
                return true;
            default:
                break;
            }
            return false;
        }

        bool generateItalianSpecificImages( const int id )
        {
            switch ( id ) {
            case ICN::BTNBATTLEONLY:
                _icnVsSprite[id].resize( 2 );
                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNNEWGM, 6 + i );
                    // clean the button
                    const uint8_t buttonFillingColor = getButtonFillingColor( i == 0 );
                    Fill( out, 25, 18, 88, 23, buttonFillingColor );
                    const int32_t offsetX = 16;
                    const int32_t offsetY = 21;
                    // Add 'B'
                    Blit( GetICN( ICN::BTNBAUD, 0 + i ), 42 - i, 28, out, offsetX - i, offsetY, 13, 15 );
                    Fill( out, offsetX + 11, offsetY + 13, 1, 2, buttonFillingColor );
                    // Add 'A'
                    Blit( GetICN( ICN::BTNNEWGM, 0 + i ), 80 - i, 28, out, offsetX + 13 - i, offsetY, 14, 15 );
                    Fill( out, offsetX + 13 - i, offsetY + 3, 1, 4, buttonFillingColor );
                    // Add 'T'
                    Blit( GetICN( ICN::BTNMP, 0 + i ), 74 - i, 5, out, offsetX + 27 - 2 * i, offsetY, 12, 15 );
                    // Add 'T'
                    Blit( GetICN( ICN::BTNMP, 0 + i ), 74 - i, 5, out, offsetX + 39 - 2 * i, offsetY, 12, 15 );
                    // Add 'A'
                    Blit( GetICN( ICN::BTNNEWGM, 0 + i ), 80 - i, 28, out, offsetX + 50 - i, offsetY, 14, 15 );
                    Fill( out, offsetX + 65 - i, offsetY + 5, 1, 2, buttonFillingColor );
                    Fill( out, offsetX + 65 - i, offsetY + 14, 1, 3, buttonFillingColor );
                    Fill( out, offsetX + 50 - i, offsetY + 3, 1, 4, buttonFillingColor );
                    // Add 'G'
                    Blit( GetICN( ICN::BTNNEWGM, 0 + i ), 44 - i, 12, out, offsetX + 65 - i, offsetY, 11, 15 );
                    // Add 'L'
                    Blit( GetICN( ICN::BTNDC, 4 + i ), 77 - i, 21, out, offsetX + 77 - 2 * i, offsetY, 9, 15 );
                    // Add 'I'
                    Blit( GetICN( ICN::BTNNEWGM, 0 + i ), 56 - i, 12, out, offsetX + 86 - i, offsetY, 7, 15 );
                    // Add 'A'
                    Blit( GetICN( ICN::BTNNEWGM, 0 + i ), 80 - i, 28, out, offsetX + 93 - i, offsetY, 14, 15 );
                    Fill( out, offsetX + 109 - i, offsetY + 5, 1, 2, buttonFillingColor );
                    Fill( out, offsetX + 93 - i, offsetY + 3, 1, 4, buttonFillingColor );
                }
                return true;
            default:
                break;
            }
            return false;
        }

        void generateLanguageSpecificImages( int id )
        {
            assert( isLanguageDependentIcnId( id ) );

            const fheroes2::SupportedLanguage resourceLanguage = fheroes2::getResourceLanguage();

            // Language-specific image generators, may fail
            if ( fheroes2::getCurrentLanguage() == resourceLanguage ) {
                switch ( resourceLanguage ) {
                case fheroes2::SupportedLanguage::German:
                    if ( generateGermanSpecificImages( id ) ) {
                        return;
                    }
                    break;
                case fheroes2::SupportedLanguage::French:
                    if ( generateFrenchSpecificImages( id ) ) {
                        return;
                    }
                    break;
                case fheroes2::SupportedLanguage::Polish:
                    if ( generatePolishSpecificImages( id ) ) {
                        return;
                    }
                    break;
                case fheroes2::SupportedLanguage::Italian:
                    if ( generateItalianSpecificImages( id ) ) {
                        return;
                    }
                    break;
                default:
                    break;
                }
            }
            // Image generator of a last resort, must provide the generation of the "default" variant
            // for all image ids for which this function can be called, and must not fail.
            generateDefaultImages( id );
        }

        // This function must return true is resources have been modified, false otherwise.
        bool LoadModifiedICN( const int id )
        {
            // If this assertion blows up then you are calling this function in a recursion. Check your code!
            assert( _icnVsSprite[id].empty() );

            // IMPORTANT!!!
            // Call LoadOriginalICN() function only if you are handling the same ICN.
            // If you need to load a different ICN use loadICN() function.

            switch ( id ) {
            case ICN::ROUTERED:
                CopyICNWithPalette( id, ICN::ROUTE, PAL::PaletteType::RED );
                return true;
            case ICN::FONT:
            case ICN::SMALFONT: {
                LoadOriginalICN( id );

                auto & imageArray = _icnVsSprite[id];
                if ( imageArray.size() < 96 ) {
                    // 96 symbols is the minimum requirement for English.
                    throw std::logic_error( "The game resources are corrupted. Please use resources from a licensed version of Heroes of Might and Magic II." );
                }

                // Compare '(' and ')' symbols. By size they are always the same. However, we play safe and fail if both dimensions are different.
                if ( ( imageArray[8].width() != imageArray[9].width() ) && ( imageArray[8].height() != imageArray[9].height() ) ) {
                    // This is most likely a corrupted font or a pirated translation to a non-English language which causes all sorts of rendering issues.
                    throw std::logic_error( "The game resources are corrupted. Please use resources from a licensed version of Heroes of Might and Magic II." );
                }

                const std::vector<uint8_t> & body = ::AGG::getDataFromAggFile( ICN::GetString( id ) );
                const uint32_t crc32 = fheroes2::calculateCRC32( body.data(), body.size() );

                if ( id == ICN::SMALFONT ) {
                    // Small font in official Polish GoG version has all letters shifted 1 pixel down.
                    if ( crc32 == 0xE9EC7A63 ) {
                        for ( Sprite & letter : imageArray ) {
                            letter.setPosition( letter.x(), letter.y() - 1 );
                        }
                    }
                    modifyBaseSmallFont( _icnVsSprite[id] );
                }
                else {
                    assert( id == ICN::FONT );
                    // The original images contain an issue: image layer has value 50 which is '2' in UTF-8. We must correct these (only 3) places
                    for ( fheroes2::Sprite & fontImage : imageArray ) {
                        ReplaceColorIdByTransformId( fontImage, 50, 2 );
                    }
                    modifyBaseNormalFont( _icnVsSprite[id] );
                }

                // Some checks that we really have CP1251 font
                const int32_t verifiedFontWidth = ( id == ICN::FONT ) ? 19 : 12;
                if ( imageArray.size() == 162 && imageArray[121].width() == verifiedFontWidth ) {
                    // Engine expects that letter indexes correspond to charcode - 0x20.
                    // In case CP1251 font.icn contains sprites for chars 0x20-0x7F, 0xC0-0xDF, 0xA8, 0xE0-0xFF, 0xB8 (in that order).
                    // We rearrange sprites array for corresponding sprite indexes to charcode - 0x20.
                    const Sprite firstSprite{ imageArray[0] };
                    imageArray.insert( imageArray.begin() + 96, 64, firstSprite );
                    std::swap( imageArray[136], imageArray[192] ); // Move sprites for chars 0xA8
                    std::swap( imageArray[152], imageArray[225] ); // and 0xB8 to it's places.
                    imageArray.pop_back();
                    imageArray.erase( imageArray.begin() + 192 );
                }
                // German version uses CP1252
                if ( crc32 == 0x04745D1D || crc32 == 0xD0F0D852 ) {
                    const Sprite firstSprite{ imageArray[0] };
                    imageArray.insert( imageArray.begin() + 96, 124, firstSprite );
                    std::swap( imageArray[164], imageArray[224] );
                    std::swap( imageArray[182], imageArray[225] );
                    std::swap( imageArray[188], imageArray[226] );
                    std::swap( imageArray[191], imageArray[223] );
                    std::swap( imageArray[196], imageArray[220] );
                    std::swap( imageArray[214], imageArray[221] );
                    std::swap( imageArray[220], imageArray[222] );
                    imageArray.erase( imageArray.begin() + 221, imageArray.end() );
                }
                // French version has its own special encoding but should conform to CP1252 too
                if ( crc32 == 0xD9556567 || crc32 == 0x406967B9 ) {
                    const Sprite firstSprite{ imageArray[0] };
                    imageArray.insert( imageArray.begin() + 96, 160 - 32, firstSprite );
                    imageArray[192 - 32] = imageArray[33];
                    imageArray[199 - 32] = imageArray[35];
                    imageArray[201 - 32] = imageArray[37];
                    imageArray[202 - 32] = imageArray[37];
                    imageArray[244 - 32] = imageArray[3];
                    imageArray[251 - 32] = imageArray[4];
                    imageArray[249 - 32] = imageArray[6];
                    imageArray[226 - 32] = imageArray[10];
                    imageArray[239 - 32] = imageArray[28];
                    imageArray[238 - 32] = imageArray[30];
                    imageArray[224 - 32] = imageArray[32];
                    imageArray[231 - 32] = imageArray[62];
                    imageArray[232 - 32] = imageArray[64];
                    imageArray[239 - 32] = imageArray[91];
                    imageArray[234 - 32] = imageArray[92];
                    imageArray[238 - 32] = imageArray[93];
                    imageArray[233 - 32] = imageArray[94];
                    imageArray[238 - 32] = imageArray[95];
                    imageArray.erase( imageArray.begin() + 252 - 32, imageArray.end() );
                }
                // Italian version uses CP1252
                if ( crc32 == 0x219B3124 || crc32 == 0x1F3C3C74 ) {
                    const Sprite firstSprite{ imageArray[0] };
                    imageArray.insert( imageArray.begin() + 101, 155 - 32, firstSprite );
                    imageArray[192 - 32] = imageArray[33];
                    imageArray[200 - 32] = imageArray[37];
                    imageArray[201 - 32] = imageArray[37];
                    imageArray[204 - 32] = imageArray[41];
                    imageArray[210 - 32] = imageArray[47];
                    imageArray[217 - 32] = imageArray[53];
                    imageArray[224 - 32] = imageArray[96];
                    imageArray[232 - 32] = imageArray[97];
                    imageArray[233 - 32] = imageArray[69];
                    imageArray[236 - 32] = imageArray[98];
                    imageArray[242 - 32] = imageArray[99];
                    imageArray[249 - 32] = imageArray[100];
                    imageArray.erase( imageArray.begin() + 250 - 32, imageArray.end() );
                }
                return true;
            }
            case ICN::YELLOW_FONT:
                CopyICNWithPalette( id, ICN::FONT, PAL::PaletteType::YELLOW_FONT );
                return true;
            case ICN::YELLOW_SMALLFONT:
                CopyICNWithPalette( id, ICN::SMALFONT, PAL::PaletteType::YELLOW_FONT );
                return true;
            case ICN::GRAY_FONT:
                CopyICNWithPalette( id, ICN::FONT, PAL::PaletteType::GRAY_FONT );
                return true;
            case ICN::GRAY_SMALL_FONT:
                CopyICNWithPalette( id, ICN::SMALFONT, PAL::PaletteType::GRAY_FONT );
                return true;
            case ICN::SPELLS:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() != 60 ) {
                    return true;
                }

                _icnVsSprite[id].resize( 73 );

                for ( uint32_t i = 60; i < 66; ++i ) {
                    // Mass Cure spell. ( when i == 60 ).
                    size_t originalIndex = 6;
                    if ( i == 61 ) {
                        // Mass Haste spell.
                        originalIndex = 14;
                    }
                    else if ( i == 62 ) {
                        // Mass Slow spell.
                        originalIndex = 1;
                    }
                    else if ( i == 63 ) {
                        // Mass Bless spell.
                        originalIndex = 7;
                    }
                    else if ( i == 64 ) {
                        // Mass Curse spell.
                        originalIndex = 3;
                    }
                    else if ( i == 65 ) {
                        // Mass Shield spell.
                        originalIndex = 15;
                    }

                    const Sprite & originalImage = _icnVsSprite[id][originalIndex];
                    Sprite & image = _icnVsSprite[id][i];

                    image.resize( originalImage.width() + 8, originalImage.height() + 8 );
                    image.setPosition( originalImage.x() + 4, originalImage.y() + 4 );
                    image.fill( 1 );

                    AlphaBlit( originalImage, image, 0, 0, 128 );
                    AlphaBlit( originalImage, image, 4, 4, 192 );
                    Blit( originalImage, image, 8, 8 );

                    AddTransparency( image, 1 );
                }

                // The Petrification spell does not have its own icon in the original game.
                h2d::readImage( "petrification_spell_icon.image", _icnVsSprite[id][66] );

                // Generate random spell image for Editor.
                {
                    const Sprite & randomSpellImage = _icnVsSprite[id][2];
                    int32_t imageWidth = randomSpellImage.width();

                    Copy( randomSpellImage, _icnVsSprite[id][67] );

                    // Add text on random spell images.
                    for ( uint32_t i = 1; i < 6; ++i ) {
                        Sprite & originalImage = _icnVsSprite[id][i + 67];
                        Copy( randomSpellImage, originalImage );

                        const Text text( std::to_string( i ), FontType::normalWhite() );
                        text.draw( ( imageWidth - text.width() ) / 2, 22, originalImage );
                    }
                }
                return true;
            case ICN::CSLMARKER:
                _icnVsSprite[id].resize( 3 );
                for ( uint32_t i = 0; i < 3; ++i ) {
                    _icnVsSprite[id][i] = GetICN( ICN::LOCATORS, 24 );
                    if ( i == 1 ) {
                        ReplaceColorId( _icnVsSprite[id][i], 0x0A, 0xD6 );
                    }
                    else if ( i == 2 ) {
                        ReplaceColorId( _icnVsSprite[id][i], 0x0A, 0xDE );
                    }
                }
                return true;
            case ICN::BATTLESKIP:
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::TEXTBAR, 4 + i );

                    // clean the button
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 3, 8, out, 3, 1, 43, 14 );

                    // add 'skip'
                    Blit( GetICN( ICN::TEXTBAR, i ), 3, 10, out, 3, 0, 43, 14 );
                }
                return true;
            case ICN::BUYMAX:
            case ICN::BUTTON_NEW_GAME_GOOD:
            case ICN::BUTTON_NEW_GAME_EVIL:
            case ICN::BUTTON_SAVE_GAME_GOOD:
            case ICN::BUTTON_SAVE_GAME_EVIL:
            case ICN::BUTTON_LOAD_GAME_GOOD:
            case ICN::BUTTON_LOAD_GAME_EVIL:
            case ICN::BUTTON_INFO_GOOD:
            case ICN::BUTTON_INFO_EVIL:
            case ICN::BUTTON_QUIT_GOOD:
            case ICN::BUTTON_QUIT_EVIL:
            case ICN::BUTTON_SMALL_CANCEL_GOOD:
            case ICN::BUTTON_SMALL_CANCEL_EVIL:
            case ICN::BUTTON_SMALL_OKAY_GOOD:
            case ICN::BUTTON_SMALL_OKAY_EVIL:
            case ICN::BUTTON_SMALLER_OKAY_GOOD:
            case ICN::BUTTON_SMALLER_OKAY_EVIL:
            case ICN::BUTTON_SMALL_ACCEPT_GOOD:
            case ICN::BUTTON_SMALL_ACCEPT_EVIL:
            case ICN::BUTTON_SMALL_DECLINE_GOOD:
            case ICN::BUTTON_SMALL_DECLINE_EVIL:
            case ICN::BUTTON_SMALL_LEARN_GOOD:
            case ICN::BUTTON_SMALL_LEARN_EVIL:
            case ICN::BUTTON_SMALL_TRADE_GOOD:
            case ICN::BUTTON_SMALL_TRADE_EVIL:
            case ICN::BUTTON_SMALL_YES_GOOD:
            case ICN::BUTTON_SMALL_YES_EVIL:
            case ICN::BUTTON_SMALL_NO_GOOD:
            case ICN::BUTTON_SMALL_NO_EVIL:
            case ICN::BUTTON_SMALL_EXIT_GOOD:
            case ICN::BUTTON_SMALL_EXIT_EVIL:
            case ICN::BUTTON_EXIT_HEROES_MEETING:
            case ICN::BUTTON_EXIT_TOWN:
            case ICN::BUTTON_EXIT_PUZZLE_DDOOR_EVIL:
            case ICN::BUTTON_EXIT_PUZZLE_DDOOR_GOOD:
            case ICN::BUTTON_SMALL_DISMISS_GOOD:
            case ICN::BUTTON_SMALL_DISMISS_EVIL:
            case ICN::BUTTON_SMALL_UPGRADE_GOOD:
            case ICN::BUTTON_SMALL_UPGRADE_EVIL:
            case ICN::BUTTON_SMALL_RESTART_GOOD:
            case ICN::BUTTON_SMALL_RESTART_EVIL:
            case ICN::BUTTON_KINGDOM_EXIT:
            case ICN::BUTTON_KINGDOM_HEROES:
            case ICN::BUTTON_KINGDOM_TOWNS:
            case ICN::BUTTON_MAPSIZE_SMALL:
            case ICN::BUTTON_MAPSIZE_MEDIUM:
            case ICN::BUTTON_MAPSIZE_LARGE:
            case ICN::BUTTON_MAPSIZE_XLARGE:
            case ICN::BUTTON_MAPSIZE_ALL:
            case ICN::BUTTON_MAP_SELECT:
            case ICN::BUTTON_STANDARD_GAME:
            case ICN::BUTTON_CAMPAIGN_GAME:
            case ICN::BUTTON_MULTIPLAYER_GAME:
            case ICN::BUTTON_LARGE_CANCEL:
            case ICN::BUTTON_LARGE_CONFIG:
            case ICN::BUTTON_ORIGINAL_CAMPAIGN:
            case ICN::BUTTON_EXPANSION_CAMPAIGN:
            case ICN::BUTTON_HOT_SEAT:
            case ICN::BUTTON_2_PLAYERS:
            case ICN::BUTTON_3_PLAYERS:
            case ICN::BUTTON_4_PLAYERS:
            case ICN::BUTTON_5_PLAYERS:
            case ICN::BUTTON_6_PLAYERS:
            case ICN::BTNBATTLEONLY:
            case ICN::BTNGIFT_GOOD:
            case ICN::BTNGIFT_EVIL:
            case ICN::UNIFORM_EVIL_MAX_BUTTON:
            case ICN::UNIFORM_EVIL_MIN_BUTTON:
            case ICN::UNIFORM_GOOD_MAX_BUTTON:
            case ICN::UNIFORM_GOOD_MIN_BUTTON:
            case ICN::UNIFORM_GOOD_OKAY_BUTTON:
            case ICN::UNIFORM_EVIL_OKAY_BUTTON:
            case ICN::UNIFORM_GOOD_CANCEL_BUTTON:
            case ICN::UNIFORM_EVIL_CANCEL_BUTTON:
            case ICN::UNIFORM_GOOD_EXIT_BUTTON:
            case ICN::UNIFORM_EVIL_EXIT_BUTTON:
            case ICN::BUTTON_SMALL_MIN_GOOD:
            case ICN::BUTTON_SMALL_MIN_EVIL:
            case ICN::BUTTON_SMALL_MAX_GOOD:
            case ICN::BUTTON_SMALL_MAX_EVIL:
            case ICN::BUTTON_GUILDWELL_EXIT:
            case ICN::GOOD_CAMPAIGN_BUTTONS:
            case ICN::EVIL_CAMPAIGN_BUTTONS:
            case ICN::POL_CAMPAIGN_BUTTONS:
            case ICN::BUTTON_VIEWWORLD_EXIT_GOOD:
            case ICN::BUTTON_VIEWWORLD_EXIT_EVIL:
            case ICN::BUTTON_VERTICAL_DISMISS:
            case ICN::BUTTON_VERTICAL_EXIT:
            case ICN::BUTTON_HSCORES_VERTICAL_CAMPAIGN:
            case ICN::BUTTON_HSCORES_VERTICAL_EXIT:
            case ICN::BUTTON_HSCORES_VERTICAL_STANDARD:
                generateLanguageSpecificImages( id );
                return true;
            case ICN::PHOENIX:
                LoadOriginalICN( id );
                // First sprite has cropped shadow. We copy missing part from another 'almost' identical frame
                if ( _icnVsSprite[id].size() >= 32 ) {
                    const Sprite & in = _icnVsSprite[id][32];
                    Copy( in, 60, 73, _icnVsSprite[id][1], 60, 73, 14, 13 );
                    Copy( in, 56, 72, _icnVsSprite[id][30], 56, 72, 18, 9 );
                }
                return true;
            case ICN::MONH0028: // phoenix
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() == 1 ) {
                    const Sprite & correctFrame = GetICN( ICN::PHOENIX, 32 );
                    Copy( correctFrame, 60, 73, _icnVsSprite[id][0], 58, 70, 14, 13 );
                }
                return true;
            case ICN::CAVALRYR:
                LoadOriginalICN( id );
                // Sprite 23 has incorrect colors, we need to replace them
                if ( _icnVsSprite[id].size() >= 23 ) {
                    Sprite & out = _icnVsSprite[id][23];

                    std::vector<uint8_t> indexes( 256 );
                    for ( uint32_t i = 0; i < 256; ++i ) {
                        indexes[i] = static_cast<uint8_t>( i );
                    }

                    indexes[69] = 187;
                    indexes[71] = 195;
                    indexes[73] = 188;
                    indexes[74] = 190;
                    indexes[75] = 193;
                    indexes[76] = 191;
                    indexes[77] = 195;
                    indexes[80] = 195;
                    indexes[81] = 196;
                    indexes[83] = 196;
                    indexes[84] = 197;
                    indexes[151] = 197;

                    ApplyPalette( out, indexes );
                }
                return true;
            case ICN::TITANMSL:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() == 7 ) {
                    // We need to shift Titan lightning arrow sprite position to correctly render it.
                    _icnVsSprite[id][0].setPosition( _icnVsSprite[id][0].x(), _icnVsSprite[id][0].y() - 5 );
                    _icnVsSprite[id][1].setPosition( _icnVsSprite[id][1].x() - 5, _icnVsSprite[id][1].y() - 5 );
                    _icnVsSprite[id][2].setPosition( _icnVsSprite[id][2].x() - 10, _icnVsSprite[id][2].y() );
                    _icnVsSprite[id][3].setPosition( _icnVsSprite[id][3].x() - 15, _icnVsSprite[id][3].y() );
                    _icnVsSprite[id][4].setPosition( _icnVsSprite[id][4].x() - 10, _icnVsSprite[id][2].y() );
                    _icnVsSprite[id][5].setPosition( _icnVsSprite[id][5].x() - 5, _icnVsSprite[id][5].y() - 5 );
                    _icnVsSprite[id][6].setPosition( _icnVsSprite[id][6].x(), _icnVsSprite[id][6].y() - 5 );
                }
                return true;
            case ICN::TROLLMSL:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() == 1 ) {
                    Sprite & out = _icnVsSprite[id][0];
                    // The original sprite contains 2 pixels which are empty
                    if ( out.width() * out.height() > 188 && out.transform()[147] == 1 && out.transform()[188] == 1 ) {
                        out.transform()[147] = 0;
                        out.image()[147] = 22;

                        out.transform()[188] = 0;
                        out.image()[188] = 24;
                    }
                }
                return true;
            case ICN::TROLL2MSL:
                loadICN( ICN::TROLLMSL );
                if ( _icnVsSprite[ICN::TROLLMSL].size() == 1 ) {
                    _icnVsSprite[id].resize( 1 );

                    Sprite & out = _icnVsSprite[id][0];
                    out = _icnVsSprite[ICN::TROLLMSL][0];

                    // The original sprite contains 2 pixels which are empty
                    if ( out.width() * out.height() > 188 && out.transform()[147] == 1 && out.transform()[188] == 1 ) {
                        out.transform()[147] = 0;
                        out.image()[147] = 22;

                        out.transform()[188] = 0;
                        out.image()[188] = 24;
                    }

                    std::vector<uint8_t> indexes( 256 );
                    for ( uint32_t i = 0; i < 256; ++i ) {
                        indexes[i] = static_cast<uint8_t>( i );
                    }

                    indexes[10] = 152;
                    indexes[11] = 153;
                    indexes[12] = 154;
                    indexes[13] = 155;
                    indexes[14] = 155;
                    indexes[15] = 156;
                    indexes[16] = 157;
                    indexes[17] = 158;
                    indexes[18] = 159;
                    indexes[19] = 160;
                    indexes[20] = 160;
                    indexes[21] = 161;
                    indexes[22] = 162;
                    indexes[23] = 163;
                    indexes[24] = 164;
                    indexes[25] = 165;
                    indexes[26] = 166;
                    indexes[27] = 166;
                    indexes[28] = 167;
                    indexes[29] = 168;
                    indexes[30] = 169;
                    indexes[31] = 170;
                    indexes[32] = 171;
                    indexes[33] = 172;
                    indexes[34] = 172;
                    indexes[35] = 173;

                    ApplyPalette( out, indexes );
                }
                return true;
            case ICN::GOLEM:
            case ICN::GOLEM2:
                LoadOriginalICN( id );
                // Original Golem ICN contains 40 frames. We make the corrections only for original sprite.
                if ( _icnVsSprite[id].size() == 40 ) {
                    // Movement animation fix for Iron and Steel Golem: its 'MOVE_MAIN' animation is missing 1/4 of animation start.
                    // The 'MOVE_START' (for first and one cell move) has this 1/4 of animation, but 'MOVE_TILE_START` is empty,
                    // so we make a copy of 'MOVE_MAIN' frames to the end of sprite vector and correct their 'x' coordinate
                    // to cover the whole cell except the last frame, that has correct coordinates.
                    const size_t golemICNSize = _icnVsSprite[id].size();
                    // 'MOVE_MAIN' has 7 frames and we copy only first 6.
                    const int32_t copyFramesNum = 6;

                    _icnVsSprite[id].reserve( golemICNSize + copyFramesNum );
                    // 'MOVE_MAIN' frames starts from the 6th frame in Golem ICN sprites.
                    size_t copyFrame = 6;

                    for ( int32_t i = 0; i < copyFramesNum; ++i, ++copyFrame ) {
                        // IMPORTANT: we MUST do a copy of a vector element if we want to insert it to the same vector.
                        fheroes2::Sprite originalFrame = _icnVsSprite[id][copyFrame];
                        _icnVsSprite[id].emplace_back( std::move( originalFrame ) );

                        const size_t frameID = golemICNSize + i;
                        // We have 7 'MOVE_MAIN' frames and 1/4 of cell to expand the horizontal movement, so we shift the first copied frame by "6*CELLW/(4*7)" to the
                        // left and reduce this shift every next frame by "CELLW/(7*4)".
                        _icnVsSprite[id][frameID].setPosition( _icnVsSprite[id][frameID].x() - ( copyFramesNum - i ) * CELLW / 28, _icnVsSprite[id][frameID].y() );
                    }
                }
                return true;
            case ICN::LOCATORE:
            case ICN::LOCATORS:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() > 15 ) {
                    if ( _icnVsSprite[id][12].width() == 47 ) {
                        Sprite & out = _icnVsSprite[id][12];
                        out = Crop( out, 0, 0, out.width() - 1, out.height() );
                    }
                    if ( _icnVsSprite[id][15].width() == 47 ) {
                        Sprite & out = _icnVsSprite[id][15];
                        out = Crop( out, 0, 0, out.width() - 1, out.height() );
                    }
                }
                return true;
            case ICN::TOWNBKG2:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() == 1 ) {
                    Sprite & out = _icnVsSprite[id][0];
                    // The first pixel of the original sprite has incorrect color.
                    if ( !out.empty() ) {
                        out._disableTransformLayer();
                        out.image()[0] = 10;
                    }
                }
                return true;
            case ICN::HSICONS:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() > 7 ) {
                    Sprite & out = _icnVsSprite[id][7];
                    if ( out.width() == 34 && out.height() == 19 ) {
                        Sprite temp;
                        std::swap( temp, out );

                        out.resize( temp.width() + 1, temp.height() );
                        out.reset();
                        Copy( temp, 0, 0, out, 1, 0, temp.width(), temp.height() );
                        Copy( temp, temp.width() - 1, 10, out, 0, 10, 1, 3 );
                    }
                }
                return true;
            case ICN::LISTBOX_EVIL:
                CopyICNWithPalette( id, ICN::LISTBOX, PAL::PaletteType::GRAY );
                for ( fheroes2::Sprite & sprite : _icnVsSprite[id] ) {
                    ApplyPalette( sprite, 2 );
                }
                return true;
            case ICN::MONS32:
                LoadOriginalICN( id );

                if ( _icnVsSprite[id].size() > 4 ) { // Veteran Pikeman
                    Sprite & modified = _icnVsSprite[id][4];

                    Sprite temp( modified.width(), modified.height() + 1 );
                    temp.reset();
                    Blit( modified, 0, 0, temp, 0, 1, modified.width(), modified.height() );
                    modified = std::move( temp );
                    Fill( modified, 7, 0, 4, 1, 36 );
                }
                if ( _icnVsSprite[id].size() > 6 ) { // Master Swordsman
                    Sprite & modified = _icnVsSprite[id][6];

                    Sprite temp( modified.width(), modified.height() + 1 );
                    temp.reset();
                    Blit( modified, 0, 0, temp, 0, 1, modified.width(), modified.height() );
                    modified = std::move( temp );
                    Fill( modified, 2, 0, 5, 1, 36 );
                }
                if ( _icnVsSprite[id].size() > 8 ) { // Champion
                    Sprite & modified = _icnVsSprite[id][8];

                    Sprite temp( modified.width(), modified.height() + 1 );
                    temp.reset();
                    Blit( modified, 0, 0, temp, 0, 1, modified.width(), modified.height() );
                    modified = std::move( temp );
                    Fill( modified, 12, 0, 5, 1, 36 );
                }
                if ( _icnVsSprite[id].size() > 33 ) {
                    // Minotaur King original mini sprite has blue armlets. We make them gold to correspond the ICN::MINOTAU2.
                    Sprite & modified = _icnVsSprite[id][33];

                    if ( modified.width() == 20 && modified.height() == 36 ) {
                        // We update these pixels: 6x16, 7x16, 8x16, 5x17, 6x17, 7x17, 8x17, 6x18, 7x18, 14x18, 14x19.
                        for ( const uint32_t pixelNumber : { 326, 327, 328, 345, 346, 347, 348, 366, 367, 374, 394 } ) {
                            // The gold color gradient has -42 offset from blue color gradient.
                            modified.image()[pixelNumber] -= 42;
                        }
                    }
                }
                if ( _icnVsSprite[id].size() > 62 ) {
                    const Point shadowOffset( -1, 2 );
                    for ( size_t i = 0; i < 62; ++i ) {
                        Sprite & modified = _icnVsSprite[id][i];
                        const Point originalOffset( modified.x(), modified.y() );
                        Sprite temp = addShadow( modified, { -1, 2 }, 2 );
                        temp.setPosition( originalOffset.x - 1, originalOffset.y + 2 );

                        const Rect area = GetActiveROI( temp, 2 );
                        if ( area.x > 0 || area.height != temp.height() ) {
                            const Point offset( temp.x() - area.x, temp.y() - temp.height() + area.y + area.height );
                            modified = Crop( temp, area.x, area.y, area.width, area.height );
                            modified.setPosition( offset.x, offset.y );
                        }
                        else {
                            modified = std::move( temp );
                        }
                    }
                }
                if ( _icnVsSprite[id].size() > 63 && _icnVsSprite[id][63].width() == 19 && _icnVsSprite[id][63].height() == 37 ) { // Air Elemental
                    Sprite & modified = _icnVsSprite[id][63];
                    modified.image()[19 * 9 + 9] = modified.image()[19 * 5 + 11];
                    modified.transform()[19 * 9 + 9] = modified.transform()[19 * 5 + 11];
                }

                return true;
            case ICN::MONSTER_SWITCH_LEFT_ARROW:
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    const Sprite & source = GetICN( ICN::RECRUIT, i );
                    Sprite & out = _icnVsSprite[id][i];
                    out.resize( source.height(), source.width() );
                    Transpose( source, out );
                    out = Flip( out, false, true );
                    out.setPosition( source.y() - static_cast<int32_t>( i ), source.x() );
                }
                return true;
            case ICN::MONSTER_SWITCH_RIGHT_ARROW:
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    const Sprite & source = GetICN( ICN::RECRUIT, i + 2 );
                    Sprite & out = _icnVsSprite[id][i];
                    out.resize( source.height(), source.width() );
                    Transpose( source, out );
                    out = Flip( out, false, true );
                    out.setPosition( source.y(), source.x() );
                }
                return true;
            case ICN::SURRENDR:
            case ICN::SURRENDE:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 4 ) {
                    if ( id == ICN::SURRENDR ) {
                        // Fix incorrect font color on good ACCEPT button.
                        ReplaceColorId( _icnVsSprite[id][0], 28, 56 );
                    }
                    // Fix pressed buttons background.
                    for ( const uint32_t i : { 0, 2 } ) {
                        Sprite & out = _icnVsSprite[id][i + 1];

                        Sprite tmp( out.width(), out.height() );
                        tmp.reset();
                        Copy( out, 0, 1, tmp, 1, 0, tmp.width() - 1, tmp.height() - 1 );
                        CopyTransformLayer( _icnVsSprite[id][i], tmp );

                        out.reset();
                        Copy( tmp, 1, 0, out, 0, 1, tmp.width() - 1, tmp.height() - 1 );
                    }
                }
                return true;
            case ICN::NON_UNIFORM_GOOD_RESTART_BUTTON:
                _icnVsSprite[id].resize( 2 );
                _icnVsSprite[id][0] = Crop( GetICN( ICN::CAMPXTRG, 2 ), 6, 0, 108, 25 );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                _icnVsSprite[id][1] = GetICN( ICN::CAMPXTRG, 3 );
                _icnVsSprite[id][1].setPosition( 0, 0 );

                // fix transparent corners
                CopyTransformLayer( _icnVsSprite[id][1], _icnVsSprite[id][0] );
                return true;
            case ICN::NON_UNIFORM_EVIL_RESTART_BUTTON:
                _icnVsSprite[id].resize( 2 );
                _icnVsSprite[id][0] = Crop( GetICN( ICN::CAMPXTRE, 2 ), 4, 0, 108, 25 );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                _icnVsSprite[id][1] = GetICN( ICN::CAMPXTRE, 3 );
                _icnVsSprite[id][1].setPosition( 0, 0 );

                // fix transparent corners
                CopyTransformLayer( _icnVsSprite[id][1], _icnVsSprite[id][0] );
                return true;
            case ICN::WHITE_LARGE_FONT: {
                GetICN( ICN::FONT, 0 );
                const std::vector<Sprite> & original = _icnVsSprite[ICN::FONT];
                _icnVsSprite[id].resize( original.size() );
                for ( size_t i = 0; i < _icnVsSprite[id].size(); ++i ) {
                    const Sprite & in = original[i];
                    Sprite & out = _icnVsSprite[id][i];
                    out.resize( in.width() * 2, in.height() * 2 );
                    Resize( in, out, true );
                    out.setPosition( in.x() * 2, in.y() * 2 );
                }
                return true;
            }
            case ICN::SWAP_ARROW_LEFT_TO_RIGHT:
            case ICN::SWAP_ARROW_RIGHT_TO_LEFT: {
                // Since the original game does not have such resources we could generate it from hero meeting sprite.
                const Sprite & original = GetICN( ICN::SWAPWIN, 0 );
                std::array<Image, 4> input;

                const int32_t width = 45;
                const int32_t height = 20;

                for ( Image & image : input ) {
                    image.resize( width, height );
                }

                Copy( original, 295, 270, input[0], 0, 0, width, height );
                Copy( original, 295, 291, input[1], 0, 0, width, height );
                Copy( original, 295, 363, input[2], 0, 0, width, height );
                Copy( original, 295, 384, input[3], 0, 0, width, height );

                input[1] = Flip( input[1], true, false );
                input[3] = Flip( input[3], true, false );

                Image out = ExtractCommonPattern( { &input[0], &input[1], &input[2], &input[3] } );

                // Here are 2 pixels which should be removed.
                if ( out.width() == width && out.height() == height ) {
                    out.image()[40] = 0;
                    out.transform()[40] = 1;

                    out.image()[30 + 3 * width] = 0;
                    out.transform()[30 + 3 * width] = 1;
                }

                _icnVsSprite[id].resize( 2 );
                _icnVsSprite[id][0] = ( id == ICN::SWAP_ARROW_LEFT_TO_RIGHT ) ? out : Flip( out, true, false );

                _icnVsSprite[id][1] = _icnVsSprite[id][0];
                _icnVsSprite[id][1].setPosition( -1, 1 );
                ApplyPalette( _icnVsSprite[id][1], 4 );

                return true;
            }
            case ICN::EDITPANL:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() > 5 ) {
                    Sprite & erasePanel = _icnVsSprite[id][5];
                    // To select object types for erasure we copy object buttons.
                    Copy( _icnVsSprite[id][1], 15, 68, erasePanel, 15, 68, 114, 55 );

                    // Make 3 empty buttons for terrain objects, roads and streams.
                    Fill( erasePanel, 16, 69, 24, 24, 65U );
                    Copy( erasePanel, 15, 68, erasePanel, 44, 96, 27, 27 );
                    Copy( erasePanel, 15, 68, erasePanel, 73, 96, 27, 27 );

                    // Make erase terrain objects button image.
                    Image objectsImage( 24, 24 );
                    Copy( GetICN( ICN::EDITBTNS, 2 ), 13, 4, objectsImage, 0, 0, 24, 24 );
                    AddTransparency( objectsImage, 10U );
                    AddTransparency( objectsImage, 38U );
                    AddTransparency( objectsImage, 39U );
                    AddTransparency( objectsImage, 40U );
                    AddTransparency( objectsImage, 41U );
                    AddTransparency( objectsImage, 46U );
                    Blit( objectsImage, 0, 0, erasePanel, 16, 69, 24, 24 );

                    // Make erase roads button image.
                    Blit( GetICN( ICN::ROAD, 2 ), 0, 0, erasePanel, 45, 104, 24, 5 );
                    Blit( GetICN( ICN::ROAD, 1 ), 1, 0, erasePanel, 45, 109, 24, 5 );

                    // Make erase streams button image.
                    Blit( GetICN( ICN::STREAM, 2 ), 0, 0, erasePanel, 74, 104, 24, 11 );
                }
                return true;
            case ICN::EDITOR:
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    // Fix the cycling colors in original editor main menu background.
                    fheroes2::ApplyPalette( _icnVsSprite[id][0], PAL::GetPalette( PAL::PaletteType::NO_CYCLE ) );
                }
                return true;
            case ICN::HEROES:
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    // This is the main menu image which shouldn't have any transform layer.
                    original._disableTransformLayer();
                    if ( original.width() == 640 && original.height() == 480 ) {
                        // Fix incorrect pixel at position 260x305.
                        original.image()[195460] = 31;
                    }
                }
                return true;
            case ICN::TOWNBKG3:
                // Warlock town background image contains 'empty' pixels leading to appear them as black.
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 640 && original.height() == 256 ) {
                        original._disableTransformLayer();
                        uint8_t * imageData = original.image();
                        imageData[51945] = 17;
                        imageData[61828] = 25;
                        imageData[64918] = 164;
                        imageData[77685] = 18;
                        imageData[84618] = 19;
                    }
                }
                return true;
            case ICN::MINIPORT:
                // Some heroes portraits have incorrect transparent pixels.
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() > 60 ) {
                    Sprite & original = _icnVsSprite[id][60];
                    if ( original.width() == 30 && original.height() == 22 ) {
                        original._disableTransformLayer();
                        uint8_t * imageData = original.image();
                        imageData[5] = 75;
                        imageData[310] = 48;
                        imageData[358] = 64;
                        imageData[424] = 65;
                    }
                }
                if ( _icnVsSprite[id].size() > 61 ) {
                    Sprite & original = _icnVsSprite[id][61];
                    if ( original.width() == 30 && original.height() == 22 ) {
                        original._disableTransformLayer();
                        uint8_t * imageData = original.image();
                        imageData[51] = 30;
                        imageData[80] = 28;
                        imageData[81] = 30;
                        imageData[383] = 24;
                        imageData[445] = 24;
                    }
                }
                if ( _icnVsSprite[id].size() > 65 ) {
                    Sprite & original = _icnVsSprite[id][65];
                    if ( original.width() == 30 && original.height() == 22 ) {
                        original._disableTransformLayer();
                        uint8_t * imageData = original.image();
                        imageData[499] = 60;
                        imageData[601] = 24;
                        imageData[631] = 28;
                    }
                }
                if ( _icnVsSprite[id].size() > 67 ) {
                    Sprite & original = _icnVsSprite[id][67];
                    if ( original.width() == 30 && original.height() == 22 ) {
                        original._disableTransformLayer();
                        original.image()[42] = 28;
                    }
                }
                return true;
            case ICN::MINICAPT:
                // Barbarian captain mini icon has bad pixel at position 22x2.
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() > 1 ) {
                    Sprite & original = _icnVsSprite[id][1];
                    if ( original.width() == 30 && original.height() == 22 ) {
                        original._disableTransformLayer();
                        original.image()[82] = 244;
                    }
                }
                return true;
            case ICN::PORT0091:
                // Barbarian captain has one bad pixel.
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 101 && original.height() == 93 ) {
                        original._disableTransformLayer();
                        original.image()[9084] = 77;
                    }
                }
                return true;
            case ICN::PORT0090:
                // Knight captain has multiple bad pixels.
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 101 && original.height() == 93 ) {
                        original._disableTransformLayer();
                        uint8_t * imageData = original.image();
                        imageData[2314] = 70;
                        imageData[5160] = 71;
                        imageData[5827] = 18;
                        imageData[7474] = 167;
                    }
                }
                return true;
            case ICN::PORT0092:
                // Sorceress captain has two bad transparent pixels (8x20 and 8x66).
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 101 && original.height() == 93 ) {
                        original._disableTransformLayer();
                        uint8_t * imageData = original.image();
                        imageData[2028] = 42;
                        imageData[6674] = 100;
                    }
                }
                return true;
            case ICN::PORT0095:
                // Necromancer captain have incorrect transparent pixel at position 8x22.
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 101 && original.height() == 93 ) {
                        original._disableTransformLayer();
                        original.image()[2230] = 212;
                    }
                }
                return true;
            case ICN::CSTLWZRD:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 8 ) {
                    // Statue image has bad pixels.
                    Sprite & original = _icnVsSprite[id][7];
                    if ( original.width() == 135 && original.height() == 57 ) {
                        original._disableTransformLayer();
                        uint8_t * imageData = original.image();
                        imageData[3687] = 50;
                        imageData[5159] = 108;
                        imageData[5294] = 108;
                    }
                }
                if ( _icnVsSprite[id].size() > 28 ) {
                    // Mage tower image has a bad pixel.
                    for ( const uint32_t index : { 23, 28 } ) {
                        Sprite & original = _icnVsSprite[id][index];
                        if ( original.width() == 135 && original.height() == 57 ) {
                            original._disableTransformLayer();
                            original.image()[4333] = 23;
                        }
                    }
                }
                return true;
            case ICN::CSTLCAPK:
                // Knight captain has a bad pixel.
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 2 ) {
                    Sprite & original = _icnVsSprite[id][1];
                    if ( original.width() == 84 && original.height() == 81 ) {
                        original._disableTransformLayer();
                        original.image()[4934] = 18;
                    }
                }
                return true;
            case ICN::CSTLCAPW:
                // Warlock captain quarters have bad pixels.
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 84 && original.height() == 81 ) {
                        original._disableTransformLayer();
                        uint8_t * imageData = original.image();
                        imageData[1692] = 26;
                        imageData[2363] = 32;
                        imageData[2606] = 21;
                        imageData[2608] = 21;
                    }
                }
                return true;
            case ICN::CSTLSORC:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 14 ) {
                    // Rainbow has bad pixels.
                    Sprite & original = _icnVsSprite[id][13];
                    if ( original.width() == 135 && original.height() == 57 ) {
                        original._disableTransformLayer();
                        uint8_t * imageData = original.image();
                        imageData[2047] = 160;
                        imageData[2052] = 159;
                        imageData[2055] = 160;
                        imageData[2060] = 67;
                        imageData[2063] = 159;
                        imageData[2067] = 67;
                        imageData[2184] = 67;
                        imageData[2192] = 158;
                        imageData[3508] = 67;
                        imageData[3641] = 67;
                        imageData[3773] = 69;
                        imageData[3910] = 67;
                        imageData[4039] = 69;
                        imageData[4041] = 67;
                        imageData[4172] = 67;
                        imageData[4578] = 69;
                    }
                }
                if ( _icnVsSprite[id].size() >= 25 ) {
                    // Red tower has bad pixels.
                    Sprite & original = _icnVsSprite[id][24];
                    if ( original.width() == 135 && original.height() == 57 ) {
                        original._disableTransformLayer();
                        uint8_t * imageData = original.image();
                        imageData[2830] = 165;
                        imageData[3101] = 165;
                        imageData[3221] = 69;
                    }
                }
                return true;
            case ICN::COLOR_CURSOR_ADVENTURE_MAP:
            case ICN::MONO_CURSOR_ADVENTURE_MAP: {
                // Create needed digits.
                const std::vector<Point> twoPoints = { { 2, 1 }, { 3, 1 }, { 1, 2 }, { 4, 2 }, { 3, 3 }, { 2, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 }, { 4, 5 } };
                const std::vector<Point> threePoints = { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 }, { 4, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 } };
                const std::vector<Point> fourPoints = { { 1, 1 }, { 3, 1 }, { 1, 2 }, { 3, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 }, { 4, 3 }, { 3, 4 }, { 3, 5 } };
                const std::vector<Point> fivePoints
                    = { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 1, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 }, { 4, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 } };
                const std::vector<Point> sixPoints = { { 2, 1 }, { 3, 1 }, { 1, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 }, { 1, 4 }, { 4, 4 }, { 2, 5 }, { 3, 5 } };
                const std::vector<Point> sevenPoints = { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 4, 2 }, { 3, 3 }, { 2, 4 }, { 2, 5 } };
                const std::vector<Point> plusPoints = { { 2, 1 }, { 1, 2 }, { 2, 2 }, { 3, 2 }, { 2, 3 } };

                const bool isColorCursor = ( id == ICN::COLOR_CURSOR_ADVENTURE_MAP );
                const uint8_t digitColor = isColorCursor ? 115 : 11;

                std::vector<Image> digits( 7 );
                digits[0] = createDigit( 6, 7, twoPoints, digitColor );
                digits[1] = createDigit( 6, 7, threePoints, digitColor );
                digits[2] = createDigit( 6, 7, fourPoints, digitColor );
                digits[3] = createDigit( 6, 7, fivePoints, digitColor );
                digits[4] = createDigit( 6, 7, sixPoints, digitColor );
                digits[5] = createDigit( 6, 7, sevenPoints, digitColor );
                digits[6] = addDigit( digits[5], createDigit( 5, 5, plusPoints, digitColor ), { -1, -1 } );

                _icnVsSprite[id].reserve( 7 * 8 );

                const int originalCursorId = isColorCursor ? ICN::ADVMCO : ICN::MONO_CURSOR_ADVMBW;

                populateCursorIcons( _icnVsSprite[id], GetICN( originalCursorId, 4 ), digits, isColorCursor ? Point( -2, 1 ) : Point( -4, -6 ) );
                populateCursorIcons( _icnVsSprite[id], GetICN( originalCursorId, 5 ), digits, isColorCursor ? Point( 1, 1 ) : Point( -6, -6 ) );
                populateCursorIcons( _icnVsSprite[id], GetICN( originalCursorId, 6 ), digits, isColorCursor ? Point( 0, 1 ) : Point( -8, -7 ) );
                populateCursorIcons( _icnVsSprite[id], GetICN( originalCursorId, 7 ), digits, isColorCursor ? Point( -2, 1 ) : Point( -15, -8 ) );
                populateCursorIcons( _icnVsSprite[id], GetICN( originalCursorId, 8 ), digits, isColorCursor ? Point( 1, 1 ) : Point( -16, -11 ) );
                populateCursorIcons( _icnVsSprite[id], GetICN( originalCursorId, 9 ), digits, isColorCursor ? Point( -6, 1 ) : Point( -8, -1 ) );
                populateCursorIcons( _icnVsSprite[id], GetICN( originalCursorId, 28 ), digits, isColorCursor ? Point( 0, 1 ) : Point( -8, -7 ) );

                return true;
            }
            case ICN::DISMISS_HERO_DISABLED_BUTTON:
            case ICN::NEW_CAMPAIGN_DISABLED_BUTTON: {
                _icnVsSprite[id].resize( 1 );

                int buttonIcnId = ( id == ICN::DISMISS_HERO_DISABLED_BUTTON ) ? ICN::BUTTON_VERTICAL_DISMISS : ICN::BUTTON_CAMPAIGN_GAME;

                const Sprite & released = GetICN( buttonIcnId, 0 );
                const Sprite & pressed = GetICN( buttonIcnId, 1 );

                Sprite & output = _icnVsSprite[id][0];
                output = released;

                ApplyPalette( output, PAL::GetPalette( PAL::PaletteType::DARKENING ) );

                Image common = ExtractCommonPattern( { &released, &pressed } );
                common = FilterOnePixelNoise( common );
                common = FilterOnePixelNoise( common );
                common = FilterOnePixelNoise( common );

                Blit( common, output );
                return true;
            }
            case ICN::KNIGHT_CASTLE_RIGHT_FARM: {
                _icnVsSprite[id].resize( 1 );
                Sprite & output = _icnVsSprite[id][0];
                output = GetICN( ICN::TWNKWEL2, 0 );

                ApplyPalette( output, 28, 21, output, 28, 21, 39, 1, 8 );
                ApplyPalette( output, 0, 22, output, 0, 22, 69, 1, 8 );
                ApplyPalette( output, 0, 23, output, 0, 23, 53, 1, 8 );
                ApplyPalette( output, 0, 24, output, 0, 24, 54, 1, 8 );
                ApplyPalette( output, 0, 25, output, 0, 25, 62, 1, 8 );
                return true;
            }
            case ICN::KNIGHT_CASTLE_LEFT_FARM:
                _icnVsSprite[id].resize( 1 );
                h2d::readImage( "knight_castle_left_farm.image", _icnVsSprite[id][0] );
                return true;
            case ICN::BARBARIAN_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE:
                _icnVsSprite[id].resize( 1 );
                h2d::readImage( "barbarian_castle_captain_quarter_left_side.image", _icnVsSprite[id][0] );
                return true;
            case ICN::SORCERESS_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE:
                _icnVsSprite[id].resize( 1 );
                h2d::readImage( "sorceress_castle_captain_quarter_left_side.image", _icnVsSprite[id][0] );
                return true;
            case ICN::NECROMANCER_CASTLE_STANDALONE_CAPTAIN_QUARTERS: {
                _icnVsSprite[id].resize( 1 );
                Sprite & output = _icnVsSprite[id][0];
                const Sprite & original = GetICN( ICN::TWNNCAPT, 0 );

                output = Crop( original, 21, 0, original.width() - 21, original.height() );
                output.setPosition( original.x() + 21, original.y() );

                for ( int32_t y = 47; y < output.height(); ++y ) {
                    SetTransformPixel( output, 0, y, 1 );
                }

                const Sprite & castle = GetICN( ICN::TWNNCSTL, 0 );
                Copy( castle, 402, 123, output, 1, 56, 2, 11 );

                return true;
            }
            case ICN::NECROMANCER_CASTLE_CAPTAIN_QUARTERS_BRIDGE: {
                _icnVsSprite[id].resize( 1 );
                Sprite & output = _icnVsSprite[id][0];
                const Sprite & original = GetICN( ICN::TWNNCAPT, 0 );

                output = Crop( original, 0, 0, 23, original.height() );
                output.setPosition( original.x(), original.y() );

                return true;
            }
            case ICN::ESCROLL:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() > 4 ) {
                    // fix missing black border on the right side of the "up" button
                    Sprite & out = _icnVsSprite[id][4];
                    if ( out.width() == 16 && out.height() == 16 ) {
                        out._disableTransformLayer();
                        Copy( out, 0, 0, out, 15, 0, 1, 16 );
                    }
                }
                return true;
            case ICN::MAP_TYPE_ICON: {
                // TODO: add a new icon for the Resurrection add-on map type.
                _icnVsSprite[id].resize( 2 );
                for ( Sprite & icon : _icnVsSprite[id] ) {
                    icon._disableTransformLayer();
                    icon.resize( 17, 17 );
                    icon.fill( 0 );
                }

                const Sprite & successionWarsIcon = GetICN( ICN::ARTFX, 6 );
                const Sprite & priceOfLoyaltyIcon = GetICN( ICN::ARTFX, 90 );

                if ( !successionWarsIcon.empty() ) {
                    Resize( successionWarsIcon, 0, 0, successionWarsIcon.width(), successionWarsIcon.height(), _icnVsSprite[id][0], 1, 1, 15, 15 );
                }

                if ( !priceOfLoyaltyIcon.empty() ) {
                    Resize( priceOfLoyaltyIcon, 0, 0, priceOfLoyaltyIcon.width(), priceOfLoyaltyIcon.height(), _icnVsSprite[id][1], 1, 1, 15, 15 );
                }

                return true;
            }
            case ICN::TWNWWEL2: {
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() == 7 ) {
                    if ( _icnVsSprite[id][0].width() == 122 && _icnVsSprite[id][0].height() == 226 ) {
                        FillTransform( _icnVsSprite[id][0], 0, 57, 56, 62, 1 );
                    }

                    for ( size_t i = 1; i < 7; ++i ) {
                        Sprite & original = _icnVsSprite[id][i];
                        if ( original.width() == 121 && original.height() == 151 ) {
                            FillTransform( original, 0, 0, 64, 39, 1 );
                        }
                    }
                }
                return true;
            }
            case ICN::TWNWCAPT: {
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 118 && original.height() ) {
                        // Remove shadow from left side.
                        FillTransform( original, 85, 84, 33, 26, 1 );

                        // Remove extra terrain at the bottom.
                        FillTransform( original, 0, 114, 40, 4, 1 );
                        FillTransform( original, 9, 112, 51, 2, 1 );
                        FillTransform( original, 35, 110, 47, 2, 1 );
                        FillTransform( original, 57, 108, 51, 2, 1 );
                    }
                }
                return true;
            }
            case ICN::GOOD_ARMY_BUTTON:
            case ICN::GOOD_MARKET_BUTTON: {
                _icnVsSprite[id].resize( 2 );

                loadICN( ICN::ADVBTNS );

                const int releasedIndex = ( id == ICN::GOOD_ARMY_BUTTON ) ? 0 : 4;
                Copy( GetICN( ICN::ADVBTNS, releasedIndex ), _icnVsSprite[id][0] );
                Copy( GetICN( ICN::ADVBTNS, releasedIndex + 1 ), _icnVsSprite[id][1] );

                // Make all black pixels transparent.
                AddTransparency( _icnVsSprite[id][0], 36 );
                AddTransparency( _icnVsSprite[id][1], 36 );
                AddTransparency( _icnVsSprite[id][1], 61 ); // remove the extra brown border

                return true;
            }
            case ICN::EVIL_ARMY_BUTTON:
            case ICN::EVIL_MARKET_BUTTON: {
                _icnVsSprite[id].resize( 2 );

                loadICN( ICN::ADVEBTNS );

                const int releasedIndex = ( id == ICN::EVIL_ARMY_BUTTON ) ? 0 : 4;
                Copy( GetICN( ICN::ADVEBTNS, releasedIndex ), _icnVsSprite[id][0] );
                Copy( GetICN( ICN::ADVEBTNS, releasedIndex + 1 ), _icnVsSprite[id][1] );

                // Make all black pixels transparent.
                AddTransparency( _icnVsSprite[id][0], 36 );
                AddTransparency( _icnVsSprite[id][1], 36 );

                // Add the bottom-left dark border.
                Fill( _icnVsSprite[id][1], 1, 4, 1, 30, 36 );
                Fill( _icnVsSprite[id][1], 1, 34, 31, 1, 36 );

                // Restore back black pixels in the middle of the image.
                Copy( _icnVsSprite[ICN::ADVEBTNS][releasedIndex], 9, 6, _icnVsSprite[id][0], 9, 6, 20, 22 );
                Copy( _icnVsSprite[ICN::ADVEBTNS][releasedIndex + 1], 8, 7, _icnVsSprite[id][1], 8, 7, 20, 22 );

                return true;
            }
            case ICN::SPANBTN:
            case ICN::SPANBTNE:
            case ICN::CSPANBTN:
            case ICN::CSPANBTE: {
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    // add missing part of the released button state on the left
                    Sprite & out = _icnVsSprite[id][0];

                    Sprite released( out.width() + 1, out.height() );
                    released.reset();
                    const uint8_t color = id == ICN::SPANBTN || id == ICN::CSPANBTN ? 57 : 32;
                    DrawLine( released, { 0, 3 }, { 0, out.height() - 1 }, color );
                    Blit( out, released, 1, 0 );

                    out = std::move( released );
                }
                return true;
            }
            case ICN::TRADPOSE: {
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 19 ) {
                    // fix background for TRADE and EXIT buttons
                    for ( const uint32_t i : { 16, 18 } ) {
                        Sprite pressed;
                        std::swap( pressed, _icnVsSprite[id][i] );
                        AddTransparency( pressed, 25 ); // remove too dark background

                        // take background from the empty system button
                        _icnVsSprite[id][i] = GetICN( ICN::SYSTEME, 12 );

                        // put back dark-gray pixels in the middle of the button
                        Fill( _icnVsSprite[id][i], 5, 5, 86, 17, 25 );
                        Blit( pressed, _icnVsSprite[id][i] );
                    }
                }
                return true;
            }
            case ICN::RECRUIT: {
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 10 ) {
                    // fix transparent corners on released OKAY button
                    CopyTransformLayer( _icnVsSprite[id][9], _icnVsSprite[id][8] );
                }
                return true;
            }
            case ICN::NGEXTRA: {
                LoadOriginalICN( id );
                std::vector<Sprite> & images = _icnVsSprite[id];

                if ( images.size() >= 34 ) {
                    // Fix extra column at the end of AI controlled player.
                    for ( size_t i = 27; i < 34; ++i ) {
                        if ( images[i].width() == 62 && images[i].height() == 58 ) {
                            Copy( images[i], 58, 44, images[i], 59, 44, 1, 11 );
                        }
                    }

                    for ( size_t i = 39; i < 45; ++i ) {
                        if ( images[i].width() == 62 && images[i].height() == 58 ) {
                            Copy( images[i], 58, 44, images[i], 59, 44, 1, 11 );
                        }
                    }
                }

                if ( images.size() >= 70 ) {
                    // fix transparent corners on pressed OKAY and CANCEL buttons
                    CopyTransformLayer( images[66], images[67] );
                    CopyTransformLayer( images[68], images[69] );
                }
                return true;
            }
            case ICN::TWNWUP_5: {
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() && _icnVsSprite[id].front().width() == 84 && _icnVsSprite[id].front().height() == 256 ) {
                    // Fix glowing red pixel.
                    Copy( _icnVsSprite[id].front(), 52, 92, _icnVsSprite[id].front(), 54, 92, 1, 1 );
                }

                return true;
            }
            case ICN::MONO_CURSOR_ADVMBW: {
                loadICN( ICN::ADVMCO );

                _icnVsSprite[id].resize( _icnVsSprite[ICN::ADVMCO].size() );
                for ( size_t i = 0; i < _icnVsSprite[id].size(); ++i ) {
                    std::string digit;
                    if ( i < 9 ) {
                        digit += '0';
                    }
                    digit += std::to_string( i + 1 );

                    _icnVsSprite[id][i] = loadBMPFile( std::string( "ADVMBW" ) + digit + ".BMP" );
                }
                return true;
            }
            case ICN::MONO_CURSOR_SPELBW: {
                loadICN( ICN::SPELCO );

                _icnVsSprite[id].resize( _icnVsSprite[ICN::SPELCO].size() );
                for ( size_t i = 0; i < _icnVsSprite[id].size(); ++i ) {
                    std::string digit;
                    if ( i < 10 ) {
                        digit += '0';
                    }
                    digit += std::to_string( i );

                    _icnVsSprite[id][i] = loadBMPFile( std::string( "SPELBW" ) + digit + ".BMP" );
                }
                return true;
            }
            case ICN::MONO_CURSOR_CMSSBW: {
                loadICN( ICN::CMSECO );

                _icnVsSprite[id].resize( _icnVsSprite[ICN::CMSECO].size() );
                for ( size_t i = 0; i < _icnVsSprite[id].size(); ++i ) {
                    std::string digit;
                    if ( i < 9 ) {
                        digit += '0';
                    }
                    digit += std::to_string( i + 1 );

                    _icnVsSprite[id][i] = loadBMPFile( std::string( "CMSEBW" ) + digit + ".BMP" );
                }
                return true;
            }
            case ICN::ADVBTNS:
            case ICN::ADVEBTNS:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() == 16 && _icnVsSprite[id][2].width() == 36 && _icnVsSprite[id][2].height() == 36 && _icnVsSprite[id][3].width() == 36
                     && _icnVsSprite[id][3].height() == 36 ) {
                    // Add hero action button released and pressed.
                    _icnVsSprite[id].resize( 18 );
                    Copy( _icnVsSprite[id][2], _icnVsSprite[id][16] );
                    Copy( _icnVsSprite[id][3], _icnVsSprite[id][17] );

                    // Get the button's icon colors.
                    const uint8_t mainReleasedColor = _icnVsSprite[id][2].image()[7 * 36 + 26];
                    const uint8_t mainPressedColor = _icnVsSprite[id][3].image()[8 * 36 + 25];
                    const uint8_t backgroundReleasedColor = _icnVsSprite[id][2].image()[1 * 36 + 5];
                    const uint8_t backgroundPressedColor = _icnVsSprite[id][3].image()[5 * 36 + 6];

                    // Clean-up the buttons' background
                    Fill( _icnVsSprite[id][16], 23, 5, 8, 5, backgroundReleasedColor );
                    Fill( _icnVsSprite[id][16], 8, 10, 24, 8, backgroundReleasedColor );
                    Fill( _icnVsSprite[id][16], 6, 18, 24, 10, backgroundReleasedColor );
                    Fill( _icnVsSprite[id][17], 22, 6, 8, 5, backgroundPressedColor );
                    Fill( _icnVsSprite[id][17], 7, 11, 24, 8, backgroundPressedColor );
                    Fill( _icnVsSprite[id][17], 5, 19, 24, 10, backgroundPressedColor );

                    // Get the action cursor and prepare it for button. We make it a little smaller.
                    const Sprite & originalActionCursor = GetICN( ICN::ADVMCO, 9 );
                    const int32_t actionCursorWidth = originalActionCursor.width() - 1;
                    const int32_t actionCursorHeight = originalActionCursor.height() - 3;
                    Image actionCursor( actionCursorWidth, actionCursorHeight );
                    actionCursor.reset();

                    // Head.
                    Copy( originalActionCursor, 19, 1, actionCursor, 17, 2, 8, 5 );
                    Copy( originalActionCursor, 16, 7, actionCursor, 14, 7, 12, 2 );
                    actionCursor.transform()[15 + 7 * actionCursorWidth] = 1U;
                    // Tail.
                    Copy( originalActionCursor, 1, 10, actionCursor, 1, 9, 12, 11 );
                    // Middle part.
                    Copy( originalActionCursor, 14, 10, actionCursor, 13, 9, 1, 11 );
                    // Front legs.
                    Copy( originalActionCursor, 16, 10, actionCursor, 14, 9, 13, 11 );
                    // Hind legs.
                    Copy( originalActionCursor, 7, 22, actionCursor, 7, 19, 7, 7 );

                    // Make contour transparent and the horse figure filled with solid color.
                    const int32_t actionCursorSize = actionCursorWidth * actionCursorHeight;
                    for ( int32_t i = 0; i < actionCursorSize; ++i ) {
                        if ( actionCursor.transform()[i] == 1U ) {
                            // Skip transparent pixel.
                            continue;
                        }
                        if ( actionCursor.image()[i] < 152U ) {
                            // It is the contour color, make it transparent.
                            actionCursor.transform()[i] = 1U;
                        }
                        else {
                            actionCursor.image()[i] = mainPressedColor;
                        }
                    }

                    // Add shadows to the horse image.
                    updateShadow( actionCursor, { 1, -1 }, 2, true );
                    updateShadow( actionCursor, { -1, 1 }, 6, true );
                    updateShadow( actionCursor, { 2, -2 }, 4, true );
                    Blit( actionCursor, _icnVsSprite[id][17], 4, 4 );

                    // Replace colors for the released button.
                    for ( int32_t i = 0; i < actionCursorSize; ++i ) {
                        if ( actionCursor.transform()[i] == 6U ) {
                            // Disable whitening transform and set white color.
                            actionCursor.transform()[i] = 0U;
                            actionCursor.image()[i] = 10U;
                        }
                    }
                    ReplaceColorId( actionCursor, mainPressedColor, mainReleasedColor );
                    Blit( actionCursor, _icnVsSprite[id][16], 5, 3 );
                }
                return true;
            case ICN::ARTIFACT:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() > 99 ) {
                    // This fixes "Arm of the Martyr" (#88) and " Sphere of Negation" (#99) artifacts rendering which initially has some incorrect transparent pixels.
                    for ( const int32_t index : { 88, 99 } ) {
                        Sprite & originalImage = _icnVsSprite[id][index];
                        Sprite temp( originalImage.width(), originalImage.height() );
                        temp.setPosition( originalImage.x(), originalImage.y() );
                        temp._disableTransformLayer();
                        temp.fill( 0 );
                        Blit( originalImage, temp );
                        originalImage = std::move( temp );
                    }
                }
                return true;
            case ICN::OBJNARTI:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() == 206 ) {
                    // If we have the Price of Loyalty assets we make a map sprite for the Magic Book artifact.
                    _icnVsSprite[id].resize( 208 );

                    // Magic book sprite shadow.
                    _icnVsSprite[id][206] = _icnVsSprite[id][162];
                    FillTransform( _icnVsSprite[id][206], 0, 0, 5, 1, 1U );
                    FillTransform( _icnVsSprite[id][206], 0, 1, 2, 1, 1U );
                    FillTransform( _icnVsSprite[id][206], 2, 1, 4, 1, 3U );
                    FillTransform( _icnVsSprite[id][206], 0, 2, 3, 1, 3U );
                    FillTransform( _icnVsSprite[id][206], 18, 1, 2, 1, 1U );
                    FillTransform( _icnVsSprite[id][206], 17, 2, 3, 1, 3U );
                    FillTransform( _icnVsSprite[id][206], 20, 2, 1, 1, 1U );
                    FillTransform( _icnVsSprite[id][206], 19, 3, 2, 1, 3U );

                    // Magic Book main sprite. We use sprite from the info dialog to make the map sprite.
                    _icnVsSprite[id][207].resize( 21, 32 );
                    Copy( GetICN( ICN::ARTFX, 81 ), 6, 0, _icnVsSprite[id][207], 0, 0, 21, 32 );
                    FillTransform( _icnVsSprite[id][207], 0, 0, 12, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 15, 0, 6, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 0, 1, 9, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 16, 1, 5, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 0, 2, 6, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 20, 2, 1, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 0, 4, 1, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 0, 3, 3, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 20, 25, 1, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 18, 26, 3, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 16, 27, 5, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 14, 28, 9, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 0, 29, 1, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 12, 29, 11, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 0, 30, 3, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 10, 30, 13, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 0, 31, 5, 1, 1U );
                    FillTransform( _icnVsSprite[id][207], 8, 31, 15, 1, 1U );
                }
                return true;
            case ICN::TWNSDW_5:
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() && _icnVsSprite[id][0].width() == 140 && _icnVsSprite[id][0].height() == 165 ) {
                    Sprite & image = _icnVsSprite[id][0];
                    // Red Tower has multiple defects.
                    // First one is the area between columns in middle of the Tower is prerendered. We need to remove it.
                    const int32_t windowBottom = 88;
                    FillTransform( image, 39, 68, 1, windowBottom - 68, 1 );
                    FillTransform( image, 40, 67, 1, windowBottom - 67, 1 );
                    FillTransform( image, 41, 66, 1, windowBottom - 66, 1 );
                    FillTransform( image, 42, 65, 1, windowBottom - 65, 1 );
                    FillTransform( image, 43, 66, 1, windowBottom - 66, 1 );
                    FillTransform( image, 44, 67, 1, windowBottom - 67, 1 );
                    FillTransform( image, 45, 71, 1, windowBottom - 71, 1 );
                    FillTransform( image, 49, 70, 1, windowBottom - 70, 1 );
                    FillTransform( image, 50, 68, 2, windowBottom - 68, 1 );
                    FillTransform( image, 52, 69, 1, windowBottom - 69, 1 );
                    FillTransform( image, 53, 74, 1, windowBottom - 74, 1 );
                    FillTransform( image, 57, 70, 1, windowBottom - 70, 1 );
                    FillTransform( image, 58, 67, 1, windowBottom - 67, 1 );
                    FillTransform( image, 59, 66, 1, windowBottom - 66, 1 );
                    FillTransform( image, 60, 65, 2, windowBottom - 65, 1 );
                    FillTransform( image, 62, 67, 1, windowBottom - 67, 1 );
                    FillTransform( image, 63, 69, 1, windowBottom - 69, 1 );
                    FillTransform( image, 64, 72, 1, windowBottom - 72, 1 );

                    // The lower part of the tower is truncated and blocked by partial castle's sprite. The fix is done in multiple stages.
                    // Fix right red part of the building by copying a piece of the same wall.
                    Copy( image, 67, 135, image, 67, 119, 1, 1 );
                    Copy( image, 67, 144, image, 67, 120, 2, 2 );
                    Copy( image, 67, 134, image, 67, 122, 3, 2 );
                    Copy( image, 67, 148, image, 67, 125, 1, 4 );

                    // Remove a part of the castle at the bottom left part of the image.
                    FillTransform( image, 62, 157, 3, 8, 1 );

                    // Top part of the castle's tower touches Red Tower level separation part.
                    Copy( image, 61, 101, image, 57, 101, 2, 1 );
                    Copy( image, 52, 100, image, 57, 100, 2, 1 );

                    // Generate programmatically the left part of the building.
                    std::mt19937 seededGen( 751 ); // 751 is and ID of this sprite. To keep the changes constant we need to hardcode this value.

                    fillRandomPixelsFromImage( image, { 33, 105, 4, 7 }, image, { 33, 117, 4, 39 }, seededGen );
                    fillRandomPixelsFromImage( image, { 41, 105, 5, 9 }, image, { 41, 121, 5, 36 }, seededGen );
                    fillRandomPixelsFromImage( image, { 46, 104, 4, 13 }, image, { 46, 118, 4, 39 }, seededGen );

                    Copy( image, 37, 113, image, 37, 115, 1, 2 );
                    Copy( image, 37, 104, image, 37, 117, 1, 2 );
                    Copy( image, 38, 104, image, 38, 118, 2, 1 );
                    Copy( image, 37, 113, image, 38, 117, 1, 1 );

                    // Create a temporary image to be a holder of pixels.
                    Sprite temp( 4 * 2, 8 );
                    Copy( image, 33, 105, temp, 0, 0, 4, 8 );
                    Copy( image, 41, 105, temp, 4, 0, 4, 8 );
                    fillRandomPixelsFromImage( temp, { 0, 0, temp.width(), temp.height() }, image, { 37, 119, 4, 37 }, seededGen );

                    Copy( image, 35, 131, image, 35, 113, 2, 4 );

                    Copy( image, 43, 133, image, 43, 115, 3, 6 );

                    // Fix the main arc.
                    Copy( image, 61, 102, image, 56, 102, 3, 1 );

                    // TODO: the distribution of light inside Red Tower is actually not uniform and follows the window on from th left.
                    // However, generating such complex image requires a lot of code so we simply make the rest of the arc uniformed filled.
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 50, 110, 12, 47 }, seededGen );
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 52, 107, 9, 3 }, seededGen );
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 62, 111, 1, 46 }, seededGen );
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 63, 113, 1, 20 }, seededGen );
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 63, 141, 1, 16 }, seededGen );
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 64, 115, 1, 17 }, seededGen );
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 64, 152, 1, 5 }, seededGen );
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 65, 116, 1, 15 }, seededGen );
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 66, 118, 1, 12 }, seededGen );
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 51, 108, 1, 2 }, seededGen );
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 55, 103, 5, 4 }, seededGen );
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 61, 109, 1, 1 }, seededGen );
                    fillRandomPixelsFromImage( image, { 61, 104, 2, 3 }, image, { 52, 106, 1, 1 }, seededGen );
                }
                return true;
            case ICN::SCENIBKG:
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() && _icnVsSprite[id][0].width() == 436 && _icnVsSprite[id][0].height() == 476 ) {
                    const Sprite & helper = GetICN( ICN::CSPANBKE, 1 );
                    if ( !helper.empty() ) {
                        Sprite & original = _icnVsSprite[id][0];
                        Sprite temp( original.width(), original.height() + 12 );
                        temp.reset();
                        Copy( original, 0, 0, temp, 0, 0, original.width(), original.height() );
                        Copy( helper, 0, helper.height() - 12, temp, 0, temp.height() - 12, 300, 12 );
                        Copy( helper, helper.width() - ( temp.width() - 300 ), helper.height() - 12, temp, 300 - 16, temp.height() - 12, temp.width() - 300, 12 );
                        original = std::move( temp );
                    }
                }
                return true;
            case ICN::CSTLCAPS:
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() && _icnVsSprite[id][0].width() == 84 && _icnVsSprite[id][0].height() == 81 ) {
                    const Sprite & castle = GetICN( ICN::TWNSCSTL, 0 );
                    if ( !castle.empty() ) {
                        Blit( castle, 206, 106, _icnVsSprite[id][0], 2, 2, 33, 67 );
                    }
                }
                return true;
            case ICN::LGNDXTRA:
                // Exit button is too huge due to 1 pixel presence at the bottom of the image.
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 6 ) {
                    auto & original = _icnVsSprite[id];
                    if ( original[4].height() == 142 ) {
                        const Point offset( original[4].x(), original[4].y() );
                        original[4] = Crop( original[4], 0, 0, original[4].width(), 25 );
                        original[4].setPosition( offset.x, offset.y );
                    }

                    if ( original[5].height() == 142 ) {
                        const Point offset( original[5].x(), original[5].y() );
                        original[5] = Crop( original[5], 0, 0, original[5].width(), 25 );
                        original[5].setPosition( offset.x, offset.y );
                    }
                }
                return true;
            case ICN::LGNDXTRE:
                // Exit button is too huge due to 1 pixel presence at the bottom of the image.
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 6 ) {
                    auto & original = _icnVsSprite[id];
                    if ( original[4].height() == 142 ) {
                        const Point offset( original[4].x(), original[4].y() );
                        original[4] = Crop( original[4], 0, 0, original[4].width(), 25 );
                        original[4].setPosition( offset.x, offset.y );
                    }
                }
                return true;
            case ICN::ESPANBKG_EVIL: {
                _icnVsSprite[id].resize( 2 );

                const Rect roi{ 28, 28, 265, 206 };

                Sprite & output = _icnVsSprite[id][0];
                _icnVsSprite[id][0] = GetICN( ICN::CSPANBKE, 0 );
                Copy( GetICN( ICN::ESPANBKG, 0 ), roi.x, roi.y, output, roi.x, roi.y, roi.width, roi.height );

                convertToEvilInterface( output, roi );

                _icnVsSprite[id][1] = GetICN( ICN::ESPANBKG, 1 );

                return true;
            }
            case ICN::RECR2BKG_EVIL: {
                GetICN( ICN::RECR2BKG, 0 );
                _icnVsSprite[id] = _icnVsSprite[ICN::RECR2BKG];
                if ( !_icnVsSprite[id].empty() ) {
                    const Rect roi( 0, 0, _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() );
                    convertToEvilInterface( _icnVsSprite[id][0], roi );
                    copyEvilInterfaceElements( _icnVsSprite[id][0], roi );
                }

                return true;
            }
            case ICN::RECRBKG_EVIL: {
                GetICN( ICN::RECRBKG, 0 );
                _icnVsSprite[id] = _icnVsSprite[ICN::RECRBKG];
                if ( !_icnVsSprite[id].empty() ) {
                    const Rect roi( 0, 0, _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() );
                    convertToEvilInterface( _icnVsSprite[id][0], roi );
                    copyEvilInterfaceElements( _icnVsSprite[id][0], roi );
                }

                return true;
            }
            case ICN::STONEBAK_EVIL: {
                GetICN( ICN::STONEBAK, 0 );
                _icnVsSprite[id] = _icnVsSprite[ICN::STONEBAK];
                if ( !_icnVsSprite[id].empty() ) {
                    const Rect roi( 0, 0, _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() );
                    convertToEvilInterface( _icnVsSprite[id][0], roi );
                }

                return true;
            }
            case ICN::STONEBAK_SMALL_POL: {
                _icnVsSprite[id].resize( 1 );
                const fheroes2::Sprite & original = GetICN( ICN::X_CMPBKG, 0 );
                if ( !original.empty() ) {
                    _icnVsSprite[id][0] = Crop( original, original.width() - 272, original.height() - 52, 244, 28 );
                }
                return true;
            }
            case ICN::REDBAK_SMALL_VERTICAL: {
                _icnVsSprite[id].resize( 1 );
                const fheroes2::Sprite & original = GetICN( ICN::HEROBKG, 0 );
                if ( !original.empty() ) {
                    _icnVsSprite[id][0] = Crop( original, 0, 0, 37, 230 );
                }
                return true;
            }
            case ICN::WELLBKG_EVIL: {
                GetICN( ICN::WELLBKG, 0 );
                _icnVsSprite[id] = _icnVsSprite[ICN::WELLBKG];
                if ( !_icnVsSprite[id].empty() ) {
                    const Rect roi( 0, 0, _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() - 19 );
                    convertToEvilInterface( _icnVsSprite[id][0], roi );
                }

                return true;
            }
            case ICN::CASLWIND_EVIL: {
                GetICN( ICN::CASLWIND, 0 );
                _icnVsSprite[id] = _icnVsSprite[ICN::CASLWIND];
                if ( !_icnVsSprite[id].empty() ) {
                    const Rect roi( 0, 0, _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() );
                    convertToEvilInterface( _icnVsSprite[id][0], roi );
                }

                return true;
            }
            case ICN::CASLXTRA_EVIL: {
                GetICN( ICN::CASLXTRA, 0 );
                _icnVsSprite[id] = _icnVsSprite[ICN::CASLXTRA];
                if ( !_icnVsSprite[id].empty() ) {
                    const Rect roi( 0, 0, _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() );
                    convertToEvilInterface( _icnVsSprite[id][0], roi );
                }

                return true;
            }
            case ICN::STRIP_BACKGROUND_EVIL: {
                _icnVsSprite[id].resize( 1 );
                _icnVsSprite[id][0] = GetICN( ICN::STRIP, 11 );

                const Rect roi( 0, 0, _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() - 7 );
                convertToEvilInterface( _icnVsSprite[id][0], roi );

                return true;
            }
            case ICN::B_BFLG32:
            case ICN::G_BFLG32:
            case ICN::R_BFLG32:
            case ICN::Y_BFLG32:
            case ICN::O_BFLG32:
            case ICN::P_BFLG32:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() > 31 && _icnVsSprite[id][31].height() == 248 ) {
                    Sprite & original = _icnVsSprite[id][31];
                    Sprite temp = Crop( original, 0, 0, original.width(), 4 );
                    temp.setPosition( original.x(), original.y() );

                    original = std::move( temp );
                }
                return true;
            case ICN::FLAG32:
                LoadOriginalICN( id );
                // Only first 14 images are properly aligned within an Adventure Map tile. The rest of images should be rendered on multiple tiles.
                // To keep proper rendering logic we are creating new images by diving existing ones into 2 parts and setting up new sprite offsets.
                // This helps to solve the problem with rendering order.
                _icnVsSprite[id].resize( 128 + _icnVsSprite[id].size() * 2 );
                for ( int32_t i = 14; i < 14 + 7; ++i ) {
                    const Sprite & original = _icnVsSprite[id][i];

                    _icnVsSprite[id][i + 128] = Crop( original, 0, 0, 32 - original.x(), original.height() );
                    _icnVsSprite[id][i + 128].setPosition( original.x(), 32 + original.y() );

                    _icnVsSprite[id][i + 128 + 7] = Crop( original, 32 - original.x(), 0, original.width(), original.height() );
                    _icnVsSprite[id][i + 128 + 7].setPosition( 0, 32 + original.y() );
                }

                for ( int32_t i = 42; i < 42 + 7; ++i ) {
                    const Sprite & original = _icnVsSprite[id][i];

                    _icnVsSprite[id][i + 128] = Crop( original, 0, 0, -original.x(), original.height() );
                    _icnVsSprite[id][i + 128].setPosition( 32 + original.x(), original.y() );

                    _icnVsSprite[id][i + 128 + 7] = Crop( original, -original.x(), 0, original.width(), original.height() );
                    _icnVsSprite[id][i + 128 + 7].setPosition( 0, original.y() );
                }
                return true;
            case ICN::SHADOW32:
                LoadOriginalICN( id );
                // The shadow sprite of hero needs to be shifted to match the hero sprite.
                if ( _icnVsSprite[id].size() == 86 ) {
                    // Direction: TOP (0-8), TOP_RIGHT (9-17), RIGHT (18-26), BOTTOM_RIGHT (27-35), BOTTOM (36-44)
                    for ( int32_t i = 0; i < 45; ++i ) {
                        Sprite & original = _icnVsSprite[id][i];
                        original.setPosition( original.x(), original.y() - 3 );
                    }

                    // Direction:TOP_LEFT
                    for ( int32_t i = 59; i < 68; ++i ) {
                        Sprite & original = _icnVsSprite[id][i];
                        original.setPosition( original.x() + 1, original.y() - 3 );
                    }

                    // Direction:LEFT
                    for ( int32_t i = 68; i < 77; ++i ) {
                        Sprite & original = _icnVsSprite[id][i];
                        original.setPosition( original.x() - 5, original.y() - 3 );
                    }

                    // Direction:BOTTOM_LEFT
                    for ( int32_t i = 77; i < 86; ++i ) {
                        Sprite & original = _icnVsSprite[id][i];
                        if ( i == 80 ) {
                            // This sprite needs extra fix.
                            original.setPosition( original.x() - 5, original.y() - 3 );
                        }
                        else {
                            original.setPosition( original.x() - 10, original.y() - 3 );
                        }
                    }
                }
                return true;
            case ICN::MINI_MONSTER_IMAGE:
            case ICN::MINI_MONSTER_SHADOW: {
                // It doesn't matter which image is being called. We are generating both of them at the same time.
                loadICN( ICN::MINIMON );

                // Minotaur King original Adventure map sprite has blue armlets. We make them gold to correspond the ICN::MINOTAU2.
                if ( _icnVsSprite[ICN::MINIMON].size() > 303 ) {
                    // The gold color gradient has -42 offset from blue color gradient.
                    if ( _icnVsSprite[ICN::MINIMON][297].width() == 38 && _icnVsSprite[ICN::MINIMON][297].height() == 34 ) {
                        // We update these pixels: 29x15, 30x15, 31x15, 30x16.
                        for ( const uint32_t pixelNumber : { 599, 600, 601, 638 } ) {
                            _icnVsSprite[ICN::MINIMON][297].image()[pixelNumber] -= 42;
                        }
                    }
                    for ( uint32_t icnNumber = 298; icnNumber < 300; ++icnNumber ) {
                        if ( _icnVsSprite[ICN::MINIMON][icnNumber].width() == 44 && _icnVsSprite[ICN::MINIMON][icnNumber].height() == 32 ) {
                            // We update these pixels: 29x17, 30x17, 32x17, 30x18, 31x18, 38x18, 38x19, 38x20.
                            for ( const uint32_t pixelNumber : { 777, 778, 780, 822, 823, 830, 874, 918 } ) {
                                _icnVsSprite[ICN::MINIMON][icnNumber].image()[pixelNumber] -= 42;
                            }
                        }
                    }
                    if ( _icnVsSprite[ICN::MINIMON][300].width() == 45 && _icnVsSprite[ICN::MINIMON][300].height() == 32 ) {
                        // We update these pixels: 30x17, 31x17, 33x17, 31x18, 32x18, 39x18, 39x19, 39x20
                        for ( const uint32_t pixelNumber : { 795, 796, 798, 841, 842, 849, 894, 939 } ) {
                            _icnVsSprite[ICN::MINIMON][300].image()[pixelNumber] -= 42;
                        }
                    }
                    if ( _icnVsSprite[ICN::MINIMON][301].width() == 45 && _icnVsSprite[ICN::MINIMON][301].height() == 32 ) {
                        // We update these pixels: 29x17, 30x17, 32x17, 30x18, 31x18, 39x18, 39x19, 39x20
                        for ( const uint32_t pixelNumber : { 794, 795, 797, 840, 841, 849, 894, 939 } ) {
                            _icnVsSprite[ICN::MINIMON][301].image()[pixelNumber] -= 42;
                        }
                    }
                    if ( _icnVsSprite[ICN::MINIMON][302].width() == 45 && _icnVsSprite[ICN::MINIMON][302].height() == 32 ) {
                        // We update these pixels: 35x16, 29x17, 30x17, 32x17, 33x17, 34x17, 30x18, 31x18, 31x19, 32x20.
                        for ( const uint32_t pixelNumber : { 755, 794, 795, 797, 798, 799, 840, 841, 886, 932 } ) {
                            _icnVsSprite[ICN::MINIMON][302].image()[pixelNumber] -= 42;
                        }
                    }
                    if ( _icnVsSprite[ICN::MINIMON][303].width() == 44 && _icnVsSprite[ICN::MINIMON][303].height() == 32 ) {
                        // We update these pixels: 29x17, 30x17, 30x18, 31x18, 31x19.
                        for ( const uint32_t pixelNumber : { 777, 778, 822, 823, 867 } ) {
                            _icnVsSprite[ICN::MINIMON][303].image()[pixelNumber] -= 42;
                        }
                    }
                }

                // TODO: optimize image sizes.
                _icnVsSprite[ICN::MINI_MONSTER_IMAGE] = _icnVsSprite[ICN::MINIMON];
                _icnVsSprite[ICN::MINI_MONSTER_SHADOW] = _icnVsSprite[ICN::MINIMON];

                for ( Sprite & image : _icnVsSprite[ICN::MINI_MONSTER_IMAGE] ) {
                    uint8_t * transform = image.transform();
                    const uint8_t * transformEnd = transform + image.width() * image.height();
                    for ( ; transform != transformEnd; ++transform ) {
                        if ( *transform > 1 ) {
                            *transform = 1;
                        }
                    }
                }

                for ( Sprite & image : _icnVsSprite[ICN::MINI_MONSTER_SHADOW] ) {
                    uint8_t * transform = image.transform();
                    const uint8_t * transformEnd = transform + image.width() * image.height();
                    for ( ; transform != transformEnd; ++transform ) {
                        if ( *transform == 0 ) {
                            *transform = 1;
                        }
                    }
                }

                return true;
            }
            case ICN::BUTTON_GOOD_FONT_RELEASED:
            case ICN::BUTTON_GOOD_FONT_PRESSED:
            case ICN::BUTTON_EVIL_FONT_RELEASED:
            case ICN::BUTTON_EVIL_FONT_PRESSED: {
                generateBaseButtonFont( _icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], _icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED],
                                        _icnVsSprite[ICN::BUTTON_EVIL_FONT_RELEASED], _icnVsSprite[ICN::BUTTON_EVIL_FONT_PRESSED] );
                return true;
            }
            case ICN::HISCORE: {
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() == 9 ) {
                    // Campaign title bar needs to include rating.
                    const int32_t imageHeight = _icnVsSprite[id][7].height();
                    const Sprite temp = Crop( _icnVsSprite[id][7], 215, 0, 300, imageHeight );

                    Copy( temp, 0, 0, _icnVsSprite[id][7], 215 - 57, 0, temp.width(), imageHeight );
                    Copy( _icnVsSprite[id][6], 324, 0, _icnVsSprite[id][7], 324, 0, _icnVsSprite[id][6].width() - 324, imageHeight );
                }
                return true;
            }
            case ICN::SPELLINL: {
                LoadOriginalICN( id );

                if ( _icnVsSprite[id].size() > 11 ) {
                    // Replace petrification spell mini-icon.
                    h2d::readImage( "petrification_spell_icon_mini.image", _icnVsSprite[id][11] );
                }

                return true;
            }
            case ICN::EMPTY_GOOD_BUTTON:
            case ICN::EMPTY_EVIL_BUTTON: {
                const bool isGoodInterface = ( id == ICN::EMPTY_GOOD_BUTTON );
                const int32_t originalId = isGoodInterface ? ICN::SYSTEM : ICN::SYSTEME;
                loadICN( originalId );

                if ( _icnVsSprite[originalId].size() < 13 ) {
                    return true;
                }

                _icnVsSprite[id].resize( 2 );

                Sprite & released = _icnVsSprite[id][0];
                Sprite & pressed = _icnVsSprite[id][1];

                released = _icnVsSprite[originalId][11];

                // fix single bright pixel in the left part of the text area of the released state buttons
                Fill( released, 8, 7, 1, 1, getButtonFillingColor( true, isGoodInterface ) );

                const Sprite & originalPressed = GetICN( originalId, 12 );

                if ( originalPressed.width() > 2 && originalPressed.height() > 2 ) {
                    pressed.resize( originalPressed.width(), originalPressed.height() );
                    // copy the original pressed button but add the missing darker leftside border from the released state
                    Copy( released, 0, 0, pressed, 0, 0, 1, released.height() );
                    Copy( originalPressed, 0, 0, pressed, 1, 0, originalPressed.width() - 1, originalPressed.height() );

                    // Make the background transparent.
                    FillTransform( pressed, 1, 0, pressed.width() - 1, 1, 1 );
                    FillTransform( pressed, pressed.width() - 1, 1, 1, pressed.height() - 1, 1 );

                    FillTransform( pressed, 1, 1, 2, 1, 1 );
                    FillTransform( pressed, 1, 2, 1, 1, 1 );

                    FillTransform( pressed, pressed.width() - 3, 1, 2, 1, 1 );
                    FillTransform( pressed, pressed.width() - 2, 2, 1, 1, 1 );

                    FillTransform( pressed, pressed.width() - 4, pressed.height() - 1, 3, 1, 1 );
                    FillTransform( pressed, pressed.width() - 3, pressed.height() - 2, 2, 1, 1 );
                    FillTransform( pressed, pressed.width() - 2, pressed.height() - 3, 1, 1, 1 );
                }

                return true;
            }
            case ICN::EMPTY_POL_BUTTON: {
                const int originalID = ICN::X_CMPBTN;
                loadICN( originalID );

                if ( _icnVsSprite[originalID].size() < 8 ) {
                    return true;
                }

                _icnVsSprite[id].resize( 2 );
                // move dark border to new released state from original pressed state button
                const Sprite & originalReleased = GetICN( originalID, 4 );
                const Sprite & originalPressed = GetICN( originalID, 5 );
                if ( originalReleased.width() != 94 && originalPressed.width() != 94 && originalReleased.height() < 5 && originalPressed.height() < 5 ) {
                    return true;
                }
                Sprite & releasedWithDarkBorder = _icnVsSprite[id][0];
                releasedWithDarkBorder.resize( originalReleased.width() + 2, originalReleased.height() + 1 );
                releasedWithDarkBorder.reset();

                Copy( originalReleased, 0, 0, releasedWithDarkBorder, 1, 0, originalReleased.width(), originalReleased.height() );
                Copy( originalReleased, 0, 2, releasedWithDarkBorder, 1, 21, 1, 1 );
                Copy( originalReleased, 0, 2, releasedWithDarkBorder, 2, 22, 1, 1 );
                Copy( originalPressed, 0, 2, releasedWithDarkBorder, 0, 3, 1, 19 );
                Copy( originalPressed, 0, originalPressed.height() - 1, releasedWithDarkBorder, 0, originalPressed.height(), originalPressed.width(), 1 );
                Copy( originalPressed, 0, 2, releasedWithDarkBorder, 1, 22, 1, 1 );

                // pressed state
                Sprite & pressed = _icnVsSprite[id][1];
                pressed.resize( originalPressed.width() + 2, originalPressed.height() + 1 );
                pressed.reset();
                Copy( originalPressed, 0, 0, pressed, 0, 1, originalPressed.width(), originalPressed.height() );

                // the empty buttons need to be widened by 1 px so that they can be evenly divided by 3 in resizeButton() in ui_tools.cpp
                Copy( originalReleased, originalReleased.width() - 5, 0, releasedWithDarkBorder, releasedWithDarkBorder.width() - 5, 0, 5, originalReleased.height() );
                Copy( originalPressed, originalPressed.width() - 5, 0, pressed, pressed.width() - 6, 1, 5, originalPressed.height() );

                const int32_t pixelPosition = 4 * 94 + 6;
                Fill( releasedWithDarkBorder, 5, 3, 88, 18, originalReleased.image()[pixelPosition] );
                Fill( pressed, 4, 5, 87, 17, originalPressed.image()[pixelPosition] );

                return true;
            }
            case ICN::EMPTY_GUILDWELL_BUTTON: {
                const int originalID = ICN::WELLXTRA;
                loadICN( originalID );

                if ( _icnVsSprite[originalID].size() < 3 ) {
                    return true;
                }
                _icnVsSprite[id].resize( 2 );

                for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                    const Sprite & original = GetICN( originalID, 0 + i );

                    Sprite & out = _icnVsSprite[id][i];
                    // the empty button needs to shortened by 1 px so that when it is divided by 3 in resizeButton() in ui_tools.h it will give an integer result
                    out.resize( original.width() - 1, original.height() );

                    Copy( original, 0, 0, out, 0, 0, original.width() - 4, original.height() );
                    Copy( original, original.width() - 3, 0, out, original.width() - 4, 0, 3, original.height() );

                    Fill( out, 7 - i * 2, 2 + i, 50 + i, 14, getButtonFillingColor( i == 0 ) );
                }

                return true;
            }
            case ICN::EMPTY_GOOD_MEDIUM_BUTTON:
            case ICN::EMPTY_EVIL_MEDIUM_BUTTON: {
                const bool isGoodInterface = ( id == ICN::EMPTY_GOOD_MEDIUM_BUTTON );
                const int32_t originalId = isGoodInterface ? ICN::APANEL : ICN::APANELE;
                loadICN( originalId );

                if ( _icnVsSprite[originalId].size() < 10 ) {
                    return true;
                }

                _icnVsSprite[id].resize( 2 );

                Sprite & released = _icnVsSprite[id][0];
                Sprite & pressed = _icnVsSprite[id][1];

                released = _icnVsSprite[originalId][2];
                pressed = _icnVsSprite[originalId][3];

                if ( released.width() > 2 && released.height() > 2 && pressed.width() > 2 && pressed.height() > 2 ) {
                    // Clean the buttons.
                    Fill( released, 28, 15, 42, 27, getButtonFillingColor( true, isGoodInterface ) );
                    Fill( pressed, 27, 16, 42, 27, getButtonFillingColor( false, isGoodInterface ) );
                }

                return true;
            }
            case ICN::EMPTY_VERTICAL_GOOD_BUTTON: {
                const int32_t originalId = ICN::HSBTNS;
                loadICN( originalId );

                if ( _icnVsSprite[originalId].size() < 9 ) {
                    return true;
                }

                _icnVsSprite[id].resize( 2 );
                const Sprite & originalReleased = GetICN( originalId, 2 );
                const Sprite & originalPressed = GetICN( originalId, 3 );

                Sprite & released = _icnVsSprite[id][0];
                Sprite & pressed = _icnVsSprite[id][1];

                if ( originalReleased.width() > 2 && originalReleased.height() > 2 && originalPressed.width() > 2 && originalPressed.height() > 2 ) {
                    released.resize( originalReleased.width() + 1, originalReleased.height() + 1 );
                    pressed.resize( originalPressed.width() + 1, originalPressed.height() + 1 );
                    released.reset();
                    pressed.reset();

                    Copy( originalReleased, 0, 0, released, 1, 0, originalReleased.width(), originalReleased.height() );
                    Copy( originalPressed, 0, 0, pressed, 1, 0, originalPressed.width(), originalPressed.height() );
                    FillTransform( released, 1, 4, 1, released.height() - 4, 1 );

                    // Fix the carried over broken transform layer of the original vertical button that is being used.
                    Image exitCommonMask = ExtractCommonPattern( { &released, &pressed } );
                    // Fix wrong non-transparent pixels of the transform layer that ExtractCommonPattern() missed.
                    FillTransform( exitCommonMask, 4, 2, 1, 115, 1 );
                    FillTransform( exitCommonMask, 5, 116, 1, 1, 1 );
                    FillTransform( exitCommonMask, 5, 117, 18, 1, 1 );
                    FillTransform( exitCommonMask, exitCommonMask.width() - 4, 114, 1, 2, 1 );

                    invertTransparency( exitCommonMask );
                    // Make the extended width and height lines transparent.
                    FillTransform( exitCommonMask, 0, 0, 1, exitCommonMask.height(), 1 );
                    FillTransform( exitCommonMask, exitCommonMask.width() - 4, exitCommonMask.height() - 1, 4, 1, 1 );

                    CopyTransformLayer( exitCommonMask, released );
                    CopyTransformLayer( exitCommonMask, pressed );

                    // Restore dark-brown lines on the left and bottom borders of the button backgrounds.
                    const Sprite & originalDismiss = GetICN( ICN::HSBTNS, 0 );

                    Copy( originalReleased, 0, 4, released, 1, 4, 1, released.height() - 4 );
                    Copy( originalDismiss, 6, originalDismiss.height() - 7, released, 2, released.height() - 1, 22, 1 );
                    Copy( originalDismiss, 6, originalDismiss.height() - 7, released, 1, released.height() - 1, 1, 1 );

                    Copy( originalPressed, 0, 4, pressed, 1, 4, 1, pressed.height() - 4 );
                    Copy( originalDismiss, 6, originalDismiss.height() - 7, pressed, 2, pressed.height() - 1, 22, 1 );
                    Copy( originalDismiss, 6, originalDismiss.height() - 7, pressed, 1, pressed.height() - 1, 1, 1 );

                    // Clean the button states' text areas.
                    Fill( released, 6, 4, 18, 111, getButtonFillingColor( true ) );
                    Fill( pressed, 5, 5, 18, 111, getButtonFillingColor( false ) );

                    // Make the background transparent by removing remaining red background pieces.
                    FillTransform( pressed, 5, 0, 21, 1, 1 );
                    FillTransform( pressed, pressed.width() - 3, 1, 2, 1, 1 );
                    FillTransform( pressed, pressed.width() - 2, 2, 2, 1, 1 );
                    FillTransform( pressed, pressed.width() - 1, 3, 1, pressed.height() - 6, 1 );
                }

                return true;
            }
            case ICN::BRCREST: {
                LoadOriginalICN( id );
                // First sprite in this ICN has incorrect transparent pixel at position 30x5.
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 50 && original.height() == 47 ) {
                        original._disableTransformLayer();
                        original.image()[280] = 117;
                    }
                }
                return true;
            }
            case ICN::CBKGWATR: {
                // Ship battlefield background has incorrect transparent pixel at position 125x36.
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 640 && original.height() == 443 ) {
                        original._disableTransformLayer();
                        original.image()[23165] = 24;
                    }
                }
                return true;
            }
            case ICN::SWAPWIN:
            case ICN::WELLBKG: {
                // Hero Meeting dialog and Castle Well images can be used with disabled transform layer.
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    _icnVsSprite[id][0]._disableTransformLayer();
                }
                return true;
            }
            case ICN::GAME_OPTION_ICON: {
                _icnVsSprite[id].resize( 2 );

                h2d::readImage( "hotkeys_icon.image", _icnVsSprite[id][0] );
                h2d::readImage( "graphics_icon.image", _icnVsSprite[id][1] );

                return true;
            }
            case ICN::COVR0010:
            case ICN::COVR0011:
            case ICN::COVR0012: {
                // The original image contains some foreign pixels that do not belong to the image.
                LoadOriginalICN( id );

                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & sprite = _icnVsSprite[id][0];
                    const uint8_t * image = sprite.image();
                    const uint8_t * imageEnd = image + static_cast<size_t>( sprite.width() ) * sprite.height();
                    uint8_t * transform = sprite.transform();

                    for ( ; image != imageEnd; ++image, ++transform ) {
                        // Mask all non white/black or brown pixels.
                        if ( *transform == 0 && *image > 36 && ( *image < 108 || *image > 130 ) ) {
                            *transform = 1;
                        }
                    }
                }

                return true;
            }
            default:
                break;
            }

            return false;
        }

        void loadICN( const int id )
        {
            if ( !_icnVsSprite[id].empty() ) {
                // The images have been loaded.
                return;
            }

            if ( !LoadModifiedICN( id ) ) {
                LoadOriginalICN( id );
            }
        }

        size_t GetMaximumICNIndex( int id )
        {
            loadICN( id );

            return _icnVsSprite[id].size();
        }

        size_t GetMaximumTILIndex( int id )
        {
            if ( _tilVsImage[id].empty() ) {
                _tilVsImage[id].resize( 4 ); // 4 possible sides

                const std::vector<uint8_t> & data = ::AGG::getDataFromAggFile( tilFileName[id] );
                if ( data.size() < headerSize ) {
                    // The important resource is absent! Make sure that you are using the correct version of the game.
                    assert( 0 );
                    return 0;
                }

                StreamBuf buffer( data );

                const size_t count = buffer.getLE16();
                const int32_t width = buffer.getLE16();
                const int32_t height = buffer.getLE16();
                if ( count < 1 || width < 1 || height < 1 || ( headerSize + count * width * height ) != data.size() ) {
                    return 0;
                }

                std::vector<Image> & originalTIL = _tilVsImage[id][0];
                decodeTILImages( data.data() + headerSize, count, width, height, originalTIL );

                for ( uint32_t shapeId = 1; shapeId < 4; ++shapeId ) {
                    std::vector<Image> & currentTIL = _tilVsImage[id][shapeId];
                    currentTIL.resize( count );

                    const bool horizontalFlip = ( shapeId & 2 ) != 0;
                    const bool verticalFlip = ( shapeId & 1 ) != 0;

                    for ( size_t i = 0; i < count; ++i ) {
                        currentTIL[i] = Flip( originalTIL[i], horizontalFlip, verticalFlip );
                    }
                }
            }

            return _tilVsImage[id][0].size();
        }

        // We have few ICNs which we need to scale like some related to main screen
        bool IsScalableICN( const int id )
        {
            switch ( id ) {
            case ICN::EDITOR:
            case ICN::HEROES:
            case ICN::BTNSHNGL:
            case ICN::SHNGANIM:
                return true;
            default:
                return false;
            }
        }

        const Sprite & GetScaledICN( const int icnId, const uint32_t index )
        {
            const Sprite & originalIcn = _icnVsSprite[icnId][index];
            const Display & display = Display::instance();

            if ( display.width() == Display::DEFAULT_WIDTH && display.height() == Display::DEFAULT_HEIGHT ) {
                return originalIcn;
            }

            if ( _icnVsScaledSprite[icnId].empty() ) {
                _icnVsScaledSprite[icnId].resize( _icnVsSprite[icnId].size() );
            }

            Sprite & resizedIcn = _icnVsScaledSprite[icnId][index];

            if ( originalIcn.singleLayer() && !resizedIcn.singleLayer() ) {
                resizedIcn._disableTransformLayer();
            }

            const double scaleFactorX = static_cast<double>( display.width() ) / Display::DEFAULT_WIDTH;
            const double scaleFactorY = static_cast<double>( display.height() ) / Display::DEFAULT_HEIGHT;

            const double scaleFactor = std::min( scaleFactorX, scaleFactorY );
            const int32_t resizedWidth = static_cast<int32_t>( std::lround( originalIcn.width() * scaleFactor ) );
            const int32_t resizedHeight = static_cast<int32_t>( std::lround( originalIcn.height() * scaleFactor ) );
            const int32_t offsetX = static_cast<int32_t>( std::lround( display.width() - Display::DEFAULT_WIDTH * scaleFactor ) ) / 2;
            const int32_t offsetY = static_cast<int32_t>( std::lround( display.height() - Display::DEFAULT_HEIGHT * scaleFactor ) ) / 2;
            assert( offsetX >= 0 && offsetY >= 0 );

            // Resize only if needed
            if ( resizedIcn.height() != resizedHeight || resizedIcn.width() != resizedWidth ) {
                resizedIcn.resize( resizedWidth, resizedHeight );
                resizedIcn.setPosition( static_cast<int32_t>( std::lround( originalIcn.x() * scaleFactor ) ) + offsetX,
                                        static_cast<int32_t>( std::lround( originalIcn.y() * scaleFactor ) ) + offsetY );
                Resize( originalIcn, resizedIcn, false );
            }
            else {
                // No need to resize but we have to update the offset.
                resizedIcn.setPosition( static_cast<int32_t>( std::lround( originalIcn.x() * scaleFactor ) ) + offsetX,
                                        static_cast<int32_t>( std::lround( originalIcn.y() * scaleFactor ) ) + offsetY );
            }

            return resizedIcn;
        }

        const Sprite & GetICN( int icnId, uint32_t index )
        {
            if ( !IsValidICNId( icnId ) ) {
                return errorImage;
            }

            if ( index >= GetMaximumICNIndex( icnId ) ) {
                return errorImage;
            }

            if ( IsScalableICN( icnId ) ) {
                return GetScaledICN( icnId, index );
            }

            return _icnVsSprite[icnId][index];
        }

        uint32_t GetICNCount( int icnId )
        {
            if ( !IsValidICNId( icnId ) ) {
                return 0;
            }

            return static_cast<uint32_t>( GetMaximumICNIndex( icnId ) );
        }

        const Image & GetTIL( int tilId, uint32_t index, uint32_t shapeId )
        {
            if ( shapeId > 3 ) {
                return errorImage;
            }

            if ( !IsValidTILId( tilId ) ) {
                return errorImage;
            }

            const size_t maxTILIndex = GetMaximumTILIndex( tilId );
            if ( index >= maxTILIndex ) {
                return errorImage;
            }

            return _tilVsImage[tilId][shapeId][index];
        }

        int32_t GetAbsoluteICNHeight( int icnId )
        {
            const uint32_t frameCount = GetICNCount( icnId );
            if ( frameCount == 0 ) {
                return 0;
            }

            int32_t height = 0;
            for ( uint32_t i = 0; i < frameCount; ++i ) {
                const int32_t offset = -GetICN( icnId, i ).y();
                if ( offset > height ) {
                    height = offset;
                }
            }

            return height;
        }

        uint32_t getCharacterLimit( const FontSize fontSize )
        {
            switch ( fontSize ) {
            case FontSize::SMALL:
                return static_cast<uint32_t>( GetMaximumICNIndex( ICN::SMALFONT ) ) + 0x20 - 1;
            case FontSize::NORMAL:
            case FontSize::LARGE:
                return static_cast<uint32_t>( GetMaximumICNIndex( ICN::FONT ) ) + 0x20 - 1;
            case FontSize::BUTTON_RELEASED:
            case FontSize::BUTTON_PRESSED:
                return static_cast<uint32_t>( GetMaximumICNIndex( ICN::BUTTON_GOOD_FONT_RELEASED ) ) + 0x20 - 1;
            default:
                assert( 0 ); // Did you add a new font size? Please add implementation.
            }

            return 0;
        }

        const Sprite & getChar( const uint8_t character, const FontType & fontType )
        {
            if ( character < 0x21 ) {
                return errorImage;
            }

            switch ( fontType.size ) {
            case FontSize::SMALL:
                switch ( fontType.color ) {
                case FontColor::WHITE:
                    return GetICN( ICN::SMALFONT, character - 0x20 );
                case FontColor::GRAY:
                    return GetICN( ICN::GRAY_SMALL_FONT, character - 0x20 );
                case FontColor::YELLOW:
                    return GetICN( ICN::YELLOW_SMALLFONT, character - 0x20 );
                default:
                    // Did you add a new font color? Add the corresponding logic for it!
                    assert( 0 );
                    break;
                }
                break;
            case FontSize::NORMAL:
                switch ( fontType.color ) {
                case FontColor::WHITE:
                    return GetICN( ICN::FONT, character - 0x20 );
                case FontColor::GRAY:
                    return GetICN( ICN::GRAY_FONT, character - 0x20 );
                case FontColor::YELLOW:
                    return GetICN( ICN::YELLOW_FONT, character - 0x20 );
                default:
                    // Did you add a new font color? Add the corresponding logic for it!
                    assert( 0 );
                    break;
                }
                break;
            case FontSize::LARGE:
                switch ( fontType.color ) {
                case FontColor::WHITE:
                    return GetICN( ICN::WHITE_LARGE_FONT, character - 0x20 );
                default:
                    // Did you add a new font color? Add the corresponding logic for it!
                    assert( 0 );
                    break;
                }
                break;
            case FontSize::BUTTON_RELEASED:
                switch ( fontType.color ) {
                case FontColor::WHITE:
                    return GetICN( ICN::BUTTON_GOOD_FONT_RELEASED, character - 0x20 );
                case FontColor::GRAY:
                    return GetICN( ICN::BUTTON_EVIL_FONT_RELEASED, character - 0x20 );
                default:
                    // Did you add a new font color? Add the corresponding logic for it!
                    assert( 0 );
                    break;
                }
                break;
            case FontSize::BUTTON_PRESSED:
                switch ( fontType.color ) {
                case FontColor::WHITE:
                    return GetICN( ICN::BUTTON_GOOD_FONT_PRESSED, character - 0x20 );
                case FontColor::GRAY:
                    return GetICN( ICN::BUTTON_EVIL_FONT_PRESSED, character - 0x20 );
                default:
                    // Did you add a new font color? Add the corresponding logic for it!
                    assert( 0 );
                    break;
                }
                break;
            default:
                // Did you add a new font size? Add the corresponding logic for it!
                assert( 0 );
                break;
            }

            assert( 0 ); // Did you add a new font size? Please add implementation.

            return errorImage;
        }

        void updateLanguageDependentResources( const SupportedLanguage language, const bool loadOriginalAlphabet )
        {
            if ( loadOriginalAlphabet || !isAlphabetSupported( language ) ) {
                if ( !alphabetPreserver.isPreserved() ) {
                    // This can happen when we try to change a language without loading assets.
                    alphabetPreserver.preserve();
                }
                else {
                    alphabetPreserver.restore();
                }
            }
            else {
                alphabetPreserver.preserve();
                // Restore original letters when changing language to avoid changes to them being carried over.
                alphabetPreserver.restore();
                generateAlphabet( language, _icnVsSprite );
            }
            generateButtonAlphabet( language, _icnVsSprite );

            // Clear language dependent resources.
            for ( const int id : languageDependentIcnId ) {
                _icnVsSprite[id].clear();
            }
        }
    }
}
