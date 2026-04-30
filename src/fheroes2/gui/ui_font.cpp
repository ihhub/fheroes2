/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2026                                             *
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

#include "ui_font.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>

#include "game_language.h"
#include "icn.h"
#include "image.h"
#include "math_base.h"
#include "ui_language.h"

namespace
{
    constexpr size_t asciiCharSetSize{ 128 };
    constexpr size_t codePageExtraCharacterCount{ 128 };

    const uint8_t buttonGoodReleasedColor = 56;
    const uint8_t buttonGoodPressedColor = 62;
    const uint8_t buttonEvilReleasedColor = 30;
    const uint8_t buttonEvilPressedColor = 36;
    const uint8_t buttonContourColor = 10;
    const fheroes2::Point buttonFontOffset{ -1, 0 };

    // The widths must be the same as space width for respective fonts.
    // Refer to FontCharHandler::_getSpaceCharWidth() function.
    constexpr int32_t smallFontSpaceWidth{ 4 };
    constexpr int32_t normalFontSpaceWidth{ 6 };
    constexpr int32_t bigFontSpaceWidth{ 8 };

    void updateNormalFontLetterShadow( fheroes2::Image & letter )
    {
        fheroes2::updateShadow( letter, { -1, 2 }, 2, false );
    }

    void updateSmallFontLetterShadow( fheroes2::Image & letter )
    {
        fheroes2::updateShadow( letter, { -1, 1 }, 2, true );
    }

    void removeShadows( fheroes2::Image & image )
    {
        if ( image.empty() || image.singleLayer() ) {
            return;
        }

        uint8_t * data = image.transform();
        const uint8_t * dataEnd = data + image.width() * image.height();

        for ( ; data != dataEnd; ++data ) {
            if ( *data > 0 ) {
                *data = 1;
            }
        }
    }

    fheroes2::Sprite addContour( fheroes2::Sprite & input, const fheroes2::Point & contourOffset, const uint8_t colorId )
    {
        if ( input.empty() || input.singleLayer() || contourOffset.x > 0 || contourOffset.y < 0 || ( -contourOffset.x >= input.width() )
             || ( contourOffset.y >= input.height() ) ) {
            return input;
        }

        fheroes2::Sprite output = input;

        const int32_t imageWidth = output.width();

        const int32_t width = imageWidth + contourOffset.x;
        const int32_t height = output.height() - contourOffset.y;

        const int32_t offsetY = imageWidth * contourOffset.y;
        uint8_t * imageOutY = output.image() + offsetY;
        const uint8_t * transformInY = input.transform() - contourOffset.x;
        uint8_t * transformOutY = output.transform() + offsetY;
        const uint8_t * transformOutYEnd = transformOutY + imageWidth * height;

        for ( ; transformOutY != transformOutYEnd; transformInY += imageWidth, transformOutY += imageWidth, imageOutY += imageWidth ) {
            uint8_t * imageOutX = imageOutY;
            const uint8_t * transformInX = transformInY;
            uint8_t * transformOutX = transformOutY;
            const uint8_t * transformOutXEnd = transformOutX + width;

            for ( ; transformOutX != transformOutXEnd; ++transformInX, ++transformOutX, ++imageOutX ) {
                if ( *transformOutX == 1 && ( *transformInX == 0 || ( *( transformInX + contourOffset.x ) == 0 && *( transformInX + offsetY ) == 0 ) ) ) {
                    // When there are two pixels adjacent diagonally we create a contour pixel in the corner that is closer to the image.
                    // Doing so there will be no "empty" pixels between the image and its shadow (for 1 pixel offset case).
                    *imageOutX = colorId;
                    *transformOutX = 0;
                }
            }
        }

        return output;
    }

    void applyGoodButtonReleasedLetterEffects( fheroes2::Sprite & letter )
    {
        updateShadow( letter, { 1, -1 }, 2, true );
        updateShadow( letter, { 2, -2 }, 4, true );
        letter = addContour( letter, { -1, 1 }, buttonContourColor );
        updateShadow( letter, { -1, 1 }, 7, true );
    }

    void applyGoodButtonPressedLetterEffects( fheroes2::Sprite & letter )
    {
        ReplaceColorId( letter, buttonGoodReleasedColor, buttonGoodPressedColor );

        fheroes2::updateShadow( letter, { 1, -1 }, 2, true );
        fheroes2::updateShadow( letter, { -1, 1 }, 7, true );
        fheroes2::updateShadow( letter, { -2, 2 }, 8, true );
    }

    void applyEvilButtonReleasedLetterEffects( fheroes2::Sprite & letter )
    {
        ReplaceColorId( letter, buttonGoodReleasedColor, buttonEvilReleasedColor );
    }

    void applyEvilButtonPressedLetterEffects( fheroes2::Sprite & letter )
    {
        ReplaceColorId( letter, buttonGoodPressedColor, buttonEvilPressedColor );
    }

    void updateButtonFont( std::vector<fheroes2::Sprite> & goodReleased, std::vector<fheroes2::Sprite> & goodPressed, std::vector<fheroes2::Sprite> & evilReleased,
                           std::vector<fheroes2::Sprite> & evilPressed )
    {
        goodPressed.resize( goodReleased.size() );
        evilReleased.resize( goodReleased.size() );
        evilPressed.resize( goodReleased.size() );

        for ( size_t i = 0; i < goodReleased.size(); ++i ) {
            goodPressed[i] = goodReleased[i];

            // Apply special effects on good interface letters first.
            applyGoodButtonReleasedLetterEffects( goodReleased[i] );
            applyGoodButtonPressedLetterEffects( goodPressed[i] );

            evilReleased[i] = goodReleased[i];
            evilPressed[i] = goodPressed[i];

            applyEvilButtonReleasedLetterEffects( evilReleased[i] );
            applyEvilButtonPressedLetterEffects( evilPressed[i] );
        }
    }

    void resizeCodePage( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            std::vector<fheroes2::Sprite> & original = icnVsSprite[icnId];

            // Resize the font to ASCII first to remove any redundant non-ASCII characters.
            original.resize( asciiCharSetSize );

            // Add the rest of characters to have the full code page.
            const fheroes2::Sprite firstSprite{ original[0] };
            original.insert( original.end(), codePageExtraCharacterCount, firstSprite );

            // Any code page should have 256 characters only.
            assert( original.size() == codePageExtraCharacterCount + asciiCharSetSize );
            static_assert( asciiCharSetSize + codePageExtraCharacterCount == 256 );
        }
    }

    void generateCP1250Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        resizeCodePage( icnVsSprite );

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // Uppercase S with caron. Generate accent for further use.
            font[138].resize( font[83].width(), font[83].height() + 3 );
            font[138].reset();
            fheroes2::Copy( font[83], 0, 0, font[138], 0, 3, font[83].width(), font[83].height() );
            fheroes2::Copy( font[97], 1, 1, font[138], 4, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 1, font[138], 6, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[138], 4, 1, 1, 1 );
            fheroes2::Copy( font[97], 7, 1, font[138], 5, 1, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[138], 6, 1, 1, 1 );
            font[138].setPosition( font[83].x(), font[83].y() - 3 );
            updateNormalFontLetterShadow( font[138] );

            // Uppercase S with acute. Generate accent for further use.
            font[140].resize( font[83].width(), font[83].height() + 3 );
            font[140].reset();
            fheroes2::Copy( font[83], 0, 0, font[140], 0, 3, font[83].width(), font[83].height() );
            fheroes2::Copy( font[111], 2, 0, font[140], 4, 0, 3, 2 );
            fheroes2::FillTransform( font[140], 4, 0, 1, 1, 1 );
            fheroes2::FillTransform( font[140], 6, 1, 1, 1, 1 );
            font[140].setPosition( font[83].x(), font[83].y() - 3 );
            updateNormalFontLetterShadow( font[140] );

            // Uppercase T with caron
            font[141].resize( font[84].width(), font[84].height() + 3 );
            font[141].reset();
            fheroes2::Copy( font[84], 0, 0, font[141], 0, 3, font[84].width(), font[84].height() );
            fheroes2::Copy( font[138], 4, 0, font[141], 5, 0, 3, 2 );
            font[141].setPosition( font[84].x(), font[84].y() - 3 );
            updateNormalFontLetterShadow( font[141] );

            // Uppercase Z with caron
            font[142].resize( font[90].width(), font[90].height() + 3 );
            font[142].reset();
            fheroes2::Copy( font[90], 0, 0, font[142], 0, 3, font[90].width(), font[90].height() );
            fheroes2::Copy( font[138], 4, 0, font[142], 5, 0, 3, 2 );
            font[142].setPosition( font[90].x(), font[90].y() - 3 );
            updateNormalFontLetterShadow( font[142] );

            // Uppercase Z with acute
            font[143].resize( font[90].width(), font[90].height() + 3 );
            font[143].reset();
            fheroes2::Copy( font[90], 0, 0, font[143], 0, 3, font[90].width(), font[90].height() );
            fheroes2::Copy( font[140], 4, 0, font[143], 4, 0, 3, 2 );
            font[143].setPosition( font[90].x(), font[90].y() - 3 );
            updateNormalFontLetterShadow( font[143] );

            // Lowercase s with caron
            font[154].resize( font[115].width(), font[115].height() + 3 );
            font[154].reset();
            fheroes2::Copy( font[115], 0, 0, font[154], 0, 3, font[115].width(), font[115].height() );
            fheroes2::Copy( font[138], 4, 0, font[154], 3, 0, 3, 2 );
            font[154].setPosition( font[115].x(), font[115].y() - 3 );
            updateNormalFontLetterShadow( font[154] );

            // Lowercase s with acute
            font[156].resize( font[115].width(), font[115].height() + 3 );
            font[156].reset();
            fheroes2::Copy( font[115], 0, 0, font[156], 0, 3, font[115].width(), font[115].height() );
            fheroes2::Copy( font[140], 4, 0, font[156], 4, 0, 3, 2 );
            font[156].setPosition( font[115].x(), font[115].y() - 3 );
            updateNormalFontLetterShadow( font[156] );

            // Lowercase t with caron
            font[157].resize( font[116].width(), font[116].height() + 1 );
            font[157].reset();
            fheroes2::Copy( font[116], 0, 0, font[157], 0, 1, font[116].width(), font[116].height() );
            fheroes2::Copy( font[97], 1, 1, font[157], 5, 1, 1, 1 );
            fheroes2::Copy( font[97], 1, 1, font[157], 6, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[157], 4, 1, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[157], 5, 0, 1, 1 );
            font[157].setPosition( font[116].x(), font[116].y() - 1 );
            updateNormalFontLetterShadow( font[157] );

            // Lowercase z with caron
            font[158].resize( font[122].width(), font[122].height() + 3 );
            font[158].reset();
            fheroes2::Copy( font[122], 0, 0, font[158], 0, 3, font[122].width(), font[122].height() );
            fheroes2::Copy( font[138], 4, 0, font[158], 4, 0, 3, 2 );
            font[158].setPosition( font[122].x(), font[122].y() - 3 );
            updateNormalFontLetterShadow( font[158] );

            // Lowercase z with acute
            font[159].resize( font[122].width(), font[122].height() + 3 );
            font[159].reset();
            fheroes2::Copy( font[122], 0, 0, font[159], 0, 3, font[122].width(), font[122].height() );
            fheroes2::Copy( font[140], 4, 0, font[159], 4, 0, 3, 2 );
            font[159].setPosition( font[122].x(), font[122].y() - 3 );
            updateNormalFontLetterShadow( font[159] );

            // NBSP character.
            font[160].resize( normalFontSpaceWidth, 1 );
            font[160].reset();

            // Uppercase L with stroke
            font[163].resize( font[76].width(), font[76].height() + 3 );
            font[163].reset();
            fheroes2::Copy( font[76], 0, 0, font[163], 0, 0, font[76].width(), font[76].height() );
            // Stroke diacritic.
            fheroes2::Copy( font[76], 1, 1, font[163], 6, 5, 1, 1 );
            fheroes2::Copy( font[76], 1, 1, font[163], 7, 4, 1, 1 );
            fheroes2::Copy( font[76], 1, 1, font[163], 8, 3, 1, 1 );
            fheroes2::Copy( font[76], 2, 1, font[163], 6, 6, 1, 1 );
            fheroes2::Copy( font[76], 2, 1, font[163], 7, 5, 1, 1 );
            fheroes2::Copy( font[76], 2, 1, font[163], 8, 4, 1, 1 );
            font[163].setPosition( font[76].x(), font[76].y() );
            updateNormalFontLetterShadow( font[163] );

            // Uppercase A with ogonek. Generate ogonek for further use.
            font[165].resize( font[65].width(), font[65].height() + 3 );
            font[165].reset();
            fheroes2::Copy( font[65], 0, 0, font[165], 0, 0, font[65].width(), font[65].height() );
            // Ogonek diacritic.
            fheroes2::Copy( font[97], 6, 5, font[165], 12, 11, 1, 1 );
            fheroes2::Copy( font[97], 6, 5, font[165], 11, 12, 1, 1 );
            fheroes2::Copy( font[97], 6, 5, font[165], 12, 13, 2, 1 );
            fheroes2::Copy( font[97], 6, 5, font[165], 14, 12, 1, 1 );
            fheroes2::Copy( font[97], 1, 3, font[165], 11, 11, 1, 1 );
            fheroes2::Copy( font[97], 1, 3, font[165], 12, 12, 1, 1 );
            fheroes2::Copy( font[97], 1, 3, font[165], 11, 13, 1, 1 );
            fheroes2::Copy( font[97], 1, 3, font[165], 14, 13, 1, 1 );
            font[165].setPosition( font[65].x(), font[65].y() );
            updateNormalFontLetterShadow( font[165] );
            // Remove unnecessary shadows
            fheroes2::FillTransform( font[165], 10, 13, 1, 1, 1 );
            fheroes2::FillTransform( font[165], 11, 14, 1, 1, 1 );
            fheroes2::FillTransform( font[165], 10, 15, 1, 1, 1 );
            fheroes2::FillTransform( font[165], 13, 15, 1, 1, 1 );

            // Uppercase Z with dot above. Generate dot for further use.
            font[175].resize( font[90].width(), font[90].height() + 3 );
            font[175].reset();
            fheroes2::Copy( font[90], 0, 0, font[175], 0, 3, font[90].width(), font[90].height() );
            fheroes2::Copy( font[122], 7, 5, font[175], 5, 0, 2, 1 );
            fheroes2::Copy( font[122], 6, 1, font[175], 5, 1, 2, 1 );
            font[175].setPosition( font[90].x(), font[90].y() - 3 );
            updateNormalFontLetterShadow( font[175] );

            // Lowercase l with stroke
            font[179].resize( font[108].width(), font[108].height() + 3 );
            font[179].reset();
            fheroes2::Copy( font[108], 0, 0, font[179], 0, 0, font[108].width(), font[108].height() );
            // Stroke diacritic.
            fheroes2::Copy( font[108], 3, 1, font[179], 4, 3, 1, 1 );
            fheroes2::Copy( font[108], 2, 1, font[179], 4, 4, 1, 1 );
            fheroes2::Copy( font[108], 3, 1, font[179], 2, 6, 1, 1 );
            fheroes2::Copy( font[108], 2, 1, font[179], 1, 6, 1, 1 );
            fheroes2::Copy( font[108], 3, 0, font[179], 2, 7, 1, 1 );
            fheroes2::Copy( font[108], 1, 1, font[179], 1, 7, 1, 1 );
            font[179].setPosition( font[108].x(), font[108].y() );
            updateNormalFontLetterShadow( font[179] );

            // Lowercase a with ogonek
            font[185].resize( font[97].width(), font[97].height() + 3 );
            font[185].reset();
            fheroes2::Copy( font[97], 0, 0, font[185], 0, 0, font[97].width(), font[97].height() );
            font[185].setPosition( font[97].x(), font[97].y() );
            updateNormalFontLetterShadow( font[185] );
            // Shadows are already made for the ogonek.
            fheroes2::Copy( font[165], 10, 11, font[185], 5, 7, 5, 5 );

            // Uppercase L with caron (NOT an uppercase Y with diaeresis)
            font[188].resize( font[76].width(), font[76].height() );
            font[188].reset();
            fheroes2::Copy( font[76], 0, 0, font[188], 0, 0, font[76].width(), font[76].height() );
            fheroes2::Copy( font[97], 1, 1, font[188], 9, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 1, font[188], 9, 1, 1, 1 );
            fheroes2::Copy( font[97], 1, 1, font[188], 10, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[188], 8, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[188], 8, 1, 1, 1 );
            font[188].setPosition( font[76].x(), font[76].y() );
            updateNormalFontLetterShadow( font[188] );

            // Lowercase l with caron (NOT an uppercase L with caron)
            font[190].resize( font[108].width() + 2, font[108].height() );
            font[190].reset();
            fheroes2::Copy( font[108], 0, 0, font[190], 0, 0, font[108].width(), font[108].height() );
            fheroes2::Copy( font[188], 8, 0, font[190], 4, 0, 3, 2 );
            font[190].setPosition( font[108].x(), font[108].y() );
            updateNormalFontLetterShadow( font[190] );

            // Lowercase z with dot above
            font[191].resize( font[122].width(), font[122].height() + 3 );
            font[191].reset();
            fheroes2::Copy( font[122], 0, 0, font[191], 0, 3, font[122].width(), font[122].height() );
            fheroes2::Copy( font[175], 5, 0, font[191], 4, 0, 2, 2 );
            font[191].setPosition( font[122].x(), font[122].y() - 3 );
            updateNormalFontLetterShadow( font[191] );

            // Uppercase R with acute
            font[192].resize( font[82].width(), font[82].height() + 3 );
            font[192].reset();
            fheroes2::Copy( font[82], 0, 0, font[192], 0, 3, font[82].width(), font[82].height() );
            fheroes2::Copy( font[140], 4, 0, font[192], 7, 0, 3, 2 );
            font[192].setPosition( font[82].x(), font[82].y() - 3 );
            updateNormalFontLetterShadow( font[192] );

            // Uppercase A with acute
            font[193].resize( font[65].width(), font[65].height() + 3 );
            font[193].reset();
            fheroes2::Copy( font[65], 0, 0, font[193], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[140], 4, 0, font[193], 7, 0, 3, 2 );
            font[193].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[193] );

            // Uppercase A with circumflex
            font[194].resize( font[65].width(), font[65].height() + 3 );
            font[194].reset();
            fheroes2::Copy( font[65], 0, 0, font[194], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[97], 7, 1, font[194], 7, 1, 1, 1 );
            fheroes2::Copy( font[97], 7, 1, font[194], 8, 0, 1, 1 );
            fheroes2::Copy( font[97], 7, 1, font[194], 9, 1, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[194], 7, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[194], 9, 0, 1, 1 );
            font[194].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[194] );

            // Uppercase A with diaeresis
            font[196].resize( font[65].width(), font[65].height() + 3 );
            font[196].reset();
            fheroes2::Copy( font[65], 0, 0, font[196], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[175], 5, 0, font[196], 5, 0, 2, 2 );
            fheroes2::Copy( font[175], 5, 0, font[196], 10, 0, 2, 2 );
            font[196].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[196] );

            // Uppercase L with acute
            font[197].resize( font[76].width(), font[76].height() + 3 );
            font[197].reset();
            fheroes2::Copy( font[76], 0, 0, font[197], 0, 3, font[76].width(), font[76].height() );
            fheroes2::Copy( font[140], 4, 0, font[197], 8, 0, 3, 2 );
            font[197].setPosition( font[76].x(), font[76].y() - 3 );
            updateNormalFontLetterShadow( font[197] );

            // Uppercase C with acute
            font[198].resize( font[67].width(), font[67].height() + 3 );
            font[198].reset();
            fheroes2::Copy( font[67], 0, 0, font[198], 0, 3, font[67].width(), font[67].height() );
            fheroes2::Copy( font[140], 4, 0, font[198], 8, 0, 3, 2 );
            font[198].setPosition( font[67].x(), font[67].y() - 3 );
            updateNormalFontLetterShadow( font[198] );

            // Uppercase C with caron
            font[200].resize( font[67].width(), font[67].height() + 3 );
            font[200].reset();
            fheroes2::Copy( font[67], 0, 0, font[200], 0, 3, font[67].width(), font[67].height() );
            fheroes2::Copy( font[138], 4, 0, font[200], 7, 0, 3, 2 );
            font[200].setPosition( font[67].x(), font[67].y() - 3 );
            updateNormalFontLetterShadow( font[200] );

            // Uppercase E with acute
            font[201].resize( font[69].width(), font[69].height() + 3 );
            font[201].reset();
            fheroes2::Copy( font[69], 0, 0, font[201], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[140], 4, 0, font[201], 6, 0, 3, 2 );
            font[201].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[201] );

            // Uppercase E with ogonek
            font[202].resize( font[69].width(), font[69].height() + 3 );
            font[202].reset();
            fheroes2::Copy( font[69], 0, 0, font[202], 0, 0, font[69].width(), font[69].height() );
            font[202].setPosition( font[69].x(), font[69].y() );
            updateNormalFontLetterShadow( font[202] );
            // Shadows are already made for the ogonek.
            fheroes2::Copy( font[165], 10, 11, font[202], 5, 11, 5, 5 );

            // Uppercase E with caron
            font[204].resize( font[69].width(), font[69].height() + 3 );
            font[204].reset();
            fheroes2::Copy( font[69], 0, 0, font[204], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[138], 4, 0, font[204], 5, 0, 3, 2 );
            font[204].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[204] );

            // Uppercase I with acute
            font[205].resize( font[73].width(), font[73].height() + 3 );
            font[205].reset();
            fheroes2::Copy( font[73], 0, 0, font[205], 0, 3, font[73].width(), font[73].height() );
            fheroes2::Copy( font[140], 4, 0, font[205], 4, 0, 3, 2 );
            font[205].setPosition( font[73].x(), font[73].y() - 3 );
            updateNormalFontLetterShadow( font[205] );

            // Uppercase D with caron
            font[207].resize( font[68].width(), font[68].height() + 3 );
            font[207].reset();
            fheroes2::Copy( font[68], 0, 0, font[207], 0, 3, font[68].width(), font[68].height() );
            fheroes2::Copy( font[138], 4, 0, font[207], 5, 0, 3, 2 );
            font[207].setPosition( font[68].x(), font[68].y() - 3 );
            updateNormalFontLetterShadow( font[207] );

            // Uppercase N with acute
            font[209].resize( font[78].width(), font[78].height() + 3 );
            font[209].reset();
            fheroes2::Copy( font[78], 0, 0, font[209], 0, 3, font[78].width(), font[78].height() );
            fheroes2::Copy( font[140], 4, 0, font[209], 8, 0, 3, 2 );
            font[209].setPosition( font[78].x(), font[78].y() - 3 );
            updateNormalFontLetterShadow( font[209] );

            // Uppercase N with caron
            font[210].resize( font[78].width(), font[78].height() + 3 );
            font[210].reset();
            fheroes2::Copy( font[78], 0, 0, font[210], 0, 3, font[78].width(), font[78].height() );
            fheroes2::Copy( font[138], 4, 0, font[210], 7, 0, 3, 2 );
            font[210].setPosition( font[78].x(), font[78].y() - 3 );
            updateNormalFontLetterShadow( font[210] );

            // Uppercase O with acute
            font[211].resize( font[79].width(), font[79].height() + 3 );
            font[211].reset();
            fheroes2::Copy( font[79], 0, 0, font[211], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[140], 4, 0, font[211], 8, 0, 3, 2 );
            font[211].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[211] );

            // Uppercase O with circumflex
            font[212].resize( font[79].width(), font[79].height() + 3 );
            font[212].reset();
            fheroes2::Copy( font[79], 0, 0, font[212], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[97], 1, 1, font[212], 6, 1, 1, 1 );
            fheroes2::Copy( font[97], 1, 1, font[212], 7, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 1, font[212], 8, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 1, font[212], 9, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 1, font[212], 10, 1, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[212], 6, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[212], 10, 0, 1, 1 );
            font[212].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[212] );

            // Uppercase O with double acute
            font[213].resize( font[79].width(), font[79].height() + 3 );
            font[213].reset();
            fheroes2::Copy( font[79], 0, 0, font[213], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[140], 4, 0, font[213], 5, 0, 3, 2 );
            fheroes2::Copy( font[140], 4, 0, font[213], 9, 0, 3, 2 );
            font[213].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[213] );

            // Uppercase O with diaeresis
            font[214].resize( font[79].width(), font[79].height() + 3 );
            font[214].reset();
            fheroes2::Copy( font[79], 0, 0, font[214], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[175], 5, 0, font[214], 5, 0, 2, 2 );
            fheroes2::Copy( font[175], 5, 0, font[214], 10, 0, 2, 2 );
            font[214].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[214] );

            // Uppercase R with caron
            font[216].resize( font[82].width(), font[82].height() + 3 );
            font[216].reset();
            fheroes2::Copy( font[82], 0, 0, font[216], 0, 3, font[82].width(), font[82].height() );
            fheroes2::Copy( font[138], 4, 0, font[216], 5, 0, 3, 2 );
            font[216].setPosition( font[82].x(), font[82].y() - 3 );
            updateNormalFontLetterShadow( font[216] );

            // Uppercase U with ring above
            font[217].resize( font[85].width(), font[85].height() + 4 );
            font[217].reset();
            fheroes2::Copy( font[85], 0, 0, font[217], 0, 4, font[85].width(), font[85].height() );
            fheroes2::Copy( font[112], 5, 6, font[217], 5, 0, 4, 1 );
            fheroes2::Copy( font[112], 5, 6, font[217], 5, 2, 4, 1 );
            fheroes2::Copy( font[116], 1, 0, font[217], 5, 1, 1, 1 );
            fheroes2::Copy( font[116], 1, 0, font[217], 8, 1, 1, 1 );
            font[217].setPosition( font[85].x(), font[85].y() - 4 );
            updateNormalFontLetterShadow( font[217] );

            // Uppercase U with acute
            font[218].resize( font[85].width(), font[85].height() + 3 );
            font[218].reset();
            fheroes2::Copy( font[85], 0, 0, font[218], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[140], 4, 0, font[218], 6, 0, 3, 2 );
            font[218].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[218] );

            // Uppercase U with double acute
            font[219].resize( font[85].width(), font[85].height() + 3 );
            font[219].reset();
            fheroes2::Copy( font[85], 0, 0, font[219], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[140], 4, 0, font[219], 4, 0, 3, 2 );
            fheroes2::Copy( font[140], 4, 0, font[219], 8, 0, 3, 2 );
            font[219].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[219] );

            // Uppercase U with diaeresis
            font[220].resize( font[85].width(), font[85].height() + 3 );
            font[220].reset();
            fheroes2::Copy( font[85], 0, 0, font[220], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[175], 5, 0, font[220], 4, 0, 2, 2 );
            fheroes2::Copy( font[175], 5, 0, font[220], 9, 0, 2, 2 );
            font[220].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[220] );

            // Uppercase Y with acute
            font[221].resize( font[89].width(), font[89].height() + 3 );
            font[221].reset();
            fheroes2::Copy( font[89], 0, 0, font[221], 0, 3, font[89].width(), font[89].height() );
            fheroes2::Copy( font[140], 4, 0, font[221], 7, 0, 3, 2 );
            font[221].setPosition( font[89].x(), font[89].y() - 3 );
            updateNormalFontLetterShadow( font[221] );

            // Lowercase r with acute
            font[224].resize( font[114].width(), font[114].height() + 3 );
            font[224].reset();
            fheroes2::Copy( font[114], 0, 0, font[224], 0, 3, font[114].width(), font[114].height() );
            fheroes2::Copy( font[140], 4, 0, font[224], 4, 0, 3, 2 );
            font[224].setPosition( font[114].x(), font[114].y() - 3 );
            updateNormalFontLetterShadow( font[224] );

            // Lowercase a with acute
            font[225].resize( font[97].width(), font[97].height() + 3 );
            font[225].reset();
            fheroes2::Copy( font[97], 0, 0, font[225], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[140], 4, 0, font[225], 3, 0, 3, 2 );
            font[225].setPosition( font[97].x(), font[97].y() - 3 );
            updateNormalFontLetterShadow( font[225] );

            // Lowercase a with circumflex
            font[226].resize( font[97].width(), font[97].height() + 3 );
            font[226].reset();
            fheroes2::Copy( font[97], 0, 0, font[226], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[194], 7, 0, font[226], 3, 0, 3, 2 );
            font[226].setPosition( font[97].x(), font[97].y() - 3 );
            updateNormalFontLetterShadow( font[226] );

            // Lowercase a with diaeresis
            font[228].resize( font[97].width(), font[97].height() + 3 );
            font[228].reset();
            fheroes2::Copy( font[97], 0, 0, font[228], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[175], 5, 0, font[228], 2, 0, 2, 2 );
            fheroes2::Copy( font[175], 5, 0, font[228], 6, 0, 2, 2 );
            font[228].setPosition( font[97].x(), font[97].y() - 3 );
            updateNormalFontLetterShadow( font[228] );

            // Lowercase l with acute
            font[229].resize( font[108].width(), font[108].height() + 3 );
            font[229].reset();
            fheroes2::Copy( font[108], 0, 0, font[229], 0, 3, font[108].width(), font[108].height() );
            fheroes2::Copy( font[140], 4, 0, font[229], 2, 0, 3, 2 );
            font[229].setPosition( font[108].x(), font[108].y() - 3 );
            updateNormalFontLetterShadow( font[229] );

            // Lowercase c with acute
            font[230].resize( font[99].width(), font[99].height() + 3 );
            font[230].reset();
            fheroes2::Copy( font[99], 0, 0, font[230], 0, 3, font[99].width(), font[99].height() );
            fheroes2::Copy( font[140], 4, 0, font[230], 4, 0, 3, 2 );
            font[230].setPosition( font[99].x(), font[99].y() - 3 );
            updateNormalFontLetterShadow( font[230] );

            // Lowercase c with caron
            font[232].resize( font[99].width(), font[99].height() + 3 );
            font[232].reset();
            fheroes2::Copy( font[99], 0, 0, font[232], 0, 3, font[99].width(), font[99].height() );
            fheroes2::Copy( font[138], 4, 0, font[232], 4, 0, 3, 2 );
            font[232].setPosition( font[99].x(), font[99].y() - 3 );
            updateNormalFontLetterShadow( font[232] );

            // Lowercase e with acute
            font[233].resize( font[101].width(), font[101].height() + 3 );
            font[233].reset();
            fheroes2::Copy( font[101], 0, 0, font[233], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[140], 4, 0, font[233], 4, 0, 3, 2 );
            font[233].setPosition( font[101].x(), font[101].y() - 3 );
            updateNormalFontLetterShadow( font[233] );

            // Lowercase e with ogonek
            font[234].resize( font[101].width(), font[101].height() + 3 );
            font[234].reset();
            fheroes2::Copy( font[101], 0, 0, font[234], 0, 0, font[101].width(), font[101].height() );
            font[234].setPosition( font[101].x(), font[101].y() );
            updateNormalFontLetterShadow( font[234] );
            // Shadows are already made for the ogonek.
            fheroes2::Copy( font[165], 10, 11, font[234], 3, 7, 5, 5 );

            // Lowercase e with caron
            font[236].resize( font[101].width(), font[101].height() + 3 );
            font[236].reset();
            fheroes2::Copy( font[101], 0, 0, font[236], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[138], 4, 0, font[236], 4, 0, 3, 2 );
            font[236].setPosition( font[101].x(), font[101].y() - 3 );
            updateNormalFontLetterShadow( font[236] );

            // Lowercase i with acute
            font[237].resize( font[105].width(), font[105].height() );
            font[237].reset();
            fheroes2::Copy( font[105], 0, 3, font[237], 0, 3, font[105].width(), font[105].height() );
            // Remove old dot shadow
            fheroes2::FillTransform( font[237], 0, 3, 1, 1, 1 );
            // Add acute accent
            fheroes2::Copy( font[140], 4, 0, font[237], 2, 0, 3, 2 );
            font[237].setPosition( font[105].x(), font[105].y() );
            updateNormalFontLetterShadow( font[237] );

            // Lowercase d with caron. Requires acute accent.
            font[239].resize( font[100].width() + 3, font[100].height() );
            font[239].reset();
            fheroes2::Copy( font[100], 0, 0, font[239], 0, 0, font[100].width(), font[100].height() );
            fheroes2::Copy( font[140], 4, 0, font[239], 10, 0, 3, 2 );
            font[239].setPosition( font[100].x(), font[100].y() );
            updateNormalFontLetterShadow( font[239] );

            // Lowercase n with acute
            font[241].resize( font[110].width(), font[110].height() + 3 );
            font[241].reset();
            fheroes2::Copy( font[110], 0, 0, font[241], 0, 3, font[110].width(), font[110].height() );
            fheroes2::Copy( font[140], 4, 0, font[241], 4, 0, 3, 2 );
            font[241].setPosition( font[110].x(), font[110].y() - 3 );
            updateNormalFontLetterShadow( font[241] );

            // Lowercase n with caron
            font[242].resize( font[110].width(), font[110].height() + 3 );
            font[242].reset();
            fheroes2::Copy( font[110], 0, 0, font[242], 0, 3, font[110].width(), font[110].height() );
            fheroes2::Copy( font[138], 4, 0, font[242], 4, 0, 3, 2 );
            font[242].setPosition( font[110].x(), font[110].y() - 3 );
            updateNormalFontLetterShadow( font[242] );

            // Lowercase o with acute
            font[243].resize( font[111].width(), font[111].height() + 3 );
            font[243].reset();
            fheroes2::Copy( font[111], 0, 0, font[243], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[140], 4, 0, font[243], 4, 0, 3, 2 );
            font[243].setPosition( font[111].x(), font[111].y() - 3 );
            updateNormalFontLetterShadow( font[243] );

            // Lowercase o with circumflex
            font[244].resize( font[111].width(), font[111].height() + 3 );
            font[244].reset();
            fheroes2::Copy( font[111], 0, 0, font[244], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[97], 1, 1, font[244], 3, 1, 1, 1 );
            fheroes2::Copy( font[97], 1, 1, font[244], 4, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 1, font[244], 5, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 1, font[244], 6, 1, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[244], 3, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[244], 6, 0, 1, 1 );
            font[244].setPosition( font[111].x(), font[111].y() - 3 );
            updateNormalFontLetterShadow( font[244] );

            // Lowercase o with double acute
            font[245].resize( font[111].width(), font[111].height() + 3 );
            font[245].reset();
            fheroes2::Copy( font[111], 0, 0, font[245], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[140], 4, 0, font[245], 2, 0, 3, 2 );
            fheroes2::Copy( font[140], 4, 0, font[245], 6, 0, 3, 2 );
            font[245].setPosition( font[111].x(), font[111].y() - 3 );
            updateNormalFontLetterShadow( font[245] );

            // Lowercase o with diaeresis
            font[246].resize( font[111].width(), font[111].height() + 3 );
            font[246].reset();
            fheroes2::Copy( font[111], 0, 0, font[246], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[175], 5, 0, font[246], 2, 0, 2, 2 );
            fheroes2::Copy( font[175], 5, 0, font[246], 6, 0, 2, 2 );
            font[246].setPosition( font[111].x(), font[111].y() - 3 );
            updateNormalFontLetterShadow( font[246] );

            // Lowercase r with caron
            font[248].resize( font[114].width(), font[114].height() + 3 );
            font[248].reset();
            fheroes2::Copy( font[114], 0, 0, font[248], 0, 3, font[114].width(), font[114].height() );
            fheroes2::Copy( font[138], 4, 0, font[248], 4, 0, 3, 2 );
            font[248].setPosition( font[114].x(), font[114].y() - 3 );
            updateNormalFontLetterShadow( font[248] );

            // Lowercase u with ring above
            font[249].resize( font[117].width(), font[117].height() + 4 );
            font[249].reset();
            fheroes2::Copy( font[117], 0, 0, font[249], 0, 4, font[117].width(), font[117].height() );
            fheroes2::Copy( font[217], 5, 0, font[249], 3, 0, 1, 3 );
            fheroes2::Copy( font[217], 8, 0, font[249], 7, 0, 1, 3 );
            fheroes2::Copy( font[97], 2, 0, font[249], 4, 0, 3, 1 );
            fheroes2::Copy( font[97], 2, 0, font[249], 4, 2, 3, 1 );
            fheroes2::Copy( font[101], 3, 2, font[249], 4, 1, 3, 1 );
            font[249].setPosition( font[117].x(), font[117].y() - 4 );
            updateNormalFontLetterShadow( font[249] );

            // Lowercase u with acute
            font[250].resize( font[117].width(), font[117].height() + 3 );
            font[250].reset();
            fheroes2::Copy( font[117], 0, 0, font[250], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[140], 4, 0, font[250], 4, 0, 3, 2 );
            font[250].setPosition( font[117].x(), font[117].y() - 3 );
            updateNormalFontLetterShadow( font[250] );

            // Lowercase u with double acute
            font[251].resize( font[117].width(), font[117].height() + 3 );
            font[251].reset();
            fheroes2::Copy( font[117], 0, 0, font[251], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[140], 4, 0, font[251], 2, 0, 3, 2 );
            fheroes2::Copy( font[140], 4, 0, font[251], 6, 0, 3, 2 );
            font[251].setPosition( font[117].x(), font[117].y() - 3 );
            updateNormalFontLetterShadow( font[251] );

            // Lowercase u with diaeresis
            font[252].resize( font[117].width(), font[117].height() + 3 );
            font[252].reset();
            fheroes2::Copy( font[117], 0, 0, font[252], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[175], 5, 0, font[252], 2, 0, 2, 2 );
            fheroes2::Copy( font[175], 5, 0, font[252], 6, 0, 2, 2 );
            font[252].setPosition( font[117].x(), font[117].y() - 3 );
            updateNormalFontLetterShadow( font[252] );

            // Lowercase y with acute
            font[253].resize( font[121].width(), font[121].height() + 3 );
            font[253].reset();
            fheroes2::Copy( font[121], 0, 0, font[253], 0, 3, font[121].width(), font[121].height() );
            fheroes2::Copy( font[140], 4, 0, font[253], 5, 0, 3, 2 );
            font[253].setPosition( font[121].x(), font[121].y() - 3 );
            updateNormalFontLetterShadow( font[253] );
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // Uppercase S with caron
            font[138].resize( font[83].width(), font[83].height() + 3 );
            font[138].reset();
            fheroes2::Copy( font[83], 0, 0, font[138], 0, 3, font[83].width(), font[83].height() );
            fheroes2::Copy( font[116], 2, 5, font[138], 3, 0, 3, 2 );
            font[138].setPosition( font[83].x(), font[83].y() - 3 );
            updateSmallFontLetterShadow( font[138] );

            // Uppercase S with acute
            font[140].resize( font[83].width(), font[83].height() + 3 );
            font[140].reset();
            fheroes2::Copy( font[83], 0, 0, font[140], 0, 3, font[83].width(), font[83].height() );
            fheroes2::Copy( font[122], 2, 2, font[140], 4, 0, 2, 2 );
            font[140].setPosition( font[83].x(), font[83].y() - 3 );
            updateSmallFontLetterShadow( font[140] );

            // Uppercase T with caron
            font[141].resize( font[84].width(), font[84].height() + 3 );
            font[141].reset();
            fheroes2::Copy( font[84], 0, 0, font[141], 0, 3, font[84].width(), font[84].height() );
            fheroes2::Copy( font[116], 2, 5, font[141], 3, 0, 3, 2 );
            font[141].setPosition( font[84].x(), font[84].y() - 3 );
            updateSmallFontLetterShadow( font[141] );

            // Uppercase Z with caron
            font[142].resize( font[90].width(), font[90].height() + 3 );
            font[142].reset();
            fheroes2::Copy( font[90], 0, 0, font[142], 0, 3, font[90].width(), font[90].height() );
            fheroes2::Copy( font[116], 2, 5, font[142], 3, 0, 3, 2 );
            font[142].setPosition( font[90].x(), font[90].y() - 3 );
            updateSmallFontLetterShadow( font[142] );

            // Uppercase Z with acute
            font[143].resize( font[90].width(), font[90].height() + 3 );
            font[143].reset();
            fheroes2::Copy( font[90], 0, 0, font[143], 0, 3, font[90].width(), font[90].height() );
            fheroes2::Copy( font[122], 2, 2, font[143], 4, 0, 2, 2 );
            font[143].setPosition( font[90].x(), font[90].y() - 3 );
            updateSmallFontLetterShadow( font[143] );

            // Lowercase s with caron
            font[154].resize( font[115].width(), font[115].height() + 3 );
            font[154].reset();
            fheroes2::Copy( font[115], 0, 0, font[154], 0, 3, font[115].width(), font[115].height() );
            fheroes2::Copy( font[116], 2, 5, font[154], 2, 0, 3, 2 );
            font[154].setPosition( font[115].x(), font[115].y() - 3 );
            updateSmallFontLetterShadow( font[154] );

            // Lowercase s with acute
            font[156].resize( font[115].width(), font[115].height() + 3 );
            font[156].reset();
            fheroes2::Copy( font[115], 0, 0, font[156], 0, 3, font[115].width(), font[115].height() );
            fheroes2::Copy( font[122], 2, 2, font[156], 3, 0, 2, 2 );
            font[156].setPosition( font[115].x(), font[115].y() - 3 );
            updateSmallFontLetterShadow( font[156] );

            // Lowercase t with caron
            font[157].resize( font[116].width(), font[116].height() + 1 );
            font[157].reset();
            fheroes2::Copy( font[116], 0, 0, font[157], 0, 1, font[116].width(), font[116].height() );
            fheroes2::Copy( font[97], 2, 0, font[157], 4, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[157], 4, 1, 1, 1 );
            font[157].setPosition( font[116].x(), font[116].y() - 1 );
            updateSmallFontLetterShadow( font[157] );

            // Lowercase z with caron
            font[158].resize( font[122].width(), font[122].height() + 3 );
            font[158].reset();
            fheroes2::Copy( font[122], 0, 0, font[158], 0, 3, font[122].width(), font[122].height() );
            fheroes2::Copy( font[116], 2, 5, font[158], 2, 0, 3, 2 );
            font[158].setPosition( font[122].x(), font[122].y() - 3 );
            updateSmallFontLetterShadow( font[158] );

            // Lowercase z with acute
            font[159].resize( font[122].width(), font[122].height() + 3 );
            font[159].reset();
            fheroes2::Copy( font[122], 0, 0, font[159], 0, 3, font[122].width(), font[122].height() );
            fheroes2::Copy( font[122], 2, 2, font[159], 3, 0, 2, 2 );
            font[159].setPosition( font[122].x(), font[122].y() - 3 );
            updateSmallFontLetterShadow( font[159] );

            // NBSP character.
            font[160].resize( smallFontSpaceWidth, 1 );
            font[160].reset();

            // Uppercase L with stroke
            font[163].resize( font[76].width(), font[76].height() );
            font[163].reset();
            fheroes2::Copy( font[76], 0, 0, font[163], 0, 0, font[76].width(), font[76].height() );
            fheroes2::Copy( font[122], 2, 2, font[163], 4, 2, 2, 2 );
            font[163].setPosition( font[76].x(), font[76].y() );
            updateSmallFontLetterShadow( font[163] );

            // Uppercase A with ogonek
            font[165].resize( font[65].width(), font[65].height() + 2 );
            font[165].reset();
            fheroes2::Copy( font[65], 0, 0, font[165], 0, 0, font[65].width(), font[65].height() );
            fheroes2::Copy( font[65], 7, 5, font[165], 7, 7, 2, 2 );
            font[165].setPosition( font[65].x(), font[65].y() );
            updateSmallFontLetterShadow( font[165] );

            // Uppercase Z with dot above
            font[175].resize( font[90].width(), font[90].height() + 2 );
            font[175].reset();
            fheroes2::Copy( font[90], 0, 0, font[175], 0, 2, font[90].width(), font[90].height() );
            fheroes2::Copy( font[90], 2, 0, font[175], 3, 0, 2, 1 );
            font[175].setPosition( font[90].x(), font[90].y() - 2 );
            updateSmallFontLetterShadow( font[175] );

            // Lowercase l with stroke
            font[179].resize( font[108].width(), font[108].height() );
            font[179].reset();
            fheroes2::Copy( font[108], 0, 0, font[179], 0, 0, font[108].width(), font[108].height() );
            fheroes2::Copy( font[122], 2, 3, font[179], 3, 2, 1, 1 );
            fheroes2::Copy( font[122], 2, 3, font[179], 1, 4, 1, 1 );
            font[179].setPosition( font[108].x(), font[108].y() );
            updateSmallFontLetterShadow( font[179] );

            // Lowercase a with ogonek
            font[185].resize( font[97].width(), font[97].height() + 2 );
            font[185].reset();
            fheroes2::Copy( font[97], 0, 0, font[185], 0, 0, font[97].width(), font[97].height() );
            fheroes2::Copy( font[65], 7, 5, font[185], 5, 5, 2, 2 );
            font[185].setPosition( font[97].x(), font[97].y() );
            updateSmallFontLetterShadow( font[185] );

            // Uppercase L with caron (NOT an uppercase Y with diaeresis)
            font[188].resize( font[76].width(), font[76].height() );
            font[188].reset();
            fheroes2::Copy( font[76], 0, 0, font[188], 0, 0, font[76].width(), font[76].height() );
            fheroes2::Copy( font[97], 2, 0, font[188], 6, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[188], 7, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[188], 6, 1, 1, 1 );
            font[188].setPosition( font[76].x(), font[76].y() );
            updateSmallFontLetterShadow( font[188] );

            // Lowercase l with caron (NOT an uppercase L with caron)
            font[190].resize( font[108].width() + 1, font[108].height() );
            font[190].reset();
            fheroes2::Copy( font[108], 0, 0, font[190], 0, 0, font[108].width(), font[108].height() );
            fheroes2::Copy( font[97], 2, 0, font[190], 4, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[190], 4, 1, 1, 1 );
            font[190].setPosition( font[108].x(), font[108].y() );
            updateSmallFontLetterShadow( font[190] );

            // Lowercase z with dot above
            font[191].resize( font[122].width(), font[122].height() + 2 );
            font[191].reset();
            fheroes2::Copy( font[122], 0, 0, font[191], 0, 2, font[122].width(), font[122].height() );
            fheroes2::Copy( font[90], 2, 0, font[191], 3, 0, 2, 1 );
            font[191].setPosition( font[122].x(), font[122].y() - 2 );
            updateSmallFontLetterShadow( font[191] );

            // Uppercase R with acute
            font[192].resize( font[82].width(), font[82].height() + 3 );
            font[192].reset();
            fheroes2::Copy( font[82], 0, 0, font[192], 0, 3, font[82].width(), font[82].height() );
            fheroes2::Copy( font[122], 2, 2, font[192], 5, 0, 2, 2 );
            font[192].setPosition( font[82].x(), font[82].y() - 3 );
            updateSmallFontLetterShadow( font[192] );

            // Uppercase A with acute
            font[193].resize( font[65].width(), font[65].height() + 3 );
            font[193].reset();
            fheroes2::Copy( font[65], 0, 0, font[193], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[122], 2, 2, font[193], 5, 0, 2, 2 );
            font[193].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[193] );

            // Uppercase A with circumflex
            font[194].resize( font[65].width(), font[65].height() + 3 );
            font[194].reset();
            fheroes2::Copy( font[65], 0, 0, font[194], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[65], 4, 0, font[194], 4, 1, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[194], 5, 0, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[194], 6, 1, 1, 1 );
            font[194].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[194] );

            // Uppercase A with diaeresis
            font[196].resize( font[65].width(), font[65].height() + 2 );
            font[196].reset();
            fheroes2::Copy( font[65], 0, 0, font[196], 0, 2, font[65].width(), font[65].height() );
            fheroes2::Copy( font[97], 2, 0, font[196], 3, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[196], 6, 0, 1, 1 );
            font[196].setPosition( font[65].x(), font[65].y() - 2 );
            updateSmallFontLetterShadow( font[196] );

            // Uppercase L with acute
            font[197].resize( font[76].width(), font[76].height() + 3 );
            font[197].reset();
            fheroes2::Copy( font[76], 0, 0, font[197], 0, 3, font[76].width(), font[76].height() );
            fheroes2::Copy( font[122], 2, 2, font[197], 5, 0, 2, 2 );
            font[197].setPosition( font[76].x(), font[76].y() - 3 );
            updateSmallFontLetterShadow( font[197] );

            // Uppercase C with acute
            font[198].resize( font[67].width(), font[67].height() + 3 );
            font[198].reset();
            fheroes2::Copy( font[67], 0, 0, font[198], 0, 3, font[67].width(), font[67].height() );
            fheroes2::Copy( font[122], 2, 2, font[198], 4, 0, 2, 2 );
            font[198].setPosition( font[67].x(), font[67].y() - 3 );
            updateSmallFontLetterShadow( font[198] );

            // Uppercase C with caron
            font[200].resize( font[67].width(), font[67].height() + 3 );
            font[200].reset();
            fheroes2::Copy( font[67], 0, 0, font[200], 0, 3, font[67].width(), font[67].height() );
            fheroes2::Copy( font[116], 2, 5, font[200], 3, 0, 3, 2 );
            font[200].setPosition( font[67].x(), font[67].y() - 3 );
            updateSmallFontLetterShadow( font[200] );

            // Uppercase E with acute
            font[201].resize( font[69].width(), font[69].height() + 3 );
            font[201].reset();
            fheroes2::Copy( font[69], 0, 0, font[201], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[122], 2, 2, font[201], 4, 0, 2, 2 );
            font[201].setPosition( font[69].x(), font[69].y() - 3 );
            updateSmallFontLetterShadow( font[201] );

            // Uppercase E with ogonek
            font[202].resize( font[69].width(), font[69].height() + 2 );
            font[202].reset();
            fheroes2::Copy( font[69], 0, 0, font[202], 0, 0, font[69].width(), font[69].height() );
            fheroes2::Copy( font[65], 7, 5, font[202], 5, 7, 2, 2 );
            font[202].setPosition( font[69].x(), font[69].y() );
            updateSmallFontLetterShadow( font[202] );

            // Uppercase E with caron
            font[204].resize( font[69].width(), font[69].height() + 3 );
            font[204].reset();
            fheroes2::Copy( font[69], 0, 0, font[204], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[116], 2, 5, font[204], 3, 0, 3, 2 );
            font[204].setPosition( font[69].x(), font[69].y() - 3 );
            updateSmallFontLetterShadow( font[204] );

            // Uppercase I with acute
            font[205].resize( font[73].width(), font[73].height() + 3 );
            font[205].reset();
            fheroes2::Copy( font[73], 0, 0, font[205], 0, 3, font[73].width(), font[73].height() );
            fheroes2::Copy( font[122], 2, 2, font[205], 2, 0, 2, 2 );
            font[205].setPosition( font[73].x(), font[73].y() - 3 );
            updateSmallFontLetterShadow( font[205] );

            // Uppercase D with caron
            font[207].resize( font[68].width(), font[68].height() + 3 );
            font[207].reset();
            fheroes2::Copy( font[68], 0, 0, font[207], 0, 3, font[68].width(), font[68].height() );
            fheroes2::Copy( font[116], 2, 5, font[207], 3, 0, 3, 2 );
            font[207].setPosition( font[68].x(), font[68].y() - 3 );
            updateSmallFontLetterShadow( font[207] );

            // Uppercase N with acute
            font[209].resize( font[78].width(), font[78].height() + 3 );
            font[209].reset();
            fheroes2::Copy( font[78], 0, 0, font[209], 0, 3, font[78].width(), font[78].height() );
            fheroes2::Copy( font[122], 2, 2, font[209], 5, 0, 2, 2 );
            font[209].setPosition( font[78].x(), font[78].y() - 3 );
            updateSmallFontLetterShadow( font[209] );

            // Uppercase N with caron
            font[210].resize( font[78].width(), font[78].height() + 3 );
            font[210].reset();
            fheroes2::Copy( font[78], 0, 0, font[210], 0, 3, font[78].width(), font[78].height() );
            fheroes2::Copy( font[116], 2, 5, font[210], 5, 0, 3, 2 );
            font[210].setPosition( font[78].x(), font[78].y() - 3 );
            updateSmallFontLetterShadow( font[210] );

            // Uppercase O with acute
            font[211].resize( font[79].width(), font[79].height() + 3 );
            font[211].reset();
            fheroes2::Copy( font[79], 0, 0, font[211], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[122], 2, 2, font[211], 4, 0, 2, 2 );
            font[211].setPosition( font[79].x(), font[79].y() - 3 );
            updateSmallFontLetterShadow( font[211] );

            // Uppercase O with double circumflex
            font[212].resize( font[79].width(), font[79].height() + 3 );
            font[212].reset();
            fheroes2::Copy( font[79], 0, 0, font[212], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[97], 2, 0, font[212], 2, 1, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[212], 3, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[212], 4, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[212], 5, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[212], 6, 1, 1, 1 );
            font[212].setPosition( font[79].x(), font[79].y() - 3 );
            updateSmallFontLetterShadow( font[212] );

            // Uppercase O with double acute
            font[213].resize( font[79].width(), font[79].height() + 3 );
            font[213].reset();
            fheroes2::Copy( font[79], 0, 0, font[213], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[122], 2, 2, font[213], 2, 0, 2, 2 );
            fheroes2::Copy( font[122], 2, 2, font[213], 5, 0, 2, 2 );
            font[213].setPosition( font[79].x(), font[79].y() - 3 );
            updateSmallFontLetterShadow( font[213] );

            // Uppercase O with diaeresis
            font[214].resize( font[79].width(), font[79].height() + 2 );
            font[214].reset();
            fheroes2::Copy( font[79], 0, 0, font[214], 0, 2, font[79].width(), font[79].height() );
            fheroes2::Copy( font[122], 3, 2, font[214], 2, 0, 1, 1 );
            fheroes2::Copy( font[122], 3, 2, font[214], 6, 0, 1, 1 );
            font[214].setPosition( font[79].x(), font[79].y() - 2 );
            updateSmallFontLetterShadow( font[214] );

            // Uppercase R with caron
            font[216].resize( font[82].width(), font[82].height() + 3 );
            font[216].reset();
            fheroes2::Copy( font[82], 0, 0, font[216], 0, 3, font[82].width(), font[82].height() );
            fheroes2::Copy( font[116], 2, 5, font[216], 4, 0, 3, 2 );
            font[216].setPosition( font[82].x(), font[82].y() - 3 );
            updateSmallFontLetterShadow( font[216] );

            // Uppercase U with ring above
            font[217].resize( font[85].width(), font[85].height() + 3 );
            font[217].reset();
            fheroes2::Copy( font[85], 0, 0, font[217], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[116], 2, 5, font[217], 5, 0, 1, 1 );
            fheroes2::Copy( font[116], 2, 5, font[217], 4, 1, 3, 2 );
            font[217].setPosition( font[85].x(), font[85].y() - 3 );
            updateSmallFontLetterShadow( font[217] );

            // Uppercase U with acute
            font[218].resize( font[85].width(), font[85].height() + 3 );
            font[218].reset();
            fheroes2::Copy( font[85], 0, 0, font[218], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[122], 2, 2, font[218], 5, 0, 2, 2 );
            font[218].setPosition( font[85].x(), font[85].y() - 3 );
            updateSmallFontLetterShadow( font[218] );

            // Uppercase U with double acute
            font[219].resize( font[85].width(), font[85].height() + 3 );
            font[219].reset();
            fheroes2::Copy( font[85], 0, 0, font[219], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[122], 2, 2, font[219], 3, 0, 2, 2 );
            fheroes2::Copy( font[122], 2, 2, font[219], 7, 0, 2, 2 );
            font[219].setPosition( font[85].x(), font[85].y() - 3 );
            updateSmallFontLetterShadow( font[219] );

            // Uppercase U with diaeresis
            font[220].resize( font[85].width(), font[85].height() + 2 );
            font[220].reset();
            fheroes2::Copy( font[85], 0, 0, font[220], 0, 2, font[85].width(), font[85].height() );
            fheroes2::Copy( font[122], 3, 2, font[220], 3, 0, 1, 1 );
            fheroes2::Copy( font[122], 3, 2, font[220], 7, 0, 1, 1 );
            font[220].setPosition( font[85].x(), font[85].y() - 2 );
            updateSmallFontLetterShadow( font[220] );

            // Uppercase Y with acute
            font[221].resize( font[89].width(), font[89].height() + 3 );
            font[221].reset();
            fheroes2::Copy( font[89], 0, 0, font[221], 0, 3, font[89].width(), font[89].height() );
            fheroes2::Copy( font[122], 2, 2, font[221], 5, 0, 2, 2 );
            font[221].setPosition( font[89].x(), font[89].y() - 3 );
            updateSmallFontLetterShadow( font[221] );

            // Lowercase r with acute
            font[224].resize( font[114].width(), font[114].height() + 3 );
            font[224].reset();
            fheroes2::Copy( font[114], 0, 0, font[224], 0, 3, font[114].width(), font[114].height() );
            fheroes2::Copy( font[122], 2, 2, font[224], 3, 0, 2, 2 );
            font[224].setPosition( font[114].x(), font[114].y() - 3 );
            updateSmallFontLetterShadow( font[224] );

            // Lowercase a with acute
            font[225].resize( font[97].width(), font[97].height() + 3 );
            font[225].reset();
            fheroes2::Copy( font[97], 0, 0, font[225], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[122], 2, 2, font[225], 3, 0, 2, 2 );
            font[225].setPosition( font[97].x(), font[97].y() - 3 );
            updateSmallFontLetterShadow( font[225] );

            // Lowercase a with circumflex
            font[226].resize( font[97].width(), font[97].height() + 3 );
            font[226].reset();
            fheroes2::Copy( font[97], 0, 0, font[226], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[97], 2, 0, font[226], 2, 1, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[226], 3, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[226], 4, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[226], 5, 1, 1, 1 );
            font[226].setPosition( font[97].x(), font[97].y() - 3 );
            updateSmallFontLetterShadow( font[226] );

            // Lowercase a with diaeresis
            font[228].resize( font[97].width(), font[97].height() + 2 );
            font[228].reset();
            fheroes2::Copy( font[97], 0, 0, font[228], 0, 2, font[97].width(), font[97].height() );
            fheroes2::Copy( font[97], 2, 0, font[228], 2, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[228], 5, 0, 1, 1 );
            font[228].setPosition( font[97].x(), font[97].y() - 2 );
            updateSmallFontLetterShadow( font[228] );

            // Lowercase l with acute
            font[229].resize( font[108].width(), font[108].height() + 3 );
            font[229].reset();
            fheroes2::Copy( font[108], 0, 0, font[229], 0, 3, font[108].width(), font[108].height() );
            fheroes2::Copy( font[122], 2, 2, font[229], 2, 0, 2, 2 );
            font[229].setPosition( font[108].x(), font[108].y() - 3 );
            updateSmallFontLetterShadow( font[229] );

            // Lowercase c with acute
            font[230].resize( font[99].width(), font[99].height() + 3 );
            font[230].reset();
            fheroes2::Copy( font[99], 0, 0, font[230], 0, 3, font[99].width(), font[99].height() );
            fheroes2::Copy( font[122], 2, 2, font[230], 3, 0, 2, 2 );
            font[230].setPosition( font[99].x(), font[99].y() - 3 );
            updateSmallFontLetterShadow( font[230] );

            // Lowercase c with caron
            font[232].resize( font[99].width(), font[99].height() + 3 );
            font[232].reset();
            fheroes2::Copy( font[99], 0, 0, font[232], 0, 3, font[99].width(), font[99].height() );
            fheroes2::Copy( font[116], 2, 5, font[232], 2, 0, 3, 2 );
            font[232].setPosition( font[99].x(), font[99].y() - 3 );
            updateSmallFontLetterShadow( font[232] );

            // Lowercase e with acute
            font[233].resize( font[101].width(), font[101].height() + 3 );
            font[233].reset();
            fheroes2::Copy( font[101], 0, 0, font[233], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[122], 2, 2, font[233], 3, 0, 2, 2 );
            font[233].setPosition( font[101].x(), font[101].y() - 3 );
            updateSmallFontLetterShadow( font[233] );

            // Lowercase e with ogonek
            font[234].resize( font[101].width(), font[101].height() + 2 );
            font[234].reset();
            fheroes2::Copy( font[101], 0, 0, font[234], 0, 0, font[101].width(), font[101].height() );
            fheroes2::Copy( font[65], 7, 5, font[234], 3, 5, 2, 2 );
            font[234].setPosition( font[101].x(), font[101].y() );
            updateSmallFontLetterShadow( font[234] );

            // Lowercase e with caron
            font[236].resize( font[101].width(), font[101].height() + 3 );
            font[236].reset();
            fheroes2::Copy( font[101], 0, 0, font[236], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[116], 2, 5, font[236], 2, 0, 3, 2 );
            font[236].setPosition( font[101].x(), font[101].y() - 3 );
            updateSmallFontLetterShadow( font[236] );

            // Lowercase i with acute
            font[237].resize( font[105].width(), font[105].height() + 1 );
            font[237].reset();
            fheroes2::Copy( font[105], 0, 0, font[237], 0, 1, font[105].width(), font[105].height() );
            fheroes2::Copy( font[122], 3, 2, font[237], 3, 0, 1, 1 );
            font[237].setPosition( font[105].x(), font[105].y() - 1 );
            updateSmallFontLetterShadow( font[237] );

            // Lowercase d with caron
            font[239].resize( font[100].width() + 2, font[100].height() );
            font[239].reset();
            fheroes2::Copy( font[100], 0, 0, font[239], 0, 0, font[100].width(), font[100].height() );
            fheroes2::Copy( font[122], 2, 2, font[239], 7, 0, 2, 2 );
            font[239].setPosition( font[100].x(), font[100].y() );
            updateSmallFontLetterShadow( font[239] );

            // Lowercase n with acute
            font[241].resize( font[110].width(), font[110].height() + 3 );
            font[241].reset();
            fheroes2::Copy( font[110], 0, 0, font[241], 0, 3, font[110].width(), font[110].height() );
            fheroes2::Copy( font[122], 2, 2, font[241], 4, 0, 2, 2 );
            font[241].setPosition( font[110].x(), font[110].y() - 3 );
            updateSmallFontLetterShadow( font[241] );

            // Lowercase n with caron
            font[242].resize( font[110].width(), font[110].height() + 3 );
            font[242].reset();
            fheroes2::Copy( font[110], 0, 0, font[242], 0, 3, font[110].width(), font[110].height() );
            fheroes2::Copy( font[116], 2, 5, font[242], 3, 0, 3, 2 );
            font[242].setPosition( font[110].x(), font[110].y() - 3 );
            updateSmallFontLetterShadow( font[242] );

            // Lowercase o with acute
            font[243].resize( font[111].width(), font[111].height() + 3 );
            font[243].reset();
            fheroes2::Copy( font[111], 0, 0, font[243], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[122], 2, 2, font[243], 3, 0, 2, 2 );
            font[243].setPosition( font[111].x(), font[111].y() - 3 );
            updateSmallFontLetterShadow( font[243] );

            // Lowercase o with circumflex
            font[244].resize( font[111].width() + 1, font[111].height() + 3 );
            font[244].reset();
            fheroes2::Copy( font[111], 0, 0, font[244], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[97], 2, 0, font[244], 2, 1, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[244], 3, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[244], 4, 1, 1, 1 );
            font[244].setPosition( font[111].x(), font[111].y() - 3 );
            updateSmallFontLetterShadow( font[244] );

            // Lowercase o with double acute
            font[245].resize( font[111].width() + 1, font[111].height() + 3 );
            font[245].reset();
            fheroes2::Copy( font[111], 0, 0, font[245], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[122], 2, 2, font[245], 2, 0, 2, 2 );
            fheroes2::Copy( font[122], 2, 2, font[245], 5, 0, 2, 2 );
            font[245].setPosition( font[111].x(), font[111].y() - 3 );
            updateSmallFontLetterShadow( font[245] );

            // Lowercase o with diaeresis
            font[246].resize( font[111].width(), font[111].height() + 2 );
            font[246].reset();
            fheroes2::Copy( font[111], 0, 0, font[246], 0, 2, font[111].width(), font[111].height() );
            fheroes2::Copy( font[122], 3, 2, font[246], 2, 0, 1, 1 );
            fheroes2::Copy( font[122], 3, 2, font[246], 4, 0, 1, 1 );
            font[246].setPosition( font[111].x(), font[111].y() - 2 );
            updateSmallFontLetterShadow( font[246] );

            // Lowercase r with caron
            font[248].resize( font[114].width(), font[114].height() + 3 );
            font[248].reset();
            fheroes2::Copy( font[114], 0, 0, font[248], 0, 3, font[114].width(), font[114].height() );
            fheroes2::Copy( font[116], 2, 5, font[248], 3, 0, 3, 2 );
            font[248].setPosition( font[114].x(), font[114].y() - 3 );
            updateSmallFontLetterShadow( font[248] );

            // Lowercase u with ring above
            font[249].resize( font[117].width(), font[117].height() + 4 );
            font[249].reset();
            fheroes2::Copy( font[117], 0, 0, font[249], 0, 4, font[117].width(), font[117].height() );
            fheroes2::Copy( font[116], 2, 5, font[249], 4, 0, 1, 1 );
            fheroes2::Copy( font[116], 2, 5, font[249], 3, 1, 3, 2 );
            font[249].setPosition( font[117].x(), font[117].y() - 4 );
            updateSmallFontLetterShadow( font[249] );

            // Lowercase u with acute
            font[250].resize( font[117].width(), font[117].height() + 3 );
            font[250].reset();
            fheroes2::Copy( font[117], 0, 0, font[250], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[122], 2, 2, font[250], 3, 0, 2, 2 );
            font[250].setPosition( font[117].x(), font[117].y() - 3 );
            updateSmallFontLetterShadow( font[250] );

            // Lowercase u with double acute
            font[251].resize( font[117].width(), font[117].height() + 3 );
            font[251].reset();
            fheroes2::Copy( font[117], 0, 0, font[251], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[122], 2, 2, font[251], 2, 0, 2, 2 );
            fheroes2::Copy( font[122], 2, 2, font[251], 5, 0, 2, 2 );
            font[251].setPosition( font[117].x(), font[117].y() - 3 );
            updateSmallFontLetterShadow( font[251] );

            // Lowercase u with diaeresis
            font[252].resize( font[117].width(), font[117].height() + 2 );
            font[252].reset();
            fheroes2::Copy( font[117], 0, 0, font[252], 0, 2, font[117].width(), font[117].height() );
            fheroes2::Copy( font[122], 3, 2, font[252], 2, 0, 1, 1 );
            fheroes2::Copy( font[122], 3, 2, font[252], 6, 0, 1, 1 );
            font[252].setPosition( font[117].x(), font[117].y() - 2 );
            updateSmallFontLetterShadow( font[252] );

            // Lowercase y with acute
            font[253].resize( font[121].width(), font[121].height() + 3 );
            font[253].reset();
            fheroes2::Copy( font[121], 0, 0, font[253], 0, 3, font[121].width(), font[121].height() );
            fheroes2::Copy( font[122], 2, 2, font[253], 4, 0, 2, 2 );
            font[253].setPosition( font[121].x(), font[121].y() - 3 );
            updateSmallFontLetterShadow( font[253] );
        }
    }

    // CP-1251 supports Russian, Ukrainian, Belarussian, Bulgarian, Serbian Cyrillic, Macedonian and English.
    void generateCP1251Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        // TODO: add support for Serbian Cyrillic and Macedonian languages by generating missing letters.

        resizeCodePage( icnVsSprite );

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            size_t offset = 0;

            // ' (right single quotation mark)
            font[146] = font[44];
            font[146].setPosition( font[146].x(), font[146].y() - 6 );

            // E with 2 dots on top.
            font[168].resize( font[69 + offset].width(), font[69 + offset].height() + 3 );
            font[168].reset();
            fheroes2::Copy( font[69 + offset], 0, 0, font[168], 0, 3, font[69 + offset].width(), font[69 + offset].height() );
            fheroes2::Copy( font[168], 5, 5, font[168], 4, 0, 1, 1 );
            fheroes2::Copy( font[168], 5, 5, font[168], 7, 0, 1, 1 );
            fheroes2::Copy( font[168], 4, 5, font[168], 4, 1, 1, 1 );
            fheroes2::Copy( font[168], 4, 5, font[168], 7, 1, 1, 1 );
            font[168].setPosition( font[69 + offset].x(), font[69 + offset].y() - 3 );
            updateNormalFontLetterShadow( font[168] );

            // NBSP character.
            font[160].resize( normalFontSpaceWidth, 1 );
            font[160].reset();

            font[161].resize( font[89 + offset].width(), font[89 + offset].height() + 3 );
            font[161].reset();
            fheroes2::Copy( font[89 + offset], 0, 0, font[161], 0, 3, font[89 + offset].width(), font[89 + offset].height() );
            fheroes2::Copy( font[168], 3, 0, font[161], 7, 0, 2, 3 );
            font[161].setPosition( font[89 + offset].x(), font[89 + offset].y() - 3 );
            updateNormalFontLetterShadow( font[161] );

            font[162].resize( font[121 + offset].width(), font[121 + offset].height() + 3 );
            font[162].reset();
            fheroes2::Copy( font[121 + offset], 0, 0, font[162], 0, 3, font[121 + offset].width(), font[121 + offset].height() );
            fheroes2::Copy( font[121 + offset], 4, 1, font[162], 6, 0, 1, 1 );
            fheroes2::Copy( font[121 + offset], 4, 0, font[162], 6, 1, 1, 1 );
            font[162].setPosition( font[121 + offset].x(), font[121 + offset].y() - 3 );
            updateNormalFontLetterShadow( font[162] );

            // C with a horizontal line in the middle.
            font[170] = font[67];
            fheroes2::Copy( font[170], 7, 0, font[170], 6, 5, 4, 2 );
            updateNormalFontLetterShadow( font[170] );

            font[186] = font[99];
            fheroes2::Copy( font[186], 4, 0, font[186], 3, 3, 3, 1 );
            updateNormalFontLetterShadow( font[186] );

            // I and i with 2 dots.
            font[175].resize( font[73].width(), font[73].height() + 3 );
            font[175].reset();
            fheroes2::Copy( font[73], 0, 0, font[175], 0, 3, font[73].width(), font[73].height() );
            fheroes2::Copy( font[168], 3, 0, font[175], 2, 0, 5, 3 );
            font[175].setPosition( font[73].x(), font[73].y() - 3 );

            font[191] = font[105];
            fheroes2::FillTransform( font[191], 2, 0, 1, 3, 1 );

            // J and j.
            font[163] = font[74];
            font[188] = font[106];

            // S and s.
            font[189] = font[83];
            font[190] = font[115];

            // I and i.
            font[178] = font[73];
            font[179] = font[105];

            // A
            font[192] = font[65];

            font[193] = font[66 + offset];
            fheroes2::FillTransform( font[193], 9, 4, 2, 1, 1 );
            fheroes2::Copy( font[70], 6, 0, font[193], 6, 0, 5, 4 );
            fheroes2::Copy( font[193], 9, 5, font[193], 8, 4, 1, 1 );
            updateNormalFontLetterShadow( font[193] );

            font[194] = font[66 + offset];

            font[195] = font[70];
            fheroes2::FillTransform( font[195], 6, 4, 3, 4, 1 );

            // The same letter as above but with a vertical line at the top.
            font[165].resize( font[195].width(), font[195].height() + 1 );
            font[165].reset();
            fheroes2::Copy( font[195], 0, 0, font[165], 0, 1, font[195].width(), font[195].height() );
            fheroes2::Copy( font[195], 9, 1, font[165], 9, 0, 2, 1 );
            fheroes2::Copy( font[195], 9, 1, font[165], 10, 1, 1, 1 );
            fheroes2::Copy( font[195], 10, 0, font[165], 10, 2, 1, 1 );
            fheroes2::Copy( font[195], 8, 1, font[165], 9, 2, 1, 1 );
            font[165].setPosition( font[195].x(), font[195].y() - 1 );

            font[196] = font[68 + offset];

            font[197] = font[69 + offset];

            // x with | in the middle.
            font[198].resize( font[88].width() + 1, font[88].height() );
            font[198].reset();
            fheroes2::Copy( font[88], 1, 0, font[198], 1, 0, 8, 11 );
            fheroes2::Copy( font[88], 9, 0, font[198], 10, 0, 6, 11 );
            fheroes2::Fill( font[198], 9, 1, 1, 9, font[198].image()[1 + font[198].width()] );
            font[198].setPosition( font[88].x(), font[88].y() );
            updateNormalFontLetterShadow( font[198] );

            font[199].resize( font[51].width() + 1, font[51].height() );
            font[199].reset();
            fheroes2::Copy( font[51], 1, 0, font[199], 1, 0, 5, 3 );
            fheroes2::Copy( font[51], 5, 0, font[199], 6, 0, 3, 4 );
            fheroes2::Copy( font[51], 3, 5, font[199], 4, 4, 5, 4 );
            fheroes2::Copy( font[51], 1, 8, font[199], 1, 8, 5, 3 );
            fheroes2::Copy( font[51], 5, 8, font[199], 6, 8, 3, 3 );
            fheroes2::FillTransform( font[199], 2, 6, 5, 3, 1 );
            font[199].setPosition( font[51].x(), font[51].y() );
            updateNormalFontLetterShadow( font[199] );

            // Reverted N.
            font[200] = font[78];
            fheroes2::FillTransform( font[200], 6, 1, 5, 11, 1 );
            fheroes2::Copy( font[78], 6, 2, font[200], 6, 6, 1, 3 );
            fheroes2::Copy( font[78], 7, 3, font[200], 7, 5, 1, 3 );
            fheroes2::Copy( font[78], 8, 4, font[200], 8, 4, 1, 3 );
            fheroes2::Copy( font[78], 8, 4, font[200], 9, 3, 1, 3 );
            fheroes2::Copy( font[78], 8, 4, font[200], 10, 2, 1, 3 );
            fheroes2::Copy( font[78], 8, 4, font[200], 11, 1, 1, 3 );
            fheroes2::Copy( font[78], 11, 7, font[200], 11, 8, 1, 1 );
            fheroes2::Copy( font[78], 13, 9, font[200], 11, 9, 1, 1 );
            updateNormalFontLetterShadow( font[200] );

            font[201].resize( font[200].width(), font[200].height() + 3 );
            font[201].reset();
            fheroes2::Copy( font[200], 0, 0, font[201], 0, 3, font[200].width(), font[200].height() );
            font[201].setPosition( font[200].x(), font[200].y() - 3 );
            fheroes2::Copy( font[201], 12, 4, font[201], 8, 0, 1, 1 );
            fheroes2::Copy( font[201], 11, 10, font[201], 8, 1, 1, 1 );
            updateNormalFontLetterShadow( font[201] );

            font[202] = font[75 + offset];

            font[204] = font[77 + offset];
            font[205] = font[72 + offset];
            font[206] = font[79 + offset];

            font[207] = font[195];
            fheroes2::Copy( font[207], 4, 1, font[207], 8, 1, 2, 9 );
            fheroes2::Copy( font[207], 4, 9, font[207], 8, 10, 2, 1 );
            fheroes2::Copy( font[207], 6, 0, font[207], 10, 0, 1, 2 );
            updateNormalFontLetterShadow( font[207] );

            font[203].resize( font[207].width() - 1, font[207].height() );
            font[203].reset();
            fheroes2::Copy( font[207], 0, 0, font[203], 0, 0, font[207].width() - 1, font[207].height() );
            fheroes2::FillTransform( font[203], 0, 0, 4, 6, 1 );
            fheroes2::FillTransform( font[203], 4, 0, 3, 2, 1 );
            fheroes2::Copy( font[203], 4, 2, font[203], 5, 1, 2, 1 );
            fheroes2::Copy( font[203], 1, 10, font[203], 5, 0, 2, 1 );
            font[203].setPosition( font[207].x(), font[207].y() );
            updateNormalFontLetterShadow( font[203] );

            font[208] = font[80 + offset];
            font[209] = font[67 + offset];

            font[210].resize( font[207].width() + 4, font[207].height() );
            font[210].reset();
            fheroes2::Copy( font[207], 0, 0, font[210], 0, 0, font[207].width(), font[207].height() );
            fheroes2::Copy( font[210], 7, 0, font[210], 11, 0, 4, font[207].height() );
            font[210].setPosition( font[207].x(), font[207].y() );

            font[211] = font[89 + offset];

            font[212].resize( font[80].width() + 1, font[80].height() );
            font[212].reset();
            fheroes2::Copy( font[80], 0, 0, font[212], 1, 0, font[80].width(), font[80].height() );
            fheroes2::Flip( font[80], 6, 0, font[212], 1, 0, 5, 6, true, false );
            font[212].setPosition( font[80].x(), font[80].y() );
            updateNormalFontLetterShadow( font[212] );

            font[213] = font[88 + offset];

            font[214].resize( font[85].width() + 2, font[85].height() + 1 );
            font[214].reset();
            fheroes2::Copy( font[85], 0, 0, font[214], 0, 0, font[84].width(), font[84].height() );
            fheroes2::Copy( font[214], 9, 1, font[214], 11, 9, 1, 1 );
            fheroes2::Copy( font[214], 9, 1, font[214], 12, 8, 1, 1 );
            fheroes2::Copy( font[214], 9, 1, font[214], 12, 10, 1, 2 );
            fheroes2::Copy( font[214], 10, 1, font[214], 12, 9, 1, 1 );
            fheroes2::Copy( font[214], 10, 1, font[214], 13, 8, 1, 4 );
            font[214].setPosition( font[85].x(), font[85].y() );
            updateNormalFontLetterShadow( font[214] );

            font[216].resize( font[85].width() + 2, font[85].height() );
            font[216].reset();
            fheroes2::Copy( font[85], 0, 0, font[216], 0, 0, 6, 11 );
            fheroes2::Copy( font[85], 8, 0, font[216], 7, 0, 3, 11 );
            fheroes2::Copy( font[85], 8, 0, font[216], 11, 0, 3, 11 );
            fheroes2::Copy( font[204], 10, 0, font[216], 6, 5, 3, 5 );
            fheroes2::Copy( font[204], 10, 0, font[216], 10, 5, 3, 5 );
            fheroes2::FillTransform( font[216], 7, 10, 1, 1, 1 );
            fheroes2::FillTransform( font[216], 11, 10, 1, 1, 1 );
            font[216].setPosition( font[85].x(), font[85].y() );
            updateNormalFontLetterShadow( font[216] );

            font[215] = font[85];
            fheroes2::FillTransform( font[215], 3, 6, 6, 7, 1 );
            fheroes2::Copy( font[216], 4, 5, font[215], 4, 3, 4, 6 );
            fheroes2::Copy( font[215], 6, 5, font[215], 8, 3, 1, 3 );
            fheroes2::Copy( font[215], 7, 4, font[215], 9, 2, 1, 2 );
            fheroes2::Copy( font[215], 9, 8, font[215], 9, 9, 1, 1 );
            updateNormalFontLetterShadow( font[215] );

            font[217].resize( font[216].width() + 2, font[216].height() + 1 );
            font[217].reset();
            fheroes2::Copy( font[216], 0, 0, font[217], 0, 0, font[216].width(), font[216].height() );
            fheroes2::Copy( font[214], 11, 8, font[217], 14, 8, 3, 4 );
            font[217].setPosition( font[216].x(), font[216].y() );
            updateNormalFontLetterShadow( font[217] );

            font[218].resize( font[193].width() + 1, font[193].height() );
            font[218].reset();
            fheroes2::Copy( font[193], 0, 0, font[218], 1, 0, font[193].width(), font[193].height() );
            fheroes2::Copy( font[193], 1, 0, font[218], 1, 0, 3, 4 );
            fheroes2::FillTransform( font[218], 7, 0, 5, 4, 1 );
            font[218].setPosition( font[193].x(), font[193].y() );
            updateNormalFontLetterShadow( font[218] );

            font[220] = font[193];
            fheroes2::FillTransform( font[220], 0, 0, 4, 6, 1 );
            fheroes2::FillTransform( font[220], 6, 0, 5, 4, 1 );
            fheroes2::Copy( font[85], 8, 0, font[220], 3, 0, 3, 1 );
            updateNormalFontLetterShadow( font[220] );

            font[219].resize( font[220].width() + 3, font[220].height() );
            font[219].reset();
            fheroes2::Copy( font[220], 0, 0, font[219], 0, 0, font[220].width(), font[220].height() );
            fheroes2::Copy( font[219], 3, 0, font[219], 11, 0, 3, 9 );
            fheroes2::Copy( font[207], 8, 9, font[219], 12, 9, 2, 2 );
            font[219].setPosition( font[220].x(), font[220].y() );
            updateNormalFontLetterShadow( font[219] );

            font[221].resize( font[79].width() - 3, font[79].height() );
            font[221].reset();
            fheroes2::Copy( font[79], 4, 0, font[221], 1, 0, 9, 11 );
            fheroes2::FillTransform( font[221], 0, 3, 3, 5, 1 );
            fheroes2::Copy( font[221], 3, 0, font[221], 4, 5, 5, 1 );
            font[221].setPosition( font[79].x(), font[79].y() );
            updateNormalFontLetterShadow( font[221] );

            font[222].resize( font[79].width() + 1, font[79].height() );
            font[222].reset();
            fheroes2::Copy( font[193], 0, 0, font[222], 0, 0, 6, 13 );
            fheroes2::Copy( font[79], 4, 1, font[222], 7, 1, 4, 8 );
            fheroes2::Copy( font[79], 10, 1, font[222], 11, 1, 3, 8 );
            fheroes2::Copy( font[79], 5, 0, font[222], 8, 0, 3, 1 );
            fheroes2::Copy( font[79], 10, 0, font[222], 11, 0, 2, 1 );
            fheroes2::Copy( font[79], 4, 9, font[222], 7, 9, 4, 2 );
            fheroes2::Copy( font[79], 10, 9, font[222], 11, 9, 3, 2 );
            fheroes2::Copy( font[222], 2, 0, font[222], 6, 5, 2, 1 );
            font[222].setPosition( font[193].x(), font[193].y() );
            updateNormalFontLetterShadow( font[222] );

            font[223].resize( font[203].width() - 1, font[203].height() );
            font[223].reset();
            fheroes2::Copy( font[65], 0, 5, font[223], 0, 5, 7, 6 );
            fheroes2::Copy( font[212], 0, 0, font[223], 1, 0, 7, 6 );
            fheroes2::Copy( font[203], 8, 0, font[223], 7, 0, 2, 11 );
            fheroes2::Copy( font[223], 6, 5, font[223], 7, 5, 1, 1 );
            font[223].setPosition( font[203].x(), font[203].y() );
            updateNormalFontLetterShadow( font[223] );

            offset = 32;

            // e with 2 dots on top.
            font[184].resize( font[101].width(), font[101].height() + 3 );
            font[184].reset();
            fheroes2::Copy( font[101], 0, 0, font[184], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[168], 3, 0, font[184], 3, 0, 2, 4 );
            fheroes2::Copy( font[168], 3, 0, font[184], 5, 0, 2, 4 );
            font[184].setPosition( font[101].x(), font[101].y() - 3 );
            // Not shadow needs to be updated here.

            font[224] = font[65 + offset];

            font[225].resize( font[101].width(), font[101].height() + 3 );
            font[225].reset();
            fheroes2::Copy( font[101], 1, 5, font[225], 1, 8, 8, 2 );
            fheroes2::Copy( font[101], 1, 0, font[225], 1, 6, 8, 2 );
            fheroes2::Copy( font[99], 1, 0, font[225], 1, 0, 8, 2 );
            fheroes2::Copy( font[77], 7, 3, font[225], 1, 2, 3, 1 );
            fheroes2::Copy( font[77], 7, 3, font[225], 2, 3, 3, 1 );
            fheroes2::Copy( font[77], 7, 3, font[225], 3, 4, 3, 1 );
            fheroes2::Copy( font[77], 8, 3, font[225], 6, 5, 2, 1 );
            fheroes2::Copy( font[77], 7, 3, font[225], 4, 5, 2, 1 );
            font[225].setPosition( font[101].x(), font[101].y() - 3 );
            updateNormalFontLetterShadow( font[225] );

            font[227] = font[114];
            fheroes2::Copy( font[227], 1, 0, font[227], 3, 0, 2, 1 );
            fheroes2::Copy( font[227], 4, 2, font[227], 4, 1, 1, 1 );
            fheroes2::SetTransformPixel( font[227], 4, 2, 1 );
            updateNormalFontLetterShadow( font[227] );

            // The same letter as above but with a vertical line at the top.
            font[180].resize( font[227].width(), font[227].height() + 1 );
            font[180].reset();
            fheroes2::Copy( font[227], 0, 0, font[180], 0, 1, font[227].width(), font[227].height() );
            fheroes2::Copy( font[227], 6, 1, font[180], 6, 0, 3, 1 );
            fheroes2::FillTransform( font[180], 7, 2, 2, 1, 1 );
            fheroes2::FillTransform( font[180], 6, 4, 2, 1, 1 );
            font[180].setPosition( font[227].x(), font[227].y() - 1 );

            font[228] = font[103];
            font[229] = font[69 + offset];

            // x with | in the middle.
            font[230].resize( font[120].width() + 2, font[120].height() );
            font[230].reset();
            fheroes2::Copy( font[120], 0, 0, font[230], 0, 0, 6, 7 );
            fheroes2::Copy( font[120], 5, 0, font[230], 7, 0, 5, 7 );
            fheroes2::Fill( font[230], 6, 1, 1, 5, font[230].image()[3 + font[230].width()] );
            font[230].setPosition( font[120].x(), font[120].y() );
            updateNormalFontLetterShadow( font[230] );

            // letter 3 (z)
            font[231].resize( font[51].width(), font[51].height() - 4 );
            font[231].reset();
            fheroes2::Copy( font[51], 0, 0, font[231], 0, 0, font[51].width(), 3 );
            fheroes2::Copy( font[51], 0, 5, font[231], 0, 3, font[51].width(), 1 );
            fheroes2::Copy( font[51], 0, 8, font[231], 0, 4, font[51].width(), 4 );
            fheroes2::FillTransform( font[231], 0, 2, 3, 3, 1 );
            font[231].setPosition( font[51].x(), font[51].y() + 4 );
            updateNormalFontLetterShadow( font[231] );

            // letter B (v)
            font[226].resize( font[231].width() + 1, font[231].height() );
            font[226].reset();
            fheroes2::Copy( font[231], 0, 0, font[226], 1, 0, font[231].width(), font[231].height() );
            fheroes2::Copy( font[109], 1, 0, font[226], 1, 0, 3, 7 );
            fheroes2::Copy( font[226], 7, 1, font[226], 3, 0, 1, 1 );
            fheroes2::Copy( font[226], 7, 1, font[226], 3, 6, 1, 1 );
            fheroes2::Copy( font[226], 3, 4, font[226], 3, 5, 1, 1 );
            font[226].setPosition( font[231].x(), font[231].y() );
            updateNormalFontLetterShadow( font[226] );

            font[232] = font[117];

            font[233].resize( font[232].width(), font[232].height() + 3 );
            font[233].reset();
            fheroes2::Copy( font[232], 0, 0, font[233], 0, 3, font[232].width(), font[232].height() );
            fheroes2::Copy( font[233], 8, 3, font[233], 5, 0, 1, 1 );
            fheroes2::Copy( font[233], 7, 3, font[233], 5, 1, 1, 1 );
            font[233].setPosition( font[232].x(), font[232].y() - 3 );
            updateNormalFontLetterShadow( font[233] );

            // Shorter k.
            font[234].resize( font[107].width() - 1, font[107].height() - 4 );
            font[234].reset();
            fheroes2::Copy( font[107], 0, 0, font[234], 0, 0, 4, 6 );
            fheroes2::Copy( font[107], 4, 4, font[234], 4, 0, 5, 6 );
            fheroes2::Copy( font[107], 0, 10, font[234], 0, 6, 4, 1 );
            fheroes2::Copy( font[107], 7, 10, font[234], 6, 6, 3, 1 );
            font[234].setPosition( font[107].x(), font[107].y() + 4 );
            updateNormalFontLetterShadow( font[234] );

            font[235] = font[110];
            fheroes2::Copy( font[235], 3, 0, font[235], 2, 1, 1, 1 );
            fheroes2::FillTransform( font[235], 0, 0, 2, 3, 1 );
            fheroes2::FillTransform( font[235], 2, 0, 1, 1, 1 );
            updateNormalFontLetterShadow( font[235] );

            font[236] = font[77 + offset];
            fheroes2::Copy( font[119], 9, 0, font[236], 3, 0, 4, 7 );
            fheroes2::Copy( font[119], 9, 0, font[236], 9, 0, 4, 7 );
            fheroes2::FillTransform( font[236], 0, 0, 3, 6, 1 );
            updateNormalFontLetterShadow( font[236] );

            font[237] = font[110];
            fheroes2::FillTransform( font[237], 4, 0, 3, 8, 1 );
            fheroes2::Copy( font[110], 4, 1, font[237], 4, 3, 1, 2 );
            fheroes2::Copy( font[110], 4, 1, font[237], 5, 3, 1, 2 );
            fheroes2::Copy( font[110], 4, 1, font[237], 6, 3, 1, 2 );
            fheroes2::Copy( font[110], 4, 1, font[237], 7, 3, 1, 1 );
            updateNormalFontLetterShadow( font[237] );

            font[238] = font[79 + offset];

            font[239] = font[110];

            font[240] = font[80 + offset];
            font[241] = font[67 + offset];
            font[242] = font[109];
            font[243] = font[89 + offset];

            font[244].resize( font[113].width(), font[113].height() );
            font[244].reset();
            fheroes2::Copy( font[112], 1, 0, font[244], 3, 0, 4, 10 );
            fheroes2::Copy( font[113], 0, 0, font[244], 0, 0, 5, 7 );
            fheroes2::Copy( font[112], 6, 0, font[244], 7, 0, 4, 7 );
            font[244].setPosition( font[113].x(), font[113].y() );
            updateNormalFontLetterShadow( font[244] );

            font[245] = font[88 + offset];

            font[246].resize( font[117].width() + 2, font[117].height() + 1 );
            font[246].reset();
            fheroes2::Copy( font[117], 0, 0, font[246], 0, 0, font[117].width(), font[117].height() );
            fheroes2::Copy( font[246], 7, 4, font[246], 9, 5, 1, 1 );
            fheroes2::Copy( font[246], 7, 4, font[246], 10, 4, 1, 1 );
            fheroes2::Copy( font[246], 8, 1, font[246], 11, 4, 1, 4 );
            fheroes2::Copy( font[246], 8, 1, font[246], 10, 5, 1, 1 );
            fheroes2::Copy( font[246], 9, 5, font[246], 10, 6, 1, 1 );
            fheroes2::Copy( font[246], 9, 5, font[246], 10, 7, 1, 1 );
            font[246].setPosition( font[117].x(), font[117].y() );
            updateNormalFontLetterShadow( font[246] );

            font[247] = font[117];
            fheroes2::Copy( font[247], 2, 5, font[247], 2, 3, 6, 2 );
            fheroes2::Copy( font[247], 8, 0, font[247], 7, 4, 1, 1 );
            fheroes2::Copy( font[247], 8, 0, font[247], 7, 5, 1, 1 );
            fheroes2::Copy( font[247], 8, 0, font[247], 7, 6, 1, 1 );
            fheroes2::FillTransform( font[247], 1, 5, 6, 4, 1 );
            updateNormalFontLetterShadow( font[247] );

            font[248].resize( font[117].width() + 3, font[117].height() );
            font[248].reset();
            fheroes2::Copy( font[117], 0, 0, font[248], 0, 0, 4, 7 );
            fheroes2::Copy( font[117], 1, 0, font[248], 5, 0, 4, 7 );
            fheroes2::Copy( font[117], 6, 0, font[248], 9, 0, 4, 7 );
            fheroes2::Copy( font[248], 8, 5, font[248], 4, 5, 4, 2 );
            font[248].setPosition( font[117].x(), font[117].y() );
            updateNormalFontLetterShadow( font[248] );

            font[249].resize( font[248].width() + 2, font[248].height() );
            font[249].reset();
            fheroes2::Copy( font[248], 0, 0, font[249], 0, 0, 12, 7 );
            fheroes2::Copy( font[246], 9, 4, font[249], 12, 4, 3, 4 );
            font[249].setPosition( font[248].x(), font[248].y() );
            updateNormalFontLetterShadow( font[249] );

            font[252] = font[226];
            fheroes2::FillTransform( font[252], 4, 0, 5, 3, 1 );

            font[250].resize( font[252].width() + 1, font[252].height() );
            font[250].reset();
            fheroes2::Copy( font[252], 0, 0, font[250], 1, 0, font[252].width(), font[252].height() );
            fheroes2::Copy( font[252], 1, 0, font[250], 1, 0, 1, 2 );
            font[250].setPosition( font[252].x(), font[252].y() );
            updateNormalFontLetterShadow( font[250] );

            font[251].resize( font[252].width() + 3, font[252].height() );
            font[251].reset();
            fheroes2::Copy( font[252], 0, 0, font[251], 0, 0, font[252].width(), font[252].height() );
            fheroes2::Copy( font[252], 2, 0, font[251], 10, 0, 2, 7 );
            font[251].setPosition( font[252].x(), font[252].y() );
            updateNormalFontLetterShadow( font[251] );

            font[253] = font[111];
            fheroes2::FillTransform( font[253], 0, 2, 3, 3, 1 );
            fheroes2::Copy( font[253], 8, 3, font[253], 7, 3, 1, 1 );
            fheroes2::Copy( font[253], 8, 3, font[253], 6, 3, 1, 1 );
            fheroes2::Copy( font[253], 8, 3, font[253], 5, 3, 1, 1 );
            updateNormalFontLetterShadow( font[253] );

            font[254].resize( font[111].width() + 1, font[111].height() );
            font[254].reset();
            fheroes2::Copy( font[251], 1, 0, font[254], 1, 0, 3, 7 );
            fheroes2::Copy( font[111], 2, 1, font[254], 6, 1, 1, 5 );
            fheroes2::Copy( font[111], 3, 0, font[254], 6, 0, 3, 2 );
            fheroes2::Copy( font[111], 7, 0, font[254], 9, 0, 1, 2 );
            fheroes2::Copy( font[111], 8, 2, font[254], 9, 2, 1, 3 );
            fheroes2::Copy( font[111], 7, 5, font[254], 9, 5, 1, 2 );
            fheroes2::Copy( font[111], 3, 6, font[254], 6, 6, 3, 1 );
            fheroes2::Copy( font[254], 1, 0, font[254], 4, 3, 2, 1 );
            font[254].setPosition( font[251].x(), font[251].y() );
            updateNormalFontLetterShadow( font[254] );

            font[255] = font[97];
            fheroes2::FillTransform( font[255], 0, 2, 6, 3, 1 );
            fheroes2::Copy( font[101], 2, 5, font[255], 1, 2, 6, 2 );
            fheroes2::Copy( font[255], 6, 4, font[255], 6, 3, 1, 1 );
            updateNormalFontLetterShadow( font[255] );

            // Cyrillic Capital Lje.
            font[138].resize( font[203].width() + font[220].width() - 6, font[203].height() );
            font[138].reset();
            fheroes2::Copy( font[203], 0, 0, font[138], 0, 0, font[203].width(), font[203].height() );
            fheroes2::Copy( font[220], 4, 0, font[138], font[203].width() - 2, 0, font[220].width() - 4, font[220].height() );
            font[138].setPosition( font[203].x(), font[203].y() );
            updateNormalFontLetterShadow( font[138] );

            // Cyrillic Capital Nje.
            font[140].resize( font[205].width() + font[220].width() - 7, font[205].height() );
            font[140].reset();
            fheroes2::Copy( font[205], 0, 0, font[140], 0, 0, font[205].width(), font[205].height() );
            fheroes2::Copy( font[220], 4, 0, font[140], font[205].width() - 3, 0, font[220].width() - 4, font[220].height() );
            font[140].setPosition( font[205].x(), font[205].y() );
            updateNormalFontLetterShadow( font[140] );

            // Cyrillic Lowercase Lje.
            font[154].resize( font[235].width() + font[252].width() - 5, font[235].height() );
            font[154].reset();
            fheroes2::Copy( font[235], 0, 0, font[154], 0, 0, font[235].width(), font[235].height() );
            fheroes2::Copy( font[252], 2, 0, font[154], font[235].width() - 3, 0, font[252].width() - 2, font[252].height() );
            font[154].setPosition( font[235].x(), font[235].y() );
            updateNormalFontLetterShadow( font[154] );

            // Cyrillic Lowercase Nje.
            font[156].resize( font[237].width() + font[252].width() - 5, font[237].height() );
            font[156].reset();
            fheroes2::Copy( font[237], 0, 0, font[156], 0, 0, font[237].width(), font[237].height() );
            fheroes2::Copy( font[252], 2, 0, font[156], font[237].width() - 3, 0, font[252].width() - 2, font[252].height() );
            font[156].setPosition( font[237].x(), font[237].y() );
            updateNormalFontLetterShadow( font[156] );
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            size_t offset = 0;

            // ' (right single quotation mark)
            font[146] = font[44];
            font[146].setPosition( font[146].x(), font[146].y() - 4 );

            // E with 2 dots on top.
            font[168].resize( font[69].width(), font[69].height() + 2 );
            font[168].reset();
            fheroes2::Copy( font[69], 0, 0, font[168], 0, 2, font[69].width(), font[69].height() );
            fheroes2::Copy( font[69], 3, 0, font[168], 3, 0, 1, 1 );
            fheroes2::Copy( font[69], 3, 0, font[168], 5, 0, 1, 1 );
            font[168].setPosition( font[69].x(), font[69].y() - 2 );
            updateSmallFontLetterShadow( font[168] );

            // NBSP character.
            font[160].resize( smallFontSpaceWidth, 1 );
            font[160].reset();

            font[161].resize( font[89 + offset].width(), font[89 + offset].height() + 2 );
            font[161].reset();
            fheroes2::Copy( font[89 + offset], 0, 0, font[161], 0, 2, font[89 + offset].width(), font[89 + offset].height() );
            fheroes2::Copy( font[89 + offset], 2, 0, font[161], 5, 0, 2, 1 );
            font[161].setPosition( font[89 + offset].x(), font[89 + offset].y() - 2 );
            updateSmallFontLetterShadow( font[161] );

            font[162].resize( font[121 + offset].width(), font[121 + offset].height() + 2 );
            font[162].reset();
            fheroes2::Copy( font[121 + offset], 0, 0, font[162], 0, 2, font[121 + offset].width(), font[121 + offset].height() );
            fheroes2::Copy( font[121 + offset], 1, 0, font[162], 4, 0, 2, 1 );
            font[162].setPosition( font[121 + offset].x(), font[121 + offset].y() - 2 );
            updateSmallFontLetterShadow( font[162] );

            // C with a horizontal line in the middle.
            font[170] = font[67];
            fheroes2::Copy( font[170], 3, 0, font[170], 2, 3, 3, 1 );
            updateSmallFontLetterShadow( font[170] );

            font[186] = font[99];
            fheroes2::Copy( font[186], 2, 0, font[186], 2, 2, 2, 1 );
            updateSmallFontLetterShadow( font[186] );

            // I and i with 2 dots.
            font[175] = font[73];

            font[191] = font[105];
            fheroes2::Copy( font[105], 1, 0, font[191], 0, 0, 2, 2 );
            fheroes2::Copy( font[105], 1, 0, font[191], 2, 0, 2, 2 );

            // J and j.
            font[163] = font[74];
            font[188] = font[106];

            // S and s.
            font[189] = font[83];
            font[190] = font[115];

            // I and i.
            font[178] = font[73];
            font[179] = font[105];

            // A.
            font[192] = font[65 + offset];

            font[193] = font[66 + offset];
            fheroes2::FillTransform( font[193], 5, 1, 2, 2, 1 );
            fheroes2::Copy( font[193], 5, 0, font[193], 6, 0, 1, 1 );
            updateSmallFontLetterShadow( font[193] );

            font[194] = font[66 + offset];

            font[195].resize( font[193].width() + 1, font[193].height() );
            font[195].reset();
            fheroes2::Copy( font[193], 0, 0, font[195], 0, 0, 4, 8 );
            fheroes2::Copy( font[193], 3, 0, font[195], 4, 0, 4, 1 );
            font[195].setPosition( font[193].x(), font[193].y() );
            updateSmallFontLetterShadow( font[195] );

            // The same letter as above but with a vertical line of top.
            font[165].resize( font[195].width() - 1, font[195].height() + 1 );
            font[165].reset();
            fheroes2::Copy( font[195], 0, 0, font[165], 0, 1, font[195].width() - 1, font[195].height() );
            fheroes2::Copy( font[195], 2, 0, font[165], 6, 0, 1, 1 );
            fheroes2::FillTransform( font[165], 6, 2, 1, 1, 1 );
            font[165].setPosition( font[195].x(), font[195].y() - 1 );
            updateSmallFontLetterShadow( font[165] );

            font[196] = font[68 + offset];
            font[197] = font[69 + offset];

            font[198].resize( font[88].width() + 1, font[88].height() );
            font[198].reset();
            fheroes2::Copy( font[88], 1, 0, font[198], 1, 0, 3, 7 );
            fheroes2::Copy( font[88], 7, 0, font[198], 7, 0, 2, 7 );
            fheroes2::Copy( font[88], 4, 2, font[198], 3, 2, 1, 3 );
            fheroes2::Copy( font[88], 6, 2, font[198], 7, 2, 1, 3 );
            fheroes2::Copy( font[69], 4, 5, font[198], 4, 2, 3, 3 );
            fheroes2::Copy( font[69], 3, 0, font[198], 5, 0, 1, 7 );
            fheroes2::Copy( font[88], 8, 0, font[198], 9, 0, 1, 7 );
            font[198].setPosition( font[88].x(), font[88].y() );
            updateSmallFontLetterShadow( font[198] );

            font[199].resize( font[51].width() + 2, font[51].height() );
            font[199].reset();
            fheroes2::Copy( font[51], 1, 0, font[199], 1, 0, 3, 2 );
            fheroes2::Copy( font[51], 2, 0, font[199], 4, 0, 3, 2 );
            fheroes2::Copy( font[51], 2, 2, font[199], 3, 2, 3, 3 );
            fheroes2::Copy( font[51], 2, 5, font[199], 4, 5, 3, 2 );
            fheroes2::Copy( font[51], 1, 5, font[199], 1, 5, 3, 2 );
            fheroes2::FillTransform( font[199], 2, 4, 3, 2, 1 );
            fheroes2::FillTransform( font[199], 5, 5, 1, 1, 1 );
            fheroes2::FillTransform( font[199], 4, 2, 1, 1, 1 );
            font[199].setPosition( font[51].x(), font[51].y() );
            updateSmallFontLetterShadow( font[199] );

            font[200] = font[72];
            fheroes2::FillTransform( font[200], 4, 2, 3, 4, 1 );
            fheroes2::Copy( font[72], 3, 0, font[200], 4, 4, 1, 1 );
            fheroes2::Copy( font[72], 3, 0, font[200], 5, 3, 1, 1 );
            fheroes2::Copy( font[72], 3, 0, font[200], 6, 2, 1, 1 );
            updateSmallFontLetterShadow( font[200] );

            font[201].resize( font[200].width(), font[200].height() + 2 );
            font[201].reset();
            fheroes2::Copy( font[200], 1, 0, font[201], 1, 2, 8, 7 );
            fheroes2::Copy( font[200], 2, 0, font[201], 5, 0, 2, 1 );
            font[201].setPosition( font[200].x(), font[200].y() - 2 );
            updateSmallFontLetterShadow( font[201] );

            font[202] = font[75 + offset];

            font[203].resize( font[66].width(), font[66].height() );
            font[203].reset();
            fheroes2::Copy( font[66], 1, 0, font[203], 1, 0, 3, 7 );
            fheroes2::Copy( font[66], 3, 0, font[203], 6, 0, 1, 7 );
            fheroes2::Copy( font[66], 3, 0, font[203], 4, 0, 2, 1 );
            fheroes2::FillTransform( font[203], 1, 0, 2, 2, 1 );
            fheroes2::FillTransform( font[203], 3, 0, 1, 1, 1 );
            font[203].setPosition( font[66].x(), font[66].y() );
            updateSmallFontLetterShadow( font[203] );

            font[204] = font[77 + offset];
            font[205] = font[72 + offset];
            font[206] = font[79 + offset];

            font[207] = font[195];
            fheroes2::Copy( font[207], 3, 0, font[207], 6, 0, 1, 7 );
            updateSmallFontLetterShadow( font[207] );

            font[208] = font[80 + offset];
            font[209] = font[67 + offset];

            font[210].resize( font[207].width() + 2, font[207].height() );
            font[210].reset();
            fheroes2::Copy( font[207], 0, 0, font[210], 0, 0, font[207].width(), font[207].height() );
            fheroes2::Copy( font[210], 5, 0, font[210], 8, 0, 2, 8 );
            font[210].setPosition( font[207].x(), font[207].y() );

            font[211] = font[89 + offset];
            font[213] = font[88 + offset];

            font[214].resize( font[85].width(), font[85].height() + 1 );
            font[214].reset();
            fheroes2::Copy( font[85], 1, 0, font[214], 1, 0, 8, 7 );
            fheroes2::Copy( font[214], 3, 0, font[214], 9, 5, 1, 3 );
            font[214].setPosition( font[85].x(), font[85].y() );
            updateSmallFontLetterShadow( font[214] );

            font[215] = font[85];
            fheroes2::Copy( font[85], 3, 5, font[215], 3, 2, 4, 2 );
            fheroes2::FillTransform( font[215], 2, 4, 5, 4, 1 );
            updateSmallFontLetterShadow( font[215] );

            font[216].resize( font[85].width(), font[85].height() );
            font[216].reset();
            fheroes2::Copy( font[85], 1, 0, font[216], 1, 0, 4, 7 );
            fheroes2::Copy( font[85], 7, 1, font[216], 6, 1, 2, 6 );
            fheroes2::Copy( font[76], 3, 0, font[216], 9, 0, 1, 8 );
            fheroes2::Copy( font[85], 7, 1, font[216], 5, 5, 1, 1 );
            fheroes2::Copy( font[85], 7, 1, font[216], 8, 5, 1, 1 );
            fheroes2::Copy( font[85], 7, 1, font[216], 6, 0, 1, 1 );
            font[216].setPosition( font[85].x(), font[85].y() );
            updateSmallFontLetterShadow( font[216] );

            font[217].resize( font[216].width() + 2, font[216].height() + 1 );
            font[217].reset();
            fheroes2::Copy( font[216], 1, 0, font[217], 1, 0, 9, 7 );
            fheroes2::Copy( font[216], 3, 0, font[217], 10, 6, 1, 1 );
            fheroes2::Copy( font[216], 3, 0, font[217], 11, 5, 1, 3 );
            font[217].setPosition( font[216].x(), font[216].y() );
            updateSmallFontLetterShadow( font[217] );

            font[220].resize( font[66].width(), font[66].height() );
            font[220].reset();
            fheroes2::Copy( font[66], 2, 0, font[220], 1, 0, 2, 7 );
            fheroes2::Copy( font[66], 4, 3, font[220], 3, 3, 1, 4 );
            fheroes2::Copy( font[66], 4, 3, font[220], 4, 3, 3, 4 );
            fheroes2::FillTransform( font[220], 1, 0, 1, 1, 1 );
            font[220].setPosition( font[66].x(), font[66].y() );
            updateSmallFontLetterShadow( font[220] );

            font[219].resize( font[220].width() + 2, font[220].height() );
            font[219].reset();
            fheroes2::Copy( font[220], 1, 0, font[219], 1, 0, 6, 7 );
            fheroes2::Copy( font[220], 2, 0, font[219], 8, 0, 1, 7 );
            font[219].setPosition( font[220].x(), font[220].y() );
            updateSmallFontLetterShadow( font[219] );

            font[218].resize( font[220].width() + 2, font[220].height() );
            font[218].reset();
            fheroes2::Copy( font[220], 1, 0, font[218], 3, 0, 6, 7 );
            fheroes2::Copy( font[220], 2, 3, font[218], 1, 0, 3, 1 );
            fheroes2::Copy( font[220], 2, 3, font[218], 1, 1, 1, 1 );
            font[218].setPosition( font[220].x(), font[220].y() );
            updateSmallFontLetterShadow( font[218] );

            font[221].resize( font[79].width() - 1, font[79].height() );
            font[221].reset();
            fheroes2::Copy( font[79], 2, 0, font[221], 1, 0, 6, 7 );
            fheroes2::Copy( font[79], 3, 0, font[221], 4, 3, 2, 1 );
            font[221].setPosition( font[79].x(), font[79].y() );
            updateSmallFontLetterShadow( font[221] );

            font[222].resize( font[79].width() + 1, font[79].height() );
            font[222].reset();
            fheroes2::Copy( font[76], 2, 0, font[222], 1, 0, 2, 7 );
            fheroes2::Copy( font[79], 2, 0, font[222], 4, 0, 5, 2 );
            fheroes2::Copy( font[79], 2, 5, font[222], 4, 5, 5, 2 );
            fheroes2::Copy( font[222], 1, 0, font[222], 3, 3, 1, 1 );
            fheroes2::Copy( font[222], 2, 0, font[222], 4, 2, 1, 3 );
            fheroes2::Copy( font[222], 2, 0, font[222], 8, 2, 1, 3 );
            font[222].setPosition( font[79].x(), font[79].y() );
            updateSmallFontLetterShadow( font[222] );

            font[223].resize( font[80].width() - 1, font[80].height() );
            font[223].reset();
            fheroes2::Copy( font[80], 7, 1, font[223], 2, 1, 1, 2 );
            fheroes2::Copy( font[80], 2, 0, font[223], 3, 0, 3, 1 );
            fheroes2::Copy( font[80], 2, 0, font[223], 3, 3, 3, 1 );
            fheroes2::Copy( font[76], 3, 0, font[223], 6, 0, 1, 7 );
            fheroes2::Copy( font[65], 1, 6, font[223], 1, 6, 2, 1 );
            fheroes2::Copy( font[65], 1, 6, font[223], 3, 5, 1, 1 );
            fheroes2::Copy( font[65], 1, 6, font[223], 4, 4, 1, 1 );
            font[223].setPosition( font[80].x(), font[80].y() );
            updateSmallFontLetterShadow( font[223] );

            offset = 32;

            // e with 2 dots on top.
            font[184].resize( font[101].width(), font[101].height() + 2 );
            font[184].reset();
            fheroes2::Copy( font[101], 0, 0, font[184], 0, 2, font[101].width(), font[101].height() );
            fheroes2::Copy( font[101], 2, 0, font[184], 2, 0, 1, 1 );
            fheroes2::Copy( font[101], 2, 0, font[184], 4, 0, 1, 1 );
            font[184].setPosition( font[101].x(), font[101].y() - 2 );
            updateSmallFontLetterShadow( font[184] );

            font[224] = font[65 + offset];

            font[225].resize( font[66].width(), font[66].height() );
            font[225].reset();
            fheroes2::Copy( font[66], 4, 3, font[225], 4, 3, 3, 4 );
            fheroes2::Copy( font[97], 1, 2, font[225], 2, 4, 2, 3 );
            fheroes2::FillTransform( font[225], 3, 5, 1, 1, 1 );
            fheroes2::Copy( font[225], 2, 5, font[225], 2, 1, 2, 2 );
            fheroes2::Copy( font[69], 2, 0, font[225], 2, 0, 5, 1 );
            font[225].setPosition( font[66].x(), font[66].y() );
            updateSmallFontLetterShadow( font[225] );

            font[226].resize( font[114].width() - 1, font[114].height() );
            font[226].reset();
            fheroes2::Copy( font[114], 1, 0, font[226], 1, 0, 2, 5 );
            fheroes2::Copy( font[114], 1, 0, font[226], 3, 0, 2, 1 );
            fheroes2::Copy( font[114], 1, 0, font[226], 3, 2, 2, 1 );
            fheroes2::Copy( font[114], 1, 0, font[226], 3, 4, 2, 1 );
            fheroes2::Copy( font[114], 1, 0, font[226], 5, 1, 1, 1 );
            fheroes2::Copy( font[114], 1, 0, font[226], 5, 3, 1, 1 );
            font[226].setPosition( font[114].x(), font[114].y() );
            updateSmallFontLetterShadow( font[226] );

            font[227] = font[114];
            fheroes2::Copy( font[227], 3, 1, font[227], 3, 0, 1, 1 );
            fheroes2::FillTransform( font[227], 3, 1, 1, 1, 1 );
            updateSmallFontLetterShadow( font[227] );

            // The same letter as above but with a vertical line of top.
            font[180].resize( font[227].width() - 1, font[227].height() + 1 );
            font[180].reset();
            fheroes2::Copy( font[227], 0, 0, font[180], 0, 1, font[227].width() - 1, font[227].height() );
            fheroes2::Copy( font[227], 5, 0, font[180], 5, 0, 1, 1 );
            fheroes2::FillTransform( font[180], 5, 2, 1, 1, 1 );
            font[180].setPosition( font[227].x(), font[227].y() - 1 );
            updateSmallFontLetterShadow( font[180] );

            font[228] = font[103];

            font[229] = font[69 + offset];

            font[230].resize( font[120].width() + 1, font[120].height() );
            font[230].reset();
            fheroes2::Copy( font[120], 0, 0, font[230], 0, 0, 4, 5 );
            fheroes2::Copy( font[120], 4, 0, font[230], 5, 0, 4, 5 );
            fheroes2::Copy( font[117], 2, 0, font[230], 4, 0, 1, 4 );
            fheroes2::Copy( font[117], 2, 0, font[230], 4, 4, 1, 1 );
            font[230].setPosition( font[120].x(), font[120].y() );
            updateSmallFontLetterShadow( font[230] );

            font[232] = font[117];

            font[233].resize( font[232].width(), font[232].height() + 2 );
            font[233].reset();
            fheroes2::Copy( font[232], 1, 0, font[233], 1, 2, 7, 5 );
            fheroes2::Copy( font[232], 1, 0, font[233], 3, 0, 2, 1 );
            font[233].setPosition( font[232].x(), font[232].y() - 2 );
            updateSmallFontLetterShadow( font[233] );

            font[234].resize( font[107].width(), font[107].height() - 2 );
            font[234].reset();
            fheroes2::Copy( font[107], 1, 0, font[234], 1, 0, 2, 5 );
            fheroes2::Copy( font[107], 3, 2, font[234], 3, 0, 3, 5 );
            font[234].setPosition( font[107].x(), font[107].y() + 2 );
            updateSmallFontLetterShadow( font[234] );

            font[235].resize( font[97].width() - 2, font[97].height() );
            font[235].reset();
            fheroes2::Copy( font[203], 2, 3, font[235], 1, 1, 2, 4 );
            fheroes2::Copy( font[203], 5, 0, font[235], 3, 0, 2, 5 );
            fheroes2::FillTransform( font[235], 1, 1, 1, 1, 1 );
            font[235].setPosition( font[97].x(), font[97].y() );
            updateSmallFontLetterShadow( font[235] );

            font[236].resize( font[235].width() + 3, font[235].height() );
            font[236].reset();
            fheroes2::Copy( font[235], 4, 0, font[236], 4, 0, 1, 5 );
            fheroes2::Copy( font[235], 1, 0, font[236], 1, 1, 3, 3 );
            fheroes2::Copy( font[236], 2, 0, font[236], 5, 0, 3, 5 );
            fheroes2::Copy( font[236], 4, 0, font[236], 1, 4, 1, 1 );
            font[236].setPosition( font[235].x(), font[235].y() );
            updateSmallFontLetterShadow( font[236] );

            font[237].resize( font[104].width() - 2, font[104].height() - 2 );
            font[237].reset();
            fheroes2::Copy( font[104], 1, 0, font[237], 1, 0, 2, 5 );
            fheroes2::Copy( font[104], 2, 0, font[237], 5, 0, 1, 5 );
            fheroes2::Copy( font[104], 3, 2, font[237], 3, 2, 2, 1 );
            font[237].setPosition( font[104].x(), font[104].y() + 2 );
            updateSmallFontLetterShadow( font[237] );

            font[238] = font[79 + offset];
            font[239] = font[110];
            font[240] = font[80 + offset];
            font[241] = font[67 + offset];

            font[242].resize( font[109].width() - 4, font[109].height() );
            font[242].reset();
            fheroes2::Copy( font[109], 1, 0, font[242], 1, 0, 2, 5 );
            fheroes2::Copy( font[109], 6, 1, font[242], 4, 1, 1, 4 );
            fheroes2::Copy( font[109], 10, 1, font[242], 6, 1, 2, 4 );
            fheroes2::Copy( font[109], 3, 0, font[242], 2, 0, 3, 1 );
            fheroes2::Copy( font[109], 3, 0, font[242], 5, 0, 1, 1 );
            font[242].setPosition( font[109].x(), font[109].y() );
            updateSmallFontLetterShadow( font[242] );

            font[243] = font[89 + offset];

            font[244].resize( font[113].width() + 3, font[113].height() + 1 );
            font[244].reset();
            fheroes2::Copy( font[113], 1, 0, font[244], 1, 0, 5, 7 );
            fheroes2::Copy( font[113], 2, 0, font[244], 6, 0, 4, 4 );
            fheroes2::Copy( font[113], 2, 4, font[244], 6, 4, 3, 1 );
            fheroes2::Copy( font[113], 2, 4, font[244], 5, 7, 1, 1 );
            font[244].setPosition( font[113].x(), font[113].y() );
            updateSmallFontLetterShadow( font[244] );

            // Bigger letter
            font[212] = font[244];
            fheroes2::Copy( font[212], 5, 1, font[212], 5, 0, 1, 1 );
            fheroes2::Copy( font[212], 5, 1, font[212], 4, 7, 1, 1 );
            font[212].setPosition( font[80].x(), font[80].y() ); // copy from a big better
            updateSmallFontLetterShadow( font[212] );

            font[245] = font[88 + offset];

            font[246].resize( font[117].width() + 1, font[117].height() + 1 );
            font[246].reset();
            fheroes2::Copy( font[117], 0, 0, font[246], 0, 0, font[117].width(), font[117].height() );
            fheroes2::Copy( font[246], 2, 0, font[246], 8, 3, 1, 3 );
            font[246].setPosition( font[117].x(), font[117].y() );
            updateSmallFontLetterShadow( font[246] );

            font[247] = font[117];
            fheroes2::Copy( font[117], 2, 4, font[247], 2, 2, 4, 1 );
            fheroes2::Copy( font[117], 1, 0, font[247], 6, 4, 1, 1 );
            fheroes2::FillTransform( font[247], 1, 3, 5, 3, 1 );
            updateSmallFontLetterShadow( font[247] );

            font[248].resize( font[117].width() + 2, font[117].height() );
            font[248].reset();
            fheroes2::Copy( font[117], 1, 0, font[248], 1, 0, 3, 5 );
            fheroes2::Copy( font[117], 6, 0, font[248], 5, 0, 2, 5 );
            fheroes2::Copy( font[117], 6, 0, font[248], 8, 0, 2, 5 );
            fheroes2::Copy( font[117], 1, 0, font[248], 4, 4, 1, 1 );
            fheroes2::Copy( font[117], 1, 0, font[248], 7, 4, 1, 1 );
            font[248].setPosition( font[117].x(), font[117].y() );
            updateSmallFontLetterShadow( font[248] );

            font[249].resize( font[248].width() + 1, font[248].height() );
            font[249].reset();
            fheroes2::Copy( font[248], 1, 0, font[249], 1, 0, 9, 5 );
            fheroes2::Copy( font[248], 2, 0, font[249], 10, 3, 1, 3 );
            font[249].setPosition( font[248].x(), font[248].y() );
            updateSmallFontLetterShadow( font[249] );

            font[252] = font[226];
            fheroes2::Copy( font[252], 1, 0, font[252], 5, 4, 1, 1 );
            fheroes2::FillTransform( font[252], 0, 0, 2, 2, 1 );
            fheroes2::FillTransform( font[252], 3, 0, 3, 2, 1 );

            font[250].resize( font[252].width() + 1, font[252].height() );
            font[250].reset();
            fheroes2::Copy( font[252], 1, 0, font[250], 2, 0, 5, 5 );
            fheroes2::Copy( font[252], 2, 2, font[250], 1, 0, 2, 1 );
            fheroes2::Copy( font[252], 2, 2, font[250], 1, 1, 1, 1 );
            font[250].setPosition( font[252].x(), font[252].y() );
            updateSmallFontLetterShadow( font[250] );

            font[251].resize( font[252].width() + 2, font[252].height() );
            font[251].reset();
            fheroes2::Copy( font[252], 1, 0, font[251], 1, 0, 5, 5 );
            fheroes2::Copy( font[252], 2, 0, font[251], 7, 0, 1, 5 );
            font[251].setPosition( font[252].x(), font[252].y() );
            updateSmallFontLetterShadow( font[251] );

            font[253].resize( font[111].width() - 1, font[111].height() );
            font[253].reset();
            fheroes2::Copy( font[111], 2, 0, font[253], 1, 0, 4, 5 );
            fheroes2::Copy( font[111], 2, 0, font[253], 2, 2, 2, 1 );
            font[253].setPosition( font[111].x(), font[111].y() );
            updateSmallFontLetterShadow( font[253] );

            font[231] = font[253];
            fheroes2::FillTransform( font[231], 0, 1, 3, 3, 1 );
            fheroes2::FillTransform( font[231], 4, 2, 1, 1, 1 );
            fheroes2::FillTransform( font[231], 1, 0, 1, 1, 1 );
            fheroes2::Copy( font[253], 1, 0, font[231], 1, 1, 1, 1 );
            updateSmallFontLetterShadow( font[231] );

            font[254].resize( font[111].width() + 2, font[111].height() );
            font[254].reset();
            fheroes2::Copy( font[114], 1, 0, font[254], 1, 0, 2, 5 );
            fheroes2::Copy( font[111], 1, 0, font[254], 4, 0, 3, 5 );
            fheroes2::Copy( font[111], 5, 1, font[254], 7, 1, 1, 3 );
            fheroes2::Copy( font[111], 5, 1, font[254], 3, 2, 1, 1 );
            font[254].setPosition( font[111].x(), font[111].y() );
            updateSmallFontLetterShadow( font[254] );

            font[255].resize( font[97].width() - 1, font[97].height() );
            font[255].reset();
            fheroes2::Copy( font[97], 1, 2, font[255], 2, 0, 3, 3 );
            fheroes2::Copy( font[235], 4, 0, font[255], 5, 0, 1, 5 );
            fheroes2::Copy( font[65], 1, 5, font[255], 1, 3, 3, 2 );
            font[255].setPosition( font[97].x(), font[97].y() );
            updateSmallFontLetterShadow( font[255] );

            // Cyrillic Capital Lje.
            font[138].resize( font[203].width() + font[220].width() - 4, font[203].height() );
            font[138].reset();
            fheroes2::Copy( font[203], 0, 0, font[138], 0, 0, font[203].width(), font[203].height() );
            fheroes2::Copy( font[220], 2, 0, font[138], font[203].width() - 2, 0, font[220].width() - 2, font[220].height() );
            font[138].setPosition( font[203].x(), font[203].y() );
            updateSmallFontLetterShadow( font[138] );

            // Cyrillic Capital Nje.
            font[140].resize( font[205].width() + font[220].width() - 4, font[205].height() );
            font[140].reset();
            fheroes2::Copy( font[205], 0, 0, font[140], 0, 0, font[205].width(), font[205].height() );
            fheroes2::Copy( font[220], 2, 0, font[140], font[205].width() - 2, 0, font[220].width() - 2, font[220].height() );
            font[140].setPosition( font[205].x(), font[205].y() );
            updateSmallFontLetterShadow( font[140] );

            // Cyrillic Lowercase Lje.
            font[154].resize( font[235].width() + font[252].width() - 3, font[235].height() );
            font[154].reset();
            fheroes2::Copy( font[235], 0, 0, font[154], 0, 0, font[235].width(), font[235].height() );
            fheroes2::Copy( font[252], 2, 0, font[154], font[235].width() - 1, 0, font[252].width() - 2, font[252].height() );
            font[154].setPosition( font[235].x(), font[235].y() );
            updateSmallFontLetterShadow( font[154] );

            // Cyrillic Lowercase Nje.
            font[156].resize( font[237].width() + font[252].width() - 3, font[237].height() );
            font[156].reset();
            fheroes2::Copy( font[237], 0, 0, font[156], 0, 0, font[237].width(), font[237].height() );
            fheroes2::Copy( font[252], 2, 0, font[156], font[237].width() - 1, 0, font[252].width() - 2, font[252].height() );
            font[156].setPosition( font[237].x(), font[237].y() );
            updateSmallFontLetterShadow( font[156] );
        }
    }

    // CP1252 supports German, Italian, Spanish, Norwegian, Swedish, Danish and French
    // (French localized maps have custom encoding that should be fixed by `fheroes2::fixFrenchCharactersForMP2Map()`)
    void generateCP1252Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        resizeCodePage( icnVsSprite );

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // NBSP character.
            font[160].resize( normalFontSpaceWidth, 1 );
            font[160].reset();

            // Inverted exclamation mark !.
            font[161].resize( font[33].width() + 1, font[33].height() + 3 );
            font[161].reset();
            fheroes2::Copy( font[33], 1, 0, font[161], 1, 3, font[33].width(), font[33].height() );
            {
                const fheroes2::Image temp = fheroes2::Flip( font[161], false, true );
                fheroes2::Copy( temp, 0, 2, font[161], 1, 2, temp.width(), temp.height() );
            }
            font[161].setPosition( font[33].x(), font[33].y() + 2 );
            updateNormalFontLetterShadow( font[161] );

            // Left-pointing double angle quotation mark <<.
            font[171].resize( 8, 9 );
            font[171].reset();
            fheroes2::Copy( font[47], 5, 0, font[171], 1, 0, 3, 3 );
            fheroes2::Flip( font[47], 5, 0, font[171], 1, 4, 3, 3, false, true );
            fheroes2::Flip( font[47], 5, 2, font[171], 1, 3, 2, 1, true, false );
            fheroes2::Copy( font[47], 5, 2, font[171], 1, 1, 1, 1 );
            fheroes2::Copy( font[47], 5, 2, font[171], 1, 5, 1, 1 );
            // Second mark.
            fheroes2::Copy( font[171], 1, 0, font[171], 5, 0, 3, 7 );
            font[171].setPosition( font[65].x(), font[65].y() + 4 );
            updateNormalFontLetterShadow( font[171] );

            // Right-pointing double angle quotation mark >>.
            font[187].resize( 8, 9 );
            font[187].reset();
            fheroes2::Flip( font[171], 1, 0, font[187], 1, 0, 7, 7, true, false );
            // Remove old shadows
            fheroes2::FillTransform( font[187], 3, 6, 1, 1, 1 );
            fheroes2::FillTransform( font[187], 7, 6, 1, 1, 1 );
            fheroes2::FillTransform( font[187], 4, 5, 1, 1, 1 );
            font[187].setPosition( font[65].x(), font[65].y() + 4 );
            updateNormalFontLetterShadow( font[187] );

            // Inverted question mark ?.
            font[191].resize( font[63].width() + 1, font[63].height() );
            font[191].reset();
            fheroes2::Copy( font[63], 1, 0, font[191], 0, 0, font[63].width(), 11 );
            {
                const fheroes2::Image temp = fheroes2::Flip( font[191], true, true );
                fheroes2::Copy( temp, 1, 2, font[191], 0, 0, temp.width(), temp.height() );
            }
            // Remove old shadows
            fheroes2::FillTransform( font[191], 6, 1, 1, 2, 1 );
            fheroes2::FillTransform( font[191], 5, 2, 1, 1, 1 );
            fheroes2::FillTransform( font[191], 2, 4, 1, 1, 1 );
            fheroes2::FillTransform( font[191], 3, 8, 5, 1, 1 );
            fheroes2::FillTransform( font[191], 4, 7, 3, 1, 1 );
            fheroes2::FillTransform( font[191], 8, 5, 1, 1, 1 );
            font[191].setPosition( font[63].x(), font[63].y() + 2 );
            updateNormalFontLetterShadow( font[191] );
            // Improve generated shadows
            fheroes2::FillTransform( font[191], 0, 8, 1, 1, 1 );
            fheroes2::FillTransform( font[191], 0, 12, 1, 1, 1 );
            fheroes2::FillTransform( font[191], 7, 12, 1, 1, 1 );

            // A with grave accent ` and generate the grave accent for further use.
            font[192].resize( font[65].width(), font[65].height() + 3 );
            font[192].reset();
            fheroes2::Copy( font[65], 0, 0, font[192], 0, 3, font[65].width(), font[65].height() );
            font[192].setPosition( font[65].x(), font[65].y() - 3 );
            fheroes2::Copy( font[192], 3, 4, font[192], 7, 0, 1, 1 );
            fheroes2::Copy( font[192], 4, 4, font[192], 8, 0, 1, 1 );
            fheroes2::Copy( font[192], 3, 4, font[192], 8, 1, 1, 1 );
            fheroes2::Copy( font[192], 4, 4, font[192], 9, 1, 1, 1 );
            fheroes2::Copy( font[192], 3, 3, font[192], 10, 1, 1, 1 );
            updateNormalFontLetterShadow( font[192] );

            // A with acute accent. Generate the accent for further use.
            font[193].resize( font[65].width(), font[65].height() + 3 );
            font[193].reset();
            fheroes2::Copy( font[65], 0, 0, font[193], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[193], 3, 4, font[193], 10, 0, 1, 1 );
            fheroes2::Copy( font[193], 4, 4, font[193], 9, 0, 1, 1 );
            fheroes2::Copy( font[193], 3, 4, font[193], 9, 1, 1, 1 );
            fheroes2::Copy( font[193], 4, 4, font[193], 8, 1, 1, 1 );
            fheroes2::Copy( font[193], 3, 3, font[193], 7, 1, 1, 1 );
            font[193].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[193] );

            // A with circumflex accent. Generation of accent for further use.
            font[194].resize( font[65].width(), font[65].height() + 3 );
            font[194].reset();
            fheroes2::Copy( font[65], 0, 0, font[194], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[97], 7, 1, font[194], 7, 1, 1, 1 );
            fheroes2::Copy( font[97], 7, 1, font[194], 8, 0, 1, 1 );
            fheroes2::Copy( font[97], 7, 1, font[194], 9, 1, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[194], 7, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[194], 9, 0, 1, 1 );
            font[194].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[194] );

            // A with tilde accent ~. Generate accent for further use.
            font[195].resize( font[65].width(), font[65].height() + 4 );
            font[195].reset();
            fheroes2::Copy( font[65], 0, 0, font[195], 0, 4, font[65].width(), font[65].height() );
            fheroes2::Copy( font[37], 7, 5, font[195], 4, 0, 5, 2 );
            fheroes2::Copy( font[37], 8, 8, font[195], 8, 2, 3, 1 );
            fheroes2::Copy( font[37], 10, 7, font[195], 10, 1, 2, 1 );
            font[195].setPosition( font[65].x(), font[65].y() - 4 );
            updateNormalFontLetterShadow( font[195] );

            // A with diaeresis.
            font[196].resize( font[65].width(), font[65].height() + 3 );
            font[196].reset();
            fheroes2::Copy( font[65], 0, 0, font[196], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[196], 3, 1 + 3, font[196], 4, 0, 1, 1 );
            fheroes2::Copy( font[196], 4, 1 + 3, font[196], 5, 0, 1, 1 );
            fheroes2::Copy( font[196], 4, 0, font[196], 9, 0, 2, 1 );
            font[196].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[196] );

            // A with circle on top.
            font[197].resize( font[65].width(), font[65].height() + 2 );
            font[197].reset();
            fheroes2::Copy( font[65], 0, 3, font[197], 0, 5, font[65].width(), font[65].height() );
            fheroes2::FillTransform( font[197], 2, 5, 3, 3, 1 );
            fheroes2::Copy( font[112], 5, 6, font[197], 7, 4, 4, 1 );
            fheroes2::Copy( font[112], 5, 6, font[197], 7, 0, 4, 1 );
            fheroes2::Copy( font[112], 5, 6, font[197], 7, 2, 4, 1 );
            fheroes2::Copy( font[116], 1, 0, font[197], 7, 1, 1, 1 );
            fheroes2::Copy( font[116], 1, 0, font[197], 10, 1, 1, 1 );
            font[197].setPosition( font[65].x(), font[65].y() - 2 );
            updateNormalFontLetterShadow( font[197] );

            // A attached to E.
            font[198].resize( font[65].width() + 3, font[65].height() );
            font[198].reset();
            fheroes2::Copy( font[65], 0, 0, font[198], 0, 0, font[65].width(), font[65].height() );
            fheroes2::Copy( font[69], 6, 0, font[198], 12, 0, 6, 4 );
            fheroes2::Copy( font[69], 5, 0, font[198], 10, 0, 2, 2 );
            fheroes2::Copy( font[69], 6, 4, font[198], 12, 4, 3, 2 );
            fheroes2::Copy( font[69], 6, 4, font[198], 15, 4, 1, 2 );
            fheroes2::Copy( font[69], 8, 9, font[198], 14, 9, 3, 2 );
            font[198].setPosition( font[65].x(), font[65].y() );
            updateNormalFontLetterShadow( font[198] );

            // C with cedilla.
            font[199].resize( font[67].width(), font[67].height() + 3 );
            font[199].reset();
            fheroes2::Copy( font[67], 0, 0, font[199], 0, 0, font[67].width(), font[67].height() );
            fheroes2::Copy( font[99], 2, 1, font[199], 7, 11, 1, 1 );
            fheroes2::Copy( font[99], 5, 6, font[199], 8, 11, 1, 1 );
            fheroes2::Copy( font[99], 2, 6, font[199], 9, 11, 1, 1 );
            fheroes2::Copy( font[99], 2, 1, font[199], 8, 12, 1, 1 );
            fheroes2::Copy( font[99], 5, 6, font[199], 9, 12, 1, 1 );
            fheroes2::Copy( font[99], 2, 1, font[199], 7, 13, 1, 1 );
            fheroes2::Copy( font[99], 5, 6, font[199], 8, 13, 1, 1 );
            fheroes2::Copy( font[99], 3, 0, font[199], 9, 13, 1, 1 );
            font[199].setPosition( font[67].x(), font[67].y() );
            updateNormalFontLetterShadow( font[199] );

            // E with grave accent `.
            font[200].resize( font[69].width(), font[69].height() + 3 );
            font[200].reset();
            fheroes2::Copy( font[69], 0, 0, font[200], 0, 3, font[69].width(), font[69].height() );
            font[200].setPosition( font[69].x(), font[69].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[200], 4, 0, 4, 2 );
            updateNormalFontLetterShadow( font[200] );

            // E with acute accent.
            font[201].resize( font[69].width(), font[69].height() + 3 );
            font[201].reset();
            fheroes2::Copy( font[69], 0, 0, font[201], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[193], 7, 0, font[201], 5, 0, 4, 2 );
            font[201].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[201] );

            // E with circumflex accent.
            font[202].resize( font[69].width(), font[69].height() + 3 );
            font[202].reset();
            fheroes2::Copy( font[69], 0, 0, font[202], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[194], 7, 0, font[202], 5, 0, 3, 2 );
            font[202].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[202] );

            // E with diaeresis.
            font[203].resize( font[69].width(), font[69].height() + 3 );
            font[203].reset();
            fheroes2::Copy( font[69], 0, 0, font[203], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[196], 4, 0, font[203], 3, 0, 7, 1 );
            font[203].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[203] );

            // I with grave accent `.
            font[204].resize( font[73].width(), font[73].height() + 3 );
            font[204].reset();
            fheroes2::Copy( font[73], 0, 0, font[204], 0, 3, font[73].width(), font[73].height() );
            font[204].setPosition( font[73].x(), font[73].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[204], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[204] );

            // I with acute accent.
            font[205].resize( font[73].width(), font[73].height() + 3 );
            font[205].reset();
            fheroes2::Copy( font[73], 0, 0, font[205], 0, 3, font[73].width(), font[73].height() );
            fheroes2::Copy( font[193], 7, 0, font[205], 3, 0, 4, 2 );
            font[205].setPosition( font[73].x(), font[73].y() - 3 );
            updateNormalFontLetterShadow( font[205] );

            // I with circumflex.
            font[206].resize( font[73].width(), font[73].height() + 3 );
            font[206].reset();
            fheroes2::Copy( font[73], 0, 0, font[206], 0, 3, font[73].width(), font[73].height() );
            fheroes2::Copy( font[194], 7, 0, font[206], 3, 0, 3, 2 );
            font[206].setPosition( font[73].x(), font[73].y() - 3 );
            updateNormalFontLetterShadow( font[206] );

            // I with diaeresis.
            font[207].resize( font[73].width() + 1, font[73].height() + 3 );
            font[207].reset();
            fheroes2::Copy( font[73], 0, 0, font[207], 0, 3, font[73].width(), font[73].height() );
            fheroes2::Copy( font[196], 4, 0, font[207], 1, 0, 7, 1 );
            font[207].setPosition( font[73].x(), font[73].y() - 3 );
            updateNormalFontLetterShadow( font[207] );

            // N with tilde ~.
            font[209].resize( font[78].width(), font[78].height() + 3 );
            font[209].reset();
            fheroes2::Copy( font[78], 0, 0, font[209], 0, 3, font[78].width(), font[78].height() );
            fheroes2::Copy( font[195], 4, 0, font[209], 4, 0, 8, 3 );
            font[209].setPosition( font[78].x(), font[78].y() - 3 );
            updateNormalFontLetterShadow( font[209] );

            // O with grave accent `.
            font[210].resize( font[79].width(), font[79].height() + 3 );
            font[210].reset();
            fheroes2::Copy( font[79], 0, 0, font[210], 0, 3, font[79].width(), font[79].height() );
            font[210].setPosition( font[79].x(), font[79].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[210], 6, 0, 4, 2 );
            updateNormalFontLetterShadow( font[210] );

            // O with acute accent.
            font[211].resize( font[79].width(), font[79].height() + 3 );
            font[211].reset();
            fheroes2::Copy( font[79], 0, 0, font[211], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[193], 7, 0, font[211], 7, 0, 4, 2 );
            font[211].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[211] );

            // O with circumflex accent.
            font[212].resize( font[79].width(), font[79].height() + 3 );
            font[212].reset();
            fheroes2::Copy( font[79], 0, 0, font[212], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[194], 7, 0, font[212], 7, 0, 3, 2 );
            font[212].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[212] );

            // O with tilde accent ~.
            font[213].resize( font[79].width(), font[79].height() + 4 );
            font[213].reset();
            fheroes2::Copy( font[79], 0, 0, font[213], 0, 4, font[79].width(), font[79].height() );
            fheroes2::Copy( font[195], 4, 0, font[213], 4, 0, 8, 3 );
            font[213].setPosition( font[79].x(), font[79].y() - 4 );
            updateNormalFontLetterShadow( font[213] );

            // O with diaeresis.
            font[214].resize( font[79].width(), font[79].height() + 3 );
            font[214].reset();
            fheroes2::Copy( font[79], 0, 0, font[214], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[196], 4, 0, font[214], 5, 0, 7, 1 );
            font[214].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[214] );

            // O with / inside.
            font[216].resize( font[79].width() + 2, font[79].height() );
            font[216].reset();
            fheroes2::Copy( font[79], 0, 0, font[216], 0, 0, font[79].width(), font[79].height() );
            fheroes2::Copy( font[88], 10, 0, font[216], 6, 3, 5, 5 );
            fheroes2::Copy( font[88], 13, 0, font[216], font[79].width() - 1, 0, 3, 2 );
            fheroes2::Copy( font[79], font[79].width() - 3, 1, font[216], font[79].width() - 3, 2, 2, 1 );
            fheroes2::Copy( font[65], 1, 7, font[216], 1, 7, 2, 4 );
            fheroes2::Copy( font[88], 10, 3, font[216], 3, font[79].height() - 4, 2, 2 );
            font[216].setPosition( font[79].x(), font[79].y() );
            updateNormalFontLetterShadow( font[216] );

            // U with grave accent `.
            font[217].resize( font[85].width(), font[85].height() + 3 );
            font[217].reset();
            fheroes2::Copy( font[85], 0, 0, font[217], 0, 3, font[85].width(), font[85].height() );
            font[217].setPosition( font[85].x(), font[85].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[217], 5, 0, 4, 2 );
            updateNormalFontLetterShadow( font[217] );

            // U with acute accent.
            font[218].resize( font[85].width(), font[85].height() + 3 );
            font[218].reset();
            fheroes2::Copy( font[85], 0, 0, font[218], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[193], 7, 0, font[218], 5, 0, 4, 2 );
            font[218].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[218] );

            // U with circumflex.
            font[219].resize( font[85].width(), font[85].height() + 3 );
            font[219].reset();
            fheroes2::Copy( font[85], 0, 0, font[219], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[194], 7, 0, font[219], 6, 0, 3, 2 );
            font[219].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[219] );

            // U with diaeresis.
            font[220].resize( font[85].width(), font[85].height() + 3 );
            font[220].reset();
            fheroes2::Copy( font[85], 0, 0, font[220], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[196], 4, 0, font[220], 4, 0, 7, 1 );
            font[220].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[220] );

            // Eszett.
            font[223].resize( font[66].width(), font[66].height() + 3 );
            font[223].reset();
            fheroes2::Copy( font[66], 1, 0, font[223], 0, 3, font[66].width() - 1, font[66].height() );
            fheroes2::FillTransform( font[223], 0, 0 + 3, 3, 10, 1 );
            fheroes2::Copy( font[223], 0, 0 + 3, font[223], 3, 0 + 3, 1, 1 );
            fheroes2::Copy( font[223], 3, 1 + 3, font[223], 4, 0 + 3, 1, 1 );
            fheroes2::Copy( font[223], 4, 3 + 3, font[223], 3, 7 + 3, 1, 3 );
            fheroes2::Copy( font[223], 4, 3 + 3, font[223], 2, 10 + 3, 1, 1 );
            fheroes2::Copy( font[223], 3, 3 + 3, font[223], 2, 7 + 3, 1, 3 );
            fheroes2::Copy( font[223], 3, 6 + 3, font[223], 1, 10 + 3, 1, 1 );
            fheroes2::Copy( font[223], 7, 4 + 3, font[223], 4, 8 + 3, 1, 1 );
            fheroes2::Copy( font[223], 7, 4 + 3, font[223], 3, 10 + 3, 1, 1 );
            fheroes2::Copy( font[223], 8, 5 + 3, font[223], 4, 9 + 3, 1, 1 );
            font[223].setPosition( font[66].x(), font[66].y() - 3 );
            updateNormalFontLetterShadow( font[223] );

            // a with grave accent `.
            font[224].resize( font[97].width(), font[97].height() + 3 );
            font[224].reset();
            fheroes2::Copy( font[97], 0, 0, font[224], 0, 3, font[97].width(), font[97].height() );
            font[224].setPosition( font[97].x(), font[97].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[224], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[224] );

            // a with acute accent.
            font[225].resize( font[97].width(), font[97].height() + 3 );
            font[225].reset();
            fheroes2::Copy( font[97], 0, 0, font[225], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[193], 7, 0, font[225], 3, 0, 4, 2 );
            font[225].setPosition( font[97].x(), font[97].y() - 3 );
            updateNormalFontLetterShadow( font[225] );

            // a with circumflex accent.
            font[226].resize( font[97].width(), font[97].height() + 3 );
            font[226].reset();
            fheroes2::Copy( font[97], 0, 0, font[226], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[194], 7, 0, font[226], 3, 0, 3, 2 );
            font[226].setPosition( font[97].x(), font[97].y() - 3 );
            updateNormalFontLetterShadow( font[226] );

            // a with tilde accent ~.
            font[227].resize( font[97].width(), font[97].height() + 4 );
            font[227].reset();
            fheroes2::Copy( font[97], 0, 0, font[227], 0, 4, font[97].width(), font[97].height() );
            fheroes2::Copy( font[195], 4, 0, font[227], 1, 0, 8, 3 );
            font[227].setPosition( font[97].x(), font[97].y() - 4 );
            updateNormalFontLetterShadow( font[227] );

            // a with diaeresis.
            font[228].resize( font[97].width(), font[97].height() + 3 );
            font[228].reset();
            fheroes2::Copy( font[97], 0, 0, font[228], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[228], 3, 0 + 3, font[228], 3, 0, 1, 1 );
            fheroes2::Copy( font[228], 2, 1 + 3, font[228], 3, 1, 1, 1 );
            fheroes2::Copy( font[228], 2, 1 + 3, font[228], 2, 0, 1, 1 );
            fheroes2::Copy( font[228], 1, 0 + 3, font[228], 2, 1, 1, 1 );
            fheroes2::Copy( font[228], 2, 0, font[228], 5, 0, 2, 2 );
            font[228].setPosition( font[97].x(), font[97].y() - 3 );
            updateNormalFontLetterShadow( font[228] );

            // a with circle on top.
            font[229].resize( font[97].width(), font[97].height() + 5 );
            font[229].reset();
            fheroes2::Copy( font[97], 0, 0, font[229], 0, 4, font[97].width(), font[97].height() );
            fheroes2::Copy( font[197], 7, 0, font[229], 2, 0, 1, 3 );
            fheroes2::Copy( font[197], 10, 0, font[229], 6, 0, 1, 3 );
            fheroes2::Copy( font[97], 2, 0, font[229], 3, 0, 3, 1 );
            fheroes2::Copy( font[97], 2, 0, font[229], 3, 2, 3, 1 );
            fheroes2::Copy( font[101], 3, 2, font[229], 3, 1, 3, 1 );
            font[229].setPosition( font[97].x(), font[97].y() - 4 );
            updateNormalFontLetterShadow( font[229] );

            // a attached to e.
            font[230].resize( font[97].width() + 4, font[97].height() );
            font[230].reset();
            fheroes2::Copy( font[97], 0, 0, font[230], 0, 0, font[97].width(), font[97].height() );
            fheroes2::Copy( font[101], 3, 0, font[230], 8, 0, 6, 8 );
            font[230].setPosition( font[97].x(), font[97].y() );
            updateNormalFontLetterShadow( font[230] );

            // c with cedilla.
            font[231].resize( font[99].width(), font[99].height() + 3 );
            font[231].reset();
            fheroes2::Copy( font[99], 0, 0, font[231], 0, 0, font[99].width(), font[99].height() );
            fheroes2::Copy( font[199], 7, 11, font[231], 4, 7, 3, 3 );
            font[231].setPosition( font[99].x(), font[99].y() );
            updateNormalFontLetterShadow( font[231] );

            // e with grave accent `.
            font[232].resize( font[101].width(), font[101].height() + 3 );
            font[232].reset();
            fheroes2::Copy( font[101], 0, 0, font[232], 0, 3, font[101].width(), font[101].height() );
            font[232].setPosition( font[101].x(), font[101].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[232], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[232] );

            // e with acute accent.
            font[233].resize( font[101].width(), font[101].height() + 3 );
            font[233].reset();
            fheroes2::Copy( font[101], 0, 0, font[233], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[193], 7, 0, font[233], 3, 0, 4, 2 );
            font[233].setPosition( font[101].x(), font[101].y() - 3 );
            updateNormalFontLetterShadow( font[233] );

            // e with circumflex accent.
            font[234].resize( font[101].width(), font[101].height() + 3 );
            font[234].reset();
            fheroes2::Copy( font[101], 0, 0, font[234], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[194], 7, 0, font[234], 4, 0, 3, 2 );
            font[234].setPosition( font[101].x(), font[101].y() - 3 );
            updateNormalFontLetterShadow( font[234] );

            // e with diaeresis.
            font[235].resize( font[101].width(), font[101].height() + 3 );
            font[235].reset();
            fheroes2::Copy( font[101], 0, 0, font[235], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[228], 2, 0, font[235], 3, 0, 5, 2 );
            font[235].setPosition( font[101].x(), font[101].y() - 3 );
            updateNormalFontLetterShadow( font[235] );

            // i with grave accent `.
            font[236] = font[105];
            fheroes2::FillTransform( font[236], 0, 0, font[236].width(), 3, 1 );
            fheroes2::Copy( font[192], 7, 0, font[236], 1, 0, 4, 2 );
            updateNormalFontLetterShadow( font[236] );

            // i with acute accent.
            font[237] = font[105];
            fheroes2::FillTransform( font[237], 0, 0, font[237].width(), 3, 1 );
            fheroes2::Copy( font[193], 7, 0, font[237], 1, 0, 4, 2 );
            updateNormalFontLetterShadow( font[237] );

            // i with circumflex.
            font[238] = font[105];
            fheroes2::FillTransform( font[238], 0, 0, font[238].width(), 3, 1 );
            fheroes2::Copy( font[194], 7, 0, font[238], 1, 0, 3, 2 );
            updateNormalFontLetterShadow( font[238] );

            // i with diaeresis.
            font[239].resize( font[105].width() + 1, font[105].height() );
            font[239].reset();
            fheroes2::Copy( font[105], 0, 0, font[239], 1, 0, font[105].width(), font[105].height() );
            fheroes2::FillTransform( font[239], 1, 0, font[239].width(), 3, 1 );
            fheroes2::Copy( font[228], 2, 0, font[239], 1, 0, 5, 2 );
            font[239].setPosition( font[105].x() + 1, font[105].y() );
            updateNormalFontLetterShadow( font[239] );

            // n with tilde ~.
            font[241].resize( font[110].width(), font[110].height() + 4 );
            font[241].reset();
            fheroes2::Copy( font[110], 0, 0, font[241], 0, 4, font[110].width(), font[110].height() );
            fheroes2::Copy( font[195], 4, 0, font[241], 1, 0, 8, 3 );
            font[241].setPosition( font[110].x(), font[110].y() - 4 );
            updateNormalFontLetterShadow( font[241] );

            // o with grave accent `.
            font[242].resize( font[111].width(), font[111].height() + 3 );
            font[242].reset();
            fheroes2::Copy( font[111], 0, 0, font[242], 0, 3, font[111].width(), font[111].height() );
            font[242].setPosition( font[111].x(), font[111].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[242], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[242] );

            // o with acute accent.
            font[243].resize( font[111].width(), font[111].height() + 3 );
            font[243].reset();
            fheroes2::Copy( font[111], 0, 0, font[243], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[193], 7, 0, font[243], 3, 0, 4, 2 );
            font[243].setPosition( font[111].x(), font[111].y() - 3 );
            updateNormalFontLetterShadow( font[243] );

            // o with circumflex accent.
            font[244].resize( font[111].width(), font[111].height() + 3 );
            font[244].reset();
            fheroes2::Copy( font[111], 0, 0, font[244], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[194], 7, 0, font[244], 4, 0, 3, 2 );
            font[244].setPosition( font[111].x(), font[111].y() - 3 );
            updateNormalFontLetterShadow( font[244] );

            // o with tilde accent ~.
            font[245].resize( font[111].width(), font[111].height() + 4 );
            font[245].reset();
            fheroes2::Copy( font[111], 0, 0, font[245], 0, 4, font[111].width(), font[111].height() );
            fheroes2::Copy( font[195], 4, 0, font[245], 1, 0, 8, 3 );
            font[245].setPosition( font[111].x(), font[111].y() - 4 );
            updateNormalFontLetterShadow( font[245] );

            // o with diaeresis.
            font[246].resize( font[111].width(), font[111].height() + 3 );
            font[246].reset();
            fheroes2::Copy( font[111], 0, 0, font[246], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[228], 2, 0, font[246], 2, 0, 2, 2 );
            fheroes2::Copy( font[228], 2, 0, font[246], 6, 0, 2, 2 );
            font[246].setPosition( font[111].x(), font[111].y() - 3 );
            updateNormalFontLetterShadow( font[246] );

            // o with / inside.
            font[248].resize( font[111].width(), font[111].height() );
            font[248].reset();
            fheroes2::Copy( font[111], 0, 0, font[248], 0, 0, font[111].width(), font[111].height() );
            fheroes2::Copy( font[120], 5, 0, font[248], 3, 2, 4, 4 );
            fheroes2::Copy( font[111], 6, 5, font[248], 6, 5, 1, 1 );
            fheroes2::Copy( font[120], 1, 6, font[248], 1, 6, 2, 1 );
            fheroes2::Copy( font[120], 7, 0, font[248], 7, 0, 2, 1 );
            font[248].setPosition( font[111].x(), font[111].y() );
            updateNormalFontLetterShadow( font[248] );

            // u with grave accent `.
            font[249].resize( font[117].width(), font[117].height() + 3 );
            font[249].reset();
            fheroes2::Copy( font[117], 0, 0, font[249], 0, 3, font[117].width(), font[117].height() );
            font[249].setPosition( font[117].x(), font[117].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[249], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[249] );

            // u with acute accent.
            font[250].resize( font[117].width(), font[117].height() + 3 );
            font[250].reset();
            fheroes2::Copy( font[117], 0, 0, font[250], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[193], 7, 0, font[250], 3, 0, 4, 2 );
            font[250].setPosition( font[117].x(), font[117].y() - 3 );
            updateNormalFontLetterShadow( font[250] );

            // u with circumflex.
            font[251].resize( font[117].width(), font[117].height() + 3 );
            font[251].reset();
            fheroes2::Copy( font[117], 0, 0, font[251], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[194], 7, 0, font[251], 4, 0, 3, 2 );
            font[251].setPosition( font[117].x(), font[117].y() - 3 );
            updateNormalFontLetterShadow( font[251] );

            // u with diaeresis.
            font[252].resize( font[117].width(), font[117].height() + 3 );
            font[252].reset();
            fheroes2::Copy( font[117], 0, 0, font[252], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[246], 2, 0, font[252], 2, 0, 6, 2 );
            font[252].setPosition( font[117].x(), font[117].y() - 3 );
            updateNormalFontLetterShadow( font[252] );
        }
        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // NBSP character.
            font[160].resize( smallFontSpaceWidth, 1 );
            font[160].reset();

            // Inverted exclamation mark !.
            font[161].resize( font[33].width(), font[33].height() );
            font[161].reset();
            fheroes2::Copy( font[33], 1, 0, font[161], 1, 0, font[33].width(), 7 );
            {
                const fheroes2::Image temp = fheroes2::Flip( font[161], false, true );
                fheroes2::Copy( temp, 0, 1, font[161], 0, 0, temp.width(), temp.height() );
            }
            font[161].setPosition( font[33].x(), font[33].y() + 2 );
            updateSmallFontLetterShadow( font[161] );

            // Inverted question mark ?.
            font[191].resize( font[63].width(), font[63].height() );
            font[191].reset();
            fheroes2::Copy( font[63], 1, 0, font[191], 0, 0, font[63].width(), 7 );
            {
                const fheroes2::Image temp = fheroes2::Flip( font[191], true, true );
                fheroes2::Copy( temp, 0, 1, font[191], 0, 0, temp.width(), temp.height() );
            }
            // Remove old shadows
            fheroes2::FillTransform( font[191], 4, 1, 1, 1, 1 );
            fheroes2::FillTransform( font[191], 3, 5, 2, 1, 1 );
            fheroes2::FillTransform( font[191], 2, 4, 1, 1, 1 );
            font[191].setPosition( font[63].x(), font[63].y() + 2 );
            updateSmallFontLetterShadow( font[191] );

            // A with grave accent `. Generate grave accent for further use.
            font[192].resize( font[65].width(), font[65].height() + 3 );
            font[192].reset();
            fheroes2::Copy( font[65], 0, 0, font[192], 0, 4, font[65].width(), font[65].height() );
            fheroes2::Copy( font[192], 4, 4, font[192], 4, 0, 1, 1 );
            fheroes2::Copy( font[192], 4, 4, font[192], 5, 1, 1, 1 );
            fheroes2::Copy( font[192], 4, 4, font[192], 6, 2, 1, 1 );
            font[192].setPosition( font[65].x(), font[65].y() - 4 );
            updateSmallFontLetterShadow( font[192] );

            // A with acute accent. Generate acute accent for further use
            font[193].resize( font[65].width(), font[65].height() + 4 );
            font[193].reset();
            fheroes2::Copy( font[65], 0, 0, font[193], 0, 4, font[65].width(), font[65].height() );
            fheroes2::Copy( font[65], 4, 0, font[193], 4, 2, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[193], 5, 1, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[193], 6, 0, 1, 1 );
            font[193].setPosition( font[65].x(), font[65].y() - 4 );
            updateSmallFontLetterShadow( font[193] );

            // A with circumflex accent. Generate accent for further use.
            font[194].resize( font[65].width(), font[65].height() + 3 );
            font[194].reset();
            fheroes2::Copy( font[65], 0, 0, font[194], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[65], 4, 0, font[194], 4, 1, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[194], 5, 0, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[194], 6, 1, 1, 1 );
            font[194].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[194] );

            // A with tilde accent ~. Generate for further use.
            font[195].resize( font[65].width(), font[65].height() + 3 );
            font[195].reset();
            fheroes2::Copy( font[65], 0, 0, font[195], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[37], 5, 4, font[195], 3, 0, 3, 2 );
            fheroes2::Copy( font[37], 5, 4, font[195], 6, 1, 2, 2 );
            font[195].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[195] );

            // A with diaeresis. Generate for later use.
            font[196].resize( font[65].width(), font[65].height() + 2 );
            font[196].reset();
            fheroes2::Copy( font[65], 0, 0, font[196], 0, 2, font[65].width(), font[65].height() );
            fheroes2::Copy( font[196], 3, 0 + 2, font[196], 4, 0, 1, 1 );
            fheroes2::Copy( font[196], 3, 0 + 2, font[196], 6, 0, 1, 1 );
            font[196].setPosition( font[65].x(), font[65].y() - 2 );
            updateSmallFontLetterShadow( font[196] );

            // A with circle on top.
            font[197].resize( font[65].width(), font[65].height() + 4 );
            font[197].reset();
            fheroes2::Copy( font[65], 0, 0, font[197], 0, 2, font[65].width(), font[65].height() );
            fheroes2::FillTransform( font[197], 3, 2, 3, 1, 1 );
            fheroes2::FillTransform( font[197], 1, 2, 3, 3, 1 );
            // Generate circle for further use.
            fheroes2::Copy( font[101], 2, 0, font[197], 5, 0, 1, 1 );
            fheroes2::Copy( font[101], 2, 0, font[197], 6, 1, 1, 1 );
            fheroes2::Copy( font[101], 2, 0, font[197], 5, 2, 1, 1 );
            fheroes2::Copy( font[101], 2, 0, font[197], 4, 1, 1, 1 );
            font[197].setPosition( font[65].x(), font[65].y() - 2 );
            updateSmallFontLetterShadow( font[197] );

            // A attached to E.
            font[198].resize( font[65].width() + 3, font[65].height() );
            font[198].reset();
            fheroes2::Copy( font[65], 0, 0, font[198], 0, 0, font[65].width(), font[65].height() );
            fheroes2::Copy( font[69], 3, 0, font[198], 6, 0, 5, 4 );
            fheroes2::Copy( font[69], 1, 0, font[198], 9, 5, 2, 2 );
            font[198].setPosition( font[65].x(), font[65].y() );
            updateSmallFontLetterShadow( font[198] );

            // C with cedilla.
            font[199].resize( font[67].width(), font[67].height() + 3 );
            font[199].reset();
            fheroes2::Copy( font[67], 0, 0, font[199], 0, 0, font[67].width(), font[67].height() );
            fheroes2::Copy( font[67], 1, 1, font[199], 3, 7, 2, 2 );
            font[199].setPosition( font[67].x(), font[67].y() );
            updateSmallFontLetterShadow( font[199] );

            // E with grave accent `.
            font[200].resize( font[69].width(), font[69].height() + 4 );
            font[200].reset();
            fheroes2::Copy( font[69], 0, 0, font[200], 0, 4, font[69].width(), font[69].height() );
            fheroes2::Copy( font[192], 4, 0, font[200], 4, 0, 3, 3 );
            font[200].setPosition( font[69].x(), font[69].y() - 4 );
            updateSmallFontLetterShadow( font[200] );

            // E with acute accent.
            font[201].resize( font[69].width(), font[69].height() + 4 );
            font[201].reset();
            fheroes2::Copy( font[69], 0, 0, font[201], 0, 4, font[69].width(), font[69].height() );
            fheroes2::Copy( font[193], 4, 0, font[201], 3, 0, 3, 3 );
            font[201].setPosition( font[69].x(), font[69].y() - 4 );
            updateSmallFontLetterShadow( font[201] );

            // E with circumflex accent.
            font[202].resize( font[69].width(), font[69].height() + 3 );
            font[202].reset();
            fheroes2::Copy( font[69], 0, 0, font[202], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[194], 4, 0, font[202], 3, 0, 3, 2 );
            font[202].setPosition( font[69].x(), font[69].y() - 3 );
            updateSmallFontLetterShadow( font[202] );

            // E with diaeresis.
            font[203].resize( font[69].width(), font[69].height() + 2 );
            font[203].reset();
            fheroes2::Copy( font[69], 0, 0, font[203], 0, 2, font[69].width(), font[69].height() );
            fheroes2::Copy( font[196], 4, 0, font[203], 3, 0, 3, 1 );
            font[203].setPosition( font[69].x(), font[69].y() - 2 );
            updateSmallFontLetterShadow( font[203] );

            // I with grave accent `.
            font[204].resize( font[73].width(), font[73].height() + 4 );
            font[204].reset();
            fheroes2::Copy( font[73], 0, 0, font[204], 0, 4, font[73].width(), font[73].height() );
            fheroes2::Copy( font[192], 4, 0, font[204], 1, 0, 3, 3 );
            font[204].setPosition( font[73].x(), font[73].y() - 4 );
            updateSmallFontLetterShadow( font[204] );

            // I with acute accent.
            font[205].resize( font[73].width(), font[73].height() + 4 );
            font[205].reset();
            fheroes2::Copy( font[73], 0, 0, font[205], 0, 4, font[73].width(), font[73].height() );
            fheroes2::Copy( font[193], 4, 0, font[205], 1, 0, 3, 3 );
            font[205].setPosition( font[73].x(), font[73].y() - 4 );
            updateSmallFontLetterShadow( font[205] );

            // I with circumflex
            font[206].resize( font[73].width() + 1, font[73].height() + 3 );
            font[206].reset();
            fheroes2::Copy( font[73], 0, 0, font[206], 0, 3, font[73].width(), font[73].height() );
            fheroes2::Copy( font[194], 4, 0, font[206], 2, 0, 3, 2 );
            font[206].setPosition( font[73].x(), font[73].y() - 3 );
            updateSmallFontLetterShadow( font[206] );

            // I with diaeresis
            font[207].resize( font[73].width() + 1, font[73].height() + 2 );
            font[207].reset();
            fheroes2::Copy( font[73], 0, 0, font[207], 0, 2, font[73].width(), font[73].height() );
            fheroes2::Copy( font[196], 4, 0, font[207], 2, 0, 3, 1 );
            font[207].setPosition( font[73].x(), font[73].y() - 2 );
            updateSmallFontLetterShadow( font[207] );

            // N with tilde ~.
            font[209].resize( font[78].width(), font[78].height() + 3 );
            font[209].reset();
            fheroes2::Copy( font[78], 0, 0, font[209], 0, 4, font[78].width(), font[78].height() );
            fheroes2::Copy( font[195], 3, 0, font[209], 4, 0, 5, 3 );
            font[209].setPosition( font[78].x(), font[78].y() - 4 );
            updateSmallFontLetterShadow( font[209] );

            // O with grave accent `.
            font[210].resize( font[79].width(), font[79].height() + 4 );
            font[210].reset();
            fheroes2::Copy( font[79], 0, 0, font[210], 0, 4, font[79].width(), font[79].height() );
            fheroes2::Copy( font[192], 4, 0, font[210], 4, 0, 3, 3 );
            font[210].setPosition( font[79].x(), font[79].y() - 4 );
            updateSmallFontLetterShadow( font[210] );

            // O with acute accent.
            font[211].resize( font[79].width(), font[79].height() + 4 );
            font[211].reset();
            fheroes2::Copy( font[79], 0, 0, font[211], 0, 4, font[79].width(), font[79].height() );
            fheroes2::Copy( font[193], 4, 0, font[211], 3, 0, 3, 3 );
            font[211].setPosition( font[79].x(), font[79].y() - 4 );
            updateSmallFontLetterShadow( font[211] );

            // O with circumflex accent.
            font[212].resize( font[79].width(), font[79].height() + 3 );
            font[212].reset();
            fheroes2::Copy( font[79], 0, 0, font[212], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[194], 4, 0, font[212], 3, 0, 3, 2 );
            font[212].setPosition( font[79].x(), font[79].y() - 3 );
            updateSmallFontLetterShadow( font[212] );

            // O with tilde accent ~.
            font[213].resize( font[79].width(), font[79].height() + 3 );
            font[213].reset();
            fheroes2::Copy( font[79], 0, 0, font[213], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[195], 3, 0, font[213], 2, 0, 5, 3 );
            font[213].setPosition( font[79].x(), font[79].y() - 3 );
            updateSmallFontLetterShadow( font[213] );

            // O with diaeresis.
            font[214].resize( font[79].width(), font[79].height() + 2 );
            font[214].reset();
            fheroes2::Copy( font[79], 0, 0, font[214], 0, 2, font[79].width(), font[79].height() );
            fheroes2::Copy( font[196], 4, 0, font[214], 3, 0, 3, 1 );
            font[214].setPosition( font[79].x(), font[79].y() - 2 );
            updateSmallFontLetterShadow( font[214] );

            // O with / inside.
            font[216].resize( font[79].width(), font[79].height() );
            font[216].reset();
            fheroes2::Copy( font[79], 0, 0, font[216], 0, 0, font[79].width(), font[79].height() );
            fheroes2::Copy( font[88], 6, 0, font[216], 3, 2, 3, 3 );
            fheroes2::Copy( font[88], 1, 0, font[216], font[79].width() - 1, 0, 1, 1 );
            fheroes2::Copy( font[88], 1, 0, font[216], 1, font[79].height() - 2, 1, 1 );
            font[216].setPosition( font[79].x(), font[79].y() );
            updateSmallFontLetterShadow( font[216] );

            // U with grave accent `.
            font[217].resize( font[85].width(), font[85].height() + 4 );
            font[217].reset();
            fheroes2::Copy( font[85], 0, 0, font[217], 0, 4, font[85].width(), font[85].height() );
            fheroes2::Copy( font[192], 4, 0, font[217], 4, 0, 3, 3 );
            font[217].setPosition( font[85].x(), font[85].y() - 4 );
            updateSmallFontLetterShadow( font[217] );

            // U with acute accent.
            font[218].resize( font[85].width(), font[85].height() + 4 );
            font[218].reset();
            fheroes2::Copy( font[85], 0, 0, font[218], 0, 4, font[85].width(), font[85].height() );
            fheroes2::Copy( font[193], 4, 0, font[218], 4, 0, 3, 3 );
            font[218].setPosition( font[85].x(), font[85].y() - 4 );
            updateSmallFontLetterShadow( font[218] );

            // U with circumflex.
            font[219].resize( font[85].width(), font[85].height() + 3 );
            font[219].reset();
            fheroes2::Copy( font[85], 0, 0, font[219], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[194], 4, 0, font[219], 4, 0, 3, 2 );
            font[219].setPosition( font[85].x(), font[85].y() - 3 );
            updateSmallFontLetterShadow( font[219] );

            // U with diaeresis.
            font[220].resize( font[85].width(), font[85].height() + 2 );
            font[220].reset();
            fheroes2::Copy( font[85], 0, 0, font[220], 0, 2, font[85].width(), font[85].height() );
            fheroes2::Copy( font[196], 4, 0, font[220], 4, 0, 3, 1 );
            font[220].setPosition( font[85].x(), font[85].y() - 2 );
            updateSmallFontLetterShadow( font[220] );

            // Eszett.
            font[223].resize( font[66].width(), font[66].height() + 2 );
            font[223].reset();
            fheroes2::Copy( font[66], 0, 0, font[223], 0, 2, font[66].width(), font[66].height() );
            fheroes2::FillTransform( font[223], 0, 0 + 2, 4, 9, 1 );
            fheroes2::Copy( font[223], 6, 1 + 2, font[223], 2, 1 + 2, 1, 5 );
            fheroes2::Copy( font[223], 4, 3 + 2, font[223], 2, 3 + 2, 2, 1 );
            fheroes2::Copy( font[223], 5, 0 + 2, font[223], 1, 6 + 2, 1, 1 );
            fheroes2::Copy( font[223], 5, 0 + 2, font[223], 3, 0 + 2, 1, 1 );
            fheroes2::Copy( font[223], 5, 0 + 2, font[223], 3, 6 + 2, 1, 1 );
            font[223].setPosition( font[66].x(), font[66].y() - 2 );
            updateSmallFontLetterShadow( font[223] );

            // a with grave accent `.
            font[224].resize( font[97].width(), font[97].height() + 3 );
            font[224].reset();
            fheroes2::Copy( font[97], 0, 0, font[224], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[192], 4, 0, font[224], 3, 0, 3, 2 );
            font[224].setPosition( font[97].x(), font[97].y() - 3 );
            updateSmallFontLetterShadow( font[224] );

            // a with acute accent.
            font[225].resize( font[97].width(), font[97].height() + 3 );
            font[225].reset();
            fheroes2::Copy( font[97], 0, 0, font[225], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[193], 5, 0, font[225], 3, 0, 2, 2 );
            font[225].setPosition( font[97].x(), font[97].y() - 3 );
            updateSmallFontLetterShadow( font[225] );

            // a with circumflex accent.
            font[226].resize( font[97].width(), font[97].height() + 3 );
            font[226].reset();
            fheroes2::Copy( font[97], 0, 0, font[226], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[194], 4, 0, font[226], 2, 0, 3, 2 );
            font[226].setPosition( font[97].x(), font[97].y() - 3 );
            updateSmallFontLetterShadow( font[226] );

            // a with tilde accent ~.
            font[227].resize( font[97].width(), font[97].height() + 4 );
            font[227].reset();
            fheroes2::Copy( font[97], 0, 0, font[227], 0, 4, font[97].width(), font[97].height() );
            fheroes2::Copy( font[195], 3, 0, font[227], 1, 0, 5, 3 );
            font[227].setPosition( font[97].x(), font[97].y() - 4 );
            updateSmallFontLetterShadow( font[227] );

            // a with diaeresis.
            font[228].resize( font[97].width(), font[97].height() + 2 );
            font[228].reset();
            fheroes2::Copy( font[97], 0, 0, font[228], 0, 2, font[97].width(), font[97].height() );
            fheroes2::Copy( font[228], 3, 0 + 2, font[228], 2, 0, 1, 1 );
            fheroes2::Copy( font[228], 3, 0 + 2, font[228], 5, 0, 1, 1 );
            font[228].setPosition( font[97].x(), font[97].y() - 2 );
            updateSmallFontLetterShadow( font[228] );

            // a with circle on top.
            font[229].resize( font[97].width(), font[97].height() + 3 );
            font[229].reset();
            fheroes2::Copy( font[97], 0, 0, font[229], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[197], 4, 0, font[229], 2, 0, 3, 3 );
            font[229].setPosition( font[97].x(), font[97].y() - 3 );
            updateSmallFontLetterShadow( font[229] );

            // a attached to e.
            font[230].resize( font[97].width() + 3, font[97].height() );
            font[230].reset();
            fheroes2::Copy( font[97], 0, 0, font[230], 0, 0, font[97].width(), font[97].height() );
            fheroes2::Copy( font[101], 2, 0, font[230], 6, 0, font[101].width() - 2, font[101].height() );
            font[230].setPosition( font[97].x(), font[97].y() );
            updateSmallFontLetterShadow( font[230] );

            // c with cedilla.
            font[231].resize( font[99].width(), font[99].height() + 3 );
            font[231].reset();
            fheroes2::Copy( font[99], 0, 0, font[231], 0, 0, font[99].width(), font[99].height() );
            fheroes2::Copy( font[199], 1, 1, font[231], 2, 5, 2, 2 );
            font[231].setPosition( font[99].x(), font[99].y() );
            updateSmallFontLetterShadow( font[231] );

            // e with grave accent `.
            font[232].resize( font[101].width(), font[101].height() + 3 );
            font[232].reset();
            fheroes2::Copy( font[101], 0, 0, font[232], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[192], 4, 0, font[232], 3, 0, 3, 2 );
            font[232].setPosition( font[101].x(), font[101].y() - 3 );
            updateSmallFontLetterShadow( font[232] );

            // e with acute accent.
            font[233].resize( font[101].width(), font[101].height() + 3 );
            font[233].reset();
            fheroes2::Copy( font[101], 0, 0, font[233], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[193], 5, 0, font[233], 3, 0, 2, 2 );
            font[233].setPosition( font[101].x(), font[101].y() - 3 );
            updateSmallFontLetterShadow( font[233] );

            // e with circumflex accent.
            font[234].resize( font[101].width(), font[101].height() + 3 );
            font[234].reset();
            fheroes2::Copy( font[101], 0, 0, font[234], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[194], 4, 0, font[234], 2, 0, 3, 2 );
            font[234].setPosition( font[101].x(), font[101].y() - 3 );
            updateSmallFontLetterShadow( font[234] );

            // e with diaeresis.
            font[235].resize( font[101].width(), font[101].height() + 2 );
            font[235].reset();
            fheroes2::Copy( font[101], 0, 0, font[235], 0, 2, font[101].width(), font[101].height() );
            fheroes2::Copy( font[196], 4, 0, font[235], 2, 0, 3, 1 );
            font[235].setPosition( font[101].x(), font[101].y() - 2 );
            updateSmallFontLetterShadow( font[235] );

            // i with grave accent `.
            font[236].resize( font[105].width(), font[105].height() + 1 );
            font[236].reset();
            fheroes2::Copy( font[105], 0, 2, font[236], 0, 3, font[105].width(), 6 );
            fheroes2::Copy( font[192], 4, 0, font[236], 2, 0, 2, 2 );
            font[236].setPosition( font[105].x(), font[105].y() - 1 );
            updateSmallFontLetterShadow( font[236] );

            // i with acute accent.
            font[237].resize( font[105].width(), font[105].height() + 1 );
            font[237].reset();
            fheroes2::Copy( font[105], 0, 2, font[237], 0, 3, font[105].width(), 6 );
            fheroes2::Copy( font[193], 5, 0, font[237], 1, 0, 2, 2 );
            font[237].setPosition( font[105].x(), font[105].y() - 1 );
            updateSmallFontLetterShadow( font[237] );

            // i with circumflex.
            font[238].resize( font[105].width(), font[105].height() + 1 );
            font[238].reset();
            fheroes2::Copy( font[105], 0, 2, font[238], 0, 3, font[105].width(), font[105].height() );
            fheroes2::Copy( font[194], 4, 0, font[238], 1, 0, 3, 2 );
            font[238].setPosition( font[105].x(), font[105].y() - 1 );
            updateSmallFontLetterShadow( font[238] );

            // i with diaeresis.
            font[239].resize( font[105].width(), font[105].height() );
            font[239].reset();
            fheroes2::Copy( font[105], 0, 2, font[239], 0, 2, font[105].width(), font[105].height() );
            fheroes2::Copy( font[196], 4, 0, font[239], 1, 0, 3, 1 );
            font[239].setPosition( font[105].x(), font[105].y() );
            updateSmallFontLetterShadow( font[239] );

            // n with tilde ~.
            font[241].resize( font[110].width(), font[110].height() + 4 );
            font[241].reset();
            fheroes2::Copy( font[110], 0, 0, font[241], 0, 4, font[110].width(), font[110].height() );
            fheroes2::Copy( font[195], 3, 0, font[241], 2, 0, 5, 3 );
            font[241].setPosition( font[110].x(), font[110].y() - 4 );
            updateSmallFontLetterShadow( font[241] );

            // o with grave accent `.
            font[242].resize( font[111].width(), font[111].height() + 3 );
            font[242].reset();
            fheroes2::Copy( font[111], 0, 0, font[242], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[192], 4, 0, font[242], 3, 0, 3, 2 );
            font[242].setPosition( font[111].x(), font[111].y() - 3 );
            updateSmallFontLetterShadow( font[242] );

            // o with acute accent.
            font[243].resize( font[111].width(), font[111].height() + 3 );
            font[243].reset();
            fheroes2::Copy( font[111], 0, 0, font[243], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[193], 5, 0, font[243], 3, 0, 2, 2 );
            font[243].setPosition( font[111].x(), font[111].y() - 3 );
            updateSmallFontLetterShadow( font[243] );

            // o with circumflex accent.
            font[244].resize( font[111].width(), font[111].height() + 3 );
            font[244].reset();
            fheroes2::Copy( font[111], 0, 0, font[244], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[194], 4, 0, font[244], 2, 0, 3, 2 );
            font[244].setPosition( font[111].x(), font[111].y() - 3 );
            updateSmallFontLetterShadow( font[244] );

            // o with tilde accent ~.
            font[245].resize( font[111].width(), font[111].height() + 4 );
            font[245].reset();
            fheroes2::Copy( font[111], 0, 0, font[245], 0, 4, font[111].width(), font[111].height() );
            fheroes2::Copy( font[195], 3, 0, font[245], 1, 0, 5, 3 );
            font[245].setPosition( font[111].x(), font[111].y() - 4 );
            updateSmallFontLetterShadow( font[245] );

            // o with diaeresis.
            font[246].resize( font[111].width(), font[111].height() + 2 );
            font[246].reset();
            fheroes2::Copy( font[111], 0, 0, font[246], 0, 2, font[111].width(), font[111].height() );
            fheroes2::Copy( font[196], 4, 0, font[246], 2, 0, 3, 1 );
            font[246].setPosition( font[111].x(), font[111].y() - 2 );
            updateSmallFontLetterShadow( font[246] );

            // o with / inside.
            font[248].resize( font[111].width(), font[111].height() );
            font[248].reset();
            fheroes2::Copy( font[111], 0, 0, font[248], 0, 0, font[111].width(), font[111].height() );
            fheroes2::Copy( font[88], 6, 0, font[248], 2, 1, 3, 3 );
            fheroes2::Copy( font[88], 1, 0, font[248], font[111].width() - 1, 0, 1, 1 );
            fheroes2::Copy( font[88], 1, 0, font[248], 1, font[111].height() - 2, 1, 1 );
            font[248].setPosition( font[111].x(), font[111].y() );
            updateSmallFontLetterShadow( font[248] );

            // u with grave accent `.
            font[249].resize( font[117].width(), font[117].height() + 3 );
            font[249].reset();
            fheroes2::Copy( font[117], 0, 0, font[249], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[192], 4, 0, font[249], 3, 0, 3, 2 );
            font[249].setPosition( font[117].x(), font[117].y() - 3 );
            updateSmallFontLetterShadow( font[249] );

            // u with acute accent.
            font[250] = font[117];
            font[250].resize( font[117].width(), font[117].height() + 3 );
            font[250].reset();
            fheroes2::Copy( font[117], 0, 0, font[250], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[193], 5, 0, font[250], 3, 0, 2, 2 );
            font[250].setPosition( font[117].x(), font[117].y() - 3 );
            updateSmallFontLetterShadow( font[250] );

            // u with circumflex accent.
            font[251].resize( font[117].width(), font[117].height() + 3 );
            font[251].reset();
            fheroes2::Copy( font[117], 0, 0, font[251], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[194], 4, 0, font[251], 3, 0, 3, 2 );
            font[251].setPosition( font[117].x(), font[117].y() - 3 );
            updateSmallFontLetterShadow( font[251] );

            // u with diaeresis.
            font[252].resize( font[117].width(), font[117].height() + 2 );
            font[252].reset();
            fheroes2::Copy( font[117], 0, 0, font[252], 0, 2, font[117].width(), font[117].height() );
            fheroes2::Copy( font[252], 2, 0 + 2, font[252], 2, 0, 1, 1 );
            fheroes2::Copy( font[252], 2, 0 + 2, font[252], 6, 0, 1, 1 );
            font[252].setPosition( font[117].x(), font[117].y() - 2 );
            updateSmallFontLetterShadow( font[252] );
        }
    }

    // Greek uses CP1253
    void generateCP1253Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        resizeCodePage( icnVsSprite );

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // NBSP character.
            font[160].resize( normalFontSpaceWidth, 1 );
            font[160].reset();

            // Greek capital letter alpha
            font[193] = font[65];

            // Greek capital letter beta
            font[194] = font[66];

            // Greek capital letter gamma
            font[195] = font[70];
            fheroes2::FillTransform( font[195], 6, 4, 3, 4, 1 );

            // Greek capital letter epsilon
            font[197] = font[69];

            // Greek capital letter zeta
            font[198] = font[90];

            // Greek capital letter eta
            font[199] = font[72];

            // Greek capital letter theta
            font[200] = font[79];
            fheroes2::Copy( font[193], 7, 5, font[200], 7, 5, 3, 2 );
            updateNormalFontLetterShadow( font[200] );

            // Greek capital letter iota
            font[201] = font[73];

            // Greek capital letter kappa
            font[202] = font[75];

            // Greek capital letter lambda
            font[203] = font[65];
            fheroes2::FillTransform( font[203], 8, 5, 2, 1, 1 );
            fheroes2::FillTransform( font[203], 7, 6, 4, 3, 1 );
            fheroes2::Copy( font[65], 7, 6, font[203], 7, 5, 1, 1 );
            fheroes2::Copy( font[65], 10, 6, font[203], 10, 5, 1, 1 );
            fheroes2::FillTransform( font[203], 3, 0, 4, 4, 1 );
            fheroes2::FillTransform( font[203], 2, 3, 2, 3, 1 );
            fheroes2::FillTransform( font[203], 7, 0, 1, 1, 1 );
            fheroes2::Copy( font[65], 7, 1, font[203], 8, 0, 1, 1 );
            updateNormalFontLetterShadow( font[203] );

            // Greek capital letter mu
            font[204] = font[77];

            // Greek capital letter nu
            font[205] = font[78];

            // Greek capital letter xi
            font[206] = font[197];
            fheroes2::FillTransform( font[206], 4, 2, 2, 2, 1 );
            fheroes2::Copy( font[206], 6, 1, font[206], 4, 1, 2, 1 );
            fheroes2::Copy( font[206], 6, 9, font[206], 3, 9, 3, 1 );
            fheroes2::FillTransform( font[206], 0, 2, 11, 7, 1 );
            fheroes2::Copy( font[197], 6, 4, font[206], 3, 4, 3, 2 );
            fheroes2::Copy( font[197], 6, 4, font[206], 6, 4, 3, 2 );
            updateNormalFontLetterShadow( font[206] );

            // Greek capital letter omicron
            font[207] = font[79];

            // Greek capital letter pi
            font[208] = font[195];
            fheroes2::Copy( font[208], 4, 1, font[208], 8, 1, 2, 9 );
            fheroes2::Copy( font[208], 4, 9, font[208], 8, 10, 2, 1 );
            fheroes2::Copy( font[208], 6, 0, font[208], 10, 0, 1, 2 );
            updateNormalFontLetterShadow( font[208] );

            // Greek capital letter rho
            font[209] = font[80];

            // Greek capital letter tau
            font[212] = font[84];

            // Greek capital letter upsilon
            font[213] = font[89];

            // Greek capital letter phi
            font[214].resize( font[80].width() + 1, font[80].height() );
            font[214].reset();
            fheroes2::Copy( font[80], 0, 0, font[214], 1, 0, font[80].width(), font[80].height() );
            fheroes2::Flip( font[80], 6, 0, font[214], 1, 0, 5, 6, true, false );
            font[214].setPosition( font[80].x(), font[80].y() );
            updateNormalFontLetterShadow( font[214] );

            // Greek capital letter chi
            font[215] = font[88];

            // Greek capital letter gamma
            font[227].resize( font[118].width(), font[118].height() + 2 );
            font[227].reset();
            fheroes2::Copy( font[118], 0, 0, font[227], 0, 0, 9, 7 );
            fheroes2::Copy( font[118], 4, 4, font[227], 4, 5, 3, 1 );
            fheroes2::Copy( font[118], 4, 4, font[227], 4, 6, 3, 1 );
            fheroes2::Copy( font[118], 4, 5, font[227], 4, 7, 3, 2 );
            font[227].setPosition( font[118].x(), font[118].y() );
            updateNormalFontLetterShadow( font[227] );

            // Greek capital letter eta
            font[231].resize( font[110].width() - 1, font[110].height() + 2 );
            font[231].reset();
            fheroes2::Copy( font[110], 0, 0, font[231], 0, 0, font[110].width() - 1, font[110].height() );
            fheroes2::Copy( font[110], 7, 2, font[231], 7, 4, 2, 5 );
            font[231].setPosition( font[110].x(), font[110].y() );
            updateNormalFontLetterShadow( font[231] );

            // Greek capital letter theta
            font[232] = font[48];
            fheroes2::Copy( font[53], 2, 4, font[232], 3, 5, 5, 2 );
            updateNormalFontLetterShadow( font[232] );

            // Greek capital letter iota
            font[233].resize( font[105].width(), font[105].height() - 3 );
            font[233].reset();
            fheroes2::Copy( font[105], 1, 3, font[233], 1, 0, 4, 7 );
            font[233].setPosition( font[105].x(), font[105].y() + 3 );
            updateNormalFontLetterShadow( font[233] );

            // Greek small letter kappa
            font[234].resize( font[107].width() - 1, font[107].height() - 4 );
            font[234].reset();
            fheroes2::Copy( font[107], 0, 0, font[234], 0, 0, 4, 6 );
            fheroes2::Copy( font[107], 4, 4, font[234], 4, 0, 5, 6 );
            fheroes2::Copy( font[107], 0, 10, font[234], 0, 6, 4, 1 );
            fheroes2::Copy( font[107], 7, 10, font[234], 6, 6, 3, 1 );
            font[234].setPosition( font[107].x(), font[107].y() + 4 );
            updateNormalFontLetterShadow( font[234] );

            // Greek small letter mu
            font[236].resize( font[117].width(), font[117].height() + 2 );
            font[236].reset();
            fheroes2::Copy( font[117], 0, 0, font[236], 0, 0, font[117].width(), font[117].height() );
            fheroes2::Copy( font[117], 2, 3, font[236], 2, 5, 2, 4 );
            font[236].setPosition( font[117].x(), font[117].y() );
            updateNormalFontLetterShadow( font[236] );

            // Greek small letter nu
            font[237] = font[118];

            // Greek small letter omicron
            font[239] = font[111];

            // Greek small letter pi
            font[240] = font[110];
            fheroes2::Copy( font[110], 3, 1, font[240], 3, 0, 2, 2 );
            fheroes2::FillTransform( font[240], 4, 2, 1, 1, 1 );
            fheroes2::Copy( font[110], 7, 0, font[240], 8, 0, 1, 1 );
            fheroes2::Copy( font[110], 2, 0, font[240], 9, 0, 1, 2 );
            updateNormalFontLetterShadow( font[240] );

            // Greek small letter rho
            font[241] = font[112];
            fheroes2::FillTransform( font[241], 1, 0, 2, 3, 1 );
            fheroes2::FillTransform( font[241], 3, 0, 1, 1, 1 );
            fheroes2::Copy( font[112], 4, 0, font[241], 3, 1, 1, 1 );
            updateNormalFontLetterShadow( font[241] );

            // Greek small letter sigma
            font[243].resize( font[111].width() + 1, font[111].height() );
            font[243].reset();
            fheroes2::Copy( font[111], 0, 0, font[243], 0, 0, font[111].width(), font[111].height() );
            fheroes2::Copy( font[243], 4, 0, font[243], 7, 0, 3, 1 );
            font[243].setPosition( font[111].x(), font[111].y() );
            updateNormalFontLetterShadow( font[243] );

            // Greek small letter upsilon
            font[245].resize( font[117].width() - 1, font[117].height() );
            fheroes2::Copy( font[117], 0, 0, font[245], 0, 0, font[117].width() - 1, font[117].height() );
            fheroes2::FillTransform( font[245], 7, 7, 2, 2, 1 );
            fheroes2::FillTransform( font[245], 8, 6, 2, 1, 1 );
            fheroes2::FillTransform( font[245], 8, 6, 1, 1, 1 );
            font[245].setPosition( font[117].x(), font[117].y() );
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // NBSP character.
            font[160].resize( smallFontSpaceWidth, 1 );
            font[160].reset();

            // Greek capital letter alpha
            font[193] = font[65];

            // Greek capital letter beta
            font[194] = font[66];

            // Greek capital letter gamma
            font[195] = font[70];
            fheroes2::FillTransform( font[195], 4, 3, 2, 2, 1 );

            // Greek capital letter delta
            font[196] = font[65];
            fheroes2::FillTransform( font[196], 4, 4, 3, 2, 1 );
            fheroes2::FillTransform( font[196], 1, 0, 3, 3, 1 );
            fheroes2::FillTransform( font[196], 4, 0, 1, 2, 1 );
            fheroes2::Copy( font[65], 3, 4, font[196], 3, 6, 5, 1 );
            updateSmallFontLetterShadow( font[196] );

            // Greek capital letter epsilon
            font[197] = font[69];

            // Greek capital letter zeta
            font[198] = font[90];

            // Greek capital letter eta
            font[199] = font[72];

            // Greek capital letter theta
            font[200] = font[79];
            fheroes2::Copy( font[79], 3, 0, font[200], 3, 3, 3, 1 );
            updateSmallFontLetterShadow( font[200] );

            // Greek capital letter iota
            font[201] = font[73];

            // Greek capital letter kappa
            font[202] = font[75];

            // Greek capital letter lambda
            font[203] = font[196];
            fheroes2::FillTransform( font[203], 4, 6, 3, 2, 1 );
            updateSmallFontLetterShadow( font[203] );

            // Greek capital letter mu
            font[204] = font[77];

            // Greek capital letter nu
            font[205] = font[78];

            // Greek capital letter xi
            font[206] = font[69];
            fheroes2::FillTransform( font[206], 2, 1, 5, 5, 1 );
            fheroes2::Copy( font[69], 3, 3, font[206], 3, 3, 3, 1 );
            updateSmallFontLetterShadow( font[206] );

            // Greek capital letter omicron
            font[207] = font[79];

            // Greek capital letter pi
            font[208] = font[195];
            fheroes2::Copy( font[195], 2, 2, font[208], 6, 2, 2, 5 );
            updateSmallFontLetterShadow( font[208] );

            // Greek capital letter rho
            font[209] = font[80];

            // Greek capital letter tau
            font[212] = font[84];

            // Greek capital letter upsilon
            font[213] = font[89];

            // Greek capital letter phi
            font[214].resize( font[80].width() + 1, font[80].height() );
            font[214].reset();
            fheroes2::Copy( font[80], 0, 0, font[214], 1, 0, font[80].width(), font[80].height() );
            fheroes2::Flip( font[80], 6, 0, font[214], 1, 0, 5, 6, true, false );
            font[214].setPosition( font[80].x(), font[80].y() );
            updateSmallFontLetterShadow( font[214] );

            // Greek capital letter chi
            font[215] = font[88];

            // Greek small letter eta
            font[231].resize( font[110].width() - 1, font[110].height() + 1 );
            font[231].reset();
            fheroes2::Copy( font[110], 0, 0, font[231], 0, 0, font[110].width() - 1, font[110].height() );
            fheroes2::Copy( font[110], 1, 0, font[231], 6, 5, 1, 1 );
            font[231].setPosition( font[110].x(), font[110].y() );
            updateSmallFontLetterShadow( font[231] );

            // Greek small letter iota
            font[233].resize( font[105].width(), font[105].height() - 2 );
            fheroes2::Copy( font[105], 0, 2, font[233], 0, 0, font[233].width(), font[233].height() );
            font[233].setPosition( font[233].x(), font[233].y() - 4 );

            // Greek small letter kappa
            font[234].resize( 5, 6 );
            font[234].reset();
            fheroes2::Copy( font[75], 3, 1, font[234], 1, 1, 4, 5 );
            font[234].setPosition( font[107].x(), font[107].y() + 1 );
            updateSmallFontLetterShadow( font[234] );

            // Greek small letter lambda
            font[235].resize( font[118].width(), font[118].height() + 2 );
            font[235].reset();
            fheroes2::Flip( font[118], 1, 0, font[235], 1, 2, 7, 5, false, true );
            fheroes2::Copy( font[118], 1, 0, font[235], 3, 1, 1, 1 );
            fheroes2::Copy( font[118], 1, 0, font[235], 2, 0, 1, 1 );
            removeShadows( font[235] );
            font[235].setPosition( font[118].x(), font[118].y() - 2 );
            updateSmallFontLetterShadow( font[235] );

            // Greek small letter nu
            font[237] = font[118];

            // Greek small letter omicron
            font[239] = font[111];

            // Greek small letter omicron with tonos
            font[243].resize( font[111].width(), font[111].height() + 3 );
            font[243].reset();
            fheroes2::Copy( font[111], 0, 0, font[243], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[122], 2, 2, font[243], 3, 0, 2, 2 );
            font[243].setPosition( font[111].x(), font[111].y() - 3 );
            updateSmallFontLetterShadow( font[243] );

            // Greek small letter upsilon
            font[239].resize( font[117].width() - 1, font[117].height() );
            fheroes2::Copy( font[117], 0, 0, font[239], 0, 0, font[117].width() - 1, font[117].height() );
            fheroes2::FillTransform( font[239], 6, 5, 1, 1, 1 );
        }
    }

    // Turkish uses CP1254
    void generateCP1254Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        resizeCodePage( icnVsSprite );

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // NBSP character.
            font[160].resize( normalFontSpaceWidth, 1 );
            font[160].reset();

            // C with cedilla.
            font[199].resize( font[67].width(), font[67].height() + 3 );
            font[199].reset();
            fheroes2::Copy( font[67], 0, 0, font[199], 0, 0, font[67].width(), font[67].height() );
            fheroes2::Copy( font[99], 2, 1, font[199], 7, 11, 1, 1 );
            fheroes2::Copy( font[99], 5, 6, font[199], 8, 11, 1, 1 );
            fheroes2::Copy( font[99], 2, 6, font[199], 9, 11, 1, 1 );
            fheroes2::Copy( font[99], 2, 1, font[199], 8, 12, 1, 1 );
            fheroes2::Copy( font[99], 5, 6, font[199], 9, 12, 1, 1 );
            fheroes2::Copy( font[99], 2, 1, font[199], 7, 13, 1, 1 );
            fheroes2::Copy( font[99], 5, 6, font[199], 8, 13, 1, 1 );
            fheroes2::Copy( font[99], 3, 0, font[199], 9, 13, 1, 1 );
            font[199].setPosition( font[67].x(), font[67].y() );
            updateNormalFontLetterShadow( font[199] );

            // G with breve.
            font[208].resize( font[71].width(), font[71].height() + 3 );
            font[208].reset();
            fheroes2::Copy( font[71], 0, 0, font[208], 0, 3, font[71].width(), font[71].height() );
            fheroes2::Copy( font[71], 5, 9, font[208], 5, 0, 7, 2 );
            fheroes2::FillTransform( font[208], 7, 0, 3, 1, 1 );
            font[208].setPosition( font[71].x(), font[71].y() - 3 );
            updateNormalFontLetterShadow( font[208] );

            // O with diaeresis, two dots above.
            font[214].resize( font[79].width(), font[79].height() + 3 );
            font[214].reset();
            fheroes2::Copy( font[79], 0, 0, font[214], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[214], 1, 2 + 3, font[214], 5, 0, 1, 1 );
            fheroes2::Copy( font[214], 2, 2 + 3, font[214], 6, 0, 1, 1 );
            fheroes2::Copy( font[214], 5, 0, font[214], 10, 0, 2, 1 );
            font[214].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[214] );

            // U with diaeresis.
            font[220].resize( font[85].width(), font[85].height() + 3 );
            font[220].reset();
            fheroes2::Copy( font[85], 0, 0, font[220], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[220], 1, 1 + 3, font[220], 4, 0, 1, 1 );
            fheroes2::Copy( font[220], 2, 1 + 3, font[220], 5, 0, 1, 1 );
            fheroes2::Copy( font[220], 4, 0, font[220], 9, 0, 2, 1 );
            font[220].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[220] );

            // I with dot above.
            font[221].resize( font[73].width(), font[73].height() + 3 );
            font[221].reset();
            fheroes2::Copy( font[73], 0, 0, font[221], 0, 3, font[73].width(), font[73].height() );
            fheroes2::Copy( font[105], 1, 0, font[221], 3, 0, 3, 2 );
            font[221].setPosition( font[73].x(), font[73].y() - 3 );
            updateNormalFontLetterShadow( font[221] );

            // S with cedilla.
            font[222].resize( font[83].width(), font[83].height() + 3 );
            font[222].reset();
            fheroes2::Copy( font[83], 0, 0, font[222], 0, 0, font[83].width(), font[83].height() );
            fheroes2::Copy( font[199], 7, 11, font[222], 5, 11, 3, 3 );
            font[222].setPosition( font[83].x(), font[83].y() );
            updateNormalFontLetterShadow( font[222] );

            // c with cedilla.
            font[231].resize( font[99].width(), font[99].height() + 3 );
            font[231].reset();
            fheroes2::Copy( font[99], 0, 0, font[231], 0, 0, font[99].width(), font[99].height() );
            fheroes2::Copy( font[199], 7, 11, font[231], 4, 7, 3, 3 );
            font[231].setPosition( font[99].x(), font[99].y() );
            updateNormalFontLetterShadow( font[231] );

            // g with breve.
            font[240].resize( font[103].width(), font[103].height() + 3 );
            font[240].reset();
            fheroes2::Copy( font[103], 0, 0, font[240], 0, 3, font[103].width(), font[103].height() );
            fheroes2::Copy( font[111], 2, 5, font[240], 2, 0, 6, 2 );
            fheroes2::FillTransform( font[240], 4, 0, 2, 1, 1 );
            font[240].setPosition( font[103].x(), font[103].y() - 3 );
            updateNormalFontLetterShadow( font[240] );

            // o with diaeresis, two dots above.
            font[246].resize( font[111].width(), font[111].height() + 3 );
            font[246].reset();
            fheroes2::Copy( font[111], 0, 0, font[246], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[246], 4, 0 + 3, font[246], 3, 0, 1, 1 );
            fheroes2::Copy( font[246], 6, 1 + 3, font[246], 3, 1, 1, 1 );
            fheroes2::Copy( font[246], 6, 1 + 3, font[246], 2, 0, 1, 1 );
            fheroes2::Copy( font[246], 4, 1 + 3, font[246], 2, 1, 1, 1 );
            fheroes2::Copy( font[246], 2, 0, font[246], 6, 0, 2, 2 );
            font[246].setPosition( font[111].x(), font[111].y() - 3 );
            updateNormalFontLetterShadow( font[246] );

            // u with diaeresis.
            font[252].resize( font[117].width(), font[117].height() + 3 );
            font[252].reset();
            fheroes2::Copy( font[117], 0, 0, font[252], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[252], 2, 0 + 3, font[252], 3, 0, 1, 1 );
            fheroes2::Copy( font[252], 2, 1 + 3, font[252], 3, 1, 1, 1 );
            fheroes2::Copy( font[252], 2, 1 + 3, font[252], 2, 0, 1, 1 );
            fheroes2::Copy( font[252], 3, 6 + 3, font[252], 2, 1, 1, 1 );
            fheroes2::Copy( font[252], 2, 0, font[252], 6, 0, 2, 2 );
            font[252].setPosition( font[117].x(), font[117].y() - 3 );
            updateNormalFontLetterShadow( font[252] );

            // i without dot above.
            font[253] = font[105];
            fheroes2::FillTransform( font[253], 0, 0, font[253].width(), 3, 1 );
            fheroes2::Copy( font[115], 2, 0, font[253], 0, 3, 1, 1 );
            updateNormalFontLetterShadow( font[253] );

            // s with cedilla.
            font[254].resize( font[115].width(), font[115].height() + 3 );
            font[254].reset();
            fheroes2::Copy( font[115], 0, 0, font[254], 0, 0, font[115].width(), font[115].height() );
            fheroes2::Copy( font[199], 7, 11, font[254], 4, 7, 3, 3 );
            font[254].setPosition( font[115].x(), font[115].y() );
            updateNormalFontLetterShadow( font[254] );
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // NBSP character.
            font[160].resize( smallFontSpaceWidth, 1 );
            font[160].reset();

            // C with cedilla.
            font[199].resize( font[67].width(), font[67].height() + 3 );
            font[199].reset();
            fheroes2::Copy( font[67], 0, 0, font[199], 0, 0, font[67].width(), font[67].height() );
            fheroes2::Copy( font[67], 1, 1, font[199], 3, 7, 2, 2 );
            font[199].setPosition( font[67].x(), font[67].y() );
            updateSmallFontLetterShadow( font[199] );

            // G with breve.
            font[208].resize( font[71].width(), font[71].height() + 3 );
            font[208].reset();
            fheroes2::Copy( font[71], 0, 0, font[208], 0, 3, font[71].width(), font[71].height() );
            fheroes2::Copy( font[67], 2, 5, font[208], 3, 0, 1, 1 );
            fheroes2::Copy( font[67], 2, 5, font[208], 6, 0, 1, 1 );
            fheroes2::Copy( font[67], 3, 6, font[208], 4, 1, 2, 1 );
            font[208].setPosition( font[71].x(), font[71].y() - 3 );
            updateSmallFontLetterShadow( font[208] );

            // O with diaeresis, two dots above.
            font[214].resize( font[79].width(), font[79].height() + 2 );
            font[214].reset();
            fheroes2::Copy( font[79], 0, 0, font[214], 0, 2, font[79].width(), font[79].height() );
            fheroes2::Copy( font[214], 3, 0 + 2, font[214], 3, 0, 1, 1 );
            fheroes2::Copy( font[214], 3, 0 + 2, font[214], 5, 0, 1, 1 );
            font[214].setPosition( font[79].x(), font[79].y() - 2 );
            updateSmallFontLetterShadow( font[214] );

            // U with diaeresis.
            font[220].resize( font[85].width(), font[85].height() + 2 );
            font[220].reset();
            fheroes2::Copy( font[85], 0, 0, font[220], 0, 2, font[85].width(), font[85].height() );
            fheroes2::Copy( font[220], 3, 0 + 2, font[220], 4, 0, 1, 1 );
            fheroes2::Copy( font[220], 3, 0 + 2, font[220], 6, 0, 1, 1 );
            font[220].setPosition( font[85].x(), font[85].y() - 2 );
            updateSmallFontLetterShadow( font[220] );

            // I with dot above.
            font[221].resize( font[73].width(), font[73].height() + 2 );
            font[221].reset();
            fheroes2::Copy( font[73], 0, 0, font[221], 0, 2, font[73].width(), font[73].height() );
            fheroes2::Copy( font[73], 2, 0, font[221], 2, 0, 2, 1 );
            font[221].setPosition( font[73].x(), font[73].y() - 2 );
            updateSmallFontLetterShadow( font[221] );

            // S with cedilla.
            font[222].resize( font[83].width(), font[83].height() + 3 );
            font[222].reset();
            fheroes2::Copy( font[83], 0, 0, font[222], 0, 0, font[83].width(), font[83].height() );
            fheroes2::Copy( font[67], 1, 1, font[222], 3, 7, 2, 2 );
            font[222].setPosition( font[83].x(), font[83].y() );
            updateSmallFontLetterShadow( font[222] );

            // c with cedilla.
            font[231].resize( font[99].width(), font[99].height() + 3 );
            font[231].reset();
            fheroes2::Copy( font[99], 0, 0, font[231], 0, 0, font[99].width(), font[99].height() );
            fheroes2::Copy( font[199], 1, 1, font[231], 2, 5, 2, 2 );
            font[231].setPosition( font[99].x(), font[99].y() );
            updateSmallFontLetterShadow( font[231] );

            // g with breve.
            font[240].resize( font[103].width(), font[103].height() + 3 );
            font[240].reset();
            fheroes2::Copy( font[103], 0, 0, font[240], 0, 3, font[103].width(), font[103].height() );
            fheroes2::Copy( font[67], 2, 5, font[240], 2, 0, 1, 1 );
            fheroes2::Copy( font[67], 2, 5, font[240], 4, 0, 1, 1 );
            fheroes2::Copy( font[67], 3, 6, font[240], 3, 1, 1, 1 );
            font[240].setPosition( font[103].x(), font[103].y() - 3 );
            updateSmallFontLetterShadow( font[240] );

            // o with diaeresis, two dots above.
            font[246].resize( font[111].width(), font[111].height() + 2 );
            font[246].reset();
            fheroes2::Copy( font[111], 0, 0, font[246], 0, 2, font[111].width(), font[111].height() );
            fheroes2::Copy( font[246], 3, 0 + 2, font[246], 2, 0, 1, 1 );
            fheroes2::Copy( font[246], 3, 0 + 2, font[246], 4, 0, 1, 1 );
            font[246].setPosition( font[111].x(), font[111].y() - 2 );
            updateSmallFontLetterShadow( font[246] );

            // u with diaeresis.
            font[252].resize( font[117].width(), font[117].height() + 2 );
            font[252].reset();
            fheroes2::Copy( font[117], 0, 0, font[252], 0, 2, font[117].width(), font[117].height() );
            fheroes2::Copy( font[252], 2, 0 + 2, font[252], 2, 0, 1, 1 );
            fheroes2::Copy( font[252], 2, 0 + 2, font[252], 6, 0, 1, 1 );
            font[252].setPosition( font[117].x(), font[117].y() - 2 );
            updateSmallFontLetterShadow( font[252] );

            // i without dot above.
            font[253].resize( font[105].width(), font[105].height() + 1 );
            font[253].reset();
            fheroes2::Copy( font[105], 0, 2, font[253], 0, 3, font[105].width(), 6 );
            font[253].setPosition( font[105].x(), font[105].y() - 1 );
            updateSmallFontLetterShadow( font[253] );

            // s with cedilla.
            font[254].resize( font[115].width(), font[115].height() + 3 );
            font[254].reset();
            fheroes2::Copy( font[115], 0, 0, font[254], 0, 0, font[115].width(), font[115].height() );
            fheroes2::Copy( font[67], 1, 1, font[254], 2, 5, 2, 2 );
            font[254].setPosition( font[115].x(), font[115].y() );
            updateSmallFontLetterShadow( font[254] );
        }
    }

    void generateCP1258Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        resizeCodePage( icnVsSprite );

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // NBSP character.
            font[160].resize( normalFontSpaceWidth, 1 );
            font[160].reset();

            // A with grave accent ` and generate the grave accent for further use.
            font[192].resize( font[65].width(), font[65].height() + 3 );
            font[192].reset();
            fheroes2::Copy( font[65], 0, 0, font[192], 0, 3, font[65].width(), font[65].height() );
            font[192].setPosition( font[65].x(), font[65].y() - 3 );
            fheroes2::Copy( font[192], 3, 4, font[192], 7, 0, 1, 1 );
            fheroes2::Copy( font[192], 4, 4, font[192], 8, 0, 1, 1 );
            fheroes2::Copy( font[192], 3, 4, font[192], 8, 1, 1, 1 );
            fheroes2::Copy( font[192], 4, 4, font[192], 9, 1, 1, 1 );
            fheroes2::Copy( font[192], 3, 3, font[192], 10, 1, 1, 1 );
            updateNormalFontLetterShadow( font[192] );

            // A with acute accent. Generate the accent for further use.
            font[193].resize( font[65].width(), font[65].height() + 3 );
            font[193].reset();
            fheroes2::Copy( font[65], 0, 0, font[193], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[193], 3, 4, font[193], 10, 0, 1, 1 );
            fheroes2::Copy( font[193], 4, 4, font[193], 9, 0, 1, 1 );
            fheroes2::Copy( font[193], 3, 4, font[193], 9, 1, 1, 1 );
            fheroes2::Copy( font[193], 4, 4, font[193], 8, 1, 1, 1 );
            fheroes2::Copy( font[193], 3, 3, font[193], 7, 1, 1, 1 );
            font[193].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[193] );

            // A with circumflex accent. Generation of accent for further use.
            font[194].resize( font[65].width(), font[65].height() + 3 );
            font[194].reset();
            fheroes2::Copy( font[65], 0, 0, font[194], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[97], 7, 1, font[194], 7, 1, 1, 1 );
            fheroes2::Copy( font[97], 7, 1, font[194], 8, 0, 1, 1 );
            fheroes2::Copy( font[97], 7, 1, font[194], 9, 1, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[194], 7, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[194], 9, 0, 1, 1 );
            font[194].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[194] );

            // A with breve and generate the accent for further use.
            font[195].resize( font[65].width(), font[65].height() + 3 );
            font[195].reset();
            fheroes2::Copy( font[65], 0, 0, font[195], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[71], 5, 9, font[195], 5, 0, 7, 2 );
            fheroes2::FillTransform( font[195], 7, 0, 3, 1, 1 );
            font[195].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[195] );

            // E with grave accent `.
            font[200].resize( font[69].width(), font[69].height() + 3 );
            font[200].reset();
            fheroes2::Copy( font[69], 0, 0, font[200], 0, 3, font[69].width(), font[69].height() );
            font[200].setPosition( font[69].x(), font[69].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[200], 4, 0, 4, 2 );
            updateNormalFontLetterShadow( font[200] );

            // E with acute accent.
            font[201].resize( font[69].width(), font[69].height() + 3 );
            font[201].reset();
            fheroes2::Copy( font[69], 0, 0, font[201], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[193], 7, 0, font[201], 5, 0, 4, 2 );
            font[201].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[201] );

            // E with circumflex accent.
            font[202].resize( font[69].width(), font[69].height() + 3 );
            font[202].reset();
            fheroes2::Copy( font[69], 0, 0, font[202], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[194], 7, 0, font[202], 5, 0, 3, 2 );
            font[202].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[202] );

            // [204] COMBINING GRAVE TONE MARK `.

            // I with accute accent.
            font[205].resize( font[73].width(), font[73].height() + 3 );
            font[205].reset();
            fheroes2::Copy( font[73], 0, 0, font[205], 0, 3, font[73].width(), font[73].height() );
            fheroes2::Copy( font[193], 7, 0, font[205], 3, 0, 4, 2 );
            font[205].setPosition( font[73].x(), font[73].y() - 3 );
            updateNormalFontLetterShadow( font[205] );

            // D with stroke. Needs stroke generated.
            font[208] = font[68];

            // [210] COMBINING HOOK ABOVE.

            // O with acute accent.
            font[211].resize( font[79].width(), font[79].height() + 3 );
            font[211].reset();
            fheroes2::Copy( font[79], 0, 0, font[211], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[193], 7, 0, font[211], 7, 0, 4, 2 );
            font[211].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[211] );

            // O with circumflex accent.
            font[212].resize( font[79].width(), font[79].height() + 3 );
            font[212].reset();
            fheroes2::Copy( font[79], 0, 0, font[212], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[194], 7, 0, font[212], 7, 0, 3, 2 );
            font[212].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[212] );

            // O with horn. Needs accent generation.
            font[213] = font[79];

            // U with grave accent `.
            font[217].resize( font[85].width(), font[85].height() + 3 );
            font[217].reset();
            fheroes2::Copy( font[85], 0, 0, font[217], 0, 3, font[85].width(), font[85].height() );
            font[217].setPosition( font[85].x(), font[85].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[217], 5, 0, 4, 2 );
            updateNormalFontLetterShadow( font[217] );

            // U with acute accent.
            font[218].resize( font[85].width(), font[85].height() + 3 );
            font[218].reset();
            fheroes2::Copy( font[85], 0, 0, font[218], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[193], 7, 0, font[218], 5, 0, 4, 2 );
            font[218].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[218] );

            // U with horn. Needs accent generated.
            font[221] = font[85];

            // [222] COMBINING TILDE ~.

            // a with grave accent `.
            font[224].resize( font[97].width(), font[97].height() + 3 );
            font[224].reset();
            fheroes2::Copy( font[97], 0, 0, font[224], 0, 3, font[97].width(), font[97].height() );
            font[224].setPosition( font[97].x(), font[97].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[224], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[224] );

            // a with acute accent.
            font[225].resize( font[97].width(), font[97].height() + 3 );
            font[225].reset();
            fheroes2::Copy( font[97], 0, 0, font[225], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[193], 7, 0, font[225], 3, 0, 4, 2 );
            font[225].setPosition( font[97].x(), font[97].y() - 3 );
            updateNormalFontLetterShadow( font[225] );

            // a with circumflex accent.
            font[226].resize( font[97].width(), font[97].height() + 3 );
            font[226].reset();
            fheroes2::Copy( font[97], 0, 0, font[226], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[194], 7, 0, font[226], 3, 0, 3, 2 );
            font[226].setPosition( font[97].x(), font[97].y() - 3 );
            updateNormalFontLetterShadow( font[226] );

            // a with breve.
            font[227].resize( font[97].width(), font[97].height() + 3 );
            font[227].reset();
            fheroes2::Copy( font[97], 0, 0, font[227], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[97], 2, 0, font[227], 3, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[227], 6, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[227], 3, 1, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[227], 4, 1, 2, 1 );
            fheroes2::Copy( font[97], 1, 0, font[227], 6, 1, 1, 1 );
            font[227].setPosition( font[97].x(), font[97].y() - 3 );
            updateNormalFontLetterShadow( font[227] );

            // e with grave accent `.
            font[232].resize( font[101].width(), font[101].height() + 3 );
            font[232].reset();
            fheroes2::Copy( font[101], 0, 0, font[232], 0, 3, font[101].width(), font[101].height() );
            font[232].setPosition( font[101].x(), font[101].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[232], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[232] );

            // e with acute accent.
            font[233].resize( font[101].width(), font[101].height() + 3 );
            font[233].reset();
            fheroes2::Copy( font[101], 0, 0, font[233], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[193], 7, 0, font[233], 3, 0, 4, 2 );
            font[233].setPosition( font[101].x(), font[101].y() - 3 );
            updateNormalFontLetterShadow( font[233] );

            // e with circumflex accent.
            font[234].resize( font[101].width(), font[101].height() + 3 );
            font[234].reset();
            fheroes2::Copy( font[101], 0, 0, font[234], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[194], 7, 0, font[234], 4, 0, 3, 2 );
            font[234].setPosition( font[101].x(), font[101].y() - 3 );
            updateNormalFontLetterShadow( font[234] );

            // [236] COMBINING ACUTE TONE MARK.

            // i with acute accent.
            font[237] = font[105];
            fheroes2::FillTransform( font[237], 0, 0, font[237].width(), 3, 1 );
            fheroes2::Copy( font[193], 7, 0, font[237], 1, 0, 4, 2 );
            updateNormalFontLetterShadow( font[237] );

            // d with stroke. Needs stroke generated.
            font[240] = font[100];

            // [242] COMBINING DOT BELOW.

            // o with acute accent.
            font[243].resize( font[111].width(), font[111].height() + 3 );
            font[243].reset();
            fheroes2::Copy( font[111], 0, 0, font[243], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[193], 7, 0, font[243], 3, 0, 4, 2 );
            font[243].setPosition( font[111].x(), font[111].y() - 3 );
            updateNormalFontLetterShadow( font[243] );

            // o with circumflex accent.
            font[244].resize( font[111].width(), font[111].height() + 3 );
            font[244].reset();
            fheroes2::Copy( font[111], 0, 0, font[244], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[194], 7, 0, font[244], 4, 0, 3, 2 );
            font[244].setPosition( font[111].x(), font[111].y() - 3 );
            updateNormalFontLetterShadow( font[244] );

            // o with horn. Needs accent generated.
            font[245] = font[111];

            // u with grave accent `.
            font[249].resize( font[117].width(), font[117].height() + 3 );
            font[249].reset();
            fheroes2::Copy( font[117], 0, 0, font[249], 0, 3, font[117].width(), font[117].height() );
            font[249].setPosition( font[117].x(), font[117].y() - 3 );
            fheroes2::Copy( font[192], 7, 0, font[249], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[249] );

            // u with acute accent.
            font[250].resize( font[117].width(), font[117].height() + 3 );
            font[250].reset();
            fheroes2::Copy( font[117], 0, 0, font[250], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[193], 7, 0, font[250], 3, 0, 4, 2 );
            font[250].setPosition( font[117].x(), font[117].y() - 3 );
            updateNormalFontLetterShadow( font[250] );

            // u with horn. Needs accent generated
            font[253] = font[117];
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // NBSP character.
            font[160].resize( smallFontSpaceWidth, 1 );
            font[160].reset();

            // A with grave accent `. Generate grave accent for further use.
            font[192].resize( font[65].width(), font[65].height() + 3 );
            font[192].reset();
            fheroes2::Copy( font[65], 0, 0, font[192], 0, 4, font[65].width(), font[65].height() );
            fheroes2::Copy( font[192], 4, 4, font[192], 4, 0, 1, 1 );
            fheroes2::Copy( font[192], 4, 4, font[192], 5, 1, 1, 1 );
            fheroes2::Copy( font[192], 4, 4, font[192], 6, 2, 1, 1 );
            font[192].setPosition( font[65].x(), font[65].y() - 4 );
            updateSmallFontLetterShadow( font[192] );

            // A with acute accent. Generate acute accent for further use
            font[193].resize( font[65].width(), font[65].height() + 4 );
            font[193].reset();
            fheroes2::Copy( font[65], 0, 0, font[193], 0, 4, font[65].width(), font[65].height() );
            fheroes2::Copy( font[65], 4, 0, font[193], 4, 2, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[193], 5, 1, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[193], 6, 0, 1, 1 );
            font[193].setPosition( font[65].x(), font[65].y() - 4 );
            updateSmallFontLetterShadow( font[193] );

            // A with circumflex accent. Generate accent for further use.
            font[194].resize( font[65].width(), font[65].height() + 3 );
            font[194].reset();
            fheroes2::Copy( font[65], 0, 0, font[194], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[65], 4, 0, font[194], 4, 1, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[194], 5, 0, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[194], 6, 1, 1, 1 );
            font[194].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[194] );

            // A with breve. Generate accent for further use.
            font[195].resize( font[65].width(), font[65].height() + 3 );
            font[195].reset();
            fheroes2::Copy( font[65], 0, 0, font[195], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[65], 3, 0, font[195], 3, 0, 1, 1 );
            fheroes2::Copy( font[65], 3, 0, font[195], 4, 1, 2, 1 );
            fheroes2::Copy( font[65], 3, 0, font[195], 6, 0, 1, 1 );
            font[195].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[195] );

            // E with grave accent `.
            font[200].resize( font[69].width(), font[69].height() + 4 );
            font[200].reset();
            fheroes2::Copy( font[69], 0, 0, font[200], 0, 4, font[69].width(), font[69].height() );
            fheroes2::Copy( font[192], 4, 0, font[200], 4, 0, 3, 3 );
            font[200].setPosition( font[69].x(), font[69].y() - 4 );
            updateSmallFontLetterShadow( font[200] );

            // E with acute accent.
            font[201].resize( font[69].width(), font[69].height() + 4 );
            font[201].reset();
            fheroes2::Copy( font[69], 0, 0, font[201], 0, 4, font[69].width(), font[69].height() );
            fheroes2::Copy( font[193], 4, 0, font[201], 3, 0, 3, 3 );
            font[201].setPosition( font[69].x(), font[69].y() - 4 );
            updateSmallFontLetterShadow( font[201] );

            // E with circumflex accent.
            font[202].resize( font[69].width(), font[69].height() + 3 );
            font[202].reset();
            fheroes2::Copy( font[69], 0, 0, font[202], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[194], 4, 0, font[202], 3, 0, 3, 2 );
            font[202].setPosition( font[69].x(), font[69].y() - 3 );
            updateSmallFontLetterShadow( font[202] );

            // [204] COMBINING GRAVE TONE MARK `.

            // I with acute accent.
            font[205].resize( font[73].width(), font[73].height() + 4 );
            font[205].reset();
            fheroes2::Copy( font[73], 0, 0, font[205], 0, 4, font[73].width(), font[73].height() );
            fheroes2::Copy( font[193], 4, 0, font[205], 1, 0, 3, 3 );
            font[205].setPosition( font[73].x(), font[73].y() - 4 );
            updateSmallFontLetterShadow( font[205] );

            // D with stroke. Needs stroke generated.
            font[208] = font[68];

            // [210] COMBINING HOOK ABOVE.

            // O with acute accent.
            font[211].resize( font[79].width(), font[79].height() + 4 );
            font[211].reset();
            fheroes2::Copy( font[79], 0, 0, font[211], 0, 4, font[79].width(), font[79].height() );
            fheroes2::Copy( font[193], 4, 0, font[211], 3, 0, 3, 3 );
            font[211].setPosition( font[79].x(), font[79].y() - 4 );
            updateSmallFontLetterShadow( font[211] );

            // O with circumflex accent.
            font[212].resize( font[79].width(), font[79].height() + 3 );
            font[212].reset();
            fheroes2::Copy( font[79], 0, 0, font[212], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[194], 4, 0, font[212], 3, 0, 3, 2 );
            font[212].setPosition( font[79].x(), font[79].y() - 3 );
            updateSmallFontLetterShadow( font[212] );

            // O with horn. Needs accent.
            font[213] = font[79];

            // U with grave accent `.
            font[217].resize( font[85].width(), font[85].height() + 4 );
            font[217].reset();
            fheroes2::Copy( font[85], 0, 0, font[217], 0, 4, font[85].width(), font[85].height() );
            fheroes2::Copy( font[192], 4, 0, font[217], 4, 0, 3, 3 );
            font[217].setPosition( font[85].x(), font[85].y() - 4 );
            updateSmallFontLetterShadow( font[217] );

            // U with acute accent.
            font[218].resize( font[85].width(), font[85].height() + 4 );
            font[218].reset();
            fheroes2::Copy( font[85], 0, 0, font[218], 0, 4, font[85].width(), font[85].height() );
            fheroes2::Copy( font[193], 4, 0, font[218], 4, 0, 3, 3 );
            font[218].setPosition( font[85].x(), font[85].y() - 4 );
            updateSmallFontLetterShadow( font[218] );

            // U with horn. Needs accent generated.
            font[221] = font[85];

            // [222] COMBINING TILDE ~.

            // a with grave accent `.
            font[224].resize( font[97].width(), font[97].height() + 3 );
            font[224].reset();
            fheroes2::Copy( font[97], 0, 0, font[224], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[192], 4, 0, font[224], 3, 0, 3, 2 );
            font[224].setPosition( font[97].x(), font[97].y() - 3 );
            updateSmallFontLetterShadow( font[224] );

            // a with acute accent.
            font[225].resize( font[97].width(), font[97].height() + 3 );
            font[225].reset();
            fheroes2::Copy( font[97], 0, 0, font[225], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[193], 5, 0, font[225], 3, 0, 2, 2 );
            font[225].setPosition( font[97].x(), font[97].y() - 3 );
            updateSmallFontLetterShadow( font[225] );

            // a with circumflex accent.
            font[226].resize( font[97].width(), font[97].height() + 3 );
            font[226].reset();
            fheroes2::Copy( font[97], 0, 0, font[226], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[194], 4, 0, font[226], 2, 0, 3, 2 );
            font[226].setPosition( font[97].x(), font[97].y() - 3 );
            updateSmallFontLetterShadow( font[226] );

            // a with breve.
            font[227].resize( font[97].width(), font[97].height() + 3 );
            font[227].reset();
            fheroes2::Copy( font[97], 0, 0, font[227], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[195], 3, 0, font[227], 2, 0, 4, 2 );
            font[227].setPosition( font[97].x(), font[97].y() - 3 );
            updateSmallFontLetterShadow( font[227] );

            // e with grave accent `.
            font[232].resize( font[101].width(), font[101].height() + 3 );
            font[232].reset();
            fheroes2::Copy( font[101], 0, 0, font[232], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[192], 4, 0, font[232], 3, 0, 3, 2 );
            font[232].setPosition( font[101].x(), font[101].y() - 3 );
            updateSmallFontLetterShadow( font[232] );

            // e with acute accent.
            font[233].resize( font[101].width(), font[101].height() + 3 );
            font[233].reset();
            fheroes2::Copy( font[101], 0, 0, font[233], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[193], 5, 0, font[233], 3, 0, 2, 2 );
            font[233].setPosition( font[101].x(), font[101].y() - 3 );
            updateSmallFontLetterShadow( font[233] );

            // e with circumflex accent.
            font[234].resize( font[101].width(), font[101].height() + 3 );
            font[234].reset();
            fheroes2::Copy( font[101], 0, 0, font[234], 0, 3, font[101].width(), font[101].height() );
            fheroes2::Copy( font[194], 4, 0, font[234], 2, 0, 3, 2 );
            font[234].setPosition( font[101].x(), font[101].y() - 3 );
            updateSmallFontLetterShadow( font[234] );

            // [236] COMBINING ACUTE TONE MARK.

            // i with acute accent.
            font[237].resize( font[105].width(), font[105].height() + 1 );
            font[237].reset();
            fheroes2::Copy( font[105], 0, 2, font[237], 0, 3, font[105].width(), 6 );
            fheroes2::Copy( font[193], 5, 0, font[237], 1, 0, 2, 2 );
            font[237].setPosition( font[105].x(), font[105].y() - 1 );
            updateSmallFontLetterShadow( font[237] );

            // d with stroke. Needs stroke generated.
            font[240] = font[100];

            // [242] COMBINING DOT BELOW.

            // o with acute accent.
            font[243].resize( font[111].width(), font[111].height() + 3 );
            font[243].reset();
            fheroes2::Copy( font[111], 0, 0, font[243], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[193], 5, 0, font[243], 3, 0, 2, 2 );
            font[243].setPosition( font[111].x(), font[111].y() - 3 );
            updateSmallFontLetterShadow( font[243] );

            // o with circumflex accent.
            font[244].resize( font[111].width(), font[111].height() + 3 );
            font[244].reset();
            fheroes2::Copy( font[111], 0, 0, font[244], 0, 3, font[111].width(), font[111].height() );
            fheroes2::Copy( font[194], 4, 0, font[244], 2, 0, 3, 2 );
            font[244].setPosition( font[111].x(), font[111].y() - 3 );
            updateSmallFontLetterShadow( font[244] );

            // o with horn. Needs accent to be generated.
            font[245] = font[111];

            // u with grave accent `.
            font[249].resize( font[117].width(), font[117].height() + 3 );
            font[249].reset();
            fheroes2::Copy( font[117], 0, 0, font[249], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[192], 4, 0, font[249], 3, 0, 3, 2 );
            font[249].setPosition( font[117].x(), font[117].y() - 3 );
            updateSmallFontLetterShadow( font[249] );

            // u with acute accent.
            font[250].resize( font[117].width(), font[117].height() + 3 );
            font[250].reset();
            fheroes2::Copy( font[117], 0, 0, font[250], 0, 3, font[117].width(), font[117].height() );
            fheroes2::Copy( font[193], 5, 0, font[250], 3, 0, 2, 2 );
            font[250].setPosition( font[117].x(), font[117].y() - 3 );
            updateSmallFontLetterShadow( font[250] );

            // u with horn.
            font[253] = font[117];
        }
    }

    void generateISO8859_16Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        resizeCodePage( icnVsSprite );

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // NBSP character.
            font[160].resize( normalFontSpaceWidth, 1 );
            font[160].reset();

            // S with comma.
            font[170].resize( font[83].width(), font[83].height() + 4 );
            font[170].reset();
            fheroes2::Copy( font[83], 0, 0, font[170], 0, 0, font[83].width(), font[83].height() );
            fheroes2::Copy( font[44], 0, 0, font[170], 3, 12, font[44].width(), font[44].height() );
            font[170].setPosition( font[83].x(), font[83].y() );
            updateNormalFontLetterShadow( font[170] );

            // A with circumflex and generate the accent for further use.
            font[194].resize( font[65].width(), font[65].height() + 3 );
            font[194].reset();
            fheroes2::Copy( font[65], 0, 0, font[194], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[65], 3, 0, font[194], 7, 0, 1, 1 );
            fheroes2::Copy( font[65], 3, 0, font[194], 6, 1, 1, 1 );
            fheroes2::Copy( font[65], 3, 0, font[194], 10, 0, 1, 1 );
            fheroes2::Copy( font[65], 3, 0, font[194], 11, 1, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[194], 8, 0, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[194], 7, 1, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[194], 10, 1, 1, 1 );
            fheroes2::Copy( font[65], 4, 0, font[194], 9, 0, 1, 1 );
            font[194].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[194] );

            // A with breve and generate the accent for further use.
            font[195].resize( font[65].width(), font[65].height() + 3 );
            font[195].reset();
            fheroes2::Copy( font[65], 0, 0, font[195], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[71], 5, 9, font[195], 5, 0, 7, 2 );
            fheroes2::FillTransform( font[195], 7, 0, 3, 1, 1 );
            font[195].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[195] );

            // I with circumflex
            font[206].resize( font[73].width(), font[73].height() + 3 );
            font[206].reset();
            fheroes2::Copy( font[73], 0, 0, font[206], 0, 3, font[73].width(), font[73].height() );
            fheroes2::Copy( font[194], 6, 0, font[206], 1, 0, 6, 2 );
            font[206].setPosition( font[73].x(), font[73].y() - 3 );
            updateNormalFontLetterShadow( font[206] );

            // T with comma.
            font[222].resize( font[84].width(), font[84].height() + 4 );
            font[222].reset();
            fheroes2::Copy( font[84], 0, 0, font[222], 0, 0, font[84].width(), font[84].height() );
            fheroes2::Copy( font[44], 0, 0, font[222], 3, 12, font[44].width(), font[44].height() );
            font[222].setPosition( font[84].x(), font[84].y() );
            updateNormalFontLetterShadow( font[222] );

            // s with comma.
            font[186].resize( font[115].width(), font[115].height() + 4 );
            font[186].reset();
            fheroes2::Copy( font[115], 0, 0, font[186], 0, 0, font[115].width(), font[115].height() );
            fheroes2::Copy( font[44], 0, 0, font[186], 2, 8, font[44].width(), font[44].height() );
            font[186].setPosition( font[115].x(), font[115].y() );
            updateNormalFontLetterShadow( font[186] );

            // a with circumflex.
            font[226].resize( font[97].width(), font[97].height() + 3 );
            font[226].reset();
            fheroes2::Copy( font[97], 0, 0, font[226], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[194], 6, 0, font[226], 2, 0, 3, 2 );
            fheroes2::Copy( font[194], 10, 0, font[226], 5, 0, 2, 2 );
            font[226].setPosition( font[97].x(), font[97].y() - 3 );
            updateNormalFontLetterShadow( font[226] );

            // a with breve.
            font[227].resize( font[97].width(), font[97].height() + 3 );
            font[227].reset();
            fheroes2::Copy( font[97], 0, 0, font[227], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[97], 2, 0, font[227], 3, 0, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[227], 6, 0, 1, 1 );
            fheroes2::Copy( font[97], 1, 0, font[227], 3, 1, 1, 1 );
            fheroes2::Copy( font[97], 2, 0, font[227], 4, 1, 2, 1 );
            fheroes2::Copy( font[97], 1, 0, font[227], 6, 1, 1, 1 );
            font[227].setPosition( font[97].x(), font[97].y() - 3 );
            updateNormalFontLetterShadow( font[227] );

            // i with circumflex
            font[238].resize( font[105].width(), font[105].height() );
            font[238].reset();
            fheroes2::Copy( font[105], 0, 0, font[238], 0, 0, font[105].width(), font[105].height() );
            fheroes2::Copy( font[226], 3, 0, font[238], 1, 0, 3, 2 );
            font[238].setPosition( font[105].x(), font[105].y() );
            updateNormalFontLetterShadow( font[238] );

            // t with comma.
            font[254].resize( font[116].width(), font[116].height() + 4 );
            font[254].reset();
            fheroes2::Copy( font[116], 0, 0, font[254], 0, 0, font[116].width(), font[116].height() );
            fheroes2::Copy( font[44], 0, 0, font[254], 1, 12, font[44].width(), font[44].height() );
            font[254].setPosition( font[116].x(), font[116].y() );
            updateNormalFontLetterShadow( font[254] );
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // NBSP character.
            font[160].resize( smallFontSpaceWidth, 1 );
            font[160].reset();

            // S with comma.
            font[170].resize( font[83].width(), font[83].height() + 4 );
            font[170].reset();
            fheroes2::Copy( font[83], 0, 0, font[170], 0, 0, font[83].width(), font[83].height() );
            fheroes2::Copy( font[44], 0, 0, font[170], 2, 8, font[44].width(), font[44].height() );
            font[170].setPosition( font[83].x(), font[83].y() );
            updateSmallFontLetterShadow( font[170] );

            // A with circumflex and generate the accent for further use.
            font[194].resize( font[65].width(), font[65].height() + 3 );
            font[194].reset();
            fheroes2::Copy( font[65], 0, 0, font[194], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[65], 3, 0, font[194], 5, 0, 1, 1 );
            fheroes2::Copy( font[65], 3, 0, font[194], 4, 1, 1, 1 );
            fheroes2::Copy( font[65], 3, 0, font[194], 6, 1, 1, 1 );
            font[194].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[194] );

            // A with breve and generate the accent for further use.
            font[195].resize( font[65].width(), font[65].height() + 3 );
            font[195].reset();
            fheroes2::Copy( font[65], 0, 0, font[195], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[65], 3, 0, font[195], 3, 0, 1, 1 );
            fheroes2::Copy( font[65], 3, 0, font[195], 4, 1, 2, 1 );
            fheroes2::Copy( font[65], 3, 0, font[195], 6, 0, 1, 1 );
            font[195].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[195] );

            // I with circumflex
            font[206].resize( font[73].width(), font[73].height() + 3 );
            font[206].reset();
            fheroes2::Copy( font[73], 0, 0, font[206], 0, 3, font[73].width(), font[73].height() );
            fheroes2::Copy( font[194], 4, 0, font[206], 1, 0, 3, 2 );
            font[206].setPosition( font[73].x(), font[73].y() - 3 );
            updateSmallFontLetterShadow( font[206] );

            // T with comma.
            font[222].resize( font[84].width(), font[84].height() + 4 );
            font[222].reset();
            fheroes2::Copy( font[84], 0, 0, font[222], 0, 0, font[84].width(), font[84].height() );
            fheroes2::Copy( font[44], 0, 0, font[222], 2, 8, font[44].width(), font[44].height() );
            font[222].setPosition( font[84].x(), font[84].y() );
            updateSmallFontLetterShadow( font[222] );

            // s with comma.
            font[186].resize( font[115].width(), font[115].height() + 4 );
            font[186].reset();
            fheroes2::Copy( font[115], 0, 0, font[186], 0, 0, font[115].width(), font[115].height() );
            fheroes2::Copy( font[44], 0, 0, font[186], 1, 6, font[44].width(), font[44].height() );
            font[186].setPosition( font[115].x(), font[115].y() );
            updateSmallFontLetterShadow( font[186] );

            // a with circumflex.
            font[226].resize( font[97].width(), font[97].height() + 3 );
            font[226].reset();
            fheroes2::Copy( font[97], 0, 0, font[226], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[194], 4, 0, font[226], 2, 0, 3, 2 );
            font[226].setPosition( font[97].x(), font[97].y() - 3 );
            updateSmallFontLetterShadow( font[226] );

            // a with breve.
            font[227].resize( font[97].width(), font[97].height() + 3 );
            font[227].reset();
            fheroes2::Copy( font[97], 0, 0, font[227], 0, 3, font[97].width(), font[97].height() );
            fheroes2::Copy( font[195], 3, 0, font[227], 2, 0, 4, 2 );
            font[227].setPosition( font[97].x(), font[97].y() - 3 );
            updateSmallFontLetterShadow( font[227] );

            // i with circumflex
            font[238].resize( font[105].width(), font[105].height() + 1 );
            font[238].reset();
            fheroes2::Copy( font[105], 0, 0, font[238], 0, 1, font[105].width(), font[105].height() );
            fheroes2::Copy( font[226], 2, 0, font[238], 1, 0, 3, 2 );
            font[238].setPosition( font[105].x(), font[105].y() - 1 );
            updateSmallFontLetterShadow( font[238] );

            // t with comma.
            font[254].resize( font[116].width(), font[116].height() + 4 );
            font[254].reset();
            fheroes2::Copy( font[116], 0, 0, font[254], 0, 0, font[116].width(), font[116].height() );
            fheroes2::Copy( font[44], 0, 0, font[254], 1, 8, font[44].width(), font[44].height() );
            font[254].setPosition( font[116].x(), font[116].y() );
            updateSmallFontLetterShadow( font[254] );
        }
    }

    void generateISO8859_3Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        resizeCodePage( icnVsSprite );

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // NBSP character.
            font[160].resize( normalFontSpaceWidth, 1 );
            font[160].reset();
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // NBSP character.
            font[160].resize( smallFontSpaceWidth, 1 );
            font[160].reset();
        }
    }

    void generateGoodButtonFontBaseShape( std::vector<fheroes2::Sprite> & released )
    {
        // Button font does not exist in the original game assets but we can regenerate it from scratch.
        // All letters in buttons have some variations in colors but overall shapes are the same.
        // We want to standardize the font and to use one approach to generate letters.
        // The shape of the letter is defined only by one color (in general). The rest of information is generated from transformations and contours.
        //
        // Another essential difference from normal fonts is that button font has only uppercase letters.
        // This means that we need to generate only 26 letter of English alphabet, 10 digits and few special characters, totalling in about 50 symbols.
        // The downside of this font is that code is necessary for the generation of released and pressed states of each letter.

        released.resize( asciiCharSetSize );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // Since all symbols have -1 shift by X axis to avoid any issues with alignment we need to makes all images at least 1 pixel in size.
        // These images are completely transparent.
        for ( fheroes2::Sprite & letter : released ) {
            letter.resize( 1, 1 );
            letter.reset();

            letter.setPosition( buttonFontOffset.x, buttonFontOffset.y );
        }
        // Address symbols that should have even less space to neighboring symbols.
        released[33].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[45].setPosition( buttonFontOffset.x - 2, buttonFontOffset.y );
        released[46].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[58].setPosition( buttonFontOffset.x - 2, buttonFontOffset.y );
        released[59].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[63].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[65].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[86].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[89].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );

        // !
        released[33].resize( 2 + offset * 2, 10 + offset * 2 );
        released[33].reset();
        fheroes2::DrawLine( released[33], { offset + 0, offset + 0 }, { offset + 0, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[33], { offset + 1, offset + 0 }, { offset + 1, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[33], { offset + 0, offset + 8 }, { offset + 0, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[33], { offset + 1, offset + 8 }, { offset + 1, offset + 9 }, buttonGoodReleasedColor );

        // "
        released[34].resize( 6 + offset * 2, 10 + offset * 2 );
        released[34].reset();
        fheroes2::DrawLine( released[34], { offset + 1, offset + 0 }, { offset + 1, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[34], { offset + 5, offset + 0 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[34], offset + 0, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[34], offset + 4, offset + 0, buttonGoodReleasedColor );

        // #
        released[35].resize( 10 + offset * 2, 10 + offset * 2 );
        released[35].reset();
        fheroes2::DrawLine( released[35], { offset + 1, offset + 3 }, { offset + 9, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[35], { offset + 0, offset + 6 }, { offset + 8, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[35], { offset + 4, offset + 0 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[35], { offset + 7, offset + 0 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );

        // %
        released[37].resize( 9 + offset * 2, 10 + offset * 2 );
        released[37].reset();
        fheroes2::DrawLine( released[37], { offset + 6, offset + 0 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[37], { offset + 1, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[37], { offset + 1, offset + 3 }, { offset + 2, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[37], { offset + 0, offset + 1 }, { offset + 0, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[37], { offset + 3, offset + 1 }, { offset + 3, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[37], { offset + 7, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[37], { offset + 7, offset + 6 }, { offset + 6, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[37], { offset + 8, offset + 8 }, { offset + 8, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[37], { offset + 5, offset + 8 }, { offset + 5, offset + 7 }, buttonGoodReleasedColor );

        // &
        released[38].resize( 8 + offset * 2, 10 + offset * 2 );
        released[38].reset();
        fheroes2::DrawLine( released[38], { offset + 2, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[38], { offset + 1, offset + 1 }, { offset + 1, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[38], { offset + 4, offset + 1 }, { offset + 4, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[38], { offset + 2, offset + 3 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[38], { offset + 3, offset + 3 }, { offset + 1, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[38], { offset + 0, offset + 6 }, { offset + 0, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[38], { offset + 1, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[38], { offset + 5, offset + 8 }, { offset + 7, offset + 5 }, buttonGoodReleasedColor );

        // '
        released[39].resize( 2 + offset * 2, 10 + offset * 2 );
        released[39].reset();
        fheroes2::DrawLine( released[39], { offset + 1, offset + 0 }, { offset + 1, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[39], offset + 0, offset + 0, buttonGoodReleasedColor );

        // (
        released[40].resize( 3 + offset * 2, 10 + offset * 2 );
        released[40].reset();
        fheroes2::DrawLine( released[40], { offset + 0, offset + 3 }, { offset + 0, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[40], { offset + 1, offset + 1 }, { offset + 1, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[40], { offset + 1, offset + 7 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[40], offset + 2, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[40], offset + 2, offset + 9, buttonGoodReleasedColor );

        // )
        released[41].resize( 3 + offset * 2, 10 + offset * 2 );
        released[41].reset();
        fheroes2::DrawLine( released[41], { offset + 2, offset + 3 }, { offset + 2, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[41], { offset + 1, offset + 1 }, { offset + 1, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[41], { offset + 1, offset + 7 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[41], offset + 0, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[41], offset + 0, offset + 9, buttonGoodReleasedColor );

        //*
        released[42].resize( 5 + offset * 2, 10 + offset * 2 );
        released[42].reset();
        fheroes2::DrawLine( released[42], { offset + 2, offset + 0 }, { offset + 2, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[42], { offset + 0, offset + 1 }, { offset + 4, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[42], { offset + 0, offset + 4 }, { offset + 4, offset + 1 }, buttonGoodReleasedColor );

        // +
        released[43].resize( 5 + offset * 2, 10 + offset * 2 );
        released[43].reset();
        fheroes2::DrawLine( released[43], { offset + 0, offset + 5 }, { offset + 4, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[43], { offset + 2, offset + 3 }, { offset + 2, offset + 7 }, buttonGoodReleasedColor );

        // ,
        released[44].resize( 3 + offset * 2, 11 + offset * 2 );
        released[44].reset();
        fheroes2::DrawLine( released[44], { offset + 1, offset + 8 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[44], { offset + 1, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[44], { offset + 0, offset + 10 }, { offset + 1, offset + 10 }, buttonGoodReleasedColor );

        // -
        released[45].resize( 6 + offset * 2, 6 + offset * 2 );
        released[45].reset();
        fheroes2::DrawLine( released[45], { offset + 0, offset + 5 }, { offset + 5, offset + 5 }, buttonGoodReleasedColor );

        // .
        released[46].resize( 2 + offset * 2, 10 + offset * 2 );
        released[46].reset();
        fheroes2::DrawLine( released[46], { offset + 0, offset + 8 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[46], { offset + 0, offset + 9 }, { offset + 1, offset + 9 }, buttonGoodReleasedColor );

        // /
        released[47].resize( 4 + offset * 2, 10 + offset * 2 );
        released[47].reset();
        fheroes2::DrawLine( released[47], { offset + 3, offset + 0 }, { offset + 0, offset + 9 }, buttonGoodReleasedColor );

        // 0
        released[48].resize( 9 + offset * 2, 10 + offset * 2 );
        released[48].reset();
        fheroes2::DrawLine( released[48], { offset + 2, offset + 0 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[48], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[48], { offset + 2, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[48], { offset + 8, offset + 2 }, { offset + 8, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[48], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[48], offset + 7, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[48], offset + 1, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[48], offset + 7, offset + 8, buttonGoodReleasedColor );

        // 1
        released[49].resize( 5 + offset * 2, 10 + offset * 2 );
        released[49].reset();
        fheroes2::DrawLine( released[49], { offset + 2, offset + 0 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[49], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[49], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[49], offset + 0, offset + 2, buttonGoodReleasedColor );

        // 2
        released[50].resize( 7 + offset * 2, 10 + offset * 2 );
        released[50].reset();
        fheroes2::DrawLine( released[50], { offset + 1, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[50], { offset + 6, offset + 1 }, { offset + 6, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[50], { offset + 5, offset + 4 }, { offset + 0, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[50], { offset + 1, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[50], { offset + 0, offset + 1 }, { offset + 0, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[50], offset + 6, offset + 8, buttonGoodReleasedColor );

        // 3
        released[51].resize( 7 + offset * 2, 10 + offset * 2 );
        released[51].reset();
        fheroes2::DrawLine( released[51], { offset + 1, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[51], { offset + 6, offset + 1 }, { offset + 6, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[51], { offset + 2, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[51], { offset + 6, offset + 5 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[51], { offset + 1, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[51], offset + 0, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[51], offset + 0, offset + 8, buttonGoodReleasedColor );

        // 4
        released[52].resize( 8 + offset * 2, 10 + offset * 2 );
        released[52].reset();
        fheroes2::DrawLine( released[52], { offset + 0, offset + 4 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[52], { offset + 5, offset + 0 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[52], { offset + 5, offset + 6 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[52], { offset + 0, offset + 5 }, { offset + 7, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[52], { offset + 3, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );

        // 5
        released[53].resize( 7 + offset * 2, 10 + offset * 2 );
        released[53].reset();
        fheroes2::DrawLine( released[53], { offset + 0, offset + 0 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[53], { offset + 0, offset + 1 }, { offset + 0, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[53], { offset + 1, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[53], { offset + 6, offset + 5 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[53], { offset + 1, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[53], offset + 0, offset + 8, buttonGoodReleasedColor );

        // 6
        released[54].resize( 7 + offset * 2, 10 + offset * 2 );
        released[54].reset();
        fheroes2::DrawLine( released[54], { offset + 1, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[54], { offset + 0, offset + 1 }, { offset + 0, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[54], { offset + 1, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[54], { offset + 6, offset + 5 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[54], { offset + 1, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[54], offset + 6, offset + 1, buttonGoodReleasedColor );

        // 7
        released[55].resize( 7 + offset * 2, 10 + offset * 2 );
        released[55].reset();
        fheroes2::DrawLine( released[55], { offset + 0, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[55], { offset + 6, offset + 0 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[55], offset + 0, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[55], offset + 1, offset + 9, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[55], offset + 3, offset + 9, buttonGoodReleasedColor );

        // 8
        released[56].resize( 7 + offset * 2, 10 + offset * 2 );
        released[56].reset();
        fheroes2::DrawLine( released[56], { offset + 1, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[56], { offset + 0, offset + 1 }, { offset + 0, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[56], { offset + 6, offset + 1 }, { offset + 6, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[56], { offset + 1, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[56], { offset + 0, offset + 5 }, { offset + 0, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[56], { offset + 6, offset + 5 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[56], { offset + 1, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );

        // 9
        released[57].resize( 7 + offset * 2, 10 + offset * 2 );
        released[57].reset();
        fheroes2::DrawLine( released[57], { offset + 1, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[57], { offset + 0, offset + 1 }, { offset + 0, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[57], { offset + 6, offset + 1 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[57], { offset + 1, offset + 5 }, { offset + 5, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[57], { offset + 1, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[57], offset + 0, offset + 8, buttonGoodReleasedColor );

        // :
        released[58].resize( 2 + offset * 2, 10 + offset * 2 );
        released[58].reset();
        fheroes2::DrawLine( released[58], { offset + 0, offset + 3 }, { offset + 1, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[58], { offset + 0, offset + 4 }, { offset + 1, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[58], { offset + 0, offset + 8 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[58], { offset + 0, offset + 9 }, { offset + 1, offset + 9 }, buttonGoodReleasedColor );

        // ;
        released[59].resize( 3 + offset * 2, 11 + offset * 2 );
        released[59].reset();
        fheroes2::DrawLine( released[59], { offset + 1, offset + 3 }, { offset + 2, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[59], { offset + 1, offset + 4 }, { offset + 2, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[59], { offset + 1, offset + 8 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[59], { offset + 1, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[59], { offset + 0, offset + 10 }, { offset + 1, offset + 10 }, buttonGoodReleasedColor );

        // <
        released[60].resize( 4 + offset * 2, 10 + offset * 2 );
        released[60].reset();
        fheroes2::DrawLine( released[60], { offset + 3, offset + 2 }, { offset + 0, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[60], { offset + 3, offset + 8 }, { offset + 1, offset + 6 }, buttonGoodReleasedColor );

        // =
        released[61].resize( 6 + offset * 2, 8 + offset * 2 );
        released[61].reset();
        fheroes2::DrawLine( released[61], { offset + 0, offset + 3 }, { offset + 5, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[61], { offset + 0, offset + 7 }, { offset + 5, offset + 7 }, buttonGoodReleasedColor );

        // >
        released[62].resize( 4 + offset * 2, 10 + offset * 2 );
        released[62].reset();
        fheroes2::DrawLine( released[62], { offset + 0, offset + 2 }, { offset + 3, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[62], { offset + 0, offset + 8 }, { offset + 2, offset + 6 }, buttonGoodReleasedColor );

        // ?
        released[63].resize( 6 + offset * 2, 10 + offset * 2 );
        released[63].reset();
        fheroes2::DrawLine( released[63], { offset + 0, offset + 1 }, { offset + 0, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[63], { offset + 1, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[63], { offset + 5, offset + 1 }, { offset + 5, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[63], { offset + 4, offset + 3 }, { offset + 3, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[63], { offset + 2, offset + 8 }, { offset + 3, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[63], { offset + 2, offset + 9 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );

        // A
        released[65].resize( 13 + offset * 2, 10 + offset * 2 );
        released[65].reset();
        fheroes2::DrawLine( released[65], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[65], { offset + 8, offset + 9 }, { offset + 12, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[65], { offset + 5, offset + 5 }, { offset + 8, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[65], { offset + 2, offset + 8 }, { offset + 4, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[65], { offset + 7, offset + 1 }, { offset + 10, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[65], offset + 4, offset + 4, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[65], offset + 5, offset + 3, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[65], offset + 5, offset + 2, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[65], offset + 6, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[65], offset + 6, offset + 0, buttonGoodReleasedColor );

        // B
        released[66].resize( 11 + offset * 2, 10 + offset * 2 );
        released[66].reset();
        fheroes2::DrawLine( released[66], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[66], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[66], { offset + 3, offset + 5 }, { offset + 9, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[66], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[66], { offset + 10, offset + 2 }, { offset + 10, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[66], { offset + 10, offset + 6 }, { offset + 10, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[66], offset + 9, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[66], offset + 9, offset + 1, buttonGoodReleasedColor );

        // C
        released[67].resize( 10 + offset * 2, 10 + offset * 2 );
        released[67].reset();
        fheroes2::DrawLine( released[67], { offset + 2, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[67], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[67], { offset + 2, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[67], { offset + 9, offset + 0 }, { offset + 9, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[67], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[67], offset + 1, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[67], offset + 8, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[67], offset + 8, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[67], offset + 9, offset + 7, buttonGoodReleasedColor );

        // D
        released[68].resize( 11 + offset * 2, 10 + offset * 2 );
        released[68].reset();
        fheroes2::DrawLine( released[68], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[68], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[68], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[68], { offset + 10, offset + 2 }, { offset + 10, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[68], offset + 9, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[68], offset + 9, offset + 8, buttonGoodReleasedColor );

        // E
        released[69].resize( 9 + offset * 2, 10 + offset * 2 );
        released[69].reset();
        fheroes2::DrawLine( released[69], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[69], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[69], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[69], { offset + 3, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[69], offset + 8, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[69], offset + 8, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[69], offset + 6, offset + 3, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[69], offset + 6, offset + 5, buttonGoodReleasedColor );

        // F
        released[70].resize( 9 + offset * 2, 10 + offset * 2 );
        released[70].reset();
        fheroes2::DrawLine( released[70], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[70], { offset + 0, offset + 9 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[70], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[70], { offset + 3, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[70], offset + 8, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[70], offset + 6, offset + 3, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[70], offset + 6, offset + 5, buttonGoodReleasedColor );

        // G
        released[71].resize( 11 + offset * 2, 10 + offset * 2 );
        released[71].reset();
        fheroes2::DrawLine( released[71], { offset + 2, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[71], { offset + 2, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[71], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[71], { offset + 7, offset + 5 }, { offset + 10, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[71], { offset + 9, offset + 0 }, { offset + 9, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[71], { offset + 9, offset + 6 }, { offset + 9, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[71], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[71], offset + 1, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[71], offset + 8, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[71], offset + 8, offset + 8, buttonGoodReleasedColor );

        // H
        released[72].resize( 13 + offset * 2, 10 + offset * 2 );
        released[72].reset();
        fheroes2::DrawLine( released[72], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[72], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[72], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[72], { offset + 8, offset + 0 }, { offset + 12, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[72], { offset + 8, offset + 9 }, { offset + 12, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[72], { offset + 10, offset + 1 }, { offset + 10, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[72], { offset + 3, offset + 5 }, { offset + 9, offset + 5 }, buttonGoodReleasedColor );

        // I
        released[73].resize( 5 + offset * 2, 10 + offset * 2 );
        released[73].reset();
        fheroes2::DrawLine( released[73], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[73], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[73], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );

        // J
        released[74].resize( 8 + offset * 2, 10 + offset * 2 );
        released[74].reset();
        fheroes2::DrawLine( released[74], { offset + 3, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[74], { offset + 1, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[74], { offset + 5, offset + 1 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[74], { offset + 0, offset + 7 }, { offset + 0, offset + 8 }, buttonGoodReleasedColor );

        // K
        released[75].resize( 12 + offset * 2, 10 + offset * 2 );
        released[75].reset();
        fheroes2::DrawLine( released[75], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[75], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[75], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[75], { offset + 3, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[75], { offset + 6, offset + 3 }, { offset + 8, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[75], { offset + 6, offset + 5 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[75], { offset + 7, offset + 0 }, { offset + 10, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[75], { offset + 8, offset + 9 }, { offset + 11, offset + 9 }, buttonGoodReleasedColor );

        // L
        released[76].resize( 9 + offset * 2, 10 + offset * 2 );
        released[76].reset();
        fheroes2::DrawLine( released[76], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[76], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[76], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[76], offset + 8, offset + 8, buttonGoodReleasedColor );

        // M
        released[77].resize( 15 + offset * 2, 10 + offset * 2 );
        released[77].reset();
        fheroes2::DrawLine( released[77], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[77], { offset + 2, offset + 0 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[77], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[77], { offset + 3, offset + 1 }, { offset + 7, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[77], { offset + 8, offset + 4 }, { offset + 11, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[77], { offset + 12, offset + 1 }, { offset + 12, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[77], { offset + 12, offset + 0 }, { offset + 14, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[77], { offset + 10, offset + 9 }, { offset + 14, offset + 9 }, buttonGoodReleasedColor );

        // N
        released[78].resize( 14 + offset * 2, 10 + offset * 2 );
        released[78].reset();
        fheroes2::DrawLine( released[78], { offset + 0, offset + 0 }, { offset + 1, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[78], { offset + 2, offset + 0 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[78], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[78], { offset + 3, offset + 1 }, { offset + 10, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[78], { offset + 9, offset + 0 }, { offset + 13, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[78], { offset + 11, offset + 0 }, { offset + 11, offset + 9 }, buttonGoodReleasedColor );

        // O
        released[79].resize( 10 + offset * 2, 10 + offset * 2 );
        released[79].reset();
        fheroes2::DrawLine( released[79], { offset + 2, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[79], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[79], { offset + 2, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[79], { offset + 9, offset + 2 }, { offset + 9, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[79], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[79], offset + 8, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[79], offset + 1, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[79], offset + 8, offset + 8, buttonGoodReleasedColor );

        // P
        released[80].resize( 11 + offset * 2, 10 + offset * 2 );
        released[80].reset();
        fheroes2::DrawLine( released[80], { offset + 0, offset + 0 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[80], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[80], { offset + 3, offset + 5 }, { offset + 9, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[80], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[80], { offset + 10, offset + 1 }, { offset + 10, offset + 4 }, buttonGoodReleasedColor );

        // Q
        released[81].resize( 11 + offset * 2, 11 + offset * 2 );
        released[81].reset();
        fheroes2::DrawLine( released[81], { offset + 2, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[81], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[81], { offset + 2, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[81], { offset + 9, offset + 2 }, { offset + 9, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[81], { offset + 4, offset + 7 }, { offset + 5, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[81], { offset + 6, offset + 7 }, { offset + 9, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[81], { offset + 10, offset + 10 }, { offset + 11, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[81], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[81], offset + 8, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[81], offset + 1, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[81], offset + 8, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[81], offset + 12, offset + 9, buttonGoodReleasedColor );

        // R
        released[82].resize( 12 + offset * 2, 10 + offset * 2 );
        released[82].reset();
        fheroes2::DrawLine( released[82], { offset + 0, offset + 0 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[82], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[82], { offset + 8, offset + 9 }, { offset + 11, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[82], { offset + 3, offset + 5 }, { offset + 9, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[82], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[82], { offset + 10, offset + 1 }, { offset + 10, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[82], { offset + 7, offset + 6 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );

        // S
        released[83].resize( 9 + offset * 2, 10 + offset * 2 );
        released[83].reset();
        fheroes2::DrawLine( released[83], { offset + 1, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[83], { offset + 0, offset + 1 }, { offset + 0, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[83], { offset + 1, offset + 4 }, { offset + 7, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[83], { offset + 8, offset + 5 }, { offset + 8, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[83], { offset + 1, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[83], { offset + 0, offset + 8 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[83], offset + 8, offset + 1, buttonGoodReleasedColor );

        // T
        released[84].resize( 11 + offset * 2, 10 + offset * 2 );
        released[84].reset();
        fheroes2::DrawLine( released[84], { offset + 0, offset + 0 }, { offset + 10, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[84], { offset + 5, offset + 1 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[84], { offset + 0, offset + 1 }, { offset + 0, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[84], { offset + 10, offset + 1 }, { offset + 10, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[84], { offset + 4, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );

        // U
        released[85].resize( 13 + offset * 2, 10 + offset * 2 );
        released[85].reset();
        fheroes2::DrawLine( released[85], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[85], { offset + 8, offset + 0 }, { offset + 12, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[85], { offset + 2, offset + 1 }, { offset + 2, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[85], { offset + 10, offset + 1 }, { offset + 10, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[85], { offset + 4, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[85], offset + 3, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[85], offset + 9, offset + 8, buttonGoodReleasedColor );

        // V
        released[86].resize( 11 + offset * 2, 10 + offset * 2 );
        released[86].reset();
        fheroes2::DrawLine( released[86], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[86], { offset + 6, offset + 0 }, { offset + 10, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[86], { offset + 2, offset + 1 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[86], { offset + 8, offset + 1 }, { offset + 6, offset + 7 }, buttonGoodReleasedColor );

        // W
        released[87].resize( 17 + offset * 2, 10 + offset * 2 );
        released[87].reset();
        fheroes2::DrawLine( released[87], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[87], { offset + 7, offset + 0 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[87], { offset + 12, offset + 0 }, { offset + 16, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[87], { offset + 2, offset + 1 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[87], { offset + 8, offset + 1 }, { offset + 6, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[87], { offset + 9, offset + 3 }, { offset + 10, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[87], { offset + 14, offset + 1 }, { offset + 11, offset + 9 }, buttonGoodReleasedColor );

        // X
        released[88].resize( 12 + offset * 2, 10 + offset * 2 );
        released[88].reset();
        fheroes2::DrawLine( released[88], { offset + 0, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[88], { offset + 8, offset + 0 }, { offset + 11, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[88], { offset + 0, offset + 9 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[88], { offset + 8, offset + 9 }, { offset + 11, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[88], { offset + 2, offset + 1 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[88], { offset + 2, offset + 8 }, { offset + 9, offset + 1 }, buttonGoodReleasedColor );

        // Y
        released[89].resize( 11 + offset * 2, 10 + offset * 2 );
        released[89].reset();
        fheroes2::DrawLine( released[89], { offset + 0, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[89], { offset + 7, offset + 0 }, { offset + 10, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[89], { offset + 2, offset + 1 }, { offset + 4, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[89], { offset + 6, offset + 3 }, { offset + 8, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[89], { offset + 5, offset + 4 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[89], { offset + 3, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );

        // Z
        released[90].resize( 9 + offset * 2, 10 + offset * 2 );
        released[90].reset();
        fheroes2::DrawLine( released[90], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[90], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[90], { offset + 7, offset + 1 }, { offset + 0, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[90], offset + 0, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[90], offset + 8, offset + 8, buttonGoodReleasedColor );

        // [
        released[91].resize( 4 + offset * 2, 10 + offset * 2 );
        released[91].reset();
        fheroes2::DrawLine( released[91], { offset + 0, offset + 0 }, { offset + 0, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[91], { offset + 1, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[91], { offset + 1, offset + 9 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );

        /* \ */
        released[92].resize( 4 + offset * 2, 10 + offset * 2 );
        released[92].reset();
        fheroes2::DrawLine( released[92], { offset + 0, offset + 0 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );

        // ]
        released[93].resize( 4 + offset * 2, 10 + offset * 2 );
        released[93].reset();
        fheroes2::DrawLine( released[93], { offset + 3, offset + 0 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[93], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[93], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );

        // ^
        released[94].resize( 5 + offset * 2, 3 + offset * 2 );
        released[94].reset();
        fheroes2::DrawLine( released[94], { offset + 0, offset + 2 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[94], { offset + 3, offset + 1 }, { offset + 4, offset + 2 }, buttonGoodReleasedColor );

        // _
        released[95].resize( 8 + offset * 2, 11 + offset * 2 );
        released[95].reset();
        fheroes2::DrawLine( released[95], { offset + 0, offset + 10 }, { offset + 7, offset + 10 }, buttonGoodReleasedColor );

        // | - replaced with Caps Lock symbol for virtual keyboard
        // TODO: put the Caps Lock symbol to a special font to not replace any other ASCII character.
        released[124].resize( 11 + offset * 2, 11 + offset * 2 );
        released[124].reset();
        fheroes2::SetPixel( released[124], offset + 5, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[124], { offset + 4, offset + 1 }, { offset + 6, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[124], { offset + 3, offset + 2 }, { offset + 7, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[124], { offset + 2, offset + 3 }, { offset + 8, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[124], { offset + 1, offset + 4 }, { offset + 9, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[124], { offset + 0, offset + 5 }, { offset + 10, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[124], { offset + 3, offset + 6 }, { offset + 7, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[124], { offset + 3, offset + 7 }, { offset + 7, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[124], { offset + 3, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[124], { offset + 3, offset + 10 }, { offset + 7, offset + 10 }, buttonGoodReleasedColor );

        // ~ - replaced with Backspace symbol (<x]) for virtual keyboard
        // TODO: put the Backspace symbol to a special font to not replace any other ASCII character.
        released[126].resize( 17 + offset * 2, 9 + offset * 2 );
        released[126].reset();
        fheroes2::SetPixel( released[126], offset + 0, offset + 4, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[126], { offset + 1, offset + 3 }, { offset + 1, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[126], { offset + 2, offset + 2 }, { offset + 2, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[126], { offset + 3, offset + 1 }, { offset + 3, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::Fill( released[126], offset + 4, offset + 0, 13, 9, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[126], { offset + 6, offset + 1 }, { offset + 12, offset + 7 }, 12 );
        fheroes2::DrawLine( released[126], { offset + 7, offset + 1 }, { offset + 13, offset + 7 }, 12 );
        fheroes2::DrawLine( released[126], { offset + 12, offset + 1 }, { offset + 6, offset + 7 }, 12 );
        fheroes2::DrawLine( released[126], { offset + 13, offset + 1 }, { offset + 7, offset + 7 }, 12 );

        // Replaced with Change Language symbol for virtual keyboard
        // TODO: put the Change Language symbol to a special font to not replace any other ASCII character.
        released[127].resize( 14 + offset * 2, 13 + offset * 2 );
        released[127].reset();
        released[127].setPosition( buttonFontOffset.x, buttonFontOffset.y - 2 );
        fheroes2::DrawLine( released[127], { offset + 4, offset + 0 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 1, offset + 2 }, { offset + 2, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 1, offset + 3 }, { offset + 3, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 10, offset + 1 }, { offset + 12, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 11, offset + 1 }, { offset + 12, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 0, offset + 4 }, { offset + 13, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 0, offset + 8 }, { offset + 13, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 1, offset + 9 }, { offset + 3, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 1, offset + 10 }, { offset + 2, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 4, offset + 12 }, { offset + 9, offset + 12 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 10, offset + 11 }, { offset + 12, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 11, offset + 11 }, { offset + 12, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 4, offset + 3 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 9, offset + 3 }, { offset + 9, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 5, offset + 1 }, { offset + 5, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 8, offset + 1 }, { offset + 8, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 5, offset + 10 }, { offset + 5, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 8, offset + 10 }, { offset + 8, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 0, offset + 5 }, { offset + 0, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[127], { offset + 13, offset + 5 }, { offset + 13, offset + 7 }, buttonGoodReleasedColor );
    }

    void generateCP1250GoodButtonFont( std::vector<fheroes2::Sprite> & released )
    {
        // Increase size to fit full CP1252 set of characters. Fill with 1px transparent images.
        const fheroes2::Sprite firstSprite{ released[0] };
        released.insert( released.end(), codePageExtraCharacterCount, firstSprite );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // Offset letters with diacritics above them.
        for ( const int charCode : { 138, 140, 141, 142, 143, 175, 192, 193, 194, 195, 196, 197, 198, 200, 201, 203,
                                     204, 205, 206, 207, 209, 210, 211, 212, 213, 214, 216, 218, 219, 220, 221 } ) {
            released[charCode].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        }
        released[217].setPosition( buttonFontOffset.x, buttonFontOffset.y - 4 );

        // S with caron.
        released[138].resize( released[83].width(), released[83].height() + 3 );
        released[138].reset();
        fheroes2::Copy( released[83], 0, 0, released[138], 0, 3, released[83].width(), released[83].height() );
        fheroes2::SetPixel( released[138], offset + 3, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[138], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // S with acute accent.
        released[140].resize( released[83].width(), released[83].height() + 3 );
        released[140].reset();
        fheroes2::Copy( released[83], 0, 0, released[140], 0, 3, released[83].width(), released[83].height() );
        fheroes2::DrawLine( released[140], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // T with caron.
        released[141].resize( released[84].width(), released[84].height() + 3 );
        released[141].reset();
        fheroes2::Copy( released[84], 0, 0, released[141], 0, 3, released[84].width(), released[84].height() );
        fheroes2::SetPixel( released[141], offset + 4, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[141], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // Z with caron.
        released[142].resize( released[90].width(), released[90].height() + 3 );
        released[142].reset();
        fheroes2::Copy( released[90], 0, 0, released[142], 0, 3, released[90].width(), released[90].height() );
        fheroes2::SetPixel( released[142], offset + 3, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[142], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // Z with acute.
        released[143].resize( released[90].width(), released[90].height() + 3 );
        released[143].reset();
        fheroes2::Copy( released[90], 0, 0, released[143], 0, 3, released[90].width(), released[90].height() );
        fheroes2::DrawLine( released[143], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // NBSP character.
        released[160].resize( bigFontSpaceWidth, 1 );
        released[160].reset();

        // L with stroke.
        released[163] = released[76];
        fheroes2::DrawLine( released[163], { offset + 1, offset + 6 }, { offset + 5, offset + 2 }, buttonGoodReleasedColor );

        // A with ogonek.
        released[165].resize( released[65].width(), released[65].height() + 3 );
        released[165].reset();
        fheroes2::Copy( released[65], 0, 0, released[165], 0, 0, released[65].width(), released[65].height() );
        fheroes2::DrawLine( released[165], { offset + 9, offset + 11 }, { offset + 10, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[165], { offset + 10, offset + 12 }, { offset + 11, offset + 12 }, buttonGoodReleasedColor );

        // S with cedilla.
        released[170].resize( released[83].width(), released[83].height() + 3 );
        released[170].reset();
        fheroes2::Copy( released[83], 0, 0, released[170], 0, 0, released[83].width(), released[83].height() );
        fheroes2::DrawLine( released[170], { offset + 4, offset + 10 }, { offset + 5, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[170], { offset + 3, offset + 12 }, { offset + 4, offset + 12 }, buttonGoodReleasedColor );

        // Z with dot above.
        released[175].resize( released[90].width(), released[90].height() + 3 );
        released[175].reset();
        fheroes2::Copy( released[90], 0, 0, released[175], 0, 3, released[90].width(), released[90].height() );
        fheroes2::DrawLine( released[175], { offset + 4, offset + 1 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );

        // L with caron.
        released[188] = released[76];
        fheroes2::DrawLine( released[188], { offset + 6, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[188], { offset + 6, offset + 2 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // R with acute.
        released[192].resize( released[82].width(), released[82].height() + 3 );
        released[192].reset();
        fheroes2::Copy( released[82], 0, 0, released[192], 0, 3, released[82].width(), released[82].height() );
        fheroes2::DrawLine( released[192], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // A with acute.
        released[193].resize( released[65].width(), released[65].height() + 3 );
        released[193].reset();
        fheroes2::Copy( released[65], 0, 0, released[193], 0, 3, released[65].width(), released[65].height() );
        fheroes2::DrawLine( released[193], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // A with circumflex.
        released[194].resize( released[65].width(), released[65].height() + 3 );
        released[194].reset();
        fheroes2::Copy( released[65], 0, 0, released[194], 0, 3, released[65].width(), released[65].height() );
        fheroes2::SetPixel( released[194], offset + 5, offset + 1, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[194], { offset + 6, offset + 0 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // A with breve.
        released[195].resize( released[65].width(), released[65].height() + 3 );
        released[195].reset();
        fheroes2::Copy( released[65], 0, 0, released[195], 0, 3, released[65].width(), released[65].height() );
        fheroes2::DrawLine( released[195], { offset + 5, offset + 0 }, { offset + 6, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[195], { offset + 7, offset + 1 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );

        // A with diaeresis.
        released[196].resize( released[65].width(), released[65].height() + 3 );
        released[196].reset();
        fheroes2::Copy( released[65], 0, 0, released[196], 0, 3, released[65].width(), released[65].height() );
        fheroes2::DrawLine( released[196], { offset + 4, offset + 1 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[196], { offset + 8, offset + 1 }, { offset + 9, offset + 1 }, buttonGoodReleasedColor );

        // L with acute.
        released[197].resize( released[76].width(), released[76].height() + 3 );
        released[197].reset();
        fheroes2::Copy( released[76], 0, 0, released[197], 0, 3, released[76].width(), released[76].height() );
        fheroes2::DrawLine( released[197], { offset + 3, offset + 1 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );

        // C with acute accent.
        released[198].resize( released[67].width(), released[67].height() + 3 );
        released[198].reset();
        fheroes2::Copy( released[67], 0, 0, released[198], 0, 3, released[67].width(), released[67].height() );
        fheroes2::DrawLine( released[198], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // C with cedilla.
        released[199].resize( released[67].width(), released[67].height() + 3 );
        released[199].reset();
        fheroes2::Copy( released[67], 0, 0, released[199], 0, 0, released[67].width(), released[67].height() );
        fheroes2::DrawLine( released[199], { offset + 5, offset + 10 }, { offset + 6, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[199], { offset + 4, offset + 12 }, { offset + 5, offset + 12 }, buttonGoodReleasedColor );

        // C with caron.
        released[200].resize( released[67].width(), released[67].height() + 3 );
        released[200].reset();
        fheroes2::Copy( released[67], 0, 0, released[200], 0, 3, released[67].width(), released[67].height() );
        fheroes2::SetPixel( released[200], offset + 4, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[200], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // E with acute.
        released[201].resize( released[69].width(), released[69].height() + 3 );
        released[201].reset();
        fheroes2::Copy( released[69], 0, 0, released[201], 0, 3, released[69].width(), released[69].height() );
        fheroes2::DrawLine( released[201], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // E with ogonek.
        released[202].resize( released[69].width(), released[69].height() + 3 );
        released[202].reset();
        fheroes2::Copy( released[69], 0, 0, released[202], 0, 0, released[69].width(), released[69].height() );
        fheroes2::DrawLine( released[202], { offset + 6, offset + 11 }, { offset + 7, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[202], { offset + 7, offset + 12 }, { offset + 8, offset + 12 }, buttonGoodReleasedColor );

        // E with diaeresis.
        released[203].resize( released[69].width(), released[69].height() + 3 );
        released[203].reset();
        fheroes2::Copy( released[69], 0, 0, released[203], 0, 3, released[69].width(), released[69].height() );
        fheroes2::DrawLine( released[203], { offset + 2, offset + 1 }, { offset + 3, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[203], { offset + 6, offset + 1 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // E with caron.
        released[204].resize( released[69].width(), released[69].height() + 3 );
        released[204].reset();
        fheroes2::Copy( released[69], 0, 0, released[204], 0, 3, released[69].width(), released[69].height() );
        fheroes2::SetPixel( released[204], offset + 4, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[204], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // I with acute.
        released[205].resize( released[73].width(), released[73].height() + 3 );
        released[205].reset();
        fheroes2::Copy( released[73], 0, 0, released[205], 0, 3, released[73].width(), released[73].height() );
        fheroes2::DrawLine( released[205], { offset + 2, offset + 1 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );

        // I with circumflex.
        released[206].resize( released[73].width(), released[73].height() + 3 );
        released[206].reset();
        fheroes2::Copy( released[73], 0, 0, released[206], 0, 3, released[73].width(), released[73].height() );
        fheroes2::SetPixel( released[206], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[206], { offset + 2, offset + 0 }, { offset + 3, offset + 1 }, buttonGoodReleasedColor );

        // D with caron.
        released[207].resize( released[68].width(), released[68].height() + 3 );
        released[207].reset();
        fheroes2::Copy( released[68], 0, 0, released[207], 0, 3, released[68].width(), released[68].height() );
        fheroes2::SetPixel( released[207], offset + 4, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[207], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // D with stroke.
        released[208] = released[68];
        fheroes2::DrawLine( released[208], { offset + 1, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );

        // N with acute.
        released[209].resize( released[78].width(), released[78].height() + 3 );
        released[209].reset();
        fheroes2::Copy( released[78], 0, 0, released[209], 0, 3, released[78].width(), released[78].height() );
        fheroes2::DrawLine( released[209], { offset + 7, offset + 1 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );

        // N with caron.
        released[210].resize( released[78].width(), released[78].height() + 3 );
        released[210].reset();
        fheroes2::Copy( released[78], 0, 0, released[210], 0, 3, released[78].width(), released[78].height() );
        fheroes2::SetPixel( released[210], offset + 6, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[210], { offset + 7, offset + 1 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );

        // O with acute.
        released[211].resize( released[79].width(), released[79].height() + 3 );
        released[211].reset();
        fheroes2::Copy( released[79], 0, 0, released[211], 0, 3, released[79].width(), released[79].height() );
        fheroes2::DrawLine( released[211], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // O with circumflex.
        released[212].resize( released[79].width(), released[79].height() + 3 );
        released[212].reset();
        fheroes2::Copy( released[79], 0, 0, released[212], 0, 3, released[79].width(), released[79].height() );
        fheroes2::SetPixel( released[212], offset + 4, offset + 1, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[212], { offset + 5, offset + 0 }, { offset + 6, offset + 1 }, buttonGoodReleasedColor );

        // O with double acute.
        released[213].resize( released[79].width(), released[79].height() + 3 );
        released[213].reset();
        fheroes2::Copy( released[79], 0, 0, released[213], 0, 3, released[79].width(), released[79].height() );
        fheroes2::DrawLine( released[213], { offset + 3, offset + 1 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[213], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // O with diaeresis.
        released[214].resize( released[79].width(), released[79].height() + 3 );
        released[214].reset();
        fheroes2::Copy( released[79], 0, 0, released[214], 0, 3, released[79].width(), released[79].height() );
        fheroes2::DrawLine( released[214], { offset + 2, offset + 1 }, { offset + 3, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214], { offset + 6, offset + 1 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // R with caron.
        released[216].resize( released[82].width(), released[82].height() + 3 );
        released[216].reset();
        fheroes2::Copy( released[82], 0, 0, released[216], 0, 3, released[82].width(), released[82].height() );
        fheroes2::SetPixel( released[216], offset + 5, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // U with ring above.
        released[217].resize( released[85].width(), released[85].height() + 4 );
        released[217].reset();
        fheroes2::Copy( released[85], 0, 0, released[217], 0, 4, released[85].width(), released[85].height() );
        fheroes2::DrawLine( released[217], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[217], { offset + 6, offset + 2 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // U with acute.
        released[218].resize( released[85].width(), released[85].height() + 3 );
        released[218].reset();
        fheroes2::Copy( released[85], 0, 0, released[218], 0, 3, released[85].width(), released[85].height() );
        fheroes2::DrawLine( released[218], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // U with double acute.
        released[219].resize( released[85].width(), released[85].height() + 3 );
        released[219].reset();
        fheroes2::Copy( released[85], 0, 0, released[219], 0, 3, released[85].width(), released[85].height() );
        fheroes2::DrawLine( released[219], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[219], { offset + 8, offset + 1 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );

        // U with diaeresis.
        released[220].resize( released[85].width(), released[85].height() + 3 );
        released[220].reset();
        fheroes2::Copy( released[85], 0, 0, released[220], 0, 3, released[85].width(), released[85].height() );
        fheroes2::DrawLine( released[220], { offset + 4, offset + 1 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[220], { offset + 8, offset + 1 }, { offset + 9, offset + 1 }, buttonGoodReleasedColor );

        // Y with acute.
        released[221].resize( released[89].width(), released[89].height() + 3 );
        released[221].reset();
        fheroes2::Copy( released[89], 0, 0, released[221], 0, 3, released[89].width(), released[89].height() );
        fheroes2::DrawLine( released[221], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // T with cedilla. Only copied from T.
        released[222].resize( released[84].width(), released[84].height() + 3 );
        released[222].reset();
        fheroes2::Copy( released[84], 0, 0, released[222], 0, 0, released[84].width(), released[84].height() );
        fheroes2::DrawLine( released[222], { offset + 5, offset + 10 }, { offset + 6, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[222], { offset + 4, offset + 12 }, { offset + 5, offset + 12 }, buttonGoodReleasedColor );
    }

    void generateCP1251GoodButtonFont( std::vector<fheroes2::Sprite> & released )
    {
        // Increase size to fit full CP1252 set of characters. Fill with 1px transparent images.
        const fheroes2::Sprite firstSprite{ released[0] };
        released.insert( released.end(), codePageExtraCharacterCount, firstSprite );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // Offset symbols that either have diacritics or need less space to neighboring symbols.
        released[141].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        released[161].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        released[165].setPosition( buttonFontOffset.x, buttonFontOffset.y - 2 );
        released[168].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        released[175].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        released[192].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[201].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        released[218].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );

        // K with acute, Cyrillic KJE. Needs to have upper right arm adjusted.
        released[141].resize( released[75].width(), released[75].height() + 4 );
        released[141].reset();
        fheroes2::Copy( released[75], offset + 2, offset + 1, released[141], offset + 1, offset + 4, 7, released[75].height() - offset * 2 - 2 );
        fheroes2::DrawLine( released[141], { offset + 0, offset + 3 }, { offset + 2, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[141], { offset + 0, offset + 12 }, { offset + 2, offset + 12 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[141], { offset + 6, offset + 3 }, { offset + 8, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[141], { offset + 6, offset + 12 }, { offset + 8, offset + 12 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[141], offset + 7, offset + 11, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[141], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // ' (right single quotation mark)
        released[146].resize( 3 + offset * 2, 4 + offset * 2 );
        released[146].reset();
        fheroes2::DrawLine( released[146], { offset + 1, offset + 0 }, { offset + 1, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[146], { offset + 2, offset + 0 }, { offset + 2, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[146], offset + 0, offset + 3, buttonGoodReleasedColor );

        // NBSP character.
        released[160].resize( bigFontSpaceWidth, 1 );
        released[160].reset();

        // y with breve.
        released[161].resize( 9 + offset * 2, 13 + offset * 2 );
        released[161].reset();
        fheroes2::DrawLine( released[161], { offset + 0, offset + 3 }, { offset + 2, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 6, offset + 3 }, { offset + 8, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 3, offset + 8 }, { offset + 1, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 5, offset + 8 }, { offset + 7, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 4, offset + 8 }, { offset + 3, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 0, offset + 12 }, { offset + 2, offset + 12 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[161], offset + 0, offset + 11, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 3, offset + 1 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[161], offset + 2, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[161], offset + 6, offset + 0, buttonGoodReleasedColor );

        // J
        released[163] = released[74];

        // GHE with upturn.
        released[165].resize( 8 + offset * 2, 12 + offset * 2 );
        released[165].reset();
        fheroes2::DrawLine( released[165], { offset + 0, offset + 2 }, { offset + 7, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[165], { offset + 7, offset + 0 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[165], { offset + 0, offset + 11 }, { offset + 2, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[165], { offset + 1, offset + 3 }, { offset + 1, offset + 10 }, buttonGoodReleasedColor );

        // E with two dots above.
        released[168].resize( released[69].width() - 1, released[69].height() + 3 );
        released[168].reset();
        fheroes2::Copy( released[69], offset + 1, offset + 0, released[168], offset + 0, offset + 3, released[69].width() - 1 - offset * 2,
                        released[69].height() - offset * 2 );
        fheroes2::SetPixel( released[168], offset + 3, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[168], offset + 6, offset + 1, buttonGoodReleasedColor );

        // Ukrainian IE (index 138) is made after the letter with index 189.

        // I with two dots above, Cyrillic YI
        released[175].resize( released[73].width(), released[73].height() + 3 );
        released[175].reset();
        fheroes2::Copy( released[73], 0, 0, released[175], 0, 3, released[73].width(), released[73].height() );
        fheroes2::SetPixel( released[175], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[175], offset + 4, offset + 1, buttonGoodReleasedColor );

        // I, Belarusian-Ukrainian I
        released[178] = released[73];

        // S
        released[189].resize( released[83].width() - 1, released[83].height() );
        fheroes2::Copy( released[83], 0, 0, released[189], 0, 0, 8, released[83].height() );
        fheroes2::Copy( released[83], 9, 0, released[189], 8, 0, released[83].width() - 9, released[83].height() );

        // A
        released[192].resize( 10 + offset * 2, 10 + offset * 2 );
        released[192].reset();
        fheroes2::DrawLine( released[192], { offset + 1, offset + 9 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[192], { offset + 5, offset + 0 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[192], { offset + 3, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[192], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[192], { offset + 7, offset + 9 }, { offset + 9, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[192], { offset + 2, offset + 6 }, { offset + 7, offset + 6 }, buttonGoodReleasedColor );

        // 6, Cyrillic BE
        released[193].resize( 8 + offset * 2, 10 + offset * 2 );
        released[193].reset();
        fheroes2::DrawLine( released[193], { offset + 0, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[193], { offset + 0, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[193], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[193], { offset + 2, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[193], { offset + 7, offset + 5 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[193], offset + 7, offset + 1, buttonGoodReleasedColor );

        // B
        released[194].resize( 8 + offset * 2, 10 + offset * 2 );
        released[194].reset();
        fheroes2::DrawLine( released[194], { offset + 0, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[194], { offset + 0, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[194], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[194], { offset + 2, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[194], { offset + 6, offset + 1 }, { offset + 6, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[194], { offset + 7, offset + 5 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );

        // r, Cyrillic GHE
        released[195].resize( 8 + offset * 2, 10 + offset * 2 );
        released[195].reset();
        fheroes2::DrawLine( released[195], { offset + 0, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[195], { offset + 7, offset + 1 }, { offset + 7, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[195], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[195], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );

        // Cyrillic DE
        released[196].resize( 10 + offset * 2, 13 + offset * 2 );
        released[196].reset();
        fheroes2::DrawLine( released[196], { offset + 2, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[196], { offset + 8, offset + 1 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[196], { offset + 3, offset + 1 }, { offset + 3, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[196], { offset + 0, offset + 9 }, { offset + 9, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[196], { offset + 0, offset + 10 }, { offset + 0, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[196], { offset + 9, offset + 10 }, { offset + 9, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[196], offset + 2, offset + 8, buttonGoodReleasedColor );

        // E
        released[197].resize( released[69].width() - 1, released[69].height() );
        fheroes2::Copy( released[69], 1, 0, released[197], 0, 0, released[69].width() - 1, released[69].height() );
        fheroes2::SetTransformPixel( released[197], 1, offset + 0, 1 );
        fheroes2::SetTransformPixel( released[197], 1, offset + 9, 1 );

        // X with vertical stroke through it, Cyrillic ZHE. Needs to have upper right and left arms adjusted.
        released[198].resize( released[88].width() - 1, released[88].height() );
        released[198].reset();
        fheroes2::Copy( released[88], offset + 1, offset + 0, released[198], offset + 0, offset + 0, 5, released[88].height() - offset * 2 );
        fheroes2::Copy( released[88], offset + 6, offset + 0, released[198], offset + 6, offset + 0, 5, released[88].height() - offset * 2 );
        fheroes2::DrawLine( released[198], { offset + 5, offset + 1 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198], { offset + 4, offset + 0 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198], { offset + 4, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );

        // 3, Cyrillic ZE
        released[199].resize( 7 + offset * 2, 10 + offset * 2 );
        released[199].reset();
        fheroes2::DrawLine( released[199], { offset + 2, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[199], { offset + 0, offset + 0 }, { offset + 0, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[199], { offset + 6, offset + 1 }, { offset + 6, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[199], { offset + 2, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[199], { offset + 6, offset + 5 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[199], { offset + 1, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[199], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[199], offset + 0, offset + 8, buttonGoodReleasedColor );

        // Mirrored N, Cyrillic I
        released[200].resize( 9 + offset * 2, 10 + offset * 2 );
        released[200].reset();
        fheroes2::DrawLine( released[200], { offset + 1, offset + 1 }, { offset + 1, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[200], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[200], { offset + 2, offset + 8 }, { offset + 6, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[200], { offset + 7, offset + 0 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[200], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[200], offset + 0, offset + 9, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[200], offset + 8, offset + 0, buttonGoodReleasedColor );

        // Mirrored N with breve, Cyrillic Short I
        released[201].resize( released[200].width(), released[200].height() + 3 );
        released[201].reset();
        fheroes2::Copy( released[200], 0, 0, released[201], 0, 3, released[200].width(), released[200].height() );
        fheroes2::DrawLine( released[201], { offset + 3, offset + 1 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[201], offset + 2, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[201], offset + 6, offset + 0, buttonGoodReleasedColor );

        // K.
        released[202].resize( released[75].width() - 3, released[75].height() );
        released[202].reset();
        fheroes2::Copy( released[75], offset + 2, offset + 1, released[202], offset + 1, offset + 1, 7, released[75].height() - offset * 2 - 2 );
        fheroes2::DrawLine( released[202], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[202], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[202], { offset + 6, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[202], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[202], offset + 7, offset + 8, buttonGoodReleasedColor );

        // /\, Cyrillic EL
        released[203].resize( 10 + offset * 2, 10 + offset * 2 );
        released[203].reset();
        fheroes2::DrawLine( released[203], { offset + 1, offset + 9 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[203], { offset + 5, offset + 0 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[203], { offset + 3, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[203], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[203], { offset + 7, offset + 9 }, { offset + 9, offset + 9 }, buttonGoodReleasedColor );

        // M
        released[204].resize( 9 + offset * 2, 10 + offset * 2 );
        released[204].reset();
        fheroes2::DrawLine( released[204], { offset + 1, offset + 0 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[204], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[204], { offset + 2, offset + 1 }, { offset + 4, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[204], { offset + 5, offset + 4 }, { offset + 6, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[204], { offset + 7, offset + 0 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[204], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[204], offset + 0, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[204], offset + 8, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[204], offset + 4, offset + 4, buttonGoodReleasedColor );

        // H
        released[205].resize( 9 + offset * 2, 10 + offset * 2 );
        released[205].reset();
        fheroes2::DrawLine( released[205], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[205], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[205], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[205], { offset + 2, offset + 5 }, { offset + 7, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[205], { offset + 7, offset + 1 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[205], { offset + 6, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[205], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );

        // O
        released[206].resize( released[79].width() - 2, released[79].height() );
        fheroes2::Copy( released[79], 0, 0, released[206], 0, 0, 7, released[79].height() );
        fheroes2::Copy( released[79], 9, 0, released[206], 7, 0, released[79].width() - 9, released[79].height() );

        // Cyrillic PE
        released[207].resize( 9 + offset * 2, 10 + offset * 2 );
        released[207].reset();
        fheroes2::DrawLine( released[207], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[207], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[207], { offset + 7, offset + 1 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[207], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[207], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );

        // P
        released[208].resize( 8 + offset * 2, 10 + offset * 2 );
        released[208].reset();
        fheroes2::DrawLine( released[208], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[208], { offset + 0, offset + 0 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[208], { offset + 2, offset + 5 }, { offset + 6, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[208], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[208], { offset + 7, offset + 1 }, { offset + 7, offset + 4 }, buttonGoodReleasedColor );

        // C
        released[209].resize( released[67].width() - 2, released[67].height() );
        fheroes2::Copy( released[67], 0, 0, released[209], 0, 0, 7, released[67].height() );
        fheroes2::Copy( released[67], 9, 0, released[209], 7, 0, released[67].width() - 9, released[79].height() );

        // T
        released[210].resize( released[84].width() - 2, released[84].height() );
        fheroes2::Copy( released[84], 0, 0, released[210], 0, 0, 5, released[84].height() );
        fheroes2::Copy( released[84], 6, 0, released[210], 5, 0, 3, released[84].height() );
        fheroes2::Copy( released[84], 10, 0, released[210], 8, 0, 5, released[84].height() );

        // y, Cyrillic U
        released[211].resize( 9 + offset * 2, 10 + offset * 2 );
        released[211].reset();
        fheroes2::DrawLine( released[211], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[211], { offset + 6, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[211], { offset + 3, offset + 5 }, { offset + 1, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[211], { offset + 5, offset + 5 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[211], { offset + 4, offset + 5 }, { offset + 3, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[211], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[211], offset + 0, offset + 8, buttonGoodReleasedColor );

        // O with vertical bar, Cyrillic EF
        released[212].resize( 10 + offset * 2, 10 + offset * 2 );
        released[212].reset();
        fheroes2::DrawLine( released[212], { offset + 1, offset + 2 }, { offset + 7, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[212], { offset + 0, offset + 3 }, { offset + 0, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[212], { offset + 1, offset + 7 }, { offset + 7, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[212], { offset + 8, offset + 3 }, { offset + 8, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[212], { offset + 4, offset + 1 }, { offset + 4, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[212], { offset + 3, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[212], { offset + 3, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );

        // X
        released[213].resize( released[88].width() - 3, released[88].height() );
        released[213].reset();
        fheroes2::Copy( released[88], offset + 1, offset + 0, released[213], offset + 0, offset + 0, 5, released[88].height() - offset * 2 );
        fheroes2::Copy( released[88], offset + 7, offset + 0, released[213], offset + 5, offset + 0, 4, released[88].height() - offset * 2 );

        // Cyrillic TSE
        released[214].resize( 9 + offset * 2, 12 + offset * 2 );
        released[214].reset();
        fheroes2::DrawLine( released[214], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214], { offset + 7, offset + 1 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214], { offset + 6, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214], { offset + 8, offset + 10 }, { offset + 8, offset + 11 }, buttonGoodReleasedColor );

        // Cyrillic CHE
        released[215].resize( 9 + offset * 2, 10 + offset * 2 );
        released[215].reset();
        fheroes2::DrawLine( released[215], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[215], { offset + 6, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[215], { offset + 1, offset + 1 }, { offset + 1, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[215], { offset + 7, offset + 1 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[215], { offset + 2, offset + 5 }, { offset + 6, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[215], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );

        // Cyrillic SHA
        released[216].resize( 11 + offset * 2, 10 + offset * 2 );
        released[216].reset();
        fheroes2::DrawLine( released[216], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 5, offset + 1 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 9, offset + 1 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 0, offset + 9 }, { offset + 10, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 4, offset + 0 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 8, offset + 0 }, { offset + 10, offset + 0 }, buttonGoodReleasedColor );

        // Cyrillic SHCHA
        released[217].resize( 11 + offset * 2, 12 + offset * 2 );
        released[217].reset();
        fheroes2::Copy( released[216], 0, 0, released[217], 0, 0, released[216].width(), released[216].height() );
        fheroes2::DrawLine( released[217], { offset + 10, offset + 10 }, { offset + 10, offset + 11 }, buttonGoodReleasedColor );

        // b, Cyrillic hard sign
        released[218].resize( 10 + offset * 2, 10 + offset * 2 );
        released[218].reset();
        fheroes2::DrawLine( released[218], { offset + 0, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[218], { offset + 2, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[218], { offset + 3, offset + 1 }, { offset + 3, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[218], { offset + 4, offset + 4 }, { offset + 8, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[218], { offset + 9, offset + 5 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );

        // bI, Cyrillic YERU
        released[219].resize( 10 + offset * 2, 10 + offset * 2 );
        released[219].reset();
        fheroes2::DrawLine( released[219], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[219], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[219], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[219], { offset + 2, offset + 4 }, { offset + 4, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[219], { offset + 5, offset + 5 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::Copy( released[73], offset + 1, 0, released[219], offset + 7, 0, released[73].width() - 2 - offset * 2, released[73].height() );

        // b, Cyrillic soft sign
        released[220].resize( 8 + offset * 2, 10 + offset * 2 );
        released[220].reset();
        fheroes2::DrawLine( released[220], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[220], { offset + 0, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[220], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[220], { offset + 2, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[220], { offset + 7, offset + 5 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );

        // Flipped C with line inside, Cyrillic E
        released[221].resize( 8 + offset * 2, 10 + offset * 2 );
        released[221].reset();
        fheroes2::DrawLine( released[221], { offset + 2, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[221], { offset + 7, offset + 2 }, { offset + 7, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[221], { offset + 2, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[221], { offset + 0, offset + 0 }, { offset + 0, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[221], { offset + 2, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[221], offset + 0, offset + 7, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[221], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[221], offset + 1, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[221], offset + 6, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[221], offset + 6, offset + 8, buttonGoodReleasedColor );

        // Ukrainian IE. Make it by mirroring horizontally the previous letter.
        released[170].resize( 8 + offset * 2, 10 + offset * 2 );
        released[170].reset();
        fheroes2::Blit( released[221], released[170], true );

        // IO, Cyrillic YU
        released[222].resize( 11 + offset * 2, 10 + offset * 2 );
        released[222].reset();
        fheroes2::Copy( released[73], offset + 1, 0, released[222], offset + 0, 0, released[73].width() - 2 - offset * 2, released[73].height() );
        fheroes2::Copy( released[79], offset, 0, released[222], offset + 4, 0, 3, released[79].height() );
        fheroes2::Copy( released[79], offset + 6, 0, released[222], offset + 7, 0, 4, released[79].height() );
        fheroes2::DrawLine( released[222], { offset + 2, offset + 4 }, { offset + 3, offset + 4 }, buttonGoodReleasedColor );

        // Mirrored R, Cyrillic YA
        released[223].resize( released[208].width(), released[208].height() );
        fheroes2::Flip( released[208], 0, 0, released[223], 0, 0, released[208].width(), released[208].height(), true, false );
        fheroes2::DrawLine( released[223], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[223], { offset + 3, offset + 6 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );

        // Cyrillic Capital Lje
        released[138].resize( released[203].width() + released[220].width() - 8, released[203].height() );
        released[138].reset();
        fheroes2::Copy( released[203], 0, 0, released[138], 0, 0, released[203].width(), released[203].height() );
        fheroes2::Copy( released[220], 2, 0, released[138], released[203].width() - 6, 0, released[220].width() - 2, released[220].height() );
        fheroes2::FillTransform( released[138], 7, 3, 1, 1, 1 );
        released[138].setPosition( released[203].x(), released[203].y() );

        // Cyrillic Capital Nje
        released[140].resize( released[205].width() + released[220].width() - 7, released[205].height() );
        released[140].reset();
        fheroes2::Copy( released[205], 0, 0, released[140], 0, 0, released[205].width(), released[205].height() );
        fheroes2::Copy( released[220], 3, 0, released[140], released[205].width() - 4, 0, released[220].width() - 4, released[220].height() );
        released[140].setPosition( released[205].x(), released[205].y() );
    }

    void generateCP1252GoodButtonFont( std::vector<fheroes2::Sprite> & released )
    {
        // Increase size to fit full CP1252 set of characters. Fill with 1px transparent images.
        const fheroes2::Sprite firstSprite{ released[0] };
        released.insert( released.end(), codePageExtraCharacterCount, firstSprite );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // Offset letters with diacritics above them.
        for ( const int charCode : { 192, 193, 194, 195, 200, 201, 202, 205, 206, 209, 211, 212, 213, 217, 218, 219 } ) {
            released[charCode].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        }

        for ( const int charCode : { 196, 197, 203, 207, 214, 220 } ) {
            released[charCode].setPosition( buttonFontOffset.x, buttonFontOffset.y - 2 );
        }

        released[216].setPosition( buttonFontOffset.x, buttonFontOffset.y - 1 );

        // Offset A-related letters to have less space to neighboring letters. Keep the y-offset change from earlier.
        for ( const int charCode : { 65, 192, 193, 194, 195, 196, 197, 198 } ) {
            released[charCode].setPosition( buttonFontOffset.x - 2, released[charCode].y() );
        }

        // NBSP character.
        released[160].resize( bigFontSpaceWidth, 1 );
        released[160].reset();

        // A with grave.
        released[192].resize( released[65].width(), released[65].height() + 3 );
        released[192].reset();
        fheroes2::Copy( released[65], 0, 0, released[192], 0, 3, released[65].width(), released[65].height() );
        fheroes2::DrawLine( released[192], { offset + 6, offset + 0 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // A with acute.
        released[193].resize( released[65].width(), released[65].height() + 3 );
        released[193].reset();
        fheroes2::Copy( released[65], 0, 0, released[193], 0, 3, released[65].width(), released[65].height() );
        fheroes2::DrawLine( released[193], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // A with circumflex
        released[194].resize( released[65].width(), released[65].height() + 3 );
        released[194].reset();
        fheroes2::Copy( released[65], 0, 0, released[194], 0, 3, released[65].width(), released[65].height() );
        fheroes2::DrawLine( released[194], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[194], offset + 7, offset + 1, buttonGoodReleasedColor );

        // A with tilde
        released[195].resize( released[65].width(), released[65].height() + 3 );
        released[195].reset();
        fheroes2::Copy( released[65], 0, 0, released[195], 0, 3, released[65].width(), released[65].height() );
        fheroes2::DrawLine( released[195], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[195], offset + 6, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[195], offset + 7, offset + 1, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[195], { offset + 8, offset + 1 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );

        // A with diaeresis
        released[196].resize( released[65].width(), released[65].height() + 2 );
        released[196].reset();
        fheroes2::Copy( released[65], 0, 0, released[196], 0, 2, released[65].width(), released[65].height() );
        fheroes2::SetPixel( released[196], offset + 5, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[196], offset + 7, offset + 0, buttonGoodReleasedColor );

        // A with circle on top
        released[197].resize( 13 + offset * 2, 12 + offset * 2 );
        released[197].reset();
        fheroes2::DrawLine( released[197], { offset + 0, offset + 11 }, { offset + 4, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197], { offset + 8, offset + 11 }, { offset + 12, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197], { offset + 5, offset + 7 }, { offset + 8, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197], { offset + 2, offset + 10 }, { offset + 4, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197], { offset + 7, offset + 3 }, { offset + 10, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197], { offset + 5, offset + 1 }, { offset + 5, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197], { offset + 8, offset + 1 }, { offset + 8, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197], { offset + 6, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[197], offset + 4, offset + 6, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[197], offset + 5, offset + 5, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[197], offset + 5, offset + 4, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[197], offset + 6, offset + 3, buttonGoodReleasedColor );

        // A attached to E.
        released[198].resize( 15 + offset * 2, 10 + offset * 2 );
        released[198].reset();
        fheroes2::DrawLine( released[198], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198], { offset + 8, offset + 9 }, { offset + 12, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198], { offset + 5, offset + 5 }, { offset + 8, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198], { offset + 2, offset + 8 }, { offset + 4, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198], { offset + 7, offset + 1 }, { offset + 10, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198], { offset + 7, offset + 0 }, { offset + 14, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198], { offset + 13, offset + 9 }, { offset + 14, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198], { offset + 9, offset + 4 }, { offset + 12, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198], offset + 4, offset + 4, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198], offset + 5, offset + 3, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198], offset + 5, offset + 2, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198], offset + 6, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198], offset + 6, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198], offset + 14, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198], offset + 14, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198], offset + 12, offset + 3, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198], offset + 12, offset + 5, buttonGoodReleasedColor );

        // C with cedilla.
        released[199].resize( released[67].width(), released[67].height() + 3 );
        released[199].reset();
        fheroes2::Copy( released[67], 0, 0, released[199], 0, 0, released[67].width(), released[67].height() );
        fheroes2::DrawLine( released[199], { offset + 5, offset + 10 }, { offset + 6, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[199], { offset + 4, offset + 12 }, { offset + 5, offset + 12 }, buttonGoodReleasedColor );

        // E with grave.
        released[200].resize( released[69].width(), released[69].height() + 3 );
        released[200].reset();
        fheroes2::Copy( released[69], 0, 0, released[200], 0, 3, released[69].width(), released[69].height() );
        fheroes2::DrawLine( released[200], { offset + 4, offset + 0 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );

        // E with acute.
        released[201].resize( released[69].width(), released[69].height() + 3 );
        released[201].reset();
        fheroes2::Copy( released[69], 0, 0, released[201], 0, 3, released[69].width(), released[69].height() );
        fheroes2::DrawLine( released[201], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // E with circumflex.
        released[202].resize( released[69].width(), released[69].height() + 3 );
        released[202].reset();
        fheroes2::Copy( released[69], 0, 0, released[202], 0, 3, released[69].width(), released[69].height() );
        fheroes2::Copy( released[194], offset + 5, offset, released[202], offset + 3, offset, 3, 2 );

        // E with diaeresis.
        released[203].resize( released[69].width(), released[69].height() + 2 );
        released[203].reset();
        fheroes2::Copy( released[69], 0, 0, released[203], 0, 2, released[69].width(), released[69].height() );
        fheroes2::SetPixel( released[203], offset + 3, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[203], offset + 6, offset + 0, buttonGoodReleasedColor );

        // I with grave.
        released[204].resize( released[73].width(), released[73].height() + 3 );
        released[204].reset();
        fheroes2::Copy( released[73], 0, 0, released[204], 0, 3, released[73].width(), released[73].height() );
        fheroes2::DrawLine( released[204], { offset + 2, offset + 0 }, { offset + 3, offset + 1 }, buttonGoodReleasedColor );

        // I with acute.
        released[205].resize( released[73].width(), released[73].height() + 3 );
        released[205].reset();
        fheroes2::Copy( released[73], 0, 0, released[205], 0, 3, released[73].width(), released[73].height() );
        fheroes2::DrawLine( released[205], { offset + 2, offset + 1 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );

        // I with circumflex.
        released[206].resize( released[73].width(), released[73].height() + 3 );
        released[206].reset();
        fheroes2::Copy( released[73], 0, 0, released[206], 0, 3, released[73].width(), released[73].height() );
        fheroes2::Copy( released[194], offset + 5, offset, released[206], offset + 1, offset, 3, 2 );

        // I with diaeresis.
        released[207].resize( released[73].width(), released[73].height() + 2 );
        released[207].reset();
        fheroes2::Copy( released[73], 0, 0, released[207], 0, 2, released[73].width(), released[73].height() );
        fheroes2::SetPixel( released[207], offset + 0, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[207], offset + 4, offset + 0, buttonGoodReleasedColor );

        // N with tilde.
        released[209].resize( released[78].width(), released[78].height() + 3 );
        released[209].reset();
        fheroes2::Copy( released[78], 0, 0, released[209], 0, 3, released[78].width(), released[78].height() );
        fheroes2::Copy( released[195], offset + 4, offset, released[209], offset + 4, offset, 6, 2 );

        // O with acute.
        released[211].resize( released[79].width(), released[79].height() + 3 );
        released[211].reset();
        fheroes2::Copy( released[79], 0, 0, released[211], 0, 3, released[79].width(), released[79].height() );
        fheroes2::DrawLine( released[211], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // O with circumflex.
        released[212].resize( released[79].width(), released[79].height() + 3 );
        released[212].reset();
        fheroes2::Copy( released[79], 0, 0, released[212], 0, 3, released[79].width(), released[79].height() );
        fheroes2::DrawLine( released[212], { offset + 3, offset + 1 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[212], { offset + 5, offset + 0 }, { offset + 6, offset + 1 }, buttonGoodReleasedColor );

        // O with tilde.
        released[213].resize( released[79].width(), released[79].height() + 3 );
        released[213].reset();
        fheroes2::Copy( released[79], 0, 0, released[213], 0, 3, released[79].width(), released[79].height() );
        fheroes2::Copy( released[195], offset + 4, offset, released[213], offset + 2, offset, 6, 2 );

        // O with diaeresis.
        released[214].resize( released[79].width(), released[79].height() + 2 );
        released[214].reset();
        fheroes2::Copy( released[79], 0, 0, released[214], 0, 2, released[79].width(), released[79].height() );
        fheroes2::SetPixel( released[214], offset + 3, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[214], offset + 6, offset + 0, buttonGoodReleasedColor );

        // O with slash.
        released[216].resize( released[79].width() + 2, released[79].height() + 2 );
        released[216].reset();
        fheroes2::Copy( released[79], 0, 0, released[216], 1, 1, released[79].width(), released[79].height() );
        fheroes2::DrawLine( released[216], { released[79].width() - 1, 2 }, { 2, released[79].height() - 1 }, buttonGoodReleasedColor );

        // U with grave.
        released[217].resize( released[85].width(), released[85].height() + 3 );
        released[217].reset();
        fheroes2::Copy( released[85], 0, 0, released[217], 0, 3, released[85].width(), released[85].height() );
        fheroes2::DrawLine( released[217], { offset + 6, offset + 0 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // U with acute.
        released[218].resize( released[85].width(), released[85].height() + 3 );
        released[218].reset();
        fheroes2::Copy( released[85], 0, 0, released[218], 0, 3, released[85].width(), released[85].height() );
        fheroes2::DrawLine( released[218], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // U with circumflex.
        released[219].resize( released[85].width(), released[85].height() + 3 );
        released[219].reset();
        fheroes2::Copy( released[85], 0, 0, released[219], 0, 3, released[85].width(), released[85].height() );
        fheroes2::DrawLine( released[219], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[219], offset + 6, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[219], { offset + 7, offset + 0 }, { offset + 8, offset + 1 }, buttonGoodReleasedColor );

        // U with diaeresis.
        released[220].resize( released[85].width(), released[85].height() + 2 );
        released[220].reset();
        fheroes2::Copy( released[85], 0, 0, released[220], 0, 2, released[85].width(), released[85].height() );
        fheroes2::SetPixel( released[220], offset + 4, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[220], offset + 8, offset + 0, buttonGoodReleasedColor );

        // Eszett.
        released[223].resize( 12 + offset * 2, 10 + offset * 2 );
        released[223].reset();
        fheroes2::DrawLine( released[223], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[223], { offset + 2, offset + 9 }, { offset + 2, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[223], offset + 3, offset + 1, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[223], { offset + 4, offset + 0 }, { offset + 10, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[223], { offset + 11, offset + 1 }, { offset + 11, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[223], offset + 10, offset + 3, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[223], { offset + 9, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[223], offset + 10, offset + 5, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[223], { offset + 11, offset + 6 }, { offset + 11, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[223], { offset + 10, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[223], offset + 5, offset + 8, buttonGoodReleasedColor );
    }

    void generateCP1253GoodButtonFont( std::vector<fheroes2::Sprite> & released )
    {
        // Increase size to fit full CP1254 set of characters. Fill with 1px transparent images.
        const fheroes2::Sprite firstSprite{ released[0] };
        released.insert( released.end(), codePageExtraCharacterCount, firstSprite );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // NBSP character.
        released[160].resize( bigFontSpaceWidth, 1 );
        released[160].reset();

        // Greek capital letter alpha
        released[193] = released[65];

        // Greek capital letter beta
        released[194] = released[66];

        // Greek capital letter gamma
        released[195].resize( 8 + offset * 2, 10 + offset * 2 );
        released[195].reset();
        fheroes2::DrawLine( released[195], { offset + 0, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[195], { offset + 7, offset + 1 }, { offset + 7, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[195], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[195], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );

        // Greek capital letter delta
        released[196].resize( released[65].width() - 2, released[65].height() );
        released[196].reset();
        fheroes2::Copy( released[65], 3, 2, released[196], 2, 2, 11, 10 );
        fheroes2::FillTransform( released[196], 6, 7, 4, 1, 1 );
        fheroes2::DrawLine( released[196], { offset + 4, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );

        // Greek capital letter epsilon
        released[197] = released[69];

        // Greek capital letter zeta
        released[198] = released[90];

        // Greek capital letter eta
        released[199] = released[72];

        // Greek capital letter theta
        released[200] = released[79];
        fheroes2::DrawLine( released[200], { offset + 3, offset + 5 }, { offset + 6, offset + 5 }, buttonGoodReleasedColor );

        // Greek capital letter iota
        released[201] = released[73];

        // Greek capital letter kappa
        released[202] = released[75];

        // Greek capital letter lambda
        released[203].resize( released[65].width() - 2, released[65].height() );
        released[203].reset();
        fheroes2::Copy( released[65], 3, 2, released[203], 2, 2, 11, 10 );
        fheroes2::FillTransform( released[203], 6, 7, 4, 1, 1 );
        fheroes2::FillTransform( released[203], 5, 11, 1, 1, 1 );
        fheroes2::FillTransform( released[203], 9, 11, 1, 1, 1 );

        // Greek capital letter mu
        released[204] = released[77];

        // Greek capital letter nu
        released[205] = released[78];

        // Greek capital letter xi
        released[206].resize( released[69].width(), released[69].height() );
        released[206].reset();
        fheroes2::DrawLine( released[206], { offset + 0, offset + 0 }, { offset + 0, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[206], { offset + 1, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[206], { offset + 8, offset + 0 }, { offset + 8, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[206], { offset + 0, offset + 8 }, { offset + 0, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[206], { offset + 1, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[206], { offset + 8, offset + 8 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[206], { offset + 2, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );

        // Greek capital letter omicron
        released[207] = released[79];

        // Greek capital letter pi
        released[208].resize( 9 + offset * 2, 10 + offset * 2 );
        released[208].reset();
        fheroes2::DrawLine( released[208], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[208], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[208], { offset + 7, offset + 1 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[208], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[208], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );

        // Greek capital letter rho
        released[209] = released[80];

        // Greek capital letter sigma
        released[211].resize( released[69].width(), released[69].height() );
        released[211].reset();
        fheroes2::DrawLine( released[211], { offset + 0, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[211], { offset + 8, offset + 0 }, { offset + 8, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[211], { offset + 0, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[211], { offset + 8, offset + 8 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[211], { offset + 0, offset + 1 }, { offset + 4, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[211], { offset + 0, offset + 8 }, { offset + 4, offset + 5 }, buttonGoodReleasedColor );

        // Greek capital letter tau
        released[212] = released[84];

        // Greek capital letter upsilon
        released[213] = released[89];

        // Greek capital letter phi
        released[214].resize( 9 + offset * 2, 10 + offset * 2 );
        released[214].reset();
        fheroes2::DrawLine( released[214], { offset + 1, offset + 2 }, { offset + 7, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214], { offset + 0, offset + 3 }, { offset + 0, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214], { offset + 1, offset + 7 }, { offset + 7, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214], { offset + 8, offset + 3 }, { offset + 8, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214], { offset + 4, offset + 1 }, { offset + 4, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214], { offset + 3, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214], { offset + 3, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );

        // Greek capital letter chi
        released[215] = released[88];

        // Greek capital letter psi
        released[216].resize( released[85].width(), released[85].height() );
        released[216].reset();
        fheroes2::DrawLine( released[216], { offset + 0, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 9, offset + 0 }, { offset + 12, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 2, offset + 1 }, { offset + 2, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 10, offset + 1 }, { offset + 10, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[216], offset + 3, offset + 6, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[216], offset + 9, offset + 6, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 4, offset + 7 }, { offset + 8, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 6, offset + 0 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216], { offset + 5, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );

        // Greek capital letter omega
        released[217].resize( released[48].width(), released[48].height() );
        released[217].reset();
        fheroes2::DrawLine( released[217], { offset + 2, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[217], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[217], offset + 7, offset + 1, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[217], { offset + 0, offset + 2 }, { offset + 0, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[217], { offset + 8, offset + 2 }, { offset + 8, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[217], offset + 1, offset + 7, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[217], offset + 2, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[217], offset + 7, offset + 7, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[217], offset + 6, offset + 8, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[217], { offset + 0, offset + 9 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[217], { offset + 5, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );

        // Greek capital letter iota with dialytika
        released[218].resize( released[73].width(), released[73].height() + 2 );
        released[218].reset();
        fheroes2::Copy( released[73], 0, 0, released[218], 0, 2, released[73].width(), released[73].height() );
        fheroes2::SetPixel( released[218], offset + 0, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[218], offset + 4, offset + 0, buttonGoodReleasedColor );

        // Greek small letter sigma
        released[242].resize( 6 + offset * 2, 9 + offset * 2 );
        released[242].reset();
        fheroes2::DrawLine( released[242], { offset + 1, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[242], { offset + 0, offset + 1 }, { offset + 0, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[242], { offset + 1, offset + 6 }, { offset + 3, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[242], { offset + 4, offset + 7 }, { offset + 3, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[242], offset + 2, offset + 8, buttonGoodReleasedColor );
    }

    void generateCP1254GoodButtonFont( std::vector<fheroes2::Sprite> & released )
    {
        // Increase size to fit full CP1254 set of characters. Fill with 1px transparent images.
        const fheroes2::Sprite firstSprite{ released[0] };
        released.insert( released.end(), codePageExtraCharacterCount, firstSprite );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // Offset letters with diacritics above them.
        for ( const int charCode : { 214, 220, 221 } ) {
            released[charCode].setPosition( buttonFontOffset.x, buttonFontOffset.y - 2 );
        }

        // NBSP character.
        released[160].resize( bigFontSpaceWidth, 1 );
        released[160].reset();

        // Offset G with breve.
        released[208].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );

        // C with cedilla.
        released[199].resize( released[67].width(), released[67].height() + 3 );
        released[199].reset();
        fheroes2::Copy( released[67], 0, 0, released[199], 0, 0, released[67].width(), released[67].height() );
        fheroes2::DrawLine( released[199], { offset + 5, offset + 10 }, { offset + 6, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[199], { offset + 4, offset + 12 }, { offset + 5, offset + 12 }, buttonGoodReleasedColor );

        // G with breve.
        released[208].resize( released[71].width(), released[71].height() + 3 );
        released[208].reset();
        fheroes2::Copy( released[71], 0, 0, released[208], 0, 3, released[71].width(), released[71].height() );
        fheroes2::DrawLine( released[208], { offset + 3, offset + 0 }, { offset + 4, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[208], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // O with diaeresis.
        released[214].resize( released[79].width(), released[79].height() + 2 );
        released[214].reset();
        fheroes2::Copy( released[79], 0, 0, released[214], 0, 2, released[79].width(), released[79].height() );
        fheroes2::SetPixel( released[214], offset + 3, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[214], offset + 6, offset + 0, buttonGoodReleasedColor );

        // U with diaeresis.
        released[220].resize( released[85].width(), released[85].height() + 2 );
        released[220].reset();
        fheroes2::Copy( released[85], 0, 0, released[220], 0, 2, released[85].width(), released[85].height() );
        fheroes2::SetPixel( released[220], offset + 4, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[220], offset + 8, offset + 0, buttonGoodReleasedColor );

        // I with dot above.
        released[221].resize( released[73].width(), released[73].height() + 2 );
        released[221].reset();
        fheroes2::Copy( released[73], 0, 0, released[221], 0, 2, released[73].width(), released[73].height() );
        fheroes2::SetPixel( released[221], offset + 2, offset + 0, buttonGoodReleasedColor );

        // S with cedilla.
        released[222].resize( released[83].width(), released[83].height() + 3 );
        released[222].reset();
        fheroes2::Copy( released[83], 0, 0, released[222], 0, 0, released[83].width(), released[83].height() );
        fheroes2::DrawLine( released[222], { offset + 4, offset + 10 }, { offset + 5, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[222], { offset + 3, offset + 12 }, { offset + 4, offset + 12 }, buttonGoodReleasedColor );
    }
}

namespace fheroes2
{
    void generateAlphabet( const SupportedLanguage language, std::vector<std::vector<Sprite>> & icnVsSprite )
    {
        const CodePage codePage = getCodePage( language );

        switch ( codePage ) {
        case CodePage::CP1250:
            generateCP1250Alphabet( icnVsSprite );
            break;
        case CodePage::CP1251:
            generateCP1251Alphabet( icnVsSprite );
            break;
        case CodePage::CP1252:
            generateCP1252Alphabet( icnVsSprite );
            break;
        case CodePage::CP1253:
            generateCP1253Alphabet( icnVsSprite );
            break;
        case CodePage::CP1254:
            generateCP1254Alphabet( icnVsSprite );
            break;
        case CodePage::CP1258:
            generateCP1258Alphabet( icnVsSprite );
            break;
        case CodePage::ISO8859_16:
            generateISO8859_16Alphabet( icnVsSprite );
            break;
        case CodePage::ISO8859_3:
            generateISO8859_3Alphabet( icnVsSprite );
            break;
        default:
            // Add new code page generation code!
            assert( 0 );
            break;
        }

        icnVsSprite[ICN::YELLOW_FONT].clear();
        icnVsSprite[ICN::YELLOW_SMALLFONT].clear();
        icnVsSprite[ICN::GRAY_FONT].clear();
        icnVsSprite[ICN::GRAY_SMALL_FONT].clear();
        icnVsSprite[ICN::WHITE_LARGE_FONT].clear();
        icnVsSprite[ICN::GOLDEN_GRADIENT_FONT].clear();
        icnVsSprite[ICN::GOLDEN_GRADIENT_LARGE_FONT].clear();
        icnVsSprite[ICN::SILVER_GRADIENT_FONT].clear();
        icnVsSprite[ICN::SILVER_GRADIENT_LARGE_FONT].clear();
    }

    void generateButtonAlphabet( const SupportedLanguage language, std::vector<std::vector<Sprite>> & icnVsSprite )
    {
        generateGoodButtonFontBaseShape( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );

        const CodePage codePage = getCodePage( language );

        switch ( codePage ) {
        case CodePage::ASCII:
            // Do nothing since the ASCII is the base font.
            break;
        case CodePage::CP1250:
            generateCP1250GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case CodePage::CP1251:
            generateCP1251GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case CodePage::CP1252:
            generateCP1252GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case CodePage::CP1253:
            generateCP1253GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case CodePage::CP1254:
            generateCP1254GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case CodePage::CP1258:
            // generateCP1258GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case CodePage::ISO8859_16:
            // generateISO8859_16GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case CodePage::ISO8859_3:
            // generateISO8859_3GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        default:
            // Add new code page generation code!
            assert( 0 );
            break;
        }

        updateButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED], icnVsSprite[ICN::BUTTON_EVIL_FONT_RELEASED],
                          icnVsSprite[ICN::BUTTON_EVIL_FONT_PRESSED] );
    }

    void modifyBaseNormalFont( std::vector<Sprite> & icnVsSprite )
    {
        if ( icnVsSprite.size() < 96 ) {
            return;
        }

        // $ (dollar sign). Fix position.
        icnVsSprite[36 - 32].setPosition( 0, 2 );

        // Remove white line from % symbol
        FillTransform( icnVsSprite[37 - 32], 5, 0, 5, 1, 1 );
        FillTransform( icnVsSprite[37 - 32], 6, 2, 2, 1, 1 );
        updateNormalFontLetterShadow( icnVsSprite[37 - 32] );

        // & (ampersand). Fix position.
        icnVsSprite[38 - 32].setPosition( 0, 1 );

        // Move "-" further down
        icnVsSprite[45 - 32].setPosition( icnVsSprite[45 - 32].x(), icnVsSprite[45 - 32].y() + 1 );
        updateNormalFontLetterShadow( icnVsSprite[45 - 32] );

        // Add the '\' character.
        icnVsSprite[92 - 32].resize( 8, 14 );
        icnVsSprite[92 - 32].reset();
        Blit( icnVsSprite[47 - 32], 0, 0, icnVsSprite[92 - 32], 1, 0, 7, 12, true );
        icnVsSprite[92 - 32].setPosition( icnVsSprite[47 - 32].x(), icnVsSprite[47 - 32].y() );
        updateNormalFontLetterShadow( icnVsSprite[92 - 32] );

        // Proper lowercase k.
        FillTransform( icnVsSprite[107 - 32], 4, 1, 5, 8, 1 );
        Copy( icnVsSprite[75 - 32], 6, 5, icnVsSprite[107 - 32], 4, 7, 3, 1 );
        Copy( icnVsSprite[75 - 32], 6, 4, icnVsSprite[107 - 32], 4, 6, 4, 1 );
        Copy( icnVsSprite[75 - 32], 7, 4, icnVsSprite[107 - 32], 6, 5, 3, 1 );
        Copy( icnVsSprite[75 - 32], 7, 4, icnVsSprite[107 - 32], 7, 4, 2, 1 );
        Copy( icnVsSprite[75 - 32], 6, 6, icnVsSprite[107 - 32], 4, 8, 4, 1 );
        icnVsSprite[107 - 32].setPosition( icnVsSprite[107 - 32].x(), icnVsSprite[107 - 32].y() );
        updateNormalFontLetterShadow( icnVsSprite[107 - 32] );

        // Add the vertical bar '|' character. It is also used for the text input cursor.
        icnVsSprite[124 - 32].resize( 3, icnVsSprite[91 - 32].height() + 3 );
        Copy( icnVsSprite[91 - 32], 0, 0, icnVsSprite[124 - 32], 0, 0, 3, icnVsSprite[91 - 32].height() - 4 );
        Copy( icnVsSprite[91 - 32], 0, icnVsSprite[91 - 32].height() - 7, icnVsSprite[124 - 32], 0, icnVsSprite[91 - 32].height() - 4, 3, 7 );
        icnVsSprite[124 - 32].setPosition( icnVsSprite[91 - 32].x(), icnVsSprite[91 - 32].y() );

        // Restore the original ASCII characters replaced in French assets with CP1252 symbols.
        if ( getResourceLanguage() == SupportedLanguage::French ) {
            // $ (dollar sign).
            icnVsSprite[36 - 32].resize( 9, 12 );
            icnVsSprite[36 - 32].reset();
            SetPixel( icnVsSprite[36 - 32], 3, 0, 23 );
            SetPixel( icnVsSprite[36 - 32], 4, 0, 18 );
            SetPixel( icnVsSprite[36 - 32], 5, 0, 12 );
            SetPixel( icnVsSprite[36 - 32], 6, 0, 23 );
            SetPixel( icnVsSprite[36 - 32], 1, 1, 21 );
            DrawLine( icnVsSprite[36 - 32], { 2, 1 }, { 7, 1 }, 10 );
            SetPixel( icnVsSprite[36 - 32], 8, 1, 21 );
            DrawLine( icnVsSprite[36 - 32], { 1, 2 }, { 1, 3 }, 10 );
            DrawLine( icnVsSprite[36 - 32], { 2, 2 }, { 2, 3 }, 16 );
            SetPixel( icnVsSprite[36 - 32], 3, 2, 21 );
            DrawLine( icnVsSprite[36 - 32], { 4, 2 }, { 4, 3 }, 16 );
            DrawLine( icnVsSprite[36 - 32], { 5, 2 }, { 5, 7 }, 10 );
            SetPixel( icnVsSprite[36 - 32], 6, 2, 21 );
            SetPixel( icnVsSprite[36 - 32], 7, 2, 16 );
            SetPixel( icnVsSprite[36 - 32], 8, 2, 10 );
            SetPixel( icnVsSprite[36 - 32], 1, 4, 21 );
            DrawLine( icnVsSprite[36 - 32], { 2, 4 }, { 7, 4 }, 10 );
            SetPixel( icnVsSprite[36 - 32], 8, 4, 21 );
            SetPixel( icnVsSprite[36 - 32], 2, 5, 21 );
            SetPixel( icnVsSprite[36 - 32], 3, 5, 16 );
            DrawLine( icnVsSprite[36 - 32], { 4, 5 }, { 4, 7 }, 16 );
            SetPixel( icnVsSprite[36 - 32], 1, 7, 10 );
            SetPixel( icnVsSprite[36 - 32], 2, 7, 16 );
            SetPixel( icnVsSprite[36 - 32], 3, 7, 21 );
            Copy( icnVsSprite[36 - 32], 3, 5, icnVsSprite[36 - 32], 6, 5, 3, 3 );
            SetPixel( icnVsSprite[36 - 32], 1, 8, 23 );
            DrawLine( icnVsSprite[36 - 32], { 2, 8 }, { 7, 8 }, 12 );
            SetPixel( icnVsSprite[36 - 32], 8, 8, 23 );
            SetPixel( icnVsSprite[36 - 32], 3, 9, 25 );
            SetPixel( icnVsSprite[36 - 32], 4, 9, 20 );
            SetPixel( icnVsSprite[36 - 32], 5, 9, 14 );
            SetPixel( icnVsSprite[36 - 32], 6, 9, 25 );
            updateNormalFontLetterShadow( icnVsSprite[36 - 32] );

            // & (ampersand).
            icnVsSprite[38 - 32].resize( 11, 12 );
            icnVsSprite[38 - 32].reset();
            SetPixel( icnVsSprite[38 - 32], 1, 0, 25 );
            SetPixel( icnVsSprite[38 - 32], 2, 0, 18 );
            DrawLine( icnVsSprite[38 - 32], { 3, 0 }, { 5, 0 }, 12 );
            SetPixel( icnVsSprite[38 - 32], 6, 0, 25 );
            DrawLine( icnVsSprite[38 - 32], { 1, 1 }, { 1, 3 }, 16 );
            DrawLine( icnVsSprite[38 - 32], { 2, 1 }, { 2, 3 }, 10 );
            DrawLine( icnVsSprite[38 - 32], { 3, 1 }, { 4, 1 }, 21 );
            DrawLine( icnVsSprite[38 - 32], { 5, 1 }, { 5, 2 }, 16 );
            DrawLine( icnVsSprite[38 - 32], { 6, 1 }, { 6, 2 }, 10 );
            DrawLine( icnVsSprite[38 - 32], { 3, 3 }, { 3, 5 }, 10 );
            SetPixel( icnVsSprite[38 - 32], 5, 3, 10 );
            SetPixel( icnVsSprite[38 - 32], 6, 3, 21 );
            DrawLine( icnVsSprite[38 - 32], { 2, 4 }, { 1, 5 }, 21 );
            DrawLine( icnVsSprite[38 - 32], { 4, 4 }, { 7, 7 }, 10 );
            SetPixel( icnVsSprite[38 - 32], 5, 4, 21 );
            SetPixel( icnVsSprite[38 - 32], 8, 4, 16 );
            DrawLine( icnVsSprite[38 - 32], { 9, 4 }, { 10, 4 }, 10 );
            DrawLine( icnVsSprite[38 - 32], { 2, 5 }, { 2, 7 }, 10 );
            SetPixel( icnVsSprite[38 - 32], 4, 5, 16 );
            SetPixel( icnVsSprite[38 - 32], 6, 5, 21 );
            SetPixel( icnVsSprite[38 - 32], 7, 5, 16 );
            SetPixel( icnVsSprite[38 - 32], 8, 5, 10 );
            DrawLine( icnVsSprite[38 - 32], { 9, 5 }, { 10, 5 }, 21 );
            DrawLine( icnVsSprite[38 - 32], { 1, 6 }, { 1, 7 }, 16 );
            SetPixel( icnVsSprite[38 - 32], 5, 6, 16 );
            SetPixel( icnVsSprite[38 - 32], 7, 6, 10 );
            SetPixel( icnVsSprite[38 - 32], 6, 7, 16 );
            SetPixel( icnVsSprite[38 - 32], 8, 7, 21 );
            SetPixel( icnVsSprite[38 - 32], 1, 8, 18 );
            SetPixel( icnVsSprite[38 - 32], 2, 8, 12 );
            DrawLine( icnVsSprite[38 - 32], { 3, 8 }, { 5, 8 }, 23 );
            SetPixel( icnVsSprite[38 - 32], 6, 8, 18 );
            DrawLine( icnVsSprite[38 - 32], { 7, 8 }, { 8, 8 }, 12 );
            SetPixel( icnVsSprite[38 - 32], 9, 8, 23 );
            SetPixel( icnVsSprite[38 - 32], 1, 9, 25 );
            SetPixel( icnVsSprite[38 - 32], 2, 9, 20 );
            DrawLine( icnVsSprite[38 - 32], { 3, 9 }, { 6, 9 }, 14 );
            SetPixel( icnVsSprite[38 - 32], 7, 9, 25 );
            SetPixel( icnVsSprite[38 - 32], 8, 9, 20 );
            SetPixel( icnVsSprite[38 - 32], 9, 9, 14 );
            updateNormalFontLetterShadow( icnVsSprite[38 - 32] );

            // * (asterisk).
            icnVsSprite[42 - 32].resize( 12, 15 );
            icnVsSprite[42 - 32].reset();
            icnVsSprite[42 - 32].setPosition( 0, 0 );
            SetPixel( icnVsSprite[42 - 32], 6, 0, 16 );
            SetPixel( icnVsSprite[42 - 32], 7, 0, 21 );
            DrawLine( icnVsSprite[42 - 32], { 6, 1 }, { 6, 11 }, 10 );
            SetPixel( icnVsSprite[42 - 32], 7, 1, 19 );
            DrawLine( icnVsSprite[42 - 32], { 1, 2 }, { 2, 2 }, 12 );
            SetPixel( icnVsSprite[42 - 32], 7, 2, 18 );
            SetPixel( icnVsSprite[42 - 32], 10, 2, 18 );
            SetPixel( icnVsSprite[42 - 32], 11, 2, 12 );
            DrawLine( icnVsSprite[42 - 32], { 1, 3 }, { 4, 6 }, 21 );
            DrawLine( icnVsSprite[42 - 32], { 2, 3 }, { 4, 5 }, 16 );
            DrawLine( icnVsSprite[42 - 32], { 3, 3 }, { 8, 8 }, 10 );
            DrawLine( icnVsSprite[42 - 32], { 7, 3 }, { 7, 4 }, 16 );
            DrawLine( icnVsSprite[42 - 32], { 9, 3 }, { 8, 4 }, 16 );
            DrawLine( icnVsSprite[42 - 32], { 10, 3 }, { 7, 6 }, 10 );
            DrawLine( icnVsSprite[42 - 32], { 11, 3 }, { 8, 6 }, 21 );
            SetPixel( icnVsSprite[42 - 32], 7, 5, 10 );
            SetPixel( icnVsSprite[42 - 32], 5, 6, 10 );
            DrawLine( icnVsSprite[42 - 32], { 4, 7 }, { 2, 9 }, 16 );
            DrawLine( icnVsSprite[42 - 32], { 5, 7 }, { 3, 9 }, 10 );
            DrawLine( icnVsSprite[42 - 32], { 8, 7 }, { 9, 8 }, 16 );
            DrawLine( icnVsSprite[42 - 32], { 5, 8 }, { 4, 9 }, 21 );
            DrawLine( icnVsSprite[42 - 32], { 7, 8 }, { 7, 9 }, 16 );
            SetPixel( icnVsSprite[42 - 32], 8, 9, 23 );
            SetPixel( icnVsSprite[42 - 32], 9, 9, 12 );
            SetPixel( icnVsSprite[42 - 32], 10, 9, 18 );
            SetPixel( icnVsSprite[42 - 32], 11, 9, 23 );
            DrawLine( icnVsSprite[42 - 32], { 1, 10 }, { 2, 10 }, 14 );
            SetPixel( icnVsSprite[42 - 32], 3, 10, 25 );
            SetPixel( icnVsSprite[42 - 32], 7, 10, 18 );
            SetPixel( icnVsSprite[42 - 32], 9, 10, 25 );
            DrawLine( icnVsSprite[42 - 32], { 10, 10 }, { 11, 10 }, 14 );
            SetPixel( icnVsSprite[42 - 32], 7, 11, 19 );
            SetPixel( icnVsSprite[42 - 32], 6, 12, 16 );
            SetPixel( icnVsSprite[42 - 32], 7, 12, 21 );
            updateNormalFontLetterShadow( icnVsSprite[42 - 32] );

            // Put dots instead of the other replaced ASCII characters like it is done in the original English assets.
            // TODO: make ASCII character's sprites for the printable characters instead of dots for all languages.
            const Sprite & dotSprite = icnVsSprite[46 - 32];
            // # (number sign).
            icnVsSprite[35 - 32] = dotSprite;
            // < (less than).
            icnVsSprite[60 - 32] = dotSprite;
            // > (greater than).
            icnVsSprite[62 - 32] = dotSprite;
            // @ (at sign).
            icnVsSprite[64 - 32] = dotSprite;
            // ^ (caret - circumflex).
            icnVsSprite[94 - 32] = dotSprite;
            // ` (grave accent).
            icnVsSprite[96 - 32] = dotSprite;
            // { (opening brace).
            icnVsSprite[123 - 32] = dotSprite;
            // } (closing brace).
            icnVsSprite[125 - 32] = dotSprite;
            // ~ (tilde).
            icnVsSprite[126 - 32] = dotSprite;
            // DEL (delete).
            icnVsSprite[127 - 32] = dotSprite;
        }
    }

    void modifyBaseSmallFont( std::vector<Sprite> & icnVsSprite )
    {
        if ( icnVsSprite.size() < 96 ) {
            return;
        }

        // $ (dollar sign). Fix position.
        icnVsSprite[36 - 32].setPosition( 0, -1 );

        // Remove white line from % symbol
        FillTransform( icnVsSprite[37 - 32], 3, 0, 4, 1, 1 );
        FillTransform( icnVsSprite[37 - 32], 4, 1, 2, 1, 1 );
        updateNormalFontLetterShadow( icnVsSprite[37 - 32] );

        // Add the '*' (asterisk) character.
        icnVsSprite[42 - 32].resize( 6, 8 );
        icnVsSprite[42 - 32].reset();
        icnVsSprite[42 - 32].setPosition( 0, 0 );
        DrawLine( icnVsSprite[42 - 32], { 3, 0 }, { 3, 6 }, 10 );
        DrawLine( icnVsSprite[42 - 32], { 1, 1 }, { 5, 5 }, 10 );
        DrawLine( icnVsSprite[42 - 32], { 5, 1 }, { 1, 5 }, 10 );
        updateSmallFontLetterShadow( icnVsSprite[42 - 32] );

        // Add the '\' character.
        icnVsSprite[92 - 32].resize( 5, 9 );
        icnVsSprite[92 - 32].reset();
        Copy( icnVsSprite[47 - 32], 4, 0, icnVsSprite[92 - 32], 1, 0, 1, 2 );
        Copy( icnVsSprite[47 - 32], 4, 0, icnVsSprite[92 - 32], 2, 2, 1, 2 );
        Copy( icnVsSprite[47 - 32], 4, 0, icnVsSprite[92 - 32], 3, 4, 1, 2 );
        Copy( icnVsSprite[47 - 32], 4, 0, icnVsSprite[92 - 32], 4, 6, 1, 2 );
        icnVsSprite[92 - 32].setPosition( icnVsSprite[47 - 32].x(), icnVsSprite[47 - 32].y() );
        updateSmallFontLetterShadow( icnVsSprite[92 - 32] );

        // Proper lowercase k.
        icnVsSprite[107 - 32].resize( 6, 8 );
        icnVsSprite[107 - 32].reset();
        Copy( icnVsSprite[108 - 32], 1, 0, icnVsSprite[107 - 32], 1, 0, 2, 7 );
        Copy( icnVsSprite[108 - 32], 1, 0, icnVsSprite[107 - 32], 1, 6, 1, 1 );
        Copy( icnVsSprite[88 - 32], 6, 0, icnVsSprite[107 - 32], 3, 2, 3, 3 );
        Copy( icnVsSprite[97 - 32], 2, icnVsSprite[97 - 32].height() - 2, icnVsSprite[107 - 32], 5, 6, 2, 1 );
        Copy( icnVsSprite[97 - 32], 2, 0, icnVsSprite[107 - 32], 4, 5, 1, 1 );
        icnVsSprite[107 - 32].setPosition( icnVsSprite[107 - 32].x(), icnVsSprite[107 - 32].y() );
        updateSmallFontLetterShadow( icnVsSprite[107 - 32] );

        // Restore the original ASCII characters replaced in French assets with CP1252 symbols.
        if ( getResourceLanguage() == SupportedLanguage::French ) {
            // $ (dollar sign).
            icnVsSprite[36 - 32].resize( 6, 10 );
            icnVsSprite[36 - 32].reset();
            DrawLine( icnVsSprite[36 - 32], { 3, 0 }, { 3, 8 }, 10 );
            SetPixel( icnVsSprite[36 - 32], 2, 1, 10 );
            SetPixel( icnVsSprite[36 - 32], 4, 1, 10 );
            DrawLine( icnVsSprite[36 - 32], { 1, 2 }, { 1, 3 }, 10 );
            SetPixel( icnVsSprite[36 - 32], 5, 2, 10 );
            SetPixel( icnVsSprite[36 - 32], 2, 4, 10 );
            SetPixel( icnVsSprite[36 - 32], 4, 4, 10 );
            DrawLine( icnVsSprite[36 - 32], { 5, 5 }, { 5, 6 }, 10 );
            SetPixel( icnVsSprite[36 - 32], 1, 6, 10 );
            SetPixel( icnVsSprite[36 - 32], 2, 7, 10 );
            SetPixel( icnVsSprite[36 - 32], 4, 7, 10 );
            updateSmallFontLetterShadow( icnVsSprite[36 - 32] );

            // & (ampersand).
            icnVsSprite[38 - 32].resize( 7, 8 );
            icnVsSprite[38 - 32].reset();
            icnVsSprite[38 - 32].setPosition( 0, 0 );
            SetPixel( icnVsSprite[38 - 32], 3, 0, 10 );
            DrawLine( icnVsSprite[38 - 32], { 2, 1 }, { 2, 2 }, 10 );
            DrawLine( icnVsSprite[38 - 32], { 4, 1 }, { 4, 2 }, 10 );
            SetPixel( icnVsSprite[38 - 32], 3, 3, 10 );
            SetPixel( icnVsSprite[38 - 32], 2, 4, 10 );
            SetPixel( icnVsSprite[38 - 32], 4, 4, 10 );
            SetPixel( icnVsSprite[38 - 32], 6, 4, 10 );
            SetPixel( icnVsSprite[38 - 32], 1, 5, 10 );
            SetPixel( icnVsSprite[38 - 32], 5, 5, 10 );
            DrawLine( icnVsSprite[38 - 32], { 2, 6 }, { 4, 6 }, 10 );
            SetPixel( icnVsSprite[38 - 32], 6, 6, 10 );
            updateSmallFontLetterShadow( icnVsSprite[38 - 32] );

            // Put dots instead of the other replaced ASCII characters like it is done in the original English assets.
            // TODO: make ASCII character's sprites for the printable characters instead of dots for all languages.
            const Sprite & dotSprite = icnVsSprite[46 - 32];
            // # (number sign).
            icnVsSprite[35 - 32] = dotSprite;
            // < (less than).
            icnVsSprite[60 - 32] = dotSprite;
            // > (greater than).
            icnVsSprite[62 - 32] = dotSprite;
            // @ (at sign).
            icnVsSprite[64 - 32] = dotSprite;
            // ^ (caret - circumflex).
            icnVsSprite[94 - 32] = dotSprite;
            // ` (grave accent).
            icnVsSprite[96 - 32] = dotSprite;
            // { (opening brace).
            icnVsSprite[123 - 32] = dotSprite;
            // } (closing brace).
            icnVsSprite[125 - 32] = dotSprite;
            // ~ (tilde).
            icnVsSprite[126 - 32] = dotSprite;
            // DEL (delete).
            icnVsSprite[127 - 32] = dotSprite;
        }
    }

    void applyFontVerticalGradient( Image & image, const uint8_t insideColor, const uint8_t outsideColor )
    {
        assert( !image.singleLayer() );

        if ( image.width() < 2 || image.height() < 2 ) {
            return;
        }

        const int32_t height = image.height();
        const int32_t width = image.width();

        uint8_t * imageY = image.image();
        uint8_t * transformY = image.transform();

        const int32_t centerY = std::max<int32_t>( 1, ( height / 2 ) - height % 2 );
        const uint8_t dColor = outsideColor - insideColor;

        for ( int32_t row = 0; row < height; ++row, imageY += width, transformY += width ) {
            const int32_t heightScale = ( dColor * std::abs( centerY - row ) ) / centerY;
            const uint8_t color = static_cast<uint8_t>( std::abs( insideColor + heightScale ) );

            uint8_t * imageX = imageY;
            const uint8_t * imageXEnd = imageX + width;
            uint8_t * transformX = transformY;

            for ( ; imageX != imageXEnd; ++imageX, ++transformX ) {
                if ( *transformX == 0 ) {
                    // 21 is the pixel limit of shadows in Base white Font
                    if ( *imageX < 21 ) {
                        *imageX = color;
                    }
                    else {
                        *transformX = 1;
                    }
                }
            }
        }
    }

    void fixFrenchCharactersForMP2Map( std::string & str )
    {
        for ( char & c : str ) {
            switch ( c ) {
            case 9:
                // Horizontal tab. Used for lowercase i with circumflex.
                c = static_cast<char>( 238 );
                break;
            case 35:
                // Number sign (#). Used for lowercase o with circumflex.
                c = static_cast<char>( 244 );
                break;
            case 36:
                // Dollar ($). Used for lowercase u with circumflex.
                c = static_cast<char>( 251 );
                break;
            case 38:
                // Ampersand (&). Used for lowercase u with grave accent.
                c = static_cast<char>( 249 );
                break;
            case 42:
                // Asterisk (*). Used for lowercase a with circumflex.
                c = static_cast<char>( 226 );
                break;
            case 60:
                // Less sign (<). Used for lowercase i with diaeresis.
                c = static_cast<char>( 239 );
                break;
            case 62:
                // Greater sign (>). Used for lowercase i with circumflex.
                c = static_cast<char>( 238 );
                break;
            case 64:
                // At sign (@). Used for lowercase a with grave accent.
                c = static_cast<char>( 224 );
                break;
            case 92:
                // Backslash (\). Used in some maps as the full stop '.'.
                c = '.';
                break;
            case 94:
                // Caret - circumflex (^). Used for lowercase c with cedilla.
                c = static_cast<char>( 231 );
                break;
            case 96:
                // Grave accent (`). Used for lowercase e with grave accent.
                c = static_cast<char>( 232 );
                break;
            case 123:
                // Opening brace ({). Used for lowercase i with diaeresis.
                c = static_cast<char>( 239 );
                break;
            case 124:
                // Vertical bar (|). Used for lowercase e with circumflex.
                c = static_cast<char>( 234 );
                break;
            case 125:
                // Closing brace (}). Used for lowercase i with diaeresis.
                c = static_cast<char>( 239 );
                break;
            case 126:
                // Tilde (~). Used for lowercase e with acute.
                c = static_cast<char>( 233 );
                break;
            case 127:
                // Delete (DEL). Used for lowercase i with circumflex.
                c = static_cast<char>( 238 );
                break;
            default:
                break;
            }
        }
    }
}
