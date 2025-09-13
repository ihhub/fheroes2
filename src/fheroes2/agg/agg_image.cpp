/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2025                                             *
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
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "agg.h"
#include "agg_file.h"
#include "battle_cell.h"
#include "exception.h"
#include "game_language.h"
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
#include "ui_tool.h"

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

    const std::set<int> languageDependentIcnId{ ICN::BUTTON_WELL_MAX,
                                                ICN::BUTTONS_FILE_DIALOG_GOOD,
                                                ICN::BUTTONS_FILE_DIALOG_EVIL,
                                                ICN::BUTTONS_EDITOR_FILE_DIALOG_GOOD,
                                                ICN::BUTTONS_EDITOR_FILE_DIALOG_EVIL,
                                                ICN::BUTTON_INFO_GOOD,
                                                ICN::BUTTON_INFO_EVIL,
                                                ICN::BUTTON_QUIT_GOOD,
                                                ICN::BUTTON_QUIT_EVIL,
                                                ICN::BUTTON_SMALL_CANCEL_GOOD,
                                                ICN::BUTTON_SMALL_CANCEL_EVIL,
                                                ICN::BUTTON_SMALL_OKAY_GOOD,
                                                ICN::BUTTON_SMALL_OKAY_EVIL,
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
                                                ICN::BUTTON_OKAY_TOWN,
                                                ICN::BUTTON_EXIT_PUZZLE_DIM_DOOR_GOOD,
                                                ICN::BUTTON_EXIT_PUZZLE_DIM_DOOR_EVIL,
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
                                                ICN::BUTTON_1_GOOD,
                                                ICN::BUTTON_2_GOOD,
                                                ICN::BUTTON_3_GOOD,
                                                ICN::BUTTON_4_GOOD,
                                                ICN::BUTTON_5_GOOD,
                                                ICN::BUTTON_1_EVIL,
                                                ICN::BUTTON_2_EVIL,
                                                ICN::BUTTON_3_EVIL,
                                                ICN::BUTTON_4_EVIL,
                                                ICN::BUTTON_5_EVIL,
                                                ICN::BUTTON_MAP_SELECT_GOOD,
                                                ICN::BUTTON_MAP_SELECT_EVIL,
                                                ICN::BUTTONS_NEW_GAME_MENU_GOOD,
                                                ICN::BUTTONS_NEW_GAME_MENU_EVIL,
                                                ICN::BUTTONS_EDITOR_MENU_GOOD,
                                                ICN::BUTTONS_EDITOR_MENU_EVIL,
                                                ICN::BUTTON_GIFT_GOOD,
                                                ICN::BUTTON_GIFT_EVIL,
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
                                                ICN::BUTTON_RESET_GOOD,
                                                ICN::BUTTON_RESET_EVIL,
                                                ICN::BUTTON_START_GOOD,
                                                ICN::BUTTON_GUILDWELL_EXIT,
                                                ICN::GOOD_CAMPAIGN_BUTTONS,
                                                ICN::EVIL_CAMPAIGN_BUTTONS,
                                                ICN::POL_CAMPAIGN_BUTTONS,
                                                ICN::BUTTON_VIEWWORLD_EXIT_GOOD,
                                                ICN::BUTTON_VIEWWORLD_EXIT_EVIL,
                                                ICN::BUTTON_VERTICAL_DISMISS,
                                                ICN::BUTTON_VERTICAL_EXIT,
                                                ICN::BUTTON_VERTICAL_PATROL,
                                                ICN::BUTTON_HSCORES_VERTICAL_CAMPAIGN,
                                                ICN::BUTTON_HSCORES_VERTICAL_EXIT,
                                                ICN::BUTTON_HSCORES_VERTICAL_STANDARD,
                                                ICN::DISMISS_HERO_DISABLED_BUTTON,
                                                ICN::BUTTON_SELECT_GOOD,
                                                ICN::BUTTON_SELECT_EVIL };

    bool isLanguageDependentIcnId( const int id )
    {
        return languageDependentIcnId.count( id ) > 0;
    }

    bool useOriginalResources()
    {
        const fheroes2::SupportedLanguage currentLanguage = fheroes2::getCurrentLanguage();
        const fheroes2::SupportedLanguage resourceLanguage = fheroes2::getResourceLanguage();

        if ( currentLanguage != resourceLanguage ) {
            return false;
        }

        // We return false for the Russian assets to use engine-generated buttons.
        // It is done to fix some issues in Russian assets and for consistency in CP1251 fonts for all assets.
        switch ( currentLanguage ) {
        case fheroes2::SupportedLanguage::Czech:
        case fheroes2::SupportedLanguage::English:
        case fheroes2::SupportedLanguage::French:
        case fheroes2::SupportedLanguage::German:
        case fheroes2::SupportedLanguage::Polish:
            return true;
        default:
            return false;
        }
    }

    bool IsValidICNId( int id )
    {
        return id > ICN::UNKNOWN && static_cast<size_t>( id ) < _icnVsSprite.size();
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

    fheroes2::Image addDigit( const fheroes2::Image & original, const fheroes2::Image & digit, const fheroes2::Point & offset )
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
                                    Rand::PCG32 & seededGen )
    {
        for ( int x = outputRoi.x; x < outputRoi.x + outputRoi.width; ++x ) {
            for ( int y = outputRoi.y; y < outputRoi.y + outputRoi.height; ++y ) {
                const fheroes2::Point pixelLocation{ static_cast<int32_t>( Rand::GetWithGen( originalRoi.x, originalRoi.x + originalRoi.width - 1, seededGen ) ),
                                                     static_cast<int32_t>( Rand::GetWithGen( originalRoi.y, originalRoi.y + originalRoi.height - 1, seededGen ) ) };

                fheroes2::Copy( original, pixelLocation.x, pixelLocation.y, output, x, y, 1, 1 );
            }
        }
    }

    // This class serves the purpose of preserving the original alphabet which is loaded from AGG files for cases when we generate new language alphabet.
    class OriginalAlphabetPreserver final
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
            _icnVsSprite[ICN::GOLDEN_GRADIENT_FONT].clear();
            _icnVsSprite[ICN::GOLDEN_GRADIENT_LARGE_FONT].clear();
            _icnVsSprite[ICN::SILVER_GRADIENT_FONT].clear();
            _icnVsSprite[ICN::SILVER_GRADIENT_LARGE_FONT].clear();
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
    class ButtonFontOffsetRestorer final
    {
    public:
        ButtonFontOffsetRestorer( std::vector<fheroes2::Sprite> & font, const int32_t offsetX )
            : _font( font )
        {
            _originalXOffsets.reserve( _font.size() );

            for ( fheroes2::Sprite & characterSprite : _font ) {
                _originalXOffsets.emplace_back( characterSprite.x() );
                characterSprite.setPosition( offsetX, characterSprite.y() );
            }
        }

        ButtonFontOffsetRestorer( const ButtonFontOffsetRestorer & ) = delete;

        ~ButtonFontOffsetRestorer()
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

        ButtonFontOffsetRestorer & operator=( const ButtonFontOffsetRestorer & ) = delete;

    private:
        std::vector<fheroes2::Sprite> & _font;
        std::vector<int32_t> _originalXOffsets;
    };

    // Replace a particular pixel value by transparency value (transform layer value will be 1)
    void addTransparency( fheroes2::Image & image, const uint8_t valueToReplace )
    {
        fheroes2::ReplaceColorIdByTransformId( image, valueToReplace, 1 );
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

    // NOTE: Do not call this with an evil style button's ICN ID.
    void convertToEvilButtonBackground( fheroes2::Sprite & released, fheroes2::Sprite & pressed, const int goodButtonIcnId, const uint32_t index )
    {
        released = fheroes2::AGG::GetICN( goodButtonIcnId, index );
        pressed = fheroes2::AGG::GetICN( goodButtonIcnId, index + 1 );

        const std::vector<uint8_t> & goodToEvilPalette = PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_BUTTON );
        fheroes2::ApplyPalette( released, goodToEvilPalette );
        fheroes2::ApplyPalette( pressed, goodToEvilPalette );
    }

    void createCampaignButtonSet( const int campaignSetIcnId, const std::array<const char *, 5> & untranslatedTexts )
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

        for ( size_t i = 0; i < untranslatedTexts.size(); ++i ) {
            const size_t icnIndex = 2 * i;

            const char * translatedText = fheroes2::getSupportedText( untranslatedTexts[i], fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::getTextAdaptedSprite( _icnVsSprite[campaignSetIcnId][icnIndex], _icnVsSprite[campaignSetIcnId][icnIndex + 1], translatedText, emptyButtonIcnID,
                                            buttonBackgroundICN );
        }
    }

    void convertToEvilInterface( fheroes2::Sprite & image, const fheroes2::Rect & roi )
    {
        fheroes2::ApplyPalette( image, roi.x, roi.y, image, roi.x, roi.y, roi.width, roi.height, PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );
    }

    // Sets the upper left (offset 3 pixels), upper right (offset 2 pixels), lower right (offset 3 pixels) corners transparent.
    void setButtonCornersTransparent( fheroes2::Sprite & buttonSprite )
    {
        if ( buttonSprite.singleLayer() ) {
            // There is no transform layer to add transparency to.
            assert( 0 );
            return;
        }

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

    void fillTransparentButtonText( fheroes2::Sprite & released )
    {
        fheroes2::Sprite background( released.width(), released.height(), released.x(), released.y() );
        background.fill( 10 );
        Blit( released, background );
        released = std::move( background );
        setButtonCornersTransparent( released );
    }

    // Remove all shadows from the image and make them fully transparent.
    void setTransformLayerTransparent( fheroes2::Image & image )
    {
        assert( !image.empty() && !image.singleLayer() );

        uint8_t * transform = image.transform();
        const uint8_t * transformEnd = transform + static_cast<ptrdiff_t>( image.width() ) * image.height();

        for ( ; transform != transformEnd; ++transform ) {
            if ( *transform > 1 ) {
                *transform = 1U;
            }
        }
    }

    // Draw the given image in the center of the button images (released and pressed states) and add extra shading and brightening at the edges of the image.
    void drawImageOnButton( const fheroes2::Image & image, const int32_t maxImageWidth, const int32_t maxImageHeight, fheroes2::Image & releasedSprite,
                            fheroes2::Image & pressedSprite )
    {
        assert( !image.empty() && !releasedSprite.empty() && !pressedSprite.empty() );

        fheroes2::Image newImage( std::min( maxImageWidth, image.width() + 4 ), std::min( maxImageHeight, image.height() + 4 ) );
        newImage.reset();
        fheroes2::Blit( image, 0, 0, newImage, 2, 2, 35, 25 );
        // Remove shadow from the image.
        setTransformLayerTransparent( newImage );
        // Add extra shading and brightening at the edges of the image.
        fheroes2::updateShadow( newImage, { 1, -1 }, 2U, false );
        fheroes2::updateShadow( newImage, { 2, -2 }, 5U, false );
        fheroes2::updateShadow( newImage, { -1, 1 }, 6U, false );
        fheroes2::updateShadow( newImage, { -2, 2 }, 9U, false );
        // Draw the image on the button.
        const int32_t offsetX = ( pressedSprite.width() - newImage.width() ) / 2;
        const int32_t offsetY = ( pressedSprite.height() - newImage.height() ) / 2;
        fheroes2::Blit( newImage, pressedSprite, offsetX + 1, offsetY );
        fheroes2::ReplaceTransformIdByColorId( newImage, 6U, 10U );
        fheroes2::Blit( newImage, releasedSprite, offsetX + 2, offsetY - 1 );
    }

    void loadICN( const int id );

    void replacePOLAssetWithSW( const int id, const int assetIndex )
    {
        const std::vector<uint8_t> & body = ::AGG::getDataFromAggFile( ICN::getIcnFileName( id ), true );
        ROStreamBuf imageStream( body );

        imageStream.seek( headerSize + assetIndex * 13 );

        fheroes2::ICNHeader header1;
        imageStream >> header1;

        fheroes2::ICNHeader header2;
        imageStream >> header2;
        const uint32_t dataSize = header2.offsetData - header1.offsetData;

        const uint8_t * data = body.data() + headerSize + header1.offsetData;
        const uint8_t * dataEnd = data + dataSize;

        _icnVsSprite[id][assetIndex] = fheroes2::decodeICNSprite( data, dataEnd, header1 );
    }

    // This function returns true if sprites were successfully loaded from AGG file.
    // WARNING: this function must be called once - only in the beginning of `loadICN()` function.
    bool readIcnFromAgg( const int id )
    {
        // If this assertion blows up then something wrong with your logic and you load resources more than once!
        assert( _icnVsSprite[id].empty() );

        const std::vector<uint8_t> & body = ::AGG::getDataFromAggFile( ICN::getIcnFileName( id ), false );

        if ( body.empty() ) {
            return false;
        }

        ROStreamBuf imageStream( body );

        const uint32_t count = imageStream.getLE16();
        const uint32_t blockSize = imageStream.getLE32();
        if ( count == 0 || blockSize == 0 ) {
            return false;
        }

        _icnVsSprite[id].resize( count );

        for ( uint32_t i = 0; i < count; ++i ) {
            imageStream.seek( headerSize + i * 13 );

            fheroes2::ICNHeader header1;
            imageStream >> header1;

            // There should be enough frames for ICNs with animation. When animationFrames is equal to 32 then it is a Monochromatic image
            assert( header1.animationFrames == 32 || header1.animationFrames <= count );

            uint32_t dataSize = 0;
            if ( i + 1 != count ) {
                fheroes2::ICNHeader header2;
                imageStream >> header2;
                dataSize = header2.offsetData - header1.offsetData;
            }
            else {
                dataSize = blockSize - header1.offsetData;
            }

            if ( headerSize + header1.offsetData + dataSize > body.size() ) {
                // This is a corrupted AGG file.
                throw fheroes2::InvalidDataResources( "ICN Id " + std::to_string( id ) + ", index " + std::to_string( i )
                                                      + " is being corrupted. "
                                                        "Make sure that you own an official version of the game." );
            }

            const uint8_t * data = body.data() + headerSize + header1.offsetData;
            const uint8_t * dataEnd = data + dataSize;

            _icnVsSprite[id][i] = fheroes2::decodeICNSprite( data, dataEnd, header1 );
        }

        return true;
    }

    // Helper function for processICN
    void CopyICNWithPalette( const int icnId, const int originalIcnId, const PAL::PaletteType paletteType )
    {
        assert( icnId != originalIcnId );

        fheroes2::AGG::GetICN( originalIcnId, 0 ); // always avoid calling `readIcnFromAgg()` directly

        _icnVsSprite[icnId] = _icnVsSprite[originalIcnId];
        const std::vector<uint8_t> & palette = PAL::GetPalette( paletteType );
        for ( fheroes2::Sprite & sprite : _icnVsSprite[icnId] ) {
            ApplyPalette( sprite, palette );
        }
    }

    void generateDefaultImages( const int id )
    {
        switch ( id ) {
        case ICN::BUTTON_MAP_SELECT_GOOD: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::NGEXTRA, 64 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::NGEXTRA, 65 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "SELECT" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, ICN::EMPTY_MAP_SELECT_BUTTON, ICN::UNKNOWN );

            break;
        }
        case ICN::BUTTON_MAPSIZE_SMALL: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::REQUESTS, 9 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::REQUESTS, 10 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "S" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 46, 25 }, false, ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_MAPSIZE_MEDIUM: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::REQUESTS, 11 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::REQUESTS, 12 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "M" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 46, 25 }, false, ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_MAPSIZE_LARGE: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::REQUESTS, 13 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::REQUESTS, 14 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "L" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 46, 25 }, false, ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_MAPSIZE_XLARGE: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::REQUESTS, 15 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::REQUESTS, 16 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "X-L" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 46, 25 }, false, ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_MAPSIZE_ALL: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::REQUESTS, 17 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::REQUESTS, 18 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "ALL" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 58, 25 }, false, ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_1_GOOD:
        case ICN::BUTTON_1_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_1_EVIL );

            const char * text = fheroes2::getSupportedText( gettext_noop( "1" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 46, 25 }, isEvilInterface,
                                         isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_2_GOOD:
        case ICN::BUTTON_2_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_2_EVIL );

            const char * text = fheroes2::getSupportedText( gettext_noop( "2" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 46, 25 }, isEvilInterface,
                                         isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_3_GOOD:
        case ICN::BUTTON_3_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_3_EVIL );

            const char * text = fheroes2::getSupportedText( gettext_noop( "3" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 46, 25 }, isEvilInterface,
                                         isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_4_GOOD:
        case ICN::BUTTON_4_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_4_EVIL );

            const char * text = fheroes2::getSupportedText( gettext_noop( "4" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 46, 25 }, isEvilInterface,
                                         isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_5_GOOD:
        case ICN::BUTTON_5_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_5_EVIL );

            const char * text = fheroes2::getSupportedText( gettext_noop( "5" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 46, 25 }, isEvilInterface,
                                         isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_GUILDWELL_EXIT: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::WELLXTRA, 0 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::WELLXTRA, 1 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "EXIT" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, ICN::EMPTY_GUILDWELL_BUTTON, ICN::UNKNOWN );

            break;
        }
        case ICN::BUTTON_EXIT_TOWN: {
            std::vector<fheroes2::Sprite> & buttonStates = _icnVsSprite[id];
            buttonStates.resize( 2 );

            if ( useOriginalResources() ) {
                buttonStates[0] = fheroes2::AGG::GetICN( ICN::TREASURY, 1 );
                buttonStates[1] = fheroes2::AGG::GetICN( ICN::TREASURY, 2 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "smallerButton|EXIT" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 70, 25 }, false, ICN::BLACKBAK );

            break;
        }
        case ICN::BUTTON_OKAY_TOWN: {
            _icnVsSprite[id].resize( 2 );

            const char * text = fheroes2::getSupportedText( gettext_noop( "OKAY" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, ICN::EMPTY_GUILDWELL_BUTTON, ICN::UNKNOWN );

            break;
        }
        case ICN::BUTTON_EXIT_PUZZLE_DIM_DOOR_GOOD:
        case ICN::BUTTON_EXIT_PUZZLE_DIM_DOOR_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_EXIT_PUZZLE_DIM_DOOR_EVIL );

            if ( useOriginalResources() ) {
                const int originalButtonICN = isEvilInterface ? ICN::LGNDXTRE : ICN::LGNDXTRA;
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( originalButtonICN, 4 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( originalButtonICN, 5 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "smallerButton|EXIT" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 70, 25 }, isEvilInterface,
                                         isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_VIEWWORLD_EXIT_GOOD:
        case ICN::BUTTON_VIEWWORLD_EXIT_EVIL: {
            _icnVsSprite[id].resize( 2 );
            const bool isEvilInterface = ( id == ICN::BUTTON_VIEWWORLD_EXIT_EVIL );

            if ( useOriginalResources() ) {
                const int originalIcnId = isEvilInterface ? ICN::LGNDXTRE : ICN::LGNDXTRA;
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( originalIcnId, 2 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( originalIcnId, 3 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "smallerButton|EXIT" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 70, 35 }, isEvilInterface,
                                         isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_KINGDOM_EXIT: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::OVERVIEW, 4 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::OVERVIEW, 5 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "smallerButton|EXIT" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 89, 25 }, false, ICN::BROWNBAK );

            break;
        }
        case ICN::BUTTON_KINGDOM_HEROES: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::OVERVIEW, 0 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::OVERVIEW, 1 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "HEROES" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 89, 42 }, false, ICN::BROWNBAK );

            break;
        }
        case ICN::BUTTON_KINGDOM_TOWNS: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::OVERVIEW, 2 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::OVERVIEW, 3 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "TOWNS/\nCASTLES" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 90, 42 }, false, ICN::BROWNBAK );

            break;
        }
        case ICN::BUTTON_SMALL_CANCEL_GOOD:
        case ICN::BUTTON_SMALL_CANCEL_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_CANCEL_EVIL );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( isEvilInterface ? ICN::CPANELE : ICN::CPANEL, 8 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( isEvilInterface ? ICN::CPANELE : ICN::CPANEL, 9 );

                // To properly generate shadows and Blit the button we need to make transparent pixels in its released state the same as in the pressed state.
                copyTransformLayer( _icnVsSprite[id][0], _icnVsSprite[id][1] );

                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "CANCEL" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                  isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_SMALL_OKAY_GOOD:
        case ICN::BUTTON_SMALL_OKAY_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_OKAY_EVIL );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( isEvilInterface ? ICN::SPANBTNE : ICN::SPANBTN, 0 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( isEvilInterface ? ICN::SPANBTNE : ICN::SPANBTN, 1 );

                // To properly generate shadows and Blit the button we need to make transparent pixels in its released state the same as in the pressed state.
                copyTransformLayer( _icnVsSprite[id][0], _icnVsSprite[id][1] );

                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "OKAY" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                            isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_SMALL_ACCEPT_GOOD: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::SURRENDR, 0 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::SURRENDR, 1 );

                // Fix incorrect font color.
                ReplaceColorId( _icnVsSprite[id][0], 28, 56 );

                // To properly generate shadows and Blit the button we need to make transparent pixels in its released state the same as in the pressed state.
                copyTransformLayer( _icnVsSprite[id][0], _icnVsSprite[id][1] );

                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "ACCEPT" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, ICN::EMPTY_GOOD_BUTTON, ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_SMALL_DECLINE_GOOD: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::SURRENDR, 2 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::SURRENDR, 3 );

                // Fix incorrect font color.
                ReplaceColorId( _icnVsSprite[id][0], 28, 56 );

                // To properly generate shadows and Blit the button we need to make transparent pixels in its released state the same as in the pressed state.
                copyTransformLayer( _icnVsSprite[id][0], _icnVsSprite[id][1] );
                setButtonCornersTransparent( _icnVsSprite[id][1] );

                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "DECLINE" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, ICN::EMPTY_GOOD_BUTTON, ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_SMALL_LEARN_GOOD:
        case ICN::BUTTON_SMALL_LEARN_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_LEARN_EVIL );

            if ( useOriginalResources() ) {
                const int baseIcnID = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( baseIcnID, 9 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( baseIcnID, 10 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "LEARN" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                            isEvilInterface ? ICN::UNIFORMBAK_EVIL : ICN::UNIFORMBAK_GOOD );

            break;
        }
        case ICN::BUTTON_SMALL_TRADE_GOOD:
        case ICN::BUTTON_SMALL_TRADE_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_TRADE_EVIL );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( isEvilInterface ? ICN::TRADPOSE : ICN::TRADPOST, 15 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( isEvilInterface ? ICN::TRADPOSE : ICN::TRADPOST, 16 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "TRADE" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                  isEvilInterface ? ICN::UNIFORMBAK_EVIL : ICN::UNIFORMBAK_GOOD );
            break;
        }
        case ICN::BUTTON_SMALL_YES_GOOD:
        case ICN::BUTTON_SMALL_YES_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_YES_EVIL );

            if ( useOriginalResources() ) {
                const int baseIcnID = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( baseIcnID, 5 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( baseIcnID, 6 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "YES" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                            isEvilInterface ? ICN::UNIFORMBAK_EVIL : ICN::UNIFORMBAK_GOOD );
            break;
        }
        case ICN::BUTTON_SMALL_NO_GOOD:
        case ICN::BUTTON_SMALL_NO_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_NO_EVIL );

            if ( useOriginalResources() ) {
                const int baseIcnID = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( baseIcnID, 7 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( baseIcnID, 8 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "NO" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                            isEvilInterface ? ICN::UNIFORMBAK_EVIL : ICN::UNIFORMBAK_GOOD );
            break;
        }
        case ICN::BUTTON_SMALL_DISMISS_GOOD:
        case ICN::BUTTON_SMALL_DISMISS_EVIL:
        case ICN::BUTTON_SMALL_EXIT_GOOD:
        case ICN::BUTTON_SMALL_EXIT_EVIL:
        case ICN::BUTTON_SMALL_UPGRADE_GOOD:
        case ICN::BUTTON_SMALL_UPGRADE_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_SMALL_EXIT_EVIL || id == ICN::BUTTON_SMALL_DISMISS_EVIL || id == ICN::BUTTON_SMALL_UPGRADE_EVIL );

            if ( useOriginalResources() ) {
                // We avoid copying the embedded shadows so that we can generate our own.
                fheroes2::Sprite & released = _icnVsSprite[id][0];
                fheroes2::Sprite & pressed = _icnVsSprite[id][1];
                const int buttonIcnID = isEvilInterface ? ICN::VIEWARME : ICN::VIEWARMY;
                int buttonIcnIndex = 0;
                if ( id == ICN::BUTTON_SMALL_DISMISS_GOOD || id == ICN::BUTTON_SMALL_DISMISS_EVIL ) {
                    buttonIcnIndex = 1;
                }
                else if ( id == ICN::BUTTON_SMALL_EXIT_GOOD || id == ICN::BUTTON_SMALL_EXIT_EVIL ) {
                    buttonIcnIndex = 3;
                }
                else {
                    buttonIcnIndex = 5;
                }
                const fheroes2::Sprite & originalReleased = fheroes2::AGG::GetICN( buttonIcnID, buttonIcnIndex );
                const fheroes2::Sprite & originalPressed = fheroes2::AGG::GetICN( buttonIcnID, buttonIcnIndex + 1 );
                const int32_t originalWidth = originalReleased.width();
                const int32_t originalHeight = originalReleased.height();
                released.resize( originalWidth - 2, originalHeight - 1 );
                pressed.resize( originalWidth - 2, originalHeight - 1 );
                // We use this copy function to ensure that the output image has a transform layer because the original doesn't.
                Copy( originalReleased, 2, 0, released, 0, 0, originalWidth - 2, originalHeight - 1 );
                Copy( originalPressed, 2, 0, pressed, 0, 0, originalWidth - 2, originalHeight - 1 );

                for ( fheroes2::Sprite & image : _icnVsSprite[id] ) {
                    setButtonCornersTransparent( image );
                }
                break;
            }
            const char * text;
            if ( id == ICN::BUTTON_SMALL_DISMISS_GOOD || id == ICN::BUTTON_SMALL_DISMISS_EVIL ) {
                text = gettext_noop( "DISMISS" );
            }
            else if ( id == ICN::BUTTON_SMALL_EXIT_GOOD || id == ICN::BUTTON_SMALL_EXIT_EVIL ) {
                text = gettext_noop( "EXIT" );
            }
            else {
                text = gettext_noop( "UPGRADE" );
            }

            text = fheroes2::getSupportedText( text, fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                  isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_GIFT_GOOD: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_GIFT_EVIL );

            const char * text = fheroes2::getSupportedText( gettext_noop( "GIFT" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                            isEvilInterface ? ICN::UNIFORMBAK_EVIL : ICN::UNIFORMBAK_GOOD );

            break;
        }
        case ICN::BUTTON_SMALL_RESTART_GOOD: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::NON_UNIFORM_GOOD_RESTART_BUTTON, 0 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::NON_UNIFORM_GOOD_RESTART_BUTTON, 1 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "RESTART" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, ICN::EMPTY_GOOD_BUTTON, ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_SMALL_MAX_GOOD: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                // The original assets ICN contains button with shadow. We crop only the button.
                _icnVsSprite[id][0] = fheroes2::Crop( fheroes2::AGG::GetICN( ICN::RECRUIT, 4 ), 5, 0, 60, 25 );
                _icnVsSprite[id][1] = fheroes2::Crop( fheroes2::AGG::GetICN( ICN::RECRUIT, 5 ), 5, 0, 60, 25 );
                _icnVsSprite[id][0].setPosition( 0, 0 );
                _icnVsSprite[id][1].setPosition( 0, 0 );

                // To properly generate shadows and Blit the button we need to make some pixels transparent.
                for ( fheroes2::Sprite & image : _icnVsSprite[id] ) {
                    setButtonCornersTransparent( image );
                }

                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "MAX" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 61, 25 }, false, ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_SMALL_MIN_GOOD: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = id == ICN::BUTTON_SMALL_MIN_EVIL;

            const char * text = fheroes2::getSupportedText( gettext_noop( "MIN" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { fheroes2::AGG::GetICN( ICN::BUTTON_SMALL_MAX_GOOD, 0 ).width() - 10, 25 },
                                         isEvilInterface, isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::UNIFORM_GOOD_MIN_BUTTON:
        case ICN::UNIFORM_GOOD_MAX_BUTTON: {
            // To preserve language-specific generations from generateLanguageSpecificImages() we clean the non uniform button.
            _icnVsSprite[id].resize( 2 );
            fheroes2::Sprite & released = _icnVsSprite[id][0];
            fheroes2::Sprite & pressed = _icnVsSprite[id][1];
            const bool maxButton = id == ICN::UNIFORM_GOOD_MAX_BUTTON;
            const int buttonIcnID = maxButton ? ICN::BUTTON_SMALL_MAX_GOOD : ICN::BUTTON_SMALL_MIN_GOOD;
            released = fheroes2::AGG::GetICN( buttonIcnID, 0 );
            pressed = fheroes2::AGG::GetICN( buttonIcnID, 1 );
            // Clean the borders of the pressed state.
            FillTransform( pressed, 4, 0, pressed.width() - 3, 1, 1 );
            FillTransform( pressed, pressed.width() - 3, 1, 2, 1, 1 );
            FillTransform( pressed, pressed.width() - 2, 2, 2, 1, 1 );
            FillTransform( pressed, pressed.width() - 1, 3, 1, pressed.height() - 5, 1 );
            fheroes2::makeTransparentBackground( released, pressed, ICN::UNIFORMBAK_GOOD );

            break;
        }
        case ICN::UNIFORM_GOOD_OKAY_BUTTON:
        case ICN::UNIFORM_EVIL_OKAY_BUTTON: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::UNIFORM_EVIL_OKAY_BUTTON );

            if ( useOriginalResources() ) {
                const int baseIcnId = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( baseIcnId, 1 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( baseIcnId, 2 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "OKAY" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                            isEvilInterface ? ICN::UNIFORMBAK_EVIL : ICN::UNIFORMBAK_GOOD );
            break;
        }
        case ICN::UNIFORM_GOOD_CANCEL_BUTTON:
        case ICN::UNIFORM_EVIL_CANCEL_BUTTON: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::UNIFORM_EVIL_CANCEL_BUTTON );

            if ( useOriginalResources() ) {
                const int baseIcnId = isEvilInterface ? ICN::SYSTEME : ICN::SYSTEM;
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( baseIcnId, 3 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( baseIcnId, 4 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "CANCEL" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                  isEvilInterface ? ICN::UNIFORMBAK_EVIL : ICN::UNIFORMBAK_GOOD );

            break;
        }
        case ICN::UNIFORM_GOOD_EXIT_BUTTON:
        case ICN::UNIFORM_EVIL_EXIT_BUTTON: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::UNIFORM_EVIL_EXIT_BUTTON );
            const int baseIcnId = isEvilInterface ? ICN::TRADPOSE : ICN::TRADPOST;

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( baseIcnId, 17 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( baseIcnId, 18 );
                break;
            }

            const char * text = fheroes2::getSupportedText( gettext_noop( "EXIT" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                  isEvilInterface ? ICN::UNIFORMBAK_EVIL : ICN::UNIFORMBAK_GOOD );

            break;
        }
        case ICN::BUTTON_VERTICAL_DISMISS: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                // The original DISMISS button has a broken transform layer and thinner pressed state than released state,
                // so we use our empty vertical button as a template
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::EMPTY_VERTICAL_GOOD_BUTTON, 0 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::EMPTY_VERTICAL_GOOD_BUTTON, 1 );

                // Copy the DISMISS text back from the original button
                const fheroes2::Sprite & originalReleased = fheroes2::AGG::GetICN( ICN::HSBTNS, 0 );
                const fheroes2::Sprite & originalPressed = fheroes2::AGG::GetICN( ICN::HSBTNS, 1 );

                Copy( originalReleased, 9, 2, _icnVsSprite[id][0], 5, 2, 21, 112 );
                Copy( originalPressed, 9, 5, _icnVsSprite[id][1], 4, 5, 19, 111 );

                fheroes2::makeTransparentBackground( _icnVsSprite[id][0], _icnVsSprite[id][1], ICN::REDBAK_SMALL_VERTICAL );

                break;
            }

            // We need to temporarily remove the letter-specific X offsets in the font because if not the letters will
            // be off-centered when we are displaying one letter per line
            const ButtonFontOffsetRestorer fontReleased( _icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], -1 );
            const ButtonFontOffsetRestorer fontPressed( _icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED], -1 );

            const char * text = fheroes2::getSupportedText( gettext_noop( "D\nI\nS\nM\nI\nS\nS" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, ICN::EMPTY_VERTICAL_GOOD_BUTTON, ICN::REDBAK_SMALL_VERTICAL );

            break;
        }
        case ICN::BUTTON_VERTICAL_EXIT: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                // The original EXIT button has a broken transform layer and we need to remove the shadows,
                // so we use our empty vertical button as a template
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::EMPTY_VERTICAL_GOOD_BUTTON, 0 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::EMPTY_VERTICAL_GOOD_BUTTON, 1 );

                // Copy the EXIT text back from the original button
                const fheroes2::Sprite & originalReleased = fheroes2::AGG::GetICN( ICN::HSBTNS, 2 );
                const fheroes2::Sprite & originalPressed = fheroes2::AGG::GetICN( ICN::HSBTNS, 3 );

                Copy( originalReleased, 4, 2, _icnVsSprite[id][0], 5, 2, 21, 112 );
                Copy( originalPressed, 3, 5, _icnVsSprite[id][1], 4, 5, 19, 111 );

                fheroes2::makeTransparentBackground( _icnVsSprite[id][0], _icnVsSprite[id][1], ICN::REDBAK_SMALL_VERTICAL );

                break;
            }

            // We need to temporarily remove the letter specific X offsets in the font because if not the letters will
            // be off-centered when we are displaying one letter per line
            const ButtonFontOffsetRestorer fontReleased( _icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], -1 );
            const ButtonFontOffsetRestorer fontPressed( _icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED], -1 );

            const char * text = fheroes2::getSupportedText( gettext_noop( "E\nX\nI\nT" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, ICN::EMPTY_VERTICAL_GOOD_BUTTON, ICN::REDBAK_SMALL_VERTICAL );

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
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( originalID, originalICNIndex );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( originalID, originalICNIndex + 1 );
                break;
            }

            for ( size_t i = 0; i < _icnVsSprite[id].size(); ++i ) {
                const fheroes2::Sprite & originalButton = fheroes2::AGG::GetICN( originalID, originalICNIndex + static_cast<uint32_t>( i ) );
                fheroes2::Sprite & out = _icnVsSprite[id][i];

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

            const ButtonFontOffsetRestorer fontRestorerReleased( _icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], -1 );
            const ButtonFontOffsetRestorer fontRestorerPressed( _icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED], -1 );

            const char * translatedText = fheroes2::getSupportedText( buttonText, fheroes2::FontType{ fheroes2::FontSize::BUTTON_RELEASED, fheroes2::FontColor::WHITE } );
            fheroes2::renderTextOnButton( _icnVsSprite[id][0], _icnVsSprite[id][1], translatedText, { 3, 4 }, { 2, 5 }, { 23, 133 }, fheroes2::FontColor::WHITE );

            break;
        }
        case ICN::BUTTON_GIFT_EVIL:
        case ICN::BUTTON_MAP_SELECT_EVIL:
        case ICN::BUTTON_SMALL_ACCEPT_EVIL:
        case ICN::BUTTON_SMALL_DECLINE_EVIL:
        case ICN::BUTTON_SMALL_MAX_EVIL:
        case ICN::BUTTON_SMALL_MIN_EVIL:
        case ICN::BUTTON_SMALL_RESTART_EVIL:
        case ICN::BUTTONS_NEW_GAME_MENU_EVIL:
        case ICN::BUTTONS_EDITOR_FILE_DIALOG_EVIL:
        case ICN::BUTTONS_EDITOR_MENU_EVIL:
        case ICN::UNIFORM_EVIL_MAX_BUTTON:
        case ICN::UNIFORM_EVIL_MIN_BUTTON: {
            // We do palette swaps for these due to either one of two reasons: to generate completely new buttons, or to preserve
            // language-specific generations from generateLanguageSpecificImages().

            int goodButtonIcnID = ICN::UNKNOWN;
            if ( id == ICN::BUTTON_MAP_SELECT_EVIL ) {
                goodButtonIcnID = ICN::BUTTON_MAP_SELECT_GOOD;
            }
            else if ( id == ICN::BUTTON_SMALL_RESTART_EVIL ) {
                goodButtonIcnID = ICN::BUTTON_SMALL_RESTART_GOOD;
            }
            else if ( id == ICN::BUTTON_GIFT_EVIL ) {
                goodButtonIcnID = ICN::BUTTON_GIFT_GOOD;
            }
            else if ( id == ICN::BUTTON_SMALL_DECLINE_EVIL ) {
                goodButtonIcnID = ICN::BUTTON_SMALL_DECLINE_GOOD;
            }
            else if ( id == ICN::BUTTON_SMALL_ACCEPT_EVIL ) {
                goodButtonIcnID = ICN::BUTTON_SMALL_ACCEPT_GOOD;
            }
            else if ( id == ICN::BUTTON_SMALL_MIN_EVIL ) {
                goodButtonIcnID = ICN::BUTTON_SMALL_MIN_GOOD;
            }
            else if ( id == ICN::BUTTON_SMALL_MAX_EVIL ) {
                goodButtonIcnID = ICN::BUTTON_SMALL_MAX_GOOD;
            }
            else if ( id == ICN::UNIFORM_EVIL_MIN_BUTTON ) {
                goodButtonIcnID = ICN::UNIFORM_GOOD_MIN_BUTTON;
            }
            else if ( id == ICN::UNIFORM_EVIL_MAX_BUTTON ) {
                goodButtonIcnID = ICN::UNIFORM_GOOD_MAX_BUTTON;
            }
            else if ( id == ICN::BUTTONS_NEW_GAME_MENU_EVIL ) {
                goodButtonIcnID = ICN::BUTTONS_NEW_GAME_MENU_GOOD;
            }
            else if ( id == ICN::BUTTONS_EDITOR_FILE_DIALOG_EVIL ) {
                goodButtonIcnID = ICN::BUTTONS_EDITOR_FILE_DIALOG_GOOD;
            }
            else if ( id == ICN::BUTTONS_EDITOR_MENU_EVIL ) {
                goodButtonIcnID = ICN::BUTTONS_EDITOR_MENU_GOOD;
            }
            // Did you add new buttons?
            assert( goodButtonIcnID != ICN::UNKNOWN );

            _icnVsSprite[id].resize( fheroes2::AGG::GetICNCount( goodButtonIcnID ) );
            const size_t icnSize = _icnVsSprite[id].size();
            for ( size_t i = 0; i < ( icnSize / 2 ); ++i ) {
                convertToEvilButtonBackground( _icnVsSprite[id][i * 2], _icnVsSprite[id][i * 2 + 1], goodButtonIcnID, static_cast<uint32_t>( i * 2 ) );
            }

            break;
        }
        case ICN::BUTTONS_FILE_DIALOG_EVIL:
        case ICN::BUTTONS_FILE_DIALOG_GOOD: {
            _icnVsSprite[id].resize( 8 );

            const bool isEvilInterface = ( id == ICN::BUTTONS_FILE_DIALOG_EVIL );
            if ( useOriginalResources() ) {
                const int buttonIcnID = isEvilInterface ? ICN::CPANELE : ICN::CPANEL;
                for ( size_t i = 0; i < _icnVsSprite[id].size(); ++i ) {
                    _icnVsSprite[id][i] = fheroes2::AGG::GetICN( buttonIcnID, static_cast<uint32_t>( i ) );
                }
                break;
            }

            const fheroes2::FontType buttonFontType = fheroes2::FontType::buttonReleasedWhite();
            fheroes2::makeSymmetricBackgroundSprites( _icnVsSprite[id],
                                                      { fheroes2::getSupportedText( gettext_noop( "NEW\nGAME" ), buttonFontType ),
                                                        fheroes2::getSupportedText( gettext_noop( "LOAD\nGAME" ), buttonFontType ),
                                                        fheroes2::getSupportedText( gettext_noop( "SAVE\nGAME" ), buttonFontType ),
                                                        fheroes2::getSupportedText( gettext_noop( "QUIT" ), buttonFontType ) },
                                                      isEvilInterface, 80 );

            break;
        }
        case ICN::BUTTONS_EDITOR_FILE_DIALOG_GOOD: {
            _icnVsSprite[id].resize( 12 );

            const bool isEvilInterface = ( id == ICN::BUTTONS_EDITOR_FILE_DIALOG_EVIL );
            if ( useOriginalResources() ) {
                const int buttonIcnID = ICN::ECPANEL;
                // We don't add all the ICN buttons in original order because when we render the buttons we want a different order.
                for ( size_t i = 0; i < 4; ++i ) {
                    _icnVsSprite[id][i] = fheroes2::AGG::GetICN( buttonIcnID, static_cast<uint32_t>( i ) );
                }
                // Save Map
                _icnVsSprite[id][6] = fheroes2::AGG::GetICN( buttonIcnID, static_cast<uint32_t>( 4 ) );
                _icnVsSprite[id][7] = fheroes2::AGG::GetICN( buttonIcnID, static_cast<uint32_t>( 5 ) );
                // Add generated buttons.
                const fheroes2::FontType buttonFontType = fheroes2::FontType::buttonReleasedWhite();
                const fheroes2::Size buttonSize{ _icnVsSprite[id][0].width() - 10, _icnVsSprite[id][0].height() };
                fheroes2::makeButtonSprites( _icnVsSprite[id][4], _icnVsSprite[id][5], fheroes2::getSupportedText( gettext_noop( "START\nMAP" ), buttonFontType ),
                                             buttonSize, isEvilInterface, ICN::STONEBAK );
                fheroes2::makeButtonSprites( _icnVsSprite[id][8], _icnVsSprite[id][9], fheroes2::getSupportedText( gettext_noop( "MAIN\nMENU" ), buttonFontType ),
                                             buttonSize, isEvilInterface, ICN::STONEBAK );

                // Quit
                _icnVsSprite[id][10] = fheroes2::AGG::GetICN( buttonIcnID, static_cast<uint32_t>( 6 ) );
                _icnVsSprite[id][11] = fheroes2::AGG::GetICN( buttonIcnID, static_cast<uint32_t>( 7 ) );

                break;
            }

            const fheroes2::FontType buttonFontType = fheroes2::FontType::buttonReleasedWhite();
            fheroes2::makeSymmetricBackgroundSprites( _icnVsSprite[id],
                                                      { fheroes2::getSupportedText( gettext_noop( "NEW\nMAP" ), buttonFontType ),
                                                        fheroes2::getSupportedText( gettext_noop( "LOAD\nMAP" ), buttonFontType ),
                                                        fheroes2::getSupportedText( gettext_noop( "START\nMAP" ), buttonFontType ),
                                                        fheroes2::getSupportedText( gettext_noop( "SAVE\nMAP" ), buttonFontType ),
                                                        fheroes2::getSupportedText( gettext_noop( "MAIN\nMENU" ), buttonFontType ),
                                                        fheroes2::getSupportedText( gettext_noop( "QUIT" ), buttonFontType ) },
                                                      isEvilInterface, 86 );

            break;
        }
        case ICN::BUTTON_INFO_EVIL:
        case ICN::BUTTON_INFO_GOOD:
        case ICN::BUTTON_QUIT_EVIL:
        case ICN::BUTTON_QUIT_GOOD: {
            _icnVsSprite[id].resize( 2 );

            const bool isEvilInterface = ( id == ICN::BUTTON_QUIT_EVIL || id == ICN::BUTTON_INFO_EVIL );
            const bool isInfoButton = ( id == ICN::BUTTON_INFO_GOOD || id == ICN::BUTTON_INFO_EVIL );

            if ( useOriginalResources() ) {
                int buttonIcnID = ICN::UNKNOWN;
                std::pair<int, int> icnIndex;

                if ( isInfoButton ) {
                    buttonIcnID = isEvilInterface ? ICN::APANELE : ICN::APANEL;
                    icnIndex = { 4, 5 };
                }
                else {
                    buttonIcnID = isEvilInterface ? ICN::CPANELE : ICN::CPANEL;
                    icnIndex = { 6, 7 };
                }

                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( buttonIcnID, icnIndex.first );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( buttonIcnID, icnIndex.second );
                break;
            }

            const char * text = isInfoButton ? gettext_noop( "INFO" ) : gettext_noop( "QUIT" );

            text = fheroes2::getSupportedText( text, fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 86, 56 }, isEvilInterface,
                                         isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::BUTTONS_NEW_GAME_MENU_GOOD: {
            // Set the size depending on whether PoL assets are present or not, in which case add 4 more for campaign buttons.
            const bool isPoLPresent = !::AGG::getDataFromAggFile( ICN::getIcnFileName( ICN::X_TRACK1 ), false ).empty();
            if ( isPoLPresent ) {
                _icnVsSprite[id].resize( 28 );
            }
            else {
                _icnVsSprite[id].resize( 24 );
            }

            if ( useOriginalResources() ) {
                for ( size_t i = 0; i < 3; ++i ) {
                    _icnVsSprite[id][i * 2] = fheroes2::AGG::GetICN( ICN::BTNNEWGM, static_cast<uint32_t>( i * 2 ) );
                    _icnVsSprite[id][i * 2 + 1] = fheroes2::AGG::GetICN( ICN::BTNNEWGM, static_cast<uint32_t>( i * 2 + 1 ) );
                }
                // Add generated buttons.
                const fheroes2::FontType buttonFontType = fheroes2::FontType::buttonReleasedWhite();
                const fheroes2::Size buttonSize{ _icnVsSprite[id][0].width() - 10, _icnVsSprite[id][0].height() };
                fheroes2::makeButtonSprites( _icnVsSprite[id][6], _icnVsSprite[id][7], fheroes2::getSupportedText( gettext_noop( "BATTLE\nONLY" ), buttonFontType ),
                                             buttonSize, false, ICN::STONEBAK );
                fheroes2::makeButtonSprites( _icnVsSprite[id][8], _icnVsSprite[id][9], fheroes2::getSupportedText( gettext_noop( "SETTINGS" ), buttonFontType ),
                                             buttonSize, false, ICN::STONEBAK );
                // Add cancel button.
                _icnVsSprite[id][10] = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 6 );
                _icnVsSprite[id][11] = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 7 );
                // Add hot seat button.
                _icnVsSprite[id][12] = fheroes2::AGG::GetICN( ICN::BTNMP, 0 );
                _icnVsSprite[id][13] = fheroes2::AGG::GetICN( ICN::BTNMP, 1 );
                // Add player count buttons.
                for ( size_t i = 0; i < 5; ++i ) {
                    _icnVsSprite[id][( i + 7 ) * 2] = fheroes2::AGG::GetICN( ICN::BTNHOTST, static_cast<uint32_t>( i * 2 ) );
                    _icnVsSprite[id][( i + 7 ) * 2 + 1] = fheroes2::AGG::GetICN( ICN::BTNHOTST, static_cast<uint32_t>( i * 2 + 1 ) );
                }

                if ( isPoLPresent ) {
                    _icnVsSprite[id][24] = fheroes2::AGG::GetICN( ICN::X_LOADCM, 0 );
                    _icnVsSprite[id][25] = fheroes2::AGG::GetICN( ICN::X_LOADCM, 1 );
                    _icnVsSprite[id][26] = fheroes2::AGG::GetICN( ICN::X_LOADCM, 2 );
                    _icnVsSprite[id][27] = fheroes2::AGG::GetICN( ICN::X_LOADCM, 3 );
                }
                break;
            }
            const fheroes2::FontType buttonFontType = fheroes2::FontType::buttonReleasedWhite();
            std::vector<const char *> texts = { fheroes2::getSupportedText( gettext_noop( "STANDARD\nGAME" ), buttonFontType ),
                                                fheroes2::getSupportedText( gettext_noop( "CAMPAIGN\nGAME" ), buttonFontType ),
                                                fheroes2::getSupportedText( gettext_noop( "MULTI-\nPLAYER\nGAME" ), buttonFontType ),
                                                fheroes2::getSupportedText( gettext_noop( "BATTLE\nONLY" ), buttonFontType ),
                                                fheroes2::getSupportedText( gettext_noop( "SETTINGS" ), buttonFontType ),
                                                fheroes2::getSupportedText( gettext_noop( "CANCEL" ), buttonFontType ),
                                                fheroes2::getSupportedText( gettext_noop( "HOT SEAT" ), buttonFontType ),
                                                fheroes2::getSupportedText( gettext_noop( "2 PLAYERS" ), buttonFontType ),
                                                fheroes2::getSupportedText( gettext_noop( "3 PLAYERS" ), buttonFontType ),
                                                fheroes2::getSupportedText( gettext_noop( "4 PLAYERS" ), buttonFontType ),
                                                fheroes2::getSupportedText( gettext_noop( "5 PLAYERS" ), buttonFontType ),
                                                fheroes2::getSupportedText( gettext_noop( "6 PLAYERS" ), buttonFontType ) };
            if ( isPoLPresent ) {
                texts.emplace_back( fheroes2::getSupportedText( gettext_noop( "ORIGINAL\nCAMPAIGN" ), buttonFontType ) );
                texts.emplace_back( fheroes2::getSupportedText( gettext_noop( "EXPANSION\nCAMPAIGN" ), buttonFontType ) );
            }

            fheroes2::makeSymmetricBackgroundSprites( _icnVsSprite[id], texts, false, 117 );
            break;
        }
        case ICN::BUTTONS_EDITOR_MENU_GOOD: {
            _icnVsSprite[id].resize( 20 );

            if ( useOriginalResources() ) {
                for ( size_t i = 0; i < 2; ++i ) {
                    _icnVsSprite[id][i * 2] = fheroes2::AGG::GetICN( ICN::BTNEMAIN, static_cast<uint32_t>( i * 2 ) );
                    _icnVsSprite[id][i * 2 + 1] = fheroes2::AGG::GetICN( ICN::BTNEMAIN, static_cast<uint32_t>( i * 2 + 1 ) );
                }
                // Add generated Main Menu button.
                const fheroes2::FontType buttonFontType = fheroes2::FontType::buttonReleasedWhite();
                const fheroes2::Size buttonSize{ _icnVsSprite[id][0].width() - 10, _icnVsSprite[id][0].height() };
                fheroes2::makeButtonSprites( _icnVsSprite[id][4], _icnVsSprite[id][5], fheroes2::getSupportedText( gettext_noop( "MAIN\nMENU" ), buttonFontType ),
                                             buttonSize, false, ICN::STONEBAK );
                // Add generated Back button.
                fheroes2::makeButtonSprites( _icnVsSprite[id][6], _icnVsSprite[id][7],
                                             fheroes2::getSupportedText( gettext_noop( "editorMainMenu|BACK" ), buttonFontType ), buttonSize, false, ICN::STONEBAK );

                // Add From Scratch and Random buttons.
                for ( size_t i = 0; i < 2; ++i ) {
                    _icnVsSprite[id][( i + 4 ) * 2] = fheroes2::AGG::GetICN( ICN::BTNENEW, static_cast<uint32_t>( i * 2 ) );
                    _icnVsSprite[id][( i + 4 ) * 2 + 1] = fheroes2::AGG::GetICN( ICN::BTNENEW, static_cast<uint32_t>( i * 2 + 1 ) );
                }
                // Add map size buttons.
                for ( size_t i = 0; i < 4; ++i ) {
                    _icnVsSprite[id][( i + 6 ) * 2] = fheroes2::AGG::GetICN( ICN::BTNESIZE, static_cast<uint32_t>( i * 2 ) );
                    _icnVsSprite[id][( i + 6 ) * 2 + 1] = fheroes2::AGG::GetICN( ICN::BTNESIZE, static_cast<uint32_t>( i * 2 + 1 ) );
                }

                break;
            }
            const fheroes2::FontType buttonFontType = fheroes2::FontType::buttonReleasedWhite();
            const std::vector<const char *> texts = { fheroes2::getSupportedText( gettext_noop( "NEW\nMAP" ), buttonFontType ),
                                                      fheroes2::getSupportedText( gettext_noop( "LOAD\nMAP" ), buttonFontType ),
                                                      fheroes2::getSupportedText( gettext_noop( "MAIN\nMENU" ), buttonFontType ),
                                                      fheroes2::getSupportedText( gettext_noop( "editorMainMenu|BACK" ), buttonFontType ),
                                                      fheroes2::getSupportedText( gettext_noop( "newMap|FROM\nSCRATCH" ), buttonFontType ),
                                                      fheroes2::getSupportedText( gettext_noop( "newMap|RANDOM" ), buttonFontType ),
                                                      "36 X 36",
                                                      "72 X 72",
                                                      "108 X 108",
                                                      "144 X 144" };

            fheroes2::makeSymmetricBackgroundSprites( _icnVsSprite[id], texts, false, 117 );
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
                    const fheroes2::Sprite & originalReleased = fheroes2::AGG::GetICN( originalIcnId, i );

                    fheroes2::Sprite & released = _icnVsSprite[id][i];
                    released.resize( originalReleased.width() - 6 + offsetEvilX, originalReleased.height() - 8 );
                    released.reset();

                    Copy( originalReleased, 6 - offsetEvilX, 0, released, 0, 0, originalReleased.width() - 1, originalReleased.height() - 8 );

                    // pressed
                    const fheroes2::Sprite & originalPressed = fheroes2::AGG::GetICN( originalIcnId, i + 1 );

                    fheroes2::Sprite & pressed = _icnVsSprite[id][i + 1];
                    pressed.resize( originalPressed.width(), originalPressed.height() );
                    pressed.reset();

                    Copy( originalPressed, 0, 1, pressed, 0, 1, originalPressed.width() - 1, originalPressed.height() );
                    setButtonCornersTransparent( released );
                    fheroes2::makeTransparentBackground( released, pressed, buttonBackground );
                }

                // Generate the DIFFICULTY button because it is not present in the original resources
                const char * text = fheroes2::getSupportedText( gettext_noop( "DIFFICULTY" ), fheroes2::FontType::buttonReleasedWhite() );
                fheroes2::getTextAdaptedSprite( _icnVsSprite[id][8], _icnVsSprite[id][9], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                                buttonBackground );
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
                    const fheroes2::Sprite & originalReleased = fheroes2::AGG::GetICN( baseIcnId, i );

                    fheroes2::Sprite & released = _icnVsSprite[id][i];
                    released.resize( originalReleased.width() + 1, originalReleased.height() + 1 );
                    released.reset();

                    Copy( originalReleased, 0, 0, released, 1, 0, originalReleased.width(), originalReleased.height() );
                    const fheroes2::Sprite & originalPressed = fheroes2::AGG::GetICN( baseIcnId, i + 1 );
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
                    fheroes2::Sprite & pressed = _icnVsSprite[id][i + 1];
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
                const char * text = fheroes2::getSupportedText( gettext_noop( "DIFFICULTY" ), fheroes2::FontType::buttonReleasedWhite() );
                fheroes2::getTextAdaptedSprite( _icnVsSprite[id][8], _icnVsSprite[id][9], text, ICN::EMPTY_POL_BUTTON, ICN::STONEBAK_SMALL_POL );
                break;
            }
            createCampaignButtonSet( id, { gettext_noop( "VIEW INTRO" ), gettext_noop( "RESTART" ), gettext_noop( "OKAY" ), gettext_noop( "CANCEL" ),
                                           gettext_noop( "DIFFICULTY" ) } );

            break;
        }
        case ICN::BUTTON_EXIT_HEROES_MEETING: {
            _icnVsSprite[id].resize( 2 );

            if ( useOriginalResources() ) {
                _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::SWAPBTN, 0 );
                _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::SWAPBTN, 1 );
                // fix some wrong pixels in the original pressed state
                setButtonCornersTransparent( _icnVsSprite[id][1] );
                break;
            }

            // The heroes meeting screen has an embedded shadow so the button needs to be fixed at the same size as the original one.
            // TODO: Remove the embedded shadow and button in the heroes meeting screen and use getTextAdaptedSprite() instead.
            const char * text = fheroes2::getSupportedText( gettext_noop( "smallerButton|EXIT" ), fheroes2::FontType::buttonReleasedWhite() );
            fheroes2::makeButtonSprites( _icnVsSprite[id][0], _icnVsSprite[id][1], text, { 70, 25 }, false, ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_RESET_GOOD:
        case ICN::BUTTON_RESET_EVIL: {
            const bool isEvilInterface = ( id == ICN::BUTTON_RESET_EVIL );

            _icnVsSprite[id].resize( 2 );

            const char * text = fheroes2::getSupportedText( gettext_noop( "RESET" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                  isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_START_GOOD: {
            _icnVsSprite[id].resize( 2 );

            const char * text = fheroes2::getSupportedText( gettext_noop( "START" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, ICN::EMPTY_GOOD_BUTTON, ICN::STONEBAK );

            break;
        }
        case ICN::BUTTON_WELL_MAX: {
            _icnVsSprite[id].resize( 2 );

            const char * text = fheroes2::getSupportedText( gettext_noop( "MAX" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, ICN::EMPTY_GUILDWELL_BUTTON, ICN::UNKNOWN );

            break;
        }
        case ICN::BUTTON_VERTICAL_PATROL: {
            _icnVsSprite[id].resize( 2 );

            // We need to temporarily remove the letter specific X offsets in the font because if not the letters will
            // be off-centered when we are displaying one letter per line
            const ButtonFontOffsetRestorer fontReleased( _icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], -1 );
            const ButtonFontOffsetRestorer fontPressed( _icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED], -1 );

            const char * text = fheroes2::getSupportedText( gettext_noop( "P\nA\nT\nR\nO\nL" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, ICN::EMPTY_VERTICAL_GOOD_BUTTON, ICN::REDBAK_SMALL_VERTICAL );

            break;
        }
        case ICN::DISMISS_HERO_DISABLED_BUTTON: {
            _icnVsSprite[id].resize( 1 );

            const int buttonIcnId = ICN::BUTTON_VERTICAL_DISMISS;

            const fheroes2::Sprite & released = fheroes2::AGG::GetICN( buttonIcnId, 0 );
            const fheroes2::Sprite & pressed = fheroes2::AGG::GetICN( buttonIcnId, 1 );

            fheroes2::Sprite & output = _icnVsSprite[id][0];
            output = released;

            fheroes2::ApplyPalette( output, PAL::GetPalette( PAL::PaletteType::DARKENING ) );

            fheroes2::Image common = fheroes2::ExtractCommonPattern( { &released, &pressed } );
            common = fheroes2::FilterOnePixelNoise( common );
            common = fheroes2::FilterOnePixelNoise( common );
            common = fheroes2::FilterOnePixelNoise( common );
            FillTransform( common, 1, common.height() - 1, 23, 1, 1 );

            fheroes2::Blit( common, output );
            break;
        }
        case ICN::BUTTON_SELECT_GOOD:
        case ICN::BUTTON_SELECT_EVIL: {
            const bool isEvilInterface = ( id == ICN::BUTTON_SELECT_EVIL );

            _icnVsSprite[id].resize( 2 );

            const char * text = fheroes2::getSupportedText( gettext_noop( "SELECT" ), fheroes2::FontType::buttonReleasedWhite() );
            getTextAdaptedSprite( _icnVsSprite[id][0], _icnVsSprite[id][1], text, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                                  isEvilInterface ? ICN::UNIFORMBAK_EVIL : ICN::UNIFORMBAK_GOOD );
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
        case ICN::BUTTON_SMALL_DECLINE_GOOD:
        case ICN::BUTTON_SMALL_ACCEPT_GOOD: {
            _icnVsSprite[id].resize( 2 );
            const int buttonIcnIndex = id == ICN::BUTTON_SMALL_ACCEPT_GOOD ? 0 : 2;
            fheroes2::Sprite & released = _icnVsSprite[id][0];
            fheroes2::Sprite & pressed = _icnVsSprite[id][1];
            released = fheroes2::AGG::GetICN( ICN::SURRENDR, buttonIcnIndex );
            pressed = fheroes2::AGG::GetICN( ICN::SURRENDR, buttonIcnIndex + 1 );
            // Fill wrong transparent text pixels by using single-colored background.
            fillTransparentButtonText( released );
            return true;
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

            fheroes2::Sprite & released = _icnVsSprite[id][0];
            fheroes2::Sprite & pressed = _icnVsSprite[id][1];
            released = fheroes2::AGG::GetICN( originalID, originalICNIndex );
            pressed = fheroes2::AGG::GetICN( originalID, originalICNIndex + 1 );
            // Fill wrong transparent text pixels by using single-colored background.
            fillTransparentButtonText( released );
            return true;
        }
        default:
            break;
        }
        return false;
    }

    bool generateFrenchSpecificImages( const int id )
    {
        switch ( id ) {
        case ICN::BUTTONS_NEW_GAME_MENU_GOOD: {
            const bool isPoLPresent = !::AGG::getDataFromAggFile( ICN::getIcnFileName( ICN::X_TRACK1 ), false ).empty();
            if ( isPoLPresent ) {
                _icnVsSprite[id].resize( 28 );
            }
            else {
                _icnVsSprite[id].resize( 24 );
            }

            for ( size_t i = 0; i < 3; ++i ) {
                _icnVsSprite[id][i * 2] = fheroes2::AGG::GetICN( ICN::BTNNEWGM, static_cast<uint32_t>( i * 2 ) );
                _icnVsSprite[id][i * 2 + 1] = fheroes2::AGG::GetICN( ICN::BTNNEWGM, static_cast<uint32_t>( i * 2 + 1 ) );
            }
            // Add battle only button.
            for ( int32_t i = 0; i < 2; ++i ) {
                fheroes2::Sprite & out = _icnVsSprite[id][6 + i];
                out = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 6 + i );
                // Clean the button
                Fill( out, 27 - i, 21 + i, 77, 14, getButtonFillingColor( i == 0 ) );
                const int32_t secondLine = 28;
                // Add 'MODE'
                Copy( fheroes2::AGG::GetICN( ICN::BTNNEWGM, 4 + i ), 35 - i, 13, out, 40 - i, 13, 50, 15 );
                // Clean up 'MODE'
                Copy( fheroes2::AGG::GetICN( ICN::BTNEMAIN, 0 + i ), 109 - i, 18, out, 89 - i, 18, 1, 10 );
                // Add 'BA'
                Copy( fheroes2::AGG::GetICN( ICN::BTNBAUD, 2 + i ), 42 - i, 28, out, 23 - i, secondLine, 22, 15 );
                // Clean up 'BA'
                Copy( fheroes2::AGG::GetICN( ICN::BTNBAUD, 2 + i ), 42 - i, 31, out, 34 - i, secondLine, 1, 1 );
                Copy( fheroes2::AGG::GetICN( ICN::BTNBAUD, 2 + i ), 39 - i, 31, out, 44 - i, secondLine + 4, 1, 2 );
                // Add 'T'
                Copy( fheroes2::AGG::GetICN( ICN::BTNDC, 2 + i ), 89 - i, 21, out, 44 - i, secondLine, 12, 15 );
                // Clean up 'AT'
                Copy( fheroes2::AGG::GetICN( ICN::BTNDC, 2 + i ), 89 - i, 18, out, 45 - i, secondLine, 1, 1 );
                Copy( fheroes2::AGG::GetICN( ICN::BTNDC, 2 + i ), 92 - ( 5 * i ), 27 - i, out, 44 - i, secondLine + 4 + i, 1, 3 );
                // Add 'AI'.
                Copy( fheroes2::AGG::GetICN( ICN::BTNMP, 6 + i ), 51 - i, 13, out, 57 - i, secondLine, 18, 15 );
                // Clean up 'TA'
                Copy( fheroes2::AGG::GetICN( ICN::BTNBAUD, 2 + i ), 51 - i, 40, out, 55 - i, secondLine + 12, 3, 3 );
                // Add 'LLE'
                Copy( fheroes2::AGG::GetICN( ICN::BTNEMAIN, 0 + i ), 80 - i, 13, out, 76 - i, secondLine, 31, 15 );
                // Clean up "IL"
                Copy( fheroes2::AGG::GetICN( ICN::BTNEMAIN, 0 + i ), 80 - i, 18, out, 76 - i, secondLine + 7, 1, 1 );
                Copy( fheroes2::AGG::GetICN( ICN::BTNEMAIN, 0 + i ), 89 - i, 17, out, 75 - i, secondLine + 4, 2, 2 );
                Copy( fheroes2::AGG::GetICN( ICN::BTNEMAIN, 0 + i ), 88 - i, 25, out, 74 - i, secondLine + 12, 3, 3 );
                Copy( fheroes2::AGG::GetICN( ICN::BTNDC, 4 + i ), 23 - i, 8, out, 74 - i, secondLine + 5, 1, 10 );
                Copy( fheroes2::AGG::GetICN( ICN::BTNMP, 6 + i ), 68 - i, 16, out, 74 - i, secondLine + 9, 1, 1 );
            }
            // Add config button.
            _icnVsSprite[id][8] = fheroes2::AGG::GetICN( ICN::BTNDCCFG, 4 );
            _icnVsSprite[id][9] = fheroes2::AGG::GetICN( ICN::BTNDCCFG, 5 );
            // Add cancel button.
            _icnVsSprite[id][10] = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 6 );
            _icnVsSprite[id][11] = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 7 );
            // Add hot seat button.
            _icnVsSprite[id][12] = fheroes2::AGG::GetICN( ICN::BTNMP, 0 );
            _icnVsSprite[id][13] = fheroes2::AGG::GetICN( ICN::BTNMP, 1 );
            // Add player count buttons.
            for ( size_t i = 0; i < 5; ++i ) {
                _icnVsSprite[id][( i + 7 ) * 2] = fheroes2::AGG::GetICN( ICN::BTNHOTST, static_cast<uint32_t>( i * 2 ) );
                _icnVsSprite[id][( i + 7 ) * 2 + 1] = fheroes2::AGG::GetICN( ICN::BTNHOTST, static_cast<uint32_t>( i * 2 + 1 ) );
            }
            if ( isPoLPresent ) {
                _icnVsSprite[id][24] = fheroes2::AGG::GetICN( ICN::X_LOADCM, 0 );
                _icnVsSprite[id][25] = fheroes2::AGG::GetICN( ICN::X_LOADCM, 1 );
                _icnVsSprite[id][26] = fheroes2::AGG::GetICN( ICN::X_LOADCM, 2 );
                _icnVsSprite[id][27] = fheroes2::AGG::GetICN( ICN::X_LOADCM, 3 );
            }
            return true;
        }
        case ICN::BUTTON_GIFT_GOOD: {
            _icnVsSprite[id].resize( 2 );
            for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                fheroes2::Sprite & out = _icnVsSprite[id][i];
                const fheroes2::Sprite & original = fheroes2::AGG::GetICN( ICN::TRADPOST, 17 + i );
                // We use this Copy function to make the pressed state not single layered like the original.
                Copy( original, out );
                // Clean the button
                Fill( out, 33, 5, 31, 16, getButtonFillingColor( i == 0 ) );
                const int32_t offsetY = 5;
                // Add 'D'
                const int32_t offsetXD = 14;
                Copy( fheroes2::AGG::GetICN( ICN::CPANEL, 4 + i ), 48 - i, 28 + i, out, offsetXD - i, offsetY + i, 10, 15 );
                // Clean up 'D' and restore button ornament
                Copy( fheroes2::AGG::GetICN( ICN::CPANEL, 4 + i ), 48 - i, 36, out, offsetXD - 1 - i, offsetY + 4 + i, 1, 1 );
                Copy( fheroes2::AGG::GetICN( ICN::CPANEL, 4 + i ), 48 - i, 35, out, offsetXD - i, offsetY + 9 + i, 1, 2 );
                Copy( fheroes2::AGG::GetICN( ICN::CPANEL, 4 + i ), 48 - i, 35, out, offsetXD - 1 - i, offsetY + 13 + i, 1, 1 );
                Fill( out, offsetXD + 9 - i, offsetY + 13 + i, 1, 1, getButtonFillingColor( i == 0 ) );
                Copy( fheroes2::AGG::GetICN( ICN::TRADPOST, 17 + i ), offsetXD, offsetY, out, offsetXD, offsetY, 1, 1 );
                // Add 'O'
                const int32_t offsetXO = 10;
                Copy( fheroes2::AGG::GetICN( ICN::CAMPXTRG, i ), 40 - ( 7 * i ), 5 + i, out, offsetXD + offsetXO + 1 - i, offsetY + i, 13 - i, 15 );
                // Clean up 'DO'
                Copy( fheroes2::AGG::GetICN( ICN::CPANEL, 4 + i ), 51 - i, 34, out, offsetXD + offsetXO - i, offsetY + 5, 2, 2 );
                Copy( fheroes2::AGG::GetICN( ICN::CPANEL, 4 + i ), 51 - i, 34, out, offsetXD + offsetXO - i, offsetY + 7, 1, 1 + i );
                Copy( fheroes2::AGG::GetICN( ICN::CPANEL, 4 + i ), 55 - i, 28 + i, out, offsetXD + 9 - i, offsetY + 2 + i, 3, 3 );
                Fill( out, offsetXD + 11 - i, offsetY + i, 2, 2, getButtonFillingColor( i == 0 ) );
                // Add 'N'
                const int32_t offsetXN = 13;
                Copy( fheroes2::AGG::GetICN( ICN::TRADPOST, 17 + i ), 50 - i, 5, out, offsetXD + offsetXO + offsetXN - i, offsetY, 14, 15 );
                // Clean up 'ON'
                Fill( out, offsetXD + offsetXO + offsetXN, offsetY, 1, 1, getButtonFillingColor( i == 0 ) );
                Fill( out, offsetXD + offsetXO + offsetXN - i, offsetY + 9, 1, 1, getButtonFillingColor( i == 0 ) );
                // Add 'N'
                Copy( fheroes2::AGG::GetICN( ICN::TRADPOST, 17 + i ), 50 - i, 5, out, offsetXD + 10 + offsetXN + offsetXN - i, offsetY, 14, 15 );
                // Clean up 'NN'
                Fill( out, offsetXD + offsetXO + offsetXN + offsetXN - i, offsetY + 9, 1, 1, getButtonFillingColor( i == 0 ) );
                // Add 'ER'
                Copy( fheroes2::AGG::GetICN( ICN::CAMPXTRG, 2 + i ), 75 - ( 8 * i ), 5, out, offsetXD + offsetXO + offsetXN + offsetXN + offsetXN - ( 2 * i ), offsetY,
                      23, 15 );
                // Restore button ornament
                Copy( fheroes2::AGG::GetICN( ICN::TRADPOST, 17 + i ), offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 20, offsetY, out,
                      offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 20, offsetY, 1, 1 );
                Copy( fheroes2::AGG::GetICN( ICN::TRADPOST, 17 + i ), offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 21, offsetY + 1, out,
                      offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 21, offsetY + 1, 2, 3 );
                Copy( fheroes2::AGG::GetICN( ICN::TRADPOST, 17 + i ), offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 20, offsetY, out,
                      offsetXD + offsetXO + offsetXN + offsetXN + offsetXN + 21, offsetY + 4, 1, 1 );
            }
            return true;
        }
        case ICN::BUTTON_SMALL_MAX_GOOD: {
            _icnVsSprite[id].resize( 2 );
            fheroes2::Sprite & released = _icnVsSprite[id][0];
            fheroes2::Sprite & pressed = _icnVsSprite[id][1];
            const fheroes2::Sprite & originalReleased = fheroes2::AGG::GetICN( ICN::RECRUIT, 4 );
            const fheroes2::Sprite & originalPressed = fheroes2::AGG::GetICN( ICN::RECRUIT, 5 );
            released = originalReleased;
            pressed = originalPressed;
            // The original assets ICN contains button with shadow. We crop only the button.
            released = Crop( originalReleased, 5, 0, 60, 25 );
            pressed = Crop( originalPressed, 5, 0, 60, 25 );
            released.setPosition( 0, 0 );
            pressed.setPosition( 0, 0 );
            // Fill wrong transparent text pixels by using single-colored background.
            fillTransparentButtonText( released );
            // To properly generate shadows and Blit the button we need to make corner pixels transparent.
            setButtonCornersTransparent( pressed );

            fheroes2::Image common = fheroes2::ExtractCommonPattern( { &released, &pressed } );
            common = FilterOnePixelNoise( common );
            common = FilterOnePixelNoise( common );
            common = FilterOnePixelNoise( common );
            Blit( common, _icnVsSprite[id][0] );

            return true;
        }
        case ICN::BUTTON_SMALL_MIN_GOOD: {
            _icnVsSprite[id].resize( 2 );
            for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                fheroes2::Sprite & out = _icnVsSprite[id][i];
                out = fheroes2::AGG::GetICN( ICN::BUTTON_SMALL_MAX_GOOD, i );
                // Clean the button and leave 'M'
                Fill( out, 26 - 2 * i, 5 + i, 25, 15, getButtonFillingColor( i == 0 ) );
                Fill( out, 24 - 2 * i, 17 + i, 2, 2, getButtonFillingColor( i == 0 ) );
                // Add 'I'
                Copy( fheroes2::AGG::GetICN( ICN::APANEL, 4 + i ), 25 - i, 19 + i, out, 27 - i, 4 + i, 7 - i, 15 );
                Copy( fheroes2::AGG::GetICN( ICN::RECRUIT, 4 + i ), 28 - i, 7 + i, out, 31 - i, 7 + i, 3, 9 );
                Fill( out, 32 - i, 16 + i, 2, 3, getButtonFillingColor( i == 0 ) );
                // Add 'N'
                Copy( fheroes2::AGG::GetICN( ICN::TRADPOST, 17 + i ), 50 - i, 5, out, 36 - i, 5, 14, 15 );
                Fill( out, 36 - i, 5, 1, 1, getButtonFillingColor( i == 0 ) );
                Fill( out, 36 - i, 5 + 9, 1, 1, getButtonFillingColor( i == 0 ) );
            }
            return true;
        }
        case ICN::BUTTON_SMALL_DECLINE_GOOD:
        case ICN::BUTTON_SMALL_ACCEPT_GOOD: {
            _icnVsSprite[id].resize( 2 );
            const int buttonIcnIndex = id == ICN::BUTTON_SMALL_ACCEPT_GOOD ? 0 : 2;
            fheroes2::Sprite & released = _icnVsSprite[id][0];
            fheroes2::Sprite & pressed = _icnVsSprite[id][1];
            released = fheroes2::AGG::GetICN( ICN::SURRENDR, buttonIcnIndex );
            pressed = fheroes2::AGG::GetICN( ICN::SURRENDR, buttonIcnIndex + 1 );
            // Fill wrong transparent text pixels by using single-colored background.
            fillTransparentButtonText( released );
            // Fix corrupted pixels.
            fheroes2::ReplaceColorId( released, 109, 39 );
            fheroes2::ReplaceColorId( pressed, 178, 39 );
            return true;
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

            fheroes2::Sprite & released = _icnVsSprite[id][0];
            fheroes2::Sprite & pressed = _icnVsSprite[id][1];
            released = fheroes2::AGG::GetICN( originalID, originalICNIndex );
            pressed = fheroes2::AGG::GetICN( originalID, originalICNIndex + 1 );
            // Fill wrong transparent text pixels by using single-colored background.
            fillTransparentButtonText( released );
            return true;
        }
        case ICN::BUTTON_SMALL_OKAY_EVIL:
        case ICN::BUTTON_SMALL_CANCEL_EVIL: {
            // Doing a palette swap fixes black pixels in the lower right corner of the original button backgrounds.
            _icnVsSprite[id].resize( 2 );
            const int goodButtonIcnID = id == ICN::BUTTON_SMALL_OKAY_EVIL ? ICN::BUTTON_SMALL_OKAY_GOOD : ICN::BUTTON_SMALL_CANCEL_GOOD;
            convertToEvilButtonBackground( _icnVsSprite[id][0], _icnVsSprite[id][1], goodButtonIcnID, 0 );
            return true;
        }
        default:
            break;
        }
        return false;
    }

    bool generatePolishSpecificImages( const int id )
    {
        switch ( id ) {
        case ICN::BUTTONS_NEW_GAME_MENU_GOOD: {
            const bool isPoLPresent = !::AGG::getDataFromAggFile( ICN::getIcnFileName( ICN::X_TRACK1 ), false ).empty();
            if ( isPoLPresent ) {
                _icnVsSprite[id].resize( 28 );
            }
            else {
                _icnVsSprite[id].resize( 24 );
            }

            for ( size_t i = 0; i < 3; ++i ) {
                _icnVsSprite[id][i * 2] = fheroes2::AGG::GetICN( ICN::BTNNEWGM, static_cast<uint32_t>( i * 2 ) );
                _icnVsSprite[id][i * 2 + 1] = fheroes2::AGG::GetICN( ICN::BTNNEWGM, static_cast<uint32_t>( i * 2 + 1 ) );
            }
            // Add battle only button.
            for ( int32_t i = 0; i < 2; ++i ) {
                fheroes2::Sprite & out = _icnVsSprite[id][6 + i];
                out = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 6 + i );
                // clean the button
                Fill( out, 36 - i, 23 + i, 57, 11, getButtonFillingColor( i == 0 ) );
                const int32_t offsetX = 41;
                const int32_t offsetY = 23;
                // Add 'BI'
                Copy( fheroes2::AGG::GetICN( ICN::BTNMCFG, 2 + i ), 58 - i, 29, out, offsetX - i, offsetY, 14, 11 );
                // Add 'T'
                Copy( fheroes2::AGG::GetICN( ICN::BTNNEWGM, 0 + i ), 19 - i, 29, out, offsetX + 14 - i, offsetY, 9, 11 );
                // Add 'WA'
                Copy( fheroes2::AGG::GetICN( ICN::BTNEMAIN, 0 + i ), 40 - i, 23, out, offsetX + 23 - i, offsetY, 24, 11 );
                // Add pixel to 'W'
                Copy( fheroes2::AGG::GetICN( ICN::BTNEMAIN, 0 + i ), 42 - i, 23 + i, out, offsetX + 38 - i, offsetY + i, 1, 1 );
            }
            // Add config button.
            _icnVsSprite[id][8] = fheroes2::AGG::GetICN( ICN::BTNDCCFG, 4 );
            _icnVsSprite[id][9] = fheroes2::AGG::GetICN( ICN::BTNDCCFG, 5 );
            // Add cancel button.
            _icnVsSprite[id][10] = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 6 );
            _icnVsSprite[id][11] = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 7 );
            // Add hot seat button.
            _icnVsSprite[id][12] = fheroes2::AGG::GetICN( ICN::BTNMP, 0 );
            _icnVsSprite[id][13] = fheroes2::AGG::GetICN( ICN::BTNMP, 1 );
            // Add player count buttons.
            for ( size_t i = 0; i < 5; ++i ) {
                _icnVsSprite[id][( i + 7 ) * 2] = fheroes2::AGG::GetICN( ICN::BTNHOTST, static_cast<uint32_t>( i * 2 ) );
                _icnVsSprite[id][( i + 7 ) * 2 + 1] = fheroes2::AGG::GetICN( ICN::BTNHOTST, static_cast<uint32_t>( i * 2 + 1 ) );
            }
            if ( isPoLPresent ) {
                _icnVsSprite[id][24] = fheroes2::AGG::GetICN( ICN::X_LOADCM, 0 );
                _icnVsSprite[id][25] = fheroes2::AGG::GetICN( ICN::X_LOADCM, 1 );
                _icnVsSprite[id][26] = fheroes2::AGG::GetICN( ICN::X_LOADCM, 2 );
                _icnVsSprite[id][27] = fheroes2::AGG::GetICN( ICN::X_LOADCM, 3 );
            }
            return true;
        }
        case ICN::BUTTON_SMALL_MIN_GOOD: {
            _icnVsSprite[id].resize( 2 );
            for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                fheroes2::Sprite & out = _icnVsSprite[id][i];
                const fheroes2::Sprite & original = fheroes2::AGG::GetICN( ICN::BUTTON_SMALL_MAX_GOOD, i );
                out = original;
                // Wipe the button
                Fill( out, 9 - 2 * i, 7 + i, 46 + i, 10, getButtonFillingColor( i == 0 ) );
                // 'M'
                Copy( original, 9 - i, 7 + i, out, 13 - i, 7 + i, 14, 10 );
                // 'I'
                Copy( fheroes2::AGG::GetICN( ICN::BTNMCFG, 4 + i ), 53 - i, 23 + i, out, 28 - i, 7 + i, 5, 10 );
                // 'N'
                Copy( fheroes2::AGG::GetICN( ICN::BTNMCFG, 0 + i ), 83 - i, 29 + i, out, 34 - i, 7 + i, 11, 10 );
                // '.'
                Copy( original, 52 - i, 14 + i, out, 47 - i, 14 + i, 3, 3 );
                fheroes2::Fill( out, 44 - i, 8 + i, 1, 9, getButtonFillingColor( i == 0 ) );
                if ( i == 0 ) {
                    fheroes2::ReplaceColorId( out, 56, 55 );
                }
                if ( id == ICN::BUTTON_SMALL_MIN_EVIL ) {
                    ApplyPalette( _icnVsSprite[id][0], PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_BUTTON ) );
                    ApplyPalette( _icnVsSprite[id][1], PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_BUTTON ) );
                }
            }
            return true;
        }
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
            default:
                break;
            }
        }
        // Image generator of a last resort, must provide the generation of the "default" variant
        // for all image ids for which this function can be called, and must not fail.
        generateDefaultImages( id );
    }

    void generateGradientFont( const int fontId, const int originalFontId, const uint8_t gradientInnerColor, const uint8_t gradientOuterColor,
                               const uint8_t contourInnerColor, const uint8_t contourOuterColor )
    {
        assert( fontId != originalFontId );

        fheroes2::AGG::GetICN( originalFontId, 0 );
        const std::vector<fheroes2::Sprite> & original = _icnVsSprite[originalFontId];

        _icnVsSprite[fontId].resize( original.size() );

        for ( size_t i = 0; i < original.size(); ++i ) {
            const fheroes2::Sprite & in = original[i];
            fheroes2::Sprite & out = _icnVsSprite[fontId][i];
            out.resize( in.width() + 6, in.height() + 6 );
            out.reset();
            Copy( in, 0, 0, out, 3, 3, in.width(), in.height() );
            out.setPosition( in.x() - 2, in.y() - 2 );

            applyFontVerticalGradient( out, gradientInnerColor, gradientOuterColor );

            Blit( CreateContour( out, contourInnerColor ), out );
            Blit( CreateContour( out, 0 ), out );
            Blit( CreateContour( out, contourOuterColor ), out );
        }
    }

    //  This function modifies (fixes) the original ICNs and generate new fheroes2-related ICNs.
    // WARNING: This function must be called only once from `loadICN()` function!
    void processICN( const int id )
    {
        // If this assertion blows up then you are calling this function in a recursion. Check your code!
        assert( id < ICN::LAST_VALID_FILE_ICN || _icnVsSprite[id].empty() );

        switch ( id ) {
        case ICN::ROUTERED:
            CopyICNWithPalette( id, ICN::ROUTE, PAL::PaletteType::RED );
            break;
        case ICN::FONT:
        case ICN::SMALFONT: {
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

            const std::vector<uint8_t> & body = ::AGG::getDataFromAggFile( ICN::getIcnFileName( id ), false );
            const uint32_t crc32 = fheroes2::calculateCRC32( body.data(), body.size() );

            if ( id == ICN::SMALFONT ) {
                // Small font in official Polish GoG version has all letters shifted 1 pixel down.
                if ( crc32 == 0xE9EC7A63 ) {
                    for ( fheroes2::Sprite & letter : imageArray ) {
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

                // The PoL assets contain a dot ('.') sprite instead of asterisk ('*') while the SW assets contain correct asterisk sprite.
                // We replace PoL sprite with the correct one from SW assets.
                replacePOLAssetWithSW( id, 42 - 32 );

                modifyBaseNormalFont( _icnVsSprite[id] );
            }

            // Some checks that we really have CP1251 font
            const int32_t verifiedFontWidth = ( id == ICN::FONT ) ? 19 : 12;
            if ( imageArray.size() == 162 && imageArray[121].width() == verifiedFontWidth ) {
                // Engine expects that letter indexes correspond to charcode - 0x20.
                // In case CP1251 font.icn contains sprites for chars 0x20-0x7F, 0xC0-0xDF, 0xA8, 0xE0-0xFF, 0xB8 (in that order).
                // We rearrange sprites array for corresponding sprite indexes to charcode - 0x20.
                const fheroes2::Sprite firstSprite{ imageArray[0] };
                imageArray.insert( imageArray.begin() + 96, 64, firstSprite );
                std::swap( imageArray[136], imageArray[192] ); // Move sprites for chars 0xA8
                std::swap( imageArray[152], imageArray[225] ); // and 0xB8 to it's places.
                imageArray.pop_back();
                imageArray.erase( imageArray.begin() + 192 );
            }
            // German version uses CP1252
            if ( crc32 == 0x04745D1D || crc32 == 0xD0F0D852 ) {
                const fheroes2::Sprite firstSprite{ imageArray[0] };
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
            // Italian version uses CP1252
            if ( crc32 == 0x219B3124 || crc32 == 0x1F3C3C74 ) {
                const fheroes2::Sprite firstSprite{ imageArray[0] };
                imageArray.insert( imageArray.begin() + 101, 123, firstSprite );
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
                imageArray.erase( imageArray.begin() + 218, imageArray.end() );
            }
            break;
        }
        case ICN::YELLOW_FONT:
            CopyICNWithPalette( id, ICN::FONT, PAL::PaletteType::YELLOW_FONT );
            break;
        case ICN::YELLOW_SMALLFONT:
            CopyICNWithPalette( id, ICN::SMALFONT, PAL::PaletteType::YELLOW_FONT );
            break;
        case ICN::GRAY_FONT:
            CopyICNWithPalette( id, ICN::FONT, PAL::PaletteType::GRAY_FONT );
            break;
        case ICN::GRAY_SMALL_FONT:
            CopyICNWithPalette( id, ICN::SMALFONT, PAL::PaletteType::GRAY_FONT );
            break;
        case ICN::GOLDEN_GRADIENT_FONT:
            generateGradientFont( id, ICN::FONT, 108, 133, 55, 62 );
            break;
        case ICN::GOLDEN_GRADIENT_LARGE_FONT:
            generateGradientFont( id, ICN::WHITE_LARGE_FONT, 108, 127, 55, 62 );
            break;
        case ICN::SILVER_GRADIENT_FONT:
            generateGradientFont( id, ICN::FONT, 10, 31, 29, 0 );
            break;
        case ICN::SILVER_GRADIENT_LARGE_FONT:
            generateGradientFont( id, ICN::WHITE_LARGE_FONT, 10, 26, 29, 0 );
            break;
        case ICN::SPELLS:
            if ( _icnVsSprite[id].size() != 60 ) {
                break;
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

                const fheroes2::Sprite & originalImage = _icnVsSprite[id][originalIndex];
                fheroes2::Sprite & image = _icnVsSprite[id][i];

                image.resize( originalImage.width() + 8, originalImage.height() + 8 );
                image.setPosition( originalImage.x() + 4, originalImage.y() + 4 );
                image.fill( 1 );

                AlphaBlit( originalImage, image, 0, 0, 128 );
                AlphaBlit( originalImage, image, 4, 4, 192 );
                Blit( originalImage, image, 8, 8 );

                addTransparency( image, 1 );
            }

            // The Petrification spell does not have its own icon in the original game.
            fheroes2::h2d::readImage( "petrification_spell_icon.image", _icnVsSprite[id][66] );

            // Generate random spell image for Editor.
            {
                const fheroes2::Sprite & randomSpellImage = _icnVsSprite[id][2];
                const int32_t imageWidth = randomSpellImage.width();

                Copy( randomSpellImage, _icnVsSprite[id][67] );

                // Add text on random spell images.
                for ( uint32_t i = 1; i < 6; ++i ) {
                    fheroes2::Sprite & originalImage = _icnVsSprite[id][i + 67];
                    Copy( randomSpellImage, originalImage );

                    const fheroes2::Text text( std::to_string( i ), fheroes2::FontType::normalWhite() );
                    text.draw( ( imageWidth - text.width() ) / 2, 22, originalImage );
                }
            }
            break;
        case ICN::CSLMARKER:
            _icnVsSprite[id].resize( 3 );
            for ( uint32_t i = 0; i < 3; ++i ) {
                _icnVsSprite[id][i] = fheroes2::AGG::GetICN( ICN::LOCATORS, 24 );
                if ( i == 1 ) {
                    ReplaceColorId( _icnVsSprite[id][i], 0x0A, 0xD6 );
                }
                else if ( i == 2 ) {
                    ReplaceColorId( _icnVsSprite[id][i], 0x0A, 0xDE );
                }
            }
            break;
        case ICN::PHOENIX:
            // First sprite has cropped shadow. We copy missing part from another 'almost' identical frame
            if ( _icnVsSprite[id].size() >= 32 ) {
                const fheroes2::Sprite & in = _icnVsSprite[id][32];
                Copy( in, 60, 73, _icnVsSprite[id][1], 60, 73, 14, 13 );
                Copy( in, 56, 72, _icnVsSprite[id][30], 56, 72, 18, 9 );
            }
            break;
        case ICN::MONH0028: // phoenix
            if ( _icnVsSprite[id].size() == 1 ) {
                const fheroes2::Sprite & correctFrame = fheroes2::AGG::GetICN( ICN::PHOENIX, 32 );
                Copy( correctFrame, 60, 73, _icnVsSprite[id][0], 58, 70, 14, 13 );
            }
            break;
        case ICN::CAVALRYR:
            // fheroes2::Sprite 23 has incorrect colors, we need to replace them
            if ( _icnVsSprite[id].size() >= 23 ) {
                fheroes2::Sprite & out = _icnVsSprite[id][23];

                std::vector<uint8_t> indexes( 256 );
                std::iota( indexes.begin(), indexes.end(), static_cast<uint8_t>( 0 ) );

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
            break;
        case ICN::TITANMSL:
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
            break;
        case ICN::TROLLMSL:
            if ( _icnVsSprite[id].size() == 1 ) {
                fheroes2::Sprite & out = _icnVsSprite[id][0];
                // The original sprite contains 2 pixels which are empty
                if ( out.width() * out.height() > 188 && out.transform()[147] == 1 && out.transform()[188] == 1 ) {
                    out.transform()[147] = 0;
                    out.image()[147] = 22;

                    out.transform()[188] = 0;
                    out.image()[188] = 24;
                }
            }
            break;
        case ICN::TROLL2MSL:
            loadICN( ICN::TROLLMSL );
            if ( _icnVsSprite[ICN::TROLLMSL].size() == 1 ) {
                _icnVsSprite[id].resize( 1 );

                fheroes2::Sprite & out = _icnVsSprite[id][0];
                out = _icnVsSprite[ICN::TROLLMSL][0];

                // The original sprite contains 2 pixels which are empty
                if ( out.width() * out.height() > 188 && out.transform()[147] == 1 && out.transform()[188] == 1 ) {
                    out.transform()[147] = 0;
                    out.image()[147] = 22;

                    out.transform()[188] = 0;
                    out.image()[188] = 24;
                }

                std::vector<uint8_t> indexes( 256 );
                std::iota( indexes.begin(), indexes.end(), static_cast<uint8_t>( 0 ) );

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
            break;
        case ICN::BTNDCCFG:
        case ICN::BTNEMAIN:
        case ICN::BTNESIZE:
        case ICN::BTNHOTST:
        case ICN::BTNMP:
        case ICN::BTNNEWGM:
        case ICN::X_LOADCM: {
            // Remove embedded shadows and backgrounds because we generate our own. We can safely divide by two because every button has 2 states.
            for ( size_t i = 0; i < _icnVsSprite[id].size() / 2; ++i ) {
                fheroes2::Sprite & released = _icnVsSprite[id][i * 2];
                fheroes2::Sprite & pressed = _icnVsSprite[id][i * 2 + 1];

                fheroes2::Sprite tempReleased;
                fheroes2::Sprite tempPressed;

                const int32_t removedShadowOffsetWidth = 5;
                const int32_t removedShadowOffsetHeight = 6;

                tempReleased.resize( released.width() - removedShadowOffsetWidth, released.height() - removedShadowOffsetHeight );
                tempPressed.resize( pressed.width() - removedShadowOffsetWidth, pressed.height() - removedShadowOffsetHeight );
                tempReleased.reset();
                tempPressed.reset();

                fheroes2::Copy( released, removedShadowOffsetWidth, 0, tempReleased, 0, 0, released.width() - removedShadowOffsetWidth,
                                released.height() - removedShadowOffsetHeight );
                fheroes2::Copy( pressed, removedShadowOffsetWidth, 0, tempPressed, 0, 0, pressed.width() - removedShadowOffsetWidth,
                                pressed.height() - removedShadowOffsetHeight );
                released = tempReleased;
                pressed = tempPressed;

                setButtonCornersTransparent( released );
            }
            if ( id == ICN::BTNNEWGM ) {
                // Fix the disabled state for campaign button.
                fheroes2::Image common = fheroes2::ExtractCommonPattern( { &_icnVsSprite[id][2], &_icnVsSprite[id][3] } );
                common = fheroes2::FilterOnePixelNoise( common );
                common = fheroes2::FilterOnePixelNoise( common );
                common = fheroes2::FilterOnePixelNoise( common );
                fheroes2::Blit( common, _icnVsSprite[id][2] );
            }

            break;
        }
        case ICN::GOLEM:
        case ICN::GOLEM2:
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
                    // We have 7 'MOVE_MAIN' frames and 1/4 of cell to expand the horizontal movement, so we shift the first copied frame by
                    // "6 * Battle::Cell::widthPx / ( 4 * 7 )" to the left and reduce this shift every next frame by "Battle::Cell::widthPx / ( 7 * 4 )".
                    _icnVsSprite[id][frameID].setPosition( _icnVsSprite[id][frameID].x() - ( copyFramesNum - i ) * Battle::Cell::widthPx / 28,
                                                           _icnVsSprite[id][frameID].y() );
                }
            }
            break;
        case ICN::LOCATORE:
        case ICN::LOCATORS:
            if ( _icnVsSprite[id].size() > 15 ) {
                if ( _icnVsSprite[id][12].width() == 47 ) {
                    fheroes2::Sprite & out = _icnVsSprite[id][12];
                    out = Crop( out, 0, 0, out.width() - 1, out.height() );
                }
                if ( _icnVsSprite[id][15].width() == 47 ) {
                    fheroes2::Sprite & out = _icnVsSprite[id][15];
                    out = Crop( out, 0, 0, out.width() - 1, out.height() );
                }
            }

            if ( _icnVsSprite[id].size() == 25 ) {
                // Add random town and castle icons for Editor.
                // A temporary solution: blurred Wizard castle/town in purple palette.
                _icnVsSprite[id].resize( 27 );
                _icnVsSprite[id][25] = CreateHolyShoutEffect( _icnVsSprite[id][13], 1, 0 );
                _icnVsSprite[id][26] = CreateHolyShoutEffect( _icnVsSprite[id][19], 1, 0 );
                ApplyPalette( _icnVsSprite[id][25], PAL::GetPalette( PAL::PaletteType::PURPLE ) );
                ApplyPalette( _icnVsSprite[id][26], PAL::GetPalette( PAL::PaletteType::PURPLE ) );

                // Add the '?' mark above the image.
                const fheroes2::Text text( "? ? ?", fheroes2::FontType::normalWhite() );
                text.draw( ( _icnVsSprite[id][25].width() - text.width() ) / 2, 6, _icnVsSprite[id][25] );
                text.draw( ( _icnVsSprite[id][26].width() - text.width() ) / 2, 6, _icnVsSprite[id][26] );
            }
            break;
        case ICN::TOWNBKG2:
            if ( _icnVsSprite[id].size() == 1 ) {
                fheroes2::Sprite fix;
                fheroes2::h2d::readImage( "townbkg2_fix.image", fix );

                fheroes2::Sprite & out = _icnVsSprite[id][0];
                // The original sprite has incorrect transparent pixel. Make the image single-layer before applying the fix.
                out._disableTransformLayer();
                Blit( fix, 0, 0, out, fix.x(), fix.y(), fix.width(), fix.height() );
            }
            break;
        case ICN::TWNSSPEC:
            if ( _icnVsSprite[id].size() == 1 ) {
                fheroes2::Sprite fix;
                fheroes2::h2d::readImage( "twnsspec_fix.image", fix );

                fheroes2::Sprite & out = _icnVsSprite[id][0];
                Blit( fix, 0, 0, out, fix.x(), fix.y(), fix.width(), fix.height() );
            }
            break;
        case ICN::HSICONS:
            if ( _icnVsSprite[id].size() > 7 ) {
                fheroes2::Sprite & out = _icnVsSprite[id][7];
                if ( out.width() == 34 && out.height() == 19 ) {
                    fheroes2::Sprite temp;
                    std::swap( temp, out );

                    out.resize( temp.width() + 1, temp.height() );
                    out.reset();
                    Copy( temp, 0, 0, out, 1, 0, temp.width(), temp.height() );
                    Copy( temp, temp.width() - 1, 10, out, 0, 10, 1, 3 );
                }
            }
            break;
        case ICN::EVIL_DIALOG_PLAIN_CORNERS: {
            _icnVsSprite[id].resize( 1 );

            fheroes2::Sprite & cornerSprite = _icnVsSprite[id][0];
            const int32_t cornerSideLength = 43;
            cornerSprite.resize( cornerSideLength * 2, cornerSideLength * 2 );
            const fheroes2::Sprite & originalGoodDialog = fheroes2::AGG::GetICN( ICN::WINLOSE, 0 );
            Copy( originalGoodDialog, 0, 0, cornerSprite, 0, 0, cornerSideLength, cornerSideLength );
            Copy( originalGoodDialog, originalGoodDialog.width() - cornerSideLength, 0, cornerSprite, cornerSideLength, 0, cornerSideLength, cornerSideLength );
            Copy( originalGoodDialog, 0, originalGoodDialog.height() - cornerSideLength, cornerSprite, 0, cornerSideLength, cornerSideLength, cornerSideLength );
            Copy( originalGoodDialog, originalGoodDialog.width() - cornerSideLength, originalGoodDialog.height() - cornerSideLength, cornerSprite, cornerSideLength,
                  cornerSideLength, cornerSideLength, cornerSideLength );
            fheroes2::ApplyPalette( cornerSprite, PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );

            break;
        }
        case ICN::MONS32:
            if ( _icnVsSprite[id].size() > 4 ) { // Veteran Pikeman
                fheroes2::Sprite & modified = _icnVsSprite[id][4];

                fheroes2::Sprite temp( modified.width(), modified.height() + 1 );
                temp.reset();
                Blit( modified, 0, 0, temp, 0, 1, modified.width(), modified.height() );
                modified = std::move( temp );
                Fill( modified, 7, 0, 4, 1, 36 );
            }
            if ( _icnVsSprite[id].size() > 6 ) { // Master Swordsman
                fheroes2::Sprite & modified = _icnVsSprite[id][6];

                fheroes2::Sprite temp( modified.width(), modified.height() + 1 );
                temp.reset();
                Blit( modified, 0, 0, temp, 0, 1, modified.width(), modified.height() );
                modified = std::move( temp );
                Fill( modified, 2, 0, 5, 1, 36 );
            }
            if ( _icnVsSprite[id].size() > 8 ) { // Champion
                fheroes2::Sprite & modified = _icnVsSprite[id][8];

                fheroes2::Sprite temp( modified.width(), modified.height() + 1 );
                temp.reset();
                Blit( modified, 0, 0, temp, 0, 1, modified.width(), modified.height() );
                modified = std::move( temp );
                Fill( modified, 12, 0, 5, 1, 36 );
            }
            if ( _icnVsSprite[id].size() > 33 ) {
                // Minotaur King original mini sprite has blue armlets. We make them gold to correspond the ICN::MINOTAU2.
                fheroes2::Sprite & modified = _icnVsSprite[id][33];

                if ( modified.width() == 20 && modified.height() == 36 ) {
                    // We update these pixels: 6x16, 7x16, 8x16, 5x17, 6x17, 7x17, 8x17, 6x18, 7x18, 14x18, 14x19.
                    for ( const uint32_t pixelNumber : { 326, 327, 328, 345, 346, 347, 348, 366, 367, 374, 394 } ) {
                        // The gold color gradient has -42 offset from blue color gradient.
                        modified.image()[pixelNumber] -= 42;
                    }
                }
            }
            if ( _icnVsSprite[id].size() > 62 ) {
                for ( size_t i = 0; i < 62; ++i ) {
                    fheroes2::Sprite & modified = _icnVsSprite[id][i];
                    const fheroes2::Point originalOffset( modified.x(), modified.y() );
                    fheroes2::Sprite temp = addShadow( modified, { -1, 2 }, 2 );
                    temp.setPosition( originalOffset.x - 1, originalOffset.y + 2 );

                    const fheroes2::Rect area = GetActiveROI( temp, 2 );
                    if ( area.x > 0 || area.height != temp.height() ) {
                        const fheroes2::Point offset( temp.x() - area.x, temp.y() - temp.height() + area.y + area.height );
                        modified = Crop( temp, area.x, area.y, area.width, area.height );
                        modified.setPosition( offset.x, offset.y );
                    }
                    else {
                        modified = std::move( temp );
                    }
                }
            }
            if ( _icnVsSprite[id].size() > 63 && _icnVsSprite[id][63].width() == 19 && _icnVsSprite[id][63].height() == 37 ) { // Air Elemental
                fheroes2::Sprite & modified = _icnVsSprite[id][63];
                modified.image()[19 * 9 + 9] = modified.image()[19 * 5 + 11];
                modified.transform()[19 * 9 + 9] = modified.transform()[19 * 5 + 11];
            }

            break;
        case ICN::MONSTER_SWITCH_LEFT_ARROW:
            _icnVsSprite[id].resize( 2 );
            for ( uint32_t i = 0; i < 2; ++i ) {
                const fheroes2::Sprite & source = fheroes2::AGG::GetICN( ICN::RECRUIT, i );
                fheroes2::Sprite & out = _icnVsSprite[id][i];
                out.resize( source.height(), source.width() );
                Transpose( source, out );
                out = Flip( out, false, true );
                out.setPosition( source.y() - static_cast<int32_t>( i ), source.x() );
            }
            break;
        case ICN::MONSTER_SWITCH_RIGHT_ARROW:
            _icnVsSprite[id].resize( 2 );
            for ( uint32_t i = 0; i < 2; ++i ) {
                const fheroes2::Sprite & source = fheroes2::AGG::GetICN( ICN::RECRUIT, i + 2 );
                fheroes2::Sprite & out = _icnVsSprite[id][i];
                out.resize( source.height(), source.width() );
                Transpose( source, out );
                out = Flip( out, false, true );
                out.setPosition( source.y(), source.x() );
            }
            break;
        case ICN::SURRENDR:
            // We fix the pressed button backgrounds here because this also needs to be applied on all localized assets.
            if ( _icnVsSprite[id].size() >= 4 ) {
                for ( const uint32_t i : { 0, 2 } ) {
                    fheroes2::Sprite & out = _icnVsSprite[id][i + 1];
                    fheroes2::Sprite tmp( out.width(), out.height() );
                    tmp.reset();
                    Copy( out, 0, 1, tmp, 0, 1, tmp.width() - 1, tmp.height() - 1 );
                    out = std::move( tmp );
                    setButtonCornersTransparent( out );
                    FillTransform( out, out.width() - 3, 1, 2, 1, 1 );
                    FillTransform( out, out.width() - 2, 2, 1, 1, 1 );
                    fheroes2::makeTransparentBackground( _icnVsSprite[id][i], _icnVsSprite[id][i + 1], ICN::STONEBAK );
                }
            }
            break;
        case ICN::NON_UNIFORM_GOOD_RESTART_BUTTON:
            _icnVsSprite[id].resize( 2 );
            _icnVsSprite[id][0] = Crop( fheroes2::AGG::GetICN( ICN::CAMPXTRG, 2 ), 6, 0, 108, 25 );
            _icnVsSprite[id][0].setPosition( 0, 0 );

            _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::CAMPXTRG, 3 );
            _icnVsSprite[id][1].setPosition( 0, 0 );

            // fix transparent corners
            copyTransformLayer( _icnVsSprite[id][1], _icnVsSprite[id][0] );
            break;
        case ICN::WHITE_LARGE_FONT: {
            fheroes2::AGG::GetICN( ICN::FONT, 0 );
            const std::vector<fheroes2::Sprite> & original = _icnVsSprite[ICN::FONT];
            _icnVsSprite[id].resize( original.size() );
            for ( size_t i = 0; i < _icnVsSprite[id].size(); ++i ) {
                const fheroes2::Sprite & in = original[i];
                fheroes2::Sprite & out = _icnVsSprite[id][i];
                out.resize( in.width() * 2, in.height() * 2 );
                SubpixelResize( in, out );
                out.setPosition( in.x() * 2, in.y() * 2 );
            }
            break;
        }
        case ICN::SWAP_ARROW_LEFT_TO_RIGHT:
        case ICN::SWAP_ARROW_RIGHT_TO_LEFT: {
            // Since the original game does not have such resources we could generate it from hero meeting sprite.
            const fheroes2::Sprite & original = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );
            std::array<fheroes2::Image, 4> input;

            const int32_t width = 45;
            const int32_t height = 20;

            for ( fheroes2::Image & image : input ) {
                image.resize( width, height );
            }

            Copy( original, 295, 270, input[0], 0, 0, width, height );
            Copy( original, 295, 291, input[1], 0, 0, width, height );
            Copy( original, 295, 363, input[2], 0, 0, width, height );
            Copy( original, 295, 384, input[3], 0, 0, width, height );

            input[1] = Flip( input[1], true, false );
            input[3] = Flip( input[3], true, false );

            fheroes2::Image out = fheroes2::ExtractCommonPattern( { &input[0], &input[1], &input[2], &input[3] } );

            // Here are 2 pixels which should be removed.
            if ( out.width() == width && out.height() == height ) {
                out.image()[40] = 0;
                out.transform()[40] = 1;

                out.image()[30 + 3 * width] = 0;
                out.transform()[30 + 3 * width] = 1;
            }

            _icnVsSprite[id].resize( 2 );
            _icnVsSprite[id][0] = ( id == ICN::SWAP_ARROW_LEFT_TO_RIGHT ) ? std::move( out ) : Flip( out, true, false );

            _icnVsSprite[id][1] = _icnVsSprite[id][0];
            _icnVsSprite[id][1].setPosition( -1, 1 );
            ApplyPalette( _icnVsSprite[id][1], 4 );

            break;
        }
        case ICN::SWAP_ARROWS_CIRCULAR: {
            const fheroes2::Sprite & original = fheroes2::AGG::GetICN( ICN::SWAP_ARROW_LEFT_TO_RIGHT, 0 );

            const int32_t width = 47;
            const int32_t height = 42;
            fheroes2::Image out;
            out.resize( width, height );
            out.reset();

            // Rotate arrow heads.
            const int32_t arrowTipToShaftLength = 18;
            const int32_t arrowHeadWidth = 20;

            fheroes2::Image rotatedArrow;
            rotatedArrow.resize( arrowHeadWidth, arrowTipToShaftLength );
            rotatedArrow.reset();
            for ( int x = 0; x < arrowTipToShaftLength; ++x ) {
                for ( int y = 0; y < arrowHeadWidth; ++y ) {
                    Copy( original, x + ( original.width() - arrowTipToShaftLength ), y, rotatedArrow, arrowHeadWidth - y - 1, arrowTipToShaftLength - x - 1, 1, 1 );
                }
            }

            fheroes2::Copy( Flip( rotatedArrow, true, true ), 0, 0, out, 0, 5, arrowHeadWidth, arrowTipToShaftLength );
            fheroes2::Copy( Flip( rotatedArrow, true, false ), 0, 0, out, 27, 19, arrowHeadWidth, arrowTipToShaftLength );

            // Rotate arrow ends.
            const int32_t arrowEndHeight = 11;
            const int32_t arrowEndWidth = 9;

            rotatedArrow.resize( arrowEndWidth, arrowEndHeight );
            rotatedArrow.reset();

            for ( int x = 0; x < arrowEndHeight; ++x ) {
                for ( int y = 0; y < arrowEndWidth; ++y ) {
                    Copy( original, x + 2, y + 5, rotatedArrow, arrowEndWidth - y - 1, arrowEndHeight - x - 1, 1, 1 );
                }
            }

            // Clean black corner.
            fheroes2::Copy( original, 0, 0, rotatedArrow, 0, rotatedArrow.height() - 1, 1, 1 );

            fheroes2::Copy( rotatedArrow, 0, 0, out, 32, 6, arrowEndWidth, arrowEndHeight );
            fheroes2::Copy( Flip( rotatedArrow, false, true ), 0, 0, out, 5, 25, arrowEndWidth, arrowEndHeight );

            // Add straight shafts.
            Copy( original, 5, 5, out, 13, 0, 21, 10 );
            Copy( original, 5, 5, out, 12, 32, 22, 10 );

            // Lower arrow.
            // Fix overlaps.
            fheroes2::SetPixel( out, out.width() - 9, out.height() - 5, 119 );
            fheroes2::DrawLine( out, { out.width() - 13, out.height() - 6 }, { out.width() - 12, out.height() - 6 }, 109 );
            fheroes2::SetPixel( out, 12, out.height() - 10, 119 );
            fheroes2::SetPixel( out, out.width() - 14, out.height() - 10, 119 );

            // Lower right corner.
            Copy( original, 5, 10, out, out.width() - 13, out.height() - 5, 4, 5 );
            fheroes2::DrawLine( out, { out.width() - 6, out.height() - 4 }, { out.width() - 9, out.height() - 1 }, 59 );
            fheroes2::DrawLine( out, { out.width() - 6, out.height() - 5 }, { out.width() - 9, out.height() - 2 }, 59 );
            fheroes2::DrawLine( out, { out.width() - 7, out.height() - 5 }, { out.width() - 9, out.height() - 3 }, 129 );
            fheroes2::DrawLine( out, { out.width() - 8, out.height() - 5 }, { out.width() - 9, out.height() - 4 }, 123 );
            fheroes2::SetPixel( out, out.width() - 9, out.height() - 5, 119 );
            fheroes2::SetPixel( out, out.width() - 11, out.height() - 6, 112 );

            // Lower left corner.
            Copy( original, 5, 9, out, 9, out.height() - 6, 3, 6 );
            Copy( rotatedArrow, 0, 0, out, 5, out.height() - 7, 4, 2 );
            fheroes2::DrawLine( out, { 5, out.height() - 5 }, { 8, out.height() - 2 }, 129 );
            fheroes2::DrawLine( out, { 6, out.height() - 5 }, { 8, out.height() - 3 }, 123 );
            fheroes2::DrawLine( out, { 7, out.height() - 5 }, { 8, out.height() - 4 }, 119 );
            fheroes2::SetPixel( out, 8, out.height() - 5, 116 );
            fheroes2::SetPixel( out, 9, out.height() - 6, 112 );

            // Fix shading.
            fheroes2::DrawLine( out, { 14, out.height() - 15 }, { 14, out.height() - 11 }, 59 );

            // Upper arrow.
            // Upper left corner.
            fheroes2::Copy( out, 15, 0, out, 9, 0, 4, 5 );
            fheroes2::Copy( out, 21, 5, out, 11, 5, 3, 2 );
            fheroes2::DrawLine( out, { 8, 1 }, { 5, 4 }, 129 );
            fheroes2::DrawLine( out, { 8, 2 }, { 6, 4 }, 119 );
            fheroes2::DrawLine( out, { 8, 3 }, { 7, 4 }, 114 );
            fheroes2::SetPixel( out, 8, 4, 112 );
            fheroes2::SetPixel( out, 9, 4, 112 );

            // Upper right corner.
            fheroes2::Copy( out, 21, 0, out, 33, 0, 3, 6 );
            fheroes2::Copy( out, 36, 7, out, 36, 5, 5, 1 );
            fheroes2::Copy( out, 37, 6, out, 37, 4, 3, 1 );
            fheroes2::Copy( out, 37, 6, out, 37, 4, 3, 1 );
            fheroes2::Copy( out, 34, 1, out, 36, 1, 1, 3 );
            fheroes2::DrawLine( out, { 36, 0 }, { 40, 4 }, 129 );
            fheroes2::DrawLine( out, { 37, 2 }, { 38, 3 }, 119 );
            fheroes2::DrawLine( out, { 36, 4 }, { 37, 3 }, 113 );

            // Fix overlap.
            fheroes2::DrawLine( out, { 33, 8 }, { 33, 9 }, 123 );
            fheroes2::SetPixel( out, 32, 9, 129 );
            fheroes2::SetPixel( out, 13, 9, 129 );

            // Fix shading.
            fheroes2::DrawLine( out, { 10, 22 }, { 18, 14 }, 59 );
            fheroes2::DrawLine( out, { 11, 22 }, { 19, 14 }, 59 );
            fheroes2::DrawLine( out, { 34, 17 }, { 40, 17 }, 59 );
            fheroes2::DrawLine( out, { 41, 5 }, { 41, 16 }, 59 );
            fheroes2::SetPixel( out, 40, 16, 59 );
            fheroes2::Copy( original, 0, 0, out, 1, 12, 4, 1 );
            fheroes2::Copy( original, 0, 0, out, 15, 12, 5, 1 );

            // Make pressed state.
            _icnVsSprite[id].resize( 2 );
            _icnVsSprite[id][0] = std::move( out );
            _icnVsSprite[id][1] = _icnVsSprite[id][0];
            _icnVsSprite[id][1].setPosition( -1, 1 );
            ApplyPalette( _icnVsSprite[id][1], 4 );

            break;
        }
        case ICN::EDITBTNS:
            if ( _icnVsSprite[id].size() == 35 ) {
                // We add three buttons for new object groups: Adventure, Kingdom, Monsters.
                _icnVsSprite[id].resize( 45 );

                // First make clean button sprites (pressed and released).
                fheroes2::Sprite released = fheroes2::AGG::GetICN( ICN::EDITBTNS, 4 );
                fheroes2::Sprite pressed = fheroes2::AGG::GetICN( ICN::EDITBTNS, 5 );
                // Clean the image from the button.
                Fill( released, 16, 6, 18, 24, 41U );
                Fill( pressed, 16, 7, 17, 23, 46U );

                for ( size_t i = 0; i < 8; i += 2 ) {
                    _icnVsSprite[id][35 + i] = released;
                    _icnVsSprite[id][35 + 1 + i] = pressed;
                }
                _icnVsSprite[id][43] = std::move( released );
                _icnVsSprite[id][44] = std::move( pressed );

                // Adventure objects button.
                drawImageOnButton( fheroes2::AGG::GetICN( ICN::X_LOC1, 0 ), 39, 29, _icnVsSprite[id][35], _icnVsSprite[id][36] );

                // Kingdom objects button.
                drawImageOnButton( fheroes2::AGG::GetICN( ICN::OBJNARTI, 13 ), 39, 29, _icnVsSprite[id][37], _icnVsSprite[id][38] );

                // Monsters objects button.
                drawImageOnButton( fheroes2::AGG::GetICN( ICN::MONS32, 11 ), 39, 29, _icnVsSprite[id][39], _icnVsSprite[id][40] );

                // Undo and Redo buttons.
                fheroes2::Image undoImage( 19, 13 );
                undoImage.reset();
                const int mainColor = 56;
                fheroes2::SetPixel( undoImage, 0, 6, mainColor );
                for ( int x = 1; x < 7; ++x ) {
                    fheroes2::DrawLine( undoImage, { x, 6 - x }, { x, 6 + x }, mainColor );
                }
                fheroes2::Fill( undoImage, 7, 4, 9, 5, mainColor );
                fheroes2::SetPixel( undoImage, 16, 5, mainColor );
                fheroes2::Fill( undoImage, 16, 6, 2, 4, mainColor );
                fheroes2::Fill( undoImage, 17, 8, 2, 4, mainColor );
                fheroes2::SetPixel( undoImage, 18, 12, mainColor );
                drawImageOnButton( undoImage, 39, 29, _icnVsSprite[id][41], _icnVsSprite[id][42] );
                drawImageOnButton( fheroes2::Flip( undoImage, true, false ), 39, 29, _icnVsSprite[id][43], _icnVsSprite[id][44] );
                // Fix shadow pixels
                fheroes2::SetPixel( _icnVsSprite[id][43], 27, 11, 41 );
                fheroes2::SetPixel( _icnVsSprite[id][44], 26, 12, 46 );
                fheroes2::SetPixel( _icnVsSprite[id][43], 16, 17, 47 );
                fheroes2::SetPixel( _icnVsSprite[id][44], 15, 18, 52 );
                // Add aliasing
                fheroes2::SetPixel( _icnVsSprite[id][41], 32, 19, 52 );
                fheroes2::DrawLine( _icnVsSprite[id][41], { 33, 20 }, { 33, 21 }, 52 );
                fheroes2::SetPixel( _icnVsSprite[id][42], 31, 20, 53 );
                fheroes2::DrawLine( _icnVsSprite[id][42], { 32, 21 }, { 32, 22 }, 53 );
            }
            break;
        case ICN::EDITBTNS_EVIL: {
            loadICN( ICN::EDITBTNS );
            _icnVsSprite[id] = _icnVsSprite[ICN::EDITBTNS];
            for ( auto & image : _icnVsSprite[id] ) {
                convertToEvilInterface( image, { 0, 0, image.width(), image.height() } );
            }
            break;
        }
        case ICN::DROPLISL_EVIL: {
            loadICN( ICN::DROPLISL );
            _icnVsSprite[id] = _icnVsSprite[ICN::DROPLISL];

            // To convert the yellow borders of the drop list the combination of good-to-evil and gray palettes is used here.
            const std::vector<uint8_t> palette
                = PAL::CombinePalettes( PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ), PAL::GetPalette( PAL::PaletteType::GRAY ) );
            for ( auto & image : _icnVsSprite[id] ) {
                fheroes2::ApplyPalette( image, 0, 0, image, 0, 0, image.width(), image.height(), palette );
            }
            break;
        }
        case ICN::CELLWIN_EVIL: {
            loadICN( ICN::CELLWIN );

            if ( _icnVsSprite[ICN::CELLWIN].size() > 18 ) {
                // Convert to Evil only the first 19 images. The rest are not standard buttons and are player color related settings used in original editor.
                _icnVsSprite[ICN::CELLWIN_EVIL].resize( 19 );
                std::copy( _icnVsSprite[ICN::CELLWIN].begin(), _icnVsSprite[ICN::CELLWIN].begin() + 19, _icnVsSprite[ICN::CELLWIN_EVIL].begin() );

                // To convert the yellow borders of some items the combination of good-to-evil and gray palettes is used here.
                const std::vector<uint8_t> palette
                    = PAL::CombinePalettes( PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ), PAL::GetPalette( PAL::PaletteType::GRAY ) );
                for ( fheroes2::Sprite & image : _icnVsSprite[ICN::CELLWIN_EVIL] ) {
                    fheroes2::ApplyPalette( image, 0, 0, image, 0, 0, image.width(), image.height(), palette );
                }
            }
            break;
        }
        case ICN::EDITPANL:
            if ( _icnVsSprite[id].size() == 6 ) {
                _icnVsSprite[id].resize( 18 );

                // Make empty buttons for object types.
                _icnVsSprite[id][6].resize( 27, 27 );
                _icnVsSprite[id][6]._disableTransformLayer();
                _icnVsSprite[id][6].reset();
                Fill( _icnVsSprite[id][6], 1, 1, 24, 24, 65U );
                for ( size_t i = 7; i < _icnVsSprite[id].size(); ++i ) {
                    _icnVsSprite[id][i] = _icnVsSprite[id][6];
                }

                // Make Mountains objects button.
                Blit( fheroes2::AGG::GetICN( ICN::MTNCRCK, 3 ), 4, 0, _icnVsSprite[id][6], 1, 1, 24, 24 );

                // Make Rocks objects button.
                Blit( fheroes2::AGG::GetICN( ICN::OBJNGRAS, 41 ), 1, 0, _icnVsSprite[id][7], 1, 9, 23, 12 );

                // Make Trees object button. Also used as erase terrain objects button image.
                Copy( fheroes2::AGG::GetICN( ICN::EDITBTNS, 2 ), 13, 4, _icnVsSprite[id][8], 1, 1, 24, 24 );

                // Replace image contour colors with the background color.
                std::vector<uint8_t> indexes( 256 );
                std::iota( indexes.begin(), indexes.end(), static_cast<uint8_t>( 0 ) );

                indexes[10] = 65U;
                indexes[38] = 65U;
                indexes[39] = 65U;
                indexes[40] = 65U;
                indexes[41] = 65U;
                indexes[46] = 65U;

                ApplyPalette( _icnVsSprite[id][8], indexes );

                // Make Landscape Water objects button.
                Blit( fheroes2::AGG::GetICN( ICN::OBJNWAT2, 0 ), 0, 3, _icnVsSprite[id][9], 5, 1, 13, 3 );
                Blit( fheroes2::AGG::GetICN( ICN::OBJNWAT2, 2 ), 5, 0, _icnVsSprite[id][9], 1, 4, 24, 21 );

                // Make Landscape Miscellaneous objects button.
                Blit( fheroes2::AGG::GetICN( ICN::OBJNDIRT, 73 ), 8, 0, _icnVsSprite[id][10], 1, 1, 24, 24 );

                // Make Dwellings button image.
                Blit( fheroes2::AGG::GetICN( ICN::OBJNMULT, 114 ), 7, 0, _icnVsSprite[id][11], 1, 1, 24, 24 );

                // Make Mines button image.
                Blit( fheroes2::AGG::GetICN( ICN::MTNMULT, 82 ), 8, 4, _icnVsSprite[id][12], 1, 1, 24, 24 );

                // Make Power-ups button image.
                Blit( fheroes2::AGG::GetICN( ICN::OBJNMULT, 72 ), 0, 6, _icnVsSprite[id][13], 1, 1, 24, 24 );

                // Make Adventure Water objects button.
                Blit( fheroes2::AGG::GetICN( ICN::OBJNWATR, 24 ), 3, 0, _icnVsSprite[id][14], 1, 1, 24, 24 );

                // Make Adventure Miscellaneous objects button.
                Blit( fheroes2::AGG::GetICN( ICN::OBJNMUL2, 198 ), 2, 0, _icnVsSprite[id][15], 1, 1, 24, 24 );

                // Make erase Roads button image.
                Blit( fheroes2::AGG::GetICN( ICN::ROAD, 2 ), 0, 0, _icnVsSprite[id][16], 1, 8, 24, 5 );
                Blit( fheroes2::AGG::GetICN( ICN::ROAD, 1 ), 1, 0, _icnVsSprite[id][16], 1, 13, 24, 5 );

                // Make erase Streams button image.
                Blit( fheroes2::AGG::GetICN( ICN::STREAM, 2 ), 0, 0, _icnVsSprite[id][17], 1, 8, 24, 11 );
            }
            break;
        case ICN::TEXTBAR:
            if ( _icnVsSprite[id].size() > 9 ) {
                // Remove the slightly corrupted rightmost column from the text bar background image.
                for ( size_t i = 8; i < 10; ++i )
                    if ( _icnVsSprite[id][i].width() == 543 ) {
                        _icnVsSprite[id][i] = Crop( _icnVsSprite[id][i], 0, 0, _icnVsSprite[id][i].width() - 1, _icnVsSprite[id][i].height() );
                    }
            }
            break;
        case ICN::TWNWUP_5:
        case ICN::EDITOR:
            if ( !_icnVsSprite[id].empty() ) {
                // Fix the cycling colors in original editor main menu background and Red Tower (Warlock castle screen).
                ApplyPalette( _icnVsSprite[id].front(), PAL::GetPalette( PAL::PaletteType::NO_CYCLE ) );
            }
            break;
        case ICN::MINIHERO:
            if ( _icnVsSprite[id].size() == 42 ) {
                const auto & noCyclePalette = PAL::GetPalette( PAL::PaletteType::NO_CYCLE );

                // Fix cycling colors on the Green heroes' flag for Knight, Sorceress and Warlock.
                ApplyPalette( _icnVsSprite[id][7], noCyclePalette );
                ApplyPalette( _icnVsSprite[id][9], noCyclePalette );
                ApplyPalette( _icnVsSprite[id][10], noCyclePalette );

                // Fix cycling colors on the Yellow heroes' flag.
                for ( size_t i = 21; i < 28; ++i ) {
                    ApplyPalette( _icnVsSprite[id][i], noCyclePalette );
                }

                // Fix Blue Random hero flag.
                Copy( _icnVsSprite[id][5], 1, 4, _icnVsSprite[id][6], 1, 4, 17, 7 );

                // Fix Orange Necromancer hero flag.
                Copy( _icnVsSprite[id][32], 5, 4, _icnVsSprite[id][33], 5, 4, 14, 7 );

                // Fix Orange Random hero flag (in original assets he has a purple flag).
                Copy( _icnVsSprite[id][32], 2, 4, _icnVsSprite[id][34], 2, 4, 16, 7 );

                // Fix Knight heroes missing 2 leftmost sprite columns.
                for ( size_t i = 0; i < 6; ++i ) {
                    fheroes2::Sprite & kinght = _icnVsSprite[id][i * 7];
                    const fheroes2::Sprite & barbarian = _icnVsSprite[id][i * 7 + 1];
                    const int32_t width = kinght.width();
                    const int32_t height = kinght.height();
                    fheroes2::Sprite fixed( width + 2, height );
                    Copy( kinght, 1, 0, fixed, 3, 0, width, height );
                    fixed.setPosition( kinght.x(), kinght.y() );
                    Copy( barbarian, 0, 0, fixed, 0, 0, 3, height );
                    Copy( barbarian, 3, 28, fixed, 3, 28, 1, 1 );
                    kinght = std::move( fixed );
                }
            }
            break;
        case ICN::HEROES:
            if ( !_icnVsSprite[id].empty() ) {
                fheroes2::Sprite & original = _icnVsSprite[id][0];
                // This is the main menu image which shouldn't have any transform layer.
                original._disableTransformLayer();
                if ( original.width() == 640 && original.height() == 480 ) {
                    // Fix incorrect pixel at position 260x305.
                    original.image()[195460] = 31;
                }

                // Since we cannot access game settings from here we are checking an existence
                // of one of POL resources as an indicator for this version.
                if ( !::AGG::getDataFromAggFile( ICN::getIcnFileName( ICN::X_TRACK1 ), false ).empty() ) {
                    fheroes2::Sprite editorIcon;
                    fheroes2::h2d::readImage( "main_menu_editor_icon.image", editorIcon );

                    Blit( editorIcon, 0, 0, original, editorIcon.x(), editorIcon.y(), editorIcon.width(), editorIcon.height() );
                }
            }
            break;
        case ICN::BTNSHNGL:
            if ( _icnVsSprite[id].size() == 20 ) {
                _icnVsSprite[id].resize( 23 );

                fheroes2::h2d::readImage( "main_menu_editor_released_button.image", _icnVsSprite[id][20] );
                fheroes2::h2d::readImage( "main_menu_editor_highlighted_button.image", _icnVsSprite[id][21] );
                fheroes2::h2d::readImage( "main_menu_editor_pressed_button.image", _icnVsSprite[id][22] );
            }
            break;
        case ICN::TOWNBKG3:
            // Warlock town background image contains 'empty' pixels leading to appear them as black.
            if ( !_icnVsSprite[id].empty() ) {
                fheroes2::Sprite & original = _icnVsSprite[id][0];
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
            break;
        case ICN::MINIPORT:
            // Some heroes portraits have incorrect transparent pixels.
            if ( _icnVsSprite[id].size() > 60 ) {
                fheroes2::Sprite & original = _icnVsSprite[id][60];
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
                fheroes2::Sprite & original = _icnVsSprite[id][61];
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
                fheroes2::Sprite & original = _icnVsSprite[id][65];
                if ( original.width() == 30 && original.height() == 22 ) {
                    original._disableTransformLayer();
                    uint8_t * imageData = original.image();
                    imageData[499] = 60;
                    imageData[601] = 24;
                    imageData[631] = 28;
                }
            }
            if ( _icnVsSprite[id].size() > 67 ) {
                fheroes2::Sprite & original = _icnVsSprite[id][67];
                if ( original.width() == 30 && original.height() == 22 ) {
                    original._disableTransformLayer();
                    original.image()[42] = 28;
                }
            }
            break;
        case ICN::MINICAPT:
            // Barbarian captain mini icon has bad pixel at position 22x2.
            if ( _icnVsSprite[id].size() > 1 ) {
                fheroes2::Sprite & original = _icnVsSprite[id][1];
                if ( original.width() == 30 && original.height() == 22 ) {
                    original._disableTransformLayer();
                    original.image()[82] = 244;
                }
            }
            break;
        case ICN::PORT0091:
            // Barbarian captain has one bad pixel.
            if ( !_icnVsSprite[id].empty() ) {
                fheroes2::Sprite & original = _icnVsSprite[id][0];
                if ( original.width() == 101 && original.height() == 93 ) {
                    original._disableTransformLayer();
                    original.image()[9084] = 77;
                }
            }
            break;
        case ICN::PORT0090:
            // Knight captain has multiple bad pixels.
            if ( !_icnVsSprite[id].empty() ) {
                fheroes2::Sprite & original = _icnVsSprite[id][0];
                if ( original.width() == 101 && original.height() == 93 ) {
                    original._disableTransformLayer();
                    uint8_t * imageData = original.image();
                    imageData[2314] = 70;
                    imageData[5160] = 71;
                    imageData[5827] = 18;
                    imageData[7474] = 167;
                }
            }
            break;
        case ICN::PORT0092:
            // Sorceress captain has two bad transparent pixels (8x20 and 8x66).
            if ( !_icnVsSprite[id].empty() ) {
                fheroes2::Sprite & original = _icnVsSprite[id][0];
                if ( original.width() == 101 && original.height() == 93 ) {
                    original._disableTransformLayer();
                    uint8_t * imageData = original.image();
                    imageData[2028] = 42;
                    imageData[6674] = 100;
                }
            }
            break;
        case ICN::PORT0095:
            // Necromancer captain have incorrect transparent pixel at position 8x22.
            if ( !_icnVsSprite[id].empty() ) {
                fheroes2::Sprite & original = _icnVsSprite[id][0];
                if ( original.width() == 101 && original.height() == 93 ) {
                    original._disableTransformLayer();
                    original.image()[2230] = 212;
                }
            }
            break;
        case ICN::CSTLWZRD:
            if ( _icnVsSprite[id].size() >= 8 ) {
                // Statue image has bad pixels.
                fheroes2::Sprite & original = _icnVsSprite[id][7];
                if ( original.width() == 135 && original.height() == 57 ) {
                    original._disableTransformLayer();
                    uint8_t * imageData = original.image();
                    imageData[3687] = 50;
                    imageData[5159] = 108;
                    imageData[5294] = 108;
                }
            }
            if ( _icnVsSprite[id].size() > 22 ) {
                // Cliff nest image has a glowing pixel.
                ApplyPalette( _icnVsSprite[id][22], PAL::GetPalette( PAL::PaletteType::NO_CYCLE ) );
            }
            if ( _icnVsSprite[id].size() > 28 ) {
                // Mage tower image has a bad pixel.
                for ( const uint32_t index : { 23, 28 } ) {
                    fheroes2::Sprite & original = _icnVsSprite[id][index];
                    if ( original.width() == 135 && original.height() == 57 ) {
                        original._disableTransformLayer();
                        original.image()[4333] = 23;
                    }
                }
            }
            break;
        case ICN::CSTLCAPK:
            // Knight captain has a bad pixel.
            if ( _icnVsSprite[id].size() >= 2 ) {
                fheroes2::Sprite & original = _icnVsSprite[id][1];
                if ( original.width() == 84 && original.height() == 81 ) {
                    original._disableTransformLayer();
                    original.image()[4934] = 18;
                }
            }
            break;
        case ICN::CSTLCAPW:
            // Warlock captain quarters have bad pixels.
            if ( !_icnVsSprite[id].empty() ) {
                fheroes2::Sprite & original = _icnVsSprite[id][0];
                if ( original.width() == 84 && original.height() == 81 ) {
                    original._disableTransformLayer();
                    uint8_t * imageData = original.image();
                    imageData[1692] = 26;
                    imageData[2363] = 32;
                    imageData[2606] = 21;
                    imageData[2608] = 21;
                }
            }
            break;
        case ICN::CSTLSORC:
            if ( _icnVsSprite[id].size() >= 14 ) {
                // Rainbow has bad pixels.
                fheroes2::Sprite & original = _icnVsSprite[id][13];
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
                fheroes2::Sprite & original = _icnVsSprite[id][24];
                if ( original.width() == 135 && original.height() == 57 ) {
                    original._disableTransformLayer();
                    uint8_t * imageData = original.image();
                    imageData[2830] = 165;
                    imageData[3101] = 165;
                    imageData[3221] = 69;
                }
            }
            break;
        case ICN::COLOR_CURSOR_ADVENTURE_MAP:
        case ICN::MONO_CURSOR_ADVENTURE_MAP: {
            // Create needed digits.
            const std::vector<fheroes2::Point> twoPoints = { { 2, 1 }, { 3, 1 }, { 1, 2 }, { 4, 2 }, { 3, 3 }, { 2, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 }, { 4, 5 } };
            const std::vector<fheroes2::Point> threePoints
                = { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 }, { 4, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 } };
            const std::vector<fheroes2::Point> fourPoints = { { 1, 1 }, { 3, 1 }, { 1, 2 }, { 3, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 }, { 4, 3 }, { 3, 4 }, { 3, 5 } };
            const std::vector<fheroes2::Point> fivePoints
                = { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 1, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 }, { 4, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 } };
            const std::vector<fheroes2::Point> sixPoints = { { 2, 1 }, { 3, 1 }, { 1, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 }, { 1, 4 }, { 4, 4 }, { 2, 5 }, { 3, 5 } };
            const std::vector<fheroes2::Point> sevenPoints = { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 4, 2 }, { 3, 3 }, { 2, 4 }, { 2, 5 } };
            const std::vector<fheroes2::Point> plusPoints = { { 2, 1 }, { 1, 2 }, { 2, 2 }, { 3, 2 }, { 2, 3 } };

            const bool isColorCursor = ( id == ICN::COLOR_CURSOR_ADVENTURE_MAP );
            const uint8_t digitColor = isColorCursor ? 115 : 11;

            std::vector<fheroes2::Image> digits( 7 );
            digits[0] = createDigit( 6, 7, twoPoints, digitColor );
            digits[1] = createDigit( 6, 7, threePoints, digitColor );
            digits[2] = createDigit( 6, 7, fourPoints, digitColor );
            digits[3] = createDigit( 6, 7, fivePoints, digitColor );
            digits[4] = createDigit( 6, 7, sixPoints, digitColor );
            digits[5] = createDigit( 6, 7, sevenPoints, digitColor );
            digits[6] = addDigit( digits[5], createDigit( 5, 5, plusPoints, digitColor ), { -1, -1 } );

            _icnVsSprite[id].reserve( 7 * 8 );

            const int originalCursorId = isColorCursor ? ICN::ADVMCO : ICN::MONO_CURSOR_ADVMBW;

            populateCursorIcons( _icnVsSprite[id], fheroes2::AGG::GetICN( originalCursorId, 4 ), digits,
                                 isColorCursor ? fheroes2::Point( -2, 1 ) : fheroes2::Point( -4, -6 ) );
            populateCursorIcons( _icnVsSprite[id], fheroes2::AGG::GetICN( originalCursorId, 5 ), digits,
                                 isColorCursor ? fheroes2::Point( 1, 1 ) : fheroes2::Point( -6, -6 ) );
            populateCursorIcons( _icnVsSprite[id], fheroes2::AGG::GetICN( originalCursorId, 6 ), digits,
                                 isColorCursor ? fheroes2::Point( 0, 1 ) : fheroes2::Point( -8, -7 ) );
            populateCursorIcons( _icnVsSprite[id], fheroes2::AGG::GetICN( originalCursorId, 7 ), digits,
                                 isColorCursor ? fheroes2::Point( -2, 1 ) : fheroes2::Point( -15, -8 ) );
            populateCursorIcons( _icnVsSprite[id], fheroes2::AGG::GetICN( originalCursorId, 8 ), digits,
                                 isColorCursor ? fheroes2::Point( 1, 1 ) : fheroes2::Point( -16, -11 ) );
            populateCursorIcons( _icnVsSprite[id], fheroes2::AGG::GetICN( originalCursorId, 9 ), digits,
                                 isColorCursor ? fheroes2::Point( -6, 1 ) : fheroes2::Point( -8, -1 ) );
            populateCursorIcons( _icnVsSprite[id], fheroes2::AGG::GetICN( originalCursorId, 28 ), digits,
                                 isColorCursor ? fheroes2::Point( 0, 1 ) : fheroes2::Point( -8, -7 ) );

            break;
        }
        case ICN::KNIGHT_CASTLE_RIGHT_FARM: {
            _icnVsSprite[id].resize( 1 );
            fheroes2::Sprite & output = _icnVsSprite[id][0];
            output = fheroes2::AGG::GetICN( ICN::TWNKWEL2, 0 );

            ApplyPalette( output, 28, 21, output, 28, 21, 39, 1, 8 );
            ApplyPalette( output, 0, 22, output, 0, 22, 69, 1, 8 );
            ApplyPalette( output, 0, 23, output, 0, 23, 53, 1, 8 );
            ApplyPalette( output, 0, 24, output, 0, 24, 54, 1, 8 );
            ApplyPalette( output, 0, 25, output, 0, 25, 62, 1, 8 );
            break;
        }
        case ICN::KNIGHT_CASTLE_LEFT_FARM:
            _icnVsSprite[id].resize( 1 );
            fheroes2::h2d::readImage( "knight_castle_left_farm.image", _icnVsSprite[id][0] );
            break;
        case ICN::BARBARIAN_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE:
            _icnVsSprite[id].resize( 1 );
            fheroes2::h2d::readImage( "barbarian_castle_captain_quarter_left_side.image", _icnVsSprite[id][0] );
            break;
        case ICN::SORCERESS_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE:
            _icnVsSprite[id].resize( 1 );
            fheroes2::h2d::readImage( "sorceress_castle_captain_quarter_left_side.image", _icnVsSprite[id][0] );
            break;
        case ICN::NECROMANCER_CASTLE_STANDALONE_CAPTAIN_QUARTERS: {
            _icnVsSprite[id].resize( 1 );
            fheroes2::Sprite & output = _icnVsSprite[id][0];
            const fheroes2::Sprite & original = fheroes2::AGG::GetICN( ICN::TWNNCAPT, 0 );

            output = Crop( original, 21, 0, original.width() - 21, original.height() );
            output.setPosition( original.x() + 21, original.y() );

            for ( int32_t y = 47; y < output.height(); ++y ) {
                SetTransformPixel( output, 0, y, 1 );
            }

            const fheroes2::Sprite & castle = fheroes2::AGG::GetICN( ICN::TWNNCSTL, 0 );
            Copy( castle, 402, 123, output, 1, 56, 2, 11 );

            break;
        }
        case ICN::NECROMANCER_CASTLE_CAPTAIN_QUARTERS_BRIDGE: {
            _icnVsSprite[id].resize( 1 );
            fheroes2::Sprite & output = _icnVsSprite[id][0];
            const fheroes2::Sprite & original = fheroes2::AGG::GetICN( ICN::TWNNCAPT, 0 );

            output = Crop( original, 0, 0, 23, original.height() );
            output.setPosition( original.x(), original.y() );

            break;
        }
        case ICN::ESCROLL:
            if ( _icnVsSprite[id].size() > 4 ) {
                // fix missing black border on the right side of the "up" button
                fheroes2::Sprite & out = _icnVsSprite[id][4];
                if ( out.width() == 16 && out.height() == 16 ) {
                    out._disableTransformLayer();
                    Copy( out, 0, 0, out, 15, 0, 1, 16 );
                }
            }
            break;
        case ICN::MAP_TYPE_ICON: {
            _icnVsSprite[id].resize( 3 );
            for ( fheroes2::Sprite & icon : _icnVsSprite[id] ) {
                icon._disableTransformLayer();
                icon.resize( 17, 17 );
                icon.fill( 0 );
            }

            const fheroes2::Sprite & successionWarsIcon = fheroes2::AGG::GetICN( ICN::ARTFX, 6 );
            const fheroes2::Sprite & priceOfLoyaltyIcon = fheroes2::AGG::GetICN( ICN::ARTFX, 90 );
            const fheroes2::Sprite & resurrectionIcon = fheroes2::AGG::GetICN( ICN::ARTFX, 101 );

            if ( !successionWarsIcon.empty() ) {
                Resize( successionWarsIcon, 0, 0, successionWarsIcon.width(), successionWarsIcon.height(), _icnVsSprite[id][0], 1, 1, 15, 15 );
            }

            if ( !priceOfLoyaltyIcon.empty() ) {
                Resize( priceOfLoyaltyIcon, 0, 0, priceOfLoyaltyIcon.width(), priceOfLoyaltyIcon.height(), _icnVsSprite[id][1], 1, 1, 15, 15 );
            }

            if ( !resurrectionIcon.empty() ) {
                Resize( resurrectionIcon, 0, 0, resurrectionIcon.width(), resurrectionIcon.height(), _icnVsSprite[id][2], 1, 1, 15, 15 );
            }

            break;
        }
        case ICN::TWNWWEL2: {
            if ( _icnVsSprite[id].size() == 7 ) {
                if ( _icnVsSprite[id][0].width() == 122 && _icnVsSprite[id][0].height() == 226 ) {
                    FillTransform( _icnVsSprite[id][0], 0, 57, 56, 62, 1 );
                }

                for ( size_t i = 1; i < 7; ++i ) {
                    fheroes2::Sprite & original = _icnVsSprite[id][i];
                    if ( original.width() == 121 && original.height() == 151 ) {
                        FillTransform( original, 0, 0, 64, 39, 1 );
                    }
                }
            }
            break;
        }
        case ICN::TWNWCAPT: {
            if ( !_icnVsSprite[id].empty() ) {
                fheroes2::Sprite & original = _icnVsSprite[id][0];
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
            break;
        }
        case ICN::GOOD_ARMY_BUTTON:
        case ICN::GOOD_MARKET_BUTTON: {
            _icnVsSprite[id].resize( 2 );

            loadICN( ICN::ADVBTNS );

            const int releasedIndex = ( id == ICN::GOOD_ARMY_BUTTON ) ? 0 : 4;
            Copy( fheroes2::AGG::GetICN( ICN::ADVBTNS, releasedIndex ), _icnVsSprite[id][0] );
            Copy( fheroes2::AGG::GetICN( ICN::ADVBTNS, releasedIndex + 1 ), _icnVsSprite[id][1] );

            // Make all black pixels transparent.
            addTransparency( _icnVsSprite[id][0], 36 );
            addTransparency( _icnVsSprite[id][1], 36 );
            addTransparency( _icnVsSprite[id][1], 61 ); // remove the extra brown border

            break;
        }
        case ICN::EVIL_ARMY_BUTTON:
        case ICN::EVIL_MARKET_BUTTON: {
            _icnVsSprite[id].resize( 2 );

            loadICN( ICN::ADVEBTNS );

            const int releasedIndex = ( id == ICN::EVIL_ARMY_BUTTON ) ? 0 : 4;
            Copy( fheroes2::AGG::GetICN( ICN::ADVEBTNS, releasedIndex ), _icnVsSprite[id][0] );
            Copy( fheroes2::AGG::GetICN( ICN::ADVEBTNS, releasedIndex + 1 ), _icnVsSprite[id][1] );

            // Make all black pixels transparent.
            addTransparency( _icnVsSprite[id][0], 36 );
            addTransparency( _icnVsSprite[id][1], 36 );

            // Add the bottom-left dark border.
            Fill( _icnVsSprite[id][1], 1, 4, 1, 30, 36 );
            Fill( _icnVsSprite[id][1], 1, 34, 31, 1, 36 );

            // Restore back black pixels in the middle of the image.
            Copy( _icnVsSprite[ICN::ADVEBTNS][releasedIndex], 9, 6, _icnVsSprite[id][0], 9, 6, 20, 22 );
            Copy( _icnVsSprite[ICN::ADVEBTNS][releasedIndex + 1], 8, 7, _icnVsSprite[id][1], 8, 7, 20, 22 );

            break;
        }
        case ICN::SPANBTN:
        case ICN::SPANBTNE:
        case ICN::CSPANBTN:
        case ICN::CSPANBTE: {
            if ( !_icnVsSprite[id].empty() ) {
                // add missing part of the released button state on the left
                fheroes2::Sprite & out = _icnVsSprite[id][0];

                fheroes2::Sprite released( out.width() + 1, out.height() );
                released.reset();
                const uint8_t color = id == ICN::SPANBTN || id == ICN::CSPANBTN ? 57 : 32;
                DrawLine( released, { 0, 3 }, { 0, out.height() - 1 }, color );
                Blit( out, released, 1, 0 );

                out = std::move( released );
            }
            break;
        }
        case ICN::TRADPOSE: {
            if ( _icnVsSprite[id].size() >= 19 ) {
                // fix background for TRADE and EXIT buttons
                for ( const uint32_t i : { 16, 18 } ) {
                    fheroes2::Sprite pressed;
                    std::swap( pressed, _icnVsSprite[id][i] );
                    addTransparency( pressed, 25 ); // remove too dark background

                    // take background from the empty system button
                    _icnVsSprite[id][i] = fheroes2::AGG::GetICN( ICN::SYSTEME, 12 );

                    // put back dark-gray pixels in the middle of the button
                    Fill( _icnVsSprite[id][i], 5, 5, 86, 17, 25 );
                    Blit( pressed, _icnVsSprite[id][i] );
                }
            }
            break;
        }
        case ICN::RECRUIT: {
            if ( _icnVsSprite[id].size() >= 10 ) {
                // fix transparent corners on released OKAY button
                copyTransformLayer( _icnVsSprite[id][9], _icnVsSprite[id][8] );
            }
            break;
        }
        case ICN::NGEXTRA: {
            std::vector<fheroes2::Sprite> & images = _icnVsSprite[id];

            if ( images.size() != 82 ) {
                // The game assets are wrong, skip modifications.
                break;
            }

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

            // fix transparent corners on pressed OKAY and CANCEL buttons
            copyTransformLayer( images[66], images[67] );
            copyTransformLayer( images[68], images[69] );

            // Add 6 special icons for the Editor.
            images.resize( 82 + 6 );

            for ( size_t i = 0; i < 6; ++i ) {
                if ( images[i + 3].width() != 62 || images[i + 3].height() != 45 ) {
                    continue;
                }

                fheroes2::Sprite & humonOrAiImage = images[i + 82];
                Copy( images[i + 3], humonOrAiImage );

                // Fill the icon with the player's color.
                Fill( humonOrAiImage, 15, 8, 32, 30, images[i + 82].image()[252] );

                // Make a temporary image to cut human icon's background.
                fheroes2::Image temp( 33, 35 );
                Copy( images[i + 9], 15, 5, temp, 0, 0, 33, 35 );
                ReplaceColorIdByTransformId( temp, images[i + 82].image()[252], 1U );

                Copy( images[i + 3], 15, 8, humonOrAiImage, 27, 8, 31, 30 );
                Blit( temp, humonOrAiImage, 4, 5 );
            }

            break;
        }
        case ICN::DIFFICULTY_ICON_EASY:
        case ICN::DIFFICULTY_ICON_NORMAL:
        case ICN::DIFFICULTY_ICON_HARD:
        case ICN::DIFFICULTY_ICON_EXPERT:
        case ICN::DIFFICULTY_ICON_IMPOSSIBLE: {
            const int originalIcnId = ICN::NGHSBKG;

            const fheroes2::Sprite & originalBackground = fheroes2::AGG::GetICN( originalIcnId, 0 );

            if ( !originalBackground.empty() ) {
                _icnVsSprite[id].resize( 2 );

                int32_t iconOffsetX = 0;

                switch ( id ) {
                case ICN::DIFFICULTY_ICON_EASY:
                    iconOffsetX = 24;
                    break;
                case ICN::DIFFICULTY_ICON_NORMAL:
                    iconOffsetX = 101;
                    break;
                case ICN::DIFFICULTY_ICON_HARD:
                    iconOffsetX = 177;
                    break;
                case ICN::DIFFICULTY_ICON_EXPERT:
                    iconOffsetX = 254;
                    break;
                case ICN::DIFFICULTY_ICON_IMPOSSIBLE:
                    iconOffsetX = 331;
                    break;
                default:
                    // Did you add a new difficulty?
                    assert( 0 );
                }

                const int32_t iconSideLength = 65;
                const int32_t iconOffsetY = 94;

                _icnVsSprite[id][0] = Crop( originalBackground, iconOffsetX, iconOffsetY, iconSideLength, iconSideLength );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                // Generate Evil Icons
                _icnVsSprite[id][1] = _icnVsSprite[id][0];

                const std::vector<uint8_t> & goodToEvilPalette = PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE );
                fheroes2::ApplyPalette( _icnVsSprite[id][0], _icnVsSprite[id][1], goodToEvilPalette );
            }

            break;
        }
        case ICN::METALLIC_BORDERED_TEXTBOX_GOOD: {
            const int originalIcnId = ICN::NGHSBKG;

            const fheroes2::Sprite & originalBackground = fheroes2::AGG::GetICN( originalIcnId, 0 );

            if ( !originalBackground.empty() ) {
                _icnVsSprite[id].resize( 1 );

                const int32_t boxWidth = 371;
                const int32_t boxHeight = 30;
                const int32_t goodOriginalBoxOffsetX = 24;
                const int32_t goodOriginalBoxOffsetY = 40;

                _icnVsSprite[id][0] = Crop( originalBackground, goodOriginalBoxOffsetX, goodOriginalBoxOffsetY, boxWidth, boxHeight );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                // Copy red pattern and cover up embedded button.
                const fheroes2::Image & redPart = fheroes2::Flip( Crop( originalBackground, 80, 45, 81, 19 ), true, false );
                Copy( redPart, 0, 0, _icnVsSprite[id][0], 284, 5, 81, 19 );
            }

            break;
        }
        case ICN::METALLIC_BORDERED_TEXTBOX_EVIL: {
            const fheroes2::Sprite & originalEvilBackground = fheroes2::AGG::GetICN( ICN::CAMPBKGE, 0 );

            if ( !originalEvilBackground.empty() ) {
                _icnVsSprite[id].resize( 1 );

                const int32_t boxWidth = 371;
                const int32_t boxHeight = 30;
                _icnVsSprite[id][0].resize( boxWidth, boxHeight );
                _icnVsSprite[id][0].reset();

                const int32_t evilOriginalBoxOffsetX = 26;
                const int32_t evilOriginalBoxOffsetY = 27;
                const int32_t upperPartHeight = 20;

                // The metallic box frame in campbkge is slightly taller than the one in nghsbkg. The width is the same.
                Copy( originalEvilBackground, evilOriginalBoxOffsetX, evilOriginalBoxOffsetY, _icnVsSprite[id][0], 0, 0, boxWidth, upperPartHeight );

                Copy( originalEvilBackground, evilOriginalBoxOffsetX, evilOriginalBoxOffsetY + upperPartHeight + 14, _icnVsSprite[id][0], 0, upperPartHeight, boxWidth,
                      10 );

                // Copy red central part.
                const fheroes2::Sprite goodBox = fheroes2::AGG::GetICN( ICN::METALLIC_BORDERED_TEXTBOX_GOOD, 0 );
                Copy( goodBox, 6, 5, _icnVsSprite[id][0], 6, 5, 359, 19 );
            }

            break;
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

                _icnVsSprite[id][i] = fheroes2::decodeBMPFile( AGG::getDataFromAggFile( std::string( "ADVMBW" ) + digit + ".BMP", false ) );
            }
            break;
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

                _icnVsSprite[id][i] = fheroes2::decodeBMPFile( AGG::getDataFromAggFile( std::string( "SPELBW" ) + digit + ".BMP", false ) );
            }
            break;
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

                _icnVsSprite[id][i] = fheroes2::decodeBMPFile( AGG::getDataFromAggFile( std::string( "CMSEBW" ) + digit + ".BMP", false ) );
            }
            break;
        }
        case ICN::ADVBTNS:
        case ICN::ADVEBTNS:
            if ( _icnVsSprite[id].size() == 16 && _icnVsSprite[id][2].width() == 36 && _icnVsSprite[id][2].height() == 36 && _icnVsSprite[id][3].width() == 36
                 && _icnVsSprite[id][3].height() == 36 ) {
                // Add hero action button and inactive button released and pressed.
                _icnVsSprite[id].resize( 20 );
                const bool isGoodButton = id == ICN::ADVBTNS;
                const int emptyButtonId = isGoodButton ? ICN::EMPTY_INTERFACE_BUTTON_GOOD : ICN::EMPTY_INTERFACE_BUTTON_EVIL;

                // Get the action cursor and prepare it for button. We make it a little smaller.
                const fheroes2::Sprite & originalActionCursor = fheroes2::AGG::GetICN( ICN::ADVMCO, 9 );
                const int32_t actionCursorWidth = originalActionCursor.width() - 1;
                const int32_t actionCursorHeight = originalActionCursor.height() - 3;
                fheroes2::Image buttonImage( actionCursorWidth, actionCursorHeight );
                buttonImage.reset();

                // Head.
                Copy( originalActionCursor, 19, 1, buttonImage, 17, 2, 8, 5 );
                Copy( originalActionCursor, 16, 7, buttonImage, 14, 7, 12, 2 );
                buttonImage.transform()[15 + 7 * actionCursorWidth] = 1U;
                // Tail.
                Copy( originalActionCursor, 1, 10, buttonImage, 1, 9, 12, 11 );
                // Middle part.
                Copy( originalActionCursor, 14, 10, buttonImage, 13, 9, 1, 11 );
                // Front legs.
                Copy( originalActionCursor, 16, 10, buttonImage, 14, 9, 13, 11 );
                // Hind legs.
                Copy( originalActionCursor, 7, 22, buttonImage, 7, 19, 7, 7 );

                // Get the button's icon colors.
                const uint8_t mainReleasedColor = _icnVsSprite[id][2].image()[7 * 36 + 26];
                const uint8_t mainPressedColor = _icnVsSprite[id][3].image()[8 * 36 + 25];

                // Make contour transparent and the horse figure filled with solid color.
                const int32_t actionCursorSize = actionCursorWidth * actionCursorHeight;
                for ( int32_t i = 0; i < actionCursorSize; ++i ) {
                    if ( buttonImage.transform()[i] == 1U ) {
                        // Skip transparent pixel.
                        continue;
                    }
                    if ( buttonImage.image()[i] < 152U ) {
                        // It is the contour color, make it transparent.
                        buttonImage.transform()[i] = 1U;
                    }
                    else {
                        buttonImage.image()[i] = mainPressedColor;
                    }
                }

                // Add shadows to the horse image.
                updateShadow( buttonImage, { 1, -1 }, 2, true );
                updateShadow( buttonImage, { -1, 1 }, 6, true );
                updateShadow( buttonImage, { 2, -2 }, 4, true );

                const fheroes2::Sprite & emptyButtonPressed = fheroes2::AGG::GetICN( emptyButtonId, 1 );
                Copy( emptyButtonPressed, _icnVsSprite[id][17] );
                Blit( buttonImage, _icnVsSprite[id][17], 4, 4 );

                // Replace colors for the released button.
                for ( int32_t i = 0; i < actionCursorSize; ++i ) {
                    if ( buttonImage.transform()[i] == 6U ) {
                        // Disable whitening transform and set white color.
                        buttonImage.transform()[i] = 0U;
                        buttonImage.image()[i] = 10U;
                    }
                }
                ReplaceColorId( buttonImage, mainPressedColor, mainReleasedColor );

                const fheroes2::Sprite & emptyButtonReleased = fheroes2::AGG::GetICN( emptyButtonId, 0 );
                Copy( emptyButtonReleased, _icnVsSprite[id][16] );
                Blit( buttonImage, _icnVsSprite[id][16], 5, 3 );

                // Generate inactive horse icon.
                fheroes2::Sprite & releasedInactiveHorse = _icnVsSprite[id][18];
                Copy( emptyButtonReleased, releasedInactiveHorse );

                const uint8_t mainShadingColor = isGoodButton ? 46 : 25;
                const uint8_t secondShadingColor = isGoodButton ? 43 : 18;
                const uint8_t thirdShadingColor = isGoodButton ? 39 : 15;
                const uint8_t fourthShadingColor = isGoodButton ? 42 : 16;
                const uint8_t fifthShadingColor = isGoodButton ? 38 : 14;

                // Upper body.
                Copy( _icnVsSprite[id][2], 22, 5, releasedInactiveHorse, 22, 4, 10, 10 );
                fheroes2::SetPixel( releasedInactiveHorse, 31, 13, secondShadingColor );
                Fill( releasedInactiveHorse, 12, 13, 15, 5, mainReleasedColor );
                Fill( releasedInactiveHorse, 12, 18, 5, 2, mainReleasedColor );
                Fill( releasedInactiveHorse, 22, 18, 5, 2, mainReleasedColor );
                fheroes2::SetPixel( releasedInactiveHorse, 21, 12, mainReleasedColor );

                // Copy one leg 4 times.
                Copy( _icnVsSprite[id][2], 13, 22, releasedInactiveHorse, 11, 22, 4, 6 );
                fheroes2::SetPixel( releasedInactiveHorse, 11, 24, 10 );
                fheroes2::SetPixel( releasedInactiveHorse, 14, 24, secondShadingColor );
                fheroes2::SetPixel( releasedInactiveHorse, 14, 27, secondShadingColor );
                Fill( releasedInactiveHorse, 12, 20, 2, 2, mainReleasedColor );

                Copy( releasedInactiveHorse, 11, 21, releasedInactiveHorse, 21, 21, 4, 7 );
                Copy( releasedInactiveHorse, 11, 21, releasedInactiveHorse, 24, 20, 4, 7 );
                Copy( releasedInactiveHorse, 11, 21, releasedInactiveHorse, 14, 20, 4, 7 );
                fheroes2::SetPixel( releasedInactiveHorse, 24, 27, fourthShadingColor );

                fheroes2::SetPixel( releasedInactiveHorse, 17, 18, mainReleasedColor );
                fheroes2::SetPixel( releasedInactiveHorse, 21, 18, mainReleasedColor );
                fheroes2::DrawLine( releasedInactiveHorse, { 22, 20 }, { 23, 20 }, mainReleasedColor );

                // Tail.
                fheroes2::DrawLine( releasedInactiveHorse, { 11, 14 }, { 10, 15 }, mainReleasedColor );
                fheroes2::SetPixel( releasedInactiveHorse, 11, 15, mainReleasedColor );
                Fill( releasedInactiveHorse, 9, 16, 2, 4, mainReleasedColor );
                fheroes2::DrawLine( releasedInactiveHorse, { 7, 19 }, { 8, 19 }, mainReleasedColor );
                fheroes2::DrawLine( releasedInactiveHorse, { 8, 20 }, { 9, 20 }, mainReleasedColor );

                // Shading.
                fheroes2::DrawLine( releasedInactiveHorse, { 27, 13 }, { 27, 20 }, mainShadingColor );
                fheroes2::DrawLine( releasedInactiveHorse, { 21, 11 }, { 20, 12 }, mainShadingColor );
                fheroes2::DrawLine( releasedInactiveHorse, { 19, 12 }, { 17, 12 }, secondShadingColor );
                fheroes2::SetPixel( releasedInactiveHorse, 20, 11, secondShadingColor );
                fheroes2::DrawLine( releasedInactiveHorse, { 16, 12 }, { 12, 12 }, mainShadingColor );
                fheroes2::SetPixel( releasedInactiveHorse, 11, 12, secondShadingColor );
                fheroes2::DrawLine( releasedInactiveHorse, { 11, 13 }, { 9, 15 }, mainShadingColor );
                fheroes2::SetPixel( releasedInactiveHorse, 10, 13, fifthShadingColor );
                fheroes2::SetPixel( releasedInactiveHorse, 9, 14, 10 );
                fheroes2::DrawLine( releasedInactiveHorse, { 8, 15 }, { 8, 18 }, 10 );
                fheroes2::SetPixel( releasedInactiveHorse, 7, 18, secondShadingColor );
                fheroes2::DrawLine( releasedInactiveHorse, { 6, 19 }, { 8, 21 }, 10 );
                fheroes2::DrawLine( releasedInactiveHorse, { 6, 20 }, { 7, 21 }, thirdShadingColor );
                fheroes2::SetPixel( releasedInactiveHorse, 9, 21, 10 );
                fheroes2::SetPixel( releasedInactiveHorse, 10, 20, thirdShadingColor );
                fheroes2::SetPixel( releasedInactiveHorse, 11, 16, mainShadingColor );
                fheroes2::SetPixel( releasedInactiveHorse, 11, 17, secondShadingColor );
                fheroes2::SetPixel( releasedInactiveHorse, 11, 21, 10 );
                fheroes2::DrawLine( releasedInactiveHorse, { 14, 20 }, { 14, 21 }, thirdShadingColor );
                fheroes2::DrawLine( releasedInactiveHorse, { 17, 19 }, { 17, 20 }, mainShadingColor );
                fheroes2::DrawLine( releasedInactiveHorse, { 18, 18 }, { 20, 18 }, mainShadingColor );
                fheroes2::SetPixel( releasedInactiveHorse, 21, 19, mainShadingColor );
                fheroes2::SetPixel( releasedInactiveHorse, 18, 19, thirdShadingColor );
                fheroes2::DrawLine( releasedInactiveHorse, { 19, 19 }, { 20, 19 }, 10 );
                fheroes2::DrawLine( releasedInactiveHorse, { 21, 20 }, { 21, 21 }, 10 );
                fheroes2::SetPixel( releasedInactiveHorse, 24, 20, thirdShadingColor );

                // Clean up button border.
                fheroes2::DrawLine( releasedInactiveHorse, { 23, 4 }, { 22, 5 }, 10 );
                const uint8_t backgroundReleasedColor = isGoodButton ? 41 : 16;
                fheroes2::SetPixel( releasedInactiveHorse, 22, 4, backgroundReleasedColor );

                // Make pressed state.
                // To keep the button's edge colors do all color manipulations on a temporary single-layer image.
                const int32_t pressedHorseImageWidth = 26;
                const int32_t pressedHorseImageHeight = 24;
                buttonImage._disableTransformLayer();
                buttonImage.resize( pressedHorseImageWidth, pressedHorseImageHeight );

                const uint8_t backgroundPressedColor = isGoodButton ? 45 : 22;
                Fill( buttonImage, 0, 0, pressedHorseImageWidth, pressedHorseImageHeight, backgroundPressedColor );
                Copy( releasedInactiveHorse, 6, 11, buttonImage, 0, 7, 22, 17 );
                Copy( releasedInactiveHorse, 24, 4, buttonImage, 18, 0, 8, 10 );
                Copy( releasedInactiveHorse, 22, 8, buttonImage, 16, 4, 2, 3 );

                fheroes2::ReplaceColorId( buttonImage, mainReleasedColor, mainPressedColor );
                const uint8_t pressedColorOffset = isGoodButton ? 4 : 6;
                fheroes2::ReplaceColorId( buttonImage, backgroundReleasedColor, backgroundPressedColor );
                fheroes2::ReplaceColorId( buttonImage, mainShadingColor, mainShadingColor + pressedColorOffset );
                fheroes2::ReplaceColorId( buttonImage, secondShadingColor, secondShadingColor + pressedColorOffset );
                fheroes2::ReplaceColorId( buttonImage, thirdShadingColor, thirdShadingColor + pressedColorOffset );
                fheroes2::ReplaceColorId( buttonImage, fourthShadingColor, fourthShadingColor + pressedColorOffset );
                fheroes2::ReplaceColorId( buttonImage, fifthShadingColor, fifthShadingColor + pressedColorOffset );
                fheroes2::ReplaceColorId( buttonImage, 10, isGoodButton ? 40 : 14 );

                Copy( emptyButtonPressed, _icnVsSprite[id][19] );
                Copy( buttonImage, 0, 0, _icnVsSprite[id][19], 5, 5, pressedHorseImageWidth, pressedHorseImageHeight );
                // Fix left border.
                if ( isGoodButton ) {
                    fheroes2::DrawLine( _icnVsSprite[id][19], { 5, 5 }, { 5, 19 }, 44 );
                    fheroes2::DrawLine( _icnVsSprite[id][19], { 5, 21 }, { 5, 28 }, 44 );
                }
            }
            break;
        case ICN::ARTFX:
            if ( _icnVsSprite[id].size() > 82 ) {
                // Make a sprite for EDITOR_ANY_ULTIMATE_ARTIFACT used only in Editor for the special victory condition.
                // A temporary solution is below.
                const fheroes2::Sprite & originalImage = fheroes2::AGG::GetICN( ICN::ARTIFACT, 83 );
                SubpixelResize( originalImage, _icnVsSprite[id][82] );
            }
            if ( _icnVsSprite[id].size() > 63 ) {
                // This fixes "Golden Bow" (#63) small artifact icon glowing yellow pixel
                Copy( _icnVsSprite[id][63], 12, 17, _icnVsSprite[id][63], 16, 12, 1, 1 );
            }
            break;
        case ICN::ARTIFACT:
            if ( _icnVsSprite[id].size() > 99 ) {
                // This fixes "Golden Bow" (#64) large artifact icon glowing yellow pixel
                Copy( _icnVsSprite[id][64], 35, 24, _icnVsSprite[id][64], 56, 12, 1, 1 );

                // This fixes "Arm of the Martyr" (#88) and " Sphere of Negation" (#99) artifacts rendering which initially has some incorrect transparent pixels.
                for ( const int32_t index : { 88, 99 } ) {
                    fheroes2::Sprite & originalImage = _icnVsSprite[id][index];
                    fheroes2::Sprite temp( originalImage.width(), originalImage.height() );
                    temp.setPosition( originalImage.x(), originalImage.y() );
                    temp._disableTransformLayer();
                    temp.fill( 0 );
                    Blit( originalImage, temp );
                    originalImage = std::move( temp );
                }

                // Make a sprite for EDITOR_ANY_ULTIMATE_ARTIFACT used only in Editor for the special victory condition.
                // A temporary solution: apply the blur effect originally used for the Holy Shout spell and the purple palette.
                fheroes2::Sprite & targetImage = _icnVsSprite[id][83];
                targetImage = CreateHolyShoutEffect( _icnVsSprite[id][91], 1, 0 );
                ApplyPalette( targetImage, PAL::GetPalette( PAL::PaletteType::PURPLE ) );

                // The French and German Price of Loyalty assets contain a wrong artifact sprite at index 6. We replace it with the correct sprite from SW assets.
                const int assetIndex = 6;
                if ( _icnVsSprite[id][assetIndex].width() == 21 ) {
                    replacePOLAssetWithSW( id, assetIndex );
                }
            }
            break;
        case ICN::OBJNARTI:
            if ( _icnVsSprite[id].size() == 206 ) {
                // These are the Price of Loyalty assets.

                // Spell Scroll has an invalid offset by X axis.
                if ( _icnVsSprite[id][173].width() == 21 ) {
                    _icnVsSprite[id][173].setPosition( 2, _icnVsSprite[id][173].y() );
                }

                // Make a map sprite for the Magic Book artifact.
                _icnVsSprite[id].resize( 208 );

                // Magic book sprite shadow.
                fheroes2::Sprite shadow = _icnVsSprite[id][162];
                FillTransform( shadow, 0, 0, 5, 1, 1U );
                FillTransform( shadow, 0, 1, 2, 1, 1U );
                FillTransform( shadow, 2, 1, 4, 1, 3U );
                FillTransform( shadow, 0, 2, 3, 1, 3U );
                FillTransform( shadow, 18, 1, 2, 1, 1U );
                FillTransform( shadow, 17, 2, 3, 1, 3U );
                FillTransform( shadow, 20, 2, 1, 1, 1U );
                FillTransform( shadow, 19, 3, 2, 1, 3U );

                // Magic Book main sprite. We use sprite from the info dialog to make the map sprite.
                fheroes2::Sprite body( 21, 32 );
                Copy( fheroes2::AGG::GetICN( ICN::ARTFX, 81 ), 6, 0, body, 0, 0, 21, 32 );
                FillTransform( body, 0, 0, 12, 1, 1U );
                FillTransform( body, 15, 0, 6, 1, 1U );
                FillTransform( body, 0, 1, 9, 1, 1U );
                FillTransform( body, 16, 1, 5, 1, 1U );
                FillTransform( body, 0, 2, 6, 1, 1U );
                FillTransform( body, 20, 2, 1, 1, 1U );
                FillTransform( body, 0, 4, 1, 1, 1U );
                FillTransform( body, 0, 3, 3, 1, 1U );
                FillTransform( body, 20, 25, 1, 1, 1U );
                FillTransform( body, 18, 26, 3, 1, 1U );
                FillTransform( body, 16, 27, 5, 1, 1U );
                FillTransform( body, 14, 28, 9, 1, 1U );
                FillTransform( body, 0, 29, 1, 1, 1U );
                FillTransform( body, 12, 29, 11, 1, 1U );
                FillTransform( body, 0, 30, 3, 1, 1U );
                FillTransform( body, 10, 30, 13, 1, 1U );
                FillTransform( body, 0, 31, 5, 1, 1U );
                FillTransform( body, 8, 31, 15, 1, 1U );

                const int32_t shadowOffset = ( 32 - body.width() ) / 2;

                _icnVsSprite[id][206].resize( shadow.width() - shadowOffset, shadow.height() );
                Copy( shadow, 0, 0, _icnVsSprite[id][206], 0, 0, shadow.width(), shadow.height() );
                _icnVsSprite[id][206].setPosition( shadow.x() + shadowOffset, shadow.y() );

                _icnVsSprite[id][207].resize( 21 + shadowOffset, 32 );
                _icnVsSprite[id][207].reset();
                Copy( shadow, shadow.width() - shadowOffset, 0, _icnVsSprite[id][207], 0, shadow.y(), shadowOffset, shadow.height() );
                Copy( body, 0, 0, _icnVsSprite[id][207], shadowOffset, 0, body.width(), body.height() );
            }
            break;
        case ICN::TWNSDW_5:
            if ( !_icnVsSprite[id].empty() && _icnVsSprite[id][0].width() == 140 && _icnVsSprite[id][0].height() == 165 ) {
                fheroes2::Sprite & image = _icnVsSprite[id][0];
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
                Rand::PCG32 seededGen( 751 ); // 751 is and ID of this sprite. To keep the changes constant we need to hardcode this value.

                fillRandomPixelsFromImage( image, { 33, 105, 4, 7 }, image, { 33, 117, 4, 39 }, seededGen );
                fillRandomPixelsFromImage( image, { 41, 105, 5, 9 }, image, { 41, 121, 5, 36 }, seededGen );
                fillRandomPixelsFromImage( image, { 46, 104, 4, 13 }, image, { 46, 118, 4, 39 }, seededGen );

                Copy( image, 37, 113, image, 37, 115, 1, 2 );
                Copy( image, 37, 104, image, 37, 117, 1, 2 );
                Copy( image, 38, 104, image, 38, 118, 2, 1 );
                Copy( image, 37, 113, image, 38, 117, 1, 1 );

                // Create a temporary image to be a holder of pixels.
                fheroes2::Sprite temp( 4 * 2, 8 );
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
            break;
        case ICN::SCENIBKG:
            if ( !_icnVsSprite[id].empty() && _icnVsSprite[id][0].width() == 436 && _icnVsSprite[id][0].height() == 476 ) {
                const fheroes2::Sprite & helper = fheroes2::AGG::GetICN( ICN::CSPANBKE, 1 );
                if ( !helper.empty() ) {
                    fheroes2::Sprite & original = _icnVsSprite[id][0];
                    fheroes2::Sprite temp( original.width(), original.height() + 12 );
                    temp.reset();
                    Copy( original, 0, 0, temp, 0, 0, original.width(), original.height() );
                    Copy( helper, 0, helper.height() - 12, temp, 0, temp.height() - 12, 300, 12 );
                    Copy( helper, helper.width() - ( temp.width() - 300 ), helper.height() - 12, temp, 300 - 16, temp.height() - 12, temp.width() - 300, 12 );
                    original = std::move( temp );
                }
            }
            break;
        case ICN::CSTLCAPS:
            if ( !_icnVsSprite[id].empty() && _icnVsSprite[id][0].width() == 84 && _icnVsSprite[id][0].height() == 81 ) {
                const fheroes2::Sprite & castle = fheroes2::AGG::GetICN( ICN::TWNSCSTL, 0 );
                if ( !castle.empty() ) {
                    Blit( castle, 206, 106, _icnVsSprite[id][0], 2, 2, 33, 67 );
                }
            }
            break;
        case ICN::LGNDXTRA:
            // Exit button is too huge due to 1 pixel presence at the bottom of the image.
            if ( _icnVsSprite[id].size() >= 6 ) {
                auto & original = _icnVsSprite[id];
                if ( original[4].height() == 142 ) {
                    const fheroes2::Point offset( original[4].x(), original[4].y() );
                    original[4] = Crop( original[4], 0, 0, original[4].width(), 25 );
                    original[4].setPosition( offset.x, offset.y );
                }

                if ( original[5].height() == 142 ) {
                    const fheroes2::Point offset( original[5].x(), original[5].y() );
                    original[5] = Crop( original[5], 0, 0, original[5].width(), 25 );
                    original[5].setPosition( offset.x, offset.y );
                }
            }
            break;
        case ICN::LGNDXTRE:
            // Exit button is too huge due to 1 pixel presence at the bottom of the image.
            if ( _icnVsSprite[id].size() >= 6 ) {
                auto & original = _icnVsSprite[id];
                if ( original[4].height() == 142 ) {
                    const fheroes2::Point offset( original[4].x(), original[4].y() );
                    original[4] = Crop( original[4], 0, 0, original[4].width(), 25 );
                    original[4].setPosition( offset.x, offset.y );
                }
            }
            break;
        case ICN::OVERBACK: {
            fheroes2::Sprite & background = _icnVsSprite[id][0];
            // Fill button backgrounds. This bug was present in the original game too.
            Fill( background, 540, 361, 99, 83, 57 );
            Fill( background, 540, 454, 99, 24, 57 );
            fheroes2::Copy( fheroes2::AGG::GetICN( ICN::OVERBACK, 0 ), 540, 444, background, 540, 402, 99, 5 );
            break;
        }
        case ICN::ESPANBKG_EVIL: {
            _icnVsSprite[id].resize( 2 );

            const fheroes2::Rect roi{ 28, 28, 265, 206 };

            fheroes2::Sprite & output = _icnVsSprite[id][0];
            _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::CSPANBKE, 0 );
            Copy( fheroes2::AGG::GetICN( ICN::ESPANBKG, 0 ), roi.x, roi.y, output, roi.x, roi.y, roi.width, roi.height );

            convertToEvilInterface( output, roi );

            _icnVsSprite[id][1] = fheroes2::AGG::GetICN( ICN::ESPANBKG, 1 );

            break;
        }
        case ICN::STONEBAK_EVIL: {
            fheroes2::AGG::GetICN( ICN::STONEBAK, 0 );
            _icnVsSprite[id] = _icnVsSprite[ICN::STONEBAK];
            if ( !_icnVsSprite[id].empty() ) {
                const fheroes2::Rect roi( 0, 0, _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() );
                convertToEvilInterface( _icnVsSprite[id][0], roi );
            }

            break;
        }
        case ICN::STONEBAK_SMALL_POL: {
            _icnVsSprite[id].resize( 1 );
            const fheroes2::Sprite & original = fheroes2::AGG::GetICN( ICN::X_CMPBKG, 0 );
            if ( !original.empty() ) {
                _icnVsSprite[id][0] = Crop( original, original.width() - 272, original.height() - 52, 244, 28 );
            }
            break;
        }
        case ICN::REDBAK_SMALL_VERTICAL: {
            _icnVsSprite[id].resize( 1 );
            const fheroes2::Sprite & original = fheroes2::AGG::GetICN( ICN::HEROBKG, 0 );
            if ( !original.empty() ) {
                _icnVsSprite[id][0] = Crop( original, 0, 0, 37, 230 );
            }
            break;
        }
        case ICN::BLACKBAK: {
            _icnVsSprite[id].resize( 1 );
            fheroes2::Image & background = _icnVsSprite[id][0];
            // This is enough to cover the largest buttons.
            background.resize( 200, 200 );
            fheroes2::Fill( background, 0, 0, background.width(), background.height(), 9 );
            break;
        }
        case ICN::BROWNBAK: {
            _icnVsSprite[id].resize( 1 );
            fheroes2::Image & background = _icnVsSprite[id][0];
            // This is enough to cover the largest buttons.
            background.resize( 200, 200 );
            fheroes2::Fill( background, 0, 0, background.width(), background.height(), 57 );
            break;
        }
        case ICN::UNIFORMBAK_GOOD:
        case ICN::UNIFORMBAK_EVIL: {
            _icnVsSprite[id].resize( 1 );
            const bool isEvilInterface = ( id == ICN::UNIFORMBAK_EVIL );
            const fheroes2::Sprite & original = fheroes2::AGG::GetICN( isEvilInterface ? ICN::BUYBUILE : ICN::BUYBUILD, 1 );
            if ( !original.empty() ) {
                _icnVsSprite[id][0].resize( 246, 45 );
                _icnVsSprite[id][0].reset();
                Copy( original, 0, 0, _icnVsSprite[id][0], 0, 0, 123, 45 );
                Copy( original, 0, 0, _icnVsSprite[id][0], 123, 0, 123, 45 );
            }

            break;
        }
        case ICN::WELLBKG_EVIL: {
            fheroes2::AGG::GetICN( ICN::WELLBKG, 0 );
            _icnVsSprite[id] = _icnVsSprite[ICN::WELLBKG];
            if ( !_icnVsSprite[id].empty() ) {
                const fheroes2::Rect roi( 0, 0, _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() - 19 );
                convertToEvilInterface( _icnVsSprite[id][0], roi );
            }

            break;
        }
        case ICN::CASLWIND_EVIL: {
            fheroes2::AGG::GetICN( ICN::CASLWIND, 0 );
            _icnVsSprite[id] = _icnVsSprite[ICN::CASLWIND];
            if ( !_icnVsSprite[id].empty() ) {
                const fheroes2::Rect roi( 0, 0, _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() );
                convertToEvilInterface( _icnVsSprite[id][0], roi );
            }

            break;
        }
        case ICN::CASLXTRA_EVIL: {
            fheroes2::AGG::GetICN( ICN::CASLXTRA, 0 );
            _icnVsSprite[id] = _icnVsSprite[ICN::CASLXTRA];
            if ( !_icnVsSprite[id].empty() ) {
                const fheroes2::Rect roi( 0, 0, _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() );
                convertToEvilInterface( _icnVsSprite[id][0], roi );
            }

            break;
        }
        case ICN::STRIP_BACKGROUND_EVIL: {
            _icnVsSprite[id].resize( 1 );
            _icnVsSprite[id][0] = fheroes2::AGG::GetICN( ICN::STRIP, 11 );

            const fheroes2::Rect roi( 0, 0, _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() - 7 );
            convertToEvilInterface( _icnVsSprite[id][0], roi );

            break;
        }
        case ICN::B_BFLG32:
        case ICN::G_BFLG32:
        case ICN::R_BFLG32:
        case ICN::Y_BFLG32:
        case ICN::O_BFLG32:
        case ICN::P_BFLG32:
            if ( _icnVsSprite[id].size() > 31 && _icnVsSprite[id][31].height() == 248 ) {
                fheroes2::Sprite & original = _icnVsSprite[id][31];
                fheroes2::Sprite temp = Crop( original, 0, 0, original.width(), 4 );
                temp.setPosition( original.x(), original.y() );

                original = std::move( temp );
            }
            break;
        case ICN::FLAG32: {
            auto & flagImages = _icnVsSprite[id];
            if ( flagImages.size() == 49 ) {
                // Shift and crop the Lighthouse flags to properly render them on the Adventure map.
                flagImages.resize( 49 + 7 * 2 );
                for ( size_t i = 42; i < 42 + 7; ++i ) {
                    const fheroes2::Sprite & original = flagImages[i];

                    flagImages[i + 7] = Crop( original, 0, 0, -original.x(), original.height() );
                    flagImages[i + 7].setPosition( 32 + original.x(), original.y() );

                    flagImages[i + 7 + 7] = Crop( original, -original.x(), 0, original.width(), original.height() );
                    flagImages[i + 7 + 7].setPosition( 0, original.y() );
                }
            }
            break;
        }
        case ICN::SHADOW32:
            // The shadow sprite of hero needs to be shifted to match the hero sprite.
            if ( _icnVsSprite[id].size() == 86 ) {
                // Direction: TOP (0-8), TOP_RIGHT (9-17), RIGHT (18-26), BOTTOM_RIGHT (27-35), BOTTOM (36-44)
                for ( int32_t i = 0; i < 45; ++i ) {
                    fheroes2::Sprite & original = _icnVsSprite[id][i];
                    original.setPosition( original.x(), original.y() - 3 );
                }

                // Direction:TOP_LEFT
                for ( int32_t i = 59; i < 68; ++i ) {
                    fheroes2::Sprite & original = _icnVsSprite[id][i];
                    original.setPosition( original.x() + 1, original.y() - 3 );
                }

                // Direction:LEFT
                for ( int32_t i = 68; i < 77; ++i ) {
                    fheroes2::Sprite & original = _icnVsSprite[id][i];
                    original.setPosition( original.x() - 5, original.y() - 3 );
                }

                // Direction:BOTTOM_LEFT
                for ( int32_t i = 77; i < 86; ++i ) {
                    fheroes2::Sprite & original = _icnVsSprite[id][i];
                    if ( i == 80 ) {
                        // This sprite needs extra fix.
                        original.setPosition( original.x() - 5, original.y() - 3 );
                    }
                    else {
                        original.setPosition( original.x() - 10, original.y() - 3 );
                    }
                }
            }
            break;
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

            for ( fheroes2::Sprite & image : _icnVsSprite[ICN::MINI_MONSTER_IMAGE] ) {
                uint8_t * transform = image.transform();
                const uint8_t * transformEnd = transform + image.width() * image.height();
                for ( ; transform != transformEnd; ++transform ) {
                    if ( *transform > 1 ) {
                        *transform = 1;
                    }
                }
            }

            for ( fheroes2::Sprite & image : _icnVsSprite[ICN::MINI_MONSTER_SHADOW] ) {
                uint8_t * transform = image.transform();
                const uint8_t * transformEnd = transform + image.width() * image.height();
                for ( ; transform != transformEnd; ++transform ) {
                    if ( *transform == 0 ) {
                        *transform = 1;
                    }
                }
            }

            break;
        }
        case ICN::BUTTON_GOOD_FONT_RELEASED:
        case ICN::BUTTON_GOOD_FONT_PRESSED:
        case ICN::BUTTON_EVIL_FONT_RELEASED:
        case ICN::BUTTON_EVIL_FONT_PRESSED: {
            generateBaseButtonFont( _icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], _icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED],
                                    _icnVsSprite[ICN::BUTTON_EVIL_FONT_RELEASED], _icnVsSprite[ICN::BUTTON_EVIL_FONT_PRESSED] );
            break;
        }
        case ICN::HISCORE: {
            if ( _icnVsSprite[id].size() == 9 ) {
                // Campaign title bar needs to include rating.
                const int32_t imageHeight = _icnVsSprite[id][7].height();
                const fheroes2::Sprite temp = Crop( _icnVsSprite[id][7], 215, 0, 300, imageHeight );

                Copy( temp, 0, 0, _icnVsSprite[id][7], 215 - 57, 0, temp.width(), imageHeight );
                Copy( _icnVsSprite[id][6], 324, 0, _icnVsSprite[id][7], 324, 0, _icnVsSprite[id][6].width() - 324, imageHeight );
            }
            break;
        }
        case ICN::SPELLINL: {
            if ( _icnVsSprite[id].size() > 11 ) {
                // Replace petrification spell mini-icon.
                fheroes2::h2d::readImage( "petrification_spell_icon_mini.image", _icnVsSprite[id][11] );
            }

            break;
        }
        case ICN::EMPTY_GOOD_BUTTON:
        case ICN::EMPTY_EVIL_BUTTON: {
            const bool isGoodInterface = ( id == ICN::EMPTY_GOOD_BUTTON );
            const int32_t originalId = isGoodInterface ? ICN::SYSTEM : ICN::SYSTEME;
            loadICN( originalId );

            if ( _icnVsSprite[originalId].size() < 13 ) {
                break;
            }

            _icnVsSprite[id].resize( 2 );

            fheroes2::Sprite & released = _icnVsSprite[id][0];
            fheroes2::Sprite & pressed = _icnVsSprite[id][1];

            released = _icnVsSprite[originalId][11];

            Fill( released, 8, 7, 1, 1, getButtonFillingColor( true, isGoodInterface ) );

            const fheroes2::Sprite & originalPressed = fheroes2::AGG::GetICN( originalId, 12 );

            if ( originalPressed.width() > 2 && originalPressed.height() > 2 ) {
                pressed.resize( originalPressed.width(), originalPressed.height() );
                // Copy the original pressed button but add the missing darker left side border from the released state
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

            Fill( pressed, 90, 5, 1, 1, getButtonFillingColor( false, isGoodInterface ) );

            break;
        }
        case ICN::EMPTY_POL_BUTTON: {
            const int originalID = ICN::X_CMPBTN;
            loadICN( originalID );

            if ( _icnVsSprite[originalID].size() < 8 ) {
                break;
            }

            _icnVsSprite[id].resize( 2 );
            // Move dark border to new released state from original pressed state button
            const fheroes2::Sprite & originalReleased = fheroes2::AGG::GetICN( originalID, 4 );
            const fheroes2::Sprite & originalPressed = fheroes2::AGG::GetICN( originalID, 5 );
            if ( originalReleased.width() != 94 && originalPressed.width() != 94 && originalReleased.height() < 5 && originalPressed.height() < 5 ) {
                break;
            }
            fheroes2::Sprite & releasedWithDarkBorder = _icnVsSprite[id][0];
            releasedWithDarkBorder.resize( originalReleased.width() + 2, originalReleased.height() + 1 );
            releasedWithDarkBorder.reset();

            Copy( originalReleased, 0, 0, releasedWithDarkBorder, 1, 0, originalReleased.width(), originalReleased.height() );
            Copy( originalReleased, 0, 2, releasedWithDarkBorder, 1, 21, 1, 1 );
            Copy( originalReleased, 0, 2, releasedWithDarkBorder, 2, 22, 1, 1 );
            Copy( originalPressed, 0, 2, releasedWithDarkBorder, 0, 3, 1, 19 );
            Copy( originalPressed, 0, originalPressed.height() - 1, releasedWithDarkBorder, 0, originalPressed.height(), originalPressed.width(), 1 );
            Copy( originalPressed, 0, 2, releasedWithDarkBorder, 1, 22, 1, 1 );

            // Pressed state
            fheroes2::Sprite & pressed = _icnVsSprite[id][1];
            // Make sure the released and pressed states have the same size, because the original Czech's pressed state is 1 px less in height than the other versions'.
            pressed.resize( originalReleased.width() + 2, originalReleased.height() + 1 );
            pressed.reset();
            Copy( originalPressed, 0, 0, pressed, 0, 1, originalPressed.width(), originalPressed.height() );

            // This fills the 1 px vertical gap in the Czech pressed state button.
            if ( originalPressed.height() < 23 ) {
                Copy( originalPressed, 0, originalPressed.height() - 10, pressed, 0, pressed.height() - 10, originalPressed.width(), 10 );
            }

            // The empty buttons need to be widened by 1 px so that they can be evenly divided by 3 in resizeButton() in ui_tools.cpp.
            Copy( originalReleased, originalReleased.width() - 5, 0, releasedWithDarkBorder, releasedWithDarkBorder.width() - 5, 0, 5, originalReleased.height() );
            Copy( originalPressed, originalPressed.width() - 5, 0, pressed, pressed.width() - 6, 1, 5, originalReleased.height() );

            const int32_t pixelPosition = 4 * 94 + 6;
            Fill( releasedWithDarkBorder, 5, 3, 88, 18, originalReleased.image()[pixelPosition] );
            Fill( pressed, 4, 5, 87, 17, originalPressed.image()[pixelPosition] );

            break;
        }
        case ICN::EMPTY_GUILDWELL_BUTTON: {
            const int originalID = ICN::WELLXTRA;
            loadICN( originalID );

            if ( _icnVsSprite[originalID].size() < 3 ) {
                break;
            }
            _icnVsSprite[id].resize( 2 );

            for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                const fheroes2::Sprite & original = fheroes2::AGG::GetICN( originalID, 0 + i );

                fheroes2::Sprite & out = _icnVsSprite[id][i];
                // The empty button needs to shortened by 1 px so that when it is divided by 3 in resizeButton() in ui_tools.h it will give an integer result.
                out.resize( original.width() - 1, original.height() );

                Copy( original, 0, 0, out, 0, 0, original.width() - 4, original.height() );
                Copy( original, original.width() - 3, 0, out, original.width() - 4, 0, 3, original.height() );
                // We do some extra cleaning because some localized assets, like Russian, have texts which go outside of the normal button borders.
                Fill( out, 3 - i, 2 + i, 55, 14, getButtonFillingColor( i == 0 ) );
                const uint8_t borderColor = i == 0 ? 38 : 41;
                fheroes2::DrawLine( out, { 3 - i, 3 + i }, { 4 - i, 2 + i }, borderColor );
                fheroes2::SetPixel( out, 5 - 3 * i, 2 + i, borderColor );
                fheroes2::SetPixel( out, 57 - i, 2 + i, borderColor );
                fheroes2::DrawLine( out, { 26, 1 + i }, { 42, 1 + i }, borderColor );
                const uint8_t secondBorderColor = i == 0 ? 42 : 46;
                fheroes2::DrawLine( out, { 26, 0 + i }, { 42, 0 + i }, secondBorderColor );
            }

            break;
        }
        case ICN::EMPTY_VERTICAL_GOOD_BUTTON: {
            const int32_t originalId = ICN::HSBTNS;
            loadICN( originalId );

            if ( _icnVsSprite[originalId].size() < 9 ) {
                break;
            }

            _icnVsSprite[id].resize( 2 );
            const fheroes2::Sprite & originalReleased = fheroes2::AGG::GetICN( originalId, 2 );
            const fheroes2::Sprite & originalPressed = fheroes2::AGG::GetICN( originalId, 3 );

            fheroes2::Sprite & released = _icnVsSprite[id][0];
            fheroes2::Sprite & pressed = _icnVsSprite[id][1];

            if ( originalReleased.width() > 2 && originalReleased.height() > 2 && originalPressed.width() > 2 && originalPressed.height() > 2 ) {
                released.resize( originalReleased.width() + 1, originalReleased.height() );
                pressed.resize( originalPressed.width() + 1, originalPressed.height() );
                released.reset();
                pressed.reset();

                // Shorten the button by 1 pixel in the height so that it is evenly divided by 5 in resizeButton() in ui_button.cpp
                Copy( originalReleased, 0, 0, released, 1, 0, originalReleased.width(), originalReleased.height() - 7 );
                Copy( originalPressed, 0, 0, pressed, 1, 0, originalPressed.width(), originalPressed.height() - 7 );
                Copy( originalReleased, 0, originalReleased.height() - 7, released, 1, originalReleased.height() - 8, originalReleased.width(),
                      originalReleased.height() - 7 );
                Copy( originalPressed, 0, originalPressed.height() - 7, pressed, 1, originalPressed.height() - 8, originalPressed.width(), originalPressed.height() - 7 );

                FillTransform( released, 1, 4, 1, released.height() - 4, 1 );

                // Fix the carried over broken transform layer of the original vertical button that is being used.
                fheroes2::Image exitCommonMask = fheroes2::ExtractCommonPattern( { &released, &pressed } );
                // Fix wrong non-transparent pixels of the transform layer that ExtractCommonPattern() missed.
                FillTransform( exitCommonMask, 4, 2, 1, 114, 1 );
                FillTransform( exitCommonMask, 5, 115, 1, 2, 1 );
                FillTransform( exitCommonMask, 6, 116, 17, 1, 1 );
                FillTransform( exitCommonMask, exitCommonMask.width() - 4, 113, 1, 2, 1 );

                invertTransparency( exitCommonMask );
                // Make the extended width and height lines transparent.
                FillTransform( exitCommonMask, 0, 0, 1, exitCommonMask.height(), 1 );
                FillTransform( exitCommonMask, exitCommonMask.width() - 4, exitCommonMask.height() - 1, 4, 1, 1 );

                copyTransformLayer( exitCommonMask, released );
                copyTransformLayer( exitCommonMask, pressed );

                // Restore dark-brown lines on the left and bottom borders of the button backgrounds.
                const fheroes2::Sprite & originalDismiss = fheroes2::AGG::GetICN( ICN::HSBTNS, 0 );

                Copy( originalReleased, 0, 4, released, 1, 4, 1, originalReleased.height() - 4 );
                Copy( originalDismiss, 6, originalDismiss.height() - 7, released, 2, originalReleased.height() - 1, 22, 1 );
                Copy( originalPressed, 0, 4, pressed, 1, 4, 1, originalPressed.height() - 4 );
                Copy( originalDismiss, 6, originalDismiss.height() - 7, pressed, 2, originalPressed.height() - 1, 22, 1 );

                // Clean the button states' text areas.
                Fill( released, 6, 4, 18, 110, getButtonFillingColor( true ) );
                Fill( pressed, 5, 5, 18, 110, getButtonFillingColor( false ) );

                // Make the pressed background transparent by removing remaining red parts.
                FillTransform( pressed, 5, 0, 21, 1, 1 );
                FillTransform( pressed, pressed.width() - 3, 1, 2, 1, 1 );
                FillTransform( pressed, pressed.width() - 2, 2, 2, 1, 1 );
                FillTransform( pressed, pressed.width() - 1, 3, 1, originalPressed.height() - 5, 1 );
            }

            break;
        }
        case ICN::EMPTY_MAP_SELECT_BUTTON: {
            const int32_t originalId = ICN::NGEXTRA;
            loadICN( originalId );

            if ( _icnVsSprite[originalId].size() < 80 ) {
                break;
            }

            _icnVsSprite[id].resize( 2 );

            for ( int32_t i = 0; i < static_cast<int32_t>( _icnVsSprite[id].size() ); ++i ) {
                const fheroes2::Sprite & original = fheroes2::AGG::GetICN( originalId, 64 + i );

                fheroes2::Sprite & out = _icnVsSprite[id][i];
                // the empty button needs to widened by 1 px so that when it is divided by 3 in resizeButton() in ui_tools.h it will
                // give an integer result
                out.resize( original.width() + 1, original.height() );

                Copy( original, 0, 0, out, 0, 0, original.width() - 5, original.height() );
                Copy( original, original.width() - 6, 0, out, original.width() - 5, 0, 6, original.height() );

                Fill( out, 6 - i, 2 + 2 * i, 72, 15 - i, getButtonFillingColor( i == 0 ) );
            }

            break;
        }
        case ICN::EMPTY_INTERFACE_BUTTON_GOOD:
        case ICN::EMPTY_INTERFACE_BUTTON_EVIL: {
            const int originalId = ( id == ICN::EMPTY_INTERFACE_BUTTON_GOOD ) ? ICN::ADVBTNS : ICN::ADVEBTNS;
            loadICN( originalId );
            _icnVsSprite[id].resize( 2 );

            Copy( _icnVsSprite[originalId][2], _icnVsSprite[id][0] );
            Copy( _icnVsSprite[originalId][3], _icnVsSprite[id][1] );

            // Get the button's icon colors.
            const uint8_t backgroundReleasedColor = _icnVsSprite[originalId][2].image()[1 * 36 + 5];
            const uint8_t backgroundPressedColor = _icnVsSprite[originalId][3].image()[5 * 36 + 6];

            // Clean-up the buttons' background
            Fill( _icnVsSprite[id][0], 23, 5, 8, 5, backgroundReleasedColor );
            Fill( _icnVsSprite[id][0], 8, 10, 24, 8, backgroundReleasedColor );
            Fill( _icnVsSprite[id][0], 6, 18, 24, 10, backgroundReleasedColor );
            Fill( _icnVsSprite[id][1], 22, 6, 8, 5, backgroundPressedColor );
            Fill( _icnVsSprite[id][1], 7, 11, 24, 8, backgroundPressedColor );
            Fill( _icnVsSprite[id][1], 5, 19, 24, 10, backgroundPressedColor );

            break;
        }
        case ICN::BRCREST: {
            // First sprite in this ICN has incorrect transparent pixel at position 30x5.
            if ( !_icnVsSprite[id].empty() ) {
                fheroes2::Sprite & original = _icnVsSprite[id][0];
                if ( original.width() == 50 && original.height() == 47 ) {
                    original._disableTransformLayer();
                    original.image()[280] = 117;
                }
            }

            // An extra image for the neutral color (for Editor).
            if ( _icnVsSprite[id].size() == 7 ) {
                fheroes2::Sprite neutralShield( fheroes2::AGG::GetICN( ICN::SPELLS, 15 ) );
                if ( neutralShield.width() < 2 || neutralShield.height() < 2 ) {
                    // We can not make a new image if there is no original shield image.
                    break;
                }

                // Make original shield image contour transparent.
                ReplaceColorIdByTransformId( neutralShield, neutralShield.image()[1], 1U );

                fheroes2::Sprite neutralColorSprite( _icnVsSprite[id][0].width(), _icnVsSprite[id][0].height() );
                neutralColorSprite.reset();
                Blit( neutralShield, neutralColorSprite, 8, 4 );

                // Make the background.
                uint8_t * imageData = neutralColorSprite.image();
                const uint8_t * transformData = neutralColorSprite.transform();
                const int32_t imageWidth = neutralColorSprite.width();
                const int32_t imageHeight = neutralColorSprite.height();
                const int32_t imageSize = imageWidth * imageHeight;
                const int32_t startValueX = 12 * imageWidth;
                const int32_t startValueY = 12 * imageHeight;

                for ( int32_t y = 0; y < imageHeight; ++y ) {
                    const int32_t offsetY = y * imageWidth;
                    const int32_t offsetValueY = y * startValueX;
                    for ( int32_t x = 0; x < imageWidth; ++x ) {
                        if ( transformData[x + offsetY] == 0U ) {
                            // Skip pixels with image.
                            continue;
                        }

                        const uint8_t colorValue = static_cast<uint8_t>( 10 + ( offsetValueY + ( imageWidth - x ) * startValueY ) / imageSize + ( x + y ) % 2 );
                        imageData[x + offsetY] = ( ( imageWidth - x - 1 ) * imageHeight > offsetY ) ? colorValue : 44U - colorValue;
                    }
                }

                // Make all image non-transparent.
                neutralColorSprite._disableTransformLayer();
                // We add shadow twice to make it more dark.
                addGradientShadow( neutralShield, neutralColorSprite, { 8, 4 }, { -2, 5 } );
                addGradientShadow( neutralShield, neutralColorSprite, { 8, 4 }, { -2, 5 } );

                _icnVsSprite[id].push_back( std::move( neutralColorSprite ) );
            }
            break;
        }
        case ICN::CBKGWATR: {
            // Ship battlefield background has incorrect transparent pixel at position 125x36.
            if ( !_icnVsSprite[id].empty() ) {
                fheroes2::Sprite & original = _icnVsSprite[id][0];
                if ( original.width() == 640 && original.height() == 443 ) {
                    original._disableTransformLayer();
                    original.image()[23165] = 24;
                }
            }
            break;
        }
        case ICN::SWAPWIN:
        case ICN::WELLBKG: {
            // Hero Meeting dialog and Castle Well images can be used with disabled transform layer.
            if ( !_icnVsSprite[id].empty() ) {
                _icnVsSprite[id][0]._disableTransformLayer();
            }
            break;
        }
        case ICN::GAME_OPTION_ICON: {
            _icnVsSprite[id].resize( 2 );

            fheroes2::h2d::readImage( "hotkeys_icon.image", _icnVsSprite[id][0] );
            fheroes2::h2d::readImage( "graphics_icon.image", _icnVsSprite[id][1] );

            break;
        }
        case ICN::COVR0010:
        case ICN::COVR0011:
        case ICN::COVR0012: {
            // The original image contains some foreign pixels that do not belong to the image.
            if ( !_icnVsSprite[id].empty() ) {
                fheroes2::Sprite & sprite = _icnVsSprite[id][0];
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

            break;
        }
        case ICN::OBJNDSRT: {
            if ( _icnVsSprite[id].size() == 131 ) {
                _icnVsSprite[id].resize( 132 );
                fheroes2::h2d::readImage( "missing_sphinx_part.image", _icnVsSprite[id][131] );
            }

            break;
        }
        case ICN::OBJNGRAS: {
            if ( _icnVsSprite[id].size() == 151 ) {
                _icnVsSprite[id].resize( 155 );

                loadICN( ICN::OBJNSNOW );

                if ( _icnVsSprite[ICN::OBJNSNOW].size() > 210 ) {
                    fheroes2::Sprite temp;

                    fheroes2::h2d::readImage( "adventure-map-grass-cave-diff-01.image", temp );
                    _icnVsSprite[id][151] = _icnVsSprite[ICN::OBJNSNOW][2];
                    Blit( temp, _icnVsSprite[id][151] );

                    ReplaceColorIdByTransformId( _icnVsSprite[id][151], 255, 1 );

                    fheroes2::h2d::readImage( "adventure-map-grass-cave-diff-02.image", temp );
                    _icnVsSprite[id][152] = _icnVsSprite[ICN::OBJNSNOW][3];
                    Blit( temp, _icnVsSprite[id][152] );

                    ReplaceColorIdByTransformId( _icnVsSprite[id][152], 255, 1 );

                    fheroes2::h2d::readImage( "lean-to-diff-part1.image", temp );
                    _icnVsSprite[id][153] = _icnVsSprite[ICN::OBJNSNOW][12];
                    Blit( temp, _icnVsSprite[id][153] );

                    ReplaceColorIdByTransformId( _icnVsSprite[id][153], 253, 1 );
                    ReplaceColorIdByTransformId( _icnVsSprite[id][153], 254, 2 );
                    ReplaceColorIdByTransformId( _icnVsSprite[id][153], 255, 3 );

                    fheroes2::h2d::readImage( "lean-to-diff-part2.image", temp );
                    _icnVsSprite[id][154] = _icnVsSprite[ICN::OBJNSNOW][13];
                    Blit( temp, _icnVsSprite[id][154] );

                    ReplaceColorIdByTransformId( _icnVsSprite[id][154], 253, 1 );
                    ReplaceColorIdByTransformId( _icnVsSprite[id][154], 254, 2 );
                    ReplaceColorIdByTransformId( _icnVsSprite[id][154], 255, 3 );
                }
            }

            break;
        }
        case ICN::OBJNMUL2: {
            auto & images = _icnVsSprite[id];
            if ( images.size() == 218 ) {
                // Expand the existing set of Adventure Map objects:
                // - 2 extra River Delta objects. Each object has 7 image parts.
                // - 1 new Stone Liths with 3 image parts.
                // - 3 new variants of Observation Tower object. Each object has 2 image parts.
                // - 1 new "Black Cat" object that has: 1 main image + 6 animation images.
                // - 1 new "Barrel" object that has: 1 main (empty) image + 6 animation images.
                // In total, 8 new objects (37 new images).

                images.resize( 218 + ( 7 * 2 ) + 3 + ( 2 * 3 ) + ( 1 + 6 ) + ( 1 + 6 ) );

                // 2 River Deltas.
                for ( size_t i = 0; i < 14; ++i ) {
                    images[218 + i].resize( images[i].height(), images[i].width() );
                    fheroes2::Transpose( images[i], images[218 + i] );
                    images[218 + i].setPosition( images[i].y(), images[i].x() );
                }

                // 1 Stone Liths.
                fheroes2::Sprite temp;
                fheroes2::h2d::readImage( "circular_stone_liths_center.image", temp );

                images[232].resize( 32, 32 );
                images[232].reset();
                Copy( temp, 0, 0, images[232], 0, 0, temp.width(), temp.height() );
                Copy( images[116], 0, temp.height(), images[232], 0, temp.height(), images[116].width(), images[116].height() - temp.height() );

                fheroes2::h2d::readImage( "circular_stone_liths_left.image", images[233] );
                fheroes2::h2d::readImage( "circular_stone_liths_top.image", images[234] );

                // Generic Observation Tower.
                images[235] = images[201];
                fheroes2::h2d::readImage( "observation_tower_generic_bottom_part.image", temp );
                Blit( temp, 0, 0, images[235], 0, temp.y() - images[235].y(), temp.width(), temp.height() );

                // Desert Observation Tower.
                images[236] = images[201];
                fheroes2::h2d::readImage( "observation_tower_desert_bottom_part.image", temp );
                Blit( temp, 0, 0, images[236], 0, temp.y() - images[236].y(), temp.width(), temp.height() );

                fheroes2::h2d::readImage( "observation_tower_desert_right_part.image", images[237] );

                // Snow Observation Tower.
                images[238] = images[201];
                fheroes2::h2d::readImage( "observation_tower_snow_bottom_part.image", temp );
                Blit( temp, 0, 0, images[238], 0, temp.y() - images[238].y(), temp.width(), temp.height() );

                fheroes2::h2d::readImage( "observation_tower_snow_right_part.image", images[239] );

                images[240] = images[198];
                fheroes2::h2d::readImage( "observation_tower_snow_top_part.image", temp );
                Blit( temp, 0, 0, images[240], 0, 0, temp.width(), temp.height() );

                // Black Cat. Main object image.
                fheroes2::h2d::readImage( "black_cat.image", images[241] );

                // Black Cat. Tail and eyes animation.
                for ( size_t i = 0; i < 6; ++i ) {
                    fheroes2::h2d::readImage( "black_cat_animation_" + std::to_string( i ) + ".image", images[242 + i] );
                }

                // Barrel. Object has only animation images but for compatibility
                // we need to have the main image (images[248]) be empty for this object.
                for ( size_t i = 0; i < 6; ++i ) {
                    fheroes2::h2d::readImage( "barrel_animation_" + std::to_string( i ) + ".image", images[249 + i] );
                }
            }

            break;
        }
        case ICN::SCENIBKG_EVIL: {
            const int32_t originalId = ICN::SCENIBKG;
            loadICN( originalId );

            if ( _icnVsSprite[originalId].size() != 1 ) {
                break;
            }

            _icnVsSprite[id].resize( 1 );

            const auto & originalImage = _icnVsSprite[originalId][0];
            auto & outputImage = _icnVsSprite[id][0];

            outputImage = originalImage;
            convertToEvilInterface( outputImage, { 0, 0, outputImage.width(), outputImage.height() } );

            loadICN( ICN::METALLIC_BORDERED_TEXTBOX_EVIL );
            if ( _icnVsSprite[ICN::METALLIC_BORDERED_TEXTBOX_EVIL].empty() ) {
                break;
            }

            const auto & evilTextBox = _icnVsSprite[ICN::METALLIC_BORDERED_TEXTBOX_EVIL][0];

            // The original text area is shorter than one we are using so we need to make 2 image copy operations to compensate this.
            const int32_t textWidth = 361;
            fheroes2::Copy( evilTextBox, 0, 0, outputImage, 46, 23, textWidth / 2, evilTextBox.height() );
            fheroes2::Copy( evilTextBox, evilTextBox.width() - ( textWidth - textWidth / 2 ), 0, outputImage, 46 + textWidth / 2, 23, ( textWidth - textWidth / 2 ),
                            evilTextBox.height() );

            break;
        }
        case ICN::TWNZDOCK: {
            if ( _icnVsSprite[id].size() == 6 && _icnVsSprite[id][4].width() == 285 ) {
                // Fix dock sprite width: crop the extra transparent right part of the image.
                const int32_t newWidth = 184;
                fheroes2::Sprite & original = _icnVsSprite[id][4];

                fheroes2::Sprite fixed( newWidth, original.height(), original.x(), original.y() );
                fheroes2::Copy( original, 0, 0, fixed, 0, 0, newWidth, original.height() );
                original = std::move( fixed );
            }
            break;
        }
        case ICN::WIZARD_CASTLE_BAY: {
            const int docksIcnId = ICN::TWNZDOCK;
            loadICN( docksIcnId );
            auto & baySprites = _icnVsSprite[id];
            if ( _icnVsSprite[docksIcnId].size() == 6 ) {
                // Make sprites for Wizard castle bay by updating docks sprites.
                baySprites = _icnVsSprite[docksIcnId];

                fheroes2::Sprite temp;
                fheroes2::h2d::readImage( "wizard_bay_diff_to_twnzdock.image", temp );
                fheroes2::Blit( temp, 0, 0, baySprites[0], temp.x() - baySprites[0].x(), temp.y() - baySprites[0].y(), baySprites[0].width(), baySprites[0].height() );

                for ( size_t i = 1; i < 6; ++i ) {
                    fheroes2::h2d::readImage( "wizard_bay_diff_to_twnzdock_animation_" + std::to_string( i - 1 ) + ".image", temp );
                    fheroes2::Blit( temp, 0, 0, baySprites[i], temp.x() - baySprites[i].x(), temp.y() - baySprites[i].y(), baySprites[i].width(),
                                    baySprites[i].height() );
                }
            }
            break;
        }
        case ICN::BOOK: {
            if ( _icnVsSprite[id].size() == 7 ) {
                // Add straight spell book corner to indicate first and last page.
                fheroes2::Sprite & corner = _icnVsSprite[id].emplace_back();
                fheroes2::h2d::readImage( "book_corner.image", corner );
            }
            break;
        }
        case ICN::BUTTON_VIRTUAL_KEYBOARD_GOOD:
        case ICN::BUTTON_VIRTUAL_KEYBOARD_EVIL: {
            const bool isEvil = ( id == ICN::BUTTON_VIRTUAL_KEYBOARD_EVIL );
            const int emptyButtonIcn = isEvil ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON;
            loadICN( emptyButtonIcn );
            if ( _icnVsSprite[emptyButtonIcn].size() == 2 ) {
                // There will be three sprites for the Virtual Keyboard button:
                // 0 - released;
                // 1 - pressed on "standard" background;
                // 2 - pressed on "uniform" background.
                _icnVsSprite[id].reserve( 3 );

                // Read keyboard images and put them on the button sprites.
                fheroes2::Sprite temp;
                const char * const releasedImageName = ( isEvil ? "keyboard_button_released_evil.image" : "keyboard_button_released_good.image" );
                fheroes2::h2d::readImage( releasedImageName, temp );

                // Generate empty buttons.
                fheroes2::Sprite & released = _icnVsSprite[id].emplace_back();
                fheroes2::Sprite & pressed = _icnVsSprite[id].emplace_back();

                fheroes2::Point pressedOffset;
                fheroes2::Point releasedOffset;

                constexpr int32_t extraWidth = 19;

                fheroes2::getCustomNormalButton( released, pressed, isEvil, { temp.width() + extraWidth, temp.height() }, releasedOffset, pressedOffset, ICN::UNKNOWN );

                // We subtract 1 from `releasedOffset.y` argument because the  loaded released image from `h2d` file is shifted 1 pixel down.
                fheroes2::Blit( temp, 0, 0, released, extraWidth / 2 + releasedOffset.x, releasedOffset.y - 1, temp.width(), temp.height() );

                const char * const pressedImageName = ( isEvil ? "keyboard_button_pressed_evil.image" : "keyboard_button_pressed_good.image" );
                fheroes2::h2d::readImage( pressedImageName, temp );
                fheroes2::Blit( temp, 0, 0, pressed, extraWidth / 2 + pressedOffset.x, pressedOffset.y, temp.width(), temp.height() );

                // Make a button pressed sprite for the "uniform" dialog background.
                fheroes2::Sprite & pressedUniform = _icnVsSprite[id].emplace_back( pressed );
                const int uniformBackgroundIcn = isEvil ? ICN::UNIFORMBAK_EVIL : ICN::UNIFORMBAK_GOOD;
                loadICN( uniformBackgroundIcn );
                fheroes2::makeTransparentBackground( released, pressedUniform, uniformBackgroundIcn );

                // Make a button pressed sprite for the "standard" dialog background.
                const int standardDialogBackgroundIcn = isEvil ? ICN::STONEBAK_EVIL : ICN::STONEBAK;
                loadICN( standardDialogBackgroundIcn );
                fheroes2::makeTransparentBackground( released, pressed, standardDialogBackgroundIcn );
            }
            break;
        }
        case ICN::SWORDSM2:
        case ICN::SWORDSMN: {
            if ( _icnVsSprite[id].size() == 45 && _icnVsSprite[id][3].width() == 38 && _icnVsSprite[id][3].height() > 69 ) {
                // Add extra frame to make the movement animation more smooth.
                // It is based on the frame 3 with updated legs drawing.
                fheroes2::Sprite & newFrame = _icnVsSprite[id].emplace_back( _icnVsSprite[id][3] );

                fheroes2::Sprite temp;
                fheroes2::h2d::readImage( "swordsman_walking_frame_extra_part_mask.image", temp );
                fheroes2::copyTransformLayer( temp, 0, 0, newFrame, temp.x(), newFrame.height() - temp.height(), temp.width(), temp.height() );

                fheroes2::h2d::readImage( "swordsman_walking_frame_extra_part.image", temp );
                fheroes2::Blit( temp, 0, 0, newFrame, temp.x(), newFrame.height() - temp.height(), temp.width(), temp.height() );

                newFrame.setPosition( newFrame.x() + 6, newFrame.y() );
            }
            break;
        }

        default:
            break;
        }
    }

    void loadICN( const int id )
    {
        if ( !_icnVsSprite[id].empty() ) {
            // The images have been loaded.
            return;
        }

        // Some images contain text. This text should be adapted to a chosen language.
        if ( isLanguageDependentIcnId( id ) ) {
            generateLanguageSpecificImages( id );
            return;
        }

        // Load the original ICN from AGG file.
        // WARNING: The `readIcnFromAgg()` function must be called only in this place!
        if ( id < ICN::LAST_VALID_FILE_ICN && !readIcnFromAgg( id ) ) {
            // The ICN `id` is not present in AGG file. In example, if you try to load PoL ICN from SW AGG file.
            // In order to avoid subsequent attempts to get resources from this ICN we are making it as non-empty.
            _icnVsSprite[id].resize( 1 );
            return;
        }

        // WARNING: The `processICN()` function must be called only in this place!
        processICN( id );

        if ( _icnVsSprite[id].empty() ) {
            // This could happen by one reason: asking to render an ICN that simply doesn't exist within the resources.
            // In order to avoid subsequent attempts to get resources from this ICN we are making it as non-empty.
            _icnVsSprite[id].resize( 1 );
        }
    }

    size_t GetMaximumICNIndex( int id )
    {
        loadICN( id );

        return _icnVsSprite[id].size();
    }

    size_t GetMaximumTILIndex( const int id )
    {
        auto & tilImages = _tilVsImage[id];

        if ( tilImages.empty() ) {
            tilImages.resize( 4 ); // 4 possible sides

            const std::vector<uint8_t> & data = ::AGG::getDataFromAggFile( tilFileName[id], false );
            if ( data.size() < headerSize ) {
                // The important resource is absent! Make sure that you are using the correct version of the game.
                assert( 0 );
                return 0;
            }

            ROStreamBuf buffer( data );

            const size_t count = buffer.getLE16();
            const int32_t width = buffer.getLE16();
            const int32_t height = buffer.getLE16();
            if ( count < 1 || width < 1 || height < 1 || ( headerSize + count * width * height ) != data.size() ) {
                return 0;
            }

            std::vector<fheroes2::Image> & originalTIL = tilImages[0];
            decodeTILImages( data.data() + headerSize, count, width, height, originalTIL );

            for ( uint32_t shapeId = 1; shapeId < 4; ++shapeId ) {
                tilImages[shapeId].resize( count );
            }

            for ( size_t i = 0; i < count; ++i ) {
                for ( uint32_t shapeId = 1; shapeId < 4; ++shapeId ) {
                    fheroes2::Image & image = tilImages[shapeId][i];

                    const bool horizontalFlip = ( shapeId & 2 ) != 0;
                    const bool verticalFlip = ( shapeId & 1 ) != 0;

                    image._disableTransformLayer();
                    image.resize( width, height );

                    Flip( originalTIL[i], 0, 0, image, 0, 0, width, height, horizontalFlip, verticalFlip );
                }
            }
        }

        return tilImages[0].size();
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

    const fheroes2::Sprite & GetScaledICN( const int icnId, const uint32_t index )
    {
        const fheroes2::Sprite & originalIcn = _icnVsSprite[icnId][index];
        const fheroes2::Display & display = fheroes2::Display::instance();

        if ( display.width() == fheroes2::Display::DEFAULT_WIDTH && display.height() == fheroes2::Display::DEFAULT_HEIGHT ) {
            return originalIcn;
        }

        if ( _icnVsScaledSprite[icnId].empty() ) {
            _icnVsScaledSprite[icnId].resize( _icnVsSprite[icnId].size() );
        }

        fheroes2::Sprite & resizedIcn = _icnVsScaledSprite[icnId][index];

        if ( originalIcn.singleLayer() && !resizedIcn.singleLayer() ) {
            resizedIcn._disableTransformLayer();
        }

        const double scaleFactorX = static_cast<double>( display.width() ) / fheroes2::Display::DEFAULT_WIDTH;
        const double scaleFactorY = static_cast<double>( display.height() ) / fheroes2::Display::DEFAULT_HEIGHT;

        const double scaleFactor = std::min( scaleFactorX, scaleFactorY );
        const int32_t resizedWidth = static_cast<int32_t>( std::lround( originalIcn.width() * scaleFactor ) );
        const int32_t resizedHeight = static_cast<int32_t>( std::lround( originalIcn.height() * scaleFactor ) );
        const int32_t offsetX = static_cast<int32_t>( std::lround( display.width() - fheroes2::Display::DEFAULT_WIDTH * scaleFactor ) ) / 2;
        const int32_t offsetY = static_cast<int32_t>( std::lround( display.height() - fheroes2::Display::DEFAULT_HEIGHT * scaleFactor ) ) / 2;
        assert( offsetX >= 0 && offsetY >= 0 );

        // Resize only if needed
        if ( resizedIcn.height() != resizedHeight || resizedIcn.width() != resizedWidth ) {
            resizedIcn.resize( resizedWidth, resizedHeight );
            resizedIcn.setPosition( static_cast<int32_t>( std::lround( originalIcn.x() * scaleFactor ) ) + offsetX,
                                    static_cast<int32_t>( std::lround( originalIcn.y() * scaleFactor ) ) + offsetY );
            Resize( originalIcn, resizedIcn );
        }
        else {
            // No need to resize but we have to update the offset.
            resizedIcn.setPosition( static_cast<int32_t>( std::lround( originalIcn.x() * scaleFactor ) ) + offsetX,
                                    static_cast<int32_t>( std::lround( originalIcn.y() * scaleFactor ) ) + offsetY );
        }

        return resizedIcn;
    }
}

namespace fheroes2::AGG
{
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

    void updateLanguageDependentResources( const SupportedLanguage language, const bool loadOriginalAlphabet )
    {
        static bool areOriginalResourcesInUse = false;
        static CodePage currentCodePage{ CodePage::NONE };

        const bool loadOriginalResources = loadOriginalAlphabet || !isAlphabetSupported( language );

        if ( loadOriginalResources ) {
            if ( alphabetPreserver.isPreserved() ) {
                if ( areOriginalResourcesInUse ) {
                    // Since we are already using the original resources, we don't need to do anything else.
                    // This saves us a lot of time by not having to rebuild many of the images we use for fonts and buttons.
                    return;
                }

                alphabetPreserver.restore();
            }
            else {
                // This can happen when we try to change a language without loading assets.
                alphabetPreserver.preserve();
            }
        }
        else {
            if ( !areOriginalResourcesInUse && currentCodePage == getCodePage( language ) ) {
                // We are trying to load resources for the same code page. We don't need to redo the same work again.
                return;
            }

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

        currentCodePage = getCodePage( language );
        areOriginalResourcesInUse = loadOriginalResources;
    }
}
